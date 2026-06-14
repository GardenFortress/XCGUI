//============================================================================
// module_xcgui_uitool_cardpanel.cpp — CXCardPanel split
// 仅由 module_xcgui_uitool.cpp #include; 勿单独编译.
//============================================================================

#ifndef _XCGUI_UITOOL_AGGREGATED_
#else

#include <cstring>
#include <unordered_map>
#include <vector>

struct _XCard_ThemeColors
{
	COLORREF cardBg;
	COLORREF groupTitle;
	COLORREF titleDisabled;
	COLORREF itemDivider;
};

struct _XCard_GroupState
{
	int                  id         = 0;
	CXText               title;
	HELE                 hGroupWrap = NULL;
	HXCGUI               hTitle     = NULL;
	HELE                 hItemWrap  = NULL;
	std::vector<HXCGUI>  userContent;
	BOOL                 enabled    = TRUE;
	int                  minHeight  = 0;
};

constexpr int kCard_CornerRadius   = _XUITool::kCornerRadius;
constexpr int kCard_PadH           = 16;
constexpr int kCard_MinH           = 46;
constexpr int kCard_GroupGap       = 20;
constexpr int kCard_GroupInnerGap  = 12;
constexpr int kCard_GroupTitlePt   = 11;
constexpr int kCard_GroupTitleH    = 24;
constexpr int kCard_DividerH       = 1;

