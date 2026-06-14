//============================================================================
// module_xcgui_uitool_calendar.cpp — CXCalendarCard 实现
//============================================================================
//============================================================================
// CXCalendarCard — 日期 / 日期范围选择卡片
//============================================================================

namespace {

constexpr int kCal_FieldYear   = 0;
constexpr int kCal_FieldMonth  = 1;
constexpr int kCal_FieldDay    = 2;
constexpr int kCal_FieldHour   = 3;
constexpr int kCal_FieldMinute = 4;
constexpr int kCal_FieldSecond = 5;
constexpr int kCal_FieldNone   = -1;

constexpr int kCal_ResultCancel = 0;
constexpr int kCal_ResultOk     = 1;

constexpr int kCal_CornerRadius = _XUITool::kCornerRadius;
constexpr int kCal_PadH       = 24;
constexpr int kCal_PadV       = 20;
constexpr int kCal_SepGap     = 16;
constexpr int kCal_ColW       = 40;
constexpr int kCal_NavSize    = 28;
constexpr int kCal_NavSpace   = 6;
constexpr int kCal_WeekH      = 30;
constexpr int kCal_GridRows   = 6;
constexpr int kCal_GridH      = kCal_WeekH + kCal_GridRows * kCal_ColW;
constexpr int kCal_PanelW     = 7 * kCal_ColW;
constexpr int kCal_PanelGap   = 40;
constexpr int kCal_ActionH    = 32;
constexpr int kCal_QuickH     = 28;
constexpr int kCal_QuickW     = 64;
constexpr int kCal_QuickGap   = 6;
constexpr int kCal_EditH      = 32;
constexpr int kCal_EditW      = 168;
constexpr int kCal_EditGap    = 12;
constexpr int kCal_CancelW    = 76;
constexpr int kCal_ConfirmW   = 88;
constexpr int kCal_BtnGap     = 12;

enum _XCal_PopupMode
{
	_XCal_PopupMode_Default = 0,
	_XCal_PopupMode_Ele,
	_XCal_PopupMode_Pos,
};

enum _XCal_Mode
{
	_XCal_Mode_Single = 0,
	_XCal_Mode_Range  = 1,
};

enum _XCal_Cmd
{
	_XCal_Cmd_None = 0,
	_XCal_Cmd_DateCell,
	_XCal_Cmd_PrevYear,
	_XCal_Cmd_PrevMonth,
	_XCal_Cmd_NextMonth,
	_XCal_Cmd_NextYear,
	_XCal_Cmd_Today,
	_XCal_Cmd_Last7,
	_XCal_Cmd_Last15,
	_XCal_Cmd_Last30,
	_XCal_Cmd_Confirm,
	_XCal_Cmd_Cancel,
	_XCal_Cmd_StartSpinUp,
	_XCal_Cmd_StartSpinDown,
	_XCal_Cmd_EndSpinUp,
	_XCal_Cmd_EndSpinDown,
};

struct _XCal_ThemeColors
{
	COLORREF wndBg;
	COLORREF text;
	COLORREF mutedText;
	COLORREF title;
	COLORREF weekText;
	COLORREF border;
	COLORREF cellBg;
	COLORREF cellHover;
	COLORREF cellDisabledBg;
	COLORREF cellDisabledText;
	COLORREF accent;
	COLORREF accentSoft;
	COLORREF accentText;
	COLORREF rangeText;
	COLORREF inputBg;
	COLORREF inputBorder;
	COLORREF buttonBg;
	COLORREF buttonHover;
	COLORREF buttonBorder;
	COLORREF cancelBg;
	const wchar_t* bkPrimary;
	const wchar_t* bkNormal;
	const wchar_t* bkRange;
	const wchar_t* bkSubtle;
	const wchar_t* bkInput;
	const wchar_t* bkSpin;
	COLORREF separator;
};

struct _XCal_Ctx
{
	_XCal_Mode mode = _XCal_Mode_Range;
	BOOL doubleMonth = TRUE;
	xuitool_theme_ theme = xuitool_theme_auto;
	_XCal_ThemeColors colors{};

	HWINDOW hWnd = NULL;
	HWND hParent = NULL;
	int contentW = 0;
	int contentH = 0;
	int contentOffX = _XUITool::kShadowMargin;
	int contentOffY = _XUITool::kShadowMargin;
	int cornerRadius = kCal_CornerRadius;
	int sepHeaderY = -1;
	int sepFooterY = -1;

	HELE hBtnPrevYear = NULL;
	HELE hBtnPrevMonth = NULL;
	HELE hBtnNextMonth = NULL;
	HELE hBtnNextYear = NULL;
	HELE hBtnConfirm = NULL;
	HELE hBtnCancel = NULL;
	HELE hActionWrap = NULL;
	HELE hBtnToday = NULL;
	HELE hBtnLast7 = NULL;
	HELE hBtnLast15 = NULL;
	HELE hBtnLast30 = NULL;
	HELE hEditStart = NULL;
	HELE hEditEnd = NULL;
	HELE hEditWrapStart = NULL;
	HELE hEditWrapEnd = NULL;
	HELE hSpinWrapStart = NULL;
	HELE hSpinWrapEnd = NULL;
	HELE hSpinStartUp = NULL;
	HELE hSpinStartDown = NULL;
	HELE hSpinEndUp = NULL;
	HELE hSpinEndDown = NULL;

	HXCGUI hTitleLeft = NULL;
	HXCGUI hTitleRight = NULL;
	HXCGUI hRangeText = NULL;
	HXCGUI hWeekLabels[14]{};
	HELE hCellsLeft[42]{};
	HELE hCellsRight[42]{};

	HFONTX hFont = NULL;
	HFONTX hFontSmall = NULL;
	HSVG hSvgLastYear = NULL;
	HSVG hSvgNextYear = NULL;
	HSVG hSvgLastMonth = NULL;
	HSVG hSvgNextMonth = NULL;
	HSVG hSvgUp = NULL;
	HSVG hSvgDown = NULL;
	HSVG hSvgLastYearDis = NULL;
	HSVG hSvgNextYearDis = NULL;
	HSVG hSvgLastMonthDis = NULL;
	HSVG hSvgNextMonthDis = NULL;
	HSVG hSvgUpDis = NULL;
	HSVG hSvgDownDis = NULL;
	HIMAGE hImgLastYear = NULL;
	HIMAGE hImgNextYear = NULL;
	HIMAGE hImgLastMonth = NULL;
	HIMAGE hImgNextMonth = NULL;
	HIMAGE hImgUp = NULL;
	HIMAGE hImgDown = NULL;
	HIMAGE hImgLastYearDis = NULL;
	HIMAGE hImgNextYearDis = NULL;
	HIMAGE hImgLastMonthDis = NULL;
	HIMAGE hImgNextMonthDis = NULL;
	HIMAGE hImgUpDis = NULL;
	HIMAGE hImgDownDis = NULL;

