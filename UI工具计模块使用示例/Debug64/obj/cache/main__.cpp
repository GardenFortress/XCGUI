#include "common__.h"
#include "main__.h"
extern int g_build;
extern 主窗口类 主窗口;
int g_build = 110;
void 窗口模糊类::置模糊通透度(float 模糊层通透感)
{
	blur.SetBlurOpacity(模糊层通透感);
}
void 窗口模糊类::启用Snap(BOOL 是否启用)
{
	if(XC_IsHWINDOW(m_hWindow))
	{
		blur.EnableSnap(m_hWindow, 是否启用);
	}
}
void 窗口模糊类::清理()
{
	blur.Detach();
	shadow.Detach();
	m_hWindow = NULL;
}
void 窗口模糊类::初始化()
{
	blur.SetTheme(xuitool_theme_auto);
	xcl_msgBox(XC_itow(g_build));
	if(g_build == 11)
	{
		blur.AttachToWndEx(m_hWindow);
	}
	else
	{
		XWnd_SetTransparentType(m_hWindow, window_transparent_shaped);
		XWnd_SetTransparentAlpha(m_hWindow, 255);
		shadow.EnableSnap(TRUE);
		shadow.SetCornerRadius(10);
		shadow.SetBorderWidth(1.0f);
		shadow.AttachToWnd(m_hWindow);
			;
	}
}
BOOL 窗口模糊类::初始化(HWINDOW hWindow)
{
	if(! XC_IsHWINDOW(hWindow))
	{
		return FALSE;
	}
	m_hWindow = hWindow;
	初始化();
	return TRUE;
}
void 主窗口类::折叠面板测试()
{
	acc.Create(m_hWindow);
	acc.SetTheme(xuitool_theme_dark);
	acc.SetGroupTitleAlign(xaccordion_group_title_align_left);
	int g1 = acc.AddGroup(L"折叠面板1");
	int i1 = acc.AddItem(g1, L"危险的项");
	acc.SetItemBadge(i1, L"错误危险的");
	acc.SetItemBadge(i1, L"错误:0xC000005", xaccordion_badge_danger);
	acc.SetItemBodyText(i1, L"测试内容:抢先版已更新，可直接使用。");
	int i2 = acc.AddItem(g1, L"成功安全的");
	acc.SetItemIcon(i2, xaccordion_icon_status_done);
	acc.SetItemBadge(i2, L"成功安全的", xaccordion_badge_success);
	HELE hPanel = XBtn_Create(0, 0, 100, 200, L"BUTTON");
	acc.SetItemContentEle(i2, hPanel);
	acc.ExpandItem(i1) ;
	int i3 = acc.AddItem(g1, L"中性的项");
	acc.SetItemBadge(i3, L"中性一般的", xaccordion_badge_neutral);
	acc.SetItemBodyText(i3, L"测试内容:抢先版已更新，可直接使用。");
	int g2 = acc.AddGroup(L"折叠面板2");
	int i4 = acc.AddItem(g2, L"信息的项");
	acc.SetItemBadge(i4, L"信息项的内容", xaccordion_badge_info);
	acc.SetItemBodyText(i4, L"测试内容:抢先版已更新，可直接使用。");
	int i5 = acc.AddItem(g2, L"警告的项");
	acc.SetItemBadge(i5, L"警告项的内容", xaccordion_badge_warning);
	acc.SetItemBodyText(i5, L"测试内容:抢先版已更新，可直接使用。");
	int i6 = acc.AddItem(g2, L"无徽章标签的项");
	acc.SetItemBodyText(i6, L"测试内容:抢先版已更新，可直接使用。");
}
void 主窗口类::卡片面板测试()
{
	panl.Create(m_hWindow);
	panl.SetPadding(16, 0, 16, 0);
	panl.SetTheme(xuitool_theme_light);
	panl.SetGroupTitleAlign(xcardpanel_group_title_align_left);
	panl.EnableScroll(TRUE);
	int g1 = panl.AddGroup(L"系统设置");
	HELE hRow = XBtn_Create(0, 0, 0, 48, L"BUTTON1");
	XEle_EnableBkTransparent(hRow, TRUE);
	XWidget_LayoutItem_SetWidth(hRow, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(hRow, layout_size_fixed, 46);
	panl.SetGroupContentEle(g1, hRow);
	hRow = (HELE)XShapeText_Create(0, 0, 0, 48, L"可将本地文件便捷发送给我的手机、QQ好友或进行闪传发送");
	XWidget_LayoutItem_SetWidth(hRow, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(hRow, layout_size_fixed, 100);
	panl.AddGroupContentEle(g1, hRow);
	int g2 = panl.AddGroup(L"系统设置2");
	hRow = XBtn_Create(0, 0, 0, 48, L"BUTTON1");
	XWidget_LayoutItem_EnableWrap(hRow, TRUE);
	XWidget_LayoutItem_SetWidth(hRow, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(hRow, layout_size_fixed, 46);
	panl.SetGroupContentEle(g2, hRow);
}
void 主窗口类::炫彩步骤条类测试()
{
	steps.Create(m_hWindow);
	steps.SetTheme(xuitool_theme_custom);
	steps.SetOrientation(xsteps_orient_vertical);
	steps.SetContentOrder(xsteps_content_label_first);
	steps.SetAnimEnabled(TRUE);
	steps.AddStep(L"Register");
	steps.AddStep(L"Choose plan");
	steps.AddStep(L"Purchase");
	steps.AddStep(L"Receive Product");
	steps.SetCurrentStep(1) ;
	steps.AdjustLayout();
}
void 主窗口类::Test()
{
		;
}
void 主窗口类::颜色选择器()
{
	CXColorPicker::SetBindEle(_按钮5.m_hEle, 0, 6);
	CXColorPicker::SetEnableAutoClose(FALSE) ;
	CXColorPicker::SetEnableModal(FALSE) ;
	CXColorPicker::SetEnableDrag(TRUE) ;
	CXColorPicker::SetEnableTopmost(TRUE) ;
		;
}
int 主窗口类::运行()
{
	{
	HMODULE __hModule = GetModuleHandle(NULL);
	UINT __size = 0;
	void* __data = rc_findFileByID(IDR_UI_ZIP, &__size, __hModule);
	if (__data) {
		m_hWindow = (HWINDOW)XC_LoadLayoutZipMemEx(__data, __size, _布局文件.get(), NULL, NULL, NULL, NULL, NULL);
	}
#ifdef _DEBUG
	if(NULL==m_hWindow) {MessageBox(NULL, L"句柄为空:\"m_hWindow\"", L"提示", 0); return 0;}
#endif
	_按钮1.m_hEle = (HELE)XC_GetObjectByName(L"按钮1");
	_布局元素1.m_hEle = (HELE)XC_GetObjectByName(L"布局元素1");
	_按钮2.m_hEle = (HELE)XC_GetObjectByName(L"按钮2");
	_显示双日期.m_hEle = (HELE)XC_GetObjectByName(L"显示双日期");
	_布局元素2.m_hEle = (HELE)XC_GetObjectByName(L"布局元素2");
	_布局元素3.m_hEle = (HELE)XC_GetObjectByName(L"布局元素3");
	_按钮5.m_hEle = (HELE)XC_GetObjectByName(L"按钮5");
#ifdef _DEBUG
if(NULL==_按钮1.m_hEle) {MessageBox(NULL, L"绑定变量句柄为空:\"_按钮1\"", L"提示", 0); return 0;}
if(NULL==_布局元素1.m_hEle) {MessageBox(NULL, L"绑定变量句柄为空:\"_布局元素1\"", L"提示", 0); return 0;}
if(NULL==_按钮2.m_hEle) {MessageBox(NULL, L"绑定变量句柄为空:\"_按钮2\"", L"提示", 0); return 0;}
if(NULL==_显示双日期.m_hEle) {MessageBox(NULL, L"绑定变量句柄为空:\"_显示双日期\"", L"提示", 0); return 0;}
if(NULL==_布局元素2.m_hEle) {MessageBox(NULL, L"绑定变量句柄为空:\"_布局元素2\"", L"提示", 0); return 0;}
if(NULL==_布局元素3.m_hEle) {MessageBox(NULL, L"绑定变量句柄为空:\"_布局元素3\"", L"提示", 0); return 0;}
if(NULL==_按钮5.m_hEle) {MessageBox(NULL, L"绑定变量句柄为空:\"_按钮5\"", L"提示", 0); return 0;}
#endif
	XEle_RegEventCPP1(_按钮2.m_hEle, XE_BNCLICK, &主窗口类::按钮点击_按钮2);
	XEle_RegEventCPP1(_显示双日期.m_hEle, XE_BNCLICK, &主窗口类::按钮点击_显示双日期);
	XEle_RegEventCPP1(_按钮5.m_hEle, XE_BNCLICK, &主窗口类::按钮点击_按钮5);
	}
	blur.初始化(m_hWindow);
	颜色选择器();
	Show(TRUE);
	return 0;
}
int 主窗口类::按钮点击_按钮2(HELE 来源句柄, BOOL* 是否拦截)
{
	xcalendar_datetime_ date;
	xcalendar_datetime_ start;
	xcalendar_datetime_ end;
	CXCalendarCard::SetBindEle(_按钮2.m_hEle);
	CXCalendarCard::PopupSingle(m_hWindow, &date, FALSE, xuitool_theme_auto);
	return 0;
}
int 主窗口类::按钮点击_显示双日期(HELE 来源句柄, BOOL* 是否拦截)
{
	xcalendar_datetime_ date;
	xcalendar_datetime_ start;
	xcalendar_datetime_ end;
	CXCalendarCard::SetBindEle(_显示双日期.m_hEle);
	CXCalendarCard::PopupDouble(m_hWindow, &start, &end, TRUE, xuitool_theme_auto);
	return 0;
}
int 主窗口类::按钮点击_按钮5(HELE 来源句柄, BOOL* 是否拦截)
{
	xcolor_rgba_ rgba{0};
	if(color.Popup(m_hWindow, &rgba, FALSE))
	{
		xcl_log(XL_FUNNAME, L"颜色已经更新");
	}
	return 0;
}
主窗口类 主窗口;
int WINAPI wWinMain(HINSTANCE 模块句柄, HINSTANCE 先前句柄, wchar_t* 命令行, int 窗口显示标识)
{
	XInitXCGUI(TRUE);
	XC_EnableAutoRedrawUI(TRUE);
	XC_SetPaintFrequency(8);
	XC_EnableDPI(TRUE);
	XC_EnableAutoDPI(TRUE);
	HMODULE __hModule = GetModuleHandle(NULL);
	UINT __size = 0;
	void* __data = rc_findFileByID(IDR_UI_ZIP, &__size, __hModule);
	BOOL __bRetRes = XC_LoadResourceZipMem(__data, __size, LR"--(资源文件\resource.res)--");
	if(0==__bRetRes) xcl_log(L"*加载资源文件失败: resource.res");
	主窗口.运行();
	XRunXCGUI();
	XExitXCGUI();
	return 0;
}
