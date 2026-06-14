// CXColorPicker 完整接口使用示例.
// 覆盖 Popup / SetBindEle / SetPopupPosition / SetEnableAutoClose /
//       SetOnColorChanged / initialMode / Rgb / FormatHex / ParseHex / ToCOLORREF.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建后调用 BuildColorPickerDemo(hWnd).

#include "module_xcgui_uitool.h"

static HWINDOW g_hWnd           = NULL;
static HELE    g_hBtnBind       = NULL;
static HELE    g_hBtnNoAlpha    = NULL;
static HELE    g_hBtnDark       = NULL;
static HELE    g_hBtnNoAutoClose = NULL;
static HELE    g_hBtnRgbMode    = NULL;
static HELE    g_hBtnHslMode    = NULL;
static HELE    g_hBtnLive       = NULL;
static HELE    g_hPreview       = NULL;
static HXCGUI  g_hPreviewSwatch = NULL;

static xcolor_rgba_ g_color = { 255, 0, 0, 255 };

// =============================================================================
// 1. 颜色工具
// =============================================================================

static void DemoColorUtils()
{
	xcolor_rgba_ c = CXColorPicker::Rgb(18, 52, 86, 200);
	COLORREF cr = CXColorPicker::ToCOLORREF(c);
	xcolor_rgba_ fromCr = CXColorPicker::FromCOLORREF(cr);
	CXText hex = CXColorPicker::FormatHex(fromCr, TRUE);

	xcolor_rgba_ parsed{};
	(void)CXColorPicker::ParseHex(L"#C8123456", &parsed);
	(void)CXColorPicker::ParseHex(hex.getPtr(), &parsed);
	(void)hex;
}

// =============================================================================
// 2. 预览区
// =============================================================================

static void Demo_UpdatePreview(const xcolor_rgba_* pColor = NULL)
{
	if (!g_hPreview) return;
	const xcolor_rgba_& c = pColor ? *pColor : g_color;
	CXText hex = CXColorPicker::FormatHex(c, TRUE);
	wchar_t buf[80]{};
	swprintf_s(buf, L"当前: %s  (R%u G%u B%u A%u)", hex.getPtr(), c.r, c.g, c.b, c.a);
	XBtn_SetText(g_hPreview, buf);
	if (g_hPreviewSwatch){
		XShapeRect_SetFillColor(g_hPreviewSwatch, CXColorPicker::ToCOLORREF(c));
	}
}

// =============================================================================
// 3. 弹出选择
// =============================================================================

static int WINAPI OnBtnBindClick(HELE, BOOL*)
{
	CXColorPicker::SetBindEle(g_hBtnBind, 0, 6);
	xcolor_rgba_ c = g_color;
	if (CXColorPicker::Popup(g_hWnd, &c, TRUE, xuitool_theme_auto, 10, TRUE, xcolor_input_hex)){
		g_color = c;
		Demo_UpdatePreview();
	}
	return 0;
}

static int WINAPI OnBtnNoAlphaClick(HELE, BOOL*)
{
	CXColorPicker::SetBindEle(g_hBtnNoAlpha, 0, 6);
	xcolor_rgba_ c = g_color;
	c.a = 255;
	if (CXColorPicker::Popup(g_hWnd, &c, FALSE, xuitool_theme_light, 8, TRUE, xcolor_input_hex)){
		g_color = c;
		g_color.a = 255;
		Demo_UpdatePreview();
	}
	return 0;
}

static int WINAPI OnBtnDarkClick(HELE, BOOL*)
{
	POINT pt = { 240, 180 };
	CXColorPicker::SetPopupPosition(pt);
	xcolor_rgba_ c = g_color;
	if (CXColorPicker::Popup(g_hWnd, &c, TRUE, xuitool_theme_dark, 12, TRUE, xcolor_input_hex)){
		g_color = c;
		Demo_UpdatePreview();
	}
	return 0;
}

static int WINAPI OnBtnNoAutoCloseClick(HELE, BOOL*)
{
	CXColorPicker::SetBindEle(g_hBtnNoAutoClose, 0, 6);
	CXColorPicker::SetEnableAutoClose(FALSE);
	xcolor_rgba_ c = g_color;
	if (CXColorPicker::Popup(g_hWnd, &c, TRUE, xuitool_theme_auto, 10, TRUE, xcolor_input_hex)){
		g_color = c;
		Demo_UpdatePreview();
	}
	CXColorPicker::SetEnableAutoClose(TRUE);
	return 0;
}

static int WINAPI OnBtnRgbModeClick(HELE, BOOL*)
{
	CXColorPicker::SetBindEle(g_hBtnRgbMode, 0, 6);
	xcolor_rgba_ c = g_color;
	if (CXColorPicker::Popup(g_hWnd, &c, TRUE, xuitool_theme_auto, 10, TRUE, xcolor_input_rgb)){
		g_color = c;
		Demo_UpdatePreview();
	}
	return 0;
}

