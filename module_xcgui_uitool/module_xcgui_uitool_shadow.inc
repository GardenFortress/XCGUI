// 文件编码: UTF-8 with BOM (与本目录其他 module_xcgui_*.h/.cpp 保持一致).
//============================================================================
// module_xcgui_uitool_shadow.inc — CXShadow split (calendar 风格软阴影)
// 仅由 module_xcgui_uitool.cpp #include; 勿单独编译.
//
// halo: _XUITool::DrawDropShadow (draw_softshadow.inc, 与 CXCalendarCard 同内核).
// margin: _XUITool::kShadowMargin 四边等宽 (与 tooltip / calendar 一致).
//============================================================================

#ifndef _XCGUI_UITOOL_AGGREGATED_
#error "module_xcgui_uitool_shadow.inc must be included from module_xcgui_uitool.cpp only"
#endif

#include "module_xcgui_uitool_shadow_internal.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#include <commctrl.h>          // SetWindowSubclass / RemoveWindowSubclass
#pragma comment(lib, "comctl32.lib")

//============================================================================
// 工具函数 / 颜色解码
//============================================================================
//
// XCGUI COLORREF 编码 0xAABBGGRR (alpha 在高字节, 与 GDI 标准 0x00BBGGRR 兼容,
// alpha=0 时 GDI 视为无 alpha).
//============================================================================
static inline BYTE GetRGBA_R(COLORREF c){ return (BYTE)((c) & 0xFF); }
static inline BYTE GetRGBA_G(COLORREF c){ return (BYTE)(((c) >> 8) & 0xFF); }
static inline BYTE GetRGBA_B(COLORREF c){ return (BYTE)(((c) >> 16) & 0xFF); }
static inline BYTE GetRGBA_A(COLORREF c){ return (BYTE)(((c) >> 24) & 0xFF); }

namespace {

static xuitool_theme_ _CXShadow_ToUitoolTheme(int theme)
{
    switch (theme){
    case xshadow_theme_light: return xuitool_theme_light;
    case xshadow_theme_dark:  return xuitool_theme_dark;
    case xshadow_theme_auto:  return xuitool_theme_auto;
    default:                  return xuitool_theme_custom;
    }
}

static COLORREF _CXShadow_ThemeBorderColor(xuitool_theme_ eff)
{
    if (eff == xuitool_theme_light)
        return _XUITool::WithAlpha(_XUITool::kLightText, 38);
    return _XUITool::WithAlpha(_XUITool::kDarkText, 40);
}

//----------------------------------------------------------------------------
// 主窗口 HWINDOW → CXShadow* 派发表 (状态在 uitool.cpp 单点定义).
//----------------------------------------------------------------------------
static CXShadow* _CXShadow_FindByHWindow(HWINDOW hWnd){
    CXShadowModule::HostLock _lk;
    auto& map = CXShadowModule::HostMap();
    auto it = map.find(hWnd);
    return (it != map.end()) ? it->second : nullptr;
}

}  // namespace

//============================================================================
// 构造 / 析构
//============================================================================
CXShadow::CXShadow(){
    // 仅初始化, 不创建任何资源. 用户调 AttachToWnd 才真正起来.
}

CXShadow::~CXShadow(){
    Detach();
}

//============================================================================
// AttachToWnd
//
// 5 个接管动作:
//   1. 保存主窗原状态 (transparent / padding / layout)
//   2. 强设 transparent_shaped + alpha 255 (让主窗拥有 alpha 通道)
//   3. EnableLayout(TRUE) (子元素 layout 避开 padding 圈)
//   4. 计算 + Apply padding (为 shadow 留位)
//   5. 钩 XWM_WINDPROC + WM_PAINT 事件 + 注入 host map
//============================================================================
BOOL CXShadow::AttachToWnd(HWINDOW hWnd){
    CXShadowModule::EnsureInitialized();
    if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;

    // 已附加同一窗口 -> no-op
    if (m_hAttachedWnd == hWnd) return TRUE;

    // 附加到不同窗口 -> 先解除
    if (m_hAttachedWnd != NULL) Detach();

    HWND raw = ::XWnd_GetHWND(hWnd);
    if (!raw) return FALSE;

    m_hAttachedWnd = hWnd;
    m_hMainHwnd    = raw;

    // 注册全局表
    {
        CXShadowModule::HostLock _lk;
        CXShadowModule::HostMap()[hWnd] = this;
    }
    // 注册全局实例集 (供 SetGlobalTheme 广播 / WM_SETTINGCHANGE 等用)
    {
        CXShadowModule::InstancesLock _lk;
        CXShadowModule::Instances().insert(this);
    }

    // 1) 记录主窗原状态 (Detach 还原用)
    CaptureMainStyles();

    // 2) 设透明 + alpha 255 (主窗被赋予 32-bit alpha 通道; 阴影 halo 与主窗贴合必需)
    ::XWnd_SetTransparentType(hWnd, window_transparent_shaped);
    ::XWnd_SetTransparentAlpha(hWnd, 255);

    // 3) layout. 不动 EnableDrawBk — 默认 TRUE 是 XCGUI 为了渲染 backbuffer 初始化的,
    //    我们在 OnWndPaintImpl 里设 *pbHandled = TRUE 就足以跳过默认背景填充.
    //    实验: EnableDrawBk(FALSE) 会让 XCGUI 跳过整个画布 / WM_PAINT 派发, 导致窗全透.
    ::XWnd_EnableLayout(hWnd, TRUE);    // 元素 layout 避开 padding 圈

    // 4) DPI → padding
    RefreshDpi();
    ApplyPadding();   // 最大化状态在后面由 WM_SIZE 检测后调 ClearPadding

    // 5) 事件钩子
    HookEvents();

    // 初始状态
    m_isMaximized = (::IsZoomed(m_hMainHwnd) != FALSE);
    m_isMinimized = (::IsIconic(m_hMainHwnd) != FALSE);
    m_isActive    = (::GetForegroundWindow() == m_hMainHwnd);

    if (m_isMaximized) ClearPadding();   // 最大化起始 → 去 padding

    // 6) 主题: 优先 adopt 用户设的全局主题 (CXShadow::SetGlobalTheme), 否则用 m_theme
    //    默认值 (xshadow_theme_auto). 总之 attach 完后 m_shadowColor 等是 *已应用* 状态.
    int gt = CXShadowModule::GlobalTheme();
    if (gt != xshadow_theme_custom){
        m_theme.store(gt);
    }
    int t = m_theme.load();
    if (t != xshadow_theme_custom){
        ApplyThemePreset(t);
    }

    // 7) WS_MAXIMIZEBOX strip: 联合状态 m_snapDisabled || m_maxDisabled.
    //    默认 m_snapDisabled=true (本类默认 *字面禁 snap*, 与 CXBlur 反转),
    //    m_maxDisabled=false → wantStrip=true → 默认 strip 最大化按钮变灰
    //    (消除拖边 snap preview UI 是 strip 的核心动机). Win+↑ / API 最大化
    //    仍可用 (子类 proc WINDOWPOSCHANGING IsZoomed 放行).
    UpdateMaxBoxState();

    // 触发首次重绘 (主窗可能还未显示, XCGUI 会在首次 show 时走 WM_PAINT)
    ::XWnd_Redraw(hWnd, FALSE);
    return TRUE;
}

void CXShadow::Detach(){
    if (m_hAttachedWnd == NULL) return;

    UnhookEvents();

    // 还原主窗状态 (transparent / padding / layout / drawBk)
    RestoreMainStyles();

    {
        CXShadowModule::HostLock _lk;
        CXShadowModule::HostMap().erase(m_hAttachedWnd);
    }
    {
        CXShadowModule::InstancesLock _lk;
        CXShadowModule::Instances().erase(this);
    }

    m_hAttachedWnd = NULL;
    m_hMainHwnd    = NULL;
    m_isMaximized  = false;
    m_isMinimized  = false;
    m_isActive     = true;
    m_saved        = false;
}

BOOL    CXShadow::IsAttached     () const { return m_hAttachedWnd != NULL; }
HWINDOW CXShadow::GetAttachedWnd () const { return m_hAttachedWnd; }
BOOL    CXShadow::IsMaximized    () const { return m_isMaximized ? TRUE : FALSE; }

