#ifndef  XCGUI_BLUR_H
#define  XCGUI_BLUR_H
//@模块名称  炫彩界面库-亚克力高斯模糊
//@版本  3.0.0
//@日期  2026-05-15
//@作者  未闻花名
//@QQ    936599025
//@依赖  module_xcgui_class.h
//@模块备注  亚克力 / 磨砂玻璃元素. 走 DWM 系统合成路径 (HWND 级 acrylic 后
//          element 用 alpha 控制透出 backdrop blur), 跟手, 0 帧延迟, 无抓帧.
//
//          路线选择 (运行时按 OS build number 显式选, 无需调用方关心):
//            1) Win11 (build >= 22000)         : ACCENT_ENABLE_ACRYLICBLURBEHIND (真 acrylic)
//            2) Win10 1803~1809 (17134~17763)  : ACCENT_ENABLE_ACRYLICBLURBEHIND (真 acrylic)
//            3) Win10 1903~22H2 (18362~21999)  : ACCENT_ENABLE_BLURBEHIND        (绕开 ACRYLIC 阉割)
//                 微软在 1903 起把 ACRYLIC 的 blur kernel 关掉了, 仍返 TRUE 但
//                 只剩透明+tint 且 resize 卡顿. 走 BLURBEHIND 取 Win10 风格
//                 mild blur (无 tint) 是该段唯一可用方案.
//            4) Win10 1607~1709 (14393~16299)  : ACCENT_ENABLE_BLURBEHIND        (Win10 风 blur)
//            5) Win7 / Win8 / 8.1 / Vista       : 退化为"仅装饰" (tint+border+圆角).
//                 XCGUI 渲染管线不保留 per-pixel alpha, Win7 Aero BLURREGION 在
//                 实际场景下不会出 blur, 故跟 Win8/8.1 一档处理.
//
//          重要权衡 (主动声明给调用者):
//            * 受 Windows 个性化 - 颜色 - 透明效果 开关影响. 用户关闭时
//              系统会自动把 acrylic 退化成纯色, 这是 OS 行为, 应用层无法覆盖.
//              WinUI3 / Office / Edge / VS Code 都受这个影响.
//            * DWM acrylic 是 HWND 级. CXBlur 元素 (Create/AttachToEle) 也会导致
//              它所在的整个窗口客户区开 acrylic. 这与 NTQQ / 微信 / 钉钉 等所有
//              主流 IM 一致.
//
//          支持 三种 绑定模式:
//            * Create        : 自有元素, 用户指定坐标. 启用所在窗口 acrylic.
//            * AttachToEle   : 接管已存在的元素. 启用所在窗口 acrylic.
//            * AttachToWnd   : 创建覆盖整个窗口客户区的子元素. 启用窗口 acrylic.
//          也支持 m_hEle = 用户元素 这种 IDE 式的句柄赋值 (operator=).
//
//          另有 两个 HWND 级 的 DWM 渲染选项 (静态接口, 与 ACCENT 正交不干扰):
//            * EnableNativeRoundedCorner  : *推荐* Win11 21H2+ DWM 原生圆角
//                                            (DWMWA_WINDOW_CORNER_PREFERENCE).
//                                            XCGUI 默认窗带 WS_THICKFRAME, DWM
//                                            会自动给它画 shadow + 描边, 这一个
//                                            接口设上圆角即可成完整 Win11 视觉.
//                                            老系统静默失败 (返 FALSE).
//            * EnableNativeShadow         : *仅在纯 WS_POPUP borderless 窗需要*.
//                                            走 DwmExtendFrameIntoClientArea 强
//                                            触发 DWM frame + shadow. 副作用:
//                                            resize 偶发白闪 + snap 方角. 详见
//                                            接口文档警告. 默认窗别用.
//          这两个接口完全属于 DWM, 与本实例生命周期无关, *不* 在 Detach
//          时自动撤销. 调用方需显式传 FALSE 还原. 多个 CXBlur 实例在同一
//          HWND 上时, 什么时候开 / 关 native shadow / corner 由调用方控制.
//
//          *Win11 by design 限制*: 最大化 / snap 状态下 DWM 不画圆角 / shadow,
//          这是系统级行为, 任何 app 包括 Edge / Settings / Notepad 一致, 接口
//          层无法 override. 详见
//          https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-rounded-corners
//@模块信息结束

