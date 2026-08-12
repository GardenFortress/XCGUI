//============================================================================
// module_xcgui_uitool_colorpicker.cpp — CXColorPicker 实现 (P0)
//============================================================================

static void XClr_FormatHexText(const xcolor_rgba_& c, BOOL withAlpha, wchar_t* buf, size_t bufCount)
{
	if (!buf || bufCount == 0) return;
	if (withAlpha && c.a < 255)
		swprintf_s(buf, bufCount, L"#%02X%02X%02X%02X", c.a, c.r, c.g, c.b);
	else
		swprintf_s(buf, bufCount, L"#%02X%02X%02X", c.r, c.g, c.b);
}

namespace {

constexpr int kClr_PadH           = 20;
constexpr int kClr_PadV           = 8;
constexpr int kClr_CornerRadius   = _XUITool::kCornerRadius;
constexpr int kClr_HeaderH        = 28;
constexpr int kClr_HeaderSepGap   = 4;
constexpr int kClr_HeaderCloseRight = 12;
constexpr int kClr_CloseIconSize  = 24;
constexpr BYTE kClr_InputBorderBlurA  = 38;   // 15%
constexpr BYTE kClr_InputBorderHoverA = 38;   // 15%
constexpr BYTE kClr_InputBorderFocusA = 38;   // 15% 描边
constexpr BYTE kClr_InputFillFocusA   = 15;   // 6%  组合框聚焦填充
constexpr int  kClr_InputComboRound   = 4;
constexpr int  kClr_ComboDropRound    = 4;
constexpr int  kClr_ComboTextPadL     = 4;   // 与组合框 SetBorderSize 左内边距一致
constexpr int kClr_TitleFontPt    = 10;
constexpr int kClr_ComboFontPt    = 9;
constexpr int kClr_CloseBtn       = 28;
constexpr int kClr_CloseHoverA    = 36;   // 14% of 255
constexpr int kClr_CloseDownA     = 66;   // 26% of 255
constexpr int kClr_SepGap         = 16;
constexpr int kClr_SvSize         = 200;  // 设计基准，实际按 kClr_InnerW 等比放大
constexpr int kClr_VBarW          = 12;
constexpr int kClr_VBarGap        = 8;
constexpr int kClr_EditH          = 32;
constexpr int kClr_EyedropperSize = 32;
constexpr int kClr_AlphaEditW     = 44;
constexpr int kClr_ComboW         = 52;
constexpr int kClr_PctLabelW      = 16;
constexpr int kClr_PctLabelGap    = 4;
constexpr int kClr_InputGap       = 6;
constexpr int kClr_ChannelGap     = 4;
constexpr int kClr_ChannelMinW    = 36;
constexpr int kClr_HexPrefixW     = 30;
constexpr int kClr_HexMarkX       = 12;
constexpr int kClr_HexMaxLen      = 6;
constexpr int kClr_ChannelMaxLen  = 3;
constexpr int kClr_AlphaPctMaxLen = 3;
constexpr int kClr_ContentW       = 318;
constexpr int kClr_InnerW         = kClr_ContentW - kClr_PadH * 2;
constexpr int kClr_SwatchSize     = 28;
constexpr int kClr_SwatchGap      = 8;
constexpr int kClr_SwatchRound    = 4;
constexpr int kClr_SwatchSelectedBorderWidth = 2;
constexpr int kClr_SwatchSelectedRoundInset = 1;
constexpr int kClr_SwatchNeutralMaxSpread = 28;
constexpr int kClr_SwatchSimilarMaxDist = 48;
constexpr int kClr_SvRound        = 4;
constexpr int kClr_ThumbRadius  = 6;
constexpr int kClr_ThumbStroke  = 2;
constexpr int kClr_CandidateMax   = 5;
constexpr int kClr_PickPreviewSize = kClr_SwatchSize;
constexpr int kClr_ActionH        = 32;
constexpr int kClr_ActionGap      = 12;
#ifndef OCR_NORMAL
#define OCR_NORMAL 32512
#endif

constexpr int kClr_ResultCancel = 0;
constexpr int kClr_ResultOk     = 1;

enum _XClr_PopupMode
{
	_XClr_PopupMode_Default = 0,
	_XClr_PopupMode_Ele,
	_XClr_PopupMode_Pos,
};

enum _XClr_Cmd
{
	_XClr_Cmd_None = 0,
	_XClr_Cmd_Eyedropper,
	_XClr_Cmd_PickPrev,
	_XClr_Cmd_PickCurr,
	_XClr_Cmd_Mode,
	_XClr_Cmd_Close,
	_XClr_Cmd_Confirm,
	_XClr_Cmd_Cancel,
	_XClr_Cmd_SpinRUp, _XClr_Cmd_SpinRDown,
	_XClr_Cmd_SpinGUp, _XClr_Cmd_SpinGDown,
	_XClr_Cmd_SpinBUp, _XClr_Cmd_SpinBDown,
	_XClr_Cmd_SpinAUp, _XClr_Cmd_SpinADown,
	_XClr_Cmd_SpinHUp, _XClr_Cmd_SpinHDown,
	_XClr_Cmd_SpinSUp, _XClr_Cmd_SpinSDown,
	_XClr_Cmd_SpinLUp, _XClr_Cmd_SpinLDown,
	_XClr_Cmd_AddCandidate = 100,
	_XClr_Cmd_Candidate0,
	_XClr_Cmd_Candidate1,
	_XClr_Cmd_Candidate2,
	_XClr_Cmd_Candidate3,
	_XClr_Cmd_Candidate4,
};

enum _XClr_DragKind
{
	_XClr_Drag_None = 0,
	_XClr_Drag_Sv,
	_XClr_Drag_Hue,
	_XClr_Drag_Alpha,
};

enum _XClr_Channel
{
	_XClr_Channel_R = 0,
	_XClr_Channel_G = 1,
	_XClr_Channel_B = 2,
	_XClr_Channel_A = 3,
};

struct _XClr_ThemeColors
{
	COLORREF wndBg;
	COLORREF text;
	COLORREF mutedText;
	COLORREF border;
	COLORREF inputBg;
	COLORREF buttonBg;
	COLORREF accentText;
	COLORREF caretColor;
	const wchar_t* bkPrimary;
	const wchar_t* bkNormal;
	const wchar_t* bkSubtle;
	const wchar_t* bkInput;
	const wchar_t* bkSpin;
	COLORREF separator;
};

struct _XClr_Ctx
{
	xuitool_theme_ theme = xuitool_theme_auto;
	_XClr_ThemeColors colors{};
	BOOL showAlpha = TRUE;
	BOOL liveNotify = TRUE;

	HWINDOW hWnd = NULL;
	HWND hParent = NULL;
	int contentW = 0;
	int contentH = 0;
	int contentOffX = _XUITool::kShadowMargin;
	int contentOffY = _XUITool::kShadowMargin;
	int cornerRadius = kClr_CornerRadius;
	int sepHeaderY = -1;
	int sepFooterY = -1;

	HXCGUI hTitle = NULL;
	HELE hBtnClose = NULL;
	HELE hBtnEyedropper = NULL;
	HELE hBtnPickPreview = NULL;
	HELE hComboMode = NULL;
	HELE hSvPanel = NULL;
	HELE hHueBar = NULL;
	HELE hAlphaBar = NULL;
	HELE hModeWrap = NULL;
	HELE hEditHex = NULL;
	HELE hRgbWrap = NULL;
	HELE hEditR = NULL;
	HELE hEditG = NULL;
	HELE hEditB = NULL;
	HELE hHslWrap = NULL;
	HELE hEditH = NULL;
	HELE hEditS = NULL;
	HELE hEditL = NULL;
	HELE hEditAlphaPct = NULL;
	HXCGUI hLabelPct = NULL;
	HELE hBtnAddCandidate = NULL;
	HELE hCandidateSwatch[kClr_CandidateMax]{};
	HELE hBtnCancel = NULL;
	HELE hBtnConfirm = NULL;

	xcolor_input_mode_ inputMode = xcolor_input_hex;
	HFONTX hFont = NULL;
	HFONTX hFontTitle = NULL;
	HFONTX hFontCombo = NULL;
	xcolor_rgba_ rgba{};
	xcolor_rgba_ resultColor{};
	xcolor_rgba_ pickPrevColor{};
	xcolor_rgba_ pickCurrColor{};
	float hue = 0.f;
	float sat = 100.f;
	float lum = 50.f;

	_XClr_DragKind dragKind = _XClr_Drag_None;
	BOOL eyedropperActive = FALSE;
	int pickPreviewPressHalf = 0;
	xcolor_rgba_ eyedropperSnapshot{};
	BOOL confirmed = FALSE;
	BOOL closing = FALSE;
	BOOL updatingUI = FALSE;
	BOOL enableAutoClose = TRUE;
	BOOL enableModal = TRUE;
	BOOL enableDrag = FALSE;
	BOOL enableTopmost = FALSE;
	int modalResult = 0;
	_XClr_PopupMode popupMode = _XClr_PopupMode_Default;
	HELE bindEle = NULL;
	int bindOffsetX = 0;
	int bindOffsetY = 0;
	POINT popupPt{};
	int candidateSelectedIndex = -1;
	BOOL candidateSuppressClick = FALSE;

	BOOL comboDropOpen = FALSE;
	HELE hComboDropList = NULL;
	HELE comboDropListStyled = NULL;
	HSVG hSvgClose = NULL;
	HSVG hSvgPip = NULL;
	HSVG hSvgAdd = NULL;
	HSVG hSvgComboDown = NULL;
	HSVG hSvgComboUp = NULL;
	HIMAGE hImgClose = NULL;
	HIMAGE hImgPip = NULL;
	HIMAGE hImgAdd = NULL;
	HIMAGE hImgComboDown = NULL;
	HIMAGE hImgComboUp = NULL;
};

struct _XClr_Global
{
	std::unordered_map<HWINDOW, _XClr_Ctx*> windows;
	std::unordered_map<HELE, _XClr_Ctx*> elements;
	std::unordered_map<HELE, int> commands;
	_XClr_PopupMode popupMode = _XClr_PopupMode_Default;
	HELE bindEle = NULL;
	int bindOffsetX = 0;
	int bindOffsetY = 0;
	POINT popupPt{};
	BOOL enableAutoClose = TRUE;
	BOOL enableModal = TRUE;
	BOOL enableDrag = FALSE;
	BOOL enableTopmost = FALSE;
	XCOLOR_PICKER_PROC_CHANGED onChanged = NULL;
	void* onChangedUser = NULL;
	xcolor_rgba_ candidates[kClr_CandidateMax]{};
	int candidateCount = 0;
	_XClr_Ctx* eyedropperCtx = NULL;
	HHOOK eyedropperMouseHook = NULL;
	HHOOK eyedropperKeyboardHook = NULL;
	HCURSOR eyedropperCrossCursor = NULL;
	HCURSOR eyedropperSavedCursor = NULL;
	LARGE_INTEGER eyedropperLastQpc{};
	BOOL eyedropperLastQpcValid = FALSE;
};

_XClr_Global& ClrG()
{
	static _XClr_Global g;
	return g;
}

inline float _XClr_ClampF(float v, float lo, float hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

inline int _XClr_ClampI(int v, int lo, int hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

inline BYTE _XClr_ClampB(int v)
{
	return (BYTE)_XClr_ClampI(v, 0, 255);
}

void _XClr_RgbToHsl(BYTE r, BYTE g, BYTE b, float* h, float* s, float* l)
{
	float rf = r / 255.f, gf = g / 255.f, bf = b / 255.f;
	float maxc = (std::max)({ rf, gf, bf });
	float minc = (std::min)({ rf, gf, bf });
	float d = maxc - minc;
	*l = (maxc + minc) * 50.f;
	if (d < 1e-5f){
		*h = 0.f;
		*s = 0.f;
		return;
	}
	*s = (*l < 50.f) ? (d / (maxc + minc) * 100.f) : (d / (2.f - maxc - minc) * 100.f);
	if (maxc == rf){
		*h = (gf - bf) / d + (gf < bf ? 6.f : 0.f);
	} else if (maxc == gf){
		*h = (bf - rf) / d + 2.f;
	} else {
		*h = (rf - gf) / d + 4.f;
	}
	*h *= 60.f;
	if (*h >= 360.f) *h -= 360.f;
}

static float _XClr_HueToRgb(float p, float q, float t)
{
	if (t < 0.f) t += 1.f;
	if (t > 1.f) t -= 1.f;
	if (t < 1.f / 6.f) return p + (q - p) * 6.f * t;
	if (t < 1.f / 2.f) return q;
	if (t < 2.f / 3.f) return p + (q - p) * (2.f / 3.f - t) * 6.f;
	return p;
}

void _XClr_HslToRgb(float h, float s, float l, BYTE* r, BYTE* g, BYTE* b)
{
	h = fmodf(h, 360.f);
	if (h < 0.f) h += 360.f;
	s = _XClr_ClampF(s, 0.f, 100.f) / 100.f;
	l = _XClr_ClampF(l, 0.f, 100.f) / 100.f;
	if (s < 1e-5f){
		BYTE v = (BYTE)(l * 255.f + 0.5f);
		*r = *g = *b = v;
		return;
	}
	float q = (l < 0.5f) ? (l * (1.f + s)) : (l + s - l * s);
	float p = 2.f * l - q;
	float hf = h / 360.f;
	*r = (BYTE)(_XClr_HueToRgb(p, q, hf + 1.f / 3.f) * 255.f + 0.5f);
	*g = (BYTE)(_XClr_HueToRgb(p, q, hf) * 255.f + 0.5f);
	*b = (BYTE)(_XClr_HueToRgb(p, q, hf - 1.f / 3.f) * 255.f + 0.5f);
}

COLORREF _XClr_ToColorRef(BYTE r, BYTE g, BYTE b, BYTE a = 255)
{
	return RGBA(r, g, b, a);
}

void _XClr_SyncHslFromRgb(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	_XClr_RgbToHsl(ctx->rgba.r, ctx->rgba.g, ctx->rgba.b, &ctx->hue, &ctx->sat, &ctx->lum);
}

xcolor_rgba_ _XClr_NormalizeInitialColor(xcolor_rgba_ c, BOOL showAlpha)
{
	if (c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0){
		c.r = 255;
		c.g = 0;
		c.b = 0;
		c.a = 255;
	}
	if (!showAlpha && c.a == 0)
		c.a = 255;
	return c;
}

void _XClr_CopyPopupSettingsFromGlobal(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	auto& g = ClrG();
	ctx->enableAutoClose = g.enableAutoClose;
	ctx->enableModal = g.enableModal;
	ctx->enableDrag = g.enableDrag;
	ctx->enableTopmost = g.enableTopmost;
	g.enableAutoClose = TRUE;
	g.enableModal = TRUE;
	g.enableDrag = FALSE;
	g.enableTopmost = FALSE;

	if (g.popupMode == _XClr_PopupMode_Pos){
		ctx->popupMode = _XClr_PopupMode_Pos;
		ctx->popupPt = g.popupPt;
		g.popupMode = g.bindEle ? _XClr_PopupMode_Ele : _XClr_PopupMode_Default;
		return;
	}
	if (g.popupMode == _XClr_PopupMode_Ele || g.bindEle){
		ctx->popupMode = _XClr_PopupMode_Ele;
		ctx->bindEle = g.bindEle;
		ctx->bindOffsetX = g.bindOffsetX;
		ctx->bindOffsetY = g.bindOffsetY;
	}
}

// BkInfo 主题串 (文件作用域 static, 避免函数内超长字面量导致 IDE/旧编译器误解析)
static const wchar_t* const kClr_BkPrimaryDark =
	L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4293878553)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4293947431)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294016057)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(1720205852)23(102)9(4,4,4,4);8:1(16)5(4294967295);8:1(32)5(4294967295);8:1(64)5(4294967295);8:1(2)5(2146299373);}";
static const wchar_t* const kClr_BkPrimaryLight =
	L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4294734118)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4293418527)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4292693275)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(1727820070)23(102)9(4,4,4,4);8:1(16)5(4294967295);8:1(32)5(4294967295);8:1(64)5(4294967295);8:1(2)5(4294967295);}";
static const wchar_t* const kClr_BkNormalDark =
	L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4281413937)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4282006074)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4282598211)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(1513764405)23(90)9(4,4,4,4);8:1(16)5(4292927712);8:1(32)5(4293190368);8:1(64)5(4292532695);8:1(2)5(4285558124);}";
static const wchar_t* const kClr_BkNormalLight =
	L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4294177263)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4293716711)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4293321696)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(3455514099)23(205)9(4,4,4,4);8:1(16)5(4280229663);8:1(32)5(4280229663);8:1(64)5(4280229663);8:1(2)5(4290558905);}";
static const wchar_t* const kClr_BkSubtleDark =
	L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4279505939)23(255)10(1)7(1)11(3)16(1)12(4281545523)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4281084714)23(255)10(1)7(1)11(3)16(1)12(4282137402)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4281742644)23(255)10(1)7(1)11(3)16(1)12(4282597953)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4279505939)23(255)10(1)7(1)11(3)16(1)12(4280163870)13(255)9(4,4,4,4);8:1(16)5(4290032820);8:1(32)5(4294967295);8:1(64)5(4294967295);8:1(2)5(4283979607);}";
static const wchar_t* const kClr_BkSubtleLight =
	L"{99:1.9.9;98:16(0)32(1)64(2)2(3);5:2(15)20(1)21(3)26(1)22(4294769916)23(255)10(1)7(1)11(3)16(1)12(4294111729)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294111986)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294111470)23(255)10(1)7(1)11(3)16(1)12(4293059039)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294967295)23(255)10(1)7(1)11(3)16(1)12(3370904555)13(200)9(4,4,4,4);8:1(16)5(4284243036);8:1(32)5(4280229663);8:1(64)5(4280229663);8:1(2)5(4287532686);}";
static const wchar_t* const kClr_BkInputDark =
	L"{99:1.9.9;98:4(0)20(4)40(1)36(2)2(3);5:2(15)10(1)7(1)11(3)16(1)12(4281545523)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4280032284)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4279505939)23(255)10(1)7(1)11(3)16(1)12(4293878553)13(255)9(4,4,4,4);5:2(15)10(1)7(1)11(3)16(1)12(4280690214)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4279505939)23(255)10(1)7(1)11(3)16(1)12(4288914339)13(255)9(4,4,4,4);}";
static const wchar_t* const kClr_BkInputLight =
	L"{99:1.9.9;98:4(0)20(4)40(1)36(2)2(3);5:2(15)10(1)7(1)11(3)16(1)12(4293651435)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294440951)23(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294967295)23(255)10(1)7(1)11(3)16(1)12(4294734118)13(255)9(4,4,4,4);5:2(15)10(1)7(1)11(3)16(1)12(4293651435)13(255)9(4,4,4,4);5:2(15)20(1)21(3)26(1)22(4294967295)23(255)10(1)7(1)11(3)16(1)12(4279703319)13(255)9(4,4,4,4);}";
static const wchar_t* const kClr_BkSpinLight =
	L"{99:1.9.9;98:16()32(0)64(1)2();5:2(15)20(1)21(3)26(1)22(251658240)23(15)9(2,2,2,2);5:2(15)20(1)21(3)26(1)22(419430400)23(25)9(2,2,2,2);}";
static const wchar_t* const kClr_BkSpinDark =
	L"{99:1.9.9;98:16()32(0)64(1)2();5:2(15)20(1)21(3)26(1)22(268435455)23(15)9(2,2,2,2);5:2(15)20(1)21(3)26(1)22(436207615)23(25)9(2,2,2,2);}";

