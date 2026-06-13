//============================================================================
// module_xcgui_uitool_accordion.cpp — CXAccordion split
// 仅由 module_xcgui_uitool.cpp #include; 勿单独编译.
//============================================================================

#ifndef _XCGUI_UITOOL_AGGREGATED_
// Aggregated in module_xcgui_uitool.cpp only; standalone TU is intentionally empty.
#else

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

struct _XAcc_ThemeColors
{
	COLORREF cardBg;
	COLORREF title;
	COLORREF body;
	COLORREF border;
	COLORREF headerHover;
	COLORREF headerActive;
	COLORREF indicator;
	COLORREF iconMuted;
	COLORREF groupTitle;
	COLORREF badgeBgSuccess;
	COLORREF badgeTextSuccess;
	COLORREF badgeBgWarning;
	COLORREF badgeTextWarning;
	COLORREF badgeBgNeutral;
	COLORREF badgeTextNeutral;
	COLORREF badgeBgInfo;
	COLORREF badgeTextInfo;
	COLORREF badgeBgDanger;
	COLORREF badgeTextDanger;
	COLORREF statusDone;
	COLORREF statusProgress;
	COLORREF statusTodo;
	COLORREF itemBorder;
	COLORREF itemFillExpanded;
	COLORREF itemFillDisabled;
	COLORREF titleDisabled;
};

struct _XAcc_AnimState
{
	BOOL   active           = FALSE;
	BOOL   expanding        = FALSE;
	HXCGUI hAnima           = NULL;
	HXCGUI hHeightAnimItem  = NULL;
};

enum _XAcc_IconMode
{
	_xacc_icon_default = 0,
	_xacc_icon_none    = 1,
	_xacc_icon_custom  = 2,
};

struct _XAcc_ItemState
{
	int                      id           = 0;
	int                      groupId      = 0;
	CXText                   title;
	CXText                   bodyText;
	xaccordion_content_type_ contentType  = xaccordion_content_none;
	HELE                     hUserEle     = NULL;
	_XAcc_IconMode           iconMode      = _xacc_icon_default;
	HIMAGE                   hIconImg      = NULL;
	CXText                   badgeText;
	xaccordion_badge_kind_ badgeKind     = xaccordion_badge_neutral;
	BOOL                     expanded      = FALSE;
	BOOL                     enabled       = TRUE;
	int                      contentMinH   = 0;
	_XAcc_AnimState          anim;

	HELE hItemWrap     = NULL;
	HELE hItemBtn      = NULL;
	HELE hAnimWrap     = NULL;
	HXCGUI hContentText = NULL;
};

struct _XAcc_GroupState
{
	int                  id        = 0;
	CXText               title;
	HELE                 hGroupWrap = NULL;
	HXCGUI               hTitle  = NULL;
	HELE                 hCard   = NULL;
	std::vector<int>     itemIds;
	BOOL                 enabled = TRUE;
};

constexpr int  kAcc_AnimDurationMs = 220;
constexpr int  kAcc_CornerRadius  = _XUITool::kCornerRadius;
constexpr int  kAcc_PadH          = 14;
constexpr int  kAcc_HeaderH       = 40;
constexpr int  kAcc_IconW         = 18;
constexpr int  kAcc_IconGap       = 10;
constexpr int  kAcc_IndicatorW    = 16;
constexpr int  kAcc_BadgeGap      = 8;
constexpr int  kAcc_BadgeH        = 20;
constexpr int  kAcc_ContentPadTop    = 8;
constexpr int  kAcc_ContentPadBottom = 14;
constexpr int  kAcc_GroupGap      = 20;
constexpr int  kAcc_GroupInnerGap = 12;
constexpr int  kAcc_ItemGap       = 8;
constexpr int  kAcc_AnimEase      = ease_flag_cubic | ease_flag_out;
constexpr int  kAcc_ItemBorderW   = 1;
constexpr int  kAcc_ContentIndent = kAcc_PadH + kAcc_IconW + kAcc_IconGap;
constexpr int  kAcc_TitleFontPt   = 10;
constexpr int  kAcc_BodyFontPt    = 9;
constexpr int  kAcc_BadgeFontPt   = 9;
constexpr int  kAcc_IndicatorFontPt = 8;
constexpr int  kAcc_GroupTitlePt  = 13;