	xcalendar_datetime_ today{};
	xcalendar_datetime_ showDate{};
	xcalendar_datetime_ selStart{};
	xcalendar_datetime_ selEnd{};
	xcalendar_datetime_ resultStart{};
	xcalendar_datetime_ resultEnd{};
	BOOL limitMaxDate = TRUE;
	xcalendar_datetime_ maxDate{};
	BOOL confirmed = FALSE;
	BOOL closing = FALSE;
	BOOL destroyed = FALSE;
	BOOL updatingUI = FALSE;
	int focusFieldStart = kCal_FieldSecond;
	int focusFieldEnd = kCal_FieldSecond;
};

struct _XCal_Global
{
	std::unordered_map<HWINDOW, _XCal_Ctx*> windows;
	std::unordered_map<HELE, _XCal_Ctx*> elements;
	std::unordered_map<HELE, _XCal_Cmd> commands;
	_XCal_PopupMode popupMode = _XCal_PopupMode_Default;
	HELE bindEle = NULL;
	int bindOffsetX = 0;
	int bindOffsetY = 0;
	POINT popupPt{0, 0};
};

_XCal_Global& CG()
{
	static _XCal_Global s;
	return s;
}

inline int _XCal_ClampInt(int v, int lo, int hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

void _XCal_ResolveTheme(xuitool_theme_ theme, _XCal_ThemeColors* c)
{
	BOOL light = _XUITool::IsLightTheme(theme);
	static const wchar_t* kBkPrimaryDark =
		L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4293878553)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4293947431)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294016057)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(1720205852)23(102)9(4,4,4,4);8:1(16)5(4294967295);8:1(32)5(4294967295);8:1(64)5(4294967295);8:1(2)5(2146299373);}";
	static const wchar_t* kBkPrimaryLight =
		L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4294734118)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4293418527)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4292693275)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(1727820070)23(102)9(4,4,4,4);8:1(16)5(4294967295);8:1(32)5(4294967295);8:1(64)5(4294967295);8:1(2)5(4294967295);}";
	static const wchar_t* kBkNormalDark =
		L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4281413937)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4282006074)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4282598211)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(1513764405)23(90)9(4,4,4,4);8:1(16)5(4292927712);8:1(32)5(4293190368);8:1(64)5(4292532695);8:1(2)5(4285558124);}";
	static const wchar_t* kBkNormalLight =
		L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4294177263)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4293716711)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4293321696)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(3455514099)23(205)9(4,4,4,4);8:1(16)5(4280229663);8:1(32)5(4280229663);8:1(64)5(4280229663);8:1(2)5(4290558905);}";
	static const wchar_t* kBkRangeDark =
		L"{99:1.9.9;98:16(0)32(1)64(2)2(0);5:2(15)20(1)21(3)26(1)22(4282920738)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4283380772)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4283709222)23(255)9(4,4,4,4);8:1(16)5(4292927712);8:1(32)5(4293190368);8:1(64)5(4292532695);8:1(2)5(4285558124);}";
	static const wchar_t* kBkRangeLight =
		L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4293058782)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4292729816)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4292400595)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294374899)23(255)9(4,4,4,4);8:1(16)5(4280229663);8:1(32)5(4280229663);8:1(64)5(4280229663);8:1(2)5(4290558905);}";
	static const wchar_t* kBkSubtleDark =
		L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4279505939)23(255)10(1)7(1)11(3)16(1)12(4281545523)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4281084714)23(255)10(1)7(1)11(3)16(1)12(4282137402)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4281742644)23(255)10(1)7(1)11(3)16(1)12(4282597953)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4279505939)23(255)10(1)7(1)11(3)16(1)12(4280163870)13(255)9(4,4,4,4);8:1(16)5(4290032820);8:1(32)5(4294967295);8:1(64)5(4294967295);8:1(2)5(4283979607);}";
	static const wchar_t* kBkSubtleLight =
		L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4294769916)23(255)10(1)7(1)11(3)16(1)12(4294111729)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294111986)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294111470)23(255)10(1)7(1)11(3)16(1)12(4293059039)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294967295)23(255)10(1)7(1)11(3)16(1)12(3370904555)13(200)9(4,4,4,4);8:1(16)5(4284243036);8:1(32)5(4280229663);8:1(64)5(4280229663);8:1(2)5(4287532686);}";
	static const wchar_t* kBkInputDark =
		L"{99:1.9.9;98:24(0)20(4)40(1)36(2)2(3);5:2(15)10(1)7(1)11(3)16(1)12(4281545523)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4280032284)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4279505939)23(255)10(1)7(1)11(3)16(1)12(4293878553)13(255)9(4,4,4,4);5:2(15)10(1)7(1)11(3)16(1)12(4280690214)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4279505939)23(255)10(1)7(1)11(3)16(1)12(4288914339)13(255)9(4,4,4,4);}";
	static const wchar_t* kBkInputLight =
		L"{99:1.9.9;98:24(0)20(4)40(1)36(2)2(3);5:2(15)10(1)7(1)11(3)16(1)12(4293651435)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294440951)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294967295)23(255)10(1)7(1)11(3)16(1)12(4294734118)13(255)9(4,4,4,4);5:2(15)10(1)7(1)11(3)16(1)12(4293651435)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294967295)23(255)10(1)7(1)11(3)16(1)12(4279703319)13(255)9(4,4,4,4);}";
	static const wchar_t* kBkSpinLight =
		L"{99:1.9.9;98:16()32(0)64(1)2();5:2(15)20(1)21(3)26(1)22(251658240)23(15)9(2,2,2,2);5:2(15)20(1)21(3)26(1)22(419430400)23(25)9(2,2,2,2);}";
	static const wchar_t* kBkSpinDark =
		L"{99:1.9.9;98:16()32(0)64(1)2();5:2(15)20(1)21(3)26(1)22(268435455)23(15)9(2,2,2,2);5:2(15)20(1)21(3)26(1)22(436207615)23(25)9(2,2,2,2);}";
	if (light){
		c->wndBg            = _XUITool::kLightBg;
		c->text             = _XUITool::kLightText;
		c->mutedText        = _XUITool::WithAlpha(_XUITool::kLightText, 150);
		c->title            = RGBA(0x26, 0x71, 0xFC, 0xFF);
		c->weekText         = RGBA(0x6F, 0x6F, 0x6F, 0xFF);
		c->border           = _XUITool::WithAlpha(_XUITool::kLightText, 38);
		c->cellBg           = _XUITool::WithAlpha(_XUITool::kLightText, 14);
		c->cellHover        = _XUITool::WithAlpha(_XUITool::kLightText, 24);
		c->cellDisabledBg   = _XUITool::WithAlpha(_XUITool::kLightText, 8);
		c->cellDisabledText = _XUITool::WithAlpha(_XUITool::kLightText, 70);
		c->accent           = _XUITool::kLightText;
		c->accentSoft       = _XUITool::WithAlpha(_XUITool::kLightText, 32);
		c->accentText       = RGBA(0xFF, 0xFF, 0xFF, 0xFF);
		c->rangeText        = RGBA(0x17, 0x17, 0x17, 0xFF);
		c->inputBg          = _XUITool::kLightBg;
		c->inputBorder      = _XUITool::WithAlpha(_XUITool::kLightText, 58);
		c->buttonBg         = _XUITool::kLightBg;
		c->buttonHover      = _XUITool::WithAlpha(_XUITool::kLightText, 18);
		c->buttonBorder     = _XUITool::WithAlpha(_XUITool::kLightText, 58);
		c->cancelBg         = _XUITool::WithAlpha(_XUITool::kLightText, 14);
		c->bkPrimary        = kBkPrimaryLight;
		c->bkNormal         = kBkNormalLight;
		c->bkRange          = kBkRangeLight;
		c->bkSubtle         = kBkSubtleLight;
		c->bkInput          = kBkInputLight;
		c->bkSpin           = kBkSpinLight;
		c->separator        = _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 15); // 6% 黑
		return;
	}

	c->wndBg            = _XUITool::kDarkBg;
	c->text             = _XUITool::kDarkText;
	c->mutedText        = _XUITool::WithAlpha(_XUITool::kDarkText, 145);
	c->title            = RGBA(0x19, 0x63, 0xEF, 0xFF);
	c->weekText         = RGBA(0x7D, 0x7E, 0x7F, 0xFF);
	c->border           = _XUITool::WithAlpha(_XUITool::kDarkText, 40);
	c->cellBg           = _XUITool::WithAlpha(_XUITool::kDarkText, 18);
	c->cellHover        = _XUITool::WithAlpha(_XUITool::kDarkText, 32);
	c->cellDisabledBg   = _XUITool::WithAlpha(_XUITool::kDarkText, 10);
	c->cellDisabledText = _XUITool::WithAlpha(_XUITool::kDarkText, 68);
	c->accent           = _XUITool::kDarkText;
	c->accentSoft       = _XUITool::WithAlpha(_XUITool::kDarkText, 38);
	c->accentText       = RGBA(0xFF, 0xFF, 0xFF, 0xFF);
	c->rangeText        = RGBA(0xF5, 0xF5, 0xF5, 0xFF);
	c->inputBg          = _XUITool::WithAlpha(_XUITool::kDarkText, 10);
	c->inputBorder      = _XUITool::WithAlpha(_XUITool::kDarkText, 64);
	c->buttonBg         = _XUITool::WithAlpha(_XUITool::kDarkText, 16);
	c->buttonHover      = _XUITool::WithAlpha(_XUITool::kDarkText, 28);
	c->buttonBorder     = _XUITool::WithAlpha(_XUITool::kDarkText, 58);
	c->cancelBg         = _XUITool::WithAlpha(_XUITool::kDarkText, 12);
	c->bkPrimary        = kBkPrimaryDark;
	c->bkNormal         = kBkNormalDark;
	c->bkRange          = kBkRangeDark;
	c->bkSubtle         = kBkSubtleDark;
	c->bkInput          = kBkInputDark;
	c->bkSpin           = kBkSpinDark;
	c->separator        = _XUITool::WithAlpha(RGBA(255, 255, 255, 255), 20); // 8% 白
}

void _XCal_StyleButton(HELE hEle, const _XCal_ThemeColors& c, BOOL primary = FALSE, BOOL flatCancel = FALSE)
{
	if (!hEle) return;
	XEle_ClearBkInfo(hEle);
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_SetBkInfo(hEle, primary ? c.bkPrimary : (flatCancel ? c.bkSubtle : c.bkNormal));
	XEle_SetTextColor(hEle, primary ? c.accentText : c.text);
}

void _XCal_StyleInput(HELE hEle, const _XCal_ThemeColors& c)
{
	if (!hEle) return;
	XEle_ClearBkInfo(hEle);
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_SetBkInfo(hEle, c.bkInput);
	XEle_SetBorderSize(hEle, 4, 0, 24, 0);
	XEle_SetTextColor(hEle, c.text);
	XEdit_SetTextAlign(hEle, edit_textAlign_flag_left | edit_textAlign_flag_center_v);
}

void _XCal_StyleSpinButton(HELE hEle, const _XCal_ThemeColors& c)
{
	if (!hEle) return;
	XEle_ClearBkInfo(hEle);
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_SetBkInfo(hEle, c.bkSpin);
	XEle_SetTextColor(hEle, c.text);
}

void _XCal_SetSeparator(_XCal_Ctx* ctx, int index, int y)
{
	if (!ctx) return;
	if (index == 0) ctx->sepHeaderY = y;
	else ctx->sepFooterY = y;
}