static int WINAPI OnBtnHslModeClick(HELE, BOOL*)
{
	CXColorPicker::SetBindEle(g_hBtnHslMode, 0, 6);
	xcolor_rgba_ c = g_color;
	if (CXColorPicker::Popup(g_hWnd, &c, TRUE, xuitool_theme_auto, 10, TRUE, xcolor_input_hsl)){
		g_color = c;
		Demo_UpdatePreview();
	}
	return 0;
}

static int ColorChangedCb(xcolor_rgba_ color, xcolor_change_phase_ phase, void*)
{
	if (phase == xcolor_change_live || phase == xcolor_change_commit)
		Demo_UpdatePreview(&color);
	return 0;
}

static int WINAPI OnBtnLiveClick(HELE, BOOL*)
{
	CXColorPicker::SetBindEle(g_hBtnLive, 0, 6);
	CXColorPicker::SetOnColorChanged(ColorChangedCb, NULL);
	xcolor_rgba_ c = g_color;
	const BOOL ok = CXColorPicker::Popup(g_hWnd, &c, TRUE, xuitool_theme_auto, 10, TRUE, xcolor_input_hex);
	CXColorPicker::SetOnColorChanged(NULL, NULL);
	if (ok){
		g_color = c;
	}
	Demo_UpdatePreview();
	return 0;
}

// =============================================================================
// 4. 入口
// =============================================================================

void BuildColorPickerDemo(HWINDOW hWnd)
{
	g_hWnd = hWnd;

	DemoColorUtils();

	const int kTopReserved = 30;
	const int h  = 34;
	const int y1 = kTopReserved + 8;
	const int y2 = y1 + h + 10;
	const int yPreview = y2 + h + 14;

	g_hBtnBind        = XBtn_Create(20,  y1, 118, h, L"绑定弹出",   hWnd);
	g_hBtnNoAlpha     = XBtn_Create(146, y1, 100, h, L"无 Alpha",   hWnd);
	g_hBtnDark        = XBtn_Create(254, y1, 100, h, L"深色定点",   hWnd);
	g_hBtnNoAutoClose = XBtn_Create(362, y1, 100, h, L"失焦不关",   hWnd);

	g_hBtnRgbMode = XBtn_Create(20,  y2, 100, h, L"RGB 模式", hWnd);
	g_hBtnHslMode = XBtn_Create(126, y2, 100, h, L"HSL 模式", hWnd);
	g_hBtnLive    = XBtn_Create(232, y2, 118, h, L"实时回调",  hWnd);

	g_hPreviewSwatch = XShapeRect_Create(20, yPreview, 48, 48, hWnd);
	XShapeRect_SetRoundAngle(g_hPreviewSwatch, 6, 6);
	g_hPreview = XBtn_Create(76, yPreview, 386, 48, L"", hWnd);

	HELE btns[] = {
		g_hBtnBind, g_hBtnNoAlpha, g_hBtnDark, g_hBtnNoAutoClose,
		g_hBtnRgbMode, g_hBtnHslMode, g_hBtnLive,
		g_hPreview, (HELE)g_hPreviewSwatch,
	};
	for (HELE b : btns){
		if (b) XUI_EnableCSS(b, FALSE);
	}

	XEle_RegEventC1(g_hBtnBind,        XE_BNCLICK, OnBtnBindClick);
	XEle_RegEventC1(g_hBtnNoAlpha,     XE_BNCLICK, OnBtnNoAlphaClick);
	XEle_RegEventC1(g_hBtnDark,        XE_BNCLICK, OnBtnDarkClick);
	XEle_RegEventC1(g_hBtnNoAutoClose, XE_BNCLICK, OnBtnNoAutoCloseClick);
	XEle_RegEventC1(g_hBtnRgbMode,     XE_BNCLICK, OnBtnRgbModeClick);
	XEle_RegEventC1(g_hBtnHslMode,     XE_BNCLICK, OnBtnHslModeClick);
	XEle_RegEventC1(g_hBtnLive,        XE_BNCLICK, OnBtnLiveClick);

	Demo_UpdatePreview();
}

void DestroyColorPickerDemo()
{
	g_hWnd = NULL;
	g_hBtnBind = NULL;
	g_hBtnNoAlpha = NULL;
	g_hBtnDark = NULL;
	g_hBtnNoAutoClose = NULL;
	g_hBtnRgbMode = NULL;
	g_hBtnHslMode = NULL;
	g_hBtnLive = NULL;
	g_hPreview = NULL;
	g_hPreviewSwatch = NULL;
}
