//============================================================================
// module_xcgui_uitool_loading.cpp — CXLoading 实现
//============================================================================
//============================================================================
// =====================================================================
// CXLoading 实现
// =====================================================================
//
// 设计思路:
//   - 共享 16ms 心跳 timer 通过 *宿主自己* 的 XE_XC_TIMER 派发; 每个宿主一个 timer
//     (id = kLoad_TimerId), 不依赖共享 g 单例 — 多个 loading 同时跑无冲突.
//   - XE_PAINT 接管 (用户要求 XE_PAINT 不是 XE_PAINT_END):
//       running=TRUE  时 -> 画 loading + *pbHandled = TRUE
//       running=FALSE 时 -> *pbHandled = FALSE (让出, 元素自己画)
//   - 元素销毁时 XE_DESTROY 自动清理 entry.
//   - 所有几何/动画用元素的 *逻辑* 像素 (与 module_xcgui_video 经验一致, hDraw
//     在 D2D 主路径下接受逻辑坐标, XCGUI 内部按 DPI 缩到物理).
//   - 5 种动画 ease 算法:
//       spinner: 单周期 seamless snake (grow/shrink + 720° 补偿, ease-in-out)
//       dots:    sin (正弦跳动)
//       spokes:  阶梯 (经典 beachball)
//       pulse:   ease-out cubic (扩散)
//       bars:    sin (高度起伏)
//============================================================================
namespace {

// 为避免与其它子模块常量重名, 全部用 _XLoad_ / kLoad_ 前缀.
constexpr UINT kLoad_TimerId      = 0x7200;       // 不与 tooltip 的 0x7100~7102 冲突
constexpr int  kLoad_TickMs       = 16;           // 60Hz
constexpr int  kLoad_DefaultSize  = 40;
constexpr int  kLoad_TextGap      = 8;            // 动画与下方文本的间距
constexpr int  kLoad_TextFontPt   = 9;

constexpr float kLoad_Pi = 3.14159265358979323846f;

// 无缝 indeterminate spinner — 单时间轴 snake (非 SMIL dash 双通道)
// 几何参考 SVG viewBox 24 (r=9.5, stroke=3); 动画为 grow/shrink + baseRot 720° 闭环
constexpr float kLoad_Spinner_MinArcDeg  = 24.0f;
constexpr float kLoad_Spinner_MaxArcDeg  = 42.0f / (2.0f * kLoad_Pi * 9.5f) * 360.0f;  // ≈253.3°
constexpr float kLoad_Spinner_StrokeRat  = 0.125f;                        // 3/24
constexpr float kLoad_Spinner_RadiusRat  = 9.5f / 24.0f;
constexpr float kLoad_Spinner_SweepEps   = 0.01f;

//============================================================================
// 注册项
//============================================================================
struct _XLoad_Entry
{
	HXCGUI            hHost          = NULL;
	BOOL              isWnd          = FALSE;             // FALSE=HELE, TRUE=HWINDOW
	BOOL              ownEle         = FALSE;             // TRUE=Create 创建 (vs Attach)
	BOOL              running        = TRUE;              // Start/Stop
	xloading_style_   style          = xloading_style_spinner;
	xuitool_theme_   theme          = xuitool_theme_dark;
	int               sizeCx         = kLoad_DefaultSize;
	int               sizeCy         = kLoad_DefaultSize;
	std::wstring      text;
	COLORREF          customText     = _XUITool::kDarkText;
	COLORREF          customBg       = _XUITool::kDarkBg;
	COLORREF          customAccent   = _XUITool::kDarkAccent;
	int               cornerLT       = 0;
	int               cornerRT       = 0;
	int               cornerRB       = 0;
	int               cornerLB       = 0;
	float             speed          = 1.0f;
	DWORD             startTick      = 0;
	BOOL              eventsHooked   = FALSE;
	// 字体 (per-host: 改字号无副作用, 多个 host 各自独立)
	int               fontPt         = kLoad_TextFontPt;  // 默认 9
	HFONTX            hFont          = NULL;              // 懒创建, 改字号时销毁重建
	// 用户面对的句柄 — 仅 AttachWnd 路径下非 NULL.
	//   AttachWnd(hWnd) 时, 我们建子 HELE 作为真实绘制载体 (= registry key, = hHost),
	//   hUserAlias 记录用户传入的 hWnd. 后续 Set*/Stop/Detach(hWnd) 走 _XLoad_Resolve 翻译过来.
	//   非 NULL 还兼任 "Detach 时销毁子 HELE" 的标志位.
	HXCGUI            hUserAlias     = NULL;
};

struct _XLoad_Global
{
	std::unordered_map<HXCGUI, _XLoad_Entry> registry;     // key = 真实绘制宿主 (HELE 或 HWINDOW)
	std::unordered_map<HXCGUI, HXCGUI>       userToHost;   // 用户传入句柄 → registry key (仅 AttachWnd 用)
};

_XLoad_Global& LG(){
	static _XLoad_Global s;
	return s;
}

// 懒创建/重建该 entry 的字体. 调用时机:
//   - OnPaint 测量/绘制文本前
//   - SetFontSize 改字号 (先销毁旧的)
void _XLoad_EnsureFont(_XLoad_Entry& e){
	if (!e.hFont){
		e.hFont = XFont_CreateEx(L"微软雅黑", e.fontPt, 0);
	}
}
void _XLoad_DestroyFont(_XLoad_Entry& e){
	if (e.hFont){ XFont_Destroy(e.hFont); e.hFont = NULL; }
}

//============================================================================
// 主题色解析
//============================================================================
struct _XLoad_Colors
{
	COLORREF text;
	COLORREF bg;
	COLORREF accent;
};

void _XLoad_ResolveColors(const _XLoad_Entry& e, _XLoad_Colors* out)
{
	_XUITool::ThemePalette pal;
	_XUITool::ResolvePalette(e.theme, e.customText, e.customBg,
		e.customAccent, &pal);
	out->text   = pal.text;
	out->bg     = pal.bg;
	out->accent = pal.accent;
}

// 强调色 -> 非强调色 (alpha 衰减, RGB 通道一致原则保持)
inline COLORREF _XLoad_FadeAccent(COLORREF accent, BYTE alpha){
	return _XUITool::WithAlpha(accent, alpha);
}

//============================================================================
// 缓动函数 (CSS 标准: cubic-bezier(0.65,0,0.35,1) 的 closed-form 近似)
//============================================================================
inline float _XLoad_EaseInOutCubic(float t){
	return t < 0.5f ? 4.0f * t * t * t
	                : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f;
}
inline float _XLoad_EaseOutCubic(float t){
	return 1.0f - powf(1.0f - t, 3.0f);
}

//============================================================================
// 注册表查询
//============================================================================
// AttachWnd 模式下用户拿着 hWnd, 我们存的 entry 却以子 HELE 为 key.
// 所有 public API 进来先 Resolve 一下 — 若 hUser 在 userToHost 表里, 翻成真实 key.
// 否则原样返回 (Create/AttachEle/直 SetXxx(hWnd) 路径仍然透传).
HXCGUI _XLoad_Resolve(HXCGUI hUser){
	if (!hUser) return NULL;
	auto& g = LG();
	auto it = g.userToHost.find(hUser);
	return (it != g.userToHost.end()) ? it->second : hUser;
}

_XLoad_Entry* _XLoad_GetEntry(HXCGUI hHost){
	auto& g = LG();
	auto it = g.registry.find(_XLoad_Resolve(hHost));
	return (it == g.registry.end()) ? NULL : &it->second;
}

// 前向声明 (回调里用到)
int CALLBACK _XLoad_OnPaint  (HXCGUI hSelf, HDRAW hDraw,    BOOL* pbHandled);
int CALLBACK _XLoad_OnTimer  (HXCGUI hSelf, UINT nTimerID,  BOOL* pbHandled);
int CALLBACK _XLoad_OnDestroy(HXCGUI hSelf, BOOL* pbHandled);

void _XLoad_StartTimer(HXCGUI hHost){
	if (XC_IsHELE(hHost))         XEle_SetXCTimer((HELE)hHost,    kLoad_TimerId, kLoad_TickMs);
	else if (XC_IsHWINDOW(hHost)) XWnd_SetXCTimer((HWINDOW)hHost, kLoad_TimerId, kLoad_TickMs);
}
void _XLoad_StopTimer(HXCGUI hHost){
	if (XC_IsHELE(hHost))         XEle_KillXCTimer((HELE)hHost,    kLoad_TimerId);
	else if (XC_IsHWINDOW(hHost)) XWnd_KillXCTimer((HWINDOW)hHost, kLoad_TimerId);
}
void _XLoad_Redraw(HXCGUI hHost){
	if (XC_IsHELE(hHost))         XEle_Redraw((HELE)hHost,    FALSE);
	else if (XC_IsHWINDOW(hHost)) XWnd_Redraw((HWINDOW)hHost, FALSE);
}
void _XLoad_HookEvents(HXCGUI hHost, _XLoad_Entry& e){
	if (e.eventsHooked) return;
	if (XC_IsHELE(hHost)){
		HELE hEle = (HELE)hHost;
		XEle_RegEventC1(hEle, XE_PAINT,    (void*)&_XLoad_OnPaint);
		XEle_RegEventC1(hEle, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
		XEle_RegEventC1(hEle, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
	} else if (XC_IsHWINDOW(hHost)){
		HWINDOW hWnd = (HWINDOW)hHost;
		XWnd_RegEventC1(hWnd, XE_PAINT,    (void*)&_XLoad_OnPaint);
		XWnd_RegEventC1(hWnd, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
		XWnd_RegEventC1(hWnd, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
	}
	e.eventsHooked = TRUE;
}

// 通用 Set* 入口: 取/造 entry. 与 CXTooltip 的 _GetOrCreate 同思路, 让 Add/Set 顺序无关.
//   AttachWnd 路径: 用户拿着 hWnd, 先 Resolve 翻成内部子 HELE, 再走旧逻辑.
_XLoad_Entry* _XLoad_GetOrCreate(HXCGUI hHost){
	if (!hHost) return NULL;
	HXCGUI hKey = _XLoad_Resolve(hHost);
	if (!XC_IsHELE(hKey) && !XC_IsHWINDOW(hKey)) return NULL;
	auto& g = LG();
	auto& e = g.registry[hKey];
	e.hHost = hKey;
	e.isWnd = XC_IsHWINDOW(hKey) ? TRUE : FALSE;
	_XLoad_HookEvents(hKey, e);
	return &e;
}

//============================================================================
// 5 种动画绘制
//
// 输入约定:
//   hHost    — 宿主 HELE / HWINDOW (painter 用它取 DPI / window-client 物理坐标)
//   (cx, cy) — 动画中心, 宿主客户区*逻辑*像素
//   size     — 动画绘制方形边长, *逻辑*像素
//   tNorm    — 当前周期内进度 [0, 1)
//   accent   — 强调色 (XCGUI RGBA, 字节序 R | G<<8 | B<<16 | A<<24)
//
// D2D 优先路径: 用 ID2D1RenderTarget 直接画 PathGeometry/StrokeStyle, 端帽
//   D2D_CAP_STYLE_ROUND 原生, 与笔画严格对齐, sub-pixel 精度无模糊.
// GDI+ 兜底:    XDraw_DrawArcF/DrawLine + 手动 cap 圆模拟, 笔画宽手动 ×dpi 物理化.
//============================================================================

// Paint 上下文 — 一次构造贯穿所有 painter, 含坐标转换 + DPI + D2D RT.
//   *Phys 字段: 动画中心+边长在*窗口客户区物理像素*中的值, 与 D2D RT 同坐标系.
//   *Log  字段: 宿主本地逻辑坐标 (D2D 不可用时给 XDraw_* 用).
struct _XLoad_PaintCtx{
	HDRAW              hDraw;
	HXCGUI             hHost;
	ID2D1RenderTarget* rt;        // D2D 路径; NULL 表示 GDI+ 兜底
	float              dpiScale;
	// D2D 路径用
	float              cxPhys;
	float              cyPhys;
	float              sizePhys;
	// GDI+ 兜底路径用 (= 原始入参)
	int                cxLog;
	int                cyLog;
	int                sizeLog;
};

// 从 hDraw + hHost + (cx,cy,size) 逻辑值构造上下文.
void _XLoad_BuildCtx(_XLoad_PaintCtx& ctx, HDRAW hDraw, HXCGUI hHost,
	int cxLog, int cyLog, int sizeLog)
{
	ctx.hDraw   = hDraw;
	ctx.hHost   = hHost;
	ctx.cxLog   = cxLog;
	ctx.cyLog   = cyLog;
	ctx.sizeLog = sizeLog;
	ctx.rt      = (ID2D1RenderTarget*)XDraw_GetD2dRenderTarget(hDraw);

	HWINDOW hWnd = NULL;
	if (XC_IsHELE(hHost))         hWnd = XWidget_GetHWINDOW(hHost);
	else if (XC_IsHWINDOW(hHost)) hWnd = (HWINDOW)hHost;
	int dpi = hWnd ? XWnd_GetDPI(hWnd) : 96;
	if (dpi <= 0) dpi = 96;
	ctx.dpiScale = dpi / 96.0f;

	// 元素: 用 XEle_GetWndClientRectDPI 直接拿物理 rect, + 元素内逻辑偏移*dpiScale.
	// 窗口: D2D RT 原点已是窗口客户区 (0,0) 物理, 直接 *dpiScale.
	float ox = 0.0f, oy = 0.0f;
	if (XC_IsHELE(hHost)){
		RECT rcEle{};
		XEle_GetWndClientRectDPI((HELE)hHost, &rcEle);
		ox = (float)rcEle.left;
		oy = (float)rcEle.top;
	}
	ctx.cxPhys   = ox + cxLog   * ctx.dpiScale;
	ctx.cyPhys   = oy + cyLog   * ctx.dpiScale;
	ctx.sizePhys = sizeLog * ctx.dpiScale;
}

// XCGUI RGBA → D2D ColorF, 可带 alpha override (0..255, -1 表示用原 alpha).
inline D2D1_COLOR_F _XLoad_ToColorF(COLORREF c, int alphaOverride = -1){
	float r = (c        & 0xFF) / 255.0f;
	float g = ((c >> 8) & 0xFF) / 255.0f;
	float b = ((c >> 16)& 0xFF) / 255.0f;
	float a = (alphaOverride >= 0)
		? (alphaOverride / 255.0f)
		: ((c >> 24) & 0xFF) / 255.0f;
	return D2D1::ColorF(r, g, b, a);
}

// 创建 round-cap stroke style, 调用方负责 Release.
ID2D1StrokeStyle* _XLoad_MakeRoundStroke(ID2D1Factory* fac){
	if (!fac) return NULL;
	D2D1_STROKE_STYLE_PROPERTIES p = D2D1::StrokeStyleProperties();
	p.startCap = D2D1_CAP_STYLE_ROUND;
	p.endCap   = D2D1_CAP_STYLE_ROUND;
	p.dashCap  = D2D1_CAP_STYLE_ROUND;
	p.lineJoin = D2D1_LINE_JOIN_ROUND;
	ID2D1StrokeStyle* s = NULL;
	fac->CreateStrokeStyle(p, NULL, 0, &s);
	return s;
}

// 无缝 indeterminate spinner — 单周期 snake:
//   tNorm<0.5: 尾端固定, 头端拉开 (minArc→maxArc), 整体随 baseRot·tNorm 旋转
//   tNorm≥0.5: 头端固定, 尾端追上 (maxArc→minArc), start 追加 (maxArc-arcLen)
//   baseRot = 720° - (maxArc-minArc), 保证 tNorm=0/1 时 start+sweep 视觉同态, 无硬切
struct _XLoad_SpinnerPhase{
	float startDeg;
	float sweepDeg;
};

void _XLoad_ComputeSpinnerPhase(float tNorm, _XLoad_SpinnerPhase* out)
{
	const float minArc  = kLoad_Spinner_MinArcDeg;
	const float maxArc  = kLoad_Spinner_MaxArcDeg;
	const float arcDelta = maxArc - minArc;
	const float baseRot  = 720.0f - arcDelta;

	float arcLen, arcStart;
	if (tNorm < 0.5f){
		float u = _XLoad_EaseInOutCubic(tNorm * 2.0f);
		arcLen   = minArc + arcDelta * u;
		arcStart = -90.0f + baseRot * tNorm;
	} else {
		float u = _XLoad_EaseInOutCubic((tNorm - 0.5f) * 2.0f);
		arcLen   = maxArc - arcDelta * u;
		arcStart = -90.0f + baseRot * tNorm + (maxArc - arcLen);
	}
	out->startDeg = arcStart;
	out->sweepDeg = arcLen;
}

void _XLoad_PaintSpinner(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	_XLoad_SpinnerPhase ph{};
	_XLoad_ComputeSpinnerPhase(tNorm, &ph);
	if (ph.sweepDeg <= kLoad_Spinner_SweepEps) return;

	// 几何 — SVG 比例 (stroke 12.5%, 中心线半径 9.5/24).
	float sizeLog  = ctx.sizePhys / ctx.dpiScale;
	float strokeLog = sizeLog * kLoad_Spinner_StrokeRat;
	if (strokeLog < 2.0f) strokeLog = 2.0f;
	float strokePhys = floorf(strokeLog * ctx.dpiScale + 0.5f);
	if (strokePhys < 1.0f) strokePhys = 1.0f;
	float rPhys = ctx.sizePhys * kLoad_Spinner_RadiusRat;
	if (rPhys < strokePhys * 0.5f) rPhys = strokePhys * 0.5f;

	float startDeg = ph.startDeg;
	float sweepDeg = ph.sweepDeg;
	float startRad = startDeg             * (kLoad_Pi / 180.0f);
	float endRad   = (startDeg + sweepDeg) * (kLoad_Pi / 180.0f);

	if (ctx.rt){
		ID2D1Factory* fac = NULL;
		ctx.rt->GetFactory(&fac);
		if (!fac) return;

		ID2D1StrokeStyle* roundStroke = _XLoad_MakeRoundStroke(fac);
		ID2D1SolidColorBrush* brush = NULL;
		ctx.rt->CreateSolidColorBrush(_XLoad_ToColorF(accent, 255), &brush);
		if (brush){
			ID2D1PathGeometry* path = NULL;
			if (SUCCEEDED(fac->CreatePathGeometry(&path)) && path){
				ID2D1GeometrySink* sink = NULL;
				if (SUCCEEDED(path->Open(&sink)) && sink){
					D2D1_POINT_2F p0 = D2D1::Point2F(
						ctx.cxPhys + rPhys * cosf(startRad),
						ctx.cyPhys + rPhys * sinf(startRad));
					D2D1_POINT_2F p1 = D2D1::Point2F(
						ctx.cxPhys + rPhys * cosf(endRad),
						ctx.cyPhys + rPhys * sinf(endRad));
					sink->BeginFigure(p0, D2D1_FIGURE_BEGIN_HOLLOW);
					D2D1_ARC_SEGMENT seg = {
						p1,
						D2D1::SizeF(rPhys, rPhys),
						0.0f,
						D2D1_SWEEP_DIRECTION_CLOCKWISE,
						(sweepDeg > 180.0f)
							? D2D1_ARC_SIZE_LARGE
							: D2D1_ARC_SIZE_SMALL,
					};
					sink->AddArc(seg);
					sink->EndFigure(D2D1_FIGURE_END_OPEN);
					sink->Close();
					SafeRelease(sink);
				}
				ctx.rt->DrawGeometry(path, brush, strokePhys, roundStroke);
				SafeRelease(path);
			}
			SafeRelease(brush);
		}
		SafeRelease(roundStroke);
		fac->Release();
		return;
	}

	// GDI+ 兜底
	float rLog    = (float)ctx.sizeLog * kLoad_Spinner_RadiusRat;
	if (rLog < 1.0f) rLog = 1.0f;
	float boxLog  = rLog * 2.0f;
	float boxLLog = ctx.cxLog - rLog;
	float boxTLog = ctx.cyLog - rLog;

	XDraw_SetLineWidthF(ctx.hDraw, strokePhys);
	XDraw_SetBrushColor(ctx.hDraw, accent);
	XDraw_DrawArcF(ctx.hDraw, boxLLog, boxTLog, boxLog, boxLog, startDeg, sweepDeg);

	float capR = strokeLog * 0.5f;
	float sxL = ctx.cxLog + rLog * cosf(startRad);
	float syL = ctx.cyLog + rLog * sinf(startRad);
	float exL = ctx.cxLog + rLog * cosf(endRad);
	float eyL = ctx.cyLog + rLog * sinf(endRad);
	RECTF cap1{ sxL - capR, syL - capR, sxL + capR, syL + capR };
	RECTF cap2{ exL - capR, eyL - capR, exL + capR, eyL + capR };
	XDraw_FillEllipseF(ctx.hDraw, &cap1);
	XDraw_FillEllipseF(ctx.hDraw, &cap2);
}

// 2) Win10 / Win8 启动风格 5-dot Spinner.
//    设计要点 (修正前两版的问题):
//      *) 5 dot 全程可见 → 无 fade in/out, 不会出现"12 点钟有个永远静止的半透明点"
//      *) 每 dot 各自做 *完整 1 周* 的 ease-in-out cubic 旋转, 相位等分错开 1/N
//      *) 因为 ease-in-out → 同一时刻 5 dot 在圆周上分布*不均*:
//         加速段彼此拉开, 减速段彼此收拢 → 视觉上有"加速度"质感, 像跑马灯/Win10 boot
//      *) 360° 走完无缝接续 (相位累加 mod 1, ease 在 [0,1] 闭合)
//
//    D2D 主路径: ID2D1RenderTarget::FillEllipse, sub-pixel 精度
//    GDI+ 兜底:   XDraw_FillEllipseF, 同样 sub-pixel
void _XLoad_PaintDots(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	constexpr int   N        = 5;
	constexpr float startAng = -90.0f;   // 12 点钟出发, 顺时针

	float dotR  = ctx.sizePhys / 12.0f;
	if (dotR < 2.0f * ctx.dpiScale) dotR = 2.0f * ctx.dpiScale;
	float ringR = ctx.sizePhys * 0.5f - dotR;
	if (ringR < dotR) ringR = dotR;

	// D2D 工厂资源 (一次性创建, 多 dot 复用)
	ID2D1SolidColorBrush* brush = NULL;
	if (ctx.rt){
		ctx.rt->CreateSolidColorBrush(_XLoad_ToColorF(accent, 255), &brush);
		if (!brush) return;
	}

	for (int i = 0; i < N; ++i){
		// 第 i 个 dot 的相位 [0, 1)
		float phase = fmodf(tNorm + (float)i / N, 1.0f);
		// ease-in-out cubic 提供 *加速度*, 60%; 线性 40% 提供"基础速度",
		// 防止 dot 在 phase 0/1 附近完全停滞 (旧版 user 反馈"12 点钟有静止圆点"问题).
		// 数学保证: 边界处 (phase=0 或 1) eased=0/1, 总进度 = phase=0/1 → 与下个周期接合连续.
		float eased = _XLoad_EaseInOutCubic(phase);
		float prog  = eased * 0.6f + phase * 0.4f;
		float angleDeg = startAng + prog * 360.0f;
		float angleRad = angleDeg * (kLoad_Pi / 180.0f);

		if (ctx.rt){
			float xC = ctx.cxPhys + ringR * cosf(angleRad);
			float yC = ctx.cyPhys + ringR * sinf(angleRad);
			D2D1_ELLIPSE el = D2D1::Ellipse(
				D2D1::Point2F(xC, yC), dotR, dotR);
			ctx.rt->FillEllipse(el, brush);
		} else {
			// GDI+ 兜底 — 用宿主本地逻辑坐标. dotR/ringR 反推回逻辑.
			float dotRLog  = dotR  / ctx.dpiScale;
			float ringRLog = ringR / ctx.dpiScale;
			float xC = ctx.cxLog + ringRLog * cosf(angleRad);
			float yC = ctx.cyLog + ringRLog * sinf(angleRad);
			RECTF rc{ xC - dotRLog, yC - dotRLog,
			          xC + dotRLog, yC + dotRLog };
			XDraw_SetBrushColor(ctx.hDraw, accent);
			XDraw_FillEllipseF(ctx.hDraw, &rc);
		}
	}

	SafeRelease(brush);
}

// 3) Spokes — 12 条辐条, head 全亮, 其余按距离阶梯衰减 alpha.
//    D2D 路径: ID2D1RenderTarget::DrawLine + StrokeStyle{startCap=endCap=ROUND}
//             → 笔画两端原生圆角, 与笔画完全等宽对齐 (无需手画 cap 圆).
//    GDI+ 兜底: XDraw_DrawLineF + 末端 XDraw_FillEllipseF, cap 直径 = strokeLog
//             (不能 +1, 否则 cap 比笔画粗).
//
//    几何比例 (前版改进, 防 AA 模糊):
//      innerR = size * 0.30 (从 0.25), outerR = size * 0.45 (从 0.50),
//      stroke = size / 14   (从 size / 12)  — 端帽 + 辐条间距更舒展.
void _XLoad_PaintSpokes(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	constexpr int N = 12;
	float innerR_phys = ctx.sizePhys * 0.30f;
	float outerR_phys = ctx.sizePhys * 0.45f;
	float strokeLog   = (ctx.sizePhys / ctx.dpiScale) / 14.0f;
	if (strokeLog < 2.0f) strokeLog = 2.0f;
	float strokePhys  = floorf(strokeLog * ctx.dpiScale + 0.5f);
	if (strokePhys < 1.0f) strokePhys = 1.0f;

	int head = (int)(tNorm * N) % N;

	if (ctx.rt){
		// ---------- D2D 主路径 ----------
		ID2D1Factory* fac = NULL;
		ctx.rt->GetFactory(&fac);
		if (!fac) return;
		ID2D1StrokeStyle* roundStroke = _XLoad_MakeRoundStroke(fac);
		ID2D1SolidColorBrush* brush = NULL;
		ctx.rt->CreateSolidColorBrush(_XLoad_ToColorF(accent, 255), &brush);
		if (brush){
			for (int i = 0; i < N; ++i){
				int dist = (head - i + N) % N;
				BYTE a = (BYTE)(255 - dist * (205 / (N - 1)));
				if (a < 50) a = 50;
				brush->SetColor(_XLoad_ToColorF(accent, a));

				float angle = (i / (float)N) * 2.0f * kLoad_Pi - kLoad_Pi * 0.5f;
				float cs = cosf(angle), sn = sinf(angle);
				D2D1_POINT_2F p1 = D2D1::Point2F(
					ctx.cxPhys + innerR_phys * cs,
					ctx.cyPhys + innerR_phys * sn);
				D2D1_POINT_2F p2 = D2D1::Point2F(
					ctx.cxPhys + outerR_phys * cs,
					ctx.cyPhys + outerR_phys * sn);
				ctx.rt->DrawLine(p1, p2, brush, strokePhys, roundStroke);
			}
			SafeRelease(brush);
		}
		SafeRelease(roundStroke);
		fac->Release();
		return;
	}

	// ---------- GDI+ 兜底 ----------
	float innerLog = innerR_phys / ctx.dpiScale;
	float outerLog = outerR_phys / ctx.dpiScale;
	float capR     = strokeLog * 0.5f;
	XDraw_SetLineWidthF(ctx.hDraw, strokePhys);
	for (int i = 0; i < N; ++i){
		int dist = (head - i + N) % N;
		BYTE a = (BYTE)(255 - dist * (205 / (N - 1)));
		if (a < 50) a = 50;
		COLORREF c = _XLoad_FadeAccent(accent, a);
		XDraw_SetBrushColor(ctx.hDraw, c);
		float angle = (i / (float)N) * 2.0f * kLoad_Pi - kLoad_Pi * 0.5f;
		float cs = cosf(angle), sn = sinf(angle);
		float x1 = ctx.cxLog + innerLog * cs;
		float y1 = ctx.cyLog + innerLog * sn;
		float x2 = ctx.cxLog + outerLog * cs;
		float y2 = ctx.cyLog + outerLog * sn;
		XDraw_DrawLineF(ctx.hDraw, x1, y1, x2, y2);
		RECTF capIn { x1 - capR, y1 - capR, x1 + capR, y1 + capR };
		RECTF capOut{ x2 - capR, y2 - capR, x2 + capR, y2 + capR };
		XDraw_FillEllipseF(ctx.hDraw, &capIn);
		XDraw_FillEllipseF(ctx.hDraw, &capOut);
	}
}

// 4) Pulse — 2 个同心圆 ease-out 扩散 + alpha 衰减.
//    保持 GDI+ 风格 (无明显 cap/对齐问题), 简单走 XDraw_*. D2D / GDI+ 透传一致.
void _XLoad_PaintPulse(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	for (int i = 0; i < 2; ++i){
		float phase = fmodf(tNorm + i * 0.5f, 1.0f);
		float eased = _XLoad_EaseOutCubic(phase);
		float rLog = ctx.sizeLog * (0.15f + 0.35f * eased);
		BYTE a = (BYTE)((1.0f - phase) * 200);
		if (a < 5) continue;
		COLORREF c = _XLoad_FadeAccent(accent, a);
		XDraw_SetBrushColor(ctx.hDraw, c);
		XDraw_SetLineWidthF(ctx.hDraw, 2.0f * ctx.dpiScale);
		RECTF rc{ ctx.cxLog - rLog, ctx.cyLog - rLog,
		          ctx.cxLog + rLog, ctx.cyLog + rLog };
		XDraw_DrawEllipseF(ctx.hDraw, &rc);
	}
}

// 5) Bars — 现代极简胶囊均衡器条 (modern audio visualizer 风, 非 Win10 模仿).
//    设计:
//      *) 4 根 *胶囊形* (full pill) 实心条, 强调色饱和, 无渐变 / 无阴影
//      *) 高度按 smoothstep(sin) 振荡: minH = barW (条宽, 此时形状为圆) →
//         maxH = sizePhys * 0.92  (接近满高)
//      *) 相邻条相位错开 1/N → 视觉上"波"从左向右传播 (Spotify / Apple 风)
//      *) 圆角 = barW / 2 → 永远胶囊形, 高度收缩到 barW 时退化为完美圆 (无视觉跳变)
//      *) 间距 = barW * 0.6 → 修长比例, 不拥挤
//
//    D2D 主路径: ID2D1RenderTarget::FillRoundedRectangle, sub-pixel 精度
//    GDI+ 兜底:  XDraw_FillRoundRect (逻辑 RECT + barW 圆角)
void _XLoad_PaintBars(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	constexpr int   N         = 4;
	constexpr float kBarRatio = 0.13f;   // bar 宽 / size
	constexpr float kGapRatio = 0.08f;   // gap / size  (= 0.6 * kBarRatio)
	constexpr float kMaxH     = 0.92f;   // 最高 / size

	float barW = ctx.sizePhys * kBarRatio;
	float gap  = ctx.sizePhys * kGapRatio;
	float maxH = ctx.sizePhys * kMaxH;
	float minH = barW;                                // 最矮 = 条宽 → 圆形
	float totalW = N * barW + (N - 1) * gap;
	float x0 = ctx.cxPhys - totalW * 0.5f;
	float radius = barW * 0.5f;

	if (ctx.rt){
		// ---------- D2D 主路径 ----------
		ID2D1SolidColorBrush* brush = NULL;
		ctx.rt->CreateSolidColorBrush(_XLoad_ToColorF(accent, 255), &brush);
		if (!brush) return;
		for (int i = 0; i < N; ++i){
			float phase = fmodf(tNorm + (float)i / N, 1.0f);
			// 0.5 - 0.5 cos(2π·phase) = 标准正弦波 [0..1], 再 smoothstep 锐化两端 → 更"弹"
			float wave  = 0.5f - 0.5f * cosf(2.0f * kLoad_Pi * phase);
			float eased = wave * wave * (3.0f - 2.0f * wave);
			float h = minH + (maxH - minH) * eased;
			float xL = x0 + i * (barW + gap);
			D2D1_ROUNDED_RECT rr = {
				D2D1::RectF(xL, ctx.cyPhys - h * 0.5f,
				            xL + barW, ctx.cyPhys + h * 0.5f),
				radius, radius
			};
			ctx.rt->FillRoundedRectangle(rr, brush);
		}
		SafeRelease(brush);
		return;
	}

	// ---------- GDI+ 兜底 ----------
	float barWlog = barW   / ctx.dpiScale;
	float gapLog  = gap    / ctx.dpiScale;
	float maxHlog = maxH   / ctx.dpiScale;
	float minHlog = barWlog;
	float totWlog = N * barWlog + (N - 1) * gapLog;
	float x0Log   = ctx.cxLog - totWlog * 0.5f;
	int   radLog  = (int)(barWlog * 0.5f + 0.5f);

	XDraw_SetBrushColor(ctx.hDraw, accent);
	for (int i = 0; i < N; ++i){
		float phase = fmodf(tNorm + (float)i / N, 1.0f);
		float wave  = 0.5f - 0.5f * cosf(2.0f * kLoad_Pi * phase);
		float eased = wave * wave * (3.0f - 2.0f * wave);
		float hLog = minHlog + (maxHlog - minHlog) * eased;
		int xL = (int)(x0Log + i * (barWlog + gapLog) + 0.5f);
		int xR = (int)(x0Log + i * (barWlog + gapLog) + barWlog + 0.5f);
		RECT rc{ xL, ctx.cyLog - (int)(hLog * 0.5f),
		         xR, ctx.cyLog + (int)(hLog * 0.5f) };
		XDraw_FillRoundRect(ctx.hDraw, &rc, radLog * 2, radLog * 2);
	}
}

// 派发到具体风格 (统一 tNorm ∈ [0,1))
void _XLoad_PaintAnim(const _XLoad_PaintCtx& ctx, xloading_style_ style,
	float tNorm, COLORREF accent)
{
	switch (style){
	case xloading_style_spinner: _XLoad_PaintSpinner(ctx, tNorm, accent); break;
	case xloading_style_dots:    _XLoad_PaintDots   (ctx, tNorm, accent); break;
	case xloading_style_spokes:  _XLoad_PaintSpokes (ctx, tNorm, accent); break;
	case xloading_style_pulse:   _XLoad_PaintPulse  (ctx, tNorm, accent); break;
	case xloading_style_bars:    _XLoad_PaintBars   (ctx, tNorm, accent); break;
	}
}

// 每种风格的"动画周期" (ms, speed=1.0 时)
int _XLoad_PeriodMs(xloading_style_ s){
	switch (s){
	case xloading_style_spinner: return 1500;   // seamless snake 单周期
	case xloading_style_dots:    return 1800;   // Win8/10 boot 节奏 (单 dot 完整 lifecycle)
	case xloading_style_spokes:  return 1080;   // 12 spoke * 90ms
	case xloading_style_pulse:   return 1400;
	case xloading_style_bars:    return 1100;   // 现代胶囊条波动节奏 (慢一点更优雅)
	}
	return 1000;
}

//============================================================================
// 取宿主客户区 (元素本地逻辑像素, hDraw 工作坐标系)
//============================================================================
void _XLoad_GetClient(HXCGUI hHost, RECT* rc){
	rc->left = rc->top = rc->right = rc->bottom = 0;
	if (XC_IsHELE(hHost))         XEle_GetClientRect((HELE)hHost,    rc);
	else if (XC_IsHWINDOW(hHost)) XWnd_GetClientRect((HWINDOW)hHost, rc);
}

//============================================================================
// XE_PAINT 主回调 (元素 / 窗口共用, callback 签名 binary 兼容)
//============================================================================
int CALLBACK _XLoad_OnPaint(HXCGUI hSelf, HDRAW hDraw, BOOL* pbHandled)
{
	auto* p = _XLoad_GetEntry(hSelf);
	if (!p || !p->running){
		// 让出 paint, 让宿主自己渲染原内容
		if (pbHandled) *pbHandled = FALSE;
		return 0;
	}
	if (pbHandled) *pbHandled = TRUE;

	RECT rc{};
	_XLoad_GetClient(hSelf, &rc);
	int cliW = rc.right - rc.left;
	int cliH = rc.bottom - rc.top;
	if (cliW <= 0 || cliH <= 0) return 0;

	_XLoad_Colors c;
	_XLoad_ResolveColors(*p, &c);

	// 1) 背景 (圆角 — 4 角同 -> FillRoundRect; 不同 -> FillRoundRectEx)
	XDraw_SetBrushColor(hDraw, c.bg);
	if (p->cornerLT == p->cornerRT && p->cornerRT == p->cornerRB && p->cornerRB == p->cornerLB){
		if (p->cornerLT > 0)
			XDraw_FillRoundRect(hDraw, &rc, p->cornerLT, p->cornerLT);
		else
			XDraw_FillRect(hDraw, &rc);
	} else {
		XDraw_FillRoundRectEx(hDraw, &rc,
			p->cornerLT, p->cornerRT, p->cornerRB, p->cornerLB);
	}

	// 2) 文字测量 (如果有)
	SIZE textSz{0, 0};
	if (!p->text.empty()){
		_XLoad_EnsureFont(*p);
		XC_GetTextShowSizeEx(p->text.c_str(), (int)p->text.size(),
			p->hFont, textFormatFlag_NoWrap, &textSz);
	}

	// 3) 整体居中: 动画在上, 文本在下, 中间 gap
	int gap     = (textSz.cy > 0) ? kLoad_TextGap : 0;
	int totalH  = p->sizeCy + gap + textSz.cy;
	int cx      = rc.left + cliW / 2;
	int yTop    = rc.top  + (cliH - totalH) / 2;
	int cyAnim  = yTop + p->sizeCy / 2;

	// 4) 动画相位
	DWORD now     = ::GetTickCount();
	DWORD elapsed = now - p->startTick;

	// 5) 动画 (取 min(cx, cy) 为有效边长 — cx != cy 时不变形)
	int aSize = (p->sizeCx < p->sizeCy) ? p->sizeCx : p->sizeCy;
	_XLoad_PaintCtx pctx;
	_XLoad_BuildCtx(pctx, hDraw, hSelf, cx, cyAnim, aSize);
	int period = (int)(_XLoad_PeriodMs(p->style) / (p->speed > 0 ? p->speed : 1.0f));
	if (period < 50) period = 50;
	float tNorm = (elapsed % period) / (float)period;
	_XLoad_PaintAnim(pctx, p->style, tNorm, c.accent);

	// 6) 文本 (动画下方居中)
	if (!p->text.empty()){
		XDraw_SetTextRenderingHint(hDraw, 3);   // AntiAliasGridFit (灰度 AA, 透明面板防彩边)
		XDraw_SetBrushColor(hDraw, c.text);     // XDraw_DrawText 取色源 = brush
		XDraw_SetFont(hDraw, p->hFont);
		XDraw_SetTextAlign(hDraw,
			textAlignFlag_left | textAlignFlag_top | textFormatFlag_NoWrap);
		int tx = cx - textSz.cx / 2;
		int ty = yTop + p->sizeCy + gap;
		RECT trc{ tx, ty, tx + textSz.cx + 2, ty + textSz.cy + 2 };
		XDraw_DrawText(hDraw, p->text.c_str(), (int)p->text.size(), &trc);
	}
	return 0;
}

//============================================================================
// XE_XC_TIMER — 心跳重绘
//============================================================================
int CALLBACK _XLoad_OnTimer(HXCGUI hSelf, UINT nTimerID, BOOL* pbHandled)
{
	if (nTimerID != kLoad_TimerId){
		if (pbHandled) *pbHandled = FALSE;
		return 0;
	}
	auto* p = _XLoad_GetEntry(hSelf);
	if (!p || !p->running) return 0;
	_XLoad_Redraw(hSelf);
	return 0;
}

//============================================================================
// XE_DESTROY — 清理 entry
//============================================================================
int CALLBACK _XLoad_OnDestroy(HXCGUI hSelf, BOOL* pbHandled)
{
	(void)pbHandled;
	_XLoad_StopTimer(hSelf);
	auto& g = LG();
	auto it = g.registry.find(hSelf);
	if (it != g.registry.end()){
		// AttachWnd 路径下子 HELE 被外部 (e.g. 窗口关闭) 销毁 → 同步擦掉 userToHost 别名.
		if (it->second.hUserAlias){
			g.userToHost.erase(it->second.hUserAlias);
		}
		_XLoad_DestroyFont(it->second);
		g.registry.erase(it);
	}
	return 0;
}

}  // anonymous namespace (CXLoading internals)


//============================================================================
// CXLoading 公开接口
//============================================================================

HELE CXLoading::Create(int x, int y, int cx, int cy, HXCGUI hParent)
{
	HELE hEle = XEle_Create(x, y, cx, cy, hParent);
	if (!hEle) return NULL;
	XUI_EnableCSS(hEle, FALSE);
	XEle_EnableBkTransparent(hEle, FALSE);   // 我们要画自己的背景
	auto* p = _XLoad_GetOrCreate((HXCGUI)hEle);
	if (!p){
		// 不可能 (XEle_Create 返回了合法 HELE), 但兜底
		return hEle;
	}
	p->ownEle    = TRUE;
	p->running   = TRUE;
	p->startTick = ::GetTickCount();
	_XLoad_StartTimer((HXCGUI)hEle);
	return hEle;
}

BOOL CXLoading::AttachEle(HELE hEle)
{
	if (!XC_IsHELE((HXCGUI)hEle)) return FALSE;
	XUI_EnableCSS(hEle, FALSE);
	auto* p = _XLoad_GetOrCreate((HXCGUI)hEle);
	if (!p) return FALSE;
	p->ownEle    = FALSE;
	p->running   = TRUE;
	p->startTick = ::GetTickCount();
	_XLoad_StartTimer((HXCGUI)hEle);
	_XLoad_Redraw((HXCGUI)hEle);
	return TRUE;
}

BOOL CXLoading::AttachWnd(HWINDOW hWnd)
{
	// AttachWnd 语义 = 在窗口客户区上方建一个填满的子 HELE, 走 Create 同款绘制流程.
	// 直接给 hWnd 挂 XE_PAINT 会被 window 内子元素覆盖, 看不到效果. 用子 HELE + 置顶才靠谱.
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;
	auto& g = LG();

	// 同一 hWnd 二次 AttachWnd → 先 Detach 释放旧子元素.
	if (g.userToHost.find((HXCGUI)hWnd) != g.userToHost.end()){
		Detach((HXCGUI)hWnd);
	}
	
	_XLoad_Entry oldConfig;
    BOOL hasOldConfig = FALSE;
    auto itOld = g.registry.find((HXCGUI)hWnd);
    if(itOld != g.registry.end()){
        oldConfig = itOld->second;
        hasOldConfig = TRUE;
        // 移除挂在 hWnd 上的空壳事件
        XWnd_RemoveEventC(hWnd, XE_PAINT,    (void*)&_XLoad_OnPaint);
        XWnd_RemoveEventC(hWnd, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
        XWnd_RemoveEventC(hWnd, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
        g.registry.erase(itOld);
    }
	
	// 客户区减内填充 — 用户也可能 XWnd_SetPadding 留出标题栏/工具栏区不被遮.
	RECT rcCli{};
	XWnd_GetClientRect(hWnd, &rcCli);
	paddingSize_ pad{ 0, 0, 0, 0 };
	XWnd_GetPadding(hWnd, &pad);
	int x  = rcCli.left + pad.leftSize;
	int y  = rcCli.top  + pad.topSize;
	int cx = (rcCli.right  - rcCli.left) - pad.leftSize - pad.rightSize;
	int cy = (rcCli.bottom - rcCli.top)  - pad.topSize  - pad.bottomSize;
	if (cx < 1) cx = 1;
	if (cy < 1) cy = 1;

	HELE hEle = XEle_Create(x, y, cx, cy, (HXCGUI)hWnd);
	if (!hEle) return FALSE;
	XUI_EnableCSS(hEle, FALSE);

	XEle_EnableTopmost(hEle, TRUE);              // 置顶 → 盖住窗口所有同级元素
	XEle_EnableBkTransparent(hEle, FALSE);       // 我们自画背景
	// 窗口即使未启用布局, 这两行也能让子 HELE 在窗口 resize 时自动撑满客户区.
	XWidget_LayoutItem_SetWidth (hEle, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(hEle, layout_size_fill, 0);

	// 登记别名 → 后续 SetXxx(hWnd) / Stop(hWnd) / Detach(hWnd) 全都 Resolve 到这个子 HELE.
	g.userToHost[(HXCGUI)hWnd] = (HXCGUI)hEle;

	auto* p = _XLoad_GetOrCreate((HXCGUI)hEle);
	if (!p){
		g.userToHost.erase((HXCGUI)hWnd);
		XEle_Destroy(hEle);
		return FALSE;
	}
	
	if (hasOldConfig) {
		p->style        = oldConfig.style;
		p->theme        = oldConfig.theme;     // 完美继承你的 light / auto 主题
		p->sizeCx       = oldConfig.sizeCx;
		p->sizeCy       = oldConfig.sizeCy;
		p->text         = oldConfig.text;
		p->customText   = oldConfig.customText;
		p->customBg     = oldConfig.customBg;
		p->customAccent = oldConfig.customAccent;
		p->cornerLT     = oldConfig.cornerLT;
		p->cornerRT     = oldConfig.cornerRT;
		p->cornerRB     = oldConfig.cornerRB;
		p->cornerLB     = oldConfig.cornerLB;
		p->speed        = oldConfig.speed;
		p->fontPt       = oldConfig.fontPt;
		if (oldConfig.hFont) {
			p->hFont = oldConfig.hFont;
			oldConfig.hFont = NULL; // 防止旧字体句柄泄漏
		}
    }
	
	p->ownEle     = TRUE;                        // 我们建的 → Stop 隐藏, Detach 销毁
	p->hUserAlias = (HXCGUI)hWnd;                // 同时兼任"Detach 时销毁子 HELE"标志
	p->running    = TRUE;
	p->startTick  = ::GetTickCount();
	_XLoad_StartTimer((HXCGUI)hEle);
	_XLoad_Redraw((HXCGUI)hEle);
	return TRUE;
}

BOOL CXLoading::Detach(HXCGUI hHost)
{
	if (!hHost) return FALSE;
	auto& g = LG();
	// Resolve: AttachWnd 路径下用户传 hWnd, registry 用子 HELE 当 key.
	HXCGUI hKey = _XLoad_Resolve(hHost);
	auto it = g.registry.find(hKey);
	if (it == g.registry.end()) return FALSE;
	_XLoad_StopTimer(hKey);
	_XLoad_DestroyFont(it->second);
	// 反挂 3 个事件 (XEle_RemoveEventC / XWnd_RemoveEventC 对 C/C1 通用).
	if (XC_IsHELE(hKey)){
		HELE hEle = (HELE)hKey;
		XEle_RemoveEventC(hEle, XE_PAINT,    (void*)&_XLoad_OnPaint);
		XEle_RemoveEventC(hEle, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
		XEle_RemoveEventC(hEle, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
	} else if (XC_IsHWINDOW(hKey)){
		HWINDOW hWnd = (HWINDOW)hKey;
		XWnd_RemoveEventC(hWnd, XE_PAINT,    (void*)&_XLoad_OnPaint);
		XWnd_RemoveEventC(hWnd, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
		XWnd_RemoveEventC(hWnd, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
	}
	// AttachWnd 路径: 销毁我们自己建的子 HELE + 清 alias.
	HXCGUI hAlias    = it->second.hUserAlias;
	BOOL   destroyMe = (hAlias != NULL && XC_IsHELE(hKey));
	g.registry.erase(it);
	if (hAlias) g.userToHost.erase(hAlias);
	if (destroyMe){
		XEle_Destroy((HELE)hKey);
		// 父窗口接管重绘, 子元素已不存在, 不再 redraw 自己.
		if (hAlias && XC_IsHWINDOW(hAlias)) _XLoad_Redraw(hAlias);
	} else {
		_XLoad_Redraw(hKey);
	}
	return TRUE;
}

BOOL CXLoading::HasAttached(HXCGUI hHost)
{
	return _XLoad_GetEntry(hHost) ? TRUE : FALSE;
}

BOOL CXLoading::Start(HXCGUI hHost)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->running   = TRUE;
	p->startTick = ::GetTickCount();
	// 自建元素 (Create / AttachWnd 子 HELE) 若先前 Stop 隐过 → 重新显示.
	if (p->ownEle && XC_IsHELE(p->hHost)) XWidget_Show((HELE)p->hHost, TRUE);
	_XLoad_StartTimer(p->hHost);
	_XLoad_Redraw(p->hHost);
	return TRUE;
}

BOOL CXLoading::Stop(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p) return FALSE;
	p->running = FALSE;
	_XLoad_StopTimer(p->hHost);
	// 自建元素 → 直接隐藏 (Start 时再显示). 这样宿主原内容/窗口背景不被遮.
	// 非自建 (AttachEle) → 留宿主可见, 走 让出 paint 路径 redraw 一次.
	if (p->ownEle && XC_IsHELE(p->hHost)){
		XWidget_Show((HELE)p->hHost, FALSE);
	} else {
		_XLoad_Redraw(p->hHost);   // 让让出 paint 立即生效
	}
	return TRUE;
}

BOOL CXLoading::IsRunning(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return (p && p->running) ? TRUE : FALSE;
}

// ===== 风格 =====
BOOL CXLoading::SetStyle(HXCGUI hHost, xloading_style_ style)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->style     = style;
	p->startTick = ::GetTickCount();   // 切风格重置相位避免突变
	return TRUE;
}
xloading_style_ CXLoading::GetStyle(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->style : xloading_style_spinner;
}

// ===== 尺寸 =====
BOOL CXLoading::SetSize(HXCGUI hHost, int cx, int cy)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->sizeCx = (cx < 8) ? 8 : cx;
	p->sizeCy = (cy < 8) ? 8 : cy;
	return TRUE;
}
void CXLoading::GetSize(HXCGUI hHost, int* pcx, int* pcy)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p){
		if (pcx) *pcx = kLoad_DefaultSize;
		if (pcy) *pcy = kLoad_DefaultSize;
		return;
	}
	if (pcx) *pcx = p->sizeCx;
	if (pcy) *pcy = p->sizeCy;
}

// ===== 文本 =====
BOOL CXLoading::SetText(HXCGUI hHost, const wchar_t* pText)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->text = (pText ? pText : L"");
	_XLoad_Redraw(p->hHost);
	return TRUE;
}
const wchar_t* CXLoading::GetText(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->text.c_str() : NULL;
}

BOOL CXLoading::SetFontSize(HXCGUI hHost, int pt)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	if (pt < 6)  pt = 6;
	if (pt > 72) pt = 72;
	if (p->fontPt == pt) return TRUE;
	p->fontPt = pt;
	_XLoad_DestroyFont(*p);   // 下次 OnPaint 会按新 pt 重建
	_XLoad_Redraw(p->hHost);
	return TRUE;
}

int CXLoading::GetFontSize(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->fontPt : kLoad_TextFontPt;
}

// ===== 主题 / 颜色 =====
BOOL CXLoading::SetTheme(HXCGUI hHost, xuitool_theme_ theme)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->theme = theme;
	_XLoad_Redraw(p->hHost);
	return TRUE;
}
xuitool_theme_ CXLoading::GetTheme(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->theme : xuitool_theme_dark;
}

BOOL CXLoading::SetTextColor(HXCGUI hHost, COLORREF color)
{
    auto* p = _XLoad_GetOrCreate(hHost);
    if (!p) return FALSE;
    // 切换到 custom 前，先继承当前主题的其它颜色，防止变白/变黑
    if (p->theme != xuitool_theme_custom) {
        _XLoad_Colors c;
        _XLoad_ResolveColors(*p, &c);
        p->customBg = c.bg;
        p->customAccent = c.accent;
    }
    p->customText = color;
    p->theme      = xuitool_theme_custom;
    _XLoad_Redraw(p->hHost);
    return TRUE;
}
COLORREF CXLoading::GetTextColor(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p) return _XUITool::kDarkText;
	_XLoad_Colors c;
	_XLoad_ResolveColors(*p, &c);
	return c.text;
}

BOOL CXLoading::SetBkColor(HXCGUI hHost, COLORREF color)
{
    auto* p = _XLoad_GetOrCreate(hHost);
    if (!p) return FALSE;
    if (p->theme != xuitool_theme_custom) {
        _XLoad_Colors c;
        _XLoad_ResolveColors(*p, &c);
        p->customText = c.text;
        p->customAccent = c.accent;
    }
    p->customBg = color;
    p->theme    = xuitool_theme_custom;
    _XLoad_Redraw(p->hHost);
    return TRUE;
}
COLORREF CXLoading::GetBkColor(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p) return _XUITool::kDarkBg;
	_XLoad_Colors c;
	_XLoad_ResolveColors(*p, &c);
	return c.bg;
}

