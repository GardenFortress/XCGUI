//============================================================================
// module_xcgui_uitool_steps.cpp — CXSteps split
// 仅由 module_xcgui_uitool.cpp #include; 勿单独编译.
//============================================================================

#ifndef _XCGUI_UITOOL_AGGREGATED_
#else

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "module_xcgui_uitool_steps_const.inc"

struct _XSteps_ThemeColors
{
	COLORREF accent;
	COLORREF labelWaitBg;
	COLORREF labelWaitText;
	COLORREF labelActiveText;
	COLORREF stepText;
	COLORREF stepTextWait;
	COLORREF connectorWait;
	COLORREF connectorDone;
};

struct _XSteps_StepState
{
	int                  id            = 0;
	CXText               text;
	int                  statusOverride = -1;
	HIMAGE               hLabelIcon    = NULL;
	HSVG                 hLabelSvg     = NULL;
	HELE                 hStepItem     = NULL;
	HELE                 hStepLabel    = NULL;
	HXCGUI               hStepText     = NULL;
};

namespace {

inline BOOL _XSteps_IsValidEle(HXCGUI h)
{
	return h && XC_IsHELE(h);
}

inline BOOL _XSteps_IsValidUi(HXCGUI h)
{
	return h && (XC_IsHELE(h) || XC_IsShape(h));
}

inline BOOL _XSteps_IsValidFont(HFONTX h)
{
	return h && XC_IsHXCGUI((HXCGUI)h, XC_FONT);
}

inline BOOL _XSteps_IsValidImage(HIMAGE h)
{
	return h && (XC_IsHXCGUI((HXCGUI)h, XC_IMAGE_TEXTURE)
		|| XC_IsHXCGUI((HXCGUI)h, XC_IMAGE_FRAME));
}

inline BOOL _XSteps_IsValidSvg(HSVG h)
{
	return h && XC_IsHXCGUI((HXCGUI)h, XC_SVG);
}

inline void _XSteps_ReleaseFontHandle(HFONTX& h)
{
	if (_XSteps_IsValidFont(h)) XFont_Release(h);
	h = NULL;
}

inline void _XSteps_ReleaseImageHandle(HIMAGE& h)
{
	if (_XSteps_IsValidImage(h)) XImage_Release(h);
	h = NULL;
}

inline void _XSteps_ReleaseSvgHandle(HSVG& h)
{
	if (_XSteps_IsValidSvg(h)) XSvg_Release(h);
	h = NULL;
}

void _XSteps_PrepareLayoutEle(HELE hEle, BOOL bMouseThrough)
{
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return;
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_EnableDrawFocus(hEle, FALSE);
	XEle_EnableMouseThrough(hEle, bMouseThrough ? TRUE : FALSE);
}

void _XSteps_PrepareLabelEle(HELE hEle)
{
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return;
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_EnableDrawFocus(hEle, FALSE);
	XEle_EnableCanvas(hEle, TRUE);
	XEle_EnableBkTransparent(hEle, TRUE);
}

void _XSteps_DetachFromParent(HXCGUI h)
{
	if (!h) return;
	if (XC_IsHELE(h)){
		HELE hEle = (HELE)h;
		if (XWidget_GetParentEle(hEle))
			XEle_Remove(hEle);
	}else if (XC_IsShape(h)){
		XShape_RemoveShape(h);
	}
}

void _XSteps_DestroyUi(HXCGUI h)
{
	if (!h) return;
	if (XC_IsHELE(h)) XEle_Destroy((HELE)h);
	else if (XC_IsShape(h)) XShape_Destroy(h);
}

inline void _XSteps_DrawImageEx(HDRAW hDraw, HIMAGE hImg, int x, int y, int w, int h)
{
	if (hDraw && hImg) XDraw_ImageEx(hDraw, hImg, x, y, w, h);
}

inline void _XSteps_DrawSvgEx(HDRAW hDraw, HSVG hSvg, int x, int y, int w, int h)
{
	if (hDraw && hSvg) XDraw_DrawSvgEx(hDraw, hSvg, x, y, w, h);
}

inline void _XSteps_HideLayoutFrame(HXCGUI h)
{
	if (!h) return;
	if (XC_IsShape(h)){
		XShape_ShowLayout(h, FALSE);
		return;
	}
	if (XC_IsHELE(h) && XWidget_IsLayoutControl(h))
		XLayout_ShowLayoutFrame((HELE)h, FALSE);
}

inline float _XSteps_Clamp01(float v)
{
	if (v < 0.f) return 0.f;
	if (v > 1.f) return 1.f;
	return v;
}

inline float _XSteps_EaseOutCubic(float t)
{
	t = _XSteps_Clamp01(t);
	float inv = 1.f - t;
	return 1.f - inv * inv * inv;
}

int _XSteps_MeasureTextWidth(const wchar_t* text, HFONTX hFont)
{
	SIZE sz;

	if (!text || !text[0] || !_XSteps_IsValidFont(hFont)) return 0;
	memset(&sz, 0, sizeof(sz));
	XC_GetTextSize(text, (int)wcslen(text), hFont, &sz);
	return sz.cx;
}

void _XSteps_PaintConnectorProgress(HDRAW hDraw, const RECT& rc, BOOL horizontal, float progress,
	COLORREF done, COLORREF wait)
{
	RECT rcDone, rcWait;

	if (!hDraw || rc.bottom <= rc.top || rc.right <= rc.left) return;
	progress = _XSteps_Clamp01(progress);
	if (progress <= 0.f){
		XDraw_SetBrushColor(hDraw, wait);
		XDraw_FillRect(hDraw, (RECT*)&rc);
		return;
	}
	if (progress >= 1.f){
		XDraw_SetBrushColor(hDraw, done);
		XDraw_FillRect(hDraw, (RECT*)&rc);
		return;
	}
	rcDone = rc;
	rcWait = rc;
	if (horizontal){
		int split = rc.left + (int)((rc.right - rc.left) * progress);
		rcDone.right = split;
		rcWait.left = split;
	}else{
		int split = rc.top + (int)((rc.bottom - rc.top) * progress);
		rcDone.bottom = split;
		rcWait.top = split;
	}
	if (rcDone.right > rcDone.left && rcDone.bottom > rcDone.top){
		XDraw_SetBrushColor(hDraw, done);
		XDraw_FillRect(hDraw, &rcDone);
	}
	if (rcWait.right > rcWait.left && rcWait.bottom > rcWait.top){
		XDraw_SetBrushColor(hDraw, wait);
		XDraw_FillRect(hDraw, &rcWait);
	}
}

// 将前景色按权重叠到背景上，返回不透明色（避免自绘圆底半透明时露出连接线）.
inline COLORREF _XSteps_BlendOnBg(COLORREF bg, COLORREF fg, BYTE fgWeight)
{
	BYTE br = (BYTE)(bg & 0xFF);
	BYTE bG = (BYTE)((bg >> 8) & 0xFF);
	BYTE bB = (BYTE)((bg >> 16) & 0xFF);
	BYTE fr = (BYTE)(fg & 0xFF);
	BYTE fG = (BYTE)((fg >> 8) & 0xFF);
	BYTE fB = (BYTE)((fg >> 16) & 0xFF);
	BYTE inv = (BYTE)(255 - fgWeight);
	BYTE r = (BYTE)((fr * fgWeight + br * inv) / 255);
	BYTE g = (BYTE)((fG * fgWeight + bG * inv) / 255);
	BYTE b = (BYTE)((fB * fgWeight + bB * inv) / 255);
	return RGBA(r, g, b, 255);
}

void _XSteps_ResolveTheme(xuitool_theme_ theme, COLORREF customText, COLORREF customBg,
	COLORREF customAccent, _XSteps_ThemeColors* c)
{
	if (!c) return;
	_XUITool::ThemePalette base;
	memset(&base, 0, sizeof(base));
	_XUITool::ResolvePalette(theme, customText, customBg, customAccent, &base);
	BOOL light = _XUITool::IsLightTheme(theme);

	c->accent = light ? RGBA(0x31, 0x75, 0xF6, 255) : RGBA(0x37, 0x7A, 0xF6, 255);
	if (theme == xuitool_theme_custom)
		c->accent = customAccent;
	c->labelWaitBg = light ? RGBA(0xE5, 0xE7, 0xEB, 255) : _XSteps_BlendOnBg(base.bg, base.text, 30);
	c->labelWaitText = light ? RGBA(0x6B, 0x72, 0x80, 255) : _XUITool::WithAlpha(base.text, 128);
	c->labelActiveText = RGBA(255, 255, 255, 255);
	c->stepText = base.text;
	if (theme == xuitool_theme_custom)
		c->stepText = customText;
	c->stepTextWait = light ? RGBA(0x6B, 0x72, 0x80, 255) : _XSteps_BlendOnBg(base.bg, c->stepText, 153);
	c->connectorWait = light ? RGBA(0xE5, 0xE7, 0xEB, 255) : _XUITool::WithAlpha(base.text, 38);
	c->connectorDone = c->accent;
}

RECT _XSteps_CircleRectInClient(const RECT& rcClient)
{
	RECT rcCircle;
	int w = rcClient.right - rcClient.left;
	int h = rcClient.bottom - rcClient.top;
	int d = kSteps_LabelSize;
	int cx = (rcClient.left + rcClient.right) / 2;
	int cy = (rcClient.top + rcClient.bottom) / 2;
	rcCircle.left = cx - d / 2;
	rcCircle.top = cy - d / 2;
	rcCircle.right = rcCircle.left + d;
	rcCircle.bottom = rcCircle.top + d;
	if (rcCircle.left < rcClient.left) rcCircle.left = rcClient.left;
	if (rcCircle.top < rcClient.top) rcCircle.top = rcClient.top;
	if (rcCircle.right > rcClient.right) rcCircle.right = rcClient.right;
	if (rcCircle.bottom > rcClient.bottom) rcCircle.bottom = rcClient.bottom;
	(void)w;
	(void)h;
	return rcCircle;
}

} // namespace