void _XClr_ResolveTheme(xuitool_theme_ theme, _XClr_ThemeColors* c)
{
	BOOL light = _XUITool::IsLightTheme(theme);

	if (light){
		c->wndBg      = _XUITool::kLightBg;
		c->text       = _XUITool::kLightText;
		c->mutedText  = _XUITool::WithAlpha(_XUITool::kLightText, 150);
		c->border     = _XUITool::WithAlpha(_XUITool::kLightText, 38);
		c->inputBg    = _XUITool::kLightBg;
		c->buttonBg   = _XUITool::kLightBg;
		c->accentText = RGBA(255, 255, 255, 255);
		c->caretColor = _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 204);
		c->bkPrimary  = kClr_BkPrimaryLight;
		c->bkNormal   = kClr_BkNormalLight;
		c->bkSubtle   = kClr_BkSubtleLight;
		c->bkInput    = kClr_BkInputLight;
		c->bkSpin     = kClr_BkSpinLight;
		c->separator  = _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 15);
		return;
	}
	c->wndBg      = _XUITool::kDarkBg;
	c->text       = _XUITool::kDarkText;
	c->mutedText  = _XUITool::WithAlpha(_XUITool::kDarkText, 145);
	c->border     = _XUITool::WithAlpha(_XUITool::kDarkText, 40);
	c->inputBg    = _XUITool::WithAlpha(_XUITool::kDarkText, 10);
	c->buttonBg   = _XUITool::WithAlpha(_XUITool::kDarkText, 16);
	c->accentText = RGBA(255, 255, 255, 255);
	c->caretColor = _XUITool::WithAlpha(RGBA(255, 255, 255, 255), 204);
	c->bkPrimary  = kClr_BkPrimaryDark;
	c->bkNormal   = kClr_BkNormalDark;
	c->bkSubtle   = kClr_BkSubtleDark;
	c->bkInput    = kClr_BkInputDark;
	c->bkSpin     = kClr_BkSpinDark;
	c->separator  = _XUITool::WithAlpha(RGBA(255, 255, 255, 255), 20);
}

void _XClr_StyleButton(HELE hEle, const _XClr_ThemeColors& c, BOOL primary = FALSE, BOOL flatCancel = FALSE)
{
	if (!hEle) return;
	XEle_ClearBkInfo(hEle);
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_SetBkInfo(hEle, primary ? c.bkPrimary : (flatCancel ? c.bkSubtle : c.bkNormal));
	XEle_SetTextColor(hEle, primary ? c.accentText : c.text);
}

void _XClr_InitLayoutWrap(HELE wrap)
{
	if (!wrap || !XC_IsHELE((HXCGUI)wrap)) return;
	XEle_EnableBkTransparent(wrap, TRUE);
	XEle_EnableDrawBorder(wrap, FALSE);
	if (XObj_GetTypeEx((HXCGUI)wrap) == element_type_layout)
		XLayout_ShowLayoutFrame(wrap, FALSE);
	XEle_EnableMouseThrough(wrap, TRUE);
}

BOOL _XClr_IsLayoutEle(HXCGUI hWidget)
{
	return hWidget && XC_IsHELE(hWidget) &&
		XObj_GetTypeEx(hWidget) == element_type_layout;
}

int CALLBACK _XClr_OnInputPaintEnd(HELE hEle, HDRAW hDraw, BOOL* pbHandled);

void _XClr_RegisterInputPaintEvents(HELE hEle)
{
	if (!hEle) return;
	XEle_EnableEvent_XE_PAINT_END(hEle, TRUE);
	XEle_RegEventC1(hEle, XE_PAINT_END, (void*)&_XClr_OnInputPaintEnd);
}

void _XClr_StyleInput(HELE hEle, const _XClr_ThemeColors& c)
{
	if (!hEle) return;
	XEle_ClearBkInfo(hEle);
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_SetFocusBorderColor(hEle, RGBA(0, 0, 0, 0));
	XEle_SetBorderSize(hEle, 4, 0, 4, 0);
	XEle_SetTextColor(hEle, c.text);
	XEdit_SetCaretColor(hEle, c.caretColor);
	XEdit_SetTextAlign(hEle, edit_textAlign_flag_center | edit_textAlign_flag_center_v);
	_XClr_RegisterInputPaintEvents(hEle);
}

void _XClr_StyleHexInput(HELE hEle, const _XClr_ThemeColors& c)
{
	if (!hEle) return;
	XEle_ClearBkInfo(hEle);
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_SetFocusBorderColor(hEle, RGBA(0, 0, 0, 0));
	XEle_SetBorderSize(hEle, kClr_HexPrefixW, 0, 4, 0);
	XEle_SetTextColor(hEle, c.text);
	XEdit_SetCaretColor(hEle, c.caretColor);
	XEdit_SetTextAlign(hEle, edit_textAlign_flag_left | edit_textAlign_flag_center_v);
	_XClr_RegisterInputPaintEvents(hEle);
}

void _XClr_StyleCombo(HELE hEle, const _XClr_ThemeColors& c)
{
	if (!hEle) return;
	XEle_ClearBkInfo(hEle);
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_EnableDrawBorder(hEle, FALSE);
	XEle_SetFocusBorderColor(hEle, RGBA(0, 0, 0, 0));
	XEle_SetBorderSize(hEle, 4, 0, 4, 0);
	XEle_SetTextColor(hEle, c.text);
	XComboBox_EnableEdit(hEle, FALSE);
	_XClr_RegisterInputPaintEvents(hEle);
}

void _XClr_StyleSpinButton(HELE hEle, const _XClr_ThemeColors& c)
{
	if (!hEle) return;
	XEle_ClearBkInfo(hEle);
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_SetBkInfo(hEle, c.bkSpin);
	XEle_SetTextColor(hEle, c.text);
}

void _XClr_SetSeparator(_XClr_Ctx* ctx, int index, int y)
{
	if (!ctx) return;
	if (index == 0) ctx->sepHeaderY = y;
	else if (index == 1) ctx->sepFooterY = y;
}

HXCGUI _XClr_CreateText(_XClr_Ctx* ctx, int x, int y, int w, int h, const wchar_t* text,
	COLORREF color, int align = textAlignFlag_left | textAlignFlag_vcenter, HFONTX hFont = NULL)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HXCGUI t = XShapeText_Create(x, y, w, h, text ? text : L"", ctx->hWnd);
	if (!t) return NULL;
	XUI_EnableCSS(t, FALSE);
	XShapeText_SetTextColor(t, color);
	XShapeText_SetTextAlign(t, align);
	HFONTX useFont = hFont ? hFont : (ctx ? ctx->hFont : NULL);
	if (useFont) XShapeText_SetFont(t, useFont);
	return t;
}

struct _XClr_VertLayout
{
	int headerSepY = 0;
	int mainY = 0;
	int inputY = 0;
	int candidateY = 0;
	int footerSepY = 0;
	int actionY = 0;
};

_XClr_VertLayout _XClr_CalcVertLayout(int svSize)
{
	_XClr_VertLayout l{};
	const int headerY = kClr_PadV;
	l.headerSepY = headerY + kClr_HeaderH + kClr_HeaderSepGap;
	l.mainY = l.headerSepY + 1 + kClr_SepGap;
	l.inputY = l.mainY + svSize + kClr_SepGap;
	l.candidateY = l.inputY + kClr_EditH + kClr_SepGap;
	l.footerSepY = l.candidateY + kClr_SwatchSize + kClr_SepGap;
	l.actionY = l.footerSepY + 1 + kClr_SepGap;
	return l;
}

struct _XClr_PickerLayout
{
	int svSize = kClr_SvSize;
	int barW = kClr_VBarW;
	int barGap = kClr_VBarGap;
	int rowW = kClr_InnerW;
};

_XClr_PickerLayout _XClr_CalcPickerLayout(BOOL showAlpha)
{
	_XClr_PickerLayout pl{};
	const int innerW = kClr_InnerW;
	const int baseRow = kClr_SvSize + kClr_VBarGap + kClr_VBarW
		+ (showAlpha ? (kClr_VBarGap + kClr_VBarW) : 0);
	pl.barGap = (innerW * kClr_VBarGap + baseRow / 2) / baseRow;
	pl.barW = (innerW * kClr_VBarW + baseRow / 2) / baseRow;
	const int gaps = showAlpha ? 2 : 1;
	const int bars = showAlpha ? 2 : 1;
	pl.svSize = innerW - gaps * pl.barGap - bars * pl.barW;
	if (pl.svSize < kClr_SvSize) pl.svSize = kClr_SvSize;
	pl.rowW = innerW;
	return pl;
}

void _XClr_Notify(_XClr_Ctx* ctx, xcolor_change_phase_ phase)
{
	if (!ctx || ctx->closing) return;
	if (phase == xcolor_change_live && !ctx->liveNotify) return;
	auto& g = ClrG();
	if (g.onChanged) g.onChanged(ctx->rgba, phase, g.onChangedUser);
}

void _XClr_SetColor(_XClr_Ctx* ctx, BYTE r, BYTE g, BYTE b, BYTE a, BOOL notifyLive)
{
	if (!ctx || ctx->updatingUI || ctx->closing) return;
	ctx->rgba.r = r;
	ctx->rgba.g = g;
	ctx->rgba.b = b;
	ctx->rgba.a = a;
	_XClr_SyncHslFromRgb(ctx);
	if (notifyLive) _XClr_Notify(ctx, xcolor_change_live);
}

void _XClr_SetFromHsl(_XClr_Ctx* ctx, float h, float s, float l, BYTE a, BOOL notifyLive)
{
	if (!ctx || ctx->updatingUI || ctx->closing) return;
	ctx->hue = h;
	ctx->sat = s;
	ctx->lum = l;
	BYTE r, g, b;
	_XClr_HslToRgb(h, s, l, &r, &g, &b);
	ctx->rgba.r = r;
	ctx->rgba.g = g;
	ctx->rgba.b = b;
	ctx->rgba.a = a;
	if (notifyLive) _XClr_Notify(ctx, xcolor_change_live);
}

void _XClr_InvalidatePickers(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	ctx->pickCurrColor = ctx->rgba;
	if (ctx->hSvPanel) XEle_Redraw(ctx->hSvPanel, FALSE);
	if (ctx->hHueBar) XEle_Redraw(ctx->hHueBar, FALSE);
	if (ctx->hAlphaBar) XEle_Redraw(ctx->hAlphaBar, FALSE);
	if (ctx->hBtnAddCandidate) XEle_Redraw(ctx->hBtnAddCandidate, FALSE);
	if (ctx->hBtnPickPreview) XEle_Redraw(ctx->hBtnPickPreview, FALSE);
	if (ctx->hBtnEyedropper) XEle_Redraw(ctx->hBtnEyedropper, FALSE);
	for (int i = 0; i < kClr_CandidateMax; ++i){
		if (ctx->hCandidateSwatch[i]) XEle_Redraw(ctx->hCandidateSwatch[i], FALSE);
	}
}

void _XClr_RefreshCandidates(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	auto& g = ClrG();
	if (g.candidateCount > kClr_CandidateMax)
		g.candidateCount = kClr_CandidateMax;
	for (int i = 0; i < kClr_CandidateMax; ++i){
		HELE h = ctx->hCandidateSwatch[i];
		if (!h) continue;
		XEle_Enable(h, TRUE);
		XEle_Redraw(h, FALSE);
	}
	if (ctx->hBtnAddCandidate) XEle_Redraw(ctx->hBtnAddCandidate, FALSE);
}

void _XClr_UpdateEdits(_XClr_Ctx* ctx)
{
	if (!ctx || ctx->closing) return;
	BOOL nested = ctx->updatingUI;
	ctx->updatingUI = TRUE;
	wchar_t buf[32]{};
	if (ctx->inputMode == xcolor_input_hex && ctx->hEditHex){
		swprintf_s(buf, L"%02X%02X%02X", ctx->rgba.r, ctx->rgba.g, ctx->rgba.b);
		XEdit_SetText(ctx->hEditHex, buf);
	} else if (ctx->inputMode == xcolor_input_rgb){
		if (ctx->hEditR){
			swprintf_s(buf, L"%d", ctx->rgba.r);
			XEdit_SetText(ctx->hEditR, buf);
		}
		if (ctx->hEditG){
			swprintf_s(buf, L"%d", ctx->rgba.g);
			XEdit_SetText(ctx->hEditG, buf);
		}
		if (ctx->hEditB){
			swprintf_s(buf, L"%d", ctx->rgba.b);
			XEdit_SetText(ctx->hEditB, buf);
		}
	} else if (ctx->inputMode == xcolor_input_hsl){
		if (ctx->hEditH){
			int hi = (int)(ctx->hue + 0.5f);
			if (hi >= 360) hi = 0;
			swprintf_s(buf, L"%d", hi);
			XEdit_SetText(ctx->hEditH, buf);
		}
		if (ctx->hEditS){
			swprintf_s(buf, L"%d", (int)(ctx->sat + 0.5f));
			XEdit_SetText(ctx->hEditS, buf);
		}
		if (ctx->hEditL){
			swprintf_s(buf, L"%d", (int)(ctx->lum + 0.5f));
			XEdit_SetText(ctx->hEditL, buf);
		}
	}
	if (ctx->hEditAlphaPct){
		int pct = (int)(ctx->rgba.a * 100.f / 255.f + 0.5f);
		swprintf_s(buf, L"%d", pct);
		XEdit_SetText(ctx->hEditAlphaPct, buf);
	}
	if (!nested) ctx->updatingUI = FALSE;
}

int _XClr_ComboIndexFromMode(xcolor_input_mode_ mode)
{
	if (mode == xcolor_input_rgb) return 1;
	if (mode == xcolor_input_hsl) return 2;
	return 0;
}

xcolor_input_mode_ _XClr_ModeFromComboIndex(int iItem)
{
	if (iItem == 1) return xcolor_input_rgb;
	if (iItem == 2) return xcolor_input_hsl;
	return xcolor_input_hex;
}

int CALLBACK _XClr_OnEditKeyDown(HELE hEle, int iChar, int, BOOL* pbHandled);
int CALLBACK _XClr_OnEditLButtonDBClick(HELE hEle, UINT, POINT*, BOOL* pbHandled);
int CALLBACK _XClr_OnEditChar(HELE hEle, WPARAM wParam, LPARAM, BOOL* pbHandled);
int CALLBACK _XClr_OnEditChanged(HELE hEle, BOOL* pbHandled);

void _XClr_SetWidgetVisible(HXCGUI hWidget, BOOL visible)
{
	if (!hWidget) return;
	XWidget_Show(hWidget, visible);
	if (!XC_IsHELE(hWidget)) return;
	HELE hEle = (HELE)hWidget;
	XEle_Enable(hEle, visible);
	if (_XClr_IsLayoutEle(hWidget)){
		XEle_EnableMouseThrough(hEle, TRUE);
		return;
	}
	XEle_EnableMouseThrough(hEle, visible ? FALSE : TRUE);
}

void _XClr_AdjustInputRowLayout(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	if (ctx->hModeWrap) XEle_AdjustLayoutEx(ctx->hModeWrap, adjustLayout_all);
	if (ctx->hComboMode) XEle_AdjustLayoutEx(ctx->hComboMode, adjustLayout_all);
}

void _XClr_ApplyInputMode(_XClr_Ctx* ctx)
{
	if (!ctx || ctx->closing) return;
	BOOL hex = (ctx->inputMode == xcolor_input_hex);
	BOOL rgb = (ctx->inputMode == xcolor_input_rgb);
	BOOL hsl = (ctx->inputMode == xcolor_input_hsl);

	ctx->updatingUI = TRUE;
	_XClr_SetWidgetVisible((HXCGUI)ctx->hEditHex, hex);
	_XClr_SetWidgetVisible((HXCGUI)ctx->hRgbWrap, rgb);
	_XClr_SetWidgetVisible((HXCGUI)ctx->hHslWrap, hsl);
	if (hex && ctx->hEditHex)
		XEle_EnableMouseThrough(ctx->hEditHex, FALSE);

	if (ctx->hComboMode)
		XComboBox_SetSelItem(ctx->hComboMode, _XClr_ComboIndexFromMode(ctx->inputMode));

	_XClr_UpdateEdits(ctx);
	_XClr_AdjustInputRowLayout(ctx);
	ctx->updatingUI = FALSE;
}

void _XClr_RefreshAll(_XClr_Ctx* ctx, BOOL notifyLive)
{
	if (!ctx || ctx->closing) return;
	_XClr_UpdateEdits(ctx);
	_XClr_InvalidatePickers(ctx);
	if (notifyLive) _XClr_Notify(ctx, xcolor_change_live);
}

_XClr_Ctx* _XClr_CtxFromEle(HELE hEle)
{
	auto it = ClrG().elements.find(hEle);
	return (it != ClrG().elements.end()) ? it->second : NULL;
}

void _XClr_RegisterEle(_XClr_Ctx* ctx, HELE hEle)
{
	if (ctx && hEle) ClrG().elements[hEle] = ctx;
}

int CALLBACK _XClr_OnPaintWindow(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled);
int CALLBACK _XClr_OnWndMouseMove(HWINDOW hWnd, UINT nFlags, POINT* pPt, BOOL* pbHandled);
int CALLBACK _XClr_OnWndKillFocus(HWINDOW hWnd, BOOL* pbHandled);

void _XClr_DismissComboDrop(_XClr_Ctx* ctx)
{
	if (!ctx || !ctx->comboDropOpen || !ctx->hWnd || !XC_IsHWINDOW((HXCGUI)ctx->hWnd)) return;
	if (ctx->hSvPanel)
		XWnd_SetFocusEle(ctx->hWnd, ctx->hSvPanel);
}

void _XClr_UnregisterCtx(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	auto& g = ClrG();
	for (auto it = g.elements.begin(); it != g.elements.end(); ){
		if (it->second == ctx){
			g.commands.erase(it->first);
			it = g.elements.erase(it);
		} else {
			++it;
		}
	}
	if (ctx->hWnd){
		if (XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
			XWnd_RemoveEventC(ctx->hWnd, WM_PAINT,      (void*)&_XClr_OnPaintWindow);
			XWnd_RemoveEventC(ctx->hWnd, WM_MOUSEMOVE,  (void*)&_XClr_OnWndMouseMove);
			XWnd_RemoveEventC(ctx->hWnd, WM_KILLFOCUS,  (void*)&_XClr_OnWndKillFocus);
		}
		g.windows.erase(ctx->hWnd);
	}
}

HIMAGE _XClr_LoadSvgIcon(const char* svgText, int size, BOOL lightTheme, COLORREF lightColor, HSVG* outSvg)
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

BOOL _XClr_IsLightTheme(const _XClr_Ctx* ctx)
{
	if (!ctx) return TRUE;
	return _XUITool::IsLightTheme(ctx->theme) ? TRUE : FALSE;
}

void _XClr_LoadIcons(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	BOOL light = _XClr_IsLightTheme(ctx);
	COLORREF iconColor = ctx->colors.text;
	COLORREF spinColor = RGBA(0x99, 0x99, 0x99, 0xFF);

	ctx->hImgClose = _XClr_LoadSvgIcon(kClrSvg_Close, kClr_CloseIconSize, light, iconColor, &ctx->hSvgClose);
	ctx->hImgPip = _XClr_LoadSvgIcon(kClrSvg_Pip, 14, light, iconColor, &ctx->hSvgPip);
	ctx->hImgAdd = _XClr_LoadSvgIcon(kClrSvg_Add, 16, light, iconColor, &ctx->hSvgAdd);
	ctx->hImgComboDown = _XClr_LoadSvgIcon(kCalSvg_Down, 8, light, spinColor, &ctx->hSvgComboDown);
	ctx->hImgComboUp = _XClr_LoadSvgIcon(kCalSvg_Up, 8, light, spinColor, &ctx->hSvgComboUp);
}

