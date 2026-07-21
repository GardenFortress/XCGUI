//============================================================================
// module_xcgui_uitool_notify.cpp — CXNotify 系统通知 / XCGUI 降级通知
//============================================================================
// Win10/11: 复用已添加的 XCGUI 托盘图标，由 Shell 显示系统通知。
// Win7/未知老系统: 使用独立、非模态、不抢焦点的 XCGUI 通知窗。
//============================================================================

namespace {

constexpr UINT kXNotifyTimerFade      = 0x7160;
constexpr UINT kXNotifyTimerAutoClose = 0x7161;
constexpr int  kXNotifyFadeTickMs     = 16;
constexpr int  kXNotifyFadeInMs       = 160;
constexpr int  kXNotifyFadeOutMs      = 140;
constexpr int  kXNotifyWidth          = 360;
constexpr int  kXNotifyHeight         = 108;
constexpr int  kXNotifyShadow         = 8;
constexpr int  kXNotifyScreenMargin   = 16;
constexpr int  kXNotifyCornerRadius   = 10;
constexpr int  kXNotifySystemInfoFlag = 0x01;  // NIIF_INFO，避免向炫语言接口暴露 Win32 宏。

enum _XNotifyFadeState
{
	_XNotifyHidden = 0,
	_XNotifyFadeIn,
	_XNotifyShown,
	_XNotifyFadeOut,
};

struct _XNotifyState
{
	HWINDOW hWindow = NULL;
	HELE hBody = NULL;
	HWND hwnd = NULL;
	HFONTX hTitleFont = NULL;
	HFONTX hTextFont = NULL;
	std::wstring title;
	std::wstring text;
	xuitool_theme_ theme = xuitool_theme_auto;
	_XNotifyFadeState fadeState = _XNotifyHidden;
	DWORD fadeStartTick = 0;
	DWORD fadeDuration = 1;
	int fadeFrom = 0;
	int fadeTo = 255;
};

_XNotifyState& _XNotify_GetState()
{
	static _XNotifyState state;
	return state;
}

inline int _XNotify_Round(double value)
{
	return value >= 0 ? (int)(value + 0.5) : -(int)(-value + 0.5);
}

void _XNotify_HideImmediate()
{
	auto& state = _XNotify_GetState();
	if (state.hBody && XC_IsHELE((HXCGUI)state.hBody)){
		XEle_KillXCTimer(state.hBody, kXNotifyTimerFade);
		XEle_KillXCTimer(state.hBody, kXNotifyTimerAutoClose);
	}
	if (state.hWindow && XC_IsHWINDOW((HXCGUI)state.hWindow))
		XWnd_SetTransparentAlpha(state.hWindow, 0);
	if (state.hwnd && ::IsWindow(state.hwnd))
		::ShowWindow(state.hwnd, SW_HIDE);
	state.fadeState = _XNotifyHidden;
}

void _XNotify_StartFade(int from, int to, int durationMs)
{
	auto& state = _XNotify_GetState();
	if (!state.hBody || !XC_IsHELE((HXCGUI)state.hBody)) return;
	state.fadeFrom = from;
	state.fadeTo = to;
	state.fadeDuration = (DWORD)(durationMs > 0 ? durationMs : 1);
	state.fadeStartTick = ::GetTickCount();
	state.fadeState = to > from ? _XNotifyFadeIn : _XNotifyFadeOut;
	XEle_SetXCTimer(state.hBody, kXNotifyTimerFade, kXNotifyFadeTickMs);
}

int CALLBACK _XNotify_OnPaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	auto& state = _XNotify_GetState();
	if (hEle != state.hBody) return 0;

	_XUITool::ThemePalette palette;
	_XUITool::ResolvePalette(state.theme, _XUITool::kDarkText,
		_XUITool::kDarkBg, _XUITool::kDarkAccent, &palette);

	RECT body{
		kXNotifyShadow,
		kXNotifyShadow,
		kXNotifyWidth - kXNotifyShadow,
		kXNotifyHeight - kXNotifyShadow
	};
	RECTF bodyF{(float)body.left, (float)body.top, (float)body.right, (float)body.bottom};
	_XUITool::DrawDropShadow(hDraw, bodyF, (float)kXNotifyCornerRadius, state.theme);
	XDraw_SetBrushColor(hDraw, palette.bg);
	XDraw_FillRoundRect(hDraw, &body, kXNotifyCornerRadius, kXNotifyCornerRadius);

	// 左侧信息强调条，颜色沿用当前主题的 accent。
	RECT accent{body.left, body.top + 14, body.left + 4, body.bottom - 14};
	XDraw_SetBrushColor(hDraw, palette.accent);
	XDraw_FillRoundRect(hDraw, &accent, 2, 2);

