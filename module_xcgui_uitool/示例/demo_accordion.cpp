// CXAccordion 完整接口使用示例.
// 覆盖 module_xcgui_uitool.h 中 CXAccordion 全部 public 方法 + 事件回调.
// 演示 FAQ 单组模式、引导清单分组模式、元素内容区、主题/展开/禁用等交互.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建后调用 BuildAccordionDemo(hWnd); 关闭前调用 DestroyAccordionDemo().

#include "module_xcgui_uitool.h"

static CXAccordion* g_acc           = NULL;
static HWINDOW      g_hWnd          = NULL;
static int          g_faqGroupId    = 0;
static int          g_setupGroupId  = 0;
static int          g_apiGroupId    = 0;
static int          g_payItemId     = 0;
static int          g_ecItemId      = 0;
static int          g_tempItemId    = 0;

// =============================================================================
// 1. 事件回调
// =============================================================================

static int WINAPI OnAccItemExpand(CXAccordion* p, int itemId, BOOL* pbHandled)
{
	(void)p; (void)itemId; (void)pbHandled;
	return 0;
}

static int WINAPI OnAccItemCollapse(CXAccordion* p, int itemId, BOOL* pbHandled)
{
	(void)p; (void)itemId; (void)pbHandled;
	return 0;
}

static int WINAPI OnAccItemClick(CXAccordion* p, int itemId, BOOL* pbHandled)
{
	(void)p;
	// 返回 TRUE 且 *pbHandled=TRUE 可阻止默认展开/收起.
	if (itemId == g_tempItemId){
		// 示例: 临时项点击时不切换.
		// *pbHandled = TRUE;
	}
	(void)pbHandled;
	return 0;
}

static int WINAPI OnAccThemeChanged(CXAccordion* p)
{
	(void)p;
	return 0;
}

// =============================================================================
// 2. 辅助: 元素内容面板
// =============================================================================

