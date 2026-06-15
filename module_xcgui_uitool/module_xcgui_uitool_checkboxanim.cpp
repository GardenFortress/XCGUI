//============================================================================
// module_xcgui_uitool_checkboxanim.cpp — CXCheckAnim (WinUI3 风格多选框动画)
// 仅由 module_xcgui_uitool.cpp #include; 勿单独编译.
//============================================================================
#ifndef _XCGUI_UITOOL_AGGREGATED_
#else

namespace {

constexpr int  kChk_TrackW       = 32;
constexpr int  kChk_TrackH       = 20;
constexpr int  kChk_ThumbNormalW = 10;
constexpr int  kChk_ThumbPressW  = 12;
constexpr int  kChk_TextGap      = 6;
constexpr int  kChk_TrackPad     = 4;
constexpr int  kChk_AnimMs       = 200;
constexpr int  kChk_PressAnimMs  = 120;
constexpr int  kChk_FastAnimMs   = 100;
constexpr int  kChk_Ease         = ease_flag_quad | ease_flag_out;

inline BYTE _XChk_A(COLORREF c){ return (BYTE)((c >> 24) & 0xFF); }
inline BYTE _XChk_R(COLORREF c){ return (BYTE)(c & 0xFF); }
inline BYTE _XChk_G(COLORREF c){ return (BYTE)((c >> 8) & 0xFF); }
inline BYTE _XChk_B(COLORREF c){ return (BYTE)((c >> 16) & 0xFF); }
inline COLORREF _XChk_ARGB(BYTE r, BYTE g, BYTE b, BYTE a){
	return ((COLORREF)a << 24) | ((COLORREF)b << 16) | ((COLORREF)g << 8) | (COLORREF)r;
}
inline COLORREF _XChk_LerpColor(COLORREF c0, COLORREF c1, float t){
	auto l = [](BYTE a, BYTE b, float f)->BYTE {
		return (BYTE)(a + (b - a) * f + 0.5f);
	};
	return _XChk_ARGB(
		l(_XChk_R(c0), _XChk_R(c1), t),
		l(_XChk_G(c0), _XChk_G(c1), t),
		l(_XChk_B(c0), _XChk_B(c1), t),
		l(_XChk_A(c0), _XChk_A(c1), t));
}

struct _XChk_Palette
{
	COLORREF trackLeave;
	COLORREF trackHover;
	COLORREF trackPress;
	COLORREF trackDisable;
	COLORREF thumbNormal;
	COLORREF thumbDisable;
};

struct _XChk_AnimSlot
{
	BOOL     active      = FALSE;
	UINT     runToken    = 0;
	UINT     startToken  = 0;
	float    fromThumbX  = 0.f;
	float    toThumbX    = 0.f;
	float    fromThumbW  = (float)kChk_ThumbNormalW;
	float    toThumbW    = (float)kChk_ThumbNormalW;
	COLORREF fromTrack   = 0;
	COLORREF toTrack     = 0;
	HXCGUI   hAnima      = NULL;
};

struct _XChk_Entry
{
	HELE                  hBtn           = NULL;
	std::wstring          cachedTitle;
	size_t                titleHash      = 0;
	xcheckanim_text_align_ textAlign      = xcheckanim_text_align_left;
	xuitool_theme_        theme           = xuitool_theme_auto;
	BOOL                  bAnimEnabled   = TRUE;
	BOOL                  eventsHooked   = FALSE;
	BOOL                  bHover         = FALSE;
	BOOL                  bPressed       = FALSE;
	BOOL                  bCapture       = FALSE;
	BOOL                  bCheckAtPress  = FALSE;
	BOOL                  bProgrammatic  = FALSE;
	float                 thumbX         = 0.f;
	float                 thumbW         = (float)kChk_ThumbNormalW;
	COLORREF              trackColor     = 0;
	_XChk_AnimSlot        anim;
};

struct _XChk_Global
{
	std::unordered_map<HELE, _XChk_Entry> registry;
	struct Pending
	{
		BOOL                   hasTheme    = FALSE;
		xuitool_theme_         theme       = xuitool_theme_auto;
		BOOL                   hasTextAlign = FALSE;
		xcheckanim_text_align_ textAlign   = xcheckanim_text_align_left;
	};
	std::unordered_map<HELE, Pending> pending;
};

_XChk_Global& _XChk_G()
{
	static _XChk_Global s;
	return s;
}

inline BOOL _XChk_IsBtn(HELE hBtn)
{
	return hBtn && XC_IsHELE((HXCGUI)hBtn) && XC_GetObjectType((HXCGUI)hBtn) == XC_BUTTON;
}

inline size_t _XChk_HashText(const wchar_t* p)
{
	if (!p || !p[0]) return 0;
	size_t h = 1469598103934665603ull;
	for (const wchar_t* q = p; *q; ++q){
		h ^= (size_t)*q;
		h *= 1099511628211ull;
	}
	return h;
}

inline BOOL _XChk_IsLight(const _XChk_Entry& e)
{
	return _XUITool::IsLightTheme(e.theme);
}

// §8 样式 — 轨道/圆点配色 (ARGB). 禁用态 alpha=128 (#80xxxxxx).
struct _XChk_StateColors
{
	COLORREF trackLeave;
	COLORREF trackHover;
	COLORREF trackPress;
	COLORREF trackDisable;
};

static const _XChk_StateColors kChk_DarkOff = {
	(COLORREF)RGBA(81, 81, 81, 255),    // #515151 离开/弹起
	(COLORREF)RGBA(69, 69, 69, 255),    // #454545 进入
	(COLORREF)RGBA(40, 40, 40, 255),    // #282828 按下
	(COLORREF)RGBA(69, 69, 69, 128),    // #80454545 禁用
};
static const _XChk_StateColors kChk_LightOff = {
	(COLORREF)RGBA(204, 204, 204, 255), // #CCCCCC
	(COLORREF)RGBA(176, 176, 176, 255), // #B0B0B0
	(COLORREF)RGBA(224, 224, 224, 255), // #E0E0E0
	(COLORREF)RGBA(202, 202, 202, 128), // #80CACACA
};
static const _XChk_StateColors kChk_DarkOn = {
	(COLORREF)RGBA(0, 102, 204, 255),   // #0066CC
	(COLORREF)RGBA(23, 128, 232, 255),  // #1780E8
	(COLORREF)RGBA(0, 89, 179, 255),    // #0059B3
	(COLORREF)RGBA(0, 82, 163, 128),    // #800052A3
};
static const _XChk_StateColors kChk_LightOn = {
	(COLORREF)RGBA(0, 153, 255, 255),   // #0099FF
	(COLORREF)RGBA(20, 161, 255, 255),  // #14A1FF
	(COLORREF)RGBA(0, 132, 219, 255),   // #0084DB
	(COLORREF)RGBA(41, 169, 255, 128),  // #8029A9FF
};

void _XChk_FillPalette(const _XChk_Entry& e, BOOL bChecked, _XChk_Palette* out)
{
	const BOOL light = _XChk_IsLight(e);
	const _XChk_StateColors& sc = light
		? (bChecked ? kChk_LightOn : kChk_LightOff)
		: (bChecked ? kChk_DarkOn : kChk_DarkOff);
	out->trackLeave   = sc.trackLeave;
	out->trackHover   = sc.trackHover;
	out->trackPress   = sc.trackPress;
	out->trackDisable = sc.trackDisable;
	out->thumbNormal  = RGBA(255, 255, 255, 255); // #FFFFFF
	out->thumbDisable = RGBA(255, 255, 255, 128); // #80FFFFFF
}

COLORREF _XChk_TargetTrack(const _XChk_Entry& e, BOOL bChecked, BOOL bDisabled)
{
	_XChk_Palette pal{};
	_XChk_FillPalette(e, bChecked, &pal);
	if (bDisabled) return pal.trackDisable;
	if (e.bPressed) return pal.trackPress;
	if (e.bHover) return pal.trackHover;
	return pal.trackLeave;
}

COLORREF _XChk_TargetThumb(const _XChk_Entry& e, BOOL bDisabled)
{
	_XChk_Palette pal{};
	_XChk_FillPalette(e, XBtn_IsCheck(e.hBtn) == TRUE, &pal);
	return bDisabled ? pal.thumbDisable : pal.thumbNormal;
}

float _XChk_TargetThumbX(BOOL bChecked)
{
	return bChecked ? 1.f : 0.f;
}

float _XChk_TargetThumbW(const _XChk_Entry& e)
{
	return e.bPressed ? (float)kChk_ThumbPressW : (float)kChk_ThumbNormalW;
}

void _XChk_StopAnim(_XChk_Entry& e, BOOL bRunToEnd = FALSE)
{
	if (!e.hBtn) return;
	e.anim.active = FALSE;
	e.anim.runToken++;
	HXCGUI hAnima = e.anim.hAnima;
	e.anim.hAnima = NULL;
	if (hAnima){
		XAnima_Stop(hAnima);
		XAnima_Release(hAnima, bRunToEnd);
	}
	XAnima_ReleaseEx((HXCGUI)e.hBtn, FALSE);
}

void _XChk_StartAnim(_XChk_Entry& e, UINT ms, float toThumbX, float toThumbW, COLORREF toTrack,
	BOOL bInterrupt = FALSE)
{
	if (!e.bAnimEnabled){
		e.thumbX = toThumbX;
		e.thumbW = toThumbW;
		e.trackColor = toTrack;
		XEle_Redraw(e.hBtn, FALSE);
		return;
	}

	if (bInterrupt && ms > (UINT)kChk_FastAnimMs)
		ms = (UINT)kChk_FastAnimMs;
	_XChk_StopAnim(e, FALSE);

	e.anim.active     = TRUE;
	e.anim.startToken = e.anim.runToken;
	e.anim.fromThumbX = e.thumbX;
	e.anim.toThumbX   = toThumbX;
	e.anim.fromThumbW = e.thumbW;
	e.anim.toThumbW   = toThumbW;
	e.anim.fromTrack  = e.trackColor;
	e.anim.toTrack    = toTrack;

	HXCGUI hAnima = XAnima_Create((HXCGUI)e.hBtn, 1);
	if (!hAnima){
		e.thumbX = toThumbX;
		e.thumbW = toThumbW;
		e.trackColor = toTrack;
		e.anim.active = FALSE;
		XEle_Redraw(e.hBtn, FALSE);
		return;
	}
	XAnima_EnableAutoDestroy(hAnima, TRUE);
	HXCGUI hItem = XAnima_DelayEx(hAnima, (float)ms, 1, kChk_Ease, FALSE);
	XAnimaItem_SetUserData(hItem, (vint)e.hBtn);
	XAnimaItem_SetCallback(hItem, CXCheckAnim::AnimItemCb);
	e.anim.hAnima = hAnima;
	XAnima_Run(hAnima, (HXCGUI)e.hBtn);
}

void _XChk_RefreshVisual(_XChk_Entry& e, BOOL bAnimate, BOOL bInterrupt = FALSE)
{
	const BOOL disabled = !XEle_IsEnable(e.hBtn);
	const BOOL checked  = XBtn_IsCheck(e.hBtn) == TRUE;
	const COLORREF toTrack = _XChk_TargetTrack(e, checked, disabled);
	const float toX = _XChk_TargetThumbX(checked);
	const float toW = _XChk_TargetThumbW(e);
	if (bAnimate && e.bAnimEnabled){
		const UINT ms = e.bPressed ? (UINT)kChk_PressAnimMs : (UINT)kChk_AnimMs;
		_XChk_StartAnim(e, ms, toX, toW, toTrack, bInterrupt);
	}else{
		e.thumbX = toX;
		e.thumbW = toW;
		e.trackColor = toTrack;
		XEle_Redraw(e.hBtn, FALSE);
	}
}

UINT _XChk_CalcMinWidth(HELE hBtn, const wchar_t* pText)
{
	if (!pText || !pText[0]) return (UINT)kChk_TrackW;
	SIZE sz = {};
	HFONTX hFont = XEle_GetFont(hBtn);
	if (!hFont) hFont = XFont_CreateEx(L"微软雅黑", 12, 0);
	XC_GetTextSize(pText, (int)wcslen(pText), hFont, &sz);
	if (hFont && hFont != XEle_GetFont(hBtn)) XFont_Destroy(hFont);
	return (UINT)(sz.cx + kChk_TextGap + kChk_TrackW);
}

void _XChk_ApplyButtonSize(_XChk_Entry& e, UINT reqW, UINT reqH)
{
	UINT minW = _XChk_CalcMinWidth(e.hBtn, e.cachedTitle.c_str());
	UINT w = reqW > 0 ? reqW : minW;
	if (w < minW) w = minW;
	UINT h = reqH > 0 ? reqH : (UINT)kChk_TrackH;
	XEle_SetWidth(e.hBtn, (int)w);
	XEle_SetHeight(e.hBtn, (int)h);
}

void _XChk_EnsureButtonWidth(_XChk_Entry& e)
{
	if (e.cachedTitle.empty()) return;
	UINT minW = _XChk_CalcMinWidth(e.hBtn, e.cachedTitle.c_str());
	RECT rc{};
	XEle_GetClientRect(e.hBtn, &rc);
	const int curW = rc.right - rc.left;
	if (curW < (int)minW)
		XEle_SetWidth(e.hBtn, (int)minW);
}

void _XChk_ApplyBtnTextAlign(HELE hBtn, xcheckanim_text_align_ align)
{
	XBtn_SetTextAlign(hBtn, textAlignFlag_vcenter
		| (align == xcheckanim_text_align_left ? textAlignFlag_left : textAlignFlag_right));
}

void _XChk_MergePending(HELE hBtn, _XChk_Entry& e, xuitool_theme_ attachTheme, BOOL bFirstAttach)
{
	auto& pending = _XChk_G().pending;
	auto it = pending.find(hBtn);
	if (it != pending.end()){
		if (it->second.hasTheme)
			e.theme = it->second.theme;
		else if (bFirstAttach)
			e.theme = attachTheme;
		if (it->second.hasTextAlign)
			e.textAlign = it->second.textAlign;
		pending.erase(it);
	}else if (bFirstAttach){
		e.theme = attachTheme;
	}
}

void _XChk_StorePendingTheme(HELE hBtn, xuitool_theme_ theme)
{
	auto& p = _XChk_G().pending[hBtn];
	p.hasTheme = TRUE;
	p.theme = theme;
}

void _XChk_StorePendingTextAlign(HELE hBtn, xcheckanim_text_align_ align)
{
	auto& p = _XChk_G().pending[hBtn];
	p.hasTextAlign = TRUE;
	p.textAlign = align;
}

void _XChk_SyncExternalTitle(_XChk_Entry& e)
{
	const wchar_t* live = XBtn_GetText(e.hBtn);
	if (!live || !live[0]) return;
	size_t h = _XChk_HashText(live);
	if (h == e.titleHash) return;
	e.cachedTitle = live;
	e.titleHash = h;
	XBtn_SetText(e.hBtn, L"");
	_XChk_ApplyButtonSize(e, 0, (UINT)kChk_TrackH);
}

void _XChk_CalcTrackRect(const _XChk_Entry& e, const RECT& rc, RECT* rcTrack)
{
	const int h = rc.bottom - rc.top;
	const int y = rc.top + (h - kChk_TrackH) / 2;
	if (e.textAlign == xcheckanim_text_align_left){
		rcTrack->left   = rc.left;
		rcTrack->right  = rc.left + kChk_TrackW;
	}else{
		rcTrack->right  = rc.right;
		rcTrack->left   = rc.right - kChk_TrackW;
	}
	rcTrack->top    = y;
	rcTrack->bottom = y + kChk_TrackH;
}

void _XChk_CalcTextRect(const _XChk_Entry& e, const RECT& rc, const RECT& rcTrack, RECT* rcText)
{
	if (e.textAlign == xcheckanim_text_align_left){
		rcText->left   = rcTrack.right + kChk_TextGap;
		rcText->right  = rc.right;
	}else{
		rcText->left   = rc.left;
		rcText->right  = rcTrack.left - kChk_TextGap;
	}
	rcText->top    = rc.top;
	rcText->bottom = rc.bottom;
}

void _XChk_DrawSwitch(_XChk_Entry& e, HDRAW hDraw, const RECT& rcClient)
{
	RECT rcTrack{};
	_XChk_CalcTrackRect(e, rcClient, &rcTrack);

	const BOOL disabled = !XEle_IsEnable(e.hBtn);
	const COLORREF thumbColor = _XChk_TargetThumb(e, disabled);

	XDraw_SetBrushColor(hDraw, e.trackColor);
	RECTF rfTrack = {
		(float)rcTrack.left, (float)rcTrack.top,
		(float)rcTrack.right, (float)rcTrack.bottom
	};
	XDraw_FillRoundRectF(hDraw, &rfTrack, (float)kChk_TrackH * 0.5f, (float)kChk_TrackH * 0.5f);

	const float innerW = (float)(kChk_TrackW - kChk_TrackPad * 2);
	const float maxTravel = innerW - e.thumbW;
	const float xOff = (float)rcTrack.left + (float)kChk_TrackPad + e.thumbX * maxTravel;
	const float yOff = (float)rcTrack.top + ((float)kChk_TrackH - e.thumbW) * 0.5f;
	RECTF rfThumb = { xOff, yOff, xOff + e.thumbW, yOff + e.thumbW };
	XDraw_SetBrushColor(hDraw, thumbColor);
	XDraw_FillRoundRectF(hDraw, &rfThumb, e.thumbW * 0.5f, e.thumbW * 0.5f);

	if (!e.cachedTitle.empty()){
		RECT rcText{};
		_XChk_CalcTextRect(e, rcClient, rcTrack, &rcText);
		_XUITool::ThemePalette pal{};
		_XUITool::ResolvePalette(e.theme, 0, 0, 0, &pal);
		COLORREF textColor = disabled ? _XUITool::WithAlpha(pal.text, 128) : pal.text;
		HFONTX hFont = XEle_GetFont(e.hBtn);
		XDraw_SetFont(hDraw, hFont);
		XDraw_SetBrushColor(hDraw, textColor);
		int align = textAlignFlag_vcenter | textFormatFlag_NoWrap;
		align |= (e.textAlign == xcheckanim_text_align_left) ? textAlignFlag_left : textAlignFlag_right;
		XDraw_SetTextAlign(hDraw, align);
		XDraw_DrawText(hDraw, e.cachedTitle.c_str(), (int)e.cachedTitle.size(), &rcText);
	}
}

_XChk_Entry* _XChk_Find(HELE hBtn)
{
	auto it = _XChk_G().registry.find(hBtn);
	return (it != _XChk_G().registry.end()) ? &it->second : NULL;
}

void _XChk_HookEvents(HELE hBtn, _XChk_Entry& e)
{
	if (e.eventsHooked) return;
	XEle_RegEventC1(hBtn, XE_PAINT,        (void*)&CXCheckAnim::OnPaintC);
	XEle_EnableEvent_XE_PAINT_END(hBtn, TRUE);
	XEle_RegEventC1(hBtn, XE_PAINT_END,    (void*)&CXCheckAnim::OnPaintEndC);
	XEle_RegEventC1(hBtn, XE_MOUSEHOVER,   (void*)&CXCheckAnim::OnMouseHoverC);
	XEle_RegEventC1(hBtn, XE_MOUSELEAVE,   (void*)&CXCheckAnim::OnMouseLeaveC);
	XEle_RegEventC1(hBtn, XE_LBUTTONDOWN,  (void*)&CXCheckAnim::OnLButtonDownC);
	XEle_RegEventC1(hBtn, XE_LBUTTONUP,    (void*)&CXCheckAnim::OnLButtonUpC);
	XEle_RegEventC1(hBtn, XE_BNCLICK,       (void*)&CXCheckAnim::OnBnClickC);
	XEle_RegEventC1(hBtn, XE_BUTTON_CHECK, (void*)&CXCheckAnim::OnButtonCheckC);
	XEle_RegEventC1(hBtn, XE_DESTROY,      (void*)&CXCheckAnim::OnDestroyC);
	e.eventsHooked = TRUE;
}

void _XChk_UnhookEvents(HELE hBtn, _XChk_Entry& e)
{
	if (!e.eventsHooked) return;
	XEle_RemoveEventC(hBtn, XE_PAINT,        (void*)&CXCheckAnim::OnPaintC);
	XEle_RemoveEventC(hBtn, XE_PAINT_END,    (void*)&CXCheckAnim::OnPaintEndC);
	XEle_RemoveEventC(hBtn, XE_MOUSEHOVER,   (void*)&CXCheckAnim::OnMouseHoverC);
	XEle_RemoveEventC(hBtn, XE_MOUSELEAVE,   (void*)&CXCheckAnim::OnMouseLeaveC);
	XEle_RemoveEventC(hBtn, XE_LBUTTONDOWN,  (void*)&CXCheckAnim::OnLButtonDownC);
	XEle_RemoveEventC(hBtn, XE_LBUTTONUP,    (void*)&CXCheckAnim::OnLButtonUpC);
	XEle_RemoveEventC(hBtn, XE_BNCLICK,       (void*)&CXCheckAnim::OnBnClickC);
	XEle_RemoveEventC(hBtn, XE_BUTTON_CHECK, (void*)&CXCheckAnim::OnButtonCheckC);
	XEle_RemoveEventC(hBtn, XE_DESTROY,      (void*)&CXCheckAnim::OnDestroyC);
	e.eventsHooked = FALSE;
}

BOOL _XChk_PtInClient(HELE hBtn, POINT* pPt)
{
	if (!pPt) return FALSE;
	RECT rc{};
	XEle_GetClientRect(hBtn, &rc);
	return pPt->x >= rc.left && pPt->x < rc.right && pPt->y >= rc.top && pPt->y < rc.bottom;
}

} // namespace