namespace {

std::unordered_map<HXCGUI, CXAccordion*> s_xaccAnimOwners;
std::unordered_map<HXCGUI, CXAccordion*> s_xaccAnimItemOwners;
std::unordered_map<HWINDOW, std::vector<CXAccordion*>> s_xaccWndOwners;

void _xacc_RegWndOwner(HWINDOW hWnd, CXAccordion* acc)
{
	if (!hWnd || !acc) return;
	std::vector<CXAccordion*>& v = s_xaccWndOwners[hWnd];
	for (size_t i = 0; i < v.size(); ++i){
		if (v[i] == acc) return;
	}
	v.push_back(acc);
}

void _xacc_UnregWndOwner(HWINDOW hWnd, CXAccordion* acc)
{
	if (!hWnd || !acc) return;
	auto it = s_xaccWndOwners.find(hWnd);
	if (it == s_xaccWndOwners.end()) return;
	std::vector<CXAccordion*>& v = it->second;
	v.erase(std::remove(v.begin(), v.end(), acc), v.end());
	if (v.empty()) s_xaccWndOwners.erase(it);
}

inline BOOL _XAcc_IsMultiExpandMode(xaccordion_expand_mode_ mode)
{
	return mode == xaccordion_expand_mode_multiple;
}

inline unsigned _XAcc_Rgb(COLORREF c) { return (unsigned)(c | 0xFF000000u); }
inline unsigned _XAcc_Alpha(COLORREF c) { return (unsigned)((c >> 24) & 0xFF); }

void _XAcc_ApplyRoundFillBkEx(HELE hEle, COLORREF fill, int tl, int tr, int br, int bl)
{
	if (!hEle) return;
	wchar_t buf[256] = {0};
	swprintf_s(buf, 256,
		L"{99:1.9.9;98:16(0)32(1)64(2);"
		L"5:2(15)20(1)21(3)26(1)22(%u)23(%u)9(%d,%d,%d,%d);}",
		_XAcc_Rgb(fill), _XAcc_Alpha(fill), tl, tr, br, bl);
	XEle_SetBkInfo(hEle, buf);
}

void _XAcc_ApplyRoundFillBk(HELE hEle, COLORREF fill, int round)
{
	_XAcc_ApplyRoundFillBkEx(hEle, fill, round, round, round, round);
}

void _XAcc_ApplyHeaderHoverBkEx(HELE hEle, const _XAcc_ThemeColors& c, int tl, int tr, int br, int bl)
{
	if (!hEle) return;
	wchar_t buf[512] = {0};
	swprintf_s(buf, 512,
		L"{99:1.9.9;98:16(0)32(1)64(2);"
		L"5:2(15)20(1)21(3)26(1)22(%u)23(%u)9(%d,%d,%d,%d);"
		L"5:2(15)20(1)21(3)26(1)22(%u)23(%u)9(%d,%d,%d,%d);"
		L"5:2(15)20(1)21(3)26(1)22(%u)23(%u)40(1)9(%d,%d,%d,%d);}",
		_XAcc_Rgb(c.cardBg), _XAcc_Alpha(c.cardBg), tl, tr, br, bl,
		_XAcc_Rgb(c.headerHover), _XAcc_Alpha(c.headerHover), tl, tr, br, bl,
		_XAcc_Rgb(c.headerActive), _XAcc_Alpha(c.headerActive), tl, tr, br, bl);
	XEle_SetBkInfo(hEle, buf);
}

void _XAcc_ApplyHeaderHoverBk(HELE hEle, const _XAcc_ThemeColors& c, int round)
{
	_XAcc_ApplyHeaderHoverBkEx(hEle, c, round, round, round, round);
}

void _XAcc_ApplyCardBkEx(HELE hEle, const _XAcc_ThemeColors& c, int tl, int tr, int br, int bl)
{
	if (!hEle) return;
	wchar_t buf[512] = {0};
	swprintf_s(buf, 512,
		L"{99:1.9.9;98:16(0)32(1)64(2);"
		L"5:2(15)20(1)21(3)26(1)22(%u)23(%u)9(%d,%d,%d,%d);"
		L"6:2(15)20(1)21(3)26(1)22(%u)23(255)24(1)9(%d,%d,%d,%d);}",
		_XAcc_Rgb(c.cardBg), _XAcc_Alpha(c.cardBg), tl, tr, br, bl,
		_XAcc_Rgb(c.border), tl, tr, br, bl);
	XEle_SetBkInfo(hEle, buf);
}

void _XAcc_PaintItemShell(HDRAW hDraw, const RECT& rcIn, int cornerRadius,
	COLORREF borderColor, COLORREF fillColor, BOOL drawFill)
{
	RECT rcStroke;
	int w, h, r;
	BOOL hasFill, hasBorder;

	if (!hDraw) return;
	w = rcIn.right - rcIn.left;
	h = rcIn.bottom - rcIn.top;
	if (w <= 0 || h <= 0) return;

	r = cornerRadius;
	if (r * 2 > h) r = h / 2;
	if (r * 2 > w) r = w / 2;

	hasFill = drawFill && _XAcc_Alpha(fillColor) > 0 ? TRUE : FALSE;
	hasBorder = _XAcc_Alpha(borderColor) > 0 ? TRUE : FALSE;

	if (hasFill){
		XDraw_SetBrushColor(hDraw, fillColor);
		XDraw_FillRoundRectEx(hDraw, &rcIn, r, r, r, r);
	}

	if (hasBorder){
		rcStroke = rcIn;
		if (!hasFill){
			rcStroke.right  -= kAcc_ItemBorderW;
			rcStroke.bottom -= kAcc_ItemBorderW;
		}
		if (rcStroke.right > rcStroke.left && rcStroke.bottom > rcStroke.top){
			XDraw_SetBrushColor(hDraw, borderColor);
			XDraw_SetLineWidth(hDraw, kAcc_ItemBorderW);
			XDraw_DrawRoundRectEx(hDraw, &rcStroke, r, r, r, r);
		}
	}
}

void _XAcc_ClearEleBk(HELE hEle)
{
	if (hEle) XEle_SetBkInfo(hEle, L"");
}

inline COLORREF _XAcc_AlphaPct(BYTE r, BYTE g, BYTE b, int pct)
{
	return RGBA(r, g, b, (BYTE)((255 * pct + 50) / 100));
}

void _XAcc_ResolveTheme(xuitool_theme_ theme, COLORREF customText, COLORREF customBg,
	COLORREF customAccent, _XAcc_ThemeColors* c)
{
	if (!c) return;
	_XUITool::ThemePalette base;
	memset(&base, 0, sizeof(base));
	_XUITool::ResolvePalette(theme, customText, customBg, customAccent, &base);
	BOOL light = _XUITool::IsLightTheme(theme);

	c->cardBg   = base.bg;
	c->title    = base.text;
	c->body     = light ? RGBA(107, 114, 128, 255) : _XUITool::WithAlpha(base.text, 180);
	c->border   = light ? RGBA(229, 231, 235, 255) : _XUITool::WithAlpha(base.text, 40);
	c->headerHover  = light ? RGBA(249, 250, 251, 255) : RGBA(255, 255, 255, 8);
	c->headerActive = light ? RGBA(243, 244, 246, 255) : RGBA(255, 255, 255, 12);
	c->indicator    = light ? RGBA(107, 114, 128, 255) : _XUITool::WithAlpha(base.text, 200);
	c->iconMuted    = light ? RGBA(156, 163, 175, 255) : _XUITool::WithAlpha(base.text, 160);
	c->groupTitle   = base.text;
	if (light){
		c->badgeTextNeutral  = RGBA(0x60, 0x64, 0x6C, 255);
		c->badgeBgNeutral    = _XAcc_AlphaPct(0, 0, 0, 8);
		c->badgeTextSuccess  = RGBA(0x2D, 0xC2, 0x72, 255);
		c->badgeBgSuccess    = _XAcc_AlphaPct(0x38, 0xC7, 0x7A, 12);
		c->badgeTextWarning  = RGBA(0xFF, 0xA8, 0x14, 255);
		c->badgeBgWarning    = _XAcc_AlphaPct(0xFD, 0xB4, 0x38, 12);
		c->badgeTextDanger   = RGBA(0xEE, 0x3A, 0x47, 255);
		c->badgeBgDanger     = _XAcc_AlphaPct(0xFB, 0x43, 0x50, 8);
		c->badgeTextInfo     = RGBA(0x31, 0x75, 0xF6, 255);
		c->badgeBgInfo       = _XAcc_AlphaPct(0x31, 0x75, 0xF6, 8);
	}else{
		c->badgeTextNeutral  = _XAcc_AlphaPct(0xFF, 0xFF, 0xFF, 68);
		c->badgeBgNeutral    = _XAcc_AlphaPct(0xFF, 0xFF, 0xFF, 12);
		c->badgeTextSuccess  = RGBA(0x07, 0xB0, 0x56, 255);
		c->badgeBgSuccess    = _XAcc_AlphaPct(0x00, 0xB3, 0x52, 17);
		c->badgeTextWarning  = RGBA(0xB8, 0x7B, 0x00, 255);
		c->badgeBgWarning    = _XAcc_AlphaPct(0xA5, 0x74, 0x03, 17);
		c->badgeTextDanger   = _XAcc_AlphaPct(0xD2, 0x23, 0x23, 94);
		c->badgeBgDanger     = _XAcc_AlphaPct(0x99, 0x00, 0x0B, 21);
		c->badgeTextInfo     = RGBA(0x37, 0x7A, 0xF6, 255);
		c->badgeBgInfo       = _XAcc_AlphaPct(0x00, 0x46, 0xCC, 37);
	}
	c->statusDone        = RGBA(34, 197, 94, 255);
	c->statusProgress    = RGBA(249, 115, 22, 255);
	c->statusTodo        = light ? RGBA(209, 213, 219, 255) : _XUITool::WithAlpha(base.text, 80);
	c->itemBorder        = light ? RGBA(235, 235, 235, 255) : RGBA(38, 38, 38, 255);
	c->itemFillExpanded  = light ? RGBA(0, 0, 0, 20) : RGBA(255, 255, 255, 26);
	c->itemFillDisabled  = light ? RGBA(0, 0, 0, 13) : RGBA(255, 255, 255, 15);
	c->titleDisabled     = _XUITool::WithAlpha(c->title, 128);
}

void _XAcc_PassClickToParent(HELE hEle)
{
	if (hEle && XC_IsHELE((HXCGUI)hEle))
		XEle_EnableMouseThrough(hEle, TRUE);
}

// layout-only container: mouse-through, no default border/focus
void _XAcc_PrepareLayoutEle(HELE hEle)
{
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return;
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_EnableDrawFocus(hEle, FALSE);
	XEle_EnableMouseThrough(hEle, TRUE);
}

// card container: visible background, not mouse-through
void _XAcc_PrepareCardEle(HELE hEle)
{
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return;
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_EnableDrawFocus(hEle, FALSE);
}

void _XAcc_PrepareItemBtn(HELE hBtn)
{
	if (!hBtn || !XC_IsHELE((HXCGUI)hBtn)) return;
	XEle_EnableDrawBorder(hBtn, FALSE);
	XEle_EnableDrawFocus(hBtn, FALSE);
}

void _XAcc_PrepareEle(HELE hEle)
{
	_XAcc_PrepareLayoutEle(hEle);
}

void _XAcc_DestroyUi(HXCGUI h)
{
	if (!h) return;
	if (XC_IsHELE(h)) XEle_Destroy((HELE)h);
	else XShape_Destroy(h);
}

inline BOOL _XAcc_IsValidEle(HXCGUI h)
{
	return h && XC_IsHELE(h);
}

inline BOOL _XAcc_IsValidUi(HXCGUI h)
{
	return h && (XC_IsHELE(h) || XC_IsShape(h));
}

inline void _XAcc_LayoutFillWidth(HXCGUI h)
{
	if (_XAcc_IsValidUi(h))
		XWidget_LayoutItem_SetWidth(h, layout_size_fill, 0);
}

inline void _XAcc_LayoutAutoHeight(HXCGUI h)
{
	if (_XAcc_IsValidUi(h))
		XWidget_LayoutItem_SetHeight(h, layout_size_auto, 0);
}

inline BOOL _XAcc_IsValidFont(HFONTX h)
{
	return h && XC_IsHXCGUI((HXCGUI)h, XC_FONT);
}

inline BOOL _XAcc_IsValidImage(HIMAGE h)
{
	return h && (XC_IsHXCGUI((HXCGUI)h, XC_IMAGE_TEXTURE)
		|| XC_IsHXCGUI((HXCGUI)h, XC_IMAGE_FRAME));
}

inline BOOL _XAcc_IsValidSvg(HSVG h)
{
	return h && XC_IsHXCGUI((HXCGUI)h, XC_SVG);
}

inline BOOL _XAcc_IsValidAnima(HXCGUI h)
{
	return h && XC_IsHXCGUI(h, XC_ANIMATION_SEQUENCE);
}

inline void _XAcc_ReleaseFontHandle(HFONTX& h)
{
	if (_XAcc_IsValidFont(h)) XFont_Release(h);
	h = NULL;
}

inline void _XAcc_ReleaseImageHandle(HIMAGE& h)
{
	if (_XAcc_IsValidImage(h)) XImage_Release(h);
	h = NULL;
}

inline void _XAcc_ReleaseSvgHandle(HSVG& h)
{
	if (_XAcc_IsValidSvg(h)) XSvg_Release(h);
	h = NULL;
}

HIMAGE _XAcc_LoadSvgAsset(const char* svgText, int size, BOOL lightTheme, COLORREF darkTint, HSVG* outSvg)
{
	if (outSvg) *outSvg = NULL;
	if (!svgText) return NULL;
	HSVG hSvg = XSvg_LoadStringUtf8(svgText);
	if (!hSvg) return NULL;
	XSvg_SetSize(hSvg, size, size);
	if (!lightTheme) XSvg_SetUserFillColor(hSvg, darkTint, TRUE);
	HIMAGE hImg = XImage_LoadSvg(hSvg);
	if (!hImg){
		if (_XAcc_IsValidSvg(hSvg)) XSvg_Release(hSvg);
		return NULL;
	}
	if (outSvg) *outSvg = hSvg;
	else if (_XAcc_IsValidSvg(hSvg)) XSvg_Release(hSvg);
	return hImg;
}

inline void _XAcc_DrawImageEx(HDRAW hDraw, HIMAGE hImg, int x, int y, int w, int h)
{
	if (hDraw && hImg) XDraw_ImageEx(hDraw, hImg, x, y, w, h);
}

inline int _XAcc_IndicatorW(xaccordion_indicator_style_ style)
{
	(void)style;
	return kAcc_IndicatorW;
}

inline void _XAcc_MeasureText(const wchar_t* text, HFONTX hFont, SIZE* out)
{
	if (!out) return;
	out->cx = out->cy = 0;
	if (!text || !text[0] || !_XAcc_IsValidFont(hFont)) return;
	XC_GetTextShowSize(text, (int)wcslen(text), hFont, out);
}

void _XAcc_BadgeColors(const _XAcc_ThemeColors& c, xaccordion_badge_kind_ kind,
	COLORREF* bg, COLORREF* fg)
{
	if (!bg || !fg) return;
	switch (kind){
	case xaccordion_badge_success: *bg = c.badgeBgSuccess; *fg = c.badgeTextSuccess; break;
	case xaccordion_badge_warning: *bg = c.badgeBgWarning; *fg = c.badgeTextWarning; break;
	case xaccordion_badge_info:    *bg = c.badgeBgInfo;    *fg = c.badgeTextInfo;    break;
	case xaccordion_badge_danger:  *bg = c.badgeBgDanger;  *fg = c.badgeTextDanger;  break;
	default:                       *bg = c.badgeBgNeutral; *fg = c.badgeTextNeutral; break;
	}
}

int _XAcc_CalcHeaderRightPad(const _XAcc_ItemState* item, xaccordion_indicator_style_ indStyle,
	HFONTX hFontBadge, HFONTX hFontIndicator)
{
	SIZE sz;
	int pad;

	(void)hFontIndicator;
	sz.cx = sz.cy = 0;
	pad = kAcc_PadH + _XAcc_IndicatorW(indStyle);
	if (item && !item->badgeText.empty()){
		_XAcc_MeasureText(item->badgeText.getPtr(), hFontBadge, &sz);
		pad += kAcc_BadgeGap + sz.cx + kAcc_PadH * 2;
	}
	return pad;
}

} // namespace

void CALLBACK CXAccordion::AnimaCb(HXCGUI hAnima, int flag)
{
	(void)flag;
	auto it = s_xaccAnimOwners.find(hAnima);
	if (it == s_xaccAnimOwners.end() || !it->second) return;
	CXAccordion* acc = it->second;
	if (acc->m_bRootDestroyed || !acc->m_hEle) return;
	acc->OnItemAnimaEnd(hAnima, flag);
}