void _XClr_DestroyIcons(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	if (ctx->hImgClose){ XImage_Release(ctx->hImgClose); ctx->hImgClose = NULL; }
	if (ctx->hSvgClose){ XSvg_Destroy(ctx->hSvgClose); ctx->hSvgClose = NULL; }
	if (ctx->hImgPip){ XImage_Release(ctx->hImgPip); ctx->hImgPip = NULL; }
	if (ctx->hImgAdd){ XImage_Release(ctx->hImgAdd); ctx->hImgAdd = NULL; }
	if (ctx->hImgComboDown){ XImage_Release(ctx->hImgComboDown); ctx->hImgComboDown = NULL; }
	if (ctx->hImgComboUp){ XImage_Release(ctx->hImgComboUp); ctx->hImgComboUp = NULL; }
	if (ctx->hSvgClose){ XSvg_Destroy(ctx->hSvgClose); ctx->hSvgClose = NULL; }
	if (ctx->hSvgPip){ XSvg_Destroy(ctx->hSvgPip); ctx->hSvgPip = NULL; }
	if (ctx->hSvgAdd){ XSvg_Destroy(ctx->hSvgAdd); ctx->hSvgAdd = NULL; }
	if (ctx->hSvgComboDown){ XSvg_Destroy(ctx->hSvgComboDown); ctx->hSvgComboDown = NULL; }
	if (ctx->hSvgComboUp){ XSvg_Destroy(ctx->hSvgComboUp); ctx->hSvgComboUp = NULL; }
}

void _XClr_SetButtonIcon(HELE hBtn, HIMAGE hImage)
{
	if (!hBtn || !hImage) return;
	XBtn_SetText(hBtn, L"");
	XBtn_SetIcon(hBtn, hImage);
	XBtn_SetIconDisable(hBtn, hImage);
	XBtn_SetIconAlign(hBtn, button_icon_align_left);
	XBtn_SetIconSpace(hBtn, 0);
}

void _XClr_DrawCenterImage(HDRAW hDraw, HIMAGE hImg, const RECT& rc)
{
	if (!hDraw || !hImg) return;
	int iw = XImage_GetWidth(hImg);
	int ih = XImage_GetHeight(hImg);
	if (iw <= 0 || ih <= 0) return;
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	int x = rc.left + (w - iw) / 2;
	int y = rc.top + (h - ih) / 2;
	XDraw_DrawImageExAlpha(hDraw, hImg, x, y, iw, ih, 255);
}

void _XClr_ApplyIcons(_XClr_Ctx* ctx)
{
	(void)ctx;
}

void _XClr_FillRectColor(HDRAW hDraw, const RECT& rc, COLORREF color)
{
	if (!hDraw) return;
	XDraw_SetBrushColor(hDraw, color);
	XDraw_FillRect(hDraw, &rc);
}

float _XClr_RoundRectCoverage(int x, int y, int w, int h, float r)
{
	if (w <= 0 || h <= 0) return 0.f;
	float px = (float)x + 0.5f;
	float py = (float)y + 0.5f;
	if (px < 0.f || py < 0.f || px >= (float)w || py >= (float)h) return 0.f;
	if (r <= 0.f) return 1.f;

	float cx = px - (float)w * 0.5f;
	float cy = py - (float)h * 0.5f;
	float bx = (float)w * 0.5f - r;
	float by = (float)h * 0.5f - r;
	float ax = fabsf(cx) - bx;
	float ay = fabsf(cy) - by;
	float qx = fmaxf(ax, 0.f);
	float qy = fmaxf(ay, 0.f);
	float dist = sqrtf(qx * qx + qy * qy);
	float outside = fmaxf(ax, ay);
	float sdf = (outside > 0.f) ? (dist - r) : (fmaxf(ax, ay) - r);
	return _XClr_ClampF(0.5f - sdf, 0.f, 1.f);
}

BOOL _XClr_PointInUniformRoundRect(int x, int y, int w, int h, int r)
{
	return _XClr_RoundRectCoverage(x, y, w, h, (float)r) >= 0.5f;
}

inline BYTE _XClr_ColorR(COLORREF c) { return GetRValue(c); }
inline BYTE _XClr_ColorG(COLORREF c) { return GetGValue(c); }
inline BYTE _XClr_ColorB(COLORREF c) { return GetBValue(c); }
inline BYTE _XClr_ColorA(COLORREF c) { return GetAValue(c); }

COLORREF _XClr_BlendColor(COLORREF fg, COLORREF bg, float t)
{
	if (t <= 0.f) return bg;
	if (t >= 1.f) return fg;
	auto ch = [](BYTE a, BYTE b, float wt) {
		return (BYTE)(a * wt + b * (1.f - wt) + 0.5f);
	};
	return RGBA(
		ch(_XClr_ColorR(fg), _XClr_ColorR(bg), t),
		ch(_XClr_ColorG(fg), _XClr_ColorG(bg), t),
		ch(_XClr_ColorB(fg), _XClr_ColorB(bg), t),
		ch(_XClr_ColorA(fg), _XClr_ColorA(bg), t));
}

// 几何抗锯齿：只混合 RGB，保持 alpha=255，避免 XDraw 二次混合产生色边
COLORREF _XClr_BlendColorRgb(COLORREF fg, COLORREF bg, float t)
{
	if (t <= 0.f) return bg;
	if (t >= 1.f) return fg;
	auto ch = [](BYTE a, BYTE b, float wt) {
		return (BYTE)(a * wt + b * (1.f - wt) + 0.5f);
	};
	return RGBA(
		ch(_XClr_ColorR(fg), _XClr_ColorR(bg), t),
		ch(_XClr_ColorG(fg), _XClr_ColorG(bg), t),
		ch(_XClr_ColorB(fg), _XClr_ColorB(bg), t),
		255);
}

void _XClr_FillPixel(HDRAW hDraw, const RECT& rc, int x, int y, COLORREF color)
{
	RECT px{ rc.left + x, rc.top + y, rc.left + x + 1, rc.top + y + 1 };
	XDraw_SetBrushColor(hDraw, color);
	XDraw_FillRect(hDraw, &px);
}

// 胶囊竖条圆角（50%）：FillRoundRectEx / FillRoundRectGradientRotate 的 4 角半径
void _XClr_PillRoundAngle(int w, int h, RECT& ra)
{
	int r = (w + 1) / 2;
	if (r > h / 2) r = h / 2;
	if (r < 1) r = 1;
	ra.left = r;
	ra.top = r;
	ra.right = r;
	ra.bottom = r;
}

void _XClr_BarThumbRange(const RECT& rc, int h, float& minCy, float& maxCy)
{
	const float edge = (float)kClr_ThumbRadius + (float)kClr_ThumbStroke * 0.5f;
	minCy = (float)rc.top + edge;
	maxCy = (float)rc.top + (float)h - edge;
	if (maxCy < minCy) maxCy = minCy;
}

float _XClr_BarThumbCy(const RECT& rc, int /*w*/, int h, float norm)
{
	float minCy = 0.f, maxCy = 0.f;
	_XClr_BarThumbRange(rc, h, minCy, maxCy);
	return minCy + _XClr_ClampF(norm, 0.f, 1.f) * (maxCy - minCy);
}

float _XClr_BarNormFromY(const RECT& rc, int h, float y)
{
	float minCy = 0.f, maxCy = 0.f;
	_XClr_BarThumbRange(rc, h, minCy, maxCy);
	const float span = maxCy - minCy;
	if (span <= 0.f) return 0.f;
	return _XClr_ClampF((y - minCy) / span, 0.f, 1.f);
}

void _XClr_CheckerCellColors(const _XClr_Ctx* ctx, COLORREF* pLight, COLORREF* pDark)
{
	if (!ctx || !pLight || !pDark) return;
	if (_XUITool::IsLightTheme(ctx->theme)){
		*pLight = RGBA(255, 255, 255, 255);
		*pDark = RGBA(204, 204, 204, 255);
	} else {
		*pLight = RGBA(56, 56, 56, 255);
		*pDark = RGBA(40, 40, 40, 255);
	}
}

void _XClr_DrawCheckerRound(const _XClr_Ctx* ctx, HDRAW hDraw, const RECT& rc, int roundR, COLORREF bg, int cell = 6)
{
	if (!ctx || !hDraw) return;
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0) return;
	COLORREF checkerLight = 0, checkerDark = 0;
	_XClr_CheckerCellColors(ctx, &checkerLight, &checkerDark);
	const float rf = (float)roundR;
	XDraw_EnableSmoothingMode(hDraw, FALSE);
	for (int y = 0; y < h; ++y){
		for (int x = 0; x < w; ++x){
			float cov = _XClr_RoundRectCoverage(x, y, w, h, rf);
			if (cov <= 0.f) continue;
			BOOL darkCell = ((x / cell) + (y / cell)) & 1;
			COLORREF fg = darkCell ? checkerDark : checkerLight;
			_XClr_FillPixel(hDraw, rc, x, y, _XClr_BlendColorRgb(fg, bg, cov));
		}
	}
}

void _XClr_FillRoundRectColor(HDRAW hDraw, const RECT& rc, int roundR, COLORREF fg, COLORREF wndBg,
	const _XClr_Ctx* ctx, int cell = 6)
{
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0) return;
	COLORREF checkerLight = 0, checkerDark = 0;
	if (ctx) _XClr_CheckerCellColors(ctx, &checkerLight, &checkerDark);
	else { checkerLight = RGBA(255, 255, 255, 255); checkerDark = RGBA(204, 204, 204, 255); }
	const float rf = (float)roundR;
	const float fa = _XClr_ColorA(fg) / 255.f;
	XDraw_EnableSmoothingMode(hDraw, FALSE);
	for (int y = 0; y < h; ++y){
		for (int x = 0; x < w; ++x){
			float cov = _XClr_RoundRectCoverage(x, y, w, h, rf);
			if (cov <= 0.f) continue;
			BOOL darkCell = ((x / cell) + (y / cell)) & 1;
			COLORREF checker = darkCell ? checkerDark : checkerLight;
			COLORREF inner = (fa >= 1.f) ? fg : _XClr_BlendColor(fg, checker, fa);
			_XClr_FillPixel(hDraw, rc, x, y, _XClr_BlendColorRgb(inner, wndBg, cov));
		}
	}
}

COLORREF _XClr_SwatchSelectedBorderDefault(BOOL lightTheme)
{
	return lightTheme ? RGBA(0x28, 0x28, 0x28, 255) : RGBA(0xF3, 0xF3, 0xF3, 255);
}

COLORREF _XClr_SwatchSelectedBorderAccent()
{
	return RGBA(0xFF, 0x8D, 0x1A, 255);
}

COLORREF _XClr_EffectiveSwatchRgb(const _XClr_Ctx* ctx, const xcolor_rgba_& color)
{
	if (!ctx) return _XClr_ToColorRef(color.r, color.g, color.b, 255);
	BYTE alpha = ctx->showAlpha ? color.a : (BYTE)255;
	if (alpha >= 255) return _XClr_ToColorRef(color.r, color.g, color.b, 255);
	COLORREF fg = _XClr_ToColorRef(color.r, color.g, color.b, 255);
	float t = alpha / 255.f;
	return _XClr_BlendColorRgb(fg, ctx->colors.wndBg, t);
}

BOOL _XClr_IsVisuallySimilarToBorder(COLORREF swatch, COLORREF border)
{
	const int sr = _XClr_ColorR(swatch), sg = _XClr_ColorG(swatch), sb = _XClr_ColorB(swatch);
	const int lo = (std::min)({ sr, sg, sb });
	const int hi = (std::max)({ sr, sg, sb });
	if (hi - lo > kClr_SwatchNeutralMaxSpread)
		return FALSE;
	const int dr = sr - (int)_XClr_ColorR(border);
	const int dg = sg - (int)_XClr_ColorG(border);
	const int db = sb - (int)_XClr_ColorB(border);
	const int distSq = dr * dr + dg * dg + db * db;
	const int maxDistSq = kClr_SwatchSimilarMaxDist * kClr_SwatchSimilarMaxDist * 3;
	return distSq <= maxDistSq;
}

COLORREF _XClr_SelectedSwatchBorderColor(const _XClr_Ctx* ctx, const xcolor_rgba_& swatch)
{
	if (!ctx) return _XClr_SwatchSelectedBorderDefault(FALSE);
	const BOOL lightTheme = _XUITool::IsLightTheme(ctx->theme);
	COLORREF primary = _XClr_SwatchSelectedBorderDefault(lightTheme);
	COLORREF effective = _XClr_EffectiveSwatchRgb(ctx, swatch);
	if (_XClr_IsVisuallySimilarToBorder(effective, primary))
		return _XClr_SwatchSelectedBorderAccent();
	return primary;
}

void _XClr_DrawSelectedSwatchBorder(_XClr_Ctx* ctx, HDRAW hDraw, const RECT& rc, const xcolor_rgba_& swatch)
{
	if (!ctx || !hDraw) return;
	const int w = kClr_SwatchSelectedBorderWidth;
	const int inset = w / 2;
	RECT stroke = rc;
	stroke.left += inset;
	stroke.top += inset;
	stroke.right -= inset;
	stroke.bottom -= inset;
	int r = kClr_SwatchRound - inset;
	if (r < 1) r = 1;
	XDraw_SetBrushColor(hDraw, _XClr_SelectedSwatchBorderColor(ctx, swatch));
	XDraw_SetLineWidth(hDraw, w);
	XDraw_DrawRoundRectEx(hDraw, &stroke, r, r, r, r);
	XDraw_SetLineWidth(hDraw, 1);
}

void _XClr_SetCandidateSelected(_XClr_Ctx* ctx, int index)
{
	if (!ctx) return;
	if (index < -1 || index >= kClr_CandidateMax) return;
	int old = ctx->candidateSelectedIndex;
	if (old == index) return;
	ctx->candidateSelectedIndex = index;
	if (old >= 0 && old < kClr_CandidateMax && ctx->hCandidateSwatch[old])
		XEle_Redraw(ctx->hCandidateSwatch[old], FALSE);
	if (index >= 0 && index < kClr_CandidateMax && ctx->hCandidateSwatch[index])
		XEle_Redraw(ctx->hCandidateSwatch[index], FALSE);
}
COLORREF _XClr_SoftBorderColor(const _XClr_Ctx* ctx)
{
	if (!ctx) return RGBA(128, 128, 128, 80);
	BOOL light = _XUITool::IsLightTheme(ctx->theme);
	return light ? _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 42)
	             : _XUITool::WithAlpha(RGBA(255, 255, 255, 255), 48);
}

void _XClr_DrawSoftRoundBorder(HDRAW hDraw, const RECT& rc, int r, COLORREF color)
{
	if (!hDraw || r <= 0) return;
	RECT stroke = rc;
	XDraw_SetBrushColor(hDraw, color);
	XDraw_SetLineWidth(hDraw, 1);
	XDraw_DrawRoundRect(hDraw, &stroke, r, r);
}

void _XClr_DrawSoftRoundBorderEx(HDRAW hDraw, const RECT& rc, int lt, int rt, int rb, int lb, COLORREF color)
{
	if (!hDraw) return;
	XDraw_SetBrushColor(hDraw, color);
	XDraw_SetLineWidth(hDraw, 1);
	XDraw_DrawRoundRectEx(hDraw, &rc, lt, rt, rb, lb);
}

struct _XClr_InputChromeState
{
	BOOL focused = FALSE;
	BOOL hover = FALSE;
};

_XClr_InputChromeState _XClr_GetInputChromeState(_XClr_Ctx* ctx, HELE hEle)
{
	_XClr_InputChromeState st{};
	if (!hEle) return st;
	st.focused = XEle_IsFocus(hEle);
	const int flags = XEle_GetStateFlags(hEle);
	st.hover = (flags & element_state_flag_stay) != 0;
	if (ctx && hEle == ctx->hComboMode && ctx->comboDropOpen)
		st.focused = TRUE;
	return st;
}

COLORREF _XClr_InputBorderColor(const _XClr_Ctx* ctx, const _XClr_InputChromeState& st)
{
	if (!ctx) return 0;
	BYTE alpha = kClr_InputBorderBlurA;
	if (st.hover) alpha = kClr_InputBorderHoverA;
	if (st.focused) alpha = kClr_InputBorderFocusA;
	if (_XClr_IsLightTheme(ctx))
		return _XUITool::WithAlpha(RGBA(0, 0, 0, 255), alpha);
	return _XUITool::WithAlpha(RGBA(255, 255, 255, 255), alpha);
}

COLORREF _XClr_InputFillColor(const _XClr_Ctx* ctx)
{
	if (!ctx) return 0;
	if (_XClr_IsLightTheme(ctx))
		return _XUITool::WithAlpha(RGBA(0, 0, 0, 255), kClr_InputFillFocusA);
	return _XUITool::WithAlpha(RGBA(255, 255, 255, 255), kClr_InputFillFocusA);
}

BOOL _XClr_IsUnderlineEdit(_XClr_Ctx* ctx, HELE hEle)
{
	if (!ctx || !hEle) return FALSE;
	return hEle == ctx->hEditHex || hEle == ctx->hEditAlphaPct ||
		hEle == ctx->hEditR || hEle == ctx->hEditG || hEle == ctx->hEditB ||
		hEle == ctx->hEditH || hEle == ctx->hEditS || hEle == ctx->hEditL;
}

void _XClr_PaintEditUnderline(_XClr_Ctx* ctx, HDRAW hDraw, const RECT& rc, HELE hEle)
{
	if (!ctx || !hDraw) return;
	const _XClr_InputChromeState st = _XClr_GetInputChromeState(ctx, hEle);
	RECT lineRc = rc;
	lineRc.top = lineRc.bottom - 1;
	XDraw_SetBrushColor(hDraw, _XClr_InputBorderColor(ctx, st));
	XDraw_FillRect(hDraw, &lineRc);
}

// 与透明度竖条相同：D2D FillRoundRectGradientRotate 做圆角裁剪/抗锯齿，避免 SDF 逐像素叠色产生杂线
void _XClr_FillRoundRectGradientUniform(HDRAW hDraw, const RECT& rc, const RECT& roundAngle, COLORREF color)
{
	if (!hDraw) return;
	gradient_info_ gi;
	gi.pArray = new gradient_point_[2];
	gi.nCount = 2;
	gi.fAngle = 0.f;
	gi.pArray[0].color = color;
	gi.pArray[0].nPos = 0;
	gi.pArray[1].color = color;
	gi.pArray[1].nPos = 100;
	XDraw_EnableSmoothingMode(hDraw, TRUE);
	XDraw_FillRoundRectGradientRotate(hDraw, &rc, &gi, &roundAngle, 0.f);
}

void _XClr_PaintComboChrome(_XClr_Ctx* ctx, HDRAW hDraw, const RECT& rc, HELE hEle)
{
	if (!ctx || !hDraw) return;
	const _XClr_InputChromeState st = _XClr_GetInputChromeState(ctx, hEle);
	const COLORREF border = _XClr_InputBorderColor(ctx, st);
	if (st.focused){
		XDraw_EnableSmoothingMode(hDraw, TRUE);
		XDraw_SetBrushColor(hDraw, _XClr_InputFillColor(ctx));
		XDraw_FillRoundRect(hDraw, &rc, kClr_InputComboRound, kClr_InputComboRound);
	}
	_XClr_DrawSoftRoundBorder(hDraw, rc, kClr_InputComboRound, border);
}

void _XClr_ComboDropButtonRect(const RECT& rcClient, RECT& btnRc)
{
	btnRc = rcClient;
	btnRc.left = btnRc.right - 20;
	if (btnRc.left < btnRc.right) return;
	btnRc.left = btnRc.right - 1;
}

int CALLBACK _XClr_OnComboDropDrawItem(HELE hEle, HDRAW hDraw, listBox_item_* pItem, BOOL* pbHandled);
int CALLBACK _XClr_OnComboDropTempCreateEnd(HELE hEle, listBox_item_* pItem, int nFlag, BOOL* pbHandled);
int CALLBACK _XClr_OnComboDropPaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled);

void _XClr_StyleComboDropItemText(_XClr_Ctx* ctx, HELE hListBox, listBox_item_* pItem)
{
	if (!ctx || !hListBox || !pItem) return;

	HELE hLayout = (HELE)XListBox_GetTemplateObject(hListBox, pItem->index, 0);
	if (!hLayout) return;
	XEle_SetPadding(hLayout, kClr_ComboTextPadL, 0, kClr_ComboTextPadL, 0);

	HELE hText = (HELE)XEle_GetChildByIndex(hLayout, 0);
	if (!hText || XC_GetObjectType(hText) != XC_SHAPE_TEXT) return;

	if (ctx->hFontCombo) XShapeText_SetFont(hText, ctx->hFontCombo);
	XShapeText_SetTextAlign(hText, textAlignFlag_left | textAlignFlag_vcenter);
	XShapeText_SetTextColor(hText, ctx->colors.text);
}

