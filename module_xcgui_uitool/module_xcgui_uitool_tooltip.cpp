//============================================================================
// module_xcgui_uitool_tooltip.cpp — CXTooltip 实现
//============================================================================
//============================================================================
// 常量
//============================================================================
namespace {

// Timer IDs (避免与 XCGUI 内部 / 用户 timer 冲突, 用相对大的 base)
constexpr UINT kTimerId_ShowDelay   = 0x7100;   // 源元素上, ShowDelay 倒计时
constexpr UINT kTimerId_FadeAnim    = 0x7101;   // 气泡 body 上, 渐显/渐隐动画
constexpr UINT kTimerId_AutoClose   = 0x7102;   // 气泡 body 上, 自动关闭

constexpr int  kFadeTickMs          = 16;       // 60Hz 动画刷新

// 默认行为参数
constexpr int  kDefaultShowDelayMs  = 0;
constexpr int  kDefaultAutoCloseMs  = 0;        // 0 = 不自动关闭
constexpr int  kDefaultFadeMs       = 150;
constexpr int  kDefaultMarginL      = 16;
constexpr int  kDefaultMarginT      = 10;
constexpr int  kDefaultMarginR      = 16;
constexpr int  kDefaultMarginB      = 10;

// 字体: 微软雅黑 10pt
constexpr int  kFontSize            = 10;

// 图标尺寸 (与 1~4.svg viewBox 16x16 一致)
constexpr int  kIconSize            = 16;
constexpr int  kIconTextGap         = 8;        // 图标与文本之间的间距

// 阴影参数见 _XUITool::kShadow*

// 三角箭头
constexpr int  kArrowSize           = 7;        // 三角的"半宽" (像素), 即从底边到顶点的距离
constexpr int  kArrowEdgeOffset     = 20;       // 三角顶点距气泡圆角的最近距离

// 三角箭头方向 (基于源元素相对气泡的位置 = 鼠标进入源时距哪条边最近)
enum _XTip_ArrowSide
{
	_XTip_ArrowSide_None   = 0,
	_XTip_ArrowSide_Left   = 1,   // 气泡在源右侧, 箭头在气泡左侧指向左
	_XTip_ArrowSide_Right  = 2,
	_XTip_ArrowSide_Top    = 3,
	_XTip_ArrowSide_Bottom = 4,
};

//============================================================================
// 注册项 (每个源元素一份)
//============================================================================
struct _XTip_Entry
{
	std::wstring        text;
	xtooltip_type_      type           = xtooltip_type_default;
	xuitool_theme_     theme          = xuitool_theme_dark;
	BOOL                multiline      = FALSE;
	xtooltip_align_h_   alignH         = xtooltip_align_h_center;
	xtooltip_align_v_   alignV         = xtooltip_align_v_center;
	xtooltip_arrow_side_ arrowSide     = xtooltip_arrow_side_auto;
	BOOL                showArrow      = TRUE;        // 默认显示三角箭头
	COLORREF            customText     = _XUITool::kDarkText;
	COLORREF            customBg       = _XUITool::kDarkBg;
	int                 marginL        = kDefaultMarginL;
	int                 marginT        = kDefaultMarginT;
	int                 marginR        = kDefaultMarginR;
	int                 marginB        = kDefaultMarginB;
	int                 showDelayMs    = kDefaultShowDelayMs;
	int                 autoCloseMs    = kDefaultAutoCloseMs;
	int                 fadeMs         = kDefaultFadeMs;
	BOOL                eventsHooked   = FALSE;  // 是否已挂事件
};

//============================================================================
// 全局共享气泡状态 (单例, 进程内 1 份)
//============================================================================
enum _XTip_State
{
	_XTip_State_Hidden    = 0,
	_XTip_State_FadeIn    = 1,   // alpha 0 -> 255
	_XTip_State_Shown     = 2,   // alpha = 255
	_XTip_State_FadeOut   = 3,   // alpha 255 -> 0
};

struct _XTip_Global
{
	std::unordered_map<HELE, _XTip_Entry> registry;

	// 共享气泡窗口 (懒创建)
	HWINDOW           hTipWnd        = NULL;
	HELE              hTipBody       = NULL;
	HWND              hTipHwnd       = NULL;

	// SVG 句柄 (懒加载)
	HSVG              hSvgSuccess    = NULL;
	HSVG              hSvgInfo       = NULL;
	HSVG              hSvgWarning    = NULL;
	HSVG              hSvgError      = NULL;

	// 字体
	HFONTX            hFont          = NULL;

	// 当前显示状态
	HELE              hCurSrc        = NULL;    // 当前关联的源元素 (注册表 key)
	_XTip_State       state          = _XTip_State_Hidden;
	_XTip_ArrowSide   arrowSide      = _XTip_ArrowSide_None;
	int               arrowEdgePos   = 0;       // 三角顶点在指定边上的偏移 (从气泡内圆角矩形左/上)
	DWORD             fadeStartTick  = 0;
	DWORD             fadeDuration   = 0;
	int               fadeFromAlpha  = 0;
	int               fadeToAlpha    = 255;
	DWORD             autoCloseAt    = 0;       // 0 = 禁用

