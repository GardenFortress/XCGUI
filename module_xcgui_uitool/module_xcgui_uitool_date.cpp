//============================================================================
// module_xcgui_uitool_date.cpp — CXDate 实现
// 依赖同 TU 中 calendar.cpp 的 _XCal_* 主题 / 日期算法 / 图标加载.
//============================================================================
//============================================================================
// CXDate — 含日期时左月历 + 右时分秒滚动; 仅时间时只显示滚动列
//============================================================================

namespace {

constexpr int kDate_ResultCancel = 0;
constexpr int kDate_ResultOk     = 1;

constexpr int kDate_TimeColW     = 56;
constexpr int kDate_TimeItemH    = 40;
constexpr int kDate_TimeVisible  = 5;   // 奇数, 中间为选中项
constexpr int kDate_TimeColH     = kDate_TimeItemH * kDate_TimeVisible;
constexpr int kDate_CalTimeGap   = 20;
constexpr int kDate_PreviewW     = 168;
constexpr int kDate_ColHour      = 0;
constexpr int kDate_ColMinute    = 1;
constexpr int kDate_ColSecond    = 2;

enum _XDate_PopupMode
{
	_XDate_PopupMode_Default = 0,
	_XDate_PopupMode_Ele,
	_XDate_PopupMode_Pos,
};

enum _XDate_Cmd
{
	_XDate_Cmd_None = 0,
	_XDate_Cmd_DateCell,
	_XDate_Cmd_PrevYear,
	_XDate_Cmd_PrevMonth,
	_XDate_Cmd_NextMonth,
	_XDate_Cmd_NextYear,
	_XDate_Cmd_Confirm,
	_XDate_Cmd_Cancel,
	_XDate_Cmd_Now,
};

struct _XDate_Ctx
{
	xuitool_theme_ theme = xuitool_theme_auto;
	_XCal_ThemeColors colors{};
	xdate_format_ format = xdate_format_datetime;

	HWINDOW hWnd = NULL;
	HWND hParent = NULL;
	int contentW = 0;
	int contentH = 0;
	int contentOffX = _XUITool::kShadowMargin;
	int contentOffY = _XUITool::kShadowMargin;
	int cornerRadius = kCal_CornerRadius;
	int sepHeaderY = -1;
	int sepFooterY = -1;
	int sepTimeX = -1;
	int sepColX[2]{ -1, -1 };

	HELE hBtnPrevYear = NULL;
	HELE hBtnPrevMonth = NULL;
	HELE hBtnNextMonth = NULL;
	HELE hBtnNextYear = NULL;
	HELE hBtnConfirm = NULL;
	HELE hBtnCancel = NULL;
	HELE hBtnNow = NULL;
	HELE hActionWrap = NULL;
	HELE hTimeCol[3]{};
	HELE hCells[42]{};
	HXCGUI hTitle = NULL;
	HXCGUI hWeekLabels[7]{};
	HXCGUI hTimeHeaders[3]{};
	HXCGUI hPreview = NULL;

	HFONTX hFont = NULL;
	HSVG hSvgLastYear = NULL;
	HSVG hSvgNextYear = NULL;
	HSVG hSvgLastMonth = NULL;
	HSVG hSvgNextMonth = NULL;
	HSVG hSvgLastYearDis = NULL;
	HSVG hSvgNextYearDis = NULL;
	HSVG hSvgLastMonthDis = NULL;
	HSVG hSvgNextMonthDis = NULL;
	HIMAGE hImgLastYear = NULL;
	HIMAGE hImgNextYear = NULL;
	HIMAGE hImgLastMonth = NULL;
	HIMAGE hImgNextMonth = NULL;
	HIMAGE hImgLastYearDis = NULL;
	HIMAGE hImgNextYearDis = NULL;
	HIMAGE hImgLastMonthDis = NULL;
	HIMAGE hImgNextMonthDis = NULL;

	xcalendar_datetime_ today{};
	xcalendar_datetime_ showDate{};
	xcalendar_datetime_ selDate{};
	xcalendar_datetime_ resultDate{};
	BOOL limitMaxDate = FALSE;
	xcalendar_datetime_ maxDate{};
	BOOL confirmed = FALSE;
	BOOL closing = FALSE;
	BOOL destroyed = FALSE;
	BOOL updatingUI = FALSE;

	int dragCol = -1;
	int dragLastY = 0;
	int dragAcc = 0;
	BOOL dragMoved = FALSE;

	BOOL liveBind = FALSE;
	HELE hBindEdit = NULL;
	xcalendar_datetime_ openDate{};
	HWND hwnd = NULL;
	WNDPROC oldWndProc = NULL;
};

struct _XDate_Bind
{
	xdate_format_ format = xdate_format_datetime;
	BOOL limitMaxDate = FALSE;
	xuitool_theme_ theme = xuitool_theme_auto;
	xcalendar_datetime_ maxDate{};
	BOOL hasMaxDate = FALSE;
	int cornerRadius = 8;
	int offsetX = 0;
	int offsetY = 4;
	BOOL isCombo = FALSE;
	BOOL eventsHooked = FALSE;
	xcalendar_datetime_ value{};
	int focusField = kCal_FieldNone;
	int typeCount = 0;
	int typeValue = 0;
	BOOL updatingUI = FALSE;
};

struct _XDate_Global
{
	std::unordered_map<HWINDOW, _XDate_Ctx*> windows;
	std::unordered_map<HELE, _XDate_Ctx*> elements;
	std::unordered_map<HELE, _XDate_Cmd> commands;
	std::unordered_map<HELE, _XDate_Bind> binds;
	_XDate_PopupMode popupMode = _XDate_PopupMode_Default;
	HELE bindEle = NULL;
	int bindOffsetX = 0;
	int bindOffsetY = 0;
	POINT popupPt{0, 0};
	BOOL opening = FALSE;
	_XDate_Ctx* livePopup = NULL;
	HELE liveBindEle = NULL;
	BOOL suspendClose = FALSE;
};

_XDate_Global& DG()
{
	static _XDate_Global s;
	return s;
}

inline int _XDate_Mod(int v, int n)
{
	if (n <= 0) return 0;
	int r = v % n;
	return r < 0 ? r + n : r;
}

inline xdate_format_ _XDate_NormFormat(xdate_format_ f)
{
	if (f == xdate_format_date_hm || f == xdate_format_hms || f == xdate_format_hm)
		return f;
	return xdate_format_datetime;
}

inline BOOL _XDate_HasDate(xdate_format_ f)
{
	f = _XDate_NormFormat(f);
	return (f == xdate_format_datetime || f == xdate_format_date_hm) ? TRUE : FALSE;
}

inline BOOL _XDate_HasSecond(xdate_format_ f)
{
	f = _XDate_NormFormat(f);
	return (f == xdate_format_datetime || f == xdate_format_hms) ? TRUE : FALSE;
}

inline int _XDate_PreviewWidth(xdate_format_ f)
{
	switch (_XDate_NormFormat(f)){
	case xdate_format_date_hm: return 148;
	case xdate_format_hms:     return 80;
	case xdate_format_hm:      return 56;
	default:                   return kDate_PreviewW;
	}
}

inline int _XDate_ColCount(const _XDate_Ctx* ctx)
{
	return (ctx && _XDate_HasSecond(ctx->format)) ? 3 : 2;
}

struct _XDate_FieldSpan
{
	int field;
	int selBegin;
	int selEnd;
	int posBegin;
	int posEnd;
};

const _XDate_FieldSpan* _XDate_Fields(xdate_format_ format, int* count)
{
	static const _XDate_FieldSpan kDateTime[] = {
		{ kCal_FieldYear,   0,  4,  0,  4 },
		{ kCal_FieldMonth,  5,  7,  5,  7 },
		{ kCal_FieldDay,    8, 10,  8, 10 },
		{ kCal_FieldHour,  11, 13, 11, 13 },
		{ kCal_FieldMinute,14, 16, 14, 16 },
		{ kCal_FieldSecond,17, 19, 17, 19 },
	};
	static const _XDate_FieldSpan kDateHm[] = {
		{ kCal_FieldYear,   0,  4,  0,  4 },
		{ kCal_FieldMonth,  5,  7,  5,  7 },
		{ kCal_FieldDay,    8, 10,  8, 10 },
		{ kCal_FieldHour,  11, 13, 11, 13 },
		{ kCal_FieldMinute,14, 16, 14, 16 },
	};
	static const _XDate_FieldSpan kHms[] = {
		{ kCal_FieldHour,   0,  2,  0,  2 },
		{ kCal_FieldMinute, 3,  5,  3,  5 },
		{ kCal_FieldSecond, 6,  8,  6,  8 },
	};
	static const _XDate_FieldSpan kHm[] = {
		{ kCal_FieldHour,   0,  2,  0,  2 },
		{ kCal_FieldMinute, 3,  5,  3,  5 },
	};
	format = _XDate_NormFormat(format);
	switch (format){
	case xdate_format_date_hm:
		if (count) *count = (int)_countof(kDateHm);
		return kDateHm;
	case xdate_format_hms:
		if (count) *count = (int)_countof(kHms);
		return kHms;
	case xdate_format_hm:
		if (count) *count = (int)_countof(kHm);
		return kHm;
	default:
		if (count) *count = (int)_countof(kDateTime);
		return kDateTime;
	}
}

int _XDate_DefaultField(xdate_format_ format)
{
	int n = 0;
	const _XDate_FieldSpan* sp = _XDate_Fields(format, &n);
	return (sp && n > 0) ? sp[n - 1].field : kCal_FieldNone;
}

int _XDate_FieldFromPos(xdate_format_ format, int pos)
{
	int n = 0;
	const _XDate_FieldSpan* sp = _XDate_Fields(format, &n);
	if (!sp || n <= 0) return kCal_FieldNone;
	if (pos < sp[0].posBegin) return sp[0].field;
	for (int i = 0; i < n; ++i){
		if (pos >= sp[i].posBegin && pos <= sp[i].posEnd) return sp[i].field;
	}
	return sp[n - 1].field;
}

void _XDate_SelectField(HELE hEdit, xdate_format_ format, int field)
{
	if (!hEdit) return;
	int n = 0;
	const _XDate_FieldSpan* sp = _XDate_Fields(format, &n);
	if (!sp) return;
	for (int i = 0; i < n; ++i){
		if (sp[i].field == field){
			XEdit_SetSelect(hEdit, 0, sp[i].selBegin, 0, sp[i].selEnd);
			return;
		}
	}
}

int _XDate_NeighborField(xdate_format_ format, int field, int dir)
{
	int n = 0;
	const _XDate_FieldSpan* sp = _XDate_Fields(format, &n);
	if (!sp || n <= 0) return field;
	int idx = 0;
	for (int i = 0; i < n; ++i){
		if (sp[i].field == field){ idx = i; break; }
	}
	idx += dir;
	if (idx < 0) idx = 0;
	if (idx >= n) idx = n - 1;
	return sp[idx].field;
}

int _XDate_FirstField(xdate_format_ format)
{
	int n = 0;
	const _XDate_FieldSpan* sp = _XDate_Fields(format, &n);
	return (sp && n > 0) ? sp[0].field : kCal_FieldNone;
}

int _XDate_FieldWidth(int field)
{
	return (field == kCal_FieldYear) ? 4 : 2;
}

int _XDate_FieldMin(int field)
{
	if (field == kCal_FieldYear) return 1900;
	if (field == kCal_FieldMonth || field == kCal_FieldDay) return 1;
	return 0;
}

int _XDate_FieldMax(const xcalendar_datetime_& d, int field)
{
	switch (field){
	case kCal_FieldYear:   return 9999;
	case kCal_FieldMonth:  return 12;
	case kCal_FieldDay:    return _XCal_DaysInMonth(d.year, d.month);
	case kCal_FieldHour:   return 23;
	default:               return 59;
	}
}

void _XDate_SetFieldValue(xcalendar_datetime_* d, int field, int v)
{
	if (!d) return;
	switch (field){
	case kCal_FieldYear:   d->year = v; break;
	case kCal_FieldMonth:  d->month = v; break;
	case kCal_FieldDay:    d->day = v; break;
	case kCal_FieldHour:   d->hour = v; break;
	case kCal_FieldMinute: d->minute = v; break;
	default:               d->second = v; break;
	}
	*d = _XCal_Normalize(*d);
}

inline int _XDate_ColMax(int col)
{
	return (col == kDate_ColHour) ? 24 : 60;
}

int _XDate_ColValue(const _XDate_Ctx* ctx, int col)
{
	if (!ctx) return 0;
	if (col == kDate_ColHour) return ctx->selDate.hour;
	if (col == kDate_ColMinute) return ctx->selDate.minute;
	return ctx->selDate.second;
}

void _XDate_SetColValue(_XDate_Ctx* ctx, int col, int v)
{
	if (!ctx) return;
	v = _XDate_Mod(v, _XDate_ColMax(col));
	if (col == kDate_ColHour) ctx->selDate.hour = v;
	else if (col == kDate_ColMinute) ctx->selDate.minute = v;
	else ctx->selDate.second = v;
}

_XDate_Ctx* _XDate_FindByEle(HELE hEle)
{
	auto& g = DG();
	auto it = g.elements.find(hEle);
	return it == g.elements.end() ? NULL : it->second;
}

int CALLBACK _XDate_OnPaintWindow(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled);
int CALLBACK _XDate_OnWndClose(HWINDOW hWnd, BOOL* pbHandled);
int CALLBACK _XDate_OnWndDestroy(HWINDOW hWnd, BOOL* pbHandled);
void _XDate_ClosePopup(_XDate_Ctx* ctx, int result);
void _XDate_DestroyLivePopup(BOOL revert);
void _XDate_SyncLiveToEdit(_XDate_Ctx* ctx);
void _XDate_RestoreBindFocus(_XDate_Ctx* ctx);
void _XDate_ApplySelToUI(_XDate_Ctx* ctx);
void _XDate_Unsubclass(_XDate_Ctx* ctx);
void _XDate_SyncPopupFromBind(HELE hEle, _XDate_Bind* b);
void _XDate_ApplyNoActivate(_XDate_Ctx* ctx);

void _XDate_RegisterEle(_XDate_Ctx* ctx, HELE hEle)
{
	if (ctx && hEle) DG().elements[hEle] = ctx;
}

void _XDate_UnregisterCtx(_XDate_Ctx* ctx)
{
	auto& g = DG();
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
			XWnd_RemoveEventC(ctx->hWnd, WM_PAINT,   (void*)&_XDate_OnPaintWindow);
			XWnd_RemoveEventC(ctx->hWnd, WM_CLOSE,   (void*)&_XDate_OnWndClose);
			XWnd_RemoveEventC(ctx->hWnd, XE_DESTROY, (void*)&_XDate_OnWndDestroy);
		}
		g.windows.erase(ctx->hWnd);
	}
}