//============================================================================
// CXSteps
//============================================================================

CXSteps::CXSteps()
	: m_theme(xuitool_theme_auto)
	, m_customText(_XUITool::kDarkText)
	, m_customBg(_XUITool::kDarkBg)
	, m_customAccent(RGBA(0x37, 0x7A, 0xF6, 255))
	, m_orientation(xsteps_orient_horizontal)
	, m_contentOrder(xsteps_content_label_first)
	, m_currentStep(0)
	, m_visualStep(0.f)
	, m_bAnimEnabled(TRUE)
	, m_animDurationMs(kSteps_AnimDurationMs)
	, m_stepAnimActive(FALSE)
	, m_animFrom(0.f)
	, m_animTo(0.f)
	, m_animStartMs(0)
	, m_bShowCompletedIcon(FALSE)
	, m_nextStepId(1)
	, m_hFontText(NULL)
	, m_hFontLabel(NULL)
	, m_hCompletedIcon(NULL)
	, m_hCompletedSvg(NULL)
	, m_pColors(new _XSteps_ThemeColors())
	, m_onStepClick(NULL)
	, m_onThemeChanged(NULL)
	, m_bRootDestroyed(FALSE)
	, m_inAdjustLayoutEndImpl(FALSE)
	, m_inLayoutSync(FALSE)
	, m_hStepWrap(NULL)
	, m_vertTextColW(kSteps_VTextColMinW)
{
	_XSteps_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);
}

CXSteps::~CXSteps()
{
	if (m_bRootDestroyed){
		m_hFontText = NULL;
		m_hFontLabel = NULL;
		m_hCompletedIcon = NULL;
		m_hCompletedSvg = NULL;
	}else{
		if (m_hEle || !m_steps.empty()){
			_xsteps_ClearState();
		}
		m_hEle = NULL;
		_xsteps_ReleaseOwnedResources();
	}
	delete m_pColors;
	m_pColors = NULL;
}

void CXSteps::ReleaseFonts()
{
	_xsteps_DetachFontRefsFromUi();
	_XSteps_ReleaseFontHandle(m_hFontText);
	_XSteps_ReleaseFontHandle(m_hFontLabel);
}

void CXSteps::DetachFonts()
{
	_xsteps_DetachFontRefsFromUi();
	m_hFontText = NULL;
	m_hFontLabel = NULL;
}

void CXSteps::_xsteps_DetachFontRefsFromUi()
{
	for (auto& kv : m_steps){
		_XSteps_StepState* s = kv.second;
		if (!s) continue;
		if (s->hStepText && XC_IsShape(s->hStepText))
			XShapeText_SetFont(s->hStepText, NULL);
	}
}

void CXSteps::_xsteps_ReleaseOwnedResources()
{
	ReleaseFonts();
	_XSteps_ReleaseImageHandle(m_hCompletedIcon);
	_XSteps_ReleaseSvgHandle(m_hCompletedSvg);
}

void CXSteps::_xsteps_ClearState()
{
	for (auto& kv : m_steps){
		_XSteps_StepState* s = kv.second;
		if (!s) continue;
		_XSteps_ReleaseImageHandle(s->hLabelIcon);
		_XSteps_ReleaseSvgHandle(s->hLabelSvg);
		if (s->hStepItem && _XSteps_IsValidEle((HXCGUI)s->hStepItem))
			XEle_Destroy(s->hStepItem);
		delete s;
	}
	m_steps.clear();
	m_stepOrder.clear();
}