BOOL CXLoading::SetAccentColor(HXCGUI hHost, COLORREF color)
{
    auto* p = _XLoad_GetOrCreate(hHost);
    if (!p) return FALSE;
    if (p->theme != xuitool_theme_custom) {
        _XLoad_Colors c;
        _XLoad_ResolveColors(*p, &c);
        p->customText = c.text;
        p->customBg = c.bg;
    }
    p->customAccent = color;
    p->theme        = xuitool_theme_custom;
    _XLoad_Redraw(p->hHost);
    return TRUE;
}
COLORREF CXLoading::GetAccentColor(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p) return _XUITool::kDarkAccent;
	_XLoad_Colors c;
	_XLoad_ResolveColors(*p, &c);
	return c.accent;
}

// ===== 圆角 =====
BOOL CXLoading::SetCornerRadius(HXCGUI hHost, int radius)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	int r = (radius < 0) ? 0 : radius;
	p->cornerLT = p->cornerRT = p->cornerRB = p->cornerLB = r;
	return TRUE;
}
int CXLoading::GetCornerRadius(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->cornerLT : 0;
}
BOOL CXLoading::SetCornerRadiusEx(HXCGUI hHost, int leftTop, int rightTop, int rightBottom, int leftBottom)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->cornerLT = (leftTop     < 0) ? 0 : leftTop;
	p->cornerRT = (rightTop    < 0) ? 0 : rightTop;
	p->cornerRB = (rightBottom < 0) ? 0 : rightBottom;
	p->cornerLB = (leftBottom  < 0) ? 0 : leftBottom;
	return TRUE;
}
void CXLoading::GetCornerRadiusEx(HXCGUI hHost, int* pLT, int* pRT, int* pRB, int* pLB)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p){
		if (pLT) *pLT = 0;
		if (pRT) *pRT = 0;
		if (pRB) *pRB = 0;
		if (pLB) *pLB = 0;
		return;
	}
	if (pLT) *pLT = p->cornerLT;
	if (pRT) *pRT = p->cornerRT;
	if (pRB) *pRB = p->cornerRB;
	if (pLB) *pLB = p->cornerLB;
}

