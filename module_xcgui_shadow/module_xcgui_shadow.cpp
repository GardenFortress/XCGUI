// 文件编码: UTF-8 with BOM (与本目录其他 module_xcgui_*.h/.cpp 保持一致).
//============================================================================
// module_xcgui_shadow.cpp
//
// CXShadow v2.0 - Win11 风格的窗口阴影 + AA 圆角描边 + 圆角内圈背景.
//
// 设计要点:
//
//   * 阴影直接绘制在主窗 WM_PAINT 中, 与主窗共享同一 D2D/GDI 渲染上下文,
//     无外置 layered HWND, 移窗 / resize 永远同步.
//
//   * AttachToWnd 接管以下主窗属性 (Detach 还原):
//       - XWnd_SetTransparentType(window_transparent_shaped) + Alpha(255)
//         (让主窗拥有 alpha 通道, 与阴影 halo 自然贴合)
//       - XWnd_SetPadding(margin)  ← 留出阴影圈, margin = ambBlur + spread + |offset|
//       - XWnd_EnableLayout(TRUE)  ← 元素 layout 自动避开 padding 圈
//       EnableDrawBk 不动 — OnWndPaint 里设 *pbHandled = TRUE 就足以跳过默认背景填充,
//       关闭它反而会让 XCGUI 跳过整个画布派发, 窗变全透.
//
//   * 与 CXBlur (DWM acrylic) 共存 — *优雅降级* (见 OnWndPaintImpl 详注):
//       CXShadow halo 与 DWM acrylic 都使用同一 alpha 通道 (前者: per-pixel
//       透桌面; 后者: 透 acrylic blur), 二者在同窗 alpha<255 像素上语义冲突,
//       DWM 架构层无法兼得. CXShadow 每帧检测宿主透明类型: 非 shaped 时
//       ClearPadding + 仅填内圈 bg (无 halo / 无描边), 让出舞台给 acrylic;
//       切回 shaped 时下一帧自动恢复完整阴影. *用户* 通过 XWnd_SetTransparentType
//       自由切换决定此刻要 blur 还是要 shadow.
//
//   * 钩 XWM_WINDPROC + WM_PAINT:
//       - WM_PAINT: padding 外圈画 shadow halo (DIB → HIMAGE → XDraw_ImageEx);
//                   padding 内圈画 inner bg (圆角填充) + 1px 圆角 AA 描边.
//                   最大化态: 全矩 inner bg 填充 (无阴影 / 无描边).
//       - WM_SIZE: SIZE_MAXIMIZED → ClearPadding (仅当 WS_MAXIMIZEBOX);
//                  SIZE_RESTORED → ApplyPadding.
//       - WM_NCHITTEST: shadow halo 区返 HTTRANSPARENT (鼠标穿透);
//                       内圈 4 边返 HTLEFT/TOP/RIGHT/BOTTOM/角 (走系统 resize, 仅当
//                       WS_THICKFRAME); 内圈内部返 HTNOWHERE (走 XCGUI 标准逻辑).
//                       边宽来自 XWnd_GetDragBorderSize.
//       - WM_GETMINMAXINFO: ptMinTrackSize 加上当前 padding 物理像素, 保证用户配置
//                           的最小内圈尺寸不被阴影窗框吃掉.
//       - WM_DPICHANGED + WM_SIZE 兜底: 重算 padding + dirty 重渲染.
//       - WM_ACTIVATE: 切换 active / inactive shadow color.
//       - WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE: 拖拽中 1-pass blur, 退出强渲 3-pass.
//
//   * 兼容 XCGUI 原生 borderless 接口:
//       - EnableDragBorder(BOOL)  ← 设 WS_THICKFRAME, 读它决定 NCHitTest resize 边
//       - EnableMaxWindow(BOOL)   ← 设 WS_MAXIMIZEBOX, 读它决定是否响应最大化
//       - EnableDragCaption / SetCaptionMargin → XCGUI 自处理.
//
//   * 单 GDI+ 渲染路径同时服务 XInitXCGUI(TRUE/FALSE), 兼容 Win7 SP1+ (GDI+ 1.1).
//============================================================================

#include "module_xcgui_shadow.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
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

//----------------------------------------------------------------------------
// GDI+ 进程级 lazy init. C++17 magic static 多线程安全. 不 Shutdown - 进程退出
// 时 OS 回收, 避免在 XCGUI 仍在用 GDI+ 时被我们提前关掉 (与 editdw 同策略).
//----------------------------------------------------------------------------
static ULONG_PTR _CXShadow_EnsureGdiPlus(){
    static ULONG_PTR token = []() -> ULONG_PTR {
        ULONG_PTR t = 0;
        Gdiplus::GdiplusStartupInput in;
        (void)Gdiplus::GdiplusStartup(&t, &in, NULL);
        return t;
    }();
    return token;
}

//----------------------------------------------------------------------------
// 主窗口 HWINDOW → CXShadow* 派发表.
// XCGUI 的 XWnd_RegEventC1 是 C 风格 *无 user-data* 回调, 静态 thunk 必须靠这张
// 表把 HWINDOW 反查到具体 CXShadow 实例. 1 主窗 1 CXShadow (1:1).
//----------------------------------------------------------------------------
static std::mutex                                  g_hostMutex;
static std::unordered_map<HWINDOW, CXShadow*>      g_hostMap;

// 全局活实例集合 (CXShadow::SetGlobalTheme 广播 / WM_SETTINGCHANGE 跟随).
// Attach 添加, Detach / WM_DESTROY 移除. 与 g_hostMap 不同的是 g_hostMap 按 HWINDOW
// 索引 (用于回调反查), 这里按指针索引 (用于遍历).
static std::mutex                                  g_shadowInstancesMutex;
static std::set<CXShadow*>                         g_shadowInstances;
static std::atomic<int>                            g_globalTheme{xshadow_theme_custom};

static CXShadow* _CXShadow_FindByHWindow(HWINDOW hWnd){
    std::lock_guard<std::mutex> lk(g_hostMutex);
    auto it = g_hostMap.find(hWnd);
    return (it != g_hostMap.end()) ? it->second : nullptr;
}