// =================================================================
// 头文件依赖拓扑顺序说明:
//   - @依赖 是 IDE 解析器(智能感知/别名/语法着色)用的, 不会自动注入 #include 给 cl.exe;
//     所以下面要再用真实的 #include 链一次.
//   - d2d1.h 必须先于 module_xcgui.h 完成.
// =================================================================

#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <gdiplus.h>

#include <atomic>
#include <cstdint>

#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"

//@src "module_xcgui_blur.cpp"

// =================================================================
// 第三方依赖: GDI / GDI+ / D2D / User32 / DWM.
// =================================================================

//@lib "Gdiplus.lib"
//@lib "Gdi32.lib"
//@lib "User32.lib"
//@lib "D2d1.lib"
//@lib "Dxguid.lib"
//@lib "Dwmapi.lib"

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "D2d1.lib")
#pragma comment(lib, "Dxguid.lib")
#pragma comment(lib, "Dwmapi.lib")

//@别名 取系统版本()
static inline int GetCurrentVersion() {
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (RtlGetVersion) {
            RTL_OSVERSIONINFOW osvi = { 0 };
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            
            if (RtlGetVersion(&osvi) == 0) { // STATUS_SUCCESS
                DWORD major = osvi.dwMajorVersion;
                DWORD minor = osvi.dwMinorVersion;
                DWORD build = osvi.dwBuildNumber;
                
                // Windows 11: 10.0.22000+
                if (major == 10 && build >= 22000) return 11;
                // Windows 10: 10.0 (build < 22000)
                if (major == 10) return 10;
                // Windows 8.1: 6.3
                // Windows 8: 6.2
                if (major == 6 && minor >= 2) return 8;
                // Windows 7: 6.1
                if (major == 6 && minor == 1) return 7;
            }
        }
	}
}

//@隐藏{
class CXBlur;
//@隐藏}

///模糊效果主题预设 (CXBlur::SetTheme)
//@别名 模糊主题
enum xblur_theme_
{
	//@别名 模糊主题_自定义
	xblur_theme_custom    = 0,
	//@别名 模糊主题_浅色
	xblur_theme_light     = 1,
	//@别名 模糊主题_深色
	xblur_theme_dark      = 2,
	//@别名 模糊主题_跟随系统
	xblur_theme_auto      = 3,
};

///DWM 原生圆角预设 (CXBlur::EnableNativeRoundedCorner 参数).
///这些是 Windows 11 21H2+ DwmSetWindowAttribute(DWMWA_WINDOW_CORNER_PREFERENCE)
///的 4 个取值. 老系统 (Win10 / Win7) 调用静默失败, 本枚举值与 DWM
///原生 DWM_WINDOW_CORNER_PREFERENCE 枚举二进制一致 (0/1/2/3).
//@别名 原生圆角预设
enum xblur_corner_
{
	//@别名 原生圆角_默认
	xblur_corner_default     = 0,   // DWMWCP_DEFAULT     由 DWM 决定 (顶层窗 ≈ round)
	//@别名 原生圆角_不圆角
	xblur_corner_donotround  = 1,   // DWMWCP_DONOTROUND  强制直角
	//@别名 原生圆角_圆角
	xblur_corner_round       = 2,   // DWMWCP_ROUND       8 px (Win11 窗体默认)
	//@别名 原生圆角_小圆角
	xblur_corner_roundsmall  = 3,   // DWMWCP_ROUNDSMALL  4 px (菜单 / popup)
};

