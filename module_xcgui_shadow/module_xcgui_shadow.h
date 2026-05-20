// 文件编码: UTF-8 with BOM (与本目录其他 module_xcgui_*.h/.cpp 保持一致).
#ifndef  XCGUI_SHADOW_H
#define  XCGUI_SHADOW_H
//@模块名称  炫彩界面库-窗口阴影
//@版本  2.0.0
//@日期  2026-05-18
//@作者  未闻花名
//@QQ    936599025
//@依赖  module_xcgui_class.h
//@模块备注  Win11 风格的窗口外阴影 + 圆角 AA 描边 + 圆角内圈背景. 阴影位于
//          主窗 padding 区域 (XWnd_SetPadding 留出的圈), 不占用真正的元素布局区.
//
//          架构:
//            * 不使用外置 layered HWND. AttachToWnd 把主窗设为
//              window_transparent_shaped + alpha 255, 然后直接接管 WM_PAINT —
//              阴影 halo + 内圈圆角背景 + 1px AA 圆角描边 全部画在主窗自身.
//              与主窗共享 D2D / GDI 渲染上下文, 零帧 lag, 移窗 / resize 永远同步.
//            * 阴影 bitmap 在内部 m_pPixels (BGRA premultiplied DIB) 上软件渲:
//              key 层 + ambient 派生层 → BoxBlur 三次近似高斯 → 镂空内圈圆角 →
//              包成 HIMAGE 给 XDraw_ImageEx. 主窗每次重绘时 1 张图贴上去.
//            * 命中测试 (WM_NCHITTEST) 走 SetWindowSubclass 子类化 — 比 XCGUI
//              自身的 wndproc 钩更靠前, 保证 padding 圈返 HTTRANSPARENT 鼠标穿透.
//            * 最小尺寸 (WM_GETMINMAXINFO) 自动把当前 padding 加到 ptMinTrackSize,
//              用户配置的 "可见内圈最小宽高" 不被阴影撑大的窗框吃掉.
//            * 最大化 / aero snap 时自动 ClearPadding, 无可见阴影; 还原后 ApplyPadding 恢复.
//
//          DPI:
//            * 用户配置 (corner / shadow radius / spread / offset / borderWidth)
//              全部为 *逻辑像素 @ 96 DPI*. 内部 m_margin* / m_curPad* / 用户参数
//              全存逻辑, 仅在 DIB 渲染 / NCHITTEST / WM_GETMINMAXINFO / stroke 宽
//              4 处临时 × dpiScale 物理化.
//            * 描边宽物理化后 round 到整像素, 防 GDI+ / D2D 1.5 px stroke 渲成 2 px
//              半透的 "还是 1 px" 视觉错觉.
//            * 运行期切换缩放 (WM_DPICHANGED) 在 WM_SIZE 与每帧 paint 各兜底一次.
//
//          兼容性:
//            * 同时兼容 XInitXCGUI(TRUE) D2D 主路径 与 XInitXCGUI(FALSE) GDI+ 兜底.
//            * 阴影 DIB 渲染走 GDI+ (BoxBlur + AddRoundedRectPath), Win7 SP1+ 可用.
//            * 与 CXBlur (DWM acrylic) 不能在同窗"halo + acrylic"同时生效 — DWM
//              架构限制 (alpha 通道在 shaped 与 acrylic 路径之间二选一). 用户在
//              attach 之后通过 XWnd_SetTransparentType 切走 shaped 时, 本类自动
//              进入降级路径: ClearPadding + 仅填内圈 bg (无 halo / 无描边), 让出
//              alpha 通道给 acrylic. 切回 shaped 下一帧自动恢复完整阴影.
//
// ============================================================================
//  ★ 使用 ★ — AttachToWnd 即可, SetTransparentType / XE_PAINT 自绘圆角填充
//             等前置都由本类内部接管, 不需用户插手.
// ============================================================================
//
//      CXShadow* p = new CXShadow();
//      p->AttachToWnd(hWnd);          // 接管: padding / 透明属性 / WM_PAINT / NCHITTEST
//      p->SetTheme(xshadow_theme_auto);
//      p->SetCornerRadius(8);
//      // Detach() 或 ~CXShadow() 时自动还原主窗原状态.
//
//   注意:
//     * *不要* 再给主窗设 XWnd_SetRound / SetWindowRgn — HRGN 是 1-bit 位图, 与
//       本模块的 AA 圆角描边在 ±1 px 处错位; 内圈圆角填充也由本模块负责, 不需要
//       用户在 XE_PAINT 里再画.
//     * 兼容 XCGUI 原生 EnableDragBorder (WS_THICKFRAME) / EnableMaxWindow
//       (WS_MAXIMIZEBOX) / EnableDragCaption / SetCaptionMargin — 本模块读它们的
//       状态决定 NCHITTEST 与最大化响应.
//
// ============================================================================
//@模块信息结束