void _XDate_ApplyPopupPosition(_XDate_Ctx* ctx)
{
	if (!ctx || !ctx->hWnd || !XC_IsHWINDOW((HXCGUI)ctx->hWnd)) return;
	auto& g = DG();
	if (g.popupMode == _XDate_PopupMode_Pos){
		XWnd_SetPosition(ctx->hWnd, g.popupPt.x, g.popupPt.y);
		XWnd_AdjustInScreen(ctx->hWnd, 0, FALSE);
		g.popupMode = _XDate_PopupMode_Default;
		return;
	}
	if (g.popupMode != _XDate_PopupMode_Ele) return;

	HELE hEle = g.bindEle;
	int offX = g.bindOffsetX;
	int offY = g.bindOffsetY;
	g.popupMode = _XDate_PopupMode_Default;
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

BOOL _XDate_IsAfterMaxDate(const _XDate_Ctx* ctx, const xcalendar_datetime_& d)
{
	if (!ctx || !ctx->limitMaxDate || !_XDate_HasDate(ctx->format)) return FALSE;
	return _XCal_CompareDate(d, ctx->maxDate) > 0;
}

xcalendar_datetime_ _XDate_ClampMaxDate(const _XDate_Ctx* ctx, xcalendar_datetime_ d)
{
	d = _XCal_Normalize(d);
	if (_XDate_IsAfterMaxDate(ctx, d)){
		int h = d.hour, m = d.minute, s = d.second;
		d = ctx->maxDate;
		d.hour = h;
		d.minute = m;
		d.second = s;
		d = _XCal_Normalize(d);
	}
	if (!ctx || _XDate_HasSecond(ctx->format)) return d;
	d.second = 0;
	return d;
}

BOOL _XDate_CanMoveToMonth(const _XDate_Ctx* ctx, xcalendar_datetime_ showDate)
{
	if (!ctx || !ctx->limitMaxDate) return TRUE;
	if (showDate.year > ctx->maxDate.year) return FALSE;
	if (showDate.year == ctx->maxDate.year && showDate.month > ctx->maxDate.month) return FALSE;
	return TRUE;
}

CXText _XDate_Format(const xcalendar_datetime_& d, xdate_format_ format)
{
	wchar_t buf[32]{};
	switch (_XDate_NormFormat(format)){
	case xdate_format_date_hm:
		swprintf_s(buf, _countof(buf), L"%04d-%02d-%02d %02d:%02d",
			d.year, d.month, d.day, d.hour, d.minute);
		break;
	case xdate_format_hms:
		swprintf_s(buf, _countof(buf), L"%02d:%02d:%02d",
			d.hour, d.minute, d.second);
		break;
	case xdate_format_hm:
		swprintf_s(buf, _countof(buf), L"%02d:%02d",
			d.hour, d.minute);
		break;
	default:
		swprintf_s(buf, _countof(buf), L"%04d-%02d-%02d %02d:%02d:%02d",
			d.year, d.month, d.day, d.hour, d.minute, d.second);
		break;
	}
	CXText text;
	text = buf;
	return text;
}

void _XDate_LoadIcons(_XDate_Ctx* ctx)
{
	if (!ctx) return;
	BOOL light = _XUITool::IsLightTheme(ctx->theme);
	COLORREF navColor = RGBA(0x45, 0x4D, 0x5A, 0xFF);
	COLORREF navDisabledColor = light ? RGBA(0x99, 0x99, 0x99, 0xFF) : RGBA(0x7D, 0x7E, 0x7F, 0xFF);
	ctx->hImgLastYear = _XCal_LoadSvgIcon(kCalSvg_LastYear, 24, light, navColor, &ctx->hSvgLastYear);
	ctx->hImgNextYear = _XCal_LoadSvgIcon(kCalSvg_NextYear, 24, light, navColor, &ctx->hSvgNextYear);
	ctx->hImgLastMonth = _XCal_LoadSvgIcon(kCalSvg_LastMonth, 24, light, navColor, &ctx->hSvgLastMonth);
	ctx->hImgNextMonth = _XCal_LoadSvgIcon(kCalSvg_NextMonth, 24, light, navColor, &ctx->hSvgNextMonth);
	ctx->hImgLastYearDis = _XCal_LoadSvgIcon(kCalSvg_LastYear, 24, TRUE, navDisabledColor, &ctx->hSvgLastYearDis);
	ctx->hImgNextYearDis = _XCal_LoadSvgIcon(kCalSvg_NextYear, 24, TRUE, navDisabledColor, &ctx->hSvgNextYearDis);
	ctx->hImgLastMonthDis = _XCal_LoadSvgIcon(kCalSvg_LastMonth, 24, TRUE, navDisabledColor, &ctx->hSvgLastMonthDis);
	ctx->hImgNextMonthDis = _XCal_LoadSvgIcon(kCalSvg_NextMonth, 24, TRUE, navDisabledColor, &ctx->hSvgNextMonthDis);
}

void _XDate_DestroyIcons(_XDate_Ctx* ctx)
{
	if (!ctx) return;
	if (ctx->hImgLastYear){ XImage_Release(ctx->hImgLastYear); ctx->hImgLastYear = NULL; }
	if (ctx->hImgNextYear){ XImage_Release(ctx->hImgNextYear); ctx->hImgNextYear = NULL; }
	if (ctx->hImgLastMonth){ XImage_Release(ctx->hImgLastMonth); ctx->hImgLastMonth = NULL; }
	if (ctx->hImgNextMonth){ XImage_Release(ctx->hImgNextMonth); ctx->hImgNextMonth = NULL; }
	if (ctx->hImgLastYearDis){ XImage_Release(ctx->hImgLastYearDis); ctx->hImgLastYearDis = NULL; }
	if (ctx->hImgNextYearDis){ XImage_Release(ctx->hImgNextYearDis); ctx->hImgNextYearDis = NULL; }
	if (ctx->hImgLastMonthDis){ XImage_Release(ctx->hImgLastMonthDis); ctx->hImgLastMonthDis = NULL; }
	if (ctx->hImgNextMonthDis){ XImage_Release(ctx->hImgNextMonthDis); ctx->hImgNextMonthDis = NULL; }
	if (ctx->hSvgLastYear){ XSvg_Destroy(ctx->hSvgLastYear); ctx->hSvgLastYear = NULL; }
	if (ctx->hSvgNextYear){ XSvg_Destroy(ctx->hSvgNextYear); ctx->hSvgNextYear = NULL; }
	if (ctx->hSvgLastMonth){ XSvg_Destroy(ctx->hSvgLastMonth); ctx->hSvgLastMonth = NULL; }
	if (ctx->hSvgNextMonth){ XSvg_Destroy(ctx->hSvgNextMonth); ctx->hSvgNextMonth = NULL; }
	if (ctx->hSvgLastYearDis){ XSvg_Destroy(ctx->hSvgLastYearDis); ctx->hSvgLastYearDis = NULL; }
	if (ctx->hSvgNextYearDis){ XSvg_Destroy(ctx->hSvgNextYearDis); ctx->hSvgNextYearDis = NULL; }
	if (ctx->hSvgLastMonthDis){ XSvg_Destroy(ctx->hSvgLastMonthDis); ctx->hSvgLastMonthDis = NULL; }
	if (ctx->hSvgNextMonthDis){ XSvg_Destroy(ctx->hSvgNextMonthDis); ctx->hSvgNextMonthDis = NULL; }
}

int CALLBACK _XDate_OnPaintWindow(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	if (!hWnd || !hDraw) return 0;
	auto it = DG().windows.find(hWnd);
	_XDate_Ctx* ctx = (it != DG().windows.end()) ? it->second : NULL;
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

	auto drawHSep = [&](int y){
		if (y < 0) return;
		RECT rc{ bodyRc.left, bodyRc.top + y, bodyRc.right, bodyRc.top + y + 1 };
		XDraw_SetBrushColor(hDraw, ctx->colors.separator);
		XDraw_FillRect(hDraw, &rc);
	};
	auto drawVSep = [&](int x, int y0, int y1){
		if (x < 0) return;
		RECT rc{ bodyRc.left + x, bodyRc.top + y0, bodyRc.left + x + 1, bodyRc.top + y1 };
		XDraw_SetBrushColor(hDraw, ctx->colors.separator);
		XDraw_FillRect(hDraw, &rc);
	};
	drawHSep(ctx->sepHeaderY);
	drawHSep(ctx->sepFooterY);
	int timeTop = (ctx->sepHeaderY >= 0) ? (ctx->sepHeaderY + 1) : kCal_PadV;
	int timeBot = (ctx->sepFooterY >= 0) ? ctx->sepFooterY : (ctx->contentH - kCal_PadV);
	drawVSep(ctx->sepTimeX, timeTop, timeBot);
	drawVSep(ctx->sepColX[0], timeTop, timeBot);
	drawVSep(ctx->sepColX[1], timeTop, timeBot);

	_XUITool::DrawBodyBorder(hDraw, bodyRc, ctx->cornerRadius, ctx->colors.border,
		_XUITool::WindowDpiScale(hWnd));
	return 0;
}

void _XDate_EndModal(_XDate_Ctx* ctx, int result)
{
	_XDate_ClosePopup(ctx, result);
}

void _XDate_ApplyCellStyle(_XDate_Ctx* ctx, HELE hCell, const xcalendar_datetime_& d, int month, BOOL enabled)
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
	BOOL selected = (_XCal_CompareDate(d, ctx->selDate) == 0);
	if (selected){
		XEle_SetBkInfo(hCell, c.bkPrimary);
		XEle_SetTextColor(hCell, c.accentText);
	} else {
		XEle_SetBkInfo(hCell, c.bkNormal);
		XEle_SetTextColor(hCell, (d.month == month) ? c.text : c.mutedText);
	}
}