static HELE CreatePaymentPanel()
{
	HELE hRoot = XLayout_Create(0, 0, 420, 10, NULL);
	if (!hRoot) return NULL;
	XLayout_EnableLayout(hRoot, TRUE);
	XLayoutBox_SetAlignV(hRoot, layout_align_top);
	XLayoutBox_SetSpace(hRoot, 8);
	XEle_EnableBkTransparent(hRoot, TRUE);
	XWidget_LayoutItem_SetWidth(hRoot, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(hRoot, layout_size_auto, 0);

	HXCGUI hDesc = XShapeText_Create(0, 0, 400, 36,
		L"Connect your payment provider to start accepting orders.",
		hRoot);
	if (hDesc){
		HFONTX hFont = XFont_CreateEx(L"Segoe UI", 9, fontStyle_regular);
		if (hFont) XShapeText_SetFont(hDesc, hFont);
		XShapeText_SetTextColor(hDesc, RGBA(107, 114, 128, 255));
		XShapeText_SetTextAlign(hDesc, textAlignFlag_left | textAlignFlag_top);
		XWidget_LayoutItem_SetWidth(hDesc, layout_size_fill, 0);
		XWidget_LayoutItem_SetHeight(hDesc, layout_size_auto, 0);
	}

	HELE hRow = XLayout_Create(0, 0, 400, 32, hRoot);
	if (hRow){
		XLayout_EnableLayout(hRow, TRUE);
		XLayoutBox_EnableHorizon(hRow, TRUE);
		XLayoutBox_SetAlignV(hRow, layout_align_center);
		XLayoutBox_SetSpace(hRow, 8);
		XEle_EnableBkTransparent(hRow, TRUE);
		XWidget_LayoutItem_SetWidth(hRow, layout_size_fill, 0);
		XWidget_LayoutItem_SetHeight(hRow, layout_size_auto, 0);

		HELE hEdit = XEdit_Create(0, 0, 260, 32, L"you@store.com", hRow);
		if (hEdit){
			XWidget_LayoutItem_SetWidth(hEdit, layout_size_weight, 1);
			XWidget_LayoutItem_SetHeight(hEdit, layout_size_fixed, 32);
		}
		HELE hBtn = XBtn_Create(0, 0, 80, 32, L"Connect", hRow);
		if (hBtn){
			XWidget_LayoutItem_SetWidth(hBtn, layout_size_auto, 0);
			XWidget_LayoutItem_SetHeight(hBtn, layout_size_fixed, 32);
		}
	}
	return hRoot;
}

static HELE CreateDownloadPanel()
{
	HELE hRoot = XLayout_Create(0, 0, 420, 10, NULL);
	if (!hRoot) return NULL;
	XLayout_EnableLayout(hRoot, TRUE);
	XLayoutBox_SetAlignV(hRoot, layout_align_top);
	XLayoutBox_SetSpace(hRoot, 10);
	XEle_EnableBkTransparent(hRoot, TRUE);
	XWidget_LayoutItem_SetWidth(hRoot, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(hRoot, layout_size_auto, 0);

	HXCGUI hTitle = XShapeText_Create(0, 0, 400, 24, L"皮肤 sdk_ec 版", hRoot);
	if (hTitle){
		HFONTX hFont = XFont_CreateEx(L"Segoe UI", 12, fontStyle_bold);
		if (hFont) XShapeText_SetFont(hTitle, hFont);
		XShapeText_SetTextColor(hTitle, RGBA(23, 23, 23, 255));
		XShapeText_SetTextAlign(hTitle, textAlignFlag_left | textAlignFlag_top);
		XWidget_LayoutItem_SetWidth(hTitle, layout_size_fill, 0);
		XWidget_LayoutItem_SetHeight(hTitle, layout_size_auto, 0);
	}

	HXCGUI hVer = XShapeText_Create(0, 0, 400, 18, L"版本号: 2.1.0 (2025-5-22)", hRoot);
	if (hVer){
		HFONTX hFont = XFont_CreateEx(L"Segoe UI", 9, fontStyle_regular);
		if (hFont) XShapeText_SetFont(hVer, hFont);
		XShapeText_SetTextColor(hVer, RGBA(107, 114, 128, 255));
		XShapeText_SetTextAlign(hVer, textAlignFlag_left | textAlignFlag_top);
		XWidget_LayoutItem_SetWidth(hVer, layout_size_fill, 0);
		XWidget_LayoutItem_SetHeight(hVer, layout_size_auto, 0);
	}

	HELE hTrack = XLayout_Create(0, 0, 400, 4, hRoot);
	if (hTrack){
		XLayout_EnableLayout(hTrack, FALSE);
		XEle_AddBkFill(hTrack, 0, RGBA(229, 231, 235, 255));
		XWidget_LayoutItem_SetWidth(hTrack, layout_size_fill, 0);
		XWidget_LayoutItem_SetHeight(hTrack, layout_size_fixed, 4);
	}
	return hRoot;
}

static HIMAGE DemoLoadIconImage()
{
	static const char kSvg[] =
		R"SVG(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">)"
		R"SVG(<circle cx="12" cy="12" r="8" fill="#3175F6"/></svg>)SVG";
	HSVG hSvg = XSvg_LoadStringUtf8(kSvg);
	if (!hSvg) return NULL;
	XSvg_SetSize(hSvg, 18, 18);
	HIMAGE hImg = XImage_LoadSvg(hSvg);
	XSvg_Destroy(hSvg);
	return hImg;
}

// =============================================================================
// 3. 全局配置 + Get 系列
// =============================================================================

static void DemoAccordionConfig(CXAccordion& acc)
{
	acc.SetTheme(xuitool_theme_light);
	acc.SetExpandMode(xaccordion_expand_mode_single);
	acc.SetAllowMultipleExpand(FALSE);
	acc.SetAnimEnabled(TRUE);
	acc.SetAnimDuration(220);
	acc.SetIndicatorStyle(xaccordion_indicator_chevron);
	acc.SetCornerRadius(8);
	acc.EnableScroll(TRUE);

	acc.SetOnItemExpand(OnAccItemExpand);
	acc.SetOnItemCollapse(OnAccItemCollapse);
	acc.SetOnItemClick(OnAccItemClick);
	acc.SetOnThemeChanged(OnAccThemeChanged);

	(void)acc.GetTheme();
	(void)acc.GetExpandMode();
	(void)acc.IsAllowMultipleExpand();
	(void)acc.IsValid();
	(void)acc.GetHandle();
}

static void DemoAccordionCustomTheme(CXAccordion& acc)
{
	acc.SetTheme(xuitool_theme_custom);
	acc.SetTextColor(RGB(0xF5, 0xF5, 0xF5));
	acc.SetBkColor(RGB(0x1A, 0x1A, 0x1C));
	acc.SetAccentColor(RGB(0x60, 0xA5, 0xFA));
	acc.SetIndicatorStyle(xaccordion_indicator_text);
	acc.SetTheme(xuitool_theme_light);
}

static void DemoAccordionExpandModes(CXAccordion& acc)
{
	acc.SetExpandMode(xaccordion_expand_mode_multiple);
	acc.SetAllowMultipleExpand(TRUE);
	(void)acc.IsAllowMultipleExpand();

	acc.SetExpandMode(xaccordion_expand_mode_single_global);
	acc.SetAllowMultipleExpand(FALSE);

	acc.SetExpandMode(xaccordion_expand_mode_single);
}

// =============================================================================
// 4. FAQ 单组模式 (样式 A)
// =============================================================================

static void BuildFaqGroup(CXAccordion& acc)
{
	g_faqGroupId = acc.AddGroup();
	if (!g_faqGroupId) return;

	int i1 = acc.AddItem(g_faqGroupId,
		L"How do I update my account information?",
		xaccordion_content_text);
	acc.SetItemIcon(i1, xaccordion_icon_custom);
	acc.SetItemBodyText(i1,
		L"You can update your account information on the Settings page. "
		L"Click your profile picture at the top right corner and select "
		L"\"Settings\" from the dropdown menu.");

	int i2 = acc.AddItem(g_faqGroupId,
		L"What payment methods are accepted?",
		xaccordion_content_text);
	acc.SetItemIcon(i2, xaccordion_icon_none);
	acc.SetItemBodyText(i2,
		L"We accept all major credit and debit cards, PayPal, and bank transfers. "
		L"For enterprise customers, we also support invoicing.");

	int i3 = acc.AddItem(g_faqGroupId,
		L"How do I cancel my subscription?",
		xaccordion_content_text);
	HIMAGE hIcon = DemoLoadIconImage();
	if (hIcon){
		acc.SetItemIconImage(i3, hIcon);
		XImage_Release(hIcon);
	}
	acc.SetItemBodyText(i3,
		L"You can cancel your subscription at any time from the Billing section. "
		L"Your access will continue until the end of the current billing period.");

	acc.SetItemTitle(i1, L"How do I update my account?");
	acc.ExpandItem(i2, FALSE);
	(void)acc.GetItemCount(g_faqGroupId);
	(void)acc.GetExpandedItem(g_faqGroupId);
	(void)acc.IsItemExpanded(i2);
	(void)acc.GetItemHeaderEle(i2);
	(void)acc.GetItemContentHost(i2);
}

// =============================================================================
// 5. 引导清单分组模式 (样式 B)
// =============================================================================

static void BuildSetupGroup(CXAccordion& acc)
{
	g_setupGroupId = acc.AddGroup(L"Set-up your online store");
	if (!g_setupGroupId) return;

	int i1 = acc.AddItem(g_setupGroupId, L"Add products");
	acc.SetItemIcon(i1, xaccordion_icon_status_done);
	acc.SetItemBadge(i1, L"Ready", xaccordion_badge_success);
	acc.SetItemBodyText(i1, L"Start by adding your first product to the catalog.");

	int i2 = acc.AddItem(g_setupGroupId, L"Set up payments");
	acc.SetItemIcon(i2, xaccordion_icon_status_progress);
	acc.SetItemBadge(i2, L"Pending", xaccordion_badge_warning);
	HELE hPay = CreatePaymentPanel();
	if (hPay){
		acc.SetItemContentEle(i2, hPay);
		acc.SetItemContentMinHeight(i2, 72);
	}
	g_payItemId = i2;

	int i3 = acc.AddItem(g_setupGroupId, L"Configure shipping");
	acc.SetItemIcon(i3, xaccordion_icon_status_todo);
	acc.SetItemBadge(i3, L"Optional", xaccordion_badge_info);
	acc.SetItemBodyText(i3, L"Set shipping zones and rates for your store.");

	int i4 = acc.AddItem(g_setupGroupId, L"Verify domain");
	acc.SetItemBadge(i4, L"Action required", xaccordion_badge_danger);
	acc.SetItemBodyText(i4, L"Connect and verify your custom domain before going live.");

	acc.ExpandItem(i2, TRUE);
}

static void BuildUpdateGroup(CXAccordion& acc)
{
	int g = acc.AddGroup(L"组件更新");
	if (!g) return;

	int i1 = acc.AddItem(g, L"抢先版");
	acc.SetItemBadge(i1, L"最新 2.1.0", xaccordion_badge_success);
	acc.SetItemBodyText(i1, L"抢先版已更新至 2.1.0。");

	int i2 = acc.AddItem(g, L"皮肤 sdk_ec 版");
	acc.SetItemBadge(i2, L"下载中", xaccordion_badge_neutral);
	HELE hPanel = CreateDownloadPanel();
	if (hPanel){
		acc.SetItemContentEle(i2, hPanel);
		acc.SetItemContentMinHeight(i2, 80);
	}
	g_ecItemId = i2;

	int i3 = acc.AddItem(g, L"稳定版");
	acc.SetItemBadge(i3, L"最新 2.0.8", xaccordion_badge_success);
	acc.SetItemBodyText(i3, L"稳定版为当前推荐版本。");
}

// =============================================================================
// 6. 动态 API: 禁用 / 删除 / 收起 / 切换
// =============================================================================

static void BuildApiTestGroup(CXAccordion& acc)
{
	g_apiGroupId = acc.AddGroup(L"API 测试组");
	if (!g_apiGroupId) return;

	HIMAGE hIcon = DemoLoadIconImage();
	g_tempItemId = acc.AddItem(g_apiGroupId, L"带图标添加项",
		xaccordion_content_text, hIcon);
	if (hIcon) XImage_Release(hIcon);
	acc.SetItemBodyText(g_tempItemId, L"通过 AddItem(..., HIMAGE) 重载添加。");

	int iDisabled = acc.AddItem(g_apiGroupId, L"禁用项示例");
	acc.SetItemBodyText(iDisabled, L"此项已禁用, 不可展开。");
	acc.SetItemEnabled(iDisabled, FALSE);
	(void)acc.IsItemEnabled(iDisabled);

	int iRemove = acc.AddItem(g_apiGroupId, L"待删除项");
	acc.SetItemBodyText(iRemove, L"稍后 RemoveItem 删除。");
	acc.RemoveItem(iRemove);

	int gRemove = acc.AddGroup(L"待删除组");
	if (gRemove){
		acc.AddItem(gRemove, L"组内项");
		acc.RemoveGroup(gRemove);
	}

	acc.SetGroupTitle(g_apiGroupId, L"API 测试组 (已改名)");
	(void)acc.IsGroupEnabled(g_apiGroupId);
}

static void DemoAccordionOperations(CXAccordion& acc)
{
	if (g_payItemId){
		acc.ToggleItem(g_payItemId, TRUE);
		acc.ExpandItem(g_payItemId, TRUE);
		acc.CollapseItem(g_payItemId, TRUE);
		acc.ExpandItem(g_payItemId, FALSE);
	}

	acc.CollapseAll(g_faqGroupId);
	if (g_ecItemId)
		acc.ExpandItem(g_ecItemId, FALSE);

	if (g_apiGroupId){
		acc.SetGroupEnabled(g_apiGroupId, FALSE);
		acc.SetGroupEnabled(g_apiGroupId, TRUE);
	}

	if (g_tempItemId){
		acc.ClearItemContent(g_tempItemId);
		acc.SetItemBodyText(g_tempItemId, L"ClearItemContent 后重新设置正文。");
	}

	DemoAccordionCustomTheme(acc);
	DemoAccordionExpandModes(acc);

	acc.SetAnimEnabled(FALSE);
	acc.CollapseAll(0);
	acc.SetAnimEnabled(TRUE);

	(void)acc.GetGroupCount();
}

static void DemoAccordionLifecycle(HWINDOW hWnd)
{
	CXAccordion tmp;
	tmp.Create(hWnd);
	int g = tmp.AddGroup(L"临时组");
	if (g){
		tmp.AddItem(g, L"临时项");
		tmp.ClearGroups();
	}
	tmp.DestroyAccordion();
	(void)tmp.IsValid();
}

// =============================================================================
// 7. 入口
// =============================================================================

void BuildAccordionDemo(HWINDOW hWnd)
{
	g_hWnd = hWnd;

	DemoAccordionLifecycle(hWnd);

	g_acc = new CXAccordion();
	g_acc->Create(hWnd);

	DemoAccordionConfig(*g_acc);
	BuildFaqGroup(*g_acc);
	BuildSetupGroup(*g_acc);
	BuildUpdateGroup(*g_acc);
	BuildApiTestGroup(*g_acc);
	DemoAccordionOperations(*g_acc);

	g_acc->AdjustLayout();
	if (g_ecItemId)
		g_acc->ExpandItem(g_ecItemId, FALSE);
	g_acc->AdjustLayout();
}

void DestroyAccordionDemo()
{
	if (g_acc){
		if (g_acc->IsValid())
			g_acc->DestroyAccordion();
		delete g_acc;
		g_acc = NULL;
	}
	g_faqGroupId = g_setupGroupId = g_apiGroupId = 0;
	g_payItemId = g_ecItemId = g_tempItemId = 0;
	g_hWnd = NULL;
}