// =================================================================
// 头文件依赖拓扑 (照抄, 不要改顺序):
//   - @依赖 是 IDE 解析器用的, 不会自动注入 #include 给 cl.exe; 下面要再用
//     真实 #include 链一次.
//   - d2d1.h 必须先于 module_xcgui.h 完成 (POINTF 名字抢占).
// =================================================================

#include <d2d1.h>
#include <d2d1helper.h>
#include <gdiplus.h>

#include <atomic>
#include <cstdint>
#include <vector>

#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"

//@src "module_xcgui_shadow.cpp"

// =================================================================
// 第三方依赖: GDI / GDI+ / D2D / User32. 不需要 DWM (本模块不走 DWM acrylic).
// =================================================================

//@lib "Gdiplus.lib"
//@lib "Gdi32.lib"
//@lib "User32.lib"
//@lib "D2d1.lib"

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "D2d1.lib")

//@隐藏{
class CXShadow;
//@隐藏}

///<阴影主题预设 (CXShadow::SetTheme)
//@别名 阴影主题
enum xshadow_theme_
{
    //@别名 阴影主题_自定义
    xshadow_theme_custom    = 0,
    //@别名 阴影主题_浅色
    xshadow_theme_light     = 1,
    //@别名 阴影主题_深色
    xshadow_theme_dark      = 2,
    //@别名 阴影主题_跟随系统
    xshadow_theme_auto      = 3,
};

//@分组{ 窗口阴影
//@备注  Win11 风格的窗口外阴影 + 圆角 AA 描边 + 圆角内圈背景. 通过 AttachToWnd
//       附加到一个 XCGUI 窗口接管其 paint, 不继承 CXEle, 也不创建额外 HWND.
//@别名  炫彩窗口阴影类
class CXShadow
{
public:
    //@隐藏{
    CXShadow();
    virtual ~CXShadow();
    //@隐藏}

//@备注 把阴影附加到一个炫彩窗口. 不创建额外 HWND, 而是接管主窗的透明属性
//      + padding + WM_PAINT / NCHITTEST / SIZE / DPICHANGED 事件.
//      主窗原状态 (transparent type/alpha, padding, layout) 被保存,
//      Detach 时还原. 与 XCGUI EnableDragBorder / EnableMaxWindow / EnableDragCaption 兼容.
//
//      *与 CXBlur (DWM acrylic) 共存说明*:
//      本类强制把宿主切到 window_transparent_shaped (halo alpha 通道需要).
//      若 attach 之后调用方再 XWnd_SetTransparentType 把宿主切回 false / shadow /
//      simple (例如想让 CXBlur 的 acrylic 生效), 本类自动 *降级* 为 "无 halo, 无
//      描边, 仅填内圈 bg" — 因为 DWM 架构限制 alpha 通道在 shaped 与 acrylic 路径
//      间只能二选一. 切回 shaped 下一帧自动恢复完整阴影. 用户通过切换 transparent
//      type 自由选择 "此刻要 acrylic blur 还是要 shadow halo".
//@参数 hWnd 目标窗口.
//@返回 TRUE 成功, FALSE 句柄非法或事件注册失败.
//@别名  附加窗口()
    BOOL AttachToWnd(HWINDOW hWnd);

//@备注 构造时直接附加.
    CXShadow(HWINDOW hWnd){ AttachToWnd(hWnd); }

//@备注 解除附加. 还原主窗 transparent / padding / layout 状态,
//      反注册事件钩子. 不影响主窗本身的子元素.
//@别名  解除绑定()
    void Detach();

//@备注 当前是否已附加到窗口.
//@别名  是否已附加()
    BOOL IsAttached() const;

//@备注 取被附加的窗口句柄.
//@别名  取附加窗口()
    HWINDOW GetAttachedWnd() const;

    // ===== 圆角 =====
//@备注 设置圆角半径. 单位 = 逻辑像素 @ 96 DPI, 内部按 DPI/96 缩放. Win11 默认 8.
//      传 0 = 直角 (无圆角, 仅外阴影). 大于内圈短边一半时自动 clamp.
//@参数 radius 圆角半径 (逻辑像素).
//@别名  置圆角()
    void SetCornerRadius(int radius);

//@备注 取圆角半径 (逻辑像素).
//@别名  取圆角()
    int GetCornerRadius() const;

