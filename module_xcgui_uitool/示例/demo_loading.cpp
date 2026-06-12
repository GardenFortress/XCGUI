// CXLoading 完整接口使用示例.
// 覆盖 module_xcgui_uitool.h 中 CXLoading 全部静态 public 方法.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建后调用 BuildLoadingDemo(hWnd).

#include "module_xcgui_uitool.h"

static HELE    g_hPanel     = NULL;
static HELE    g_hLoadingEle = NULL;
static HWINDOW g_hWnd       = NULL;

// =============================================================================
// 1. 三种宿主形态: Create / AttachEle / AttachWnd
// =============================================================================

static void SetupLoadingOnEle(HELE hHost)
{
	CXLoading::AttachEle(hHost);
	CXLoading::SetStyle(hHost, xloading_style_spinner);
	CXLoading::SetSize(hHost, 48, 48);
	CXLoading::SetText(hHost, L"加载中...");
	CXLoading::SetFontSize(hHost, 10);
	CXLoading::SetTheme(hHost, xuitool_theme_dark);
	CXLoading::SetCornerRadius(hHost, 8);
	CXLoading::SetSpeed(hHost, 1.0f);
	CXLoading::Start(hHost);
}

static void SetupLoadingOnWnd(HWINDOW hWnd)
{
	CXLoading::AttachWnd(hWnd);
	CXLoading::SetStyle(hWnd, xloading_style_dots);
	CXLoading::SetText(hWnd, L"整窗加载蒙层");
	CXLoading::SetTheme(hWnd, xuitool_theme_auto);
	CXLoading::SetCornerRadiusEx(hWnd, 12, 12, 0, 0);
	CXLoading::SetSpeed(hWnd, 1.5f);
	// CXLoading::Start(hWnd);  // 默认 Create/Attach 后已启动, 可按需 Stop/Start
}

static void SetupLoadingCreate(HELE hParent)
{
	g_hLoadingEle = CXLoading::Create(20, 120, 200, 120, hParent);
	CXLoading::SetStyle(g_hLoadingEle, xloading_style_bars);
	CXLoading::SetTheme(g_hLoadingEle, xuitool_theme_custom);
	CXLoading::SetTextColor(g_hLoadingEle, RGB(0xE0, 0xE0, 0xE0));
	CXLoading::SetBkColor(g_hLoadingEle, RGB(0x2D, 0x2D, 0x30));
	CXLoading::SetAccentColor(g_hLoadingEle, RGB(0x60, 0xA5, 0xFA));
	CXLoading::SetText(g_hLoadingEle, L"频谱条风格");
}

// =============================================================================
// 2. 五种动画风格 + Get 系列
// =============================================================================

static void DemoLoadingStyles(HELE hHost)
{
	const xloading_style_ styles[] = {
		xloading_style_spinner,
		xloading_style_dots,
		xloading_style_spokes,
		xloading_style_pulse,
		xloading_style_bars,
	};
	for (int i = 0; i < 5; ++i){
		CXLoading::SetStyle(hHost, styles[i]);
		(void)CXLoading::GetStyle(hHost);
	}
}

static void DemoLoadingGet(HELE hHost)
{
	if (!CXLoading::HasAttached(hHost)) return;

	int cx = 0, cy = 0;
	CXLoading::GetSize(hHost, &cx, &cy);
	(void)CXLoading::GetText(hHost);
	(void)CXLoading::GetFontSize(hHost);
	(void)CXLoading::GetTheme(hHost);
	(void)CXLoading::GetTextColor(hHost);
	(void)CXLoading::GetBkColor(hHost);
	(void)CXLoading::GetAccentColor(hHost);
	(void)CXLoading::GetCornerRadius(hHost);
	int lt = 0, rt = 0, rb = 0, lb = 0;
	CXLoading::GetCornerRadiusEx(hHost, &lt, &rt, &rb, &lb);
	(void)CXLoading::GetSpeed(hHost);
	(void)CXLoading::IsRunning(hHost);

	CXLoading::Stop(hHost);
	(void)CXLoading::IsRunning(hHost);
	CXLoading::Start(hHost);
}

static void DemoLoadingDetach(HELE hHost)
{
	CXLoading::Detach(hHost);
	// 进程退出前: CXLoading::Cleanup();
}

// =============================================================================
// 3. 入口
// =============================================================================

void BuildLoadingDemo(HXCGUI hWnd)
{
	g_hWnd = (HWINDOW)hWnd;
	g_hPanel = XLayout_Create(20, 20, 360, 80, hWnd);
	XEle_SetBkColor(g_hPanel, RGB(0xF0, 0xF0, 0xF0));

	SetupLoadingOnEle(g_hPanel);
	SetupLoadingOnWnd(g_hWnd);
	SetupLoadingCreate(hWnd);

	DemoLoadingStyles(g_hPanel);
	DemoLoadingGet(g_hPanel);
	// DemoLoadingDetach(g_hPanel);
}