//----------------------------------------------------------------------------
// 圆角矩形 path 构造 (GDI+).
//   x, y, w, h : 矩形左上 + 宽高
//   r          : 圆角半径 (0 = 直角)
//----------------------------------------------------------------------------
static void _CXShadow_AddRoundedRectPath(
    Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float r)
{
    path.Reset();
    if (r <= 0.0f){
        path.AddRectangle(Gdiplus::RectF(x, y, w, h));
        return;
    }
    float maxR = (std::min)(w, h) * 0.5f;
    if (r > maxR) r = maxR;
    float d = r * 2.0f;

    // 顺时针: 左上 -> 右上 -> 右下 -> 左下
    path.AddArc(x,         y,         d, d, 180.0f, 90.0f);   // 左上角
    path.AddArc(x + w - d, y,         d, d, 270.0f, 90.0f);   // 右上角
    path.AddArc(x + w - d, y + h - d, d, d,   0.0f, 90.0f);   // 右下角
    path.AddArc(x,         y + h - d, d, d,  90.0f, 90.0f);   // 左下角
    path.CloseFigure();
}

//----------------------------------------------------------------------------
// 单 pass 水平 box blur (32-bit BGRA premultiplied, 滑窗求和, O(n)).
//   src/dst 不可重叠. radius >= 0.
//----------------------------------------------------------------------------
static void _CXShadow_BoxBlurH(uint8_t* dst, const uint8_t* src,
                                int w, int h, int stride, int radius)
{
    if (radius <= 0){
        if (dst != src) std::memcpy(dst, src, (size_t)stride * h);
        return;
    }
    int kernel = 2 * radius + 1;
    int halfK  = radius;

    for (int y = 0; y < h; ++y){
        const uint8_t* srcRow = src + (size_t)y * stride;
        uint8_t*       dstRow = dst + (size_t)y * stride;

        // 初始 sumX = 范围 [-r..r] 内的累加 (左右各 clamp 到边界)
        int sumB = 0, sumG = 0, sumR = 0, sumA = 0;
        for (int k = -halfK; k <= halfK; ++k){
            int sx = k;
            if (sx < 0)      sx = 0;
            else if (sx >= w) sx = w - 1;
            const uint8_t* p = srcRow + sx * 4;
            sumB += p[0]; sumG += p[1]; sumR += p[2]; sumA += p[3];
        }

        for (int x = 0; x < w; ++x){
            uint8_t* d = dstRow + x * 4;
            d[0] = (uint8_t)(sumB / kernel);
            d[1] = (uint8_t)(sumG / kernel);
            d[2] = (uint8_t)(sumR / kernel);
            d[3] = (uint8_t)(sumA / kernel);

            int xLeave = x - halfK;
            int xEnter = x + halfK + 1;
            if (xLeave < 0)      xLeave = 0;
            else if (xLeave >= w) xLeave = w - 1;
            if (xEnter < 0)      xEnter = 0;
            else if (xEnter >= w) xEnter = w - 1;
            const uint8_t* pIn  = srcRow + xEnter * 4;
            const uint8_t* pOut = srcRow + xLeave * 4;
            sumB += pIn[0] - pOut[0];
            sumG += pIn[1] - pOut[1];
            sumR += pIn[2] - pOut[2];
            sumA += pIn[3] - pOut[3];
        }
    }
}

//----------------------------------------------------------------------------
// 单 pass 垂直 box blur (32-bit BGRA premultiplied, 滑窗求和, O(n)).
//----------------------------------------------------------------------------
static void _CXShadow_BoxBlurV(uint8_t* dst, const uint8_t* src,
                                int w, int h, int stride, int radius)
{
    if (radius <= 0){
        if (dst != src) std::memcpy(dst, src, (size_t)stride * h);
        return;
    }
    int kernel = 2 * radius + 1;
    int halfK  = radius;

    for (int x = 0; x < w; ++x){
        // 初始累加: 列 [-r..r] (clamp)
        int sumB = 0, sumG = 0, sumR = 0, sumA = 0;
        for (int k = -halfK; k <= halfK; ++k){
            int sy = k;
            if (sy < 0)       sy = 0;
            else if (sy >= h)  sy = h - 1;
            const uint8_t* p = src + (size_t)sy * stride + x * 4;
            sumB += p[0]; sumG += p[1]; sumR += p[2]; sumA += p[3];
        }

        for (int y = 0; y < h; ++y){
            uint8_t* d = dst + (size_t)y * stride + x * 4;
            d[0] = (uint8_t)(sumB / kernel);
            d[1] = (uint8_t)(sumG / kernel);
            d[2] = (uint8_t)(sumR / kernel);
            d[3] = (uint8_t)(sumA / kernel);

            int yLeave = y - halfK;
            int yEnter = y + halfK + 1;
            if (yLeave < 0)       yLeave = 0;
            else if (yLeave >= h)  yLeave = h - 1;
            if (yEnter < 0)       yEnter = 0;
            else if (yEnter >= h)  yEnter = h - 1;
            const uint8_t* pIn  = src + (size_t)yEnter * stride + x * 4;
            const uint8_t* pOut = src + (size_t)yLeave * stride + x * 4;
            sumB += pIn[0] - pOut[0];
            sumG += pIn[1] - pOut[1];
            sumR += pIn[2] - pOut[2];
            sumA += pIn[3] - pOut[3];
        }
    }
}

//----------------------------------------------------------------------------
// N 遍 box blur ≈ Gaussian blur. N 遍累加方差 = N · r²/3, sigma_total = r·√(N/3).
// 默认 N=3 (高斯近似最佳 / W3C box-shadow 视觉); 拖拽时 N=1 跑得最快, 视觉偏 "盒"
// 但 60Hz 下不易察觉, 拖拽结束后会再渲一帧 N=3 高质量.
//
// extTmp 由调用方持有以避免每帧 ~stride*h 的 heap 分配 (拖拽累计上千次).
// 单遍 box 半径 = radius / N, 保证 "总扩散" 大约不变.
//----------------------------------------------------------------------------
static void _CXShadow_GaussianBlur(
    uint8_t* pixels, int w, int h, int stride, int radius,
    int passes, std::vector<uint8_t>* extTmp)
{
    if (radius <= 0 || w <= 0 || h <= 0 || passes <= 0) return;

    int r = radius / passes;
    if (r < 1) r = 1;

    size_t need = (size_t)stride * h;
    std::vector<uint8_t> localTmp;
    std::vector<uint8_t>& tmp = extTmp ? *extTmp : localTmp;
    if (tmp.size() < need) tmp.resize(need);

    // 每遍都是 H + V; HVH 视觉与 VHV 相当, 选 HVHVHV 序列以匹配 cache friendliness
    for (int p = 0; p < passes; ++p){
        _CXShadow_BoxBlurH(tmp.data(), pixels,    w, h, stride, r);
        _CXShadow_BoxBlurV(pixels,    tmp.data(), w, h, stride, r);
    }
}

