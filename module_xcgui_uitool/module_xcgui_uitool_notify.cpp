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
constexpr int  kXNotifyWidth          = 400;
constexpr int  kXNotifyHeight         = 132;
constexpr int  kXNotifyMaxHeight      = 220;
constexpr int  kXNotifyShadow         = 20;
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
	std::wstring displayTitle;
	std::wstring displayText;
	xuitool_theme_ theme = xuitool_theme_auto;
	_XNotifyFadeState fadeState = _XNotifyHidden;
	DWORD fadeStartTick = 0;
	DWORD fadeDuration = 1;
	int fadeFrom = 0;
	int fadeTo = 255;
	int alpha = 0;
	int width = kXNotifyWidth;
	int height = kXNotifyHeight;
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

// Shell 的固定缓冲区包含结尾 NUL。按 UTF-16 截断时保留完整代理对。
std::wstring _XNotify_LimitText(const wchar_t* text, size_t limit)
{
	std::wstring result;
	if (!text || !limit) return result;
	size_t length = 0;
	while (length < limit && text[length]) ++length;
	result.assign(text, length);
	if (length == limit && text[length]){
		result.resize(limit - 1);
		if (!result.empty() && result.back() >= 0xD800 && result.back() <= 0xDBFF)
			result.pop_back();
		result += L'\x2026';
	}
	return result;
}

void _XNotify_SetAlpha(int alpha)
{
	auto& state = _XNotify_GetState();
	state.alpha = alpha;
	if (state.hWindow && XC_IsHWINDOW((HXCGUI)state.hWindow)){
		XWnd_SetTransparentAlpha(state.hWindow, (BYTE)alpha);
		XWnd_Redraw(state.hWindow);
	}
}

std::wstring _XNotify_FitText(const std::wstring& text, HFONTX font,
	int width, int height, BOOL multiline)
{
	auto fits = [&](const std::wstring& value){
		SIZE size{};
		if (multiline)
			XC_GetTextShowRect(value.c_str(), (int)value.size(), font, 0, width, &size);
		else
			XC_GetTextShowSizeEx(value.c_str(), (int)value.size(), font, textFormatFlag_NoWrap, &size);
		return size.cx <= width && size.cy <= height;
	};
	if (fits(text)) return text;
	// 预先裁短，绕开部分 XCGUI D2D 版本中 NoWrap + Ellipsis 仍换行的问题。
	size_t lo = 0, hi = text.size();
	std::wstring result = L"\x2026";
	while (lo < hi){
		size_t mid = lo + (hi - lo) / 2;
		std::wstring candidate = text.substr(0, mid);
		if (!candidate.empty() && candidate.back() >= 0xD800 && candidate.back() <= 0xDBFF)
			candidate.pop_back();
		candidate += L'\x2026';
		if (fits(candidate)) { result = candidate; lo = mid + 1; }
		else hi = mid;
	}
	return result;
}

void _XNotify_HideImmediate()
{
	auto& state = _XNotify_GetState();
	if (state.hBody && XC_IsHELE((HXCGUI)state.hBody)){
		XEle_KillXCTimer(state.hBody, kXNotifyTimerFade);
		XEle_KillXCTimer(state.hBody, kXNotifyTimerAutoClose);
	}
	_XNotify_SetAlpha(0);
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
	if (from == to || !XEle_SetXCTimer(state.hBody, kXNotifyTimerFade, kXNotifyFadeTickMs)){
		XEle_KillXCTimer(state.hBody, kXNotifyTimerFade);
		_XNotify_SetAlpha(to);
		state.fadeState = to ? _XNotifyShown : _XNotifyHidden;
		if (!to && ::IsWindow(state.hwnd)) ::ShowWindow(state.hwnd, SW_HIDE);
	}
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
		state.width - kXNotifyShadow,
		state.height - kXNotifyShadow
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
	// GDI+ 的 hint 不影响 D2D。透明窗口须使用灰度抗锯齿，避免文字出现彩边。
	auto* renderTarget = (ID2D1RenderTarget*)XDraw_GetD2dRenderTarget(hDraw);
	D2D1_TEXT_ANTIALIAS_MODE oldTextAA = D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
	if (renderTarget){
		oldTextAA = renderTarget->GetTextAntialiasMode();
		renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	}
	XDraw_SetTextAlign(hDraw, textAlignFlag_left | textAlignFlag_top | textFormatFlag_NoWrap);
	XDraw_SetBrushColor(hDraw, palette.text);
	if (state.hTitleFont) XDraw_SetFont(hDraw, state.hTitleFont);
	RECT titleRect{body.left + 20, body.top + 16, body.right - 18, body.top + 42};
	XDraw_DrawText(hDraw, state.displayTitle.c_str(), (int)state.displayTitle.size(), &titleRect);

	if (state.hTextFont) XDraw_SetFont(hDraw, state.hTextFont);
	XDraw_SetTextAlign(hDraw, textAlignFlag_left | textAlignFlag_top);
	RECT textRect{body.left + 20, body.top + 44, body.right - 18, body.bottom - 12};
	XDraw_DrawText(hDraw, state.displayText.c_str(), (int)state.displayText.size(), &textRect);
	if (renderTarget) renderTarget->SetTextAntialiasMode(oldTextAA);
	return 0;
}