void _XClr_StyleComboDropParentChain(HELE hListBox)
{
	for (HELE h = hListBox; h && XC_IsHELE((HXCGUI)h); ){
		XUI_EnableCSS((HXCGUI)h, FALSE);
		XEle_ClearBkInfo(h);
		XEle_EnableBkTransparent(h, TRUE);
		XEle_EnableDrawBorder(h, FALSE);
		HXCGUI hParent = XWidget_GetParent((HXCGUI)h);
		if (!hParent || !XC_IsHELE(hParent)) break;
		h = (HELE)hParent;
	}
}

void _XClr_StyleComboDropPopup(_XClr_Ctx* ctx, HWINDOW hDropWnd, HELE hListBox)
{
	if (!ctx || !hDropWnd || !hListBox) return;

	XUI_EnableCSS((HXCGUI)hDropWnd, FALSE);
	XWnd_ClearBkInfo(hDropWnd);
	XWnd_EnableDrawBk(hDropWnd, FALSE);
	XWnd_SetTransparentType(hDropWnd, window_transparent_shaped);
	XWnd_SetTransparentAlpha(hDropWnd, 255);
	XWnd_SetShadowInfo(hDropWnd, 10, 255, kClr_ComboDropRound, FALSE,
		_XClr_IsLightTheme(ctx) ? _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 30)
		                        : _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 80));

	const BOOL firstListStyled = (ctx->comboDropListStyled != hListBox);
	if (firstListStyled){
		XUI_EnableCSS((HXCGUI)hListBox, FALSE);
		_XClr_StyleComboDropParentChain(hListBox);
		XEle_EnableDrawFocus(hListBox, FALSE);
		XListBox_EnableFixedRowHeight(hListBox, TRUE);
		XListBox_SetDrawItemBkFlags(hListBox, list_drawItemBk_flag_nothing);
		XListBox_SetItemHeightDefault(hListBox, kClr_EditH, kClr_EditH);
		XListBox_SetRowSpace(hListBox, 0);
		XListBox_EnableMultiSel(hListBox, FALSE);
		XListBox_SetSplitLineColor(hListBox, RGBA(0, 0, 0, 0));
		XSView_EnableAutoShowScrollBar(hListBox, FALSE);
		XSView_ShowSBarV(hListBox, FALSE);
		XSView_ShowSBarH(hListBox, FALSE);
		XListBox_EnableTemplateReuse(hListBox, TRUE);
		XEle_RegEventC1(hListBox, XE_PAINT, (void*)&_XClr_OnComboDropPaint);
		XEle_RegEventC1(hListBox, XE_LISTBOX_DRAWITEM, (void*)&_XClr_OnComboDropDrawItem);
		XEle_RegEventC1(hListBox, XE_LISTBOX_TEMP_CREATE_END, (void*)&_XClr_OnComboDropTempCreateEnd);
		ctx->comboDropListStyled = hListBox;
	}

	_XClr_RegisterEle(ctx, hListBox);
	ctx->hComboDropList = hListBox;
	XEle_Redraw(hListBox, FALSE);
}

int CALLBACK _XClr_OnComboDropPaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || !hDraw || ctx->closing || hEle != ctx->hComboDropList) return 0;

	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	if (rc.right <= rc.left || rc.bottom <= rc.top) return 0;

	_XClr_InputChromeState st = _XClr_GetInputChromeState(ctx, ctx->hComboMode);
	XDraw_EnableSmoothingMode(hDraw, TRUE);
	XDraw_SetBrushColor(hDraw, ctx->colors.wndBg);
	XDraw_FillRoundRect(hDraw, &rc, kClr_ComboDropRound, kClr_ComboDropRound);
	_XClr_DrawSoftRoundBorder(hDraw, rc, kClr_ComboDropRound,
		_XClr_InputBorderColor(ctx, st));
	return 0;
}

int CALLBACK _XClr_OnComboDropDrawItem(HELE hEle, HDRAW hDraw, listBox_item_* pItem, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || !hDraw || !pItem || ctx->closing || hEle != ctx->hComboDropList) return 0;

	RECT rc = pItem->rcItem;
	if (rc.right <= rc.left || rc.bottom <= rc.top) return 0;

	BOOL selected = (pItem->nState == list_item_state_select);
	BOOL hover = (pItem->nState == list_item_state_stay);
	if (selected || hover){
		COLORREF fill = _XClr_InputFillColor(ctx);
		if (selected)
			fill = _XClr_IsLightTheme(ctx)
				? _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 24)
				: _XUITool::WithAlpha(RGBA(255, 255, 255, 255), 24);
		XDraw_SetBrushColor(hDraw, fill);
		XDraw_FillRect(hDraw, &rc);
	}
	return 0;
}

int CALLBACK _XClr_OnComboDropTempCreateEnd(HELE hEle, listBox_item_* pItem, int nFlag, BOOL* pbHandled)
{
	(void)nFlag;
	if (pbHandled) *pbHandled = FALSE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing || hEle != ctx->hComboDropList || !pItem) return 0;
	_XClr_StyleComboDropItemText(ctx, hEle, pItem);
	return 0;
}

void _XClr_SwatchRoundAngle(RECT& ra)
{
	ra.left = kClr_SwatchRound;
	ra.top = kClr_SwatchRound;
	ra.right = kClr_SwatchRound;
	ra.bottom = kClr_SwatchRound;
}

void _XClr_PaintSelectedSwatchFill(_XClr_Ctx* ctx, HDRAW hDraw, const RECT& rc, const xcolor_rgba_& color)
{
	if (!ctx || !hDraw) return;
	const int bgRound = kClr_SwatchRound + kClr_SwatchSelectedRoundInset;
	RECT bgRoundAngle{ bgRound, bgRound, bgRound, bgRound };
	_XClr_FillRoundRectGradientUniform(hDraw, rc, bgRoundAngle, ctx->colors.wndBg);

	const int pad = kClr_SwatchSelectedRoundInset;
	RECT fillRc = rc;
	fillRc.left += pad;
	fillRc.top += pad;
	fillRc.right -= pad;
	fillRc.bottom -= pad;
	if (fillRc.right <= fillRc.left || fillRc.bottom <= fillRc.top) return;

	RECT fillRound{ kClr_SwatchRound, kClr_SwatchRound, kClr_SwatchRound, kClr_SwatchRound };
	BYTE alpha = ctx->showAlpha ? color.a : (BYTE)255;
	if (ctx->showAlpha && color.a < 255){
		_XClr_DrawCheckerRound(ctx, hDraw, fillRc, kClr_SwatchRound, ctx->colors.wndBg);
		_XClr_FillRoundRectGradientUniform(hDraw, fillRc, fillRound,
			_XClr_ToColorRef(color.r, color.g, color.b, color.a));
	} else {
		_XClr_FillRoundRectGradientUniform(hDraw, fillRc, fillRound,
			_XClr_ToColorRef(color.r, color.g, color.b, alpha));
	}
}

void _XClr_PaintColorBlock(_XClr_Ctx* ctx, HDRAW hDraw, const RECT& rc, const xcolor_rgba_& color,
	int lt, int rt, int rb, int lb, BOOL drawBorder = TRUE)
{
	if (!ctx || !hDraw) return;
	_XClr_FillRectColor(hDraw, rc, ctx->colors.wndBg);
	RECT roundAngle{ lt, rt, rb, lb };
	int checkerR = (std::max)({ lt, rt, rb, lb });
	BYTE alpha = ctx->showAlpha ? color.a : (BYTE)255;
	if (ctx->showAlpha && color.a < 255){
		if (checkerR > 0)
			_XClr_DrawCheckerRound(ctx, hDraw, rc, checkerR, ctx->colors.wndBg);
		_XClr_FillRoundRectGradientUniform(hDraw, rc, roundAngle,
			_XClr_ToColorRef(color.r, color.g, color.b, color.a));
	} else {
		_XClr_FillRoundRectGradientUniform(hDraw, rc, roundAngle,
			_XClr_ToColorRef(color.r, color.g, color.b, alpha));
	}
	if (drawBorder && (lt || rt || rb || lb))
		_XClr_DrawSoftRoundBorderEx(hDraw, rc, lt, rt, rb, lb, _XClr_SoftBorderColor(ctx));
}

void _XClr_PaintEyedropper(_XClr_Ctx* ctx, HELE hEle, HDRAW hDraw)
{
	if (!ctx || !hEle || !hDraw) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	RECT roundAngle{};
	_XClr_SwatchRoundAngle(roundAngle);
	_XClr_FillRoundRectGradientUniform(hDraw, rc, roundAngle, ctx->colors.wndBg);
	if (ctx->hImgPip)
		_XClr_DrawCenterImage(hDraw, ctx->hImgPip, rc);
	_XClr_DrawSoftRoundBorderEx(hDraw, rc, kClr_SwatchRound, kClr_SwatchRound, kClr_SwatchRound,
		kClr_SwatchRound, _XClr_SoftBorderColor(ctx));
}

void _XClr_PaintPickPreview(_XClr_Ctx* ctx, HELE hEle, HDRAW hDraw)
{
	if (!ctx || !hEle || !hDraw) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	const int h = rc.bottom - rc.top;
	if (h <= 0) return;
	const int halfH = h / 2;
	RECT rcTop = rc;
	rcTop.bottom = rc.top + halfH;
	RECT rcBot = rc;
	rcBot.top = rc.top + halfH;

	_XClr_PaintColorBlock(ctx, hDraw, rcTop, ctx->pickPrevColor,
		kClr_SwatchRound, kClr_SwatchRound, 0, 0, FALSE);
	_XClr_PaintColorBlock(ctx, hDraw, rcBot, ctx->pickCurrColor,
		0, 0, kClr_SwatchRound, kClr_SwatchRound, FALSE);
	_XClr_DrawSoftRoundBorder(hDraw, rc, kClr_SwatchRound, _XClr_SoftBorderColor(ctx));
}

COLORREF _XClr_CloseHoverFill(const _XClr_Ctx* ctx, BOOL down)
{
	if (!ctx) return 0;
	const BYTE alpha = down ? (BYTE)kClr_CloseDownA : (BYTE)kClr_CloseHoverA;
	if (_XClr_IsLightTheme(ctx))
		return _XUITool::WithAlpha(RGBA(0, 0, 0, 255), alpha);
	return _XUITool::WithAlpha(RGBA(255, 255, 255, 255), alpha);
}

void _XClr_PaintCloseButton(_XClr_Ctx* ctx, HELE hEle, HDRAW hDraw)
{
	if (!ctx || !hEle || !hDraw) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	button_state_ st = XBtn_GetStateEx(hEle);
	if (st != button_state_leave){
		const BOOL down = (st == button_state_down);
		RECT roundAngle{};
		roundAngle.left = roundAngle.right = roundAngle.top = roundAngle.bottom = 6;
		_XClr_FillRoundRectGradientUniform(hDraw, rc, roundAngle, _XClr_CloseHoverFill(ctx, down));
	}
	if (ctx->hImgClose) _XClr_DrawCenterImage(hDraw, ctx->hImgClose, rc);
}

void _XClr_DrawThumb(HDRAW hDraw, int cx, int cy)
{
	if (!hDraw) return;
	const int r = kClr_ThumbRadius;
	const int stroke = kClr_ThumbStroke;

	XDraw_EnableSmoothingMode(hDraw, TRUE);

	RECT shadowOuter{ cx - r - 1, cy - r - 1, cx + r + 2, cy + r + 2 };
	XDraw_SetBrushColor(hDraw, _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 36));
	XDraw_FillEllipse(hDraw, &shadowOuter);

	RECT shadowInner{ cx - r, cy - r, cx + r + 1, cy + r + 1 };
	XDraw_SetBrushColor(hDraw, _XUITool::WithAlpha(RGBA(0, 0, 0, 255), 20));
	XDraw_FillEllipse(hDraw, &shadowInner);

	RECT ring{ cx - r, cy - r, cx + r, cy + r };
	XDraw_SetBrushColor(hDraw, RGBA(255, 255, 255, 255));
	XDraw_SetLineWidth(hDraw, stroke);
	XDraw_DrawEllipse(hDraw, &ring);
}

void _XClr_PaintSv(_XClr_Ctx* ctx, HELE hEle, HDRAW hDraw)
{
	if (!ctx || !hEle || !hDraw) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0) return;

	XDraw_EnableSmoothingMode(hDraw, FALSE);
	_XClr_FillRectColor(hDraw, rc, ctx->colors.wndBg);
	const float roundR = (float)kClr_SvRound;
	for (int y = 0; y < h; ++y){
		float l = (1.f - (y + 0.5f) / (float)h) * 100.f;
		for (int x = 0; x < w; ++x){
			float cov = _XClr_RoundRectCoverage(x, y, w, h, roundR);
			if (cov <= 0.f) continue;
			float s = (x + 0.5f) / (float)w * 100.f;
			BYTE r, g, b;
			_XClr_HslToRgb(ctx->hue, s, l, &r, &g, &b);
			COLORREF fg = _XClr_ToColorRef(r, g, b);
			_XClr_FillPixel(hDraw, rc, x, y, _XClr_BlendColorRgb(fg, ctx->colors.wndBg, cov));
		}
	}

	int tx = rc.left + (int)(ctx->sat / 100.f * w + 0.5f);
	int ty = rc.top + (int)((1.f - ctx->lum / 100.f) * h + 0.5f);
	tx = _XClr_ClampI(tx, rc.left, rc.right - 1);
	ty = _XClr_ClampI(ty, rc.top, rc.bottom - 1);
	XDraw_EnableSmoothingMode(hDraw, TRUE);
	_XClr_DrawThumb(hDraw, tx, ty);
}

void _XClr_PaintHue(_XClr_Ctx* ctx, HELE hEle, HDRAW hDraw)
{
	if (!ctx || !hEle || !hDraw) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 1) return;

	RECT roundAngle{};
	_XClr_PillRoundAngle(w, h, roundAngle);

	constexpr int kHueStops = 37;
	gradient_info_ gi;
	gi.pArray = new gradient_point_[kHueStops];
	gi.nCount = kHueStops;
	gi.fAngle = 90.f;
	for (int i = 0; i < kHueStops; ++i){
		float hue = i / (float)(kHueStops - 1) * 360.f;
		BYTE r, g, b;
		_XClr_HslToRgb(hue, 100.f, 50.f, &r, &g, &b);
		gi.pArray[i].color = RGBA(r, g, b, 255);
		gi.pArray[i].nPos = (int)(i * 100.f / (kHueStops - 1) + 0.5f);
	}

	XDraw_EnableSmoothingMode(hDraw, TRUE);
	XDraw_FillRoundRectGradientRotate(hDraw, &rc, &gi, &roundAngle, 0.f);

	float cy = _XClr_BarThumbCy(rc, w, h, ctx->hue / 360.f);
	int cx = (rc.left + rc.right) / 2;
	_XClr_DrawThumb(hDraw, cx, (int)(cy + 0.5f));
}

void _XClr_PaintAlpha(_XClr_Ctx* ctx, HELE hEle, HDRAW hDraw)
{
	if (!ctx || !hEle || !hDraw) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 1) return;

	RECT roundAngle{};
	_XClr_PillRoundAngle(w, h, roundAngle);

	_XClr_DrawCheckerRound(ctx, hDraw, rc, roundAngle.left, ctx->colors.wndBg, 4);

	gradient_info_ gi;
	gi.pArray = new gradient_point_[2];
	gi.nCount = 2;
	gi.fAngle = 90.f;
	gi.pArray[0].color = RGBA(ctx->rgba.r, ctx->rgba.g, ctx->rgba.b, 0);
	gi.pArray[0].nPos = 0;
	gi.pArray[1].color = RGBA(ctx->rgba.r, ctx->rgba.g, ctx->rgba.b, 255);
	gi.pArray[1].nPos = 100;
	XDraw_EnableSmoothingMode(hDraw, TRUE);
	XDraw_FillRoundRectGradientRotate(hDraw, &rc, &gi, &roundAngle, 0.f);

	float cy = _XClr_BarThumbCy(rc, w, h, ctx->rgba.a / 255.f);
	int cx = (rc.left + rc.right) / 2;
	_XClr_DrawThumb(hDraw, cx, (int)(cy + 0.5f));
}

void _XClr_PaintSwatch(_XClr_Ctx* ctx, HELE hEle, HDRAW hDraw, int index)
{
	if (!ctx || !hEle || !hDraw || index < 0 || index >= kClr_CandidateMax) return;
	auto& g = ClrG();
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	if (index < g.candidateCount){
		const BOOL selected = (index == ctx->candidateSelectedIndex);
		if (selected){
			_XClr_PaintSelectedSwatchFill(ctx, hDraw, rc, g.candidates[index]);
			_XClr_DrawSelectedSwatchBorder(ctx, hDraw, rc, g.candidates[index]);
		} else {
			_XClr_PaintColorBlock(ctx, hDraw, rc, g.candidates[index],
				kClr_SwatchRound, kClr_SwatchRound, kClr_SwatchRound, kClr_SwatchRound, TRUE);
		}
	} else {
		RECT roundAngle{};
		_XClr_SwatchRoundAngle(roundAngle);
		_XClr_FillRoundRectGradientUniform(hDraw, rc, roundAngle, ctx->colors.wndBg);
		if (ctx->showAlpha)
			_XClr_DrawCheckerRound(ctx, hDraw, rc, kClr_SwatchRound, ctx->colors.wndBg);
		_XClr_DrawSoftRoundBorderEx(hDraw, rc, kClr_SwatchRound, kClr_SwatchRound, kClr_SwatchRound,
			kClr_SwatchRound, _XClr_SoftBorderColor(ctx));
	}
}

void _XClr_PaintAddSlot(_XClr_Ctx* ctx, HELE hEle, HDRAW hDraw)
{
	if (!ctx || !hEle || !hDraw) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	RECT roundAngle{};
	_XClr_SwatchRoundAngle(roundAngle);
	_XClr_FillRoundRectGradientUniform(hDraw, rc, roundAngle, ctx->colors.wndBg);
	if (ctx->hImgAdd)
		_XClr_DrawCenterImage(hDraw, ctx->hImgAdd, rc);
	else {
		int cx = (rc.left + rc.right) / 2;
		int cy = (rc.top + rc.bottom) / 2;
		XDraw_SetBrushColor(hDraw, ctx->colors.mutedText);
		RECT hLine{ cx - 6, cy, cx + 6, cy + 1 };
		RECT vLine{ cx, cy - 6, cx + 1, cy + 6 };
		XDraw_FillRect(hDraw, &hLine);
		XDraw_FillRect(hDraw, &vLine);
	}
	_XClr_DrawSoftRoundBorderEx(hDraw, rc, kClr_SwatchRound, kClr_SwatchRound, kClr_SwatchRound,
		kClr_SwatchRound, _XClr_SoftBorderColor(ctx));
}

void _XClr_UpdateFromSv(_XClr_Ctx* ctx, HELE hEle, const POINT& pt)
{
	if (!ctx || !hEle) return;
	int w = XEle_GetWidth(hEle);
	int h = XEle_GetHeight(hEle);
	if (w <= 0 || h <= 0) return;
	float s = _XClr_ClampF(pt.x / (float)w * 100.f, 0.f, 100.f);
	float l = _XClr_ClampF((1.f - pt.y / (float)h) * 100.f, 0.f, 100.f);
	_XClr_SetFromHsl(ctx, ctx->hue, s, l, ctx->rgba.a, TRUE);
	_XClr_UpdateEdits(ctx);
	_XClr_InvalidatePickers(ctx);
}

void _XClr_UpdateFromHue(_XClr_Ctx* ctx, HELE hEle, const POINT& pt)
{
	if (!ctx || !hEle) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	int h = rc.bottom - rc.top;
	if (h <= 0) return;
	float norm = _XClr_BarNormFromY(rc, h, (float)pt.y);
	float hue = norm * 360.f;
	_XClr_SetFromHsl(ctx, hue, ctx->sat, ctx->lum, ctx->rgba.a, TRUE);
	_XClr_UpdateEdits(ctx);
	_XClr_InvalidatePickers(ctx);
}