BOOL _XCal_IsLeapYear(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int _XCal_DaysInMonth(int year, int month)
{
	static const int days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (month < 1) month = 1;
	if (month > 12) month = 12;
	if (month == 2 && _XCal_IsLeapYear(year)) return 29;
	return days[month];
}

int _XCal_WeekDay(int y, int m, int d)
{
	if (m == 1 || m == 2){
		m += 12;
		--y;
	}
	return (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;
}

int _XCal_StartIndex(int year, int month)
{
	int wd = _XCal_WeekDay(year, month, 1);
	return (wd == 0) ? 6 : (wd - 1);
}

xcalendar_datetime_ _XCal_Normalize(xcalendar_datetime_ d)
{
	if (d.year < 1) d.year = 1;
	if (d.year > 9999) d.year = 9999;
	d.month = _XCal_ClampInt(d.month, 1, 12);
	d.day = _XCal_ClampInt(d.day, 1, _XCal_DaysInMonth(d.year, d.month));
	d.hour = _XCal_ClampInt(d.hour, 0, 23);
	d.minute = _XCal_ClampInt(d.minute, 0, 59);
	d.second = _XCal_ClampInt(d.second, 0, 59);
	return d;
}

xcalendar_datetime_ _XCal_Current()
{
	SYSTEMTIME st{};
	::GetLocalTime(&st);
	xcalendar_datetime_ d{};
	d.year = st.wYear;
	d.month = st.wMonth;
	d.day = st.wDay;
	d.hour = st.wHour;
	d.minute = st.wMinute;
	d.second = st.wSecond;
	return _XCal_Normalize(d);
}

int _XCal_CompareDate(const xcalendar_datetime_& a, const xcalendar_datetime_& b)
{
	if (a.year != b.year) return (a.year > b.year) ? 1 : -1;
	if (a.month != b.month) return (a.month > b.month) ? 1 : -1;
	if (a.day != b.day) return (a.day > b.day) ? 1 : -1;
	return 0;
}

int _XCal_CompareFull(const xcalendar_datetime_& a, const xcalendar_datetime_& b)
{
	int c = _XCal_CompareDate(a, b);
	if (c != 0) return c;
	if (a.hour != b.hour) return (a.hour > b.hour) ? 1 : -1;
	if (a.minute != b.minute) return (a.minute > b.minute) ? 1 : -1;
	if (a.second != b.second) return (a.second > b.second) ? 1 : -1;
	return 0;
}

BOOL _XCal_IsDateField(int field)
{
	return field == kCal_FieldYear || field == kCal_FieldMonth || field == kCal_FieldDay;
}

xcalendar_datetime_ _XCal_AddMonths(xcalendar_datetime_ d, int delta);

BOOL _XCal_IsAfterMaxDate(const _XCal_Ctx* ctx, const xcalendar_datetime_& d)
{
	return ctx && ctx->limitMaxDate && _XCal_CompareDate(d, ctx->maxDate) > 0;
}

xcalendar_datetime_ _XCal_ClampMaxDate(const _XCal_Ctx* ctx, xcalendar_datetime_ d, BOOL endOfDay)
{
	d = _XCal_Normalize(d);
	if (_XCal_IsAfterMaxDate(ctx, d)){
		int h = endOfDay ? 23 : d.hour;
		int m = endOfDay ? 59 : d.minute;
		int s = endOfDay ? 59 : d.second;
		d = ctx->maxDate;
		d.hour = h;
		d.minute = m;
		d.second = s;
		d = _XCal_Normalize(d);
	}
	return d;
}

BOOL _XCal_CanMoveToMonth(const _XCal_Ctx* ctx, xcalendar_datetime_ showDate)
{
	if (!ctx || !ctx->limitMaxDate) return TRUE;
	if (ctx->doubleMonth && ctx->mode == _XCal_Mode_Range){
		xcalendar_datetime_ leftDate = _XCal_AddMonths(showDate, -1);
		if (leftDate.year > ctx->maxDate.year) return FALSE;
		if (leftDate.year == ctx->maxDate.year && leftDate.month > ctx->maxDate.month) return FALSE;
		return TRUE;
	}
	if (showDate.year > ctx->maxDate.year) return FALSE;
	if (showDate.year == ctx->maxDate.year && showDate.month > ctx->maxDate.month) return FALSE;
	return TRUE;
}

xcalendar_datetime_ _XCal_DateFromIndex(int index, int year, int month)
{
	int start = _XCal_StartIndex(year, month);
	int daysIn = _XCal_DaysInMonth(year, month);
	xcalendar_datetime_ d{};
	if (index < start){
		d.year = (month == 1) ? year - 1 : year;
		d.month = (month == 1) ? 12 : month - 1;
		d.day = _XCal_DaysInMonth(d.year, d.month) - (start - index - 1);
	} else if (index < start + daysIn){
		d.year = year;
		d.month = month;
		d.day = index - start + 1;
	} else {
		d.year = (month == 12) ? year + 1 : year;
		d.month = (month == 12) ? 1 : month + 1;
		d.day = index - (start + daysIn) + 1;
	}
	d.hour = 0;
	d.minute = 0;
	d.second = 0;
	return _XCal_Normalize(d);
}

xcalendar_datetime_ _XCal_AddMonths(xcalendar_datetime_ d, int delta)
{
	int month0 = (d.year * 12 + (d.month - 1)) + delta;
	if (month0 < 12) month0 = 12;
	d.year = month0 / 12;
	d.month = month0 % 12 + 1;
	int maxDay = _XCal_DaysInMonth(d.year, d.month);
	if (d.day > maxDay) d.day = maxDay;
	return _XCal_Normalize(d);
}

xcalendar_datetime_ _XCal_AddDays(xcalendar_datetime_ d, int delta)
{
	d = _XCal_Normalize(d);
	if (delta > 0){
		while (delta-- > 0){
			++d.day;
			int maxDay = _XCal_DaysInMonth(d.year, d.month);
			if (d.day > maxDay){
				d.day = 1;
				++d.month;
				if (d.month > 12){
					d.month = 1;
					++d.year;
				}
			}
		}
	} else {
		while (delta++ < 0){
			--d.day;
			if (d.day < 1){
				--d.month;
				if (d.month < 1){
					d.month = 12;
					--d.year;
				}
				d.day = _XCal_DaysInMonth(d.year, d.month);
			}
		}
	}
	return _XCal_Normalize(d);
}

CXText _XCal_Pad2(int n)
{
	wchar_t buf[8]{};
	swprintf_s(buf, _countof(buf), L"%02d", n);
	CXText text;
	text = buf;
	return text;
}

CXText _XCal_FormatDateTime(const xcalendar_datetime_& d)
{
	wchar_t buf[32]{};
	swprintf_s(buf, _countof(buf), L"%04d-%02d-%02d %02d:%02d:%02d",
		d.year, d.month, d.day, d.hour, d.minute, d.second);
	CXText text;
	text = buf;
	return text;
}

CXText _XCal_FormatShortDate(const xcalendar_datetime_& d)
{
	wchar_t buf[16]{};
	swprintf_s(buf, _countof(buf), L"%02d/%02d/%02d", d.year % 100, d.month, d.day);
	CXText text;
	text = buf;
	return text;
}

CXText _XCal_FormatRangeLabel(const xcalendar_datetime_& s, const xcalendar_datetime_& e)
{
	wchar_t buf[96]{};
	swprintf_s(buf, _countof(buf), L"%04d.%02d.%02d %02d:%02d:%02d -- %04d.%02d.%02d %02d:%02d:%02d",
		s.year, s.month, s.day, s.hour, s.minute, s.second,
		e.year, e.month, e.day, e.hour, e.minute, e.second);
	CXText text;
	text = buf;
	return text;
}

xcalendar_datetime_ _XCal_ParseDateTime(const wchar_t* text, const xcalendar_datetime_& fallback)
{
	if (!text) return fallback;
	xcalendar_datetime_ d = fallback;
	int y = 0, mo = 0, da = 0, h = 0, mi = 0, se = 0;
	if (swscanf_s(text, L"%d-%d-%d %d:%d:%d", &y, &mo, &da, &h, &mi, &se) >= 3){
		d.year = y;
		d.month = mo;
		d.day = da;
		d.hour = h;
		d.minute = mi;
		d.second = se;
		return _XCal_Normalize(d);
	}
	if (swscanf_s(text, L"%d/%d/%d", &y, &mo, &da) >= 3){
		if (y < 100) y += (y >= 70) ? 1900 : 2000;
		d.year = y;
		d.month = mo;
		d.day = da;
		return _XCal_Normalize(d);
	}
	return fallback;
}

int _XCal_FieldFromPos(int pos)
{
	if (pos >= 0 && pos <= 4) return kCal_FieldYear;
	if (pos >= 5 && pos <= 7) return kCal_FieldMonth;
	if (pos >= 8 && pos <= 10) return kCal_FieldDay;
	if (pos >= 11 && pos <= 13) return kCal_FieldHour;
	if (pos >= 14 && pos <= 16) return kCal_FieldMinute;
	if (pos >= 17 && pos <= 19) return kCal_FieldSecond;
	return kCal_FieldNone;
}

void _XCal_SelectField(HELE hEdit, int field)
{
	if (!hEdit) return;
	switch (field){
	case kCal_FieldYear:   XEdit_SetSelect(hEdit, 0, 0,  0, 4);  break;
	case kCal_FieldMonth:  XEdit_SetSelect(hEdit, 0, 5,  0, 7);  break;
	case kCal_FieldDay:    XEdit_SetSelect(hEdit, 0, 8,  0, 10); break;
	case kCal_FieldHour:   XEdit_SetSelect(hEdit, 0, 11, 0, 13); break;
	case kCal_FieldMinute: XEdit_SetSelect(hEdit, 0, 14, 0, 16); break;
	case kCal_FieldSecond: XEdit_SetSelect(hEdit, 0, 17, 0, 19); break;
	default: break;
	}
}

xcalendar_datetime_ _XCal_AdjustField(xcalendar_datetime_ d, int field, int delta)
{
	switch (field){
	case kCal_FieldYear:
		d.year += delta;
		break;
	case kCal_FieldMonth:
		d.month += delta;
		while (d.month > 12){ d.month -= 12; ++d.year; }
		while (d.month < 1) { d.month += 12; --d.year; }
		break;
	case kCal_FieldDay: {
		d.day += delta;
		int maxDay = _XCal_DaysInMonth(d.year, d.month);
		if (d.day > maxDay) d.day = 1;
		if (d.day < 1) d.day = maxDay;
		break;
	}
	case kCal_FieldHour:
		d.hour = (d.hour + delta + 24) % 24;
		break;
	case kCal_FieldMinute:
		d.minute = (d.minute + delta + 60) % 60;
		break;
	case kCal_FieldSecond:
	default:
		d.second = (d.second + delta + 60) % 60;
		break;
	}
	return _XCal_Normalize(d);
}

int _XCal_DateUserData(const xcalendar_datetime_& d, int panelIndex)
{
	int val = d.year * 10000 + d.month * 100 + d.day;
	return val * 10 + panelIndex;
}

xcalendar_datetime_ _XCal_DateFromUserData(vint raw)
{
	int data = (int)raw;
	int val = data / 10;
	xcalendar_datetime_ d{};
	d.year = val / 10000;
	d.month = (val % 10000) / 100;
	d.day = val % 100;
	return _XCal_Normalize(d);
}

_XCal_Ctx* _XCal_FindByEle(HELE hEle)
{
	auto& g = CG();
	auto it = g.elements.find(hEle);
	return it == g.elements.end() ? NULL : it->second;
}

int CALLBACK _XCal_OnPaintWindow(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled);
int CALLBACK _XCal_OnWndClose(HWINDOW hWnd, BOOL* pbHandled);
int CALLBACK _XCal_OnWndDestroy(HWINDOW hWnd, BOOL* pbHandled);

void _XCal_RegisterEle(_XCal_Ctx* ctx, HELE hEle)
{
	if (ctx && hEle) CG().elements[hEle] = ctx;
}

void _XCal_UnregisterCtx(_XCal_Ctx* ctx)
{
	auto& g = CG();
	for (auto it = g.elements.begin(); it != g.elements.end(); ){
		if (it->second == ctx){
			g.commands.erase(it->first);
			it = g.elements.erase(it);
		} else {
			++it;
		}
	}
	if (ctx && ctx->hWnd){
		if (XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
			XWnd_RemoveEventC(ctx->hWnd, WM_PAINT,   (void*)&_XCal_OnPaintWindow);
			XWnd_RemoveEventC(ctx->hWnd, WM_CLOSE,   (void*)&_XCal_OnWndClose);
			XWnd_RemoveEventC(ctx->hWnd, XE_DESTROY, (void*)&_XCal_OnWndDestroy);
		}
		g.windows.erase(ctx->hWnd);
	}
}

void _XCal_ApplyPopupPosition(_XCal_Ctx* ctx)
{
	if (!ctx || !ctx->hWnd || !XC_IsHWINDOW((HXCGUI)ctx->hWnd)) return;
	auto& g = CG();
	if (g.popupMode == _XCal_PopupMode_Pos){
		XWnd_SetPosition(ctx->hWnd, g.popupPt.x, g.popupPt.y);
		XWnd_AdjustInScreen(ctx->hWnd, 0, FALSE);
		g.popupMode = _XCal_PopupMode_Default;
		return;
	}
	if (g.popupMode != _XCal_PopupMode_Ele) return;

	HELE hEle = g.bindEle;
	int offX = g.bindOffsetX;
	int offY = g.bindOffsetY;
	g.popupMode = _XCal_PopupMode_Default;
	g.bindEle = NULL;
	g.bindOffsetX = 0;
	g.bindOffsetY = 0;
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return;

	HWINDOW hEleWnd = XWidget_GetHWINDOW((HXCGUI)hEle);
	HWND hwndEle = hEleWnd ? XWnd_GetHWND(hEleWnd) : NULL;
	if (!hwndEle) return;

	POINT eleTL{0, 0};
	POINT eleBR{XEle_GetWidth(hEle), XEle_GetHeight(hEle)};
	XEle_PointClientToWndClientDPI(hEle, &eleTL);
	XEle_PointClientToWndClientDPI(hEle, &eleBR);
	::ClientToScreen(hwndEle, &eleTL);
	::ClientToScreen(hwndEle, &eleBR);

	int x = eleTL.x + offX;
	int y = eleBR.y + offY;
	XWnd_SetPosition(ctx->hWnd, x, y);
	XWnd_AdjustInScreen(ctx->hWnd, 0, FALSE);
}

void _XCal_SetMonthTitle(const _XCal_Ctx* ctx, HXCGUI hText, int year, int month)
{
	if (!hText) return;
	wchar_t buf[32]{};
	swprintf_s(buf, _countof(buf), L"%04d年%d月", year, month);
	XShapeText_SetText(hText, buf);
	if (ctx) XShapeText_SetTextColor(hText, ctx->colors.title);
}

void _XCal_UpdateEditBox(_XCal_Ctx* ctx, HELE hEdit, const xcalendar_datetime_& d)
{
	if (!ctx || !hEdit || ctx->closing || ctx->destroyed) return;
	ctx->updatingUI = TRUE;
	CXText t = _XCal_FormatDateTime(d);
	XEdit_SetText(hEdit, t.get());
	ctx->updatingUI = FALSE;
}

void _XCal_RefreshCalendar(_XCal_Ctx* ctx);

BOOL _XCal_IsLightTheme(const _XCal_Ctx* ctx)
{
	if (!ctx) return TRUE;
	return _XUITool::IsLightTheme(ctx->theme) ? TRUE : FALSE;
}

HIMAGE _XCal_LoadSvgIcon(const char* svgText, int size, BOOL lightTheme, COLORREF lightColor, HSVG* outSvg)
{
	if (outSvg) *outSvg = NULL;
	if (!svgText) return NULL;
	HSVG hSvg = XSvg_LoadStringUtf8(svgText);
	if (!hSvg) return NULL;
	XSvg_EnableAutoDestroy(hSvg, FALSE);
	XSvg_SetSize(hSvg, size, size);
	if (lightTheme) XSvg_SetUserFillColor(hSvg, lightColor, TRUE);
	HIMAGE hImage = XImage_LoadSvg(hSvg);
	if (hImage) XImage_EnableAutoDestroy(hImage, FALSE);
	if (outSvg) *outSvg = hSvg;
	else XSvg_Destroy(hSvg);
	return hImage;
}

void _XCal_LoadIcons(_XCal_Ctx* ctx)
{
	if (!ctx) return;
	BOOL light = _XCal_IsLightTheme(ctx);
	COLORREF navColor = RGBA(0x45, 0x4D, 0x5A, 0xFF);
	COLORREF spinColor = RGBA(0x99, 0x99, 0x99, 0xFF);
	COLORREF navDisabledColor = light ? RGBA(0x99, 0x99, 0x99, 0xFF) : RGBA(0x7D, 0x7E, 0x7F, 0xFF);
	ctx->hImgLastYear = _XCal_LoadSvgIcon(kCalSvg_LastYear, 24, light, navColor, &ctx->hSvgLastYear);
	ctx->hImgNextYear = _XCal_LoadSvgIcon(kCalSvg_NextYear, 24, light, navColor, &ctx->hSvgNextYear);
	ctx->hImgLastMonth = _XCal_LoadSvgIcon(kCalSvg_LastMonth, 24, light, navColor, &ctx->hSvgLastMonth);
	ctx->hImgNextMonth = _XCal_LoadSvgIcon(kCalSvg_NextMonth, 24, light, navColor, &ctx->hSvgNextMonth);
	ctx->hImgUp = _XCal_LoadSvgIcon(kCalSvg_Up, 8, light, spinColor, &ctx->hSvgUp);
	ctx->hImgDown = _XCal_LoadSvgIcon(kCalSvg_Down, 8, light, spinColor, &ctx->hSvgDown);
	ctx->hImgLastYearDis = _XCal_LoadSvgIcon(kCalSvg_LastYear, 24, TRUE, navDisabledColor, &ctx->hSvgLastYearDis);
	ctx->hImgNextYearDis = _XCal_LoadSvgIcon(kCalSvg_NextYear, 24, TRUE, navDisabledColor, &ctx->hSvgNextYearDis);
	ctx->hImgLastMonthDis = _XCal_LoadSvgIcon(kCalSvg_LastMonth, 24, TRUE, navDisabledColor, &ctx->hSvgLastMonthDis);
	ctx->hImgNextMonthDis = _XCal_LoadSvgIcon(kCalSvg_NextMonth, 24, TRUE, navDisabledColor, &ctx->hSvgNextMonthDis);
	ctx->hImgUpDis = _XCal_LoadSvgIcon(kCalSvg_Up, 8, TRUE, spinColor, &ctx->hSvgUpDis);
	ctx->hImgDownDis = _XCal_LoadSvgIcon(kCalSvg_Down, 8, TRUE, spinColor, &ctx->hSvgDownDis);
}

void _XCal_DestroyIcons(_XCal_Ctx* ctx)
{
	if (!ctx) return;
	if (ctx->hImgLastYear){ XImage_Release(ctx->hImgLastYear); ctx->hImgLastYear = NULL; }
	if (ctx->hImgNextYear){ XImage_Release(ctx->hImgNextYear); ctx->hImgNextYear = NULL; }
	if (ctx->hImgLastMonth){ XImage_Release(ctx->hImgLastMonth); ctx->hImgLastMonth = NULL; }
	if (ctx->hImgNextMonth){ XImage_Release(ctx->hImgNextMonth); ctx->hImgNextMonth = NULL; }
	if (ctx->hImgUp){ XImage_Release(ctx->hImgUp); ctx->hImgUp = NULL; }
	if (ctx->hImgDown){ XImage_Release(ctx->hImgDown); ctx->hImgDown = NULL; }
	if (ctx->hImgLastYearDis){ XImage_Release(ctx->hImgLastYearDis); ctx->hImgLastYearDis = NULL; }
	if (ctx->hImgNextYearDis){ XImage_Release(ctx->hImgNextYearDis); ctx->hImgNextYearDis = NULL; }
	if (ctx->hImgLastMonthDis){ XImage_Release(ctx->hImgLastMonthDis); ctx->hImgLastMonthDis = NULL; }
	if (ctx->hImgNextMonthDis){ XImage_Release(ctx->hImgNextMonthDis); ctx->hImgNextMonthDis = NULL; }
	if (ctx->hImgUpDis){ XImage_Release(ctx->hImgUpDis); ctx->hImgUpDis = NULL; }
	if (ctx->hImgDownDis){ XImage_Release(ctx->hImgDownDis); ctx->hImgDownDis = NULL; }
	if (ctx->hSvgLastYear){ XSvg_Destroy(ctx->hSvgLastYear); ctx->hSvgLastYear = NULL; }
	if (ctx->hSvgNextYear){ XSvg_Destroy(ctx->hSvgNextYear); ctx->hSvgNextYear = NULL; }
	if (ctx->hSvgLastMonth){ XSvg_Destroy(ctx->hSvgLastMonth); ctx->hSvgLastMonth = NULL; }
	if (ctx->hSvgNextMonth){ XSvg_Destroy(ctx->hSvgNextMonth); ctx->hSvgNextMonth = NULL; }
	if (ctx->hSvgUp){ XSvg_Destroy(ctx->hSvgUp); ctx->hSvgUp = NULL; }
	if (ctx->hSvgDown){ XSvg_Destroy(ctx->hSvgDown); ctx->hSvgDown = NULL; }
	if (ctx->hSvgLastYearDis){ XSvg_Destroy(ctx->hSvgLastYearDis); ctx->hSvgLastYearDis = NULL; }
	if (ctx->hSvgNextYearDis){ XSvg_Destroy(ctx->hSvgNextYearDis); ctx->hSvgNextYearDis = NULL; }
	if (ctx->hSvgLastMonthDis){ XSvg_Destroy(ctx->hSvgLastMonthDis); ctx->hSvgLastMonthDis = NULL; }
	if (ctx->hSvgNextMonthDis){ XSvg_Destroy(ctx->hSvgNextMonthDis); ctx->hSvgNextMonthDis = NULL; }
	if (ctx->hSvgUpDis){ XSvg_Destroy(ctx->hSvgUpDis); ctx->hSvgUpDis = NULL; }
	if (ctx->hSvgDownDis){ XSvg_Destroy(ctx->hSvgDownDis); ctx->hSvgDownDis = NULL; }
}

int CALLBACK _XCal_OnPaintWindow(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	if (!hWnd || !hDraw) return 0;
	auto it = CG().windows.find(hWnd);
	_XCal_Ctx* ctx = (it != CG().windows.end()) ? it->second : NULL;
	if (!ctx || ctx->destroyed) return 0;

	RECT rcClient{};
	if (!XWnd_GetClientRect(hWnd, &rcClient)) return 0;
	int cw = rcClient.right - rcClient.left;
	int ch = rcClient.bottom - rcClient.top;
	if (cw <= 0 || ch <= 0) return 0;

	XDraw_EnableSmoothingMode(hDraw, TRUE);

	int innerW = cw - ctx->contentOffX - _XUITool::kShadowMargin;
	int innerH = ch - ctx->contentOffY - _XUITool::kShadowMargin;
	if (innerW <= 0 || innerH <= 0) return 0;
	RECT bodyRc{
		ctx->contentOffX,
		ctx->contentOffY,
		ctx->contentOffX + innerW,
		ctx->contentOffY + innerH
	};

	RECTF shadowBody{
		(float)bodyRc.left,
		(float)bodyRc.top,
		(float)bodyRc.right,
		(float)bodyRc.bottom
	};
	_XUITool::DrawDropShadow(hDraw, shadowBody, (float)ctx->cornerRadius, ctx->theme);

	XDraw_SetBrushColor(hDraw, ctx->colors.wndBg);
	XDraw_FillRoundRect(hDraw, &bodyRc, ctx->cornerRadius, ctx->cornerRadius);

	auto drawSep = [&](int y){
		if (y < 0) return;
		RECT rc{ bodyRc.left, bodyRc.top + y, bodyRc.right, bodyRc.top + y + 1 };
		XDraw_SetBrushColor(hDraw, ctx->colors.separator);
		XDraw_FillRect(hDraw, &rc);
	};
	drawSep(ctx->sepHeaderY);
	drawSep(ctx->sepFooterY);
	return 0;
}

void _XCal_EndModal(_XCal_Ctx* ctx, int result)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	ctx->closing = TRUE;
	if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
		XModalWnd_EndModal(ctx->hWnd, result);
	}
}