	// 上一次计算的几何 (供 OnPaint 用)
	int               winW           = 0;
	int               winH           = 0;
	int               bodyOffX       = 0;       // body 相对 window 客户区左上的偏移
	int               bodyOffY       = 0;
	int               bodyW          = 0;
	int               bodyH          = 0;
	int               textOffX       = 0;       // 文本相对 body 左上的偏移
	int               textOffY       = 0;
	int               textW          = 0;
	int               textH          = 0;
	BOOL              hasIcon        = FALSE;
	int               iconOffX       = 0;
	int               iconOffY       = 0;
};

_XTip_Global& G(){
	static _XTip_Global s;
	return s;
}

//============================================================================
// 工具函数
//============================================================================

// 四舍五入. 直接 (int)(x*scale) 在边界处会差 1 物理像素 (规范 §3.5.5 #2).
inline int RP(double v){ return v >= 0 ? (int)(v + 0.5) : -(int)(-v + 0.5); }

// 颜色取 ARGB 各分量
inline BYTE A(COLORREF c){ return (BYTE)((c >> 24) & 0xFF); }
inline BYTE R(COLORREF c){ return (BYTE) (c        & 0xFF); }
inline BYTE Gc(COLORREF c){ return (BYTE)((c >>  8) & 0xFF); }
inline BYTE B(COLORREF c){ return (BYTE)((c >> 16) & 0xFF); }

// 重新组合 (a, r, g, b) -> XCGUI COLORREF
inline COLORREF ARGB(BYTE r, BYTE g, BYTE b, BYTE a){
	return ((COLORREF)a << 24) | ((COLORREF)b << 16) | ((COLORREF)g << 8) | (COLORREF)r;
}

//============================================================================
// SVG 资源生命周期
//============================================================================
void EnsureSvgsLoaded()
{
	auto& g = G();
	if (!g.hSvgSuccess) g.hSvgSuccess = XSvg_LoadStringUtf8(kTipSvg_Success);
	if (!g.hSvgInfo)    g.hSvgInfo    = XSvg_LoadStringUtf8(kTipSvg_Info);
	if (!g.hSvgWarning) g.hSvgWarning = XSvg_LoadStringUtf8(kTipSvg_Warning);
	if (!g.hSvgError)   g.hSvgError   = XSvg_LoadStringUtf8(kTipSvg_Error);
	// EnableAutoDestroy 关掉, 我们手动控制 (避免某次 paint 内部释放掉 SVG 句柄).
	if (g.hSvgSuccess) XSvg_EnableAutoDestroy(g.hSvgSuccess, FALSE);
	if (g.hSvgInfo)    XSvg_EnableAutoDestroy(g.hSvgInfo,    FALSE);
	if (g.hSvgWarning) XSvg_EnableAutoDestroy(g.hSvgWarning, FALSE);
	if (g.hSvgError)   XSvg_EnableAutoDestroy(g.hSvgError,   FALSE);
}

void DestroySvgs()
{
	auto& g = G();
	if (g.hSvgSuccess){ XSvg_Destroy(g.hSvgSuccess); g.hSvgSuccess = NULL; }
	if (g.hSvgInfo)   { XSvg_Destroy(g.hSvgInfo);    g.hSvgInfo    = NULL; }
	if (g.hSvgWarning){ XSvg_Destroy(g.hSvgWarning); g.hSvgWarning = NULL; }
	if (g.hSvgError)  { XSvg_Destroy(g.hSvgError);   g.hSvgError   = NULL; }
}

HSVG SvgForType(xtooltip_type_ t)
{
	auto& g = G();
	switch (t){
	case xtooltip_type_success: return g.hSvgSuccess;
	case xtooltip_type_info:    return g.hSvgInfo;
	case xtooltip_type_warning: return g.hSvgWarning;
	case xtooltip_type_error:   return g.hSvgError;
	default:                    return NULL;
	}
}

//============================================================================
// 字体
//============================================================================
void EnsureFontLoaded()
{
	auto& g = G();
	if (!g.hFont){
		g.hFont = XFont_CreateEx(L"微软雅黑", kFontSize, 0);
	}
}

//============================================================================
// 主题颜色解析
//============================================================================
void ResolveColors(const _XTip_Entry& e, COLORREF* outText, COLORREF* outBg)
{
	_XUITool::ThemePalette pal;
	_XUITool::ResolvePalette(e.theme, e.customText, e.customBg,
		_XUITool::kDarkAccent, &pal);
	*outText = pal.text;
	*outBg = pal.bg;
}

//============================================================================
// 文本测量 (XCGUI 原生 API, 保证与 XDraw_DrawText 同字体引擎下渲染尺寸一致)
//
//   - 单行: XC_GetTextShowSize       (无 wrap)
//   - 多行: XC_GetTextShowRect       (NoWrap=off + width 上限, 自动断行)
//
// 旧版本走 GDI CreateFontIndirect 临时 DC, 在 D2D 主路径下会有 ~1px 量级误差
// (DirectWrite 与 GDI text metric 略有差); 改用 XCGUI 原生 helper 后零误差.
//============================================================================
void MeasureText(const wchar_t* text, BOOL multiline, int maxWidth, int* outW, int* outH)
{
	*outW = *outH = 0;
	if (!text || !*text) { *outW = 0; *outH = (int)(kFontSize * 1.4); return; }
	EnsureFontLoaded();
	HFONTX hFont = G().hFont;

	SIZE sz{};
	int len = (int)wcslen(text);
	if (multiline){
		int w = (maxWidth > 0) ? maxWidth : 320;
		XC_GetTextShowRect(text, len, hFont, textAlignFlag_left | textAlignFlag_top, w, &sz);
	} else {
		XC_GetTextShowSizeEx(text, len, hFont, textFormatFlag_NoWrap, &sz);
	}
	*outW = sz.cx;
	*outH = sz.cy;
}

//============================================================================
// 几何布局: 根据文本 + 类型 + margin 计算 body 尺寸 & 窗口尺寸 (含阴影 margin)
//============================================================================
void LayoutTooltip(const _XTip_Entry& e)
{
	auto& g = G();
	BOOL hasIcon = (e.type != xtooltip_type_default);

	// 1. 文本测量
	int textMaxW = e.multiline ? 320 : 0;   // 多行最大 320, 单行无限
	int tw = 0, th = 0;
	MeasureText(e.text.c_str(), e.multiline, textMaxW, &tw, &th);

	// 2. body 尺寸 = text + icon + 间距 + margin
	int contentW = tw + (hasIcon ? (kIconSize + kIconTextGap) : 0);
	int contentH = (hasIcon ? std::max<int>(th, kIconSize) : th);

	int bodyW = e.marginL + contentW + e.marginR;
	int bodyH = e.marginT + contentH + e.marginB;
	if (bodyW < kIconSize + e.marginL + e.marginR) bodyW = kIconSize + e.marginL + e.marginR;
	if (bodyH < kIconSize + e.marginT + e.marginB) bodyH = kIconSize + e.marginT + e.marginB;

	// 3. 窗口尺寸 = body + 阴影外圈 + 箭头侧 (箭头在哪边, 那边加 kArrowSize). showArrow=FALSE 时不预留.
	int padL = _XUITool::kShadowMargin, padT = _XUITool::kShadowMargin,
	    padR = _XUITool::kShadowMargin, padB = _XUITool::kShadowMargin;
	if (e.showArrow){
		switch (g.arrowSide){
		case _XTip_ArrowSide_Left:   padL += kArrowSize; break;
		case _XTip_ArrowSide_Right:  padR += kArrowSize; break;
		case _XTip_ArrowSide_Top:    padT += kArrowSize; break;
		case _XTip_ArrowSide_Bottom: padB += kArrowSize; break;
		default: break;
		}
	}

	g.bodyOffX = padL;
	g.bodyOffY = padT;
	g.bodyW    = bodyW;
	g.bodyH    = bodyH;
	g.winW     = padL + bodyW + padR;
	g.winH     = padT + bodyH + padB;

	// 4. icon + text 在 body 内偏移
	int innerL = g.bodyOffX + e.marginL;
	int innerT = g.bodyOffY + e.marginT;
	int innerR = g.bodyOffX + bodyW - e.marginR;
	int innerB = g.bodyOffY + bodyH - e.marginB;

	g.hasIcon  = hasIcon;
	if (hasIcon){
		g.iconOffX = innerL;
		// icon 垂直居中于内容高度
		g.iconOffY = innerT + (contentH - kIconSize) / 2;
		innerL += kIconSize + kIconTextGap;
	}

	g.textW = tw;
	g.textH = th;

	// 文本水平对齐 (单行才有意义, 多行总是左对齐 -> 用户 SetAlignH 也覆盖单行)
	int textBoxW = innerR - innerL;
	switch (e.alignH){
	case xtooltip_align_h_right:  g.textOffX = innerR - tw; break;
	case xtooltip_align_h_center: g.textOffX = innerL + (textBoxW - tw) / 2; break;
	default:                      g.textOffX = innerL; break;
	}
	// 文本垂直对齐
	int textBoxH = innerB - innerT;
	switch (e.alignV){
	case xtooltip_align_v_bottom: g.textOffY = innerB - th; break;
	case xtooltip_align_v_center: g.textOffY = innerT + (textBoxH - th) / 2; break;
	default:                      g.textOffY = innerT; break;
	}

	// 三角顶点在边上的偏移 (距气泡圆角左 / 上 至少 kArrowEdgeOffset 像素).
	if (g.arrowSide == _XTip_ArrowSide_Left || g.arrowSide == _XTip_ArrowSide_Right){
		g.arrowEdgePos = std::min<int>(std::max<int>(kArrowEdgeOffset, bodyH / 2), bodyH - kArrowEdgeOffset);
	} else if (g.arrowSide == _XTip_ArrowSide_Top || g.arrowSide == _XTip_ArrowSide_Bottom){
		g.arrowEdgePos = std::min<int>(std::max<int>(kArrowEdgeOffset, bodyW / 2), bodyW - kArrowEdgeOffset);
	}
}

//============================================================================
// 弹出位置 (基于 *元素位置* 而非鼠标位置)
//
// 输入: 源元素 hSrc, 已决出的箭头方向 side, 提示窗口物理宽高, 物理 scale.
// 输出: 提示窗口屏幕左上角 (物理像素, 给 SetWindowPos 用)
//
// 算法: 锚点 = 元素的 *对面边* 中点 (与 mouse 进入边相反, 这样 cursor 不会盖住气泡).
//       Side_Left  (tooltip 在源右侧) -> anchor = (eleR, eleMidY)
//       Side_Right                    -> anchor = (eleL, eleMidY)
//       Side_Top   (tooltip 在源下方) -> anchor = (eleMidX, eleB)
//       Side_Bottom                   -> anchor = (eleMidX, eleT)
//       winTopLeft = anchor - arrowTip_in_tooltip
//
// 元素 -> 屏幕坐标转换:
//   XEle_PointClientToWndClientDPI: 元素客户区逻辑 -> 窗口客户区物理
//   Win32 ClientToScreen:           窗口客户区物理 -> 屏幕物理
//============================================================================
void CalcTipAnchorScreen(HELE hSrc, _XTip_ArrowSide side, BOOL withArrow,
                         float scale, int* outX, int* outY)
{
	auto& g = G();
	*outX = *outY = 0;

	HWINDOW hWnd = XWidget_GetHWINDOW((HXCGUI)hSrc);
	if (!hWnd) return;
	HWND hwnd = XWnd_GetHWND(hWnd);
	if (!hwnd) return;

	int eleW = XEle_GetWidth (hSrc);
	int eleH = XEle_GetHeight(hSrc);

	// 元素 (0,0) 和 (W,H) -> 窗口客户区物理坐标
	POINT eleTL{0, 0};
	POINT eleBR{eleW, eleH};
	XEle_PointClientToWndClientDPI(hSrc, &eleTL);
	XEle_PointClientToWndClientDPI(hSrc, &eleBR);

	// 窗口客户区 -> 屏幕 (物理)
	::ClientToScreen(hwnd, &eleTL);
	::ClientToScreen(hwnd, &eleBR);

	int midX = (eleTL.x + eleBR.x) / 2;
	int midY = (eleTL.y + eleBR.y) / 2;

	// 锚点坐标 in tooltip window: withArrow=TRUE 则为三角顶点 (body 边 ± kArrowSize),
	// FALSE 则为 body 本身边, 避免关闭三角后与元素之间出现 kArrowSize 的间隔.
	int armX = withArrow ? kArrowSize : 0;
	int armY = withArrow ? kArrowSize : 0;
	int anchorX = midX, anchorY = midY;
	int tipX = 0, tipY = 0;
	switch (side){
	case _XTip_ArrowSide_Left:
		anchorX = eleBR.x;  anchorY = midY;
		tipX = RP((g.bodyOffX - armX) * scale);
		tipY = RP((g.bodyOffY + g.arrowEdgePos) * scale);
		break;
	case _XTip_ArrowSide_Right:
		anchorX = eleTL.x;  anchorY = midY;
		tipX = RP((g.bodyOffX + g.bodyW + armX) * scale);
		tipY = RP((g.bodyOffY + g.arrowEdgePos) * scale);
		break;
	case _XTip_ArrowSide_Top:
		anchorX = midX;     anchorY = eleBR.y;
		tipX = RP((g.bodyOffX + g.arrowEdgePos) * scale);
		tipY = RP((g.bodyOffY - armY) * scale);
		break;
	case _XTip_ArrowSide_Bottom:
		anchorX = midX;     anchorY = eleTL.y;
		tipX = RP((g.bodyOffX + g.arrowEdgePos) * scale);
		tipY = RP((g.bodyOffY + g.bodyH + armY) * scale);
		break;
	default:
		// 不应到这, 退化为元素下方居中.
		anchorX = midX;     anchorY = eleBR.y;
		tipX = RP((g.bodyOffX + g.arrowEdgePos) * scale);
		tipY = 0;
		break;
	}
	*outX = anchorX - tipX;
	*outY = anchorY - tipY;
}

//============================================================================
// 三角箭头方向判定
//
// 输入: 源元素客户区中鼠标位置 pt (逻辑), 元素宽高 (逻辑).
// 输出: 哪条边距离鼠标最近 (取作鼠标"进入边"的近似).
//============================================================================
_XTip_ArrowSide DetectArrowSide(POINT ptLogical, int eleW, int eleH)
{
	int dL = ptLogical.x;
	int dT = ptLogical.y;
	int dR = eleW - ptLogical.x;
	int dB = eleH - ptLogical.y;
	int dMin = (std::min)((std::min)(dL, dT), (std::min)(dR, dB));
	if (dMin == dL) return _XTip_ArrowSide_Left;
	if (dMin == dT) return _XTip_ArrowSide_Top;
	if (dMin == dR) return _XTip_ArrowSide_Right;
	return _XTip_ArrowSide_Bottom;
}

//============================================================================
// 共享气泡窗口创建 / 销毁 (懒)
//============================================================================
void DestroyTipWindow()
{
	auto& g = G();
	if (g.hTipWnd){
		// XEle_KillXCTimer 会在窗口销毁时一起被回收; 显式 kill 避免遗留 timer message.
		if (g.hTipBody){
			XEle_KillXCTimer(g.hTipBody, kTimerId_FadeAnim);
			XEle_KillXCTimer(g.hTipBody, kTimerId_AutoClose);
		}
		XWnd_DestroyWindow(g.hTipWnd);
		g.hTipWnd  = NULL;
		g.hTipBody = NULL;
		g.hTipHwnd = NULL;
	}
}

// 前置声明
int CALLBACK _XTip_OnPaint  (HELE hEle, HDRAW hDraw, BOOL* pbHandled);
int CALLBACK _XTip_OnTimer  (HELE hEle, UINT nTimerID, BOOL* pbHandled);

void EnsureTipWindow()
{
	auto& g = G();
	if (g.hTipWnd) return;

	// 创建 *无任何 chrome* 的窗口. 初始 1x1, 后续根据内容调整.
	g.hTipWnd = XWnd_Create(0, 0, 1, 1, L"", NULL, window_style_nothing);
	if (!g.hTipWnd) return;

	g.hTipHwnd = XWnd_GetHWND(g.hTipWnd);

	// 每像素 alpha 通道 + 初始透明 (避免首帧闪).
	XWnd_SetTransparentType(g.hTipWnd, window_transparent_shaped);
	XWnd_SetTransparentAlpha(g.hTipWnd, 0);
	XWnd_SetTop(g.hTipWnd, TRUE);

	// HWND 层鼠标穿透 + 不抢焦点
	if (g.hTipHwnd){
		LONG_PTR ex = ::GetWindowLongPtrW(g.hTipHwnd, GWL_EXSTYLE);
		ex |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
		::SetWindowLongPtrW(g.hTipHwnd, GWL_EXSTYLE, ex);
	}

	// body 元素 (填满窗口客户区, 接管 paint).
	g.hTipBody = XEle_Create(0, 0, 1, 1, (HXCGUI)g.hTipWnd);
	if (g.hTipBody){
		XEle_EnableBkTransparent(g.hTipBody, TRUE);   // 不让 XCGUI 画默认背景
		XEle_EnableDrawBorder   (g.hTipBody, FALSE);
		XEle_EnableDrawFocus    (g.hTipBody, FALSE);
		XEle_EnableMouseThrough (g.hTipBody, TRUE);   // XCGUI 层穿透
		XEle_RegEventC1(g.hTipBody, XE_PAINT,    (void*)&_XTip_OnPaint);
		XEle_RegEventC1(g.hTipBody, XE_XC_TIMER, (void*)&_XTip_OnTimer);
	}

	EnsureSvgsLoaded();
	EnsureFontLoaded();
}

//============================================================================
// 实际显示 / 隐藏 (含动画)
//============================================================================
void StartFadeAnim(int fromAlpha, int toAlpha, int durationMs)
{
	auto& g = G();
	if (!g.hTipBody) return;
	g.fadeFromAlpha = fromAlpha;
	g.fadeToAlpha   = toAlpha;
	g.fadeDuration  = (DWORD)(durationMs > 0 ? durationMs : 1);
	g.fadeStartTick = ::GetTickCount();
	if (durationMs <= 0){
		// 关闭动画 -> 直接到位
		XWnd_SetTransparentAlpha(g.hTipWnd, (BYTE)toAlpha);
		XWnd_Redraw(g.hTipWnd);
		g.state = (toAlpha == 0) ? _XTip_State_Hidden : _XTip_State_Shown;
		if (toAlpha == 0){
			::ShowWindow(g.hTipHwnd, SW_HIDE);
			g.hCurSrc = NULL;
		}
		return;
	}
	XEle_SetXCTimer(g.hTipBody, kTimerId_FadeAnim, kFadeTickMs);
}

void HideTipImmediate()
{
	auto& g = G();
	if (!g.hTipWnd) return;
	XEle_KillXCTimer(g.hTipBody, kTimerId_FadeAnim);
	XEle_KillXCTimer(g.hTipBody, kTimerId_AutoClose);
	XWnd_SetTransparentAlpha(g.hTipWnd, 0);
	if (g.hTipHwnd) ::ShowWindow(g.hTipHwnd, SW_HIDE);
	g.state    = _XTip_State_Hidden;
	g.hCurSrc  = NULL;
}

void BeginHide()
{
	auto& g = G();
	if (g.state == _XTip_State_Hidden || g.state == _XTip_State_FadeOut) return;
	const auto it = g.registry.find(g.hCurSrc);
	int fadeMs = (it != g.registry.end()) ? it->second.fadeMs : kDefaultFadeMs;
	g.state = _XTip_State_FadeOut;
	XEle_KillXCTimer(g.hTipBody, kTimerId_AutoClose);
	StartFadeAnim(255, 0, fadeMs);
}

void ShowTipFor(HELE hSrc)
{
	auto& g = G();
	auto it = g.registry.find(hSrc);
	if (it == g.registry.end()) return;
	const _XTip_Entry& e = it->second;
	if (e.text.empty()) return;   // 没文本不显示, 但保留注册

	EnsureTipWindow();
	if (!g.hTipWnd || !g.hTipBody) return;

	// 1) 取鼠标当前位置 (element 局部, 逻辑) -> 决定箭头方向
	//
	//    XCGUI 没有"取元素 hover 时鼠标位置"现成 API, 走 Win32: 屏幕 -> client -> 除 DPI.
	HWINDOW hSrcWnd = XWidget_GetHWINDOW((HXCGUI)hSrc);
	int dpi = XWnd_GetDPI(hSrcWnd);
	if (dpi <= 0) dpi = 96;
	float scale = dpi / 96.0f;

	POINT mousePt{0, 0};
	::GetCursorPos(&mousePt);
	HWND hSrcHwnd = XWnd_GetHWND(hSrcWnd);
	if (hSrcHwnd){
		::ScreenToClient(hSrcHwnd, &mousePt);
		// 物理 -> 逻辑 (XEle_PointClientToWndClientDPI / DetectArrowSide 用逻辑)
		mousePt.x = RP(mousePt.x / scale);
		mousePt.y = RP(mousePt.y / scale);
		// 进一步: 鼠标在窗口客户区逻辑 -> 元素客户区逻辑
		// XCGUI 暂无反向 (WndClient -> EleClient), 自己减:
		RECT eleRcInWnd{};
		XEle_GetWndClientRect(hSrc, &eleRcInWnd);  // 元素在窗口客户区的逻辑坐标
		mousePt.x -= eleRcInWnd.left;
		mousePt.y -= eleRcInWnd.top;
	}

	int eleW = XEle_GetWidth (hSrc);
	int eleH = XEle_GetHeight(hSrc);
	// 强制锚定优先: SetArrowSide 设了固定方向就用, 否则按鼠标进入边判断 (现状默认).
	// 公开 enum -> 内部 enum 是 1:1 映射 (left=1, right=2, top=3, bottom=4).
	if (e.arrowSide != xtooltip_arrow_side_auto){
		g.arrowSide = (_XTip_ArrowSide)e.arrowSide;
	} else {
		g.arrowSide = DetectArrowSide(mousePt, eleW, eleH);
	}

	// 2) 布局
	LayoutTooltip(e);

	int winWPhys = RP(g.winW * scale);
	int winHPhys = RP(g.winH * scale);

	// 3) 算屏幕位置 (锚点 = 元素对应边中点, 不是鼠标)
	int wx = 0, wy = 0;
	CalcTipAnchorScreen(hSrc, g.arrowSide, e.showArrow, scale, &wx, &wy);

	// 4) 文本色 *必须* 在窗口可见 / SetSize 之前写入元素 (那些操作会同步触发 paint).
	//    body 默认无 TextColor -> 走 XCGUI 系统默认 (经常是黑/白随主题), 在透明窗口
	//    + 暗背景上看起来就是"奇怪的颜色". 提前 set 一次, 后续 paint 直接复用.
	{
		COLORREF cText, cBg;
		ResolveColors(e, &cText, &cBg);
		(void)cBg;
		XEle_SetTextColor(g.hTipBody, cText);
		XEle_SetSize(g.hTipBody, g.winW, g.winH, FALSE);
	}

	// 5) SetWindowPos (物理) + XWnd_AdjustInScreen 防越界
	if (g.hTipHwnd){
		::SetWindowPos(g.hTipHwnd, HWND_TOPMOST, wx, wy,
			winWPhys, winHPhys, SWP_NOACTIVATE | SWP_NOSENDCHANGING);
		// XWnd_AdjustInScreen 会自动挪到当前显示器范围内. nBorderSpace=0,
		// bCoverTaskBar=FALSE: 保留任务栏空间.
		XWnd_AdjustInScreen(g.hTipWnd, 0, FALSE);
	}

	g.hCurSrc = hSrc;

	// 6) 显示 + fade-in
	if (g.hTipHwnd && !::IsWindowVisible(g.hTipHwnd)){
		XWnd_SetTransparentAlpha(g.hTipWnd, 0);
		::ShowWindow(g.hTipHwnd, SW_SHOWNOACTIVATE);
	}
	g.state = _XTip_State_FadeIn;
	StartFadeAnim(0, 255, e.fadeMs);

	if (e.autoCloseMs > 0){
		g.autoCloseAt = ::GetTickCount() + (DWORD)e.autoCloseMs;
		XEle_SetXCTimer(g.hTipBody, kTimerId_AutoClose, (UINT)e.autoCloseMs);
	} else {
		g.autoCloseAt = 0;
	}

	XWnd_Redraw(g.hTipWnd);
}

//============================================================================
// OnPaint (body 元素)
//
// 画顺序:
//   1) 多层圆角矩形 阴影 halo (向外扩, 递减 alpha)
//   2) 圆角矩形 实心背景 (bg color)
//   3) 1px 内描边 #0F0F0F (CSS L5)
//   4) 三角箭头 (与背景同色 -> 自然 "连接" body)
//   5) 图标 (SVG)
//   6) 文本
//============================================================================
void DrawTriangle(HDRAW hDraw, _XTip_ArrowSide side, int bodyL, int bodyT, int bodyR, int bodyB,
                  int edgePos, int arrowSize, COLORREF bgColor)
{
	POINT pts[3]{};
	switch (side){
	case _XTip_ArrowSide_Left:
		pts[0] = { bodyL,             bodyT + edgePos - arrowSize };
		pts[1] = { bodyL,             bodyT + edgePos + arrowSize };
		pts[2] = { bodyL - arrowSize, bodyT + edgePos              };
		break;
	case _XTip_ArrowSide_Right:
		pts[0] = { bodyR,             bodyT + edgePos - arrowSize };
		pts[1] = { bodyR,             bodyT + edgePos + arrowSize };
		pts[2] = { bodyR + arrowSize, bodyT + edgePos              };
		break;
	case _XTip_ArrowSide_Top:
		pts[0] = { bodyL + edgePos - arrowSize, bodyT };
		pts[1] = { bodyL + edgePos + arrowSize, bodyT };
		pts[2] = { bodyL + edgePos,             bodyT - arrowSize };
		break;
	case _XTip_ArrowSide_Bottom:
		pts[0] = { bodyL + edgePos - arrowSize, bodyB };
		pts[1] = { bodyL + edgePos + arrowSize, bodyB };
		pts[2] = { bodyL + edgePos,             bodyB + arrowSize };
		break;
	default: return;
	}
	XDraw_SetBrushColor(hDraw, bgColor);
	XDraw_FillPolygon(hDraw, pts, 3);
}

int CALLBACK _XTip_OnPaint(HELE hEle, HDRAW hDraw, BOOL* pbHandled)
{
	auto& g = G();
	if (pbHandled) *pbHandled = TRUE;
	if (!g.hCurSrc) return 0;

	auto it = g.registry.find(g.hCurSrc);
	if (it == g.registry.end()) return 0;
	const _XTip_Entry& e = it->second;

	COLORREF cText, cBg;
	ResolveColors(e, &cText, &cBg);

	// body 矩形 (像素 = 逻辑 单位, XCGUI 内部会按 DPI 缩到物理)
	RECT bodyRc{ g.bodyOffX, g.bodyOffY,
	             g.bodyOffX + g.bodyW, g.bodyOffY + g.bodyH };

	// 1) 阴影: 浅色主题用更淡的不透明度, 深色/自定义保持原强度.
	{
		RECTF rsF{
			(float)bodyRc.left,
			(float)bodyRc.top,
			(float)bodyRc.right,
			(float)bodyRc.bottom
		};
		_XUITool::DrawDropShadow(hDraw, rsF, (float)_XUITool::kCornerRadius, e.theme);
	}

	// 2) 三角 (画在 body 实心之前, 这样三角与 body 自然融为一体, 圆角处不切断)
	if (e.showArrow && g.arrowSide != _XTip_ArrowSide_None){
		DrawTriangle(hDraw, g.arrowSide,
			bodyRc.left, bodyRc.top, bodyRc.right, bodyRc.bottom,
			g.arrowEdgePos, kArrowSize, cBg);
	}

	// 3) body 圆角实心背景 (无边框 - 用户要求)
	XDraw_SetBrushColor(hDraw, cBg);
	XDraw_FillRoundRect(hDraw, &bodyRc, _XUITool::kCornerRadius, _XUITool::kCornerRadius);

	// 4) 图标 (SVG)
	//    用 XDraw_DrawSvg (基于 XSvg_SetSize 设的内禀尺寸), 不用 XDraw_DrawSvgEx —
	//    后者在 DPI != 100% 时会有重采样 ribbon 伪影 (XCGUI 内部把 dst w/h 与 svg
	//    intrinsic 都缩, 算两遍 DPI). DrawSvg 让 XCGUI 自己处理 DPI -> 干净.
	if (g.hasIcon){
		HSVG hSvg = SvgForType(e.type);
		if (hSvg){
			XSvg_SetSize(hSvg, kIconSize, kIconSize);
			XDraw_DrawSvg(hDraw, hSvg, g.iconOffX, g.iconOffY);
		}
	}

	// 5) 文本
	//    重要 — *XDraw_DrawText 的取色优先级*:
	//      1. 当前 BRUSH 颜色 (XDraw_SetBrushColor)  ← 真正起作用的
	//      2. 元素自身 TextColor (XEle_SetTextColor) ← 仅对部分 widget (按钮等) 生效;
	//                                                 对纯 XEle_Create 的元素无效
	//    用户报 "浅色文本几乎看不见" -> 因为我们只 SetTextColor 没 SetBrushColor,
	//    刚才画 body 背景把 brush 留在了 cBg (浅色主题=白), 然后 DrawText 就用白色
	//    画在白底上, 几乎不可见. 修复: 文本绘制前显式 SetBrushColor(cText).
	//
	//    AntiAliasGridFit 灰度 AA: 透明窗口 (per-pixel alpha) + ClearType 子像素 AA
	//    会产生彩色边 — 强制灰度 AA 解决.
	XDraw_SetTextRenderingHint(hDraw, 3 /* TextRenderingHintAntiAliasGridFit */);
	XDraw_SetBrushColor(hDraw, cText);
	(void)hEle;
	if (g.hFont) XDraw_SetFont(hDraw, g.hFont);
	int align = textAlignFlag_left | textAlignFlag_top;
	if (!e.multiline) align |= textFormatFlag_NoWrap;
	XDraw_SetTextAlign(hDraw, align);
	// rect 高/宽 多 +2 防边界字 sub-pixel 截断
	RECT textRc{ g.textOffX, g.textOffY,
	             g.textOffX + g.textW + 2, g.textOffY + g.textH + 2 };
	XDraw_DrawText(hDraw, e.text.c_str(), (int)e.text.size(), &textRc);

	return 0;
}

//============================================================================
// OnTimer (body 元素) - fade 动画 + 自动关闭
//============================================================================
int CALLBACK _XTip_OnTimer(HELE hEle, UINT nTimerID, BOOL* pbHandled)
{
	auto& g = G();
	if (pbHandled) *pbHandled = TRUE;

	if (nTimerID == kTimerId_FadeAnim){
		DWORD now = ::GetTickCount();
		DWORD elapsed = now - g.fadeStartTick;
		BYTE alpha;
		BOOL finished = FALSE;
		if (elapsed >= g.fadeDuration){
			alpha = (BYTE)g.fadeToAlpha;
			finished = TRUE;
		} else {
			float t = (float)elapsed / (float)g.fadeDuration;
			// 简单线性 (后续可换 ease-out)
			int v = g.fadeFromAlpha + RP((g.fadeToAlpha - g.fadeFromAlpha) * t);
			if (v < 0) v = 0; if (v > 255) v = 255;
			alpha = (BYTE)v;
		}
		if (g.hTipWnd){
			XWnd_SetTransparentAlpha(g.hTipWnd, alpha);
			XWnd_Redraw(g.hTipWnd);
		}
		if (finished){
			XEle_KillXCTimer(g.hTipBody, kTimerId_FadeAnim);
			if (g.fadeToAlpha == 0){
				if (g.hTipHwnd) ::ShowWindow(g.hTipHwnd, SW_HIDE);
				g.state   = _XTip_State_Hidden;
				g.hCurSrc = NULL;
			} else {
				g.state = _XTip_State_Shown;
			}
		}
		return 0;
	}

	if (nTimerID == kTimerId_AutoClose){
		XEle_KillXCTimer(g.hTipBody, kTimerId_AutoClose);
		BeginHide();
		return 0;
	}

	return 0;
}

//============================================================================
// 源元素事件 (鼠标 进入 / 移动 / 离开 / 销毁) - C 风格 静态 thunk
//============================================================================
int CALLBACK _XTip_OnSrcMouseStay(HELE hEle, BOOL* pbHandled)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	if (it == g.registry.end()) return 0;
	// 倒计时 showDelay 后显示
	XEle_SetXCTimer(hEle, kTimerId_ShowDelay, (UINT)std::max<int>(1, it->second.showDelayMs));
	return 0;
}