void _XClr_UpdateFromAlpha(_XClr_Ctx* ctx, HELE hEle, const POINT& pt)
{
	if (!ctx || !hEle) return;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	int h = rc.bottom - rc.top;
	if (h <= 0) return;
	float t = _XClr_BarNormFromY(rc, h, (float)pt.y);
	BYTE a = (BYTE)(t * 255.f + 0.5f);
	_XClr_SetColor(ctx, ctx->rgba.r, ctx->rgba.g, ctx->rgba.b, a, TRUE);
	_XClr_UpdateEdits(ctx);
	_XClr_InvalidatePickers(ctx);
}

HELE _XClr_DragTargetEle(_XClr_Ctx* ctx)
{
	if (!ctx) return NULL;
	if (ctx->dragKind == _XClr_Drag_Sv) return ctx->hSvPanel;
	if (ctx->dragKind == _XClr_Drag_Hue) return ctx->hHueBar;
	if (ctx->dragKind == _XClr_Drag_Alpha) return ctx->hAlphaBar;
	return NULL;
}

void _XClr_ApplyDragAt(_XClr_Ctx* ctx, HELE hEle, POINT pt)
{
	if (!ctx || !hEle) return;
	if (ctx->dragKind == _XClr_Drag_Sv) _XClr_UpdateFromSv(ctx, hEle, pt);
	else if (ctx->dragKind == _XClr_Drag_Hue) _XClr_UpdateFromHue(ctx, hEle, pt);
	else if (ctx->dragKind == _XClr_Drag_Alpha) _XClr_UpdateFromAlpha(ctx, hEle, pt);
}

int CALLBACK _XClr_OnBtnClick(HELE hEle, BOOL* pbHandled);
int CALLBACK _XClr_OnEyedropperLDown(HELE hEle, UINT, POINT*, BOOL* pbHandled);
int CALLBACK _XClr_OnPickPreviewLDown(HELE hEle, UINT, POINT* pPt, BOOL* pbHandled);
int CALLBACK _XClr_OnPickPreviewLUp(HELE hEle, UINT, POINT*, BOOL* pbHandled);
int CALLBACK _XClr_OnClosePaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled);
int CALLBACK _XClr_OnCloseMouseStay(HELE hEle, BOOL* pbHandled);
int CALLBACK _XClr_OnCloseMouseLeave(HELE hEle, BOOL* pbHandled);
int CALLBACK _XClr_OnPickerPaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled);
int CALLBACK _XClr_OnPickerLDown(HELE hEle, UINT, POINT* pPt, BOOL* pbHandled);
int CALLBACK _XClr_OnPickerMove(HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled);
int CALLBACK _XClr_OnPickerLUp(HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled);
int CALLBACK _XClr_OnPickerKillCapture(HELE hEle, BOOL* pbHandled);
int CALLBACK _XClr_OnSwatchClick(HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled);
int CALLBACK _XClr_OnSwatchDblClick(HELE hEle, UINT, POINT*, BOOL* pbHandled);
int CALLBACK _XClr_OnSwatchRButtonUp(HELE hEle, UINT, POINT*, BOOL* pbHandled);
int CALLBACK _XClr_OnAddSlotClick(HELE hEle, UINT, POINT*, BOOL* pbHandled);
int CALLBACK _XClr_OnWndMouseMove(HWINDOW hWnd, UINT nFlags, POINT* pPt, BOOL* pbHandled);
int CALLBACK _XClr_OnEditKeyDown(HELE hEle, int iChar, int, BOOL* pbHandled);
int CALLBACK _XClr_OnEditLButtonDBClick(HELE hEle, UINT, POINT*, BOOL* pbHandled);
int CALLBACK _XClr_OnEditChar(HELE hEle, WPARAM wParam, LPARAM, BOOL* pbHandled);
int CALLBACK _XClr_OnEditChanged(HELE hEle, BOOL* pbHandled);

void _XClr_EndModal(_XClr_Ctx* ctx, int result)
{
	if (!ctx || ctx->closing) return;
	_XClr_DismissComboDrop(ctx);
	ctx->closing = TRUE;
	ctx->modalResult = result;
	if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
		if (ctx->enableModal)
			XModalWnd_EndModal(ctx->hWnd, result);
		else
			XWnd_ShowWindow(ctx->hWnd, SW_HIDE);
	}
}

int _XClr_RunPopupLoop(_XClr_Ctx* ctx)
{
	if (!ctx) return kClr_ResultCancel;
	if (ctx->enableModal)
		return XModalWnd_DoModal(ctx->hWnd);

	ctx->modalResult = kClr_ResultCancel;
	ctx->closing = FALSE;
	while (!ctx->closing){
		MSG msg{};
		while (::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)){
			if (msg.message == WM_QUIT){
				::PostQuitMessage((int)msg.wParam);
				ctx->closing = TRUE;
				break;
			}
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
		if (!ctx->closing)
			::WaitMessage();
	}
	return ctx->modalResult;
}

BOOL _XClr_ParseHexString(const wchar_t* txt, BYTE* r, BYTE* g, BYTE* b, BYTE* a, BOOL allowAlpha)
{
	if (!txt || !r || !g || !b) return FALSE;
	wchar_t hex[12]{};
	int n = 0;
	for (const wchar_t* p = txt; *p && n < 8; ++p){
		wchar_t c = *p;
		if (c == L'#') continue;
		if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F'))
			hex[n++] = c;
	}
	if (n != 3 && n != 6 && n != 8) return FALSE;

	wchar_t full[12]{};
	if (n == 3){
		for (int i = 0; i < 3; ++i){
			full[i * 2] = hex[i];
			full[i * 2 + 1] = hex[i];
		}
		n = 6;
	} else {
		for (int i = 0; i < n; ++i) full[i] = hex[i];
	}
	if (n < 6){
		while (n < 6) full[n++] = L'0';
	}

	unsigned ur = 0, ug = 0, ub = 0;
	if (n >= 8 && a && allowAlpha){
		unsigned uaa = 0;
		if (swscanf_s(full, L"%2x%2x%2x%2x", &uaa, &ur, &ug, &ub) != 4) return FALSE;
		*a = (BYTE)uaa;
		*r = (BYTE)ur;
		*g = (BYTE)ug;
		*b = (BYTE)ub;
		return TRUE;
	}
	if (swscanf_s(full, L"%2x%2x%2x", &ur, &ug, &ub) != 3) return FALSE;
	*r = (BYTE)ur;
	*g = (BYTE)ug;
	*b = (BYTE)ub;
	if (a) *a = 255;
	return TRUE;
}

BOOL _XClr_IsModeColorEdit(_XClr_Ctx* ctx, HELE hEdit);

void _XClr_ParseHexInput(_XClr_Ctx* ctx, BOOL refreshUI)
{
	if (!ctx || !ctx->hEditHex || ctx->updatingUI) return;
	const wchar_t* txt = XEdit_GetText_Temp(ctx->hEditHex);
	BYTE r = 0, g = 0, b = 0;
	if (!_XClr_ParseHexString(txt, &r, &g, &b, NULL, FALSE)) return;
	_XClr_SetColor(ctx, r, g, b, ctx->rgba.a, FALSE);
	if (refreshUI) _XClr_RefreshAll(ctx, TRUE);
}

void _XClr_ParseRgbInput(_XClr_Ctx* ctx, BOOL refreshUI)
{
	if (!ctx || ctx->updatingUI || !ctx->hEditR || !ctx->hEditG || !ctx->hEditB) return;
	int r = _XClr_ClampI(_wtoi(XEdit_GetText_Temp(ctx->hEditR)), 0, 255);
	int g = _XClr_ClampI(_wtoi(XEdit_GetText_Temp(ctx->hEditG)), 0, 255);
	int b = _XClr_ClampI(_wtoi(XEdit_GetText_Temp(ctx->hEditB)), 0, 255);
	_XClr_SetColor(ctx, (BYTE)r, (BYTE)g, (BYTE)b, ctx->rgba.a, FALSE);
	if (refreshUI) _XClr_RefreshAll(ctx, TRUE);
}

void _XClr_ParseHslInput(_XClr_Ctx* ctx, BOOL refreshUI)
{
	if (!ctx || ctx->updatingUI || !ctx->hEditH || !ctx->hEditS || !ctx->hEditL) return;
	float h = (float)_wtoi(XEdit_GetText_Temp(ctx->hEditH));
	h = fmodf(h, 360.f);
	if (h < 0.f) h += 360.f;
	float s = _XClr_ClampF((float)_wtoi(XEdit_GetText_Temp(ctx->hEditS)), 0.f, 100.f);
	float l = _XClr_ClampF((float)_wtoi(XEdit_GetText_Temp(ctx->hEditL)), 0.f, 100.f);
	_XClr_SetFromHsl(ctx, h, s, l, ctx->rgba.a, FALSE);
	if (refreshUI) _XClr_RefreshAll(ctx, TRUE);
}

void _XClr_ParseAlphaPctInput(_XClr_Ctx* ctx, BOOL refreshUI)
{
	if (!ctx || !ctx->hEditAlphaPct || ctx->updatingUI) return;
	const wchar_t* txt = XEdit_GetText_Temp(ctx->hEditAlphaPct);
	int pct = _XClr_ClampI(_wtoi(txt ? txt : L"100"), 0, 100);
	BYTE a = (BYTE)(pct * 255 / 100.0 + 0.5);
	_XClr_SetColor(ctx, ctx->rgba.r, ctx->rgba.g, ctx->rgba.b, a, FALSE);
	if (refreshUI) _XClr_RefreshAll(ctx, TRUE);
}

void _XClr_SyncColorFromEdits(_XClr_Ctx* ctx)
{
	if (!ctx || ctx->updatingUI || ctx->closing) return;
	if (ctx->inputMode == xcolor_input_hex && ctx->hEditHex)
		_XClr_ParseHexInput(ctx, FALSE);
	else if (ctx->inputMode == xcolor_input_rgb)
		_XClr_ParseRgbInput(ctx, FALSE);
	else if (ctx->inputMode == xcolor_input_hsl)
		_XClr_ParseHslInput(ctx, FALSE);
	if (ctx->showAlpha && ctx->hEditAlphaPct)
		_XClr_ParseAlphaPctInput(ctx, FALSE);
}

void _XClr_ParseEditInput(_XClr_Ctx* ctx, HELE hEle, BOOL refreshUI)
{
	if (!ctx || !hEle || ctx->updatingUI || ctx->closing) return;
	if (hEle == ctx->hEditAlphaPct){
		if (ctx->showAlpha) _XClr_ParseAlphaPctInput(ctx, refreshUI);
		return;
	}
	if (!_XClr_IsModeColorEdit(ctx, hEle)) return;
	if (hEle == ctx->hEditHex) _XClr_ParseHexInput(ctx, refreshUI);
	else if (hEle == ctx->hEditR || hEle == ctx->hEditG || hEle == ctx->hEditB)
		_XClr_ParseRgbInput(ctx, refreshUI);
	else if (hEle == ctx->hEditH || hEle == ctx->hEditS || hEle == ctx->hEditL)
		_XClr_ParseHslInput(ctx, refreshUI);
}

void _XClr_CommitPendingEdits(_XClr_Ctx* ctx)
{
	if (!ctx || ctx->updatingUI || ctx->closing) return;
	_XClr_SyncColorFromEdits(ctx);
	_XClr_RefreshAll(ctx, TRUE);
}

BOOL _XClr_CopyTextToClipboard(const wchar_t* text)
{
	if (!text || !*text) return FALSE;
	if (!::OpenClipboard(NULL)) return FALSE;
	::EmptyClipboard();
	size_t bytes = (wcslen(text) + 1) * sizeof(wchar_t);
	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (!hMem){
		::CloseClipboard();
		return FALSE;
	}
	wchar_t* dst = (wchar_t*)::GlobalLock(hMem);
	if (!dst){
		::GlobalFree(hMem);
		::CloseClipboard();
		return FALSE;
	}
	wcscpy_s(dst, bytes / sizeof(wchar_t), text);
	::GlobalUnlock(hMem);
	::SetClipboardData(CF_UNICODETEXT, hMem);
	::CloseClipboard();
	return TRUE;
}

BOOL _XClr_IsModeColorEdit(_XClr_Ctx* ctx, HELE hEdit)
{
	if (!ctx || !hEdit) return FALSE;
	if (ctx->inputMode == xcolor_input_hex) return hEdit == ctx->hEditHex;
	if (ctx->inputMode == xcolor_input_rgb)
		return hEdit == ctx->hEditR || hEdit == ctx->hEditG || hEdit == ctx->hEditB;
	if (ctx->inputMode == xcolor_input_hsl)
		return hEdit == ctx->hEditH || hEdit == ctx->hEditS || hEdit == ctx->hEditL;
	return FALSE;
}

void _XClr_FormatCopyColorText(_XClr_Ctx* ctx, wchar_t* buf, size_t bufCount)
{
	if (!ctx || !buf || bufCount == 0) return;
	if (ctx->inputMode == xcolor_input_hex){
		XClr_FormatHexText(ctx->rgba, ctx->showAlpha, buf, bufCount);
	} else if (ctx->inputMode == xcolor_input_rgb){
		swprintf_s(buf, bufCount, L"%d,%d,%d,%d",
			ctx->rgba.r, ctx->rgba.g, ctx->rgba.b, ctx->rgba.a);
	} else if (ctx->inputMode == xcolor_input_hsl){
		int hi = (int)(ctx->hue + 0.5f);
		if (hi >= 360) hi = 0;
		swprintf_s(buf, bufCount, L"%d,%d,%d,%d",
			hi, (int)(ctx->sat + 0.5f), (int)(ctx->lum + 0.5f), ctx->rgba.a);
	}
}

void _XClr_RegisterModeEditEvents(HELE edit)
{
	if (!edit) return;
	XEle_RegEventC1(edit, XE_KEYDOWN, (void*)&_XClr_OnEditKeyDown);
	XEle_RegEventC1(edit, XE_LBUTTONDBCLICK, (void*)&_XClr_OnEditLButtonDBClick);
	XEle_RegEventC1(edit, XE_CHAR, (void*)&_XClr_OnEditChar);
	XEle_RegEventC1(edit, XE_EDIT_CHANGED, (void*)&_XClr_OnEditChanged);
}

BOOL _XClr_IsHexDigit(wchar_t c)
{
	return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
}

BOOL _XClr_IsDigitChar(wchar_t c)
{
	return c >= L'0' && c <= L'9';
}

BOOL _XClr_EditWouldExceedMaxLen(HELE hEdit, int maxLen)
{
	if (!hEdit || maxLen <= 0) return TRUE;
	int len = XEdit_GetLength(hEdit);
	int selLen = XEdit_GetSelectTextLength(hEdit);
	if (selLen > 0) return (len - selLen) >= maxLen;
	return len >= maxLen;
}

int _XClr_EditMaxLen(_XClr_Ctx* ctx, HELE hEdit)
{
	if (!ctx || !hEdit) return 0;
	if (hEdit == ctx->hEditHex) return kClr_HexMaxLen;
	if (hEdit == ctx->hEditAlphaPct) return kClr_AlphaPctMaxLen;
	if (hEdit == ctx->hEditR || hEdit == ctx->hEditG || hEdit == ctx->hEditB ||
		hEdit == ctx->hEditH || hEdit == ctx->hEditS || hEdit == ctx->hEditL){
		return kClr_ChannelMaxLen;
	}
	return 0;
}

BOOL _XClr_SanitizeEditText(_XClr_Ctx* ctx, HELE hEdit, wchar_t* buf, size_t bufCount)
{
	if (!ctx || !hEdit || !buf || bufCount == 0) return FALSE;
	const wchar_t* src = buf;
	wchar_t out[16]{};
	int n = 0;
	int maxLen = _XClr_EditMaxLen(ctx, hEdit);
	if (maxLen <= 0 || maxLen >= (int)_countof(out)) return FALSE;

	if (hEdit == ctx->hEditHex){
		for (; *src && n < maxLen; ++src){
			if (*src == L'#') continue;
			if (_XClr_IsHexDigit(*src))
				out[n++] = (wchar_t)towupper(*src);
		}
	} else {
		for (; *src && n < maxLen; ++src){
			if (_XClr_IsDigitChar(*src))
				out[n++] = *src;
		}
	}
	out[n] = L'\0';
	if (wcscmp(buf, out) == 0) return FALSE;
	wcscpy_s(buf, bufCount, out);
	return TRUE;
}

BOOL _XClr_EditValueRange(_XClr_Ctx* ctx, HELE hEdit, int* outMin, int* outMax)
{
	if (!ctx || !hEdit || !outMin || !outMax) return FALSE;
	*outMin = 0;
	if (hEdit == ctx->hEditR || hEdit == ctx->hEditG || hEdit == ctx->hEditB){
		*outMax = 255;
		return TRUE;
	}
	if (hEdit == ctx->hEditH){
		*outMax = 359;
		return TRUE;
	}
	if (hEdit == ctx->hEditS || hEdit == ctx->hEditL || hEdit == ctx->hEditAlphaPct){
		*outMax = 100;
		return TRUE;
	}
	return FALSE;
}

BOOL _XClr_BuildEditPreviewWithChar(HELE hEdit, wchar_t c, wchar_t* out, size_t outCount)
{
	if (!hEdit || !out || outCount == 0) return FALSE;
	const wchar_t* txt = XEdit_GetText_Temp(hEdit);
	if (!txt) txt = L"";
	int len = (int)wcslen(txt);
	int pos = XEdit_GetCurPos(hEdit);
	int selLen = XEdit_GetSelectTextLength(hEdit);
	int start = pos;
	int end = pos + selLen;
	if (start < 0) start = 0;
	if (start > len) start = len;
	if (end < start) end = start;
	if (end > len) end = len;

	int outN = 0;
	for (int i = 0; i < start && outN + 1 < (int)outCount; ++i)
		out[outN++] = txt[i];
	if (outN + 1 >= (int)outCount) return FALSE;
	out[outN++] = c;
	for (int i = end; txt[i] && outN + 1 < (int)outCount; ++i)
		out[outN++] = txt[i];
	out[outN] = L'\0';
	return TRUE;
}

BOOL _XClr_PreviewCharExceedsRange(_XClr_Ctx* ctx, HELE hEdit, wchar_t c)
{
	int minV = 0, maxV = 0;
	if (!_XClr_EditValueRange(ctx, hEdit, &minV, &maxV)) return FALSE;
	wchar_t preview[16]{};
	if (!_XClr_BuildEditPreviewWithChar(hEdit, c, preview, _countof(preview))) return TRUE;
	if (!preview[0]) return FALSE;
	int v = _wtoi(preview);
	return v > maxV;
}

BOOL _XClr_ClampEditValueText(_XClr_Ctx* ctx, HELE hEdit, wchar_t* buf, size_t bufCount)
{
	if (!ctx || !hEdit || !buf || bufCount == 0 || !buf[0]) return FALSE;
	int minV = 0, maxV = 0;
	if (!_XClr_EditValueRange(ctx, hEdit, &minV, &maxV)) return FALSE;
	int v = _wtoi(buf);
	int clamped = _XClr_ClampI(v, minV, maxV);
	if (v == clamped) return FALSE;
	swprintf_s(buf, bufCount, L"%d", clamped);
	return TRUE;
}

BOOL _XClr_IsEditCharAllowed(_XClr_Ctx* ctx, HELE hEdit, wchar_t c)
{
	if (!ctx || !hEdit) return FALSE;
	if (hEdit == ctx->hEditHex){
		if (c == L'#') return FALSE;
		return _XClr_IsHexDigit(c);
	}
	if (hEdit == ctx->hEditAlphaPct || hEdit == ctx->hEditR || hEdit == ctx->hEditG ||
		hEdit == ctx->hEditB || hEdit == ctx->hEditH || hEdit == ctx->hEditS || hEdit == ctx->hEditL){
		return _XClr_IsDigitChar(c);
	}
	return TRUE;
}