void _XCal_SetQuickRange(_XCal_Ctx* ctx, int days)
{
	if (!ctx) return;
	xcalendar_datetime_ base = (ctx->limitMaxDate ? ctx->maxDate : ctx->today);
	ctx->selEnd = base;
	ctx->selEnd.hour = 23;
	ctx->selEnd.minute = 59;
	ctx->selEnd.second = 59;
	ctx->selStart = _XCal_AddDays(base, -(days - 1));
	ctx->selStart.hour = 0;
	ctx->selStart.minute = 0;
	ctx->selStart.second = 0;
	ctx->showDate.year = ctx->selEnd.year;
	ctx->showDate.month = ctx->selEnd.month;
	ctx->showDate.day = 1;
	_XCal_RefreshCalendar(ctx);
}

void _XCal_ApplyCellStyle(_XCal_Ctx* ctx, HELE hCell, const xcalendar_datetime_& d, int month, BOOL enabled)
{
	if (!ctx || !hCell) return;
	const auto& c = ctx->colors;
	XEle_ClearBkInfo(hCell);
	XEle_EnableBkTransparent(hCell, TRUE);
	XEle_Enable(hCell, enabled);

	if (!enabled){
		XEle_SetBkInfo(hCell, c.bkNormal);
		XEle_SetTextColor(hCell, c.cellDisabledText);
		return;
	}

	BOOL selected = FALSE;
	BOOL inRange = FALSE;
	if (ctx->mode == _XCal_Mode_Single){
		selected = (_XCal_CompareDate(d, ctx->selStart) == 0);
	} else {
		int cmpStart = _XCal_CompareDate(d, ctx->selStart);
		int cmpEnd   = _XCal_CompareDate(d, ctx->selEnd);
		selected = (cmpStart == 0 || cmpEnd == 0);
		inRange = (cmpStart > 0 && cmpEnd < 0);
	}

	if (selected){
		XEle_SetBkInfo(hCell, c.bkPrimary);
		XEle_SetTextColor(hCell, c.accentText);
	} else if (inRange){
		XEle_SetBkInfo(hCell, c.bkRange);
		XEle_SetTextColor(hCell, c.text);
	} else {
		XEle_SetBkInfo(hCell, c.bkNormal);
		XEle_SetTextColor(hCell, (d.month == month) ? c.text : c.mutedText);
	}
}

void _XCal_RefreshGrid(_XCal_Ctx* ctx, HELE* cells, int year, int month, int panelIndex)
{
	if (!ctx || !cells) return;
	for (int i = 0; i < 42; ++i){
		xcalendar_datetime_ d = _XCal_DateFromIndex(i, year, month);
		wchar_t dayBuf[8]{};
		swprintf_s(dayBuf, _countof(dayBuf), L"%d", d.day);
		XBtn_SetText(cells[i], dayBuf);
		XEle_SetUserData(cells[i], _XCal_DateUserData(d, panelIndex));
		BOOL enabled = (d.month == month && !_XCal_IsAfterMaxDate(ctx, d));
		_XCal_ApplyCellStyle(ctx, cells[i], d, month, enabled);
	}
}

