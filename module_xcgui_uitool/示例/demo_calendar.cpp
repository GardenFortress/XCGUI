// CXCalendarCard 完整接口使用示例.
// 覆盖 module_xcgui_uitool.h 中 CXCalendarCard 全部静态 public 方法.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建后调用 BuildCalendarDemo(hWnd).

#include "module_xcgui_uitool.h"

static HWINDOW g_hWnd       = NULL;
static HELE    g_hBtnSingle = NULL;
static HELE    g_hBtnDouble = NULL;
static HELE    g_hBtnToday  = NULL;

// =============================================================================
// 1. 日期工具
// =============================================================================

static void DemoDateUtils()
{
	xcalendar_datetime_ today = CXCalendarCard::GetToday();
	(void)today;

	CXText sFull = CXCalendarCard::FormatDateTime(today);
	CXText sShort = CXCalendarCard::FormatShortDate(today);
	(void)sFull;
	(void)sShort;

	(void)CXCalendarCard::FormatShortDatePtr(today);
	(void)CXCalendarCard::FormatDateTimePtr(today);
}

// =============================================================================
// 2. 弹出选择 (单月历 / 双月历)
// =============================================================================

static int WINAPI OnBtnSingleClick(HELE, BOOL*)
{
	xcalendar_datetime_ date = CXCalendarCard::GetToday();
	CXCalendarCard::SetBindEle(g_hBtnSingle, 0, 4);

	if (CXCalendarCard::PopupSingle(g_hWnd, &date, TRUE, xuitool_theme_auto, NULL, 10)){
		CXText s = CXCalendarCard::FormatShortDate(date);
		XEle_SetText(g_hBtnSingle, s.getPtr());
	}
	return 0;
}

static int WINAPI OnBtnDoubleClick(HELE, BOOL*)
{
	xcalendar_datetime_ start = CXCalendarCard::GetToday();
	xcalendar_datetime_ end   = start;
	end.hour = 23; end.minute = 59; end.second = 59;

	POINT pt = { 200, 200 };
	CXCalendarCard::SetPopupPosition(pt);

	if (CXCalendarCard::PopupDouble(g_hWnd, &start, &end, TRUE, xuitool_theme_dark, NULL, 12)){
		CXText s1 = CXCalendarCard::FormatDateTime(start);
		CXText s2 = CXCalendarCard::FormatDateTime(end);
		wchar_t buf[128];
		swprintf_s(buf, L"%s ~ %s", s1.getPtr(), s2.getPtr());
		XEle_SetText(g_hBtnDouble, buf);
	}
	return 0;
}

static int WINAPI OnBtnTodayClick(HELE, BOOL*)
{
	xcalendar_datetime_ maxDate = CXCalendarCard::GetToday();
	xcalendar_datetime_ date = { 2020, 1, 1, 0, 0, 0 };

	if (CXCalendarCard::PopupSingle(g_hWnd, &date, TRUE, xuitool_theme_light, &maxDate, 8)){
		CXText s = CXCalendarCard::FormatShortDate(date);
		XEle_SetText(g_hBtnToday, s.getPtr());
	}
	return 0;
}

// =============================================================================
// 3. 入口
// =============================================================================

void BuildCalendarDemo(HXCGUI hWnd)
{
	g_hWnd = (HWINDOW)hWnd;

	DemoDateUtils();

	g_hBtnSingle = XBtn_Create(20,  20, 140, 36, L"单月历选择", hWnd);
	g_hBtnDouble = XBtn_Create(170, 20, 140, 36, L"双月历范围", hWnd);
	g_hBtnToday  = XBtn_Create(320, 20, 140, 36, L"限制到今天", hWnd);

	if (g_hBtnSingle) XUI_EnableCSS(g_hBtnSingle, FALSE);
	if (g_hBtnDouble) XUI_EnableCSS(g_hBtnDouble, FALSE);
	if (g_hBtnToday) XUI_EnableCSS(g_hBtnToday, FALSE);

	XEle_RegEventC1(g_hBtnSingle, XE_BNCLICK, OnBtnSingleClick);
	XEle_RegEventC1(g_hBtnDouble, XE_BNCLICK, OnBtnDoubleClick);
	XEle_RegEventC1(g_hBtnToday,  XE_BNCLICK, OnBtnTodayClick);
}
