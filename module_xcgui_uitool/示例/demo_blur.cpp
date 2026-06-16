// CXBlur 完整接口使用示例.
// 覆盖 module_xcgui_uitool.h 中 CXBlur 全部 public 方法.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建后调用 BuildBlurDemo(hWnd).
//       返回后可用 DestroyBlurDemo() 释放.

#include "module_xcgui_uitool.h"

static CXBlur* g_pBlurWnd  = NULL;
static CXBlur* g_pBlurOwn  = NULL;
static CXBlur* g_pBlurEle  = NULL;
static HELE    g_hUserEle  = NULL;

// =============================================================================
// 1. 三种绑定: Create / AttachToEle / AttachToWnd / AttachToWndEx
// =============================================================================

static void SetupBlurOwn(HXCGUI hParent)
{
	g_pBlurOwn = new CXBlur();
	g_pBlurOwn->Create(20, 20, 200, 160, hParent);
	g_pBlurOwn->SetTheme(xuitool_theme_light);
	g_pBlurOwn->SetCornerRadius(12);
	g_pBlurOwn->SetBorderColor(0x33000000u);
	g_pBlurOwn->SetBorderWidth(1.0f);
}

static void SetupBlurAttachEle(HXCGUI hParent)
{
	g_hUserEle = XLayout_Create(240, 20, 200, 160, hParent);
	XEle_EnableBkTransparent(g_hUserEle, TRUE);

	g_pBlurEle = new CXBlur();
	g_pBlurEle->AttachToEle(g_hUserEle);
	g_pBlurEle->SetTheme(xuitool_theme_dark);
	g_pBlurEle->SetCornerRadiusEx(12, 12, 0, 0);
}

static void SetupBlurAttachWnd(HWINDOW hWnd)
{
	g_pBlurWnd = new CXBlur();
	// 不显式 SetTheme — 依赖 BuildBlurDemo 开头 CXBlur::SetGlobalTheme (P0 回归).
	g_pBlurWnd->AttachToWndEx(hWnd, xblur_path_auto);
	g_pBlurWnd->SetNoise(0.06f);
	g_pBlurWnd->SetUniformBrightness(TRUE);
	g_pBlurWnd->SetBlurOpacity(0.5f);
}

// =============================================================================
// 2. 主题 / 颜色 / Get 系列 + 静态窗口 API
// =============================================================================

static void DemoBlurParams(CXBlur& blur)
{
	blur.SetTintColor(RGBA(0xF3, 0xF3, 0xF3, 0xCC));
	(void)blur.GetTintColor();
	(void)blur.GetTheme();
	(void)blur.GetNoise();
	(void)blur.GetUniformBrightness();
	(void)blur.GetBlurOpacity();
	(void)blur.GetCornerRadius();
	int lt = 0, rt = 0, rb = 0, lb = 0;
	blur.GetCornerRadiusEx(&lt, &rt, &rb, &lb);
	(void)blur.GetBorderColor();
	(void)blur.GetBorderWidth();
	(void)blur.GetBindMode();
	(void)blur.IsSystemAcrylicEnabled();
	blur.Invalidate();
}

static void DemoBlurStaticApi(HWINDOW hWnd)
{
	(void)CXBlur::IsSystemAcrylicSupported();

	(void)CXBlur::GetGlobalTheme();

	CXBlurThemeDefaults def = CXBlur::GetThemeDefault(xuitool_theme_light);
	def.noise = 0.08f;
	CXBlur::SetThemeDefault(xuitool_theme_light, def);

	CXBlur::EnableNativeRoundedCorner(hWnd, xblur_corner_round);
	// CXBlur::EnableNativeShadow(hWnd, TRUE);   // 纯 popup 窗才需要
	CXBlur::EnableSnap(hWnd, TRUE);
	CXBlur::EnableMaximize(hWnd, TRUE);
	// CXBlur::ForceSystemTransparencyOn(TRUE);  // 用户级设置, 慎用
}

// =============================================================================
// 3. 入口 / 销毁
// =============================================================================

void BuildBlurDemo(HXCGUI hWnd)
{
	HWINDOW hw = (HWINDOW)hWnd;

	CXBlur::ForceSystemTransparencyOn(FALSE);
	// P0: 全局主题在 AttachToWndEx 之前设置; SetupBlurAttachWnd 不再调 SetTheme.
	CXBlur::SetGlobalTheme(xuitool_theme_auto);
	SetupBlurOwn(hw);
	SetupBlurAttachEle(hw);
	SetupBlurAttachWnd(hw);

	if (g_pBlurOwn) DemoBlurParams(*g_pBlurOwn);
	if (g_pBlurEle) DemoBlurParams(*g_pBlurEle);
	if (g_pBlurWnd) DemoBlurParams(*g_pBlurWnd);
	DemoBlurStaticApi(hw);
}

void DestroyBlurDemo()
{
	if (g_pBlurOwn){ g_pBlurOwn->Detach(); delete g_pBlurOwn; g_pBlurOwn = NULL; }
	if (g_pBlurEle){ g_pBlurEle->Detach(); delete g_pBlurEle; g_pBlurEle = NULL; }
	if (g_pBlurWnd){ g_pBlurWnd->Detach(); delete g_pBlurWnd; g_pBlurWnd = NULL; }
}
