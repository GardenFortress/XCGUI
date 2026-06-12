// 这里只 include 自己的 .h, module_base.h / module_xcgui.h / module_xcgui_class.h
// 由 .h 中的 //@依赖 链路自动展开, 不要再 #include 一次, 否则会和 IDE 的 PCH 冲突.

#include "module_xcgui_editdw.h"

// dwrite_2.h 提供 IDWriteFactory2 / IDWriteColorGlyphRunEnumerator / DWRITE_COLOR_GLYPH_RUN
// (Win8.1+ SDK 自带). 自定义文本渲染器要解码彩色字体 (emoji), 必须用它.
// 运行时若 QI IDWriteFactory2 失败 (老 Win7 D2D1.0), 自动降级走普通 DrawGlyphRun, 不崩.
#include <dwrite_2.h>

// shellapi.h: HDROP / DragQueryFileW / DragFinish, XE_DROPFILES 事件处理用.
// 注意 hDropInfo 的所有权: XCGUI 把 WM_DROPFILES 转发为 XE_DROPFILES, 我们*不要* 调
// DragFinish - XCGUI 在事件分发完毕后自己处理释放, 重复 DragFinish 会触发资源错误.
#include <shellapi.h>

// gdiplus: Bitmap / Graphics. 用于 InsertImageThumb 的 *预缩放*: 大图在 GDI+ 里一次性
// bicubic 压成缩略尺寸再交给 HIMAGE. 否则 XCGUI 每帧按 fixed_ratio 对全尺寸原图实时
// 重采样, 4000x3000→200x150 每帧几十~几百 ms, GDI+ 模式直接卡死.
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

//===================================================================
//  内嵌对象 IDWriteInlineObject 实现
//===================================================================
// 仅提供尺寸 (DirectWrite 据此为 \uFFFC 占位字符预留绘制框) 和空 Draw (实际位置由
// CXEditDW::PositionInlineObjects 在每次 OnPaintImpl 时把 HXCGUI 子元素 SetPosition).
// 这样 XCGUI 默认子元素绘制管线接管对象本身的渲染, 与文本同行布局.
class _CXEditDW_InlineObj : public IDWriteInlineObject{
	LONG  m_ref;
	int   m_widthPhys;   // 物理像素 (与 layout 同坐标系)
	int   m_heightPhys;
public:
	_CXEditDW_InlineObj(int wp, int hp) : m_ref(1), m_widthPhys(wp), m_heightPhys(hp) {}
	virtual ~_CXEditDW_InlineObj(){}

