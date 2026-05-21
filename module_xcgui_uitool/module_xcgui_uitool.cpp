//============================================================================
// module_xcgui_uitool.cpp
//
// XCGUI UI 工具集 v1.0.0 — 多个 *顶层* 工具类共存的模块文件.
// 当前实现的类:
//   - CXTooltip  : 鼠标悬停气泡提示 (本文件 100% 是它的逻辑).
// 后续会逐步加入 CXToast / CXPopover / CXMenu 等顶层类 (各自独立 class,
// 不嵌套, 这样 XCGUI 的 //@别名 工具能逐个识别).
//
// 设计原则:
//   - 全静态方法, 不创建实例. 用户在程序任意时机调用
//     CXTooltip::AddEleTip(hEle, L"...") 注册.
//   - 单例的"共享气泡窗口"在首次显示时懒创建, Cleanup() 显式销毁.
//     同一时刻至多 1 个气泡, 切换元素时直接复用窗口 (新位置 + 重新淡入).
//   - 注册表 std::unordered_map<HELE, Entry>:
//       - AddEleTip 首次: 在源元素挂 XE_MOUSESTAY / XE_MOUSEMOVE / XE_MOUSELEAVE
//                       / XE_DESTROY / XE_XC_TIMER 5 个 C1 风格事件.
//       - 重复 AddEleTip: 仅更新文本, 事件已挂不重复挂.
//       - DelEleTip / Cleanup 同步反挂事件: XEle_RemoveEventC 对 C 与 C1 注册
//         的函数都通用 (XCGUI 不提供独立的 RemoveEventC1).
//       - 事件回调先从注册表查询; 元素 destroy 时 XCGUI 也会自动清, 双保险.
//
//   - 渲染: 共享气泡窗口的 body 元素 XE_PAINT 接管:
//       D2D 主路径 (XCGUI XInitXCGUI(TRUE)) — XEle_GetWndClientRectDPI 直接物理.
//       GDI 兜底 — XDraw_ConvRect 转坐标 + 离屏 DIB 防闪烁 (本模块场景轻, 直绘
//       够用, 不做 DIB).
//
//   - 阴影: window_transparent_shaped 模式下 body 自行画多层圆角矩形堆叠
//          近似 CSS 5 层 box-shadow, 不依赖 XWnd_SetShadowInfo (其需要
//          window_transparent_shadow 类型, 与本类透明语义冲突).
//
//   - 鼠标穿透:
//       1) XEle_EnableMouseThrough(body, TRUE)  — XCGUI 层穿透
//       2) HWND 加 WS_EX_TRANSPARENT + WS_EX_NOACTIVATE  — Win32 层穿透 + 不抢焦点
//      (两层都要做, 缺一不可)
//
//   - 弹出位置: 严格照搬用户给的伪码公式 (XWnd_GetRect -> RectToDPI ->
//     PointToDPI -> PointClientToWndClientDPI -> sum + SetPosition).
//
//   - 渐变动画: body 元素 XE_XC_TIMER 16ms 心跳, 按 (GetTickCount - tStart)/fadeMs
//     插值 alpha 0..255, 调 XWnd_SetTransparentAlpha. tick 完直接停 timer.

#include "module_xcgui_uitool.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "D2d1.lib")

#ifndef SafeRelease
#define SafeRelease(p) do { if (p) { (p)->Release(); (p) = NULL; } } while (0)
#endif

// SVG 字面量 (4 个图标: success / info / warning / error)
#include "module_xcgui_uitool_svgs.inc"

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
constexpr int  kDefaultShowDelayMs  = 500;
constexpr int  kDefaultAutoCloseMs  = 0;        // 0 = 不自动关闭
constexpr int  kDefaultFadeMs       = 150;
constexpr int  kDefaultMarginL      = 16;
constexpr int  kDefaultMarginT      = 10;
constexpr int  kDefaultMarginR      = 16;
constexpr int  kDefaultMarginB      = 10;

// 字体: 微软雅黑 10pt
constexpr int  kFontSize            = 10;

// 圆角
constexpr int  kCornerRadius        = 8;

// 图标尺寸 (与 1~4.svg viewBox 16x16 一致)
constexpr int  kIconSize            = 16;
constexpr int  kIconTextGap         = 8;        // 图标与文本之间的间距

// 阴影 (body 外围预留, CSS 5 层 box-shadow 衰减空间)
constexpr int  kShadowMargin        = 12;

// 三角箭头
constexpr int  kArrowSize           = 7;        // 三角的"半宽" (像素), 即从底边到顶点的距离
constexpr int  kArrowEdgeOffset     = 20;       // 三角顶点距气泡圆角的最近距离

// 预设主题颜色 (XCGUI COLORREF = ARGB, 0xAABBGGRR)
constexpr COLORREF kTheme_DarkText  = ((COLORREF)0xFF << 24) | (0xF5 << 16) | (0xF5 << 8) | 0xF5;
constexpr COLORREF kTheme_DarkBg    = ((COLORREF)0xFF << 24) | (0x17 << 16) | (0x17 << 8) | 0x17;
constexpr COLORREF kTheme_LightText = ((COLORREF)0xFF << 24) | (0x17 << 16) | (0x17 << 8) | 0x17;
constexpr COLORREF kTheme_LightBg   = ((COLORREF)0xFF << 24) | (0xFF << 16) | (0xFF << 8) | 0xFF;

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
	xtooltip_theme_     theme          = xtooltip_theme_dark;
	BOOL                multiline      = FALSE;
	xtooltip_align_h_   alignH         = xtooltip_align_h_center;
	xtooltip_align_v_   alignV         = xtooltip_align_v_center;
	xtooltip_arrow_side_ arrowSide     = xtooltip_arrow_side_auto;
	COLORREF            customText     = kTheme_DarkText;
	COLORREF            customBg       = kTheme_DarkBg;
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

// 调整 alpha (保持 RGB)
inline COLORREF WithAlpha(COLORREF c, BYTE a){
	return (c & 0x00FFFFFF) | ((COLORREF)a << 24);
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
BOOL DetectSystemDarkMode()
{
	// 读 HKCU\...\Personalize\AppsUseLightTheme. 0 = 深色, 1 = 浅色.
	// 失败时默认深色 (匹配本模块默认主题).
	HKEY hKey = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		0, KEY_READ, &hKey) != ERROR_SUCCESS) return TRUE;
	DWORD val = 1, size = sizeof(val), type = REG_DWORD;
	BOOL dark = TRUE;
	if (RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, &type,
		(LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD){
		dark = (val == 0);
	}
	RegCloseKey(hKey);
	return dark;
}