///主题预设默认参数 (CXBlur::SetThemeDefault / GetThemeDefault)
///v3.0 DWM acrylic 路径下只保留 tint + noise (两个应用可控参数).
///blur 强度/饱和/亮度/对比度 都由 DWM 系统决定, 应用层不控。
//@别名 模糊主题默认参数
struct CXBlurThemeDefaults
{
	//@别名 叠加色 ()
	COLORREF tintColor;
	//@别名 噪点
	float    noise;
};

///绑定模式 (CXBlur::GetBindMode 返回值)
//@别名 模糊绑定模式
enum xblur_bind_
{
	//@别名 模糊绑定_未绑定
	xblur_bind_none      = 0,
	//@别名 模糊绑定_自有元素
	xblur_bind_owned     = 1,
	//@别名 模糊绑定_附加元素
	xblur_bind_attach    = 2,
	//@别名 模糊绑定_附加窗口
	xblur_bind_window    = 3,
};

//@分组{ 亚克力模糊元素
//@备注  继承: CXEle, CXWidgetUI, CXObjectUI, CXBase. 走 Windows DWM 系统级 acrylic.
//@别名  炫彩亚克力模糊类
class CXBlur : public CXEle
{
public:
	//@隐藏{
	CXBlur();
	virtual ~CXBlur();
	virtual HXCGUI GetHXCGUI() override { return m_hEle; }
	operator HELE() const { return m_hEle; }
	operator HXCGUI(){ return m_hEle; }
	virtual void operator=(const HELE hEle) override;
	//@隐藏}

//@备注 创建模糊元素 (自有). 自动启用所在窗口的 DWM acrylic.
//@参数 x 元素x坐标 (相对父元素客户区).
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄.
//@别名  创建()
	HELE Create(int x, int y, int cx, int cy, HXCGUI hParent = NULL);

//@备注 创建模糊元素 (构造函数版本).
	CXBlur(int x, int y, int cx, int cy, HXCGUI hParent = NULL){ Create(x, y, cx, cy, hParent); }

//@备注 把模糊效果 *附加* 到一个已存在的元素. 启用所在窗口的 DWM acrylic.
//      副作用 (Detach 时还原):
//        1) 强制 XEle_EnableBkTransparent(hUserEle, TRUE).
//        2) 注册 XE_PAINT 自绘 tint+border+圆角.
//@参数 hUserEle 用户已创建好的元素句柄.
//@返回 TRUE 成功, FALSE 句柄非法.
//@别名  附加元素()
	BOOL AttachToEle(HELE hUserEle);

//@备注 创建一个 *覆盖整个窗口客户区* 的模糊背板, 作为窗口级亚克力背景使用.
//@参数 hWnd 目标窗口.
//@返回 TRUE 成功, FALSE 句柄非法.
//@别名  附加窗口()
	BOOL AttachToWnd(HWINDOW hWnd);

//@备注 解除当前绑定. 还原元素的 EnableBkTransparent 状态.
//      *不会* 主动关闭窗口的 DWM acrylic, 因为同一窗口可能有多个 CXBlur 共享, 也可能
//      用户希望 CXBlur 销毁后窗口仍保持 acrylic 视觉. 用户需要彻底关 acrylic
//      请直接销毁窗口.
//@别名  解除绑定()
	void Detach();

//@备注 当前绑定模式 (xblur_bind_*).
//@别名  取绑定模式()
	int  GetBindMode() const;