void CALLBACK CXCheckAnim::AnimItemCb(HXCGUI hAnimItem, float pos)
{
	HELE hBtn = (HELE)XAnimaItem_GetUserData(hAnimItem);
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e || !e->anim.active || e->anim.runToken != e->anim.startToken) return;
	const float t = pos;
	e->thumbX     = e->anim.fromThumbX + (e->anim.toThumbX - e->anim.fromThumbX) * t;
	e->thumbW     = e->anim.fromThumbW + (e->anim.toThumbW - e->anim.fromThumbW) * t;
	e->trackColor = _XChk_LerpColor(e->anim.fromTrack, e->anim.toTrack, t);
	XEle_Redraw(hBtn, FALSE);
	if (t >= 1.f){
		e->thumbX     = e->anim.toThumbX;
		e->thumbW     = e->anim.toThumbW;
		e->trackColor = e->anim.toTrack;
		e->anim.active = FALSE;
		e->anim.hAnima = NULL;
	}
}

int CALLBACK CXCheckAnim::OnPaintC(HELE hBtn, HDRAW hDraw, BOOL* pbHandled)
{
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e){ if (pbHandled) *pbHandled = FALSE; return 0; }
	_XChk_SyncExternalTitle(*e);
	_XChk_EnsureButtonWidth(*e);
	if (pbHandled) *pbHandled = TRUE;
	RECT rc{};
	XEle_GetClientRect(hBtn, &rc);
	_XChk_DrawSwitch(*e, hDraw, rc);
	return 0;
}