int CALLBACK _XTip_OnSrcMouseMove(HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled)
{
	// 鼠标在元素内移动: 仅当气泡 *未* 显示时, 我们用这个更新 "鼠标最后位置" 给 ShowTipFor
	// 拿. 如果气泡已显示, 用户希望气泡保持位置不抖动, 所以不重定位.
	// 实际上 ShowTipFor 直接调 GetCursorPos, 这里不需要缓存 — 留空, 但保留挂点避免空 callback 引起 XCGUI 警告.
	(void)hEle; (void)nFlags; (void)pPt;
	if (pbHandled) *pbHandled = FALSE;   // 不消费, 继续传给其他 handler
	return 0;
}

int CALLBACK _XTip_OnSrcMouseLeave(HELE hEle, HELE hEleStay, BOOL* pbHandled)
{
	auto& g = G();
	// 取消 pending show-delay
	XEle_KillXCTimer(hEle, kTimerId_ShowDelay);
	// 如果当前显示的就是这个元素, 触发隐藏
	if (g.hCurSrc == hEle){
		BeginHide();
	}
	return 0;
}

int CALLBACK _XTip_OnSrcDestroy(HELE hEle, BOOL* pbHandled)
{
	auto& g = G();
	XEle_KillXCTimer(hEle, kTimerId_ShowDelay);
	g.registry.erase(hEle);
	if (g.hCurSrc == hEle){
		HideTipImmediate();
	}
	// 关键 — 修复 "主窗关闭后进程不退出":
	//   XRunXCGUI() 退出条件 = 所有炫彩窗口数为 0. 我们的共享气泡窗口若不主动销毁,
	//   就会拖住消息循环, 导致主进程无法结束. 这里在 *最后一个* 源元素销毁时连带
	//   销毁共享气泡 (含释放 SVG + 字体可放到 XExitXCGUI 前的 Cleanup 里, 这里只
	//   销毁窗口本身防卡死).
	if (g.registry.empty()){
		DestroyTipWindow();
	}
	return 0;
}