namespace {

inline BOOL _XCard_IsValidEle(HXCGUI h)
{
	return h && XC_IsHELE(h);
}

inline BOOL _XCard_IsValidUi(HXCGUI h)
{
	return h && (XC_IsHELE(h) || XC_IsShape(h));
}

inline BOOL _XCard_IsValidFont(HFONTX h)
{
	return h && XC_IsHXCGUI((HXCGUI)h, XC_FONT);
}

inline void _XCard_ReleaseFontHandle(HFONTX& h)
{
	if (_XCard_IsValidFont(h)) XFont_Release(h);
	h = NULL;
}

inline void _XCard_LayoutFillWidth(HXCGUI h)
{
	if (_XCard_IsValidUi(h))
		XWidget_LayoutItem_SetWidth(h, layout_size_fill, 0);
}

void _XCard_PrepareLayoutEle(HELE hEle)
{
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return;
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_EnableDrawFocus(hEle, FALSE);
	XEle_EnableMouseThrough(hEle, TRUE);
}

void _XCard_PrepareCardShell(HELE hEle)
{
	if (!hEle || !XC_IsHELE((HXCGUI)hEle)) return;
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_EnableDrawFocus(hEle, FALSE);
}

void _XCard_ClearEleBk(HELE hEle)
{
	if (hEle) XEle_SetBkInfo(hEle, L"");
}

void _XCard_DetachFromParent(HXCGUI h)
{
	if (!h) return;
	if (XC_IsHELE(h)){
		HELE hEle = (HELE)h;
		if (XWidget_GetParentEle(h))
			XEle_Remove(hEle);
	}else if (XC_IsShape(h)){
		XShape_RemoveShape(h);
	}
}

BOOL _XCard_AttachToItemWrap(HELE hItemWrap, HXCGUI h)
{
	if (!hItemWrap || !h) return FALSE;
	if (!XC_IsHELE(h) && !XC_IsShape(h)) return FALSE;
	_XCard_DetachFromParent(h);
	return XEle_AddChild(hItemWrap, h);
}

void _XCard_DestroyUi(HXCGUI h)
{
	if (!h) return;
	if (XC_IsHELE(h)) XEle_Destroy((HELE)h);
	else if (XC_IsShape(h)) XShape_Destroy(h);
}

void _XCard_DestroyGroupTitle(HXCGUI& hTitle)
{
	if (!hTitle || !XC_IsShape(hTitle)) return;
	XShapeText_SetFont(hTitle, NULL);
	XShape_RemoveShape(hTitle);
	XShape_Destroy(hTitle);
	hTitle = NULL;
}

void _XCard_PaintCardShell(HDRAW hDraw, const RECT& rcIn, int tl, int tr, int br, int bl,
	COLORREF fillColor)
{
	int w, h, rTl, rTr, rBr, rBl;

	if (!hDraw) return;
	w = rcIn.right - rcIn.left;
	h = rcIn.bottom - rcIn.top;
	if (w <= 0 || h <= 0) return;

	rTl = tl; rTr = tr; rBr = br; rBl = bl;
	if (rTl * 2 > h) rTl = h / 2;
	if (rTr * 2 > h) rTr = h / 2;
	if (rBr * 2 > h) rBr = h / 2;
	if (rBl * 2 > h) rBl = h / 2;
	if (rTl * 2 > w) rTl = w / 2;
	if (rTr * 2 > w) rTr = w / 2;
	if (rBr * 2 > w) rBr = w / 2;
	if (rBl * 2 > w) rBl = w / 2;

	if (((fillColor >> 24) & 0xFF) == 0) return;
	XDraw_SetBrushColor(hDraw, fillColor);
	XDraw_FillRoundRectEx(hDraw, &rcIn, rTl, rTr, rBr, rBl);
}

BOOL _XCard_GetUiRectInEleClient(HELE hParent, HXCGUI hUi, RECT* outRc)
{
	if (!hParent || !hUi || !outRc) return FALSE;
	memset(outRc, 0, sizeof(RECT));
	if (XC_IsHELE(hUi))
		XEle_GetWndClientRect((HELE)hUi, outRc);
	else if (XC_IsShape(hUi))
		XShape_GetWndClientRect(hUi, outRc);
	else
		return FALSE;
	XEle_RectWndClientToEleClient(hParent, outRc);
	return (outRc->bottom > outRc->top && outRc->right > outRc->left);
}

void _XCard_PaintItemDividers(HDRAW hDraw, HELE hItemWrap, const RECT& rcCard,
	const std::vector<HXCGUI>& items, COLORREF dividerColor)
{
	size_t i;
	int y;
	RECT rcLine;

	if (!hDraw || !hItemWrap || items.size() < 2) return;
	if (((dividerColor >> 24) & 0xFF) == 0) return;

	for (i = 0; i + 1 < items.size(); ++i){
		RECT rcItem;
		if (!_XCard_GetUiRectInEleClient(hItemWrap, items[i], &rcItem))
			continue;
		y = rcItem.bottom;
		if (y <= rcCard.top || y >= rcCard.bottom) continue;
		rcLine.left   = rcCard.left;
		rcLine.top    = y;
		rcLine.right  = rcCard.right;
		rcLine.bottom = y + kCard_DividerH;
		XDraw_SetBrushColor(hDraw, dividerColor);
		XDraw_FillRect(hDraw, &rcLine);
	}
}

void _XCard_ResolveTheme(xuitool_theme_ theme, COLORREF customText, COLORREF customBg,
	COLORREF customAccent, _XCard_ThemeColors* c)
{
	(void)customAccent;
	if (!c) return;
	_XUITool::ThemePalette base;
	memset(&base, 0, sizeof(base));
	_XUITool::ResolvePalette(theme, customText, customBg, customAccent, &base);
	BOOL light = _XUITool::IsLightTheme(theme);

	c->cardBg = light ? RGBA(255, 255, 255, 255) : RGBA(38, 38, 38, 255);
	if (theme == xuitool_theme_custom)
		c->cardBg = customBg;
	c->groupTitle = base.text;
	c->titleDisabled = _XUITool::WithAlpha(c->groupTitle, 128);
	c->itemDivider = light ? RGBA(235, 235, 235, 255) : RGBA(51, 51, 51, 255);
}

} // namespace

//============================================================================
// CXCardPanel
//============================================================================