void CXSteps::_xsteps_OnRootDestroyed()
{
	m_bRootDestroyed = TRUE;
	_xsteps_StopAnimTimer();
	DetachFonts();
	_XSteps_ReleaseImageHandle(m_hCompletedIcon);
	_XSteps_ReleaseSvgHandle(m_hCompletedSvg);
	for (auto& kv : m_steps){
		_XSteps_StepState* s = kv.second;
		if (!s) continue;
		_XSteps_ReleaseImageHandle(s->hLabelIcon);
		_XSteps_ReleaseSvgHandle(s->hLabelSvg);
		s->hStepItem = NULL;
		s->hStepLabel = NULL;
		s->hStepText = NULL;
		delete s;
	}
	m_steps.clear();
	m_stepOrder.clear();
	m_hStepWrap = NULL;
}

void CXSteps::EnsureFonts()
{
	if (!_XSteps_IsValidFont(m_hFontText))
		m_hFontText = XFont_CreateEx(L"Segoe UI", kSteps_TextFontPt, fontStyle_regular);
	if (!_XSteps_IsValidFont(m_hFontLabel))
		m_hFontLabel = XFont_CreateEx(L"Segoe UI", kSteps_LabelFontPt, fontStyle_bold);
}

_XSteps_StepState* CXSteps::FindStep(int stepId)
{
	auto it = m_steps.find(stepId);
	if (it == m_steps.end()) return NULL;
	return it->second;
}

int CXSteps::_xsteps_IndexOfStep(int stepId) const
{
	for (size_t i = 0; i < m_stepOrder.size(); ++i){
		if (m_stepOrder[i] == stepId) return (int)i;
	}
	return -1;
}

xsteps_status_ CXSteps::_xsteps_ResolveStatus(const _XSteps_StepState* step, int index) const
{
	if (!step) return xsteps_status_wait;
	if (step->statusOverride >= 0)
		return (xsteps_status_)step->statusOverride;
	float vs = _xsteps_GetVisualStep();
	int cur = (int)vs;
	if (index < cur) return xsteps_status_finish;
	if (index == cur) return xsteps_status_process;
	return xsteps_status_wait;
}

float CXSteps::_xsteps_GetVisualStep() const
{
	return m_stepAnimActive ? m_visualStep : (float)m_currentStep;
}

void CXSteps::_xsteps_StartAnimTimer()
{
	if (!m_hEle || !_XSteps_IsValidEle((HXCGUI)m_hEle)) return;
	XEle_SetXCTimer(m_hEle, kSteps_AnimTimerId, kSteps_AnimTickMs);
}

void CXSteps::_xsteps_StopAnimTimer()
{
	if (m_hEle && _XSteps_IsValidEle((HXCGUI)m_hEle))
		XEle_KillXCTimer(m_hEle, kSteps_AnimTimerId);
	m_stepAnimActive = FALSE;
}

void CXSteps::_xsteps_SyncVisualProgress()
{
	_xsteps_StopAnimTimer();
	m_visualStep = (float)m_currentStep;
}

void CXSteps::_xsteps_UpdateAllStepVisuals()
{
	for (auto& kv : m_steps)
		UpdateStepVisual(kv.second);
}

void CXSteps::_xsteps_TickStepAnim()
{
	if (!m_stepAnimActive || !m_hEle) return;
	DWORD elapsed = GetTickCount() - m_animStartMs;
	float t = (float)elapsed / (float)(m_animDurationMs > 0 ? m_animDurationMs : kSteps_AnimDurationMs);
	if (t >= 1.f){
		t = 1.f;
		m_visualStep = m_animTo;
		_xsteps_StopAnimTimer();
	}else{
		m_visualStep = m_animFrom + (m_animTo - m_animFrom) * _XSteps_EaseOutCubic(t);
	}
	_xsteps_UpdateAllStepVisuals();
	_xsteps_InvalidConnectors();
}

void CXSteps::_xsteps_RecalcVerticalTextColWidth()
{
	int maxW, w;

	if (m_orientation != xsteps_orient_vertical){
		m_vertTextColW = kSteps_VTextColMinW;
		return;
	}

	EnsureFonts();
	maxW = kSteps_VTextColMinW;
	for (int id : m_stepOrder){
		_XSteps_StepState* s = FindStep(id);
		if (!s) continue;
		w = _XSteps_MeasureTextWidth(s->text.getPtr(), m_hFontText);
		if (w > maxW) maxW = w;
	}
	maxW += kSteps_TextPadH * 2;
	if (maxW < kSteps_VTextColMinW) maxW = kSteps_VTextColMinW;
	if (maxW > kSteps_VTextColMaxW) maxW = kSteps_VTextColMaxW;
	m_vertTextColW = maxW;
}