void _XCal_RefreshCalendar(_XCal_Ctx* ctx)
{
	if (!ctx || ctx->updatingUI || ctx->closing || ctx->destroyed || !XC_IsHWINDOW((HXCGUI)ctx->hWnd)) return;

	ctx->selStart = _XCal_ClampMaxDate(ctx, ctx->selStart, FALSE);
	ctx->selEnd = _XCal_ClampMaxDate(ctx, ctx->selEnd, TRUE);

	if (ctx->mode == _XCal_Mode_Range && _XCal_CompareFull(ctx->selStart, ctx->selEnd) > 0){
		xcalendar_datetime_ tmp = ctx->selStart;
		ctx->selStart = ctx->selEnd;
		ctx->selEnd = tmp;
	}

	int rightY = ctx->showDate.year;
	int rightM = ctx->showDate.month;
	if (ctx->doubleMonth && ctx->mode == _XCal_Mode_Range){
		xcalendar_datetime_ leftDate = _XCal_AddMonths(ctx->showDate, -1);
		_XCal_RefreshGrid(ctx, ctx->hCellsLeft, leftDate.year, leftDate.month, 0);
		_XCal_SetMonthTitle(ctx, ctx->hTitleLeft, leftDate.year, leftDate.month);
		_XCal_RefreshGrid(ctx, ctx->hCellsRight, rightY, rightM, 1);
		_XCal_SetMonthTitle(ctx, ctx->hTitleRight, rightY, rightM);
	} else {
		_XCal_RefreshGrid(ctx, ctx->hCellsLeft, rightY, rightM, 0);
		_XCal_SetMonthTitle(ctx, ctx->hTitleLeft, rightY, rightM);
	}

	if (ctx->hRangeText){
		CXText label = (ctx->mode == _XCal_Mode_Single)
			? _XCal_FormatShortDate(ctx->selStart)
			: _XCal_FormatRangeLabel(ctx->selStart, ctx->selEnd);
		XShapeText_SetText(ctx->hRangeText, label.get());
		XShapeText_SetTextColor(ctx->hRangeText, ctx->colors.rangeText);
	}
	_XCal_UpdateEditBox(ctx, ctx->hEditStart, ctx->selStart);
	_XCal_UpdateEditBox(ctx, ctx->hEditEnd, ctx->selEnd);

	if (ctx->hBtnNextMonth){
		XEle_Enable(ctx->hBtnNextMonth, _XCal_CanMoveToMonth(ctx, _XCal_AddMonths(ctx->showDate, 1)));
	}
	if (ctx->hBtnNextYear){
		xcalendar_datetime_ nextYear = ctx->showDate;
		++nextYear.year;
		nextYear = _XCal_Normalize(nextYear);
		XEle_Enable(ctx->hBtnNextYear, _XCal_CanMoveToMonth(ctx, nextYear));
	}

	XWnd_Redraw(ctx->hWnd);
}

void _XCal_RefreshTimeText(_XCal_Ctx* ctx)
{
	if (!ctx || ctx->updatingUI || ctx->closing || ctx->destroyed || !XC_IsHWINDOW((HXCGUI)ctx->hWnd)) return;
	ctx->selStart = _XCal_ClampMaxDate(ctx, ctx->selStart, FALSE);
	ctx->selEnd = _XCal_ClampMaxDate(ctx, ctx->selEnd, TRUE);
	if (ctx->mode == _XCal_Mode_Range && _XCal_CompareFull(ctx->selStart, ctx->selEnd) > 0){
		xcalendar_datetime_ tmp = ctx->selStart;
		ctx->selStart = ctx->selEnd;
		ctx->selEnd = tmp;
	}
	if (ctx->hRangeText){
		CXText label = (ctx->mode == _XCal_Mode_Single)
			? _XCal_FormatShortDate(ctx->selStart)
			: _XCal_FormatRangeLabel(ctx->selStart, ctx->selEnd);
		XShapeText_SetText(ctx->hRangeText, label.get());
		XShapeText_SetTextColor(ctx->hRangeText, ctx->colors.rangeText);
	}
	_XCal_UpdateEditBox(ctx, ctx->hEditStart, ctx->selStart);
	_XCal_UpdateEditBox(ctx, ctx->hEditEnd, ctx->selEnd);
	XWnd_Redraw(ctx->hWnd);
}

void _XCal_MoveMonth(_XCal_Ctx* ctx, int delta)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	xcalendar_datetime_ next = _XCal_AddMonths(ctx->showDate, delta);
	if (!_XCal_CanMoveToMonth(ctx, next)) return;
	ctx->showDate = next;
	_XCal_RefreshCalendar(ctx);
}

void _XCal_MoveYear(_XCal_Ctx* ctx, int delta)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	xcalendar_datetime_ next = ctx->showDate;
	next.year += delta;
	next = _XCal_Normalize(next);
	if (!_XCal_CanMoveToMonth(ctx, next)) return;
	ctx->showDate = next;
	_XCal_RefreshCalendar(ctx);
}

void _XCal_OnDateCell(_XCal_Ctx* ctx, HELE hEle)
{
	if (!ctx || !hEle || ctx->closing || ctx->destroyed) return;
	xcalendar_datetime_ clicked = _XCal_DateFromUserData(XEle_GetUserData(hEle));
	if (_XCal_IsAfterMaxDate(ctx, clicked)) return;
	if (ctx->mode == _XCal_Mode_Single){
		ctx->selStart = clicked;
		ctx->selStart.hour = 0;
		ctx->selStart.minute = 0;
		ctx->selStart.second = 0;
		_XCal_RefreshCalendar(ctx);
		return;
	}

	xcalendar_datetime_ clickedStart = clicked;
	clickedStart.hour = 0;
	clickedStart.minute = 0;
	clickedStart.second = 0;
	xcalendar_datetime_ clickedEnd = clicked;
	clickedEnd.hour = 23;
	clickedEnd.minute = 59;
	clickedEnd.second = 59;

	int cmpStart = _XCal_CompareDate(clickedStart, ctx->selStart);
	int cmpEnd = _XCal_CompareDate(clickedStart, ctx->selEnd);
	BOOL singleDay = (_XCal_CompareDate(ctx->selStart, ctx->selEnd) == 0);
	if (singleDay){
		if (cmpStart < 0){
			ctx->selStart = clickedStart;
		} else {
			ctx->selEnd = clickedEnd;
		}
	} else if (cmpStart < 0){
		ctx->selStart = clickedStart;
	} else if (cmpEnd <= 0){
		ctx->selEnd = clickedEnd;
	} else {
		ctx->selStart = clickedStart;
		ctx->selEnd = clickedEnd;
	}
	_XCal_RefreshCalendar(ctx);
}

void _XCal_EditLogic(_XCal_Ctx* ctx, HELE hEdit, xcalendar_datetime_* pDate, int eventType, int keyOrPos, BOOL isStart)
{
	if (!ctx || !hEdit || !pDate || ctx->updatingUI || ctx->closing || ctx->destroyed) return;
	if (eventType == XE_LBUTTONUP){
		int pos = XEdit_GetCurPos(hEdit);
		int field = _XCal_FieldFromPos(pos);
		if (isStart) ctx->focusFieldStart = field;
		else ctx->focusFieldEnd = field;
		_XCal_SelectField(hEdit, field);
		return;
	}
	if (eventType == XE_KEYDOWN){
		if (keyOrPos == VK_UP || keyOrPos == VK_DOWN){
			int field = isStart ? ctx->focusFieldStart : ctx->focusFieldEnd;
			if (field == kCal_FieldNone) field = kCal_FieldSecond;
			*pDate = _XCal_AdjustField(*pDate, field, keyOrPos == VK_UP ? 1 : -1);
			*pDate = _XCal_ClampMaxDate(ctx, *pDate, !isStart);
			if (ctx->mode == _XCal_Mode_Range){
				if (_XCal_CompareFull(ctx->selStart, ctx->selEnd) > 0){
					if (isStart) ctx->selEnd = ctx->selStart;
					else ctx->selStart = ctx->selEnd;
				}
			}
			if (_XCal_IsDateField(field)){
				ctx->showDate.year = pDate->year;
				ctx->showDate.month = pDate->month;
				ctx->showDate.day = 1;
				_XCal_RefreshCalendar(ctx);
			} else {
				_XCal_RefreshTimeText(ctx);
			}
			_XCal_SelectField(hEdit, field);
		}
		return;
	}
	if (eventType == XE_KILLFOCUS){
		xcalendar_datetime_ oldDate = *pDate;
		const wchar_t* txt = XEdit_GetText_Temp(hEdit);
		*pDate = _XCal_ParseDateTime(txt, *pDate);
		*pDate = _XCal_ClampMaxDate(ctx, *pDate, !isStart);
		if (ctx->mode == _XCal_Mode_Range){
			if (_XCal_CompareFull(ctx->selStart, ctx->selEnd) > 0){
				if (isStart) *pDate = ctx->selEnd;
				else *pDate = ctx->selStart;
			}
		}
		if (_XCal_CompareDate(oldDate, *pDate) != 0){
			ctx->showDate.year = pDate->year;
			ctx->showDate.month = pDate->month;
			ctx->showDate.day = 1;
			_XCal_RefreshCalendar(ctx);
		} else {
			_XCal_RefreshTimeText(ctx);
		}
	}
}

void _XCal_Spin(_XCal_Ctx* ctx, BOOL isStart, int delta)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	HELE hEdit = isStart ? ctx->hEditStart : ctx->hEditEnd;
	xcalendar_datetime_* pDate = isStart ? &ctx->selStart : &ctx->selEnd;
	int field = isStart ? ctx->focusFieldStart : ctx->focusFieldEnd;
	if (field == kCal_FieldNone) field = kCal_FieldSecond;
	*pDate = _XCal_AdjustField(*pDate, field, delta);
	*pDate = _XCal_ClampMaxDate(ctx, *pDate, !isStart);
	if (ctx->mode == _XCal_Mode_Range && _XCal_CompareFull(ctx->selStart, ctx->selEnd) > 0){
		if (isStart) ctx->selEnd = ctx->selStart;
		else ctx->selStart = ctx->selEnd;
	}
	if (_XCal_IsDateField(field)){
		ctx->showDate.year = pDate->year;
		ctx->showDate.month = pDate->month;
		ctx->showDate.day = 1;
		_XCal_RefreshCalendar(ctx);
	} else {
		_XCal_RefreshTimeText(ctx);
	}
	if (hEdit){
		XWnd_SetFocusEle(ctx->hWnd, hEdit);
		_XCal_SelectField(hEdit, field);
	}
}

int CALLBACK _XCal_OnBtnClick(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XCal_Ctx* ctx = _XCal_FindByEle(hEle);
	if (!ctx || ctx->closing || ctx->destroyed) return 0;
	_XCal_Cmd cmd = _XCal_Cmd_None;
	auto cmdIt = CG().commands.find(hEle);
	if (cmdIt != CG().commands.end()) cmd = cmdIt->second;
	switch (cmd){
	case _XCal_Cmd_DateCell:      _XCal_OnDateCell(ctx, hEle); break;
	case _XCal_Cmd_PrevYear:      _XCal_MoveYear(ctx, -1); break;
	case _XCal_Cmd_PrevMonth:     _XCal_MoveMonth(ctx, -1); break;
	case _XCal_Cmd_NextMonth:     _XCal_MoveMonth(ctx, 1); break;
	case _XCal_Cmd_NextYear:      _XCal_MoveYear(ctx, 1); break;
	case _XCal_Cmd_Today:         _XCal_SetQuickRange(ctx, 1); break;
	case _XCal_Cmd_Last7:         _XCal_SetQuickRange(ctx, 7); break;
	case _XCal_Cmd_Last15:        _XCal_SetQuickRange(ctx, 15); break;
	case _XCal_Cmd_Last30:        _XCal_SetQuickRange(ctx, 30); break;
	case _XCal_Cmd_StartSpinUp:   _XCal_Spin(ctx, TRUE, 1); break;
	case _XCal_Cmd_StartSpinDown: _XCal_Spin(ctx, TRUE, -1); break;
	case _XCal_Cmd_EndSpinUp:     _XCal_Spin(ctx, FALSE, 1); break;
	case _XCal_Cmd_EndSpinDown:   _XCal_Spin(ctx, FALSE, -1); break;
	case _XCal_Cmd_Confirm:
		ctx->confirmed = TRUE;
		ctx->resultStart = _XCal_ClampMaxDate(ctx, ctx->selStart, FALSE);
		ctx->resultEnd = _XCal_ClampMaxDate(ctx, ctx->selEnd, TRUE);
		_XCal_EndModal(ctx, kCal_ResultOk);
		break;
	case _XCal_Cmd_Cancel:
		ctx->confirmed = FALSE;
		_XCal_EndModal(ctx, kCal_ResultCancel);
		break;
	default:
		break;
	}
	return 0;
}