CXCardPanel::CXCardPanel()
	: m_theme(xuitool_theme_auto)
	, m_customText(_XUITool::kDarkText)
	, m_customBg(RGBA(38, 38, 38, 255))
	, m_customAccent(_XUITool::kDarkAccent)
	, m_cornerRadius(kCard_CornerRadius)
	, m_cornerTL(kCard_CornerRadius)
	, m_cornerTR(kCard_CornerRadius)
	, m_cornerBR(kCard_CornerRadius)
	, m_cornerBL(kCard_CornerRadius)
	, m_bCornerIndividual(FALSE)
	, m_groupTitleAlign(xcardpanel_group_title_align_left)
	, m_bScrollEnabled(TRUE)
	, m_nextGroupId(1)
	, m_hFontGroup(NULL)
	, m_onThemeChanged(NULL)
	, m_bRootDestroyed(FALSE)
	, m_inAdjustLayoutEndImpl(FALSE)
	, m_inLayoutSync(FALSE)
	, m_pColors(new _XCard_ThemeColors())
{
	_XCard_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);
}

CXCardPanel::~CXCardPanel()
{
	if (m_bRootDestroyed){
		m_hFontGroup = NULL;
	}else{
		if (m_hEle || !m_groups.empty()){
			_xcard_DetachAllUserContent();
			_xcard_DestroyDetachedUserContent();
			_xcard_ClearState();
		}
		m_hEle = NULL;
		_xcard_ReleaseOwnedResources();
	}
	delete m_pColors;
	m_pColors = NULL;
}

void CXCardPanel::DetachFonts()
{
	m_hFontGroup = NULL;
}

void CXCardPanel::_xcard_DetachFontRefsFromUi()
{
	for (auto& kv : m_groups){
		_XCard_GroupState* g = kv.second;
		if (!g || !g->hTitle || !_XCard_IsValidUi(g->hTitle)) continue;
		XShapeText_SetFont(g->hTitle, NULL);
	}
}

void CXCardPanel::_xcard_ReleaseOwnedResources()
{
	_xcard_DetachFontRefsFromUi();
	ReleaseFonts();
}

void CXCardPanel::_xcard_DetachAllUserContent()
{
	for (auto& kv : m_groups){
		_XCard_GroupState* g = kv.second;
		if (!g) continue;
		for (size_t i = 0; i < g->userContent.size(); ++i)
			_XCard_DetachFromParent(g->userContent[i]);
		g->userContent.clear();
	}
}

void CXCardPanel::_xcard_DestroyDetachedUserContent()
{
	for (auto& kv : m_groups){
		_XCard_GroupState* g = kv.second;
		if (!g) continue;
		for (size_t i = 0; i < g->userContent.size(); ++i)
			_XCard_DestroyUi(g->userContent[i]);
		g->userContent.clear();
	}
}

void CXCardPanel::_xcard_ClearState()
{
	for (auto& kv : m_groups)
		delete kv.second;
	m_groups.clear();
}

void CXCardPanel::_xcard_OnRootDestroyed()
{
	if (m_bRootDestroyed) return;
	m_bRootDestroyed = TRUE;
	_xcard_ClearState();
	m_hEle = NULL;
	_xcard_ReleaseOwnedResources();
}

void CXCardPanel::ReleaseFonts()
{
	_XCard_ReleaseFontHandle(m_hFontGroup);
}

void CXCardPanel::EnsureFonts()
{
	if (!_XCard_IsValidFont(m_hFontGroup))
		m_hFontGroup = XFont_CreateEx(L"Segoe UI", kCard_GroupTitlePt, fontStyle_bold);
}

HELE CXCardPanel::Create(HXCGUI hParent)
{
	m_bRootDestroyed = FALSE;
	m_hEle = XLayoutFrame_CreateEx(hParent);
	if (!_XCard_IsValidEle((HXCGUI)m_hEle)){
		m_hEle = NULL;
		return NULL;
	}
	XUI_EnableCSS(m_hEle, FALSE);

	EnsureFonts();
	_XCard_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);

	EnableCanvas(TRUE);
	EnableDrawBorder(FALSE);
	EnableDrawFocus(FALSE);
	XEle_EnableBkTransparent(m_hEle, TRUE);

	XLayoutFrame_EnableLayout(m_hEle, TRUE);
	XLayoutBox_SetAlignV(m_hEle, layout_align_top);
	XLayoutBox_SetSpace(m_hEle, 0);
	XWidget_LayoutItem_SetWidth(m_hEle, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(m_hEle, layout_size_fill, 0);
	_xcard_ApplyScroll();

	InstallEvents();
	return m_hEle;
}