void CXSteps::_xsteps_ApplyRootLayout()
{
	if (!_XSteps_IsValidEle((HXCGUI)m_hEle)) return;
	XLayoutBox_SetAlignV(m_hEle, layout_align_top);
	XLayoutBox_SetSpace(m_hEle, 0);
	XWidget_LayoutItem_SetWidth(m_hEle, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(m_hEle, layout_size_fill, 0);
	if (m_orientation == xsteps_orient_horizontal)
		XWidget_LayoutItem_SetMinSize(m_hEle, 0, kSteps_MinBarH);
	else
		XWidget_LayoutItem_SetMinSize(m_hEle, kSteps_MinItemW, kSteps_MinBarH);
}

void CXSteps::_xsteps_ApplyWrapLayout()
{
	if (!m_hStepWrap || !_XSteps_IsValidEle((HXCGUI)m_hStepWrap)) return;
	if (m_orientation == xsteps_orient_horizontal){
		XLayoutBox_EnableHorizon(m_hStepWrap, TRUE);
		XLayoutBox_SetAlignV(m_hStepWrap, layout_align_top);
		XLayoutBox_SetAlignH(m_hStepWrap, layout_align_left);
		XLayoutBox_SetSpace(m_hStepWrap, 0);
		XWidget_LayoutItem_SetWidth(m_hStepWrap, layout_size_fill, 0);
		XWidget_LayoutItem_SetHeight(m_hStepWrap, layout_size_fixed, kSteps_MinBarH);
		XWidget_LayoutItem_SetMinSize(m_hStepWrap, 0, kSteps_MinBarH);
	}else{
		XLayoutBox_EnableHorizon(m_hStepWrap, FALSE);
		XLayoutBox_SetAlignV(m_hStepWrap, layout_align_top);
		XLayoutBox_SetSpace(m_hStepWrap, 0);
		XWidget_LayoutItem_SetWidth(m_hStepWrap, layout_size_fill, 0);
		XWidget_LayoutItem_SetHeight(m_hStepWrap, layout_size_fill, 0);
		XWidget_LayoutItem_SetMinSize(m_hStepWrap, kSteps_MinItemW, kSteps_MinBarH);
	}
}

void CXSteps::_xsteps_ApplyStepItemLayout(_XSteps_StepState* step)
{
	if (!step || !step->hStepItem) return;
	const int gap = m_orientation == xsteps_orient_horizontal ? kSteps_ItemGapH : kSteps_ItemGapV;
	if (m_orientation == xsteps_orient_horizontal){
		XLayoutBox_EnableHorizon(step->hStepItem, FALSE);
		XLayoutBox_SetAlignV(step->hStepItem, layout_align_top);
		XLayoutBox_SetSpace(step->hStepItem, gap);
		XWidget_LayoutItem_SetWidth(step->hStepItem, layout_size_weight, 1);
		XWidget_LayoutItem_SetHeight(step->hStepItem, layout_size_fill, 0);
		XWidget_LayoutItem_SetMinSize(step->hStepItem, kSteps_MinItemW, kSteps_MinBarH);
		if (step->hStepLabel){
			XWidget_LayoutItem_SetWidth(step->hStepLabel, layout_size_fill, 0);
			XWidget_LayoutItem_SetHeight(step->hStepLabel, layout_size_fixed, kSteps_TextH);
		}
		if (step->hStepText && XC_IsShape(step->hStepText)){
			XWidget_LayoutItem_SetWidth(step->hStepText, layout_size_fill, 0);
			XWidget_LayoutItem_SetHeight(step->hStepText, layout_size_fixed, kSteps_TextH);
		}
	}else{
		XLayoutBox_EnableHorizon(step->hStepItem, TRUE);
		XLayoutBox_SetAlignV(step->hStepItem, layout_align_center);
		XLayoutBox_SetAlignH(step->hStepItem, layout_align_left);
		XLayoutBox_SetSpace(step->hStepItem, gap);
		XWidget_LayoutItem_SetWidth(step->hStepItem, layout_size_fill, 0);
		XWidget_LayoutItem_SetHeight(step->hStepItem, layout_size_weight, 1);
		if (step->hStepLabel){
			XWidget_LayoutItem_SetWidth(step->hStepLabel, layout_size_fixed, kSteps_LabelSize);
			XWidget_LayoutItem_SetHeight(step->hStepLabel, layout_size_fill, 0);
		}
		if (step->hStepText && XC_IsShape(step->hStepText)){
			// 垂直模式统一固定宽文本列；label_first 下 fill 会塌缩为 0 导致文本不可见.
			XWidget_LayoutItem_SetWidth(step->hStepText, layout_size_fixed, m_vertTextColW);
			XWidget_LayoutItem_SetHeight(step->hStepText, layout_size_fixed, kSteps_TextH);
			XWidget_LayoutItem_SetMinSize(step->hStepText, 0, kSteps_TextH);
		}
	}
}

void CXSteps::_xsteps_ApplyAllStepLayouts()
{
	for (int id : m_stepOrder){
		_XSteps_StepState* s = FindStep(id);
		if (s) _xsteps_ApplyStepItemLayout(s);
	}
}

void CXSteps::_xsteps_ApplyStepTextAlign(_XSteps_StepState* step)
{
	if (!step || !step->hStepText || !XC_IsShape(step->hStepText)) return;
	int align;
	if (m_orientation == xsteps_orient_horizontal){
		align = textAlignFlag_center | textAlignFlag_vcenter | textTrimming_EllipsisCharacter;
	}else{
		if (m_contentOrder == xsteps_content_text_first)
			align = textAlignFlag_right | textAlignFlag_vcenter | textTrimming_EllipsisCharacter;
		else
			align = textAlignFlag_left | textAlignFlag_vcenter | textTrimming_EllipsisCharacter;
	}
	XShapeText_SetTextAlign(step->hStepText, align);
}

void CXSteps::_xsteps_ReorderStepItemChildren(_XSteps_StepState* step)
{
	if (!step || !step->hStepItem) return;
	HXCGUI first = NULL;
	HXCGUI second = NULL;
	if (m_contentOrder == xsteps_content_label_first){
		first = (HXCGUI)step->hStepLabel;
		second = step->hStepText;
	}else{
		first = step->hStepText;
		second = (HXCGUI)step->hStepLabel;
	}
	if (first) _XSteps_DetachFromParent(first);
	if (second) _XSteps_DetachFromParent(second);
	if (first) XEle_AddChild(step->hStepItem, first);
	if (second) XEle_AddChild(step->hStepItem, second);
}

void CXSteps::_xsteps_ApplyAllContentOrder()
{
	for (int id : m_stepOrder){
		_XSteps_StepState* s = FindStep(id);
		if (!s) continue;
		_xsteps_ReorderStepItemChildren(s);
		_xsteps_ApplyStepTextAlign(s);
	}
}

BOOL CXSteps::_xsteps_CreateStepElements(_XSteps_StepState* step)
{
	if (!step || !m_hStepWrap) return FALSE;

	step->hStepItem = XLayout_Create(0, 0, 100, kSteps_TextH * 2, m_hStepWrap);
	if (!step->hStepItem) return FALSE;
	XLayout_EnableLayout(step->hStepItem, TRUE);
	_XSteps_PrepareLayoutEle(step->hStepItem, FALSE);
	XEle_EnableBkTransparent(step->hStepItem, TRUE);
	XEle_SetUserData(step->hStepItem, (vint)step->id);

	step->hStepLabel = XEle_Create(0, 0, kSteps_LabelSize, kSteps_TextH, step->hStepItem);
	if (!step->hStepLabel){
		XEle_Destroy(step->hStepItem);
		step->hStepItem = NULL;
		return FALSE;
	}
	XUI_EnableCSS(step->hStepLabel, FALSE);
	_XSteps_PrepareLabelEle(step->hStepLabel);
	XEle_SetUserData(step->hStepLabel, (vint)step->id);
	XEle_EnableEvent_XE_PAINT_END(step->hStepLabel, TRUE);
	XEle_RegEventCPP1(step->hStepLabel, XE_PAINT_END, &CXSteps::OnStepLabelPaintImpl);
	XEle_RegEventCPP1(step->hStepLabel, XE_LBUTTONUP, &CXSteps::OnStepClickImpl);
	XEle_RegEventCPP1(step->hStepItem, XE_LBUTTONUP, &CXSteps::OnStepClickImpl);

	step->hStepText = XShapeText_Create(0, 0, 80, kSteps_TextH, step->text.getPtr(), step->hStepItem);
	if (step->hStepText){
		XUI_EnableCSS(step->hStepText, FALSE);
		XShapeText_SetFont(step->hStepText, m_hFontText);
		XShapeText_SetTextColor(step->hStepText, m_pColors->stepText);
		_XSteps_HideLayoutFrame(step->hStepText);
		XWidget_Show(step->hStepText, TRUE);
	}
	_XSteps_HideLayoutFrame((HXCGUI)step->hStepItem);
	XWidget_Show(step->hStepLabel, TRUE);
	XWidget_Show(step->hStepItem, TRUE);

	_xsteps_ReorderStepItemChildren(step);
	_xsteps_ApplyStepItemLayout(step);
	_xsteps_ApplyStepTextAlign(step);
	if (step->hStepText && XC_IsShape(step->hStepText))
		XShape_AdjustLayout(step->hStepText);
	return TRUE;
}

void CXSteps::UpdateStepVisual(_XSteps_StepState* step)
{
	if (!step) return;
	int index = _xsteps_IndexOfStep(step->id);
	xsteps_status_ st = _xsteps_ResolveStatus(step, index);
	if (step->hStepText && XC_IsShape(step->hStepText)){
		COLORREF tc = (st == xsteps_status_wait) ? m_pColors->stepTextWait : m_pColors->stepText;
		XShapeText_SetTextColor(step->hStepText, tc);
		XShape_AdjustLayout(step->hStepText);
	}
	if (step->hStepLabel && _XSteps_IsValidEle((HXCGUI)step->hStepLabel))
		XEle_Redraw(step->hStepLabel, FALSE);
}

void CXSteps::RefreshTheme()
{
	_XSteps_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);
	for (auto& kv : m_steps)
		UpdateStepVisual(kv.second);
	if (m_onThemeChanged) m_onThemeChanged(this);
	_xsteps_InvalidConnectors();
}