void ResolveColors(const _XTip_Entry& e, COLORREF* outText, COLORREF* outBg)
{
	xtooltip_theme_ effective = e.theme;
	if (effective == xtooltip_theme_auto){
		effective = DetectSystemDarkMode() ? xtooltip_theme_dark : xtooltip_theme_light;
	}
	switch (effective){
	case xtooltip_theme_light:
		*outText = kTheme_LightText; *outBg = kTheme_LightBg; break;
	case xtooltip_theme_custom:
		*outText = e.customText;     *outBg = e.customBg;     break;
	case xtooltip_theme_dark:
	default:
		*outText = kTheme_DarkText;  *outBg = kTheme_DarkBg;  break;
	}
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

	// 3. 窗口尺寸 = body + 阴影外圈 + 箭头侧 (箭头在哪边, 那边加 kArrowSize)
	int padL = kShadowMargin, padT = kShadowMargin, padR = kShadowMargin, padB = kShadowMargin;
	switch (g.arrowSide){
	case _XTip_ArrowSide_Left:   padL += kArrowSize; break;
	case _XTip_ArrowSide_Right:  padR += kArrowSize; break;
	case _XTip_ArrowSide_Top:    padT += kArrowSize; break;
	case _XTip_ArrowSide_Bottom: padB += kArrowSize; break;
	default: break;
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
void CalcTipAnchorScreen(HELE hSrc, _XTip_ArrowSide side,
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

	// 三角顶点 in tooltip window 坐标 (逻辑 -> 物理).
	// 注: bodyOffX/Y 已包含 arrow 一侧的 kArrowSize 内 (LayoutTooltip 时算的).
	// 所以箭头尖端 = body 边 + 反向 kArrowSize.
	int anchorX = midX, anchorY = midY;
	int tipX = 0, tipY = 0;
	switch (side){
	case _XTip_ArrowSide_Left:
		anchorX = eleBR.x;  anchorY = midY;
		tipX = RP((g.bodyOffX - kArrowSize) * scale);
		tipY = RP((g.bodyOffY + g.arrowEdgePos) * scale);
		break;
	case _XTip_ArrowSide_Right:
		anchorX = eleTL.x;  anchorY = midY;
		tipX = RP((g.bodyOffX + g.bodyW + kArrowSize) * scale);
		tipY = RP((g.bodyOffY + g.arrowEdgePos) * scale);
		break;
	case _XTip_ArrowSide_Top:
		anchorX = midX;     anchorY = eleBR.y;
		tipX = RP((g.bodyOffX + g.arrowEdgePos) * scale);
		tipY = RP((g.bodyOffY - kArrowSize) * scale);
		break;
	case _XTip_ArrowSide_Bottom:
		anchorX = midX;     anchorY = eleTL.y;
		tipX = RP((g.bodyOffX + g.arrowEdgePos) * scale);
		tipY = RP((g.bodyOffY + g.bodyH + kArrowSize) * scale);
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
	CalcTipAnchorScreen(hSrc, g.arrowSide, scale, &wx, &wy);

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

	// 1) 阴影 halo: 4 层逐渐外扩 + alpha 递减, 近似 CSS 5 层 box-shadow.
	//    CSS 数据:
	//      L1 (31,31,31, 1%)  16y 8b
	//      L2 (31,31,31, 4%)  12y 6b
	//      L3 (31,31,31, 7%)  4y  4b
	//      L4 (31,31,31, 8%)  1.5y 3b
	//    我们用同心扩张矩形堆叠, 每层 alpha = "blur 半径内的平均不透明度".
	struct ShadowLayer { int dx, dy, expand; BYTE alpha; };
	static const ShadowLayer kLayers[4] = {
		{0, 16, 8,  3 },   // ~1% * 60 范围估算
		{0, 12, 6,  10},   // ~4%
		{0,  4, 4,  18},   // ~7%
		{0,  2, 3,  20},   // ~8%
	};
	for (const auto& L : kLayers){
		RECT rs{ bodyRc.left   - L.expand + L.dx,
		         bodyRc.top    - L.expand + L.dy,
		         bodyRc.right  + L.expand + L.dx,
		         bodyRc.bottom + L.expand + L.dy };
		COLORREF cShadow = ARGB(31, 31, 31, L.alpha);
		XDraw_SetBrushColor(hDraw, cShadow);
		XDraw_FillRoundRect(hDraw, &rs,
			kCornerRadius + L.expand, kCornerRadius + L.expand);
	}

	// 2) 三角 (画在 body 实心之前, 这样三角与 body 自然融为一体, 圆角处不切断)
	if (g.arrowSide != _XTip_ArrowSide_None){
		DrawTriangle(hDraw, g.arrowSide,
			bodyRc.left, bodyRc.top, bodyRc.right, bodyRc.bottom,
			g.arrowEdgePos, kArrowSize, cBg);
	}

	// 3) body 圆角实心背景 (无边框 - 用户要求)
	XDraw_SetBrushColor(hDraw, cBg);
	XDraw_FillRoundRect(hDraw, &bodyRc, kCornerRadius, kCornerRadius);

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

BOOL CXTooltip::SetTheme(HELE hEle, xtooltip_theme_ theme)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->theme = theme;
	return TRUE;
}

xtooltip_theme_ CXTooltip::GetTheme(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	return (it == g.registry.end()) ? xtooltip_theme_dark : it->second.theme;
}

BOOL CXTooltip::SetTextColor(HELE hEle, COLORREF color)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->customText = color;
	p->theme = xtooltip_theme_custom;
	return TRUE;
}

COLORREF CXTooltip::GetTextColor(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	if (it == g.registry.end()) return kTheme_DarkText;
	COLORREF t, b;
	ResolveColors(it->second, &t, &b);
	return t;
}

BOOL CXTooltip::SetBkColor(HELE hEle, COLORREF color)
{
	auto* p = _GetOrCreate(hEle);
	if (!p) return FALSE;
	p->customBg = color;
	p->theme = xtooltip_theme_custom;
	return TRUE;
}

COLORREF CXTooltip::GetBkColor(HELE hEle)
{
	auto& g = G();
	auto it = g.registry.find(hEle);
	if (it == g.registry.end()) return kTheme_DarkBg;
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


//============================================================================
// =====================================================================
// CXLoading 实现
// =====================================================================
//
// 设计思路:
//   - 共享 16ms 心跳 timer 通过 *宿主自己* 的 XE_XC_TIMER 派发; 每个宿主一个 timer
//     (id = kLoad_TimerId), 不依赖共享 g 单例 — 多个 loading 同时跑无冲突.
//   - XE_PAINT 接管 (用户要求 XE_PAINT 不是 XE_PAINT_END):
//       running=TRUE  时 -> 画 loading + *pbHandled = TRUE
//       running=FALSE 时 -> *pbHandled = FALSE (让出, 元素自己画)
//   - 元素销毁时 XE_DESTROY 自动清理 entry.
//   - 所有几何/动画用元素的 *逻辑* 像素 (与 module_xcgui_video 经验一致, hDraw
//     在 D2D 主路径下接受逻辑坐标, XCGUI 内部按 DPI 缩到物理).
//   - 5 种动画 ease 算法:
//       spinner: ease-in-out cubic (弧长振荡)
//       dots:    sin (正弦跳动)
//       spokes:  阶梯 (经典 beachball)
//       pulse:   ease-out cubic (扩散)
//       bars:    sin (高度起伏)
//============================================================================
namespace {

// 为避免与 tooltip 的常量重名, 全部用 _XLoad_ / kLoad_ 前缀.
constexpr UINT kLoad_TimerId      = 0x7200;       // 不与 tooltip 的 0x7100~7102 冲突
constexpr int  kLoad_TickMs       = 16;           // 60Hz
constexpr int  kLoad_DefaultSize  = 40;
constexpr int  kLoad_TextGap      = 8;            // 动画与下方文本的间距
constexpr int  kLoad_TextFontPt   = 9;

// 主题色 (XCGUI ARGB)
constexpr COLORREF kLoad_DarkText    = ((COLORREF)0xFF << 24) | (0xF5 << 16) | (0xF5 << 8) | 0xF5;
constexpr COLORREF kLoad_DarkBg      = ((COLORREF)0xFF << 24) | (0x17 << 16) | (0x17 << 8) | 0x17;
constexpr COLORREF kLoad_DarkAccent  = ((COLORREF)0xFF << 24) | (0xFF << 16) | (0xFF << 8) | 0xFF;
constexpr COLORREF kLoad_LightText   = ((COLORREF)0xFF << 24) | (0x17 << 16) | (0x17 << 8) | 0x17;
constexpr COLORREF kLoad_LightBg     = ((COLORREF)0xFF << 24) | (0xFF << 16) | (0xFF << 8) | 0xFF;
constexpr COLORREF kLoad_LightAccent = ((COLORREF)0xFF << 24) | (0x17 << 16) | (0x17 << 8) | 0x17;

constexpr float kLoad_Pi = 3.14159265358979323846f;

//============================================================================
// 注册项
//============================================================================
struct _XLoad_Entry
{
	HXCGUI            hHost          = NULL;
	BOOL              isWnd          = FALSE;             // FALSE=HELE, TRUE=HWINDOW
	BOOL              ownEle         = FALSE;             // TRUE=Create 创建 (vs Attach)
	BOOL              running        = TRUE;              // Start/Stop
	xloading_style_   style          = xloading_style_spinner;
	xloading_theme_   theme          = xloading_theme_dark;
	int               sizeCx         = kLoad_DefaultSize;
	int               sizeCy         = kLoad_DefaultSize;
	std::wstring      text;
	COLORREF          customText     = kLoad_DarkText;
	COLORREF          customBg       = kLoad_DarkBg;
	COLORREF          customAccent   = kLoad_DarkAccent;
	int               cornerLT       = 0;
	int               cornerRT       = 0;
	int               cornerRB       = 0;
	int               cornerLB       = 0;
	float             speed          = 1.0f;
	DWORD             startTick      = 0;
	BOOL              eventsHooked   = FALSE;
	// 字体 (per-host: 改字号无副作用, 多个 host 各自独立)
	int               fontPt         = kLoad_TextFontPt;  // 默认 9
	HFONTX            hFont          = NULL;              // 懒创建, 改字号时销毁重建
	// 用户面对的句柄 — 仅 AttachWnd 路径下非 NULL.
	//   AttachWnd(hWnd) 时, 我们建子 HELE 作为真实绘制载体 (= registry key, = hHost),
	//   hUserAlias 记录用户传入的 hWnd. 后续 Set*/Stop/Detach(hWnd) 走 _XLoad_Resolve 翻译过来.
	//   非 NULL 还兼任 "Detach 时销毁子 HELE" 的标志位.
	HXCGUI            hUserAlias     = NULL;
};

struct _XLoad_Global
{
	std::unordered_map<HXCGUI, _XLoad_Entry> registry;     // key = 真实绘制宿主 (HELE 或 HWINDOW)
	std::unordered_map<HXCGUI, HXCGUI>       userToHost;   // 用户传入句柄 → registry key (仅 AttachWnd 用)
};

_XLoad_Global& LG(){
	static _XLoad_Global s;
	return s;
}

// 懒创建/重建该 entry 的字体. 调用时机:
//   - OnPaint 测量/绘制文本前
//   - SetFontSize 改字号 (先销毁旧的)
void _XLoad_EnsureFont(_XLoad_Entry& e){
	if (!e.hFont){
		e.hFont = XFont_CreateEx(L"微软雅黑", e.fontPt, 0);
	}
}
void _XLoad_DestroyFont(_XLoad_Entry& e){
	if (e.hFont){ XFont_Destroy(e.hFont); e.hFont = NULL; }
}

//============================================================================
// 系统主题 (与 tooltip 的 DetectSystemDarkMode 重复实现, 隔离, 避免跨 ns 引用)
//============================================================================
BOOL _XLoad_DetectSystemDarkMode()
{
	HKEY hKey = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		0, KEY_READ, &hKey) != ERROR_SUCCESS) return TRUE;
	DWORD val = 1, size = sizeof(val), type = REG_DWORD;
	BOOL dark = TRUE;
	if (RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, &type,
		(LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD){
		dark = (val == 0);
	}
	RegCloseKey(hKey);
	return dark;
}

//============================================================================
// 主题色解析
//============================================================================
struct _XLoad_Colors
{
	COLORREF text;
	COLORREF bg;
	COLORREF accent;
};

void _XLoad_ResolveColors(const _XLoad_Entry& e, _XLoad_Colors* out)
{
	xloading_theme_ eff = e.theme;
	if (eff == xloading_theme_auto)
		eff = _XLoad_DetectSystemDarkMode() ? xloading_theme_dark : xloading_theme_light;
	switch (eff){
	case xloading_theme_light:
		out->text   = kLoad_LightText;
		out->bg     = kLoad_LightBg;
		out->accent = kLoad_LightAccent;
		break;
	case xloading_theme_custom:
		out->text   = e.customText;
		out->bg     = e.customBg;
		out->accent = e.customAccent;
		break;
	case xloading_theme_dark:
	default:
		out->text   = kLoad_DarkText;
		out->bg     = kLoad_DarkBg;
		out->accent = kLoad_DarkAccent;
		break;
	}
}

// 强调色 -> 非强调色 (alpha 衰减, RGB 通道一致原则保持)
inline COLORREF _XLoad_FadeAccent(COLORREF accent, BYTE alpha){
	return (accent & 0x00FFFFFF) | ((COLORREF)alpha << 24);
}

//============================================================================
// 缓动函数 (CSS 标准: cubic-bezier(0.65,0,0.35,1) 的 closed-form 近似)
//============================================================================
inline float _XLoad_EaseInOutCubic(float t){
	return t < 0.5f ? 4.0f * t * t * t
	                : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f;
}
inline float _XLoad_EaseOutCubic(float t){
	return 1.0f - powf(1.0f - t, 3.0f);
}

//============================================================================
// 注册表查询
//============================================================================
// AttachWnd 模式下用户拿着 hWnd, 我们存的 entry 却以子 HELE 为 key.
// 所有 public API 进来先 Resolve 一下 — 若 hUser 在 userToHost 表里, 翻成真实 key.
// 否则原样返回 (Create/AttachEle/直 SetXxx(hWnd) 路径仍然透传).
HXCGUI _XLoad_Resolve(HXCGUI hUser){
	if (!hUser) return NULL;
	auto& g = LG();
	auto it = g.userToHost.find(hUser);
	return (it != g.userToHost.end()) ? it->second : hUser;
}

_XLoad_Entry* _XLoad_GetEntry(HXCGUI hHost){
	auto& g = LG();
	auto it = g.registry.find(_XLoad_Resolve(hHost));
	return (it == g.registry.end()) ? NULL : &it->second;
}

// 前向声明 (回调里用到)
int CALLBACK _XLoad_OnPaint  (HXCGUI hSelf, HDRAW hDraw,    BOOL* pbHandled);
int CALLBACK _XLoad_OnTimer  (HXCGUI hSelf, UINT nTimerID,  BOOL* pbHandled);
int CALLBACK _XLoad_OnDestroy(HXCGUI hSelf, BOOL* pbHandled);

void _XLoad_StartTimer(HXCGUI hHost){
	if (XC_IsHELE(hHost))         XEle_SetXCTimer((HELE)hHost,    kLoad_TimerId, kLoad_TickMs);
	else if (XC_IsHWINDOW(hHost)) XWnd_SetXCTimer((HWINDOW)hHost, kLoad_TimerId, kLoad_TickMs);
}
void _XLoad_StopTimer(HXCGUI hHost){
	if (XC_IsHELE(hHost))         XEle_KillXCTimer((HELE)hHost,    kLoad_TimerId);
	else if (XC_IsHWINDOW(hHost)) XWnd_KillXCTimer((HWINDOW)hHost, kLoad_TimerId);
}
void _XLoad_Redraw(HXCGUI hHost){
	if (XC_IsHELE(hHost))         XEle_Redraw((HELE)hHost,    FALSE);
	else if (XC_IsHWINDOW(hHost)) XWnd_Redraw((HWINDOW)hHost, FALSE);
}
void _XLoad_HookEvents(HXCGUI hHost, _XLoad_Entry& e){
	if (e.eventsHooked) return;
	if (XC_IsHELE(hHost)){
		HELE hEle = (HELE)hHost;
		XEle_RegEventC1(hEle, XE_PAINT,    (void*)&_XLoad_OnPaint);
		XEle_RegEventC1(hEle, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
		XEle_RegEventC1(hEle, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
	} else if (XC_IsHWINDOW(hHost)){
		HWINDOW hWnd = (HWINDOW)hHost;
		XWnd_RegEventC1(hWnd, XE_PAINT,    (void*)&_XLoad_OnPaint);
		XWnd_RegEventC1(hWnd, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
		XWnd_RegEventC1(hWnd, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
	}
	e.eventsHooked = TRUE;
}

// 通用 Set* 入口: 取/造 entry. 与 CXTooltip 的 _GetOrCreate 同思路, 让 Add/Set 顺序无关.
//   AttachWnd 路径: 用户拿着 hWnd, 先 Resolve 翻成内部子 HELE, 再走旧逻辑.
_XLoad_Entry* _XLoad_GetOrCreate(HXCGUI hHost){
	if (!hHost) return NULL;
	HXCGUI hKey = _XLoad_Resolve(hHost);
	if (!XC_IsHELE(hKey) && !XC_IsHWINDOW(hKey)) return NULL;
	auto& g = LG();
	auto& e = g.registry[hKey];
	e.hHost = hKey;
	e.isWnd = XC_IsHWINDOW(hKey) ? TRUE : FALSE;
	_XLoad_HookEvents(hKey, e);
	return &e;
}

//============================================================================
// 5 种动画绘制
//
// 输入约定:
//   hHost    — 宿主 HELE / HWINDOW (painter 用它取 DPI / window-client 物理坐标)
//   (cx, cy) — 动画中心, 宿主客户区*逻辑*像素
//   size     — 动画绘制方形边长, *逻辑*像素
//   tNorm    — 当前周期内进度 [0, 1)
//   accent   — 强调色 (XCGUI RGBA, 字节序 R | G<<8 | B<<16 | A<<24)
//
// D2D 优先路径: 用 ID2D1RenderTarget 直接画 PathGeometry/StrokeStyle, 端帽
//   D2D_CAP_STYLE_ROUND 原生, 与笔画严格对齐, sub-pixel 精度无模糊.
// GDI+ 兜底:    XDraw_DrawArcF/DrawLine + 手动 cap 圆模拟, 笔画宽手动 ×dpi 物理化.
//============================================================================

// Paint 上下文 — 一次构造贯穿所有 painter, 含坐标转换 + DPI + D2D RT.
//   *Phys 字段: 动画中心+边长在*窗口客户区物理像素*中的值, 与 D2D RT 同坐标系.
//   *Log  字段: 宿主本地逻辑坐标 (D2D 不可用时给 XDraw_* 用).
struct _XLoad_PaintCtx{
	HDRAW              hDraw;
	HXCGUI             hHost;
	ID2D1RenderTarget* rt;        // D2D 路径; NULL 表示 GDI+ 兜底
	float              dpiScale;
	// D2D 路径用
	float              cxPhys;
	float              cyPhys;
	float              sizePhys;
	// GDI+ 兜底路径用 (= 原始入参)
	int                cxLog;
	int                cyLog;
	int                sizeLog;
};

// 从 hDraw + hHost + (cx,cy,size) 逻辑值构造上下文.
void _XLoad_BuildCtx(_XLoad_PaintCtx& ctx, HDRAW hDraw, HXCGUI hHost,
	int cxLog, int cyLog, int sizeLog)
{
	ctx.hDraw   = hDraw;
	ctx.hHost   = hHost;
	ctx.cxLog   = cxLog;
	ctx.cyLog   = cyLog;
	ctx.sizeLog = sizeLog;
	ctx.rt      = (ID2D1RenderTarget*)XDraw_GetD2dRenderTarget(hDraw);

	HWINDOW hWnd = NULL;
	if (XC_IsHELE(hHost))         hWnd = XWidget_GetHWINDOW(hHost);
	else if (XC_IsHWINDOW(hHost)) hWnd = (HWINDOW)hHost;
	int dpi = hWnd ? XWnd_GetDPI(hWnd) : 96;
	if (dpi <= 0) dpi = 96;
	ctx.dpiScale = dpi / 96.0f;

	// 元素: 用 XEle_GetWndClientRectDPI 直接拿物理 rect, + 元素内逻辑偏移*dpiScale.
	// 窗口: D2D RT 原点已是窗口客户区 (0,0) 物理, 直接 *dpiScale.
	float ox = 0.0f, oy = 0.0f;
	if (XC_IsHELE(hHost)){
		RECT rcEle{};
		XEle_GetWndClientRectDPI((HELE)hHost, &rcEle);
		ox = (float)rcEle.left;
		oy = (float)rcEle.top;
	}
	ctx.cxPhys   = ox + cxLog   * ctx.dpiScale;
	ctx.cyPhys   = oy + cyLog   * ctx.dpiScale;
	ctx.sizePhys = sizeLog * ctx.dpiScale;
}

// XCGUI RGBA → D2D ColorF, 可带 alpha override (0..255, -1 表示用原 alpha).
inline D2D1_COLOR_F _XLoad_ToColorF(COLORREF c, int alphaOverride = -1){
	float r = (c        & 0xFF) / 255.0f;
	float g = ((c >> 8) & 0xFF) / 255.0f;
	float b = ((c >> 16)& 0xFF) / 255.0f;
	float a = (alphaOverride >= 0)
		? (alphaOverride / 255.0f)
		: ((c >> 24) & 0xFF) / 255.0f;
	return D2D1::ColorF(r, g, b, a);
}

// 创建 round-cap stroke style, 调用方负责 Release.
ID2D1StrokeStyle* _XLoad_MakeRoundStroke(ID2D1Factory* fac){
	if (!fac) return NULL;
	D2D1_STROKE_STYLE_PROPERTIES p = D2D1::StrokeStyleProperties();
	p.startCap = D2D1_CAP_STYLE_ROUND;
	p.endCap   = D2D1_CAP_STYLE_ROUND;
	p.dashCap  = D2D1_CAP_STYLE_ROUND;
	p.lineJoin = D2D1_LINE_JOIN_ROUND;
	ID2D1StrokeStyle* s = NULL;
	fac->CreateStrokeStyle(p, NULL, 0, &s);
	return s;
}

// 1) Material Design 风格的不定进度 Spinner — daisyUI / MUI / 系统 InProgress 同款.
//    *两个* 同步动画叠加, 产生"游动的弧"质感:
//      A) 整体匀速旋转  (linear)
//      B) 弧长在 minArc <-> maxArc 之间 ease-in-out 振荡
//    阶段 A (tNorm<0.5): 尾固定, 头 ease-in-out 拉开 → 弧从 minArc 长到 maxArc.
//    阶段 B (tNorm>=0.5): 头固定, 尾 ease-in-out 追上 → 弧从 maxArc 缩到 minArc.
//
//    无缝循环数学:
//      令 baseRot = 每周期 tail 累计推进度, arcDelta = maxArc - minArc.
//      一周期内 tail 推进 baseRot + arcDelta, head 推进 baseRot + arcDelta.
//      取 baseRot = 720 - arcDelta → 推进量 = 720 (= 2 * 360), 跨周期角度位置 mod 360
//      自然对齐, 无视觉跳变.
//
//    Rendering (D2D 主路径):
//      ID2D1PathGeometry + AddArc + DrawGeometry(stroke=round-cap) → 端帽与笔画
//      天然对齐, sub-pixel 精度, DPI 任意倍率干净.
//
//    Rendering (GDI+ 兜底):
//      XDraw_DrawArcF 主体 + XDraw_FillEllipseF 模拟端帽. cap 直径必须 *等于*
//      笔画物理宽 (不能 +1), 否则视觉端帽比笔画粗.
void _XLoad_PaintSpinner(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	// 几何 — 全部物理像素.
	float strokeLog  = (ctx.sizePhys / ctx.dpiScale / 10.0f);
	if (strokeLog < 2.0f) strokeLog = 2.0f;
	float strokePhys = floorf(strokeLog * ctx.dpiScale + 0.5f);
	if (strokePhys < 1.0f) strokePhys = 1.0f;
	float rPhys = (ctx.sizePhys - strokePhys) * 0.5f;     // 笔画 centerline 半径
	if (rPhys < 1.0f) rPhys = 1.0f;

	// Material 风双动画
	constexpr float minArc   = 24.0f;
	constexpr float maxArc   = 280.0f;
	constexpr float arcDelta = maxArc - minArc;            // 256
	constexpr float baseRot  = 720.0f - arcDelta;          // 464 → 周期推进 720, 无缝
	float arcLen, arcStart;
	if (tNorm < 0.5f){
		float u = tNorm * 2.0f;
		float e = _XLoad_EaseInOutCubic(u);
		arcLen   = minArc + arcDelta * e;
		arcStart = tNorm * baseRot;                        // tail 匀速推进 (u 与 t 线性)
	} else {
		float u = (tNorm - 0.5f) * 2.0f;
		float e = _XLoad_EaseInOutCubic(u);
		arcLen   = maxArc - arcDelta * e;
		arcStart = tNorm * baseRot + arcDelta * e;         // tail 在 ease 加速段追头
	}

	// Track + 主弧 端点 (12 点钟方向起算 → -90° 偏移).
	float startDeg = arcStart - 90.0f;
	float sweepDeg = arcLen;
	float startRad = startDeg            * (kLoad_Pi / 180.0f);
	float endRad   = (startDeg + sweepDeg)* (kLoad_Pi / 180.0f);

	if (ctx.rt){
		// ---------- D2D 主路径 ----------
		ID2D1Factory* fac = NULL;
		ctx.rt->GetFactory(&fac);
		if (!fac) return;

		ID2D1StrokeStyle* roundStroke = _XLoad_MakeRoundStroke(fac);
		ID2D1SolidColorBrush* brush = NULL;
		ctx.rt->CreateSolidColorBrush(_XLoad_ToColorF(accent, 45), &brush);
		if (brush){
			// L1: 完整圆环 track (半透明)
			D2D1_ELLIPSE el = D2D1::Ellipse(
				D2D1::Point2F(ctx.cxPhys, ctx.cyPhys), rPhys, rPhys);
			ctx.rt->DrawEllipse(el, brush, strokePhys, NULL);

			// L2: 主体活动弧 (圆角端帽)
			brush->SetColor(_XLoad_ToColorF(accent, 255));
			ID2D1PathGeometry* path = NULL;
			if (SUCCEEDED(fac->CreatePathGeometry(&path)) && path){
				ID2D1GeometrySink* sink = NULL;
				if (SUCCEEDED(path->Open(&sink)) && sink){
					D2D1_POINT_2F p0 = D2D1::Point2F(
						ctx.cxPhys + rPhys * cosf(startRad),
						ctx.cyPhys + rPhys * sinf(startRad));
					D2D1_POINT_2F p1 = D2D1::Point2F(
						ctx.cxPhys + rPhys * cosf(endRad),
						ctx.cyPhys + rPhys * sinf(endRad));
					sink->BeginFigure(p0, D2D1_FIGURE_BEGIN_HOLLOW);
					D2D1_ARC_SEGMENT seg = {
						p1,
						D2D1::SizeF(rPhys, rPhys),
						0.0f,
						D2D1_SWEEP_DIRECTION_CLOCKWISE,
						(sweepDeg > 180.0f)
							? D2D1_ARC_SIZE_LARGE
							: D2D1_ARC_SIZE_SMALL,
					};
					sink->AddArc(seg);
					sink->EndFigure(D2D1_FIGURE_END_OPEN);
					sink->Close();
					SafeRelease(sink);
				}
				ctx.rt->DrawGeometry(path, brush, strokePhys, roundStroke);
				SafeRelease(path);
			}
			SafeRelease(brush);
		}
		SafeRelease(roundStroke);
		fac->Release();
		return;
	}

	// ---------- GDI+ 兜底 ----------
	// XDraw_DrawArcF 用宿主本地*逻辑*坐标 (XCGUI 内部 ×dpi); 但 stroke 宽必须物理化
	// (XDraw_SetLineWidthF transform-invariant, 见 模块封装规范.md §3.5.5#3).
	// cap 直径 = strokeLog (逻辑) → 渲染后 = strokePhys (物理), 与笔画完全等宽.
	float rLog       = (float)ctx.sizeLog * 0.5f - strokeLog * 0.5f;
	if (rLog < 1.0f) rLog = 1.0f;
	float boxLog     = rLog * 2.0f;
	float boxLLog    = ctx.cxLog - rLog;
	float boxTLog    = ctx.cyLog - rLog;

	XDraw_SetLineWidthF(ctx.hDraw, strokePhys);

	// L1: track
	XDraw_SetBrushColor(ctx.hDraw, _XLoad_FadeAccent(accent, 45));
	XDraw_DrawArcF(ctx.hDraw, boxLLog, boxTLog, boxLog, boxLog, -90.0f, 360.0f);

	// L2: 主弧
	XDraw_SetBrushColor(ctx.hDraw, accent);
	XDraw_DrawArcF(ctx.hDraw, boxLLog, boxTLog, boxLog, boxLog, startDeg, sweepDeg);

	// L3: 圆角端帽 - 直径 = strokeLog (逻辑), 圆心在 (cx,cy) + rLog * (cos,sin)
	float capR = strokeLog * 0.5f;
	float sxL = ctx.cxLog + rLog * cosf(startRad);
	float syL = ctx.cyLog + rLog * sinf(startRad);
	float exL = ctx.cxLog + rLog * cosf(endRad);
	float eyL = ctx.cyLog + rLog * sinf(endRad);
	RECTF cap1{ sxL - capR, syL - capR, sxL + capR, syL + capR };
	RECTF cap2{ exL - capR, eyL - capR, exL + capR, eyL + capR };
	XDraw_FillEllipseF(ctx.hDraw, &cap1);
	XDraw_FillEllipseF(ctx.hDraw, &cap2);
}

// 2) Win10 / Win8 启动风格 5-dot Spinner.
//    设计要点 (修正前两版的问题):
//      *) 5 dot 全程可见 → 无 fade in/out, 不会出现"12 点钟有个永远静止的半透明点"
//      *) 每 dot 各自做 *完整 1 周* 的 ease-in-out cubic 旋转, 相位等分错开 1/N
//      *) 因为 ease-in-out → 同一时刻 5 dot 在圆周上分布*不均*:
//         加速段彼此拉开, 减速段彼此收拢 → 视觉上有"加速度"质感, 像跑马灯/Win10 boot
//      *) 360° 走完无缝接续 (相位累加 mod 1, ease 在 [0,1] 闭合)
//
//    D2D 主路径: ID2D1RenderTarget::FillEllipse, sub-pixel 精度
//    GDI+ 兜底:   XDraw_FillEllipseF, 同样 sub-pixel
void _XLoad_PaintDots(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	constexpr int   N        = 5;
	constexpr float startAng = -90.0f;   // 12 点钟出发, 顺时针

	float dotR  = ctx.sizePhys / 12.0f;
	if (dotR < 2.0f * ctx.dpiScale) dotR = 2.0f * ctx.dpiScale;
	float ringR = ctx.sizePhys * 0.5f - dotR;
	if (ringR < dotR) ringR = dotR;

	// D2D 工厂资源 (一次性创建, 多 dot 复用)
	ID2D1SolidColorBrush* brush = NULL;
	if (ctx.rt){
		ctx.rt->CreateSolidColorBrush(_XLoad_ToColorF(accent, 255), &brush);
		if (!brush) return;
	}

	for (int i = 0; i < N; ++i){
		// 第 i 个 dot 的相位 [0, 1)
		float phase = fmodf(tNorm + (float)i / N, 1.0f);
		// ease-in-out cubic 提供 *加速度*, 60%; 线性 40% 提供"基础速度",
		// 防止 dot 在 phase 0/1 附近完全停滞 (旧版 user 反馈"12 点钟有静止圆点"问题).
		// 数学保证: 边界处 (phase=0 或 1) eased=0/1, 总进度 = phase=0/1 → 与下个周期接合连续.
		float eased = _XLoad_EaseInOutCubic(phase);
		float prog  = eased * 0.6f + phase * 0.4f;
		float angleDeg = startAng + prog * 360.0f;
		float angleRad = angleDeg * (kLoad_Pi / 180.0f);

		if (ctx.rt){
			float xC = ctx.cxPhys + ringR * cosf(angleRad);
			float yC = ctx.cyPhys + ringR * sinf(angleRad);
			D2D1_ELLIPSE el = D2D1::Ellipse(
				D2D1::Point2F(xC, yC), dotR, dotR);
			ctx.rt->FillEllipse(el, brush);
		} else {
			// GDI+ 兜底 — 用宿主本地逻辑坐标. dotR/ringR 反推回逻辑.
			float dotRLog  = dotR  / ctx.dpiScale;
			float ringRLog = ringR / ctx.dpiScale;
			float xC = ctx.cxLog + ringRLog * cosf(angleRad);
			float yC = ctx.cyLog + ringRLog * sinf(angleRad);
			RECTF rc{ xC - dotRLog, yC - dotRLog,
			          xC + dotRLog, yC + dotRLog };
			XDraw_SetBrushColor(ctx.hDraw, accent);
			XDraw_FillEllipseF(ctx.hDraw, &rc);
		}
	}

	SafeRelease(brush);
}

// 3) Spokes — 12 条辐条, head 全亮, 其余按距离阶梯衰减 alpha.
//    D2D 路径: ID2D1RenderTarget::DrawLine + StrokeStyle{startCap=endCap=ROUND}
//             → 笔画两端原生圆角, 与笔画完全等宽对齐 (无需手画 cap 圆).
//    GDI+ 兜底: XDraw_DrawLineF + 末端 XDraw_FillEllipseF, cap 直径 = strokeLog
//             (不能 +1, 否则 cap 比笔画粗).
//
//    几何比例 (前版改进, 防 AA 模糊):
//      innerR = size * 0.30 (从 0.25), outerR = size * 0.45 (从 0.50),
//      stroke = size / 14   (从 size / 12)  — 端帽 + 辐条间距更舒展.
void _XLoad_PaintSpokes(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	constexpr int N = 12;
	float innerR_phys = ctx.sizePhys * 0.30f;
	float outerR_phys = ctx.sizePhys * 0.45f;
	float strokeLog   = (ctx.sizePhys / ctx.dpiScale) / 14.0f;
	if (strokeLog < 2.0f) strokeLog = 2.0f;
	float strokePhys  = floorf(strokeLog * ctx.dpiScale + 0.5f);
	if (strokePhys < 1.0f) strokePhys = 1.0f;

	int head = (int)(tNorm * N) % N;

	if (ctx.rt){
		// ---------- D2D 主路径 ----------
		ID2D1Factory* fac = NULL;
		ctx.rt->GetFactory(&fac);
		if (!fac) return;
		ID2D1StrokeStyle* roundStroke = _XLoad_MakeRoundStroke(fac);
		ID2D1SolidColorBrush* brush = NULL;
		ctx.rt->CreateSolidColorBrush(_XLoad_ToColorF(accent, 255), &brush);
		if (brush){
			for (int i = 0; i < N; ++i){
				int dist = (head - i + N) % N;
				BYTE a = (BYTE)(255 - dist * (205 / (N - 1)));
				if (a < 50) a = 50;
				brush->SetColor(_XLoad_ToColorF(accent, a));

				float angle = (i / (float)N) * 2.0f * kLoad_Pi - kLoad_Pi * 0.5f;
				float cs = cosf(angle), sn = sinf(angle);
				D2D1_POINT_2F p1 = D2D1::Point2F(
					ctx.cxPhys + innerR_phys * cs,
					ctx.cyPhys + innerR_phys * sn);
				D2D1_POINT_2F p2 = D2D1::Point2F(
					ctx.cxPhys + outerR_phys * cs,
					ctx.cyPhys + outerR_phys * sn);
				ctx.rt->DrawLine(p1, p2, brush, strokePhys, roundStroke);
			}
			SafeRelease(brush);
		}
		SafeRelease(roundStroke);
		fac->Release();
		return;
	}

	// ---------- GDI+ 兜底 ----------
	float innerLog = innerR_phys / ctx.dpiScale;
	float outerLog = outerR_phys / ctx.dpiScale;
	float capR     = strokeLog * 0.5f;
	XDraw_SetLineWidthF(ctx.hDraw, strokePhys);
	for (int i = 0; i < N; ++i){
		int dist = (head - i + N) % N;
		BYTE a = (BYTE)(255 - dist * (205 / (N - 1)));
		if (a < 50) a = 50;
		COLORREF c = _XLoad_FadeAccent(accent, a);
		XDraw_SetBrushColor(ctx.hDraw, c);
		float angle = (i / (float)N) * 2.0f * kLoad_Pi - kLoad_Pi * 0.5f;
		float cs = cosf(angle), sn = sinf(angle);
		float x1 = ctx.cxLog + innerLog * cs;
		float y1 = ctx.cyLog + innerLog * sn;
		float x2 = ctx.cxLog + outerLog * cs;
		float y2 = ctx.cyLog + outerLog * sn;
		XDraw_DrawLineF(ctx.hDraw, x1, y1, x2, y2);
		RECTF capIn { x1 - capR, y1 - capR, x1 + capR, y1 + capR };
		RECTF capOut{ x2 - capR, y2 - capR, x2 + capR, y2 + capR };
		XDraw_FillEllipseF(ctx.hDraw, &capIn);
		XDraw_FillEllipseF(ctx.hDraw, &capOut);
	}
}

// 4) Pulse — 2 个同心圆 ease-out 扩散 + alpha 衰减.
//    保持 GDI+ 风格 (无明显 cap/对齐问题), 简单走 XDraw_*. D2D / GDI+ 透传一致.
void _XLoad_PaintPulse(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	for (int i = 0; i < 2; ++i){
		float phase = fmodf(tNorm + i * 0.5f, 1.0f);
		float eased = _XLoad_EaseOutCubic(phase);
		float rLog = ctx.sizeLog * (0.15f + 0.35f * eased);
		BYTE a = (BYTE)((1.0f - phase) * 200);
		if (a < 5) continue;
		COLORREF c = _XLoad_FadeAccent(accent, a);
		XDraw_SetBrushColor(ctx.hDraw, c);
		XDraw_SetLineWidthF(ctx.hDraw, 2.0f * ctx.dpiScale);
		RECTF rc{ ctx.cxLog - rLog, ctx.cyLog - rLog,
		          ctx.cxLog + rLog, ctx.cyLog + rLog };
		XDraw_DrawEllipseF(ctx.hDraw, &rc);
	}
}

// 5) Bars — 现代极简胶囊均衡器条 (modern audio visualizer 风, 非 Win10 模仿).
//    设计:
//      *) 4 根 *胶囊形* (full pill) 实心条, 强调色饱和, 无渐变 / 无阴影
//      *) 高度按 smoothstep(sin) 振荡: minH = barW (条宽, 此时形状为圆) →
//         maxH = sizePhys * 0.92  (接近满高)
//      *) 相邻条相位错开 1/N → 视觉上"波"从左向右传播 (Spotify / Apple 风)
//      *) 圆角 = barW / 2 → 永远胶囊形, 高度收缩到 barW 时退化为完美圆 (无视觉跳变)
//      *) 间距 = barW * 0.6 → 修长比例, 不拥挤
//
//    D2D 主路径: ID2D1RenderTarget::FillRoundedRectangle, sub-pixel 精度
//    GDI+ 兜底:  XDraw_FillRoundRect (逻辑 RECT + barW 圆角)
void _XLoad_PaintBars(const _XLoad_PaintCtx& ctx, float tNorm, COLORREF accent)
{
	constexpr int   N         = 4;
	constexpr float kBarRatio = 0.13f;   // bar 宽 / size
	constexpr float kGapRatio = 0.08f;   // gap / size  (= 0.6 * kBarRatio)
	constexpr float kMaxH     = 0.92f;   // 最高 / size

	float barW = ctx.sizePhys * kBarRatio;
	float gap  = ctx.sizePhys * kGapRatio;
	float maxH = ctx.sizePhys * kMaxH;
	float minH = barW;                                // 最矮 = 条宽 → 圆形
	float totalW = N * barW + (N - 1) * gap;
	float x0 = ctx.cxPhys - totalW * 0.5f;
	float radius = barW * 0.5f;

	if (ctx.rt){
		// ---------- D2D 主路径 ----------
		ID2D1SolidColorBrush* brush = NULL;
		ctx.rt->CreateSolidColorBrush(_XLoad_ToColorF(accent, 255), &brush);
		if (!brush) return;
		for (int i = 0; i < N; ++i){
			float phase = fmodf(tNorm + (float)i / N, 1.0f);
			// 0.5 - 0.5 cos(2π·phase) = 标准正弦波 [0..1], 再 smoothstep 锐化两端 → 更"弹"
			float wave  = 0.5f - 0.5f * cosf(2.0f * kLoad_Pi * phase);
			float eased = wave * wave * (3.0f - 2.0f * wave);
			float h = minH + (maxH - minH) * eased;
			float xL = x0 + i * (barW + gap);
			D2D1_ROUNDED_RECT rr = {
				D2D1::RectF(xL, ctx.cyPhys - h * 0.5f,
				            xL + barW, ctx.cyPhys + h * 0.5f),
				radius, radius
			};
			ctx.rt->FillRoundedRectangle(rr, brush);
		}
		SafeRelease(brush);
		return;
	}

	// ---------- GDI+ 兜底 ----------
	float barWlog = barW   / ctx.dpiScale;
	float gapLog  = gap    / ctx.dpiScale;
	float maxHlog = maxH   / ctx.dpiScale;
	float minHlog = barWlog;
	float totWlog = N * barWlog + (N - 1) * gapLog;
	float x0Log   = ctx.cxLog - totWlog * 0.5f;
	int   radLog  = (int)(barWlog * 0.5f + 0.5f);

	XDraw_SetBrushColor(ctx.hDraw, accent);
	for (int i = 0; i < N; ++i){
		float phase = fmodf(tNorm + (float)i / N, 1.0f);
		float wave  = 0.5f - 0.5f * cosf(2.0f * kLoad_Pi * phase);
		float eased = wave * wave * (3.0f - 2.0f * wave);
		float hLog = minHlog + (maxHlog - minHlog) * eased;
		int xL = (int)(x0Log + i * (barWlog + gapLog) + 0.5f);
		int xR = (int)(x0Log + i * (barWlog + gapLog) + barWlog + 0.5f);
		RECT rc{ xL, ctx.cyLog - (int)(hLog * 0.5f),
		         xR, ctx.cyLog + (int)(hLog * 0.5f) };
		XDraw_FillRoundRect(ctx.hDraw, &rc, radLog * 2, radLog * 2);
	}
}

// 派发到具体风格
void _XLoad_PaintAnim(const _XLoad_PaintCtx& ctx, xloading_style_ style,
	float tNorm, COLORREF accent)
{
	switch (style){
	case xloading_style_spinner: _XLoad_PaintSpinner(ctx, tNorm, accent); break;
	case xloading_style_dots:    _XLoad_PaintDots   (ctx, tNorm, accent); break;
	case xloading_style_spokes:  _XLoad_PaintSpokes (ctx, tNorm, accent); break;
	case xloading_style_pulse:   _XLoad_PaintPulse  (ctx, tNorm, accent); break;
	case xloading_style_bars:    _XLoad_PaintBars   (ctx, tNorm, accent); break;
	}
}

// 每种风格的"动画周期" (ms, speed=1.0 时)
int _XLoad_PeriodMs(xloading_style_ s){
	switch (s){
	case xloading_style_spinner: return 1200;   // daisyUI / Material 平滑旋转
	case xloading_style_dots:    return 1800;   // Win8/10 boot 节奏 (单 dot 完整 lifecycle)
	case xloading_style_spokes:  return 1080;   // 12 spoke * 90ms
	case xloading_style_pulse:   return 1400;
	case xloading_style_bars:    return 1100;   // 现代胶囊条波动节奏 (慢一点更优雅)
	}
	return 1000;
}

//============================================================================
// 取宿主客户区 (元素本地逻辑像素, hDraw 工作坐标系)
//============================================================================
void _XLoad_GetClient(HXCGUI hHost, RECT* rc){
	rc->left = rc->top = rc->right = rc->bottom = 0;
	if (XC_IsHELE(hHost))         XEle_GetClientRect((HELE)hHost,    rc);
	else if (XC_IsHWINDOW(hHost)) XWnd_GetClientRect((HWINDOW)hHost, rc);
}

//============================================================================
// XE_PAINT 主回调 (元素 / 窗口共用, callback 签名 binary 兼容)
//============================================================================
int CALLBACK _XLoad_OnPaint(HXCGUI hSelf, HDRAW hDraw, BOOL* pbHandled)
{
	auto* p = _XLoad_GetEntry(hSelf);
	if (!p || !p->running){
		// 让出 paint, 让宿主自己渲染原内容
		if (pbHandled) *pbHandled = FALSE;
		return 0;
	}
	if (pbHandled) *pbHandled = TRUE;

	RECT rc{};
	_XLoad_GetClient(hSelf, &rc);
	int cliW = rc.right - rc.left;
	int cliH = rc.bottom - rc.top;
	if (cliW <= 0 || cliH <= 0) return 0;

	_XLoad_Colors c;
	_XLoad_ResolveColors(*p, &c);

	// 1) 背景 (圆角 — 4 角同 -> FillRoundRect; 不同 -> FillRoundRectEx)
	XDraw_SetBrushColor(hDraw, c.bg);
	if (p->cornerLT == p->cornerRT && p->cornerRT == p->cornerRB && p->cornerRB == p->cornerLB){
		if (p->cornerLT > 0)
			XDraw_FillRoundRect(hDraw, &rc, p->cornerLT, p->cornerLT);
		else
			XDraw_FillRect(hDraw, &rc);
	} else {
		XDraw_FillRoundRectEx(hDraw, &rc,
			p->cornerLT, p->cornerRT, p->cornerRB, p->cornerLB);
	}

	// 2) 文字测量 (如果有)
	SIZE textSz{0, 0};
	if (!p->text.empty()){
		_XLoad_EnsureFont(*p);
		XC_GetTextShowSizeEx(p->text.c_str(), (int)p->text.size(),
			p->hFont, textFormatFlag_NoWrap, &textSz);
	}

	// 3) 整体居中: 动画在上, 文本在下, 中间 gap
	int gap     = (textSz.cy > 0) ? kLoad_TextGap : 0;
	int totalH  = p->sizeCy + gap + textSz.cy;
	int cx      = rc.left + cliW / 2;
	int yTop    = rc.top  + (cliH - totalH) / 2;
	int cyAnim  = yTop + p->sizeCy / 2;

	// 4) 动画相位
	DWORD now = ::GetTickCount();
	int   period = (int)(_XLoad_PeriodMs(p->style) / (p->speed > 0 ? p->speed : 1.0f));
	if (period < 50) period = 50;
	DWORD elapsed = now - p->startTick;
	float tNorm = (elapsed % period) / (float)period;

	// 5) 动画 (取 min(cx, cy) 为有效边长 — cx != cy 时不变形)
	int aSize = (p->sizeCx < p->sizeCy) ? p->sizeCx : p->sizeCy;
	_XLoad_PaintCtx pctx;
	_XLoad_BuildCtx(pctx, hDraw, hSelf, cx, cyAnim, aSize);
	_XLoad_PaintAnim(pctx, p->style, tNorm, c.accent);

	// 6) 文本 (动画下方居中)
	if (!p->text.empty()){
		XDraw_SetTextRenderingHint(hDraw, 3);   // AntiAliasGridFit (灰度 AA, 透明面板防彩边)
		XDraw_SetBrushColor(hDraw, c.text);     // XDraw_DrawText 取色源 = brush
		XDraw_SetFont(hDraw, p->hFont);
		XDraw_SetTextAlign(hDraw,
			textAlignFlag_left | textAlignFlag_top | textFormatFlag_NoWrap);
		int tx = cx - textSz.cx / 2;
		int ty = yTop + p->sizeCy + gap;
		RECT trc{ tx, ty, tx + textSz.cx + 2, ty + textSz.cy + 2 };
		XDraw_DrawText(hDraw, p->text.c_str(), (int)p->text.size(), &trc);
	}
	return 0;
}

//============================================================================
// XE_XC_TIMER — 心跳重绘
//============================================================================
int CALLBACK _XLoad_OnTimer(HXCGUI hSelf, UINT nTimerID, BOOL* pbHandled)
{
	if (nTimerID != kLoad_TimerId){
		if (pbHandled) *pbHandled = FALSE;
		return 0;
	}
	auto* p = _XLoad_GetEntry(hSelf);
	if (!p || !p->running) return 0;
	_XLoad_Redraw(hSelf);
	return 0;
}

//============================================================================
// XE_DESTROY — 清理 entry
//============================================================================
int CALLBACK _XLoad_OnDestroy(HXCGUI hSelf, BOOL* pbHandled)
{
	(void)pbHandled;
	_XLoad_StopTimer(hSelf);
	auto& g = LG();
	auto it = g.registry.find(hSelf);
	if (it != g.registry.end()){
		// AttachWnd 路径下子 HELE 被外部 (e.g. 窗口关闭) 销毁 → 同步擦掉 userToHost 别名.
		if (it->second.hUserAlias){
			g.userToHost.erase(it->second.hUserAlias);
		}
		_XLoad_DestroyFont(it->second);
		g.registry.erase(it);
	}
	return 0;
}

}  // anonymous namespace (CXLoading internals)


//============================================================================
// CXLoading 公开接口
//============================================================================

HELE CXLoading::Create(int x, int y, int cx, int cy, HXCGUI hParent)
{
	HELE hEle = XEle_Create(x, y, cx, cy, hParent);
	if (!hEle) return NULL;
	XEle_EnableBkTransparent(hEle, FALSE);   // 我们要画自己的背景
	auto* p = _XLoad_GetOrCreate((HXCGUI)hEle);
	if (!p){
		// 不可能 (XEle_Create 返回了合法 HELE), 但兜底
		return hEle;
	}
	p->ownEle    = TRUE;
	p->running   = TRUE;
	p->startTick = ::GetTickCount();
	_XLoad_StartTimer((HXCGUI)hEle);
	return hEle;
}

BOOL CXLoading::AttachEle(HELE hEle)
{
	if (!XC_IsHELE((HXCGUI)hEle)) return FALSE;
	auto* p = _XLoad_GetOrCreate((HXCGUI)hEle);
	if (!p) return FALSE;
	p->ownEle    = FALSE;
	p->running   = TRUE;
	p->startTick = ::GetTickCount();
	_XLoad_StartTimer((HXCGUI)hEle);
	_XLoad_Redraw((HXCGUI)hEle);
	return TRUE;
}

BOOL CXLoading::AttachWnd(HWINDOW hWnd)
{
	// AttachWnd 语义 = 在窗口客户区上方建一个填满的子 HELE, 走 Create 同款绘制流程.
	// 直接给 hWnd 挂 XE_PAINT 会被 window 内子元素覆盖, 看不到效果. 用子 HELE + 置顶才靠谱.
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;
	auto& g = LG();

	// 同一 hWnd 二次 AttachWnd → 先 Detach 释放旧子元素.
	if (g.userToHost.find((HXCGUI)hWnd) != g.userToHost.end()){
		Detach((HXCGUI)hWnd);
	}

	// 客户区减内填充 — 用户也可能 XWnd_SetPadding 留出标题栏/工具栏区不被遮.
	RECT rcCli{};
	XWnd_GetClientRect(hWnd, &rcCli);
	paddingSize_ pad{ 0, 0, 0, 0 };
	XWnd_GetPadding(hWnd, &pad);
	int x  = rcCli.left + pad.leftSize;
	int y  = rcCli.top  + pad.topSize;
	int cx = (rcCli.right  - rcCli.left) - pad.leftSize - pad.rightSize;
	int cy = (rcCli.bottom - rcCli.top)  - pad.topSize  - pad.bottomSize;
	if (cx < 1) cx = 1;
	if (cy < 1) cy = 1;

	HELE hEle = XEle_Create(x, y, cx, cy, (HXCGUI)hWnd);
	if (!hEle) return FALSE;

	XEle_EnableTopmost(hEle, TRUE);              // 置顶 → 盖住窗口所有同级元素
	XEle_EnableBkTransparent(hEle, FALSE);       // 我们自画背景
	// 窗口即使未启用布局, 这两行也能让子 HELE 在窗口 resize 时自动撑满客户区.
	XWidget_LayoutItem_SetWidth (hEle, layout_size_fill, 0);
	XWidget_LayoutItem_SetHeight(hEle, layout_size_fill, 0);

	// 登记别名 → 后续 SetXxx(hWnd) / Stop(hWnd) / Detach(hWnd) 全都 Resolve 到这个子 HELE.
	g.userToHost[(HXCGUI)hWnd] = (HXCGUI)hEle;

	auto* p = _XLoad_GetOrCreate((HXCGUI)hEle);
	if (!p){
		g.userToHost.erase((HXCGUI)hWnd);
		XEle_Destroy(hEle);
		return FALSE;
	}
	p->ownEle     = TRUE;                        // 我们建的 → Stop 隐藏, Detach 销毁
	p->hUserAlias = (HXCGUI)hWnd;                // 同时兼任"Detach 时销毁子 HELE"标志
	p->running    = TRUE;
	p->startTick  = ::GetTickCount();
	_XLoad_StartTimer((HXCGUI)hEle);
	_XLoad_Redraw((HXCGUI)hEle);
	return TRUE;
}

BOOL CXLoading::Detach(HXCGUI hHost)
{
	if (!hHost) return FALSE;
	auto& g = LG();
	// Resolve: AttachWnd 路径下用户传 hWnd, registry 用子 HELE 当 key.
	HXCGUI hKey = _XLoad_Resolve(hHost);
	auto it = g.registry.find(hKey);
	if (it == g.registry.end()) return FALSE;
	_XLoad_StopTimer(hKey);
	_XLoad_DestroyFont(it->second);
	// 反挂 3 个事件 (XEle_RemoveEventC / XWnd_RemoveEventC 对 C/C1 通用).
	if (XC_IsHELE(hKey)){
		HELE hEle = (HELE)hKey;
		XEle_RemoveEventC(hEle, XE_PAINT,    (void*)&_XLoad_OnPaint);
		XEle_RemoveEventC(hEle, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
		XEle_RemoveEventC(hEle, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
	} else if (XC_IsHWINDOW(hKey)){
		HWINDOW hWnd = (HWINDOW)hKey;
		XWnd_RemoveEventC(hWnd, XE_PAINT,    (void*)&_XLoad_OnPaint);
		XWnd_RemoveEventC(hWnd, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
		XWnd_RemoveEventC(hWnd, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
	}
	// AttachWnd 路径: 销毁我们自己建的子 HELE + 清 alias.
	HXCGUI hAlias    = it->second.hUserAlias;
	BOOL   destroyMe = (hAlias != NULL && XC_IsHELE(hKey));
	g.registry.erase(it);
	if (hAlias) g.userToHost.erase(hAlias);
	if (destroyMe){
		XEle_Destroy((HELE)hKey);
		// 父窗口接管重绘, 子元素已不存在, 不再 redraw 自己.
		if (hAlias && XC_IsHWINDOW(hAlias)) _XLoad_Redraw(hAlias);
	} else {
		_XLoad_Redraw(hKey);
	}
	return TRUE;
}

BOOL CXLoading::HasAttached(HXCGUI hHost)
{
	return _XLoad_GetEntry(hHost) ? TRUE : FALSE;
}

BOOL CXLoading::Start(HXCGUI hHost)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->running   = TRUE;
	p->startTick = ::GetTickCount();
	// 自建元素 (Create / AttachWnd 子 HELE) 若先前 Stop 隐过 → 重新显示.
	if (p->ownEle && XC_IsHELE(p->hHost)) XWidget_Show((HELE)p->hHost, TRUE);
	_XLoad_StartTimer(p->hHost);
	_XLoad_Redraw(p->hHost);
	return TRUE;
}

BOOL CXLoading::Stop(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p) return FALSE;
	p->running = FALSE;
	_XLoad_StopTimer(p->hHost);
	// 自建元素 → 直接隐藏 (Start 时再显示). 这样宿主原内容/窗口背景不被遮.
	// 非自建 (AttachEle) → 留宿主可见, 走 让出 paint 路径 redraw 一次.
	if (p->ownEle && XC_IsHELE(p->hHost)){
		XWidget_Show((HELE)p->hHost, FALSE);
	} else {
		_XLoad_Redraw(p->hHost);   // 让让出 paint 立即生效
	}
	return TRUE;
}

BOOL CXLoading::IsRunning(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return (p && p->running) ? TRUE : FALSE;
}

// ===== 风格 =====
BOOL CXLoading::SetStyle(HXCGUI hHost, xloading_style_ style)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->style     = style;
	p->startTick = ::GetTickCount();   // 切风格重置相位避免突变
	return TRUE;
}
xloading_style_ CXLoading::GetStyle(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->style : xloading_style_spinner;
}

// ===== 尺寸 =====
BOOL CXLoading::SetSize(HXCGUI hHost, int cx, int cy)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->sizeCx = (cx < 8) ? 8 : cx;
	p->sizeCy = (cy < 8) ? 8 : cy;
	return TRUE;
}
void CXLoading::GetSize(HXCGUI hHost, int* pcx, int* pcy)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p){
		if (pcx) *pcx = kLoad_DefaultSize;
		if (pcy) *pcy = kLoad_DefaultSize;
		return;
	}
	if (pcx) *pcx = p->sizeCx;
	if (pcy) *pcy = p->sizeCy;
}

// ===== 文本 =====
BOOL CXLoading::SetText(HXCGUI hHost, const wchar_t* pText)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->text = (pText ? pText : L"");
	_XLoad_Redraw(p->hHost);
	return TRUE;
}
const wchar_t* CXLoading::GetText(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->text.c_str() : NULL;
}

BOOL CXLoading::SetFontSize(HXCGUI hHost, int pt)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	if (pt < 6)  pt = 6;
	if (pt > 72) pt = 72;
	if (p->fontPt == pt) return TRUE;
	p->fontPt = pt;
	_XLoad_DestroyFont(*p);   // 下次 OnPaint 会按新 pt 重建
	_XLoad_Redraw(p->hHost);
	return TRUE;
}

int CXLoading::GetFontSize(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->fontPt : kLoad_TextFontPt;
}

// ===== 主题 / 颜色 =====
BOOL CXLoading::SetTheme(HXCGUI hHost, xloading_theme_ theme)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->theme = theme;
	return TRUE;
}
xloading_theme_ CXLoading::GetTheme(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->theme : xloading_theme_dark;
}

BOOL CXLoading::SetTextColor(HXCGUI hHost, COLORREF color)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->customText = color;
	p->theme      = xloading_theme_custom;
	return TRUE;
}
COLORREF CXLoading::GetTextColor(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p) return kLoad_DarkText;
	_XLoad_Colors c;
	_XLoad_ResolveColors(*p, &c);
	return c.text;
}