void _XDate_RefreshPreview(_XDate_Ctx* ctx)
{
	if (!ctx || !ctx->hPreview) return;
	CXText t = _XDate_Format(ctx->selDate, ctx->format);
	XShapeText_SetText(ctx->hPreview, t.get());
	XShapeText_SetTextColor(ctx->hPreview, ctx->colors.rangeText);
}

void _XDate_RefreshCalendar(_XDate_Ctx* ctx);

void _XDate_RefreshTime(_XDate_Ctx* ctx)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	ctx->selDate = _XDate_ClampMaxDate(ctx, ctx->selDate);
	_XDate_RefreshPreview(ctx);
	int n = _XDate_ColCount(ctx);
	for (int i = 0; i < n; ++i){
		if (ctx->hTimeCol[i]) XEle_Redraw(ctx->hTimeCol[i], FALSE);
	}
	if (!ctx->updatingUI) _XDate_SyncLiveToEdit(ctx);
}

void _XDate_RefreshCalendar(_XDate_Ctx* ctx)
{
	if (!ctx || ctx->closing || ctx->destroyed || !XC_IsHWINDOW((HXCGUI)ctx->hWnd)) return;
	ctx->selDate = _XDate_ClampMaxDate(ctx, ctx->selDate);
	if (!_XDate_HasDate(ctx->format)){
		_XDate_RefreshPreview(ctx);
		XWnd_Redraw(ctx->hWnd);
		if (!ctx->updatingUI) _XDate_SyncLiveToEdit(ctx);
		return;
	}

	int y = ctx->showDate.year;
	int m = ctx->showDate.month;
	for (int i = 0; i < 42; ++i){
		xcalendar_datetime_ d = _XCal_DateFromIndex(i, y, m);
		d.hour = ctx->selDate.hour;
		d.minute = ctx->selDate.minute;
		d.second = ctx->selDate.second;
		wchar_t dayBuf[8]{};
		swprintf_s(dayBuf, _countof(dayBuf), L"%d", d.day);
		XBtn_SetText(ctx->hCells[i], dayBuf);
		XEle_SetUserData(ctx->hCells[i], _XCal_DateUserData(d, 0));
		BOOL enabled = (d.month == m && !_XDate_IsAfterMaxDate(ctx, d));
		_XDate_ApplyCellStyle(ctx, ctx->hCells[i], d, m, enabled);
	}
	if (ctx->hTitle){
		wchar_t buf[32]{};
		swprintf_s(buf, _countof(buf), L"%04d年%d月", y, m);
		XShapeText_SetText(ctx->hTitle, buf);
		XShapeText_SetTextColor(ctx->hTitle, ctx->colors.title);
	}
	if (ctx->hBtnNextMonth){
		XEle_Enable(ctx->hBtnNextMonth, _XDate_CanMoveToMonth(ctx, _XCal_AddMonths(ctx->showDate, 1)));
	}
	if (ctx->hBtnNextYear){
		xcalendar_datetime_ nextYear = ctx->showDate;
		++nextYear.year;
		nextYear = _XCal_Normalize(nextYear);
		XEle_Enable(ctx->hBtnNextYear, _XDate_CanMoveToMonth(ctx, nextYear));
	}
	_XDate_RefreshPreview(ctx);
	XWnd_Redraw(ctx->hWnd);
	if (!ctx->updatingUI) _XDate_SyncLiveToEdit(ctx);
}

void _XDate_MoveMonth(_XDate_Ctx* ctx, int delta)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	xcalendar_datetime_ next = _XCal_AddMonths(ctx->showDate, delta);
	if (!_XDate_CanMoveToMonth(ctx, next)) return;
	ctx->showDate = next;
	_XDate_RefreshCalendar(ctx);
}

void _XDate_MoveYear(_XDate_Ctx* ctx, int delta)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	xcalendar_datetime_ next = ctx->showDate;
	next.year += delta;
	next = _XCal_Normalize(next);
	if (!_XDate_CanMoveToMonth(ctx, next)) return;
	ctx->showDate = next;
	_XDate_RefreshCalendar(ctx);
}

void _XDate_OnDateCell(_XDate_Ctx* ctx, HELE hEle)
{
	if (!ctx || !hEle || ctx->closing || ctx->destroyed) return;
	xcalendar_datetime_ clicked = _XCal_DateFromUserData(XEle_GetUserData(hEle));
	if (_XDate_IsAfterMaxDate(ctx, clicked)) return;
	clicked.hour = ctx->selDate.hour;
	clicked.minute = ctx->selDate.minute;
	clicked.second = ctx->selDate.second;
	ctx->selDate = _XDate_ClampMaxDate(ctx, clicked);
	_XDate_RefreshCalendar(ctx);
}

void _XDate_AdjustTime(_XDate_Ctx* ctx, int col, int delta)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	if (col < 0 || col >= _XDate_ColCount(ctx)) return;
	_XDate_SetColValue(ctx, col, _XDate_ColValue(ctx, col) + delta);
	ctx->selDate = _XDate_ClampMaxDate(ctx, ctx->selDate);
	_XDate_RefreshTime(ctx);
}

int CALLBACK _XDate_OnBtnClick(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XDate_Ctx* ctx = _XDate_FindByEle(hEle);
	if (!ctx || ctx->closing || ctx->destroyed) return 0;
	_XDate_Cmd cmd = _XDate_Cmd_None;
	auto cmdIt = DG().commands.find(hEle);
	if (cmdIt != DG().commands.end()) cmd = cmdIt->second;
	switch (cmd){
	case _XDate_Cmd_DateCell:  _XDate_OnDateCell(ctx, hEle); break;
	case _XDate_Cmd_PrevYear:  _XDate_MoveYear(ctx, -1); break;
	case _XDate_Cmd_PrevMonth: _XDate_MoveMonth(ctx, -1); break;
	case _XDate_Cmd_NextMonth: _XDate_MoveMonth(ctx, 1); break;
	case _XDate_Cmd_NextYear:  _XDate_MoveYear(ctx, 1); break;
	case _XDate_Cmd_Confirm:
		ctx->confirmed = TRUE;
		ctx->resultDate = _XDate_ClampMaxDate(ctx, ctx->selDate);
		_XDate_ClosePopup(ctx, kDate_ResultOk);
		break;
	case _XDate_Cmd_Cancel:
		ctx->confirmed = FALSE;
		_XDate_ClosePopup(ctx, kDate_ResultCancel);
		break;
	case _XDate_Cmd_Now: {
		xcalendar_datetime_ now = _XCal_Current();
		if (!_XDate_HasSecond(ctx->format)) now.second = 0;
		ctx->today = now;
		ctx->selDate = _XDate_ClampMaxDate(ctx, now);
		_XDate_ApplySelToUI(ctx);
		_XDate_RestoreBindFocus(ctx);
		break;
	}
	default:
		break;
	}
	return 0;
}

int _XDate_HitOffset(int y, int height)
{
	int mid = height / 2;
	int rel = y - mid;
	if (rel >= 0) return (rel + kDate_TimeItemH / 2) / kDate_TimeItemH;
	return (rel - kDate_TimeItemH / 2) / kDate_TimeItemH;
}