void CXSteps::_xsteps_InvalidConnectors()
{
	for (auto& kv : m_steps){
		_XSteps_StepState* s = kv.second;
		if (s && s->hStepLabel && _XSteps_IsValidEle((HXCGUI)s->hStepLabel))
			XEle_Redraw(s->hStepLabel, FALSE);
	}
	if (m_hEle) XEle_Redraw(m_hEle, FALSE);
	if (m_hStepWrap && _XSteps_IsValidEle((HXCGUI)m_hStepWrap))
		XEle_Redraw(m_hStepWrap, FALSE);
}

void CXSteps::_xsteps_PaintLabelConnectors(HDRAW hDraw, const RECT& rcClient, int index) const
{
	RECT rcLine;
	int cx, cy, halfW, stepCount;
	float vs, seg;

	if (!hDraw || rcClient.bottom <= rcClient.top || rcClient.right <= rcClient.left) return;
	stepCount = (int)m_stepOrder.size();
	if (stepCount < 2) return;

	vs = _xsteps_GetVisualStep();
	cx = (rcClient.left + rcClient.right) / 2;
	cy = (rcClient.top + rcClient.bottom) / 2;
	halfW = kSteps_ConnectorW / 2;

	if (m_orientation == xsteps_orient_horizontal){
		if (index > 0){
			seg = _XSteps_Clamp01(vs - (float)(index - 1));
			rcLine.left = rcClient.left;
			rcLine.right = cx;
			rcLine.top = cy - halfW;
			rcLine.bottom = cy + halfW + (kSteps_ConnectorW % 2);
			_XSteps_PaintConnectorProgress(hDraw, rcLine, TRUE, seg,
				m_pColors->connectorDone, m_pColors->connectorWait);
		}
		if (index + 1 < stepCount){
			seg = _XSteps_Clamp01(vs - (float)index);
			rcLine.left = cx;
			rcLine.right = rcClient.right;
			rcLine.top = cy - halfW;
			rcLine.bottom = cy + halfW + (kSteps_ConnectorW % 2);
			_XSteps_PaintConnectorProgress(hDraw, rcLine, TRUE, seg,
				m_pColors->connectorDone, m_pColors->connectorWait);
		}
	}else{
		if (index > 0){
			seg = _XSteps_Clamp01(vs - (float)(index - 1));
			rcLine.top = rcClient.top;
			rcLine.bottom = cy;
			rcLine.left = cx - halfW;
			rcLine.right = cx + halfW + (kSteps_ConnectorW % 2);
			_XSteps_PaintConnectorProgress(hDraw, rcLine, FALSE, seg,
				m_pColors->connectorDone, m_pColors->connectorWait);
		}
		if (index + 1 < stepCount){
			seg = _XSteps_Clamp01(vs - (float)index);
			rcLine.top = cy;
			rcLine.bottom = rcClient.bottom;
			rcLine.left = cx - halfW;
			rcLine.right = cx + halfW + (kSteps_ConnectorW % 2);
			_XSteps_PaintConnectorProgress(hDraw, rcLine, FALSE, seg,
				m_pColors->connectorDone, m_pColors->connectorWait);
		}
	}
}