int CALLBACK _XNotify_OnTimer(HELE hEle, UINT timerId, BOOL* pbHandled)
{
	auto& state = _XNotify_GetState();
	if (hEle != state.hBody) return 0;
	if (timerId != kXNotifyTimerAutoClose && timerId != kXNotifyTimerFade) return 0;
	if (pbHandled) *pbHandled = TRUE;

	if (timerId == kXNotifyTimerAutoClose){
		XEle_KillXCTimer(state.hBody, kXNotifyTimerAutoClose);
		_XNotify_StartFade(state.alpha, 0, kXNotifyFadeOutMs);
		return 0;
	}
	if (state.fadeState != _XNotifyFadeIn && state.fadeState != _XNotifyFadeOut) return 0;

	DWORD elapsed = ::GetTickCount() - state.fadeStartTick;
	BOOL finished = elapsed >= state.fadeDuration;
	int alpha = state.fadeTo;
	if (!finished){
		float t = (float)elapsed / (float)state.fadeDuration;
		// 三次方缓出，出现和消失都保持短促但不突兀。
		float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
		alpha = state.fadeFrom + _XNotify_Round((state.fadeTo - state.fadeFrom) * eased);
	}
	if (alpha < 0) alpha = 0;
	if (alpha > 255) alpha = 255;
	_XNotify_SetAlpha(alpha);
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

void _XNotify_ReleaseState()
{
	auto& state = _XNotify_GetState();
	if (state.hTitleFont) XFont_Destroy(state.hTitleFont);
	if (state.hTextFont) XFont_Destroy(state.hTextFont);
	state = _XNotifyState{};
}

int CALLBACK _XNotify_OnDestroy(HWINDOW window, BOOL*)
{
	if (window == _XNotify_GetState().hWindow) _XNotify_ReleaseState();
	return 0;
}

BOOL _XNotify_EnsureWindow()
{
	auto& state = _XNotify_GetState();
	if (state.hWindow && XC_IsHWINDOW((HXCGUI)state.hWindow) &&
		state.hBody && XC_IsHELE((HXCGUI)state.hBody) && ::IsWindow(state.hwnd)) return TRUE;
	CXNotify::Cleanup();

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
	if (!XWnd_RegEventC1(state.hWindow, WM_DESTROY, (void*)&_XNotify_OnDestroy)){
		CXNotify::Cleanup();
		return FALSE;
	}

	state.hTitleFont = XFont_CreateEx(L"微软雅黑", 11, fontStyle_bold);
	state.hTextFont = XFont_CreateEx(L"微软雅黑", 9, fontStyle_regular);
	// 字体只用于直接绘制，由通知窗拥有；不交给 XCGUI 的自动回收管理。
	if (state.hTitleFont) XFont_EnableAutoDestroy(state.hTitleFont, FALSE);
	if (state.hTextFont) XFont_EnableAutoDestroy(state.hTextFont, FALSE);
	if (!state.hTitleFont || !state.hTextFont){
		CXNotify::Cleanup();
		return FALSE;
	}
	return TRUE;
}

BOOL _XNotify_ShowFallback(HWINDOW hOwner, const wchar_t* title,
	const wchar_t* text, xuitool_theme_ theme, int autoCloseMs)
{
	if (!_XNotify_EnsureWindow()) return FALSE;
	auto& state = _XNotify_GetState();
	XEle_KillXCTimer(state.hBody, kXNotifyTimerFade);
	XEle_KillXCTimer(state.hBody, kXNotifyTimerAutoClose);
	// 连续通知复用当前透明度，避免每次都隐藏、重新淡入造成闪烁。
	int alpha = ::IsWindowVisible(state.hwnd) ? state.alpha : 0;
	state.title = _XNotify_LimitText((title && title[0]) ? title : L"通知", 256);
	std::replace(state.title.begin(), state.title.end(), L'\r', L' ');
	std::replace(state.title.begin(), state.title.end(), L'\n', L' ');
	state.text = _XNotify_LimitText(text, 4096);
	state.theme = _XUITool::EffectiveTheme(theme);

	int dpi = XWnd_GetDPI(hOwner);
	if (dpi < 96) dpi = 96;
	double scale = (double)dpi / 96.0;
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
	int availableWidth = workArea.right - workArea.left - 2 * margin;
	int availableHeight = workArea.bottom - workArea.top - 2 * margin;
	state.width = (std::min)(kXNotifyWidth, (int)(availableWidth / scale));
	int maxHeight = (std::min)(kXNotifyMaxHeight, (int)(availableHeight / scale));
	if (state.width < 160 || maxHeight < kXNotifyHeight){
		_XNotify_HideImmediate();
		return FALSE;
	}
	SIZE textSize{};
	XC_GetTextShowRect(state.text.c_str(), (int)state.text.size(), state.hTextFont,
		textAlignFlag_left | textAlignFlag_top, state.width - 2 * kXNotifyShadow - 38, &textSize);
	state.height = (std::min)(maxHeight, (std::max)(kXNotifyHeight,
		(int)textSize.cy + 2 * kXNotifyShadow + 58));
	int textWidth = state.width - 2 * kXNotifyShadow - 40;
	state.displayTitle = _XNotify_FitText(state.title, state.hTitleFont, textWidth, 26, FALSE);
	state.displayText = _XNotify_FitText(state.text, state.hTextFont, textWidth,
		state.height - 2 * kXNotifyShadow - 58, TRUE);
	int width = _XNotify_Round(state.width * scale);
	int height = _XNotify_Round(state.height * scale);
	int x = workArea.right - width - margin;
	int y = workArea.bottom - height - margin;
	if (!::SetWindowPos(state.hwnd, HWND_TOPMOST, x, y, width, height,
		SWP_NOACTIVATE | SWP_NOSENDCHANGING)){
		_XNotify_HideImmediate();
		return FALSE;
	}
	// 移动到目标屏幕后显式同步布局 DPI；仅缩放 HWND 会让背景和文字被裁切。
	XWnd_SetDPI(state.hWindow, dpi);
	XEle_SetSize(state.hBody, state.width, state.height, FALSE);
	_XNotify_SetAlpha(alpha);
	if (!XEle_SetXCTimer(state.hBody, kXNotifyTimerAutoClose, (UINT)autoCloseMs)){
		_XNotify_HideImmediate();
		return FALSE;
	}
	::ShowWindow(state.hwnd, SW_SHOWNOACTIVATE);
	XWnd_Redraw(state.hWindow);
	_XNotify_StartFade(alpha, 255, kXNotifyFadeInMs * (255 - alpha) / 255);
	return TRUE;
}

} // anonymous namespace