BOOL CXLoading::SetBkColor(HXCGUI hHost, COLORREF color)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->customBg = color;
	p->theme    = xloading_theme_custom;
	return TRUE;
}
COLORREF CXLoading::GetBkColor(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p) return kLoad_DarkBg;
	_XLoad_Colors c;
	_XLoad_ResolveColors(*p, &c);
	return c.bg;
}

BOOL CXLoading::SetAccentColor(HXCGUI hHost, COLORREF color)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->customAccent = color;
	p->theme        = xloading_theme_custom;
	return TRUE;
}
COLORREF CXLoading::GetAccentColor(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p) return kLoad_DarkAccent;
	_XLoad_Colors c;
	_XLoad_ResolveColors(*p, &c);
	return c.accent;
}

// ===== 圆角 =====
BOOL CXLoading::SetCornerRadius(HXCGUI hHost, int radius)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	int r = (radius < 0) ? 0 : radius;
	p->cornerLT = p->cornerRT = p->cornerRB = p->cornerLB = r;
	return TRUE;
}
int CXLoading::GetCornerRadius(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->cornerLT : 0;
}
BOOL CXLoading::SetCornerRadiusEx(HXCGUI hHost, int leftTop, int rightTop, int rightBottom, int leftBottom)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	p->cornerLT = (leftTop     < 0) ? 0 : leftTop;
	p->cornerRT = (rightTop    < 0) ? 0 : rightTop;
	p->cornerRB = (rightBottom < 0) ? 0 : rightBottom;
	p->cornerLB = (leftBottom  < 0) ? 0 : leftBottom;
	return TRUE;
}
void CXLoading::GetCornerRadiusEx(HXCGUI hHost, int* pLT, int* pRT, int* pRB, int* pLB)
{
	auto* p = _XLoad_GetEntry(hHost);
	if (!p){
		if (pLT) *pLT = 0;
		if (pRT) *pRT = 0;
		if (pRB) *pRB = 0;
		if (pLB) *pLB = 0;
		return;
	}
	if (pLT) *pLT = p->cornerLT;
	if (pRT) *pRT = p->cornerRT;
	if (pRB) *pRB = p->cornerRB;
	if (pLB) *pLB = p->cornerLB;
}