void CXCardPanel::_xcard_ApplyScroll()
{
	if (!_XCard_IsValidEle((HXCGUI)m_hEle)) return;
	XSView_EnableAutoShowScrollBar(m_hEle, m_bScrollEnabled ? TRUE : FALSE);
	XSView_ShowSBarH(m_hEle, FALSE);
	XSView_ShowSBarV(m_hEle, FALSE);
}

void CXCardPanel::InstallEvents()
{
	if (!_XCard_IsValidEle((HXCGUI)m_hEle)) return;
	XEle_RegEventCPP1(m_hEle, XE_DESTROY, &CXCardPanel::OnDestroyImpl);
	XEle_RegEventCPP1(m_hEle, XE_DESTROY_END, &CXCardPanel::OnDestroyEndImpl);
	XEle_RegEventCPP1(m_hEle, XE_SHOW, &CXCardPanel::OnShowImpl);
	XEle_RegEventCPP1(m_hEle, XE_SIZE, &CXCardPanel::OnSizeImpl);
	XEle_RegEventCPP1(m_hEle, XE_ADJUSTLAYOUT_END, &CXCardPanel::OnAdjustLayoutEndImpl);
}

BOOL CXCardPanel::IsValid() const
{
	return _XCard_IsValidEle((HXCGUI)m_hEle);
}

HELE CXCardPanel::GetHandle() const
{
	return m_hEle;
}

void CXCardPanel::SetTheme(xuitool_theme_ theme)
{
	m_theme = theme;
	RefreshTheme();
}

xuitool_theme_ CXCardPanel::GetTheme() const
{
	return m_theme;
}