xnotify_channel_ CXNotify::ShowTray(HWINDOW hOwner, const wchar_t* pTitle,
	const wchar_t* pText, xuitool_theme_ theme, int autoCloseMs)
{
	if (!hOwner || !XC_IsHWINDOW((HXCGUI)hOwner) ||
		!::IsWindow(XWnd_GetHWND(hOwner)) || !pText || !pText[0])
		return xnotify_channel_failed;
	const wchar_t* title = (pTitle && pTitle[0]) ? pTitle : L"通知";
	if (GetCurrentVersion() >= 10){
		std::wstring systemTitle = _XNotify_LimitText(title, 63);
		std::wstring systemText = _XNotify_LimitText(pText, 255);
		XTrayIcon_SetPopupBalloon(systemTitle.c_str(), systemText.c_str(), NULL, kXNotifySystemInfoFlag);
		// 托盘图标已添加后，SetPopupBalloon 只更新内部 NOTIFYICONDATA；
		// 必须 Modify 才会把 NIF_INFO 提交给 Windows Shell。
		BOOL submitted = XTrayIcon_Modify();
		// 清掉 SDK 缓存，避免后续修改/重新添加托盘时重放旧通知。
		// 此处不再 Modify，防止把刚提交的系统通知立即撤掉。
		XTrayIcon_SetPopupBalloon(L"", L"", NULL, 0);
		if (submitted){
			_XNotify_HideImmediate();
			return xnotify_channel_system;
		}
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
	_XNotify_ReleaseState();
}