void CXSteps::_xsteps_PaintLabelContent(HDRAW hDraw, _XSteps_StepState* step, int index,
	const RECT& rcCircle, xsteps_status_ status)
{
	int iconSize, ix, iy;
	const wchar_t* numText;
	wchar_t buf[16];

	if (!hDraw || !step) return;

	if (_XSteps_IsValidImage(step->hLabelIcon)){
		iconSize = kSteps_LabelSize - 8;
		ix = rcCircle.left + (kSteps_LabelSize - iconSize) / 2;
		iy = rcCircle.top + (kSteps_LabelSize - iconSize) / 2;
		_XSteps_DrawImageEx(hDraw, step->hLabelIcon, ix, iy, iconSize, iconSize);
		return;
	}
	if (_XSteps_IsValidSvg(step->hLabelSvg)){
		iconSize = kSteps_LabelSize - 8;
		ix = rcCircle.left + (kSteps_LabelSize - iconSize) / 2;
		iy = rcCircle.top + (kSteps_LabelSize - iconSize) / 2;
		_XSteps_DrawSvgEx(hDraw, step->hLabelSvg, ix, iy, iconSize, iconSize);
		return;
	}
	if (m_bShowCompletedIcon && status == xsteps_status_finish){
		if (_XSteps_IsValidImage(m_hCompletedIcon)){
			iconSize = kSteps_LabelSize - 8;
			ix = rcCircle.left + (kSteps_LabelSize - iconSize) / 2;
			iy = rcCircle.top + (kSteps_LabelSize - iconSize) / 2;
			_XSteps_DrawImageEx(hDraw, m_hCompletedIcon, ix, iy, iconSize, iconSize);
			return;
		}
		if (_XSteps_IsValidSvg(m_hCompletedSvg)){
			iconSize = kSteps_LabelSize - 8;
			ix = rcCircle.left + (kSteps_LabelSize - iconSize) / 2;
			iy = rcCircle.top + (kSteps_LabelSize - iconSize) / 2;
			_XSteps_DrawSvgEx(hDraw, m_hCompletedSvg, ix, iy, iconSize, iconSize);
			return;
		}
		numText = L"\u221A"; // √
		XDraw_SetFont(hDraw, m_hFontLabel);
		XDraw_SetBrushColor(hDraw, m_pColors->labelActiveText);
		XDraw_SetTextAlign(hDraw, textAlignFlag_center | textAlignFlag_vcenter);
		XDraw_DrawText(hDraw, numText, 1, (RECT*)&rcCircle);
		return;
	}

	swprintf_s(buf, 16, L"%d", index + 1);
	numText = buf;
	XDraw_SetFont(hDraw, m_hFontLabel);
	if (status == xsteps_status_wait)
		XDraw_SetBrushColor(hDraw, m_pColors->labelWaitText);
	else
		XDraw_SetBrushColor(hDraw, m_pColors->labelActiveText);
	XDraw_SetTextAlign(hDraw, textAlignFlag_center | textAlignFlag_vcenter);
	XDraw_DrawText(hDraw, numText, (int)wcslen(numText), (RECT*)&rcCircle);
}

HELE CXSteps::Create(HXCGUI hParent)
{
	m_bRootDestroyed = FALSE;
	if (XC_IsHWINDOW(hParent))
		XWnd_EnableLayout((HWINDOW)hParent, TRUE);
	else if (XC_IsHELE(hParent))
		XLayout_EnableLayout((HELE)hParent, TRUE);

	m_hEle = XLayoutFrame_CreateEx(hParent);
	if (!_XSteps_IsValidEle((HXCGUI)m_hEle)){
		m_hEle = NULL;
		return NULL;
	}
	XUI_EnableCSS(m_hEle, FALSE);

	EnsureFonts();
	_XSteps_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);

	EnableDrawBorder(FALSE);
	EnableDrawFocus(FALSE);
	XEle_EnableBkTransparent(m_hEle, TRUE);
	EnableCanvas(TRUE);

	XLayoutFrame_EnableLayout(m_hEle, TRUE);
	XLayoutBox_EnableHorizon(m_hEle, FALSE);
	XLayoutFrame_ShowLayoutFrame(m_hEle, FALSE);
	_xsteps_ApplyRootLayout();

	m_hStepWrap = XLayout_Create(0, 0, 100, kSteps_MinBarH, m_hEle);
	if (!m_hStepWrap){
		XEle_Destroy(m_hEle);
		m_hEle = NULL;
		return NULL;
	}
	XLayout_EnableLayout(m_hStepWrap, TRUE);
	_XSteps_PrepareLayoutEle(m_hStepWrap, TRUE);
	XEle_EnableBkTransparent(m_hStepWrap, TRUE);
	_XSteps_HideLayoutFrame((HXCGUI)m_hStepWrap);
	_xsteps_ApplyWrapLayout();

	InstallEvents();
	return m_hEle;
}

void CXSteps::InstallEvents()
{
	if (!_XSteps_IsValidEle((HXCGUI)m_hEle)) return;
	XEle_RegEventCPP1(m_hEle, XE_DESTROY, &CXSteps::OnDestroyImpl);
	XEle_RegEventCPP1(m_hEle, XE_DESTROY_END, &CXSteps::OnDestroyEndImpl);
	XEle_RegEventCPP1(m_hEle, XE_SHOW, &CXSteps::OnShowImpl);
	XEle_RegEventCPP1(m_hEle, XE_SIZE, &CXSteps::OnSizeImpl);
	XEle_RegEventCPP1(m_hEle, XE_ADJUSTLAYOUT_END, &CXSteps::OnAdjustLayoutEndImpl);
	XEle_RegEventCPP1(m_hEle, XE_XC_TIMER, &CXSteps::OnAnimTimerImpl);
}

BOOL CXSteps::IsValid() const
{
	return _XSteps_IsValidEle((HXCGUI)m_hEle);
}

HELE CXSteps::GetHandle() const
{
	return m_hEle;
}

void CXSteps::SetTheme(xuitool_theme_ theme)
{
	m_theme = theme;
	RefreshTheme();
}

xuitool_theme_ CXSteps::GetTheme() const
{
	return m_theme;
}