// 源元素 XE_XC_TIMER (show-delay 触发)
int CALLBACK _XTip_OnSrcTimer(HELE hEle, UINT nTimerID, BOOL* pbHandled)
{
	if (nTimerID == kTimerId_ShowDelay){
		XEle_KillXCTimer(hEle, kTimerId_ShowDelay);
		ShowTipFor(hEle);
	}
	return 0;
}

//============================================================================
// 注册时挂事件 (幂等)
//============================================================================
void EnsureSrcEventsHooked(HELE hEle, _XTip_Entry& e)
{
	if (e.eventsHooked) return;
	XEle_RegEventC1(hEle, XE_MOUSESTAY,  (void*)&_XTip_OnSrcMouseStay);
	XEle_RegEventC1(hEle, XE_MOUSEMOVE,  (void*)&_XTip_OnSrcMouseMove);
	XEle_RegEventC1(hEle, XE_MOUSELEAVE, (void*)&_XTip_OnSrcMouseLeave);
	XEle_RegEventC1(hEle, XE_DESTROY,    (void*)&_XTip_OnSrcDestroy);
	XEle_RegEventC1(hEle, XE_XC_TIMER,   (void*)&_XTip_OnSrcTimer);
	e.eventsHooked = TRUE;
}

//============================================================================
// _GetOrCreate: 所有 Set* 公开接口的统一入口.
//
// 旧实现里每个 Set* 都先 find() 判 end()->return FALSE, 这导致一个常见误用:
//     CXTooltip::SetTheme(h, light);   // entry 还没建, 此调用静默失败!
//     CXTooltip::AddEleTip(h, L"...");// 默认 dark 主题 entry, 用户看到深色
// 用户报告的 "SetTheme 浅色无效" 即此. 这里改为: Set* 调用即按需建 entry
// (+ 挂源元素事件), 让 Add/Set 的相对顺序无关紧要.
//
// 返回 NULL 仅当 hEle 不是合法 XCGUI 元素.
//============================================================================
_XTip_Entry* _GetOrCreate(HELE hEle)
{
	if (!XC_IsHELE((HXCGUI)hEle)) return NULL;
	auto& g = G();
	auto& e = g.registry[hEle];   // operator[] 缺省构造
	EnsureSrcEventsHooked(hEle, e);
	return &e;
}

}  // anonymous namespace

