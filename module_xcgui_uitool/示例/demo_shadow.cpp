// CXShadow 完整接口使用示例.
// 覆盖 module_xcgui_uitool.h 中 CXShadow 全部 public 方法.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在无边框窗口创建后调用 BuildShadowDemo(hWnd).
//       返回的 CXShadow* 由调用方在窗口销毁前 CXShadow::Destroy().

#include "module_xcgui_uitool.h"

static CXShadow* g_pShadow = NULL;

// =============================================================================
// 1. 创建 + 附加 + 视觉参数
// =============================================================================

static void SetupShadowStyle(CXShadow& shadow)
{
	shadow.SetCornerRadius(10);
	shadow.SetBorderColor(0x0F000000u);
	shadow.SetBorderWidth(1.0f);
	shadow.SetTheme(xshadow_theme_auto);

	// 内圈填充: 默认随主题 (#202020 / #f3f3f3); 可自定义覆盖
	shadow.SetInnerBgColor(RGB(0x2A, 0x2A, 0x2A));
	(void)shadow.GetInnerBgColor();
	shadow.ClearInnerBgColor();
	(void)shadow.GetInnerBgColor();

	(void)shadow.GetCornerRadius();
	(void)shadow.GetBorderColor();
	(void)shadow.GetBorderWidth();
	(void)shadow.GetTheme();

	shadow.Invalidate();
}

// =============================================================================
// 2. Snap / 最大化控制 + 全局主题
// =============================================================================

static void DemoShadowControls(CXShadow& shadow)
{
	shadow.EnableSnap(TRUE);
	(void)shadow.IsSnapEnabled();
	shadow.EnableMaximize(TRUE);
	(void)shadow.IsMaximizeEnabled();
	(void)shadow.IsMaximized();
}

static void DemoShadowGlobalTheme()
{
	CXShadow::SetGlobalTheme(xshadow_theme_dark);
	(void)CXShadow::GetGlobalTheme();
	CXShadow::SetGlobalTheme(xshadow_theme_auto);
}

// =============================================================================
// 3. 入口
// =============================================================================

CXShadow* BuildShadowDemo(HWINDOW hWnd)
{
	g_pShadow = CXShadow::Create();
	if (!g_pShadow) return NULL;

	if (!g_pShadow->AttachToWnd(hWnd)){
		CXShadow::Destroy(g_pShadow);
		g_pShadow = NULL;
		return NULL;
	}

	(void)g_pShadow->IsAttached();
	(void)g_pShadow->GetAttachedWnd();

	SetupShadowStyle(*g_pShadow);
	DemoShadowControls(*g_pShadow);
	DemoShadowGlobalTheme();

	return g_pShadow;
}

void DestroyShadowDemo()
{
	if (g_pShadow){
		g_pShadow->Detach();
		CXShadow::Destroy(g_pShadow);
		g_pShadow = NULL;
	}
}