void CALLBACK CXAccordion::AnimaItemProgressCb(HXCGUI hAnimaItem, float pos)
{
	(void)pos;
	auto it = s_xaccAnimItemOwners.find(hAnimaItem);
	if (it == s_xaccAnimItemOwners.end() || !it->second) return;
	CXAccordion* acc = it->second;
	if (acc->m_bRootDestroyed || !acc->m_hEle) return;
	if (acc->m_hEle) XEle_AdjustLayoutEx(acc->m_hEle, adjustLayout_all);
}

//============================================================================
// CXAccordion
//============================================================================

CXAccordion::CXAccordion()
	: m_theme(xuitool_theme_auto)
	, m_customText(_XUITool::kDarkText)
	, m_customBg(_XUITool::kDarkBg)
	, m_customAccent(_XUITool::kDarkAccent)
	, m_cornerRadius(kAcc_CornerRadius)
	, m_expandMode(xaccordion_expand_mode_single)
	, m_bAnimEnabled(TRUE)
	, m_animDurationMs(kAcc_AnimDurationMs)
	, m_indicatorStyle(xaccordion_indicator_chevron)
	, m_groupTitleAlign(xaccordion_group_title_align_left)
	, m_bScrollEnabled(TRUE)
	, m_nextGroupId(1)
	, m_nextItemId(1)
	, m_hFontTitle(NULL)
	, m_hFontTitleBold(NULL)
	, m_hFontBody(NULL)
	, m_hFontBadge(NULL)
	, m_hFontIndicator(NULL)
	, m_hFontGroup(NULL)
	, m_onItemExpand(NULL)
	, m_onItemCollapse(NULL)
	, m_onItemClick(NULL)
	, m_onThemeChanged(NULL)
	, m_programmaticBtnCheck(FALSE)
	, m_bRootDestroyed(FALSE)
	, m_inAdjustLayoutEndImpl(FALSE)
	, m_inLayoutSync(FALSE)
	, m_hOwnerWnd(NULL)
	, m_hSvgIndCollapsed(NULL)
	, m_hSvgIndExpanded(NULL)
	, m_hSvgDefaultIcon(NULL)
	, m_hImgIndCollapsed(NULL)
	, m_hImgIndExpanded(NULL)
	, m_hImgDefaultIcon(NULL)
	, m_pColors(new _XAcc_ThemeColors())
{
	_XAcc_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);
}

CXAccordion::~CXAccordion()
{
	_xacc_UnbindOwnerWnd();
	if (m_bRootDestroyed){
		DetachFonts();
		m_hImgIndCollapsed = NULL;
		m_hImgIndExpanded = NULL;
		m_hImgDefaultIcon = NULL;
		m_hSvgIndCollapsed = NULL;
		m_hSvgIndExpanded = NULL;
		m_hSvgDefaultIcon = NULL;
	}else{
		_xacc_StopAllAnima(FALSE);
		if (m_hEle || !m_items.empty() || !m_groups.empty()){
			_xacc_DetachAllUserContent();
			_xacc_DestroyDetachedUserContent();
			_xacc_ClearState();
		}
		m_hEle = NULL;
		_xacc_ReleaseOwnedResources();
	}
	delete m_pColors;
	m_pColors = NULL;
}

void CXAccordion::DetachFonts()
{
	m_hFontTitle = NULL;
	m_hFontTitleBold = NULL;
	m_hFontBody = NULL;
	m_hFontBadge = NULL;
	m_hFontIndicator = NULL;
	m_hFontGroup = NULL;
}

void CXAccordion::_xacc_DetachFontRefsFromUi()
{
	for (auto& kv : m_items){
		_XAcc_ItemState* item = kv.second;
		if (!item) continue;
		if (item->hItemBtn && _XAcc_IsValidEle((HXCGUI)item->hItemBtn))
			XEle_SetFont(item->hItemBtn, NULL);
		if (item->hContentText && _XAcc_IsValidUi(item->hContentText))
			XShapeText_SetFont(item->hContentText, NULL);
	}
	for (auto& kv : m_groups){
		_XAcc_GroupState* g = kv.second;
		if (!g) continue;
		if (g->hTitle && _XAcc_IsValidUi(g->hTitle))
			XShapeText_SetFont(g->hTitle, NULL);
	}
}

void CXAccordion::_xacc_ReleaseOwnedResources()
{
	_xacc_DetachFontRefsFromUi();
	ReleaseFonts();
	_xacc_ReleaseSvgAssets();
}

void CXAccordion::_xacc_OnWindowClosing()
{
	if (!m_bRootDestroyed)
		_xacc_StopAllAnima(TRUE);
}

int CALLBACK CXAccordion::OnOwnerWndCloseImpl(HWINDOW hWnd, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	if (!hWnd) return 0;
	auto it = s_xaccWndOwners.find(hWnd);
	if (it == s_xaccWndOwners.end()) return 0;
	for (size_t i = 0; i < it->second.size(); ++i){
		CXAccordion* acc = it->second[i];
		if (!acc || acc->m_bRootDestroyed) continue;
		acc->_xacc_OnWindowClosing();
	}
	return 0;
}

void CXAccordion::_xacc_BindOwnerWnd(HWINDOW hWnd)
{
	if (!hWnd || !XC_IsHWINDOW((HXCGUI)hWnd)) return;
	const BOOL firstOnWnd = (s_xaccWndOwners.find(hWnd) == s_xaccWndOwners.end());
	m_hOwnerWnd = hWnd;
	_xacc_RegWndOwner(hWnd, this);
	// 窗口关闭前停止动画; 必须用 WM_CLOSE, 禁止误用 XE_DESTROY(33).
	if (firstOnWnd)
		XWnd_RegEventC1(hWnd, WM_CLOSE, (void*)&CXAccordion::OnOwnerWndCloseImpl);
}

void CXAccordion::_xacc_UnbindOwnerWnd()
{
	if (!m_hOwnerWnd) return;
	HWINDOW hWnd = m_hOwnerWnd;
	m_hOwnerWnd = NULL;
	_xacc_UnregWndOwner(hWnd, this);
	if (s_xaccWndOwners.find(hWnd) != s_xaccWndOwners.end()) return;
	if (XC_IsHWINDOW((HXCGUI)hWnd))
		XWnd_RemoveEventC(hWnd, WM_CLOSE, (void*)&CXAccordion::OnOwnerWndCloseImpl);
}

void CXAccordion::_xacc_DestroyDetachedUserContent()
{
	for (auto& kv : m_items){
		_XAcc_ItemState* item = kv.second;
		if (!item || !item->hUserEle) continue;
		if (_XAcc_IsValidEle((HXCGUI)item->hUserEle))
			XEle_Destroy(item->hUserEle);
		item->hUserEle = NULL;
	}
}

void CXAccordion::_xacc_DetachAllUserContent()
{
	for (auto& kv : m_items){
		_XAcc_ItemState* item = kv.second;
		if (!item) continue;
		if (item->hUserEle && _XAcc_IsValidEle((HXCGUI)item->hUserEle)){
			XEle_Remove(item->hUserEle);
			item->hUserEle = NULL;
		}
	}
}

void CXAccordion::_xacc_ReleaseSvgAssets()
{
	_XAcc_ReleaseImageHandle(m_hImgIndCollapsed);
	_XAcc_ReleaseImageHandle(m_hImgIndExpanded);
	_XAcc_ReleaseImageHandle(m_hImgDefaultIcon);
	_XAcc_ReleaseSvgHandle(m_hSvgIndCollapsed);
	_XAcc_ReleaseSvgHandle(m_hSvgIndExpanded);
	_XAcc_ReleaseSvgHandle(m_hSvgDefaultIcon);
}

void CXAccordion::_xacc_LoadSvgAssets()
{
	_xacc_ReleaseSvgAssets();
	const BOOL light = _XUITool::IsLightTheme(m_theme);
	const COLORREF darkIndCollapsed = RGBA(0xD4, 0xD4, 0xD4, 0xFF);
	const COLORREF darkIndExpanded  = RGBA(0x7B, 0x7B, 0x7B, 0xFF);
	const COLORREF darkItemIcon     = RGBA(0xD4, 0xD4, 0xD4, 0xFF);
	m_hImgIndCollapsed = _XAcc_LoadSvgAsset(kAccSvg_IndCollapsed, kAcc_IndicatorW, light,
		darkIndCollapsed, &m_hSvgIndCollapsed);
	m_hImgIndExpanded = _XAcc_LoadSvgAsset(kAccSvg_IndExpanded, kAcc_IndicatorW, light,
		darkIndExpanded, &m_hSvgIndExpanded);
	m_hImgDefaultIcon = _XAcc_LoadSvgAsset(kAccSvg_ItemIcon, kAcc_IconW, light,
		darkItemIcon, &m_hSvgDefaultIcon);
}

void CXAccordion::_xacc_ApplyItemIcon(_XAcc_ItemState* item, HIMAGE hIcon)
{
	if (!item) return;
	if (!hIcon){
		item->iconMode = _xacc_icon_none;
		item->hIconImg = NULL;
	}else{
		item->iconMode = _xacc_icon_custom;
		item->hIconImg = hIcon;
	}
}

void CXAccordion::_xacc_ApplyDefaultItemIcon(_XAcc_ItemState* item)
{
	if (!item) return;
	item->iconMode = _xacc_icon_default;
	item->hIconImg = NULL;
}

HIMAGE CXAccordion::_xacc_ResolveItemIcon(const _XAcc_ItemState* item) const
{
	if (!item) return NULL;
	switch (item->iconMode){
	case _xacc_icon_none: return NULL;
	case _xacc_icon_custom: return item->hIconImg;
	default: return m_hImgDefaultIcon;
	}
}

BOOL CXAccordion::_xacc_ItemHasIcon(const _XAcc_ItemState* item) const
{
	return _xacc_ResolveItemIcon(item) != NULL;
}

void CXAccordion::_xacc_ClearState()
{
	for (auto it = s_xaccAnimOwners.begin(); it != s_xaccAnimOwners.end(); ){
		if (it->second == this) it = s_xaccAnimOwners.erase(it);
		else ++it;
	}
	for (auto it = s_xaccAnimItemOwners.begin(); it != s_xaccAnimItemOwners.end(); ){
		if (it->second == this) it = s_xaccAnimItemOwners.erase(it);
		else ++it;
	}
	for (auto& kv : m_items){
		delete kv.second;
	}
	m_items.clear();
	for (auto& kv : m_groups){
		delete kv.second;
	}
	m_groups.clear();
}

void CXAccordion::_xacc_OnRootDestroyed()
{
	if (m_bRootDestroyed) return;
	m_bRootDestroyed = TRUE;
	_xacc_UnbindOwnerWnd();
	_xacc_StopAllAnima(FALSE);
	_xacc_ClearState();
	m_hEle = NULL;
	_xacc_ReleaseOwnedResources();
}

