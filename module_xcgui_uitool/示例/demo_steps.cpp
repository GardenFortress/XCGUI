// CXSteps 使用示例 — 水平/垂直步骤条 + 深浅主题 + 顺序调换 + 切换动画.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 窗口创建后 BuildStepsDemo(hWnd); 关闭前 DestroyStepsDemo().

#include "module_xcgui_uitool.h"

static CXSteps* g_steps = NULL;

void BuildStepsDemo(HWINDOW hWnd)
{
	if (g_steps) return;

	g_steps = new CXSteps();
	if (!g_steps->Create(hWnd)){
		delete g_steps;
		g_steps = NULL;
		return;
	}

	g_steps->SetTheme(xuitool_theme_dark);
	g_steps->SetOrientation(xsteps_orient_horizontal);
	g_steps->SetContentOrder(xsteps_content_label_first); // 水平: 圆在上; 垂直: 圆在左
	g_steps->SetAnimEnabled(TRUE);
	g_steps->SetAnimDuration(300);

	g_steps->AddStep(L"Register");
	g_steps->AddStep(L"Choose plan");
	g_steps->AddStep(L"Purchase");
	g_steps->AddStep(L"Receive Product");
	g_steps->SetCurrentStep(1);
	g_steps->AdjustLayout();
}

void DestroyStepsDemo()
{
	if (!g_steps) return;
	g_steps->DestroySteps();
	delete g_steps;
	g_steps = NULL;
}