//============================================================================
// 事件钩子:
//   - XWM_WINDPROC : 获取 WM_SIZE / WM_NCHITTEST / WM_DPICHANGED / WM_ACTIVATE /
//                    WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE / WM_DESTROY.
//   - WM_PAINT     : 直接拦截主窗绘制, 走我们的 shadow halo + inner bg + border.
//============================================================================
namespace {

//----------------------------------------------------------------------------
// Win32 子类化 — WM_NCHITTEST 专用通道
//
// 为什么不走 XWM_WINDPROC?
//   XCGUI 的 XWM_WINDPROC 对 WM_NCHITTEST 的处理是: 它会 *先* 自己处理 (识别
//   caption / button 等), 把结果当做最终值, 再决定要不要派给我们. 在某些路径下
//   它根本不让回调改返回值 → 我们返 HTTRANSPARENT 没有效果, 阴影区还是吃鼠标.
//
//   SetWindowSubclass 注入到 Win32 的 wndproc 链, 比 XCGUI 的处理更靠前. 我们
//   先决定 HT 结果, 再决定要不要 Def 给 XCGUI. 这是最稳的 click-through 方案.
//
// dwRefData = CXShadow*; 仅处理 WM_NCHITTEST + WM_NCDESTROY 自卸. 其它消息直接
// DefSubclassProc 走原链.
//----------------------------------------------------------------------------
constexpr UINT_PTR kCXShadowSubclassId  = 0x58536864UL;  // 'XShd'

// 诊断 toggle: 打开后会向 DebugView 打 NCHITTEST trace, 用于定位 click-through 故障.
// 关闭即 0 开销 (编译期常量).
#ifndef XSHADOW_DEBUG_NCHIT
#define XSHADOW_DEBUG_NCHIT 0
#endif

// ----- Snap target geometry detection -----
// 与 module_xcgui_blur.cpp 的 XBlur_IsSnapTargetGeom 同语义 (容差 2 px). 检测
// WINDOWPOS 的目标矩形是否匹配 snap layout (full / 左右半屏 / 上下半屏 / 四角).
// EnableSnap(FALSE) 时, 在 WM_WINDOWPOSCHANGING 用此函数 + SWP_NOMOVE|SWP_NOSIZE
// 阻止 Aero Snap 落位.
static bool _CXShadow_IsSnapTargetGeom(const WINDOWPOS* wp, HWND raw){
    if (!wp || !raw) return false;
    HMONITOR hMon = ::MonitorFromWindow(raw, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!hMon || !::GetMonitorInfoW(hMon, &mi)) return false;
    const RECT& wa = mi.rcWork;
    int waW = wa.right - wa.left;
    int waH = wa.bottom - wa.top;
    if (waW <= 0 || waH <= 0) return false;

    auto eq = [](int a, int b){ int d = a - b; return (d < 0 ? -d : d) <= 2; };
    int newL = wp->x;
    int newT = wp->y;
    int newR = wp->x + wp->cx;
    int newB = wp->y + wp->cy;

    bool atL = eq(newL, wa.left);
    bool atT = eq(newT, wa.top);
    bool atR = eq(newR, wa.right);
    bool atB = eq(newB, wa.bottom);

    bool fullW = eq(wp->cx, waW);
    bool fullH = eq(wp->cy, waH);
    bool halfW = eq(wp->cx, waW / 2);
    bool halfH = eq(wp->cy, waH / 2);

    if (fullW && fullH && atL && atT) return true;            // 全屏 = maximize
    if (halfW && fullH && (atL || atR) && atT && atB) return true;  // 左/右半屏
    if (fullW && halfH && atL && atR && (atT || atB)) return true;  // 上/下半屏
    if (halfW && halfH && (atL || atR) && (atT || atB)) return true;// 四角
    return false;
}

static LRESULT CALLBACK _CXShadow_HwndSubclassProc(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    // ----- Snap / 最大化 阻止拦截 (默认: snap 阻止, 最大化允许) -----
    // 在子类 proc 处理, 比 XCGUI wndproc 更靠前 — Aero Snap / SC_MAXIMIZE 在到达
    // XCGUI / DefWindowProc 之前就被拦截或修改.
    //
    // *EnableSnap(TRUE) 默认值 = 阻止 snap. 与 CXBlur::EnableSnap 反转 —
    //  shadow 类下 snap 状态视觉打断比较严重, 默认就阻止.*
    // *EnableMaximize(TRUE) 默认值 = 允许最大化. 与 XWnd_EnableMaxWindow 一致.*
    if (msg == WM_WINDOWPOSCHANGING && dwRefData){
        CXShadow* p = reinterpret_cast<CXShadow*>(dwRefData);
        // 区分 "用户最大化" vs "Aero Snap 落位": IsZoomed=true 时是真最大化
        // (任何路径: SC_MAXIMIZE / ShowWindow(SW_MAXIMIZE) / SetWindowPlacement /
        // WS_MAXIMIZE 创建属性 / 拖到顶 snap-to-max), 几何虽然 = 全工作区, 必须
        // 放行. 仅在窗口处于非最大化状态时才执行 snap 几何过滤.
        if (p->IsSnapEnabled() && !::IsZoomed(hwnd)){
            WINDOWPOS* wpos = reinterpret_cast<WINDOWPOS*>(lp);
            if (wpos && _CXShadow_IsSnapTargetGeom(wpos, hwnd)){
                wpos->flags |= SWP_NOMOVE | SWP_NOSIZE;
            }
        }
        // fall through to DefSubclassProc — 用修改后的 wpos 让消息链继续
    }
    if (msg == WM_SYSCOMMAND && dwRefData){
        // EnableMaximize(FALSE) → 吞 SC_MAXIMIZE (Win+Up / 双击标题栏 / 系统菜单).
        // 默认 (m_maxDisabled=false) 此分支零开销.
        CXShadow* p = reinterpret_cast<CXShadow*>(dwRefData);
        if ((wp & 0xFFF0) == SC_MAXIMIZE && !p->IsMaximizeEnabled()){
            return 0;   // swallow, 不调 DefSubclassProc
        }
    }

    if (msg == WM_NCHITTEST){
        CXShadow* p = reinterpret_cast<CXShadow*>(dwRefData);
        if (p){
            int sx = (int)(short)LOWORD(lp);
            int sy = (int)(short)HIWORD(lp);
            LRESULT hit = p->ComputeNcHitTest(sx, sy);
        #if XSHADOW_DEBUG_NCHIT
            // *NOT spam guard*: NCHITTEST 触发频次 = 鼠标 move + WM_SETCURSOR.
            // 移动鼠标会 ~60Hz 打日志. 调试完记得 #define XSHADOW_DEBUG_NCHIT 0.
            wchar_t buf[160];
            swprintf_s(buf, L"[CXShadow] NCHIT scr=(%d,%d) -> %lld %s\n",
                sx, sy, (long long)hit,
                hit == HTTRANSPARENT ? L"TRANSPARENT (click-thru)" :
                hit == HTNOWHERE     ? L"NOWHERE (-> XCGUI)" :
                hit == HTLEFT        ? L"LEFT (resize)" :
                hit == HTRIGHT       ? L"RIGHT (resize)" :
                hit == HTTOP         ? L"TOP (resize)" :
                hit == HTBOTTOM      ? L"BOTTOM (resize)" :
                hit == HTTOPLEFT     ? L"TOPLEFT (resize)" :
                hit == HTTOPRIGHT    ? L"TOPRIGHT (resize)" :
                hit == HTBOTTOMLEFT  ? L"BOTTOMLEFT (resize)" :
                hit == HTBOTTOMRIGHT ? L"BOTTOMRIGHT (resize)" : L"?");
            ::OutputDebugStringW(buf);
        #endif
            // HTNOWHERE = "我不管 (落到 XCGUI 默认逻辑)". 其余都直接返回, 越过 XCGUI.
            if (hit != HTNOWHERE) return hit;
        }
    #if XSHADOW_DEBUG_NCHIT
        else {
            ::OutputDebugStringW(L"[CXShadow] NCHIT dwRefData=NULL!\n");
        }
    #endif
    }
    // WM_GETMINMAXINFO: 把系统 / XCGUI / 用户给的 ptMinTrackSize 沿 4 边各加上
    // 当前 padding, 让 *visible inner 最小尺寸* = 原配置最小尺寸 — 不被阴影边框
    // padding 吃掉.
    //
    // 时序: 先 DefSubclassProc 让 XCGUI / Win32 填好 MINMAXINFO, 我们 *post-process*
    // 加 padding. 这样无论 XCGUI 后来是否拦 WM_GETMINMAXINFO 改了 ptMinTrackSize,
    // 我们的累加都生效 (子类链 wndproc 顺序保证).
    //
    // *单位*: ptMinTrackSize 是 *物理像素* (Win32 原生约定, NCCALCSIZE/SetWindowPos
    // 路径上的尺寸全部物理). m_curPadL/T/R/B 是 *逻辑像素* (与 XWnd_SetPadding 输入
    // 一致). 必须 × dpiScale 物理化, 否则 DPI 模式下 minSize 加得太少, padding 仍
    // 吃 inner.
    //
    // 边界:
    //   * snap / maximized 时 padding=0, 加 0 = no-op, 不影响.
    //   * ptMaxTrackSize / ptMaxSize 不动 — 最大化的时候 padding 也清掉, 系统给的
    //     工作区尺寸刚好.
    if (msg == WM_GETMINMAXINFO){
        LRESULT r = ::DefSubclassProc(hwnd, msg, wp, lp);
        CXShadow* p = reinterpret_cast<CXShadow*>(dwRefData);
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        if (p && mmi){
            float s = p->GetDpiScale();
            int padX_phys = (int)((float)p->GetCurPadX() * s + 0.5f);
            int padY_phys = (int)((float)p->GetCurPadY() * s + 0.5f);
            if (padX_phys > 0) mmi->ptMinTrackSize.x += padX_phys;
            if (padY_phys > 0) mmi->ptMinTrackSize.y += padY_phys;
        }
        return r;
    }
    if (msg == WM_NCDESTROY){
        // 主窗销毁时自卸; 避免悬挂引用. UnhookEvents 也会显式 remove, 这里只是 fallback.
        ::RemoveWindowSubclass(hwnd, _CXShadow_HwndSubclassProc, uIdSubclass);
    }
    return ::DefSubclassProc(hwnd, msg, wp, lp);
}

static int CALLBACK _CXShadow_WindProcCB(HWINDOW hWnd, UINT msg,
                                         WPARAM wParam, LPARAM lParam,
                                         BOOL* pbHandled)
{
    CXShadow* p = _CXShadow_FindByHWindow(hWnd);
    if (!p) return 0;
    return p->OnWndProcImpl(hWnd, msg, wParam, lParam, pbHandled);
}

static int CALLBACK _CXShadow_PaintCB(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled){
    CXShadow* p = _CXShadow_FindByHWindow(hWnd);
    if (!p) return 0;
    return p->OnWndPaintImpl(hWnd, hDraw, pbHandled);
}
}  // namespace