void CXAccordion::ReleaseFonts()
{
	_XAcc_ReleaseFontHandle(m_hFontTitle);
	_XAcc_ReleaseFontHandle(m_hFontTitleBold);
	_XAcc_ReleaseFontHandle(m_hFontBody);
	_XAcc_ReleaseFontHandle(m_hFontBadge);
	_XAcc_ReleaseFontHandle(m_hFontIndicator);
	_XAcc_ReleaseFontHandle(m_hFontGroup);
}

void CXAccordion::EnsureFonts()
{
	auto createFont = [](int pt, int style) -> HFONTX {
		return XFont_CreateEx(L"Segoe UI", pt, style);
	};
	if (!_XAcc_IsValidFont(m_hFontTitle))      m_hFontTitle      = createFont(kAcc_TitleFontPt, fontStyle_regular);
	if (!_XAcc_IsValidFont(m_hFontTitleBold))  m_hFontTitleBold  = createFont(kAcc_TitleFontPt, fontStyle_bold);
	if (!_XAcc_IsValidFont(m_hFontBody))       m_hFontBody       = createFont(kAcc_BodyFontPt, fontStyle_regular);
	if (!_XAcc_IsValidFont(m_hFontBadge))      m_hFontBadge      = createFont(kAcc_BadgeFontPt, fontStyle_regular);
	if (!_XAcc_IsValidFont(m_hFontIndicator))  m_hFontIndicator  = createFont(kAcc_IndicatorFontPt, fontStyle_regular);
	if (!_XAcc_IsValidFont(m_hFontGroup))      m_hFontGroup      = createFont(kAcc_GroupTitlePt, fontStyle_bold);
}

HELE CXAccordion::Create(HXCGUI hParent)
{
	m_bRootDestroyed = FALSE;
	m_hEle = XLayoutFrame_CreateEx(hParent);
	if (!_XAcc_IsValidEle((HXCGUI)m_hEle)) {
		m_hEle = NULL;
		return NULL;
	}

	EnsureFonts();
	_xacc_LoadSvgAssets();
	_XAcc_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);

	EnableCanvas(TRUE);
	EnableDrawBorder(FALSE);
	EnableDrawFocus(FALSE);
	XEle_EnableBkTransparent(m_hEle, TRUE);

	XLayoutFrame_EnableLayout(m_hEle, TRUE);
	XLayoutBox_SetAlignV(m_hEle, layout_align_top);
	XLayoutBox_SetSpace(m_hEle, 0);
	XEle_SetPadding(m_hEle, kAcc_PadH, kAcc_PadH, kAcc_PadH, kAcc_PadH);
	XWidget_LayoutItem_SetWidth(m_hEle, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(m_hEle, layout_size_fill, 0);
	_xacc_ApplyScroll();

	InstallEvents();
	if (XC_IsHWINDOW(hParent))
		_xacc_BindOwnerWnd((HWINDOW)hParent);
	return m_hEle;
}

void CXAccordion::_xacc_ApplyScroll()
{
	if (!_XAcc_IsValidEle((HXCGUI)m_hEle)) return;
	XSView_EnableAutoShowScrollBar(m_hEle, m_bScrollEnabled ? TRUE : FALSE);
	XSView_ShowSBarH(m_hEle, FALSE);
	XSView_ShowSBarV(m_hEle, FALSE);
}

void CXAccordion::AdjustLayout()
{
	if (m_inLayoutSync || !m_hEle) return;
	m_inLayoutSync = TRUE;
	for (auto& kv : m_items){
		if (kv.second) _xacc_NormalizeItemLayout(kv.second);
	}
	for (auto& gkv : m_groups){
		_XAcc_GroupState* g = gkv.second;
		if (g && g->hCard && _XAcc_IsValidEle((HXCGUI)g->hCard))
			XEle_AdjustLayoutEx(g->hCard, adjustLayout_all);
	}
	XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
	_xacc_UpdateScrollTotalSize();
	m_inLayoutSync = FALSE;
}

void CXAccordion::_xacc_NormalizeItemLayout(_XAcc_ItemState* item)
{
	if (!item) return;
	if (item->expanded || item->anim.active){
		if (!item->anim.active || item->anim.expanding)
			_xacc_ApplyExpandedLayout(item);
		return;
	}
	// 与 ExpandItem 相同: 先测量内容自然高度, 再收回折叠态, 避免 auto 子项撑乱布局.
	if (item->contentType != xaccordion_content_none)
		(void)MeasureItemContentHeight(item);
	_xacc_ApplyCollapsedLayout(item);
}

void CXAccordion::_xacc_ApplyCollapsedLayout(_XAcc_ItemState* item)
{
	if (!item || !item->hItemWrap) return;
	if (item->hAnimWrap){
		if (item->hContentText && _XAcc_IsValidUi(item->hContentText))
			XWidget_Show(item->hContentText, FALSE);
		if (item->hUserEle && _XAcc_IsValidEle((HXCGUI)item->hUserEle))
			XWidget_Show(item->hUserEle, FALSE);
		XWidget_Show(item->hAnimWrap, FALSE);
		XWidget_LayoutItem_SetHeight(item->hAnimWrap, layout_size_fixed, 0);
	}
	XWidget_LayoutItem_SetHeight(item->hItemWrap, layout_size_fixed, kAcc_HeaderH);
}

void CXAccordion::_xacc_ApplyExpandedLayout(_XAcc_ItemState* item)
{
	if (!item || !item->hItemWrap || !item->hAnimWrap) return;
	XWidget_LayoutItem_SetHeight(item->hItemWrap, layout_size_auto, 0);
	XWidget_Show(item->hAnimWrap, TRUE);
	XWidget_LayoutItem_SetHeight(item->hAnimWrap, layout_size_auto, 0);
	if (item->hContentText && _XAcc_IsValidUi(item->hContentText))
		XWidget_Show(item->hContentText, TRUE);
	if (item->hUserEle && _XAcc_IsValidEle((HXCGUI)item->hUserEle))
		XWidget_Show(item->hUserEle, TRUE);
}

void CXAccordion::_xacc_UpdateScrollTotalSize()
{
	if (!_XAcc_IsValidEle((HXCGUI)m_hEle) || !m_bScrollEnabled) return;

	int w = XLayoutFrame_GetWidthIn(m_hEle);
	if (w <= 0) w = 1;

	int contentH = kAcc_PadH * 2;
	for (auto& gkv : m_groups){
		_XAcc_GroupState* g = gkv.second;
		if (!g || !g->hGroupWrap || !_XAcc_IsValidEle((HXCGUI)g->hGroupWrap)) continue;
		if (contentH > 0) contentH += kAcc_GroupGap;
		contentH += XEle_GetHeight(g->hGroupWrap);
	}
	if (contentH <= 0) return;

	SIZE total = {0};
	XSView_GetTotalSize(m_hEle, &total);
	if (total.cx != w || total.cy != contentH)
		XSView_SetTotalSize(m_hEle, w, contentH);
}

void CXAccordion::_xacc_RefreshScrollExtent()
{
	if (!m_hEle) return;
	XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
	_xacc_UpdateScrollTotalSize();
}

void CXAccordion::InstallEvents()
{
	if (!_XAcc_IsValidEle((HXCGUI)m_hEle)) return;
	XEle_RegEventCPP1(m_hEle, XE_DESTROY, &CXAccordion::OnDestroyImpl);
	XEle_RegEventCPP1(m_hEle, XE_DESTROY_END, &CXAccordion::OnDestroyEndImpl);
	XEle_RegEventCPP1(m_hEle, XE_SHOW, &CXAccordion::OnShowImpl);
	XEle_RegEventCPP1(m_hEle, XE_SIZE, &CXAccordion::OnSizeImpl);
	XEle_RegEventCPP1(m_hEle, XE_ADJUSTLAYOUT_END, &CXAccordion::OnAdjustLayoutEndImpl);
}

BOOL CXAccordion::IsValid() const
{
	return _XAcc_IsValidEle((HXCGUI)m_hEle);
}

HELE CXAccordion::GetHandle() const
{
	return m_hEle;
}

void CXAccordion::SetTheme(xuitool_theme_ theme)
{
	m_theme = theme;
	RefreshTheme();
}

xuitool_theme_ CXAccordion::GetTheme() const
{
	return m_theme;
}