    // ===== 阴影 =====
//@备注 设置阴影模糊半径. Gaussian blur 标准差近似. 单位 = 逻辑像素.
//      值越大阴影越柔越远. Win11 默认 16. 传 0 = 不模糊 (硬边阴影).
//@参数 radius 模糊半径 (逻辑像素).
//@别名  置阴影模糊半径()
    void SetShadowRadius(int radius);

//@备注 取阴影模糊半径 (逻辑像素).
//@别名  取阴影模糊半径()
    int GetShadowRadius() const;

//@备注 设置阴影扩散. 类似 CSS box-shadow 的第三个长度参数. 阴影源矩形向外膨胀
//      此值后再应用 blur. 默认 0. 单位 = 逻辑像素.
//@参数 spread 扩散 (逻辑像素).
//@别名  置阴影扩散()
    void SetShadowSpread(int spread);

//@备注 取阴影扩散 (逻辑像素).
//@别名  取阴影扩散()
    int GetShadowSpread() const;

//@备注 设置阴影偏移. (dx, dy) 单位 = 逻辑像素. Win11 默认 (0, 4) (轻微下沉).
//      正 dx 阴影向右; 正 dy 阴影向下.
//@参数 dx X 偏移 (逻辑像素).
//@参数 dy Y 偏移 (逻辑像素).
//@别名  置阴影偏移()
    void SetShadowOffset(int dx, int dy);

//@备注 取阴影偏移 (逻辑像素). pdx / pdy 可为 NULL.
//@参数 pdx 接收 X 偏移.
//@参数 pdy 接收 Y 偏移.
//@别名  取阴影偏移()
    void GetShadowOffset(int* pdx, int* pdy) const;

//@备注 设置阴影颜色. RGBA, alpha 高字节, XCGUI 标准编码 0xAABBGGRR.
//      默认 0x40000000 (黑 25% alpha).
//@参数 color 阴影色.
//@别名  置阴影色()
    void SetShadowColor(COLORREF color);

//@备注 取阴影色.
//@别名  取阴影色()
    COLORREF GetShadowColor() const;

//@备注 设置主窗口失活时的阴影色. 主窗口失去焦点 (WM_ACTIVATE = WA_INACTIVE)
//      自动切换到此色, 重新激活恢复. 默认 0x20000000 (黑 12% alpha).
//      若不希望区分激活态, 设为与 SetShadowColor 相同值.
//@参数 color 阴影色 (失活态).
//@别名  置失活阴影色()
    void SetInactiveShadowColor(COLORREF color);

//@备注 取失活阴影色.
//@别名  取失活阴影色()
    COLORREF GetInactiveShadowColor() const;

    // ===== 圆角描边 (Win11 风格 stroke) =====
//@备注 设置圆角描边色. RGBA, 默认 0x33000000 (黑 20% alpha, Win11 风格).
//      此描边画在阴影 bitmap 内圈圆角上, AA, 用于强化主窗口边缘的 Win11 视感.
//      圆角半径 = 0 时仅画矩形描边.
//@参数 color 描边色.
//@别名  置描边色()
    void SetBorderColor(COLORREF color);

//@备注 取描边色.
//@别名  取描边色()
    COLORREF GetBorderColor() const;

//@备注 设置描边宽度. 单位 = 逻辑像素 (浮点, 支持 0.5 这种亚像素描边).
//      默认 1.0. 传 0 关闭描边.
//@参数 w 描边宽度 (逻辑像素).
//@别名  置描边宽()
    void SetBorderWidth(float w);

//@备注 取描边宽度 (逻辑像素).
//@别名  取描边宽()
    float GetBorderWidth() const;

    // ===== 圆角内扣修正 =====
//@备注 设置 *圆角内扣* 像素数 (覆盖主窗硬直角的修正量). 单位 = 物理像素.
//      默认 1. 增大可在大圆角 + 大模糊场景下消除主窗口边缘锯齿露出, 但
//      会有 1~2px 边缘色与主窗口内容混色 (取决于阴影色 alpha). 0 = 不修正.
//@参数 px 内扣像素 (物理像素, 不随 DPI 缩放, 与位图本身锯齿对应).
//@别名  置内扣修正()
    void SetInsetCorrection(int px);

//@备注 取内扣修正像素数.
//@别名  取内扣修正()
    int GetInsetCorrection() const;