void CXShadow::HookEvents(){
    if (m_eventsHooked || m_hAttachedWnd == NULL) return;
    if (!XC_IsHWINDOW((HXCGUI)m_hAttachedWnd)) return;
    ::XWnd_RegEventC1(m_hAttachedWnd, XWM_WINDPROC, (void*)_CXShadow_WindProcCB);
    // 窗口绘制事件: WM_PAINT (0x000F) — XE_PAINT(=2) 是 *元素* 事件, 不会派给 window!
    ::XWnd_RegEventC1(m_hAttachedWnd, WM_PAINT,     (void*)_CXShadow_PaintCB);

    // Win32 子类化 — WM_NCHITTEST 走子类, 比 XCGUI wndproc 更靠前 (click-through 必备).
    //
    // *关键*: SetWindowSubclass 用同一 id 重复调用是 no-op (不会移到 top), 但 XCGUI
    // 内部某些 API (XWnd_EnableDragBorder / XWnd_EnableDragCaption / 第一次 Show)
    // *可能* 在我们 Attach 后才装它自己的子类 → 我们就被压到 chain 下层, NCHITTEST
    // 落到 XCGUI 的子类返回 HTCLIENT 后就不再下传, 我们的 HTTRANSPARENT 永远不被 system 看见.
    //
    // 防御: Remove + Add. RemoveWindowSubclass 后 SetWindowSubclass 会重新插到 top.
    // 这样在 HookEvents 时刻是 top; 后续如果 XCGUI 又装了, 再 first WM_PAINT 时
    // (BumpSubclassToTop) 再放一次. 见 _CXShadow_PaintCB.
    ForceSubclassToTop();

    m_eventsHooked = true;
}

void CXShadow::ForceSubclassToTop(){
    if (!m_hMainHwnd || !::IsWindow(m_hMainHwnd)) return;
    // 先 remove, 再 add → 强制插到 chain 顶部.
    if (m_subclassInstalled){
        ::RemoveWindowSubclass(m_hMainHwnd, _CXShadow_HwndSubclassProc, kCXShadowSubclassId);
        m_subclassInstalled = false;
    }
    BOOL ok = ::SetWindowSubclass(m_hMainHwnd, _CXShadow_HwndSubclassProc,
                                  kCXShadowSubclassId, (DWORD_PTR)this);
    if (ok) m_subclassInstalled = true;
#if XSHADOW_DEBUG_NCHIT
    wchar_t buf[160];
    swprintf_s(buf, L"[CXShadow] ForceSubclassToTop hwnd=%p = %s (err=%lu) this=%p\n",
               (void*)m_hMainHwnd, ok ? L"OK" : L"FAIL", ::GetLastError(), (void*)this);
    ::OutputDebugStringW(buf);
#endif
}

void CXShadow::UnhookEvents(){
    if (!m_eventsHooked) return;
    m_eventsHooked = false;

    // 子类先卸 (失败也无所谓, WM_NCDESTROY 时会自卸)
    if (m_subclassInstalled && m_hMainHwnd && ::IsWindow(m_hMainHwnd)){
        ::RemoveWindowSubclass(m_hMainHwnd, _CXShadow_HwndSubclassProc, kCXShadowSubclassId);
    }
    m_subclassInstalled = false;

    // 主窗已被 XCGUI 析构 → HWINDOW 句柄失效 → 跳过 RemoveEvent
    // (XCGUI 销毁窗口时会自动清其事件表). 避免 'XWnd_RemoveEventC 输入句柄可能无效'.
    if (m_hAttachedWnd == NULL) return;
    if (!XC_IsHWINDOW((HXCGUI)m_hAttachedWnd)) return;
    ::XWnd_RemoveEventC(m_hAttachedWnd, XWM_WINDPROC, (void*)_CXShadow_WindProcCB);
    ::XWnd_RemoveEventC(m_hAttachedWnd, WM_PAINT,     (void*)_CXShadow_PaintCB);
}