void CXAccordion::SetTextColor(COLORREF c)
{
	m_customText = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXAccordion::SetBkColor(COLORREF c)
{
	m_customBg = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXAccordion::SetAccentColor(COLORREF c)
{
	m_customAccent = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXAccordion::SetCornerRadius(int r)
{
	m_cornerRadius = r > 0 ? r : kAcc_CornerRadius;
	RefreshTheme();
}

void CXAccordion::SetExpandMode(xaccordion_expand_mode_ mode)
{
	m_expandMode = mode;
	ApplyAllItemsBtnSelectMode();
}

xaccordion_expand_mode_ CXAccordion::GetExpandMode() const
{
	return m_expandMode;
}

void CXAccordion::SetAllowMultipleExpand(BOOL bAllow)
{
	SetExpandMode(bAllow ? xaccordion_expand_mode_multiple : xaccordion_expand_mode_single);
}

BOOL CXAccordion::IsAllowMultipleExpand() const
{
	return _XAcc_IsMultiExpandMode(m_expandMode);
}

void CXAccordion::SetAnimEnabled(BOOL bEnable)
{
	m_bAnimEnabled = bEnable;
}

void CXAccordion::SetAnimDuration(int ms)
{
	m_animDurationMs = ms > 0 ? ms : kAcc_AnimDurationMs;
}

void CXAccordion::SetIndicatorStyle(xaccordion_indicator_style_ style)
{
	m_indicatorStyle = style;
	for (auto& kv : m_items){
		if (kv.second) UpdateItemVisual(kv.second);
	}
}

void CXAccordion::EnableScroll(BOOL bEnable)
{
	m_bScrollEnabled = bEnable;
	_xacc_ApplyScroll();
}

void CXAccordion::SetOnItemExpand(xaccordion_item_event fn)   { m_onItemExpand = fn; }
void CXAccordion::SetOnItemCollapse(xaccordion_item_event fn){ m_onItemCollapse = fn; }
void CXAccordion::SetOnItemClick(xaccordion_item_event fn)  { m_onItemClick = fn; }
void CXAccordion::SetOnThemeChanged(xaccordion_void_event fn){ m_onThemeChanged = fn; }

_XAcc_GroupState* CXAccordion::FindGroup(int groupId)
{
	auto it = m_groups.find(groupId);
	return it != m_groups.end() ? it->second : NULL;
}

_XAcc_ItemState* CXAccordion::FindItem(int itemId)
{
	auto it = m_items.find(itemId);
	return it != m_items.end() ? it->second : NULL;
}

BOOL CXAccordion::IsItemOperable(_XAcc_ItemState* item) const
{
	if (!item || !item->enabled) return FALSE;
	auto it = m_groups.find(item->groupId);
	if (it == m_groups.end() || !it->second) return FALSE;
	return it->second->enabled;
}

void CXAccordion::UpdateGroupVisual(_XAcc_GroupState* g)
{
	if (!g) return;
	if (g->hGroupWrap && _XAcc_IsValidEle((HXCGUI)g->hGroupWrap))
		XEle_SetAlpha(g->hGroupWrap, 255);
	if (g->hCard && _XAcc_IsValidEle((HXCGUI)g->hCard)){
		XEle_Enable(g->hCard, g->enabled);
		XEle_SetAlpha(g->hCard, 255);
	}
	if (g->hTitle && _XAcc_IsValidUi(g->hTitle)){
		COLORREF c = g->enabled ? m_pColors->groupTitle : m_pColors->titleDisabled;
		XShapeText_SetTextColor(g->hTitle, c);
	}
	for (size_t i = 0; i < g->itemIds.size(); ++i){
		_XAcc_ItemState* item = FindItem(g->itemIds[i]);
		if (item) UpdateItemVisual(item);
	}
	UpdateGroupItemsShell(g);
}

void CXAccordion::_xacc_ApplyGroupCardMargin(_XAcc_GroupState* g)
{
	if (!g || !g->hCard) return;
	if (g->hTitle && _XAcc_IsValidUi(g->hTitle) && !g->title.empty())
		XWidget_LayoutItem_SetMargin(g->hCard, 0, kAcc_GroupInnerGap, 0, 0);
	else
		XWidget_LayoutItem_SetMargin(g->hCard, 0, 0, 0, 0);
}

void CXAccordion::_xacc_ApplyGroupItemGaps(_XAcc_GroupState* g)
{
	if (!g) return;
	const size_t n = g->itemIds.size();
	for (size_t i = 0; i < n; ++i){
		_XAcc_ItemState* item = FindItem(g->itemIds[i]);
		if (!item || !item->hItemWrap) continue;
		const int mb = (i + 1 < n) ? kAcc_ItemGap : 0;
		XWidget_LayoutItem_SetMargin(item->hItemWrap, 0, 0, 0, mb);
	}
}

int CXAccordion::_xacc_GroupTitleAlignFlags() const
{
	switch (m_groupTitleAlign){
	case xaccordion_group_title_align_center:
		return textAlignFlag_center | textAlignFlag_vcenter;
	case xaccordion_group_title_align_right:
		return textAlignFlag_right | textAlignFlag_vcenter;
	default:
		return textAlignFlag_left | textAlignFlag_vcenter;
	}
}

void CXAccordion::_xacc_ApplyGroupTitleAlign(_XAcc_GroupState* g)
{
	if (!g || !g->hTitle || !_XAcc_IsValidUi(g->hTitle)) return;
	XShapeText_SetTextAlign(g->hTitle, _xacc_GroupTitleAlignFlags());
}

void CXAccordion::_xacc_ApplyAllGroupTitleAlign()
{
	for (auto& gkv : m_groups)
		_xacc_ApplyGroupTitleAlign(gkv.second);
}

void CXAccordion::SetGroupTitleAlign(xaccordion_group_title_align_ align)
{
	if (m_groupTitleAlign == align) return;
	m_groupTitleAlign = align;
	_xacc_ApplyAllGroupTitleAlign();
	if (m_hEle) XEle_Redraw(m_hEle, FALSE);
}

xaccordion_group_title_align_ CXAccordion::GetGroupTitleAlign() const
{
	return m_groupTitleAlign;
}

int CXAccordion::AddGroup(const wchar_t* pTitle)
{
	if (!m_hEle) return 0;
	EnsureFonts();

	_XAcc_GroupState* g = new _XAcc_GroupState();
	g->id = m_nextGroupId++;
	if (pTitle && pTitle[0]) g->title = pTitle;

	g->hGroupWrap = XLayout_Create(0, 0, 100, 10, m_hEle);
	if (!g->hGroupWrap){
		delete g;
		return 0;
	}
	_XAcc_PrepareLayoutEle(g->hGroupWrap);
	XLayout_EnableLayout(g->hGroupWrap, TRUE);
	XLayoutBox_SetAlignV(g->hGroupWrap, layout_align_top);
	XLayoutBox_SetSpace(g->hGroupWrap, 0);
	XWidget_LayoutItem_SetWidth(g->hGroupWrap, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(g->hGroupWrap, layout_size_auto, 0);
	if (!m_groups.empty())
		XWidget_LayoutItem_SetMargin(g->hGroupWrap, 0, kAcc_GroupGap, 0, 0);

	if (!g->title.empty()){
		HXCGUI hTitle = XShapeText_Create(0, 0, 100, 28, g->title.getPtr(), g->hGroupWrap);
		g->hTitle = hTitle;
		if (g->hTitle){
			XShapeText_SetTextColor(hTitle, m_pColors->groupTitle);
			XShapeText_SetFont(hTitle, m_hFontGroup);
			_xacc_ApplyGroupTitleAlign(g);
			_XAcc_LayoutFillWidth(g->hTitle);
			_XAcc_LayoutAutoHeight(g->hTitle);
		}
	}

	g->hCard = XLayout_Create(0, 0, 100, 100, g->hGroupWrap);
	if (!g->hCard){
		if (g->hGroupWrap) XEle_Destroy(g->hGroupWrap);
		delete g;
		return 0;
	}
	_XAcc_PrepareLayoutEle(g->hCard);
	XLayout_EnableLayout(g->hCard, TRUE);
	XEle_EnableBkTransparent(g->hCard, TRUE);
	_XAcc_ClearEleBk(g->hCard);
	XLayoutBox_SetAlignV(g->hCard, layout_align_top);
	XLayoutBox_SetSpace(g->hCard, 0);
	XWidget_LayoutItem_SetWidth(g->hCard, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(g->hCard, layout_size_auto, 0);
	_xacc_ApplyGroupCardMargin(g);

	m_groups[g->id] = g;
	UpdateGroupVisual(g);
	return g->id;
}

BOOL CXAccordion::SetGroupEnabled(int groupId, BOOL bEnabled)
{
	_XAcc_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	if (g->enabled == (bEnabled ? TRUE : FALSE)) return TRUE;
	g->enabled = bEnabled ? TRUE : FALSE;
	if (!g->enabled)
		CollapseAll(groupId);
	UpdateGroupVisual(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

BOOL CXAccordion::IsGroupEnabled(int groupId) const
{
	auto it = m_groups.find(groupId);
	if (it == m_groups.end() || !it->second) return FALSE;
	return it->second->enabled;
}

BOOL CXAccordion::SetGroupTitle(int groupId, const wchar_t* pTitle)
{
	_XAcc_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	g->title = pTitle ? pTitle : L"";
	if (g->hTitle && _XAcc_IsValidUi(g->hTitle)){
		XShapeText_SetText(g->hTitle, g->title.getPtr());
		XWidget_Show(g->hTitle, g->title.empty() ? FALSE : TRUE);
	}
	_xacc_ApplyGroupCardMargin(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

BOOL CXAccordion::RemoveGroup(int groupId)
{
	_XAcc_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	std::vector<int> ids = g->itemIds;
	for (size_t i = 0; i < ids.size(); ++i) RemoveItem(ids[i]);
	if (g->hGroupWrap && _XAcc_IsValidEle((HXCGUI)g->hGroupWrap))
		XEle_Destroy(g->hGroupWrap);
	m_groups.erase(groupId);
	delete g;
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

void CXAccordion::ClearGroups()
{
	std::vector<int> gids;
	gids.reserve(m_groups.size());
	for (auto& kv : m_groups) gids.push_back(kv.first);
	for (size_t i = 0; i < gids.size(); ++i) RemoveGroup(gids[i]);
}

int CXAccordion::GetGroupCount() const
{
	return (int)m_groups.size();
}

HELE CXAccordion::CreateItemLayout(_XAcc_ItemState* item, HELE hCard)
{
	if (!item || !hCard) return NULL;

	item->hItemWrap = XLayout_Create(0, 0, 100, kAcc_HeaderH, hCard);
	if (!item->hItemWrap) return NULL;
	XLayout_EnableLayout(item->hItemWrap, TRUE);
	XLayoutBox_SetAlignV(item->hItemWrap, layout_align_top);
	XLayoutBox_SetSpace(item->hItemWrap, 0);
	XEle_EnableDrawBorder(item->hItemWrap, FALSE);
	XEle_EnableDrawFocus(item->hItemWrap, FALSE);
	XEle_EnableBkTransparent(item->hItemWrap, TRUE);
	XEle_SetUserData(item->hItemWrap, (vint)item->id);
	XEle_RegEventCPP1(item->hItemWrap, XE_PAINT, &CXAccordion::OnItemWrapPaintImpl);
	XWidget_LayoutItem_SetWidth(item->hItemWrap, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(item->hItemWrap, layout_size_auto, 0);

	item->hItemBtn = XBtn_Create(0, 0, 100, kAcc_HeaderH, item->title.getPtr(), item->hItemWrap);
	if (!item->hItemBtn) return NULL;
	_XAcc_PrepareItemBtn(item->hItemBtn);
	XEle_EnableBkTransparent(item->hItemBtn, TRUE);
	_XAcc_ClearEleBk(item->hItemBtn);
	XEle_SetFont(item->hItemBtn, m_hFontTitleBold);
	XBtn_SetTextAlign(item->hItemBtn, textAlignFlag_left | textAlignFlag_vcenter);
	XEle_SetTextColor(item->hItemBtn, m_pColors->title);
	XWidget_LayoutItem_SetWidth(item->hItemBtn, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(item->hItemBtn, layout_size_fixed, kAcc_HeaderH);
	XEle_SetUserData(item->hItemBtn, (vint)item->id);
	XEle_RegEventCPP1(item->hItemBtn, XE_BUTTON_CHECK, &CXAccordion::OnHeaderCheckImpl);
	XEle_EnableEvent_XE_PAINT_END(item->hItemBtn, TRUE);
	XEle_RegEventCPP1(item->hItemBtn, XE_PAINT_END, &CXAccordion::OnHeaderPaintImpl);

	static HCURSOR sHand = NULL;
	if (!sHand) sHand = LoadCursorW(NULL, MAKEINTRESOURCEW(32649));
	if (sHand) XEle_SetCursor(item->hItemBtn, sHand);

	item->hAnimWrap = XLayout_Create(0, 0, 100, 10, item->hItemWrap);
	if (!item->hAnimWrap) return item->hItemBtn;
	_XAcc_PrepareLayoutEle(item->hAnimWrap);
	XEle_EnableBkTransparent(item->hAnimWrap, TRUE);
	_XAcc_ClearEleBk(item->hAnimWrap);
	XLayout_EnableLayout(item->hAnimWrap, TRUE);
	XLayoutBox_SetAlignV(item->hAnimWrap, layout_align_top);
	_xacc_ApplyItemContentPadding(item);
	XWidget_LayoutItem_SetWidth(item->hAnimWrap, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(item->hAnimWrap, layout_size_fixed, 0);
	XWidget_Show(item->hAnimWrap, FALSE);

	ApplyItemBtnSelectMode(item);
	UpdateItemShell(item);
	_xacc_ApplyCollapsedLayout(item);
	return item->hItemBtn;
}

void CXAccordion::ApplyItemBtnSelectMode(_XAcc_ItemState* item)
{
	if (!item || !item->hItemBtn) return;
	XBtn_SetTypeEx(item->hItemBtn, button_type_check);
	XUI_SetStyle(item->hItemBtn, object_style_default);
	XBtn_SetGroupID(item->hItemBtn, 0);
	SyncItemBtnCheck(item);
}

void CXAccordion::ApplyAllItemsBtnSelectMode()
{
	for (auto& kv : m_items){
		if (kv.second) ApplyItemBtnSelectMode(kv.second);
	}
}

void CXAccordion::SyncItemBtnCheck(_XAcc_ItemState* item)
{
	if (!item || !item->hItemBtn) return;
	BOOL wantCheck = item->expanded;
	if (item->anim.active)
		wantCheck = item->anim.expanding;
	if (XBtn_IsCheck(item->hItemBtn) == wantCheck) return;
	m_programmaticBtnCheck = TRUE;
	XBtn_SetCheck(item->hItemBtn, wantCheck);
	m_programmaticBtnCheck = FALSE;
}

int CXAccordion::AddItem(int groupId, const wchar_t* pTitle, xaccordion_content_type_ type)
{
	_XAcc_GroupState* g = FindGroup(groupId);
	if (!g || !g->hCard) return 0;

	_XAcc_ItemState* item = new _XAcc_ItemState();
	item->id = m_nextItemId++;
	item->groupId = groupId;
	item->title = pTitle ? pTitle : L"";
	item->contentType = type;
	_xacc_ApplyDefaultItemIcon(item);

	if (!CreateItemLayout(item, g->hCard)){
		delete item;
		return 0;
	}

	g->itemIds.push_back(item->id);
	m_items[item->id] = item;
	UpdateItemVisual(item);
	UpdateGroupItemsShell(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return item->id;
}

int CXAccordion::AddItem(int groupId, const wchar_t* pTitle, xaccordion_content_type_ type, HIMAGE hIcon)
{
	_XAcc_GroupState* g = FindGroup(groupId);
	if (!g || !g->hCard) return 0;

	_XAcc_ItemState* item = new _XAcc_ItemState();
	item->id = m_nextItemId++;
	item->groupId = groupId;
	item->title = pTitle ? pTitle : L"";
	item->contentType = type;
	_xacc_ApplyItemIcon(item, hIcon);

	if (!CreateItemLayout(item, g->hCard)){
		delete item;
		return 0;
	}

	g->itemIds.push_back(item->id);
	m_items[item->id] = item;
	UpdateItemVisual(item);
	UpdateGroupItemsShell(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return item->id;
}

void CXAccordion::_xacc_ApplyItemContentPadding(_XAcc_ItemState* item)
{
	if (!item || !item->hAnimWrap) return;
	const int leftPad = _xacc_ItemHasIcon(item) ? kAcc_ContentIndent : kAcc_PadH;
	const int rightPad = _XAcc_CalcHeaderRightPad(item, m_indicatorStyle, m_hFontBadge, m_hFontIndicator);
	XEle_SetPadding(item->hAnimWrap, leftPad, kAcc_ContentPadTop, rightPad, kAcc_ContentPadBottom);
}

void CXAccordion::UpdateItemShell(_XAcc_ItemState* item)
{
	if (!item || !item->hItemWrap) return;
	_XAcc_ClearEleBk(item->hItemWrap);
	if (item->hItemBtn) _XAcc_ClearEleBk(item->hItemBtn);
	if (item->hAnimWrap) _XAcc_ClearEleBk(item->hAnimWrap);
	XEle_Redraw(item->hItemWrap, FALSE);
}

void CXAccordion::UpdateGroupItemsShell(_XAcc_GroupState* g)
{
	if (!g) return;
	_xacc_ApplyGroupItemGaps(g);
	for (size_t i = 0; i < g->itemIds.size(); ++i){
		_XAcc_ItemState* item = FindItem(g->itemIds[i]);
		if (item) UpdateItemShell(item);
	}
}

void CXAccordion::UpdateItemBadge(_XAcc_ItemState* item)
{
	if (!item || !item->hItemBtn) return;
	UpdateItemVisual(item);
}

void CXAccordion::UpdateItemVisual(_XAcc_ItemState* item)
{
	if (!item || !item->hItemBtn) return;

	XBtn_SetText(item->hItemBtn, item->title.getPtr());
	XEle_SetFont(item->hItemBtn, m_hFontTitleBold);
	const BOOL operable = IsItemOperable(item);
	XEle_SetTextColor(item->hItemBtn, operable ? m_pColors->title : m_pColors->titleDisabled);

	const int leftPad = _xacc_ItemHasIcon(item) ? kAcc_ContentIndent : kAcc_PadH;
	const int rightPad = _XAcc_CalcHeaderRightPad(item, m_indicatorStyle, m_hFontBadge, m_hFontIndicator);
	XEle_SetPadding(item->hItemBtn, leftPad, 0, rightPad, 0);
	_xacc_ApplyItemContentPadding(item);

	if (item->hItemBtn){
		XEle_Enable(item->hItemBtn, operable);
	}

	SyncItemBtnCheck(item);
	UpdateItemShell(item);
	if (item->expanded && !item->anim.active)
		RemeasureItem(item);
	XEle_Redraw(item->hItemBtn, FALSE);
}

BOOL CXAccordion::SetItemTitle(int itemId, const wchar_t* pTitle)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	item->title = pTitle ? pTitle : L"";
	UpdateItemVisual(item);
	return TRUE;
}

BOOL CXAccordion::ClearItemContentEle(_XAcc_ItemState* item)
{
	if (!item) return FALSE;
	if (item->hContentText){
		_XAcc_DestroyUi(item->hContentText);
		item->hContentText = NULL;
	}
	if (item->hUserEle && _XAcc_IsValidEle((HXCGUI)item->hUserEle)){
		XEle_Remove(item->hUserEle);
		item->hUserEle = NULL;
	}
	item->contentType = xaccordion_content_none;
	return TRUE;
}

BOOL CXAccordion::SetItemBodyText(int itemId, const wchar_t* pText)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item || !item->hAnimWrap) return FALSE;
	ClearItemContentEle(item);
	item->bodyText = pText ? pText : L"";
	item->contentType = xaccordion_content_text;

	HXCGUI hText = XShapeText_Create(0, 0, 100, 40, item->bodyText.getPtr(), item->hAnimWrap);
	item->hContentText = hText;
	if (!item->hContentText) return FALSE;
	XShapeText_SetFont(hText, m_hFontBody);
	XShapeText_SetTextColor(hText, m_pColors->body);
	XShapeText_SetTextAlign(hText, textAlignFlag_left | textAlignFlag_top);
	_XAcc_LayoutFillWidth(item->hContentText);
	_XAcc_LayoutAutoHeight(item->hContentText);
	if (item->expanded) RemeasureItem(item);
	else _xacc_NormalizeItemLayout(item);
	return TRUE;
}

BOOL CXAccordion::SetItemContentEle(int itemId, HELE hEle)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item || !item->hAnimWrap || !hEle) return FALSE;
	ClearItemContentEle(item);
	item->hUserEle = hEle;
	item->contentType = xaccordion_content_element;

	XEle_Remove(hEle);
	XEle_AddChild(item->hAnimWrap, hEle);

	if (item->expanded) RemeasureItem(item);
	else _xacc_NormalizeItemLayout(item);
	return TRUE;
}

BOOL CXAccordion::ClearItemContent(int itemId)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	return ClearItemContentEle(item);
}

BOOL CXAccordion::SetItemContentMinHeight(int itemId, int h)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	item->contentMinH = h > 0 ? h : 0;
	if (item->expanded) RemeasureItem(item);
	return TRUE;
}

BOOL CXAccordion::SetItemIcon(int itemId, xaccordion_icon_type_ type, HSVG hSvg)
{
	(void)hSvg;
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	if (type == xaccordion_icon_none)
		_xacc_ApplyItemIcon(item, NULL);
	else
		_xacc_ApplyDefaultItemIcon(item);
	UpdateItemVisual(item);
	return TRUE;
}

BOOL CXAccordion::SetItemIconImage(int itemId, HIMAGE hIcon)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	_xacc_ApplyItemIcon(item, hIcon);
	UpdateItemVisual(item);
	return TRUE;
}

BOOL CXAccordion::SetItemBadge(int itemId, const wchar_t* pText, xaccordion_badge_kind_ kind)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	item->badgeText = pText ? pText : L"";
	item->badgeKind = kind;
	UpdateItemVisual(item);
	return TRUE;
}

BOOL CXAccordion::SetItemEnabled(int itemId, BOOL bEnabled)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	item->enabled = bEnabled ? TRUE : FALSE;
	if (!item->enabled && (item->expanded || item->anim.active))
		CollapseItem(itemId, FALSE);
	UpdateItemVisual(item);
	return TRUE;
}

BOOL CXAccordion::IsItemEnabled(int itemId) const
{
	auto it = m_items.find(itemId);
	if (it == m_items.end() || !it->second) return FALSE;
	return it->second->enabled;
}

int CXAccordion::GetItemCount(int groupId) const
{
	auto it = m_groups.find(groupId);
	if (it == m_groups.end() || !it->second) return 0;
	return (int)it->second->itemIds.size();
}

BOOL CXAccordion::RemoveItem(int itemId)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	StopItemAnima(item);
	_XAcc_GroupState* g = FindGroup(item->groupId);

	if (item->hUserEle && _XAcc_IsValidEle((HXCGUI)item->hUserEle)){
		XEle_Remove(item->hUserEle);
		item->hUserEle = NULL;
	}
	if (item->hItemWrap && _XAcc_IsValidEle((HXCGUI)item->hItemWrap))
		XEle_Destroy(item->hItemWrap);

	if (g){
		auto& ids = g->itemIds;
		ids.erase(std::remove(ids.begin(), ids.end(), itemId), ids.end());
		UpdateGroupItemsShell(g);
	}
	m_items.erase(itemId);
	delete item;
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

int CXAccordion::MeasureItemContentHeight(_XAcc_ItemState* item)
{
	if (!item || !item->hAnimWrap) return 0;
	if (item->contentType == xaccordion_content_none) return 0;

	// 折叠态下测量: 须先把 itemWrap 放开, 再在 card 级做布局; 否则文本宽度/换行不准,
	// 首次会得到偏高的单行估算, 动画结束后切 auto 就会回弹.
	const BOOL restoreCollapsed = !item->expanded && !item->anim.active;

	if (item->hItemWrap)
		XWidget_LayoutItem_SetHeight(item->hItemWrap, layout_size_auto, 0);
	XWidget_Show(item->hAnimWrap, TRUE);
	XWidget_LayoutItem_SetHeight(item->hAnimWrap, layout_size_auto, 0);
	if (item->hContentText && _XAcc_IsValidUi(item->hContentText)){
		_XAcc_LayoutAutoHeight(item->hContentText);
		XWidget_Show(item->hContentText, TRUE);
	}
	if (item->hUserEle && _XAcc_IsValidEle((HXCGUI)item->hUserEle))
		XWidget_Show(item->hUserEle, TRUE);

	_XAcc_GroupState* g = FindGroup(item->groupId);
	if (g && g->hCard && _XAcc_IsValidEle((HXCGUI)g->hCard))
		XEle_AdjustLayoutEx(g->hCard, adjustLayout_all);
	else if (m_hEle)
		XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);

	int h = 0;
	if (_XAcc_IsValidEle((HXCGUI)item->hAnimWrap))
		h = XEle_GetHeight(item->hAnimWrap);

	if (h <= 0){
		int contentH = 0;
		if (item->contentType == xaccordion_content_element && item->hUserEle)
			contentH = XEle_GetHeight(item->hUserEle);
		else if (item->hContentText && _XAcc_IsValidUi(item->hContentText)){
			XShape_AdjustLayout(item->hContentText);
			contentH = XShape_GetHeight(item->hContentText);
		}
		h = contentH + kAcc_ContentPadTop + kAcc_ContentPadBottom;
	}
	if (item->contentMinH > 0 && h < item->contentMinH) h = item->contentMinH;

	if (restoreCollapsed)
		_xacc_ApplyCollapsedLayout(item);

	return h > 0 ? h : 0;
}

void CXAccordion::_xacc_ApplyExpandedHeight(_XAcc_ItemState* item)
{
	if (!item || !item->hAnimWrap) return;
	_xacc_ApplyExpandedLayout(item);
	if (m_hEle) XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
}

void CXAccordion::ApplyItemContentHeight(_XAcc_ItemState* item, int h)
{
	if (!item || !item->hAnimWrap) return;
	if (h <= 0){
		_xacc_ApplyCollapsedLayout(item);
	}else{
		XWidget_LayoutItem_SetHeight(item->hItemWrap, layout_size_auto, 0);
		XWidget_Show(item->hAnimWrap, TRUE);
		if (item->hContentText && _XAcc_IsValidUi(item->hContentText))
			XWidget_Show(item->hContentText, TRUE);
		if (item->hUserEle && _XAcc_IsValidEle((HXCGUI)item->hUserEle))
			XWidget_Show(item->hUserEle, TRUE);
		XWidget_LayoutItem_SetHeight(item->hAnimWrap, layout_size_fixed, h);
	}
	if (m_hEle) XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
}

void CXAccordion::FinishItemAnim(_XAcc_ItemState* item)
{
	if (!item) return;
	item->anim.active = FALSE;
	item->anim.hAnima = NULL;
	if (item->expanded){
		_xacc_ApplyExpandedHeight(item);
		if (m_onItemExpand) m_onItemExpand(this, item->id, NULL);
	}else{
		ApplyItemContentHeight(item, 0);
		if (m_onItemCollapse) m_onItemCollapse(this, item->id, NULL);
	}
	UpdateItemVisual(item);
	_XAcc_GroupState* g = FindGroup(item->groupId);
	if (g) UpdateGroupItemsShell(g);
	if (m_hEle) _xacc_UpdateScrollTotalSize();
}

void CXAccordion::RemeasureItem(_XAcc_ItemState* item)
{
	if (!item || !item->expanded || !item->hAnimWrap) return;
	_xacc_ApplyExpandedHeight(item);
}

void CXAccordion::StopItemAnima(_XAcc_ItemState* item, BOOL bReleaseAnima)
{
	if (!item) return;
	if (item->anim.hHeightAnimItem){
		s_xaccAnimItemOwners.erase(item->anim.hHeightAnimItem);
		item->anim.hHeightAnimItem = NULL;
	}
	if (item->anim.hAnima){
		s_xaccAnimOwners.erase(item->anim.hAnima);
		if (bReleaseAnima && _XAcc_IsValidAnima(item->anim.hAnima))
			XAnima_Release(item->anim.hAnima, TRUE);
		item->anim.hAnima = NULL;
	}
	item->anim.active = FALSE;
	item->anim.expanding = FALSE;
}

void CXAccordion::_xacc_StopAllAnima(BOOL bReleaseAnima)
{
	for (auto& kv : m_items){
		if (kv.second) StopItemAnima(kv.second, bReleaseAnima);
	}
}

void CXAccordion::StartItemAnim(_XAcc_ItemState* item, BOOL expanding, BOOL bInstant)
{
	if (!item || !item->hAnimWrap) return;

	StopItemAnima(item);

	int naturalH = 0;
	if (expanding){
		naturalH = MeasureItemContentHeight(item);
		if (naturalH <= 0 && item->contentType != xaccordion_content_none)
			naturalH = item->contentMinH > 0 ? item->contentMinH : 40;
	}

	_XAcc_GroupState* g = FindGroup(item->groupId);

	if (!m_bAnimEnabled || bInstant){
		item->expanded = expanding;
		UpdateItemVisual(item);
		if (g) UpdateGroupItemsShell(g);
		if (expanding){
			_xacc_ApplyExpandedHeight(item);
			if (m_onItemExpand) m_onItemExpand(this, item->id, NULL);
		}else{
			ApplyItemContentHeight(item, 0);
			if (m_onItemCollapse) m_onItemCollapse(this, item->id, NULL);
		}
		if (m_hEle){
			XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
			_xacc_UpdateScrollTotalSize();
		}
		return;
	}

	item->anim.active = TRUE;
	item->anim.expanding = expanding;
	UpdateItemVisual(item);
	if (g) UpdateGroupItemsShell(g);
	SyncItemBtnCheck(item);

	const int ease = kAcc_AnimEase;
	const UINT dur = (UINT)m_animDurationMs;
	HXCGUI hSeq = XAnima_Create((HXCGUI)item->hAnimWrap, 1);
	if (!hSeq){
		item->expanded = expanding;
		if (expanding) _xacc_ApplyExpandedHeight(item);
		else ApplyItemContentHeight(item, 0);
		FinishItemAnim(item);
		return;
	}

	// 动画期间 itemWrap 必须为 auto, 否则 header 固定高度会截断高度动画.
	XWidget_LayoutItem_SetHeight(item->hItemWrap, layout_size_auto, 0);

	if (expanding){
		XWidget_Show(item->hAnimWrap, TRUE);
		if (item->hContentText && _XAcc_IsValidUi(item->hContentText))
			XWidget_Show(item->hContentText, TRUE);
		if (item->hUserEle && _XAcc_IsValidEle((HXCGUI)item->hUserEle))
			XWidget_Show(item->hUserEle, TRUE);
		XWidget_LayoutItem_SetHeight(item->hAnimWrap, layout_size_fixed, 0);
	}else{
		int curH = XEle_GetHeight(item->hAnimWrap);
		if (curH <= 0 && (item->expanded || XWidget_IsShow(item->hAnimWrap)))
			curH = MeasureItemContentHeight(item);
		if (curH <= 0) curH = item->contentMinH > 0 ? item->contentMinH : 40;
		XWidget_Show(item->hAnimWrap, TRUE);
		XWidget_LayoutItem_SetHeight(item->hAnimWrap, layout_size_fixed, curH);
		if (m_hEle) XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
	}

	const float targetH = expanding ? (float)naturalH : 0.f;
	HXCGUI hHeightAnim = XAnima_LayoutHeight(hSeq, dur, layout_size_fixed, targetH, 1, ease, FALSE);
	item->anim.hHeightAnimItem = hHeightAnim;
	XAnimaItem_SetUserData(hHeightAnim, item->id);
	s_xaccAnimItemOwners[hHeightAnim] = this;
	XAnimaItem_SetCallback(hHeightAnim, CXAccordion::AnimaItemProgressCb);

	item->anim.hAnima = hSeq;
	s_xaccAnimOwners[hSeq] = this;
	XAnima_SetUserData(hSeq, item->id);
	XAnima_SetCallback(hSeq, CXAccordion::AnimaCb);
	XAnima_Run(hSeq, (HXCGUI)m_hEle);
	if (m_hEle) XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
}

void CXAccordion::OnItemAnimaEnd(HXCGUI hAnima, int flag)
{
	(void)flag;
	const int itemId = (int)XAnima_GetUserData(hAnima);
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item || item->anim.hAnima != hAnima) return;

	s_xaccAnimOwners.erase(hAnima);
	item->anim.hAnima = NULL;
	if (item->anim.hHeightAnimItem){
		s_xaccAnimItemOwners.erase(item->anim.hHeightAnimItem);
		item->anim.hHeightAnimItem = NULL;
	}
	item->expanded = item->anim.expanding;
	FinishItemAnim(item);
	if (m_hEle){
		XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
		_xacc_UpdateScrollTotalSize();
	}
}

void CXAccordion::CollapseOthers(int itemId, int groupId)
{
	if (_XAcc_IsMultiExpandMode(m_expandMode)) return;
	for (auto& kv : m_items){
		_XAcc_ItemState* other = kv.second;
		if (!other || other->id == itemId) continue;
		const BOOL otherOpen = other->expanded
			|| (other->anim.active && other->anim.expanding);
		if (!otherOpen) continue;
		BOOL sameScope = FALSE;
		if (m_expandMode == xaccordion_expand_mode_single_global)
			sameScope = TRUE;
		else if (m_expandMode == xaccordion_expand_mode_single)
			sameScope = (other->groupId == groupId);
		if (sameScope)
			StartItemAnim(other, FALSE, FALSE);
	}
}

BOOL CXAccordion::ExpandItem(int itemId, BOOL bAnimate)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item || !IsItemOperable(item)) return FALSE;
	if (item->expanded && !item->anim.active) return TRUE;
	CollapseOthers(itemId, item->groupId);
	StartItemAnim(item, TRUE, !bAnimate || !m_bAnimEnabled);
	return TRUE;
}

BOOL CXAccordion::CollapseItem(int itemId, BOOL bAnimate)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item) return FALSE;
	if (!item->expanded && !item->anim.active) return TRUE;
	StartItemAnim(item, FALSE, !bAnimate || !m_bAnimEnabled);
	return TRUE;
}