    // ===== 主题 =====
//@备注 应用主题预设. light = 浅色背景下的暖灰阴影; dark = 深色背景下的强阴影;
//      auto = 根据系统亮/暗模式自动选. 主题会同时调整 shadow color / border color /
//      inactive shadow color, 不改变 radius / spread / offset / corner.
//@参数 theme 见 xshadow_theme_*.
//@别名  置主题()
    void SetTheme(int theme);

//@备注 取当前主题.
//@别名  取主题()
    int GetTheme() const;

//@备注 全局主题: 设置后会同步到当前所有 CXShadow 实例, 之后新创建
//      的实例默认也使用此主题. 适用于 "整个应用统一阴影风格" 场景.
//      个别窗口仍可通过 SetTheme(...) 单独 override.
//@别名  置全局主题()
    static void SetGlobalTheme(int theme);
//@别名  取全局主题()
    static int  GetGlobalTheme();

    // ===== 内圈背景填充 =====
//@备注 设置内圈背景色. 本类接管了主窗 WM_PAINT, 默认按主题 (light/dark/auto) 填一个
//      不透明背景, 调本函数可覆盖. 颜色 = 0xAABBGGRR XCGUI 标准;
//      alpha = 0 等价 ClearInnerBgColor (仅画阴影+描边, 内圈透明).
//      推荐: light 主题 RGBA(252,252,252,255), dark 主题 RGBA(32,32,32,255).
//      最大化状态下全矩面填该色 (此时 padding 已去, 主窗 = 全屏内圈).
//@参数 color 0xAABBGGRR 背景色.
//@别名  置内圈背景色()
    void SetInnerBgColor(COLORREF color);

//@备注 取消用户自定义背景, 退回主题默认. 调 SetTheme 后会同步重置为主题默认色.
//@别名  清除内圈背景色()
    void ClearInnerBgColor();

//@备注 取当前生效的内圈背景色 (用户自定义 或 主题默认).
//@别名  取内圈背景色()
    COLORREF GetInnerBgColor() const;

    // ===== 控制 =====
//@备注 当前主窗口是否处于最大化 (SIZE_MAXIMIZED) 状态. 此状态下阴影自动隐藏.
//@别名  是否最大化()
    BOOL IsMaximized() const;

//@备注 立即重绘阴影 bitmap (尺寸 / 位置不变). 改了任何视觉参数后内部已自动调用,
//      用户一般无需手动调.
//@别名  立即刷新()
    void Invalidate();