// ===== 速度 =====
BOOL CXLoading::SetSpeed(HXCGUI hHost, float speed)
{
	auto* p = _XLoad_GetOrCreate(hHost);
	if (!p) return FALSE;
	if (speed < 0.1f) speed = 0.1f;
	if (speed > 5.0f) speed = 5.0f;
	p->speed = speed;
	return TRUE;
}
float CXLoading::GetSpeed(HXCGUI hHost)
{
	auto* p = _XLoad_GetEntry(hHost);
	return p ? p->speed : 1.0f;
}

// ===== 全局清理 =====
void CXLoading::Cleanup()
{
	auto& g = LG();
	// 先收集 AttachWnd 路径下我们自己建的子 HELE — 一会儿要 XEle_Destroy 它们.
	std::vector<HELE> ownedChildren;
	ownedChildren.reserve(g.registry.size());
	// 停所有 timer (避免 timer 回调引用已清的 entry) + 销毁字体 + 反挂宿主事件.
	for (auto& kv : g.registry){
		HXCGUI hHost = kv.first;
		_XLoad_StopTimer(hHost);
		_XLoad_DestroyFont(kv.second);
		if (XC_IsHELE(hHost)){
			HELE h = (HELE)hHost;
			XEle_RemoveEventC(h, XE_PAINT,    (void*)&_XLoad_OnPaint);
			XEle_RemoveEventC(h, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
			XEle_RemoveEventC(h, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
			if (kv.second.hUserAlias) ownedChildren.push_back(h);   // AttachWnd 建的, 待销毁
		} else if (XC_IsHWINDOW(hHost)){
			HWINDOW w = (HWINDOW)hHost;
			XWnd_RemoveEventC(w, XE_PAINT,    (void*)&_XLoad_OnPaint);
			XWnd_RemoveEventC(w, XE_XC_TIMER, (void*)&_XLoad_OnTimer);
			XWnd_RemoveEventC(w, XE_DESTROY,  (void*)&_XLoad_OnDestroy);
		}
	}
	g.registry.clear();
	g.userToHost.clear();
	// XEle_Destroy 必须在 registry 清空之后 (否则会触发 OnDestroy 回调修改正在遍历的容器).
	for (HELE h : ownedChildren){
		if (XC_IsHELE((HXCGUI)h)) XEle_Destroy(h);
	}
}