//----------------------------------------------------------------------------
// 简易 Win11 风格主题预设.
//   light = 浅色背景下的暖灰阴影 + 浅描边
//   dark  = 深色背景下的强阴影 + 深描边 (略加白边补偿)
//----------------------------------------------------------------------------
struct _XShadow_ThemePreset{
    COLORREF shadowColor;
    COLORREF inactiveShadow;
    COLORREF borderColor;
    COLORREF innerBg;          // 内圈背景默认色 (alpha=255 不透明)
};

static _XShadow_ThemePreset _CXShadow_GetThemePreset(int theme){
    _XShadow_ThemePreset p;
    switch (theme){
    case xshadow_theme_light:
        // Win11 light 窗口 (双层 key+ambient 合成估值, 参 Fluent2 shadow28):
        //   key  alpha 黑 10% (active) / 5.5% (inactive)
        //   ambient 自动派生: blur×1.6, alpha×0.55
        //   stroke alpha 黑 25% — center stroke 50% 压在白 innerBg 上, 12% 黑 alpha
        //     blend 出来的浅灰 (~#E0E0E0) 在 #FCFCFC 上对比仅 3% → 视觉糊边. 25%
        //     blend 出 ~#C0C0C0, 对比 ~24%, 跟 Win11 Edge / 设置 实测描边可视感对齐.
        //     深色主题白描边 12% 在 #202020 上对比已 ~25%, 不需要调.
        //   innerBg = #FCFCFC (Win11 Mica fallback / Acrylic Light 同调)
        p.shadowColor    = 0x1A000000u;
        p.inactiveShadow = 0x0E000000u;
        p.borderColor    = 0x0F000000u;
        p.innerBg        = 0xFFFCFCFCu;
        break;
    case xshadow_theme_dark:
        // Win11 dark 窗口: 深背景需要更强 key (黑 25%) 才看得清, stroke 白 12%.
        // innerBg = #202020 (Win11 dark mode Mica fallback)
        p.shadowColor    = 0x40000000u;     // 25% 黑
        p.inactiveShadow = 0x20000000u;     // 12.5% 黑
        p.borderColor    = 0x1FFFFFFFu;     // 12% 白
        p.innerBg        = 0xFF202020u;
        break;
    default:
        // custom 默认 = light 同调
        p.shadowColor    = 0x1A000000u;
        p.inactiveShadow = 0x0E000000u;
        p.borderColor    = 0x0F000000u;
        p.innerBg        = 0xFFFCFCFCu;
        break;
    }
    return p;
}

}  // namespace

//============================================================================
// 内部几何计算
//============================================================================
struct _XShadow_Geom {
    int marginL, marginT, marginR, marginB;  // 物理像素
    int innerW,  innerH;                      // 物理像素 (= 主窗口外接矩形宽高)
    int dibW,    dibH;                        // = innerW + marginL + marginR ...
    int dx, dy;                               // 物理
    int spread;                               // 物理
    int blur;                                 // 物理
    int corner;                               // 物理
    int inset;                                // 物理
    float borderW;                            // 物理
};