	XDraw_SetTextRenderingHint(hDraw, 3 /* TextRenderingHintAntiAliasGridFit */);
	XDraw_SetTextAlign(hDraw, textAlignFlag_left | textAlignFlag_top | textFormatFlag_NoWrap);
	XDraw_SetBrushColor(hDraw, palette.text);
	if (state.hTitleFont) XDraw_SetFont(hDraw, state.hTitleFont);
	RECT titleRect{body.left + 20, body.top + 16, body.right - 18, body.top + 42};
	XDraw_DrawText(hDraw, state.title.c_str(), (int)state.title.size(), &titleRect);

	if (state.hTextFont) XDraw_SetFont(hDraw, state.hTextFont);
	XDraw_SetTextAlign(hDraw, textAlignFlag_left | textAlignFlag_top);
	RECT textRect{body.left + 20, body.top + 44, body.right - 18, body.bottom - 12};
	XDraw_DrawText(hDraw, state.text.c_str(), (int)state.text.size(), &textRect);
	return 0;
}

int CALLBACK _XNotify_OnTimer(HELE hEle, UINT timerId, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	auto& state = _XNotify_GetState();
	if (hEle != state.hBody) return 0;

	if (timerId == kXNotifyTimerAutoClose){
		XEle_KillXCTimer(state.hBody, kXNotifyTimerAutoClose);
		_XNotify_StartFade(255, 0, kXNotifyFadeOutMs);
		return 0;
	}
	if (timerId != kXNotifyTimerFade) return 0;

	DWORD elapsed = ::GetTickCount() - state.fadeStartTick;
	BOOL finished = elapsed >= state.fadeDuration;
	int alpha = state.fadeTo;
	if (!finished){
		float t = (float)elapsed / (float)state.fadeDuration;
		// 三次方缓出，出现和消失都保持短促但不突兀。
		float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
		alpha = state.fadeFrom + (int)((state.fadeTo - state.fadeFrom) * eased + 0.5f);
	}
	if (alpha < 0) alpha = 0;
	if (alpha > 255) alpha = 255;
	if (state.hWindow && XC_IsHWINDOW((HXCGUI)state.hWindow)){
		XWnd_SetTransparentAlpha(state.hWindow, (BYTE)alpha);
		XWnd_Redraw(state.hWindow);
	}
	if (!finished) return 0;

	XEle_KillXCTimer(state.hBody, kXNotifyTimerFade);
	if (state.fadeTo == 0){
		if (state.hwnd && ::IsWindow(state.hwnd)) ::ShowWindow(state.hwnd, SW_HIDE);
		state.fadeState = _XNotifyHidden;
	} else {
		state.fadeState = _XNotifyShown;
	}
	return 0;
}

BOOL _XNotify_EnsureWindow()
{
	auto& state = _XNotify_GetState();
	if (state.hWindow && XC_IsHWINDOW((HXCGUI)state.hWindow) &&
		state.hBody && XC_IsHELE((HXCGUI)state.hBody)) return TRUE;

	state.hWindow = XWnd_Create(0, 0, kXNotifyWidth, kXNotifyHeight,
		L"", NULL, window_style_nothing);
	if (!state.hWindow) return FALSE;
	state.hwnd = XWnd_GetHWND(state.hWindow);
	if (!state.hwnd){
		XWnd_DestroyWindow(state.hWindow);
		state.hWindow = NULL;
		return FALSE;
	}

	XWnd_EnableDrawBk(state.hWindow, FALSE);
	XWnd_SetTransparentType(state.hWindow, window_transparent_shaped);
	XWnd_SetTransparentAlpha(state.hWindow, 0);
	XWnd_SetTop(state.hWindow, TRUE);
	LONG_PTR exStyle = ::GetWindowLongPtrW(state.hwnd, GWL_EXSTYLE);
	exStyle |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT;
	::SetWindowLongPtrW(state.hwnd, GWL_EXSTYLE, exStyle);

	state.hBody = XEle_Create(0, 0, kXNotifyWidth, kXNotifyHeight,
		(HXCGUI)state.hWindow);
	if (!state.hBody){
		XWnd_DestroyWindow(state.hWindow);
		state.hWindow = NULL;
		state.hwnd = NULL;
		return FALSE;
	}
	XUI_EnableCSS(state.hBody, FALSE);
	XEle_EnableBkTransparent(state.hBody, TRUE);
	XEle_EnableDrawBorder(state.hBody, FALSE);
	XEle_EnableDrawFocus(state.hBody, FALSE);
	XEle_EnableMouseThrough(state.hBody, TRUE);
	XEle_RegEventC1(state.hBody, XE_PAINT, (void*)&_XNotify_OnPaint);
	XEle_RegEventC1(state.hBody, XE_XC_TIMER, (void*)&_XNotify_OnTimer);

	state.hTitleFont = XFont_CreateEx(L"微软雅黑", 11, fontStyle_bold);
	state.hTextFont = XFont_CreateEx(L"微软雅黑", 9, fontStyle_regular);
	return TRUE;
}