    // ===== Snap / 最大化 控制 =====
//@备注 启用 / 禁用本窗的 *Aero Snap 阻止* 功能. **默认 TRUE — snap 被阻止**.
//      *(注意: 与 CXBlur::EnableSnap 语义相反, 本类默认就阻止 snap. 因为 snap
//       状态会让 CXShadow 必须 ClearPadding 收起阴影, 视觉打断, 多数用户不希
//       望出现这种状态.)*
//
//      参数语义:
//        * bEnable = TRUE  → **阻止 snap** (默认). 字面禁 snap.
//        * bEnable = FALSE → 允许 snap (系统行为).
//
//      *bEnable=TRUE 是 "字面禁 snap"*:
//        - strip WS_MAXIMIZEBOX → 消除拖窗到屏幕边时浮出的 snap preview UI
//                                   (半透蒙层 / Snap Layouts 飞出框).
//                                   *副作用*: 标题栏最大化按钮变灰 *不可点*.
//        - WM_WINDOWPOSCHANGING 几何过滤 → 子类 proc 检测目标矩形是否匹配
//                                          snap layout (full / half / quarter),
//                                          是则设 SWP_NOMOVE | SWP_NOSIZE 阻止
//                                          落位. 兜底, 即便 preview 漏出也拦.
//        - *不* 吞 SC_MAXIMIZE — 用户仍可通过键盘 Win+↑ / 程序化 ShowWindow
//                                  (SW_MAXIMIZE) / SetWindowPlacement /
//                                  WS_MAXIMIZE 创建属性 来最大化.
//
//        几何过滤里用 IsZoomed(hwnd) 区分 "真最大化" 和 "snap 全屏": Win32
//        在派发 WINDOWPOSCHANGING 前已更新 WINDOWPLACEMENT.showCmd, 真最大
//        化时 IsZoomed=true → 跳过过滤. 这覆盖所有真最大化路径.
//
//      *与 EnableMaximize 的关系*:
//        - 本接口 (EnableSnap(TRUE)) 让按钮变灰但保留 Win+↑ / API 通路.
//        - EnableMaximize(FALSE) 在此基础上额外吞 SC_MAXIMIZE → 拦键盘 Win+↑
//          + 双击标题栏 + 系统菜单 "最大化". (API 路径 ShowWindow 仍能用.)
//        - 二者共享 WS_MAXIMIZEBOX strip 状态: 只要任一为禁用就 strip, 二者
//          都启用才还原.
//
//      *bEnable=TRUE 的额外性能收益*: 既然 snap 不可能发生, SyncWindowState
//      会跳过 IsWindowSnapped() 计算 (m_isSnapped 强制为 false), 省去每次
//      WM_SIZE / WM_WINDOWPOSCHANGED 的 GetWindowRect / MonitorFromWindow /
//      GetMonitorInfo 调用.
//
//      *副作用 / 限制*:
//        * snap 几何检测有 2 px 容差, 用户手动恰好 resize 到 1/2 屏 / 1/4
//          屏尺寸会被误拦. 概率极低 (要求 4 边都对齐 work area).
//        * 触摸板三指手势 / 屏幕投递的 snap 不走以上路径, 拦不住.
//
//      *bEnable=FALSE (从 TRUE 切回)*: 还原 WS_MAXIMIZEBOX (除非 EnableMaximize
//      (FALSE) 仍然要求 strip), 几何过滤 / IsWindowSnapped 计算恢复.
//
//      *attach 时序*:
//        * AttachToWnd 之前调本接口 → 仅记忆设置, AttachToWnd 时按值 strip.
//        * AttachToWnd 之后调本接口 → 立即生效.
//
//      返回 void (设置不会失败).
//@参数 bEnable TRUE 阻止 snap (默认, 按钮灰, 保留 Win+↑/API), FALSE 允许 snap.
//@别名  启用Snap阻止()
    void EnableSnap(BOOL bEnable);

//@备注 取当前 snap 阻止启用状态. 默认 TRUE (阻止). 返 EnableSnap 最近一次
//      参数; 与 EnableSnap 参数语义保持一致 (TRUE = 阻止 snap).
//@别名  是否阻止Snap()
    BOOL IsSnapEnabled() const;

//@备注 启用 / 禁用本窗的最大化能力. **默认 TRUE — 允许最大化** (与
//      XWnd_EnableMaxWindow 接口语义一致, 跟随窗口 WS_MAXIMIZEBOX 原始状态).
//
//      参数语义:
//        * bEnable = TRUE  → 允许最大化 (默认).
//        * bEnable = FALSE → 禁最大化: strip WS_MAXIMIZEBOX (按钮变灰, Snap
//                              Layouts 飞出框消失) + 子类 proc 吞 SC_MAXIMIZE
//                              (拦键盘 Win+Up + 双击标题栏 + 系统菜单 "最大化"
//                              + 程序化 ShowWindow(SW_MAXIMIZE)).
//
//      *与 EnableSnap 完全独立*. 见 EnableSnap 文档 "解耦" 章节.
//
//      *Detach 行为*: Detach 时本类自动还原 attach 前的 WS_MAXIMIZEBOX 状态.
//
//      *attach 时序*:
//        * AttachToWnd 之前调本接口 → 仅记忆设置, AttachToWnd 时按值 strip.
//        * AttachToWnd 之后调本接口 → 立即生效.
//
//      返回 void (设置不会失败).
//@参数 bEnable TRUE 允许最大化 (默认), FALSE 禁用最大化.
//@别名  启用最大化()
    void EnableMaximize(BOOL bEnable);

//@备注 取当前最大化启用状态. 默认 TRUE (允许). 返 EnableMaximize 最近一次
//      参数 (TRUE = 允许最大化).
//@别名  是否允许最大化()
    BOOL IsMaximizeEnabled() const;

    //@隐藏{
private:
    // ===== 绑定状态 =====
    HWINDOW m_hAttachedWnd  = NULL;       // 主窗口 (XCGUI 句柄)
    HWND    m_hMainHwnd     = NULL;       // 主窗口的 Win32 HWND
    int     m_dpi           = 96;
    float   m_dpiScale      = 1.0f;

    // ===== 状态机 =====
    bool    m_isMaximized   = false;
    bool    m_isMinimized   = false;
    bool    m_isSnapped     = false;      // aero snap (任意一边贴 monitor work area) → 阴影隐藏
    bool    m_isActive      = true;
    bool    m_dirty         = true;       // bitmap 需要重绘
    bool    m_eventsHooked  = false;
    bool    m_subclassInstalled = false;  // Win32 SetWindowSubclass 状态
    bool    m_firstPaintDone    = false;  // 第一次 WM_PAINT 完成 → 触发 ForceSubclassToTop 第二轮 bump
    bool    m_inSizeMove    = false;      // 主窗 WM_ENTERSIZEMOVE..WM_EXITSIZEMOVE 区间.
                                          // 区间内: 阴影用 1-pass box blur 快速通道,
                                          // 退出后再渲一帧高质量 (3-pass ≈ 高斯).