void CXSteps::SetTextColor(COLORREF c)
{
	m_customText = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXSteps::SetBkColor(COLORREF c)
{
	m_customBg = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXSteps::SetAccentColor(COLORREF c)
{
	m_customAccent = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXSteps::SetOrientation(xsteps_orientation_ orient)
{
	if (m_orientation == orient) return;
	m_orientation = orient;
	_xsteps_SyncVisualProgress();
	_xsteps_ApplyRootLayout();
	_xsteps_ApplyWrapLayout();
	_xsteps_ApplyAllStepLayouts();
	_xsteps_ApplyAllContentOrder();
	AdjustLayout();
}

xsteps_orientation_ CXSteps::GetOrientation() const
{
	return m_orientation;
}

void CXSteps::SetContentOrder(xsteps_content_order_ order)
{
	if (m_contentOrder == order) return;
	m_contentOrder = order;
	_xsteps_RecalcVerticalTextColWidth();
	_xsteps_ApplyAllStepLayouts();
	_xsteps_ApplyAllContentOrder();
	AdjustLayout();
}

xsteps_content_order_ CXSteps::GetContentOrder() const
{
	return m_contentOrder;
}

void CXSteps::SetAnimEnabled(BOOL bEnable)
{
	m_bAnimEnabled = bEnable ? TRUE : FALSE;
	if (!m_bAnimEnabled && m_stepAnimActive){
		m_visualStep = (float)m_currentStep;
		_xsteps_StopAnimTimer();
		_xsteps_UpdateAllStepVisuals();
		_xsteps_InvalidConnectors();
	}
}

BOOL CXSteps::IsAnimEnabled() const
{
	return m_bAnimEnabled;
}

void CXSteps::SetAnimDuration(int ms)
{
	m_animDurationMs = ms > 0 ? ms : kSteps_AnimDurationMs;
}

int CXSteps::GetAnimDuration() const
{
	return m_animDurationMs;
}

void CXSteps::AdjustLayout()
{
	if (m_inLayoutSync || !m_hEle) return;
	m_inLayoutSync = TRUE;
	_xsteps_RecalcVerticalTextColWidth();
	_xsteps_ApplyAllStepLayouts();
	for (int id : m_stepOrder){
		_XSteps_StepState* s = FindStep(id);
		if (s && s->hStepItem && _XSteps_IsValidEle((HXCGUI)s->hStepItem))
			XEle_AdjustLayoutEx(s->hStepItem, adjustLayout_all);
	}
	if (m_hStepWrap && _XSteps_IsValidEle((HXCGUI)m_hStepWrap))
		XEle_AdjustLayoutEx(m_hStepWrap, adjustLayout_all);
	XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
	_xsteps_InvalidConnectors();
	HXCGUI hParent = m_hEle ? XWidget_GetParent((HXCGUI)m_hEle) : NULL;
	if (XC_IsHWINDOW(hParent))
		XWnd_AdjustLayoutEx((HWINDOW)hParent, adjustLayout_all);
	else if (XC_IsHELE(hParent))
		XEle_AdjustLayoutEx((HELE)hParent, adjustLayout_all);
	m_inLayoutSync = FALSE;
}

int CXSteps::AddStep(const wchar_t* pText)
{
	if (!m_hEle || !m_hStepWrap) return 0;
	EnsureFonts();

	_XSteps_StepState* step = new _XSteps_StepState();
	step->id = m_nextStepId++;
	if (pText && pText[0]) step->text = pText;

	if (!_xsteps_CreateStepElements(step)){
		delete step;
		return 0;
	}

	m_steps[step->id] = step;
	m_stepOrder.push_back(step->id);
	UpdateStepVisual(step);
	AdjustLayout();
	return step->id;
}

int CXSteps::InsertStep(int index, const wchar_t* pText)
{
	if (!m_hEle || !m_hStepWrap) return 0;
	if (index < 0) index = 0;
	if (index > (int)m_stepOrder.size()) index = (int)m_stepOrder.size();

	EnsureFonts();
	_XSteps_StepState* step = new _XSteps_StepState();
	step->id = m_nextStepId++;
	if (pText && pText[0]) step->text = pText;

	if (!_xsteps_CreateStepElements(step)){
		delete step;
		return 0;
	}

	m_steps[step->id] = step;
	m_stepOrder.insert(m_stepOrder.begin() + index, step->id);
	if (index <= m_currentStep)
		++m_currentStep;
	_xsteps_SyncVisualProgress();
	UpdateStepVisual(step);
	AdjustLayout();
	return step->id;
}

BOOL CXSteps::SetStepText(int stepId, const wchar_t* pText)
{
	_XSteps_StepState* step = FindStep(stepId);
	if (!step) return FALSE;
	step->text = pText ? pText : L"";
	if (step->hStepText && XC_IsShape(step->hStepText))
		XShapeText_SetText(step->hStepText, step->text.getPtr());
	AdjustLayout();
	return TRUE;
}

BOOL CXSteps::RemoveStep(int stepId)
{
	_XSteps_StepState* step = FindStep(stepId);
	if (!step) return FALSE;

	int removedIndex = -1;
	auto it = std::find(m_stepOrder.begin(), m_stepOrder.end(), stepId);
	if (it != m_stepOrder.end()){
		removedIndex = (int)(it - m_stepOrder.begin());
		m_stepOrder.erase(it);
	}

	_XSteps_ReleaseImageHandle(step->hLabelIcon);
	_XSteps_ReleaseSvgHandle(step->hLabelSvg);
	if (step->hStepItem && _XSteps_IsValidEle((HXCGUI)step->hStepItem))
		XEle_Destroy(step->hStepItem);

	m_steps.erase(stepId);
	delete step;

	if (removedIndex >= 0){
		if (removedIndex < m_currentStep)
			--m_currentStep;
	}
	if (m_currentStep > (int)m_stepOrder.size() - 1)
		m_currentStep = (int)m_stepOrder.size() > 0 ? (int)m_stepOrder.size() - 1 : 0;
	if (m_currentStep < 0) m_currentStep = 0;

	_xsteps_SyncVisualProgress();
	_xsteps_UpdateAllStepVisuals();
	AdjustLayout();
	return TRUE;
}

void CXSteps::ClearSteps()
{
	_xsteps_ClearState();
	m_currentStep = 0;
	_xsteps_SyncVisualProgress();
	if (m_hEle) AdjustLayout();
}

int CXSteps::GetStepCount() const
{
	return (int)m_stepOrder.size();
}

BOOL CXSteps::SetCurrentStep(int index)
{
	int oldStep;

	if (m_stepOrder.empty()){
		m_currentStep = 0;
		m_visualStep = 0.f;
		return TRUE;
	}
	if (index < 0) index = 0;
	if (index >= (int)m_stepOrder.size()) index = (int)m_stepOrder.size() - 1;
	if (m_currentStep == index && !m_stepAnimActive) return TRUE;

	oldStep = m_currentStep;
	m_currentStep = index;

	if (!m_bAnimEnabled || m_animDurationMs <= 0 || !_XSteps_IsValidEle((HXCGUI)m_hEle)){
		m_visualStep = (float)index;
		_xsteps_StopAnimTimer();
		_xsteps_UpdateAllStepVisuals();
		_xsteps_InvalidConnectors();
		return TRUE;
	}

	m_animFrom = m_stepAnimActive ? m_visualStep : (float)oldStep;
	m_animTo = (float)index;
	m_visualStep = m_animFrom;
	m_stepAnimActive = TRUE;
	m_animStartMs = GetTickCount();
	_xsteps_StartAnimTimer();
	_xsteps_TickStepAnim();
	return TRUE;
}

int CXSteps::GetCurrentStep() const
{
	return m_currentStep;
}

BOOL CXSteps::SetStepStatus(int stepId, int status)
{
	_XSteps_StepState* step = FindStep(stepId);
	if (!step) return FALSE;
	if (status < 0){
		step->statusOverride = -1;
	}else if (status <= xsteps_status_finish){
		step->statusOverride = status;
	}else{
		return FALSE;
	}
	UpdateStepVisual(step);
	_xsteps_InvalidConnectors();
	return TRUE;
}

xsteps_status_ CXSteps::GetStepStatus(int stepId) const
{
	_XSteps_StepState* step = const_cast<CXSteps*>(this)->FindStep(stepId);
	if (!step) return xsteps_status_wait;
	return _xsteps_ResolveStatus(step, _xsteps_IndexOfStep(stepId));
}

BOOL CXSteps::SetStepLabelIcon(int stepId, HIMAGE hImg)
{
	_XSteps_StepState* step = FindStep(stepId);
	if (!step) return FALSE;
	_XSteps_ReleaseSvgHandle(step->hLabelSvg);
	_XSteps_ReleaseImageHandle(step->hLabelIcon);
	step->hLabelIcon = hImg;
	if (step->hStepLabel) XEle_Redraw(step->hStepLabel, FALSE);
	return TRUE;
}

BOOL CXSteps::SetStepLabelSvg(int stepId, HSVG hSvg)
{
	_XSteps_StepState* step = FindStep(stepId);
	if (!step) return FALSE;
	_XSteps_ReleaseImageHandle(step->hLabelIcon);
	_XSteps_ReleaseSvgHandle(step->hLabelSvg);
	step->hLabelSvg = hSvg;
	if (step->hStepLabel) XEle_Redraw(step->hStepLabel, FALSE);
	return TRUE;
}

BOOL CXSteps::ClearStepLabelIcon(int stepId)
{
	_XSteps_StepState* step = FindStep(stepId);
	if (!step) return FALSE;
	_XSteps_ReleaseImageHandle(step->hLabelIcon);
	_XSteps_ReleaseSvgHandle(step->hLabelSvg);
	if (step->hStepLabel) XEle_Redraw(step->hStepLabel, FALSE);
	return TRUE;
}

void CXSteps::SetCompletedIcon(HIMAGE hImg)
{
	_XSteps_ReleaseSvgHandle(m_hCompletedSvg);
	_XSteps_ReleaseImageHandle(m_hCompletedIcon);
	m_hCompletedIcon = hImg;
	RefreshTheme();
}

void CXSteps::SetCompletedIconSvg(HSVG hSvg)
{
	_XSteps_ReleaseImageHandle(m_hCompletedIcon);
	_XSteps_ReleaseSvgHandle(m_hCompletedSvg);
	m_hCompletedSvg = hSvg;
	RefreshTheme();
}

void CXSteps::SetShowCompletedIcon(BOOL bShow)
{
	m_bShowCompletedIcon = bShow ? TRUE : FALSE;
	RefreshTheme();
}

BOOL CXSteps::IsShowCompletedIcon() const
{
	return m_bShowCompletedIcon;
}

HELE CXSteps::GetStepLabelEle(int stepId) const
{
	_XSteps_StepState* step = const_cast<CXSteps*>(this)->FindStep(stepId);
	return step ? step->hStepLabel : NULL;
}

HXCGUI CXSteps::GetStepTextEle(int stepId) const
{
	_XSteps_StepState* step = const_cast<CXSteps*>(this)->FindStep(stepId);
	return step ? step->hStepText : NULL;
}

HELE CXSteps::GetStepItemWrap(int stepId) const
{
	_XSteps_StepState* step = const_cast<CXSteps*>(this)->FindStep(stepId);
	return step ? step->hStepItem : NULL;
}

void CXSteps::SetOnStepClick(xsteps_step_event fn)
{
	m_onStepClick = fn;
}

void CXSteps::SetOnThemeChanged(xsteps_void_event fn)
{
	m_onThemeChanged = fn;
}

void CXSteps::DestroySteps()
{
	if (m_bRootDestroyed || !m_hEle) return;
	if (_XSteps_IsValidEle((HXCGUI)m_hEle))
		XEle_Destroy(m_hEle);
}

int CXSteps::OnStepLabelPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	RECT rcClient, rcCircle;
	int stepId, index;
	_XSteps_StepState* step;
	xsteps_status_ status;
	COLORREF fill;

	if (pbHandled) *pbHandled = TRUE;

	stepId = (int)XEle_GetUserData(hEle);
	step = FindStep(stepId);
	if (!step || !hDraw) return 0;

	index = _xsteps_IndexOfStep(stepId);
	status = _xsteps_ResolveStatus(step, index);

	memset(&rcClient, 0, sizeof(rcClient));
	XEle_GetClientRect(hEle, &rcClient);
	if (rcClient.bottom <= rcClient.top || rcClient.right <= rcClient.left) return 0;

	_xsteps_PaintLabelConnectors(hDraw, rcClient, index);

	rcCircle = _XSteps_CircleRectInClient(rcClient);

	if (status == xsteps_status_wait)
		fill = m_pColors->labelWaitBg;
	else
		fill = m_pColors->accent;

	XDraw_SetBrushColor(hDraw, fill);
	XDraw_FillEllipse(hDraw, &rcCircle);

	_xsteps_PaintLabelContent(hDraw, step, index, rcCircle, status);
	return 0;
}

int CXSteps::OnStepClickImpl(HELE hEle, BOOL* pbHandled)
{
	int stepId = (int)XEle_GetUserData(hEle);
	_XSteps_StepState* step = FindStep(stepId);
	if (!step) return 0;
	if (hEle != step->hStepLabel && hEle != step->hStepItem) return 0;
	if (m_onStepClick){
		BOOL handled = FALSE;
		m_onStepClick(this, stepId, &handled);
		if (pbHandled) *pbHandled = handled;
	}
	return 0;
}

int CXSteps::OnDestroyImpl(HELE hEle, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle) return 0;
	return 0;
}

int CXSteps::OnDestroyEndImpl(HELE hEle, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle) return 0;
	_xsteps_OnRootDestroyed();
	m_hEle = NULL;
	return 0;
}