//============================================================================
// 主窗口消息处理
//
// 设计要点:
//   * 阴影 = 主窗自绘, 不需 WM_WINDOWPOSCHANGING 异步同步, 移窗 / resize 天然同步
//   * WM_SIZE: padding 切换 (max / snap ↔ restore)
//   * WM_NCHITTEST: shadow halo → HTTRANSPARENT, 内圈 4 边 → HT* (走系统 resize)
//============================================================================
int CXShadow::OnWndProcImpl(HWINDOW hWnd, UINT msg,
                             WPARAM wParam, LPARAM lParam,
                             BOOL* pbHandled)
{
    switch (msg){

    case WM_ENTERSIZEMOVE:
        m_inSizeMove = true;
        break;

    case WM_EXITSIZEMOVE:
        m_inSizeMove = false;
        SyncWindowState();
        if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
        break;

    case WM_SIZE:
        // WM_SIZE 是处理状态变化 (max/min/restore/snap) + DPI 跟随的统一入口.
        //
        // 1) DPI 防御: 系统从小缩放改大时, XCGUI 的 EnableDPI 会处理 WM_DPICHANGED 但
        //    我们的 hook 链有时收不到 (XCGUI 自己 swallow). 在 WM_SIZE 里再检测一遍,
        //    若 dpi 变了 → RefreshDpi (不直接 ApplyPadding, 留给 SyncWindowState).
        {
            int oldDpi = m_dpi;
            RefreshDpi();
            (void)oldDpi;
        }

        if (wParam == SIZE_MINIMIZED){
            m_isMinimized = true;
        } else {
            // SIZE_MAXIMIZED / SIZE_RESTORED 都走 SyncWindowState 统一判定:
            //   - 真最大化 → ClearPadding
            //   - aero snap → ClearPadding
            //   - 普通 restore → ApplyPadding
            SyncWindowState();
        }
        break;

    case WM_WINDOWPOSCHANGED:
        // aero snap 部分情况只发 WM_WINDOWPOSCHANGED 而不发 WM_SIZE 的 SIZE_RESTORED
        // (例如从 snap 到另一个 snap 位置). 这里再 sync 一次保险.
        if (!m_isMinimized) SyncWindowState();
        break;

    // WM_NCHITTEST 不在这里处理 — 已走 SetWindowSubclass 通道.
    //   见 _CXShadow_HwndSubclassProc 与 HookEvents 中的 SetWindowSubclass 调用.

    case WM_SETTINGCHANGE:
        // 系统主题切换 (浅/深 模式) 会派 WM_SETTINGCHANGE("ImmersiveColorSet").
        // 仅当用户主题为 auto 时, 自动跟随重新应用一遍. 其他主题保持不变.
        if (m_theme.load() == xshadow_theme_auto){
            ApplyThemePreset(xshadow_theme_auto);
            if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
        }
        break;

    case WM_ACTIVATE:
        {
            WORD a = LOWORD(wParam);
            bool nowActive = (a != WA_INACTIVE);
            if (nowActive != m_isActive){
                m_isActive = nowActive;
                if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
            }
        }
        break;

    case WM_DPICHANGED:
        // 关键: 必须先 RefreshDpi (即使 WM_SIZE 里也会再读一次, 这里保证如果系统先发
        // WM_DPICHANGED 再发 WM_SIZE, 第一帧 SyncWindowState 用的就是新 DPI).
        RefreshDpi();
        SyncWindowState();
        if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
        break;

    case WM_DESTROY:
        // 主窗口被销毁: 短路所有后续 XCGUI 调用 (句柄即将 invalidate).
        m_eventsHooked = false;
        m_subclassInstalled = false;   // 子类随主窗死掉, 不要再 RemoveWindowSubclass
        {
            CXShadowModule::HostLock _lk;
            CXShadowModule::HostMap().erase(m_hAttachedWnd);
        }
        {
            CXShadowModule::InstancesLock _lk;
            CXShadowModule::Instances().erase(this);
        }
        m_hAttachedWnd = NULL;
        m_hMainHwnd    = NULL;
        m_saved = false;   // 别再尝试 Detach 时还原 — 主窗已死
        // Snap 控制状态: 主窗已死, 不再 SetWindowLong, 仅清 flag.
        m_maxBoxSaved         = false;
        m_maxBoxOriginallySet = false;
        break;
    }
    (void)hWnd;
    return 0;
}

//============================================================================
// 几何 / DPI / 同步
//============================================================================
void CXShadow::RefreshDpi(){
    // XCGUI XWnd_GetDPI 返回 *物理 DPI* (Win32 标准): 100% → 96, 125% → 120,
    // 150% → 144, 200% → 192. 与 module_xcgui_video / module_xcgui_image /
    // module_xcgui_editdw / controls/XEditDW 同惯例 (该模块占绝大多数). 故
    // scale = dpi / 96.
    //
    // 注: CXBlur::RefreshDpiScale 用 /100 是误差遗留 — 在 150% 缩放下两者结果
    // 相同 (1.5), 只在 不常见 custom DPI 时分叉, 看不出, 没人改. 本模块走 /96.
    if (!m_hAttachedWnd) return;
    int dpi = ::XWnd_GetDPI(m_hAttachedWnd);
    if (dpi <= 0) dpi = 96;
    m_dpi      = dpi;
    m_dpiScale = (float)dpi / 96.0f;
}

void CXShadow::Invalidate(){
    if (m_hAttachedWnd == NULL) return;
    ::XWnd_Redraw(m_hAttachedWnd, FALSE);
}

//============================================================================
// 视觉参数 setter / getter (set 后 dirty + 立即重绘)
//============================================================================
#define DECL_SET_INT(field, member)                                             \
    void CXShadow::Set##field(int v){                                           \
        if (member.exchange(v) != v){ Invalidate(); }                           \
    }
#define DECL_GET_INT(field, member, ret_t)                                      \
    ret_t CXShadow::Get##field() const { return (ret_t)member.load(); }

void CXShadow::SetCornerRadius(int radius){
    if (radius < 0) radius = 0;
    if (m_cornerRadius.exchange(radius) != radius){ Invalidate(); }
}
int  CXShadow::GetCornerRadius() const { return m_cornerRadius.load(); }

void CXShadow::SetShadowRadius(int radius){
    if (radius < 0) radius = 0;
    if (m_shadowRadius.exchange(radius) != radius){ Invalidate(); }
}
int  CXShadow::GetShadowRadius() const { return m_shadowRadius.load(); }

void CXShadow::SetShadowSpread(int spread){
    if (spread < 0) spread = 0;
    if (m_shadowSpread.exchange(spread) != spread){ Invalidate(); }
}
int  CXShadow::GetShadowSpread() const { return m_shadowSpread.load(); }

void CXShadow::SetShadowOffset(int dx, int dy){
    int oldDx = m_shadowDx.exchange(dx);
    int oldDy = m_shadowDy.exchange(dy);
    if (oldDx != dx || oldDy != dy){ Invalidate(); }
}
void CXShadow::GetShadowOffset(int* pdx, int* pdy) const {
    if (pdx) *pdx = m_shadowDx.load();
    if (pdy) *pdy = m_shadowDy.load();
}

void CXShadow::SetShadowColor(COLORREF color){
    if (m_shadowColor.exchange(color) != color){ Invalidate(); }
}
COLORREF CXShadow::GetShadowColor() const { return m_shadowColor.load(); }

void CXShadow::SetInactiveShadowColor(COLORREF color){
    if (m_inactiveShadow.exchange(color) != color){ Invalidate(); }
}
COLORREF CXShadow::GetInactiveShadowColor() const { return m_inactiveShadow.load(); }

void CXShadow::SetBorderColor(COLORREF color){
    if (m_borderColor.exchange(color) != color){ Invalidate(); }
}
COLORREF CXShadow::GetBorderColor() const { return m_borderColor.load(); }

void CXShadow::SetBorderWidth(float w){
    if (w < 0.0f) w = 0.0f;
    float old = m_borderWidth.exchange(w);
    if (old != w) Invalidate();
}
float CXShadow::GetBorderWidth() const { return m_borderWidth.load(); }

void CXShadow::SetInsetCorrection(int px){
    if (px < 0) px = 0;
    if (m_inset.exchange(px) != px){ Invalidate(); }
}
int CXShadow::GetInsetCorrection() const { return m_inset.load(); }

//============================================================================
// 主题 (含 SetGlobalTheme / WM_SETTINGCHANGE 同步)
//
// 设计 (参考 CXBlur::SetGlobalTheme):
//   * m_theme 保留 *用户意图* (custom / light / dark / auto), 不在 SetTheme
//     里把 auto 解析掉. ApplyThemePreset 内部按需把 auto 折成 light/dark.
//     这样 WM_SETTINGCHANGE 来时, 我们仍知道 "这个实例是 auto, 应跟随".
//   * SetGlobalTheme 静态接口: 写入全局原子 + 广播到所有活实例.
//   * AttachToWnd: 注册到 shadowInstances; 若 globalTheme != custom,
//     adopt 全局值.
//============================================================================

// (shadowInstances / shadowInstancesMutex / globalTheme 在文件顶部声明)

void CXShadow::SetTheme(int theme){
    // 接受 4 个合法值; 其他 (例如负数) 忽略.
    if (theme != xshadow_theme_light && theme != xshadow_theme_dark
        && theme != xshadow_theme_custom && theme != xshadow_theme_auto){
        return;
    }
    m_theme.store(theme);
    if (theme != xshadow_theme_custom){
        ApplyThemePreset(theme);   // 内部解析 auto → light/dark
    }
    Invalidate();
}
int CXShadow::GetTheme() const { return m_theme.load(); }