static _XShadow_Geom _CXShadow_ComputeGeom(
    int innerW, int innerH, float dpiScale,
    int dx_log, int dy_log, int spread_log, int blur_log, int corner_log,
    int inset_phys, float borderW_log)
{
    _XShadow_Geom g{};
    auto round_pos = [](double v) -> int {
        if (v >= 0) return (int)(v + 0.5);
        return -(int)(-v + 0.5);
    };
    g.dx       = round_pos(dx_log     * dpiScale);
    g.dy       = round_pos(dy_log     * dpiScale);
    g.spread   = round_pos(spread_log * dpiScale);
    g.blur     = round_pos(blur_log   * dpiScale);
    g.corner   = round_pos(corner_log * dpiScale);
    g.inset    = inset_phys;
    // 描边宽度: 缩放后 *四舍五入到整数像素*. 不取整会因为 GDI+ Pen 用 1.5 px
    // 渲成 2-px AA (边缘半透) 而被用户看成 "还是 1px". 取整后:
    //   100% -> round(1.0) = 1 (1 px 实心)
    //   125% -> round(1.25) = 1
    //   150% -> round(1.5) = 2 (2 px 实心, 视觉变粗)
    //   200% -> round(2.0) = 2
    float bwRaw = borderW_log * dpiScale;
    if (bwRaw < 0.0f) bwRaw = 0.0f;
    g.borderW = (bwRaw < 0.5f) ? bwRaw : std::floor(bwRaw + 0.5f);

    if (g.spread < 0) g.spread = 0;
    if (g.blur   < 0) g.blur   = 0;
    if (g.corner < 0) g.corner = 0;
    if (g.inset  < 0) g.inset  = 0;

    // 阴影模糊 + 扩散 + 偏移决定的 margin
    //
    // 关键: 必须为 *两个层都* 留够 fade 空间, 否则 bitmap 边缘截断 Gaussian 尾巴
    // → 视觉上 "一刀切" 平整边. 旧公式 mT = blur - dy (dy>0 时) 会让 top 只有 18px
    // 而 ambient blur 实际reach ≈ 1.6*blur*sqrt(3) ≈ 66px, 严重截断.
    //
    // 新公式: 用 ambient blur (= 1.6*key blur) 作为基准, 加上 offset 方向的额外
    // 位移. 不再减 offset (旧版的省像素优化太激进, 视觉就毁了).
    //
    //   half = ambBlur + spread        (两层都覆盖)
    //   mL = half + max(0, -dx)        (dx<0 → shadow 左移 → 左边需更多 margin)
    //   mR = half + max(0,  dx)
    //   mT = half + max(0, -dy)
    //   mB = half + max(0,  dy)        (dy>0 → shadow 下移 → 底边更宽, 顶边仍 half)
    int ambBlur = (int)((float)g.blur * 1.6f + 0.5f);
    if (ambBlur < g.blur) ambBlur = g.blur;
    int half = ambBlur + g.spread;
    int mL = half + ((g.dx < 0) ? -g.dx : 0);
    int mT = half + ((g.dy < 0) ? -g.dy : 0);
    int mR = half + ((g.dx > 0) ?  g.dx : 0);
    int mB = half + ((g.dy > 0) ?  g.dy : 0);
    g.marginL = mL;
    g.marginT = mT;
    g.marginR = mR;
    g.marginB = mB;

    g.innerW = innerW;
    g.innerH = innerH;
    g.dibW   = innerW + g.marginL + g.marginR;
    g.dibH   = innerH + g.marginT + g.marginB;

    // clamp 圆角到内圈短边一半
    int maxC = (std::min)(innerW, innerH) / 2;
    if (g.corner > maxC) g.corner = maxC;

    return g;
}

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
    if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;

    // 已附加同一窗口 -> no-op
    if (m_hAttachedWnd == hWnd) return TRUE;

    // 附加到不同窗口 -> 先解除
    if (m_hAttachedWnd != NULL) Detach();

    HWND raw = ::XWnd_GetHWND(hWnd);
    if (!raw) return FALSE;

    _CXShadow_EnsureGdiPlus();

    m_hAttachedWnd = hWnd;
    m_hMainHwnd    = raw;

    // 注册全局表
    {
        std::lock_guard<std::mutex> lk(g_hostMutex);
        g_hostMap[hWnd] = this;
    }
    // 注册全局实例集 (供 SetGlobalTheme 广播 / WM_SETTINGCHANGE 等用)
    {
        std::lock_guard<std::mutex> lk(g_shadowInstancesMutex);
        g_shadowInstances.insert(this);
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
    m_dirty       = true;

    if (m_isMaximized) ClearPadding();   // 最大化起始 → 去 padding

    // 6) 主题: 优先 adopt 用户设的全局主题 (CXShadow::SetGlobalTheme), 否则用 m_theme
    //    默认值 (xshadow_theme_auto). 总之 attach 完后 m_shadowColor 等是 *已应用* 状态.
    int gt = g_globalTheme.load();
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
        std::lock_guard<std::mutex> lk(g_hostMutex);
        g_hostMap.erase(m_hAttachedWnd);
    }
    {
        std::lock_guard<std::mutex> lk(g_shadowInstancesMutex);
        g_shadowInstances.erase(this);
    }

    // 释放 DIB + HIMAGE
    ReleaseShadowImage();
    FreeDib();

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
        // 拖拽 (移动 / 边框 resize) 开始 → 快速 1-pass blur
        m_inSizeMove = true;
        break;

    case WM_EXITSIZEMOVE:
        // 拖拽结束 → 立即强渲一帧 3-pass 高质量.
        // 此时还要 SyncWindowState — 拖拽期间 SyncWindowState 走 m_inSizeMove guard
        // 跳过 padding 切换, 累积的 snap / max 状态在松手这一刻一次性 commit. 不调
        // 的话会出现 "拖到 snap 区域松手, 阴影却仍在" 的状态滞留.
        m_inSizeMove = false;
        SyncWindowState();
        m_dirty = true;
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
            if (m_dpi != oldDpi){
                m_dirty = true;
            }
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
            m_dirty = true;
            if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
        }
        break;

    case WM_ACTIVATE:
        {
            WORD a = LOWORD(wParam);
            bool nowActive = (a != WA_INACTIVE);
            if (nowActive != m_isActive){
                m_isActive = nowActive;
                m_dirty = true;
                if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
            }
        }
        break;

    case WM_DPICHANGED:
        // 关键: 必须先 RefreshDpi (即使 WM_SIZE 里也会再读一次, 这里保证如果系统先发
        // WM_DPICHANGED 再发 WM_SIZE, 第一帧 SyncWindowState 用的就是新 DPI).
        RefreshDpi();
        SyncWindowState();
        m_dirty = true;
        if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
        break;

    case WM_DESTROY:
        // 主窗口被销毁: 短路所有后续 XCGUI 调用 (句柄即将 invalidate).
        m_eventsHooked = false;
        m_subclassInstalled = false;   // 子类随主窗死掉, 不要再 RemoveWindowSubclass
        {
            std::lock_guard<std::mutex> lk(g_hostMutex);
            g_hostMap.erase(m_hAttachedWnd);
        }
        {
            std::lock_guard<std::mutex> lk(g_shadowInstancesMutex);
            g_shadowInstances.erase(this);
        }
        m_hAttachedWnd = NULL;
        m_hMainHwnd    = NULL;
        ReleaseShadowImage();
        FreeDib();
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