int CALLBACK _XDate_OnTimePaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XDate_Ctx* ctx = _XDate_FindByEle(hEle);
	if (!ctx || !hDraw || ctx->destroyed) return 0;

	int col = (int)XEle_GetUserData(hEle);
	int maxV = _XDate_ColMax(col);
	int cur = _XDate_ColValue(ctx, col);

	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0) return 0;

	XDraw_EnableSmoothingMode(hDraw, TRUE);
	int midY = (rc.top + rc.bottom) / 2;
	int bandT = midY - kDate_TimeItemH / 2;
	int bandB = bandT + kDate_TimeItemH;
	RECT band{ rc.left + 4, bandT, rc.right - 4, bandB };
	XDraw_SetBrushColor(hDraw, ctx->colors.title);
	XDraw_FillRoundRect(hDraw, &band, 4, 4);

	if (ctx->hFont) XDraw_SetFont(hDraw, ctx->hFont);
	XDraw_SetTextAlign(hDraw, textAlignFlag_center | textAlignFlag_vcenter);
	const int half = kDate_TimeVisible / 2;
	for (int i = -half; i <= half; ++i){
		int v = _XDate_Mod(cur + i, maxV);
		wchar_t buf[8]{};
		swprintf_s(buf, _countof(buf), L"%02d", v);
		RECT itemRc{
			rc.left,
			midY - kDate_TimeItemH / 2 + i * kDate_TimeItemH,
			rc.right,
			midY + kDate_TimeItemH / 2 + i * kDate_TimeItemH
		};
		COLORREF color = ctx->colors.mutedText;
		if (i == 0) color = ctx->colors.accentText;
		else if (i == -1 || i == 1) color = ctx->colors.text;
		XDraw_SetBrushColor(hDraw, color);
		XDraw_DrawText(hDraw, buf, 2, &itemRc);
	}
	return 0;
}

int CALLBACK _XDate_OnTimeWheel(HELE hEle, UINT nFlags, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XDate_Ctx* ctx = _XDate_FindByEle(hEle);
	if (!ctx || ctx->closing || ctx->destroyed) return 0;
	int col = (int)XEle_GetUserData(hEle);
	short delta = GET_WHEEL_DELTA_WPARAM(nFlags);
	_XDate_AdjustTime(ctx, col, (delta > 0) ? -1 : 1);
	return 0;
}

int CALLBACK _XDate_OnTimeLButtonDown(HELE hEle, UINT, POINT* pPt, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XDate_Ctx* ctx = _XDate_FindByEle(hEle);
	if (!ctx || !pPt || ctx->closing || ctx->destroyed) return 0;
	ctx->dragCol = (int)XEle_GetUserData(hEle);
	ctx->dragLastY = pPt->y;
	ctx->dragAcc = 0;
	ctx->dragMoved = FALSE;
	XEle_SetCapture(hEle, TRUE);
	return 0;
}

int CALLBACK _XDate_OnTimeMouseMove(HELE hEle, UINT, POINT* pPt, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XDate_Ctx* ctx = _XDate_FindByEle(hEle);
	if (!ctx || !pPt || ctx->dragCol < 0 || ctx->closing || ctx->destroyed) return 0;
	int dy = pPt->y - ctx->dragLastY;
	ctx->dragLastY = pPt->y;
	ctx->dragAcc += dy;
	if (ctx->dragAcc <= -4 || ctx->dragAcc >= 4) ctx->dragMoved = TRUE;
	while (ctx->dragAcc <= -kDate_TimeItemH){
		_XDate_AdjustTime(ctx, ctx->dragCol, 1);
		ctx->dragAcc += kDate_TimeItemH;
	}
	while (ctx->dragAcc >= kDate_TimeItemH){
		_XDate_AdjustTime(ctx, ctx->dragCol, -1);
		ctx->dragAcc -= kDate_TimeItemH;
	}
	if (pbHandled) *pbHandled = TRUE;
	return 0;
}

int CALLBACK _XDate_OnTimeLButtonUp(HELE hEle, UINT, POINT* pPt, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XDate_Ctx* ctx = _XDate_FindByEle(hEle);
	XEle_SetCapture(hEle, FALSE);
	if (!ctx || ctx->closing || ctx->destroyed) return 0;
	int col = ctx->dragCol;
	BOOL moved = ctx->dragMoved;
	ctx->dragCol = -1;
	ctx->dragAcc = 0;
	ctx->dragMoved = FALSE;
	if (moved || !pPt) return 0;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	int offset = _XDate_HitOffset(pPt->y, rc.bottom - rc.top);
	if (offset != 0) _XDate_AdjustTime(ctx, col, offset);
	return 0;
}

int CALLBACK _XDate_OnWndClose(HWINDOW hWnd, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	auto it = DG().windows.find(hWnd);
	if (it != DG().windows.end()){
		_XDate_Ctx* ctx = it->second;
		if (ctx) ctx->closing = TRUE;
		if (ctx && ctx->liveBind){
			if (pbHandled) *pbHandled = TRUE;
			_XDate_DestroyLivePopup(FALSE);
			return 0;
		}
		if (ctx && ctx->hBtnConfirm && XC_IsHELE((HXCGUI)ctx->hBtnConfirm)){
			XWnd_SetFocusEle(hWnd, ctx->hBtnConfirm);
		}
	}
	return 0;
}

int CALLBACK _XDate_OnWndDestroy(HWINDOW hWnd, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	auto it = DG().windows.find(hWnd);
	if (it != DG().windows.end()){
		_XDate_Ctx* ctx = it->second;
		ctx->destroyed = TRUE;
		auto& g = DG();
		if (g.livePopup == ctx){
			g.livePopup = NULL;
			g.liveBindEle = NULL;
		}
		_XDate_Unsubclass(ctx);
		_XDate_UnregisterCtx(ctx);
	}
	return 0;
}

HELE _XDate_CreateCmdButton(_XDate_Ctx* ctx, int x, int y, int w, int h,
	const wchar_t* text, _XDate_Cmd cmd, BOOL primary = FALSE, BOOL cancel = FALSE)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE btn = XBtn_Create(x, y, w, h, text, ctx->hWnd);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	XEle_SetUserData(btn, (vint)cmd);
	_XDate_RegisterEle(ctx, btn);
	DG().commands[btn] = cmd;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XDate_OnBtnClick);
	_XCal_StyleButton(btn, ctx->colors, primary, cancel);
	if (ctx->hFont) XEle_SetFont(btn, ctx->hFont);
	return btn;
}

HELE _XDate_CreateChildCmdButton(_XDate_Ctx* ctx, HXCGUI hParent, int x, int y, int w, int h,
	const wchar_t* text, _XDate_Cmd cmd, BOOL primary = FALSE, BOOL cancel = FALSE)
{
	HELE btn = XBtn_Create(x, y, w, h, text, hParent);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	XEle_SetUserData(btn, (vint)cmd);
	_XDate_RegisterEle(ctx, btn);
	DG().commands[btn] = cmd;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XDate_OnBtnClick);
	_XCal_StyleButton(btn, ctx->colors, primary, cancel);
	if (ctx->hFont) XEle_SetFont(btn, ctx->hFont);
	return btn;
}

HELE _XDate_CreateCell(_XDate_Ctx* ctx, int x, int y, int size)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE btn = XBtn_Create(x, y, size, size, L"", ctx->hWnd);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	XEle_EnableBkTransparent(btn, TRUE);
	XEle_SetUserData(btn, (vint)_XDate_Cmd_DateCell);
	_XDate_RegisterEle(ctx, btn);
	DG().commands[btn] = _XDate_Cmd_DateCell;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XDate_OnBtnClick);
	if (ctx->hFont) XEle_SetFont(btn, ctx->hFont);
	return btn;
}

HXCGUI _XDate_CreateText(_XDate_Ctx* ctx, int x, int y, int w, int h, const wchar_t* text,
	COLORREF color, int align = textAlignFlag_center | textAlignFlag_vcenter)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HXCGUI t = XShapeText_Create(x, y, w, h, text ? text : L"", ctx->hWnd);
	if (!t) return NULL;
	XUI_EnableCSS(t, FALSE);
	XShapeText_SetTextColor(t, color);
	XShapeText_SetTextAlign(t, align);
	if (ctx->hFont) XShapeText_SetFont(t, ctx->hFont);
	return t;
}

HELE _XDate_CreateActionLayout(_XDate_Ctx* ctx, int x, int y, int w, int h)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE wrap = XLayout_Create(x, y, w, h, (HXCGUI)ctx->hWnd);
	if (!wrap) return NULL;
	_XDate_RegisterEle(ctx, wrap);
	XLayoutBox_EnableHorizon(wrap, TRUE);
	XLayoutBox_SetSpace(wrap, 8);
	XLayout_EnableLayout(wrap, TRUE);
	XEle_EnableBkTransparent(wrap, TRUE);
	XWidget_LayoutItem_SetWidth(wrap, layout_size_fixed, w);
	XWidget_LayoutItem_SetHeight(wrap, layout_size_fixed, h);
	return wrap;
}

HELE _XDate_CreateTimeCol(_XDate_Ctx* ctx, int x, int y, int w, int h, int col)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE ele = XEle_Create(x, y, w, h, ctx->hWnd);
	if (!ele) return NULL;
	XUI_EnableCSS(ele, FALSE);
	XEle_EnableBkTransparent(ele, TRUE);
	XEle_SetUserData(ele, (vint)col);
	_XDate_RegisterEle(ctx, ele);
	XEle_RegEventC1(ele, XE_PAINT, (void*)&_XDate_OnTimePaint);
	XEle_EnableEvent_XE_MOUSEWHEEL(ele, TRUE);
	XEle_RegEventC1(ele, XE_MOUSEWHEEL, (void*)&_XDate_OnTimeWheel);
	XEle_RegEventC1(ele, XE_LBUTTONDOWN, (void*)&_XDate_OnTimeLButtonDown);
	XEle_RegEventC1(ele, XE_MOUSEMOVE, (void*)&_XDate_OnTimeMouseMove);
	XEle_RegEventC1(ele, XE_LBUTTONUP, (void*)&_XDate_OnTimeLButtonUp);
	return ele;
}

