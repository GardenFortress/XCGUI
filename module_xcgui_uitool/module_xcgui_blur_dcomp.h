// =============================================================================
// module_xcgui_blur_dcomp.h — Win10 1803+ 直接走 DComp + Windows.UI.Composition
// 实现真正的 Win11 acrylic (Blur + Saturation + LuminosityBlend + Noise),
// 不再受 ACCENT_POLICY 在不同 OS 上的退化影响.
//
// 这是 module_xcgui_blur 的内部辅助模块, 提供 C 风格 API. 实现细节 (WinRT 类型,
// D2D effect 实现, 噪声烤制等) 全部隐藏在 .cpp 内, header 不暴露任何 WinRT/D2D
// 类型给上层. 主模块只看 HWND + 参数.
//
// 路线选择上下文:
//   主模块 XBlur_PickPath() 负责选路径. 这模块只对应 XBLUR_PATH_DCOMP_WINRT 一档.
//   走这条路径时不再调 SetWindowCompositionAttribute, 由 DComp visual tree 直接
//   接管客户区合成. 与 ACCENT_POLICY 不能同时启用 (互斥).
//
// 集成点:
//   * XBlurDComp_Apply           — 装 / 重装 effect chain
//   * XBlurDComp_Resize          — WM_SIZE 时同步 visual 大小
//   * XBlurDComp_Disable         — 解绑, 析构资源
//   * XBlurDComp_IsSupported     — 运行时探测 (build + DLL 加载)
// =============================================================================

#pragma once
#include <windows.h>