int CALLBACK CXCheckAnim::OnPaintEndC(HELE hBtn, HDRAW hDraw, BOOL* pbHandled)
{
	(void)hDraw;
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e){ if (pbHandled) *pbHandled = FALSE; return 0; }
	if (pbHandled) *pbHandled = TRUE;
	return 0;
}

int CALLBACK CXCheckAnim::OnMouseHoverC(HELE hBtn, UINT nFlags, POINT* pPt, BOOL* pbHandled)
{
	(void)nFlags; (void)pPt; (void)pbHandled;
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e || e->bCapture) return 0;
	if (e->bHover) return 0;
	e->bHover = TRUE;
	_XChk_RefreshVisual(*e, TRUE);
	return 0;
}

int CALLBACK CXCheckAnim::OnMouseLeaveC(HELE hBtn, BOOL* pbHandled)
{
	(void)pbHandled;
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e || e->bCapture) return 0;
	if (!e->bHover && !e->bPressed) return 0;
	e->bHover = FALSE;
	if (!e->bPressed) _XChk_RefreshVisual(*e, TRUE);
	return 0;
}

int CALLBACK CXCheckAnim::OnLButtonDownC(HELE hBtn, UINT nFlags, POINT* pPt, BOOL* pbHandled)
{
	(void)nFlags; (void)pPt;
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e || !XEle_IsEnable(hBtn)) return 0;
	if (pbHandled) *pbHandled = TRUE;
	const BOOL bInterrupt = e->anim.active || e->anim.hAnima;
	if (bInterrupt)
		_XChk_StopAnim(*e, FALSE);
	else
		XAnima_ReleaseEx((HXCGUI)hBtn, FALSE);
	e->bCheckAtPress = XBtn_IsCheck(hBtn) == TRUE;
	e->bPressed = TRUE;
	e->bCapture = TRUE;
	XEle_SetCapture(hBtn, TRUE);
	_XChk_RefreshVisual(*e, TRUE, bInterrupt);
	return 0;
}