    // ===== Snap / 最大化 控制 =====
    //
    // m_snapDisabled (EnableSnap / IsSnapEnabled):
    //   true  (默认) → *字面禁 snap*. UpdateMaxBoxState 触发 strip WS_MAXIMIZEBOX
    //                  (消除拖边 snap preview UI), 子类 proc 几何过滤兜底,
    //                  SyncWindowState 跳过 IsWindowSnapped 计算 (m_isSnapped
    //                  强制 false). *不* 吞 SC_MAXIMIZE — Win+↑ / API 仍可
    //                  最大化.
    //   false           → snap 系统默认行为.
    //
    //   *与 CXBlur 语义反转*: CXBlur 的 m_snapEnabled=true 表示 snap *允许*,
    //   本类的 m_snapDisabled=true 表示 snap *被阻止*. 默认值都是 true 但
    //   含义相反 — 本类默认阻止 snap, CXBlur 默认允许. 详见 EnableSnap 文档.
    //
    // m_maxDisabled (EnableMaximize / IsMaximizeEnabled):
    //   false (默认) → 允许最大化 (与 XWnd_EnableMaxWindow 一致).
    //   true             → 禁最大化: UpdateMaxBoxState 触发 strip WS_MAXIMIZEBOX
    //                       (与 m_snapDisabled 共享 strip 状态) + 子类 proc 吞
    //                       SC_MAXIMIZE.
    //
    // *WS_MAXIMIZEBOX strip 共享状态机*: m_snapDisabled || m_maxDisabled 任一
    // 为 true 都 strip, 二者都 false 才还原. 由 UpdateMaxBoxState 集中维护,
    // EnableSnap / EnableMaximize / AttachToWnd 三入口都调它.
    //
    // *区分 "用户最大化" vs "snap 全屏" (二者 WINDOWPOSCHANGING 几何相同 = 全
    // 工作区)*: 子类 proc 直接调 IsZoomed(hwnd). Win32 在派发 WINDOWPOSCHANGING
    // 之前已更新 WINDOWPLACEMENT.showCmd, IsZoomed=true 覆盖所有真最大化路径
    // (键盘 Win+Up / 鼠标按钮 / API ShowWindow / SetWindowPlacement /
    // WS_MAXIMIZE 启动 / 拖到顶 snap-to-max). 不需要额外的暂态 flag.
    //
    // atomic 因为 EnableSnap / EnableMaximize 可能被 UI 线程外的线程调 (与
    // SetTheme 一致策略), 子类 proc 在 Win32 消息派发 (UI) 线程读, 共享访问
    // 需 atomic 防 tearing.
    std::atomic<bool> m_snapDisabled        {true};
    std::atomic<bool> m_maxDisabled         {false};
    bool              m_maxBoxSaved         = false;   // 是否已 strip WS_MAXIMIZEBOX
    bool              m_maxBoxOriginallySet = false;   // strip 前 WS_MAXIMIZEBOX 是否本来置位

    // ===== 主窗原状态备份 (Detach 还原用) =====
    bool    m_saved             = false;      // 是否已抓取过原状态
    int     m_savedTransType    = 0;          // window_transparent_* 原值
    BYTE    m_savedTransAlpha   = 255;        // 原 alpha
    int     m_savedPadL         = 0;          // 原 padding (Attach 前)
    int     m_savedPadT         = 0;
    int     m_savedPadR         = 0;
    int     m_savedPadB         = 0;
    BOOL    m_savedLayout       = TRUE;       // 原 EnableLayout

    // ===== 当前几何 / padding =====
    //
    // 区分两组值, 不能混用 (历史 bug 来源):
    //   m_marginL/T/R/B  : *阴影几何 margin* (= ambBlur + spread + |dx|/|dy|),
    //                       与 _CXShadow_ComputeGeom 的 marginL/T/R/B 严格相等.
    //                       用于: 阴影 bitmap 渲染坐标 (innerL = m_marginL),
    //                              内圈 bg / 描边 的 RECT,
    //                              WM_NCHITTEST shadow halo 边界.
    //   m_curPadL/T/R/B  : *OS padding* (= margin + borderWidth_phys),
    //                       通过 XWnd_SetPadding 写到 XCGUI. 比 margin 多 borderWidth
    //                       是为让子元素 layout 起点不压在 1px 描边上.
    //
    // ApplyPadding 会同步写两组值; ClearPadding 把两组都清零.
    int     m_marginL = 0, m_marginT = 0, m_marginR = 0, m_marginB = 0;
    int     m_curPadL = 0, m_curPadT = 0, m_curPadR = 0, m_curPadB = 0;


