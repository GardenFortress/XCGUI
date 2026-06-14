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
} // namespace _XUITool (draw_softshadow)
#endif // _XCGUI_UITOOL_DRAWSHADOW_INC_