BOOL CXAccordion::ToggleItem(int itemId, BOOL bAnimate)
{
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item || !IsItemOperable(item)) return FALSE;
	if (m_onItemClick){
		BOOL handled = FALSE;
		m_onItemClick(this, itemId, &handled);
		if (handled) return TRUE;
	}
	const BOOL willExpand = !(item->expanded || (item->anim.active && item->anim.expanding));
	const BOOL instant = !bAnimate || !m_bAnimEnabled;
	if (willExpand){
		CollapseOthers(itemId, item->groupId);
		StartItemAnim(item, TRUE, instant);
	}else{
		StartItemAnim(item, FALSE, instant);
	}
	return TRUE;
}

BOOL CXAccordion::IsItemExpanded(int itemId) const
{
	_XAcc_ItemState* item = const_cast<CXAccordion*>(this)->FindItem(itemId);
	return item ? item->expanded : FALSE;
}

BOOL CXAccordion::CollapseAll(int groupId)
{
	BOOL ok = TRUE;
	for (auto& kv : m_items){
		_XAcc_ItemState* item = kv.second;
		if (!item) continue;
		const BOOL open = item->expanded
			|| (item->anim.active && item->anim.expanding);
		if (!open) continue;
		if (groupId != 0 && item->groupId != groupId) continue;
		if (!CollapseItem(item->id, TRUE)) ok = FALSE;
	}
	return ok;
}

