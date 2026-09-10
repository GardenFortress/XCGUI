//============================================================================
// module_xcgui_draw_softshadow.inc — 多遍圆角描边软阴影 (calendar 风格)
// 由 module_xcgui_uitool.cpp 聚合 TU #include; 依赖 common.inc 中的 _XUITool 主题.
//============================================================================
#ifndef _XCGUI_UITOOL_DRAWSHADOW_INC_
#define _XCGUI_UITOOL_DRAWSHADOW_INC_
namespace _XUITool {
// 窗口 drop-shadow (CXTooltip / CXCalendarCard 共用)
constexpr int   kShadowMargin             = 20;
constexpr int   kShadowSteps              = 21;
constexpr float kShadowMaxBlurFactor      = 40.0f / kShadowSteps;
constexpr float kShadowOpacityFactorDark  = 0.035f;   // 深色 / 自定义 / auto→dark
constexpr float kShadowOpacityFactorLight = 0.013f; // 浅色 / auto→light, 更轻更淡
constexpr int   kShadowOffsetY            = 1;

inline float ShadowOpacityFactor(xuitool_theme_ theme)
{
	return IsLightTheme(theme) ? kShadowOpacityFactorLight : kShadowOpacityFactorDark;
}

inline void DrawDropShadow(HDRAW hDraw, const RECTF& bodyRect, float cornerRadius,
	xuitool_theme_ theme)
{
	if (!hDraw) return;
	const float opacityFactor = ShadowOpacityFactor(theme);
	RECTF rsF = bodyRect;
	rsF.top    += (float)kShadowOffsetY;
	rsF.bottom += (float)kShadowOffsetY;
	for (int i = kShadowSteps; i >= 1; --i){
		float t = (float)(kShadowSteps + 1 - i) / (float)kShadowSteps;
		float opacity = opacityFactor * t * t;
		int alphaI = (int)(opacity * 255.0 + 0.5);
		if (alphaI <= 0) continue;
		if (alphaI > 255) alphaI = 255;
		XDraw_SetLineWidthF(hDraw, i * kShadowMaxBlurFactor);
		XDraw_SetBrushColor(hDraw, ((COLORREF)alphaI << 24));
		XDraw_DrawRoundRectF(hDraw, &rsF, cornerRadius, cornerRadius);
	}
	XDraw_SetLineWidth(hDraw, 1);
}

// 视觉主体圆角描边 (CXCalendarCard / CXColorPicker 弹窗与 CXShadow 主窗共用观感).
// 软阴影只做窗体外的柔和过渡, 主体填充色与桌面亮度接近时仍需要这条描边划出边界.
//
// borderWidth 单位是 *逻辑像素*: XDraw_SetLineWidthF 传的是设备像素面值, 不随
// XCGUI 的 DPI transform 缩放, 所以这里手动 × dpiScale 再取整到整像素, 保证
// 1.0 逻辑 = 1px@100% / 2px@150%, 与 Win11 系统描边和 CXShadow 一致.
constexpr float kBodyBorderWidth = 1.0f;

inline void DrawBodyBorder(HDRAW hDraw, const RECT& bodyRect, int cornerRadius,
	COLORREF borderColor, float dpiScale = 1.0f, float borderWidth = kBodyBorderWidth)
{
	if (!hDraw || borderWidth < 0.5f) return;
	if (((borderColor >> 24) & 0xFF) == 0) return;
	float widthPhys = borderWidth * ((dpiScale > 0.0f) ? dpiScale : 1.0f);
	widthPhys = (float)(int)(widthPhys + 0.5f);
	if (widthPhys < 1.0f) widthPhys = 1.0f;
	RECT rc = bodyRect;
	XDraw_SetBrushColor(hDraw, borderColor);
	XDraw_SetLineWidthF(hDraw, widthPhys);
	XDraw_DrawRoundRect(hDraw, &rc, cornerRadius, cornerRadius);
	XDraw_SetLineWidth(hDraw, 1);
}

// 弹窗当前 DPI 缩放; 取不到窗口句柄时按 96 DPI.
inline float WindowDpiScale(HWINDOW hWnd)
{
	HWND raw = hWnd ? ::XWnd_GetHWND(hWnd) : NULL;
	if (!raw) return 1.0f;
	return (float)XUITool_GetWindowDpi(raw) / 96.0f;
}
} // namespace _XUITool (draw_softshadow)
#endif // _XCGUI_UITOOL_DRAWSHADOW_INC_