    // ===== 视觉参数 (逻辑像素 @ 96 DPI) =====
    //
    // Win11 Fluent 真实做法是 *双层阴影* — key (focused, 稍重, 偏下) + ambient (柔晕, 全方向):
    //   - key:     dy=6, blur=24, alpha~10% (黑) → 主"在桌上压下来"的感觉
    //   - ambient: dy=0, blur≈key.blur * 1.6, alpha ≈ key.alpha * 0.55 → "景深"软晕
    //   - stroke:  alpha ~12% 黑, 1 px (DPI scale 取整) — Win11 标志性的 "勾边"
    //   - inset=0 (描边是 outset, 不需要内抠)
    //
    // 单层 (老版) 的问题: 不论 alpha 多低, 都是均匀环, 看着 "死". 双层非均匀的
    //                    亮度梯度才有 "光从上方打下来" 的体感.
    //
    // 双层内部由 RenderBitmapFor 自动派生 ambient 参数, 不暴露到 setter (用户只
    // 调 key 参数即可, 视觉风格保持一致).
    std::atomic<int>      m_cornerRadius  {8};
    std::atomic<int>      m_shadowRadius  {24};        // = key.blur (logical px)
    std::atomic<int>      m_shadowSpread  {0};
    std::atomic<int>      m_shadowDx      {0};
    std::atomic<int>      m_shadowDy      {6};         // = key.dy
    std::atomic<COLORREF> m_shadowColor   {0x1A000000u};   // 10% 黑 (light active key)
    std::atomic<COLORREF> m_inactiveShadow{0x0E000000u};   //  5.5% 黑 (light inactive)
    std::atomic<COLORREF> m_borderColor   {0x0F000000u};   //  6% 黑 (light stroke)
    std::atomic<float>    m_borderWidth   {1.0f};
    std::atomic<int>      m_inset         {0};
    std::atomic<int>      m_theme         {xshadow_theme_auto};   // 默认跟随系统

    // 内圈背景. m_innerBgUserSet=true → 用 m_innerBgColor;
    // false → ApplyThemePreset 根据当前 theme 算一个默认值写进 m_innerBgColor.
    std::atomic<COLORREF> m_innerBgColor  {0xFFFCFCFCu};   // light 主题默认 (FCFCFC 不透明)
    std::atomic<bool>     m_innerBgUserSet{false};

    // ===== bitmap (DIB 32-bit BGRA premultiplied) =====
    HDC      m_hMemDC       = NULL;       // 离屏 memDC
    HBITMAP  m_hDib         = NULL;       // DIB section
    HBITMAP  m_hOldBmp      = NULL;       // memDC 旧 bitmap (SelectObject 还原用)
    void*    m_pPixels      = NULL;       // DIB 像素指针
    int      m_dibW         = 0;
    int      m_dibH         = 0;

    // ===== XCGUI 图像包装 =====
    // 把 m_pPixels (PARGB DIB) 包成 HIMAGE, 给 XDraw_Image 用. 维度变了就重建.
    HIMAGE   m_hShadowImage = NULL;
    int      m_imgW         = 0;
    int      m_imgH         = 0;

    // 复用 box blur 临时缓冲 (避免每帧 ~2MB heap 分配, 拖拽时累计上千次).
    // 容量按需 grow, never shrink. FreeDib 时一并释放.
    std::vector<uint8_t> m_blurTmp;

    // 双层阴影 ambient 层暂存 (与 m_pPixels 同 stride×height, PARGB).
    // 流程: 渲染 ambient src → blur big → 加性叠到 m_pPixels (premultiplied saturating add) → 一并 punch.
    // 持久化以避免每帧 alloc; FreeDib 时随 m_blurTmp 一起释放.
    std::vector<uint8_t> m_ambientLayer;

    // GDI+ token (本模块自己管理, 不依赖外部 GdiplusStartup)
    ULONG_PTR m_gdipToken   = 0;

public:
    // =================================================================
    // 内部回调入口 (公开仅为方便 C 风格回调调用; 用户代码请勿直接调用)
    // =================================================================
    int OnWndPaintImpl(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled);
    int OnWndProcImpl(HWINDOW hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* pbHandled);

    // 公开是因为 Win32 SetWindowSubclass 的子类 proc 在匿名 namespace 里, 不属于 CXShadow.
    // 用户代码请勿直接调用 — 仅用于 click-through 命中测试.
    LRESULT ComputeNcHitTest(int screenX, int screenY) const;