int CALLBACK _XCal_OnEditStartLButtonUp(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XCal_Ctx* ctx = _XCal_FindByEle(hEle);
	if (ctx && !ctx->closing && !ctx->destroyed) _XCal_EditLogic(ctx, hEle, &ctx->selStart, XE_LBUTTONUP, 0, TRUE);
	return 0;
}

int CALLBACK _XCal_OnEditEndLButtonUp(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XCal_Ctx* ctx = _XCal_FindByEle(hEle);
	if (ctx && !ctx->closing && !ctx->destroyed) _XCal_EditLogic(ctx, hEle, &ctx->selEnd, XE_LBUTTONUP, 0, FALSE);
	return 0;
}

int CALLBACK _XCal_OnEditStartKeyDown(HELE hEle, int iChar, int, BOOL* pbHandled)
{
	_XCal_Ctx* ctx = _XCal_FindByEle(hEle);
	if (ctx && !ctx->closing && !ctx->destroyed) _XCal_EditLogic(ctx, hEle, &ctx->selStart, XE_KEYDOWN, iChar, TRUE);
	if (pbHandled && (iChar == VK_UP || iChar == VK_DOWN)) *pbHandled = TRUE;
	return 0;
}

int CALLBACK _XCal_OnEditEndKeyDown(HELE hEle, int iChar, int, BOOL* pbHandled)
{
	_XCal_Ctx* ctx = _XCal_FindByEle(hEle);
	if (ctx && !ctx->closing && !ctx->destroyed) _XCal_EditLogic(ctx, hEle, &ctx->selEnd, XE_KEYDOWN, iChar, FALSE);
	if (pbHandled && (iChar == VK_UP || iChar == VK_DOWN)) *pbHandled = TRUE;
	return 0;
}

int CALLBACK _XCal_OnEditStartKillFocus(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XCal_Ctx* ctx = _XCal_FindByEle(hEle);
	if (ctx && !ctx->closing && !ctx->destroyed) _XCal_EditLogic(ctx, hEle, &ctx->selStart, XE_KILLFOCUS, 0, TRUE);
	return 0;
}

int CALLBACK _XCal_OnEditEndKillFocus(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XCal_Ctx* ctx = _XCal_FindByEle(hEle);
	if (ctx && !ctx->closing && !ctx->destroyed) _XCal_EditLogic(ctx, hEle, &ctx->selEnd, XE_KILLFOCUS, 0, FALSE);
	return 0;
}

int CALLBACK _XCal_OnWndClose(HWINDOW hWnd, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	auto it = CG().windows.find(hWnd);
	if (it != CG().windows.end()){
		_XCal_Ctx* ctx = it->second;
		if (ctx) ctx->closing = TRUE;
		if (ctx && ctx->hBtnConfirm && XC_IsHELE((HXCGUI)ctx->hBtnConfirm)){
			XWnd_SetFocusEle(hWnd, ctx->hBtnConfirm);
		}
	}
	return 0;
}

int CALLBACK _XCal_OnWndDestroy(HWINDOW hWnd, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	auto it = CG().windows.find(hWnd);
	if (it != CG().windows.end()){
		_XCal_Ctx* ctx = it->second;
		ctx->destroyed = TRUE;
		_XCal_UnregisterCtx(ctx);
	}
	return 0;
}

HELE _XCal_CreateCmdButton(_XCal_Ctx* ctx, int x, int y, int w, int h,
	const wchar_t* text, _XCal_Cmd cmd, BOOL primary = FALSE, BOOL cancel = FALSE)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE btn = XBtn_Create(x, y, w, h, text, ctx->hWnd);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	XEle_SetUserData(btn, (vint)cmd);
	_XCal_RegisterEle(ctx, btn);
	CG().commands[btn] = cmd;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XCal_OnBtnClick);
	_XCal_StyleButton(btn, ctx->colors, primary, cancel);
	if (ctx->hFont) XEle_SetFont(btn, ctx->hFont);
	return btn;
}

HELE _XCal_CreateChildCmdButton(_XCal_Ctx* ctx, HXCGUI hParent, int x, int y, int w, int h,
	const wchar_t* text, _XCal_Cmd cmd, BOOL primary = FALSE, BOOL cancel = FALSE)
{
	HELE btn = XBtn_Create(x, y, w, h, text, hParent);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	XEle_SetUserData(btn, (vint)cmd);
	_XCal_RegisterEle(ctx, btn);
	CG().commands[btn] = cmd;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XCal_OnBtnClick);
	_XCal_StyleButton(btn, ctx->colors, primary, cancel);
	if (ctx->hFont) XEle_SetFont(btn, ctx->hFont);
	return btn;
}

void _XCal_SetButtonIcon(HELE hBtn, HIMAGE hImage, HIMAGE hDisableImage = NULL)
{
	if (!hBtn || !hImage) return;
	XBtn_SetText(hBtn, L"");
	XBtn_SetIcon(hBtn, hImage);
	XBtn_SetIconDisable(hBtn, hDisableImage ? hDisableImage : hImage);
	XBtn_SetIconAlign(hBtn, button_icon_align_left);
	XBtn_SetIconSpace(hBtn, 0);
}

HELE _XCal_CreateCell(_XCal_Ctx* ctx, int x, int y, int size)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE btn = XBtn_Create(x, y, size, size, L"", ctx->hWnd);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	XEle_EnableBkTransparent(btn, TRUE);
	XEle_SetUserData(btn, (vint)_XCal_Cmd_DateCell);
	_XCal_RegisterEle(ctx, btn);
	CG().commands[btn] = _XCal_Cmd_DateCell;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XCal_OnBtnClick);
	if (ctx->hFont) XEle_SetFont(btn, ctx->hFont);
	return btn;
}

HELE _XCal_CreateActionLayout(_XCal_Ctx* ctx, int x, int y, int w, int h)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE wrap = XLayout_Create(x, y, w, h, (HXCGUI)ctx->hWnd);
	if (!wrap) return NULL;
	_XCal_RegisterEle(ctx, wrap);
	XLayoutBox_EnableHorizon(wrap, TRUE);
	XLayoutBox_SetSpace(wrap, 8);
	XLayout_EnableLayout(wrap, TRUE);
	XEle_EnableBkTransparent(wrap, TRUE);
	XWidget_LayoutItem_SetWidth(wrap, layout_size_fixed, w);
	XWidget_LayoutItem_SetHeight(wrap, layout_size_fixed, h);
	return wrap;
}

HXCGUI _XCal_CreateText(_XCal_Ctx* ctx, int x, int y, int w, int h, const wchar_t* text,
	COLORREF color, int align = textAlignFlag_center | textAlignFlag_vcenter, BOOL useSmallFont = FALSE)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HXCGUI t = XShapeText_Create(x, y, w, h, text ? text : L"", ctx->hWnd);
	if (!t) return NULL;
	XUI_EnableCSS(t, FALSE);
	XShapeText_SetTextColor(t, color);
	XShapeText_SetTextAlign(t, align);
	if (useSmallFont && ctx->hFontSmall) XShapeText_SetFont(t, ctx->hFontSmall);
	else if (ctx->hFont) XShapeText_SetFont(t, ctx->hFont);
	XShapeText_SetTextColor(t, color);
	return t;
}

HELE _XCal_CreateEdit(_XCal_Ctx* ctx, int x, int y, int w, int h, BOOL isStart,
	HELE* phWrap = NULL, HELE* phSpinWrap = NULL, HELE* phSpinUp = NULL, HELE* phSpinDown = NULL)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE wrap = XLayout_Create(x, y, w, h, (HXCGUI)ctx->hWnd);
	if (!wrap) return NULL;
	_XCal_RegisterEle(ctx, wrap);
	XLayout_EnableLayout(wrap, FALSE);
	XEle_EnableBkTransparent(wrap, TRUE);
	XWidget_LayoutItem_SetWidth(wrap, layout_size_fixed, w);
	XWidget_LayoutItem_SetHeight(wrap, layout_size_fixed, h);

	HELE edit = XEdit_Create(0, 0, w, h, (HXCGUI)wrap);
	if (!edit) return NULL;
	XUI_EnableCSS(edit, FALSE);
	_XCal_RegisterEle(ctx, edit);
	_XCal_StyleInput(edit, ctx->colors);
	if (ctx->hFont) XEle_SetFont(edit, ctx->hFont);
	XEle_RegEventC1(edit, XE_LBUTTONUP, (void*)(isStart ? &_XCal_OnEditStartLButtonUp : &_XCal_OnEditEndLButtonUp));
	XEle_RegEventC1(edit, XE_KEYDOWN,   (void*)(isStart ? &_XCal_OnEditStartKeyDown   : &_XCal_OnEditEndKeyDown));
	XEle_RegEventC1(edit, XE_KILLFOCUS, (void*)(isStart ? &_XCal_OnEditStartKillFocus : &_XCal_OnEditEndKillFocus));

	HELE spinWrap = XLayout_Create(w - 20, 0, 20, h, (HXCGUI)wrap);
	if (spinWrap){
		_XCal_RegisterEle(ctx, spinWrap);
		XLayoutBox_EnableHorizon(spinWrap, FALSE);
		XLayout_EnableLayout(spinWrap, TRUE);
		XEle_EnableBkTransparent(spinWrap, TRUE);
		XEle_SetPadding(spinWrap, 0, 1, 1, 1);
		XWidget_LayoutItem_SetPosition(spinWrap, -3200, -3200, 0, 0);
		HELE up = _XCal_CreateChildCmdButton(ctx, (HXCGUI)spinWrap, 0, 0, 20, h / 2, L"",
			isStart ? _XCal_Cmd_StartSpinUp : _XCal_Cmd_EndSpinUp);
		HELE down = _XCal_CreateChildCmdButton(ctx, (HXCGUI)spinWrap, 0, h / 2, 20, h / 2, L"",
			isStart ? _XCal_Cmd_StartSpinDown : _XCal_Cmd_EndSpinDown);
		if (up){
			_XCal_StyleSpinButton(up, ctx->colors);
			XWidget_LayoutItem_SetWidth(up, layout_size_fill, 0);
			XWidget_LayoutItem_SetHeight(up, layout_size_weight, 1);
			_XCal_SetButtonIcon(up, ctx->hImgUp, ctx->hImgUpDis);
		}
		if (down){
			_XCal_StyleSpinButton(down, ctx->colors);
			XWidget_LayoutItem_SetWidth(down, layout_size_fill, 0);
			XWidget_LayoutItem_SetHeight(down, layout_size_weight, 1);
			_XCal_SetButtonIcon(down, ctx->hImgDown, ctx->hImgDownDis);
		}
		if (phSpinUp) *phSpinUp = up;
		if (phSpinDown) *phSpinDown = down;
	}
	if (phWrap) *phWrap = wrap;
	if (phSpinWrap) *phSpinWrap = spinWrap;
	return edit;
}