int CXSteps::OnShowImpl(HELE hEle, BOOL bShow, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle || !bShow || m_bRootDestroyed) return 0;
	AdjustLayout();
	return 0;
}

int CXSteps::OnSizeImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled)
{
	(void)nFlags;
	(void)nAdjustNo;
	(void)pbHandled;
	if (hEle != m_hEle || m_bRootDestroyed) return 0;
	_xsteps_InvalidConnectors();
	return 0;
}

int CXSteps::OnAdjustLayoutEndImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled)
{
	(void)nFlags;
	(void)nAdjustNo;
	(void)pbHandled;
	if (hEle != m_hEle || m_bRootDestroyed || m_inAdjustLayoutEndImpl) return 0;
	m_inAdjustLayoutEndImpl = TRUE;
	_xsteps_InvalidConnectors();
	m_inAdjustLayoutEndImpl = FALSE;
	return 0;
}

int CXSteps::OnAnimTimerImpl(HELE hEle, UINT nID, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle || m_bRootDestroyed || nID != kSteps_AnimTimerId) return 0;
	if (!m_stepAnimActive){
		_xsteps_StopAnimTimer();
		return 0;
	}
	_xsteps_TickStepAnim();
	return 0;
}

#endif // _XCGUI_UITOOL_AGGREGATED_