int _XClr_HexChannelFromPos(int pos)
{
	if (pos <= 1) return 0;
	if (pos <= 3) return 1;
	return 2;
}

void _XClr_SelectHexChannel(HELE hEdit, int channel)
{
	if (!hEdit || channel < 0 || channel > 2) return;
	int s = channel * 2;
	XEdit_SetSelect(hEdit, 0, s, 0, s + 2);
}

void _XClr_EditSpin(_XClr_Ctx* ctx, HELE hEdit, int delta)
{
	if (!ctx || !hEdit || ctx->updatingUI || ctx->closing) return;
	if (hEdit != ctx->hEditAlphaPct && !_XClr_IsModeColorEdit(ctx, hEdit)) return;

	if (hEdit == ctx->hEditHex){
		int channel = _XClr_HexChannelFromPos(XEdit_GetCurPos(hEdit));
		BYTE r = ctx->rgba.r, g = ctx->rgba.g, b = ctx->rgba.b;
		if (channel == 0) r = _XClr_ClampB((int)r + delta);
		else if (channel == 1) g = _XClr_ClampB((int)g + delta);
		else b = _XClr_ClampB((int)b + delta);
		_XClr_SetColor(ctx, r, g, b, ctx->rgba.a, TRUE);
		_XClr_UpdateEdits(ctx);
		_XClr_InvalidatePickers(ctx);
		_XClr_SelectHexChannel(hEdit, channel);
		return;
	}
	if (hEdit == ctx->hEditR){
		int v = _XClr_ClampI(_wtoi(XEdit_GetText_Temp(hEdit)) + delta, 0, 255);
		_XClr_SetColor(ctx, (BYTE)v, ctx->rgba.g, ctx->rgba.b, ctx->rgba.a, TRUE);
		_XClr_UpdateEdits(ctx);
		_XClr_InvalidatePickers(ctx);
		return;
	}
	if (hEdit == ctx->hEditG){
		int v = _XClr_ClampI(_wtoi(XEdit_GetText_Temp(hEdit)) + delta, 0, 255);
		_XClr_SetColor(ctx, ctx->rgba.r, (BYTE)v, ctx->rgba.b, ctx->rgba.a, TRUE);
		_XClr_UpdateEdits(ctx);
		_XClr_InvalidatePickers(ctx);
		return;
	}
	if (hEdit == ctx->hEditB){
		int v = _XClr_ClampI(_wtoi(XEdit_GetText_Temp(hEdit)) + delta, 0, 255);
		_XClr_SetColor(ctx, ctx->rgba.r, ctx->rgba.g, (BYTE)v, ctx->rgba.a, TRUE);
		_XClr_UpdateEdits(ctx);
		_XClr_InvalidatePickers(ctx);
		return;
	}
	if (hEdit == ctx->hEditH){
		int hi = (int)(ctx->hue + 0.5f);
		hi = ((hi + delta) % 360 + 360) % 360;
		_XClr_SetFromHsl(ctx, (float)hi, ctx->sat, ctx->lum, ctx->rgba.a, TRUE);
		_XClr_UpdateEdits(ctx);
		_XClr_InvalidatePickers(ctx);
		return;
	}
	if (hEdit == ctx->hEditS){
		float s = _XClr_ClampF(ctx->sat + (float)delta, 0.f, 100.f);
		_XClr_SetFromHsl(ctx, ctx->hue, s, ctx->lum, ctx->rgba.a, TRUE);
		_XClr_UpdateEdits(ctx);
		_XClr_InvalidatePickers(ctx);
		return;
	}
	if (hEdit == ctx->hEditL){
		float l = _XClr_ClampF(ctx->lum + (float)delta, 0.f, 100.f);
		_XClr_SetFromHsl(ctx, ctx->hue, ctx->sat, l, ctx->rgba.a, TRUE);
		_XClr_UpdateEdits(ctx);
		_XClr_InvalidatePickers(ctx);
		return;
	}
	if (hEdit == ctx->hEditAlphaPct){
		int pct = _XClr_ClampI(_wtoi(XEdit_GetText_Temp(hEdit)) + delta, 0, 100);
		BYTE a = (BYTE)(pct * 255 / 100.0 + 0.5);
		_XClr_SetColor(ctx, ctx->rgba.r, ctx->rgba.g, ctx->rgba.b, a, TRUE);
		_XClr_UpdateEdits(ctx);
		_XClr_InvalidatePickers(ctx);
	}
}

void _XClr_AddCandidate(_XClr_Ctx* ctx)
{
	if (!ctx || ctx->closing) return;
	auto& g = ClrG();
	if (ctx->candidateSelectedIndex >= 0 && ctx->candidateSelectedIndex < g.candidateCount){
		g.candidates[ctx->candidateSelectedIndex] = ctx->rgba;
		_XClr_RefreshCandidates(ctx);
		return;
	}
	if (g.candidateCount < kClr_CandidateMax){
		g.candidates[g.candidateCount++] = ctx->rgba;
	} else {
		g.candidates[kClr_CandidateMax - 1] = ctx->rgba;
	}
	_XClr_RefreshCandidates(ctx);
}

void _XClr_ReplaceSelectedCandidate(_XClr_Ctx* ctx)
{
	if (!ctx || ctx->closing) return;
	auto& g = ClrG();
	int idx = ctx->candidateSelectedIndex;
	if (idx < 0 || idx >= g.candidateCount) return;
	g.candidates[idx] = ctx->rgba;
	_XClr_RefreshCandidates(ctx);
}

void _XClr_ClearCandidate(_XClr_Ctx* ctx, int index)
{
	if (!ctx || ctx->closing || index < 0 || index >= kClr_CandidateMax) return;
	auto& g = ClrG();
	if (index >= g.candidateCount) return;
	for (int j = index; j < g.candidateCount - 1; ++j)
		g.candidates[j] = g.candidates[j + 1];
	g.candidateCount--;
	if (ctx->candidateSelectedIndex == index)
		ctx->candidateSelectedIndex = -1;
	else if (ctx->candidateSelectedIndex > index)
		ctx->candidateSelectedIndex--;
	_XClr_RefreshCandidates(ctx);
}

void _XClr_ApplyPickColor(_XClr_Ctx* ctx, const xcolor_rgba_& color)
{
	if (!ctx || ctx->closing) return;
	ctx->rgba = color;
	_XClr_SyncHslFromRgb(ctx);
	_XClr_RefreshAll(ctx, TRUE);
}

void _XClr_UpdatePickPreviewOnCommit(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	ctx->pickPrevColor = ctx->eyedropperSnapshot;
	ctx->pickCurrColor = ctx->rgba;
	if (ctx->hBtnPickPreview) XEle_Redraw(ctx->hBtnPickPreview, FALSE);
}

int _XClr_SwatchIndexFromEle(_XClr_Ctx* ctx, HELE hEle)
{
	if (!ctx || !hEle) return -1;
	for (int i = 0; i < kClr_CandidateMax; ++i){
		if (ctx->hCandidateSwatch[i] == hEle) return i;
	}
	return -1;
}

LRESULT CALLBACK _XClr_EyedropperMouseHook(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK _XClr_EyedropperKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);

void _XClr_EyedropperSetCrossCursor()
{
	auto& g = ClrG();
	if (!g.eyedropperCrossCursor)
		g.eyedropperCrossCursor = LoadCursorW(NULL, IDC_CROSS);
	if (g.eyedropperCrossCursor)
		SetCursor(g.eyedropperCrossCursor);
}

void _XClr_EyedropperInstallCrossCursor()
{
	auto& g = ClrG();
	if (!g.eyedropperCrossCursor)
		g.eyedropperCrossCursor = LoadCursorW(NULL, IDC_CROSS);
	if (!g.eyedropperCrossCursor) return;
	HCURSOR hCopy = (HCURSOR)CopyImage(g.eyedropperCrossCursor, IMAGE_CURSOR, 0, 0, LR_COPYFROMRESOURCE);
	if (hCopy)
		SetSystemCursor(hCopy, OCR_NORMAL);
	_XClr_EyedropperSetCrossCursor();
}

void _XClr_EyedropperRestoreSystemCursor()
{
	SystemParametersInfoW(SPI_SETCURSORS, 0, NULL, 0);
}

void _XClr_UnhookEyedropper()
{
	auto& g = ClrG();
	if (g.eyedropperMouseHook){
		UnhookWindowsHookEx(g.eyedropperMouseHook);
		g.eyedropperMouseHook = NULL;
	}
	if (g.eyedropperKeyboardHook){
		UnhookWindowsHookEx(g.eyedropperKeyboardHook);
		g.eyedropperKeyboardHook = NULL;
	}
}

void _XClr_StopEyedropper(BOOL restoreSnapshot)
{
	auto& g = ClrG();
	_XClr_Ctx* ctx = g.eyedropperCtx;
	_XClr_UnhookEyedropper();
	_XClr_EyedropperRestoreSystemCursor();
	if (g.eyedropperSavedCursor){
		SetCursor(g.eyedropperSavedCursor);
		g.eyedropperSavedCursor = NULL;
	}
	g.eyedropperLastQpcValid = FALSE;
	if (ctx){
		if (restoreSnapshot && !ctx->closing){
			ctx->rgba = ctx->eyedropperSnapshot;
			_XClr_SyncHslFromRgb(ctx);
			_XClr_RefreshAll(ctx, FALSE);
		}
		ctx->eyedropperActive = FALSE;
		if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
			XWnd_ShowWindow(ctx->hWnd, SW_SHOWNOACTIVATE);
			XModalWnd_EnableAutoClose(ctx->hWnd, ctx->enableAutoClose);
			XModalWnd_EnableEscClose(ctx->hWnd, TRUE);
		}
	}
	g.eyedropperCtx = NULL;
}

BOOL _XClr_SampleScreenColor(POINT pt, BYTE* r, BYTE* g, BYTE* b)
{
	if (!r || !g || !b) return FALSE;
	HDC hdc = GetDC(NULL);
	if (!hdc) return FALSE;
	COLORREF c = GetPixel(hdc, pt.x, pt.y);
	ReleaseDC(NULL, hdc);
	if (c == CLR_INVALID) return FALSE;
	*r = GetRValue(c);
	*g = GetGValue(c);
	*b = GetBValue(c);
	return TRUE;
}

void _XClr_ApplyEyedropperSample(_XClr_Ctx* ctx, POINT pt, BOOL notifyLive)
{
	if (!ctx || ctx->closing) return;
	BYTE r = 0, g = 0, b = 0;
	if (!_XClr_SampleScreenColor(pt, &r, &g, &b)) return;
	BYTE a = ctx->showAlpha ? ctx->rgba.a : 255;
	_XClr_SetColor(ctx, r, g, b, a, notifyLive && ctx->liveNotify);
	_XClr_UpdateEdits(ctx);
	_XClr_InvalidatePickers(ctx);
}

void _XClr_StartEyedropper(_XClr_Ctx* ctx)
{
	if (!ctx || ctx->closing || ctx->eyedropperActive) return;
	auto& g = ClrG();
	if (g.eyedropperCtx) _XClr_StopEyedropper(FALSE);

	ctx->eyedropperSnapshot = ctx->rgba;
	ctx->eyedropperActive = TRUE;
	g.eyedropperCtx = ctx;
	g.eyedropperLastQpcValid = FALSE;
	g.eyedropperSavedCursor = GetCursor();

	// 隐藏窗口前必须先关模态「失焦自动关闭」，否则 SW_HIDE 会立刻结束 DoModal
	if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
		XModalWnd_EnableAutoClose(ctx->hWnd, FALSE);
		XModalWnd_EnableEscClose(ctx->hWnd, FALSE);
	}

	HMODULE hMod = GetModuleHandleW(NULL);
	g.eyedropperMouseHook = SetWindowsHookExW(
		WH_MOUSE_LL, _XClr_EyedropperMouseHook, hMod, 0);
	g.eyedropperKeyboardHook = SetWindowsHookExW(
		WH_KEYBOARD_LL, _XClr_EyedropperKeyboardHook, hMod, 0);
	if (!g.eyedropperMouseHook || !g.eyedropperKeyboardHook){
		if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
			XModalWnd_EnableAutoClose(ctx->hWnd, ctx->enableAutoClose);
			XModalWnd_EnableEscClose(ctx->hWnd, TRUE);
		}
		ctx->eyedropperActive = FALSE;
		g.eyedropperCtx = NULL;
		_XClr_UnhookEyedropper();
		return;
	}

	_XClr_EyedropperInstallCrossCursor();
	if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd))
		XWnd_ShowWindow(ctx->hWnd, SW_HIDE);

	POINT pt{};
	GetCursorPos(&pt);
	_XClr_ApplyEyedropperSample(ctx, pt, TRUE);
	_XClr_EyedropperInstallCrossCursor();
}

void _XClr_CommitEyedropper(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	_XClr_UpdatePickPreviewOnCommit(ctx);
	_XClr_ReplaceSelectedCandidate(ctx);
	_XClr_Notify(ctx, xcolor_change_commit);
	_XClr_StopEyedropper(FALSE);
}

void _XClr_CancelEyedropper(_XClr_Ctx* ctx)
{
	if (!ctx) return;
	_XClr_StopEyedropper(TRUE);
}

LRESULT CALLBACK _XClr_EyedropperMouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	auto& g = ClrG();
	if (nCode < 0)
		return CallNextHookEx(g.eyedropperMouseHook, nCode, wParam, lParam);

	_XClr_Ctx* ctx = g.eyedropperCtx;
	if (!ctx || !ctx->eyedropperActive || ctx->closing)
		return CallNextHookEx(g.eyedropperMouseHook, nCode, wParam, lParam);

	_XClr_EyedropperSetCrossCursor();

	const MSLLHOOKSTRUCT* ms = (const MSLLHOOKSTRUCT*)lParam;
	if (wParam == WM_MOUSEMOVE){
		LARGE_INTEGER now{};
		QueryPerformanceCounter(&now);
		if (g.eyedropperLastQpcValid){
			LARGE_INTEGER freq{};
			QueryPerformanceFrequency(&freq);
			double msElapsed = (double)(now.QuadPart - g.eyedropperLastQpc.QuadPart) * 1000.0 / (double)freq.QuadPart;
			if (msElapsed < 16.0)
				return CallNextHookEx(g.eyedropperMouseHook, nCode, wParam, lParam);
		}
		g.eyedropperLastQpc = now;
		g.eyedropperLastQpcValid = TRUE;
		_XClr_ApplyEyedropperSample(ctx, ms->pt, TRUE);
		if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)){
			_XClr_ApplyEyedropperSample(ctx, ms->pt, FALSE);
			_XClr_CommitEyedropper(ctx);
			return 1;
		}
		return CallNextHookEx(g.eyedropperMouseHook, nCode, wParam, lParam);
	}

	if (wParam == WM_LBUTTONUP){
		_XClr_ApplyEyedropperSample(ctx, ms->pt, FALSE);
		_XClr_CommitEyedropper(ctx);
		return 1;
	}
	return CallNextHookEx(g.eyedropperMouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK _XClr_EyedropperKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	auto& g = ClrG();
	if (nCode < 0)
		return CallNextHookEx(g.eyedropperKeyboardHook, nCode, wParam, lParam);

	_XClr_Ctx* ctx = g.eyedropperCtx;
	if (!ctx || !ctx->eyedropperActive || ctx->closing)
		return CallNextHookEx(g.eyedropperKeyboardHook, nCode, wParam, lParam);

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN){
		const KBDLLHOOKSTRUCT* kb = (const KBDLLHOOKSTRUCT*)lParam;
		if (kb && kb->vkCode == VK_ESCAPE){
			_XClr_CancelEyedropper(ctx);
			return 1;
		}
	}
	return CallNextHookEx(g.eyedropperKeyboardHook, nCode, wParam, lParam);
}

void _XClr_HandleCmd(_XClr_Ctx* ctx, _XClr_Cmd cmd)
{
	if (!ctx || ctx->closing) return;
	switch (cmd){
	case _XClr_Cmd_Close:
	case _XClr_Cmd_Cancel:
		_XClr_EndModal(ctx, kClr_ResultCancel);
		break;
	case _XClr_Cmd_Confirm:
		_XClr_CommitPendingEdits(ctx);
		ctx->resultColor = ctx->rgba;
		ctx->confirmed = TRUE;
		_XClr_Notify(ctx, xcolor_change_commit);
		_XClr_EndModal(ctx, kClr_ResultOk);
		break;
	case _XClr_Cmd_AddCandidate:
		_XClr_AddCandidate(ctx);
		break;
	case _XClr_Cmd_PickPrev:
		_XClr_ApplyPickColor(ctx, ctx->pickPrevColor);
		break;
	case _XClr_Cmd_PickCurr:
		_XClr_ApplyPickColor(ctx, ctx->pickCurrColor);
		break;
	default:
		break;
	}
}

int CALLBACK _XClr_OnBtnClick(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing) return 0;
	auto it = ClrG().commands.find(hEle);
	if (it == ClrG().commands.end()) return 0;
	_XClr_HandleCmd(ctx, (_XClr_Cmd)it->second);
	return 0;
}

int CALLBACK _XClr_OnEyedropperLDown(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing || hEle != ctx->hBtnEyedropper) return 0;
	_XClr_StartEyedropper(ctx);
	return 0;
}

int CALLBACK _XClr_OnPickPreviewLDown(HELE hEle, UINT, POINT* pPt, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing || hEle != ctx->hBtnPickPreview || !pPt) return 0;
	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	const int halfH = (rc.bottom - rc.top) / 2;
	ctx->pickPreviewPressHalf = (pPt->y < rc.top + halfH) ? 1 : 2;
	return 0;
}

int CALLBACK _XClr_OnPickPreviewLUp(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing || hEle != ctx->hBtnPickPreview || ctx->eyedropperActive) return 0;
	if (ctx->pickPreviewPressHalf == 1)
		_XClr_ApplyPickColor(ctx, ctx->pickPrevColor);
	else if (ctx->pickPreviewPressHalf == 2)
		_XClr_ApplyPickColor(ctx, ctx->pickCurrColor);
	ctx->pickPreviewPressHalf = 0;
	return 0;
}

int CALLBACK _XClr_OnClosePaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || !hDraw || hEle != ctx->hBtnClose || ctx->closing) return 0;
	_XClr_PaintCloseButton(ctx, hEle, hDraw);
	return 0;
}

int CALLBACK _XClr_OnCloseMouseStay(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	if (hEle) XEle_Redraw(hEle, FALSE);
	return 0;
}

int CALLBACK _XClr_OnCloseMouseLeave(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	if (hEle) XEle_Redraw(hEle, FALSE);
	return 0;
}

int CALLBACK _XClr_OnPickerPaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || !hDraw || ctx->closing) return 0;
	if (hEle == ctx->hSvPanel) _XClr_PaintSv(ctx, hEle, hDraw);
	else if (hEle == ctx->hHueBar) _XClr_PaintHue(ctx, hEle, hDraw);
	else if (hEle == ctx->hAlphaBar) _XClr_PaintAlpha(ctx, hEle, hDraw);
	else if (hEle == ctx->hBtnAddCandidate) _XClr_PaintAddSlot(ctx, hEle, hDraw);
	else if (hEle == ctx->hBtnEyedropper) _XClr_PaintEyedropper(ctx, hEle, hDraw);
	else if (hEle == ctx->hBtnPickPreview) _XClr_PaintPickPreview(ctx, hEle, hDraw);
	else {
		int idx = _XClr_SwatchIndexFromEle(ctx, hEle);
		if (idx >= 0) _XClr_PaintSwatch(ctx, hEle, hDraw, idx);
	}
	return 0;
}