//============================================================================
// DIB 管理
//============================================================================
BOOL CXShadow::EnsureDib(int w, int h){
    if (w <= 0 || h <= 0) return FALSE;
    if (m_hDib && m_dibW == w && m_dibH == h) return TRUE;

    FreeDib();

    HDC hScreen = ::GetDC(NULL);
    if (!hScreen) return FALSE;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = w;
    bi.bmiHeader.biHeight      = -h;       // 负数 = top-down
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hBmp = ::CreateDIBSection(hScreen, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
    ::ReleaseDC(NULL, hScreen);
    if (!hBmp || !pBits) return FALSE;

    HDC memDC = ::CreateCompatibleDC(NULL);
    if (!memDC){
        ::DeleteObject(hBmp);
        return FALSE;
    }
    HBITMAP hOld = (HBITMAP)::SelectObject(memDC, hBmp);

    m_hMemDC   = memDC;
    m_hDib     = hBmp;
    m_hOldBmp  = hOld;
    m_pPixels  = pBits;
    m_dibW     = w;
    m_dibH     = h;
    return TRUE;
}

void CXShadow::FreeDib(){
    if (m_hMemDC){
        if (m_hOldBmp) ::SelectObject(m_hMemDC, m_hOldBmp);
        ::DeleteDC(m_hMemDC);
        m_hMemDC = NULL;
    }
    if (m_hDib){
        ::DeleteObject(m_hDib);
        m_hDib = NULL;
    }
    m_hOldBmp = NULL;
    m_pPixels = NULL;
    m_dibW    = 0;
    m_dibH    = 0;
    // 也释放 blur 临时缓冲 + ambient 暂存, 避免 Detach 后还占着 stride*h ~MB × 2
    m_blurTmp.clear();
    m_blurTmp.shrink_to_fit();
    m_ambientLayer.clear();
    m_ambientLayer.shrink_to_fit();
}

//============================================================================
// Invalidate: 标 dirty + 触发主窗重绘. 主窗 WM_PAINT 走到 OnWndPaintImpl 时,
// 会基于 dirty 标志重新生成 shadow bitmap.
//============================================================================
void CXShadow::Invalidate(){
    if (m_hAttachedWnd == NULL) return;
    m_dirty = true;
    ::XWnd_Redraw(m_hAttachedWnd, FALSE);
}

//----------------------------------------------------------------------------
// RenderBitmapFor: 按内圈像素尺寸 (mw, mh) 渲染阴影 bitmap 到 m_pPixels (DIB).
//   mw, mh = 主窗 client width/height - padding 总和, 即除去阴影圈后的真实元素区尺寸.
//   shadow 画在 padding 圈 (client 内的最外圈), 不占元素区.
//----------------------------------------------------------------------------
void CXShadow::RenderBitmapFor(int mw, int mh){
    if (mw <= 0 || mh <= 0) return;

    // 每帧重取 DPI: 不能只靠 WM_DPICHANGED. 用户运行期改系统缩放, XCGUI 不一定
    // 把 WM_DPICHANGED 派到我们的 hook 链, 但 XWnd_GetDPI 会 *实时* 返回新值.
    // 单次调用 ~0.1us, 完全可以承受.
    RefreshDpi();

    auto g = _CXShadow_ComputeGeom(
        mw, mh, m_dpiScale,
        m_shadowDx.load(), m_shadowDy.load(),
        m_shadowSpread.load(), m_shadowRadius.load(),
        m_cornerRadius.load(), m_inset.load(),
        m_borderWidth.load());

    if (!EnsureDib(g.dibW, g.dibH)) return;

    int stride = g.dibW * 4;

    // 1) 清零 DIB (全透明)
    std::memset(m_pPixels, 0, (size_t)stride * g.dibH);

    // 内圈坐标 (主窗口在 DIB 中对应的矩形)
    int innerL = g.marginL;
    int innerT = g.marginT;
    int innerR = innerL + g.innerW;
    int innerB = innerT + g.innerH;

    // 当前活动状态决定阴影色
    COLORREF shc = m_isActive ? m_shadowColor.load() : m_inactiveShadow.load();
    BYTE shA = GetRGBA_A(shc);

    // 派生 ambient 层参数 — 比 key 更柔更广更轻, 给 "景深" 感:
    //   blur:  key.blur * 1.6  (大 ~60% 的散射半径)
    //   alpha: key.alpha * 0.55 (~55% 强度)
    //   dy:    0 (不偏移, 全方向均匀晕)
    //   spread: key.spread + 2 (源略外胀, 模拟 ambient 亮度从窗外开始)
    const int  ambBlur   = (g.blur > 0) ? (int)((float)g.blur * 1.6f + 0.5f) : 0;
    const BYTE ambA      = (BYTE)(((int)shA * 55) / 100);
    const int  ambSpread = g.spread + 2;
    const bool useAmbient = (ambBlur > 0 && ambA > 0);

    // ===== Step 1a: ambient 层 → m_ambientLayer (软晕, 大模糊) =====
    if (useAmbient){
        size_t need = (size_t)stride * g.dibH;
        if (m_ambientLayer.size() < need) m_ambientLayer.resize(need);
        std::memset(m_ambientLayer.data(), 0, need);

        int aL = innerL - ambSpread;
        int aT = innerT - ambSpread;
        int aR = innerR + ambSpread;
        int aB = innerB + ambSpread;
        int aCornerR = g.corner + ambSpread;
        if (aCornerR < 0) aCornerR = 0;

        Gdiplus::Bitmap bmp(g.dibW, g.dibH, stride,
                            PixelFormat32bppPARGB, m_ambientLayer.data());
        Gdiplus::Graphics gp(&bmp);
        gp.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        gp.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        gp.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);

        Gdiplus::Color color(ambA, GetRGBA_R(shc), GetRGBA_G(shc), GetRGBA_B(shc));
        Gdiplus::SolidBrush brush(color);
        Gdiplus::GraphicsPath path;
        _CXShadow_AddRoundedRectPath(path,
            (float)aL, (float)aT,
            (float)(aR - aL), (float)(aB - aT),
            (float)aCornerR);
        gp.FillPath(&brush, &path);

        // ambient 用 *与 key 相同* 的 pass 数 (拖拽 1 / 静止 3) 保证视觉一致.
        int passes = m_inSizeMove ? 1 : 3;
        _CXShadow_GaussianBlur(m_ambientLayer.data(), g.dibW, g.dibH, stride,
                               ambBlur, passes, &m_blurTmp);
    }

    // ===== Step 1b: key 层 → m_pPixels (主阴影, 偏下, 较小模糊) =====
    if (shA > 0){
        // 阴影源 = 内圈 + offset (dx, dy), 每边膨胀 spread
        int srcL = innerL + g.dx - g.spread;
        int srcT = innerT + g.dy - g.spread;
        int srcR = innerR + g.dx + g.spread;
        int srcB = innerB + g.dy + g.spread;
        int srcCornerR = g.corner + g.spread;
        if (srcCornerR < 0) srcCornerR = 0;

        Gdiplus::Bitmap bmp(g.dibW, g.dibH, stride,
                            PixelFormat32bppPARGB, (BYTE*)m_pPixels);
        Gdiplus::Graphics gp(&bmp);
        gp.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        gp.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        // SourceCopy: 直接写入像素值 (而非 alpha blend), 后面的 blur 期望
        // 阴影源是干净的纯色填充.
        gp.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);

        Gdiplus::Color color(shA, GetRGBA_R(shc), GetRGBA_G(shc), GetRGBA_B(shc));
        Gdiplus::SolidBrush brush(color);

        Gdiplus::GraphicsPath path;
        _CXShadow_AddRoundedRectPath(path,
            (float)srcL, (float)srcT,
            (float)(srcR - srcL), (float)(srcB - srcT),
            (float)srcCornerR);
        gp.FillPath(&brush, &path);
    }

    // ===== Step 2: 模糊 key 层 =====
    //
    // 拖拽 (m_inSizeMove=true) 时降级为 1 遍 box blur:
    //   - 3 遍 -> 1 遍 ≈ 3x 加速, 单帧从 ~6ms 降到 ~2ms
    //   - 视觉差异: 1 遍偏 "盒形" 边缘 (盒子外突然截止), 高斯没有那么硬
    //   - 60Hz 拖拽下用户察觉不到, 释放鼠标 (WM_EXITSIZEMOVE) 时强制再渲一帧 3 遍
    if (g.blur > 0 && shA > 0){
        int passes = m_inSizeMove ? 1 : 3;
        _CXShadow_GaussianBlur((uint8_t*)m_pPixels, g.dibW, g.dibH, stride,
                               g.blur, passes, &m_blurTmp);
    }

    // ===== Step 2b: ambient 层 + key 层 加性合成 =====
    //
    // PARGB premultiplied 下的加性叠加 (saturating):
    //   dst.b = sat(dst.b + src.b)  — premultiplied RGB 等比 alpha 缩放, 直接加即可
    //   dst.g = sat(dst.g + src.g)
    //   dst.r = sat(dst.r + src.r)
    //   dst.a = sat(dst.a + src.a)
    // 物理意义: 两个 "墨" 层叠加, 总不透明度 = 两层不透明度叠加 (上限 1).
    // 这是 "screen" 反义 (multiply 反义) — 加法刚好对应 "光被吸收两次".
    // 4-byte unrolled 32-bit 处理速度: 850×600 → ~510K pixel → ~0.5ms.
    if (useAmbient){
        const size_t total = (size_t)stride * g.dibH;
        uint8_t* dst = (uint8_t*)m_pPixels;
        const uint8_t* src = m_ambientLayer.data();
        for (size_t i = 0; i < total; ++i){
            int v = (int)dst[i] + (int)src[i];
            dst[i] = (v > 255) ? (uint8_t)255 : (uint8_t)v;
        }
    }

    // ===== Step 3: 镂空内圈 (向内扣 inset px) =====
    //
    // *性能关键路径* — 拖拽 resize 时每帧都跑.
    //
    // 旧版: 渲一张 Gdiplus::Bitmap AA 遮罩 (FillPath ~2ms) + LockBits + 全 DIB 像素
    //       逐字节 modulate (~3ms 800x540). 单帧总耗 ~5ms 仅这一步, 是 resize 卡顿
    //       的主要来源.
    //
    // 新版: 拆成 3 区直接写 m_pPixels:
    //         A) 中间矩形 (corners 之间)   — 一行 memset(stride 段) 归零
    //         B) 上 / 下 矩形条           — 一行 memset 归零
    //         C) 四角各 cR×cR 小方块       — 仅这里跑 per-pixel signed-distance AA
    //       省了 GDI+ Bitmap 构造 / FillPath / LockBits 三大开销, 80% 区域只是
    //       memset, 真正逐像素 AA 数学只发生在 ~4 × cR² = 几百~几千像素.
    //
    // 实测 800×540 + corner=8: 旧版 ~5ms → 新版 <0.5ms.
    {
        int phL = innerL + g.inset;
        int phT = innerT + g.inset;
        int phR = innerR - g.inset;
        int phB = innerB - g.inset;
        int phCornerR = g.corner - g.inset;
        if (phCornerR < 0) phCornerR = 0;

        if (phR > phL && phB > phT){
            // clamp 圆角到能放进内圈半边
            int halfW = (phR - phL) / 2;
            int halfH = (phB - phT) / 2;
            int maxC  = (halfW < halfH) ? halfW : halfH;
            if (phCornerR > maxC) phCornerR = maxC;

            uint8_t* const dibBase = (uint8_t*)m_pPixels;

            // 一行内 [xStart, xEnd) 像素四字节全清零 (premultiplied: 一次性归 0 = 透明)
            auto zeroRow = [&](int y, int xStart, int xEnd){
                if (xStart >= xEnd) return;
                std::memset(dibBase + (size_t)y * stride + (size_t)xStart * 4,
                            0, (size_t)(xEnd - xStart) * 4);
            };

            // A) 中间矩形 (上下 corner 之间): 整行宽 [phL, phR)
            for (int y = phT + phCornerR; y < phB - phCornerR; ++y){
                zeroRow(y, phL, phR);
            }
            // B) 上 / 下条 (在 corners 内部的水平段, 不含两端 corner 方块)
            for (int y = phT; y < phT + phCornerR; ++y){
                zeroRow(y, phL + phCornerR, phR - phCornerR);
            }
            for (int y = phB - phCornerR; y < phB; ++y){
                zeroRow(y, phL + phCornerR, phR - phCornerR);
            }

            // C) 四角 AA — signed distance from rounded boundary
            //   覆盖率 coverage(x,y):
            //     d = sqrt((x+0.5-cx)² + (y+0.5-cy)²) - cR
            //     d <= -0.5  -> 完全内 (coverage = 1.0, 全击穿)
            //     d >=  0.5  -> 完全外 (coverage = 0,   不动)
            //     其他        -> AA 带 (coverage = 0.5 - d), 1 像素宽
            //   修正 DIB 像素 = old * (1 - coverage). premultiplied 下 BGRA 整体衰减
            //   既保 RGB 又保 A, 不会产生 fringe.
            if (phCornerR > 0){
                const float cR = (float)phCornerR;

                auto punchCorner = [&](int boxL, int boxT, float cx, float cy){
                    for (int yi = 0; yi < phCornerR; ++yi){
                        const int y  = boxT + yi;
                        const float fy = (float)y + 0.5f - cy;
                        const float fy2 = fy * fy;
                        uint8_t* row = dibBase + (size_t)y * stride;
                        for (int xi = 0; xi < phCornerR; ++xi){
                            const int x  = boxL + xi;
                            const float fx = (float)x + 0.5f - cx;
                            const float dist = std::sqrt(fx*fx + fy2);
                            const float sd   = dist - cR;
                            float coverage;
                            if      (sd <= -0.5f) coverage = 1.0f;
                            else if (sd >=  0.5f) coverage = 0.0f;
                            else                  coverage = 0.5f - sd;

                            if (coverage <= 0.0f) continue;
                            uint8_t* p = row + (size_t)x * 4;
                            if (coverage >= 1.0f){
                                // 整 4 字节归零 (32-bit store, 比 4 次 8-bit 快)
                                *reinterpret_cast<uint32_t*>(p) = 0u;
                            } else {
                                const int inv = (int)((1.0f - coverage) * 255.0f + 0.5f);
                                p[0] = (uint8_t)((p[0] * inv) / 255);
                                p[1] = (uint8_t)((p[1] * inv) / 255);
                                p[2] = (uint8_t)((p[2] * inv) / 255);
                                p[3] = (uint8_t)((p[3] * inv) / 255);
                            }
                        }
                    }
                };

                // 四个 cR×cR 方块, 各自的圆心是 *主圆角* 弧的圆心:
                //   TL: 弧心 (phL+cR, phT+cR), 方块左上 (phL, phT)
                //   TR: 弧心 (phR-cR, phT+cR), 方块左上 (phR-cR, phT)
                //   BL/BR: 对称
                punchCorner(phL,             phT,             (float)phL + cR, (float)phT + cR);
                punchCorner(phR - phCornerR, phT,             (float)phR - cR, (float)phT + cR);
                punchCorner(phL,             phB - phCornerR, (float)phL + cR, (float)phB - cR);
                punchCorner(phR - phCornerR, phB - phCornerR, (float)phR - cR, (float)phB - cR);
            }
        }
    }

    // 描边不 bake 进 DIB — 独立在主窗 WM_PAINT 里用 XDraw_DrawRoundRect 画:
    //   1. D2D 模式走 GPU 路径, 与主窗内容使用同一渲染上下文;
    //   2. 描边颜色 / 宽度改变不需重渲 shadow DIB (DIB 只负责 halo).
    //
    // 见 DrawNormalPaint 的 step 3.
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
    if (m_shadowRadius.exchange(radius) != radius){
        // blur 影响 margin → 必须重 Apply padding, 否则 RenderBitmapFor 渲染的 DIB
        // 与窗口当前 padding 大小不一致, 内圈位置错位 / 内圈描边对不上 shadow halo.
        if (!m_isMaximized) ApplyPadding();
        Invalidate();
    }
}
int  CXShadow::GetShadowRadius() const { return m_shadowRadius.load(); }