int CALLBACK CXCheckAnim::OnLButtonUpC(HELE hBtn, UINT nFlags, POINT* pPt, BOOL* pbHandled)
{
	(void)nFlags;
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e) return 0;
	if (pbHandled) *pbHandled = TRUE;
	if (!e->bPressed) return 0;
	if (e->bCapture){
		XEle_SetCapture(hBtn, FALSE);
		e->bCapture = FALSE;
	}
	const BOOL inside = _XChk_PtInClient(hBtn, pPt);
	const BOOL wasCheck = e->bCheckAtPress;
	const BOOL bInterrupt = e->anim.active || e->anim.hAnima;
	e->bPressed = FALSE;
	if (bInterrupt) _XChk_StopAnim(*e, FALSE);
	if (inside){
		const BOOL want = !wasCheck;
		e->bProgrammatic = TRUE;
		XBtn_SetCheck(hBtn, want ? TRUE : FALSE);
		e->bProgrammatic = FALSE;
		_XChk_RefreshVisual(*e, TRUE, bInterrupt);
	}else{
		e->bProgrammatic = TRUE;
		XBtn_SetCheck(hBtn, wasCheck ? TRUE : FALSE);
		e->bProgrammatic = FALSE;
		_XChk_RefreshVisual(*e, TRUE, bInterrupt);
	}
	return 0;
}

