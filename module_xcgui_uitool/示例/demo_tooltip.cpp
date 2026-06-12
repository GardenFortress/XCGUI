// CXTooltip 完整接口使用示例.
// 覆盖 module_xcgui_uitool.h 中 CXTooltip 全部静态 public 方法.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建后调用 BuildTooltipDemo(hWnd).

#include "module_xcgui_uitool.h"

static HELE g_hBtnDefault  = NULL;
static HELE g_hBtnSuccess  = NULL;
static HELE g_hBtnInfo     = NULL;
static HELE g_hBtnWarning  = NULL;
static HELE g_hBtnError    = NULL;
static HELE g_hBtnMulti    = NULL;
static HELE g_hBtnCustom   = NULL;

// =============================================================================
// 1. 注册 / 样式配置
// =============================================================================

static void SetupEleTip(HELE hEle, const wchar_t* pText, xtooltip_type_ type)
{
	CXTooltip::AddEleTip(hEle, pText);
	CXTooltip::SetType(hEle, type);
}

static void SetupTooltipStyles()
{
	// --- 默认单行 ---
	SetupEleTip(g_hBtnDefault, L"默认提示: 鼠标悬停显示", xtooltip_type_default);

	// --- 语义图标 (success / info / warning / error) ---
	SetupEleTip(g_hBtnSuccess, L"操作成功", xtooltip_type_success);
	SetupEleTip(g_hBtnInfo,    L"这是一条信息", xtooltip_type_info);
	SetupEleTip(g_hBtnWarning, L"请注意风险", xtooltip_type_warning);
	SetupEleTip(g_hBtnError,   L"发生错误", xtooltip_type_error);

	// --- 多行 + 对齐 + 箭头 ---
	CXTooltip::AddEleTip(g_hBtnMulti, L"多行提示:\n第一行说明\n第二行补充");
	CXTooltip::SetMultiline(g_hBtnMulti, TRUE);
	CXTooltip::SetAlignH(g_hBtnMulti, xtooltip_align_h_left);
	CXTooltip::SetAlignV(g_hBtnMulti, xtooltip_align_v_top);
	CXTooltip::SetArrowSide(g_hBtnMulti, xtooltip_arrow_side_bottom);
	CXTooltip::SetShowDelay(g_hBtnMulti, 300);
	CXTooltip::SetAutoCloseTime(g_hBtnMulti, 5000);
	CXTooltip::SetFadeDuration(g_hBtnMulti, 150);

	// --- 自定义主题 / 颜色 / 边距 ---
	CXTooltip::AddEleTip(g_hBtnCustom, L"自定义深色气泡");
	CXTooltip::SetTheme(g_hBtnCustom, xuitool_theme_custom);
	CXTooltip::SetTextColor(g_hBtnCustom, RGB(0xFF, 0xE0, 0xB2));
	CXTooltip::SetBkColor(g_hBtnCustom, RGB(0x1A, 0x23, 0x32));
	CXTooltip::SetMargin(g_hBtnCustom, 20, 12, 20, 12);
	CXTooltip::SetArrowSide(g_hBtnCustom, xtooltip_arrow_side_right);
	CXTooltip::SetShowArrow(g_hBtnCustom, TRUE);
}

// =============================================================================
// 2. Get 系列 + 动态修改 / 注销
// =============================================================================

static void DemoTooltipGetAndModify()
{
	HELE h = g_hBtnDefault;
	if (!CXTooltip::HasTip(h)) return;

	(void)CXTooltip::GetText(h);
	(void)CXTooltip::GetType(h);
	(void)CXTooltip::IsMultiline(h);
	(void)CXTooltip::GetAlignH(h);
	(void)CXTooltip::GetAlignV(h);
	(void)CXTooltip::GetArrowSide(h);
	(void)CXTooltip::GetShowArrow(h);
	(void)CXTooltip::GetTheme(h);
	(void)CXTooltip::GetTextColor(h);
	(void)CXTooltip::GetBkColor(h);
	int l = 0, t = 0, r = 0, b = 0;
	CXTooltip::GetMargin(h, &l, &t, &r, &b);
	(void)CXTooltip::GetShowDelay(h);
	(void)CXTooltip::GetAutoCloseTime(h);
	(void)CXTooltip::GetFadeDuration(h);

	CXTooltip::SetText(h, L"文本已更新 (SetText)");
	CXTooltip::SetTheme(h, xuitool_theme_auto);
	CXTooltip::SetShowArrow(h, FALSE);

	// 演示注销后重新注册
	// CXTooltip::DelEleTip(h);
	// CXTooltip::AddEleTip(h, L"重新注册");
}

// =============================================================================
// 3. 入口
// =============================================================================

void BuildTooltipDemo(HXCGUI hWnd)
{
	g_hBtnDefault = XBtn_Create(20,  20, 100, 32, L"默认",   hWnd);
	g_hBtnSuccess = XBtn_Create(130, 20, 100, 32, L"成功",   hWnd);
	g_hBtnInfo    = XBtn_Create(240, 20, 100, 32, L"信息",   hWnd);
	g_hBtnWarning = XBtn_Create(350, 20, 100, 32, L"警告",   hWnd);
	g_hBtnError   = XBtn_Create(20,  70, 100, 32, L"错误",   hWnd);
	g_hBtnMulti   = XBtn_Create(130, 70, 160, 32, L"多行提示", hWnd);
	g_hBtnCustom  = XBtn_Create(300, 70, 160, 32, L"自定义",  hWnd);

	SetupTooltipStyles();
	DemoTooltipGetAndModify();

	// 进程退出前调用: CXTooltip::Cleanup();
}