//============================================================================
// 公开接口实现 (CXTooltip::*)
//============================================================================

BOOL CXTooltip::AddEleTip(HELE hEle, const wchar_t* pText)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->text = (pText ? pText : L"");
	return TRUE;
}

BOOL CXTooltip::DelEleTip(HELE hEle)
{
	if (!XC_IsHELE((HXCGUI)hEle)) return FALSE;
	auto& g = G();
	auto it = g.registry.find(hEle);
	if (it == g.registry.end()) return FALSE;
	// 当前正显示这个: 立即隐藏
	if (g.hCurSrc == hEle) HideTipImmediate();
	XEle_KillXCTimer(hEle, kTimerId_ShowDelay);
	// 反挂源事件 — XEle_RemoveEventC 对 C 与 C1 注册的函数都通用 (XCGUI 不区分
	// removal API). 镜像 EnsureSrcEventsHooked 注册的 5 个事件.
	XEle_RemoveEventC(hEle, XE_MOUSESTAY,  (void*)&_XTip_OnSrcMouseStay);
	XEle_RemoveEventC(hEle, XE_MOUSEMOVE,  (void*)&_XTip_OnSrcMouseMove);
	XEle_RemoveEventC(hEle, XE_MOUSELEAVE, (void*)&_XTip_OnSrcMouseLeave);
	XEle_RemoveEventC(hEle, XE_DESTROY,    (void*)&_XTip_OnSrcDestroy);
	XEle_RemoveEventC(hEle, XE_XC_TIMER,   (void*)&_XTip_OnSrcTimer);
	g.registry.erase(it);
	return TRUE;
}