	// ===== 着色叠加 =====
//@备注 设置 *叠加色* (Tint). 直接传给 DWM acrylic 的 GradientColor.
//      XCGUI RGBA 0xAABBGGRR 与 ACCENT_POLICY.GradientColor 编码完全对齐, 直接透传.
//@别名  置叠加色()
	void SetTintColor(COLORREF color);
//@别名  取叠加色()
	COLORREF GetTintColor() const;

//@备注 应用主题预设 (xblur_theme_*) 到当前实例.
//@别名  置主题()
	void SetTheme(int theme);
//@别名  取主题()
	int  GetTheme() const;

//@备注 全局主题: 设置后会同步到当前所有 CXBlur 实例, 之后新创建的实例
//      默认也使用此主题. 适用于"整个应用统一 acrylic 风格"的场景.
//      个别 element 仍可通过 SetTheme(...) 单独 override.
//@别名  置全局主题()
	static void SetGlobalTheme(int theme);
//@别名  取全局主题()
	static int  GetGlobalTheme();

//@备注 修改 light/dark 主题预设的默认参数. 后续 SetTheme(light/dark) 会用
//      你设的值而非硬编码默认. 用于"调好参数后保存, 避免每次启动重设".
//      theme 只接受 xblur_theme_light 或 xblur_theme_dark (其他忽略).
//      auto 主题运行时根据系统设置选 light/dark 调用此值.
//@别名  置主题默认参数()
	static void SetThemeDefault(int theme, const CXBlurThemeDefaults& d);

//@备注 读取 light/dark 主题预设当前默认参数. 配合 SetThemeDefault 可"读出
//      → 改单字段 → 写回"实现细粒度调整.
//@别名  取主题默认参数()
	static CXBlurThemeDefaults GetThemeDefault(int theme);

//@备注 设置噪点强度. 增加 acrylic 砂质感, 缓解纯色块感.
//      0 = 关闭, 1 = 满, 推荐 0 ~ 0.15. 默认 0.06.
//@别名  置噪点()
	void SetNoise(float amount);
//@别名  取噪点()
	float GetNoise() const;

//@备注 当前运行环境是否支持系统级 acrylic / blur (Vista+ 任意一档即视为支持).
//      不支持时 (Win8/8.1) CXBlur 仅画 tint+border+圆角.
//@别名  是否支持系统亚克力()
	static BOOL IsSystemAcrylicSupported();

//@备注 当前 CXBlur 实例的系统 acrylic / blur 是否成功启用.
//@别名  是否已启用系统亚克力()
	BOOL IsSystemAcrylicEnabled() const;

//@备注 启用 / 关闭 DWM 原生窗外阴影 (DwmExtendFrameIntoClientArea).
//
//      *绝大多数 XCGUI 用户不需要本接口* ——————————————————
//      XCGUI 默认窗 (CXWindow / window_style_default) 自带 WS_THICKFRAME,
//      DWM 已自动给它画 drop shadow + 1 px 描边. 这种窗只要调用
//      EnableNativeRoundedCorner 设圆角就够了, 阴影自来.
//
//      本接口存在的唯一场景: *纯 WS_POPUP* (无 WS_THICKFRAME / WS_CAPTION)
//      的 borderless 窗 — DWM 不会自动给它画 shadow, 必须靠 frame extension
//      显式触发.
//
//      *代价 (DwmExtendFrameIntoClientArea 的固有副作用)*:
//        1. frame extension 区域 (1 px 边带) 在 resize 瞬间会闪出窗类
//           hbrBackground 默认色 (XCGUI 通常是白色). 深色主题上肉眼可见.
//           cloak 机制只能盖首帧, resize 期偶发白闪改不掉.
//        2. snap state 下 DWM frame 强制方角, EnableNativeRoundedCorner
//           的 corner pref 此时无效 — 表现为 "snap 中方角 + 阴影". 这是
//           Win11 DWM by design (frame extension 与 corner pref 在 snap
//           state 互斥).
//
//      *自动行为* (host 状态机):
//        * 真最大化 (IsZoomed): margins=0 屏蔽 shadow, 还原后恢复.
//        * 启动白闪缓解: 调用时若窗未 visible, 自动 DWMWA_CLOAK 150 ms.
//
//      本接口与 CXBlur 实例生命周期无关 —— 静态 HWND 级, *不* 在 Detach
//      时撤销, 调用方须显式传 FALSE 还原.
//
//      返回 TRUE = DWM 接受, FALSE = 句柄非法 / DWM 未启用 / 老 OS 不支持.
//@参数 hWnd     XCGUI 窗口句柄.
//@参数 bEnable  TRUE 启用, FALSE 还原.
//@别名  启用原生阴影()
	static BOOL EnableNativeShadow(HWINDOW hWnd, BOOL bEnable);

//@备注 启用 Win11 DWM 原生窗体圆角 (DwmSetWindowAttribute /
//      DWMWA_WINDOW_CORNER_PREFERENCE = 33). 仅 Win11 21H2+ (build >= 22000)
//      生效, 老系统调用返 E_INVALIDARG, 本函数透传 FALSE.
//
//      圆角由 DWM 在合成级别处理, 不动 SetWindowRgn / 位图, 与
//      ACCENT_ACRYLIC 兼容. 与 EnableNativeShadow 解耦 (二者状态机共享但
//      可独立 set).
//
//      *推荐用法 (默认 XCGUI 窗)*:
//        XCGUI 默认窗带 WS_THICKFRAME, DWM 会 *自动* 给它画 drop shadow,
//        所以一般 *只调本接口* 即可: 圆角 + 自动阴影 + 描边都有, 无白闪.
//        *不要* 再调 EnableNativeShadow (那是给纯 popup 窗用的, 调了会
//        引入白闪 + snap 方角, 详见其文档).
//
//      *自动行为* (host 状态机):
//        * 真最大化 (IsZoomed): 强制 DWMWCP_DONOTROUND, 还原后恢复 user pref.
//        * WM_SIZE / WM_WINDOWPOSCHANGED / WM_DPICHANGED: 重 set corner
//          pref, 保险 DWM 在状态切换时仍按 user pref 渲染.
//
//      *已知限制 (Win11 by design, 引 MS 官方文档)*:
//        原文: "By design, apps are not rounded when maximized, snapped,
//              running in a Virtual Machine (VM), running on a Windows
//              Virtual Desktop (WVD), or running as a Windows Defender
//              Application Guard (WDAG) window."
//        参考: https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-rounded-corners
//
//        即:
//          * 最大化 → 方角 + 无 shadow (Edge / Settings / Notepad 一致).
//          * snap (Aero Snap / Snap Layouts) → 方角 + 无 shadow + 无描边.
//            这是 Win11 让 snap 窗紧贴屏幕边 / 邻窗的设计选择.
//          * VM / 远程桌面 → DWM 不参与渲染, 圆角自然无.
//        本接口对上述状态的 set 调用 *会* 成功 (返 S_OK), 但 DWM 不渲染.
//        无任何接口 / DWM 私有 attribute 能 override 这个 by-design 行为.
//        若必须在 snap 也保留圆角/阴影, 唯一办法是放弃 DWM 路径全程自绘
//        (per-pixel alpha layering + SetWindowRgn), 代价见 MS 文档.
//
//      本接口同样是静态 HWND 级, *不* 在 Detach 时还原.
//
//      返回 TRUE = Win11+ 且 DWM 接受, FALSE = Win10- / DWM 未启用 / 句柄非法.
//@参数 hWnd          XCGUI 窗口句柄.
//@参数 cornerStyle   xblur_corner_* 枚举值.
//@别名  启用原生圆角()
	static BOOL EnableNativeRoundedCorner(HWINDOW hWnd, int cornerStyle);

//@备注 启用 / 禁用本窗的系统 snap 行为. 默认 TRUE (启用, 与系统一致).
//
//      *动机*: Win11 snap 状态下 DWM by design 不画圆角 / 阴影 / 描边
//      (见 EnableNativeRoundedCorner 文档已知限制). 若你的 UI 不希望
//      切到 snap 状态破坏视觉, 可调本接口禁掉 snap.
//
//      *bEnable=FALSE 的实现策略 (Win 没有 per-window snap 关闭官方 API,
//      用三件套组合)*:
//        1. strip WS_MAXIMIZEBOX → 杀 Win11 Snap Layouts 飞出框 (悬停最大
//           化按钮的小窗格选择). *副作用: 最大化按钮变灰不可点.*
//        2. WM_WINDOWPOSCHANGING 几何过滤 → 检测目标矩形是否匹配 snap
//           layout (full / half / quarter), 是则设 SWP_NOMOVE | SWP_NOSIZE
//           阻止落位 (拦 Aero Snap 拖标题到屏幕边).
//        3. WM_SYSCOMMAND 吞 SC_MAXIMIZE → 拦键盘 Win+Up 最大化, 标题栏
//           双击最大化, 系统菜单 "最大化".
//
//      *副作用 / 限制*:
//        * 最大化按钮变灰. 本接口与 "用户期望最大化窗" 互斥, 二选一.
//        * snap 几何检测有 2 px 容差, 用户手动恰好 resize 到 1/2 屏 / 1/4
//          屏尺寸会被误拦. 概率极低 (要求 4 边都对齐 work area).
//        * 触摸板三指手势 / 屏幕投递的 snap 不走以上路径, 拦不住.
//          (Win 系统 hook, 接口层无法干预.)
//
//      *bEnable=TRUE (从 FALSE 切回)*: 还原 WS_MAXIMIZEBOX 到 strip 前状态
//      (若原本就没 MAXIMIZEBOX, 不改). 拦截过滤逻辑空转.
//
//      本接口同样是静态 HWND 级, 与 EnableNativeShadow / EnableNativeRoundedCorner
//      共用同一 hook + 状态机, *不* 在 Detach 时还原. 调用方须显式 EnableSnap(TRUE)
//      还原.
//
//      返回 TRUE = 设置成功, FALSE = 句柄非法.
//@参数 hWnd     XCGUI 窗口句柄.
//@参数 bEnable  TRUE 启用 (默认), FALSE 禁用所有 snap 入口.
//@别名  启用Snap()
	static BOOL EnableSnap(HWINDOW hWnd, BOOL bEnable);

//@备注 强制开启 / 关闭 *系统* 透明效果 (写 HKCU 注册表 EnableTransparency).
//      让 ACCENT_ACRYLIC 在用户关了"个性化-颜色-透明效果"时仍能出真 blur.
//
//      bForce = TRUE  → 保存老值, 写入 1, 广播本进程窗口, 注册退出还原.
//      bForce = FALSE → 还原老值, 广播本进程窗口 (若未启用则 no-op).
//
//      *重要权衡*:
//        * 这是 *用户级* 系统设置, 写下去整个用户帐户的所有 app 都开.
//          务必在 EULA / 启动提示里告知用户.
//        * Win7 / Win8 / 8.1 上没用 (它们没有 ACCENT_ACRYLIC).
//        * 进程正常退出 atexit / Ctrl-C / Ctrl-Break / Close 会自动还原;
//          被强杀 (任务管理器 / TerminateProcess / 蓝屏) 时还原不上.
//        * 老值已经是 1 时不写也不还原 (用户本来就开着).
//        * 重复调用幂等: 第一次 TRUE 真正写入并注册 atexit, 后续 TRUE 直接返回;
//          FALSE 仅在已 forced 时执行还原, 否则 no-op.
//        * 广播只发本进程顶层窗口 (避免打扰其他 app).
//        * 不需要管理员权限 (HKCU 当前用户就能写).
//
//      调用时机: 建议在 XInitXCGUI 之后, 创建第一个 CXBlur 之前.
//@参数 bForce  TRUE=开启, FALSE=还原. 默认 FALSE.
//@别名  强制开启系统透明效果()
	static void ForceSystemTransparencyOn(BOOL bForce = FALSE);

//@别名  立即刷新()
	void Invalidate();