_XDate_Ctx* _XDate_CreateWindow(HWINDOW hParent, xuitool_theme_ theme, int cornerRadius,
	xdate_format_ format, BOOL liveBind)
{
	_XDate_Ctx* ctx = new _XDate_Ctx();
	if (!ctx) return NULL;
	ctx->theme = theme;
	ctx->format = _XDate_NormFormat(format);
	ctx->liveBind = liveBind ? TRUE : FALSE;
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

	const BOOL hasDate = _XDate_HasDate(ctx->format);
	const int timeCols = _XDate_ColCount(ctx);
	const int timeW = timeCols * kDate_TimeColW;
	const int previewW = _XDate_PreviewWidth(ctx->format);
	if (hasDate){
		ctx->contentW = kCal_PadH * 2 + kCal_PanelW + kDate_CalTimeGap + timeW;
		ctx->contentH = kCal_PadV + kCal_NavSize + kCal_SepGap + 1 + kCal_SepGap
			+ kCal_GridH + kCal_SepGap + 1 + kCal_SepGap + kCal_ActionH + kCal_PadV;
	} else {
		const int minFooter = previewW + 8 + 220;
		ctx->contentW = kCal_PadH * 2 + timeW;
		if (ctx->contentW < kCal_PadH * 2 + minFooter)
			ctx->contentW = kCal_PadH * 2 + minFooter;
		ctx->contentH = kCal_PadV + 28 + kDate_TimeColH + kCal_SepGap + 1 + kCal_SepGap
			+ kCal_ActionH + kCal_PadV;
	}

	int winW = ctx->contentW + _XUITool::kShadowMargin * 2;
	int winH = ctx->contentH + _XUITool::kShadowMargin * 2;
	const wchar_t* title = hasDate ? L"选择日期时间" : L"选择时间";
	if (liveBind){
		ctx->hWnd = XWnd_Create(0, 0, winW, winH, title, ctx->hParent, window_style_nothing);
	} else {
		ctx->hWnd = XModalWnd_Create(winW, winH, title, ctx->hParent, window_style_nothing);
	}
	if (!ctx->hWnd){
		delete ctx;
		return NULL;
	}
	DG().windows[ctx->hWnd] = ctx;
	XWnd_SetTransparentType(ctx->hWnd, window_transparent_shaped);
	XWnd_SetTransparentAlpha(ctx->hWnd, 255);
	XWnd_EnableDragWindow(ctx->hWnd, FALSE);
	XWnd_EnableDragBorder(ctx->hWnd, FALSE);
	XWnd_EnableDragCaption(ctx->hWnd, FALSE);
	XWnd_EnableDrawBk(ctx->hWnd, TRUE);
	XWnd_SetTextColor(ctx->hWnd, ctx->colors.text);
	XWnd_ClearBkInfo(ctx->hWnd);
	XWnd_RegEventC1(ctx->hWnd, WM_PAINT,   (void*)&_XDate_OnPaintWindow);
	XWnd_RegEventC1(ctx->hWnd, WM_CLOSE,   (void*)&_XDate_OnWndClose);
	XWnd_RegEventC1(ctx->hWnd, XE_DESTROY, (void*)&_XDate_OnWndDestroy);
	if (liveBind){
		XWnd_SetTop(ctx->hWnd, TRUE);
	} else {
		XModalWnd_EnableAutoClose(ctx->hWnd, TRUE);
		XModalWnd_EnableEscClose(ctx->hWnd, TRUE);
	}

	ctx->hFont = XFont_CreateEx(L"微软雅黑", 10, fontStyle_regular);

	const int left = kCal_PadH;
	static const wchar_t* timeHeads[] = { L"时", L"分", L"秒" };
	int timeX = 0;
	int timeColY = 0;
	int actionY = 0;
	if (hasDate){
		_XDate_LoadIcons(ctx);
		const int panelRight = left + kCal_PanelW;
		const int navY = kCal_PadV;
		const int headerSepY = navY + kCal_NavSize + kCal_SepGap;
		const int gridY = headerSepY + 1 + kCal_SepGap;
		const int footerSepY = gridY + kCal_GridH + kCal_SepGap;
		actionY = footerSepY + 1 + kCal_SepGap;
		const int nextYearX = panelRight - kCal_NavSize;
		const int nextMonthX = nextYearX - kCal_NavSpace - kCal_NavSize;
		timeX = panelRight + kDate_CalTimeGap;
		const int cellAreaY = gridY + kCal_WeekH;
		const int cellAreaH = kCal_GridRows * kCal_ColW;
		timeColY = cellAreaY + (cellAreaH - kDate_TimeColH) / 2;

		ctx->sepHeaderY = headerSepY;
		ctx->sepFooterY = footerSepY;
		ctx->sepTimeX = panelRight + kDate_CalTimeGap / 2;
		ctx->sepColX[0] = (timeCols >= 2) ? (timeX + kDate_TimeColW) : -1;
		ctx->sepColX[1] = (timeCols >= 3) ? (timeX + kDate_TimeColW * 2) : -1;

		ctx->hBtnPrevYear = _XDate_CreateCmdButton(ctx, left, navY, kCal_NavSize, kCal_NavSize, L"<<", _XDate_Cmd_PrevYear);
		ctx->hBtnPrevMonth = _XDate_CreateCmdButton(ctx, left + kCal_NavSize + kCal_NavSpace, navY,
			kCal_NavSize, kCal_NavSize, L"<", _XDate_Cmd_PrevMonth);
		ctx->hBtnNextMonth = _XDate_CreateCmdButton(ctx, nextMonthX, navY, kCal_NavSize, kCal_NavSize, L">", _XDate_Cmd_NextMonth);
		ctx->hBtnNextYear = _XDate_CreateCmdButton(ctx, nextYearX, navY, kCal_NavSize, kCal_NavSize, L">>", _XDate_Cmd_NextYear);
		_XCal_SetButtonIcon(ctx->hBtnPrevYear, ctx->hImgLastYear, ctx->hImgLastYearDis);
		_XCal_SetButtonIcon(ctx->hBtnPrevMonth, ctx->hImgLastMonth, ctx->hImgLastMonthDis);
		_XCal_SetButtonIcon(ctx->hBtnNextMonth, ctx->hImgNextMonth, ctx->hImgNextMonthDis);
		_XCal_SetButtonIcon(ctx->hBtnNextYear, ctx->hImgNextYear, ctx->hImgNextYearDis);
		ctx->hTitle = _XDate_CreateText(ctx, left + (kCal_PanelW - 120) / 2, navY, 120, kCal_NavSize, L"", ctx->colors.title);

		static const wchar_t* weeks[] = { L"一", L"二", L"三", L"四", L"五", L"六", L"日" };
		for (int i = 0; i < 7; ++i){
			ctx->hWeekLabels[i] = _XDate_CreateText(ctx, left + i * kCal_ColW, gridY,
				kCal_ColW, 20, weeks[i], ctx->colors.weekText);
		}
		int gridCellsY = gridY + 30;
		for (int i = 0; i < 42; ++i){
			int r = i / 7;
			int c = i % 7;
			ctx->hCells[i] = _XDate_CreateCell(ctx, left + c * kCal_ColW + 2, gridCellsY + r * kCal_ColW, 36);
		}

		for (int i = 0; i < timeCols; ++i){
			ctx->hTimeHeaders[i] = _XDate_CreateText(ctx, timeX + i * kDate_TimeColW, gridY,
				kDate_TimeColW, 20, timeHeads[i], ctx->colors.weekText);
			ctx->hTimeCol[i] = _XDate_CreateTimeCol(ctx, timeX + i * kDate_TimeColW, timeColY,
				kDate_TimeColW, kDate_TimeColH, i);
		}
	} else {
		const int headerY = kCal_PadV;
		timeColY = headerY + 28;
		const int footerSepY = timeColY + kDate_TimeColH + kCal_SepGap;
		actionY = footerSepY + 1 + kCal_SepGap;
		timeX = (ctx->contentW - timeW) / 2;

		ctx->sepHeaderY = -1;
		ctx->sepFooterY = footerSepY;
		ctx->sepTimeX = -1;
		ctx->sepColX[0] = (timeCols >= 2) ? (timeX + kDate_TimeColW) : -1;
		ctx->sepColX[1] = (timeCols >= 3) ? (timeX + kDate_TimeColW * 2) : -1;

		for (int i = 0; i < timeCols; ++i){
			ctx->hTimeHeaders[i] = _XDate_CreateText(ctx, timeX + i * kDate_TimeColW, headerY,
				kDate_TimeColW, 20, timeHeads[i], ctx->colors.weekText);
			ctx->hTimeCol[i] = _XDate_CreateTimeCol(ctx, timeX + i * kDate_TimeColW, timeColY,
				kDate_TimeColW, kDate_TimeColH, i);
		}
	}

	ctx->hPreview = _XDate_CreateText(ctx, left, actionY, previewW, kCal_ActionH, L"",
		ctx->colors.rangeText, textAlignFlag_left | textAlignFlag_vcenter);
	const int actionW = ctx->contentW - kCal_PadH * 2 - previewW - 8;
	ctx->hActionWrap = _XDate_CreateActionLayout(ctx, left + previewW + 8, actionY, actionW, kCal_ActionH);
	if (ctx->hActionWrap){
		ctx->hBtnNow = _XDate_CreateChildCmdButton(ctx, (HXCGUI)ctx->hActionWrap, 0, 0, 100, 32,
			L"此刻", _XDate_Cmd_Now);
		ctx->hBtnCancel = _XDate_CreateChildCmdButton(ctx, (HXCGUI)ctx->hActionWrap, 0, 0, 100, 32,
			L"取消", _XDate_Cmd_Cancel, FALSE, TRUE);
		ctx->hBtnConfirm = _XDate_CreateChildCmdButton(ctx, (HXCGUI)ctx->hActionWrap, 0, 0, 100, 32,
			L"确定", _XDate_Cmd_Confirm, TRUE);
		if (ctx->hBtnNow){
			XWidget_LayoutItem_SetWidth(ctx->hBtnNow, layout_size_weight, 1);
			XWidget_LayoutItem_SetHeight(ctx->hBtnNow, layout_size_fill, 0);
		}
		XWidget_LayoutItem_SetWidth(ctx->hBtnCancel, layout_size_weight, 1);
		XWidget_LayoutItem_SetHeight(ctx->hBtnCancel, layout_size_fill, 0);
		XWidget_LayoutItem_SetWidth(ctx->hBtnConfirm, layout_size_weight, 1);
		XWidget_LayoutItem_SetHeight(ctx->hBtnConfirm, layout_size_fill, 0);
	}
	return ctx;
}