BOOL _XNotify_ShowFallback(HWINDOW hOwner, const wchar_t* title,
	const wchar_t* text, xuitool_theme_ theme, int autoCloseMs)
{
	if (!_XNotify_EnsureWindow()) return FALSE;
	auto& state = _XNotify_GetState();
	_XNotify_HideImmediate();
	state.title = (title && title[0]) ? title : L"通知";
	state.text = text;
	state.theme = theme;

	int dpi = XWnd_GetDPI(hOwner);
	if (dpi < 96) dpi = 96;
	double scale = (double)dpi / 96.0;
	int width = _XNotify_Round(kXNotifyWidth * scale);
	int height = _XNotify_Round(kXNotifyHeight * scale);
	int margin = _XNotify_Round(kXNotifyScreenMargin * scale);

	HWND ownerHwnd = XWnd_GetHWND(hOwner);
	HMONITOR monitor = ::MonitorFromWindow(ownerHwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo{sizeof(monitorInfo)};
	RECT workArea{};
	if (monitor && ::GetMonitorInfoW(monitor, &monitorInfo)){
		workArea = monitorInfo.rcWork;
	} else if (!::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)){
		workArea.right = ::GetSystemMetrics(SM_CXSCREEN);
		workArea.bottom = ::GetSystemMetrics(SM_CYSCREEN);
	}
	int x = workArea.right - width - margin;
	int y = workArea.bottom - height - margin;
	::SetWindowPos(state.hwnd, HWND_TOPMOST, x, y, width, height,
		SWP_NOACTIVATE | SWP_NOSENDCHANGING);
	XEle_SetSize(state.hBody, kXNotifyWidth, kXNotifyHeight, FALSE);
	XWnd_SetTransparentAlpha(state.hWindow, 0);
	::ShowWindow(state.hwnd, SW_SHOWNOACTIVATE);
	XWnd_Redraw(state.hWindow);
	_XNotify_StartFade(0, 255, kXNotifyFadeInMs);
	XEle_SetXCTimer(state.hBody, kXNotifyTimerAutoClose, (UINT)autoCloseMs);
	return TRUE;
}

} // anonymous namespace

xnotify_channel_ CXNotify::ShowTray(HWINDOW hOwner, const wchar_t* pTitle,
	const wchar_t* pText, xuitool_theme_ theme, int autoCloseMs)
{
	if (!hOwner || !XC_IsHWINDOW((HXCGUI)hOwner) || !pText || !pText[0])
		return xnotify_channel_failed;
	const wchar_t* title = (pTitle && pTitle[0]) ? pTitle : L"通知";
	if (GetCurrentVersion() >= 10){
		XTrayIcon_SetPopupBalloon(title, pText, NULL, kXNotifySystemInfoFlag);
		// 托盘图标已添加后，SetPopupBalloon 只更新内部 NOTIFYICONDATA；
		// 必须 Modify 才会把 NIF_INFO 提交给 Windows Shell。
		if (XTrayIcon_Modify()) return xnotify_channel_system;
	}
	if (autoCloseMs < 1500) autoCloseMs = 1500;
	if (autoCloseMs > 30000) autoCloseMs = 30000;
	return _XNotify_ShowFallback(hOwner, title, pText, theme, autoCloseMs)
		? xnotify_channel_xcgui : xnotify_channel_failed;
}

void CXNotify::Cleanup()
{
	auto& state = _XNotify_GetState();
	_XNotify_HideImmediate();
	if (state.hWindow && XC_IsHWINDOW((HXCGUI)state.hWindow))
		XWnd_DestroyWindow(state.hWindow);
	state.hWindow = NULL;
	state.hBody = NULL;
	state.hwnd = NULL;
	if (state.hTitleFont){ XFont_Destroy(state.hTitleFont); state.hTitleFont = NULL; }
	if (state.hTextFont){ XFont_Destroy(state.hTextFont); state.hTextFont = NULL; }
	state.title.clear();
	state.text.clear();
	state.fadeState = _XNotifyHidden;
}