	// ===== 圆角 / 边框 =====
//@备注 统一圆角半径. 四个角设为同一值. 单位 = 元素逻辑像素.
//@别名  置圆角()
	void SetCornerRadius(int radius);
//@备注 返回左上角圆角半径 (当四角不一致时返回左上角的值).
//@别名  取圆角()
	int  GetCornerRadius() const;

//@备注 分别设置四个角的圆角半径. 顺序按 CSS border-radius 标准:
//      左上 → 右上 → 右下 → 左下 (顺时针). 单位 = 元素逻辑像素.
//      传 0 表示该角不倒, 可与圆角混合实现"半圆角矩形" / 上圆下方 等效果.
//@参数 leftTop      左上角半径 (CSS 第 1 位)
//@参数 rightTop     右上角半径 (CSS 第 2 位)
//@参数 rightBottom  右下角半径 (CSS 第 3 位)
//@参数 leftBottom   左下角半径 (CSS 第 4 位)
//@别名  置圆角扩展()
	void SetCornerRadiusEx(int leftTop, int rightTop, int rightBottom, int leftBottom);

//@备注 读出四个角圆角半径.
//@别名  取圆角扩展()
	void GetCornerRadiusEx(int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom) const;

//@别名  置边框色()
	void SetBorderColor(COLORREF color);
//@别名  取边框色()
	COLORREF GetBorderColor() const;

//@别名  置边框宽()
	void SetBorderWidth(float w);
//@别名  取边框宽()
	float GetBorderWidth() const;