    // 当前 padding (逻辑像素, 与 XWnd_SetPadding 单位一致). 供子类 proc 的
    // WM_GETMINMAXINFO 用, 将 ptMinTrackSize 加上 padding × dpiScale (物理),
    // 让用户配置的 minSize 不被阴影吃掉.
    // snap / max 状态 padding=0, 返 0 — 不影响系统对那两种状态的 sizing.
    int   GetCurPadX()  const { return m_curPadL + m_curPadR; }
    int   GetCurPadY()  const { return m_curPadT + m_curPadB; }
    float GetDpiScale() const { return m_dpiScale; }

private:
    // ===== 生命周期 =====
    void HookEvents();
    void UnhookEvents();
    // 强制把 Win32 子类放到 chain 顶部 (Remove + Add). XCGUI 后续装它自己的子类
    // 时, 会把我们压到下层, NCHITTEST → HTTRANSPARENT 失效. 在 HookEvents +
    // 第一次 WM_PAINT 各调一次防御性 bump.
    void ForceSubclassToTop();
    void EnsureGdiPlus();
    void ShutdownGdiPlus();

    // 主窗属性接管/还原 (不动 EnableDrawBk — 靠 OnWndPaint 设 pbHandled=TRUE 跳默认背景填充)
    void CaptureMainStyles();    // Attach 第一次调: 抓 transparent / padding / layout 原值
    void RestoreMainStyles();    // Detach: 还原全部

    // 最大化禁用: WS_MAXIMIZEBOX strip / restore. 调用方在主线程, *不持任何锁*,
    // 所以可以直接 SetWindowPos(SWP_FRAMECHANGED) — 与 CXBlur 的死锁回避不同,
    // 这里没有共享状态需要锁.
    void StripMaxBox();          // 记录原态 + 关 WS_MAXIMIZEBOX (n-op 若已 strip)
    void RestoreMaxBox();        // 还原 WS_MAXIMIZEBOX 到 strip 前 (n-op 若没 strip)
    // WS_MAXIMIZEBOX 联合状态机: snap 阻止 / 最大化禁用 任一为 true 都 strip.
    // EnableSnap / EnableMaximize / AttachToWnd 三个入口都调本函数, 不直接
    // 用 StripMaxBox/RestoreMaxBox.
    void UpdateMaxBoxState();

    // ===== DIB 管理 =====
    BOOL EnsureDib(int w, int h);
    void FreeDib();

    // ===== 同步 / 渲染 =====
    void RefreshDpi();                              // 读 XWnd_GetDPI(主窗), 更新 m_dpi / m_dpiScale
    void RenderBitmapFor(int mainW, int mainH);     // 渲染阴影到 m_pPixels DIB
    BOOL EnsureShadowImage();                       // (重建) HIMAGE 包装当前 DIB
    void ReleaseShadowImage();

    // ===== padding 控制 =====
    void ComputeShadowMargin(int* pL, int* pT, int* pR, int* pB) const;  // 按 DPI 算 4 边 margin
    void ApplyPadding();         // = ComputeMargin + XWnd_SetPadding + EnableLayout(TRUE) + 缓存
    void ClearPadding();         // SetPadding(0,0,0,0) (最大化态用)

    // aero snap 检测: 任意一边贴 monitor work area 即视为 snap. 阴影应隐藏 (SyncWindowState)
    // (issue #4: 窗口拖到屏幕边缘触发系统缩放后, 阴影占太多空间)
    bool IsWindowSnapped() const;
    // 重新计算 m_isMaximized/m_isSnapped/m_isMinimized 并 ClearPadding 或 ApplyPadding.
    void SyncWindowState();

    // ===== Paint 实际绘制 =====
    void DrawNormalPaint(HDRAW hDraw, int clientW, int clientH);     // 普通态: shadow halo + 内圈 bg + 描边
    void DrawMaximizedPaint(HDRAW hDraw, int clientW, int clientH);  // 最大化态: 全矩内圈 bg

    // ComputeNcHitTest 已上移到 public 区 (subclass proc 在 namespace 里调用).

    void ApplyThemePreset(int theme);   // 同时计算 inner bg 默认色 (若用户未自定义)
    static BOOL IsSystemDarkMode();

    // ===== 渲染分流 =====
    BOOL RenderD2D();         // D2D 路径; 失败 (device-lost / Win7 SP1 无平台更新) 返 FALSE 让上层回退 GDI+
    void RenderGdiPlus();     // GDI+ 兜底, 永远成功

    //@隐藏}
};
//@分组}

#endif // XCGUI_SHADOW_H