// ===== 速度 =====
BOOL CXLoading::SetSpeed(HXCGUI hHost, float speed)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	if (speed < 0.1f) speed = 0.1f;
	if (speed > 5.0f) speed = 5.0f;
	p->speed = speed;
	return TRUE;
}
float CXLoading::GetSpeed(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->speed : 1.0f;
}

// ===== 全局清理 =====
void CXLoading::Cleanup()
{
	auto& g = LG();
	// 先收集 AttachWnd 路径下我们自己建的子 HELE — 一会儿要 XEle_Destroy 它们.
	std::vector<HELE> ownedChildren;
	ownedChildren.reserve(g.registry.size());
	// 停所有 timer (避免 timer 回调引用已清的 entry) + 销毁字体 + 反挂宿主事件.
	for (auto& kv : g.registry){
		HXCGUI hHost = kv.first;
		_XLoad_StopTimer(hHost);
		_XLoad_DestroyFont(kv.second);
		if (XC_IsHELE(hHost)){
			HELE h = (HELE)hHost;
			XEle_RemoveEventC(h, XE_PAINT,    (void*)&_XLoad_OnPaint);
			XEle_RemoveEventC(h, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
			XEle_RemoveEventC(h, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
			if (kv.second.hUserAlias) ownedChildren.push_back(h);   // AttachWnd 建的, 待销毁
		} else if (XC_IsHWINDOW(hHost)){
			HWINDOW w = (HWINDOW)hHost;
			XWnd_RemoveEventC(w, XE_PAINT,    (void*)&_XLoad_OnPaint);
			XWnd_RemoveEventC(w, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
			XWnd_RemoveEventC(w, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
		}
	}
	g.registry.clear();
	g.userToHost.clear();
	// XEle_Destroy 必须在 registry 清空之后 (否则会触发 OnDestroy 回调修改正在遍历的容器).
	for (HELE h : ownedChildren){
		if (XC_IsHELE((HXCGUI)h)) XEle_Destroy(h);
	}
}