void CXShadow::SetGlobalTheme(int theme){
    CXShadowModule::GlobalTheme() = theme;
    // 广播到所有活实例. 拷贝一份指针快照, 避免持锁回调用户代码.
    std::vector<CXShadow*> snapshot;
    {
        CXShadowModule::InstancesLock _lk;
        auto& inst = CXShadowModule::Instances();
        snapshot.assign(inst.begin(), inst.end());
    }
    for (CXShadow* p : snapshot){
        if (p) p->SetTheme(theme);
    }
}
int CXShadow::GetGlobalTheme(){
    return CXShadowModule::GlobalTheme();
}

void CXShadow::ApplyThemePreset(int theme){
    xuitool_theme_ uit = _CXShadow_ToUitoolTheme(theme);
    xuitool_theme_ eff = _XUITool::EffectiveTheme(uit);
    _XUITool::ThemePalette pal{};
    _XUITool::ResolvePalette(uit, 0, 0, 0, &pal);
    m_borderColor.store(_CXShadow_ThemeBorderColor(eff));
    if (!m_innerBgUserSet.load()){
        m_innerBgColor.store(pal.bg);
    }
}

//============================================================================
// 内圈背景
//============================================================================
void CXShadow::SetInnerBgColor(COLORREF color){
    m_innerBgUserSet.store(true);
    if (m_innerBgColor.exchange(color) != color) Invalidate();
}

void CXShadow::ClearInnerBgColor(){
    if (!m_innerBgUserSet.exchange(false)){
        return;   // 没自定义过, no-op
    }
    xuitool_theme_ uit = _CXShadow_ToUitoolTheme(m_theme.load());
    _XUITool::ThemePalette pal{};
    _XUITool::ResolvePalette(uit, 0, 0, 0, &pal);
    if (m_innerBgColor.exchange(pal.bg) != pal.bg) Invalidate();
}

COLORREF CXShadow::GetInnerBgColor() const { return m_innerBgColor.load(); }

BOOL CXShadow::IsSystemDarkMode(){
    return _XUITool::DetectSystemDarkMode() ? TRUE : FALSE;
}

//============================================================================
// 主窗属性接管 / 还原
//============================================================================
void CXShadow::CaptureMainStyles(){
    if (m_saved || !m_hAttachedWnd) return;
    m_savedTransType = (int)::XWnd_GetTransparentType(m_hAttachedWnd);

    paddingSize_ pad = {};
    ::XWnd_GetPadding(m_hAttachedWnd, &pad);
    m_savedPadL = pad.leftSize;
    m_savedPadT = pad.topSize;
    m_savedPadR = pad.rightSize;
    m_savedPadB = pad.bottomSize;

    m_savedLayout = ::XWnd_IsEnableLayout(m_hAttachedWnd);
    m_saved = true;
}

void CXShadow::RestoreMainStyles(){
    if (!m_saved || !m_hAttachedWnd) return;
    if (!XC_IsHWINDOW((HXCGUI)m_hAttachedWnd)) return;

    ::XWnd_SetTransparentType(m_hAttachedWnd, (window_transparent_)m_savedTransType);
    ::XWnd_SetPadding (m_hAttachedWnd, m_savedPadL, m_savedPadT, m_savedPadR, m_savedPadB);
    ::XWnd_EnableLayout(m_hAttachedWnd, m_savedLayout);
    // 不需恢复 EnableDrawBk — 本类从未修改它.

    // 最大化控制: 还原 WS_MAXIMIZEBOX (若 EnableMaximize(FALSE) 期间 strip 过).
    // RestoreMaxBox 内部对 "未 strip" 是 no-op, 安全调用.
    RestoreMaxBox();

    m_curPadL = m_curPadT = m_curPadR = m_curPadB = 0;
}