int CALLBACK CXCheckAnim::OnBnClickC(HELE hBtn, BOOL* pbHandled)
{
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e) return 0;
	if (pbHandled) *pbHandled = TRUE;
	return 0;
}

int CALLBACK CXCheckAnim::OnButtonCheckC(HELE hBtn, BOOL bCheck, BOOL* pbHandled)
{
	(void)bCheck;
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e) return 0;
	if (e->bProgrammatic) return 0;
	if (e->bPressed){
		if (pbHandled) *pbHandled = TRUE;
		e->bProgrammatic = TRUE;
		XBtn_SetCheck(hBtn, e->bCheckAtPress ? TRUE : FALSE);
		e->bProgrammatic = FALSE;
		return 0;
	}
	const BOOL bInterrupt = e->anim.active || e->anim.hAnima;
	if (bInterrupt) _XChk_StopAnim(*e, FALSE);
	_XChk_RefreshVisual(*e, TRUE, bInterrupt);
	return 0;
}

int CALLBACK CXCheckAnim::OnDestroyC(HELE hBtn, BOOL* pbHandled)
{
	(void)pbHandled;
	CXCheckAnim::Detach(hBtn);
	return 0;
}

BOOL CXCheckAnim::AttachBtn(HELE hBtn, UINT nWidth, UINT nHeight, xuitool_theme_ theme)
{
	if (!_XChk_IsBtn(hBtn)) return FALSE;

	auto& g = _XChk_G();
	auto it = g.registry.find(hBtn);
	if (it != g.registry.end()){
		_XChk_MergePending(hBtn, it->second, theme, FALSE);
		_XChk_ApplyBtnTextAlign(hBtn, it->second.textAlign);
		_XChk_RefreshVisual(it->second, FALSE);
		_XChk_ApplyButtonSize(it->second, nWidth, nHeight);
		return TRUE;
	}

	_XChk_Entry e{};
	e.hBtn = hBtn;
	const wchar_t* pText = XBtn_GetText(hBtn);
	if (pText && pText[0]){
		e.cachedTitle = pText;
		e.titleHash = _XChk_HashText(pText);
	}
	_XChk_MergePending(hBtn, e, theme, TRUE);

	XUI_EnableCSS(hBtn, FALSE);
	XBtn_SetTypeEx(hBtn, button_type_check);
	XBtn_EnableAnimation(hBtn, FALSE);
	XEle_EnableBkTransparent(hBtn, TRUE);
	_XChk_ApplyBtnTextAlign(hBtn, e.textAlign);

	_XChk_ApplyButtonSize(e, nWidth, nHeight > 0 ? nHeight : (UINT)kChk_TrackH);
	XBtn_SetText(hBtn, L"");

	const BOOL checked = XBtn_IsCheck(hBtn) == TRUE;
	e.thumbX = _XChk_TargetThumbX(checked);
	e.thumbW = (float)kChk_ThumbNormalW;
	e.trackColor = _XChk_TargetTrack(e, checked, !XEle_IsEnable(hBtn));

	_XChk_HookEvents(hBtn, e);
	g.registry[hBtn] = e;
	return TRUE;
}