int CXAccordion::GetExpandedItem(int groupId) const
{
	for (auto& kv : m_items){
		_XAcc_ItemState* item = kv.second;
		if (!item || item->groupId != groupId || !item->expanded) continue;
		return item->id;
	}
	return -1;
}

HELE CXAccordion::GetItemHeaderEle(int itemId) const
{
	_XAcc_ItemState* item = const_cast<CXAccordion*>(this)->FindItem(itemId);
	return item ? item->hItemBtn : NULL;
}

HELE CXAccordion::GetItemContentHost(int itemId) const
{
	_XAcc_ItemState* item = const_cast<CXAccordion*>(this)->FindItem(itemId);
	return item ? item->hAnimWrap : NULL;
}

void CXAccordion::RefreshTheme()
{
	_XAcc_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);
	_xacc_LoadSvgAssets();
	for (auto& gkv : m_groups){
		_XAcc_GroupState* g = gkv.second;
		if (!g) continue;
		UpdateGroupVisual(g);
	}
	for (auto& ikv : m_items){
		_XAcc_ItemState* item = ikv.second;
		if (!item) continue;
		if (item->hContentText && _XAcc_IsValidUi(item->hContentText))
			XShapeText_SetTextColor(item->hContentText, m_pColors->body);
		UpdateItemVisual(item);
	}
	if (m_onThemeChanged) m_onThemeChanged(this);
	if (m_hEle) XEle_Redraw(m_hEle, FALSE);
}