void _XCal_BuildGrid(_XCal_Ctx* ctx, HELE* cells, int startX, int startY, BOOL rightPanel)
{
	static const wchar_t* weeks[] = { L"一", L"二", L"三", L"四", L"五", L"六", L"日" };
	const int colW = 40;
	const int btnSize = 36;
	int labelOffset = rightPanel ? 7 : 0;
	for (int i = 0; i < 7; ++i){
		ctx->hWeekLabels[labelOffset + i] = _XCal_CreateText(ctx, startX + i * colW, startY,
			colW, 20, weeks[i], ctx->colors.weekText);
	}
	int gridY = startY + 30;
	for (int i = 0; i < 42; ++i){
		int r = i / 7;
		int c = i % 7;
		cells[i] = _XCal_CreateCell(ctx, startX + c * colW + 2, gridY + r * colW, btnSize);
	}
}

_XCal_Ctx* _XCal_CreateWindow(_XCal_Mode mode, BOOL doubleMonth, HWINDOW hParent,
	xuitool_theme_ theme, int cornerRadius)
{
	_XCal_Ctx* ctx = new _XCal_Ctx();
	if (!ctx) return NULL;
	ctx->mode = mode;
	ctx->doubleMonth = (mode == _XCal_Mode_Range) ? doubleMonth : FALSE;
	ctx->theme = theme;
	ctx->hParent = hParent ? XWnd_GetHWND(hParent) : NULL;
	ctx->cornerRadius = _XCal_ClampInt(cornerRadius, 0, 32);
	_XCal_ResolveTheme(theme, &ctx->colors);
	ctx->today = _XCal_Current();
	ctx->showDate = ctx->today;
	ctx->showDate.day = 1;
	ctx->maxDate = ctx->today;
	ctx->maxDate.hour = 23;
	ctx->maxDate.minute = 59;
	ctx->maxDate.second = 59;

	int contentW = 0;
	int contentH = 0;
	if (mode == _XCal_Mode_Single){
		contentW = kCal_PadH * 2 + kCal_PanelW;
		contentH = kCal_PadV + kCal_NavSize + kCal_SepGap + 1 + kCal_SepGap
			+ kCal_GridH + kCal_SepGap + 1 + kCal_SepGap + kCal_ActionH + kCal_PadV;
	} else if (ctx->doubleMonth){
		contentW = kCal_PadH * 2 + kCal_PanelW * 2 + kCal_PanelGap;
		contentH = kCal_PadV + kCal_NavSize + kCal_SepGap + 1 + kCal_SepGap
			+ kCal_GridH + kCal_SepGap + 1 + kCal_SepGap + kCal_EditH + kCal_SepGap
			+ kCal_QuickH + kCal_PadV;
	} else {
		contentW = kCal_PadH * 2 + kCal_PanelW;
		contentH = kCal_PadV + kCal_NavSize + kCal_SepGap + 1 + kCal_SepGap
			+ kCal_GridH + kCal_SepGap + 1 + kCal_SepGap + kCal_EditH + kCal_EditGap + kCal_EditH
			+ kCal_SepGap + kCal_QuickH + kCal_EditGap + kCal_QuickH + kCal_SepGap
			+ kCal_ActionH + kCal_PadV;
	}
	ctx->contentW = contentW;
	ctx->contentH = contentH;
	int winW = contentW + _XUITool::kShadowMargin * 2;
	int winH = contentH + _XUITool::kShadowMargin * 2;
	ctx->hWnd = XModalWnd_Create(winW, winH,
		mode == _XCal_Mode_Single ? L"选择日期" : L"选择日期范围",
		ctx->hParent, window_style_nothing);
	if (!ctx->hWnd){
		delete ctx;
		return NULL;
	}
	CG().windows[ctx->hWnd] = ctx;
	XWnd_SetTransparentType(ctx->hWnd, window_transparent_shaped);
	XWnd_SetTransparentAlpha(ctx->hWnd, 255);
	XWnd_EnableDragWindow(ctx->hWnd, FALSE);
	XWnd_EnableDragBorder(ctx->hWnd, FALSE);
	XWnd_EnableDragCaption(ctx->hWnd, FALSE);
	XWnd_EnableDrawBk(ctx->hWnd, TRUE);
	XWnd_SetTextColor(ctx->hWnd, ctx->colors.text);
	XWnd_ClearBkInfo(ctx->hWnd);
	XWnd_RegEventC1(ctx->hWnd, WM_PAINT,   (void*)&_XCal_OnPaintWindow);
	XWnd_RegEventC1(ctx->hWnd, WM_CLOSE,   (void*)&_XCal_OnWndClose);
	XWnd_RegEventC1(ctx->hWnd, XE_DESTROY, (void*)&_XCal_OnWndDestroy);
	XModalWnd_EnableAutoClose(ctx->hWnd, TRUE);
	XModalWnd_EnableEscClose(ctx->hWnd, TRUE);

	ctx->hFont = XFont_CreateEx(L"微软雅黑", 10, fontStyle_regular);
	ctx->hFontSmall = XFont_CreateEx(L"微软雅黑", 9, fontStyle_regular);
	_XCal_LoadIcons(ctx);

	if (mode == _XCal_Mode_Single){
		const int left = kCal_PadH;
		const int panelRight = left + kCal_PanelW;
		const int navY = kCal_PadV;
		const int headerSepY = navY + kCal_NavSize + kCal_SepGap;
		const int gridY = headerSepY + 1 + kCal_SepGap;
		const int footerSepY = gridY + kCal_GridH + kCal_SepGap;
		const int actionY = footerSepY + 1 + kCal_SepGap;
		const int nextYearX = panelRight - kCal_NavSize;
		const int nextMonthX = nextYearX - kCal_NavSpace - kCal_NavSize;
		ctx->hBtnPrevYear = _XCal_CreateCmdButton(ctx, left, navY, kCal_NavSize, kCal_NavSize, L"<<", _XCal_Cmd_PrevYear);
		ctx->hBtnPrevMonth = _XCal_CreateCmdButton(ctx, left + kCal_NavSize + kCal_NavSpace, navY,
			kCal_NavSize, kCal_NavSize, L"<", _XCal_Cmd_PrevMonth);
		ctx->hBtnNextMonth = _XCal_CreateCmdButton(ctx, nextMonthX, navY, kCal_NavSize, kCal_NavSize, L">", _XCal_Cmd_NextMonth);
		ctx->hBtnNextYear = _XCal_CreateCmdButton(ctx, nextYearX, navY, kCal_NavSize, kCal_NavSize, L">>", _XCal_Cmd_NextYear);
		_XCal_SetButtonIcon(ctx->hBtnPrevYear, ctx->hImgLastYear, ctx->hImgLastYearDis);
		_XCal_SetButtonIcon(ctx->hBtnPrevMonth, ctx->hImgLastMonth, ctx->hImgLastMonthDis);
		_XCal_SetButtonIcon(ctx->hBtnNextMonth, ctx->hImgNextMonth, ctx->hImgNextMonthDis);
		_XCal_SetButtonIcon(ctx->hBtnNextYear, ctx->hImgNextYear, ctx->hImgNextYearDis);
		ctx->hTitleLeft = _XCal_CreateText(ctx, left + (kCal_PanelW - 120) / 2, navY, 120, kCal_NavSize, L"", ctx->colors.title);
		_XCal_SetSeparator(ctx, 0, headerSepY);
		_XCal_BuildGrid(ctx, ctx->hCellsLeft, left, gridY, FALSE);
		_XCal_SetSeparator(ctx, 1, footerSepY);
		ctx->hActionWrap = _XCal_CreateActionLayout(ctx, left, actionY, kCal_PanelW, kCal_ActionH);
		if (ctx->hActionWrap){
			ctx->hBtnCancel = _XCal_CreateChildCmdButton(ctx, (HXCGUI)ctx->hActionWrap, 0, 0, 100, 32,
				L"取消", _XCal_Cmd_Cancel, FALSE, TRUE);
			ctx->hBtnConfirm = _XCal_CreateChildCmdButton(ctx, (HXCGUI)ctx->hActionWrap, 0, 0, 100, 32,
				L"确定", _XCal_Cmd_Confirm, TRUE);
			XWidget_LayoutItem_SetWidth(ctx->hBtnCancel, layout_size_weight, 1);
			XWidget_LayoutItem_SetHeight(ctx->hBtnCancel, layout_size_fill, 0);
			XWidget_LayoutItem_SetWidth(ctx->hBtnConfirm, layout_size_weight, 1);
			XWidget_LayoutItem_SetHeight(ctx->hBtnConfirm, layout_size_fill, 0);
		}
	} else {
		const int left = kCal_PadH;
		const int navY = kCal_PadV;
		const int rightStart = ctx->doubleMonth ? (left + kCal_PanelW + kCal_PanelGap) : left;
		const int panelRight = ctx->doubleMonth ? (rightStart + kCal_PanelW) : (left + kCal_PanelW);
		const int headerSepY = navY + kCal_NavSize + kCal_SepGap;
		const int gridY = headerSepY + 1 + kCal_SepGap;
		const int footerSepY = gridY + kCal_GridH + kCal_SepGap;
		const int editY = footerSepY + 1 + kCal_SepGap;
		const int nextYearX = panelRight - kCal_NavSize;
		const int nextMonthX = nextYearX - kCal_NavSpace - kCal_NavSize;
		const int confirmX = panelRight - kCal_ConfirmW;
		const int cancelX = confirmX - kCal_BtnGap - kCal_CancelW;
		const int editW = ctx->doubleMonth ? kCal_EditW : kCal_PanelW;

		ctx->hBtnPrevYear = _XCal_CreateCmdButton(ctx, left, navY, kCal_NavSize, kCal_NavSize, L"<<", _XCal_Cmd_PrevYear);
		ctx->hBtnPrevMonth = _XCal_CreateCmdButton(ctx, left + kCal_NavSize + kCal_NavSpace, navY,
			kCal_NavSize, kCal_NavSize, L"<", _XCal_Cmd_PrevMonth);
		ctx->hBtnNextMonth = _XCal_CreateCmdButton(ctx, nextMonthX, navY, kCal_NavSize, kCal_NavSize, L">", _XCal_Cmd_NextMonth);
		ctx->hBtnNextYear = _XCal_CreateCmdButton(ctx, nextYearX, navY, kCal_NavSize, kCal_NavSize, L">>", _XCal_Cmd_NextYear);
		_XCal_SetButtonIcon(ctx->hBtnPrevYear, ctx->hImgLastYear, ctx->hImgLastYearDis);
		_XCal_SetButtonIcon(ctx->hBtnPrevMonth, ctx->hImgLastMonth, ctx->hImgLastMonthDis);
		_XCal_SetButtonIcon(ctx->hBtnNextMonth, ctx->hImgNextMonth, ctx->hImgNextMonthDis);
		_XCal_SetButtonIcon(ctx->hBtnNextYear, ctx->hImgNextYear, ctx->hImgNextYearDis);
		_XCal_SetSeparator(ctx, 0, headerSepY);

		if (ctx->doubleMonth){
			ctx->hTitleLeft = _XCal_CreateText(ctx, left + (kCal_PanelW - 120) / 2, navY, 120, kCal_NavSize, L"", ctx->colors.title);
			ctx->hTitleRight = _XCal_CreateText(ctx, rightStart + (kCal_PanelW - 120) / 2, navY, 120, kCal_NavSize, L"", ctx->colors.title);
			_XCal_BuildGrid(ctx, ctx->hCellsLeft, left, gridY, FALSE);
			_XCal_BuildGrid(ctx, ctx->hCellsRight, rightStart, gridY, TRUE);
		} else {
			ctx->hTitleLeft = _XCal_CreateText(ctx, left + (kCal_PanelW - 120) / 2, navY, 120, kCal_NavSize, L"", ctx->colors.title);
			_XCal_BuildGrid(ctx, ctx->hCellsLeft, left, gridY, FALSE);
		}

		_XCal_SetSeparator(ctx, 1, footerSepY);

		if (ctx->doubleMonth){
			const int botY = editY + kCal_EditH + kCal_SepGap;
			ctx->hEditStart = _XCal_CreateEdit(ctx, left, editY, editW, kCal_EditH, TRUE,
				&ctx->hEditWrapStart, &ctx->hSpinWrapStart, &ctx->hSpinStartUp, &ctx->hSpinStartDown);
			ctx->hEditEnd = _XCal_CreateEdit(ctx, left + editW + kCal_QuickGap, editY, editW, kCal_EditH, FALSE,
				&ctx->hEditWrapEnd, &ctx->hSpinWrapEnd, &ctx->hSpinEndUp, &ctx->hSpinEndDown);
			ctx->hRangeText = _XCal_CreateText(ctx, rightStart, editY, kCal_PanelW, kCal_EditH, L"",
				ctx->colors.rangeText, textAlignFlag_right | textAlignFlag_vcenter, TRUE);
			ctx->hBtnToday = _XCal_CreateCmdButton(ctx, left, botY, kCal_QuickW, kCal_QuickH, L"今天", _XCal_Cmd_Today, FALSE, TRUE);
			ctx->hBtnLast7 = _XCal_CreateCmdButton(ctx, left + kCal_QuickW + kCal_QuickGap, botY, kCal_QuickW, kCal_QuickH, L"近7天", _XCal_Cmd_Last7, FALSE, TRUE);
			ctx->hBtnLast15 = _XCal_CreateCmdButton(ctx, left + (kCal_QuickW + kCal_QuickGap) * 2, botY, kCal_QuickW, kCal_QuickH, L"近15天", _XCal_Cmd_Last15, FALSE, TRUE);
			ctx->hBtnLast30 = _XCal_CreateCmdButton(ctx, left + (kCal_QuickW + kCal_QuickGap) * 3, botY, kCal_QuickW, kCal_QuickH, L"近30天", _XCal_Cmd_Last30, FALSE, TRUE);
			ctx->hBtnCancel = _XCal_CreateCmdButton(ctx, cancelX, botY, kCal_CancelW, kCal_QuickH, L"取消", _XCal_Cmd_Cancel, FALSE, TRUE);
			ctx->hBtnConfirm = _XCal_CreateCmdButton(ctx, confirmX, botY, kCal_ConfirmW, kCal_QuickH, L"确认", _XCal_Cmd_Confirm, TRUE);
		} else {
			int edit2Y = editY + kCal_EditH + kCal_EditGap;
			int botY = edit2Y + kCal_EditH + kCal_SepGap;
			int rangeY = botY + kCal_QuickH + kCal_EditGap;
			int actionY = rangeY + kCal_QuickH + kCal_SepGap;
			ctx->hEditStart = _XCal_CreateEdit(ctx, left, editY, kCal_PanelW, kCal_EditH, TRUE,
				&ctx->hEditWrapStart, &ctx->hSpinWrapStart, &ctx->hSpinStartUp, &ctx->hSpinStartDown);
			ctx->hEditEnd = _XCal_CreateEdit(ctx, left, edit2Y, kCal_PanelW, kCal_EditH, FALSE,
				&ctx->hEditWrapEnd, &ctx->hSpinWrapEnd, &ctx->hSpinEndUp, &ctx->hSpinEndDown);
			ctx->hBtnToday = _XCal_CreateCmdButton(ctx, left, botY, kCal_QuickW, kCal_QuickH, L"今天", _XCal_Cmd_Today, FALSE, TRUE);
			ctx->hBtnLast7 = _XCal_CreateCmdButton(ctx, left + kCal_QuickW + kCal_QuickGap, botY, kCal_QuickW, kCal_QuickH, L"近7天", _XCal_Cmd_Last7, FALSE, TRUE);
			ctx->hBtnLast15 = _XCal_CreateCmdButton(ctx, left + (kCal_QuickW + kCal_QuickGap) * 2, botY, kCal_QuickW, kCal_QuickH, L"近15天", _XCal_Cmd_Last15, FALSE, TRUE);
			ctx->hBtnLast30 = _XCal_CreateCmdButton(ctx, left + (kCal_QuickW + kCal_QuickGap) * 3, botY, kCal_QuickW, kCal_QuickH, L"近30天", _XCal_Cmd_Last30, FALSE, TRUE);
			ctx->hRangeText = _XCal_CreateText(ctx, left, rangeY, kCal_PanelW, kCal_QuickH, L"",
				ctx->colors.rangeText, textAlignFlag_center | textAlignFlag_vcenter, TRUE);
			ctx->hBtnCancel = _XCal_CreateCmdButton(ctx, cancelX, actionY, kCal_CancelW, kCal_ActionH, L"取消", _XCal_Cmd_Cancel, FALSE, TRUE);
			ctx->hBtnConfirm = _XCal_CreateCmdButton(ctx, confirmX, actionY, kCal_ConfirmW, kCal_ActionH, L"确认", _XCal_Cmd_Confirm, TRUE);
		}
	}

	return ctx;
}