BOOL _XDate_Show(HWINDOW hParent, xcalendar_datetime_* pDate, xdate_format_ format,
	BOOL bLimitMaxDate, const xcalendar_datetime_* pMaxDate, xuitool_theme_ theme,
	int cornerRadius)
{
	format = _XDate_NormFormat(format);
	_XDate_Ctx* ctx = _XDate_CreateWindow(hParent, theme, cornerRadius, format, FALSE);
	if (!ctx) return FALSE;
	ctx->limitMaxDate = (_XDate_HasDate(format) && bLimitMaxDate) ? TRUE : FALSE;
	if (pMaxDate && pMaxDate->year > 0){
		ctx->maxDate = _XCal_Normalize(*pMaxDate);
	}
	ctx->maxDate.hour = 23;
	ctx->maxDate.minute = 59;
	ctx->maxDate.second = 59;

	if (_XCal_IsValidInputDate(pDate)) ctx->selDate = _XCal_Normalize(*pDate);
	else ctx->selDate = ctx->today;
	if (!_XDate_HasSecond(ctx->format)) ctx->selDate.second = 0;
	ctx->selDate = _XDate_ClampMaxDate(ctx, ctx->selDate);

	if (_XDate_HasDate(ctx->format)){
		ctx->showDate.year = ctx->selDate.year;
		ctx->showDate.month = ctx->selDate.month;
		ctx->showDate.day = 1;
		if (!_XDate_CanMoveToMonth(ctx, ctx->showDate)){
			ctx->showDate.year = ctx->maxDate.year;
			ctx->showDate.month = ctx->maxDate.month;
			ctx->showDate.day = 1;
		}
		_XDate_RefreshCalendar(ctx);
	} else {
		_XDate_RefreshTime(ctx);
	}
	_XDate_ApplyPopupPosition(ctx);
	XWnd_ShowWindow(ctx->hWnd, SW_SHOWNOACTIVATE);
	int result = XModalWnd_DoModal(ctx->hWnd);
	if (!ctx->confirmed) ctx->closing = TRUE;

	BOOL ok = (result == kDate_ResultOk && ctx->confirmed);
	xcalendar_datetime_ out = ctx->resultDate;
	HWINDOW hWnd = ctx->hWnd;
	BOOL destroyed = ctx->destroyed;
	_XDate_UnregisterCtx(ctx);

	if (!destroyed && XC_IsHWINDOW((HXCGUI)hWnd)){
		XWnd_DestroyWindow(hWnd);
	}
	_XDate_DestroyIcons(ctx);
	ctx->hFont = NULL;
	delete ctx;

	if (ok && pDate) *pDate = out;
	return ok;
}

BOOL _XDate_IsEditEle(HELE hEle)
{
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return FALSE;
	XC_OBJECT_TYPE t = XC_GetObjectType((HXCGUI)hEle);
	return (t == XC_EDIT || t == XC_COMBOBOX) ? TRUE : FALSE;
}

BOOL _XDate_TryParse(const wchar_t* text, xcalendar_datetime_* out)
{
	if (!text || !text[0]) return FALSE;
	while (*text == L' ' || *text == L'\t') ++text;
	if (!*text) return FALSE;

	xcalendar_datetime_ empty{};
	xcalendar_datetime_ d = _XCal_ParseDateTime(text, empty);
	if (d.year >= 1900){
		if (out) *out = d;
		return TRUE;
	}

	int h = 0, mi = 0, se = 0;
	int n = swscanf_s(text, L"%d:%d:%d", &h, &mi, &se);
	if (n < 2) return FALSE;
	if (h < 0 || h > 23 || mi < 0 || mi > 59) return FALSE;
	if (n >= 3 && (se < 0 || se > 59)) return FALSE;
	d = _XCal_Current();
	d.hour = h;
	d.minute = mi;
	d.second = (n >= 3) ? se : 0;
	d = _XCal_Normalize(d);
	if (out) *out = d;
	return TRUE;
}

void _XDate_WriteEle(HELE hEle, const xcalendar_datetime_& d, xdate_format_ format)
{
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return;
	xcalendar_datetime_ v = _XCal_Normalize(d);
	if (!_XDate_HasSecond(format)) v.second = 0;
	_XDate_Bind* b = NULL;
	auto it = DG().binds.find(hEle);
	if (it != DG().binds.end()){
		b = &it->second;
		b->updatingUI = TRUE;
		b->value = v;
	}
	CXText t = _XDate_Format(v, format);
	XEdit_SetText(hEle, t.get());
	XEle_Redraw(hEle, FALSE);
	if (b) b->updatingUI = FALSE;
}

_XDate_Bind* _XDate_FindBind(HELE hEle)
{
	if (!hEle) return NULL;
	auto& g = DG();
	auto it = g.binds.find(hEle);
	return it == g.binds.end() ? NULL : &it->second;
}

xcalendar_datetime_ _XDate_ClampBindValue(const _XDate_Bind* b, xcalendar_datetime_ d)
{
	d = _XCal_Normalize(d);
	if (b && b->limitMaxDate && _XDate_HasDate(b->format)){
		xcalendar_datetime_ maxDate = b->hasMaxDate ? b->maxDate : _XCal_Current();
		if (_XCal_CompareDate(d, maxDate) > 0){
			int h = d.hour, m = d.minute, s = d.second;
			d = maxDate;
			d.hour = h;
			d.minute = m;
			d.second = s;
			d = _XCal_Normalize(d);
		}
	}
	if (b && !_XDate_HasSecond(b->format)) d.second = 0;
	return d;
}

void _XDate_CommitBind(HELE hEle, _XDate_Bind* b, BOOL keepField)
{
	if (!hEle || !b) return;
	b->value = _XDate_ClampBindValue(b, b->value);
	_XDate_WriteEle(hEle, b->value, b->format);
	if (keepField){
		if (b->focusField == kCal_FieldNone) b->focusField = _XDate_DefaultField(b->format);
		_XDate_SelectField(hEle, b->format, b->focusField);
	}
	_XDate_SyncPopupFromBind(hEle, b);
}

void _XDate_Unsubclass(_XDate_Ctx* ctx)
{
	if (!ctx || !ctx->hwnd || !ctx->oldWndProc) return;
	::SetWindowLongPtrW(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)ctx->oldWndProc);
	ctx->oldWndProc = NULL;
}

LRESULT CALLBACK _XDate_NoActivateProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	_XDate_Ctx* ctx = NULL;
	for (auto& kv : DG().windows){
		if (kv.second && kv.second->hwnd == hwnd){
			ctx = kv.second;
			break;
		}
	}
	if (msg == WM_MOUSEACTIVATE)
		return MA_NOACTIVATE;
	if (ctx && ctx->oldWndProc)
		return ::CallWindowProcW(ctx->oldWndProc, hwnd, msg, wParam, lParam);
	return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void _XDate_ApplyNoActivate(_XDate_Ctx* ctx)
{
	if (!ctx || !ctx->hWnd || !XC_IsHWINDOW((HXCGUI)ctx->hWnd)) return;
	HWND hwnd = XWnd_GetHWND(ctx->hWnd);
	if (!hwnd) return;
	ctx->hwnd = hwnd;
	LONG_PTR ex = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
	::SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
	if (!ctx->oldWndProc){
		ctx->oldWndProc = (WNDPROC)::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)&_XDate_NoActivateProc);
	}
}

void _XDate_RestoreBindFocus(_XDate_Ctx* ctx)
{
	if (!ctx || !ctx->liveBind || !ctx->hBindEdit || !XC_IsHELE((HXCGUI)ctx->hBindEdit)) return;
	HWINDOW hWnd = XWidget_GetHWINDOW((HXCGUI)ctx->hBindEdit);
	if (!hWnd) return;
	auto& g = DG();
	g.suspendClose = TRUE;
	XWnd_SetFocusEle(hWnd, ctx->hBindEdit);
	_XDate_Bind* b = _XDate_FindBind(ctx->hBindEdit);
	if (b){
		if (b->focusField == kCal_FieldNone) b->focusField = _XDate_DefaultField(b->format);
		_XDate_SelectField(ctx->hBindEdit, b->format, b->focusField);
	}
	g.suspendClose = FALSE;
}

void _XDate_ApplySelToUI(_XDate_Ctx* ctx)
{
	if (!ctx) return;
	ctx->selDate = _XDate_ClampMaxDate(ctx, ctx->selDate);
	if (_XDate_HasDate(ctx->format)){
		ctx->showDate.year = ctx->selDate.year;
		ctx->showDate.month = ctx->selDate.month;
		ctx->showDate.day = 1;
		if (!_XDate_CanMoveToMonth(ctx, ctx->showDate)){
			ctx->showDate.year = ctx->maxDate.year;
			ctx->showDate.month = ctx->maxDate.month;
			ctx->showDate.day = 1;
		}
		_XDate_RefreshCalendar(ctx);
	} else {
		_XDate_RefreshTime(ctx);
	}
}

void _XDate_SyncLiveToEdit(_XDate_Ctx* ctx)
{
	if (!ctx || !ctx->liveBind || ctx->updatingUI || ctx->closing || ctx->destroyed) return;
	if (!ctx->hBindEdit || !XC_IsHELE((HXCGUI)ctx->hBindEdit)) return;
	_XDate_Bind* b = _XDate_FindBind(ctx->hBindEdit);
	if (!b) return;
	ctx->updatingUI = TRUE;
	b->value = ctx->selDate;
	_XDate_CommitBind(ctx->hBindEdit, b, TRUE);
	ctx->updatingUI = FALSE;
	if (ctx->dragCol < 0)
		_XDate_RestoreBindFocus(ctx);
}

void _XDate_SyncPopupFromBind(HELE hEle, _XDate_Bind* b)
{
	auto& g = DG();
	if (!b || !g.livePopup || g.liveBindEle != hEle) return;
	_XDate_Ctx* ctx = g.livePopup;
	if (!ctx || ctx->updatingUI || ctx->closing || ctx->destroyed) return;
	if (_XCal_CompareFull(ctx->selDate, b->value) == 0) return;
	ctx->selDate = _XDate_ClampMaxDate(ctx, b->value);
	ctx->updatingUI = TRUE;
	_XDate_ApplySelToUI(ctx);
	ctx->updatingUI = FALSE;
}

void _XDate_DestroyLivePopup(BOOL revert)
{
	auto& g = DG();
	_XDate_Ctx* ctx = g.livePopup;
	g.livePopup = NULL;
	g.liveBindEle = NULL;
	if (!ctx) return;

	HELE hEdit = ctx->hBindEdit;
	xcalendar_datetime_ openDate = ctx->openDate;
	ctx->closing = TRUE;
	_XDate_Unsubclass(ctx);

	HWINDOW hWnd = ctx->hWnd;
	BOOL destroyed = ctx->destroyed;
	_XDate_UnregisterCtx(ctx);
	if (!destroyed && hWnd && XC_IsHWINDOW((HXCGUI)hWnd)){
		XWnd_DestroyWindow(hWnd);
	}
	_XDate_DestroyIcons(ctx);
	ctx->hFont = NULL;
	delete ctx;

	if (revert && hEdit && XC_IsHELE((HXCGUI)hEdit)){
		_XDate_Bind* b = _XDate_FindBind(hEdit);
		if (b){
			b->value = openDate;
			_XDate_CommitBind(hEdit, b, TRUE);
		}
	}
}