int CALLBACK _XClr_OnPickerLDown(HELE hEle, UINT, POINT* pPt, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || !pPt || ctx->closing) return 0;
	if (hEle == ctx->hSvPanel){
		ctx->dragKind = _XClr_Drag_Sv;
		XEle_SetCapture(hEle, TRUE);
		_XClr_UpdateFromSv(ctx, hEle, *pPt);
	} else if (hEle == ctx->hHueBar){
		ctx->dragKind = _XClr_Drag_Hue;
		XEle_SetCapture(hEle, TRUE);
		_XClr_UpdateFromHue(ctx, hEle, *pPt);
	} else if (hEle == ctx->hAlphaBar){
		ctx->dragKind = _XClr_Drag_Alpha;
		XEle_SetCapture(hEle, TRUE);
		_XClr_UpdateFromAlpha(ctx, hEle, *pPt);
	}
	return 0;
}

int CALLBACK _XClr_OnPickerMove(HELE hEle, UINT, POINT* pPt, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || !pPt || ctx->closing || ctx->dragKind == _XClr_Drag_None) return 0;
	HELE dragEle = _XClr_DragTargetEle(ctx);
	if (!dragEle) return 0;
	_XClr_ApplyDragAt(ctx, dragEle, *pPt);
	return 0;
}

int CALLBACK _XClr_OnPickerLUp(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->dragKind == _XClr_Drag_None) return 0;
	HELE dragEle = _XClr_DragTargetEle(ctx);
	ctx->dragKind = _XClr_Drag_None;
	if (dragEle) XEle_SetCapture(dragEle, FALSE);
	return 0;
}

int CALLBACK _XClr_OnPickerKillCapture(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx) return 0;
	if (hEle == ctx->hSvPanel || hEle == ctx->hHueBar || hEle == ctx->hAlphaBar)
		ctx->dragKind = _XClr_Drag_None;
	return 0;
}

int CALLBACK _XClr_OnAddSlotClick(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing || hEle != ctx->hBtnAddCandidate) return 0;
	_XClr_AddCandidate(ctx);
	return 0;
}

int CALLBACK _XClr_OnSwatchClear(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing) return 0;
	int idx = _XClr_SwatchIndexFromEle(ctx, hEle);
	if (idx < 0 || idx >= ClrG().candidateCount) return 0;
	ctx->candidateSuppressClick = TRUE;
	_XClr_ClearCandidate(ctx, idx);
	return 0;
}

int CALLBACK _XClr_OnSwatchClick(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing) return 0;
	if (ctx->candidateSuppressClick){
		ctx->candidateSuppressClick = FALSE;
		return 0;
	}
	int idx = _XClr_SwatchIndexFromEle(ctx, hEle);
	if (idx < 0 || idx >= ClrG().candidateCount) return 0;
	if (ctx->candidateSelectedIndex == idx){
		_XClr_SetCandidateSelected(ctx, -1);
		return 0;
	}
	_XClr_SetCandidateSelected(ctx, idx);
	_XClr_ApplyPickColor(ctx, ClrG().candidates[idx]);
	return 0;
}

int CALLBACK _XClr_OnSwatchDblClick(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	return _XClr_OnSwatchClear(hEle, pbHandled);
}

int CALLBACK _XClr_OnSwatchRButtonUp(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	return _XClr_OnSwatchClear(hEle, pbHandled);
}

int CALLBACK _XClr_OnWndMouseMove(HWINDOW hWnd, UINT nFlags, POINT* pPt, BOOL* pbHandled)
{
	if (!hWnd || !pPt) return 0;
	auto it = ClrG().windows.find(hWnd);
	_XClr_Ctx* ctx = (it != ClrG().windows.end()) ? it->second : NULL;
	if (!ctx || ctx->closing || ctx->dragKind == _XClr_Drag_None) return 0;
	HELE dragEle = _XClr_DragTargetEle(ctx);
	if (!dragEle) return 0;
	POINT pt = *pPt;
	XEle_PointWndClientToEleClient(dragEle, &pt);
	_XClr_ApplyDragAt(ctx, dragEle, pt);
	if (pbHandled) *pbHandled = TRUE;
	return 0;
}

int CALLBACK _XClr_OnWndKillFocus(HWINDOW hWnd, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	auto it = ClrG().windows.find(hWnd);
	_XClr_Ctx* ctx = (it != ClrG().windows.end()) ? it->second : NULL;
	if (!ctx || ctx->closing) return 0;
	_XClr_DismissComboDrop(ctx);
	return 0;
}

int CALLBACK _XClr_OnEditKeyDown(HELE hEle, int iChar, int, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing || ctx->updatingUI) return 0;
	if (iChar == VK_UP || iChar == VK_DOWN){
		if (hEle != ctx->hEditAlphaPct && !_XClr_IsModeColorEdit(ctx, hEle)) return 0;
		_XClr_EditSpin(ctx, hEle, iChar == VK_UP ? 1 : -1);
		if (pbHandled) *pbHandled = TRUE;
		return 0;
	}
	if (iChar != VK_RETURN) return 0;
	_XClr_ParseEditInput(ctx, hEle, TRUE);
	if (pbHandled) *pbHandled = TRUE;
	return 0;
}

int CALLBACK _XClr_OnEditLButtonDBClick(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing || !_XClr_IsModeColorEdit(ctx, hEle)) return 0;
	wchar_t buf[64]{};
	_XClr_FormatCopyColorText(ctx, buf, _countof(buf));
	if (buf[0]) _XClr_CopyTextToClipboard(buf);
	return 0;
}

int CALLBACK _XClr_OnEditChar(HELE hEle, WPARAM wParam, LPARAM, BOOL* pbHandled)
{
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->updatingUI || ctx->closing) return 0;
	int maxLen = _XClr_EditMaxLen(ctx, hEle);
	if (maxLen <= 0) return 0;

	wchar_t c = (wchar_t)wParam;
	if (c < 0x20) return 0;

	if (!_XClr_IsEditCharAllowed(ctx, hEle, c) || _XClr_EditWouldExceedMaxLen(hEle, maxLen) ||
		_XClr_PreviewCharExceedsRange(ctx, hEle, c)){
		if (pbHandled) *pbHandled = TRUE;
		return 0;
	}
	return 0;
}

int CALLBACK _XClr_OnEditChanged(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->updatingUI || ctx->closing) return 0;
	if (_XClr_EditMaxLen(ctx, hEle) <= 0) return 0;

	const wchar_t* txt = XEdit_GetText_Temp(hEle);
	wchar_t buf[16]{};
	if (txt) wcscpy_s(buf, txt);

	BOOL changed = FALSE;
	if (_XClr_SanitizeEditText(ctx, hEle, buf, _countof(buf)))
		changed = TRUE;
	if (_XClr_ClampEditValueText(ctx, hEle, buf, _countof(buf)))
		changed = TRUE;

	if (changed){
		ctx->updatingUI = TRUE;
		XEdit_SetText(hEle, buf);
		ctx->updatingUI = FALSE;
	}

	_XClr_ParseEditInput(ctx, hEle, TRUE);
	return 0;
}

int CALLBACK _XClr_OnInputPaintEnd(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	if (!hEle || !hDraw) return 0;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || ctx->closing) return 0;
	if (!XEle_IsEnable(hEle)) return 0;

	RECT rc{};
	XEle_GetClientRect(hEle, &rc);
	if (rc.right <= rc.left || rc.bottom <= rc.top) return 0;

	if (hEle == ctx->hComboMode){
		_XClr_PaintComboChrome(ctx, hDraw, rc, hEle);
		HIMAGE hImg = ctx->comboDropOpen ? ctx->hImgComboUp : ctx->hImgComboDown;
		if (hImg){
			RECT btnRc{};
			_XClr_ComboDropButtonRect(rc, btnRc);
			_XClr_DrawCenterImage(hDraw, hImg, btnRc);
		}
	} else if (_XClr_IsUnderlineEdit(ctx, hEle)){
		_XClr_PaintEditUnderline(ctx, hDraw, rc, hEle);
		if (hEle == ctx->hEditHex){
			RECT markRc = rc;
			markRc.left = rc.left + kClr_HexMarkX;
			markRc.right = markRc.left + 12;
			XDraw_EnableSmoothingMode(hDraw, TRUE);
			XDraw_SetTextAlign(hDraw, textAlignFlag_left | textAlignFlag_vcenter);
			XDraw_SetBrushColor(hDraw, ctx->colors.mutedText);
			if (ctx->hFont) XDraw_SetFont(hDraw, ctx->hFont);
			XDraw_DrawText(hDraw, L"#", -1, &markRc);
		}
	}
	return 0;
}

int CALLBACK _XClr_OnComboSelect(HELE hEle, int iItem, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || hEle != ctx->hComboMode || ctx->updatingUI || ctx->closing) return 0;
	int sel = XComboBox_GetSelItem(hEle);
	if (sel < 0) sel = iItem;
	xcolor_input_mode_ newMode = _XClr_ModeFromComboIndex(sel);
	if (newMode == ctx->inputMode) return 0;
	_XClr_CommitPendingEdits(ctx);
	ctx->inputMode = newMode;
	_XClr_ApplyInputMode(ctx);
	return 0;
}

int CALLBACK _XClr_OnComboPopupList(HELE hEle, HWINDOW hDropWnd, HELE hListBox, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || hEle != ctx->hComboMode || ctx->closing) return 0;
	ctx->comboDropOpen = TRUE;
	if (hDropWnd && hListBox)
		_XClr_StyleComboDropPopup(ctx, hDropWnd, hListBox);
	if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
		XModalWnd_EnableAutoClose(ctx->hWnd, FALSE);
		XModalWnd_EnableEscClose(ctx->hWnd, FALSE);
	}
	_XClr_SyncColorFromEdits(ctx);
	return 0;
}

int CALLBACK _XClr_OnComboExitList(HELE hEle, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = FALSE;
	_XClr_Ctx* ctx = _XClr_CtxFromEle(hEle);
	if (!ctx || hEle != ctx->hComboMode || ctx->closing) return 0;
	ctx->comboDropOpen = FALSE;
	ctx->hComboDropList = NULL;
	if (ctx->hWnd && XC_IsHWINDOW((HXCGUI)ctx->hWnd)){
		XModalWnd_EnableAutoClose(ctx->hWnd, ctx->enableAutoClose);
		XModalWnd_EnableEscClose(ctx->hWnd, TRUE);
	}
	return 0;
}

int CALLBACK _XClr_OnPaintWindow(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled)
{
	if (pbHandled) *pbHandled = TRUE;
	if (!hWnd || !hDraw) return 0;
	auto it = ClrG().windows.find(hWnd);
	_XClr_Ctx* ctx = (it != ClrG().windows.end()) ? it->second : NULL;
	if (!ctx || ctx->closing) return 0;

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
		(float)bodyRc.left, (float)bodyRc.top,
		(float)bodyRc.right, (float)bodyRc.bottom
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

	// 描边最后画: 盖住分隔线抵到圆角处的端头.
	_XUITool::DrawBodyBorder(hDraw, bodyRc, ctx->cornerRadius, ctx->colors.border,
		_XUITool::WindowDpiScale(hWnd));
	return 0;
}

void _XClr_ApplyPopupPosition(_XClr_Ctx* ctx)
{
	if (!ctx || !ctx->hWnd || !XC_IsHWINDOW((HXCGUI)ctx->hWnd)) return;
	if (ctx->popupMode == _XClr_PopupMode_Pos){
		XWnd_SetPosition(ctx->hWnd, ctx->popupPt.x, ctx->popupPt.y);
		XWnd_AdjustInScreen(ctx->hWnd, 0, FALSE);
		return;
	}
	if (ctx->popupMode != _XClr_PopupMode_Ele) return;

	HELE hEle = ctx->bindEle;
	int offX = ctx->bindOffsetX;
	int offY = ctx->bindOffsetY;
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

HELE _XClr_CreateCloseButton(_XClr_Ctx* ctx, int x, int y, int w, int h)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE btn = XBtn_Create(x, y, w, h, L"", ctx->hWnd);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	XEle_ClearBkInfo(btn);
	XEle_EnableBkTransparent(btn, TRUE);
	XEle_EnableDrawBorder(btn, FALSE);
	_XClr_RegisterEle(ctx, btn);
	ClrG().commands[btn] = _XClr_Cmd_Close;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XClr_OnBtnClick);
	XEle_RegEventC1(btn, XE_PAINT, (void*)&_XClr_OnClosePaint);
	XEle_RegEventC1(btn, XE_MOUSESTAY, (void*)&_XClr_OnCloseMouseStay);
	XEle_RegEventC1(btn, XE_MOUSELEAVE, (void*)&_XClr_OnCloseMouseLeave);
	return btn;
}

HELE _XClr_CreateCmdButton(_XClr_Ctx* ctx, int x, int y, int w, int h,
	const wchar_t* text, _XClr_Cmd cmd, BOOL primary = FALSE, BOOL flatCancel = FALSE)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE btn = XBtn_Create(x, y, w, h, text, ctx->hWnd);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	_XClr_RegisterEle(ctx, btn);
	ClrG().commands[btn] = cmd;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XClr_OnBtnClick);
	_XClr_StyleButton(btn, ctx->colors, primary, flatCancel);
	if (ctx->hFont) XEle_SetFont(btn, ctx->hFont);
	return btn;
}

HELE _XClr_CreatePickerEle(_XClr_Ctx* ctx, int x, int y, int w, int h)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE ele = XEle_Create(x, y, w, h, (HXCGUI)ctx->hWnd);
	if (!ele) return NULL;
	XUI_EnableCSS(ele, FALSE);
	XEle_ClearBkInfo(ele);
	XEle_EnableBkTransparent(ele, TRUE);
	XEle_EnableDrawBorder(ele, FALSE);
	XEle_EnableMouseThrough(ele, FALSE);
	_XClr_RegisterEle(ctx, ele);
	// 必须用 XE_PAINT 接管绘制；XE_PAINT_END 会在默认白底之后再画，圆角外露出白角
	XEle_RegEventC1(ele, XE_PAINT, (void*)&_XClr_OnPickerPaint);
	XEle_RegEventC1(ele, XE_LBUTTONDOWN, (void*)&_XClr_OnPickerLDown);
	XEle_RegEventC1(ele, XE_MOUSEMOVE, (void*)&_XClr_OnPickerMove);
	XEle_RegEventC1(ele, XE_LBUTTONUP, (void*)&_XClr_OnPickerLUp);
	XEle_RegEventC1(ele, XE_KILLCAPTURE, (void*)&_XClr_OnPickerKillCapture);
	return ele;
}

HELE _XClr_CreateEyedropperEle(_XClr_Ctx* ctx, int x, int y, int w, int h)
{
	HELE ele = _XClr_CreatePickerEle(ctx, x, y, w, h);
	if (!ele) return NULL;
	ClrG().commands[ele] = _XClr_Cmd_Eyedropper;
	XEle_RegEventC1(ele, XE_LBUTTONDOWN, (void*)&_XClr_OnEyedropperLDown);
	return ele;
}

HELE _XClr_CreatePickPreviewEle(_XClr_Ctx* ctx, int x, int y, int w, int h)
{
	HELE ele = _XClr_CreatePickerEle(ctx, x, y, w, h);
	if (!ele) return NULL;
	XEle_RegEventC1(ele, XE_LBUTTONDOWN, (void*)&_XClr_OnPickPreviewLDown);
	XEle_RegEventC1(ele, XE_LBUTTONUP, (void*)&_XClr_OnPickPreviewLUp);
	return ele;
}

HELE _XClr_CreateModeParentWrap(_XClr_Ctx* ctx, int x, int y, int w, int h)
{
	if (!ctx) return NULL;
	x += ctx->contentOffX;
	y += ctx->contentOffY;
	HELE wrap = XLayout_Create(x, y, w, h, (HXCGUI)ctx->hWnd);
	if (!wrap) return NULL;
	_XClr_RegisterEle(ctx, wrap);
	XLayout_EnableLayout(wrap, FALSE);
	_XClr_InitLayoutWrap(wrap);
	return wrap;
}

HELE _XClr_CreateChildHexEdit(_XClr_Ctx* ctx, HELE hParent, int w, int h)
{
	if (!ctx || !hParent) return NULL;
	HELE edit = XEdit_Create(0, 0, w, h, (HXCGUI)hParent);
	if (!edit) return NULL;
	XUI_EnableCSS(edit, FALSE);
	_XClr_RegisterEle(ctx, edit);
	_XClr_StyleHexInput(edit, ctx->colors);
	if (ctx->hFont) XEle_SetFont(edit, ctx->hFont);
	_XClr_RegisterModeEditEvents(edit);
	XEle_EnableFocus(edit, TRUE);
	return edit;
}

HELE _XClr_CreateChildHBoxLayout(_XClr_Ctx* ctx, HELE hParent, int w, int h, int space)
{
	if (!ctx || !hParent) return NULL;
	HELE wrap = XLayout_Create(0, 0, w, h, (HXCGUI)hParent);
	if (!wrap) return NULL;
	_XClr_RegisterEle(ctx, wrap);
	XLayoutBox_EnableHorizon(wrap, TRUE);
	XLayoutBox_SetSpace(wrap, space);
	XLayout_EnableLayout(wrap, TRUE);
	_XClr_InitLayoutWrap(wrap);
	return wrap;
}

HELE _XClr_CreateChildEdit(_XClr_Ctx* ctx, HXCGUI hParent, int h)
{
	if (!ctx || !hParent) return NULL;
	HELE edit = XEdit_Create(0, 0, kClr_ChannelMinW, h, hParent);
	if (!edit) return NULL;
	XUI_EnableCSS(edit, FALSE);
	_XClr_RegisterEle(ctx, edit);
	_XClr_StyleInput(edit, ctx->colors);
	if (ctx->hFont) XEle_SetFont(edit, ctx->hFont);
	_XClr_RegisterModeEditEvents(edit);
	XWidget_LayoutItem_SetWidth(edit, layout_size_weight, 1);
	XWidget_LayoutItem_SetMinSize(edit, kClr_ChannelMinW, h);
	XWidget_LayoutItem_SetHeight(edit, layout_size_fill, 0);
	return edit;
}

// 单编辑框模式布局：对齐 calendar 的 _XCal_CreateEdit 外层 wrap，保证边框样式生效
HELE _XClr_CreateSingleEditWrap(_XClr_Ctx* ctx, int x, int y, int w, int h, HELE* pEdit)
{
	if (!ctx) return NULL;
	x += ctx->contentOffX;
	y += ctx->contentOffY;
	HELE wrap = XLayout_Create(x, y, w, h, (HXCGUI)ctx->hWnd);
	if (!wrap) return NULL;
	_XClr_RegisterEle(ctx, wrap);
	XLayout_EnableLayout(wrap, FALSE);
	_XClr_InitLayoutWrap(wrap);
	XWidget_LayoutItem_SetWidth(wrap, layout_size_fixed, w);
	XWidget_LayoutItem_SetHeight(wrap, layout_size_fixed, h);

	HELE edit = XEdit_Create(0, 0, w, h, (HXCGUI)wrap);
	if (!edit){
		if (pEdit) *pEdit = NULL;
		return wrap;
	}
	XUI_EnableCSS(edit, FALSE);
	_XClr_RegisterEle(ctx, edit);
	_XClr_StyleInput(edit, ctx->colors);
	if (ctx->hFont) XEle_SetFont(edit, ctx->hFont);
	_XClr_RegisterModeEditEvents(edit);
	if (pEdit) *pEdit = edit;
	return wrap;
}

HELE _XClr_CreateSimpleEdit(_XClr_Ctx* ctx, int x, int y, int w, int h)
{
	HELE edit = NULL;
	_XClr_CreateSingleEditWrap(ctx, x, y, w, h, &edit);
	return edit;
}