BOOL CXCheckAnim::Detach(HELE hBtn)
{
	if (!_XChk_IsBtn(hBtn)) return FALSE;
	auto& g = _XChk_G();
	auto it = g.registry.find(hBtn);
	if (it == g.registry.end()) return FALSE;
	if (it->second.bCapture){
		XEle_SetCapture(hBtn, FALSE);
		it->second.bCapture = FALSE;
	}
	_XChk_StopAnim(it->second);
	_XChk_UnhookEvents(hBtn, it->second);
	if (!it->second.cachedTitle.empty())
		XBtn_SetText(hBtn, it->second.cachedTitle.c_str());
	g.registry.erase(it);
	g.pending.erase(hBtn);
	return TRUE;
}

BOOL CXCheckAnim::HasAttached(HELE hBtn)
{
	return _XChk_Find(hBtn) != NULL;
}

BOOL CXCheckAnim::SetAnimEnabled(HELE hBtn, BOOL bEnable)
{
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e) return FALSE;
	e->bAnimEnabled = bEnable;
	return TRUE;
}

BOOL CXCheckAnim::IsAnimEnabled(HELE hBtn)
{
	_XChk_Entry* e = _XChk_Find(hBtn);
	return e ? e->bAnimEnabled : FALSE;
}

BOOL CXCheckAnim::SetTextAlign(HELE hBtn, xcheckanim_text_align_ align)
{
	if (!_XChk_IsBtn(hBtn)) return FALSE;
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e){
		_XChk_StorePendingTextAlign(hBtn, align);
		return TRUE;
	}
	e->textAlign = align;
	_XChk_ApplyBtnTextAlign(hBtn, align);
	_XChk_ApplyButtonSize(*e, 0, (UINT)kChk_TrackH);
	XEle_Redraw(hBtn, FALSE);
	return TRUE;
}