void _XDate_ClosePopup(_XDate_Ctx* ctx, int result)
{
	if (!ctx || ctx->closing || ctx->destroyed) return;
	if (ctx->liveBind){
		if (result == kDate_ResultOk){
			ctx->resultDate = _XDate_ClampMaxDate(ctx, ctx->selDate);
			if (ctx->hBindEdit && XC_IsHELE((HXCGUI)ctx->hBindEdit)){
				_XDate_Bind* b = _XDate_FindBind(ctx->hBindEdit);
				if (b){
					b->value = ctx->resultDate;
					_XDate_CommitBind(ctx->hBindEdit, b, TRUE);
				}
			}
			_XDate_DestroyLivePopup(FALSE);
		} else {
			_XDate_DestroyLivePopup(TRUE);
		}
		return;
	}
	ctx->closing = TRUE;
	if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
		XModalWnd_EndModal(ctx->hWnd, result);
	}
}

BOOL _XDate_SyncBindFromText(HELE hEle, _XDate_Bind* b)
{
	if (!hEle || !b) return FALSE;
	xcalendar_datetime_ parsed{};
	if (!_XDate_TryParse(XEdit_GetText_Temp(hEle), &parsed)) return FALSE;
	if (!_XDate_HasSecond(b->format)) parsed.second = 0;
	b->value = _XDate_ClampBindValue(b, parsed);
	return TRUE;
}

void _XDate_UnhookBind(HELE hEle, _XDate_Bind& b);

int CALLBACK _XDate_OnBoundComboPopup(HELE hEle, BOOL* pbHandled);
int CALLBACK _XDate_OnBoundLButtonUp(HELE hEle, UINT, POINT*, BOOL* pbHandled);
int CALLBACK _XDate_OnBoundKeyDown(HELE hEle, int iChar, int, BOOL* pbHandled);
int CALLBACK _XDate_OnBoundChar(HELE hEle, WPARAM wParam, LPARAM, BOOL* pbHandled);
int CALLBACK _XDate_OnBoundKillFocus(HELE hEle, BOOL* pbHandled);
int CALLBACK _XDate_OnBoundSetFocus(HELE hEle, BOOL* pbHandled);
int CALLBACK _XDate_OnBoundMouseWheel(HELE hEle, UINT nFlags, POINT*, BOOL* pbHandled);
int CALLBACK _XDate_OnBoundEditChanged(HELE hEle, BOOL* pbHandled);
int CALLBACK _XDate_OnBoundDestroy(HELE hEle, BOOL* pbHandled);

void _XDate_HookBind(HELE hEle, _XDate_Bind& b)
{
	if (b.eventsHooked || !hEle) return;
	XEle_RegEventC1(hEle, XE_DESTROY, (void*)&_XDate_OnBoundDestroy);
	XEle_RegEventC1(hEle, XE_LBUTTONUP, (void*)&_XDate_OnBoundLButtonUp);
	XEle_RegEventC1(hEle, XE_KEYDOWN, (void*)&_XDate_OnBoundKeyDown);
	XEle_RegEventC1(hEle, XE_CHAR, (void*)&_XDate_OnBoundChar);
	XEle_RegEventC1(hEle, XE_KILLFOCUS, (void*)&_XDate_OnBoundKillFocus);
	XEle_RegEventC1(hEle, XE_SETFOCUS, (void*)&_XDate_OnBoundSetFocus);
	XEle_EnableEvent_XE_MOUSEWHEEL(hEle, TRUE);
	XEle_RegEventC1(hEle, XE_MOUSEWHEEL, (void*)&_XDate_OnBoundMouseWheel);
	XEle_RegEventC1(hEle, XE_EDIT_CHANGED, (void*)&_XDate_OnBoundEditChanged);
	if (b.isCombo){
		XEle_RegEventC1(hEle, XE_COMBOBOX_POPUP, (void*)&_XDate_OnBoundComboPopup);
	}
	b.eventsHooked = TRUE;
}

void _XDate_UnhookBind(HELE hEle, _XDate_Bind& b)
{
	if (!b.eventsHooked || !hEle) return;
	if (XC_IsHELE((HXCGUI)hEle)){
		XEle_RemoveEventC(hEle, XE_DESTROY, (void*)&_XDate_OnBoundDestroy);
		XEle_RemoveEventC(hEle, XE_LBUTTONUP, (void*)&_XDate_OnBoundLButtonUp);
		XEle_RemoveEventC(hEle, XE_KEYDOWN, (void*)&_XDate_OnBoundKeyDown);
		XEle_RemoveEventC(hEle, XE_CHAR, (void*)&_XDate_OnBoundChar);
		XEle_RemoveEventC(hEle, XE_KILLFOCUS, (void*)&_XDate_OnBoundKillFocus);
		XEle_RemoveEventC(hEle, XE_SETFOCUS, (void*)&_XDate_OnBoundSetFocus);
		XEle_RemoveEventC(hEle, XE_MOUSEWHEEL, (void*)&_XDate_OnBoundMouseWheel);
		XEle_RemoveEventC(hEle, XE_EDIT_CHANGED, (void*)&_XDate_OnBoundEditChanged);
		if (b.isCombo){
			XEle_RemoveEventC(hEle, XE_COMBOBOX_POPUP, (void*)&_XDate_OnBoundComboPopup);
		}
	}
	b.eventsHooked = FALSE;
}

BOOL _XDate_LivePopupVisible()
{
	auto& g = DG();
	if (!g.livePopup || !g.livePopup->hWnd || g.livePopup->destroyed || g.livePopup->closing)
		return FALSE;
	if (!XC_IsHWINDOW((HXCGUI)g.livePopup->hWnd)) return FALSE;
	HWND hwnd = g.livePopup->hwnd ? g.livePopup->hwnd : XWnd_GetHWND(g.livePopup->hWnd);
	return (hwnd && ::IsWindowVisible(hwnd)) ? TRUE : FALSE;
}

BOOL _XDate_OpenBound(HELE hEle)
{
	auto& g = DG();
	_XDate_Bind* b = _XDate_FindBind(hEle);
	if (!b) return FALSE;
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return FALSE;

	_XDate_SyncBindFromText(hEle, b);
	xcalendar_datetime_ d = b->value.year >= 1900 ? b->value : _XCal_Current();
	if (!_XDate_HasSecond(b->format)) d.second = 0;
	d = _XDate_ClampBindValue(b, d);
	b->value = d;

	if (g.livePopup && g.liveBindEle == hEle && _XDate_LivePopupVisible()){
		g.livePopup->selDate = d;
		g.livePopup->updatingUI = TRUE;
		_XDate_ApplySelToUI(g.livePopup);
		g.livePopup->updatingUI = FALSE;
		return TRUE;
	}
	if (g.livePopup)
		_XDate_DestroyLivePopup(FALSE);

	HWINDOW hParent = XWidget_GetHWINDOW((HXCGUI)hEle);
	g.popupMode = _XDate_PopupMode_Ele;
	g.bindEle = hEle;
	g.bindOffsetX = b->offsetX;
	g.bindOffsetY = b->offsetY;

	const xcalendar_datetime_* pMax = b->hasMaxDate ? &b->maxDate : NULL;
	_XDate_Ctx* ctx = _XDate_CreateWindow(hParent, b->theme, b->cornerRadius, b->format, TRUE);
	if (!ctx) return FALSE;
	ctx->liveBind = TRUE;
	ctx->hBindEdit = hEle;
	ctx->openDate = d;
	ctx->limitMaxDate = (_XDate_HasDate(b->format) && b->limitMaxDate) ? TRUE : FALSE;
	if (pMax && pMax->year > 0)
		ctx->maxDate = _XCal_Normalize(*pMax);
	ctx->maxDate.hour = 23;
	ctx->maxDate.minute = 59;
	ctx->maxDate.second = 59;
	ctx->selDate = _XDate_ClampMaxDate(ctx, d);
	ctx->updatingUI = TRUE;
	_XDate_ApplySelToUI(ctx);
	ctx->updatingUI = FALSE;
	_XDate_ApplyPopupPosition(ctx);
	g.livePopup = ctx;
	g.liveBindEle = hEle;
	_XDate_ApplyNoActivate(ctx);
	XWnd_ShowWindow(ctx->hWnd, SW_SHOWNOACTIVATE);
	_XDate_ApplyNoActivate(ctx);
	_XDate_RestoreBindFocus(ctx);
	return TRUE;
}

void _XDate_AdjustBoundField(HELE hEle, _XDate_Bind* b, int delta)
{
	if (!hEle || !b) return;
	if (b->focusField == kCal_FieldNone) b->focusField = _XDate_DefaultField(b->format);
	b->typeCount = 0;
	b->value = _XCal_AdjustField(b->value, b->focusField, delta);
	_XDate_CommitBind(hEle, b, TRUE);
}

int CALLBACK _XDate_OnBoundComboPopup(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XDate_OpenBound(hEle);
	return 0;
}

int CALLBACK _XDate_OnBoundLButtonUp(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XDate_Bind* b = _XDate_FindBind(hEle);
	if (!b || b->updatingUI) return 0;
	int pos = XEdit_GetCurPos(hEle);
	if (!_XDate_SyncBindFromText(hEle, b)){
		_XDate_CommitBind(hEle, b, FALSE);
		pos = 0;
	}
	b->focusField = _XDate_FieldFromPos(b->format, pos);
	b->typeCount = 0;
	_XDate_SelectField(hEle, b->format, b->focusField);
	_XDate_OpenBound(hEle);
	return 0;
}

int CALLBACK _XDate_OnBoundKeyDown(HELE hEle, int iChar, int, BOOL* pbHandled)
{
	_XDate_Bind* b = _XDate_FindBind(hEle);
	if (!b || b->updatingUI) return 0;
	if (iChar == VK_ESCAPE){
		if (DG().livePopup && DG().liveBindEle == hEle){
			if (pbHandled) *pbHandled = TRUE;
			_XDate_DestroyLivePopup(TRUE);
			return 0;
		}
	}
	if (iChar == VK_UP || iChar == VK_DOWN){
		if (pbHandled) *pbHandled = TRUE;
		_XDate_AdjustBoundField(hEle, b, (iChar == VK_UP) ? 1 : -1);
		return 0;
	}
	if (iChar == VK_LEFT || iChar == VK_RIGHT || iChar == VK_HOME || iChar == VK_END){
		if (pbHandled) *pbHandled = TRUE;
		if (b->focusField == kCal_FieldNone) b->focusField = _XDate_DefaultField(b->format);
		if (iChar == VK_HOME) b->focusField = _XDate_FirstField(b->format);
		else if (iChar == VK_END) b->focusField = _XDate_DefaultField(b->format);
		else b->focusField = _XDate_NeighborField(b->format, b->focusField, (iChar == VK_RIGHT) ? 1 : -1);
		b->typeCount = 0;
		_XDate_SelectField(hEle, b->format, b->focusField);
		return 0;
	}
	if (iChar == VK_BACK || iChar == VK_DELETE){
		if (pbHandled) *pbHandled = TRUE;
		if (b->focusField == kCal_FieldNone) b->focusField = _XDate_DefaultField(b->format);
		_XDate_SetFieldValue(&b->value, b->focusField, _XDate_FieldMin(b->focusField));
		b->typeCount = 0;
		_XDate_CommitBind(hEle, b, TRUE);
		return 0;
	}
	return 0;
}