	//@隐藏{
private:
	// ===== 元素 / DPI =====
	bool   m_owned              = false;
	bool   m_attachToWindow     = false;
	HWINDOW m_attachedWnd       = NULL;
	bool   m_savedBkTransparent  = false;
	bool   m_hasSavedTransparent = false;
	float  m_dpiScale           = 1.0f;

	// ===== 模糊参数 (DWM 路径只控装饰层) =====
	std::atomic<COLORREF> m_tintColor    {0};
	std::atomic<int>      m_theme        {xblur_theme_custom};
	std::atomic<float>    m_noise        {0.06f};
	// 四角圆角半径: TL / TR / BR / BL (顺时针). 四值相等时走快路径
	// FillRoundedRectangle, 否则用 path geometry 拼 4 段弧.
	std::atomic<int>      m_cornerTL     {0};
	std::atomic<int>      m_cornerTR     {0};
	std::atomic<int>      m_cornerBR     {0};
	std::atomic<int>      m_cornerBL     {0};
	std::atomic<COLORREF> m_borderColor  {0};
	std::atomic<float>    m_borderWidth  {0.0f};

	// ===== host acrylic 状态 =====
	bool  m_acrylicApplied = false;   // host acrylic 是否成功启用
	HWND  m_hostHwnd       = NULL;    // host HWND (acrylic 生效的窗口)

public:
	// =================================================================
	// 内部回调入口 (公开仅为方便 C 风格事件回调调用; 用户代码请勿直接调用).
	// =================================================================
	int  OnPaintImpl   (HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int  OnSizeImpl    (HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled);
	int  OnDestroyImpl (HELE hEle, BOOL* pbHandled);
	int  OnWndSizeImpl (HWINDOW hWnd, UINT nFlags, SIZE* pSize, BOOL* pbHandled);
	int  OnWndSettingChangeImpl(HWINDOW hWnd, UINT uFlags, void* pStr, BOOL* pbHandled);

private:
	// =================================================================
	// 内部方法
	// =================================================================
	void AttachInternal(HELE hEle, bool owned);
	void DetachInternal();
	void HookEvents(HELE hEle);
	void UnhookEvents(HELE hEle);
	void RegisterWindowSizeHook(HWINDOW hWnd);
	void UnregisterWindowSizeHook();

	// 找到本元素所在的顶层 HWND.
	HWND FindHostHwnd() const;

	// 渲染分流: 只画 tint + noise + border + 圆角. host acrylic 背景由 DWM 提供.
	void OnPaintD2D(ID2D1RenderTarget* rt, HDRAW hDraw);
	void OnPaintGdi(HDC hdc, HDRAW hDraw);

	void RefreshDpiScale();
	void RedrawSelf();

	void ApplyThemePreset(int theme);
	static BOOL  IsSystemDarkMode();
	//@隐藏}
};
//@分组}

#endif // XCGUI_BLUR_H