BOOL CXTooltip::HasTip(HELE hEle)
{
	if (!XC_IsHELE((HXCGUI)hEle)) return FALSE;
	return G().registry.count(hEle) > 0 ? TRUE : FALSE;
}

BOOL CXTooltip::SetText(HELE hEle, const wchar_t* pText)
{
	return AddEleTip(hEle, pText);
}

const wchar_t* CXTooltip::GetText(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	if (it == g.registry.end()) return NULL;
	return it->second.text.c_str();
}

BOOL CXTooltip::SetType(HELE hEle, xtooltip_type_ type)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->type = type;
	return TRUE;
}

xtooltip_type_ CXTooltip::GetType(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? xtooltip_type_default : it->second.type;
}

BOOL CXTooltip::SetMultiline(HELE hEle, BOOL bMultiline)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->multiline = bMultiline ? TRUE : FALSE;
	// 切换 multiline 同时调整默认对齐 (用户后续可显式 SetAlignH/V 覆盖)
	if (bMultiline){
		p->alignH = xtooltip_align_h_left;
		p->alignV = xtooltip_align_v_top;
	} else {
		p->alignH = xtooltip_align_h_center;
		p->alignV = xtooltip_align_v_center;
	}
	return TRUE;
}

BOOL CXTooltip::IsMultiline(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it != g.registry.end()) ? it->second.multiline : FALSE;
}