void CXShadow::SetShadowSpread(int spread){
    if (spread < 0) spread = 0;
    if (m_shadowSpread.exchange(spread) != spread){
        if (!m_isMaximized) ApplyPadding();  // spread 同样影响 margin
        Invalidate();
    }
}
int  CXShadow::GetShadowSpread() const { return m_shadowSpread.load(); }

void CXShadow::SetShadowOffset(int dx, int dy){
    int oldDx = m_shadowDx.exchange(dx);
    int oldDy = m_shadowDy.exchange(dy);
    if (oldDx != dx || oldDy != dy){
        if (!m_isMaximized) ApplyPadding();  // offset 影响 margin 4 边 (各向异性)
        Invalidate();
    }
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
//   * AttachToWnd: 注册到 g_shadowInstances; 若 g_globalTheme != custom,
//     adopt 全局值.
//============================================================================

// (g_shadowInstances / g_shadowInstancesMutex / g_globalTheme 在文件顶部声明)

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
    g_globalTheme.store(theme);
    // 广播到所有活实例. 拷贝一份指针快照, 避免持锁回调用户代码.
    std::vector<CXShadow*> snapshot;
    {
        std::lock_guard<std::mutex> lk(g_shadowInstancesMutex);
        snapshot.assign(g_shadowInstances.begin(), g_shadowInstances.end());
    }
    for (CXShadow* p : snapshot){
        if (p) p->SetTheme(theme);
    }
}
int CXShadow::GetGlobalTheme(){
    return g_globalTheme.load();
}