	HRESULT STDMETHODCALLTYPE Draw(void*, IDWriteTextRenderer*, FLOAT, FLOAT, BOOL, BOOL, IUnknown*) override {
		return S_OK; // no-op: 真正定位走 PositionInlineObjects
	}
	HRESULT STDMETHODCALLTYPE GetMetrics(DWRITE_INLINE_OBJECT_METRICS* m) override {
		m->width = (FLOAT)m_widthPhys;
		m->height = (FLOAT)m_heightPhys;
		// baseline = height: 对象底部与文本基线对齐 (常见 inline 图标/按钮排版).
		m->baseline = (FLOAT)m_heightPhys;
		m->supportsSideways = FALSE;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetOverhangMetrics(DWRITE_OVERHANG_METRICS* o) override {
		o->left = o->right = o->top = o->bottom = 0.0f;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetBreakConditions(DWRITE_BREAK_CONDITION* before, DWRITE_BREAK_CONDITION* after) override {
		*before = DWRITE_BREAK_CONDITION_NEUTRAL;
		*after  = DWRITE_BREAK_CONDITION_NEUTRAL;
		return S_OK;
	}
	// IUnknown
	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
	ULONG STDMETHODCALLTYPE Release() override {
		ULONG r = InterlockedDecrement(&m_ref);
		if (r == 0) delete this;
		return r;
	}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppv) override {
		if (iid == __uuidof(IUnknown) || iid == __uuidof(IDWriteInlineObject)){
			*ppv = static_cast<IDWriteInlineObject*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}
};

//===================================================================
//  共用颜色 effect (D2D + GDI 同一个)
//===================================================================
// 把 IDWriteTextLayout::SetDrawingEffect 的 IUnknown 槽位用来携带 COLORREF (XCGUI 风格的
// 0xRRGGBB, 上层做色域换算). 关键点: effect 是纯数据, 不绑 RT/HDC, 所以可以在 layout
// 重建时 *一次性* set 进去, 跨多帧反复用 layout - 不会因 RT 重建而失效.
//
// D2D 路径: 由 _CXEditDW_TextRenderer 在 DrawGlyphRun 时 QI 取色, 按色查 / 建 brush
// (renderer 内部缓存, 仅本次 Draw 期间有效).
// GDI 路径: 由 _CXEditDW_GdiRenderer 同样 QI 取色, 转 GDI BGR 后传给 BitmapRT.
// 双路径共用同一 IID, 同一 effect class - 减少重复 + 避免 "D2D 误用 GDI effect" 的脆性.
//
// 私有 IID: 我们这个进程内部唯一标识, 不发布给外部.
// {7A6B5F2D-3C4E-4D1E-9F12-2C8E8E1C2A47}
static const GUID IID_CXEditDW_ColorEffect_local =
	{ 0x7a6b5f2d, 0x3c4e, 0x4d1e, { 0x9f, 0x12, 0x2c, 0x8e, 0x8e, 0x1c, 0x2a, 0x47 } };

class _CXEditDW_ColorEffect : public IUnknown{
	LONG              m_ref;
	COLORREF          m_color;     // RGB; alpha 部分不传, GDI 文本不支持半透明叠加
	IDWriteFontFace*  m_pFontFace; // 不拥有; 样式字体 DrawGlyphRun 兜底
public:
	_CXEditDW_ColorEffect(COLORREF c, IDWriteFontFace* pFace = NULL)
		: m_ref(1), m_color(c), m_pFontFace(pFace){}
	virtual ~_CXEditDW_ColorEffect(){}
	COLORREF GetColor() const { return m_color; }
	IDWriteFontFace* GetFontFace() const { return m_pFontFace; }

	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
	ULONG STDMETHODCALLTYPE Release() override {
		ULONG r = InterlockedDecrement(&m_ref);
		if (r == 0) delete this;
		return r;
	}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppv) override {
		if (iid == IID_CXEditDW_ColorEffect_local || iid == __uuidof(IUnknown)){
			*ppv = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}
};

// DrawGlyphRun 兜底: 仅换 fontFace 无法修正 glyph 索引.
struct _EditDW_GlyphRunRemap{
	std::vector<UINT16>              indices;
	std::vector<FLOAT>               advances;
	std::vector<DWRITE_GLYPH_OFFSET> offsets;
	DWRITE_GLYPH_RUN                 run;
};

static bool _EditDW_RemapGlyphRunToFace(
	IDWriteFontFace* pFace,
	DWRITE_GLYPH_RUN const* src,
	DWRITE_GLYPH_RUN_DESCRIPTION const* desc,
	_EditDW_GlyphRunRemap& out)
{
	if (!pFace || !src || src->glyphCount == 0 || !src->glyphIndices) return false;
	if (!desc || !desc->string || !desc->clusterMap || desc->stringLength == 0) return false;

	UINT16 firstG = src->glyphIndices[0];
	UINT16 lastG  = src->glyphIndices[src->glyphCount - 1];
	UINT32 cStart = desc->stringLength;
	UINT32 cEnd   = 0;
	bool found = false;
	for (UINT32 ci = 0; ci < desc->stringLength; ++ci){
		UINT16 g = desc->clusterMap[ci];
		if (g >= firstG && g <= lastG){
			if (ci < cStart) cStart = ci;
			if (ci + 1 > cEnd) cEnd = ci + 1;
			found = true;
		}
	}
	if (!found || cEnd <= cStart) return false;

	std::vector<UINT32> codepoints;
	codepoints.reserve(cEnd - cStart);
	for (UINT32 i = cStart; i < cEnd; ){
		wchar_t w = desc->string[i];
		if (w >= 0xD800 && w <= 0xDBFF && i + 1 < cEnd){
			wchar_t w2 = desc->string[i + 1];
			if (w2 >= 0xDC00 && w2 <= 0xDFFF){
				codepoints.push_back(0x10000u + (((UINT32)(w - 0xD800) << 10) | (UINT32)(w2 - 0xDC00)));
				i += 2;
				continue;
			}
		}
		codepoints.push_back((UINT32)w);
		++i;
	}
	UINT32 nGlyphs = (UINT32)codepoints.size();
	if (nGlyphs == 0) return false;

	out.indices.resize(nGlyphs);
	if (FAILED(pFace->GetGlyphIndices(codepoints.data(), nGlyphs, out.indices.data())))
		return false;

	DWRITE_FONT_METRICS fm;
	pFace->GetMetrics(&fm);
	if (fm.designUnitsPerEm <= 0) return false;

	std::vector<DWRITE_GLYPH_METRICS> metrics(nGlyphs);
	if (FAILED(pFace->GetDesignGlyphMetrics(out.indices.data(), nGlyphs, metrics.data(), FALSE)))
		return false;

	float emScale = src->fontEmSize / (float)fm.designUnitsPerEm;
	out.advances.resize(nGlyphs);
	out.offsets.assign(nGlyphs, DWRITE_GLYPH_OFFSET{ 0.0f, 0.0f });
	for (UINT32 i = 0; i < nGlyphs; ++i)
		out.advances[i] = metrics[i].advanceWidth * emScale;

	out.run = *src;
	out.run.fontFace      = pFace;
	out.run.glyphCount    = nGlyphs;
	out.run.glyphIndices  = out.indices.data();
	out.run.glyphAdvances = out.advances.data();
	out.run.glyphOffsets  = out.offsets.data();
	out.run.isSideways    = FALSE;
	out.run.bidiLevel     = 0;
	return true;
}

static IDWriteFontFace* _EditDW_PickRemapFace(
	IUnknown* drawingEffect, IDWriteFontFace* pDefaultFace)
{
	if (drawingEffect){
		_CXEditDW_ColorEffect* eff = NULL;
		if (SUCCEEDED(drawingEffect->QueryInterface(IID_CXEditDW_ColorEffect_local, (void**)&eff)) && eff){
			IDWriteFontFace* p = eff->GetFontFace();
			eff->Release();
			if (p) return p;
		}
	}
	return pDefaultFace;
}

static const DWRITE_GLYPH_RUN* _EditDW_PickRemappedRun(
	IDWriteFontFace* pFace,
	DWRITE_GLYPH_RUN const* src,
	DWRITE_GLYPH_RUN_DESCRIPTION const* desc,
	bool remap,
	_EditDW_GlyphRunRemap& buf,
	DWRITE_GLYPH_RUN& faceOnlyCopy)
{
	if (!pFace || !remap) return src;
	if (_EditDW_RemapGlyphRunToFace(pFace, src, desc, buf))
		return &buf.run;
	faceOnlyCopy = *src;
	faceOnlyCopy.fontFace = pFace;
	return &faceOnlyCopy;
}

static void _EditDW_ReleaseFontBinding(_EditDW_FontBinding& b){
	if (b.pFace){ b.pFace->Release(); b.pFace = NULL; }
	if (b.pCollection){ b.pCollection->Release(); b.pCollection = NULL; }
	b.hFontX = NULL;
	ZeroMemory(&b.logFont, sizeof(b.logFont));
	b.valid = false;
}

//===================================================================
//  自定义文本渲染器: 让 SetDrawingEffect(ID2D1Brush) 100% 生效 + 保留彩色 emoji
//===================================================================
// 背景: ID2D1RenderTarget::DrawTextLayout 在某些 D2D 版本/驱动 (含 D2D 1.1 Win8.1) 上
// 会静默忽略 IDWriteTextLayout::SetDrawingEffect 设的 brush, 全部用 defaultForegroundBrush.
// 见 https://github.com/sharpdx/SharpDX/issues/440 . 唯一稳定方案是改走
// IDWriteTextLayout::Draw + 自定义 IDWriteTextRenderer, 自己处理 drawingEffect.
//
// 同时为不丢彩色 emoji, 在 DrawGlyphRun 里先尝试 IDWriteFactory2::TranslateColorGlyphRun
// (Win8.1+) 把彩色字形拆成多层 ID2D1 brush + glyph run, 逐层 DrawGlyphRun. 拿不到
// IDWriteFactory2 (老 Win7) 直接走普通 DrawGlyphRun, 仅丢彩色但文本可见.
class _CXEditDW_TextRenderer : public IDWriteTextRenderer{
	LONG                 m_ref;
	ID2D1RenderTarget*   m_rt;            // 不持有引用, 仅借用本帧 RT (调用方保证活性)
	ID2D1Brush*          m_defaultBrush;  // 同上, 仅本帧用
	IDWriteFactory2*     m_pDW2;          // 用于 TranslateColorGlyphRun, 可能 NULL
	IDWriteFontFace*     m_pDefaultFace;  // 不持有; 默认文本 DrawGlyphRun 兜底

	// 颜色 -> brush 缓存. 用小 vector 顺序查 - 典型文档 1~4 种文本色, 线性查更省内存比
	// std::unordered_map 还快. brush 由 renderer 持有 (ref), 析构时统一 Release.
	struct _ColorBrush { COLORREF c; ID2D1Brush* p; };
	mutable std::vector<_ColorBrush> m_brushCache;

	// 拿 / 建按色 brush. 命中复用, 缺失从 RT 建后入缓存. 返回值已 AddRef, 调用方 Release.
	ID2D1Brush* GetOrCreateColorBrush(COLORREF col) const{
		for (size_t i = 0; i < m_brushCache.size(); ++i){
			if (m_brushCache[i].c == col){
				m_brushCache[i].p->AddRef();
				return m_brushCache[i].p;
			}
		}
		if (!m_rt) return NULL;
		BYTE a = GetAValue(col);
		if (a == 0) a = 0xFF;
		D2D1_COLOR_F c = D2D1::ColorF(
			GetRValue(col) / 255.0f, GetGValue(col) / 255.0f,
			GetBValue(col) / 255.0f, a / 255.0f);
		ID2D1SolidColorBrush* pNew = NULL;
		HRESULT hr = m_rt->CreateSolidColorBrush(c, &pNew);
		if (FAILED(hr) || !pNew) return NULL;
		_ColorBrush cb; cb.c = col; cb.p = pNew;
		m_brushCache.push_back(cb);
		pNew->AddRef(); // 一份给缓存 (析构 Release), 一份返给调用方
		return pNew;
	}

	// drawingEffect 优先按我们的 _CXEditDW_ColorEffect QI 取色 -> 缓存 brush. 兼容老路径
	// 也认 ID2D1Brush (如果别处真有人挂了 brush). 都不命中回落默认色. 调用方需 Release.
	ID2D1Brush* PickBrush(IUnknown* drawingEffect) const{
		if (drawingEffect){
			_CXEditDW_ColorEffect* eff = NULL;
			if (SUCCEEDED(drawingEffect->QueryInterface(IID_CXEditDW_ColorEffect_local, (void**)&eff)) && eff){
				COLORREF c = eff->GetColor();
				eff->Release();
				ID2D1Brush* pb = GetOrCreateColorBrush(c);
				if (pb) return pb;
			}
			ID2D1Brush* p = NULL;
			if (SUCCEEDED(drawingEffect->QueryInterface(__uuidof(ID2D1Brush), (void**)&p)) && p){
				return p; // 已被 QI AddRef, 调用方 Release
			}
		}
		if (m_defaultBrush){
			m_defaultBrush->AddRef();
			return m_defaultBrush;
		}
		return NULL;
	}
public:
	_CXEditDW_TextRenderer(ID2D1RenderTarget* rt, ID2D1Brush* defaultBrush, IDWriteFactory2* pDW2,
	                       IDWriteFontFace* pDefaultFace = NULL)
		: m_ref(1), m_rt(rt), m_defaultBrush(defaultBrush), m_pDW2(pDW2), m_pDefaultFace(pDefaultFace) {}
	virtual ~_CXEditDW_TextRenderer() {
		for (size_t i = 0; i < m_brushCache.size(); ++i){
			if (m_brushCache[i].p) m_brushCache[i].p->Release();
		}
	}

	// ===== IDWriteTextRenderer =====
	HRESULT STDMETHODCALLTYPE DrawGlyphRun(
		void*                            /*clientCtx*/,
		FLOAT                            baselineOriginX,
		FLOAT                            baselineOriginY,
		DWRITE_MEASURING_MODE            measuringMode,
		DWRITE_GLYPH_RUN const*          glyphRun,
		DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
		IUnknown*                        clientDrawingEffect) override
	{
		if (!m_rt || !glyphRun) return S_OK;
		ID2D1Brush* pBrush = PickBrush(clientDrawingEffect);
		if (!pBrush) return S_OK;

		IDWriteFontFace* pRemapFace = _EditDW_PickRemapFace(clientDrawingEffect, m_pDefaultFace);
		_EditDW_GlyphRunRemap remapBuf;
		DWRITE_GLYPH_RUN faceOnlyCopy;
		const DWRITE_GLYPH_RUN* runToDraw = _EditDW_PickRemappedRun(
			pRemapFace, glyphRun, glyphRunDescription,
			(pRemapFace != NULL),
			remapBuf, faceOnlyCopy);

		bool drawn = false;
		// 仅在 IDWriteFactory2 + 字体本身有彩色字形 (DWRITE_E_NOCOLOR=0x88985002) 时才走多层路径.
		if (m_pDW2){
			IDWriteColorGlyphRunEnumerator* pEnum = NULL;
			DWRITE_MATRIX dwm = { 1, 0, 0, 1, 0, 0 };
			HRESULT hr = m_pDW2->TranslateColorGlyphRun(
				baselineOriginX, baselineOriginY,
				runToDraw, glyphRunDescription,
				measuringMode, &dwm, 0, &pEnum);
			if (SUCCEEDED(hr) && pEnum){
				BOOL hasRun = FALSE;
				while (SUCCEEDED(pEnum->MoveNext(&hasRun)) && hasRun){
					DWRITE_COLOR_GLYPH_RUN const* colorRun = NULL;
					if (FAILED(pEnum->GetCurrentRun(&colorRun)) || !colorRun) break;
					ID2D1Brush* pLayerBrush = NULL;
					ID2D1SolidColorBrush* pTmp = NULL;
					// paletteIndex == 0xFFFF: 该层用 *前景刷* (单色字符, 与 drawingEffect 同色),
					// 其他: 用调色板色 (彩色字形多层叠加).
					if (colorRun->paletteIndex == 0xFFFF){
						pLayerBrush = pBrush;
						pBrush->AddRef();
					} else {
						m_rt->CreateSolidColorBrush(colorRun->runColor, &pTmp);
						pLayerBrush = pTmp;
					}
					if (pLayerBrush){
						// DWRITE_COLOR_GLYPH_RUN 不带 measuringMode (只 DWRITE_COLOR_GLYPH_RUN1 才有);
						// 直接复用外层 DrawGlyphRun 收到的 measuringMode 即可.
						m_rt->DrawGlyphRun(
							D2D1::Point2F(colorRun->baselineOriginX, colorRun->baselineOriginY),
							&colorRun->glyphRun,
							pLayerBrush,
							measuringMode);
						pLayerBrush->Release();
					}
				}
				drawn = true;
				pEnum->Release();
			}
			// hr==DWRITE_E_NOCOLOR 时 SUCCEEDED 也会返 false, 落 fallback 路径.
		}

		if (!drawn){
			m_rt->DrawGlyphRun(
				D2D1::Point2F(baselineOriginX, baselineOriginY),
				runToDraw, pBrush, measuringMode);
		}
		pBrush->Release();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DrawUnderline(
		void*                       /*clientCtx*/,
		FLOAT                       baselineOriginX,
		FLOAT                       baselineOriginY,
		DWRITE_UNDERLINE const*     underline,
		IUnknown*                   clientDrawingEffect) override
	{
		if (!m_rt || !underline) return S_OK;
		ID2D1Brush* pBrush = PickBrush(clientDrawingEffect);
		if (!pBrush) return S_OK;
		D2D1_RECT_F r = D2D1::RectF(
			baselineOriginX,
			baselineOriginY + underline->offset,
			baselineOriginX + underline->width,
			baselineOriginY + underline->offset + underline->thickness);
		m_rt->FillRectangle(r, pBrush);
		pBrush->Release();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DrawStrikethrough(
		void*                          /*clientCtx*/,
		FLOAT                          baselineOriginX,
		FLOAT                          baselineOriginY,
		DWRITE_STRIKETHROUGH const*    strikethrough,
		IUnknown*                      clientDrawingEffect) override
	{
		if (!m_rt || !strikethrough) return S_OK;
		ID2D1Brush* pBrush = PickBrush(clientDrawingEffect);
		if (!pBrush) return S_OK;
		D2D1_RECT_F r = D2D1::RectF(
			baselineOriginX,
			baselineOriginY + strikethrough->offset,
			baselineOriginX + strikethrough->width,
			baselineOriginY + strikethrough->offset + strikethrough->thickness);
		m_rt->FillRectangle(r, pBrush);
		pBrush->Release();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DrawInlineObject(
		void*                  clientCtx,
		FLOAT                  originX,
		FLOAT                  originY,
		IDWriteInlineObject*   inlineObject,
		BOOL                   isSideways,
		BOOL                   isRightToLeft,
		IUnknown*              clientDrawingEffect) override
	{
		if (!inlineObject) return S_OK;
		// 转回 inline 对象的 Draw (我们的 _CXEditDW_InlineObj::Draw 是 no-op, 真实绘制走子元素).
		return inlineObject->Draw(clientCtx, this, originX, originY, isSideways, isRightToLeft, clientDrawingEffect);
	}

	// ===== IDWritePixelSnapping =====
	HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void* /*ctx*/, BOOL* isDisabled) override {
		*isDisabled = FALSE; // 关闭 = 启用像素对齐, 文本更锐利
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetCurrentTransform(void* /*ctx*/, DWRITE_MATRIX* transform) override {
		if (!m_rt){
			transform->m11 = 1; transform->m12 = 0;
			transform->m21 = 0; transform->m22 = 1;
			transform->dx  = 0; transform->dy  = 0;
			return S_OK;
		}
		D2D1_MATRIX_3X2_F m;
		m_rt->GetTransform(&m);
		transform->m11 = m.m11; transform->m12 = m.m12;
		transform->m21 = m.m21; transform->m22 = m.m22;
		transform->dx  = m.dx;  transform->dy  = m.dy;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void* /*ctx*/, FLOAT* pixelsPerDip) override {
		FLOAT dx = 96.0f, dy = 96.0f;
		if (m_rt) m_rt->GetDpi(&dx, &dy);
		*pixelsPerDip = dx / 96.0f;
		return S_OK;
	}

	// ===== IUnknown =====
	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
	ULONG STDMETHODCALLTYPE Release() override {
		ULONG r = InterlockedDecrement(&m_ref);
		if (r == 0) delete this;
		return r;
	}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppv) override {
		if (iid == __uuidof(IUnknown)
			|| iid == __uuidof(IDWriteTextRenderer)
			|| iid == __uuidof(IDWritePixelSnapping)){
			*ppv = static_cast<IDWriteTextRenderer*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}
};

// XCGUI RGBA -> GDI COLORREF (0x00BBGGRR). alpha 丢弃.
static COLORREF _ToGdiColor(COLORREF xcCol){
	return RGB(GetRValue(xcCol), GetGValue(xcCol), GetBValue(xcCol));
}

//===================================================================
//  GDI 渲染回退路径 (Win7 无 GPU VM 等 D2D 不可用环境)
//===================================================================
// IDWriteTextRenderer GDI 实现.
// DrawGlyphRun 转发到 IDWriteBitmapRenderTarget::DrawGlyphRun (输出 glyph 到 DIB).
// DrawUnderline / DrawStrikethrough 用 ::FillRect 在 DIB 上画矩形.
// DrawInlineObject 不出现 (\uFFFC inline 由 XCGUI 子元素自己画, 我们的 inline obj Draw 是 no-op).
// IsPixelSnappingDisabled = FALSE (GDI 整像素更锐).
// GetCurrentTransform = identity (BitmapRT 内部已经把坐标处理好).
// GetPixelsPerDip 从 BitmapRT 取.
class _CXEditDW_GdiRenderer : public IDWriteTextRenderer{
	LONG                       m_ref;
	IDWriteBitmapRenderTarget* m_bmpRT;          // 不持有引用
	IDWriteRenderingParams*    m_params;         // 不持有引用
	COLORREF                   m_defaultColor;   // 0xRRGGBB (Win32 RGB, 已 RGB->BGR 转过)
	IDWriteFontFace*           m_pDefaultFace;   // 不持有; 默认文本 DrawGlyphRun 兜底

	COLORREF PickColor(IUnknown* drawingEffect) const{
		if (drawingEffect){
			_CXEditDW_ColorEffect* eff = NULL;
			if (SUCCEEDED(drawingEffect->QueryInterface(IID_CXEditDW_ColorEffect_local, (void**)&eff)) && eff){
				COLORREF c = eff->GetColor();
				eff->Release();
				return _ToGdiColor(c);
			}
		}
		return m_defaultColor;
	}
public:
	_CXEditDW_GdiRenderer(IDWriteBitmapRenderTarget* bmpRT, IDWriteRenderingParams* params,
	                      COLORREF defGdiColor, IDWriteFontFace* pDefaultFace = NULL)
		: m_ref(1), m_bmpRT(bmpRT), m_params(params), m_defaultColor(defGdiColor),
		  m_pDefaultFace(pDefaultFace) {}
	virtual ~_CXEditDW_GdiRenderer(){}

	HRESULT STDMETHODCALLTYPE DrawGlyphRun(
		void*                            /*clientCtx*/,
		FLOAT                            baselineOriginX,
		FLOAT                            baselineOriginY,
		DWRITE_MEASURING_MODE            measuringMode,
		DWRITE_GLYPH_RUN const*          glyphRun,
		DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
		IUnknown*                        clientDrawingEffect) override
	{
		if (!m_bmpRT || !glyphRun) return S_OK;
		COLORREF col = PickColor(clientDrawingEffect);
		IDWriteFontFace* pRemapFace = _EditDW_PickRemapFace(clientDrawingEffect, m_pDefaultFace);
		_EditDW_GlyphRunRemap remapBuf;
		DWRITE_GLYPH_RUN faceOnlyCopy;
		const DWRITE_GLYPH_RUN* runToDraw = _EditDW_PickRemappedRun(
			pRemapFace, glyphRun, glyphRunDescription,
			(pRemapFace != NULL),
			remapBuf, faceOnlyCopy);
		// IDWriteBitmapRenderTarget::DrawGlyphRun 期望已转换好的 GDI COLORREF (BGR).
		// 不需要 IDWriteFactory2::TranslateColorGlyphRun - BitmapRT 不支持彩色 emoji.
		RECT dirty = { 0, 0, 0, 0 };
		m_bmpRT->DrawGlyphRun(
			baselineOriginX, baselineOriginY,
			measuringMode,
			runToDraw,
			m_params,
			col,
			&dirty);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DrawUnderline(
		void*                       /*clientCtx*/,
		FLOAT                       baselineOriginX,
		FLOAT                       baselineOriginY,
		DWRITE_UNDERLINE const*     underline,
		IUnknown*                   clientDrawingEffect) override
	{
		if (!m_bmpRT || !underline) return S_OK;
		HDC dc = m_bmpRT->GetMemoryDC();
		if (!dc) return S_OK;
		COLORREF col = PickColor(clientDrawingEffect);
		RECT r;
		r.left   = (LONG)baselineOriginX;
		r.top    = (LONG)(baselineOriginY + underline->offset);
		r.right  = (LONG)(baselineOriginX + underline->width);
		r.bottom = (LONG)(baselineOriginY + underline->offset + underline->thickness);
		if (r.bottom <= r.top) r.bottom = r.top + 1;
		HBRUSH br = ::CreateSolidBrush(col);
		if (br){ ::FillRect(dc, &r, br); ::DeleteObject(br); }
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DrawStrikethrough(
		void*                          /*clientCtx*/,
		FLOAT                          baselineOriginX,
		FLOAT                          baselineOriginY,
		DWRITE_STRIKETHROUGH const*    strikethrough,
		IUnknown*                      clientDrawingEffect) override
	{
		if (!m_bmpRT || !strikethrough) return S_OK;
		HDC dc = m_bmpRT->GetMemoryDC();
		if (!dc) return S_OK;
		COLORREF col = PickColor(clientDrawingEffect);
		RECT r;
		r.left   = (LONG)baselineOriginX;
		r.top    = (LONG)(baselineOriginY + strikethrough->offset);
		r.right  = (LONG)(baselineOriginX + strikethrough->width);
		r.bottom = (LONG)(baselineOriginY + strikethrough->offset + strikethrough->thickness);
		if (r.bottom <= r.top) r.bottom = r.top + 1;
		HBRUSH br = ::CreateSolidBrush(col);
		if (br){ ::FillRect(dc, &r, br); ::DeleteObject(br); }
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DrawInlineObject(
		void*                  clientCtx,
		FLOAT                  originX,
		FLOAT                  originY,
		IDWriteInlineObject*   inlineObject,
		BOOL                   isSideways,
		BOOL                   isRightToLeft,
		IUnknown*              clientDrawingEffect) override
	{
		if (!inlineObject) return S_OK;
		// _CXEditDW_InlineObj::Draw 是 no-op, inline 子元素由 XCGUI 子元素管线绘制. 同 D2D 路径.
		return inlineObject->Draw(clientCtx, this, originX, originY, isSideways, isRightToLeft, clientDrawingEffect);
	}

	HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void* /*ctx*/, BOOL* isDisabled) override {
		*isDisabled = FALSE;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetCurrentTransform(void* /*ctx*/, DWRITE_MATRIX* transform) override {
		transform->m11 = 1; transform->m12 = 0;
		transform->m21 = 0; transform->m22 = 1;
		transform->dx  = 0; transform->dy  = 0;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void* /*ctx*/, FLOAT* pixelsPerDip) override {
		FLOAT v = 1.0f;
		if (m_bmpRT) v = m_bmpRT->GetPixelsPerDip();
		if (v <= 0.0f) v = 1.0f;
		*pixelsPerDip = v;
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
	ULONG STDMETHODCALLTYPE Release() override {
		ULONG r = InterlockedDecrement(&m_ref);
		if (r == 0) delete this;
		return r;
	}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppv) override {
		if (iid == __uuidof(IUnknown)
			|| iid == __uuidof(IDWriteTextRenderer)
			|| iid == __uuidof(IDWritePixelSnapping)){
			*ppv = static_cast<IDWriteTextRenderer*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}
};

//===================================================================
//  生命周期
//===================================================================
CXEditDW::~CXEditDW(){
	ReleaseDWriteResources();
	// 兜底: 与 SetText / 元素销毁 共用同一释放路径. 若元素已先被 XE_DESTROY 触发清空过,
	// 这里再走一遍也只是空循环, ReleaseStyleSlot 内部对空句柄/已标记槽位都直接跳过.
	// (XCGUI 类设计的常规约定: 服务类析构不主动管资源 - 但本类管理的是 *自己 AddStyleEx
	//  内部 XFont_CreateEx 出来的字体*, 不释就泄漏, 所以保留兜底.)
	ReleaseAllStyleResources();
}

HELE CXEditDW::Create(int x, int y, int cx, int cy, HXCGUI hParent){
	m_hEle = XSView_Create(x, y, cx, cy, hParent);
	if (!m_hEle) return NULL;
	RefreshDpiScale();
	// XSView_* 接口一律以 *逻辑像素* 为单位 (与其他 XCGUI 布局 API 一致):
	// 行高、页高、SetTotalSize、ViewPos、ViewWidth/Height 都是逻辑. *不要乘 m_dpiScale*.
	XSView_SetLineSize(m_hEle, (int)m_fontSize, (int)(m_fontSize * 1.4f));
	XSView_EnableAutoShowScrollBar(m_hEle, TRUE);
	XEle_SetCursor(m_hEle, ::LoadCursor(NULL, IDC_IBEAM));
	XEle_EnableFocus(m_hEle, TRUE);
	// 默认 BorderSize - 作为文本与元素边的呼吸距离 (XCGUI 语义: BorderSize = 内容区收缩).
	// 与 Padding 不同 (Padding 是给子元素布局用的, *本类文本不受 Padding 影响*).
	XEle_SetBorderSize(m_hEle, 6, 4, 6, 4);
	// XE_PAINT_END: 在 XCGUI 默认绘制(BkInfo填充/边框/XCGUI内置焦点框)走完之后再调我们,
	// 让自绘文本/选区/光标叠加在背景之上. 关于三个相关 API 的实际作用范围:
	//   - SetBkInfo / AddBkFill / AddBkBorder : 走 BkInfo 路径, 我们用 RebuildBkInfo 包了一层.
	//   - XEle_EnableDrawBorder : 仅控制 *默认隐含* 边框, 不影响 BkInfo border 项. 关 BkInfo
	//     border 需用本类 EnableDrawBorderEx(FALSE) (也透传 XCGUI 内置焦点框成透明).
	//   - XEle_EnableDrawFocus  : XCGUI 内置焦点框 + 本类 caret 自绘均尊重 XEle_IsDrawFocus,
	//     FALSE 时两者都不画.
	XEle_EnableEvent_XE_PAINT_END(m_hEle, TRUE);
	// 按 m_bkColor / m_borderColor / m_focusBorderColor 装上默认 BkInfo (白底 + 灰边框 + 焦点蓝边框).
	RebuildBkInfo();
	InstallEvents();
	return m_hEle;
}

//===================================================================
//  文本
//===================================================================
void CXEditDW::SetText(const wchar_t* pString){
	// SetText 的语义: *硬重置*. 清空文本 + 已插入的图片/字体/UI对象, 撤销栈也一并清掉
	// (因为新内容覆盖之后, 老内容的插入对象将无法回滚, 不释放就会内存泄漏).
	// 与 EraseCharsRaw (只减引用, 留待 Undo 用) 是两条不同的路径.
	ReleaseAllStyleResources();
	int len = pString ? (int)wcslen(pString) : 0;
	InsertCharsRaw(0, pString ? pString : L"", len, -1);
	m_caret = (int)m_text.size();
	m_anchor = m_caret;
	// InsertCharsRaw 已走 ParaOnTextInserted 同步段结构. 不再调 InvalidateLayout (那是
	// release-all, 会破坏未受影响段的 layout 缓存). EnsureLayout 只建缺失段.
	EnsureLayout();
	RedrawSelf();
}

// 把 m_text 里的 \uFFFC (inline 对象占位字符) 过滤掉, 给外部公开 API 用. 内部计算仍走 m_text.
static std::wstring _FilterObjectReplacementChar(const std::wstring& s){
	std::wstring t;
	t.reserve(s.size());
	for (size_t i = 0; i < s.size(); ++i){
		if (s[i] != L'\xFFFC') t.push_back(s[i]);
	}
	return t;
}

CXText CXEditDW::GetText() const{
	// 公开接口 *不* 输出 \uFFFC: 用户调 GetText 是为显示 / 保存 / 序列化, 占位字符没有意义.
	// 内部 m_text 不动, 不影响撤销 / 布局 / 剪贴板的内部副本.
	return CXText(_FilterObjectReplacementChar(m_text));
}

const wchar_t* CXEditDW::GetTextTemp() const{
	// 返回 const wchar_t* 需要 stable pointer - 用 mutable 缓存承载过滤后结果.
	// 每次调用 *重新生成* 缓存, 简单可靠. 若日后高频调用可改为 dirty flag 增量.
	m_textNoOrcCache = _FilterObjectReplacementChar(m_text);
	return m_textNoOrcCache.c_str();
}

int CXEditDW::GetLength() const{
	// 与 GetText / GetTextTemp 对称: 不计 \uFFFC 占位字符 (inline 对象专用).
	// 单次扫 O(n) 计数, 不像 GetText 那样得分配 wstring; 长文本下也只是 m_text.size() 的常数倍.
	int n = (int)m_text.size();
	int orc = 0;
	for (int i = 0; i < n; ++i){
		if (m_text[i] == kObjectReplacementChar) ++orc;
	}
	return n - orc;
}

BOOL CXEditDW::IsEmpty() const{
	return m_text.empty() ? TRUE : FALSE;
}

void CXEditDW::AddText(const wchar_t* pString){
	if (!pString || !*pString) return;
	int len = (int)wcslen(pString);
	InsertCharsRaw((int)m_text.size(), pString, len, m_curStyle);
	m_caret = (int)m_text.size();
	m_anchor = m_caret;
	// InsertCharsRaw 内部已同步段结构 + 设脏. 不再调 InvalidateLayout.
	EnsureLayout();
	RedrawSelf();
}

void CXEditDW::InsertText(const wchar_t* pString){
	if (m_readOnly || !pString || !*pString) return;
	InsertTextAtCursor(pString, (int)wcslen(pString));
}

//===================================================================
//  字体
//===================================================================
void CXEditDW::SetFont(HFONTX hFont){
	m_fontNameOnly = false;
	m_hFontX = hFont;
	m_useFontHandle = (hFont != NULL);
	if (hFont){
		font_info_ fi; ZeroMemory(&fi, sizeof(fi));
		XFont_GetFontInfo(hFont, &fi);
		if (fi.name[0]) m_fontName = fi.name;
		if (fi.nSize > 0) m_fontSize = (float)fi.nSize;
	}
	InvalidateFontBinding();
	ReleaseTextFormat();
	InvalidateLayout();
	RedrawSelf();
}

void CXEditDW::SetFontName(const wchar_t* pName){
	if (!pName || !*pName) return;
	m_fontNameOnly = true;
	m_useFontHandle = false;
	m_hFontX = NULL;
	m_fontName = pName;
	InvalidateFontBinding();
	ReleaseTextFormat();
	InvalidateLayout();
	RedrawSelf();
}

void CXEditDW::SetFontSize(float pt){
	if (pt < 1.0f) pt = 1.0f;
	m_fontSize = pt;
	InvalidateFontBinding();
	InvalidateStyleFontBindings();
	ReleaseTextFormat();
	InvalidateLayout();
	RedrawSelf();
}

float CXEditDW::GetFontSize() const{
	return m_fontSize;
}

void CXEditDW::SetLineSpacing(float nPixels){
	if (nPixels < 0.0f) nPixels = 0.0f;
	if (m_lineSpacing == nPixels) return;
	m_lineSpacing = nPixels;
	// 行高改变需要重建段 layout (SetLineSpacing 是 IDWriteTextLayout 创建后立即设置, 改值
	// 必须重建). m_pTextFormat 可保留, 行高不在 TextFormat 上.
	ParaReleaseAllLayouts();
	InvalidateLayout();
	RedrawSelf();
}

float CXEditDW::GetLineSpacing() const{
	return m_lineSpacing;
}

//===================================================================
//  颜色
//===================================================================
// SetTextColor: 除了缓存到 m_textColor (自绘文本用) 外, 还同步到 XCGUI 元素级 XEle_SetTextColor,
// 让基类 GetTextColor / 任何走 XCGUI 标准接口取色的下游 (例如 XEle_DrawText 调用方 / 序列化路径)
// 拿到一致的值. 缺这一步, 用户调本类 SetTextColor 后通过 (CXEle*)pEdit->GetTextColor() 仍读旧值.
void CXEditDW::SetTextColor(COLORREF color){
	m_textColor = color;
	if (m_hEle) XEle_SetTextColor(m_hEle, color);
	RedrawSelf();
}
void CXEditDW::SetSelectBkColor(COLORREF color){      m_selBgColor = color;       RedrawSelf(); }
void CXEditDW::SetCaretColor(COLORREF color){         m_caretColor = color;       RedrawSelf(); }
// 下面三个设置器走 XCGUI BkInfo (XEle_AddBkFill / XEle_AddBkBorder), 与基类
// SetBkInfo / EnableDrawBorder / EnableDrawFocus 等 API 共用同一套默认绘制.
void CXEditDW::SetBkColor(COLORREF color){            m_bkColor = color;          RebuildBkInfo(); }
void CXEditDW::SetBorderColor(COLORREF color){        m_borderColor = color;      RebuildBkInfo(); }
// SetFocusBorderColor: BkInfo focus 边框色 + XCGUI 内置焦点边框色 一并同步, 避免两者
// 视觉割裂 (BkInfo 用新色 / XCGUI 内置色仍是 #58B1FC 默认蓝).
// 特殊: 处于 EnableDrawBorderEx(FALSE) 禁用态时, XCGUI 内置色被透明覆盖, 这里只更新
// m_savedXcguiFocusBorderColor 缓存, 让用户后续 EnableDrawBorderEx(TRUE) 时恢复到新色.
void CXEditDW::SetFocusBorderColor(COLORREF color){
	m_focusBorderColor = color;
	if (m_hEle){
		if (m_focusBorderSaved){
			m_savedXcguiFocusBorderColor = color;
		}else{
			XEle_SetFocusBorderColor(m_hEle, color);
		}
	}
	RebuildBkInfo();
}
void CXEditDW::SetHintColor(COLORREF color){          m_hintColor = color;        RedrawSelf(); }

//===================================================================
//  边框 / 插入符 / XEdit 接口对齐
//===================================================================
void CXEditDW::SetCaretWidth(int nWidth){
	m_caretWidth = (nWidth < 1) ? 1 : nWidth;
	RedrawSelf();
}

int CXEditDW::GetCaretWidth() const{
	return m_caretWidth;
}

void CXEditDW::SetBorderWidth(int nWidth){
	int v = (nWidth < 1) ? 1 : nWidth;
	if (v == m_borderWidth) return;
	m_borderWidth = v;
	RebuildBkInfo();   // 重建 BkInfo 中 BkBorder 项 (宽度变化必须重建, 走 RedrawSelf).
}

int CXEditDW::GetBorderWidth() const{
	return m_borderWidth;
}

void CXEditDW::SetBorderSize(int left, int top, int right, int bottom){
	if (!m_hEle) return;
	XEle_SetBorderSize(m_hEle, left, top, right, bottom);
	// SetBorderSize *只是* 文本内容区收缩距离, 不参与 BkInfo 颜色 / 笔画宽.
	// 文本布局依赖新的宽高 (GetContentWidth/Height 按 XEle_GetBorderSize 读取), 重建 layout.
	InvalidateLayout();
	RedrawSelf();
}

void CXEditDW::SetDefaultText(const wchar_t* pString){ SetHintText(pString); }
void CXEditDW::SetDefaultTextColor(COLORREF color){    SetHintColor(color); }

void CXEditDW::EnableAutoSelAll(BOOL bEnable){    m_autoSelAll    = bEnable ? true : false; }
void CXEditDW::EnableAutoCancelSel(BOOL bEnable){ m_autoCancelSel = bEnable ? true : false; }

void CXEditDW::MoveEnd(){
	int p = (int)m_text.size();
	m_caret  = p;
	m_anchor = p;
	EnsureCaretVisible();
	RedrawSelf();
	UpdateCaret();
}

BOOL CXEditDW::AutoScroll(){
	EnsureCaretVisible();
	return TRUE;
}

void CXEditDW::SetTextInt(int nValue){
	SetText(XC_itow(nValue));
}

//===================================================================
//  模式
//===================================================================
void CXEditDW::EnableMultiLine(BOOL bEnable){
	m_multiLine = bEnable ? true : false;
	if (!m_multiLine) XSView_ShowSBarV(m_hEle, FALSE);
	ReleaseTextFormat();
	InvalidateLayout();
	RedrawSelf();
}

BOOL CXEditDW::IsMultiLine() const{
	return m_multiLine ? TRUE : FALSE;
}

void CXEditDW::EnableAutoWrap(BOOL bEnable){
	m_wrap = bEnable ? true : false;
	ReleaseTextFormat();
	InvalidateLayout();
	RedrawSelf();
}

BOOL CXEditDW::IsAutoWrap() const{
	return m_wrap ? TRUE : FALSE;
}

void CXEditDW::RelayoutNow(){
	// SetRect / SetBorderSize / EnableAutoWrap 等改尺寸/换行的接口只 InvalidateLayout +
	// RedrawSelf, 真正重建分段 layout 在下一帧 OnPaintImpl 触发的 EnsureLayout 里. 调用
	// 方若紧接着用 XSView_GetTotalSize 读内容尺寸 (例如聊天气泡按内容收缩), 还拿不到
	// 新数据. 本接口同步调一次 EnsureLayout, 末尾会 RecomputeParaYOffsets +
	// XSView_SetTotalSize, 调用后立即可读到最新 totalSize.
	EnsureLayout();
}

void CXEditDW::EnableReadOnly(BOOL bEnable){
	m_readOnly = bEnable ? true : false;
}

BOOL CXEditDW::IsReadOnly() const{
	return m_readOnly ? TRUE : FALSE;
}

// 单行模式下改变对齐需要 *重建* layout (maxW 与 SetTextAlignment 要重设). 多行模式下
// 此字段被忽略, 也不影响 layout, 这里仍触发一次 Invalidate + Redraw 保证语义最小意外.
void CXEditDW::SetTextAlign(int align){
	if (m_textAlign == align) return;
	m_textAlign = align;
	InvalidateLayout();
	RedrawSelf();
}

int CXEditDW::GetTextAlign() const{
	return m_textAlign;
}

void CXEditDW::SetHintText(const wchar_t* pString){
	m_hint = pString ? pString : L"";
	RedrawSelf();
}

//===================================================================
//  选择 / 光标
//===================================================================
int CXEditDW::GetCurPos() const{
	return m_caret;
}

BOOL CXEditDW::SetCurPos(int pos){
	pos = ClampPos(pos);
	if (pos > 0 && pos < (int)m_text.size()
		&& IsLowSurrogate(m_text[pos]) && IsHighSurrogate(m_text[pos - 1])){
		pos += 1;
	}
	m_caret = pos;
	m_anchor = pos;
	EnsureCaretVisible();
	RedrawSelf();
	UpdateCaret();
	return TRUE;
}

BOOL CXEditDW::SelectAll(){
	if (m_text.empty()) return FALSE;
	m_anchor = 0;
	m_caret = (int)m_text.size();
	RedrawSelf();
	UpdateCaret();
	return TRUE;
}

BOOL CXEditDW::CancelSelect(){
	if (m_anchor == m_caret) return FALSE;
	m_anchor = m_caret;
	RedrawSelf();
	return TRUE;
}

BOOL CXEditDW::DeleteSelect(){
	if (m_readOnly) return FALSE;
	if (!HasSelectionInner()) return FALSE;
	PushUndo();
	int s, e; GetSelectionRangeInner(s, e);
	EraseCharsRaw(s, e - s);
	m_caret = m_anchor = s;
	// EraseCharsRaw 已走 ParaOnTextErased 同步段结构. 不调 InvalidateLayout (release-all).
	m_scrollToCaretPending = true;   // 由 EnsureLayout 末尾 post-rebuild hook 精确滚动
	RedrawSelf();
	UpdateCaret();
	return TRUE;
}

BOOL CXEditDW::HasSelection() const{
	return m_caret != m_anchor ? TRUE : FALSE;
}

int CXEditDW::GetSelStart() const{
	int s, e; GetSelectionRangeInner(s, e);
	return s;
}

int CXEditDW::GetSelEnd() const{
	int s, e; GetSelectionRangeInner(s, e);
	return e;
}

CXText CXEditDW::GetSelText() const{
	int s, e; GetSelectionRangeInner(s, e);
	if (s >= e) return CXText();
	return CXText(_FilterObjectReplacementChar(m_text.substr(s, e - s)));
}

//===================================================================
//  剪贴板
//===================================================================
BOOL CXEditDW::ClipboardCopy(){
	if (!HasSelectionInner()) return FALSE;
	int s, e; GetSelectionRangeInner(s, e);
	std::wstring sel = m_text.substr(s, e - s);    // 原始, 含 \uFFFC
	// 系统剪贴板放 *过滤版* (无 \uFFFC), 外部程序 (Notepad / 浏览器) 粘出来是纯文本.
	std::wstring selDisplay = _FilterObjectReplacementChar(sel);
	if (!CopyToClipboardImpl(selDisplay)) return FALSE;
	// 内部副本仍存 *原始* (含 \uFFFC) + 完整 styleIds. 粘回自己时用它恢复 inline 对象.
	m_clipText   = sel;
	m_clipStyles.assign(m_charStyle.begin() + s, m_charStyle.begin() + e);
	m_clipSeq    = ::GetClipboardSequenceNumber();
	return TRUE;
}

BOOL CXEditDW::ClipboardCut(){
	if (m_readOnly) return FALSE;
	if (!HasSelectionInner()) return FALSE;
	if (!ClipboardCopy()) return FALSE;   // 复用 Copy 同时存内部副本
	return DeleteSelect();
}

BOOL CXEditDW::ClipboardPaste(){
	if (m_readOnly) return FALSE;
	// 1) CF_DIB / CF_DIBV5 - 截图工具 Win+Shift+S / 浏览器/QQ/微信复制图片 / 应用内 Copy
	if (ClipboardPasteImage()) return TRUE;
	// 2) CF_HDROP - 用户在资源管理器 Ctrl+C 图片文件 (Windows 剪贴板里是路径列表, 不是
	//    位图; Win+V 历史看不到内容). 把列表展开, 逐个走 TryInsertDroppedFile 分流到
	//    图片缩略图 / 文本内容.
	if (ClipboardPasteHDropFiles()) return TRUE;
	// 3) 纯文本路径 (CF_UNICODETEXT / CF_TEXT). 内部副本能匹配则用副本 (保留 inline 对象).
	std::wstring sysText;
	if (!GetClipboardTextImpl(sysText)) return FALSE;

	// 单行模式: 过滤掉换行字符.
	if (!m_multiLine){
		std::wstring t; t.reserve(sysText.size());
		for (size_t i = 0; i < sysText.size(); ++i){
			wchar_t c = sysText[i];
			if (c != L'\r' && c != L'\n') t.push_back(c);
		}
		sysText.swap(t);
	}

	// 判断剪贴板是否仍是我们 Copy 时那份: 序列号未变 + 系统文本 == 内部原始的 *过滤版*.
	// 注意 m_clipText 是原始 (含 \uFFFC), sysText 是过滤后版本; 必须比对一致的视图.
	std::wstring clipTextFiltered = _FilterObjectReplacementChar(m_clipText);
	bool useInternal =
		(m_clipSeq != 0
		 && m_clipSeq == ::GetClipboardSequenceNumber()
		 && clipTextFiltered == sysText
		 && (int)m_clipStyles.size() == (int)m_clipText.size());

	if (!useInternal){
		// 外部来源 (或系统剪贴板被外部改过): sysText 本来已过滤换行 + 不含 \uFFFC (系统剪贴板里
		// 我们写的也是过滤版, 外部程序写的可能含 \uFFFC, 防御性再剔一次).
		std::wstring t; t.reserve(sysText.size());
		for (size_t i = 0; i < sysText.size(); ++i){
			if (sysText[i] != kObjectReplacementChar) t.push_back(sysText[i]);
		}
		InsertTextAtCursor(t.c_str(), (int)t.size());
		return TRUE;
	}

	// 内部副本路径: 插入 *原始* m_clipText (含 \uFFFC), 按 m_clipStyles 给每段 SetCharStyle.
	// 关键: 对 type=2 (inline 对象) 的 styleId, 走 CloneInlineHandle 克隆出 *独立实例*,
	// 这样 N 次粘贴产生 N 个独立 HXCGUI - 解决多次粘贴只显示最后一次位置的问题.
	// 不能克隆的类型 (HELE 控件 / 未识别 shape) 退化为共享: 仍是老行为, 但至少 Copy 第一次
	// 粘贴有效. 用户自定义控件可后续加 callback.
	std::wstring& insertText = m_clipText;
	int len = (int)insertText.size();

	// styleId 重映射: oldSid → newSid (clone 出来的). 仅对 type=2 + 可克隆 + 原 nRef>0 生效.
	// 对 type=0 (字体) / type=1 (图片) - 多字符引用同一字体是 *预期* 行为, 不克隆.
	// 对 type=2 但 nRef==0 (Cut 后原占位字符已被删) - 直接复用即可, 不必克隆.
	std::vector<int> remappedStyles = m_clipStyles;
	{
		std::vector<int> oldToNew(m_styleTable.size(), -1);  // -1 = 未处理过 / 不需要重映射
		for (size_t k = 0; k < remappedStyles.size(); ++k){
			int sid = remappedStyles[k];
			if (sid < 0 || !IsStyleIdValid(sid)) continue;
			const _XEditDW_Style& orig = m_styleTable[sid];
			if (orig.type != 2) continue;          // 只克隆 inline 对象
			if (orig.nRef <= 0) continue;          // 已无引用 (Cut 后) - 直接复用
			int mapped;
			if (oldToNew[sid] >= 0){
				mapped = oldToNew[sid];
			}
			else{
				HXCGUI hClone = CloneInlineHandle(orig.hFontImageObj);
				if (hClone){
					mapped = AddStyle(hClone, orig.color, orig.bColor);
				}
				else{
					mapped = sid;                  // 不支持克隆 - 退化共享
				}
				oldToNew[sid] = mapped;
			}
			remappedStyles[k] = mapped;
		}
	}

	PushUndo();
	int insertPos = m_caret;
	if (HasSelectionInner()){
		int a, b; GetSelectionRangeInner(a, b);
		EraseCharsRaw(a, b - a);
		insertPos = a;
	}
	InsertCharsRaw(insertPos, insertText.c_str(), len, -1);
	int i = 0;
	while (i < len){
		int sid = remappedStyles[i];
		int j = i + 1;
		while (j < len && remappedStyles[j] == sid) ++j;
		if (sid != -1 && IsStyleIdValid(sid)){
			SetCharStyle(insertPos + i, j - i, sid, false);
		}
		i = j;
	}
	if (len > 0) ParaInvalidateLayoutsForRange(insertPos, len);
	m_caret = m_anchor = insertPos + len;
	// Insert/EraseCharsRaw 已同步段结构. 不调 InvalidateLayout.
	m_scrollToCaretPending = true;
	// 闪烁修复: CloneInlineHandle 出的 inline 子元素创建于 (0,0,W,H), 若只 RedrawSelf 等
	// 下一帧 OnPaint 再 EnsureLayout → PositionInlineObjects, 中间 XCGUI 主框架可能先画
	// 它们一次 (用户视觉就是粘贴瞬间 *左上角闪一下* 然后才回到光标位置). 这里立即同步
	// 触发 EnsureLayout, layout 末尾 PositionInlineObjects 把子元素 SetPosition 到目标,
	// 再 RedrawSelf 时已经在终点. 行为与 InsertImageThumb 末尾的 EnsureLayout 一致.
	EnsureLayout();
	RedrawSelf();
	UpdateCaret();
	return TRUE;
}

//===================================================================
//  图片缩略图 / 剪贴板图片 / 拖放
//===================================================================

namespace {
// GDI+ 进程级 lazy init. XCGUI 内部一般已 GdiplusStartup, 我们这里再调一次只是引用计数
// +1 (GDI+ 内部支持嵌套), 进程退出时 OS 回收 - 故意不 Shutdown, 避免在 XCGUI 还在用
// GDI+ 时被我们提前关掉. C++17 magic static 保证多线程 init 安全.
static ULONG_PTR _EnsureGdiPlus(){
	static ULONG_PTR token = []() -> ULONG_PTR {
		ULONG_PTR t = 0;
		Gdiplus::GdiplusStartupInput in;
		(void)Gdiplus::GdiplusStartup(&t, &in, NULL);
		return t;
	}();
	return token;
}

// 装载 path + 预缩放到 (≤cap) 缩略尺寸. 关键性能优化: HIMAGE 只持小图, 之后 shape 1:1
// blit, 不再每帧实时 bicubic 重采样大图 (那是 GDI+ 模式卡顿的根因).
//   - 走 GDI+ 解码 → bicubic 缩 → GetHBITMAP → XImage_LoadFromHBITMAP. 大多数格式.
//   - 解码失败 (SVG / 不识别) 走 XImage_LoadFile 兜底 (无缩放, 但 SVG 通常小不卡).
// 出参 outW/outH = HIMAGE 实际内部尺寸 (调用方据此建 shape rect 实现 1:1 渲染).
// alpha: 用 PixelFormat32bppPARGB, PNG/WebP 透明背景能保留.
static HIMAGE _LoadFileAsThumb(const wchar_t* path, int maxLong, int maxSquare,
                               int& outW, int& outH){
	outW = outH = 0;
	_EnsureGdiPlus();
	Gdiplus::Bitmap src(path);
	if (src.GetLastStatus() != Gdiplus::Ok){
		// Fallback: SVG 等 GDI+ 不识别的格式, 走 XCGUI 原生路径, 不预缩.
		HIMAGE h = XImage_LoadFile(path);
		if (h){ outW = XImage_GetWidth(h); outH = XImage_GetHeight(h); }
		return h;
	}
	UINT sw = src.GetWidth(), sh = src.GetHeight();
	if (sw == 0 || sh == 0) return NULL;
	// 算目标尺寸 (与 CXEditDW::ComputeThumbSize 同规则). 6 行重复无所谓, 这里是 file-scope.
	bool isSquare = (sw == sh);
	int  cap      = isSquare ? maxSquare : maxLong;
	UINT longEdge = (sw > sh) ? sw : sh;
	int dw, dh;
	if ((int)longEdge <= cap){
		dw = (int)sw; dh = (int)sh;
	} else {
		double scale = (double)cap / (double)longEdge;
		dw = (int)((double)sw * scale + 0.5);
		dh = (int)((double)sh * scale + 0.5);
		if (dw < 1) dw = 1;
		if (dh < 1) dh = 1;
	}
	// 不需缩放时直接 GetHBITMAP 自 src, 省一次 Bitmap 分配 + DrawImage.
	Gdiplus::Bitmap* pAllocated = NULL;
	Gdiplus::Bitmap* pBmp       = &src;
	if (dw != (int)sw || dh != (int)sh){
		pAllocated = new Gdiplus::Bitmap(dw, dh, PixelFormat32bppPARGB);
		if (pAllocated->GetLastStatus() != Gdiplus::Ok){
			delete pAllocated;
			return NULL;
		}
		Gdiplus::Graphics g(pAllocated);
		g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		g.SetSmoothingMode    (Gdiplus::SmoothingModeHighQuality);
		g.SetPixelOffsetMode  (Gdiplus::PixelOffsetModeHighQuality);
		g.DrawImage(&src, 0, 0, dw, dh);
		pBmp = pAllocated;
	}
	HBITMAP hbm = NULL;
	// 背景色 (0,0,0,0) = 全透明; 32bpp 源会保留 alpha 通道.
	if (pBmp->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hbm) != Gdiplus::Ok || !hbm){
		delete pAllocated;
		return NULL;
	}
	HIMAGE hImage = XImage_LoadFromHBITMAP(hbm);
	// XImage_LoadFromHBITMAP 文档原文: "位图句柄,如果你不使用可以释放 DeleteObject()" -
	// 即 XCGUI 内部已拷数据出来, hbm 可立刻删. 不删就泄漏一个 GDI 对象.
	::DeleteObject(hbm);
	delete pAllocated;
	outW = dw;
	outH = dh;
	return hImage;
}
} // anonymous

// ============================================================================
//  全局图片转存目录 (SetImagePersistPath)
// ============================================================================
// 静态状态: 全局 (进程级) 唯一. 默认空, 表示关闭 (沿用旧行为, imagePath 直接存原路径).
static std::wstring s_imagePersistPath;

namespace {
// 规范化目录: 去尾部反斜杠 / 正斜杠, 不区分大小写做前缀比较时也用规范化路径.
static std::wstring _NormalizeDir(const std::wstring& p){
	std::wstring r = p;
	while (!r.empty() && (r.back() == L'\\' || r.back() == L'/')) r.pop_back();
	return r;
}

// 大小写无关的前缀匹配: child 是否 *位于* parent 目录下 (parent 不含尾分隔, child 任意).
static bool _PathStartsWithDir(const std::wstring& child, const std::wstring& parent){
	if (parent.empty() || child.size() <= parent.size()) return false;
	// 用 _wcsnicmp 做大小写无关比较 (Windows 文件系统语义)
	if (_wcsnicmp(child.c_str(), parent.c_str(), parent.size()) != 0) return false;
	// 紧随的字符必须是分隔符, 否则 "C:\foo" 不应匹配 "C:\foobar".
	wchar_t sep = child[parent.size()];
	return (sep == L'\\' || sep == L'/');
}

// FNV-1a 64-bit hash, 输入 wide string. 用于生成稳定 / 短小的目标文件名.
// 同一源路径 → 同一文件名, 反复粘贴只占一份 (覆盖写, 内容同).
static uint64_t _Fnv1a64(const wchar_t* s){
	uint64_t h = 0xcbf29ce484222325ULL;
	if (!s) return h;
	for (; *s; ++s){
		// wchar_t 16 位: 把两个字节都喂进哈希, 防止只看高/低字节带来高碰撞.
		h ^= (uint8_t)(*s & 0xFF); h *= 0x100000001b3ULL;
		h ^= (uint8_t)((*s >> 8) & 0xFF); h *= 0x100000001b3ULL;
	}
	return h;
}

// 取扩展名 (含点). 没有扩展时返 ".bin"; 兼容 srcPath 是 NULL / 空.
static std::wstring _ExtractExt(const wchar_t* path){
	if (!path || !*path) return L".bin";
	const wchar_t* p = path + wcslen(path);
	while (p > path){
		--p;
		if (*p == L'\\' || *p == L'/') break;
		if (*p == L'.'){
			// "abc." 这种空扩展也接受, 但兜底成 ".bin"
			if (p[1] == 0) return L".bin";
			return std::wstring(p);
		}
	}
	return L".bin";
}

// 把 srcPath 复制到全局转存目录, 返回目标路径. 任一前置条件不满足时返 srcPath 原样,
// *不破坏* 旧路径行为. 失败 (CopyFile 错 / 目录建不出) 也兜底返 srcPath, 调用方仍能用原图.
// 关键: 目标文件名用 srcPath 的 hash, 同源路径多次粘贴只产生一份目标文件 (覆盖写).
static std::wstring _PersistImageFile(const wchar_t* srcPath){
	if (!srcPath || !*srcPath) return std::wstring();
	if (s_imagePersistPath.empty()) return srcPath;   // 全局未设置, 关闭

	std::wstring dir = _NormalizeDir(s_imagePersistPath);
	if (dir.empty()) return srcPath;

	// 原路径已在转存目录内 → 视为已持久化, 直接复用 (避免每次粘贴又拷贝一份冗余).
	std::wstring src = srcPath;
	if (_PathStartsWithDir(src, dir)) return src;

	// 确保目录存在. CreateDirectoryW 对 *已存在* 返 ERROR_ALREADY_EXISTS, 视为成功.
	if (!::CreateDirectoryW(dir.c_str(), NULL)){
		DWORD ec = ::GetLastError();
		if (ec != ERROR_ALREADY_EXISTS){
			// 父目录不存在等场景: 走 SHCreateDirectoryExW 兜底递归创建.
			// 不直接 #include <shlobj.h> (避免拉一大堆 PCH), 改用动态加载.
			typedef int (WINAPI *PFN_SHCreateDirectoryExW)(HWND, LPCWSTR, const SECURITY_ATTRIBUTES*);
			HMODULE hShell = ::GetModuleHandleW(L"shell32.dll");
			if (!hShell) hShell = ::LoadLibraryW(L"shell32.dll");
			PFN_SHCreateDirectoryExW pfn = hShell
				? (PFN_SHCreateDirectoryExW)::GetProcAddress(hShell, "SHCreateDirectoryExW")
				: NULL;
			int r = pfn ? pfn(NULL, dir.c_str(), NULL) : -1;
			if (r != 0 /*ERROR_SUCCESS*/ && r != ERROR_ALREADY_EXISTS && r != ERROR_FILE_EXISTS){
				return src;   // 目录建不出, 兜底用原路径
			}
		}
	}

	// 目标文件名 = "img_<hash16hex><ext>". hex 16 位足以避不同源路径碰撞.
	wchar_t nameBuf[64];
	uint64_t h = _Fnv1a64(src.c_str());
	std::wstring ext = _ExtractExt(src.c_str());
	::swprintf_s(nameBuf, _countof(nameBuf), L"img_%016llx%s", (unsigned long long)h, ext.c_str());

	std::wstring dst = dir + L"\\" + nameBuf;

	// 已存在同名 (hash 同源) → 跳过拷贝, 直接返目标. 不强制覆盖, 因为内容必然同源.
	DWORD attr = ::GetFileAttributesW(dst.c_str());
	if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)){
		return dst;
	}
	// 不存在 → CopyFileW. bFailIfExists=TRUE 与上面 GetFileAttributesW 检测语义一致;
	// 偶发竞态 (别的线程刚写) 时 CopyFile 失败, 我们兜底返原路径.
	if (::CopyFileW(src.c_str(), dst.c_str(), TRUE)){
		return dst;
	}
	// 拷贝失败 (磁盘满 / 权限 / 源不存在): 兜底用原路径, 行为同关闭转存.
	return src;
}
} // anonymous

void CXEditDW::SetImagePersistPath(const wchar_t* pPath){
	if (!pPath || !*pPath){
		s_imagePersistPath.clear();
		return;
	}
	s_imagePersistPath = _NormalizeDir(pPath);
}

const wchar_t* CXEditDW::GetImagePersistPath(){
	return s_imagePersistPath.c_str();
}

void CXEditDW::SetImageThumbMaxSize(int maxLong, int maxSquare){
	if (maxLong   > 0) m_imageThumbMaxLong   = maxLong;
	if (maxSquare > 0) m_imageThumbMaxSquare = maxSquare;
}

void CXEditDW::SetMaxTextLength(int maxLen){
	if (maxLen <= 0) return;
	m_maxTextLen = maxLen;
	// 注: 不主动截断已有内容 - 调小上限只对后续新增生效, 已经存在的文本保留. 调用方
	// 想清掉超额部分得自己 GetText + 处理 + SetText.
}

int CXEditDW::GetMaxTextLength() const{
	return m_maxTextLen;
}

// 算缩略图目标尺寸. 输入输出都是 *逻辑像素* (后续 XShapePic_Create 用逻辑像素).
// 规则: 正方形 (w==h) 走 maxSquare; 非正方形长边走 maxLong. 原图比阈值小则不放大 (按原).
void CXEditDW::ComputeThumbSize(int srcW, int srcH, int& outW, int& outH) const{
	if (srcW <= 0 || srcH <= 0){
		outW = (srcW > 0) ? srcW : 1;
		outH = (srcH > 0) ? srcH : 1;
		return;
	}
	bool isSquare = (srcW == srcH);
	int cap = isSquare ? m_imageThumbMaxSquare : m_imageThumbMaxLong;
	int longEdge = (srcW > srcH) ? srcW : srcH;
	if (longEdge <= cap){
		outW = srcW;
		outH = srcH;
		return;
	}
	// 等比缩到长边 = cap. 用 double 计算避免整除精度损失.
	double scale = (double)cap / (double)longEdge;
	outW = (int)((double)srcW * scale + 0.5);
	outH = (int)((double)srcH * scale + 0.5);
	if (outW < 1) outW = 1;
	if (outH < 1) outH = 1;
}

BOOL CXEditDW::InsertImageThumb(const wchar_t* pPath){
	if (m_readOnly || !pPath || !*pPath || !m_hEle) return FALSE;
	// 全局图片转存: 设了 SetImagePersistPath 时, 把原文件复制到该目录, 后续 *都* 用新路径
	// (加载缩略图 / 存 imagePath / 序列化). 未设置 / 复制失败 / 原路径已在目录下 → 返回原路径.
	std::wstring effPath = _PersistImageFile(pPath);
	if (effPath.empty()) effPath = pPath;
	pPath = effPath.c_str();
	// 关键性能路径: _LoadFileAsThumb 在 GDI+ 里把大图一次性 bicubic 压成 ≤cap 的小位图,
	// HIMAGE 之后只持小图. 取代 v1 "XImage_LoadFile 持全尺寸 + 每帧 fixed_ratio 重采样"
	// 的卡顿路径 (GDI+ 模式实测插几张大图就完全卡住, 因为 HighQualityBicubic 跑 CPU).
	int tw = 0, th = 0;
	HIMAGE hImage = _LoadFileAsThumb(pPath, m_imageThumbMaxLong, m_imageThumbMaxSquare, tw, th);
	if (!hImage || tw <= 0 || th <= 0){
		if (hImage) XImage_Release(hImage);
		return FALSE;
	}
	// SVG 兜底路径下 tw/th 可能仍是 *原始* 尺寸; 再过一遍 ComputeThumbSize 保证 shape rect
	// 一定 ≤ cap. 走 GDI+ 路径时 _LoadFileAsThumb 已缩到 ≤cap, 这步是 no-op.
	int rw = tw, rh = th;
	ComputeThumbSize(tw, th, rw, rh);

	HXCGUI hShape = XShapePic_Create(0, 0, rw, rh, (HXCGUI)m_hEle);
	if (!hShape){
		XImage_Release(hImage);
		return FALSE;
	}
	// fixed_ratio: 主要给 SVG 兜底路径 (shape rect 可能小于 image) 兜底; GDI+ 预缩放路径下
	// shape rect 与 HIMAGE 1:1, fixed_ratio 也是 no-op. 保留这行更安全 (将来 shape rect 被
	// 外部代码改大改小都不会变形).
	XImage_SetDrawType(hImage, image_draw_type_fixed_ratio);

	// XShapePic_SetImage 消耗输入引用; 之后我们不再持有 hImage 的 ref. 不能再 Release,
	// 否则 shape 拿悬挂指针只显示占位 (v1 bug).
	XShapePic_SetImage(hShape, hImage);
	hImage = NULL;

	// 建样式槽: 与 AddObject 同语义 (type=2 object). imagePath 字段单独标识 "拖入/粘贴
	// 的图片", 给未来序列化按原路径回写用.
	int sid = AddStyle(hShape, 0, FALSE);
	if (sid < 0){
		XShape_Destroy(hShape);   // shape 析构会 Release 内部 HIMAGE
		return FALSE;
	}
	m_styleTable[sid].imagePath = pPath;

	// 插入到 *光标位置*, 与剪贴板粘贴 / 输入框打字同语义. 如有选区, 先删后插. 拖入路径
	// (OnDropFilesImpl) 在循环前/中保证 m_caret 一直在末尾, 此处仍能正确 "追加".
	PushUndo();
	int insertPos = m_caret;
	if (HasSelectionInner()){
		int a, b; GetSelectionRangeInner(a, b);
		EraseCharsRaw(a, b - a);
		insertPos = a;
	}
	wchar_t orc[2] = { kObjectReplacementChar, 0 };
	InsertCharsRaw(insertPos, orc, 1, sid);
	m_caret  = insertPos + 1;
	m_anchor = m_caret;

	m_scrollToCaretPending = true;
	// InsertCharsRaw 内部已同步段结构. EnsureLayout 只建缺失段.
	EnsureLayout();   // 同步重建 (inline 对象定位需要) - post-rebuild hook 会用新 layout 精确滚动
	RedrawSelf();
	UpdateCaret();
	// 不再 XImage_Release: hImage 的 ref 已被 XShapePic_SetImage 接管 (见上注释).
	return TRUE;
}

// 读文件前 maxBytes 字节. maxBytes=0 表示整文件 (限 64MB 上限, 防御坏数据).
bool CXEditDW::ReadFileBytes(const wchar_t* path, std::vector<unsigned char>& out, size_t maxBytes){
	out.clear();
	if (!path || !*path) return false;
	HANDLE h = ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return false;
	LARGE_INTEGER li; li.QuadPart = 0;
	if (!::GetFileSizeEx(h, &li)){ ::CloseHandle(h); return false; }
	size_t fileSize = (size_t)li.QuadPart;
	const size_t kHardCap = 64u * 1024u * 1024u; // 64MB 防御 (拖一个 ISO 进来不至于 OOM)
	size_t want = fileSize;
	if (maxBytes > 0 && want > maxBytes) want = maxBytes;
	if (want > kHardCap) want = kHardCap;
	out.resize(want);
	DWORD got = 0;
	BOOL ok = (want == 0) ? TRUE : ::ReadFile(h, out.data(), (DWORD)want, &got, NULL);
	::CloseHandle(h);
	if (!ok){ out.clear(); return false; }
	if ((size_t)got < want) out.resize(got);
	return true;
}

// 图片魔数嗅探. 仅看前 16 字节, 命中常见格式即认.
bool CXEditDW::SniffIsImage(const unsigned char* d, size_t n){
	if (!d || n < 4) return false;
	// PNG: 89 50 4E 47 0D 0A 1A 0A
	if (n >= 8 && d[0]==0x89 && d[1]==0x50 && d[2]==0x4E && d[3]==0x47
	            && d[4]==0x0D && d[5]==0x0A && d[6]==0x1A && d[7]==0x0A) return true;
	// JPEG: FF D8 FF
	if (d[0]==0xFF && d[1]==0xD8 && d[2]==0xFF) return true;
	// GIF87a / GIF89a
	if (n >= 6 && d[0]=='G' && d[1]=='I' && d[2]=='F' && d[3]=='8'
	            && (d[4]=='7' || d[4]=='9') && d[5]=='a') return true;
	// BMP: 42 4D ("BM")
	if (d[0]==0x42 && d[1]==0x4D) return true;
	// WebP: "RIFF" .... "WEBP"
	if (n >= 12 && d[0]=='R' && d[1]=='I' && d[2]=='F' && d[3]=='F'
	            && d[8]=='W' && d[9]=='E' && d[10]=='B' && d[11]=='P') return true;
	return false;
}

// UTF-8 校验 (拒绝 overlong / surrogate / 超 U+10FFFF). 严格但不严苛.
bool CXEditDW::IsValidUtf8(const unsigned char* d, size_t n){
	if (!d) return false;
	size_t i = 0;
	while (i < n){
		unsigned char c = d[i];
		if (c < 0x80){ ++i; continue; }                                  // ASCII
		size_t need;
		unsigned int minVal, maxVal, val;
		if      ((c & 0xE0) == 0xC0){ need = 1; minVal = 0x0080;   maxVal = 0x07FF;   val = c & 0x1F; }
		else if ((c & 0xF0) == 0xE0){ need = 2; minVal = 0x0800;   maxVal = 0xFFFF;   val = c & 0x0F; }
		else if ((c & 0xF8) == 0xF0){ need = 3; minVal = 0x10000;  maxVal = 0x10FFFF; val = c & 0x07; }
		else return false;
		if (i + need >= n) return false;
		for (size_t k = 1; k <= need; ++k){
			unsigned char cc = d[i + k];
			if ((cc & 0xC0) != 0x80) return false;
			val = (val << 6) | (cc & 0x3F);
		}
		if (val < minVal || val > maxVal) return false;
		if (val >= 0xD800 && val <= 0xDFFF) return false;                // surrogate 非法
		i += need + 1;
	}
	return true;
}

// 二进制嗅探: 仅看前 4KB. 有 NUL → 立刻判二进制; 否则统计非可见控制字符占比.
bool CXEditDW::ProbablyText(const unsigned char* d, size_t n){
	if (!d || n == 0) return true;       // 空文件当文本 (允许追加空白)
	size_t lim = (n > 4096) ? 4096 : n;
	size_t bad = 0;
	for (size_t i = 0; i < lim; ++i){
		unsigned char c = d[i];
		if (c == 0) return false;        // NUL 在文本里极罕见, 视为强二进制信号
		if (c < 0x20 && c != 0x09 && c != 0x0A && c != 0x0D) ++bad;
	}
	return (bad * 100 / lim) < 5;        // < 5% 控制字符 → 当文本
}

// 字节 → wstring: 优先 BOM, 其次 UTF-8 校验, 最后退到系统 ACP. 返回 FALSE 表示无法解码.
bool CXEditDW::BytesToWStr(const unsigned char* d, size_t n, std::wstring& out){
	out.clear();
	if (!d || n == 0) return true;
	// UTF-8 BOM
	if (n >= 3 && d[0] == 0xEF && d[1] == 0xBB && d[2] == 0xBF){
		const char* p = (const char*)(d + 3);
		int srcLen = (int)(n - 3);
		int wn = ::MultiByteToWideChar(CP_UTF8, 0, p, srcLen, NULL, 0);
		if (wn > 0){ out.resize((size_t)wn); ::MultiByteToWideChar(CP_UTF8, 0, p, srcLen, &out[0], wn); }
		return true;
	}
	// UTF-16 LE BOM
	if (n >= 2 && d[0] == 0xFF && d[1] == 0xFE){
		size_t bytes = n - 2;
		size_t cnt = bytes / 2;
		out.assign((const wchar_t*)(d + 2), cnt);
		return true;
	}
	// UTF-16 BE BOM (按字节翻转后当 LE 用)
	if (n >= 2 && d[0] == 0xFE && d[1] == 0xFF){
		size_t bytes = n - 2;
		size_t cnt = bytes / 2;
		out.resize(cnt);
		const unsigned char* p = d + 2;
		for (size_t i = 0; i < cnt; ++i){
			out[i] = (wchar_t)((p[i*2] << 8) | p[i*2 + 1]);
		}
		return true;
	}
	// 无 BOM: 先尝试 UTF-8 校验 (绝大多数现代文本)
	if (IsValidUtf8(d, n)){
		int wn = ::MultiByteToWideChar(CP_UTF8, 0, (const char*)d, (int)n, NULL, 0);
		if (wn > 0){ out.resize((size_t)wn); ::MultiByteToWideChar(CP_UTF8, 0, (const char*)d, (int)n, &out[0], wn); }
		return true;
	}
	// 最后退到系统 ACP (中文 Windows = GBK; 英文 = Windows-1252).
	int wn = ::MultiByteToWideChar(CP_ACP, 0, (const char*)d, (int)n, NULL, 0);
	if (wn <= 0) return false;
	out.resize((size_t)wn);
	::MultiByteToWideChar(CP_ACP, 0, (const char*)d, (int)n, &out[0], wn);
	return true;
}

// 把当前剪贴板的 CF_DIB 写为 .bmp 文件. BMP 文件格式 = BITMAPFILEHEADER(14B) + DIB.
// CF_DIB 给的就是 DIB 部分, 我们补上 BITMAPFILEHEADER 即可成完整 BMP. 不用 Gdiplus.
bool CXEditDW::ClipboardImageToTempBmp(std::wstring& outPath){
	outPath.clear();
	HWND hWndOwner = NULL;
	if (m_hEle){
		HWINDOW hw = (HWINDOW)XWidget_GetHWINDOW((HXCGUI)m_hEle);
		if (hw) hWndOwner = XWnd_GetHWND(hw);
	}
	if (!::OpenClipboard(hWndOwner)) return false;
	// CF_DIB 首选; CF_DIBV5 在我们写出 BMP 时也兼容 (V5 header 是 V1 兼容超集).
	UINT fmt = 0;
	if (::IsClipboardFormatAvailable(CF_DIB))        fmt = CF_DIB;
	else if (::IsClipboardFormatAvailable(CF_DIBV5)) fmt = CF_DIBV5;
	if (fmt == 0){ ::CloseClipboard(); return false; }
	HANDLE hData = ::GetClipboardData(fmt);
	if (!hData){ ::CloseClipboard(); return false; }
	SIZE_T dibSize = ::GlobalSize(hData);
	if (dibSize < sizeof(BITMAPINFOHEADER)){ ::CloseClipboard(); return false; }
	void* pDib = ::GlobalLock(hData);
	if (!pDib){ ::CloseClipboard(); return false; }

	// 算调色板大小. biClrUsed=0 时 1/4/8 bpp 默认 2^bpp 色; >8 bpp 无调色板.
	const BITMAPINFOHEADER* bih = (const BITMAPINFOHEADER*)pDib;
	DWORD palBytes = 0;
	if (bih->biBitCount <= 8){
		DWORD nCol = bih->biClrUsed;
		if (nCol == 0) nCol = 1u << bih->biBitCount;
		palBytes = nCol * sizeof(RGBQUAD);
	}
	DWORD bihSize  = bih->biSize;
	DWORD offBits  = sizeof(BITMAPFILEHEADER) + bihSize + palBytes;
	DWORD fileSize = (DWORD)(sizeof(BITMAPFILEHEADER) + dibSize);

	BITMAPFILEHEADER bfh; ZeroMemory(&bfh, sizeof(bfh));
	bfh.bfType    = 0x4D42;
	bfh.bfSize    = fileSize;
	bfh.bfOffBits = offBits;

	// 临时路径: %TEMP%\xcgui_paste_<pid>_<tick>.bmp
	wchar_t tempDir[MAX_PATH]; tempDir[0] = 0;
	::GetTempPathW(MAX_PATH, tempDir);
	wchar_t buf[MAX_PATH + 64];
	::wsprintfW(buf, L"%sxcgui_paste_%u_%u.bmp", tempDir,
	            (unsigned)::GetCurrentProcessId(), (unsigned)::GetTickCount());

	HANDLE hFile = ::CreateFileW(buf, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		::GlobalUnlock(hData);
		::CloseClipboard();
		return false;
	}
	DWORD wrote = 0;
	BOOL ok = ::WriteFile(hFile, &bfh, sizeof(bfh), &wrote, NULL);
	if (ok) ok = ::WriteFile(hFile, pDib, (DWORD)dibSize, &wrote, NULL);
	::CloseHandle(hFile);
	::GlobalUnlock(hData);
	::CloseClipboard();
	if (!ok){ ::DeleteFileW(buf); return false; }
	outPath = buf;
	return true;
}

BOOL CXEditDW::ClipboardPasteImage(){
	if (m_readOnly) return FALSE;
	// 快速预判: 剪贴板若没图像数据直接退出, 省一次 OpenClipboard 的锁开销.
	if (!::IsClipboardFormatAvailable(CF_DIB) && !::IsClipboardFormatAvailable(CF_DIBV5)){
		return FALSE;
	}
	std::wstring path;
	if (!ClipboardImageToTempBmp(path)) return FALSE;
	// 临时 BMP 不立刻删: imagePath 引用着它, 未来序列化要读. 进程退出后系统 %TEMP% 自清.
	return InsertImageThumb(path.c_str());
}

// 资源管理器里 *Ctrl+C 文件* 后剪贴板会放 CF_HDROP (路径列表), 与拖入文件格式同. 复用
// TryInsertDroppedFile 分流: 图片走 InsertImageThumb (光标处), 文本走 InsertTextAtCursor
// (光标处). 与 OnDropFilesImpl 的区别 - 这里 *不* 把光标拉到末尾, 保留 paste 在光标位置
// 的语义; 也 *不* 自动 MoveEnd 滚到底 (EnsureCaretVisible 在每个分支内部已做).
bool CXEditDW::ClipboardPasteHDropFiles(){
	if (m_readOnly) return false;
	if (!::IsClipboardFormatAvailable(CF_HDROP)) return false;
	HWND hWndOwner = NULL;
	if (m_hEle){
		HWINDOW hw = (HWINDOW)XWidget_GetHWINDOW((HXCGUI)m_hEle);
		if (hw) hWndOwner = XWnd_GetHWND(hw);
	}
	if (!::OpenClipboard(hWndOwner)) return false;
	HDROP hDrop = (HDROP)::GetClipboardData(CF_HDROP);
	if (!hDrop){ ::CloseClipboard(); return false; }
	// 注意: clipboard 给的 HDROP 不需要 GlobalLock - 它本身就是个 HANDLE, DragQueryFileW
	// 内部自己 Lock/Unlock. 也 *不要* 调 DragFinish (DragFinish 只针对来自 WM_DROPFILES
	// 的 HDROP; 调用在 clipboard HDROP 上是未定义行为).
	UINT n = ::DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
	bool anyOk = false;
	wchar_t buf[MAX_PATH + 4];
	for (UINT i = 0; i < n; ++i){
		buf[0] = 0;
		UINT got = ::DragQueryFileW(hDrop, i, buf, MAX_PATH);
		if (got == 0) continue;
		if (TryInsertDroppedFile(buf)) anyOk = true;
	}
	::CloseClipboard();
	return anyOk;
}

bool CXEditDW::TryInsertDroppedFile(const wchar_t* path){
	if (!path || !*path) return false;
	// 读前 4KB 用于嗅探. 图片够看魔数, 文本够判二进制.
	std::vector<unsigned char> head;
	if (!ReadFileBytes(path, head, 4096)) return false;
	if (SniffIsImage(head.data(), head.size())){
		return InsertImageThumb(path) == TRUE;
	}
	if (!ProbablyText(head.data(), head.size())) return false;
	// 文本: 重新读全文件 (限 64MB), 解码后插到 *光标位置*. drop 路径 (OnDropFilesImpl)
	// 调用前已经把光标拉到 m_text.size(), 所以 "光标处插入" 此刻 = "末尾追加"; 剪贴板
	// 粘贴文件路径 (ClipboardPasteHDropFiles) 不动光标, "光标处插入" = 真正的 paste 语义.
	std::vector<unsigned char> body;
	if (!ReadFileBytes(path, body, 0)) return false;
	std::wstring text;
	if (!BytesToWStr(body.data(), body.size(), text)) return false;
	if (text.empty()) return true;     // 空文件也算成功 (无 op)
	// 单行模式: 把换行展平为空格, 跟 ClipboardPaste 一致语义.
	if (!m_multiLine){
		for (size_t i = 0; i < text.size(); ++i){
			if (text[i] == L'\r' || text[i] == L'\n') text[i] = L' ';
		}
	}
	InsertTextAtCursor(text.c_str(), (int)text.size());
	return true;
}

int CXEditDW::OnDropFilesImpl(HELE /*hEle*/, HDROP hDropInfo, BOOL* /*pbHandled*/){
	if (!hDropInfo || m_readOnly) return 0;
	UINT n = ::DragQueryFileW(hDropInfo, 0xFFFFFFFF, NULL, 0);
	if (n == 0) return 0;
	// 拖入语义 = *追加到末尾*. 循环前先把光标拉到当前末尾, 这样 InsertImageThumb (光标处插)
	// 自然变 "追加". 取消任何已有选区, 避免被 InsertImageThumb 当成 "替换选区".
	m_caret = m_anchor = (int)m_text.size();

	bool anyOk = false;
	wchar_t buf[MAX_PATH + 4];
	for (UINT i = 0; i < n; ++i){
		buf[0] = 0;
		UINT got = ::DragQueryFileW(hDropInfo, i, buf, MAX_PATH);
		if (got == 0) continue;
		if (TryInsertDroppedFile(buf)){
			anyOk = true;
			// 文本走 AddText (不动光标), 图片走 InsertImageThumb (光标 +1). 两条路径混用, 这里
			// 显式同步光标到末尾, 保证下一文件继续追加而不是落在中间.
			m_caret = m_anchor = (int)m_text.size();
		}
	}
	// 注意: 不调 DragFinish - 见文件头部 include 注释. XCGUI 自己负责释放 hDropInfo.
	if (anyOk){
		// 滚到底: MoveEnd 内部把光标定到末尾 + EnsureCaretVisible 滚动视图.
		MoveEnd();
	}
	return 0;
}

//===================================================================
//  撤销 / 重做
//===================================================================
BOOL CXEditDW::Undo(){
	if (m_readOnly || m_undoStack.empty()) return FALSE;
	_XEditDW_UndoState now;
	now.text = m_text;
	now.charStyle = m_charStyle;
	now.caret = m_caret;
	now.anchor = m_anchor;
	m_redoStack.push_back(std::move(now));
	_XEditDW_UndoState s = std::move(m_undoStack.back());
	m_undoStack.pop_back();
	// 先按现状样式全量 decRef 后载入老快照, 再 incRef 老样式. 保证引用计数准确.
	for (int i = 0; i < (int)m_charStyle.size(); ++i) StyleDecRef(m_charStyle[i]);
	m_text = std::move(s.text);
	m_charStyle = std::move(s.charStyle);
	for (int i = 0; i < (int)m_charStyle.size(); ++i) StyleIncRef(m_charStyle[i]);
	m_caret = s.caret;
	m_anchor = s.anchor;
	// Undo/Redo 走 m_text 整体赋值, 不走 Insert/EraseCharsRaw, 需人工同步段结构.
	ParaRebuildFromText();
	m_scrollToCaretPending = true;
	// 重置节流错点: Undo 后紧接着输入, 不能合并到 *已被回退* 的会话起点.
	m_lastUndoTick  = 0;
	m_lastUndoCaret = -1;
	RedrawSelf();
	UpdateCaret();
	return TRUE;
}

BOOL CXEditDW::Redo(){
	if (m_readOnly || m_redoStack.empty()) return FALSE;
	_XEditDW_UndoState now;
	now.text = m_text;
	now.charStyle = m_charStyle;
	now.caret = m_caret;
	now.anchor = m_anchor;
	m_undoStack.push_back(std::move(now));
	_XEditDW_UndoState s = std::move(m_redoStack.back());
	m_redoStack.pop_back();
	for (int i = 0; i < (int)m_charStyle.size(); ++i) StyleDecRef(m_charStyle[i]);
	m_text = std::move(s.text);
	m_charStyle = std::move(s.charStyle);
	for (int i = 0; i < (int)m_charStyle.size(); ++i) StyleIncRef(m_charStyle[i]);
	m_caret = s.caret;
	m_anchor = s.anchor;
	ParaRebuildFromText();
	m_scrollToCaretPending = true;
	// 重置节流锚: Redo 后紧接着输入, 不能合并到 *已经被前进* 的状态 (同 Undo 处理).
	m_lastUndoTick  = 0;
	m_lastUndoCaret = -1;
	RedrawSelf();
	UpdateCaret();
	return TRUE;
}

//===================================================================
//  样式表 / 对象插入 (XEdit 兼容)
//===================================================================
// 设计:
//   - m_styleTable[i] 是一条样式 (字体/图片/UI对象 + 颜色). type=0xFFFF 表示该槽
//     已被 DeleteStyle 释放, AddStyle 时优先复用.
//   - m_charStyle[i] 与 m_text[i] 一一对应; -1 表示该字符使用默认字体/颜色;
//     m_text[i] == U+FFFC 时 m_charStyle[i] 必须指向一条 image/object 样式.
//   - 引用计数: 每次 m_charStyle 写入或拷贝时 incRef, 移除时 decRef. DeleteStyle
//     仅在 nRef==0 (无字符引用) 时返回成功.

bool CXEditDW::IsStyleIdValid(int iStyle) const{
	if (iStyle < 0 || iStyle >= (int)m_styleTable.size()) return false;
	return m_styleTable[iStyle].type != 0xFFFF;
}

void CXEditDW::StyleIncRef(int iStyle){
	if (!IsStyleIdValid(iStyle)) return;
	m_styleTable[iStyle].nRef++;
}

void CXEditDW::StyleDecRef(int iStyle){
	if (!IsStyleIdValid(iStyle)) return;
	if (m_styleTable[iStyle].nRef > 0) m_styleTable[iStyle].nRef--;
}

bool CXEditDW::IsInlinePos(int i) const{
	if (i < 0 || i >= (int)m_text.size()) return false;
	return m_text[i] == kObjectReplacementChar;
}

void CXEditDW::SetCharStyle(int pos, int len, int iStyle, bool invalidateLayout){
	int n = (int)m_text.size();
	if (pos < 0) pos = 0;
	if (pos > n) pos = n;
	if (pos + len > n) len = n - pos;
	if (len <= 0) return;
	if ((int)m_charStyle.size() < n) m_charStyle.resize(n, -1);
	for (int i = pos; i < pos + len; ++i){
		StyleDecRef(m_charStyle[i]);
		m_charStyle[i] = iStyle;
		StyleIncRef(iStyle);
	}
	if (invalidateLayout) ParaInvalidateLayoutsForRange(pos, len);
}

void CXEditDW::InsertCharsRaw(int pos, const wchar_t* p, int len, int styleId){
	if (!p || len <= 0) return;
	int n = (int)m_text.size();
	// 内容长度上限: 截断到剩余配额. 0 剩余 → 整段拒绝. *截断而非整段拒绝* 是因为粘贴长
	// 文本时部分插入比丢全部更友好 (用户能看到 "到此为止" 而不是 "什么都没发生").
	// 注: 如果截断点落在 surrogate pair 中间, 会得到孤立高代理. 概率低 (10^-5), 暂不修.
	if (n >= m_maxTextLen) return;
	int room = m_maxTextLen - n;
	if (len > room) len = room;
	if (pos < 0) pos = 0;
	if (pos > n) pos = n;
	// m_text 与 m_charStyle 必须等长. 老代码可能在某些路径上跳过同步, 这里兜底对齐.
	if ((int)m_charStyle.size() != n) m_charStyle.resize(n, -1);
	m_text.insert(pos, p, len);
	m_charStyle.insert(m_charStyle.begin() + pos, len, styleId);
	for (int i = 0; i < len; ++i) StyleIncRef(styleId);
	// 分段架构: 仅释放受影响段的 layout, 其他段 layout 缓存保留, 编辑热路径关键提速点.
	ParaOnTextInserted(pos, len);
}

void CXEditDW::EraseCharsRaw(int pos, int len){
	if (len <= 0) return;
	int n = (int)m_text.size();
	if (pos < 0) pos = 0;
	if (pos + len > n) len = n - pos;
	if (len <= 0) return;
	if ((int)m_charStyle.size() < n) m_charStyle.resize(n, -1);
	for (int i = pos; i < pos + len; ++i){
		StyleDecRef(m_charStyle[i]);
		// *不在此处销毁* inline 对象 / 字体 / 图片. 原因: 撤销栈 (m_undoStack) 里的
		// 快照仍引用该 styleId, 用户按 Ctrl+Z 时需要把 \uFFFC 重新插回 m_charStyle 并
		// 重新唤醒对应子元素. 立即销毁会导致撤销后样式槽是空 (type=0xFFFF), 句柄已无效.
		// 真正的释放走以下路径之一:
		//   - SetText: 硬重置, 用户已经接受了 *无法撤销*, 全表 ReleaseAllStyleResources.
		//   - DeleteStyle(iStyle): 用户主动指定释放某条样式 (与 XEdit 同).
		//   - 元素销毁 (XE_DESTROY 或 ~CXEditDW): 整个编辑框生命周期结束.
	}
	m_text.erase(pos, len);
	m_charStyle.erase(m_charStyle.begin() + pos, m_charStyle.begin() + pos + len);
	// 同 InsertCharsRaw, 只释放跨越段的 layout.
	ParaOnTextErased(pos, len);
}

// ===== 公开样式 API =====
int CXEditDW::AddStyle(HXCGUI hFontImageObj, COLORREF color, BOOL bColor){
	_XEditDW_Style ns;
	ns.type = 0;  // 默认按字体
	ns.nRef = 0;
	ns.hFontImageObj = hFontImageObj;
	ns.color = color;
	ns.bColor = bColor;
	// 通过 XCGUI 类型识别确定 type. 对于 NULL 句柄保留 type=0(纯颜色文本样式).
	if (hFontImageObj){
		XC_OBJECT_TYPE t = XC_GetObjectType(hFontImageObj);
		if (t == XC_FONT) ns.type = 0;
		else if (t == XC_IMAGE_TEXTURE) ns.type = 1;
		else ns.type = 2;  // 其余视为 UI 对象 (XC_BUTTON / XC_ELE / ...)
	}
	// 优先复用已被 DeleteStyle 释放的槽.
	for (int i = 0; i < (int)m_styleTable.size(); ++i){
		if (m_styleTable[i].type == 0xFFFF){
			m_styleTable[i] = ns;
			return i;
		}
	}
	m_styleTable.push_back(ns);
	return (int)m_styleTable.size() - 1;
}

int CXEditDW::AddStyleEx(const wchar_t* fontName, int fontSize, int fontStyle, COLORREF color, BOOL bColor){
	HFONTX hFont = XFont_CreateEx(fontName ? fontName : L"Segoe UI", fontSize, fontStyle);
	if (!hFont) return -1;
	return AddStyle((HXCGUI)hFont, color, bColor);
}

BOOL CXEditDW::ModifyStyle(int iStyle, HFONTX hFont, COLORREF color, BOOL bColor){
	if (!IsStyleIdValid(iStyle)) return FALSE;
	_XEditDW_Style& s = m_styleTable[iStyle];
	// 传入新字体 = 所有权转移给本类. 需释放旧字体 (避免泄漏), 并存新句柄. 传 NULL 保留原字体.
	if (hFont && s.type == 0){
		if (s.hFontImageObj) XFont_Release((HFONTX)s.hFontImageObj);
		s.hFontImageObj = (HXCGUI)hFont;
	}
	s.color = color;
	s.bColor = bColor;
	InvalidateLayout();
	RedrawSelf();
	return TRUE;
}

BOOL CXEditDW::DeleteStyle(int iStyle){
	if (!IsStyleIdValid(iStyle)) return FALSE;
	if (m_styleTable[iStyle].nRef > 0) return FALSE;  // 仍被字符引用 (含撤销栈引用)
	ReleaseStyleSlot(iStyle);
	return TRUE;
}

// 单条样式槽释放: 仅做 *句柄释放 + 槽位标记*. 不动 nRef, 也不清 m_charStyle/undo 引用.
// 调用前应保证 nRef==0 或调用方明确接受 *悬挂引用* (如 SetText 的硬重置, 撤销栈即将一并清).
void CXEditDW::ReleaseStyleSlot(int iStyle){
	if (iStyle < 0 || iStyle >= (int)m_styleTable.size()) return;
	_XEditDW_Style& s = m_styleTable[iStyle];
	if (s.type == 0xFFFF) return;       // 已释放过
	if (s.hFontImageObj){
		if      (s.type == 0) XFont_Release ((HFONTX)s.hFontImageObj);
		else if (s.type == 1) XImage_Release((HIMAGE)s.hFontImageObj);
		else if (s.type == 2){
			// type=2 的 UI 对象是 m_hEle 的子元素, 父被销毁时 XCGUI 会自动级联.
			// 这里只在 *句柄仍有效* 时主动 Destroy, 防止双路径重复销毁. 元素 / 形状各
			// 走自己的 API 族 (XEle_Destroy vs XShape_Destroy); 错调会被 XCGUI 拒并报错.
			if      (XC_IsHELE (s.hFontImageObj)) XEle_Destroy  ((HELE)s.hFontImageObj);
			else if (XC_IsShape(s.hFontImageObj)) XShape_Destroy(s.hFontImageObj);
		}
		s.hFontImageObj = NULL;
	}
	s.type = 0xFFFF;
	if (m_curStyle == iStyle) m_curStyle = -1;
	if (iStyle >= 0 && iStyle < (int)m_styleFontBindings.size())
		_EditDW_ReleaseFontBinding(m_styleFontBindings[iStyle]);
}

// 克隆 inline 对象, 给 Copy+Paste 多次粘贴用. 每次粘贴需要一个 *独立 HXCGUI 实例*,
// 否则多个 \uFFFC 共享同一句柄, PositionInlineObjects 只能定位最后一个. 当前内置:
//   - XC_SHAPE_TEXT: 新建一个同尺寸/文本/颜色的形状文本.
//   - XC_SHAPE_PICTURE: 新建一个同尺寸的形状图片, *共享* 原 HIMAGE - 不调 XImage_AddRef.
//     实测 (CopyFrom + 2 张缩略图场景): XShapePic_SetImage 内部已 AddRef, 调用方再
//     AddRef 就是 +2; src 销毁 -1, dst 销毁 -1, 余 1 ref 永不归零 → 程序退出报
//     "图片(2) 其他(2)" 泄漏 (其他=与 image 伴生的 GDI+/DIB wrapper). 与 InsertImageThumb
//     (不调 AddRef 也不漏) 的语义对齐.
// 其他类型 (HELE 各种控件 / 其他 shape) 返 NULL: 上层退化为共享原句柄 (老行为).
// 用户需要为自家控件提供克隆? 后续可扩展为 callback 注册.
void CXEditDW::CopyCommonEleProps(HELE hSrc, HELE hDst){
	if (!hSrc || !hDst) return;
	// HBKM 复制: 仅 XEle_SetBkManager(hDst, hBkM) - *不* 再手动 XBkM_AddRef.
	//
	// 历史: 之前路径手动 XBkM_AddRef + SetBkManager, 实测 *程序关闭时 XCGUI 报背景对象未释放*.
	// 根因: SetBkManager 内部本身就把 hDst 关联到 HBKM (ref +1); 我们再 AddRef 一次成 +2,
	// 元素销毁路径只 Release 1 次 → 余 1 ref 永不归零 = 资源泄漏.
	// 修复: 删 AddRef. 共享同一 HBKM 的 src/dst 两元素销毁时各 Release 一次, 总 -2 与
	// SetBkManager 的两次 +1 抵消, 引用计数到 0 自然销毁.
	// 安全性: 若 src 先销毁 (-1), 仅当 dst 还在 (-1 未触发, ref=1) 时 HBKM 仍存活, dst 继续
	// 显示无碍; dst 也销毁时 ref→0 真销毁. 与 XCGUI 主题共享按钮 BkM 的常规模式一致.
	HBKM hBkM = XEle_GetBkManager(hSrc);
	if (hBkM){
		XEle_SetBkManager(hDst, hBkM);
	}
	// 文本颜色: XEle 级别, 所有 HELE 都吃. GetTextColor 走非 Ex (不读资源覆盖), 与 Set 对称.
	XEle_SetTextColor(hDst, XEle_GetTextColor(hSrc));
}

HXCGUI CXEditDW::CloneInlineHandle(HXCGUI hSrc){
	if (!hSrc || !m_hEle) return NULL;
	XC_OBJECT_TYPE t = XC_GetObjectType(hSrc);
	switch (t){
	case XC_SHAPE_TEXT: {
		int w = XShape_GetWidth (hSrc);
		int h = XShape_GetHeight(hSrc);
		const wchar_t* text = XShapeText_GetText(hSrc);
		HXCGUI hNew = XShapeText_Create(0, 0, w, h, text ? text : L"", (HXCGUI)m_hEle);
		if (hNew){
			XShapeText_SetTextColor(hNew, XShapeText_GetTextColor(hSrc));
		}
		return hNew;
	}
	case XC_SHAPE_PICTURE: {
		int w = XShape_GetWidth (hSrc);
		int h = XShape_GetHeight(hSrc);
		HXCGUI hNew = XShapePic_Create(0, 0, w, h, (HXCGUI)m_hEle);
		if (hNew){
			HIMAGE hImg = XShapePic_GetImage(hSrc);
			if (hImg){
				// 不调 XImage_AddRef: XShapePic_SetImage 内部已经 AddRef, 调用方再 AddRef
				// 会导致 ref 永远 +1 残留 (见上方函数头部块注释 - CopyFrom 漏 image 根因).
				XShapePic_SetImage(hNew, hImg);
			}
		}
		return hNew;
	}
	// HELE 类: 这五种是常见 inline 可视控件. 其它 (XEdit / XListBox / XTreeBox 等) 不在
	// 这里做克隆 - 内部状态过于复杂, 用户应在自己业务侧实现.
	case XC_BUTTON: {
		int w = XEle_GetWidth ((HELE)hSrc);
		int h = XEle_GetHeight((HELE)hSrc);
		const wchar_t* text = XBtn_GetText((HELE)hSrc);
		HELE hNew = XBtn_Create(0, 0, w, h, text ? text : L"", (HXCGUI)m_hEle);
		if (hNew){
			HIMAGE hIcon = XBtn_GetIcon((HELE)hSrc, 0);
			if (hIcon){
				XImage_AddRef(hIcon);                 // 多按钮共享同一图源, 安全
				XBtn_SetIcon(hNew, hIcon);
			}
			CopyCommonEleProps((HELE)hSrc, hNew);
		}
		return (HXCGUI)hNew;
	}
	case XC_TEXTLINK: {
		// 文本链接继承自按钮, 文本 / 图标走 XBtn_*. 颜色等附加状态量多, 暂只复制基础信息.
		int w = XEle_GetWidth ((HELE)hSrc);
		int h = XEle_GetHeight((HELE)hSrc);
		const wchar_t* text = XBtn_GetText((HELE)hSrc);
		HELE hNew = XTextLink_Create(0, 0, w, h, text ? text : L"", (HXCGUI)m_hEle);
		if (hNew){
			HIMAGE hIcon = XBtn_GetIcon((HELE)hSrc, 0);
			if (hIcon){
				XImage_AddRef(hIcon);
				XBtn_SetIcon(hNew, hIcon);
			}
			CopyCommonEleProps((HELE)hSrc, hNew);
		}
		return (HXCGUI)hNew;
	}
	case XC_ELE: {
		// 通用元素: 主要打背景 / 文本颜色用 - CopyCommonEleProps 是全部 能带的属性.
		int w = XEle_GetWidth ((HELE)hSrc);
		int h = XEle_GetHeight((HELE)hSrc);
		HELE hNew = XEle_Create(0, 0, w, h, (HXCGUI)m_hEle);
		if (hNew){
			CopyCommonEleProps((HELE)hSrc, hNew);
		}
		return (HXCGUI)hNew;
	}
	case XC_SLIDERBAR: {
		int w = XEle_GetWidth ((HELE)hSrc);
		int h = XEle_GetHeight((HELE)hSrc);
		HELE hNew = XSliderBar_Create(0, 0, w, h, (HXCGUI)m_hEle);
		if (hNew){
			XSliderBar_SetRange(hNew, XSliderBar_GetRange((HELE)hSrc));
			XSliderBar_SetPos  (hNew, XSliderBar_GetPos  ((HELE)hSrc));
			CopyCommonEleProps((HELE)hSrc, hNew);
		}
		return (HXCGUI)hNew;
	}
	case XC_PROGRESSBAR: {
		int w = XEle_GetWidth ((HELE)hSrc);
		int h = XEle_GetHeight((HELE)hSrc);
		HELE hNew = XProgBar_Create(0, 0, w, h, (HXCGUI)m_hEle);
		if (hNew){
			XProgBar_SetRange(hNew, XProgBar_GetRange((HELE)hSrc));
			XProgBar_SetPos  (hNew, XProgBar_GetPos  ((HELE)hSrc));
			CopyCommonEleProps((HELE)hSrc, hNew);
		}
		return (HXCGUI)hNew;
	}
	default:
		return NULL;
	}
}

// 全清: SetText / 析构 共用. 顺序很重要 - 先把所有 *引用方* (m_charStyle + undo + redo) 清掉,
// 这样后面释放样式槽时不会出现 "释放了句柄但 m_charStyle 里还能查到 styleId" 的悬挂状态.
void CXEditDW::ReleaseAllStyleResources(){
	m_undoStack.clear();
	m_redoStack.clear();
	// 节流错点同步重置: undo 栈空了, 下次 PushUndo 必须从空起点 (m_lastUndoCaret=-1 防合并).
	m_lastUndoTick  = 0;
	m_lastUndoCaret = -1;
	m_charStyle.clear();
	m_text.clear();
	for (int i = 0; i < (int)m_styleTable.size(); ++i){
		ReleaseStyleSlot(i);
	}
	m_styleTable.clear();
	m_curStyle = -1;
	// 内部剪贴板里残留的 styleId 已无效, 直接清, 防止 *SetText 后粘回* 走错路径.
	m_clipText.clear();
	m_clipStyles.clear();
	m_clipSeq = 0;
	// m_text 已经 clear, 段结构同步清掉. 之后第一次进入 EnsureLayout 会按 *新* m_text
	// 走 ParaRebuildFromText.
	ParaClear();
}

BOOL CXEditDW::GetStyleInfo(int iStyle, editdw_style_info_* info) const{
	if (!info) return FALSE;
	if (!IsStyleIdValid(iStyle)) return FALSE;
	const _XEditDW_Style& s = m_styleTable[iStyle];
	info->type             = s.type;
	info->nRef             = s.nRef;
	info->hFont_image_obj  = s.hFontImageObj;
	info->color            = s.color;
	info->bColor           = s.bColor;
	info->imagePath        = s.imagePath;   // CXText 支持 = std::wstring
	return TRUE;
}

void CXEditDW::SetCurStyle(int iStyle){
	if (iStyle < 0) m_curStyle = -1;
	else if (IsStyleIdValid(iStyle)) m_curStyle = iStyle;
}

int CXEditDW::GetCurStyle() const{
	return m_curStyle;
}

void CXEditDW::AddTextEx(const wchar_t* pString, int iStyle){
	if (!pString || !*pString) return;
	int len = (int)wcslen(pString);
	int pos = (int)m_text.size();
	InsertCharsRaw(pos, pString, len, iStyle);
	m_caret = (int)m_text.size();
	m_anchor = m_caret;
	EnsureLayout();
	RedrawSelf();
}

void CXEditDW::InsertTextEx(int pos, const wchar_t* pString, int iStyle){
	if (!pString || !*pString) return;
	int len = (int)wcslen(pString);
	int n = (int)m_text.size();
	if (pos < 0) pos = 0;
	if (pos > n) pos = n;
	InsertCharsRaw(pos, pString, len, iStyle);
	m_caret = pos + len;
	m_anchor = m_caret;
	EnsureLayout();
	RedrawSelf();
}

int CXEditDW::AddObject(HXCGUI hObj){
	if (!hObj) return -1;
	int sid = AddStyle(hObj, 0, FALSE);
	if (sid < 0) return -1;
	wchar_t orc[2] = { kObjectReplacementChar, 0 };
	InsertCharsRaw((int)m_text.size(), orc, 1, sid);
	m_caret = (int)m_text.size();
	m_anchor = m_caret;
	EnsureLayout();
	RedrawSelf();
	return sid;
}

void CXEditDW::InsertObject(int pos, HXCGUI hObj){
	if (!hObj) return;
	int n = (int)m_text.size();
	if (pos < 0) pos = 0;
	if (pos > n) pos = n;
	int sid = AddStyle(hObj, 0, FALSE);
	if (sid < 0) return;
	wchar_t orc[2] = { kObjectReplacementChar, 0 };
	InsertCharsRaw(pos, orc, 1, sid);
	m_caret = pos + 1;
	m_anchor = m_caret;
	EnsureLayout();
	RedrawSelf();
}

void CXEditDW::AddByStyle(int iStyle){
	if (!IsStyleIdValid(iStyle)) return;
	const _XEditDW_Style& s = m_styleTable[iStyle];
	if (s.type == 0) return;  // 字体样式不占字符位
	wchar_t orc[2] = { kObjectReplacementChar, 0 };
	InsertCharsRaw(m_caret, orc, 1, iStyle);
	m_caret += 1;
	m_anchor = m_caret;
	EnsureLayout();
	RedrawSelf();
}

// ===== 1:1 视觉拷贝 (聊天记录还原 / 编辑器镜像) =====
// 步骤: (1) 清 dst -> (2) 重建样式表, 建立 src_sid -> dst_sid 映射 -> (3) 复制 text +
// 重写 charStyle -> (4) 刷新布局.
//
// 字体 / 图片走 AddRef 共享 (XCGUI 内部引用计数, 两侧 ReleaseStyleSlot 各 -1 即可平衡).
// inline 对象走 CloneInlineHandle 重新建子元素 (父 = dst 的 m_hEle), 完全独立.
// 不支持克隆的 HELE 类型 (XEdit / XListBox 等) 当前留空 sidMap[k] = -1, 对应 \uFFFC 在
// dst 里没绑 inline obj, 会显示为占位字符. 用户业务侧可读 src.GetText 自行重建那些类型.
BOOL CXEditDW::CopyFrom(const CXEditDW& src){
	if (&src == this) return TRUE;
	if (!m_hEle) return FALSE;

	// (1) 清 dst. ReleaseAllStyleResources 会清 m_text + m_charStyle + 撤销/重做栈 +
	// 释放本表所有句柄, 这正是我们要的 "起点状态". 调用后 m_styleTable 为空.
	ReleaseAllStyleResources();
	m_caret = 0;
	m_anchor = 0;
	m_curStyle = -1;

	// (2) 重建样式表. sidMap[src 索引] = dst 索引; 无法迁移 / 已删的槽留 -1.
	std::vector<int> sidMap(src.m_styleTable.size(), -1);
	for (size_t k = 0; k < src.m_styleTable.size(); ++k){
		const _XEditDW_Style& s = src.m_styleTable[k];
		if (s.type == 0xFFFF) continue;   // 已被 DeleteStyle 标记的空槽, 跳过

		HXCGUI hForAddStyle = NULL;
		if (s.hFontImageObj){
			switch (s.type){
			case 0: {
				// 字体: deep copy. 不用 XFont_AddRef 共享 - XCGUI 内部 refcount + Release
				// 的具体时序对外不透明, 用户实测 AddRef 路径关窗时报 "XFont_Release 句柄无效".
				// 改读 font_info_ + XFont_CreateEx 重建一份独立 HFONTX, dst 与 src 各自管理,
				// ReleaseStyleSlot 一对一 Release, 不会重复释放.
				font_info_ fi; ZeroMemory(&fi, sizeof(fi));
				XFont_GetFontInfo((HFONTX)s.hFontImageObj, &fi);
				HFONTX hNewFont = XFont_CreateEx(fi.name[0] ? fi.name : L"Segoe UI",
				                                  fi.nSize > 0 ? fi.nSize : 14,
				                                  fi.nStyle);
				if (hNewFont){
					hForAddStyle = (HXCGUI)hNewFont;
				}
				// XFont_CreateEx 失败: 退化为纯颜色样式 (hForAddStyle = NULL, AddStyle 会建
				// type=0 + 颜色, 字体回落到 dst 的默认字体).
				break;
			}
			case 1:  // 图片: 仍用 AddRef 共享 (HIMAGE 是纹理数据, deep copy 代价高)
				XImage_AddRef((HIMAGE)s.hFontImageObj);
				hForAddStyle = s.hFontImageObj;
				break;
			case 2:  // inline 对象: 必须克隆出独立子元素 (父 = dst.m_hEle)
				hForAddStyle = CloneInlineHandle(s.hFontImageObj);
				if (!hForAddStyle){
					// 不支持的 HELE 类型: 不建样式槽, 对应 \uFFFC 在 dst 渲染为占位字符
					continue;
				}
				break;
			default:
				continue;
			}
		}
		// hForAddStyle 可能为 NULL (s.hFontImageObj == NULL 的纯颜色样式), AddStyle 会把
		// 该槽记成 type=0 + color, 还原 src 的颜色定义.
		int newSid = AddStyle(hForAddStyle, s.color, s.bColor);
		if (newSid >= 0){
			sidMap[k] = newSid;
			// 拷贝 imagePath: 拖入 / 粘贴的图片路径, 跨编辑器迁移时跟着走 (序列化用).
			// 其他类型这字段为空, 拷贝也无害.
			// 全局图片转存生效时, src 的 imagePath 可能位于 src 进程的临时位置 (例如 %TEMP%)
			// 或外部目录, 我们把它复制到 dst 端的转存目录后, dst 的 imagePath 改记新路径.
			// 没设全局目录 / src 已在目录下 → _PersistImageFile 直接返原路径, 行为同旧.
			if (newSid < (int)m_styleTable.size()){
				if (!s.imagePath.empty()){
					std::wstring eff = _PersistImageFile(s.imagePath.c_str());
					m_styleTable[newSid].imagePath = eff.empty() ? s.imagePath : eff;
				}else{
					m_styleTable[newSid].imagePath = s.imagePath;
				}
			}
		}
	}

	// (3) 复制文本 + 重写 charStyle. 这里直接赋值再纠正 sid - 比 InsertCharsRaw 走 100
	// 次插入快; nRef 在循环里走 StyleIncRef 显式递增 (跳过 -1 / 无效 sid).
	m_text = src.m_text;
	m_charStyle.assign(m_text.size(), -1);
	int n = (int)m_text.size();
	int srcN = (int)src.m_charStyle.size();
	int lim = (n < srcN) ? n : srcN;
	for (int i = 0; i < lim; ++i){
		int srcSid = src.m_charStyle[i];
		int dstSid = (srcSid >= 0 && srcSid < (int)sidMap.size()) ? sidMap[srcSid] : -1;
		m_charStyle[i] = dstSid;
		StyleIncRef(dstSid);   // 内部对 -1 / 无效 自动忽略
	}

	// (4) 光标到末尾 (与 SetText 一致的语义), 触发布局重建 + inline 子元素定位.
	m_caret = (int)m_text.size();
	m_anchor = m_caret;
	ParaRebuildFromText();   // m_text 整体赋值, 同 Undo/Redo, 段结构同步.
	EnsureLayout();
	RedrawSelf();
	return TRUE;
}

// 提取编辑框内容为内容原子序列 (不含样式). 详细规则见头文件 GetContents @备注.
// 算法: 单次 O(n) 扫 m_text, 维护 [runStart, i) 文本游程.
//   遇 \uFFFC : 若游程非空, 先 emit 文本(游程); 再 emit 图片 或 UI对象 (按样式表分流);
//               *不* 强制 emit 空文本 (image/object 自身已经是原子边界).
//   遇 \n     : 永远 emit 文本(游程, 可空) - 空行 / 行末换行靠这条规则保住.
//   末尾      : 若游程非空, emit 文本.
BOOL CXEditDW::GetContents(CXVector<editdw_content_item_>& out) const{
	out.clear();
	int n = (int)m_text.size();
	if (n == 0) return TRUE;

	int runStart = 0;
	auto pushText = [&](int end){
		editdw_content_item_ it;
		it.type    = editdw_content_type_text;
		it.hObject = NULL;
		if (end > runStart){
			std::wstring slice = m_text.substr(runStart, end - runStart);
			it.text = slice;       // CXText = std::wstring
		}
		out.push_back(std::move(it));
	};

	for (int i = 0; i < n; ++i){
		wchar_t c = m_text[i];
		if (c == kObjectReplacementChar){
			// 游程非空才 emit 文本; \uFFFC 紧接前一原子 (image/\n) 时不补空文本.
			if (runStart < i) pushText(i);
			// 解析 inline 样式 → 图片 / UI对象.
			int sid = (i < (int)m_charStyle.size()) ? m_charStyle[i] : -1;
			if (IsStyleIdValid(sid)){
				const _XEditDW_Style& s = m_styleTable[sid];
				// 图片判定: 优先 type=1 (老的直接 HIMAGE 渠道), 否则 type=2 但底层句柄是
				// XC_SHAPE_PICTURE + imagePath 非空 (InsertImageThumb / ClipboardPasteImage /
				// 拖入图片 的实际形态 - 创建的是 XShapePic shape, 落在 type=2 里, 但是
				// "图片" 概念上的内容, 必须按 image 暴露给 GetContents, 让调用方拿到 imagePath).
				bool isImage = false;
				if (s.type == 1){
					isImage = true;
				}
				else if (s.type == 2 && s.hFontImageObj && !s.imagePath.empty()
				         && XC_GetObjectType(s.hFontImageObj) == XC_SHAPE_PICTURE){
					isImage = true;
				}
				if (isImage){
					editdw_content_item_ it;
					it.type      = editdw_content_type_image;
					it.hObject   = NULL;
					it.imagePath = s.imagePath;   // 可能为空 (老数据 / 非路径渠道插入)
					out.push_back(std::move(it));
				} else if (s.type == 2 /*object*/){
					editdw_content_item_ it;
					it.type    = editdw_content_type_object;
					it.hObject = s.hFontImageObj;
					out.push_back(std::move(it));
				}
				// type=0 (字体) 出现在 \uFFFC 是异常状态, 跳过.
			}
			runStart = i + 1;
		}
		else if (c == L'\n'){
			// 永远 emit (可空) - 让调用方按 item 顺序还原行结构.
			pushText(i);
			runStart = i + 1;
		}
	}
	// 末尾游程: 仅在非空时 emit (避免文档以 \n 结尾时多挂一个空文本).
	if (runStart < n) pushText(n);
	return TRUE;
}

// ============================================================================
//  序列化 / 反序列化 (SaveToFile / LoadFromFile / SaveToMem / LoadFromMem)
// ============================================================================
// 二进制格式 v1, little-endian, 详细布局:
//
//   Header (16 字节):
//     Magic       4B   = "XDW1"   - 文件类型识别
//     Version     2B   = 1        - 文件格式版本
//     Flags       2B   = 0        - 保留位
//     TextLen     4B   int32      - m_text 长度 (wchar 数)
//     StyleCount  4B   int32      - 样式表条目数
//
//   Styles[StyleCount] (变长, 每条最少 8B):
//     Kind        1B   uint8      - 0=font / 1=image / 2=object / 0xFF=跳过
//     Color       4B   uint32     - COLORREF
//     BColor      1B   uint8      - 0/1 (布尔压缩)
//     Reserved    2B   uint16     - 0
//     -- Kind=0 (font): --
//       FontSize  4B   int32
//       FontStyle 4B   int32
//       NameLen   2B   uint16     - wchar 数
//       Name      NameLen * 2B    - UTF-16 字体名
//     -- Kind=1 (image): --
//       PathLen   4B   int32      - wchar 数
//       Path      PathLen * 2B    - UTF-16 原始全路径
//     -- Kind=2 (object): --
//       ObjType   4B   int32      - XC_OBJECT_TYPE
//       Width     4B   int32
//       Height    4B   int32
//       TextColor 4B   uint32     - XEle_GetTextColor / XShapeText_GetTextColor
//       TextLen   2B   uint16     - wchar 数 (SHAPE_TEXT/BUTTON/TEXTLINK 用)
//       Text      TextLen * 2B
//       Range     4B   int32      - SLIDERBAR/PROGRESSBAR 用; 其它为 0
//       Pos       4B   int32      - 同上
//     -- Kind=0xFF (跳过): -- 无额外字段, 加载时占位 (不绑 inline 对象, \uFFFC 显示为占位字符)
//
//   Text[TextLen]:   UTF-16 wchar 数组 (含 \uFFFC)
//   CharStyles[TextLen]: 每 wchar 一个 int32 styleId (-1=默认, 其它=Styles 数组下标)

static const char     kXdwMagic[4] = {'X','D','W','1'};
static const uint16_t kXdwVersion  = 1;

namespace {
// 流式写入缓冲. 全部以 LE 写 (Windows 平台默认是 LE, 直接 memcpy 即可).
struct SerCursor{
	std::vector<BYTE> buf;
	void writeBytes(const void* p, size_t n){
		if (n == 0) return;
		const BYTE* b = (const BYTE*)p;
		buf.insert(buf.end(), b, b + n);
	}
	void writeU8 (uint8_t  v){ buf.push_back(v); }
	void writeU16(uint16_t v){ writeBytes(&v, 2); }
	void writeU32(uint32_t v){ writeBytes(&v, 4); }
	void writeI32(int32_t  v){ writeBytes(&v, 4); }
	void writeWStr(const wchar_t* p, size_t n){ if (n) writeBytes(p, n * sizeof(wchar_t)); }
};

// 流式读取游标. ok=false 后所有读取变 no-op 返回默认值, 调用方在末尾一次性检查.
struct DeserCursor{
	const BYTE* p;
	size_t      remain;
	bool        ok;
	DeserCursor(const void* data, size_t size) : p((const BYTE*)data), remain(size), ok(true) {}
	bool readBytes(void* dst, size_t n){
		if (!ok || n > remain){ ok = false; return false; }
		memcpy(dst, p, n); p += n; remain -= n;
		return true;
	}
	uint8_t  readU8 (){ uint8_t  v = 0; readBytes(&v, 1); return v; }
	uint16_t readU16(){ uint16_t v = 0; readBytes(&v, 2); return v; }
	uint32_t readU32(){ uint32_t v = 0; readBytes(&v, 4); return v; }
	int32_t  readI32(){ int32_t  v = 0; readBytes(&v, 4); return v; }
	void readWStr(std::wstring& s, size_t n){
		s.clear();
		if (!ok || n == 0) return;
		size_t bytes = n * sizeof(wchar_t);
		if (bytes > remain){ ok = false; return; }
		s.assign((const wchar_t*)p, n);
		p += bytes; remain -= bytes;
	}
};

// 写一条样式. 失败 (不支持的类型 / 已删除槽) 写为 Kind=0xFF, 让加载方占位.
static void _WriteStyle(SerCursor& w, const _XEditDW_Style& s){
	// 判定 Kind: 优先识别 image (XC_SHAPE_PICTURE + imagePath 非空), 再 font/object.
	uint8_t kind = 0xFF;
	const bool deleted = (s.type == 0xFFFF);
	if (!deleted){
		if (s.type == 0){
			kind = 0;     // font
		}
		else if (s.type == 1){
			// 直接 HIMAGE 路径: 罕见 (InsertImageThumb 走 shape), 序列化为 image 但
			// 无 path → 加载时按 0xFF 占位 (HIMAGE 无法持久化).
			kind = (!s.imagePath.empty()) ? 1 : 0xFF;
		}
		else if (s.type == 2 && s.hFontImageObj){
			XC_OBJECT_TYPE t = XC_GetObjectType(s.hFontImageObj);
			if (t == XC_SHAPE_PICTURE && !s.imagePath.empty()){
				kind = 1;   // 走 image 路径 (按 path 重建)
			}
			else{
				// 内置可重建的 inline 对象 (与 CloneInlineHandle 支持集对齐).
				// SHAPE_PICTURE 无 imagePath 时仍走 kind=2: 没法重建图像数据, 但 *尺寸* 仍
				// 持久化, 加载方建一个空的占位 shape, layout 排版一致 (不会因为 inline 框
				// 消失而把后面的对象前移导致整段错位).
				switch (t){
				case XC_SHAPE_TEXT:
				case XC_SHAPE_PICTURE:
				case XC_BUTTON:
				case XC_TEXTLINK:
				case XC_ELE:
				case XC_SLIDERBAR:
				case XC_PROGRESSBAR:
					kind = 2;
					break;
				default:
					kind = 0xFF;
				}
			}
		}
	}

	// 公共首部
	w.writeU8 (kind);
	w.writeU32((uint32_t)s.color);
	w.writeU8 (s.bColor ? 1 : 0);
	w.writeU16(0);   // reserved

	if (kind == 0){
		// font: 从 HFONTX 反查 font_info_
		font_info_ fi = {};
		if (s.hFontImageObj) XFont_GetFontInfo((HFONTX)s.hFontImageObj, &fi);
		size_t nameLen = wcsnlen(fi.name, LF_FACESIZE);
		w.writeI32(fi.nSize);
		w.writeI32(fi.nStyle);
		w.writeU16((uint16_t)nameLen);
		w.writeWStr(fi.name, nameLen);
	}
	else if (kind == 1){
		const std::wstring& path = s.imagePath;
		w.writeI32((int32_t)path.size());
		w.writeWStr(path.c_str(), path.size());
	}
	else if (kind == 2){
		// object 路径: 一次性缓存 t / isEle / isShape, 避免下游 4 组分支再各自做 COM 判型.
		HXCGUI h = s.hFontImageObj;
		XC_OBJECT_TYPE t = XC_GetObjectType(h);
		const bool isEle   = !!XC_IsHELE(h);
		const bool isShape = !isEle && !!XC_IsShape(h);
		w.writeI32((int32_t)t);
		// 尺寸
		int wv = 0, hv = 0;
		if      (isEle  ){ wv = XEle_GetWidth ((HELE)h); hv = XEle_GetHeight((HELE)h); }
		else if (isShape){ wv = XShape_GetWidth(h);      hv = XShape_GetHeight(h);     }
		w.writeI32(wv);
		w.writeI32(hv);
		// 文本颜色: HELE 走 XEle_GetTextColor; SHAPE_TEXT 走 XShapeText_GetTextColor; SHAPE_PICTURE 无颜色.
		uint32_t tc = 0;
		if      (isEle)              tc = (uint32_t)XEle_GetTextColor((HELE)h);
		else if (t == XC_SHAPE_TEXT) tc = (uint32_t)XShapeText_GetTextColor(h);
		w.writeU32(tc);
		// 子文本 (SHAPE_TEXT / BUTTON / TEXTLINK)
		const wchar_t* pTxt = NULL;
		if      (t == XC_SHAPE_TEXT)                 pTxt = XShapeText_GetText(h);
		else if (t == XC_BUTTON || t == XC_TEXTLINK) pTxt = XBtn_GetText((HELE)h);
		size_t txtLen = pTxt ? wcslen(pTxt) : 0;
		if (txtLen > 0xFFFF) txtLen = 0xFFFF;
		w.writeU16((uint16_t)txtLen);
		w.writeWStr(pTxt, txtLen);
		// Range / Pos (仅 SLIDERBAR/PROGRESSBAR). 其它类型 = 0.
		int32_t range = 0, pos = 0;
		if      (t == XC_SLIDERBAR  ){ range = XSliderBar_GetRange((HELE)h); pos = XSliderBar_GetPos((HELE)h); }
		else if (t == XC_PROGRESSBAR){ range = XProgBar_GetRange  ((HELE)h); pos = XProgBar_GetPos  ((HELE)h); }
		w.writeI32(range);
		w.writeI32(pos);
	}
	// kind=0xFF 无额外字段
}
} // namespace

BOOL CXEditDW::SaveToMem(CXBytes& out) const{
	out.clear();
	SerCursor w;
	// Header
	w.writeBytes(kXdwMagic, 4);
	w.writeU16(kXdwVersion);
	w.writeU16(0);   // flags
	const int textLen   = (int)m_text.size();
	const int styleCnt  = (int)m_styleTable.size();
	w.writeI32(textLen);
	w.writeI32(styleCnt);
	// Styles
	for (int i = 0; i < styleCnt; ++i){
		_WriteStyle(w, m_styleTable[i]);
	}
	// Text
	if (textLen > 0) w.writeWStr(m_text.c_str(), textLen);
	// CharStyles
	for (int i = 0; i < textLen; ++i){
		int sid = (i < (int)m_charStyle.size()) ? m_charStyle[i] : -1;
		w.writeI32((int32_t)sid);
	}
	out.set(w.buf.empty() ? NULL : w.buf.data(), w.buf.size());
	return TRUE;
}

BOOL CXEditDW::SaveToFile(const wchar_t* pPath) const{
	if (!pPath || !*pPath) return FALSE;
	CXBytes bytes;
	if (!SaveToMem(bytes)) return FALSE;
	HANDLE h = ::CreateFileW(pPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
	                         FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return FALSE;
	DWORD written = 0;
	const BYTE* p = bytes.getPtr();
	DWORD n = (DWORD)bytes.getSize();
	BOOL  ok = TRUE;
	if (n > 0){
		ok = ::WriteFile(h, p, n, &written, NULL) && (written == n);
	}
	::CloseHandle(h);
	return ok;
}

// 反序列化路径. 失败时 *尽量* 保持现有内容 (但 deser 已经走到中途时无法回滚, 此情况
// 会清空到 "全空" 状态, 调用方应视为 FALSE 即可). magic/version/textLen 三段校验
// 在 *未触动现有状态* 的前提下完成, 这三关之前任何失败都不改原文档.
BOOL CXEditDW::LoadFromMem(const void* pData, size_t size){
	if (!pData || size < 16) return FALSE;   // header 至少 16 字节
	DeserCursor r(pData, size);

	// === Header 校验 (不触动现有状态) ===
	char magic[4] = {0};
	r.readBytes(magic, 4);
	if (!r.ok || memcmp(magic, kXdwMagic, 4) != 0) return FALSE;
	uint16_t ver   = r.readU16();
	uint16_t flags = r.readU16();
	(void)flags;
	if (!r.ok || ver != kXdwVersion) return FALSE;
	int32_t textLen  = r.readI32();
	int32_t styleCnt = r.readI32();
	if (!r.ok || textLen < 0 || styleCnt < 0) return FALSE;
	// 文本长度上限同 InsertCharsRaw 的内部上限 (5M wchars), 防御坏数据.
	if (textLen > 5 * 1024 * 1024) return FALSE;
	if (styleCnt > 1 * 1024 * 1024) return FALSE;

	// === 全清现有状态 (ReleaseAllStyleResources 已含: undo/redo/charStyle/text/styleTable/curStyle/ParaClear) ===
	ReleaseAllStyleResources();
	m_caret  = 0;
	m_anchor = 0;

	// === 读样式表 (按 Kind 重建句柄, 写入 m_styleTable, 失败的整条置 0xFFFF 空槽) ===
	// 索引映射: 文件 styleId → 内存 styleId. 因失败槽我们 *仍占位* 写入空槽, 索引保持一致.
	m_styleTable.reserve(styleCnt);
	for (int i = 0; i < styleCnt; ++i){
		uint8_t  kind   = r.readU8();
		uint32_t color  = r.readU32();
		uint8_t  bColor = r.readU8();
		(void)r.readU16();   // reserved

		_XEditDW_Style ns;
		ns.type          = 0xFFFF;
		ns.nRef          = 0;
		ns.hFontImageObj = NULL;
		ns.color         = (COLORREF)color;
		ns.bColor        = bColor ? TRUE : FALSE;

		if (kind == 0){
			int32_t  fontSize  = r.readI32();
			int32_t  fontStyle = r.readI32();
			uint16_t nameLen   = r.readU16();
			std::wstring name; r.readWStr(name, nameLen);
			if (r.ok){
				HFONTX hF = XFont_CreateEx(name.empty() ? L"Segoe UI" : name.c_str(),
				                           fontSize > 0 ? fontSize : (int)m_fontSize,
				                           fontStyle);
				if (hF){
					ns.type          = 0;
					ns.hFontImageObj = (HXCGUI)hF;
				}
			}
		}
		else if (kind == 1){
			int32_t pathLen = r.readI32();
			std::wstring path;
			if (pathLen >= 0) r.readWStr(path, (size_t)pathLen);
			if (r.ok && !path.empty()){
				// 走 InsertImageThumb 同款 _LoadFileAsThumb → XShapePic 路径, 但不入文本.
				int tw = 0, th = 0;
				HIMAGE hImg = _LoadFileAsThumb(path.c_str(), m_imageThumbMaxLong,
				                               m_imageThumbMaxSquare, tw, th);
				if (hImg && tw > 0 && th > 0){
					int rw = tw, rh = th;
					ComputeThumbSize(tw, th, rw, rh);
					HXCGUI hShape = XShapePic_Create(0, 0, rw, rh, (HXCGUI)m_hEle);
					if (hShape){
						XImage_SetDrawType(hImg, image_draw_type_fixed_ratio);
						XShapePic_SetImage(hShape, hImg);   // 转移 ref
						ns.type          = 2;
						ns.hFontImageObj = hShape;
						ns.imagePath     = path;
					}
					else{
						XImage_Release(hImg);
					}
				}
				else if (hImg){
					XImage_Release(hImg);
				}
				// 图片加载失败 → ns.type 仍为 0xFFFF, 加载方文本里 \uFFFC 会变占位字符
			}
		}
		else if (kind == 2){
			int32_t  objType   = r.readI32();
			int32_t  w_        = r.readI32();
			int32_t  h_        = r.readI32();
			uint32_t textColor = r.readU32();
			uint16_t txtLen    = r.readU16();
			std::wstring txt;  r.readWStr(txt, txtLen);
			int32_t  range     = r.readI32();
			int32_t  pos       = r.readI32();
			if (r.ok){
				if (w_ <= 0) w_ = 20;
				if (h_ <= 0) h_ = 20;
				// shape 走 hNew 直写; HELE 统一先进 hEle, 尾部一次性 SetTextColor 并转 hNew.
				HXCGUI hNew = NULL;
				HELE   hEle = NULL;
				switch ((XC_OBJECT_TYPE)objType){
				case XC_SHAPE_TEXT:
					hNew = XShapeText_Create(0, 0, w_, h_, txt.c_str(), (HXCGUI)m_hEle);
					if (hNew) XShapeText_SetTextColor(hNew, (COLORREF)textColor);
					break;
				case XC_SHAPE_PICTURE:
					// 占位 shape (无图像数据). HIMAGE 无导出 API, 没法 round-trip 图像.
					// 但保留 w/h 让 layout 排版与源端一致, 避免后面 inline 对象前移错位.
					hNew = XShapePic_Create(0, 0, w_, h_, (HXCGUI)m_hEle);
					break;
				case XC_BUTTON:
					hEle = XBtn_Create     (0, 0, w_, h_, txt.c_str(), (HXCGUI)m_hEle);
					break;
				case XC_TEXTLINK:
					hEle = XTextLink_Create(0, 0, w_, h_, txt.c_str(), (HXCGUI)m_hEle);
					break;
				case XC_ELE:
					hEle = XEle_Create     (0, 0, w_, h_,              (HXCGUI)m_hEle);
					break;
				case XC_SLIDERBAR:
					hEle = XSliderBar_Create(0, 0, w_, h_, (HXCGUI)m_hEle);
					if (hEle){ XSliderBar_SetRange(hEle, range); XSliderBar_SetPos(hEle, pos); }
					break;
				case XC_PROGRESSBAR:
					hEle = XProgBar_Create(0, 0, w_, h_, (HXCGUI)m_hEle);
					if (hEle){ XProgBar_SetRange (hEle, range); XProgBar_SetPos (hEle, pos); }
					break;
				default: break;
				}
				if (hEle){
					XEle_SetTextColor(hEle, (COLORREF)textColor);
					hNew = (HXCGUI)hEle;
				}
				if (hNew){
					ns.type          = 2;
					ns.hFontImageObj = hNew;
				}
				// 不支持的类型 → ns.type 仍为 0xFFFF (空槽占位)
			}
		}
		// kind=0xFF 或其它未知值: 直接占位空槽
		m_styleTable.push_back(ns);
	}
	// 三处失败路径共用一个清理: ReleaseAllStyleResources 内部已清 m_text / m_charStyle /
	// m_styleTable / undo / redo / para, 不再重复 .clear().
	if (!r.ok){ ReleaseAllStyleResources(); return FALSE; }

	// === 读文本 + charStyle ===
	std::wstring text; r.readWStr(text, (size_t)textLen);
	if (!r.ok){ ReleaseAllStyleResources(); return FALSE; }
	std::vector<int> charStyle((size_t)textLen, -1);
	for (int i = 0; i < textLen; ++i){
		int32_t sid = r.readI32();
		if (!r.ok) break;
		// 越界 / 指向空槽: 强制 -1 (默认样式), 文本里若是 \uFFFC 会变占位字符.
		if (sid < 0 || sid >= (int)m_styleTable.size() || m_styleTable[sid].type == 0xFFFF){
			charStyle[i] = -1;
		} else {
			charStyle[i] = sid;
		}
	}
	if (!r.ok){ ReleaseAllStyleResources(); return FALSE; }

	// === 提交到内部状态 ===
	m_text     = std::move(text);
	m_charStyle = std::move(charStyle);
	// 引用计数: 每个有效引用一次.
	for (int sid : m_charStyle){
		StyleIncRef(sid);
	}
	m_caret  = (int)m_text.size();
	m_anchor = m_caret;
	// ParaRebuildFromText 内部 ParaClear → m_layoutDirty=true, 且所有段 pLayout=NULL,
	// EnsureLayout 会全量重建. 不再额外调 InvalidateLayout (冗余).
	ParaRebuildFromText();
	EnsureLayout();
	RedrawSelf();
	return TRUE;
}

BOOL CXEditDW::LoadFromFile(const wchar_t* pPath){
	if (!pPath || !*pPath || !m_hEle) return FALSE;
	HANDLE h = ::CreateFileW(pPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return FALSE;
	LARGE_INTEGER li; li.QuadPart = 0;
	if (!::GetFileSizeEx(h, &li) || li.QuadPart <= 0 || li.QuadPart > (LONGLONG)(64 * 1024 * 1024)){
		// 上限 64MB, 防御坏数据 / 误传超大文件
		::CloseHandle(h);
		return FALSE;
	}
	std::vector<BYTE> buf((size_t)li.QuadPart);
	DWORD got = 0;
	BOOL  rd  = ::ReadFile(h, buf.data(), (DWORD)buf.size(), &got, NULL);
	::CloseHandle(h);
	if (!rd || got != buf.size()) return FALSE;
	return LoadFromMem(buf.data(), buf.size());
}

// ===== 把每个 inline UI 对象按当前 layout 放到正确像素位置 =====
// XEle_SetPosition 用的是 *父 (XEditDW) 局部逻辑* 坐标系, 且这个坐标系 *不含* 滚动偏移
// — XCGUI 在绘制带 XSView 的父时会自动把所有子元素按当前 scrollPos 平移. 所以这里如果再
// 减去一次 scroll (例如用 originX=rcEle.left+bs.leftSize*dpi-scrollX 算后转换), 会出现
// "我们减一次 + XCGUI 再加一次" 的双倍偏移, 表现就是: 滚动条一出现, 子元素位置错位.
//
// 正确做法: 取 *未叠加 scroll* 的 layout 原点 (= bs.leftSize / bs.topSize, 逻辑像素),
// 加上 (cx, cy) / dpiScale (HitTestTextPosition 返回物理像素, 相对于 layout 原点).
// 这样无论是否滚动, 子元素的 "在父中的逻辑位置" 都不变, 由 XCGUI 自动平移呈现.
void CXEditDW::PositionInlineObjects(float /*originX*/, float /*originY*/){
	if (!m_hEle) return;
	int n = (int)m_text.size();
	if ((int)m_charStyle.size() != n) return;
	// 不能再加 m_paragraphs.empty() 早退: 全选删除后 m_text 空, m_paragraphs 也空,
	// 但 step 1 必须走 (按 nRef 隐藏所有 type=2 inline 子控件), 不然子控件留在屏上不消失.

	// 步骤 1: 按 nRef 决定 type=2 (UI 对象) 子元素的 *显隐*. 与分段架构无关.
	for (size_t k = 0; k < m_styleTable.size(); ++k){
		const _XEditDW_Style& s = m_styleTable[k];
		if (s.type == 2 && s.hFontImageObj
		    && (XC_IsHELE(s.hFontImageObj) || XC_IsShape(s.hFontImageObj))){
			XWidget_Show(s.hFontImageObj, (s.nRef > 0) ? TRUE : FALSE);
		}
	}

	borderSize_ bs;
	XEle_GetBorderSize(m_hEle, &bs);
	// 单行模式垂直对齐偏移 (物理像素): 加到 layout 全局坐标的 y, 让 inline 子元素跟文本同步下移.
	const float voff = ContentVerticalOffsetPhys();

	// 步骤 2: 按段循环, 在每段内 walk inline 字符, 用 *段 layout* 的 HitTestTextPosition
	// (段内相对位置), 然后加上段的 yOffset 转回 layout 全局坐标. inline 子元素在父逻辑
	// 坐标系里定位 (不含 scroll, XCGUI 会自动平移).
	for (size_t pi = 0; pi < m_paragraphs.size(); ++pi){
		_XEditDW_Para& p = m_paragraphs[pi];
		if (!p.pLayout) continue;
		int paraEnd = p.textStart + p.textLen;
		for (int i = p.textStart; i < paraEnd; ++i){
			if (m_text[i] != kObjectReplacementChar) continue;
			int sid = m_charStyle[i];
			if (!IsStyleIdValid(sid)) continue;
			const _XEditDW_Style& s = m_styleTable[sid];
			if (s.type != 2 /*object*/ || !s.hFontImageObj) continue;
			bool isEle   = !!XC_IsHELE(s.hFontImageObj);
			bool isShape = !isEle && !!XC_IsShape(s.hFontImageObj);
			if (!isEle && !isShape) continue;

			int relPos = i - p.textStart;
			FLOAT cx = 0, cy = 0;
			DWRITE_HIT_TEST_METRICS m; ZeroMemory(&m, sizeof(m));
			HRESULT hr = p.pLayout->HitTestTextPosition((UINT32)relPos, FALSE, &cx, &cy, &m);
			if (FAILED(hr)) continue;
			// (cx, cy) 是段内物理像素, +p.yOffset 转 layout 全局; +voff 兼容单行垂直对齐.
			float gx = cx;
			float gy = cy + p.yOffset + voff;
			float vlx = (float)bs.leftSize + gx / m_dpiScale;
			float vly = (float)bs.topSize  + gy / m_dpiScale;

			if (isEle){
				XEle_SetPosition((HELE)s.hFontImageObj, (int)vlx, (int)vly, FALSE);
			}
			else{
				XShape_SetPosition(s.hFontImageObj, (int)vlx, (int)vly);
			}
		}
	}
}

//===================================================================
//  内部辅助
//===================================================================
int CXEditDW::ClampPos(int p) const{
	int n = (int)m_text.size();
	if (p < 0) p = 0;
	if (p > n) p = n;
	return p;
}

int CXEditDW::NextCodepoint(int p) const{
	int n = (int)m_text.size();
	if (p >= n) return n;
	if (IsHighSurrogate(m_text[p]) && p + 1 < n && IsLowSurrogate(m_text[p + 1])) return p + 2;
	return p + 1;
}

int CXEditDW::PrevCodepoint(int p) const{
	if (p <= 0) return 0;
	if (p >= 2 && IsLowSurrogate(m_text[p - 1]) && IsHighSurrogate(m_text[p - 2])) return p - 2;
	return p - 1;
}

bool CXEditDW::HasSelectionInner() const{
	return m_caret != m_anchor;
}

void CXEditDW::GetSelectionRangeInner(int& start, int& end) const{
	if (m_caret <= m_anchor){ start = m_caret; end = m_anchor; }
	else { start = m_anchor; end = m_caret; }
}

D2D1::ColorF CXEditDW::RgbaToD2D(COLORREF rgba){
	// XCGUI RGBA 字节序: R | G<<8 | B<<16 | A<<24
	return D2D1::ColorF(
		((rgba)       & 0xFF) / 255.0f,   // R
		((rgba >>  8) & 0xFF) / 255.0f,   // G
		((rgba >> 16) & 0xFF) / 255.0f,   // B
		((rgba >> 24) & 0xFF) / 255.0f);  // A
}

void CXEditDW::RedrawSelf(){
	XEle_Redraw(m_hEle, FALSE);
}

//===================================================================
//  分段 layout 维护 (核心: 每段独立 IDWriteTextLayout, 改一段只重 shape 那一段)
//===================================================================

void CXEditDW::ParaReleaseAllLayouts(){
	for (size_t i = 0; i < m_paragraphs.size(); ++i){
		if (m_paragraphs[i].pLayout){
			m_paragraphs[i].pLayout->Release();
			m_paragraphs[i].pLayout = NULL;
		}
		m_paragraphs[i].yOffset = 0;
		m_paragraphs[i].height = 0;
		m_paragraphs[i].width = 0;
	}
	m_paraTotalHeight = 0;
	m_paraMaxWidth = 0;
	m_layoutDirty = true;
}

void CXEditDW::ParaClear(){
	ParaReleaseAllLayouts();
	m_paragraphs.clear();
	m_layoutDirty = true;
}

// 从 m_text 完整重建段结构 (释放所有旧 layout). SetText / 加载时调.
// 段语义:
//  - 段 textLen **不包含** 末尾 \n (\n 本身不属任何段, 段间分隔符).
//  - 段数 = count('\n') + 1. "abc" → 1 段; "abc\n" → 2 段 (后一段空); "abc\nxyz" → 2 段.
//  - 这样 DirectWrite 看到的 layout 文本不含 \n, 不会渲染额外空行 -> 段间紧密排版.
//  - 末尾 \n 用幻影空段 (textLen=0) 表示, 让光标能停在 "abc\n" 的最后一行空行上.
// 空文档 → m_paragraphs 为空 (调用方处理 empty 分支, 不建空段).
void CXEditDW::ParaRebuildFromText(){
	ParaClear();
	int n = (int)m_text.size();
	if (n == 0) return;
	int segStart = 0;
	for (int i = 0; i < n; ++i){
		if (m_text[i] == L'\n'){
			_XEditDW_Para p;
			p.textStart = segStart;
			p.textLen   = i - segStart;  // 不含 \n
			p.pLayout   = NULL;
			p.yOffset   = 0;
			p.height    = 0;
			p.width     = 0;
			m_paragraphs.push_back(p);
			segStart = i + 1;
		}
	}
	// 最后一段 (可能为空, 当 m_text 以 \n 结尾时).
	_XEditDW_Para p;
	p.textStart = segStart;
	p.textLen   = n - segStart;
	p.pLayout   = NULL;
	p.yOffset   = 0;
	p.height    = 0;
	p.width     = 0;
	m_paragraphs.push_back(p);
	m_layoutDirty = true;
}

// 文本插入 hook (m_text 已经包含新字符时调). 仅 *增量更新* m_paragraphs:
//  1) 找 pos 所在段 k (pos 可在段范围内, 也可在末尾边界 textStart+textLen);
//  2) 释放段 k 的 pLayout (内容变了, 必须重 shape);
//  3) 把段 k 旧覆盖 [textStart, textStart+textLen) 扩展 newCharsLen → 新 scan 范围
//     [textStart, textStart+textLen+newCharsLen). 按 \n 切, \n 不计入新段 textLen.
//     末尾总 push 一段 (即使空, 当插入文本末尾是 \n 时表示后续幻影空段).
//  4) 替换 m_paragraphs[k] 为新批 (>=1 段), 后续段 textStart += newCharsLen.
// 受影响段数 = 1 (无 \n 插入) 或 1+\n 个数. 其他段 layout 缓存全部保留.
void CXEditDW::ParaOnTextInserted(int pos, int newCharsLen){
	if (newCharsLen <= 0) return;
	if (m_paragraphs.empty()){
		ParaRebuildFromText();
		return;
	}
	// 找 pos 所属段. 新不变量下: 段 k 覆盖 [textStart, textStart+textLen).
	// pos 可能 = textStart+textLen (恰好在 \n 位置或文档末尾, 仍归段 k) - 用 <= 边界.
	int k = -1;
	for (int i = 0; i < (int)m_paragraphs.size(); ++i){
		int ps = m_paragraphs[i].textStart;
		int pe = ps + m_paragraphs[i].textLen;
		if (pos >= ps && pos <= pe){ k = i; break; }
	}
	if (k < 0){ ParaRebuildFromText(); return; }

	// 段 k 原覆盖 [textStart, textStart+textLen) (textLen 不含 \n). 插入后扩展到
	// [textStart, textStart+textLen+newCharsLen). 这个 range 用作 \n 重切的输入.
	int newSpanStart = m_paragraphs[k].textStart;
	int newSpanEnd   = newSpanStart + m_paragraphs[k].textLen + newCharsLen;

	// 释放段 k 的 layout.
	if (m_paragraphs[k].pLayout){
		m_paragraphs[k].pLayout->Release();
		m_paragraphs[k].pLayout = NULL;
	}

	// 按 \n 切 [newSpanStart, newSpanEnd). 新不变量: textLen 不含 \n.
	std::vector<_XEditDW_Para> newSegs;
	int segStart = newSpanStart;
	for (int i = newSpanStart; i < newSpanEnd; ++i){
		if (m_text[i] == L'\n'){
			_XEditDW_Para np;
			np.textStart = segStart;
			np.textLen   = i - segStart;
			np.pLayout   = NULL;
			np.yOffset   = 0;
			np.height    = 0;
			np.width     = 0;
			newSegs.push_back(np);
			segStart = i + 1;
		}
	}
	// 最后段 (可能空, 当插入文本末尾是 \n 时).
	{
		_XEditDW_Para np;
		np.textStart = segStart;
		np.textLen   = newSpanEnd - segStart;
		np.pLayout   = NULL;
		np.yOffset   = 0;
		np.height    = 0;
		np.width     = 0;
		newSegs.push_back(np);
	}

	m_paragraphs[k] = newSegs[0];
	if (newSegs.size() > 1){
		m_paragraphs.insert(m_paragraphs.begin() + k + 1,
		                    newSegs.begin() + 1, newSegs.end());
	}

	// 后续段的 textStart 偏移 +newCharsLen.
	int afterIdx = k + (int)newSegs.size();
	for (int i = afterIdx; i < (int)m_paragraphs.size(); ++i){
		m_paragraphs[i].textStart += newCharsLen;
	}
	m_layoutDirty = true;
}

// 文本删除 hook (m_text 已经少了 eraseLen 个字符时调). 增量更新:
//  1) 找删除区间 [pos, pos+eraseLen) (旧坐标) 跨越的段 k..m. 注意 \n 分隔符不属任何段,
//     若 \n 在删除区间内, paraK-1 / paraM+1 也要被合并进来 (k/m 双向扩展).
//  2) 释放 k..m 各段 layout, 把 k+1..m 段从列表删掉;
//  3) 段 k 的范围 *合并* 到 [textStart, mergedNewEnd), 按 \n 重切. \n 不计入新段 textLen.
//  4) 后续段 textStart -= eraseLen.
void CXEditDW::ParaOnTextErased(int pos, int eraseLen){
	if (eraseLen <= 0) return;
	if (m_paragraphs.empty()) return;

	// 全删空特例: 直接清段, 与 ParaRebuildFromText 空文档分支保持一致.
	if (m_text.empty()){
		ParaClear();
		return;
	}

	int delStart = pos;
	int delEnd   = pos + eraseLen;

	// 初始 k = 删除区间起点所在段. m = 删除区间最后一字符所在段.
	int k = FindParagraphByTextPos(delStart);
	if (k < 0){ ParaRebuildFromText(); return; }
	int lastChar = delEnd - 1;
	int m = (lastChar >= delStart) ? FindParagraphByTextPos(lastChar) : k;
	if (m < k) m = k;
	if (m >= (int)m_paragraphs.size()) m = (int)m_paragraphs.size() - 1;

	// 向后扩展 m: 段 m 与下一段之间的 \n 分隔符在 OLD m_text 的位置 = paraM.textStart + paraM.textLen.
	// 若该 \n 落在 [delStart, delEnd) 删除区间内, 表示 \n 被删掉 - paraM 与 paraM+1 必须合并.
	while (m + 1 < (int)m_paragraphs.size()){
		int sepPos = m_paragraphs[m].textStart + m_paragraphs[m].textLen;
		if (sepPos >= delStart && sepPos < delEnd) ++m;
		else break;
	}
	// 向前扩展 k: 段 k 与上一段之间的 \n 分隔符在 OLD m_text 的位置 = paraK.textStart - 1.
	while (k > 0){
		int sepPos = m_paragraphs[k].textStart - 1;
		if (sepPos >= delStart && sepPos < delEnd) --k;
		else break;
	}

	int mergedStart  = m_paragraphs[k].textStart;
	int mergedNewEnd = m_paragraphs[m].textStart + m_paragraphs[m].textLen - eraseLen;

	// 释放 [k, m] 段 layouts.
	for (int i = k; i <= m; ++i){
		if (m_paragraphs[i].pLayout){
			m_paragraphs[i].pLayout->Release();
			m_paragraphs[i].pLayout = NULL;
		}
	}
	// 删 k+1..m.
	if (m > k){
		m_paragraphs.erase(m_paragraphs.begin() + k + 1,
		                   m_paragraphs.begin() + m + 1);
	}

	// 按 \n 切 [mergedStart, mergedNewEnd). 新不变量: textLen 不含 \n, 末尾总 push 一段.
	std::vector<_XEditDW_Para> newSegs;
	int segStart = mergedStart;
	for (int i = mergedStart; i < mergedNewEnd; ++i){
		if (m_text[i] == L'\n'){
			_XEditDW_Para np;
			np.textStart = segStart;
			np.textLen   = i - segStart;
			np.pLayout   = NULL;
			np.yOffset   = 0;
			np.height    = 0;
			np.width     = 0;
			newSegs.push_back(np);
			segStart = i + 1;
		}
	}
	{
		_XEditDW_Para np;
		np.textStart = segStart;
		np.textLen   = mergedNewEnd - segStart;
		np.pLayout   = NULL;
		np.yOffset   = 0;
		np.height    = 0;
		np.width     = 0;
		newSegs.push_back(np);
	}

	m_paragraphs[k] = newSegs[0];
	if (newSegs.size() > 1){
		m_paragraphs.insert(m_paragraphs.begin() + k + 1,
		                    newSegs.begin() + 1, newSegs.end());
	}

	// 后续段 textStart 偏移 -eraseLen.
	int afterIdx = k + (int)newSegs.size();
	for (int i = afterIdx; i < (int)m_paragraphs.size(); ++i){
		m_paragraphs[i].textStart -= eraseLen;
	}
	m_layoutDirty = true;
}

void CXEditDW::ParaInvalidateLayoutsForRange(int pos, int len){
	if (len <= 0 || m_paragraphs.empty()) return;
	int endPos = pos + len;
	if (pos < 0) pos = 0;
	int startPara = FindParagraphByTextPos(pos);
	if (startPara < 0) return;
	for (int pi = startPara; pi < (int)m_paragraphs.size(); ++pi){
		_XEditDW_Para& p = m_paragraphs[pi];
		if (p.textStart >= endPos) break;
		int paraEnd = p.textStart + p.textLen;
		if (paraEnd <= pos) continue;
		if (p.pLayout){
			p.pLayout->Release();
			p.pLayout = NULL;
		}
		p.height = 0;
		p.width  = 0;
	}
	m_layoutDirty = true;
}

// 确保段 idx 的 layout 已构建 (CreateTextLayout + ApplyStyles + GetMetrics).
// 已建则跳过. 关键: 这是 *热路径* 单段重建入口, 单段 cost ~几百微秒到几 ms.
void CXEditDW::EnsureParagraphLayout(int paraIdx){
	if (paraIdx < 0 || paraIdx >= (int)m_paragraphs.size()) return;
	_XEditDW_Para& p = m_paragraphs[paraIdx];
	if (p.pLayout) return;
	if (!m_pDWFactory || !m_pTextFormat) return;

	const wchar_t* txt = m_text.c_str() + p.textStart;
	UINT32 len = (UINT32)p.textLen;
	// 段宽度选择:
	//  - 多行 + autoWrap: 内容宽 (内部行内换行)
	//  - 单行 (m_multiLine=false): 视口宽; SetTextAlignment 才有参考边界 (DWRITE 在 maxW
	//    内做对齐). 文本超 maxW 时 DWRITE 仍允许 overflow, 滚动条按真实 width 出现.
	//  - 多行 + 不 wrap: 大数 (历史行为, 不变)
	float maxW;
	if (m_multiLine && m_wrap)      maxW = GetContentWidth();
	else if (!m_multiLine)          maxW = GetContentWidth();
	else                            maxW = 1000000.0f;
	float maxH = 1000000.0f;
	HRESULT hr = m_pDWFactory->CreateTextLayout(txt, len, m_pTextFormat, maxW, maxH, &p.pLayout);
	if (FAILED(hr) || !p.pLayout) return;

	// 用户配置统一行高 → 强制 UNIFORM, 取代默认按 font metrics 计算的 ascent+descent+leading.
	// baseline 取 80% 行高作为字形基线 (DW 推荐起步点; 太小会让顶部贴边, 太大会撞下行).
	// 行高乘 m_dpiScale 转物理像素, 与 layout 其他度量一致.
	// 例外: 段内含 inline 对象 (U+FFFC, 图片 / UI 对象) 时, 这些对象的真实高度通常远大于
	// 文本行高, UNIFORM 会把它们挤扁成文本行高 (用户截图: 插入图片整体塌陷成一行高).
	// 这种段保持 DirectWrite DEFAULT 行高, 由 inline object 的 metrics 自动撑开行高.
	if (m_lineSpacing > 0.0f){
		bool hasInline = false;
		for (UINT32 k = 0; k < len; ++k){
			if (txt[k] == kObjectReplacementChar){ hasInline = true; break; }
		}
		if (!hasInline){
			FLOAT lineH = m_lineSpacing * m_dpiScale;
			p.pLayout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, lineH, lineH * 0.8f);
		}
	}

	// 单行模式按 m_textAlign 的水平位选 DWRITE alignment. 多行模式永远 LEADING (顶左).
	if (!m_multiLine){
		DWRITE_TEXT_ALIGNMENT ha = DWRITE_TEXT_ALIGNMENT_LEADING;
		const int hMask = m_textAlign & (edit_textAlign_flag_right | edit_textAlign_flag_center);
		if      (hMask & edit_textAlign_flag_center) ha = DWRITE_TEXT_ALIGNMENT_CENTER;
		else if (hMask & edit_textAlign_flag_right)  ha = DWRITE_TEXT_ALIGNMENT_TRAILING;
		p.pLayout->SetTextAlignment(ha);
	}

	ApplyStylesToParagraph(paraIdx);

	DWRITE_TEXT_METRICS tm;
	ZeroMemory(&tm, sizeof(tm));
	p.pLayout->GetMetrics(&tm);
	p.height = tm.height;
	p.width  = tm.widthIncludingTrailingWhitespace;
}

void CXEditDW::ApplyStylesToParagraph(int paraIdx){
	if (paraIdx < 0 || paraIdx >= (int)m_paragraphs.size()) return;
	_XEditDW_Para& p = m_paragraphs[paraIdx];
	if (!p.pLayout) return;

	if (m_styleTable.empty()){
		if (m_useFontHandle && m_hFontX){
			EnsureFontBinding();
			if (m_fontBinding.valid && m_fontBinding.pCollection){
				DWRITE_TEXT_RANGE fullRange = { 0, (UINT32)p.textLen };
				p.pLayout->SetFontCollection(m_fontBinding.pCollection, fullRange);
			}
		}
		return;
	}

	int paraStart = p.textStart;
	int paraEnd   = paraStart + p.textLen;
	int n = (int)m_text.size();
	if ((int)m_charStyle.size() != n) m_charStyle.resize(n, -1);

	int i = paraStart;
	while (i < paraEnd){
		int curStyle = m_charStyle[i];
		bool inlineObj = (m_text[i] == kObjectReplacementChar);
		int j = i + 1;
		if (!inlineObj){
			while (j < paraEnd && m_charStyle[j] == curStyle && m_text[j] != kObjectReplacementChar) ++j;
		}
		// range 是 *段内* 偏移 (减去 paraStart).
		DWRITE_TEXT_RANGE range = { (UINT32)(i - paraStart), (UINT32)(j - i) };

		if (inlineObj && IsStyleIdValid(curStyle)){
			const _XEditDW_Style& s = m_styleTable[curStyle];
			int wL = 0, hL = 0;
			if (s.type == 2 /*object*/ && s.hFontImageObj){
				if (XC_IsHELE(s.hFontImageObj)){
					wL = XEle_GetWidth ((HELE)s.hFontImageObj);
					hL = XEle_GetHeight((HELE)s.hFontImageObj);
				}
				else if (XC_IsShape(s.hFontImageObj)){
					wL = XShape_GetWidth (s.hFontImageObj);
					hL = XShape_GetHeight(s.hFontImageObj);
				}
				if (wL <= 0) wL = 50;
				if (hL <= 0) hL = 20;
			}
			else if (s.type == 1 /*image*/ && s.hFontImageObj){
				wL = XImage_GetWidth((HIMAGE)s.hFontImageObj);
				hL = XImage_GetHeight((HIMAGE)s.hFontImageObj);
				if (wL <= 0) wL = 16;
				if (hL <= 0) hL = 16;
			}
			else{
				wL = 16; hL = 16;
			}
			int wP = (int)((float)wL * m_dpiScale);
			int hP = (int)((float)hL * m_dpiScale);
			IDWriteInlineObject* pInline = new _CXEditDW_InlineObj(wP, hP);
			p.pLayout->SetInlineObject(pInline, range);
			pInline->Release();
		}
		else if (IsStyleIdValid(curStyle)){
			const _XEditDW_Style& s = m_styleTable[curStyle];
			if (s.type == 0 /*font*/){
				IDWriteFontFace* pStyleFace = NULL;
				if (s.hFontImageObj){
					EnsureStyleFontBinding(curStyle);
					if (curStyle >= 0 && curStyle < (int)m_styleFontBindings.size()
						&& m_styleFontBindings[curStyle].valid){
						const _EditDW_FontBinding& sb = m_styleFontBindings[curStyle];
						if (sb.pCollection){
							p.pLayout->SetFontCollection(sb.pCollection, range);
						}
						if (sb.logFont.lfFaceName[0]){
							p.pLayout->SetFontFamilyName(sb.logFont.lfFaceName, range);
						}
						pStyleFace = sb.pFace;
					}
					font_info_ fi; ZeroMemory(&fi, sizeof(fi));
					XFont_GetFontInfo((HFONTX)s.hFontImageObj, &fi);
					if (fi.nSize > 0){
						p.pLayout->SetFontSize((FLOAT)fi.nSize * m_dpiScale, range);
					}
					DWRITE_FONT_WEIGHT w = (fi.nStyle & fontStyle_bold) ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
					DWRITE_FONT_STYLE  st = (fi.nStyle & fontStyle_italic) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
					p.pLayout->SetFontWeight(w, range);
					p.pLayout->SetFontStyle(st, range);
					p.pLayout->SetUnderline((fi.nStyle & fontStyle_underline) ? TRUE : FALSE, range);
					p.pLayout->SetStrikethrough((fi.nStyle & fontStyle_strikeout) ? TRUE : FALSE, range);
				}
				if (s.bColor || pStyleFace){
					COLORREF drawCol = s.bColor ? s.color : m_textColor;
					_CXEditDW_ColorEffect* eff = new _CXEditDW_ColorEffect(drawCol, pStyleFace);
					p.pLayout->SetDrawingEffect(eff, range);
					eff->Release();
				}
			}
		}
		else if (curStyle < 0 && m_useFontHandle && m_hFontX){
			EnsureFontBinding();
			if (m_fontBinding.valid && m_fontBinding.pCollection){
				p.pLayout->SetFontCollection(m_fontBinding.pCollection, range);
			}
		}
		i = j;
	}
}

void CXEditDW::RecomputeParaYOffsets(){
	float y = 0;
	float maxW = 0;
	for (size_t i = 0; i < m_paragraphs.size(); ++i){
		m_paragraphs[i].yOffset = y;
		y += m_paragraphs[i].height;
		if (m_paragraphs[i].width > maxW) maxW = m_paragraphs[i].width;
	}
	m_paraTotalHeight = y;
	m_paraMaxWidth    = maxW;
}

// 二分: 段按 textStart 单调升序, range 是 [textStart, textStart+textLen). 返回最后一个
// textStart <= textPos 的段索引. textPos 越界则 clamp 到首/尾段.
int CXEditDW::FindParagraphByTextPos(int textPos) const{
	int n = (int)m_paragraphs.size();
	if (n == 0) return -1;
	if (textPos <= m_paragraphs[0].textStart) return 0;
	int lo = 0, hi = n - 1;
	while (lo < hi){
		int mid = (lo + hi + 1) / 2;
		if (m_paragraphs[mid].textStart <= textPos) lo = mid;
		else hi = mid - 1;
	}
	return lo;
}

// 二分: 段按 yOffset 单调升序. 返回最后一个 yOffset <= y 的段. y < 0 返 0.
int CXEditDW::FindParagraphByY(float y) const{
	int n = (int)m_paragraphs.size();
	if (n == 0) return -1;
	if (y <= m_paragraphs[0].yOffset) return 0;
	int lo = 0, hi = n - 1;
	while (lo < hi){
		int mid = (lo + hi + 1) / 2;
		if (m_paragraphs[mid].yOffset <= y) lo = mid;
		else hi = mid - 1;
	}
	return lo;
}

// Backward-compat 入口名. 实际转 ParaReleaseAllLayouts (释放所有段 layout, 保留段结构).
// 用于 DPI / 字号变化等 *全量* 重建 layout 场景.
void CXEditDW::ReleaseLayoutOnly(){
	ParaReleaseAllLayouts();
}

void CXEditDW::InvalidateFontBinding(){
	_EditDW_ReleaseFontBinding(m_fontBinding);
}

void CXEditDW::InvalidateStyleFontBindings(){
	for (size_t i = 0; i < m_styleFontBindings.size(); ++i)
		_EditDW_ReleaseFontBinding(m_styleFontBindings[i]);
}

HFONTX CXEditDW::ResolveBindFont() const{
	if (m_fontNameOnly) return NULL;
	if (m_hFontX) return m_hFontX;
	return XC_GetDefaultFont();
}

bool CXEditDW::BuildFontBinding(HFONTX hFont, _EditDW_FontBinding& out, float ptSize){
	_EditDW_ReleaseFontBinding(out);
	if (!hFont) return false;
	EnsureFactory();
	if (!m_pDWFactory) return false;

	float usePt = (ptSize > 0.0f) ? ptSize : m_fontSize;

	HWND hwnd = NULL;
	if (m_hEle){
		HWINDOW hWnd = (HWINDOW)XWidget_GetHWINDOW((HXCGUI)m_hEle);
		if (hWnd) hwnd = XWnd_GetHWND(hWnd);
	}
	HDC hdc = hwnd ? ::GetDC(hwnd) : ::GetDC(NULL);
	if (!hdc) return false;

	LOGFONTW lf;
	ZeroMemory(&lf, sizeof(lf));
	if (!XFont_GetLOGFONTW(hFont, hdc, &lf)){
		::ReleaseDC(hwnd ? hwnd : NULL, hdc);
		return false;
	}
	int dpi = (int)(m_dpiScale * 96.0f);
	if (dpi < 1) dpi = 96;
	lf.lfHeight = -::MulDiv((int)usePt, dpi, 72);
	lf.lfWidth  = 0;

	HFONT hGdiFont = ::CreateFontIndirectW(&lf);
	if (!hGdiFont){
		::ReleaseDC(hwnd ? hwnd : NULL, hdc);
		return false;
	}
	HFONT hOld = (HFONT)::SelectObject(hdc, hGdiFont);

	IDWriteGdiInterop* pInterop = NULL;
	HRESULT hr = m_pDWFactory->GetGdiInterop(&pInterop);
	IDWriteFontFace* pFace = NULL;
	if (SUCCEEDED(hr) && pInterop){
		hr = pInterop->CreateFontFaceFromHdc(hdc, &pFace);
		pInterop->Release();
	}

	::SelectObject(hdc, hOld);
	::DeleteObject(hGdiFont);
	::ReleaseDC(hwnd ? hwnd : NULL, hdc);
	if (FAILED(hr) || !pFace) return false;

	IDWriteFontCollection* pCol = NULL;
	hr = m_pDWFactory->GetSystemFontCollection(&pCol, TRUE);
	if (FAILED(hr) || !pCol){
		pFace->Release();
		return false;
	}

	out.hFontX      = hFont;
	out.pFace       = pFace;
	out.pCollection = pCol;
	out.logFont     = lf;
	out.valid       = true;
	return true;
}

void CXEditDW::EnsureFontBinding(){
	if (m_fontBinding.valid) return;
	if (m_fontNameOnly) return;
	HFONTX hFont = ResolveBindFont();
	if (!hFont) return;
	// 仅解析字体面供 DrawGlyphRun 兜底 (emoji 等). 不修改 m_fontSize / m_fontName:
	// 未显式 SetFont 时 EditDW 仍保持默认 14pt Segoe UI 排版, 避免被 XC_GetDefaultFont()
	// (常为 12pt 系统默认) 悄悄改小.
	(void)BuildFontBinding(hFont, m_fontBinding);
}

void CXEditDW::EnsureStyleFontBinding(int styleId){
	if (!IsStyleIdValid(styleId)) return;
	const _XEditDW_Style& s = m_styleTable[styleId];
	if (s.type != 0 || !s.hFontImageObj) return;
	if ((int)m_styleFontBindings.size() <= styleId)
		m_styleFontBindings.resize(styleId + 1);
	_EditDW_FontBinding& b = m_styleFontBindings[styleId];
	if (b.valid && b.hFontX == (HFONTX)s.hFontImageObj) return;
	font_info_ fi; ZeroMemory(&fi, sizeof(fi));
	XFont_GetFontInfo((HFONTX)s.hFontImageObj, &fi);
	float pt = fi.nSize > 0 ? (float)fi.nSize : m_fontSize;
	(void)BuildFontBinding((HFONTX)s.hFontImageObj, b, pt);
}

IDWriteFontFace* CXEditDW::GetBoundFontFace(){
	if (m_fontNameOnly) return NULL;
	EnsureFontBinding();
	return m_fontBinding.valid ? m_fontBinding.pFace : NULL;
}

IDWriteFontFace* CXEditDW::GetStyleFontFace(int styleId){
	if (!IsStyleIdValid(styleId)) return NULL;
	EnsureStyleFontBinding(styleId);
	if (styleId < 0 || styleId >= (int)m_styleFontBindings.size()) return NULL;
	return m_styleFontBindings[styleId].valid ? m_styleFontBindings[styleId].pFace : NULL;
}

void CXEditDW::ReleaseTextFormat(){
	if (m_pTextFormat){ m_pTextFormat->Release(); m_pTextFormat = NULL; }
}

void CXEditDW::ReleaseGdiResources(){
	if (m_pBmpRT){ m_pBmpRT->Release(); m_pBmpRT = NULL; }
	if (m_pParams){ m_pParams->Release(); m_pParams = NULL; }
	m_bmpRTW = 0;
	m_bmpRTH = 0;
}

void CXEditDW::ReleaseDWriteResources(){
	InvalidateFontBinding();
	InvalidateStyleFontBindings();
	ReleaseLayoutOnly();
	ReleaseTextFormat();
	ReleaseGdiResources();
	// 自建 factory 必须 Release; 借 XCGUI 的不要动 (XCGUI 自己持有, 我们一释放它就崩).
	if (m_pDWFactory && m_dwFactoryOwned){
		m_pDWFactory->Release();
	}
	m_pDWFactory = NULL;
	m_dwFactoryOwned = false;
}

void CXEditDW::InvalidateLayout(){
	// 全量失效: 释放所有段 layout, 设脏标. 调用者通常是 *样式/字体/字号/尺寸/DPI* 变化,
	// 这些会影响所有段, 必须整体重建. 文本编辑路径 (InsertCharsRaw/EraseCharsRaw) 不要
	// 调本函数 - 那两个内部走 ParaOnText* 仅释放受影响段, 单键入提速的关键路径.
	ParaReleaseAllLayouts();
	m_layoutDirty = true;
}

//===================================================================
//  事件注册
//===================================================================
void CXEditDW::InstallEvents(){
	// 注册在 XE_PAINT_END 而不是 XE_PAINT: 让 XCGUI 先绘制默认背景 / 边框 / 焦点 (由
	// SetBkInfo / EnableDrawBorder / EnableDrawFocus 控制), 我们再在最上层叠加选区/文本/光标.
	XEle_RegEventCPP1(m_hEle, XE_PAINT_END,       &CXEditDW::OnPaintImpl);
	XEle_RegEventCPP1(m_hEle, XE_LBUTTONDOWN,     &CXEditDW::OnLButtonDownImpl);
	XEle_RegEventCPP1(m_hEle, XE_LBUTTONUP,       &CXEditDW::OnLButtonUpImpl);
	XEle_RegEventCPP1(m_hEle, XE_MOUSEMOVE,       &CXEditDW::OnMouseMoveImpl);
	XEle_RegEventCPP1(m_hEle, XE_LBUTTONDBCLICK,  &CXEditDW::OnLButtonDBClickImpl);
	XEle_RegEventCPP1(m_hEle, XE_KEYDOWN,         &CXEditDW::OnKeyDownImpl);
	XEle_RegEventCPP1(m_hEle, XE_CHAR,            &CXEditDW::OnCharImpl);
	XEle_RegEventCPP1(m_hEle, XE_SETFOCUS,        &CXEditDW::OnSetFocusImpl);
	XEle_RegEventCPP1(m_hEle, XE_KILLFOCUS,       &CXEditDW::OnKillFocusImpl);
	// XE_KILLCAPTURE: 拖选 auto-scroll 的安全网 (capture 被外部夺走时清状态).
	XEle_RegEventCPP1(m_hEle, XE_KILLCAPTURE,     &CXEditDW::OnKillCaptureImpl);
	XEle_RegEventCPP1(m_hEle, XE_XC_TIMER,        &CXEditDW::OnTimerImpl);
	// XE_DESTROY_END: 子对象级联销毁完成时触发. 在此释放 AddStyleEx 内部 XFont_CreateEx
	// 出来的 HFONTX + AddObject 的 HIMAGE, 否则 XCGUI 退出前的 "未释放资源检查" 会把
	// 这些资源算成泄漏 (析构在 XExitXCGUI 之后才跑, 太晚).
	XEle_RegEventCPP1(m_hEle, XE_DESTROY_END,     &CXEditDW::OnDestroyEndImpl);
	// XE_SIZE: 父布局填充 / SetRect 后大小一变, 旧 layout 的 maxW 就过期. 必须刷一次,
	// 不然首次 Create / CopyFrom 阶段父布局未完成时建的 maxW≈1 烂 layout 会一直沿用,
	// 视觉就是文本不按真实宽度换行, 用户随便编辑一下才好.
	XEle_RegEventCPP1(m_hEle, XE_SIZE,            &CXEditDW::OnSizeImpl);
	// XE_DROPFILES: 文件拖入. 父窗口需另调 XWnd_EnableDragFiles 开启 (此处*不* 自动开,
	// 防止影响整个窗口的拖放语义). 处理流程见 OnDropFilesImpl: 嗅探图片 → 缩略图插入;
	// 嗅探文本 → 解码 + AddText; 完成后 MoveEnd 滚到底.
	XEle_RegEventCPP1(m_hEle, XE_DROPFILES,       &CXEditDW::OnDropFilesImpl);
}

int CXEditDW::OnSizeImpl(HELE /*hEle*/, int /*nFlags*/, UINT /*nAdjustNo*/, BOOL* /*pbHandled*/){
	// 关键性能优化 (v3): 不再无脑 InvalidateLayout - resize 路径每像素都触发 XE_SIZE,
	// 全段 IDWriteTextLayout 重建在 600KB 文本下单帧几百 ms, 60fps 拉条直接卡死.
	// 只在 *视口宽真变化* 时才 Invalidate (maxW 影响段内换行); 仅高度变化 / 只滚动条变化
	// 时段 layout 仍然有效, 跳过 Invalidate, 让 EnsureLayout 末尾的 RecomputeParaYOffsets
	// + XSView_SetTotalSize 即可 (这条路径成本 O(段数), 通常 ~ms 级).
	// 与 EnsureLayout / EnsureParagraphLayout 走同一 GetContentWidth, 包含兜底.
	// 否则当 XSView_GetViewWidth 返回异常小值时 OnSize 计算的 curContentW 与 EnsureLayout
	// 缓存的 m_lastContentW 不一致, widthChanged 误判, 后续 size 变化无法触发 InvalidateLayout.
	float curContentW = GetContentWidth();
	if (curContentW < 1.0f) curContentW = 1.0f;
	// 阈值 0.5px: 同一 DPI 下 contentW 应当是整数, 浮点比较容差.
	bool widthChanged = (m_lastContentW < 1.0f) || (fabsf(curContentW - m_lastContentW) > 0.5f);
	if (widthChanged){
		InvalidateLayout();
	}
	else{
		// 段 layout 还可用, 但 yOffsets / TotalSize 仍要让 EnsureLayout 走一遍.
		m_layoutDirty = true;
	}
	RedrawSelf();
	return 0;
}

//===================================================================
//  DPI / BkInfo 同步
//===================================================================
void CXEditDW::RefreshDpiScale(){
	if (!m_hEle) return;
	HWINDOW hWnd = (HWINDOW)XWidget_GetHWINDOW((HXCGUI)m_hEle);
	int dpi = hWnd ? XWnd_GetDPI(hWnd) : 96;
	float scale = (dpi > 0) ? ((float)dpi / 96.0f) : 1.0f;
	if (scale == m_dpiScale) return;
	m_dpiScale = scale;
	// 字号 = m_fontSize * m_dpiScale, DPI 变化必须重建 format 与 layout.
	InvalidateFontBinding();
	InvalidateStyleFontBindings();
	ReleaseTextFormat();
	ReleaseLayoutOnly();
	// BitmapRT 内部依赖 PixelsPerDip 决定 glyph 实际像素密度, DPI 变了就过期.
	ReleaseGdiResources();
	m_layoutDirty = true;
	// 同步滚动行高/页高 到新字号 (逻辑像素).
	XSView_SetLineSize(m_hEle, (int)m_fontSize, (int)(m_fontSize * 1.4f));
}

void CXEditDW::RebuildBkInfo(){
	if (!m_hEle) return;
	XEle_ClearBkInfo(m_hEle);
	// 背景填充: 鼠标离开 / 停留 / 按下 三个状态都画 (最常用是 leave; 加 stay/down 防调色).
	int fillState = element_state_flag_leave | element_state_flag_stay | element_state_flag_down;
	XEle_AddBkFill(m_hEle, fillState, m_bkColor);
	// 边框按焦点状态区分: focus_no 画 m_borderColor, focus 画 m_focusBorderColor.
	// 笔画宽取 m_borderWidth, 默认 1, *与 SetBorderSize 无关* (后者只控内容收缩).
	//
	// m_drawBorder 缓存由 EnableDrawBorderEx 维护. 关闭后 BkInfo 只剩 BkFill 项, 视觉无边框.
	// (XCGUI dll 不暴露 XEle_IsDrawBorder, 故走自缓存; 见 EnableDrawBorderEx 注释.)
	if (m_drawBorder){
		XEle_AddBkBorder(m_hEle, element_state_flag_focus_no, m_borderColor,      m_borderWidth);
		XEle_AddBkBorder(m_hEle, element_state_flag_focus,    m_focusBorderColor, m_borderWidth);
	}
	RedrawSelf();
}

void CXEditDW::EnableDrawBorderEx(BOOL bEnable){
	if (!m_hEle) return;
	bool newState = (bEnable != FALSE);
	if (newState == m_drawBorder){
		return;   // 状态未变跳过, 避免无效 ClearBkInfo + 重绘 / 反复缓存 focus border.
	}

	// 透传给 XCGUI 默认边框开关, 让基类查询 (例如其他代码读 XEle_IsDrawFocus 之类) 也一致.
	// 注意: XEle_EnableDrawBorder 只关 XCGUI *默认隐含边框*, 不影响 BkInfo 边框, 也不影响
	// *XCGUI 内置焦点边框* (XEle_SetFocusBorderColor 系统). 后者实测画了一条 #58B1FC 的
	// 蓝边 (XCGUI 默认主题色), 必须单独处理 - 见下面 SetFocusBorderColor 透明覆盖.
	XEle_EnableDrawBorder(m_hEle, bEnable);

	if (!newState){
		// === 禁用: 缓存 XCGUI 内置焦点边框色, 覆盖为透明使其视觉消失. ===
		// 仅首次禁用时缓存 (m_focusBorderSaved 防止重复 disable 时把 *透明色当原色*).
		if (!m_focusBorderSaved){
			m_savedXcguiFocusBorderColor = XEle_GetFocusBorderColor(m_hEle);
			m_focusBorderSaved = true;
		}
		XEle_SetFocusBorderColor(m_hEle, RGBA(0, 0, 0, 0));
	}
	else{
		// === 启用: 从缓存恢复 XCGUI 内置焦点边框色. ===
		if (m_focusBorderSaved){
			XEle_SetFocusBorderColor(m_hEle, m_savedXcguiFocusBorderColor);
			m_focusBorderSaved = false;
		}
	}

	m_drawBorder = newState;
	RebuildBkInfo();   // 重建 BkInfo (按 m_drawBorder 决定加不加 BkBorder), 内部已 RedrawSelf.
}

BOOL CXEditDW::IsDrawBorderEx() const{
	return m_drawBorder ? TRUE : FALSE;
}

//===================================================================
//  布局 (创建 / 更新 IDWriteTextLayout)
//===================================================================
void CXEditDW::EnsureFactory(){
	if (m_pDWFactory) return;
	// 先借 XCGUI 的 factory (D2D 模式下有, GDI+ 模式下 XC_GetDWriteFactory 返 NULL).
	m_pDWFactory = (IDWriteFactory*)XC_GetDWriteFactory();
	if (m_pDWFactory){
		m_dwFactoryOwned = false;   // 借的, 不 Release
		return;
	}
	// XCGUI GDI+ 模式下没 factory, 我们自己建一份. DirectWrite 跟 D2D 解耦, dwrite.dll 在
	// Win7 SP1+ 全系统自带, 不需要 GPU 不需要 D2D - 排版照常工作, 渲染走 BitmapRenderTarget.
	HRESULT hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&m_pDWFactory));
	if (SUCCEEDED(hr) && m_pDWFactory){
		m_dwFactoryOwned = true;    // 我们建的, 析构 Release
	} else {
		m_pDWFactory = NULL;        // 极端环境 (没 dwrite.dll) 拿不到, 文本就真画不了了
	}
}

void CXEditDW::EnsureTextFormat(){
	if (m_pTextFormat) return;
	EnsureFactory();
	if (!m_pDWFactory) return;

	const wchar_t* familyName = m_fontName.c_str();
	IDWriteFontCollection* pCollection = NULL;
	DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_REGULAR;
	DWRITE_FONT_STYLE  style  = DWRITE_FONT_STYLE_NORMAL;

	// 仅显式 SetFont(HFONTX) 时让 layout 走句柄绑定的族名/集合; 否则保持 m_fontName 默认.
	if (m_useFontHandle && m_hFontX){
		EnsureFontBinding();
		if (m_fontBinding.valid){
			familyName = m_fontBinding.logFont.lfFaceName;
			pCollection = m_fontBinding.pCollection;
			weight = (DWRITE_FONT_WEIGHT)m_fontBinding.logFont.lfWeight;
			if (weight < 1) weight = DWRITE_FONT_WEIGHT_REGULAR;
			style = m_fontBinding.logFont.lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
		}
	}

	HRESULT hr = m_pDWFactory->CreateTextFormat(
		familyName, pCollection,
		weight, style,
		DWRITE_FONT_STRETCH_NORMAL,
		m_fontSize * m_dpiScale,    // 物理像素字号, 与 D2D RT face-value 一致
		L"",
		&m_pTextFormat);
	if (SUCCEEDED(hr) && m_pTextFormat){
		m_pTextFormat->SetWordWrapping(
			(m_multiLine && m_wrap) ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
		m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}
}

// 文本可显示区 = XSView 视口 × m_dpiScale.
// XSView_GetViewWidth/Height 返回逻辑像素下的视口 (已减去 BorderSize 与可见滚动条),
// 乘上 m_dpiScale 转成物理像素后与 D2D RT face-value 同坐标系. 这样:
//   1. 文本不会画到滚动条上面 (XSView 已扯除滚动条区);
//   2. 文本可显区 与 SetBorderSize / 滚动条出现与否 同步.
float CXEditDW::GetContentWidth(){
	// 关键: 用 元素宽 - 左右边框 (不扣 V 滚动条占位) 作内容宽.
	//
	// 历史方案是 XSView_GetViewWidth(已扣可见滚动条占位), 但这在
	// "多行 + autowrap" 路径下会触发死循环反馈:
	//   1) 文本按 viewW 排版, totalHeight > viewH, V 滚动条出现
	//   2) viewW 缩小约 17px, maxW 也跟着缩
	//   3) 文本被迫换行更狠, totalHeight 更大, V 滚动条仍维持
	//   4) 反馈到极端: 元素初始较窄 (例如 120px) 时直接每字符一行
	// 用 元素宽-左右边框 做 maxW 不引入 viewW → totalH → viewW 的环, 排版稳定.
	// 代价: V 滚动条可见时, 最右侧约 17px 文字可能被滚动条压住 (可接受的小瑕疵).
	int eleW = XEle_GetWidth(m_hEle);
	borderSize_ bs = { 0, 0, 0, 0 };
	XEle_GetBorderSize(m_hEle, &bs);
	int contentW = eleW - bs.leftSize - bs.rightSize;
	if (contentW < 1) contentW = 1;
	float w = (float)contentW * m_dpiScale;
	if (w < 1.0f) w = 1.0f;
	return w;
}

float CXEditDW::GetContentHeight(){
	float h = (float)XSView_GetViewHeight(m_hEle) * m_dpiScale;
	if (h < 1.0f) h = 1.0f;
	return h;
}

// 单行模式垂直对齐偏移. 多行 / 内容已撑满视口 / top 对齐时返 0.
// 注意: m_paraTotalHeight 是 *分段累加后* 的总段高 (RecomputeParaYOffsets 算出, 段 layout
// 缺失时为 0). 因此本函数应在 EnsureLayout 之后调用 (paint / hit-test 路径都是).
// 空文档 / 段未构建时返 0 (避免 totalHeight=0 让 voff=viewH, 把 hint/caret 飘到视口底).
float CXEditDW::ContentVerticalOffsetPhys() const{
	if (m_multiLine) return 0.0f;
	const int vMask = m_textAlign & (edit_textAlign_flag_bottom | edit_textAlign_flag_center_v);
	if (vMask == 0) return 0.0f;   // top (默认)
	if (m_paragraphs.empty() || m_paraTotalHeight <= 0.0f) return 0.0f;
	float viewH = (float)XSView_GetViewHeight(m_hEle) * m_dpiScale;
	if (viewH < 1.0f) return 0.0f;
	float avail = viewH - m_paraTotalHeight;
	if (avail <= 0.0f) return 0.0f;
	if      (vMask & edit_textAlign_flag_center_v) return avail * 0.5f;
	else if (vMask & edit_textAlign_flag_bottom)   return avail;
	return 0.0f;
}

// 分段架构下的 EnsureLayout:
//  (1) 若 m_paragraphs 空但 m_text 非空 → ParaRebuildFromText 重建结构 (释放 layouts).
//      这是 SetText / DPI 改变之后第一次进 EnsureLayout 的初始化路径.
//  (2) 对 pLayout==NULL 的段调 EnsureParagraphLayout 构建. 编辑/插入只有受影响段 pLayout=NULL,
//      未受影响段 pLayout 复用; 单键入热路径下通常 = 1 段 reshape (~1-5ms).
//  (3) RecomputeParaYOffsets 累加得各段 yOffset / 全局 totalHeight / maxWidth.
//  (4) XSView_SetTotalSize 同步滚动条总尺寸.
//  (5) PositionInlineObjects 把 inline 子元素 SetPosition 到新坐标.
//  (6) m_scrollToCaretPending → 精确滚动到光标.
void CXEditDW::EnsureLayout(){
	RefreshDpiScale();
	EnsureTextFormat();
	if (!m_pTextFormat) return;
	if (!m_layoutDirty && !m_paragraphs.empty()){
		// 段结构在 ParaOnText* 里持续维护, 所有段 layout 都已建 → 直接返回.
		bool anyMissing = false;
		for (size_t i = 0; i < m_paragraphs.size(); ++i){
			if (!m_paragraphs[i].pLayout){ anyMissing = true; break; }
		}
		if (!anyMissing) return;
	}

	// (1) 段结构初始化 (SetText / DPI 改变 / 析构后第一次进来).
	if (m_paragraphs.empty() && !m_text.empty()){
		ParaRebuildFromText();
	}

	// (2) 构建所有缺失的段 layout. 关键热路径: 编辑场景下通常只 1 段 NULL.
	for (int i = 0; i < (int)m_paragraphs.size(); ++i){
		if (!m_paragraphs[i].pLayout){
			EnsureParagraphLayout(i);
		}
	}

	// (3) yOffsets + 全局尺寸.
	RecomputeParaYOffsets();

	// (4) 同步滚动条 totalSize (逻辑像素).
	float invScale = (m_dpiScale > 0.0f) ? (1.0f / m_dpiScale) : 1.0f;
	int totalW = (int)(m_paraMaxWidth    * invScale + 1.0f);
	int totalH = (int)(m_paraTotalHeight * invScale + 1.0f);
	if (totalW < 1) totalW = 1;
	if (totalH < 1) totalH = 1;
	XSView_SetTotalSize(m_hEle, totalW, totalH);

	m_layoutDirty = false;
	m_lastBuiltTextLen = (int)m_text.size();
	// 缓存本次构建用的视口宽 (物理像素), 给 OnSizeImpl 跳过无效 Invalidate 判断.
	m_lastContentW = GetContentWidth();

	// (5) inline 子元素重定位 + 显隐. 必须无条件调 - 全选删除后 m_paragraphs 为空, 但
	// step 1 (按 nRef 隐藏) 跟段无关, 不调子控件就留在屏上不消失.
	PositionInlineObjects(0, 0);

	// (6) post-rebuild caret 精确滚动 hook.
	if (m_scrollToCaretPending){
		m_scrollToCaretPending = false;
		EnsureCaretVisible();
	}
}

//===================================================================
//  绘制 (XE_PAINT_END: XCGUI 默认 BkInfo 已画完, 我们只叠加选区 / 文本 / 光标 / 占位字)
//===================================================================

// D2D 1.1+ (Win8+ / 装了平台更新的 Win7) 才有 D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT (0x4).
// 在 D2D 1.0 (原生 Win7) 上传入 0x4 会让 DrawText/DrawTextLayout 静默失败, 表现为 *一字不刷*.
// 通过 QI ID2D1DeviceContext (D2D 1.1 的标志接口) 探测运行环境, 结果起动后不变 -> 缓存一次.
// 采用 GUID 直接 QI, 避免引入 <d2d1_1.h> 依赖 (依然只使用 d2d1.h 接口).
static D2D1_DRAW_TEXT_OPTIONS GetColorFontDrawOptions(ID2D1RenderTarget* rt){
	static int s_cached = -1;  // -1 = 未探测, 0 = 不支持, 1 = 支持
	if (s_cached < 0 && rt){
		// IID_ID2D1DeviceContext = {E8F7FE7A-191C-466D-AD95-975678BDA998}
		static const GUID IID_ID2D1DeviceContext_local =
			{ 0xe8f7fe7a, 0x191c, 0x466d, { 0xad, 0x95, 0x97, 0x56, 0x78, 0xbd, 0xa9, 0x98 } };
		IUnknown* pDC = NULL;
		HRESULT hr = rt->QueryInterface(IID_ID2D1DeviceContext_local, (void**)&pDC);
		if (SUCCEEDED(hr) && pDC){
			s_cached = 1;
			pDC->Release();
		} else {
			s_cached = 0;
		}
	}
	return (s_cached == 1) ? (D2D1_DRAW_TEXT_OPTIONS)0x00000004 : D2D1_DRAW_TEXT_OPTIONS_NONE;
}

int CXEditDW::OnPaintImpl(HELE /*hEle*/, HDRAW hDraw, BOOL* /*pbHandled*/){
	EnsureFactory();
	if (!m_pDWFactory) return 0;
	EnsureLayout();

	// 优先 D2D 路径 (彩色 emoji + GPU 加速). 拿不到 D2D RT (GDI+ 模式 / Win7 无 GPU VM 等)
	// 时回落 GDI 路径: 排版照样用 DirectWrite 算, 只把渲染换到 IDWriteBitmapRenderTarget.
	// emoji 降级为单色字形但中英文 / 选区 / 光标 / inline 对象都正常.
	ID2D1RenderTarget* rt = (ID2D1RenderTarget*)XDraw_GetD2dRenderTarget(hDraw);
	if (!rt){
		HDC hdc = XDraw_GetHDC(hDraw);
		if (hdc) OnPaintGdi(hdc, hDraw);
		return 0;
	}

	// HDRAW 经 XDraw_GetD2dRenderTarget 取到的 ID2D1RenderTarget 是 *窗口级* 渲染目标,
	// 原点 = 窗口客户区左上角 (物理像素). XEle_GetWndClientRectDPI 返元素全矩形
	// (与 BorderSize/Padding 无关), 手动减 BorderSize 才能得到文本可显区.
	RECT rcEle;
	XEle_GetWndClientRectDPI(m_hEle, &rcEle);
	borderSize_ bs;
	XEle_GetBorderSize(m_hEle, &bs);

	// 文本起点: 元素左上 + 边框区入尺 (逻辑 × dpiScale = 物理) - 滚动偏移.
	// XSView_GetViewPosH/V 返回逻辑像素, 乘 m_dpiScale 转为物理后才能与 D2D RT 同坐标系.
	float scrollX = (float)XSView_GetViewPosH(m_hEle) * m_dpiScale;
	float scrollY = (float)XSView_GetViewPosV(m_hEle) * m_dpiScale;
	float contentLeft = (float)rcEle.left + (float)bs.leftSize * m_dpiScale;
	float contentTop  = (float)rcEle.top  + (float)bs.topSize  * m_dpiScale;
	float originX = contentLeft - scrollX;
	float originY = contentTop  - scrollY;

	// 注: 不在这里调 PositionInlineObjects 了 - 它已被同步触发在 EnsureLayout 末尾.
	// 在 OnPaintImpl 里每帧再调一次是无意义的: XWidget_Show / XEle_SetPosition 即使
	// 值未变, XCGUI 仍会 invalidate 父 → 触发下一帧 paint → 再走一次... 形成无限重绘
	// (文本看上去 "一直在被绘制" + 发虚, 选区盖住时被 DrawSelection 遮挡所以视觉恢复正常).
	// 真正影响位置的事件: 文本/字号/边框/DPI 变 → 都会 InvalidateLayout → 下次 EnsureLayout
	// 重建 layout 后同步定位. 滚动 不需要重定位 (XCGUI 把子元素跟父滚动自动平移).

	// 文本可显区裁剪: 滚动后 originX/originY 会变负, 文本会延伸进左/右/上/下边框区,
	// 必须用 D2D PushAxisAlignedClip 把 *选区/文本/光标/占位字* 硬截在内容区里.
	// 内容区右下 = 内容左上 + (XSView_GetViewWidth/Height × m_dpiScale) — 已扣除可见滚动条与 BorderSize.
	//
	// 兼容性: D2D 1.0 (原生 Win7) 要求 PushAxisAlignedClip 的 AA 模式 严格 与 RT 当前 AA 一致,
	// 否则 "succeeds but rendering is undefined" (实际表现为全部绘制静默丢失).
	// XCGUI 默认 RT 可能是 ALIASED 也可能是 PER_PRIMITIVE - 不要假设.
	// 所以: 用 RT 现有 AA 同时作为 clip 的 AA 传入, 不改 RT 自身状态.
	// 这样 D2D 1.0 / 1.1 都安全, 且不会意外改变 XCGUI 其他部件的绘制状态.
	D2D1_RECT_F clipRect = D2D1::RectF(
		contentLeft,
		contentTop,
		contentLeft + GetContentWidth(),
		contentTop  + GetContentHeight());
	D2D1_ANTIALIAS_MODE prevAA = rt->GetAntialiasMode();
	rt->PushAxisAlignedClip(clipRect, prevAA);

	// 4. 占位提示 (无文本时). 有字体绑定时走自定义 renderer.
	if (m_text.empty() && !m_hint.empty() && m_pTextFormat){
		ID2D1SolidColorBrush* pHintBrush = NULL;
		rt->CreateSolidColorBrush(RgbaToD2D(m_hintColor), &pHintBrush);
		if (pHintBrush){
			IDWriteFontFace* pBoundFace = GetBoundFontFace();
			if (pBoundFace && m_pDWFactory){
				IDWriteTextLayout* pHintLayout = NULL;
				HRESULT hrHint = m_pDWFactory->CreateTextLayout(
					m_hint.c_str(), (UINT32)m_hint.size(),
					m_pTextFormat, GetContentWidth(), GetContentHeight(), &pHintLayout);
				if (SUCCEEDED(hrHint) && pHintLayout){
					if (m_fontBinding.valid && m_fontBinding.pCollection){
						DWRITE_TEXT_RANGE hintRange = { 0, (UINT32)m_hint.size() };
						pHintLayout->SetFontCollection(m_fontBinding.pCollection, hintRange);
					}
					IDWriteFactory2* pDW2 = NULL;
					m_pDWFactory->QueryInterface(__uuidof(IDWriteFactory2), (void**)&pDW2);
					_CXEditDW_TextRenderer hintR(rt, pHintBrush, pDW2, pBoundFace);
					pHintLayout->Draw(NULL, &hintR, originX, originY);
					if (pDW2) pDW2->Release();
					pHintLayout->Release();
				}
			} else {
				D2D1_RECT_F rcHint = D2D1::RectF(
					originX, originY,
					originX + GetContentWidth(),
					originY + GetContentHeight());
				rt->DrawText(
					m_hint.c_str(), (UINT32)m_hint.size(),
					m_pTextFormat,
					rcHint,
					pHintBrush,
					GetColorFontDrawOptions(rt));
			}
			pHintBrush->Release();
		}
	}

	// 5. 选区背景 (失去焦点时隐藏). DrawSelection 内部按段拆分 range.
	if (!m_paragraphs.empty() && HasSelectionInner() && m_focused){
		int s, e; GetSelectionRangeInner(s, e);
		DrawSelection(rt, originX, originY, s, e);
	}

	// 6. 文本 (分段架构):
	//   a) 按段循环, 每段独立 IDWriteTextLayout. 视口外段直接跳过 (viewport culling).
	//   b) 有色样式 → 用自定义 _CXEditDW_TextRenderer (per-glyph QI 取色, 缓存 brush).
	//   c) 无色样式 → rt->DrawTextLayout fast path.
	//   d) brush / renderer 跨段复用, 减少对象创建 / 释放开销.
	// viewport 在 layout 全局坐标系 (物理像素): [scrollY, scrollY + contentH).
	float viewTop    = scrollY;
	float viewBottom = scrollY + GetContentHeight();
	const float voff = ContentVerticalOffsetPhys();   // 单行垂直对齐偏移; 多行恒 0
	if (!m_paragraphs.empty()){
		IDWriteFontFace* pDefaultFace = GetBoundFontFace();
		bool slowPath = HasAnyColoredText() || (pDefaultFace != NULL);
		if (!slowPath){
			for (size_t si = 0; si < m_styleTable.size(); ++si){
				if (IsStyleIdValid((int)si) && m_styleTable[si].type == 0 && m_styleTable[si].hFontImageObj){
					slowPath = true;
					break;
				}
			}
		}
		ID2D1SolidColorBrush* pTextBrush = NULL;
		rt->CreateSolidColorBrush(RgbaToD2D(m_textColor), &pTextBrush);
		if (pTextBrush){
			IDWriteFactory2* pDW2 = NULL;
			_CXEditDW_TextRenderer* pRenderer = NULL;
			if (slowPath){
				if (m_pDWFactory){
					m_pDWFactory->QueryInterface(__uuidof(IDWriteFactory2), (void**)&pDW2);
				}
				pRenderer = new _CXEditDW_TextRenderer(rt, pTextBrush, pDW2, pDefaultFace);
			}
			for (size_t pi = 0; pi < m_paragraphs.size(); ++pi){
				_XEditDW_Para& p = m_paragraphs[pi];
				if (!p.pLayout) continue;
				// viewport culling: 段底端在视口上方 / 段顶端在视口下方 → 跳过.
				// 比较与渲染坐标一致都加 voff, 否则垂直对齐时 cull 会误剔.
				float yTop = p.yOffset + voff;
				if (yTop + p.height < viewTop) continue;
				if (yTop > viewBottom) break;
				float py = originY + yTop;
				if (slowPath){
					p.pLayout->Draw(NULL, pRenderer, originX, py);
				} else {
					rt->DrawTextLayout(
						D2D1::Point2F(originX, py),
						p.pLayout, pTextBrush,
						GetColorFontDrawOptions(rt));
				}
			}
			if (pRenderer) pRenderer->Release();
			if (pDW2)      pDW2->Release();
			pTextBrush->Release();
		}
	}

	// 7. 自绘 caret (焦点 + 无选区 + 闪烁亮帧才画). 笔画宽随 DPI 缩放.
	// 空文档也要画 caret - GetCaretPoint 在 m_paragraphs.empty() 分支返 (0,0,默认行高).
	// XEle_IsDrawFocus: 在 XCGUI 语义中 EnableDrawFocus 对应 "允许画插入符 / 焦点高亮".
	// 用户 EnableDrawFocus(FALSE) 后该返 FALSE, caret 不画 - 与 XEdit 原生行为一致.
	if (m_focused && !HasSelectionInner() && m_caretVisible && XEle_IsDrawFocus(m_hEle)){
		float cx = 0, cy = 0, ch = 0;
		if (GetCaretPoint(m_caret, &cx, &cy, &ch)){
			ID2D1SolidColorBrush* pCaretBrush = NULL;
			rt->CreateSolidColorBrush(RgbaToD2D(m_caretColor), &pCaretBrush);
			if (pCaretBrush){
				// 像素对齐: cx/cy 来自 HitTestTextPosition, 通常是分数 (例如 10.7).
				// D2D 默认 PER_PRIMITIVE 反走样会把 1px 竖线摊到两列, 每列 alpha 减半,
				// 视觉上 caret 变粗变灰 (即 "模糊"). floorf(x + 0.5) 四舍五入到整数像素,
				// + 临时切 ALIASED 模式做双保险, 让 caret 在任何子像素位置都锐利.
				float caretX = floorf(originX + cx + 0.5f);
				float caretY = floorf(originY + cy + 0.5f);
				// 插入符笔宽: m_caretWidth (逻辑) * m_dpiScale, 不少于 1px 以免高 DPI 下看不见.
				float caretW = (float)m_caretWidth * m_dpiScale;
				if (caretW < 1.0f) caretW = 1.0f;
				caretW = floorf(caretW + 0.5f);
				float caretH = floorf(ch + 0.5f);
				D2D1_ANTIALIAS_MODE oldAA = rt->GetAntialiasMode();
				rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
				rt->FillRectangle(
					D2D1::RectF(caretX, caretY, caretX + caretW, caretY + caretH),
					pCaretBrush);
				rt->SetAntialiasMode(oldAA);
				pCaretBrush->Release();
			}
		}
	}

	// 与开头 PushAxisAlignedClip 严格配对.
	rt->PopAxisAlignedClip();

	// 注意: 不设 *pbHandled = TRUE. XE_PAINT_END 在默认绘制之后触发, 叠加即可.
	return 1;
}

void CXEditDW::DrawSelection(ID2D1RenderTarget* rt, float originX, float originY, int selStart, int selEnd){
	if (m_paragraphs.empty()) return;
	if (selStart >= selEnd) return;

	ID2D1SolidColorBrush* pSelBrush = NULL;
	rt->CreateSolidColorBrush(RgbaToD2D(m_selBgColor), &pSelBrush);
	if (!pSelBrush) return;

	int n = (int)m_text.size();
	int e = selEnd > n ? n : selEnd;
	int s = selStart < 0 ? 0 : selStart;

	// 按段循环: 段内把 [s, e) 截到 [max(s, p.textStart), min(e, p.textStart+p.textLen)),
	// 再按 \uFFFC 把 range 拆成不跨 inline 占位字符的子段, 转段相对位置调 HitTestTextRange.
	// 加上 (originX, originY + p.yOffset + voff) 把段内坐标转回 layout 全局 (含单行垂直对齐偏移).
	const float voff = ContentVerticalOffsetPhys();
	int startPara = FindParagraphByTextPos(s);
	if (startPara < 0) startPara = 0;
	for (size_t pi = (size_t)startPara; pi < m_paragraphs.size(); ++pi){
		_XEditDW_Para& p = m_paragraphs[pi];
		if (p.textStart >= e) break;
		if (p.textStart + p.textLen <= s) continue;
		if (!p.pLayout) continue;

		int paraAbsStart = p.textStart;
		int paraAbsEnd   = p.textStart + p.textLen;
		int segS = (s > paraAbsStart) ? s : paraAbsStart;
		int segE = (e < paraAbsEnd)   ? e : paraAbsEnd;
		if (segS >= segE) continue;

		float yShift = originY + p.yOffset + voff;
		int i = segS;
		while (i < segE){
			while (i < segE && m_text[i] == kObjectReplacementChar) ++i;
			if (i >= segE) break;
			int j = i + 1;
			while (j < segE && m_text[j] != kObjectReplacementChar) ++j;
			UINT32 relPos = (UINT32)(i - paraAbsStart);
			UINT32 relLen = (UINT32)(j - i);

			UINT32 actualCount = 0;
			DWRITE_HIT_TEST_METRICS dummy;
			HRESULT hr = p.pLayout->HitTestTextRange(
				relPos, relLen, originX, yShift,
				&dummy, 1, &actualCount);
			if ((hr == S_OK || hr == E_NOT_SUFFICIENT_BUFFER) && actualCount > 0){
				if (m_selHitMetrics.size() < actualCount)
					m_selHitMetrics.resize(actualCount);
				hr = p.pLayout->HitTestTextRange(
					relPos, relLen, originX, yShift,
					m_selHitMetrics.data(), actualCount, &actualCount);
				if (SUCCEEDED(hr)){
					for (UINT32 k = 0; k < actualCount; ++k){
						const DWRITE_HIT_TEST_METRICS& mm = m_selHitMetrics[k];
						rt->FillRectangle(
							D2D1::RectF(mm.left, mm.top, mm.left + mm.width, mm.top + mm.height),
							pSelBrush);
					}
				}
			}
			i = j;
		}
	}

	pSelBrush->Release();
}

//===================================================================
//  GDI 渲染回退实现
//===================================================================
// 取 / 建 / 改尺寸 BitmapRT. wantW/wantH 是 *物理像素*. 第一次创建时同时建 RenderingParams.
// IDWriteBitmapRenderTarget::Resize 在尺寸没变时是 no-op, 不浪费.
IDWriteBitmapRenderTarget* CXEditDW::EnsureBmpRT(int wantW, int wantH){
	if (wantW < 1) wantW = 1;
	if (wantH < 1) wantH = 1;
	EnsureFactory();
	if (!m_pDWFactory) return NULL;

	if (!m_pBmpRT){
		IDWriteGdiInterop* pInterop = NULL;
		HRESULT hr = m_pDWFactory->GetGdiInterop(&pInterop);
		if (FAILED(hr) || !pInterop) return NULL;
		// 用 screen DC (HDC=NULL 表示桌面兼容 DC) 作 source DC, 这样能跟任意 BitBlt 目标兼容.
		hr = pInterop->CreateBitmapRenderTarget(NULL, wantW, wantH, &m_pBmpRT);
		pInterop->Release();
		if (FAILED(hr) || !m_pBmpRT) return NULL;
		m_bmpRTW = wantW;
		m_bmpRTH = wantH;
		// PixelsPerDip: BitmapRT 默认按 96 DPI, 我们的 layout 是物理像素已经包含 DPI 缩放,
		// 所以这里保持 1.0 (识别器拿到的坐标就是 BitmapRT 内的物理像素), 不再二次缩放.
		m_pBmpRT->SetPixelsPerDip(1.0f);
	} else if (wantW != m_bmpRTW || wantH != m_bmpRTH){
		HRESULT hr = m_pBmpRT->Resize(wantW, wantH);
		if (FAILED(hr)){
			// Resize 失败比较少见 (内存不足), 兜底重建.
			m_pBmpRT->Release(); m_pBmpRT = NULL;
			return EnsureBmpRT(wantW, wantH);
		}
		m_bmpRTW = wantW;
		m_bmpRTH = wantH;
	}

	if (!m_pParams){
		// 默认参数: 系统设置 (ClearType / 灰度, gamma 等).
		// CreateRenderingParams 失败也不致命, DrawGlyphRun 可传 NULL params.
		m_pDWFactory->CreateRenderingParams(&m_pParams);
	}
	return m_pBmpRT;
}

// GDI 版选区: 与 D2D 版同语义. 跳过 \uFFFC 占位字符段, inline 子元素背景透出.
// 坐标系: 入参 originX/Y 已经是 *BitmapRT 内坐标系* (DIB-local), 因为调用方在转交前减去了
// (contentLeft, contentTop). HitTestTextRange 拿到的 rect 也是 DIB-local.
void CXEditDW::DrawSelectionGdi(HDC hdc, float originX, float originY, int selStart, int selEnd){
	if (!hdc || m_paragraphs.empty()) return;
	if (selStart >= selEnd) return;

	HBRUSH brSel = ::CreateSolidBrush(_ToGdiColor(m_selBgColor));
	if (!brSel) return;

	int n = (int)m_text.size();
	int e = selEnd > n ? n : selEnd;
	int s = selStart < 0 ? 0 : selStart;

	const float voff = ContentVerticalOffsetPhys();
	int startPara = FindParagraphByTextPos(s);
	if (startPara < 0) startPara = 0;
	for (size_t pi = (size_t)startPara; pi < m_paragraphs.size(); ++pi){
		_XEditDW_Para& p = m_paragraphs[pi];
		if (p.textStart >= e) break;
		if (p.textStart + p.textLen <= s) continue;
		if (!p.pLayout) continue;

		int paraAbsStart = p.textStart;
		int paraAbsEnd   = p.textStart + p.textLen;
		int segS = (s > paraAbsStart) ? s : paraAbsStart;
		int segE = (e < paraAbsEnd)   ? e : paraAbsEnd;
		if (segS >= segE) continue;

		float yShift = originY + p.yOffset + voff;
		int i = segS;
		while (i < segE){
			while (i < segE && m_text[i] == kObjectReplacementChar) ++i;
			if (i >= segE) break;
			int j = i + 1;
			while (j < segE && m_text[j] != kObjectReplacementChar) ++j;
			UINT32 relPos = (UINT32)(i - paraAbsStart);
			UINT32 relLen = (UINT32)(j - i);

			UINT32 actualCount = 0;
			DWRITE_HIT_TEST_METRICS dummy;
			HRESULT hr = p.pLayout->HitTestTextRange(
				relPos, relLen, originX, yShift,
				&dummy, 1, &actualCount);
			if ((hr == S_OK || hr == E_NOT_SUFFICIENT_BUFFER) && actualCount > 0){
				if (m_selHitMetrics.size() < actualCount)
					m_selHitMetrics.resize(actualCount);
				hr = p.pLayout->HitTestTextRange(
					relPos, relLen, originX, yShift,
					m_selHitMetrics.data(), actualCount, &actualCount);
				if (SUCCEEDED(hr)){
					for (UINT32 k = 0; k < actualCount; ++k){
						const DWRITE_HIT_TEST_METRICS& mm = m_selHitMetrics[k];
						RECT r;
						r.left   = (LONG)mm.left;
						r.top    = (LONG)mm.top;
						r.right  = (LONG)(mm.left + mm.width);
						r.bottom = (LONG)(mm.top  + mm.height);
						::FillRect(hdc, &r, brSel);
					}
				}
			}
			i = j;
		}
	}
	::DeleteObject(brSel);
}

// GDI 主绘制. 流程与 D2D 路径对齐, 关键区别:
//   - 内容画到内部 DIB (BitmapRT.GetMemoryDC), 不直接画屏幕 HDC;
//   - 画前从屏幕 HDC BitBlt 当前像素进 DIB (保留 XCGUI BkInfo 已画的背景), 否则 DIB 是黑底,
//     blit 回去就把背景覆盖了;
//   - 选区 / 文本 / 光标 / 占位字 都在 DIB 上画;
//   - 最后整块 BitBlt 回屏幕 HDC.
// 文本颜色样式: 走 SetDrawingEffect, 但 effect 用 _CXEditDW_ColorEffect (跟 D2D 路径的
// ID2D1Brush 二选一, 不混用, 由 _CXEditDW_GdiRenderer::PickColor 通过私有 IID QI 取回).
void CXEditDW::OnPaintGdi(HDC hdcTarget, HDRAW hDraw){
	if (!hdcTarget) return;
	// 空文档允许继续走到下面的 hint 分支; 文本段绘制各分支内部各自检查 m_paragraphs 非空.

	// GDI 自绘坐标系 ≠ D2D RT 坐标系 (见 XCGUI 文档 "画布偏移问题"):
	//   D2D RT : 窗口客户区 (0,0) 为原点, 我们用 rcEle.left/top 定位.
	//   HDC    : *元素本地* (0,0) 为原点 + XDraw_GetOffset 额外偏移 (父滚动视图平移用).
	// 所以 GDI 路径下:
	//   要画到元素的内容区 (扣掉 border), 目标 HDC 坐标 = (bs.leftSize + offsX, bs.topSize + offsY)
	//   物理像素: 乘 m_dpiScale.
	borderSize_ bs;
	XEle_GetBorderSize(m_hEle, &bs);
	int offsX = 0, offsY = 0;
	XDraw_GetOffset(hDraw, &offsX, &offsY);

	// 内容区在 HDC 坐标里的起点.
	int contentLeft = (int)((float)bs.leftSize * m_dpiScale) + offsX;
	int contentTop  = (int)((float)bs.topSize  * m_dpiScale) + offsY;
	int contentW = (int)GetContentWidth();
	int contentH = (int)GetContentHeight();
	if (contentW < 1 || contentH < 1) return;

	float scrollX = (float)XSView_GetViewPosH(m_hEle) * m_dpiScale;
	float scrollY = (float)XSView_GetViewPosV(m_hEle) * m_dpiScale;

	// BitmapRT (DIB) size = 内容区. DIB(0,0) ↔ HDC(contentLeft, contentTop).
	IDWriteBitmapRenderTarget* bmpRT = EnsureBmpRT(contentW, contentH);
	if (!bmpRT) return;
	HDC dcBmp = bmpRT->GetMemoryDC();
	if (!dcBmp) return;

	// 把屏幕当前像素 (XCGUI 已画完 BkInfo + 边框) 拉进 DIB, 不然 DIB 是黑底, blit 回去
	// 会把 BkInfo 画的背景覆盖.
	::BitBlt(dcBmp, 0, 0, contentW, contentH,
	         hdcTarget, contentLeft, contentTop, SRCCOPY);

	// 文本起点在 DIB-local 坐标系. 滚动后可能为负, DIB 自带裁剪 (DIB 外的像素直接丢).
	float originX = -scrollX;
	float originY = -scrollY;

	// 1) 占位字 (无文本时)
	if (m_text.empty() && !m_hint.empty() && m_pTextFormat){
		// 占位字另起 layout, 不影响主 m_pTextLayout.
		// 用 GDI 路径里更省事的 ExtTextOut + SelectObject 不好做 (我们的 m_pTextFormat 是 DWrite
		// 内部对象, 没现成 HFONT), 索性建一个临时 layout 渲染.
		IDWriteTextLayout* pHintLayout = NULL;
		HRESULT hr = m_pDWFactory->CreateTextLayout(
			m_hint.c_str(), (UINT32)m_hint.size(),
			m_pTextFormat, (float)contentW, (float)contentH, &pHintLayout);
		if (SUCCEEDED(hr) && pHintLayout){
			IDWriteFontFace* pBoundFace = GetBoundFontFace();
			if (pBoundFace && m_fontBinding.valid && m_fontBinding.pCollection){
				DWRITE_TEXT_RANGE hintRange = { 0, (UINT32)m_hint.size() };
				pHintLayout->SetFontCollection(m_fontBinding.pCollection, hintRange);
			}
			_CXEditDW_GdiRenderer* pR = new _CXEditDW_GdiRenderer(
				bmpRT, m_pParams, _ToGdiColor(m_hintColor), pBoundFace);
			pHintLayout->Draw(NULL, pR, 0.0f, 0.0f);
			pR->Release();
			pHintLayout->Release();
		}
	}

	// 2) 选区背景
	if (!m_paragraphs.empty() && HasSelectionInner() && m_focused){
		int s, e; GetSelectionRangeInner(s, e);
		DrawSelectionGdi(dcBmp, originX, originY, s, e);
	}

	// 3) 文本 (分段架构, 与 D2D 路径同结构):
	//    按段循环, 视口外段跳过. GdiRenderer 跨段复用.
	if (!m_paragraphs.empty()){
		IDWriteFontFace* pDefaultFace = GetBoundFontFace();
		_CXEditDW_GdiRenderer* pR = new _CXEditDW_GdiRenderer(
			bmpRT, m_pParams, _ToGdiColor(m_textColor), pDefaultFace);
		float viewTop    = scrollY;
		float viewBottom = scrollY + (float)contentH;
		const float voff = ContentVerticalOffsetPhys();
		for (size_t pi = 0; pi < m_paragraphs.size(); ++pi){
			_XEditDW_Para& p = m_paragraphs[pi];
			if (!p.pLayout) continue;
			float yTop = p.yOffset + voff;
			if (yTop + p.height < viewTop) continue;
			if (yTop > viewBottom) break;
			float py = originY + yTop;
			p.pLayout->Draw(NULL, pR, originX, py);
		}
		pR->Release();
	}

	// 4) 自绘 caret. 空文档时 GetCaretPoint 在 empty 分支返 (0,0,默认行高), 必须画.
	// EnableDrawFocus(FALSE) 下跳过画 caret - 同 D2D 路径.
	if (m_focused && !HasSelectionInner() && m_caretVisible && XEle_IsDrawFocus(m_hEle)){
		float cx = 0, cy = 0, ch = 0;
		if (GetCaretPoint(m_caret, &cx, &cy, &ch)){
			float caretW = (float)m_caretWidth * m_dpiScale;
			if (caretW < 1.0f) caretW = 1.0f;
			RECT rc;
			rc.left   = (LONG)(originX + cx);
			rc.top    = (LONG)(originY + cy);
			rc.right  = rc.left + (LONG)caretW;
			rc.bottom = rc.top  + (LONG)ch;
			HBRUSH br = ::CreateSolidBrush(_ToGdiColor(m_caretColor));
			if (br){ ::FillRect(dcBmp, &rc, br); ::DeleteObject(br); }
		}
	}

	// 5) 整块 BitBlt 回屏幕.
	::BitBlt(hdcTarget, contentLeft, contentTop, contentW, contentH,
	         dcBmp, 0, 0, SRCCOPY);
}

//===================================================================
//  命中测试 / 光标位置
//===================================================================
int CXEditDW::HitTestPoint(float xLocal, float yLocal){
	EnsureLayout();
	if (m_paragraphs.empty()) return 0;

	// 单行垂直对齐下 layout 整体下移 voff: 用户点击的 yLocal 是视觉坐标, 先减去
	// voff 还原到未对齐前的 layout 全局坐标, 后面 FindParagraphByY / HitTestPoint 都在
	// 这个坐标系里走.
	const float voff = ContentVerticalOffsetPhys();
	float yEff = yLocal - voff;

	int pi = FindParagraphByY(yEff);
	if (pi < 0) pi = 0;
	if (pi >= (int)m_paragraphs.size()) pi = (int)m_paragraphs.size() - 1;
	_XEditDW_Para& p = m_paragraphs[pi];
	EnsureParagraphLayout(pi);
	if (!p.pLayout) return p.textStart;

	BOOL trailing = FALSE, inside = FALSE;
	DWRITE_HIT_TEST_METRICS m;
	ZeroMemory(&m, sizeof(m));
	// 段内 y = layout 全局 y - 段 yOffset (全局 y 已减 voff).
	HRESULT hr = p.pLayout->HitTestPoint(xLocal, yEff - p.yOffset, &trailing, &inside, &m);
	if (FAILED(hr)) return p.textStart;
	int pos = p.textStart + (int)m.textPosition + (trailing ? (int)m.length : 0);
	if (pos > (int)m_text.size()) pos = (int)m_text.size();
	if (pos < 0) pos = 0;
	return pos;
}

bool CXEditDW::GetCaretPoint(int textPos, float* outX, float* outY, float* outH){
	EnsureLayout();
	if (m_paragraphs.empty()){
		// 空文档: caret 在 (0, 0), 高度按默认字号.
		*outX = 0; *outY = 0;
		*outH = m_fontSize * m_dpiScale * 1.2f;
		return true;
	}
	int pi = FindParagraphByTextPos(textPos);
	if (pi < 0) return false;
	_XEditDW_Para& p = m_paragraphs[pi];
	EnsureParagraphLayout(pi);
	if (!p.pLayout) return false;

	int relPos = textPos - p.textStart;
	if (relPos < 0) relPos = 0;
	if (relPos > p.textLen) relPos = p.textLen;

	FLOAT cx = 0, cy = 0;
	DWRITE_HIT_TEST_METRICS m;
	ZeroMemory(&m, sizeof(m));
	HRESULT hr = p.pLayout->HitTestTextPosition((UINT32)relPos, FALSE, &cx, &cy, &m);
	if (FAILED(hr)) return false;
	*outX = cx;
	// outY = 段内 cy + 段 yOffset + 单行垂直对齐偏移. 调用方拿到的是 *视觉坐标*,
	// 与 paint / inline 摆位 / DrawSelection 三路一致.
	*outY = cy + p.yOffset + ContentVerticalOffsetPhys();
	*outH = m.height > 0 ? m.height : m_fontSize * m_dpiScale * 1.2f;
	return true;
}

// 热路径专用: 不调 EnsureLayout, 直接用现有段 layouts. 段未建 (pLayout=NULL) 或 textPos
// 越界 (m_lastBuiltTextLen clamp) 时返 false; 调用方 (EnsureCaretVisible) 拿不到精确点
// 会跳过本帧滚动, 等 EnsureLayout 后续帧建好通过 post-rebuild hook 精确补一次.
bool CXEditDW::GetCaretPointStale(int textPos, float* outX, float* outY, float* outH){
	if (m_paragraphs.empty()) return false;
	int layoutLen = m_lastBuiltTextLen;
	if (textPos > layoutLen) textPos = layoutLen;
	if (textPos < 0)         textPos = 0;

	int pi = FindParagraphByTextPos(textPos);
	if (pi < 0) return false;
	_XEditDW_Para& p = m_paragraphs[pi];
	if (!p.pLayout) return false;

	int relPos = textPos - p.textStart;
	if (relPos < 0) relPos = 0;
	if (relPos > p.textLen) relPos = p.textLen;

	FLOAT cx = 0, cy = 0;
	DWRITE_HIT_TEST_METRICS m;
	ZeroMemory(&m, sizeof(m));
	HRESULT hr = p.pLayout->HitTestTextPosition((UINT32)relPos, FALSE, &cx, &cy, &m);
	if (FAILED(hr)) return false;
	*outX = cx;
	*outY = cy + p.yOffset + ContentVerticalOffsetPhys();
	*outH = m.height > 0 ? m.height : m_fontSize * m_dpiScale * 1.2f;
	return true;
}

// 检测样式表里是否有 *被字符引用* 且 *启用自定义颜色* 的文本样式. paint 路径据此分流:
//   有 → 自定义 IDWriteTextRenderer (per-segment 颜色, 走 SetDrawingEffect)
//   无 → rt->DrawTextLayout 快路径 (跳过每帧 O(n) charStyle 扫描 + 每 GlyphRun COM 回调)
// 大文本 600KB+ 走快路径单帧 cost 从几十 ms 降到亚 ms.
bool CXEditDW::HasAnyColoredText() const{
	for (size_t i = 0; i < m_styleTable.size(); ++i){
		const _XEditDW_Style& s = m_styleTable[i];
		if (s.type == 0 && s.bColor && s.nRef > 0) return true;
	}
	return false;
}

void CXEditDW::UpdateCaret(){
	m_caretVisible = true;
	RedrawSelf();
}

void CXEditDW::EnsureCaretVisible(){
	// 热路径调用 (键盘输入 / 撤销 / 粘贴) - *不* 调 EnsureLayout, 那会触发 CreateTextLayout
	// (600KB 文本 ~200ms, 每键卡死). 用 stale 变体: layout 还没建就直接 return, 文本变更路径
	// 会设 m_scrollToCaretPending, EnsureLayout 重建完后 post-rebuild hook 精确补一次.
	float cx = 0, cy = 0, ch = 0;
	if (!GetCaretPointStale(m_caret, &cx, &cy, &ch)) return;
	// cx/cy/ch 是 *物理* 像素 (来自 TextLayout). XSView_* 是 *逻辑* 像素.
	// 转换: 物理 / m_dpiScale = 逻辑, 取三位小数 floor + ceil 以避免边界闪烁.
	float invScale = (m_dpiScale > 0.0f) ? (1.0f / m_dpiScale) : 1.0f;
	int cxLog = (int)(cx * invScale);
	int cyLog = (int)(cy * invScale);
	int chLog = (int)(ch * invScale + 0.5f);
	int viewW = XSView_GetViewWidth(m_hEle);
	int viewH = XSView_GetViewHeight(m_hEle);
	if (viewW < 1) viewW = 1;
	if (viewH < 1) viewH = 1;
	int scrollX = XSView_GetViewPosH(m_hEle);
	int scrollY = XSView_GetViewPosV(m_hEle);
	if (cxLog < scrollX) XSView_ScrollPosXH(m_hEle, cxLog);
	else if (cxLog + 2 > scrollX + viewW) XSView_ScrollPosXH(m_hEle, cxLog + 2 - viewW);
	if (m_multiLine){
		if (cyLog < scrollY) XSView_ScrollPosYV(m_hEle, cyLog);
		else if (cyLog + chLog > scrollY + viewH) XSView_ScrollPosYV(m_hEle, cyLog + chLog - viewH);
	}
}

//===================================================================
//  鼠标
//===================================================================
// XCGUI 元素事件中 POINT* 是 *元素本地 × 100% 逻辑* 坐标, 与 m_pTextLayout *物理像素*
// 系不同. 文本区起点 (元素本地逻辑) = (bs.leftSize, bs.topSize), 仅减 BorderSize.
// XSView_GetViewPosH/V 返逻辑像素, 乘 m_dpiScale 转物理后加到 layout 坐标系.
//
// 性能: m_dpiScale 直接复用. 调用方 (鼠标 / 命中 / 自动滚动) 路径上更高一层
// (OnPaint / OnSize / OnLButtonDown) 都已 RefreshDpiScale, 这里再调要走 XWidget_GetHWINDOW
// + XWnd_GetDPI 两次 dll 边界往返, 对 60fps 拖选是不必要开销.
void CXEditDW::PtToLayout(const POINT* pt, float* outX, float* outY){
	borderSize_ bs;
	XEle_GetBorderSize(m_hEle, &bs);
	*outX = ((float)pt->x - (float)bs.leftSize) * m_dpiScale + (float)XSView_GetViewPosH(m_hEle) * m_dpiScale;
	*outY = ((float)pt->y - (float)bs.topSize ) * m_dpiScale + (float)XSView_GetViewPosV(m_hEle) * m_dpiScale;
}

int CXEditDW::OnLButtonDownImpl(HELE /*hEle*/, UINT /*flags*/, POINT* pt, BOOL* /*pbHandled*/){
	if (!pt) return 0;
	// 拖选起点统一刷一次 m_dpiScale - 之后 MouseMove / 自动滚动复用缓存值.
	// (PtToLayout 内部移除了 RefreshDpiScale 避免 60fps 拖选反复 dll 边界往返.)
	RefreshDpiScale();
	HWINDOW hWnd = XWidget_GetHWINDOW(m_hEle);
	if (hWnd) XWnd_SetFocusEle(hWnd, m_hEle);
	float lx, ly; PtToLayout(pt, &lx, &ly);
	int pos = HitTestPoint(lx, ly);
	bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
	if (!shift) m_anchor = pos;
	m_caret = pos;
	m_mouseDown = true;
	XEle_SetCapture(m_hEle, TRUE);
	RedrawSelf();
	UpdateCaret();
	return 1;
}

int CXEditDW::OnLButtonUpImpl(HELE /*hEle*/, UINT /*flags*/, POINT* /*pt*/, BOOL* /*pbHandled*/){
	if (m_mouseDown){
		m_mouseDown = false;
		XEle_SetCapture(m_hEle, FALSE);
		// 拖选结束: 停 auto-scroll timer. 幂等, 没起的话 Kill 也是 no-op.
		if (m_autoScrollOn){
			XEle_KillXCTimer(m_hEle, kAutoScrollTimerId);
			m_autoScrollOn = false;
		}
	}
	return 1;
}

int CXEditDW::OnMouseMoveImpl(HELE /*hEle*/, UINT /*flags*/, POINT* pt, BOOL* /*pbHandled*/){
	if (!m_mouseDown || !pt) return 0;
	// 缓存最近的 *元素本地逻辑像素* 鼠标坐标. timer 在用户停在边外不动时复用它持续滚动.
	m_lastDragPt = *pt;

	// 越界判定 (元素本地逻辑像素, 与 XCGUI POINT 同单位). 用 XSView 视口宽 + BorderSize 计
	// 算可显区. 在区外 → 启动 auto-scroll timer (尚未启), 触发持续滚动; 区内 → 停 timer.
	borderSize_ bs;
	XEle_GetBorderSize(m_hEle, &bs);
	int viewW = XSView_GetViewWidth (m_hEle);
	int viewH = XSView_GetViewHeight(m_hEle);
	int left   = (int)bs.leftSize;
	int top    = (int)bs.topSize;
	int right  = left + viewW;
	int bottom = top  + viewH;
	bool outside =
		(pt->x < left) || (pt->x > right) ||
		(m_multiLine && (pt->y < top || pt->y > bottom));
	if (outside){
		if (!m_autoScrollOn){
			// 50ms 周期 = 20fps, 略慢于 caret 闪烁但快到看不出滞后. 太密 (16ms) 会让滚动一蹋糊涂.
			XEle_SetXCTimer(m_hEle, kAutoScrollTimerId, 50);
			m_autoScrollOn = true;
		}
		UpdateAutoScroll();   // 立即先滚一步, 不等下一个 timer tick
		return 1;
	}
	if (m_autoScrollOn){
		XEle_KillXCTimer(m_hEle, kAutoScrollTimerId);
		m_autoScrollOn = false;
	}

	// 区内: 沿用旧逻辑, 命中测试 → 更新 caret.
	float lx, ly; PtToLayout(pt, &lx, &ly);
	int pos = HitTestPoint(lx, ly);
	if (pos != m_caret){
		m_caret = pos;
		RedrawSelf();
		UpdateCaret();
	}
	return 1;
}

// 拖选自动滚动单步: 读 m_lastDragPt → 算越界量 → ScrollPos 单步 → 把鼠标点投影到边界
// HitTestPoint → 更新 m_caret. 由 OnMouseMoveImpl 与 timer 共用.
void CXEditDW::UpdateAutoScroll(){
	if (!m_mouseDown || !m_hEle) return;
	borderSize_ bs;
	XEle_GetBorderSize(m_hEle, &bs);
	int viewW = XSView_GetViewWidth (m_hEle);
	int viewH = XSView_GetViewHeight(m_hEle);
	if (viewW < 1 || viewH < 1) return;
	int left   = (int)bs.leftSize;
	int top    = (int)bs.topSize;
	int right  = left + viewW;
	int bottom = top  + viewH;

	// 单步滚动量: 越界距离 / 4, clamp [4, 48] 逻辑像素. 既保证靠近边界滚得慢 (精细),
	// 又保证拖得远滚得快 (响应).
	auto stepBy = [](int over) -> int {
		int a = over < 0 ? -over : over;
		int s = a / 4;
		if (s < 4)  s = 4;
		if (s > 48) s = 48;
		return over < 0 ? -s : s;
	};

	int scrollX = XSView_GetViewPosH(m_hEle);
	int scrollY = XSView_GetViewPosV(m_hEle);
	int dx = 0, dy = 0;
	if      (m_lastDragPt.x < left ) dx = stepBy(m_lastDragPt.x - left);    // 负 → 左滚
	else if (m_lastDragPt.x > right) dx = stepBy(m_lastDragPt.x - right);   // 正 → 右滚
	if (m_multiLine){
		if      (m_lastDragPt.y < top   ) dy = stepBy(m_lastDragPt.y - top);
		else if (m_lastDragPt.y > bottom) dy = stepBy(m_lastDragPt.y - bottom);
	}
	if (dx) XSView_ScrollPosXH(m_hEle, scrollX + dx);
	if (dy) XSView_ScrollPosYV(m_hEle, scrollY + dy);

	// 把越界鼠标投影到边界后命中测试, 让 caret 跟着滚动条 *尾部* 走 (与原生 EditControl 同).
	// 这样选区随滚动自然延伸到屏幕边界对应的文本位置.
	POINT proj = m_lastDragPt;
	if      (proj.x < left ) proj.x = left;
	else if (proj.x > right) proj.x = right;
	if      (proj.y < top   ) proj.y = top;
	else if (proj.y > bottom) proj.y = bottom;
	float lx, ly; PtToLayout(&proj, &lx, &ly);
	int pos = HitTestPoint(lx, ly);
	if (pos != m_caret){
		m_caret = pos;
		RedrawSelf();
		UpdateCaret();
	}
	else{
		// caret 没动也要重绘 - 否则用户视觉上看不出来在滚.
		RedrawSelf();
	}
}

int CXEditDW::OnLButtonDBClickImpl(HELE /*hEle*/, UINT /*flags*/, POINT* /*pt*/, BOOL* /*pbHandled*/){
	if (m_text.empty()) return 1;
	int s = m_caret, e = m_caret;
	while (s > 0){
		wchar_t c = m_text[s - 1];
		bool isWord = (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z')
			|| (c >= L'0' && c <= L'9') || c == L'_' || c >= 0x4E00;
		if (!isWord) break;
		--s;
	}
	while (e < (int)m_text.size()){
		wchar_t c = m_text[e];
		bool isWord = (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z')
			|| (c >= L'0' && c <= L'9') || c == L'_' || c >= 0x4E00;
		if (!isWord) break;
		++e;
	}
	if (s == e && e < (int)m_text.size()) e = NextCodepoint(e);
	m_anchor = s; m_caret = e;
	RedrawSelf();
	UpdateCaret();
	return 1;
}

//===================================================================
//  焦点 / 定时器
//===================================================================
int CXEditDW::OnSetFocusImpl(HELE /*hEle*/, BOOL* /*pbHandled*/){
	m_focused = true;
	m_caretVisible = true;
	XEle_SetXCTimer(m_hEle, kCaretTimerId, 530);   // 与 Win32 caret 默认闪烁间隔一致
	// XEdit 接口兼容: 获得焦点自动全选 (EnableAutoSelAll 启用后).
	if (m_autoSelAll && !m_text.empty()){
		m_anchor = 0;
		m_caret  = (int)m_text.size();
	}
	RedrawSelf();
	return 0;
}

int CXEditDW::OnKillFocusImpl(HELE /*hEle*/, BOOL* /*pbHandled*/){
	m_focused = false;
	m_caretVisible = false;
	XEle_KillXCTimer(m_hEle, kCaretTimerId);
	// XEdit 接口兼容: 失去焦点自动取消选区 (EnableAutoCancelSel 启用后).
	if (m_autoCancelSel && m_anchor != m_caret){
		m_anchor = m_caret;
	}
	RedrawSelf();
	return 0;
}

int CXEditDW::OnKillCaptureImpl(HELE /*hEle*/, BOOL* /*pbHandled*/){
	// 与 OnLButtonUpImpl 同样的清理路径. capture 被外部夺走时 mouseUp 收不到, 必须靠这里
	// 把 m_mouseDown / auto-scroll timer 都关掉, 否则 timer 在后台一直滚.
	if (m_mouseDown){
		m_mouseDown = false;
		// 不再调 XEle_SetCapture(FALSE) - 此事件本身就是 *已经* 失去 capture, 重复设会无效.
	}
	if (m_autoScrollOn){
		XEle_KillXCTimer(m_hEle, kAutoScrollTimerId);
		m_autoScrollOn = false;
	}
	return 0;
}

int CXEditDW::OnTimerImpl(HELE /*hEle*/, UINT timerId, BOOL* /*pbHandled*/){
	if (timerId == kCaretTimerId){
		m_caretVisible = !m_caretVisible;
		RedrawSelf();
	}
	else if (timerId == kAutoScrollTimerId){
		// 鼠标按住期间, 即使停在边外不动也持续滚动. 一旦 mouseUp / 失 capture, OnLButtonUpImpl
		// 与 OnKillCaptureImpl 各自 Kill timer; 这里再防御性 Kill 一次, 防止竞态遗漏.
		if (m_mouseDown){
			UpdateAutoScroll();
		}else if (m_autoScrollOn){
			XEle_KillXCTimer(m_hEle, kAutoScrollTimerId);
			m_autoScrollOn = false;
		}
	}
	return 0;
}

int CXEditDW::OnDestroyEndImpl(HELE /*hEle*/, BOOL* /*pbHandled*/){
	// 元素及其子对象已全部销毁完毕 (XCGUI 级联结束). 此时:
	//   - type=0 (HFONTX) / type=1 (HIMAGE) 还存活, 我们持有所有权, 必须 Release.
	//   - type=2 (UI 对象) 的 HELE 已被 XCGUI 级联销毁, XC_IsHELE 返回 FALSE,
	//     ReleaseStyleSlot 自动跳过 XEle_Destroy, 不会双重销毁.
	// 析构 (~CXEditDW) 跑在 wWinMain 退栈时, 比 XExitXCGUI 的 "未释放资源检查" 晚, 仅做兜底.
	ReleaseDWriteResources();
	ReleaseAllStyleResources();
	m_hEle = NULL;   // 后续 API 访问会被各处 if(!m_hEle) return 守护掉
	return 0;
}

//===================================================================
//  键盘
//===================================================================
int CXEditDW::OnKeyDownImpl(HELE /*hEle*/, WPARAM wParam, LPARAM /*lParam*/, BOOL* /*pbHandled*/){
	bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
	bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;

	if (ctrl){
		switch (wParam){
		case 'A': SelectAll(); return 1;
		case 'C': ClipboardCopy(); return 1;
		case 'X': ClipboardCut(); return 1;
		case 'V': ClipboardPaste(); return 1;
		case 'Z': if (shift) Redo(); else Undo(); return 1;
		case 'Y': Redo(); return 1;
		}
	}

	switch (wParam){
	case VK_LEFT:
		if (HasSelectionInner() && !shift){
			int s, e; GetSelectionRangeInner(s, e); m_caret = m_anchor = s;
		}
		else{
			int p = PrevCodepoint(m_caret); m_caret = p; if (!shift) m_anchor = p;
		}
		AfterCursorMove();
		return 1;
	case VK_RIGHT:
		if (HasSelectionInner() && !shift){
			int s, e; GetSelectionRangeInner(s, e); m_caret = m_anchor = e;
		}
		else{
			int p = NextCodepoint(m_caret); m_caret = p; if (!shift) m_anchor = p;
		}
		AfterCursorMove();
		return 1;
	case VK_UP:   MoveCaretByLine(-1, shift); return 1;
	case VK_DOWN: MoveCaretByLine(+1, shift); return 1;
	case VK_HOME: MoveCaretLineEdge(false, ctrl, shift); return 1;
	case VK_END:  MoveCaretLineEdge(true,  ctrl, shift); return 1;
	case VK_BACK:
		if (m_readOnly) return 1;
		if (HasSelectionInner()) DeleteSelect();
		else if (m_caret > 0){
			PushUndo();
			int p = PrevCodepoint(m_caret);
			EraseCharsRaw(p, m_caret - p);
			m_caret = m_anchor = p;
			m_scrollToCaretPending = true;
			RedrawSelf();
			UpdateCaret();
		}
		return 1;
	case VK_DELETE:
		if (m_readOnly) return 1;
		if (HasSelectionInner()) DeleteSelect();
		else if (m_caret < (int)m_text.size()){
			PushUndo();
			int p = NextCodepoint(m_caret);
			EraseCharsRaw(m_caret, p - m_caret);
			m_scrollToCaretPending = true;
			RedrawSelf();
			UpdateCaret();
		}
		return 1;
	case VK_RETURN:
		if (m_readOnly) return 1;
		// XCGUI 约定换行符为单 '\n'. 不能插 "\r\n":
		// 本模块段切分 (ParaOnTextInserted / ParaRebuildFromText) *仅* 按 '\n' 切段,
		// '\r' 会留在上一段文本里; 而 DirectWrite 的 IDWriteTextLayout 默认又把 '\r'
		// 当成硬换行机会 -> 上一段 layout 内部再断一行, 视觉上一次 Enter 表现为两次换行.
		if (m_multiLine){ const wchar_t kEol[] = L"\n"; InsertTextAtCursor(kEol, 1); }
		return 1;
	}
	return 0;
}

int CXEditDW::OnCharImpl(HELE /*hEle*/, WPARAM wParam, LPARAM /*lParam*/, BOOL* /*pbHandled*/){
	if (m_readOnly) return 1;
	wchar_t c = (wchar_t)wParam;
	if (c < 0x20){
		if (c == L'\t' && m_multiLine){
			const wchar_t t[] = L"\t";
			InsertTextAtCursor(t, 1);
		}
		return 1;
	}
	// SMP 字符通过 WM_CHAR 时, Windows 会先后发两次 (高代理 + 低代理), 这里要拼起来.
	if (IsHighSurrogate(c)){ m_pendingHigh = c; return 1; }
	if (IsLowSurrogate(c) && m_pendingHigh){
		wchar_t pair[2] = { m_pendingHigh, c };
		m_pendingHigh = 0;
		InsertTextAtCursor(pair, 2);
		return 1;
	}
	m_pendingHigh = 0;
	InsertTextAtCursor(&c, 1);
	return 1;
}

//===================================================================
//  编辑核心
//===================================================================
void CXEditDW::InsertTextAtCursor(const wchar_t* p, int len){
	if (!p || len <= 0) return;
	PushUndo();
	if (HasSelectionInner()){
		int s, e; GetSelectionRangeInner(s, e);
		EraseCharsRaw(s, e - s);
		InsertCharsRaw(s, p, len, m_curStyle);
		m_caret = m_anchor = s + len;
	}
	else{
		InsertCharsRaw(m_caret, p, len, m_curStyle);
		m_caret += len;
		m_anchor = m_caret;
	}
	m_scrollToCaretPending = true;   // EnsureLayout post-rebuild hook 用新 layout 精确滚动
	RedrawSelf();
	UpdateCaret();
}

void CXEditDW::PushUndo(){
	// 节流: 同一连续编辑会话只 push 一次起点, 之后的小改动复用. 显著降低长文本下连续
	// 打字 / 删除时的 deep-copy 开销 (600KB 文本一次 push ~5ms, 节流后大部分键入 0ms).
	const DWORD now = ::GetTickCount();
	const bool  hasPrev = !m_undoStack.empty() && m_lastUndoCaret >= 0;
	const bool  inWindow = hasPrev && (now - m_lastUndoTick < kUndoMergeMs);
	const int   caretDelta = hasPrev ? (m_caret - m_lastUndoCaret) : 0;
	const bool  caretContiguous = (caretDelta >= -1 && caretDelta <= 1);
	if (hasPrev && inWindow && caretContiguous){
		// 合并: 不 push 新 state, 只更新 *节流时间戳 + 当前 caret 锚点*. 栈顶 state 仍是
		// 该会话起点, Undo 时一次性回到起点 (与 Notepad 同).
		m_lastUndoTick  = now;
		m_lastUndoCaret = m_caret;
		// Redo 栈仍要清: 用户开始新编辑 (即使被合并到上一会话), redo 链已不可达.
		m_redoStack.clear();
		return;
	}

	_XEditDW_UndoState s;
	s.text = m_text;
	s.charStyle = m_charStyle;
	s.caret = m_caret;
	s.anchor = m_anchor;
	m_undoStack.push_back(std::move(s));
	if (m_undoStack.size() > kMaxUndoDepth) m_undoStack.erase(m_undoStack.begin());
	m_redoStack.clear();
	m_lastUndoTick  = now;
	m_lastUndoCaret = m_caret;
}

void CXEditDW::AfterCursorMove(){
	EnsureCaretVisible();
	RedrawSelf();
	UpdateCaret();
}

// 多行模式下垂直方向移动一行
void CXEditDW::MoveCaretByLine(int dir, bool extendSel){
	if (!m_multiLine) return;
	EnsureLayout();
	if (m_paragraphs.empty()) return;
	float cx = 0, cy = 0, ch = 0;
	if (!GetCaretPoint(m_caret, &cx, &cy, &ch)) return;
	float targetY = cy + (dir > 0 ? ch * 1.05f : -ch * 0.5f);
	if (targetY < 0) targetY = 0;
	// HitTestPoint 按段委派, 与单 layout 时同语义 (返回绝对 textPos).
	int pos = HitTestPoint(cx, targetY);
	pos = ClampPos(pos);
	if (pos > 0 && pos < (int)m_text.size()
		&& IsLowSurrogate(m_text[pos]) && IsHighSurrogate(m_text[pos - 1])){
		pos += 1;
	}
	m_caret = pos;
	if (!extendSel) m_anchor = m_caret;
	AfterCursorMove();
}

// Home / End
void CXEditDW::MoveCaretLineEdge(bool toEnd, bool ctrl, bool extendSel){
	int target = m_caret;
	if (ctrl){
		target = toEnd ? (int)m_text.size() : 0;
	}
	else{
		if (toEnd){
			int n = (int)m_text.size(); int p = m_caret;
			while (p < n && m_text[p] != L'\n' && m_text[p] != L'\r') ++p;
			target = p;
		}
		else{
			int p = m_caret;
			while (p > 0 && m_text[p - 1] != L'\n' && m_text[p - 1] != L'\r') --p;
			target = p;
		}
	}
	m_caret = target;
	if (!extendSel) m_anchor = m_caret;
	AfterCursorMove();
}

//===================================================================
//  剪贴板帮助
//===================================================================
bool CXEditDW::CopyToClipboardImpl(const std::wstring& s){
	if (!::OpenClipboard(NULL)) return false;
	::EmptyClipboard();
	size_t bytes = (s.size() + 1) * sizeof(wchar_t);
	HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (!h){ ::CloseClipboard(); return false; }
	void* p = ::GlobalLock(h);
	if (p){ memcpy(p, s.c_str(), bytes); ::GlobalUnlock(h); }
	::SetClipboardData(CF_UNICODETEXT, h);
	::CloseClipboard();
	return true;
}

bool CXEditDW::GetClipboardTextImpl(std::wstring& out){
	if (!::OpenClipboard(NULL)) return false;
	HANDLE h = ::GetClipboardData(CF_UNICODETEXT);
	if (!h){ ::CloseClipboard(); return false; }
	const wchar_t* p = (const wchar_t*)::GlobalLock(h);
	if (!p){ ::CloseClipboard(); return false; }
	out = p;
	::GlobalUnlock(h);
	::CloseClipboard();
	return true;
}