BOOL CXTooltip::SetAlignH(HELE hEle, xtooltip_align_h_ align)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->alignH = align;
	return TRUE;
}

xtooltip_align_h_ CXTooltip::GetAlignH(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? xtooltip_align_h_center : it->second.alignH;
}

BOOL CXTooltip::SetAlignV(HELE hEle, xtooltip_align_v_ align)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->alignV = align;
	return TRUE;
}

xtooltip_align_v_ CXTooltip::GetAlignV(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? xtooltip_align_v_center : it->second.alignV;
}

BOOL CXTooltip::SetArrowSide(HELE hEle, xtooltip_arrow_side_ side)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	// 边界保护: 越界值收敛到 auto
	if (side < xtooltip_arrow_side_auto || side > xtooltip_arrow_side_bottom){
		side = xtooltip_arrow_side_auto;
	}
	p->arrowSide = side;
	return TRUE;
}

xtooltip_arrow_side_ CXTooltip::GetArrowSide(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? xtooltip_arrow_side_auto : it->second.arrowSide;
}

BOOL CXTooltip::SetShowArrow(HELE hEle, BOOL bShow)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->showArrow = bShow ? TRUE : FALSE;
	return TRUE;
}

BOOL CXTooltip::GetShowArrow(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? TRUE : it->second.showArrow;
}