void CXShadow::ApplyThemePreset(int theme){
    // 解析 auto → 系统真实 light/dark
    int actual = theme;
    if (actual == xshadow_theme_auto){
        actual = IsSystemDarkMode() ? xshadow_theme_dark : xshadow_theme_light;
    }
    auto p = _CXShadow_GetThemePreset(actual);
    m_shadowColor.store(p.shadowColor);
    m_inactiveShadow.store(p.inactiveShadow);
    m_borderColor.store(p.borderColor);
    // 内圈背景: 若用户未通过 SetInnerBgColor 显式自定义, 则 follow theme.
    if (!m_innerBgUserSet.load()){
        m_innerBgColor.store(p.innerBg);
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
    // 回退到当前主题的默认色 (auto 这里也会解析)
    int theme = m_theme.load();
    int actual = (theme == xshadow_theme_auto)
                  ? (IsSystemDarkMode() ? xshadow_theme_dark : xshadow_theme_light)
                  : theme;
    auto p = _CXShadow_GetThemePreset(actual);
    if (m_innerBgColor.exchange(p.innerBg) != p.innerBg) Invalidate();
}

COLORREF CXShadow::GetInnerBgColor() const { return m_innerBgColor.load(); }

BOOL CXShadow::IsSystemDarkMode(){
    // HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize
    //   AppsUseLightTheme = 0 -> Dark
    HKEY hKey = NULL;
    LSTATUS s = ::RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey);
    if (s != ERROR_SUCCESS) return FALSE;
    DWORD v = 1, sz = sizeof(v), type = 0;
    s = ::RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, &type,
                           (BYTE*)&v, &sz);
    ::RegCloseKey(hKey);
    if (s != ERROR_SUCCESS || type != REG_DWORD) return FALSE;
    return v == 0 ? TRUE : FALSE;
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
// 公式与 _CXShadow_ComputeGeom 严格同步:
//   half = ambBlur(1.6*blur) + spread
//   mL = half + max(0,-dx); mT = half + max(0,-dy);
//   mR = half + max(0, dx); mB = half + max(0, dy);
// 见 _CXShadow_ComputeGeom 的详细注释.
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
// 仅在 3 个地方临时 × m_dpiScale 转物理:
//   1) RenderBitmapFor 输入 (DIB 必须按物理像素分配)
//   2) ComputeNcHitTest 比较 (GetWindowRect / NCHITTEST screen 坐标是物理)
//   3) WM_GETMINMAXINFO ptMinTrackSize (物理)
void CXShadow::ComputeShadowMargin(int* pL, int* pT, int* pR, int* pB) const {
    int dx     = m_shadowDx.load();
    int dy     = m_shadowDy.load();
    int spread = m_shadowSpread.load();
    int blur   = m_shadowRadius.load();
    if (spread < 0) spread = 0;
    if (blur   < 0) blur   = 0;

    int ambBlur = (int)((float)blur * 1.6f + 0.5f);
    if (ambBlur < blur) ambBlur = blur;
    int half = ambBlur + spread;
    int mL = half + ((dx < 0) ? -dx : 0);
    int mT = half + ((dy < 0) ? -dy : 0);
    int mR = half + ((dx > 0) ?  dx : 0);
    int mB = half + ((dy > 0) ?  dy : 0);
    if (pL) *pL = mL;
    if (pT) *pT = mT;
    if (pR) *pR = mR;
    if (pB) *pB = mB;
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
    m_dirty = true;
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
    m_dirty = true;
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
            m_dirty = true;
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
        m_dirty = true;
        if (m_hAttachedWnd) ::XWnd_Redraw(m_hAttachedWnd, FALSE);
    }
}

