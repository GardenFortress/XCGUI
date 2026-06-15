// CXCheckAnim 使用示例 — WinUI3 风格多选框动画
// 编译: build_test_checkanim.ps1 [-Run]
//
// 用法: 窗口创建后调用 BuildCheckAnimDemo(hWnd);

#include "module_xcgui_uitool.h"

static HELE    g_hPanelDark  = NULL;
static HELE    g_hPanelLight = NULL;
static HWINDOW g_hWnd        = NULL;

static HELE CreateCheckRow(HELE hParent, int y, const wchar_t* title, BOOL bChecked,
	xuitool_theme_ theme, xcheckanim_text_align_ align)
{
	HELE hBtn = XBtn_Create(16, y, 200, 20, title, hParent);
	if (!hBtn) return NULL;

	XUI_EnableCSS(hBtn, FALSE);
	XBtn_SetCheck(hBtn, bChecked ? TRUE : FALSE);
	CXCheckAnim::AttachBtn(hBtn, 0, 20, theme);
	CXCheckAnim::SetTextAlign(hBtn, align);
	CXCheckAnim::SetAnimEnabled(hBtn, TRUE);
	return hBtn;
}

void BuildCheckAnimDemo(HXCGUI hWnd)
{
	g_hWnd = (HWINDOW)hWnd;

	g_hPanelDark = XLayout_Create(16, 16, 420, 220, hWnd);
	if (g_hPanelDark){
		XShapeText_Create(12, 8, 200, 24, L"深色主题", g_hPanelDark);

		CreateCheckRow(g_hPanelDark, 40, L"启用通知", TRUE,
			xuitool_theme_dark, xcheckanim_text_align_right);
		CreateCheckRow(g_hPanelDark, 72, L"自动更新", FALSE,
			xuitool_theme_dark, xcheckanim_text_align_right);
		CreateCheckRow(g_hPanelDark, 104, L"", TRUE,
			xuitool_theme_dark, xcheckanim_text_align_right);
		CreateCheckRow(g_hPanelDark, 136, L"左对齐开关", FALSE,
			xuitool_theme_dark, xcheckanim_text_align_left);

		HELE hDisabled = XBtn_Create(16, 168, 200, 20, L"已禁用", g_hPanelDark);
		if (hDisabled){
			XUI_EnableCSS(hDisabled, FALSE);
			XEle_Enable(hDisabled, FALSE);
			CXCheckAnim::AttachBtn(hDisabled, 0, 20, xuitool_theme_dark);
		}
	}

	g_hPanelLight = XLayout_Create(16, 248, 420, 180, hWnd);
	if (g_hPanelLight){
		XShapeText_Create(12, 8, 200, 24, L"浅色主题", g_hPanelLight);

		CreateCheckRow(g_hPanelLight, 40, L"Wi-Fi", TRUE,
			xuitool_theme_light, xcheckanim_text_align_right);
		CreateCheckRow(g_hPanelLight, 72, L"蓝牙", FALSE,
			xuitool_theme_light, xcheckanim_text_align_right);
		CreateCheckRow(g_hPanelLight, 104, L"", FALSE,
			xuitool_theme_light, xcheckanim_text_align_right);
	}
}

void DestroyCheckAnimDemo()
{
	CXCheckAnim::Cleanup();
	g_hPanelDark  = NULL;
	g_hPanelLight = NULL;
	g_hWnd        = NULL;
}