namespace XBlurDComp {

//-----------------------------------------------------------------------------
// 运行时探测: 当前 OS / DLL 组合是否支持 dcomp 直接合成路径.
// 条件:
//   - Win10 1803+ (build >= 17134) — DispatcherQueue + Compositor 都有
//   - dcomp.dll / windowsapp.lib 加载成功
// 调用应缓存结果 (轻量, 但每次调要走 RtlGetVersion).
//-----------------------------------------------------------------------------
bool IsSupported();

//-----------------------------------------------------------------------------
// 装 / 更新 acrylic 到指定 HWND.
//
// 第一次调: 创建 Compositor + DesktopWindowTarget + visual tree + effect brush.
// 同 HWND 重复调: 仅重建 effect brush (反映新参数), visual tree 不动以避免闪烁.
//
// 参数 (语义 1:1 对应 PoC 测试的环境变量):
//   host                : 目标 HWND, 必须是有 redirection bitmap 的普通窗口
//                         (XCGUI 默认窗即可). 不要求 WS_EX_NOREDIRECTIONBITMAP.
//   tintR, tintG, tintB : tint 色 0..255 (XCGUI RGBA 分量已解出来)
//   tintA               : tint 不透明度 0..255. 仅 blurOpacity<0 时用 (反算
//                         blurOpacity=1-tintA/255). 若 blurOpacity>=0 忽略 tintA.
//   blurOpacity         : blur 通道可见度 0..1 (= "通透感"). <0 表示"按 tintA 反算
//                         或主题默认". 越小 tint 越主导, 越大桌面 blur 越透出.
//                         PoC light=0.5, dark=0.15.
//   saturation          : 色彩饱和度 1.0=不变, 1.2~1.5 推荐, 配合 lumi 使用
//   uniformBrightness   : 1=锁定亮度 (LuminosityBlend, Win11 Start Menu 风),
//                         0=亮度跟桌面起伏 (老式 Aero acrylic 风)
//   noiseAlphaPct       : 噪声 alpha 百分比 0..100. 0=关. Win11 标准 1~3%.
//                         用 BlendEffect.MULTIPLY 跟 blur+tint 合成.
//   shadowFrameInset    : 客户区内向 4 边收缩多少 px, 给 EnableNativeShadow 的
//                         frame extension 让位. 不需要时传 0.
//
// 返回 true 表示装好 (DComp visual 已绘制), false 表示底层创建失败.
// 失败时主模块应回退到老路径 (ACCENT / DECORATIVE).
//-----------------------------------------------------------------------------
bool Apply(HWND host,
           int tintR, int tintG, int tintB, int tintA,
           float blurOpacity,
           float saturation,
           BOOL uniformBrightness,
           float noiseAlphaPct,
           int shadowFrameInset);

//-----------------------------------------------------------------------------
// 同步 visual 尺寸到当前客户区. 在主模块 WM_SIZE / WM_EXITSIZEMOVE hook 调.
// shadowFrameInset 与 Apply 时一致 (主模块持有, dcomp 模块无状态).
// host 未绑定时静默 no-op.
//-----------------------------------------------------------------------------
void Resize(HWND host, int shadowFrameInset);

//-----------------------------------------------------------------------------
// 给 acrylic 的 root visual 加圆角 clip — visual 内容裁圆角, visual 之外 (HWND
// 矩形 4 角) 不渲染 = 透明.
//
// borderInset > 0: visual 整体再向内缩 N px, acrylic HWND 边缘 N px 完全无内容. 关键
// 用途: 配合 owner-owned 架构, 让 owned 子窗 (XCGUI) 的系统描边正好压在 acrylic 边缘
// inset 区, 描边只跟桌面 alpha 混合, 不跟 acrylic dcomp 内容混合 = Win11 标准描边视觉.
// 不内缩时 dcomp 内容画到 acrylic HWND 边缘, 透过子窗描边 alpha 透出 → 视觉描边变粗.
//
// radius=0 (含 borderInset>0) 关闭裁切. 装 acrylic 后随时可调, Resize 自动跟随.
// host 未绑定时静默 no-op.
//-----------------------------------------------------------------------------
void SetCornerRadius(HWND host, float radius, float borderInset = 0.0f);

//-----------------------------------------------------------------------------
// 解绑指定 HWND. 析构 visual tree + Compositor + DesktopWindowTarget.
// 重复调安全 (no-op for unknown host).
//-----------------------------------------------------------------------------
void Disable(HWND host);

//-----------------------------------------------------------------------------
// AttachAcrylicHost — 把 test_blur_main PoC 里手写的 acrylic 块封装成一个调用. 内部
// 1:1 跟 PoC 顺序一致 (注册类 → SetThreadDpi(PMv2) → CreateWindowExW → 还原 DPI →
// Apply → owner-owned → 装 2 个 subclass → ShowWindow). 每 XCGUI 主窗独立 acrylic
// HWND, 通过 GetAcrylicHwnd(hxw) 取出. 目标 XCGUI 窗口已有 owner 时, acrylic 会继承
// 该 owner；解绑时恢复原 owner 与 WS_EX_APPWINDOW 状态, 不要求调用方额外传递父窗口句柄.
//
// ⚠ 调用方负责在调本接口 *之前* 先做以下两步:
//      XWnd_SetTransparentType(hxw, window_transparent_shaped);
//      XWnd_EnableDrawBk(hxw, FALSE);
//
// ⚠ 调用方负责在调本接口 *之后* 紧跟以下 4 步:
//      XWnd_SetTransparentType(hxw, window_transparent_shaped);
//      XWnd_SetTransparentAlpha(hxw, 255);
//      XWnd_SetBkInfo(hxw, L"{99:1.9.9;98:1(0);5:2(15)20(1)21(3)26(1)22(16777216)23(1)9(8,8,8,8);}");
//      XWnd_EnableDragWindow(hxw, TRUE);
// 这 6 个 XCGUI API 实测必须在调用方 TU 内调 (跟 module_xcgui.h 同 TU), 不能塞进
// 本 dcomp 模块 — 跨 TU 后 XCGUI 内部行为在高 DPI 下视觉错位.
//
// hxw : XCGUI 主窗 HWINDOW (XWnd_Create 返回的). 用 void* 接收避免 dcomp.h 依赖
//       module_xcgui.h.
// 其他参数同 Apply.
// 返回 acrylic HWND (= GetAcrylicHwnd(hxw)), 失败 NULL.
//
// 销毁: 装在 XCGUI 上的 subclass 收 WM_NCDESTROY 自动 Disable + DestroyWindow(acrylic).
//-----------------------------------------------------------------------------
HWND AttachAcrylicHost(void* hxw,
                       int tintR, int tintG, int tintB, int tintA,
                       float blurOpacity,
                       float saturation,
                       BOOL  uniformBrightness,
                       float noiseAlphaPct);

//-----------------------------------------------------------------------------
// 取 AttachAcrylicHost 为指定 XCGUI 主窗创建的 acrylic HWND. 没建过 → NULL.
// hxwOpaque : XCGUI 主窗 HWINDOW (与 AttachAcrylicHost 相同). NULL → 返 NULL.
// 用法示例: 在 XWnd_SetTop / ShowWindow 之后给 acrylic 加 round corner 等 frame 装饰.
//-----------------------------------------------------------------------------
HWND GetAcrylicHwnd(void* hxwOpaque = nullptr);

//-----------------------------------------------------------------------------
// 解绑 AttachAcrylicHost 创建的 acrylic + subclass. host 销毁前可显式调.
// hxw : XCGUI 主窗 HWINDOW (与 AttachAcrylicHost 相同).
//-----------------------------------------------------------------------------
void DetachAcrylicHost(void* hxwOpaque);

} // namespace XBlurDComp