BOOL CXTooltip::SetTheme(HELE hEle, xuitool_theme_ theme)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->theme = theme;
	return TRUE;
}

xuitool_theme_ CXTooltip::GetTheme(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? xuitool_theme_dark : it->second.theme;
}

BOOL CXTooltip::SetTextColor(HELE hEle, COLORREF color)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->customText = color;
	p->theme = xuitool_theme_custom;
	return TRUE;
}

COLORREF CXTooltip::GetTextColor(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	if (it == g.registry.end()) return _XUITool::kDarkText;
	COLORREF t, b;
	ResolveColors(it->second, &t, &b);
	return t;
}

BOOL CXTooltip::SetBkColor(HELE hEle, COLORREF color)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->customBg = color;
	p->theme = xuitool_theme_custom;
	return TRUE;
}

COLORREF CXTooltip::GetBkColor(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	if (it == g.registry.end()) return _XUITool::kDarkBg;
	COLORREF t, b;
	ResolveColors(it->second, &t, &b);
	return b;
}

BOOL CXTooltip::SetMargin(HELE hEle, int l, int t, int r, int b)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->marginL = l < 0 ? 0 : l;
	p->marginT = t < 0 ? 0 : t;
	p->marginR = r < 0 ? 0 : r;
	p->marginB = b < 0 ? 0 : b;
	return TRUE;
}

void CXTooltip::GetMargin(HELE hEle, int* pl, int* pt, int* pr, int* pb)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	if (it == g.registry.end()){
		if (pl) *pl = kDefaultMarginL;
		if (pt) *pt = kDefaultMarginT;
		if (pr) *pr = kDefaultMarginR;
		if (pb) *pb = kDefaultMarginB;
		return;
	}
	if (pl) *pl = it->second.marginL;
	if (pt) *pt = it->second.marginT;
	if (pr) *pr = it->second.marginR;
	if (pb) *pb = it->second.marginB;
}

BOOL CXTooltip::SetShowDelay(HELE hEle, int ms)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->showDelayMs = ms < 0 ? 0 : ms;
	return TRUE;
}

int CXTooltip::GetShowDelay(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? kDefaultShowDelayMs : it->second.showDelayMs;
}

BOOL CXTooltip::SetAutoCloseTime(HELE hEle, int ms)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->autoCloseMs = ms < 0 ? 0 : ms;
	return TRUE;
}

int CXTooltip::GetAutoCloseTime(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? kDefaultAutoCloseMs : it->second.autoCloseMs;
}

BOOL CXTooltip::SetFadeDuration(HELE hEle, int ms)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->fadeMs = ms < 0 ? 0 : ms;
	return TRUE;
}

int CXTooltip::GetFadeDuration(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? kDefaultFadeMs : it->second.fadeMs;
}

void CXTooltip::Cleanup()
{
	HideTipImmediate();
	DestroyTipWindow();
	DestroySvgs();
	auto& g = G();
	if (g.hFont){ XFont_Destroy(g.hFont); g.hFont = NULL; }
	// 反挂所有源元素事件 (一次性卸载, 用户 XInitXCGUI 关闭前调用本接口最稳妥).
	// 元素已 destroy 的 entry 也能安全调 (XEle_RemoveEventC 内部判合法).
	for (auto& kv : g.registry){
		HELE h = kv.first;
		XEle_RemoveEventC(h, XE_MOUSESTAY,  (void*)&_XTip_OnSrcMouseStay);
		XEle_RemoveEventC(h, XE_MOUSEMOVE,  (void*)&_XTip_OnSrcMouseMove);
		XEle_RemoveEventC(h, XE_MOUSELEAVE, (void*)&_XTip_OnSrcMouseLeave);
		XEle_RemoveEventC(h, XE_DESTROY,    (void*)&_XTip_OnSrcDestroy);
		XEle_RemoveEventC(h, XE_XC_TIMER,   (void*)&_XTip_OnSrcTimer);
	}
	g.registry.clear();
}