HELE _XClr_CreateComboMode(_XClr_Ctx* ctx, int x, int y, int w, int h)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE combo = XComboBox_Create(x, y, w, h, ctx->hWnd);
	if (!combo) return NULL;
	_XClr_RegisterEle(ctx, combo);

	// 官方推荐：XAdTable_Create → BindAdapter → AddColumn → AddItemText
	// CreateAdapter 的返回值不是可直接写行的表适配器句柄，勿对其调用 XAdTable_AddRowText
	HXCGUI adapter = XAdTable_Create();
	if (!adapter){
		ClrG().elements.erase(combo);
		XEle_Destroy(combo);
		return NULL;
	}
	XComboBox_BindAdapter(combo, adapter);
	XAdTable_AddColumn(adapter, XC_NAME1);
	XComboBox_AddItemText(combo, L"HEX");
	XComboBox_AddItemText(combo, L"RGB");
	XComboBox_AddItemText(combo, L"HSL");
	XComboBox_SetSelItem(combo, 0);

	_XClr_StyleCombo(combo, ctx->colors);
	if (ctx->hFontCombo) XEle_SetFont(combo, ctx->hFontCombo);
	XComboBox_EnableDrawButton(combo, FALSE);
	XComboBox_SetButtonSize(combo, 20);
	XComboBox_EnableDropHeightFixed(combo, TRUE);
	XComboBox_SetDropHeight(combo, kClr_EditH * 3);
	XEle_RegEventC1(combo, XE_COMBOBOX_POPUP_LIST, (void*)&_XClr_OnComboPopupList);
	XEle_RegEventC1(combo, XE_COMBOBOX_EXIT_LIST, (void*)&_XClr_OnComboExitList);
	XEle_RegEventC1(combo, XE_COMBOBOX_SELECT_END, (void*)&_XClr_OnComboSelect);
	return combo;
}

HELE _XClr_CreateAddSlotEle(_XClr_Ctx* ctx, int x, int y)
{
	HELE ele = _XClr_CreatePickerEle(ctx, x, y, kClr_SwatchSize, kClr_SwatchSize);
	if (!ele) return NULL;
	ClrG().commands[ele] = _XClr_Cmd_AddCandidate;
	XEle_RegEventC1(ele, XE_LBUTTONUP, (void*)&_XClr_OnAddSlotClick);
	return ele;
}

HELE _XClr_CreateSwatchEle(_XClr_Ctx* ctx, int x, int y, int index)
{
	HELE ele = _XClr_CreatePickerEle(ctx, x, y, kClr_SwatchSize, kClr_SwatchSize);
	if (!ele) return NULL;
	(void)index;
	XEle_RegEventC1(ele, XE_LBUTTONUP, (void*)&_XClr_OnSwatchClick);
	XEle_RegEventC1(ele, XE_LBUTTONDBCLICK, (void*)&_XClr_OnSwatchDblClick);
	XEle_RegEventC1(ele, XE_RBUTTONUP, (void*)&_XClr_OnSwatchRButtonUp);
	return ele;
}

HELE _XClr_CreateHBoxLayout(_XClr_Ctx* ctx, int x, int y, int w, int h, int space)
{
	x += ctx ? ctx->contentOffX : 0;
	y += ctx ? ctx->contentOffY : 0;
	HELE wrap = XLayout_Create(x, y, w, h, (HXCGUI)ctx->hWnd);
	if (!wrap) return NULL;
	_XClr_RegisterEle(ctx, wrap);
	XLayoutBox_EnableHorizon(wrap, TRUE);
	XLayoutBox_SetSpace(wrap, space);
	XLayout_EnableLayout(wrap, TRUE);
	_XClr_InitLayoutWrap(wrap);
	XWidget_LayoutItem_SetWidth(wrap, layout_size_fixed, w);
	XWidget_LayoutItem_SetHeight(wrap, layout_size_fixed, h);
	return wrap;
}

HELE _XClr_CreateChildCmdButton(_XClr_Ctx* ctx, HXCGUI hParent, int x, int y, int w, int h,
	const wchar_t* text, _XClr_Cmd cmd, BOOL primary = FALSE, BOOL flatCancel = FALSE)
{
	HELE btn = XBtn_Create(x, y, w, h, text, hParent);
	if (!btn) return NULL;
	XUI_EnableCSS(btn, FALSE);
	XBtn_SetTypeEx(btn, button_type_default);
	XBtn_SetTextAlign(btn, textAlignFlag_center | textAlignFlag_vcenter);
	_XClr_RegisterEle(ctx, btn);
	ClrG().commands[btn] = cmd;
	XEle_RegEventC1(btn, XE_BNCLICK, (void*)&_XClr_OnBtnClick);
	_XClr_StyleButton(btn, ctx->colors, primary, flatCancel);
	if (ctx->hFont) XEle_SetFont(btn, ctx->hFont);
	return btn;
}

void _XClr_LayoutInputRow(_XClr_Ctx* ctx, int left, int inputY, BOOL showAlpha)
{
	if (!ctx) return;
	const int editY = inputY;

	// 右侧固定：模式下拉组合框
	int xRight = left + kClr_InnerW;
	xRight -= kClr_ComboW;
	ctx->hComboMode = _XClr_CreateComboMode(ctx, xRight, editY, kClr_ComboW, kClr_EditH);
	xRight -= kClr_InputGap + 2;

	// 透明度 % 在组合框左侧（% 标签单独占位，避免被组合框裁切）
	if (showAlpha){
		xRight -= kClr_PctLabelW;
		ctx->hLabelPct = _XClr_CreateText(ctx, xRight, editY + 6, kClr_PctLabelW, 20, L"%",
			ctx->colors.mutedText, textAlignFlag_right | textAlignFlag_vcenter);
		xRight -= kClr_PctLabelGap;
		xRight -= kClr_AlphaEditW;
		ctx->hEditAlphaPct = _XClr_CreateSimpleEdit(ctx, xRight, editY, kClr_AlphaEditW, kClr_EditH);
		xRight -= kClr_InputGap;
	}

	// 左侧：父 Layout 内挂 HEX 编辑框 / RGB 子 Layout / HSL 子 Layout，切换时显隐
	const int modeX = left;
	int modeW = xRight - modeX;
	if (modeW < kClr_ChannelMinW * 3 + kClr_ChannelGap * 2)
		modeW = kClr_ChannelMinW * 3 + kClr_ChannelGap * 2;

	ctx->hModeWrap = _XClr_CreateModeParentWrap(ctx, modeX, editY, modeW, kClr_EditH);
	if (ctx->hModeWrap){
		ctx->hRgbWrap = _XClr_CreateChildHBoxLayout(ctx, ctx->hModeWrap, modeW, kClr_EditH, kClr_ChannelGap);
		if (ctx->hRgbWrap){
			ctx->hEditR = _XClr_CreateChildEdit(ctx, (HXCGUI)ctx->hRgbWrap, kClr_EditH);
			ctx->hEditG = _XClr_CreateChildEdit(ctx, (HXCGUI)ctx->hRgbWrap, kClr_EditH);
			ctx->hEditB = _XClr_CreateChildEdit(ctx, (HXCGUI)ctx->hRgbWrap, kClr_EditH);
		}
		ctx->hHslWrap = _XClr_CreateChildHBoxLayout(ctx, ctx->hModeWrap, modeW, kClr_EditH, kClr_ChannelGap);
		if (ctx->hHslWrap){
			ctx->hEditH = _XClr_CreateChildEdit(ctx, (HXCGUI)ctx->hHslWrap, kClr_EditH);
			ctx->hEditS = _XClr_CreateChildEdit(ctx, (HXCGUI)ctx->hHslWrap, kClr_EditH);
			ctx->hEditL = _XClr_CreateChildEdit(ctx, (HXCGUI)ctx->hHslWrap, kClr_EditH);
		}
		// 最后创建，默认 Z 序在最上，HEX 模式可直接点击
		ctx->hEditHex = _XClr_CreateChildHexEdit(ctx, ctx->hModeWrap, modeW, kClr_EditH);
	}

	_XClr_ApplyInputMode(ctx);
}

int _XClr_CalcContentH(BOOL showAlpha)
{
	const _XClr_PickerLayout pl = _XClr_CalcPickerLayout(showAlpha);
	const _XClr_VertLayout vl = _XClr_CalcVertLayout(pl.svSize);
	return vl.actionY + kClr_ActionH + kClr_SepGap;
}

_XClr_Ctx* _XClr_CreateWindow(HWINDOW hParent, xuitool_theme_ theme, int cornerRadius, BOOL showAlpha,
	const xcolor_rgba_* pInitialColor, xcolor_input_mode_ initialMode)
{
	_XClr_Ctx* ctx = new _XClr_Ctx();
	if (!ctx) return NULL;
	ctx->theme = theme;
	ctx->showAlpha = showAlpha;
	ctx->inputMode = initialMode;
	ctx->hParent = hParent ? XWnd_GetHWND(hParent) : NULL;
	ctx->cornerRadius = _XClr_ClampI(cornerRadius, 0, 32);
	_XClr_ResolveTheme(theme, &ctx->colors);
	_XClr_CopyPopupSettingsFromGlobal(ctx);

	if (pInitialColor){
		ctx->rgba = _XClr_NormalizeInitialColor(*pInitialColor, showAlpha);
		ctx->pickPrevColor = ctx->rgba;
		ctx->pickCurrColor = ctx->rgba;
		_XClr_SyncHslFromRgb(ctx);
	}

	ctx->contentW = kClr_ContentW;
	ctx->contentH = _XClr_CalcContentH(showAlpha);
	int winW = ctx->contentW + _XUITool::kShadowMargin * 2;
	int winH = ctx->contentH + _XUITool::kShadowMargin * 2;

	ctx->hWnd = XModalWnd_Create(winW, winH, L"选择颜色", ctx->hParent, window_style_nothing);
	if (!ctx->hWnd){
		delete ctx;
		return NULL;
	}
	ClrG().windows[ctx->hWnd] = ctx;
	XWnd_SetTransparentType(ctx->hWnd, window_transparent_shaped);
	XWnd_SetTransparentAlpha(ctx->hWnd, 255);
	XWnd_EnableDragWindow(ctx->hWnd, ctx->enableDrag);
	XWnd_EnableDragBorder(ctx->hWnd, FALSE);
	XWnd_EnableDragCaption(ctx->hWnd, ctx->enableDrag);
	if (ctx->enableTopmost)
		XWnd_SetTop(ctx->hWnd, TRUE);
	XWnd_EnableDrawBk(ctx->hWnd, TRUE);
	XWnd_SetTextColor(ctx->hWnd, ctx->colors.text);
	XWnd_ClearBkInfo(ctx->hWnd);
	XWnd_RegEventC1(ctx->hWnd, WM_PAINT,      (void*)&_XClr_OnPaintWindow);
	XWnd_RegEventC1(ctx->hWnd, WM_MOUSEMOVE,  (void*)&_XClr_OnWndMouseMove);
	XWnd_RegEventC1(ctx->hWnd, WM_KILLFOCUS,  (void*)&_XClr_OnWndKillFocus);
	XModalWnd_EnableAutoClose(ctx->hWnd, ctx->enableAutoClose);
	XModalWnd_EnableEscClose(ctx->hWnd, TRUE);

	ctx->hFontTitle = XFont_CreateEx(L"微软雅黑", kClr_TitleFontPt, fontStyle_regular);
	ctx->hFont = XFont_CreateEx(L"微软雅黑", 10, fontStyle_regular);
	ctx->hFontCombo = XFont_CreateEx(L"微软雅黑", kClr_ComboFontPt, fontStyle_regular);
	_XClr_LoadIcons(ctx);

	const int left = kClr_PadH;
	const _XClr_PickerLayout pl = _XClr_CalcPickerLayout(showAlpha);
	const _XClr_VertLayout vl = _XClr_CalcVertLayout(pl.svSize);

	_XClr_SetSeparator(ctx, 0, vl.headerSepY);
	_XClr_SetSeparator(ctx, 1, vl.footerSepY);

	const int titleH = kClr_TitleFontPt + 6;
	const int headerBandH = vl.headerSepY;
	const int headerY = (headerBandH - kClr_HeaderH) / 2;
	const int closeY = (headerBandH - kClr_CloseBtn) / 2;
	const int titleY = (headerBandH - titleH) / 2;
	const int closeX = kClr_ContentW - kClr_HeaderCloseRight - kClr_CloseBtn;
	ctx->hTitle = _XClr_CreateText(ctx, left, titleY,
		120, titleH, L"颜色", ctx->colors.text, textAlignFlag_left | textAlignFlag_vcenter, ctx->hFontTitle);
	ctx->hBtnClose = _XClr_CreateCloseButton(ctx, closeX, closeY, kClr_CloseBtn, kClr_CloseBtn);

	ctx->hSvPanel = _XClr_CreatePickerEle(ctx, left, vl.mainY, pl.svSize, pl.svSize);
	int barX = left + pl.svSize + pl.barGap;
	ctx->hHueBar = _XClr_CreatePickerEle(ctx, barX, vl.mainY, pl.barW, pl.svSize);
	if (showAlpha){
		ctx->hAlphaBar = _XClr_CreatePickerEle(ctx, barX + pl.barW + pl.barGap, vl.mainY,
			pl.barW, pl.svSize);
	}

	_XClr_LayoutInputRow(ctx, left, vl.inputY, showAlpha);

	const int dropY = vl.candidateY + (kClr_SwatchSize - kClr_EyedropperSize) / 2;
	ctx->hBtnEyedropper = _XClr_CreateEyedropperEle(ctx, left, dropY,
		kClr_EyedropperSize, kClr_EyedropperSize);

	int cx = left + kClr_EyedropperSize + kClr_SwatchGap;
	ctx->hBtnPickPreview = _XClr_CreatePickPreviewEle(ctx, cx, vl.candidateY,
		kClr_PickPreviewSize, kClr_PickPreviewSize);
	cx += kClr_PickPreviewSize + kClr_SwatchGap;
	ctx->hBtnAddCandidate = _XClr_CreateAddSlotEle(ctx, cx, vl.candidateY);
	cx += kClr_SwatchSize + kClr_SwatchGap;
	for (int i = 0; i < kClr_CandidateMax; ++i){
		ctx->hCandidateSwatch[i] = _XClr_CreateSwatchEle(ctx, cx, vl.candidateY, i);
		cx += kClr_SwatchSize + kClr_SwatchGap;
	}
	_XClr_RefreshCandidates(ctx);

	HELE hActionWrap = _XClr_CreateHBoxLayout(ctx, left, vl.actionY, kClr_InnerW, kClr_ActionH, kClr_ActionGap);
	if (hActionWrap){
		ctx->hBtnCancel = _XClr_CreateChildCmdButton(ctx, (HXCGUI)hActionWrap, 0, 0, 100, kClr_ActionH,
			L"取消", _XClr_Cmd_Cancel, FALSE, TRUE);
		ctx->hBtnConfirm = _XClr_CreateChildCmdButton(ctx, (HXCGUI)hActionWrap, 0, 0, 100, kClr_ActionH,
			L"确认", _XClr_Cmd_Confirm, TRUE);
		if (ctx->hBtnCancel){
			XWidget_LayoutItem_SetWidth(ctx->hBtnCancel, layout_size_weight, 1);
			XWidget_LayoutItem_SetHeight(ctx->hBtnCancel, layout_size_fill, 0);
		}
		if (ctx->hBtnConfirm){
			XWidget_LayoutItem_SetWidth(ctx->hBtnConfirm, layout_size_weight, 1);
			XWidget_LayoutItem_SetHeight(ctx->hBtnConfirm, layout_size_fill, 0);
		}
	}

	if (!showAlpha){
		if (ctx->hEditAlphaPct) XEle_Enable(ctx->hEditAlphaPct, FALSE);
		if (ctx->hLabelPct) XWidget_Show(ctx->hLabelPct, FALSE);
	}

	_XClr_ApplyIcons(ctx);
	return ctx;
}

BOOL _XClr_Show(HWINDOW hParent, xcolor_rgba_* pColor, BOOL showAlpha,
	xuitool_theme_ theme, int cornerRadius, BOOL liveNotify, xcolor_input_mode_ initialMode)
{
	if (!pColor) return FALSE;
	if (ClrG().eyedropperCtx)
		_XClr_StopEyedropper(FALSE);

	xcolor_rgba_ initial = _XClr_NormalizeInitialColor(*pColor, showAlpha);
	_XClr_Ctx* ctx = _XClr_CreateWindow(hParent, theme, cornerRadius, showAlpha, &initial, initialMode);
	if (!ctx) return FALSE;
	ctx->liveNotify = liveNotify;
	_XClr_ApplyInputMode(ctx);
	_XClr_RefreshAll(ctx, FALSE);

	_XClr_ApplyPopupPosition(ctx);
	XWnd_ShowWindow(ctx->hWnd, SW_SHOWNOACTIVATE);
	int result = _XClr_RunPopupLoop(ctx);
	ctx->closing = TRUE;
	if (ctx->eyedropperActive || ClrG().eyedropperCtx == ctx)
		_XClr_StopEyedropper(FALSE);

	BOOL ok = (result == kClr_ResultOk && ctx->confirmed);
	xcolor_rgba_ out = ctx->resultColor;
	HWINDOW hWnd = ctx->hWnd;
	_XClr_UnregisterCtx(ctx);

	if (hWnd && XC_IsHWINDOW((HXCGUI)hWnd)){
		XWnd_DestroyWindow(hWnd);
	}
	_XClr_DestroyIcons(ctx);
	delete ctx;

	if (ok) *pColor = out;
	return ok;
}

} // anonymous namespace

//============================================================================
// CXColorPicker 公开接口
//============================================================================

xcolor_rgba_ CXColorPicker::Rgb(BYTE r, BYTE g, BYTE b, BYTE a)
{
	xcolor_rgba_ c{};
	c.r = r; c.g = g; c.b = b; c.a = a;
	return c;
}

xcolor_rgba_ CXColorPicker::FromCOLORREF(COLORREF c)
{
	xcolor_rgba_ out{};
	out.b = (BYTE)(c & 0xFF);
	out.g = (BYTE)((c >> 8) & 0xFF);
	out.r = (BYTE)((c >> 16) & 0xFF);
	out.a = (BYTE)((c >> 24) & 0xFF);
	if (out.a == 0) out.a = 255;
	return out;
}

COLORREF CXColorPicker::ToCOLORREF(xcolor_rgba_ c)
{
	return RGBA(c.r, c.g, c.b, c.a);
}

CXText CXColorPicker::FormatHex(xcolor_rgba_ c, BOOL withAlpha)
{
	wchar_t buf[16]{};
	XClr_FormatHexText(c, withAlpha, buf, _countof(buf));
	return CXText(buf);
}

void CXColorPicker::SetBindEle(HELE hEle, int offsetX, int offsetY)
{
	auto& g = ClrG();
	g.popupMode = _XClr_PopupMode_Ele;
	g.bindEle = hEle;
	g.bindOffsetX = offsetX;
	g.bindOffsetY = offsetY;
}

void CXColorPicker::SetPopupPosition(POINT pt)
{
	auto& g = ClrG();
	g.popupMode = _XClr_PopupMode_Pos;
	g.popupPt = pt;
}

void CXColorPicker::SetEnableAutoClose(BOOL bEnable)
{
	ClrG().enableAutoClose = bEnable;
}

void CXColorPicker::SetEnableModal(BOOL bEnable)
{
	ClrG().enableModal = bEnable;
}

void CXColorPicker::SetEnableDrag(BOOL bEnable)
{
	ClrG().enableDrag = bEnable;
}

void CXColorPicker::SetEnableTopmost(BOOL bEnable)
{
	ClrG().enableTopmost = bEnable;
}

void CXColorPicker::SetOnColorChanged(XCOLOR_PICKER_PROC_CHANGED proc, void* pUserData)
{
	auto& g = ClrG();
	g.onChanged = proc;
	g.onChangedUser = pUserData;
}

BOOL CXColorPicker::Popup(HWINDOW hParent, xcolor_rgba_* pColor,
	BOOL bShowAlpha, xuitool_theme_ theme, int nCornerRadius, BOOL bLiveNotify,
	xcolor_input_mode_ initialMode)
{
	return _XClr_Show(hParent, pColor, bShowAlpha, theme, nCornerRadius, bLiveNotify, initialMode);
}

BOOL CXColorPicker::ParseHex(const wchar_t* text, xcolor_rgba_* out)
{
	if (!out) return FALSE;
	BYTE r = 0, g = 0, b = 0, a = 255;
	if (!_XClr_ParseHexString(text, &r, &g, &b, &a, TRUE)) return FALSE;
	out->r = r;
	out->g = g;
	out->b = b;
	out->a = a;
	return TRUE;
}