int CALLBACK _XDate_OnBoundChar(HELE hEle, WPARAM wParam, LPARAM, BOOL* pbHandled)
{
	_XDate_Bind* b = _XDate_FindBind(hEle);
	if (!b || b->updatingUI) return 0;
	wchar_t c = (wchar_t)wParam;
	if (c < 0x20) return 0;
	if (pbHandled) *pbHandled = TRUE;
	if (c < L'0' || c > L'9') return 0;

	if (b->focusField == kCal_FieldNone) b->focusField = _XDate_DefaultField(b->format);
	int digit = (int)(c - L'0');
	int width = _XDate_FieldWidth(b->focusField);
	int maxV = _XDate_FieldMax(b->value, b->focusField);
	int minV = _XDate_FieldMin(b->focusField);
	if (b->typeCount <= 0 || b->typeCount >= width){
		b->typeCount = 0;
		b->typeValue = 0;
	}
	int next = b->typeValue * 10 + digit;
	if (b->typeCount > 0 && next > maxV){
		b->typeValue = digit;
		b->typeCount = 1;
		next = digit;
	} else {
		b->typeValue = next;
		++b->typeCount;
	}
	int v = _XCal_ClampInt(b->typeValue, minV, maxV);
	if (b->focusField == kCal_FieldYear && b->typeCount < width){
		v = b->typeValue;
		if (v < 1) v = 1;
		if (v > 9999) v = 9999;
		b->value.year = v;
		b->value = _XCal_Normalize(b->value);
	} else {
		_XDate_SetFieldValue(&b->value, b->focusField, v);
	}
	_XDate_CommitBind(hEle, b, TRUE);
	BOOL filled = (b->typeCount >= width) ? TRUE : FALSE;
	if (!filled && b->typeCount > 0 && b->typeValue * 10 > maxV) filled = TRUE;
	if (filled){
		int nextField = _XDate_NeighborField(b->format, b->focusField, 1);
		if (nextField != b->focusField){
			b->focusField = nextField;
			b->typeCount = 0;
			_XDate_SelectField(hEle, b->format, b->focusField);
		}
	}
	return 0;
}

int CALLBACK _XDate_OnBoundKillFocus(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XDate_Bind* b = _XDate_FindBind(hEle);
	if (!b || b->updatingUI) return 0;
	b->typeCount = 0;
	if (!_XDate_SyncBindFromText(hEle, b)){
		if (b->value.year < 1900) b->value = _XCal_Current();
	} else if (_XDate_HasDate(b->format) && b->value.year < 1900){
		b->value.year = 1900;
	}
	_XDate_CommitBind(hEle, b, FALSE);
	auto& g = DG();
	if (g.suspendClose) return 0;
	if (g.livePopup && g.liveBindEle == hEle){
		HWND popupHwnd = g.livePopup->hwnd ? g.livePopup->hwnd : XWnd_GetHWND(g.livePopup->hWnd);
		HWND focus = ::GetFocus();
		if (popupHwnd && focus && (focus == popupHwnd || ::IsChild(popupHwnd, focus))){
			_XDate_RestoreBindFocus(g.livePopup);
			return 0;
		}
		_XDate_DestroyLivePopup(FALSE);
	}
	return 0;
}

int CALLBACK _XDate_OnBoundSetFocus(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XDate_Bind* b = _XDate_FindBind(hEle);
	if (!b || b->updatingUI) return 0;
	if (!_XDate_SyncBindFromText(hEle, b)){
		if (b->value.year < 1900) b->value = _XCal_Current();
		_XDate_CommitBind(hEle, b, FALSE);
	}
	if (b->focusField == kCal_FieldNone) b->focusField = _XDate_DefaultField(b->format);
	_XDate_SelectField(hEle, b->format, b->focusField);
	return 0;
}

int CALLBACK _XDate_OnBoundMouseWheel(HELE hEle, UINT nFlags, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XDate_Bind* b = _XDate_FindBind(hEle);
	if (!b || b->updatingUI) return 0;
	short delta = GET_WHEEL_DELTA_WPARAM(nFlags);
	_XDate_AdjustBoundField(hEle, b, (delta > 0) ? 1 : -1);
	return 0;
}

int CALLBACK _XDate_OnBoundEditChanged(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XDate_Bind* b = _XDate_FindBind(hEle);
	if (!b || b->updatingUI) return 0;
	if (_XDate_SyncBindFromText(hEle, b)){
		CXText t = _XDate_Format(b->value, b->format);
		const wchar_t* cur = XEdit_GetText_Temp(hEle);
		if (cur && t.get() && wcscmp(cur, t.get()) == 0) return 0;
	}
	_XDate_CommitBind(hEle, b, TRUE);
	return 0;
}

int CALLBACK _XDate_OnBoundDestroy(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	auto& g = DG();
	if (g.liveBindEle == hEle)
		_XDate_DestroyLivePopup(FALSE);
	auto it = g.binds.find(hEle);
	if (it == g.binds.end()) return 0;
	it->second.eventsHooked = FALSE;
	g.binds.erase(it);
	return 0;
}

}  // anonymous namespace (CXDate internals)

//============================================================================
// CXDate 公开接口
//============================================================================

xcalendar_datetime_ CXDate::GetToday()
{
	return _XCal_Current();
}

CXText CXDate::FormatDateTime(xcalendar_datetime_ date, xdate_format_ format)
{
	date = _XCal_Normalize(date);
	if (!_XDate_HasSecond(format)) date.second = 0;
	return _XDate_Format(date, format);
}

const wchar_t* CXDate::FormatDateTimePtr(xcalendar_datetime_ date, xdate_format_ format)
{
	static thread_local CXText s_text;
	s_text = FormatDateTime(date, format);
	return s_text.get();
}

BOOL CXDate::AttachEdit(HELE hEdit, xdate_format_ format, BOOL bLimitMaxDate,
	xuitool_theme_ theme, const xcalendar_datetime_* pMaxDate, int nCornerRadius)
{
	if (!_XDate_IsEditEle(hEdit)) return FALSE;
	auto& g = DG();
	_XDate_Bind b{};
	auto it = g.binds.find(hEdit);
	if (it != g.binds.end()){
		_XDate_UnhookBind(hEdit, it->second);
		b = it->second;
	}
	b.format = _XDate_NormFormat(format);
	b.limitMaxDate = bLimitMaxDate ? TRUE : FALSE;
	b.theme = theme;
	b.cornerRadius = _XCal_ClampInt(nCornerRadius, 0, 32);
	b.hasMaxDate = (pMaxDate && pMaxDate->year > 0) ? TRUE : FALSE;
	if (b.hasMaxDate) b.maxDate = _XCal_Normalize(*pMaxDate);
	b.isCombo = (XC_GetObjectType((HXCGUI)hEdit) == XC_COMBOBOX) ? TRUE : FALSE;
	b.offsetX = 0;
	b.offsetY = 4;
	b.focusField = _XDate_DefaultField(b.format);
	b.typeCount = 0;
	b.typeValue = 0;
	_XDate_HookBind(hEdit, b);
	g.binds[hEdit] = b;

	xcalendar_datetime_ now = _XCal_Current();
	if (!_XDate_HasSecond(b.format)) now.second = 0;
	_XDate_WriteEle(hEdit, now, b.format);
	return TRUE;
}

BOOL CXDate::DetachEdit(HELE hEdit)
{
	auto& g = DG();
	if (g.liveBindEle == hEdit)
		_XDate_DestroyLivePopup(FALSE);
	auto it = g.binds.find(hEdit);
	if (it == g.binds.end()) return FALSE;
	_XDate_UnhookBind(hEdit, it->second);
	g.binds.erase(it);
	return TRUE;
}

BOOL CXDate::IsAttached(HELE hEdit)
{
	auto& g = DG();
	return g.binds.find(hEdit) != g.binds.end() ? TRUE : FALSE;
}

BOOL CXDate::GetDateTime(HELE hEdit, xcalendar_datetime_* pDate)
{
	if (!_XDate_IsEditEle(hEdit) || !pDate) return FALSE;
	return _XDate_TryParse(XEdit_GetText_Temp(hEdit), pDate);
}

BOOL CXDate::SetDateTime(HELE hEdit, xcalendar_datetime_ date, xdate_format_ format)
{
	if (!_XDate_IsEditEle(hEdit)) return FALSE;
	xdate_format_ fmt = _XDate_NormFormat(format);
	auto& g = DG();
	auto it = g.binds.find(hEdit);
	if (it != g.binds.end()){
		fmt = it->second.format;
		it->second.typeCount = 0;
	}
	if (!_XDate_HasSecond(fmt)) date.second = 0;
	_XDate_WriteEle(hEdit, date, fmt);
	return TRUE;
}

void CXDate::SetBindEle(HELE hEle, int offsetX, int offsetY)
{
	auto& g = DG();
	g.popupMode = _XDate_PopupMode_Ele;
	g.bindEle = hEle;
	g.bindOffsetX = offsetX;
	g.bindOffsetY = offsetY;
}

void CXDate::SetPopupPosition(POINT pt)
{
	auto& g = DG();
	g.popupMode = _XDate_PopupMode_Pos;
	g.popupPt = pt;
	g.bindEle = NULL;
	g.bindOffsetX = 0;
	g.bindOffsetY = 0;
}

BOOL CXDate::Popup(HWINDOW hParent, xcalendar_datetime_* pDate,
	xdate_format_ format, BOOL bLimitMaxDate, xuitool_theme_ theme,
	const xcalendar_datetime_* pMaxDate, int nCornerRadius)
{
	xcalendar_datetime_ d = _XCal_Current();
	if (_XCal_IsValidInputDate(pDate)) d = *pDate;
	format = _XDate_NormFormat(format);
	if (!_XDate_HasSecond(format)) d.second = 0;
	BOOL ok = _XDate_Show(hParent, &d, format, bLimitMaxDate, pMaxDate, theme, nCornerRadius);
	if (ok && pDate) *pDate = d;
	return ok;
}