void CXCardPanel::SetTextColor(COLORREF c)
{
	m_customText = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXCardPanel::SetBkColor(COLORREF c)
{
	m_customBg = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXCardPanel::SetAccentColor(COLORREF c)
{
	m_customAccent = c;
	if (m_theme == xuitool_theme_custom) RefreshTheme();
}

void CXCardPanel::SetCornerRadius(int r)
{
	m_cornerRadius = r > 0 ? r : kCard_CornerRadius;
	m_cornerTL = m_cornerTR = m_cornerBR = m_cornerBL = m_cornerRadius;
	m_bCornerIndividual = FALSE;
	RefreshTheme();
}

void CXCardPanel::SetCornerRadiusEx(int tl, int tr, int br, int bl)
{
	m_cornerTL = tl >= 0 ? tl : 0;
	m_cornerTR = tr >= 0 ? tr : 0;
	m_cornerBR = br >= 0 ? br : 0;
	m_cornerBL = bl >= 0 ? bl : 0;
	m_bCornerIndividual = TRUE;
	RefreshTheme();
}

void CXCardPanel::EnableScroll(BOOL bEnable)
{
	m_bScrollEnabled = bEnable;
	_xcard_ApplyScroll();
}

void CXCardPanel::SetOnThemeChanged(xcardpanel_void_event fn)
{
	m_onThemeChanged = fn;
}

void CXCardPanel::AdjustLayout()
{
	if (m_inLayoutSync || !m_hEle) return;
	m_inLayoutSync = TRUE;
	for (auto& kv : m_groups){
		_XCard_GroupState* g = kv.second;
		if (g && g->hItemWrap && _XCard_IsValidEle((HXCGUI)g->hItemWrap))
			XEle_AdjustLayoutEx(g->hItemWrap, adjustLayout_all);
	}
	XEle_AdjustLayoutEx(m_hEle, adjustLayout_all);
	_xcard_UpdateScrollTotalSize();
	m_inLayoutSync = FALSE;
}

void CXCardPanel::_xcard_UpdateScrollTotalSize()
{
	if (!_XCard_IsValidEle((HXCGUI)m_hEle) || !m_bScrollEnabled) return;

	int w = XLayoutFrame_GetWidthIn(m_hEle);
	if (w <= 0) w = 1;

	int contentH = 0;
	for (auto& gkv : m_groups){
		_XCard_GroupState* g = gkv.second;
		if (!g || !g->hGroupWrap || !_XCard_IsValidEle((HXCGUI)g->hGroupWrap)) continue;
		if (contentH > 0) contentH += kCard_GroupGap;
		contentH += XEle_GetHeight(g->hGroupWrap);
	}
	if (contentH <= 0) return;

	SIZE total = {0};
	XSView_GetTotalSize(m_hEle, &total);
	if (total.cx != w || total.cy != contentH)
		XSView_SetTotalSize(m_hEle, w, contentH);
}

_XCard_GroupState* CXCardPanel::FindGroup(int groupId)
{
	auto it = m_groups.find(groupId);
	return it != m_groups.end() ? it->second : NULL;
}

int CXCardPanel::_xcard_GroupTitleAlignFlags() const
{
	switch (m_groupTitleAlign){
	case xcardpanel_group_title_align_center:
		return textAlignFlag_center | textAlignFlag_vcenter;
	case xcardpanel_group_title_align_right:
		return textAlignFlag_right | textAlignFlag_vcenter;
	default:
		return textAlignFlag_left | textAlignFlag_vcenter;
	}
}

void CXCardPanel::_xcard_ApplyGroupTitleAlign(_XCard_GroupState* g)
{
	if (!g || !g->hTitle || !_XCard_IsValidUi(g->hTitle)) return;
	XShapeText_SetTextAlign(g->hTitle, _xcard_GroupTitleAlignFlags());
}

void CXCardPanel::_xcard_ApplyAllGroupTitleAlign()
{
	for (auto& gkv : m_groups)
		_xcard_ApplyGroupTitleAlign(gkv.second);
}

void CXCardPanel::SetGroupTitleAlign(xcardpanel_group_title_align_ align)
{
	if (m_groupTitleAlign == align) return;
	m_groupTitleAlign = align;
	_xcard_ApplyAllGroupTitleAlign();
	if (m_hEle) XEle_Redraw(m_hEle, FALSE);
}

xcardpanel_group_title_align_ CXCardPanel::GetGroupTitleAlign() const
{
	return m_groupTitleAlign;
}

void CXCardPanel::_xcard_ApplyItemWrapMinHeight(_XCard_GroupState* g)
{
	if (!g || !g->hItemWrap) return;
	const int minH = g->minHeight > 0 ? g->minHeight : kCard_MinH;
	XWidget_LayoutItem_SetMinSize(g->hItemWrap, 0, minH);
}

void CXCardPanel::_xcard_ApplyGroupTitleMargin(_XCard_GroupState* g)
{
	if (!g || !g->hTitle || !_XCard_IsValidUi(g->hTitle)) return;
	// 同 CXAccordion kAcc_GroupInnerGap：标题上下各 12px；水平 16px 与卡片 padding 对齐
	XWidget_LayoutItem_SetMargin(g->hTitle, kCard_PadH, kCard_GroupInnerGap, kCard_PadH, kCard_GroupInnerGap);
	XWidget_LayoutItem_SetHeight(g->hTitle, layout_size_fixed, kCard_GroupTitleH);
}

void CXCardPanel::_xcard_GetCornerRadii(int* tl, int* tr, int* br, int* bl) const
{
	if (tl) *tl = m_cornerTL;
	if (tr) *tr = m_cornerTR;
	if (br) *br = m_cornerBR;
	if (bl) *bl = m_cornerBL;
	(void)m_bCornerIndividual;
}

HELE CXCardPanel::_xcard_CreateItemWrap(_XCard_GroupState* g)
{
	if (!g || !g->hGroupWrap) return NULL;

	g->hItemWrap = XLayout_Create(0, 0, 100, kCard_MinH, g->hGroupWrap);
	if (!g->hItemWrap) return NULL;

	XLayout_EnableLayout(g->hItemWrap, TRUE);
	XLayoutBox_SetAlignV(g->hItemWrap, layout_align_top);
	XLayoutBox_SetSpace(g->hItemWrap, 0);
	_XCard_PrepareCardShell(g->hItemWrap);
	XEle_EnableBkTransparent(g->hItemWrap, TRUE);
	_XCard_ClearEleBk(g->hItemWrap);
	XEle_SetUserData(g->hItemWrap, (vint)g->id);
	XEle_SetPadding(g->hItemWrap, kCard_PadH, 0, kCard_PadH, 0);
	XEle_RegEventCPP1(g->hItemWrap, XE_PAINT, &CXCardPanel::OnItemWrapPaintImpl);
	XEle_EnableEvent_XE_PAINT_END(g->hItemWrap, TRUE);
	XEle_RegEventCPP1(g->hItemWrap, XE_PAINT_END, &CXCardPanel::OnItemWrapPaintEndImpl);
	XWidget_LayoutItem_SetWidth(g->hItemWrap, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(g->hItemWrap, layout_size_auto, 0);
	_xcard_ApplyItemWrapMinHeight(g);
	return g->hItemWrap;
}

void CXCardPanel::UpdateGroupVisual(_XCard_GroupState* g)
{
	if (!g) return;
	if (g->hGroupWrap && _XCard_IsValidEle((HXCGUI)g->hGroupWrap))
		XEle_SetAlpha(g->hGroupWrap, 255);
	if (g->hItemWrap && _XCard_IsValidEle((HXCGUI)g->hItemWrap)){
		XEle_Enable(g->hItemWrap, g->enabled);
		XEle_Redraw(g->hItemWrap, FALSE);
	}
	if (g->hTitle && _XCard_IsValidUi(g->hTitle)){
		COLORREF c = g->enabled ? m_pColors->groupTitle : m_pColors->titleDisabled;
		XShapeText_SetTextColor(g->hTitle, c);
		XShape_AdjustLayout(g->hTitle);
	}
}

void CXCardPanel::_xcard_RefreshGroupLayout(_XCard_GroupState* g)
{
	if (!g) return;
	UpdateGroupVisual(g);
	if (g->hItemWrap && _XCard_IsValidEle((HXCGUI)g->hItemWrap))
		XEle_AdjustLayoutEx(g->hItemWrap, adjustLayout_all);
	if (g->hGroupWrap && _XCard_IsValidEle((HXCGUI)g->hGroupWrap))
		XEle_Redraw(g->hGroupWrap, FALSE);
}

BOOL CXCardPanel::_xcard_AttachGroupContent(_XCard_GroupState* g, HXCGUI h, BOOL bReplace)
{
	if (!g || !g->hItemWrap || !h) return FALSE;
	if (!XC_IsHELE(h) && !XC_IsShape(h)) return FALSE;
	if (bReplace)
		ClearGroupContentEle(g);
	if (!_XCard_AttachToItemWrap(g->hItemWrap, h))
		return FALSE;
	g->userContent.push_back(h);
	_xcard_RefreshGroupLayout(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

int CXCardPanel::AddGroup(const wchar_t* pTitle)
{
	if (!m_hEle) return 0;
	EnsureFonts();

	_XCard_GroupState* g = new _XCard_GroupState();
	g->id = m_nextGroupId++;
	if (pTitle && pTitle[0]) g->title = pTitle;

	g->hGroupWrap = XLayout_Create(0, 0, 100, 10, m_hEle);
	if (!g->hGroupWrap){
		delete g;
		return 0;
	}
	_XCard_PrepareLayoutEle(g->hGroupWrap);
	XLayout_EnableLayout(g->hGroupWrap, TRUE);
	XLayoutBox_SetAlignV(g->hGroupWrap, layout_align_top);
	XLayoutBox_SetSpace(g->hGroupWrap, 0);
	XWidget_LayoutItem_SetWidth(g->hGroupWrap, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(g->hGroupWrap, layout_size_auto, 0);
	if (!m_groups.empty())
		XWidget_LayoutItem_SetMargin(g->hGroupWrap, 0, kCard_GroupGap, 0, 0);

	if (!g->title.empty()){
		HXCGUI hTitle = XShapeText_Create(0, 0, 100, 24, g->title.getPtr(), g->hGroupWrap);
		g->hTitle = hTitle;
		if (g->hTitle){
			XUI_EnableCSS(g->hTitle, FALSE);
			XShapeText_SetTextColor(hTitle, m_pColors->groupTitle);
			XShapeText_SetFont(hTitle, m_hFontGroup);
			_XCard_LayoutFillWidth(g->hTitle);
			_xcard_ApplyGroupTitleMargin(g);
			_xcard_ApplyGroupTitleAlign(g);
		}
	}

	if (!_xcard_CreateItemWrap(g)){
		if (g->hGroupWrap) XEle_Destroy(g->hGroupWrap);
		delete g;
		return 0;
	}

	m_groups[g->id] = g;
	UpdateGroupVisual(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return g->id;
}

BOOL CXCardPanel::SetGroupTitle(int groupId, const wchar_t* pTitle)
{
	_XCard_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	g->title = pTitle ? pTitle : L"";
	if (g->hTitle && _XCard_IsValidUi(g->hTitle)){
		XShapeText_SetText(g->hTitle, g->title.getPtr());
		XWidget_Show(g->hTitle, g->title.empty() ? FALSE : TRUE);
	}
	_xcard_ApplyGroupTitleMargin(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

BOOL CXCardPanel::SetGroupEnabled(int groupId, BOOL bEnabled)
{
	_XCard_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	if (g->enabled == (bEnabled ? TRUE : FALSE)) return TRUE;
	g->enabled = bEnabled ? TRUE : FALSE;
	UpdateGroupVisual(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

BOOL CXCardPanel::IsGroupEnabled(int groupId) const
{
	auto it = m_groups.find(groupId);
	if (it == m_groups.end() || !it->second) return FALSE;
	return it->second->enabled;
}

BOOL CXCardPanel::ClearGroupContentEle(_XCard_GroupState* g)
{
	if (!g) return FALSE;
	for (size_t i = 0; i < g->userContent.size(); ++i)
		_XCard_DetachFromParent(g->userContent[i]);
	g->userContent.clear();
	return TRUE;
}

BOOL CXCardPanel::SetGroupContentEle(int groupId, HXCGUI hObj)
{
	_XCard_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	return _xcard_AttachGroupContent(g, hObj, TRUE);
}

BOOL CXCardPanel::AddGroupContentEle(int groupId, HXCGUI hObj)
{
	_XCard_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	return _xcard_AttachGroupContent(g, hObj, FALSE);
}

BOOL CXCardPanel::ClearGroupContent(int groupId)
{
	_XCard_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	return ClearGroupContentEle(g);
}

BOOL CXCardPanel::SetGroupMinHeight(int groupId, int h)
{
	_XCard_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	g->minHeight = h > 0 ? h : 0;
	_xcard_ApplyItemWrapMinHeight(g);
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

HELE CXCardPanel::GetGroupContentHost(int groupId) const
{
	_XCard_GroupState* g = const_cast<CXCardPanel*>(this)->FindGroup(groupId);
	return g ? g->hItemWrap : NULL;
}

BOOL CXCardPanel::RemoveGroup(int groupId)
{
	_XCard_GroupState* g = FindGroup(groupId);
	if (!g) return FALSE;
	ClearGroupContentEle(g);
	_XCard_DestroyGroupTitle(g->hTitle);
	if (g->hGroupWrap && _XCard_IsValidEle((HXCGUI)g->hGroupWrap))
		XEle_Destroy(g->hGroupWrap);
	m_groups.erase(groupId);
	delete g;
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	return TRUE;
}

void CXCardPanel::ClearGroups()
{
	std::vector<int> gids;
	gids.reserve(m_groups.size());
	for (auto& kv : m_groups) gids.push_back(kv.first);
	for (size_t i = 0; i < gids.size(); ++i) RemoveGroup(gids[i]);
}

int CXCardPanel::GetGroupCount() const
{
	return (int)m_groups.size();
}

void CXCardPanel::RefreshTheme()
{
	_XCard_ResolveTheme(m_theme, m_customText, m_customBg, m_customAccent, m_pColors);
	for (auto& gkv : m_groups)
		UpdateGroupVisual(gkv.second);
	if (m_onThemeChanged) m_onThemeChanged(this);
	if (m_hEle) XEle_Redraw(m_hEle, FALSE);
}

void CXCardPanel::DestroyCardPanel()
{
	if (m_bRootDestroyed || !m_hEle) return;
	if (_XCard_IsValidEle((HXCGUI)m_hEle))
		XEle_Destroy(m_hEle);
}

int CXCardPanel::OnItemWrapPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	RECT rc;
	int groupId, tl, tr, br, bl;
	_XCard_GroupState* g;

	if (pbHandled) *pbHandled = TRUE;

	groupId = (int)XEle_GetUserData(hEle);
	g = FindGroup(groupId);
	if (!g || !hDraw) return 0;

	memset(&rc, 0, sizeof(rc));
	XEle_GetClientRect(hEle, &rc);
	if (rc.bottom <= rc.top || rc.right <= rc.left) return 0;

	_xcard_GetCornerRadii(&tl, &tr, &br, &bl);
	_XCard_PaintCardShell(hDraw, rc, tl, tr, br, bl, m_pColors->cardBg);
	return 0;
}

int CXCardPanel::OnItemWrapPaintEndImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	RECT rc;
	int groupId;
	_XCard_GroupState* g;

	if (pbHandled) *pbHandled = TRUE;

	groupId = (int)XEle_GetUserData(hEle);
	g = FindGroup(groupId);
	if (!g || !hDraw || g->userContent.size() < 2) return 0;

	memset(&rc, 0, sizeof(rc));
	XEle_GetClientRect(hEle, &rc);
	if (rc.bottom <= rc.top || rc.right <= rc.left) return 0;

	_XCard_PaintItemDividers(hDraw, hEle, rc, g->userContent, m_pColors->itemDivider);
	return 0;
}

int CXCardPanel::OnDestroyImpl(HELE hEle, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle) return 0;
	return 0;
}

int CXCardPanel::OnDestroyEndImpl(HELE hEle, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle) return 0;
	_xcard_OnRootDestroyed();
	m_hEle = NULL;
	return 0;
}

int CXCardPanel::OnShowImpl(HELE hEle, BOOL bShow, BOOL* pbHandled)
{
	(void)pbHandled;
	if (hEle != m_hEle || !bShow || m_bRootDestroyed) return 0;
	AdjustLayout();
	return 0;
}

int CXCardPanel::OnSizeImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled)
{
	(void)nFlags;
	(void)nAdjustNo;
	(void)pbHandled;
	if (hEle != m_hEle || m_bRootDestroyed) return 0;
	AdjustLayout();
	return 0;
}

int CXCardPanel::OnAdjustLayoutEndImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled)
{
	(void)nFlags;
	(void)nAdjustNo;
	(void)pbHandled;
	if (hEle != m_hEle || m_bRootDestroyed || m_inAdjustLayoutEndImpl) return 0;
	m_inAdjustLayoutEndImpl = TRUE;
	_xcard_UpdateScrollTotalSize();
	m_inAdjustLayoutEndImpl = FALSE;
	return 0;
}

#endif // _XCGUI_UITOOL_AGGREGATED_