//============================================================================
// v2 HIMAGE 包装管理
//
// 把 m_pPixels (PARGB DIB) 暴露给 XCGUI 作为 HIMAGE. 维度变 → 重建; 同维度 →
// 用 XImage_ModifyData 原地更新像素 (避免 destroy/create 引用计数抖动).
//
// 注: XImage_LoadFromData 文档说 "BGRA32位 (R,G,B,A)" — XCGUI 内部以 BGRA byte
// 顺序存储, 我们 DIB 也是 BGRA premultiplied, byte 顺序一致. premultiplied
// vs straight 的差异:
//   - premultiplied: RGB 已经乘以 alpha (例: 半透红 = (255*0.5, 0, 0, 128))
//   - straight:      RGB 是颜色, alpha 是覆盖率 (例: (255, 0, 0, 128))
// XCGUI 走 D2D 时用 DXGI_FORMAT_B8G8R8A8_UNORM, 这是 premultiplied 公约;
// GDI+ 走 PixelFormat32bppPARGB 也是 premultiplied. 一致.
//============================================================================
BOOL CXShadow::EnsureShadowImage(){
    if (!m_pPixels || m_dibW <= 0 || m_dibH <= 0) return FALSE;
    if (m_hShadowImage && m_imgW == m_dibW && m_imgH == m_dibH){
        return ::XImage_ModifyData(m_hShadowImage, m_pPixels, m_dibW, m_dibH);
    }
    if (m_hShadowImage){
        ::XImage_Destroy(m_hShadowImage);
        m_hShadowImage = NULL;
    }
    m_hShadowImage = ::XImage_LoadFromData(m_pPixels, m_dibW, m_dibH);
    if (m_hShadowImage){
        m_imgW = m_dibW;
        m_imgH = m_dibH;
        return TRUE;
    }
    m_imgW = m_imgH = 0;
    return FALSE;
}

void CXShadow::ReleaseShadowImage(){
    if (m_hShadowImage){
        ::XImage_Destroy(m_hShadowImage);
        m_hShadowImage = NULL;
    }
    m_imgW = m_imgH = 0;
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
    // 0) 开启平滑 (AA). XCGUI 默认是关的, 不开 fill/stroke 圆角都会有阶梯锯齿.
    //    XDraw_EnableSmoothingMode 同时影响 FillRoundRectEx 和 DrawRoundRectEx.
    ::XDraw_EnableSmoothingMode(hDraw, TRUE);

    // *单位: 全部逻辑像素* — cw/ch 来自 XWnd_GetClientRect (逻辑), m_margin* 现在也是
    // 逻辑 (Fix DPI 后), 所有传给 XDraw_* 的坐标 / 圆角 / 线宽都是逻辑. XCGUI 内部
    // 自己 × dpiScale 物理化.
    const int mL = m_marginL, mT = m_marginT, mR = m_marginR, mB = m_marginB;
    const int innerW = cw - mL - mR;
    const int innerH = ch - mT - mB;
    if (innerW <= 0 || innerH <= 0) return;

    // 2) Shadow bitmap (DIB) — *物理像素* 渲染. innerW × dpiScale 转物理给 RenderBitmapFor.
    //    DIB 缓存比对也用物理: image 实际像素数 vs 期望物理像素数.
    auto rp = [](double v) -> int {
        return (v >= 0) ? (int)(v + 0.5) : -(int)(-v + 0.5);
    };
    int innerW_phys = rp((double)innerW * m_dpiScale);
    int innerH_phys = rp((double)innerH * m_dpiScale);
    int mL_phys = rp((double)mL * m_dpiScale);
    int mT_phys = rp((double)mT * m_dpiScale);
    int mR_phys = rp((double)mR * m_dpiScale);
    int mB_phys = rp((double)mB * m_dpiScale);
    int dibW_phys = innerW_phys + mL_phys + mR_phys;
    int dibH_phys = innerH_phys + mT_phys + mB_phys;
    if (m_dirty || !m_hShadowImage
        || m_imgW != dibW_phys || m_imgH != dibH_phys)
    {
        RenderBitmapFor(innerW_phys, innerH_phys);
        EnsureShadowImage();
        m_dirty = false;
    }

    // 3) 画 shadow halo. *关键*: XDraw_Image (无尺寸版) 把 image 像素当 *逻辑单位*
    //    1:1 face-value 渲, image_phys 个像素 → 占 image_phys 逻辑单位 → 视觉 ×
    //    dpiScale 物理 = image_phys × 1.5 物理 → 超出窗口 1.5 倍裁掉. 必须用 XDraw_
    //    ImageEx 显式指定 *逻辑* 目标尺寸 = client logical, 这样 XCGUI 内部缩到物理
    //    cw_phys × ch_phys 像素 = image 实际像素数 (1:1 像素映射, 高清 + alignment ✓).
    if (m_hShadowImage){
        int dstW = mL + innerW + mR;   // = cw 逻辑
        int dstH = mT + innerH + mB;   // = ch 逻辑
        ::XDraw_ImageEx(hDraw, m_hShadowImage, 0, 0, dstW, dstH);
    }

    // 4) 内圈背景: 圆角 fill. corner *逻辑像素* (与 m_cornerRadius 用户配置同单位).
    int cornerLog = m_cornerRadius.load();
    if (cornerLog < 0) cornerLog = 0;
    int maxC = (std::min)(innerW, innerH) / 2;
    if (cornerLog > maxC) cornerLog = maxC;

    RECT rcInner = { mL, mT, mL + innerW, mT + innerH };

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
        // 用户清了 bg → 给个主题 fallback, 不然全屏会变成 *透明洞*.
        auto p = _CXShadow_GetThemePreset(m_theme.load());
        bg = p.innerBg;
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
