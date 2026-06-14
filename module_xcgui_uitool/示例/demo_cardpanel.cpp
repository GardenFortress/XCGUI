// CXCardPanel 使用示例 — 设置页风格 (组标题 + 圆角卡片 + 开关行).
//
// 注意:
//   - SetGroupContentEle 会*替换*卡片内容; 连续添加多个控件请用 AddGroupContentEle.
//   - 两个按钮并排时, 请放入水平布局再 SetGroupContentEle, 或对水平布局 EnableWrap.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建后调用 BuildCardPanelDemo(hWnd); 关闭前调用 DestroyCardPanelDemo().

#include "module_xcgui_uitool.h"

static CXCardPanel* g_panel = NULL;

// =============================================================================
// 辅助: 设置行 (标题 + 描述 + 右侧开关)
// =============================================================================

static HELE CreateToggleRow(const wchar_t* title, const wchar_t* desc, BOOL bChecked = TRUE)
{
	HELE hRoot = XLayout_Create(0, 0, 400, 46, NULL);
	if (!hRoot) return NULL;

	XLayout_EnableLayout(hRoot, TRUE);
	XLayoutBox_EnableHorizon(hRoot, TRUE);
	XLayoutBox_SetAlignV(hRoot, layout_align_center);
	XLayoutBox_SetSpace(hRoot, 12);
	XEle_EnableBkTransparent(hRoot, TRUE);
	XWidget_LayoutItem_SetWidth(hRoot, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(hRoot, layout_size_auto, 0);
	XEle_SetPadding(hRoot, 16, 12, 16, 12);
	XWidget_LayoutItem_SetMinSize(hRoot, 0, 46);

	HELE hTextCol = XLayout_Create(0, 0, 300, 40, hRoot);
	if (hTextCol){
		XLayout_EnableLayout(hTextCol, TRUE);
		XLayoutBox_SetAlignV(hTextCol, layout_align_top);
		XLayoutBox_SetSpace(hTextCol, 4);
		XEle_EnableBkTransparent(hTextCol, TRUE);
		XWidget_LayoutItem_SetWidth(hTextCol, layout_size_weight, 1);
		XWidget_LayoutItem_SetHeight(hTextCol, layout_size_auto, 0);

		HXCGUI hTitle = XShapeText_Create(0, 0, 280, 22, title ? title : L"", hTextCol);
		if (hTitle){
			XUI_EnableCSS(hTitle, FALSE);
			HFONTX hFont = XFont_CreateEx(L"Segoe UI", 10, fontStyle_bold);
			if (hFont) XShapeText_SetFont(hTitle, hFont);
			XShapeText_SetTextColor(hTitle, RGBA(23, 23, 23, 255));
			XShapeText_SetTextAlign(hTitle, textAlignFlag_left | textAlignFlag_top);
			XWidget_LayoutItem_SetWidth(hTitle, layout_size_fill, 0);
			XWidget_LayoutItem_SetHeight(hTitle, layout_size_auto, 0);
		}

		if (desc && desc[0]){
			HXCGUI hDesc = XShapeText_Create(0, 0, 280, 32, desc, hTextCol);
			if (hDesc){
				XUI_EnableCSS(hDesc, FALSE);
				HFONTX hFont = XFont_CreateEx(L"Segoe UI", 9, fontStyle_regular);
				if (hFont) XShapeText_SetFont(hDesc, hFont);
				XShapeText_SetTextColor(hDesc, RGBA(107, 114, 128, 255));
				XShapeText_SetTextAlign(hDesc, textAlignFlag_left | textAlignFlag_top);
				XWidget_LayoutItem_SetWidth(hDesc, layout_size_fill, 0);
				XWidget_LayoutItem_SetHeight(hDesc, layout_size_auto, 0);
			}
		}
	}

	HELE hSwitch = XBtn_Create(0, 0, 44, 24, L"", hRoot);
	if (hSwitch){
		XUI_EnableCSS(hSwitch, FALSE);
		XBtn_SetTypeEx(hSwitch, button_type_check);
		XBtn_SetCheck(hSwitch, bChecked ? TRUE : FALSE);
		XWidget_LayoutItem_SetWidth(hSwitch, layout_size_auto, 0);
		XWidget_LayoutItem_SetHeight(hSwitch, layout_size_fixed, 24);
	}

	return hRoot;
}

// =============================================================================
// 构建 / 销毁
// =============================================================================

void BuildCardPanelDemo(HWINDOW hWnd)
{
	if (g_panel) return;

	g_panel = new CXCardPanel();
	if (!g_panel->Create(hWnd)) {
		delete g_panel;
		g_panel = NULL;
		return;
	}

	g_panel->SetTheme(xuitool_theme_light);
	g_panel->SetGroupTitleAlign(xcardpanel_group_title_align_left);
	g_panel->SetCornerRadius(10);
	g_panel->EnableScroll(TRUE);

	int g1 = g_panel->AddGroup(L"系统设置");
	if (g1){
		HELE hRow = CreateToggleRow(
			L"在系统右键菜单增加「通过QQ发送」选项",
			L"可将本地文件便捷发送给我的手机、QQ好友或进行闪传发送",
			TRUE);
		g_panel->SetGroupContentEle(g1, hRow);
	}

	int g2 = g_panel->AddGroup(L"其他");
	if (g2){
		HELE hRow = CreateToggleRow(
			L"透明效果",
			L"窗口和表面显示半透明",
			TRUE);
		g_panel->SetGroupContentEle(g2, hRow);
	}

	g_panel->AdjustLayout();
}

void DestroyCardPanelDemo()
{
	if (!g_panel) return;
	g_panel->DestroyCardPanel();
	delete g_panel;
	g_panel = NULL;
}