BOOL _XCal_Show(_XCal_Mode mode, BOOL doubleMonth, HWINDOW hParent,
	xcalendar_datetime_* pStart, xcalendar_datetime_* pEnd,
	BOOL bLimitMaxDate, const xcalendar_datetime_* pMaxDate, xuitool_theme_ theme,
	int cornerRadius)
{
	_XCal_Ctx* ctx = _XCal_CreateWindow(mode, doubleMonth, hParent, theme, cornerRadius);
	if (!ctx) return FALSE;
	ctx->limitMaxDate = bLimitMaxDate;
	if (pMaxDate && pMaxDate->year > 0){
		ctx->maxDate = _XCal_Normalize(*pMaxDate);
	}
	ctx->maxDate.hour = 23;
	ctx->maxDate.minute = 59;
	ctx->maxDate.second = 59;

	xcalendar_datetime_ todayStart = ctx->today;
	todayStart.hour = 0;
	todayStart.minute = 0;
	todayStart.second = 0;
	xcalendar_datetime_ todayEnd = ctx->today;
	todayEnd.hour = 23;
	todayEnd.minute = 59;
	todayEnd.second = 59;

	if (pStart && pStart->year > 0) ctx->selStart = _XCal_Normalize(*pStart);
	else ctx->selStart = todayStart;
	ctx->selStart = _XCal_ClampMaxDate(ctx, ctx->selStart, FALSE);

	if (mode == _XCal_Mode_Range){
		if (pEnd && pEnd->year > 0) ctx->selEnd = _XCal_Normalize(*pEnd);
		else ctx->selEnd = todayEnd;
		ctx->selEnd = _XCal_ClampMaxDate(ctx, ctx->selEnd, TRUE);
		if (_XCal_CompareFull(ctx->selStart, ctx->selEnd) > 0){
			xcalendar_datetime_ tmp = ctx->selStart;
			ctx->selStart = ctx->selEnd;
			ctx->selEnd = tmp;
		}
	} else {
		ctx->selStart.hour = 0;
		ctx->selStart.minute = 0;
		ctx->selStart.second = 0;
		ctx->selEnd = ctx->selStart;
	}

	ctx->showDate.year = ctx->selStart.year;
	ctx->showDate.month = ctx->selStart.month;
	ctx->showDate.day = 1;
	if (mode == _XCal_Mode_Range && ctx->doubleMonth){
		ctx->showDate.year = ctx->selEnd.year;
		ctx->showDate.month = ctx->selEnd.month;
		ctx->showDate.day = 1;
	}
	if (!_XCal_CanMoveToMonth(ctx, ctx->showDate)){
		ctx->showDate.year = ctx->maxDate.year;
		ctx->showDate.month = ctx->maxDate.month;
		ctx->showDate.day = 1;
	}

	_XCal_RefreshCalendar(ctx);
	_XCal_ApplyPopupPosition(ctx);
	XWnd_ShowWindow(ctx->hWnd, SW_SHOWNOACTIVATE);
	int result = XModalWnd_DoModal(ctx->hWnd);
	if (!ctx->confirmed) ctx->closing = TRUE;

	BOOL ok = (result == kCal_ResultOk && ctx->confirmed);
	xcalendar_datetime_ outStart = ctx->resultStart;
	xcalendar_datetime_ outEnd = ctx->resultEnd;
	HWINDOW hWnd = ctx->hWnd;
	BOOL destroyed = ctx->destroyed;
	_XCal_UnregisterCtx(ctx);

	if (!destroyed && XC_IsHWINDOW((HXCGUI)hWnd)){
		XWnd_DestroyWindow(hWnd);
	}
	_XCal_DestroyIcons(ctx);
	delete ctx;

	if (ok){
		if (pStart) *pStart = outStart;
		if (pEnd) *pEnd = outEnd;
	}
	return ok;
}

}  // anonymous namespace (CXCalendarCard internals)

//============================================================================
// CXCalendarCard 公开接口
//============================================================================

xcalendar_datetime_ CXCalendarCard::GetToday()
{
	return _XCal_Current();
}

CXText CXCalendarCard::FormatDateTime(xcalendar_datetime_ date)
{
	return _XCal_FormatDateTime(_XCal_Normalize(date));
}

CXText CXCalendarCard::FormatShortDate(xcalendar_datetime_ date)
{
	return _XCal_FormatShortDate(_XCal_Normalize(date));
}

const wchar_t* CXCalendarCard::FormatShortDatePtr(xcalendar_datetime_ date)
{
	static thread_local CXText s_text;
	s_text = FormatShortDate(date);
	return s_text.get();
}

const wchar_t* CXCalendarCard::FormatDateTimePtr(xcalendar_datetime_ date)
{
	static thread_local CXText s_text;
	s_text = FormatDateTime(date);
	return s_text.get();
}

void CXCalendarCard::SetBindEle(HELE hEle, int offsetX, int offsetY)
{
	auto& g = CG();
	g.popupMode = _XCal_PopupMode_Ele;
	g.bindEle = hEle;
	g.bindOffsetX = offsetX;
	g.bindOffsetY = offsetY;
}

void CXCalendarCard::SetPopupPosition(POINT pt)
{
	auto& g = CG();
	g.popupMode = _XCal_PopupMode_Pos;
	g.popupPt = pt;
	g.bindEle = NULL;
	g.bindOffsetX = 0;
	g.bindOffsetY = 0;
}

BOOL CXCalendarCard::PopupSingle(HWINDOW hParent, xcalendar_datetime_* pDate,
	BOOL bLimitMaxDate, xuitool_theme_ theme, const xcalendar_datetime_* pMaxDate,
	int nCornerRadius)
{
	xcalendar_datetime_ d = pDate ? *pDate : xcalendar_datetime_{};
	BOOL ok = _XCal_Show(_XCal_Mode_Single, FALSE, hParent, &d, NULL,
		bLimitMaxDate, pMaxDate, theme, nCornerRadius);
	if (ok && pDate) *pDate = d;
	return ok;
}

BOOL CXCalendarCard::PopupDouble(HWINDOW hParent, xcalendar_datetime_* pStart, xcalendar_datetime_* pEnd,
	BOOL bLimitMaxDate, xuitool_theme_ theme, const xcalendar_datetime_* pMaxDate,
	int nCornerRadius)
{
	xcalendar_datetime_ s = pStart ? *pStart : xcalendar_datetime_{};
	xcalendar_datetime_ e = pEnd ? *pEnd : xcalendar_datetime_{};
	BOOL ok = _XCal_Show(_XCal_Mode_Range, TRUE, hParent, &s, &e,
		bLimitMaxDate, pMaxDate, theme, nCornerRadius);
	if (ok){
		if (pStart) *pStart = s;
		if (pEnd) *pEnd = e;
	}
	return ok;
}