//============================================================================
// Snap / 最大化 禁用支撑函数
//   StripMaxBox / RestoreMaxBox  — WS_MAXIMIZEBOX 控制 (供 EnableMaximize 用).
//   EnableSnap     / IsSnapEnabled       — 语义反转 (TRUE = 阻止 snap, 默认).
//   EnableMaximize / IsMaximizeEnabled   — 标准语义 (TRUE = 允许最大化, 默认).
//
// 与 CXBlur 不同: CXShadow 没有共享 mutex 保护的状态机, 所有访问都通过 this
// 指针 (实例级), 主线程顺序执行. 因此 SetWindowPos(SWP_FRAMECHANGED) 即使
// 同步派发 WM_WINDOWPOSCHANGING 回我们的子类 proc, 也不会有重入死锁问题 —
// 子类 proc 只读 m_snapDisabled / m_maxDisabled (atomic), 不重入 EnableSnap /
// EnableMaximize.
//
// *snap / 最大化 解耦*: 见类文档. EnableSnap 仅控制几何过滤 + SyncWindowState
// 短路, 完全不动 WS_MAXIMIZEBOX. WS_MAXIMIZEBOX 由 EnableMaximize 独立控制.
//============================================================================
void CXShadow::StripMaxBox(){
    if (m_maxBoxSaved) return;
    if (!m_hMainHwnd || !::IsWindow(m_hMainHwnd)) return;
    LONG s = ::GetWindowLongW(m_hMainHwnd, GWL_STYLE);
    m_maxBoxOriginallySet = (s & WS_MAXIMIZEBOX) != 0;
    m_maxBoxSaved = true;
    if (!m_maxBoxOriginallySet) return;     // 没 MAXIMIZEBOX, 不动
    ::SetWindowLongW(m_hMainHwnd, GWL_STYLE, s & ~WS_MAXIMIZEBOX);
    // SWP_FRAMECHANGED 让 USER32 重画标题栏按钮 (最大化按钮立刻变灰).
    ::SetWindowPos(m_hMainHwnd, NULL, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void CXShadow::RestoreMaxBox(){
    if (!m_maxBoxSaved) return;
    bool wasSet = m_maxBoxOriginallySet;
    m_maxBoxSaved         = false;
    m_maxBoxOriginallySet = false;
    if (!wasSet) return;                    // strip 时本来就没 MAXIMIZEBOX
    if (!m_hMainHwnd || !::IsWindow(m_hMainHwnd)) return;
    LONG s = ::GetWindowLongW(m_hMainHwnd, GWL_STYLE);
    ::SetWindowLongW(m_hMainHwnd, GWL_STYLE, s | WS_MAXIMIZEBOX);
    ::SetWindowPos(m_hMainHwnd, NULL, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

// WS_MAXIMIZEBOX 共享状态: m_snapDisabled || m_maxDisabled 任一为 true 就 strip,
// 都为 false 才还原. EnableSnap / EnableMaximize / AttachToWnd 三个入口共用.
//   - m_snapDisabled=true (CXShadow 默认): strip 消除拖边 snap preview UI.
//   - m_maxDisabled=true : strip 配合吞 SC_MAXIMIZE 实现完全禁最大化.
void CXShadow::UpdateMaxBoxState(){
    bool wantStrip = m_snapDisabled.load() || m_maxDisabled.load();
    if (wantStrip && !m_maxBoxSaved)         StripMaxBox();
    else if (!wantStrip && m_maxBoxSaved)    RestoreMaxBox();
}

void CXShadow::EnableSnap(BOOL bEnable){
    // 语义 (与 CXBlur 反转, 详见头文件 EnableSnap 文档):
    //   bEnable = TRUE  → 阻止 snap (m_snapDisabled = true). 默认.
    //   bEnable = FALSE → 允许 snap (m_snapDisabled = false, 系统默认行为).
    //
    // *字面禁 snap*: m_snapDisabled=true 时 strip WS_MAXIMIZEBOX (消除拖边
    // snap preview UI), 副作用是按钮变灰. 但 *不* 吞 SC_MAXIMIZE — Win+↑
    // 和 API 最大化仍可用 (子类 proc WINDOWPOSCHANGING 用 IsZoomed 放行真
    // 最大化). 想完全禁最大化 (含 Win+↑) 配合 EnableMaximize(FALSE).
    bool newVal = (bEnable != FALSE);
    bool prev   = m_snapDisabled.exchange(newVal);
    if (prev == newVal) return;             // 无变化, 跳过
    if (m_hAttachedWnd == NULL) return;     // 还没 attach, 仅记忆 m_snapDisabled,
                                            // AttachToWnd 时 UpdateMaxBoxState
                                            // 按当时值 strip
    UpdateMaxBoxState();
    // 切换后立即重算 m_isSnapped — 从 "阻止" 切回 "允许" 时, 当前几何若恰好
    // 在 snap target 位置, 需要 m_isSnapped=true 触发 ClearPadding; 反之亦然.
    SyncWindowState();
}

BOOL CXShadow::IsSnapEnabled() const {
    return m_snapDisabled.load() ? TRUE : FALSE;
}

void CXShadow::EnableMaximize(BOOL bEnable){
    // 语义 (与 XWnd_EnableMaxWindow 一致):
    //   bEnable = TRUE  → 允许最大化 (默认, m_maxDisabled = false).
    //   bEnable = FALSE → 禁最大化 (m_maxDisabled = true): strip WS_MAXIMIZEBOX
    //                       + 子类 proc 吞 SC_MAXIMIZE. 注意: API 路径
    //                       ShowWindow(SW_MAXIMIZE) 不走 SYSCOMMAND, 本接口
    //                       拦不住 — 想完全禁请调用方层面控制.
    bool newDisabled = (bEnable == FALSE);
    bool prev        = m_maxDisabled.exchange(newDisabled);
    if (prev == newDisabled) return;        // 无变化, 跳过
    if (m_hAttachedWnd == NULL) return;     // 还没 attach, 仅记忆 m_maxDisabled,
                                            // AttachToWnd 时 UpdateMaxBoxState
                                            // 按当时值 strip
    UpdateMaxBoxState();
}

BOOL CXShadow::IsMaximizeEnabled() const {
    return m_maxDisabled.load() ? FALSE : TRUE;
}

//============================================================================
// padding 计算与应用
//
// margin = _XUITool::kShadowMargin 四边等宽 (与 tooltip / calendar 一致).
//============================================================================
// ★ DPI 单位惯例 (跟 module_xcgui_image / video / blur / editdw 对齐) ★
//
// XCGUI 的 *用户层 API* 全部接收 *逻辑像素 @ 96 DPI*:
//   - XWnd_Create(x, y, cx, cy, ...)            逻辑
//   - XWnd_SetPadding(L, T, R, B)                逻辑
//   - XWnd_GetClientRect → 返回 RECT             逻辑
//   - XEle_GetWidth / Height                     逻辑
//   - XDraw_FillRoundRectEx / DrawRoundRectEx / Image / SetLineWidthF
//     等所有 *绘制 API* 接受的坐标都是 *逻辑* (XCGUI 内部 SetTransform 物理化)
//
// *物理像素 API* (× DPI/96 后) 仅在以下场景使用:
//   - XEle_GetWndClientRectDPI    用于 D2D RT face-value 直接 DrawBitmap
//   - GetWindowRect / GetCursorPos / GetClientRect (Win32 原生, 永远物理)
//   - ID2D1RenderTarget / ID2D1Bitmap 像素操作
//   - DIB / HBITMAP 像素数据 (按物理像素分配缓存)
//
// 因此 *m_margin* / m_curPad* / m_borderWidth / m_cornerRadius 全部存 *逻辑像素*.
// 仅在 ComputeNcHitTest / WM_GETMINMAXINFO 等处临时 × m_dpiScale 转物理.
void CXShadow::ComputeShadowMargin(int* pL, int* pT, int* pR, int* pB) const {
    const int m = _XUITool::kShadowMargin;
    if (pL) *pL = m;
    if (pT) *pT = m;
    if (pR) *pR = m;
    if (pB) *pB = m;
}

void CXShadow::ApplyPadding(){
    if (!m_hAttachedWnd) return;

    // 1) 几何 margin (与 _CXShadow_ComputeGeom 同一公式) — 用来定位 inner rect
    //    + halo bitmap. 不含 border width. *逻辑像素* (与 XWnd_SetPadding 单位一致).
    int L, T, R, B;
    ComputeShadowMargin(&L, &T, &R, &B);
    m_marginL = L; m_marginT = T; m_marginR = R; m_marginB = B;

    // 2) OS padding = margin + bw, 让子元素 layout 起点 (0,0 元素坐标) 不压在
    //    1px AA 描边上 (issue #1). bw=0 时 padding == margin.
    // bw / margin / Lp 全部 *逻辑像素* — XWnd_SetPadding 接收 logical (与 XEle_GetWidth
    // 等同惯例; XCGUI EnableDPI 模式下自动 × dpiScale 化 物理 padding).
    int bw = (int)std::floor(m_borderWidth.load() + 0.5f);
    if (bw < 0) bw = 0;
    int Lp = L + bw, Tp = T + bw, Rp = R + bw, Bp = B + bw;

    if (Lp == m_curPadL && Tp == m_curPadT && Rp == m_curPadR && Bp == m_curPadB){
        return;   // 没变化, 跳过 SetPadding (避免触发不必要的 XCGUI relayout)
    }
    ::XWnd_SetPadding(m_hAttachedWnd, Lp, Tp, Rp, Bp);
    m_curPadL = Lp; m_curPadT = Tp; m_curPadR = Rp; m_curPadB = Bp;
}

void CXShadow::ClearPadding(){
    if (!m_hAttachedWnd) return;
    bool anyMargin = (m_marginL || m_marginT || m_marginR || m_marginB);
    bool anyPad    = (m_curPadL || m_curPadT || m_curPadR || m_curPadB);
    if (!anyMargin && !anyPad) return;

    if (anyPad){
        ::XWnd_SetPadding(m_hAttachedWnd, 0, 0, 0, 0);
    }
    m_marginL = m_marginT = m_marginR = m_marginB = 0;
    m_curPadL = m_curPadT = m_curPadR = m_curPadB = 0;
}

//============================================================================
// aero snap 检测
//
// 思路: aero snap 后窗口至少有一边 *精确* 贴住 monitor 的工作区边缘 (剔除任务栏).
//   - 全屏 maximized: 4 边都 ≤ 工作区 (实际用 IsZoomed 判)
//   - half-screen snap: 左/右 半屏, 上下贴边
//   - quad snap (Win11):  4 个角分别占 1/4 屏幕, 至少 2 边贴边
// 只要任一边贴边, 视觉上 "阴影超出屏幕被裁" — 此时直接 ClearPadding 隐藏阴影.
//
// 注: GetWindowRect 不含 DPI 偏移问题, 系统物理坐标. 与 GetMonitorInfo 同坐标系.
//============================================================================
bool CXShadow::IsWindowSnapped() const {
    if (!m_hMainHwnd || !::IsWindow(m_hMainHwnd)) return false;
    // IsZoomed 真最大化已在 SyncWindowState 里单独处理, 这里只看 aero snap.
    if (::IsZoomed(m_hMainHwnd) || ::IsIconic(m_hMainHwnd)) return false;

    RECT wr;
    if (!::GetWindowRect(m_hMainHwnd, &wr)) return false;

    HMONITOR hMon = ::MonitorFromWindow(m_hMainHwnd, MONITOR_DEFAULTTONEAREST);
    if (!hMon) return false;
    MONITORINFO mi = { sizeof(mi) };
    if (!::GetMonitorInfoW(hMon, &mi)) return false;
    const RECT& wa = mi.rcWork;

    // 任意一边贴边 → snap. 用 ≤/≥ 而不是 == 是因为 Win10/11 偶尔会让窗口超出
    // 1~2 像素 (DPI 取整误差 / shaped window 的 frame 微调).
    return wr.left   <= wa.left
        || wr.top    <= wa.top
        || wr.right  >= wa.right
        || wr.bottom >= wa.bottom;
}

//============================================================================
// 状态同步 (统一入口)
//
// 任何可能改变 "阴影是否应该显示" 的消息 (WM_SIZE / WM_DPICHANGED / WM_MOVE /
// WM_WINDOWPOSCHANGED) 都走这里. 中间用 m_curPadL/T/R/B 缓存避免重复 SetPadding.
//============================================================================
void CXShadow::SyncWindowState(){
    if (!m_hMainHwnd || !::IsWindow(m_hMainHwnd)) return;
    bool wasZoomed  = m_isMaximized;
    bool wasSnapped = m_isSnapped;
    m_isMaximized = (::IsZoomed (m_hMainHwnd) != FALSE);
    m_isMinimized = (::IsIconic (m_hMainHwnd) != FALSE);
    // EnableSnap(TRUE) (m_snapDisabled=true, 默认) 时 snap 不可能发生 → 跳过
    // IsWindowSnapped 的 GetWindowRect / MonitorFromWindow / GetMonitorInfo 三连
    // 调用, m_isSnapped 强制 false. 既省开销也避免误判 (用户手动 resize 到 snap
    // 几何时 IsWindowSnapped 误返 true).
    m_isSnapped   = !m_isMaximized && !m_snapDisabled.load() && IsWindowSnapped();

    // 真最大化前提: 窗口仍带 WS_MAXIMIZEBOX. EnableMaximize(FALSE) 期间 strip
    // 掉了, 此时即使 IsZoomed 报 true (旧的 SC_MAXIMIZE 模拟态), 也强制视为
    // 非最大化 — 让阴影 padding 不被错误清掉.
    if (m_isMaximized){
        LONG s = ::GetWindowLongW(m_hMainHwnd, GWL_STYLE);
        if (!(s & WS_MAXIMIZEBOX)) m_isMaximized = false;
    }

    // 隐藏阴影条件: 最大化 || snap. 二者都不动 padding 内容, 直接 ClearPadding.
    //
    // ★ 拖拽时序 (非对称) ★ — 跟 Win11 内置应用 (Edge / 设置) 视感对齐:
    //
    //   A) *进入* snap (unsnap → snap, 用户拖到屏幕边):
    //      1) 鼠标到边: DWM 显示半透 snap preview, 窗口跟鼠标走 (位置贴 work area).
    //         IsWindowSnapped() 已返 true.
    //      2) 用户 *还没松手*: snap 未真正 commit, 窗口尺寸 = 原尺寸 + padding.
    //      3) 立即 ClearPadding 会让 inner 突然撑大 (padding 收掉, 子元素 layout 蹦
    //         margin px) — 视觉错乱.
    //      → 解法: 拖拽中维持现状 (不 ClearPadding). 松手后 WM_EXITSIZEMOVE → 这里
    //         调一次 SyncWindowState 把累积状态 commit.
    //
    //   B) *离开* snap (snap → unsnap, 用户从 snap 状态拖回普通窗口):
    //      1) Win11 在拖动开始时 *立即* 把窗口尺寸还原到 pre-snap 大小 (用户期望窗
    //         口跟着光标走). 此时 IsWindowSnapped()=false.
    //      2) 如果同样延迟到松手, 拖拽中 padding=0, 视觉上 inner 充满全窗 (没有阴
    //         影边), 直到用户松手 padding 才回来 — 用户实测体验差.
    //      → 解法: 拖拽中, 仅对 "wasHide && !nowHide" 这条路径立即 ApplyPadding.
    //         "!wasHide && nowHide" (即进入 snap) 仍延迟.
    bool nowHide = m_isMaximized || m_isSnapped;
    bool wasHide = wasZoomed     || wasSnapped;
    if (m_inSizeMove){
        if (wasHide && !nowHide){
            ApplyPadding();    // snap/max → 普通: 立即恢复 padding
            if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
        }
        // 反方向 / 状态没变: 拖拽中不动, 等松手统一同步
        return;
    }

    if (nowHide){
        ClearPadding();
    } else {
        ApplyPadding();
    }

    if (wasZoomed != m_isMaximized || wasSnapped != m_isSnapped){
        if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
    }
}

//============================================================================
// v2 WM_PAINT 主入口 (XCGUI 窗口绘制事件, 不是 Win32 原生消息接管)
//
// 流程:
//   1. 取 client rect (XCGUI 在 transparent_shaped 模式下 client = window 全区).
//   2. 最大化态: DrawMaximizedPaint (全矩 inner bg, 无阴影 / 无描边).
//   3. 普通态:   DrawNormalPaint (shadow halo + inner bg + 1px 描边).
//   4. 设 *pbHandled = TRUE → XCGUI 跳过默认背景填充, 但仍会递归画子元素.
//============================================================================
int CXShadow::OnWndPaintImpl(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled){
    if (!hDraw || !hWnd) return 0;
    RECT rcClient = {};
    if (!::XWnd_GetClientRect(hWnd, &rcClient)) return 0;
    int cw = rcClient.right  - rcClient.left;
    int ch = rcClient.bottom - rcClient.top;
    if (cw <= 0 || ch <= 0) return 0;

    // 第一次 paint: 重新把子类放到 chain 顶部. XCGUI 在 ShowWindow / first paint
    // 路径上可能装了它自己的子类, 把我们压下去. 这里强制 bump.
    if (!m_firstPaintDone){
        m_firstPaintDone = true;
        ForceSubclassToTop();
    }

    // ===== 与 CXBlur (DWM acrylic) / 用户手动 SetTransparentType 共存的优雅降级 =====
    //
    // CXShadow halo 必须在 window_transparent_shaped 模式下渲染:
    //   - halo 区 alpha<255 像素的语义 = "透出桌面" (per-pixel alpha 合成);
    //   - 主窗 backbuffer 每帧由 XCGUI 清零, 阴影一次性贴入, 不累积.
    //
    // 用户在 attach 之后手动 XWnd_SetTransparentType 切走 shaped (转 false / shadow /
    // simple), 两个 bug 同时浮出:
    //   1) 若另有 CXBlur 在同窗挂了 DWM ACCENT_POLICY: alpha 通道被 DWM 复用为
    //      "透 acrylic blur 度", halo 区被 DWM 渲成 "模糊背景 + 浅色调", 而非阴影.
    //   2) Opaque 模式下 backbuffer 不带 alpha 也不被清零, 加上我们 *pbHandled=TRUE
    //      跳过默认背景填充 → 每帧 XDraw_ImageEx SourceOver 把 halo 叠到上一帧之上,
    //      阴影越叠越深.
    //
    // 二者本质都是 DWM 架构层硬限制 (ACCENT_POLICY 是 HWND 级 + alpha 通道语义在
    // shaped 与 acrylic 之间二选一), 应用层无法 "halo + acrylic" 兼得. 此处选择
    // *优雅降级*: 检测到非 shaped → ClearPadding + 走最大化态绘制 (仅填内圈 bg,
    // 无 halo, 无描边). 用户切回 shaped 后下一帧自动恢复完整阴影.
    //
    // 性能: XWnd_GetTransparentType 是 XCGUI 内部一次原子读, < 100 ns.
    // ApplyPadding / ClearPadding 都对 m_curPad* 有早返回, 稳态零开销.
    bool shapedMode =
        (::XWnd_GetTransparentType(hWnd) == window_transparent_shaped);

    if (!shapedMode){
        // 降级路径. ClearPadding 幂等 (已清时 no-op), 仅首次切入触发一次 SetPadding.
        ClearPadding();
        DrawMaximizedPaint(hDraw, cw, ch);
    } else {
        // 防御性 ApplyPadding: 上一帧若处于降级状态把 padding 清掉了, 此处恢复.
        // 仅在 *正常态* (非 max/snap) 调用 — max/snap 由 SyncWindowState 主动维持
        // padding=0, 不能在这里把 padding 强行写回去 (否则 snap 时阴影会重新出现).
        if (!m_isMaximized && !m_isSnapped){
            ApplyPadding();   // 内部对 m_curPad* 幂等
        }
        if (m_isMaximized){
            DrawMaximizedPaint(hDraw, cw, ch);
        } else {
            DrawNormalPaint(hDraw, cw, ch);
        }
    }
    // pbHandled = TRUE: 告诉 XCGUI "背景我画了, 不要再默认填色".
    // XCGUI 还会继续走后面的子元素递归绘制 (与 pbHandled 无关).
    if (pbHandled) *pbHandled = TRUE;
    return 0;
}

//============================================================================
// 普通态绘制: shadow halo + inner bg + 1px AA 圆角描边
//============================================================================
void CXShadow::DrawNormalPaint(HDRAW hDraw, int cw, int ch){
    ::XDraw_EnableSmoothingMode(hDraw, TRUE);

    const int mL = m_marginL, mT = m_marginT, mR = m_marginR, mB = m_marginB;
    const int innerW = cw - mL - mR;
    const int innerH = ch - mT - mB;
    if (innerW <= 0 || innerH <= 0) return;

    int cornerLog = m_cornerRadius.load();
    if (cornerLog < 0) cornerLog = 0;
    int maxC = (std::min)(innerW, innerH) / 2;
    if (cornerLog > maxC) cornerLog = maxC;

    RECT rcInner = { mL, mT, mL + innerW, mT + innerH };

    RECTF shadowBody{
        (float)rcInner.left,
        (float)rcInner.top,
        (float)rcInner.right,
        (float)rcInner.bottom
    };
    _XUITool::DrawDropShadow(hDraw, shadowBody, (float)cornerLog,
        _CXShadow_ToUitoolTheme(m_theme.load()));

    COLORREF bg = m_innerBgColor.load();
    if (GetRGBA_A(bg) > 0){
        ::XDraw_SetBrushColor(hDraw, bg);
        ::XDraw_FillRoundRectEx(hDraw, &rcInner,
                                cornerLog, cornerLog, cornerLog, cornerLog);
    }

    // 5) AA 圆角描边. 几何 (rcInner, cornerLog) 用 *逻辑像素* — 与 XDraw_FillRoundRectEx
    //    同步; 但 line width 实测 *不随 dpiScale 缩放* (XDraw_SetLineWidthF 在 D2D 路径
    //    上等价 ID2D1RenderTarget::DrawXxx(stroke, width) 直传 — D2D stroke 默认是
    //    *device pixel face-value*, transform scale 不作用到 line width). 所以这里手动
    //    × dpiScale 物理化, 再 round 到整像素, 让 1.0 逻辑 = 1px@100%/2px@150%/2px@200%
    //    与 Win11 系统 stroke 一致.
    //
    //    XCGUI 没有独立 XDraw_SetPenColor — XDraw_SetBrushColor 同时控制 fill / stroke 色.
    float bwLog = m_borderWidth.load();
    if (bwLog >= 0.5f){
        COLORREF bc = m_borderColor.load();
        if (GetRGBA_A(bc) > 0){
            float bwPhys = bwLog * m_dpiScale;
            bwPhys = std::floor(bwPhys + 0.5f);
            if (bwPhys < 1.0f) bwPhys = 1.0f;
            ::XDraw_SetBrushColor(hDraw, bc);
            ::XDraw_SetLineWidthF(hDraw, bwPhys);
            ::XDraw_DrawRoundRectEx(hDraw, &rcInner,
                                    cornerLog, cornerLog, cornerLog, cornerLog);
        }
    }
}

//============================================================================
// 最大化态绘制: 全矩 inner bg 填充 (没有阴影 / 描边 / 圆角).
// 此时 padding 已被 ClearPadding 抹平, 主窗 = 全屏内圈.
//============================================================================
void CXShadow::DrawMaximizedPaint(HDRAW hDraw, int cw, int ch){
    COLORREF bg = m_innerBgColor.load();
    if (GetRGBA_A(bg) == 0){
        _XUITool::ThemePalette pal{};
        _XUITool::ResolvePalette(_CXShadow_ToUitoolTheme(m_theme.load()), 0, 0, 0, &pal);
        bg = pal.bg;
    }
    RECT rc = { 0, 0, cw, ch };
    ::XDraw_FillRectColor(hDraw, &rc, bg);
}

//============================================================================
// WM_NCHITTEST 命中测试
//
// 区域划分 (普通态):
//   * Padding 圈 (shadow halo)        → HTTRANSPARENT (鼠标穿透到下一窗)
//   * 内圈 4 边 N 像素 (resize 带)   → HTLEFT/TOP/RIGHT/BOTTOM/4 角
//                                       (仅当 WS_THICKFRAME, 即 EnableDragBorder=TRUE)
//   * 内圈其余部分                    → HTNOWHERE (交回 XCGUI 处理拖动 caption / 子元素)
//
// 边宽来源: XWnd_GetDragBorderSize, 支持用户用 XWnd_SetDragBorderSize 调.
//============================================================================
LRESULT CXShadow::ComputeNcHitTest(int screenX, int screenY) const {
    if (!m_hMainHwnd || !m_hAttachedWnd) return HTNOWHERE;

    // *坐标系*: GetWindowRect / NCHITTEST screenX,Y 全是 *物理像素*.
    // m_margin* / 拖动边宽 全是 *逻辑像素* (用户配置单位).
    // 这里必须 × m_dpiScale 物理化, 否则 100% 巧合 OK / DPI 模式视觉 inner 边 ≠ NCHIT
    // inner 边 (相差 (dpiScale-1) × margin 物理像素), 阴影区最内圈条带不穿透.
    RECT wr;
    if (!::GetWindowRect(m_hMainHwnd, &wr)) return HTNOWHERE;
    int cw = wr.right  - wr.left;
    int ch = wr.bottom - wr.top;

    // 窗口本地坐标 (物理)
    int x = screenX - wr.left;
    int y = screenY - wr.top;

    // 出窗 → 让系统处理
    if (x < 0 || y < 0 || x >= cw || y >= ch) return HTNOWHERE;

    // 最大化态 / aero snap: 没有 padding / shadow, 整窗 = 内圈 → 交 XCGUI
    if (m_isMaximized || m_isSnapped) return HTNOWHERE;

    auto rp = [](double v) -> int {
        return (v >= 0) ? (int)(v + 0.5) : -(int)(-v + 0.5);
    };
    // m_margin* 逻辑 → 物理 (NCHIT 比较物理坐标)
    const int mL = rp((double)m_marginL * m_dpiScale);
    const int mT = rp((double)m_marginT * m_dpiScale);
    const int mR = rp((double)m_marginR * m_dpiScale);
    const int mB = rp((double)m_marginB * m_dpiScale);
    const int innerL = mL;
    const int innerT = mT;
    const int innerR = cw - mR;
    const int innerB = ch - mB;

    // 1) Padding 圈 (shadow halo) → 穿透
    if (x < innerL || y < innerT || x >= innerR || y >= innerB){
        return HTTRANSPARENT;
    }

    // 2) 内圈 resize 边 (需要 WS_THICKFRAME, 即用户开启了 EnableDragBorder)
    LONG style = ::GetWindowLongW(m_hMainHwnd, GWL_STYLE);
    if (!(style & WS_THICKFRAME)){
        return HTNOWHERE;
    }

    // 拖动边宽: XWnd_GetDragBorderSize 返 *逻辑像素* (同 XWnd_SetDragBorderSize 输入).
    // 与 m_margin* 同惯例, NCHIT 比较物理坐标前需 × dpiScale 转物理.
    borderSize_ bs = { 0, 0, 0, 0 };
    ::XWnd_GetDragBorderSize(m_hAttachedWnd, &bs);
    int bL_log = (bs.leftSize   > 0) ? bs.leftSize   : 0;
    int bT_log = (bs.topSize    > 0) ? bs.topSize    : 0;
    int bR_log = (bs.rightSize  > 0) ? bs.rightSize  : 0;
    int bB_log = (bs.bottomSize > 0) ? bs.bottomSize : 0;
    // 全 0 → 用户没设, 默认给 4 *逻辑像素* (与系统 WS_THICKFRAME 一致经验值)
    if (bL_log == 0 && bT_log == 0 && bR_log == 0 && bB_log == 0){
        bL_log = bT_log = bR_log = bB_log = 4;
    }
    int bL = rp((double)bL_log * m_dpiScale);
    int bT = rp((double)bT_log * m_dpiScale);
    int bR = rp((double)bR_log * m_dpiScale);
    int bB = rp((double)bB_log * m_dpiScale);
    if (bL < 1) bL = 1;
    if (bT < 1) bT = 1;
    if (bR < 1) bR = 1;
    if (bB < 1) bB = 1;

    const bool nearL = (x < innerL + bL);
    const bool nearT = (y < innerT + bT);
    const bool nearR = (x >= innerR - bR);
    const bool nearB = (y >= innerB - bB);

    if (nearL && nearT) return HTTOPLEFT;
    if (nearR && nearT) return HTTOPRIGHT;
    if (nearL && nearB) return HTBOTTOMLEFT;
    if (nearR && nearB) return HTBOTTOMRIGHT;
    if (nearL) return HTLEFT;
    if (nearT) return HTTOP;
    if (nearR) return HTRIGHT;
    if (nearB) return HTBOTTOM;

    // 内圈非边缘部分 → 交 XCGUI (它会派 HTCAPTION / 给子元素)
    return HTNOWHERE;
}