void CXAccordion::DestroyAccordion()
{
	if (m_bRootDestroyed || !m_hEle) return;
	if (_XAcc_IsValidEle((HXCGUI)m_hEle))
		XEle_Destroy(m_hEle);
}

int CXAccordion::OnDestroyImpl(HELE hEle, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle) return 0;
	_xacc_StopAllAnima(FALSE);
	return 0;
}

int CXAccordion::OnHeaderCheckImpl(HELE hEle, BOOL bCheck, BOOL* pbHandled)
{
	(void)pbHandled;
	if (m_programmaticBtnCheck) return 0;

	int itemId = (int)XEle_GetUserData(hEle);
	_XAcc_ItemState* item = FindItem(itemId);
	if (!item || !IsItemOperable(item)){
		if (item) SyncItemBtnCheck(item);
		return 0;
	}

	if (m_onItemClick){
		BOOL handled = FALSE;
		m_onItemClick(this, itemId, &handled);
		if (handled){
			SyncItemBtnCheck(item);
			return 0;
		}
	}

	const BOOL instant = !m_bAnimEnabled;
	const BOOL expanded = item->expanded || (item->anim.active && item->anim.expanding);

	if (expanded)
		StartItemAnim(item, FALSE, instant);
	else{
		CollapseOthers(itemId, item->groupId);
		StartItemAnim(item, TRUE, instant);
	}
	return 0;
}

int CXAccordion::OnItemWrapPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	RECT rc;
	int itemId;
	_XAcc_ItemState* item;
	BOOL operable, expanded, drawFill;
	COLORREF fill;

	if (pbHandled) *pbHandled = TRUE;

	itemId = (int)XEle_GetUserData(hEle);
	item = FindItem(itemId);
	if (!item || !hDraw) return 0;

	memset(&rc, 0, sizeof(rc));
	XEle_GetClientRect(hEle, &rc);
	if (rc.bottom <= rc.top || rc.right <= rc.left) return 0;

	operable = IsItemOperable(item);
	expanded = item->expanded
		|| (item->anim.active && item->anim.expanding);

	fill = 0;
	drawFill = FALSE;
	if (!operable){
		fill = m_pColors->itemFillDisabled;
		drawFill = TRUE;
	}else if (expanded){
		fill = m_pColors->itemFillExpanded;
		drawFill = TRUE;
	}

	COLORREF border = m_pColors->itemBorder;
	if (drawFill && expanded && operable && !_XUITool::IsLightTheme(m_theme))
		border = 0;

	_XAcc_PaintItemShell(hDraw, rc, m_cornerRadius, border, fill, drawFill);
	return 0;
}

int CXAccordion::OnHeaderPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	RECT rc, badgeRc, textRc;
	SIZE indSz, badgeSz;
	int itemId, ch, xRight, iconY, indX, indY, indSzPx;
	int badgePadH, badgeW, badgeH, badgeX, badgeY;
	const wchar_t* indText;
	_XAcc_ItemState* item;
	HIMAGE hIcon, hInd;
	BOOL showExpanded;
	COLORREF bg, fg;

	if (pbHandled) *pbHandled = TRUE;

	itemId = (int)XEle_GetUserData(hEle);
	item = FindItem(itemId);
	if (!item || !hDraw) return 0;

	memset(&rc, 0, sizeof(rc));
	XEle_GetClientRect(hEle, &rc);
	ch = rc.bottom - rc.top;
	if (ch <= 0) return 0;

	hIcon = _xacc_ResolveItemIcon(item);
	if (hIcon){
		iconY = rc.top + (ch - kAcc_IconW) / 2;
		_XAcc_DrawImageEx(hDraw, hIcon, kAcc_PadH, iconY, kAcc_IconW, kAcc_IconW);
	}

	xRight = rc.right - kAcc_PadH;

	showExpanded = XBtn_IsCheck(hEle);
	if (item->anim.active)
		showExpanded = item->anim.expanding;

	if (m_indicatorStyle == xaccordion_indicator_text){
		indText = showExpanded ? L"-" : L"+";
		indSz.cx = indSz.cy = 0;
		_XAcc_MeasureText(indText, m_hFontIndicator, &indSz);
		indX = xRight - indSz.cx;
		indY = rc.top + (ch - indSz.cy) / 2;
		XDraw_SetFont(hDraw, m_hFontIndicator);
		XDraw_SetBrushColor(hDraw, m_pColors->indicator);
		XDraw_SetTextAlign(hDraw, textAlignFlag_left | textAlignFlag_top);
		XDraw_TextOut(hDraw, indX, indY, indText, (int)wcslen(indText));
		xRight = indX - kAcc_BadgeGap;
	}else{
		hInd = showExpanded ? m_hImgIndExpanded : m_hImgIndCollapsed;
		indSzPx = kAcc_IndicatorW;
		indX = xRight - indSzPx;
		indY = rc.top + (ch - indSzPx) / 2;
		_XAcc_DrawImageEx(hDraw, hInd, indX, indY, indSzPx, indSzPx);
		xRight = indX - kAcc_BadgeGap;
	}

	if (!item->badgeText.empty()){
		bg = fg = 0;
		_XAcc_BadgeColors(*m_pColors, item->badgeKind, &bg, &fg);

		badgeSz.cx = badgeSz.cy = 0;
		_XAcc_MeasureText(item->badgeText.getPtr(), m_hFontBadge, &badgeSz);
		badgePadH = kAcc_PadH;
		badgeW = badgeSz.cx + badgePadH * 2;
		badgeH = kAcc_BadgeH;
		badgeX = xRight - badgeW;
		badgeY = rc.top + (ch - badgeH) / 2;

		badgeRc.left = badgeX;
		badgeRc.top = badgeY;
		badgeRc.right = badgeX + badgeW;
		badgeRc.bottom = badgeY + badgeH;
		XDraw_SetBrushColor(hDraw, bg);
		XDraw_FillRoundRect(hDraw, &badgeRc, badgeH / 2, badgeH / 2);

		textRc.left = badgeRc.left + badgePadH;
		textRc.top = badgeRc.top;
		textRc.right = badgeRc.right - badgePadH;
		textRc.bottom = badgeRc.bottom;
		XDraw_SetFont(hDraw, m_hFontBadge);
		XDraw_SetBrushColor(hDraw, fg);
		XDraw_SetTextAlign(hDraw, textAlignFlag_left | textAlignFlag_vcenter);
		XDraw_DrawText(hDraw, item->badgeText.getPtr(), (int)item->badgeText.size(), &textRc);
	}

	return 0;
}

int CXAccordion::OnShowImpl(HELE hEle, BOOL bShow, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle || !bShow || m_bRootDestroyed) return 0;
	AdjustLayout();
	return 0;
}

int CXAccordion::OnSizeImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled)
{
	(void)nFlags;
	(void)nAdjustNo;
	(void)pbHandled;
	if (hEle != m_hEle || m_bRootDestroyed) return 0;
	AdjustLayout();
	return 0;
}

int CXAccordion::OnAdjustLayoutEndImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled)
{
	(void)nFlags;
	(void)nAdjustNo;
	(void)pbHandled;
	if (hEle != m_hEle || m_bRootDestroyed || m_inAdjustLayoutEndImpl) return 0;
	m_inAdjustLayoutEndImpl = TRUE;
	_xacc_UpdateScrollTotalSize();
	m_inAdjustLayoutEndImpl = FALSE;
	return 0;
}

int CXAccordion::OnDestroyEndImpl(HELE hEle, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle) return 0;
	_xacc_OnRootDestroyed();
	m_hEle = NULL;
	return 0;
}

#endif // _XCGUI_UITOOL_AGGREGATED_