xcheckanim_text_align_ CXCheckAnim::GetTextAlign(HELE hBtn)
{
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (e) return e->textAlign;
	auto it = _XChk_G().pending.find(hBtn);
	if (it != _XChk_G().pending.end() && it->second.hasTextAlign)
		return it->second.textAlign;
	return xcheckanim_text_align_left;
}

BOOL CXCheckAnim::SetTheme(HELE hBtn, xuitool_theme_ theme)
{
	if (!_XChk_IsBtn(hBtn)) return FALSE;
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (!e){
		_XChk_StorePendingTheme(hBtn, theme);
		return TRUE;
	}
	e->theme = theme;
	_XChk_RefreshVisual(*e, FALSE);
	return TRUE;
}

xuitool_theme_ CXCheckAnim::GetTheme(HELE hBtn)
{
	_XChk_Entry* e = _XChk_Find(hBtn);
	if (e) return e->theme;
	auto it = _XChk_G().pending.find(hBtn);
	if (it != _XChk_G().pending.end() && it->second.hasTheme)
		return it->second.theme;
	return xuitool_theme_auto;
}

void CXCheckAnim::Cleanup()
{
	auto& g = _XChk_G();
	std::vector<HELE> keys;
	keys.reserve(g.registry.size());
	for (auto& kv : g.registry) keys.push_back(kv.first);
	for (HELE h : keys) Detach(h);
}

#endif // _XCGUI_UITOOL_AGGREGATED_
