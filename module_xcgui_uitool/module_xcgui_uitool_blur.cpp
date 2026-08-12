//============================================================================
// module_xcgui_uitool_blur.cpp — CXBlur split (原 module_xcgui_blur.cpp 纯搬运)
//
// CXBlur v3.0 - 系统 acrylic / blur 元素实现.
//
// 设计原则:
//   1. attach 时给 host HWND 启用系统 backdrop blur, 由 DWM 在合成 pipeline
//      内做 backdrop + blur, 永远跟手, 0 延迟.
//   2. CXBlur element 自身只画 *装饰层* (tint + noise + 圆角 + 边框),
//      其他 XCGUI element 可以正常覆盖在它之上 (因为整个 CXBlur 绘制
//      都是 host paint content 的一部分, 不是 child HWND).
//   3. 同一 host 多个 CXBlur 共享: 引用计数, 第一个 attach 启用, 最后一个
//      detach 关闭.
//
// XCGUI Init 模式适配:
//   XInitXCGUI(0) GDI+ 路径 → OnPaintGdi (tint + border, 不画 noise)
//   XInitXCGUI(1) D2D 路径  → OnPaintD2D (tint + noise + border + 圆角)
//
// 系统 acrylic 启用路径 (运行时按 OS build number 显式选, 见 XBlur_PickPath):
//   1. Win11 (>= 22000)               : ACCENT_ENABLE_ACRYLICBLURBEHIND (真 acrylic)
//   2. Win10 1803~1809 (17134~17763)  : ACCENT_ENABLE_ACRYLICBLURBEHIND (真 acrylic)
//   3. Win10 1903~22H2 (18362~21999)  : ACCENT_ENABLE_BLURBEHIND        (绕开 ACRYLIC 阉割)
//   4. Win10 1607~1709 (14393~16299)  : ACCENT_ENABLE_BLURBEHIND        (Win10 风 blur)
//   5. Win7 / Win8 / 8.1 / 老 Win10   : 不启用 backdrop blur, CXBlur 退化为"仅装饰".
//
// dcomp 路径 (Win10 1803+, AttachToWndEx / XBLUR_FORCE_DCOMP):
//   PoC 视觉对齐 Win11 Start Menu. 完整集成走 owner-owned acrylic 子窗
//   (module_xcgui_blur_dcomp.cpp). 普通 AttachToWnd 仍走 ACCENT 路径;
//   env XBLUR_FORCE_DCOMP=1 可在 host 窗上强制 dcomp 实验.
//
// Win10 1903 起 ACRYLIC 被微软阉割: SetWindowCompositionAttribute 仍返 TRUE 但
// DWM 不再跑 blur kernel, 只剩透明+tint 且 resize 拉胯, Win10 22H2 仍未修.
// 不能用 try-ACRYLIC-then-BLURBEHIND 试探 (会成功但不出 blur, 试探发现不了),
// 必须按 build number 显式选.
//
// Win7 / Win8 / 8.1 不走 DwmEnableBlurBehindWindow + BLURREGION:
//   BLURREGION blur 生效要 host pixel alpha=0, XCGUI 渲染 pipeline
//   输出 alpha=255 → 出不了 blur. 变成仅装饰层 (tint+border).
//============================================================================

#include "module_xcgui_blur.h"
#include "module_xcgui_blur_dcomp.h"  // Win10 1803+ dcomp+WUC 直接合成路径 (XBLUR_PATH_DCOMP_WINRT)

#include <dwmapi.h>
#include <algorithm>
#include <map>
#include <set>
#include <mutex>
#include <vector>
#include <atomic>

#pragma comment(lib, "dwmapi.lib")

//============================================================================
// 工具函数
//============================================================================
static inline float Clampf(float v, float lo, float hi){
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

// 半透明 / acrylic 背景下 ClearType 与 alpha 混合异常, 需灰度抗锯齿 (mode=2).
// 必须在 XInitXCGUI 之后、host acrylic 实际生效后再设; 仅放构造函数会被
// XCGUI 初始化或后续绘制流程覆盖, 表现为"要手动再调一次才生效".
static void XBlur_EnsureGrayscaleTextAA(){
	if (XC_IsEnableD2D())
		XC_SetD2dTextAntialiasMode(2);
}

#ifndef SafeRelease
#define SafeRelease(p) do { if (p) { (p)->Release(); (p) = NULL; } } while (0)
#endif

//============================================================================
// 圆角路径构造 (per-corner): 四角不同时走 PathGeometry, 全相等走快路径.
//============================================================================
// 调用方拿走 ID2D1Geometry* 后自己 Release. 失败返回 NULL.
static ID2D1Geometry* XBlur_CreateCornerGeometry(
	ID2D1Factory* fac, const D2D1_RECT_F& rc,
	int tl, int tr, int br, int bl)
{
	if (!fac) return NULL;

	// 全 0: RectangleGeometry
	if (tl == 0 && tr == 0 && br == 0 && bl == 0){
		ID2D1RectangleGeometry* g = NULL;
		fac->CreateRectangleGeometry(rc, &g);
		return g;
	}
	// 四角一致: RoundedRectangleGeometry 快路径
	if (tl == tr && tr == br && br == bl){
		ID2D1RoundedRectangleGeometry* rr = NULL;
		D2D1_ROUNDED_RECT desc = { rc, (FLOAT)tl, (FLOAT)tl };
		fac->CreateRoundedRectangleGeometry(desc, &rr);
		return rr;
	}
	// 四角混合: PathGeometry 拼 4 段弧 + 4 段边.
	ID2D1PathGeometry* path = NULL;
	if (FAILED(fac->CreatePathGeometry(&path)) || !path) return NULL;
	ID2D1GeometrySink* sink = NULL;
	if (FAILED(path->Open(&sink)) || !sink){ path->Release(); return NULL; }

	sink->BeginFigure(D2D1::Point2F(rc.left + tl, rc.top),
	                  D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(D2D1::Point2F(rc.right - tr, rc.top));
	if (tr > 0){
		D2D1_ARC_SEGMENT a = {};
		a.point = D2D1::Point2F(rc.right, rc.top + tr);
		a.size  = D2D1::SizeF((FLOAT)tr, (FLOAT)tr);
		a.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
		a.arcSize = D2D1_ARC_SIZE_SMALL;
		sink->AddArc(a);
	}
	sink->AddLine(D2D1::Point2F(rc.right, rc.bottom - br));
	if (br > 0){
		D2D1_ARC_SEGMENT a = {};
		a.point = D2D1::Point2F(rc.right - br, rc.bottom);
		a.size  = D2D1::SizeF((FLOAT)br, (FLOAT)br);
		a.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
		a.arcSize = D2D1_ARC_SIZE_SMALL;
		sink->AddArc(a);
	}
	sink->AddLine(D2D1::Point2F(rc.left + bl, rc.bottom));
	if (bl > 0){
		D2D1_ARC_SEGMENT a = {};
		a.point = D2D1::Point2F(rc.left, rc.bottom - bl);
		a.size  = D2D1::SizeF((FLOAT)bl, (FLOAT)bl);
		a.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
		a.arcSize = D2D1_ARC_SIZE_SMALL;
		sink->AddArc(a);
	}
	sink->AddLine(D2D1::Point2F(rc.left, rc.top + tl));
	if (tl > 0){
		D2D1_ARC_SEGMENT a = {};
		a.point = D2D1::Point2F(rc.left + tl, rc.top);
		a.size  = D2D1::SizeF((FLOAT)tl, (FLOAT)tl);
		a.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
		a.arcSize = D2D1_ARC_SIZE_SMALL;
		sink->AddArc(a);
	}
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	sink->Release();
	return path;
}

// GDI+ 版: 直接往 path 里写, 调用方负责 path 生命周期.
// 起点 (x+tl, y), 顺时针绕一圈, CloseFigure 闭合.
static void XBlur_BuildGdiCornerPath(Gdiplus::GraphicsPath& path,
	float x, float y, float w, float h,
	int tl, int tr, int br, int bl)
{
	using Gdiplus::REAL;
	path.Reset();
	// 顶边
	path.AddLine((REAL)(x + tl), (REAL)y, (REAL)(x + w - tr), (REAL)y);
	if (tr > 0){
		path.AddArc((REAL)(x + w - tr*2), (REAL)y,
		            (REAL)(tr*2), (REAL)(tr*2), 270, 90);
	}
	// 右边
	path.AddLine((REAL)(x + w), (REAL)(y + tr),
	             (REAL)(x + w), (REAL)(y + h - br));
	if (br > 0){
		path.AddArc((REAL)(x + w - br*2), (REAL)(y + h - br*2),
		            (REAL)(br*2), (REAL)(br*2), 0, 90);
	}
	// 底边
	path.AddLine((REAL)(x + w - br), (REAL)(y + h),
	             (REAL)(x + bl),     (REAL)(y + h));
	if (bl > 0){
		path.AddArc((REAL)x, (REAL)(y + h - bl*2),
		            (REAL)(bl*2), (REAL)(bl*2), 90, 90);
	}
	// 左边
	path.AddLine((REAL)x, (REAL)(y + h - bl),
	             (REAL)x, (REAL)(y + tl));
	if (tl > 0){
		path.AddArc((REAL)x, (REAL)y, (REAL)(tl*2), (REAL)(tl*2), 180, 90);
	}
	path.CloseFigure();
}

//============================================================================
// Host acrylic backdrop blur (element-level 通过 alpha 控制实现)
//============================================================================
// 用 SetWindowCompositionAttribute(ACCENT_ENABLE_ACRYLICBLURBEHIND) 让 DWM
// 给整个 host 应用 acrylic 后景 (backdrop + blur). element-level 视觉通过
// host paint 区域的 alpha 控制实现:
//   host GDI 默认 alpha=255  → 覆盖 acrylic → 显示 host 内容
//   element 区域用 COPY blend 写 alpha=tint.alpha (~51) → 透出 acrylic
// Win10/11 "InAppAcrylic" 实现; user32 导出但无 MSDN, 字段多年稳定.

// undocumented ACCENT API (user32.dll). 字段定义稳定多年.
enum _XBlur_ACCENT_STATE {
	XBLUR_ACCENT_DISABLED                   = 0,
	XBLUR_ACCENT_ENABLE_BLURBEHIND          = 3,  // Win10 风格 blur (无 tint)
	XBLUR_ACCENT_ENABLE_ACRYLICBLURBEHIND   = 4,  // Win10 1803~1809 风格 acrylic
};
struct _XBlur_ACCENT_POLICY {
	DWORD AccentState;
	DWORD AccentFlags;
	DWORD GradientColor;   // 0xAABBGGRR (跟 XCGUI RGBA 编码一致)
	DWORD AnimationId;
};
enum _XBlur_WINDOWCOMPOSITIONATTRIB {
	XBLUR_WCA_ACCENT_POLICY = 19,
};
struct _XBlur_WCA_DATA {
	_XBlur_WINDOWCOMPOSITIONATTRIB Attribute;
	PVOID  pvData;
	SIZE_T cbData;
};
typedef BOOL (WINAPI* PFN_SetWindowCompositionAttribute)(HWND, _XBlur_WCA_DATA*);

// === Win11 22H2+ DWM 系统亚克力 (DwmSetWindowAttribute) ===
// dwmapi.h 在 Win11 SDK (22621+) 才定义这两个枚举, 这里手动声明保证旧 SDK
// 也能编译. 数值与官方一致.
//   https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwm_systembackdrop_type
//   https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
enum _XBlur_DWM_SYSTEMBACKDROP_TYPE {
	XBLUR_DWMSBT_AUTO              = 0,
	XBLUR_DWMSBT_NONE              = 1,
	XBLUR_DWMSBT_MAINWINDOW        = 2,   // Mica
	XBLUR_DWMSBT_TRANSIENTWINDOW   = 3,   // Acrylic (Win11 Start Menu / Flyout)
	XBLUR_DWMSBT_TABBEDWINDOW      = 4,   // Mica Alt
};
static const DWORD kXBlur_DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
static const DWORD kXBlur_DWMWA_SYSTEMBACKDROP_TYPE     = 38;
static const DWORD kXBlur_DWMWA_CAPTION_COLOR           = 35;
static const DWORD kXBlur_DWMWA_WINDOW_CORNER_PREFERENCE  = 33;
static const COLORREF kXBlur_DWMWA_COLOR_NONE           = 0xFFFFFFFE;

// DWM_TRANSIENT (DWMWA_SYSTEMBACKDROP_TYPE) 对 XCGUI redirection bitmap 窗无效,
// PickPath 永不选中. 保留枚举与实现供未来架构; 编译时默认关闭.
#ifndef XBLUR_ENABLE_DWM_TRANSIENT
#define XBLUR_ENABLE_DWM_TRANSIENT 0
#endif

// === CXBlur 内部路径选择 ===
// 把 OS 能力差异映射成 4 条互斥的渲染路径. attach 时按 OS build 选一条,
// runtime 不会切换 (除非 OS 不变换路径不变).
//
//   DECORATIVE       老 OS / 不支持 backdrop blur. 只画 tint+border 装饰层.
//   ACCENT_BLURBEHIND  Win10 1607~1709 + Win10 1903~22H2 (绕开阉割). 透明 + DWM 轻 blur,
//                      没有 tint, 完全靠 element 端 COPY blend 涂 tint.
//   ACCENT_ACRYLIC   Win10 1803~1809 + Win11 21H2 (22000~22620). DWM 自带 tint
//                      (走 GradientColor), element 端不再硬涂.
//   DWM_TRANSIENT    Win11 22H2+ (build >= 22621). DwmSetWindowAttribute 路径,
//                      DWM 内部跑完整 WinUI Acrylic 配方 (Blur + LuminosityBlend +
//                      ColorBlend + Noise), 给的就是 Win11 Start Menu 同款效果.
//                      Element 端不画 tint (DWM 自带), 仅可选作软叠加.
enum _XBlur_PathKind {
	XBLUR_PATH_DECORATIVE      = 0,
	XBLUR_PATH_ACCENT_BLURBEHIND,
	XBLUR_PATH_ACCENT_ACRYLIC,
	XBLUR_PATH_DWM_TRANSIENT,
	// Win10 1803+ 走 DComp + Windows.UI.Composition 自己组 effect chain
	// (Blur + Saturation + LuminosityBlend + Noise.MULTIPLY), 不再依赖
	// SetWindowCompositionAttribute. 视觉对齐 Win11 Start Menu, 不受 Win10 1903
	// ACRYLIC 阉割影响. 实现见 module_xcgui_blur_dcomp.cpp.
	XBLUR_PATH_DCOMP_WINRT,
};

static void XBlur_ResolveDcompEffectArgs(xuitool_theme_ themeIn, COLORREF userTint, float userBlurOpacity,
                                          int uniformBrightnessIn, float userNoise,
                                          int& tintR, int& tintG, int& tintB, int& tintA,
                                          float& blurOpacity, float& saturation,
                                          BOOL& uniformBrightness, float& noiseAlphaPct){
	bool dark;
	if      (themeIn == xuitool_theme_light) dark = false;
	else if (themeIn == xuitool_theme_dark)  dark = true;
	else                                    dark = XUITool_IsSystemDarkMode() != FALSE;

	if (userTint != 0){
		tintR = (int)GetRValue(userTint);
		tintG = (int)GetGValue(userTint);
		tintB = (int)GetBValue(userTint);
		tintA = (int)GetAValue(userTint);
	} else {
		if (dark){ tintR = 32;  tintG = 32;  tintB = 32;  tintA = 255; }
		else     { tintR = 243; tintG = 243; tintB = 243; tintA = 255; }
	}

	// 通透感默认值按新配方 (dcomp 内部 ResolveAcrylicRecipe) 重新校准:
	//   light 0.20 → tint 层 ~39% + 亮度锁定 0.85
	//   dark  0.15 → tint 层 ~73% + 亮度锁定 0.90
	// 两者的亮度锁定都在 0.85 以上, 深浅主题下都不会被桌面大面积明暗拖出割裂感;
	// tint 层不占满, 桌面色度才透得出来 = Win11 开始菜单观感.
	blurOpacity = (userBlurOpacity >= 0.0f) ? userBlurOpacity
	                                        : (dark ? 0.15f : 0.20f);
	saturation = dark ? 1.2f : 1.3f;

	if (userNoise > 0.0f && userNoise <= 1.0f && userNoise != 0.06f){
		noiseAlphaPct = userNoise * 100.0f;
	} else {
		noiseAlphaPct = dark ? 2.1f : 1.0f;
	}
	uniformBrightness = uniformBrightnessIn ? TRUE : FALSE;
}

namespace {
//
// Win10 1903 (build 18362) 起微软 *阉割* 了 ACCENT_ENABLE_ACRYLICBLURBEHIND:
// SetWindowCompositionAttribute 仍返 TRUE, 但 DWM 不再跑 blur kernel, 只剩
// 透明 + tint, 而且 resize/move 时窗口刷新有 ~500ms~1s 延迟. 这是系统级
// regression, Win10 22H2 仍未修. 修复策略: 该 build 段降级到 BLURBEHIND
// (state=3, Win10 Aero 风格 blur), 它在 22H2 上仍能跑真 blur 且无明显延迟.
//
// *为什么不用 DWMWA_SYSTEMBACKDROP_TYPE (Win11 22H2+ 的官方亚克力 API)*:
// 实测 (Win11 24H2 build 26100) 该 API 对 *XCGUI 这种带 redirection bitmap
// 的 GDI/D2D 窗* 静默 fallback 到纯色, 不跑 acrylic 合成. DWMSBT 只服务于
// 走 DComp visual tree 的 WS_EX_NOREDIRECTIONBITMAP 窗 (WinUI3 / 现代 UWP).
// XCGUI 走 GDI 渲染管线, 改架构成本巨大且会破坏现有用户代码. 故所有 Win11
// 段都 fallback 到 ACCENT_ENABLE_ACRYLICBLURBEHIND — 视觉对齐 Win11 Start Menu
// 不到 100% (走的是 Win10 RS4 老配方, 缺 LuminosityBlend), 但有真 blur, 远
// 优于纯色死灰. 见 dbgview 诊断记录.
//   保留的 XBLUR_PATH_DWM_TRANSIENT 枚举只为后续 XCGUI 改架构 (DComp interop
//   或 WS_EX_NOREDIRECTIONBITMAP 子窗叠加) 时复用, 当前不会被 PickPath 选中.
//
// build 表 (优先级从高到低):
//   >= 22000           Win11 全部       → ACCENT_ACRYLIC (Win11 上 ACRYLIC 仍跑真 blur)
//   17134 ~ 17763      Win10 1803~1809  → ACCENT_ACRYLIC (真 acrylic 还在)
//   18362 ~ 21999      Win10 1903~22H2  → ACCENT_BLURBEHIND (绕开 ACRYLIC 阉割)
//   14393 ~ 16299      Win10 1607~1709  → ACCENT_BLURBEHIND (无 acrylic)
//   < 14393            Win10 < 1607 / Win8 / 8.1 / Win7 → DECORATIVE
static int XBlur_PickPath(){
	DWORD b = XUITool_GetOsBuild();

	// env XBLUR_FORCE_DCOMP=1: 回归测试用, 强制 AttachToEle/Wnd 走 dcomp 路径.
	// 生产 AttachToWndEx 走独立 dcomp acrylic owner 子窗架构, 不经过 PickPath.
	wchar_t buf[16] = {};
	DWORD got = ::GetEnvironmentVariableW(L"XBLUR_FORCE_DCOMP", buf, 15);
	bool forceDcomp = (got > 0 && buf[0] == L'1');
	if (forceDcomp && b >= 17134 && XBlurDComp::IsSupported()){
		return XBLUR_PATH_DCOMP_WINRT;
	}

	if (b >= 22000)              return XBLUR_PATH_ACCENT_ACRYLIC;     // Win11 全部
	if (b >= 17134 && b <= 17763) return XBLUR_PATH_ACCENT_ACRYLIC;    // Win10 1803~1809
	if (b >= 18362)              return XBLUR_PATH_ACCENT_BLURBEHIND;  // 1903 ~ 22H2
	if (b >= 14393)              return XBLUR_PATH_ACCENT_BLURBEHIND;  // 1607 ~ 1709
	return XBLUR_PATH_DECORATIVE;
}

// host HWND 状态: 订阅的 element + hook 前的 WndProc + 标志位.
struct _XBlur_HostBlurState {
	std::set<HELE> subs;
	WNDPROC origWndProc  = NULL;
	bool    enabled      = false;
	bool    destroying   = false;
	bool    pendingApply = false;  // 待首次 WM_PAINT 后再装 accent (冷启动防 flash)
	int     activePath   = XBLUR_PATH_DECORATIVE;  // 当前生效路径 (paint 端读)
	COLORREF activeTint  = 0;       // ACCENT_ACRYLIC 路径已传给 DWM 的 GradientColor
	bool    darkMode     = false;   // DWM_TRANSIENT 路径已设的 immersive dark mode
};
static std::mutex                                g_hostBlurMutex;
static std::map<HWND, _XBlur_HostBlurState>      g_hostBlurMap;

// 暴露给 paint 端: 查询某 HWND 当前生效路径 (paint 时可分支不同 alpha 写法).
static int XBlur_GetActivePath(HWND host){
	if (!host) return XBLUR_PATH_DECORATIVE;
	std::lock_guard<std::mutex> lk(g_hostBlurMutex);
	auto it = g_hostBlurMap.find(host);
	if (it == g_hostBlurMap.end()) return XBLUR_PATH_DECORATIVE;
	return it->second.activePath;
}

// 前向声明: 子类 WndProc 后置阶段要回调它.
static void XBlur_ApplyHostBlur_Locked(HWND host);

// 自定义消息: 子类把延迟装 accent 的请求 PostMessage 给自己, 在下一拍消息泵里处理.
// 走 WM_APP 偏移让原 WndProc 不会去处理它.
static constexpr UINT XBLUR_WM_APPLY_ACCENT = WM_APP + 0x4321;

// subclass WndProc:
//   WM_ERASEBKGND        : 返回 1 阻止 GDI 擦背景 (否则盖掉 acrylic).
//   WM_PAINT             : 调 orig 让 XCGUI 完成首帧 paint, 之后若 pendingApply
//                          挂着则 PostMessage(XBLUR_WM_APPLY_ACCENT) 延一拍装 accent.
//                          此时 backbuffer 已被 XCGUI 填成 opaque BG + element 区
//                          COPY-blend tint, DWM 下一帧 composite 直接出正确画面,
//                          跳过"整窗 alpha=0 全 backdrop"那一闪.
//   XBLUR_WM_APPLY_ACCENT: 执行 ApplyHostBlur_Locked.
//   WM_NCDESTROY         : 标 destroying, 让 release 跳过 disable accent.
//
// 注: 不拦 WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE 卸 accent. 之前在 Win10 22H2 上
// 拖拽 ~1s 延迟是因为旧代码用了被微软阉割的 ACCENT_ACRYLIC, 切到 BLURBEHIND
// 之后 DWM 走的是另一条 (轻量) 合成路径, 拖拽帧率正常, 不需要这个开销大且会
// 导致拖拽期间显示纯色的 mitigation.
static LRESULT CALLBACK XBlur_HostSubclassWndProc(HWND h, UINT m, WPARAM w, LPARAM l){
	if (m == WM_ERASEBKGND){
		return 1;
	}
	if (m == WM_NCDESTROY){
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(h);
		if (it != g_hostBlurMap.end()){
			it->second.destroying = true;
		}
	}
	if (m == XBLUR_WM_APPLY_ACCENT){
		// 收集需要重画的 element 列表, 锁外发 XEle_Redraw 防止重入.
		std::vector<HELE> needRedraw;
		{
			std::lock_guard<std::mutex> lk(g_hostBlurMutex);
			auto it = g_hostBlurMap.find(h);
			if (it != g_hostBlurMap.end() && !it->second.destroying){
				int oldPath = it->second.activePath;
				XBlur_ApplyHostBlur_Locked(h);
				// 路径切换 (DECORATIVE → 真实路径) 必须重画, 否则首帧画的 SOURCE_OVER
				// 装饰层永远盖在 element 上, 用户看不到 acrylic.
				if (oldPath != it->second.activePath){
					needRedraw.assign(it->second.subs.begin(),
					                  it->second.subs.end());
				}
			}
		}
		for (HELE e : needRedraw){
			if (XC_IsHELE((HXCGUI)e)) XEle_Redraw(e);
		}
		return 0;  // 自定义消息不透给 orig.
	}

	WNDPROC orig = NULL;
	{
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(h);
		if (it != g_hostBlurMap.end()) orig = it->second.origWndProc;
	}
	LRESULT lr = orig ? CallWindowProcW(orig, h, m, w, l)
	                  : DefWindowProcW(h, m, w, l);

	// WM_PAINT 后置: 首帧 paint 完成 → 派发延迟装 accent 信号.
	// PostMessage 而非直接调 ApplyHostBlur_Locked, 留一拍给 swapchain Present
	// flush GPU, 防 DWM 拿到的还是上一帧空 backbuffer.
	if (m == WM_PAINT){
		bool needPost = false;
		{
			std::lock_guard<std::mutex> lk(g_hostBlurMutex);
			auto it = g_hostBlurMap.find(h);
			if (it != g_hostBlurMap.end() && it->second.pendingApply &&
			    !it->second.destroying){
				it->second.pendingApply = false;  // 只触发一次
				needPost = true;
			}
		}
		if (needPost){
			::PostMessageW(h, XBLUR_WM_APPLY_ACCENT, 0, 0);
		}
	}
	return lr;
}

// 解析 env var 里的整数 (支持 "0x80" 和 "128" 两种).
static bool XBlur_TryReadEnvDword(const wchar_t* name, DWORD* out){
	wchar_t buf[64] = {};
	DWORD got = ::GetEnvironmentVariableW(name, buf, 63);
	if (got == 0 || got >= 63) return false;
	wchar_t* end = nullptr;
	int base = 10;
	const wchar_t* s = buf;
	if (s[0] == L'0' && (s[1] == L'x' || s[1] == L'X')) { base = 16; s += 2; }
	unsigned long v = wcstoul(s, &end, base);
	if (end == s) return false;
	*out = (DWORD)v;
	return true;
}

// 调 SetWindowCompositionAttribute 给 host 装一个 ACCENT_STATE.
// state 由 XBlur_PickPath 决定; tintRgba 仅 ACRYLIC 用, BLURBEHIND 忽略.
// 调用方持 g_hostBlurMutex 锁 (修改 g_hostBlurMap 的路径).
//
// *诊断 env var* (运行时无需重编):
//   XBLUR_ACCENT_STATE=N   覆盖 AccentState (常见: 3=BLURBEHIND, 4=ACRYLIC, 5=HOSTBACKDROP)
//   XBLUR_ACCENT_FLAGS=N   覆盖 AccentFlags (未文档化位: 0x20/0x80/0x200 据传可切 luminosity 合成)
// 用 PowerShell: $env:XBLUR_ACCENT_FLAGS=0x20 ; & test_blur.exe ...
static bool XBlur_ApplyAccentBlur(HWND host, DWORD accentState, DWORD tintRgba){
	HMODULE u32 = ::GetModuleHandleW(L"user32.dll");
	if (!u32) return false;
	auto pSet = (PFN_SetWindowCompositionAttribute)
		::GetProcAddress(u32, "SetWindowCompositionAttribute");
	if (!pSet) return false;

	// 默认值
	DWORD finalState = accentState;
	DWORD finalFlags = 0;  // 注: ACCENT_FLAGS 2 会去掉 GradientColor (无 tint)

	// 诊断覆盖 (调试 luminosity 合成模式时用)
	DWORD ovState = 0, ovFlags = 0;
	if (XBlur_TryReadEnvDword(L"XBLUR_ACCENT_STATE", &ovState)) finalState = ovState;
	if (XBlur_TryReadEnvDword(L"XBLUR_ACCENT_FLAGS", &ovFlags)) finalFlags = ovFlags;

	_XBlur_ACCENT_POLICY policy = {};
	policy.AccentState   = finalState;
	policy.AccentFlags   = finalFlags;
	policy.GradientColor = tintRgba;  // 仅 ACRYLIC 用; BLURBEHIND 忽略
	policy.AnimationId   = 0;
	_XBlur_WCA_DATA data = {};
	data.Attribute = XBLUR_WCA_ACCENT_POLICY;
	data.pvData    = &policy;
	data.cbData    = sizeof(policy);
	BOOL ok = pSet(host, &data);
	return ok ? true : false;
}

// === Win11 22H2+ DWM 系统 acrylic 路径 (编译开关 XBLUR_ENABLE_DWM_TRANSIENT) ===
#if XBLUR_ENABLE_DWM_TRANSIENT
// DwmSetWindowAttribute(DWMWA_SYSTEMBACKDROP_TYPE, DWMSBT_TRANSIENTWINDOW) 让 DWM
// 内部跑完整 WinUI AcrylicBrush 配方. 真亚克力, 视觉对齐 Win11 Start Menu.
// *对 XCGUI redirection bitmap 窗静默 fallback*, 默认不启用.
static bool XBlur_ApplyDwmSystemBackdrop(HWND host, DWORD backdropType, bool dark){
	BOOL bDark = dark ? TRUE : FALSE;
	HRESULT hrDark = ::DwmSetWindowAttribute(host, kXBlur_DWMWA_USE_IMMERSIVE_DARK_MODE,
	                                          &bDark, sizeof(bDark));
	(void)hrDark;

	HRESULT hrFrame = S_OK;
	(void)hrFrame;

	COLORREF capColor = kXBlur_DWMWA_COLOR_NONE;
	HRESULT hrCap = ::DwmSetWindowAttribute(host, kXBlur_DWMWA_CAPTION_COLOR,
	                                         &capColor, sizeof(capColor));
	(void)hrCap;

	HRESULT hr = ::DwmSetWindowAttribute(host, kXBlur_DWMWA_SYSTEMBACKDROP_TYPE,
	                                     &backdropType, sizeof(backdropType));

	return SUCCEEDED(hr);
}

static void XBlur_DisableDwmSystemBackdrop(HWND host){
	DWORD none = XBLUR_DWMSBT_NONE;
	::DwmSetWindowAttribute(host, kXBlur_DWMWA_SYSTEMBACKDROP_TYPE,
	                        &none, sizeof(none));
	MARGINS mar0 = { 0, 0, 0, 0 };
	::DwmExtendFrameIntoClientArea(host, &mar0);
}
#endif // XBLUR_ENABLE_DWM_TRANSIENT

// 计算路径需要的 effective tint (从首个订阅元素读 m_tintColor).
// ACCENT_ACRYLIC 路径要把 tint 传给 DWM GradientColor 让 DWM 自己合成,
// 不再走 element 端 COPY blend (双层 tint 太死).
// 返回 0 表示无 tint (BLURBEHIND / DECORATIVE).
//
// 多 element 共享 host 时: 取迭代器首个 (std::set 按指针序), 视觉上等价于
// "随机选一个 sub element 的 tint 作 host tint". 用户用单 CXBlur::AttachToWnd
// 时只有一个 sub, 没歧义. 多 CXBlur::Create 在同窗时 *理论上* 第一个 attach
// 的占主导 — 这跟 ACCENT path 的 HWND 级 acrylic 设计相容 (HWND 只一份 backdrop).
static COLORREF XBlur_PickHostTint_Locked(const _XBlur_HostBlurState& s,
                                          int path)
{
	if (path == XBLUR_PATH_ACCENT_BLURBEHIND ||
	    path == XBLUR_PATH_DECORATIVE) return 0;
	if (s.subs.empty()) return 0;
	HELE hFirst = *s.subs.begin();
	if (!XC_IsHELE((HXCGUI)hFirst)) return 0;
	CXBlur* p = (CXBlur*)(intptr_t)XEle_GetUserData(hFirst);
	if (!p) return 0;
	return p->GetTintColor();
}

// 应用 host blur. 按 XBlur_PickPath 选的 path 执行.
// 不再"先 ACRYLIC 再 BLURBEHIND" 试探 — Win10 1903+ ACRYLIC 调用会成功返 TRUE
// 但不出 blur, 试探机制无法察觉, 必须按 build number 直接选.
static void XBlur_ApplyHostBlur_Locked(HWND host){
	auto it = g_hostBlurMap.find(host);
	if (it == g_hostBlurMap.end()) return;
	auto& s = it->second;

	// 没订阅者 → 关掉所有路径, 还原到默认.
	if (s.subs.empty()){
#if XBLUR_ENABLE_DWM_TRANSIENT
		if (s.activePath == XBLUR_PATH_DWM_TRANSIENT){
			XBlur_DisableDwmSystemBackdrop(host);
		}
#endif
		XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_DISABLED, 0);
		s.activePath = XBLUR_PATH_DECORATIVE;
		s.enabled    = false;
		return;
	}

	int path = XBlur_PickPath();
	s.activePath = path;
	s.enabled    = false;

	switch (path){
#if XBLUR_ENABLE_DWM_TRANSIENT
	case XBLUR_PATH_DWM_TRANSIENT: {
		XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_DISABLED, 0);
		bool dark = XUITool_IsSystemDarkMode() != FALSE;
		s.darkMode = dark;
		s.activeTint = 0;
		DWORD backdropType = XBLUR_DWMSBT_TRANSIENTWINDOW;
		wchar_t envBuf[16] = {};
		DWORD envLen = ::GetEnvironmentVariableW(L"XBLUR_BACKDROP_TYPE",
		                                          envBuf, _countof(envBuf));
		if (envLen > 0 && envLen < _countof(envBuf)){
			int v = _wtoi(envBuf);
			if (v >= 1 && v <= 4) backdropType = (DWORD)v;
		}
		s.enabled = XBlur_ApplyDwmSystemBackdrop(host, backdropType, dark);
		break;
	}
#endif
	case XBLUR_PATH_ACCENT_ACRYLIC: {
		// 把 element 期望的 tint 传给 DWM GradientColor, 让 DWM 自己合成 acrylic
		// (符合 Win10 1803~1809 / Win11 21H2 ACCENT_ACRYLIC 设计). element 端
		// paint 仍写 alpha (让 DWM 知道哪些区域要透), 但 *不再* 涂 tint 颜色.
		COLORREF tint = XBlur_PickHostTint_Locked(s, path);
		s.activeTint = tint;
		s.enabled = XBlur_ApplyAccentBlur(
			host, XBLUR_ACCENT_ENABLE_ACRYLICBLURBEHIND, (DWORD)tint);
		break;
	}
	case XBLUR_PATH_ACCENT_BLURBEHIND: {
		s.activeTint = 0;
		s.enabled = XBlur_ApplyAccentBlur(
			host, XBLUR_ACCENT_ENABLE_BLURBEHIND, 0);
		break;
	}
	case XBLUR_PATH_DCOMP_WINRT: {
		// =====================================================================
		// dcomp 路径 (Windows.UI.Composition + D2D effect chain).
		//
		// 跟 ACCENT/DWM 老路径互斥: 进 dcomp 前必须先 ApplyAccentBlur(DISABLED)
		// 关掉系统 backdrop, 否则 DWM 跟自合成 visual 会双层叠合, 视觉错位.
		//
		// element 端 paint 语义跟 ACCENT_ACRYLIC 一致 — OnPaintD2D 里 COPY 清
		// alpha=0, 让下方 dcomp visual 透出. tint 由 dcomp 内部 effect chain
		// 处理, *不* 在 element 端再叠一层.
		//
		// ---------------------------------------------------------------------
		// 默认值 (跟 Win11 Start Menu 对齐, 见 XBlur_ResolveDcompEffectArgs):
		//
		//   主题   | tint RGB        | 通透感 | saturation | noiseAlphaPct | uniformBright
		//   -------|-----------------|--------|------------|---------------|--------------
		//   浅色   | (243,243,243)   | 0.20   | 1.3        | 1%            | TRUE
		//   深色   | (32, 32, 32)    | 0.15   | 1.2        | 3%            | TRUE
		//
		// 通透感不是"tint / blur 二选一的比例", dcomp 内部按 WinUI AcrylicBrush
		// 配方拆成 tint 层不透明度 + 亮度锁定强度两个通道: 亮度统一由亮度锁定负责
		// (深色主题不会被桌面亮度顶穿), tint 层则不占满, 让桌面色度透出来.
		// blurOpacity < 0 (用户没设过) 时才按 tint.A 反算 (1 - A/255), 与
		// ACCENT_ACRYLIC GradientColor.A 语义保持兼容.
		//
		// 取值策略 (按字段):
		//   tint           : 用户 SetTintColor 传非 0 → 用用户值; m_tintColor==0
		//                    (用户没设过) → 套主题默认. 仅 dcomp 路径这么做,
		//                    其他路径 m_tintColor==0 时 *不画 tint* (老行为).
		//   uniformBright  : 走 m_uniformBrightness atomic, 默认 TRUE (= PoC 默认).
		//                    用户 SetUniformBrightness 可覆盖, dcomp 路径下立即
		//                    reapply effect chain.
		//   saturation     : 暂未暴露 setter, 永远走主题默认. 待 SetSaturation API
		//                    上线后从 m_saturation 读.
		//   noiseAlphaPct  : 同上, 待 SetNoiseAlphaPercent API 上线.
		//   inset          : 给 EnableNativeShadow 准备 (visual 收缩留阴影空间).
		//                    现在硬编 0; 后续接入 m_nativeShadowEnabled 后改成 8.
		// =====================================================================
		XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_DISABLED, 0);

		// 从首个 sub element (= 用户的 CXBlur 实例) 读用户显式设过的参数.
		// 多 element 共享 host 时取 std::set 迭代器首个 (指针序). 单 CXBlur::
		// AttachToWnd 用法下只有一个 sub, 无歧义.
		COLORREF userTint        = 0;
		BOOL     uniformBright   = TRUE;
		float    userBlurOpacity = -1.0f;  // -1 = 未设
		float    userNoise       = 0.06f;
		xuitool_theme_ userTheme = xuitool_theme_custom;
		if (!s.subs.empty()){
			HELE hFirst = *s.subs.begin();
			if (XC_IsHELE((HXCGUI)hFirst)){
				CXBlur* p = (CXBlur*)(intptr_t)XEle_GetUserData(hFirst);
				if (p){
					userTint        = p->GetTintColor();
					uniformBright   = p->GetUniformBrightness();
					userBlurOpacity = p->GetBlurOpacity();
					userNoise       = p->GetNoise();
					userTheme       = p->GetTheme();
				}
			}
		}

		// 用户显式 SetTheme(light/dark) → 强制对应 PoC 默认 (无视系统主题);
		// 否则 (auto/custom) 跟随系统 AppsUseLightTheme.
		bool dark;
		if (userTheme == xuitool_theme_light)      dark = false;
		else if (userTheme == xuitool_theme_dark)  dark = true;
		else                                     dark = XUITool_IsSystemDarkMode() != FALSE;
		s.darkMode = dark;

		int tintR, tintG, tintB, tintA;
		float blurOpacity, saturation, noiseAlphaPct;
		BOOL uniformBrightOut;
		XBlur_ResolveDcompEffectArgs(
			userTheme, userTint, userBlurOpacity,
			uniformBright ? 1 : 0, userNoise,
			tintR, tintG, tintB, tintA,
			blurOpacity, saturation, uniformBrightOut, noiseAlphaPct);
		uniformBright = uniformBrightOut;
		s.activeTint = RGBA(tintR, tintG, tintB, tintA);

		int inset = 0;

		s.enabled = XBlurDComp::Apply(
			host,
			tintR, tintG, tintB, tintA,
			blurOpacity,
			saturation, uniformBright, noiseAlphaPct, inset);
		break;
	}
	case XBLUR_PATH_DECORATIVE:
	default:
		// 老 OS (Win8/8.1/Win7) → 装饰层. 不调 SetWindowCompositionAttribute /
		// DwmSetWindowAttribute, element 端只画 tint+border.
		s.activeTint = 0;
		s.enabled    = false;
		break;
	}

	if (s.enabled)
		XBlur_EnsureGrayscaleTextAA();
}

// 加 sub element 到 host 订阅集合, 第一次启用 + subclass WndProc, 然后应用 accent.
//
// 冷启动 (host 还没 ShowWindow) → *不要*立即装 accent: DWM 会立刻把 host 标
// "per-pixel alpha 透 backdrop", 此时 backbuffer 还没被 XCGUI 写过 = alpha=0,
// ShowWindow 后第一帧 composite = 整窗 backdrop blur, 几十~两百 ms 后 XCGUI
// 首帧 paint 完成才"塌缩"成正确画面 → 视觉就是用户截图的整窗闪一下.
//
// 修复: 仅 subclass + 标 pendingApply, 真实 SetWindowCompositionAttribute
// 推迟到 host 首次 WM_PAINT 之后 + PostMessage 延一拍才做. 届时 backbuffer
// 已被 XCGUI 填成正确 alpha mask, DWM 第一次 composite 直接出对的画面.
//
// host 已可见: 跳过延迟逻辑, 直接装 — 后挂的 sub 早过了危险窗口.
static bool XBlur_AcquireHostBlur(HWND host, HELE hEle){
	if (!host || !::IsWindow(host) || !hEle) return false;
	std::lock_guard<std::mutex> lk(g_hostBlurMutex);
	_XBlur_HostBlurState& s = g_hostBlurMap[host];
	bool firstTime = s.subs.empty();
	s.subs.insert(hEle);
	if (firstTime){
		// subclass WndProc 处理 WM_ERASEBKGND / WM_PAINT 后置 / 自定义消息.
		s.origWndProc = (WNDPROC)::SetWindowLongPtrW(host, GWLP_WNDPROC,
			(LONG_PTR)XBlur_HostSubclassWndProc);

		if (!::IsWindowVisible(host)){
			// Cold open: 推迟装 accent. 乐观把 enabled 标 true 让 caller
			// m_acrylicApplied 一开始就反映"我打算装"; 真失败 (老 OS) 时
			// 首帧后 ApplyHostBlur_Locked 会回写 false.
			//
			// *不* 预测 activePath: 仍保留 DECORATIVE (SOURCE_OVER) 跑首帧, 让
			// 那一帧画面纯色不闪烁. 真实 apply 在 PostMessage(XBLUR_WM_APPLY_ACCENT)
			// 真实 apply 在 PostMessage(XBLUR_WM_APPLY_ACCENT) 触发后写 activePath
			// + 触发 element redraw, 第二帧起切到目标路径的 COPY 清 alpha 模式.
			s.pendingApply = true;
			s.enabled      = true;
			return true;
		}
	} else if (s.pendingApply){
		// 第二个 sub 在首帧前进来, 保持 deferred 计划, 不立刻装.
		return s.enabled;
	}
	XBlur_ApplyHostBlur_Locked(host);
	return s.enabled;
}

// 减 sub element 引用, 最后一个时关闭 blur + 还原 WndProc.
static void XBlur_ReleaseHostBlur(HWND host, HELE hEle){
	if (!host || !hEle) return;
	std::lock_guard<std::mutex> lk(g_hostBlurMutex);
	auto it = g_hostBlurMap.find(host);
	if (it == g_hostBlurMap.end()) return;
	it->second.subs.erase(hEle);
	if (it->second.subs.empty()){
		// 最后一个: host 还活着 → disable 全部 backdrop 路径 + 还原 WndProc.
		// host 销毁中 (destroying=true) → 跳过 disable, 让 DWM 自动清理,
		// 避免 SetWindowCompositionAttribute 在 destroy 路径触发画面定格.
		bool destroying = it->second.destroying;
		int  path       = it->second.activePath;
		if (!destroying && ::IsWindow(host)){
#if XBLUR_ENABLE_DWM_TRANSIENT
			if (path == XBLUR_PATH_DWM_TRANSIENT){
				XBlur_DisableDwmSystemBackdrop(host);
			}
#endif
			if (path == XBLUR_PATH_DCOMP_WINRT){
				XBlurDComp::Disable(host);
			}
			XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_DISABLED, 0);
			if (it->second.origWndProc){
				::SetWindowLongPtrW(host, GWLP_WNDPROC,
				                    (LONG_PTR)it->second.origWndProc);
			}
		} else if (destroying && path == XBLUR_PATH_DCOMP_WINRT){
			// host 销毁路径: ACCENT 是窗属性可让 DWM 自动清, 但 dcomp visual tree
			// 是我们自己创建的 COM 对象, 必须显式释放, 否则 leak (compositor 等).
			XBlurDComp::Disable(host);
		}
		g_hostBlurMap.erase(it);
	} else {
		// 还有其他 sub → 重新应用 acrylic.
		XBlur_ApplyHostBlur_Locked(host);
	}
}

}  // anonymous namespace

//============================================================================
// 全局实例 + 主题预设 (DWM acrylic 路径下需要)
//============================================================================
namespace {

// 全局活实例 + 全局主题 (CXBlur::SetGlobalTheme). attach/detach 维护.
static std::mutex            g_blurInstancesMutex;
static std::set<CXBlur*>     g_blurInstances;
static std::atomic<int>      g_blurGlobalTheme{xuitool_theme_custom};

// === 主题预设默认参数 (CXBlur::SetThemeDefault / GetThemeDefault) ===
// 用户调好后可 SetThemeDefault 保存, 后续 SetTheme(light/dark) 自动用此值.
// DWM 路径下只剩 tintColor + noise + cornerRadius + border* 有效, 其他字段
// 已废弃 (DWM acrylic 算法不让应用层控制 blurRadius/saturation/brightness/contrast).
//
// *默认 alpha 选值动机*:
//   - ACCENT_ACRYLIC (Win10 1803~1809 / Win11 全部): tintColor 直接传给 DWM
//     GradientColor, 由 DWM 的 acrylic 公式做"backdrop * (1-α) + tint * α"
//     blend. 太低 (< 30) DWM 几乎不出 tint 看着像纯透明, 太高 (> 150) 把
//     blur 盖死成纯色. Win11 Start Menu 的实际 tint 强度 light ≈ 64, dark ≈ 100.
//   - ACCENT_BLURBEHIND (Win10 1903~22H2): DWM 只 blur 不 tint, element 端
//     用 COPY blend 写 alpha+RGB. alpha 直接决定 backdrop 透出比例, 50~80
//     比较舒服. 用同一份 default 凑合, 用户嫌过重自行降.
//   - DECORATIVE (Win7/8/8.1): 没 backdrop, alpha 决定 tint 半透程度. 80~100
//     正合适. 同上凑合.
static std::mutex            g_themeDefaultsMutex;
static CXBlurThemeDefaults   g_lightDefaults = {
	RGBA(243, 243, 243, 64),   // Win11 light Start Menu 同款 (~25% alpha)
	0.06f
};
static CXBlurThemeDefaults   g_darkDefaults = {
	RGBA(32, 32, 32, 100),     // dark 需要更高 alpha (~39%)
	0.06f
};

}  // anonymous namespace

//============================================================================
// CXBlur 构造 / 析构
//============================================================================
CXBlur::CXBlur(){
	// DWM 接管 blur 强度/饱和/亮度/对比度, 我们只控 tint + noise + 边框等装饰层.
	// 默认 alpha=64 (~25%) 与 Win11 Start Menu light 主题同款 — 走 ACCENT_ACRYLIC
	// 路径时该值是传给 DWM GradientColor 的 tint 强度. SetTheme(light/dark) 之后
	// 改用主题 defaults.
	m_tintColor.store(RGBA(252, 253, 254, 64));    // Win11 light acrylic
	m_theme.store(xuitool_theme_custom);
	m_noise.store(0.06f);                              // 6% 灰度噪点纹理
	m_cornerTL.store(0);
	m_cornerTR.store(0);
	m_cornerBR.store(0);
	m_cornerBL.store(0);
	m_borderColor.store(0);
	m_borderWidth.store(0.0f);
}

CXBlur::~CXBlur(){
	if (m_attachedExDcomp){
		DetachExInternal();
	} else if (XC_IsHELE((HXCGUI)m_hEle)){
		DetachInternal();
	}
}

//============================================================================
// operator=  (IDE 风格的 m_hEle = 已有元素 自动转 Attach)
//============================================================================
void CXBlur::operator=(const HELE hEle){
	if (XC_IsHELE((HXCGUI)hEle)){
		AttachToEle(hEle);
	} else {
		Detach();
	}
}

//============================================================================
// Create
//============================================================================
HELE CXBlur::Create(int x, int y, int cx, int cy, HXCGUI hParent){
	if (XC_IsHELE((HXCGUI)m_hEle)){
		DetachInternal();
	}

	HELE hEle = XEle_Create(x, y, cx, cy, hParent);
	if (!hEle) return NULL;
	XUI_EnableCSS(hEle, FALSE);

	AttachInternal(hEle, true);
	return hEle;
}

//============================================================================
// AttachToEle
//============================================================================
BOOL CXBlur::AttachToEle(HELE hUserEle){
	if (!XC_IsHELE((HXCGUI)hUserEle)) {
		Detach();
		return FALSE;
	}
	if (m_hEle == hUserEle) return TRUE;
	if (XC_IsHELE((HXCGUI)m_hEle)){
		DetachInternal();
	}
	AttachInternal(hUserEle, false);
	return TRUE;
}

//============================================================================
// AttachToWnd
//============================================================================
BOOL CXBlur::AttachToWnd(HWINDOW hWnd){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;
	if (XC_IsHELE((HXCGUI)m_hEle)){
		DetachInternal();
	}

	// 备份 + 强制改两个窗体属性, 解决 CXBlur 元素跟 XCGUI 模拟客户区/边框冲突的视觉问题:
	//   * XWnd_EnableLayout(FALSE)
	//     关掉 XCGUI 自动布局. 不关时 CXBlur 这个 z=0 大背板元素会被 layout 当成
	//     普通 cell 参与流式排版, 跟用户后加的 layout 子元素互相挤位置 (能跑但
	//     在 layout=Vertical / Horizontal 容器下会把后续元素挤偏).
	//   * XWnd_EnableLayoutOverlayBorder(TRUE)
	//     让 layout 系统在 *模拟标题栏 + 模拟边框* 之上铺满 (overlay), 避免 CXBlur
	//     元素被强制收缩到 client - border 那块小矩形里, 出现用户截图里那种顶/左
	//     1-2px 漏边 (实际是 XCGUI 自绘边框).
	// Detach 时按 m_hasSavedWndLayout 还原 (XCGUI 无 overlayBorder getter, 假设
	// Detach 还原成 FALSE = XCGUI 默认).
	m_savedWndLayout       = (XWnd_IsEnableLayout(hWnd) != FALSE);
	m_savedWndOverlayBorder = false;  // XCGUI 默认状态, 无 getter
	m_hasSavedWndLayout    = true;
	XWnd_EnableLayout(hWnd, FALSE);
	XWnd_EnableLayoutOverlayBorder(hWnd, TRUE);

	RECT rcCli;
	XWnd_GetClientRect(hWnd, &rcCli);
	int cx = rcCli.right  - rcCli.left;
	int cy = rcCli.bottom - rcCli.top;

	HELE hEle = XEle_Create(0, 0, cx, cy, (HXCGUI)hWnd);
	if (!hEle) return FALSE;
	XUI_EnableCSS(hEle, FALSE);

	XEle_SetZOrder(hEle, 0);                          // Z 序最底 (索引 0)
	XWidget_SetID(hEle, 0xCB10A9DE);
	XEle_EnableBkTransparent(hEle, TRUE);
	// 注意: 鼠标穿透 (XEle_EnableMouseThrough) 不在此处强制启用.
	// 这是业务策略: 调用方可能希望背板捕获鼠标 (做菜单/弹窗 hit-test 起点),
	// 也可能希望穿透 (配合 XWnd_EnableDragWindow 让整窗可拖). 由用户自己决定.

	m_attachToWindow = true;
	m_attachedWnd    = hWnd;
	AttachInternal(hEle, true);

	RegisterWindowSizeHook(hWnd);
	return TRUE;
}

//============================================================================
// AttachToWndEx — 一键挂载 dcomp acrylic 主导架构.
//============================================================================
// 这是 PoC test_blur_main wWinMain 那个手写 acrylic 块的封装. 整套流程都在这调:
//   - XCGUI: SetTransparentType(shaped) + EnableDrawBk(FALSE)
//   - XBlurDComp::AttachAcrylicHost (内部建 NOREDIRECTIONBITMAP acrylic + Apply effect chain
//     + owner-owned + 2 个 subclass + ShowWindow)
//   - XCGUI: 第二次 SetTransparentType(shaped) + SetTransparentAlpha(255) +
//            SetBkInfo(1% 圆角填充) + EnableDragWindow(TRUE)
//   - DwmSetWindowAttribute(acrylic, ROUND) 加系统圆角 + BORDER + frame shadow
//
// 本接口跟 AttachToWnd 区别: AttachToWnd 走 ACCENT 路径 (在 XCGUI 客户区里
// 创建装饰 element). AttachToWndEx 走 dcomp acrylic owner 子窗 (XCGUI 整窗 layered 透明,
// 视觉浮层落到独立 acrylic 子窗). 两条路径互斥, 别混用.
//
// path 语义:
//   xblur_path_auto / dcomp → dcomp acrylic (Win10 1803+ 且 IsSupported)
//   xblur_path_dwm          → 显式降级 AttachToWnd ACCENT 路径
//
// 必须从本 TU (module_xcgui_blur.cpp) 调那 5 个 XCGUI API — dcomp.cpp TU 调它们高 DPI 下
// XCGUI 内部行为微妙不同, 视觉错位.

// 把当前实例参数折算成 XBlurDComp::Apply 所需的 8 个参数 (见 XBlur_ResolveDcompEffectArgs).
void CXBlur::ReapplyExEffects(){
	if (!m_attachedExDcomp) return;
	HWND acrylic = XBlurDComp::GetAcrylicHwnd((void*)m_attachedWnd);
	if (!acrylic || !::IsWindow(acrylic)) return;

	int tintR, tintG, tintB, tintA;
	float blurOpacity, saturation, noiseAlphaPct;
	BOOL uniformBrightness;
	XBlur_ResolveDcompEffectArgs(
		(xuitool_theme_)m_theme.load(), m_tintColor.load(), GetBlurOpacity(),
		m_uniformBrightness.load(), m_noise.load(),
		tintR, tintG, tintB, tintA,
		blurOpacity, saturation, uniformBrightness, noiseAlphaPct);
	XBlurDComp::Apply(acrylic, tintR, tintG, tintB, tintA,
	                  blurOpacity, saturation, uniformBrightness, noiseAlphaPct, 0);
}

BOOL CXBlur::AttachToWndEx(HWINDOW hWnd, int path){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;

	// 路径选择: auto/dcomp → dcomp acrylic; dwm 或 dcomp 不可用 → AttachToWnd ACCENT.
	bool wantDcomp = (path == xblur_path_auto || path == xblur_path_dcomp);
	if (path == xblur_path_dwm || !wantDcomp || !XBlurDComp::IsSupported()){
		return AttachToWnd(hWnd);
	}

	if (m_attachedExDcomp){
		DetachExInternal();
	} else if (XC_IsHELE((HXCGUI)m_hEle)){
		DetachInternal();
	}

	// === dcomp acrylic owner 路径 ===

	// 1. XCGUI 进 layered 透明 (在调 AttachAcrylicHost 之前).
	XWnd_SetTransparentType(hWnd, window_transparent_shaped);
	XWnd_EnableDrawBk(hWnd, FALSE);

	// 2. 主题预设 — 用户已经 SetTintColor/SetBlurOpacity/SetUniformBrightness/SetNoise
	//    显式设过的优先, 否则按主题默认 (PoC 校准).
	int tintR, tintG, tintB, tintA;
	float blurOpacity, saturation, noiseAlphaPct;
	BOOL uniformBright;
	XBlur_ResolveDcompEffectArgs(
		(xuitool_theme_)m_theme.load(), m_tintColor.load(), GetBlurOpacity(),
		m_uniformBrightness.load(), m_noise.load(),
		tintR, tintG, tintB, tintA,
		blurOpacity, saturation, uniformBright, noiseAlphaPct);

	// 3. 创建 acrylic + Apply effect chain + owner-owned + subclass + Show.
	HWND acrylic = XBlurDComp::AttachAcrylicHost((void*)hWnd,
		tintR, tintG, tintB, tintA,
		blurOpacity, saturation, uniformBright, noiseAlphaPct);
	if (!acrylic) return FALSE;

	// 4. XCGUI 整窗叠 1% 不透明度底色 — 视觉看不见但 layered hit-test 命中, 边缘 resize /
	//    拖动才能触发. 整窗可拖.
	XWnd_SetTransparentType(hWnd, window_transparent_shaped);
	XWnd_SetTransparentAlpha(hWnd, 255);
	XWnd_SetBkInfo(hWnd, L"{99:1.9.9;98:1(0);5:2(15)20(1)21(3)26(1)22(16777216)23(1)9(8,8,8,8);}");
	XWnd_EnableDragWindow(hWnd, TRUE);

	// 5. 给 acrylic 加系统圆角 (Win11 自动加 BORDER + frame shadow).
	{
		DWORD pref = (DWORD)xblur_corner_round;
		::DwmSetWindowAttribute(acrylic, kXBlur_DWMWA_WINDOW_CORNER_PREFERENCE,
			&pref, sizeof(pref));
	}

	m_attachToWindow  = true;
	m_attachedWnd     = hWnd;
	m_attachedExDcomp = true;
	m_acrylicApplied  = true;
	XBlur_EnsureGrayscaleTextAA();

	{
		std::lock_guard<std::mutex> lk(g_blurInstancesMutex);
		g_blurInstances.insert(this);
	}
	int gt = g_blurGlobalTheme.load();
	if (gt != xuitool_theme_custom){
		ApplyThemeInternal((xuitool_theme_)gt);
	}
	return TRUE;
}

//============================================================================
// Detach
//============================================================================
void CXBlur::DetachExInternal(){
	if (!m_attachedExDcomp) return;

	{
		std::lock_guard<std::mutex> lk(g_blurInstancesMutex);
		g_blurInstances.erase(this);
	}

	if (m_attachedWnd && XC_IsHWINDOW((HXCGUI)m_attachedWnd)){
		XBlurDComp::DetachAcrylicHost((void*)m_attachedWnd);
	} else {
		// 主窗句柄已失效: 只清 acrylic 子窗, 不调 XWnd_GetHWND.
		XBlurDComp::DetachAcrylicHost(NULL);
	}

	m_attachedExDcomp  = false;
	m_attachToWindow   = false;
	m_attachedWnd      = NULL;
	m_acrylicApplied   = false;
	m_hostHwnd         = NULL;
}

void CXBlur::Detach(){
	if (m_attachedExDcomp){
		DetachExInternal();
		return;
	}
	if (XC_IsHELE((HXCGUI)m_hEle)){
		DetachInternal();
	}
}

//============================================================================
// AttachInternal / DetachInternal
//============================================================================
void CXBlur::AttachInternal(HELE hEle, bool owned){
	m_hEle  = hEle;
	m_owned = owned;
	XUI_EnableCSS(hEle, FALSE);

	// 强制元素背景透明: 让 DWM acrylic 透过元素显示出来.
	// XCGUI 没有 IsEnableBkTransparent getter, 假设原状态为 FALSE (默认),
	// Detach 时还原为 FALSE.
	m_hasSavedTransparent = true;
	m_savedBkTransparent  = false;
	XEle_EnableBkTransparent(hEle, TRUE);

	RefreshDpiScale();
	HookEvents(hEle);

	// 仅在 D2D 模式挂 ACCENT_ACRYLIC. GDI 模式下 XCGUI 不写 per-pixel
	// alpha, ACCENT 会把 element 外的全窗区域都透为 backdrop, 出现
	// halo 伪影. GDI 模式下退化为 tint+border 装饰层.
	//
	// 另外: 宿主窗口已经是 per-pixel alpha 模式 (window_transparent_shaped /
	// window_transparent_shadow) 或 color-key 透明 (window_transparent_simple)
	// 时, *不挂* ACCENT_ACRYLIC. 原因:
	//   1) shaped/shadow 窗口的 alpha 通道含义是 "本窗对桌面的 per-pixel
	//      可见度", 而 ACCENT_ACRYLIC 把同一通道复用成 "本窗对 acrylic
	//      backdrop 的混合权重". 两者语义冲突, 叠在一起 DWM 会按 acrylic
	//      规则去混 alpha != 0/255 的中间值, 让用户给元素设的圆角外角落
	//      ( CXBlur 元素 bounding box 减去圆角 path 那 4 块 ) 出现"半透明
	//      非圆角区域"的伪影 (issue: shaped + 圆角 → 角落看上去有半透矩形).
	//   2) simple 是色键透明, ACCENT 路径不识别色键, 一挂会破坏色键效果.
	//   3) win7 模式当前 XCGUI 也不启用, 不必干预.
	// 这些模式下 CXBlur 退化为"tint + 圆角 + 边框"装饰层, 与 GDI 兜底相同.
	HWND host = FindHostHwnd();
	if (host){
		m_hostHwnd = host;  // 记录 host, Detach 时释放
		HWINDOW hxw = XWidget_GetHWINDOW((HXCGUI)hEle);
		window_transparent_ wt = hxw ? XWnd_GetTransparentType(hxw)
		                             : window_transparent_false;
		bool hostIsOpaque = (wt == window_transparent_false);
		if (XC_IsEnableD2D() && hostIsOpaque){
			m_acrylicApplied = XBlur_AcquireHostBlur(host, hEle);
			if (m_acrylicApplied)
				XBlur_EnsureGrayscaleTextAA();
		}
	}

	// 注册到全局活实例集合, 让 SetGlobalTheme 能广播到本实例.
	{
		std::lock_guard<std::mutex> lk(g_blurInstancesMutex);
		g_blurInstances.insert(this);
	}
	// 如果用户在 attach 之前已 SetGlobalTheme(...), 默认采用之.
	int gt = g_blurGlobalTheme.load();
	if (gt != xuitool_theme_custom){
		ApplyThemeInternal((xuitool_theme_)gt);
	} else {
		RedrawSelf();
	}
}

void CXBlur::DetachInternal(){
	// 从全局活实例集合移除 (即使 m_hEle 已无效也要 erase, 防止悬挂指针被
	// SetGlobalTheme 广播迭代到).
	{
		std::lock_guard<std::mutex> lk(g_blurInstancesMutex);
		g_blurInstances.erase(this);
	}

	if (!XC_IsHELE((HXCGUI)m_hEle)) return;

	UnhookEvents(m_hEle);

	if (m_attachToWindow && m_attachedWnd && XC_IsHWINDOW((HXCGUI)m_attachedWnd)){
		// 还原 AttachToWnd 时改的窗体属性 (在 UnregisterWindowSizeHook 把
		// m_attachedWnd 置 NULL 之前做).
		if (m_hasSavedWndLayout){
			XWnd_EnableLayout(m_attachedWnd, m_savedWndLayout ? TRUE : FALSE);
			XWnd_EnableLayoutOverlayBorder(m_attachedWnd, m_savedWndOverlayBorder ? TRUE : FALSE);
			m_hasSavedWndLayout = false;
		}
		UnregisterWindowSizeHook();
	}

	// 还原元素 EnableBkTransparent
	if (m_hasSavedTransparent){
		XEle_EnableBkTransparent(m_hEle, m_savedBkTransparent ? TRUE : FALSE);
		m_hasSavedTransparent = false;
	}

	// === 释放 host blur ===
	// 从 host 的订阅集合中移除当前 element. 最后一个时关闭 blur.
	if (m_hostHwnd){
		XBlur_ReleaseHostBlur(m_hostHwnd, m_hEle);
	}
	m_acrylicApplied = false;
	m_hostHwnd       = NULL;

	// owned 元素 (Create / AttachToWnd 创建的) 自动销毁
	if (m_owned){
		XEle_Destroy(m_hEle);
	}

	m_hEle           = NULL;
	m_owned          = false;
	m_attachToWindow = false;
	m_attachedWnd    = NULL;
	m_attachedExDcomp = false;
}

//============================================================================
// 事件 hook + 全局订阅表
//
// 问题: 多个 CXBlur 实例 attach 到同一窗口时, 都要监听该窗口的 WM_SETTINGCHANGE.
//        但 XCGUI 拒绝在同一窗口 + 同一事件 + 同一函数重复注册.
// 方案: 每个 host HWND 只注册 一次 全局回调; 全局回调 fan-out 到该窗口的
//        所有 CXBlur 订阅者.
//============================================================================
namespace {

struct _CXBlurHostInfo {
	std::set<CXBlur*> subs;
	bool regSetting = false;
	bool regSize    = false;
};
static std::mutex                    g_blurSubsMutex;
static std::map<HWND, _CXBlurHostInfo> g_blurSubsMap;

// 快照订阅者 (调用者不持锁遵循, 避免回调里反咬锁)
static std::vector<CXBlur*> SnapshotSubs(HWND raw) {
	std::vector<CXBlur*> v;
	std::lock_guard<std::mutex> lk(g_blurSubsMutex);
	auto it = g_blurSubsMap.find(raw);
	if (it != g_blurSubsMap.end()) v.assign(it->second.subs.begin(), it->second.subs.end());
	return v;
}

}  // namespace

static int CALLBACK _XBlur_PaintCB(HELE hEle, HDRAW hDraw, BOOL* pbHandled){
	CXBlur* pSelf = (CXBlur*)(intptr_t)XEle_GetUserData(hEle);
	if (!pSelf) return 0;
	return pSelf->OnPaintImpl(hEle, hDraw, pbHandled);
}
static int CALLBACK _XBlur_SizeCB(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled){
	CXBlur* pSelf = (CXBlur*)(intptr_t)XEle_GetUserData(hEle);
	if (!pSelf) return 0;
	return pSelf->OnSizeImpl(hEle, nFlags, nAdjustNo, pbHandled);
}
static int CALLBACK _XBlur_DestroyCB(HELE hEle, BOOL* pbHandled){
	CXBlur* pSelf = (CXBlur*)(intptr_t)XEle_GetUserData(hEle);
	if (!pSelf) return 0;
	return pSelf->OnDestroyImpl(hEle, pbHandled);
}
// 全局窗口级回调 (只会被注册一次/一个 HWND)
static int CALLBACK _CXBlur_GlobalSettingCB(HWINDOW hWnd, UINT uFlags, void* pStr, BOOL* pbHandled){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return 0;
	HWND raw = XWnd_GetHWND(hWnd);
	if (!raw) return 0;
	auto subs = SnapshotSubs(raw);
	for (auto* s : subs){
		s->OnWndSettingChangeImpl(hWnd, uFlags, pStr, pbHandled);
	}
	return 0;
}
static int CALLBACK _CXBlur_GlobalWndSizeCB(HWINDOW hWnd, UINT nFlags, SIZE* pSize, BOOL* pbHandled){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return 0;
	HWND raw = XWnd_GetHWND(hWnd);
	if (!raw) return 0;
	auto subs = SnapshotSubs(raw);
	for (auto* s : subs){
		s->OnWndSizeImpl(hWnd, nFlags, pSize, pbHandled);
	}
	return 0;
}

void CXBlur::HookEvents(HELE hEle){
	XEle_SetUserData(hEle, (vint)(intptr_t)this);
	XEle_RegEventC1(hEle, XE_PAINT,    (void*)_XBlur_PaintCB);
	XEle_RegEventC1(hEle, XE_SIZE,     (void*)_XBlur_SizeCB);
	XEle_RegEventC1(hEle, XE_DESTROY,  (void*)_XBlur_DestroyCB);

	// 让本实例加入 host 全局订阅表. WM_SETTINGCHANGE 只被首个订阅者注册一次.
	HWINDOW hxw = XWidget_GetHWINDOW((HXCGUI)hEle);
	if (!hxw || !XC_IsHWINDOW((HXCGUI)hxw)) return;
	HWND raw = XWnd_GetHWND(hxw);
	if (!raw) return;

	bool needRegSetting = false;
	{
		std::lock_guard<std::mutex> lk(g_blurSubsMutex);
		auto& info = g_blurSubsMap[raw];
		info.subs.insert(this);
		if (!info.regSetting){
			info.regSetting = true;
			needRegSetting  = true;
		}
	}
	if (needRegSetting){
		XWnd_RegEventC1(hxw, WM_SETTINGCHANGE, (void*)_CXBlur_GlobalSettingCB);
	}
}

void CXBlur::UnhookEvents(HELE hEle){
	XEle_RemoveEventC(hEle, XE_PAINT,    (void*)_XBlur_PaintCB);
	XEle_RemoveEventC(hEle, XE_SIZE,     (void*)_XBlur_SizeCB);
	XEle_RemoveEventC(hEle, XE_DESTROY,  (void*)_XBlur_DestroyCB);
	XEle_SetUserData(hEle, 0);

	HWINDOW hxw = XWidget_GetHWINDOW((HXCGUI)hEle);
	if (!hxw || !XC_IsHWINDOW((HXCGUI)hxw)) return;
	HWND raw = XWnd_GetHWND(hxw);
	if (!raw) return;

	bool needUnregSetting = false;
	bool needUnregSize    = false;
	{
		std::lock_guard<std::mutex> lk(g_blurSubsMutex);
		auto it = g_blurSubsMap.find(raw);
		if (it != g_blurSubsMap.end()){
			it->second.subs.erase(this);
			if (it->second.subs.empty()){
				needUnregSetting = it->second.regSetting;
				needUnregSize    = it->second.regSize;
				g_blurSubsMap.erase(it);
			}
		}
	}
	if (needUnregSetting){
		XWnd_RemoveEventC(hxw, WM_SETTINGCHANGE, (void*)_CXBlur_GlobalSettingCB);
	}
	if (needUnregSize){
		XWnd_RemoveEventC(hxw, WM_SIZE, (void*)_CXBlur_GlobalWndSizeCB);
	}
}

void CXBlur::RegisterWindowSizeHook(HWINDOW hWnd){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return;
	HWND raw = XWnd_GetHWND(hWnd);
	if (!raw) return;

	bool needRegSize = false;
	{
		std::lock_guard<std::mutex> lk(g_blurSubsMutex);
		auto& info = g_blurSubsMap[raw];
		info.subs.insert(this);   // 充当订阅者
		if (!info.regSize){
			info.regSize = true;
			needRegSize  = true;
		}
	}
	if (needRegSize){
		XWnd_RegEventC1(hWnd, WM_SIZE, (void*)_CXBlur_GlobalWndSizeCB);
	}
}

void CXBlur::UnregisterWindowSizeHook(){
	// 实际反注册逻辑统一在 UnhookEvents 中, 这里仅清理 m_attachedWnd 记录.
	m_attachedWnd = NULL;
}

//============================================================================
// 找到本元素所在的顶层 HWND
//============================================================================
HWND CXBlur::FindHostHwnd() const {
	if (!XC_IsHELE((HXCGUI)m_hEle)) return NULL;
	HWINDOW hxw = XWidget_GetHWINDOW((HXCGUI)m_hEle);
	if (!hxw || !XC_IsHWINDOW((HXCGUI)hxw)) return NULL;
	HWND raw = XWnd_GetHWND(hxw);
	if (!raw) return NULL;
	return raw;
}

//============================================================================
// 元素事件实现
//============================================================================
int CXBlur::OnPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled){
	if (!hDraw) return 0;

	BOOL useD2D = XC_IsEnableD2D();
	if (useD2D && (m_acrylicApplied || m_attachedExDcomp))
		XBlur_EnsureGrayscaleTextAA();
	if (useD2D){
		ID2D1RenderTarget* rt = (ID2D1RenderTarget*)XDraw_GetD2dRenderTarget(hDraw);
		if (rt) OnPaintD2D(rt, hDraw);
	} else {
		HDC hdc = (HDC)XDraw_GetHDC(hDraw);
		if (hdc) OnPaintGdi(hdc, hDraw);
	}
	if (pbHandled) *pbHandled = TRUE;
	return 0;
}

int CXBlur::OnSizeImpl(HELE /*hEle*/, int /*nFlags*/, UINT /*nAdjustNo*/, BOOL* /*pbHandled*/){
	RefreshDpiScale();
	// ACCENT 路径 DWM 会自己跟随 client area resize, 不需重发 SetWindowCompositionAttribute.
	RedrawSelf();
	return 0;
}

int CXBlur::OnDestroyImpl(HELE hEle, BOOL* /*pbHandled*/){
	// 还原 AttachToWnd 时改的窗体属性 (元素被 XCGUI 自动销毁路径).
	if (m_attachToWindow && m_attachedWnd && m_hasSavedWndLayout
	    && XC_IsHWINDOW((HXCGUI)m_attachedWnd)){
		XWnd_EnableLayout(m_attachedWnd, m_savedWndLayout ? TRUE : FALSE);
		XWnd_EnableLayoutOverlayBorder(m_attachedWnd, m_savedWndOverlayBorder ? TRUE : FALSE);
		m_hasSavedWndLayout = false;
	}
	// 释放 host blur 引用 (跟 DetachInternal 等价).
	if (m_hostHwnd){
		XBlur_ReleaseHostBlur(m_hostHwnd, hEle);
	}
	m_acrylicApplied = false;
	m_hostHwnd       = NULL;
	m_attachToWindow = false;
	m_attachedWnd    = NULL;
	m_hEle           = NULL;
	// element 已销毁, 从全局活实例集合移除, 避免 SetGlobalTheme 广播到死实例.
	{
		std::lock_guard<std::mutex> lk(g_blurInstancesMutex);
		g_blurInstances.erase(this);
	}
	return 0;
}

int CXBlur::OnWndSizeImpl(HWINDOW /*hWnd*/, UINT /*nFlags*/, SIZE* pSize, BOOL* /*pbHandled*/){
	if (m_attachToWindow && pSize && XC_IsHELE((HXCGUI)m_hEle)){
		XEle_SetWidth (m_hEle, pSize->cx);
		XEle_SetHeight(m_hEle, pSize->cy);
		RedrawSelf();
	}
	// dcomp visual resize:
	//   - AttachToWndEx: 由 acrylic subclass (XcguiToAcrylicSyncProc) 同步, 此处跳过.
	//   - AttachToEle/Wnd + XBLUR_FORCE_DCOMP: 走 host blur map, 需显式 Resize.
	if (!m_attachedExDcomp && m_acrylicApplied && m_hostHwnd){
		int activePath = XBlur_GetActivePath(m_hostHwnd);
		if (activePath == XBLUR_PATH_DCOMP_WINRT){
			XBlurDComp::Resize(m_hostHwnd, /*inset*/0);
		}
	}
	return 0;
}

int CXBlur::OnWndSettingChangeImpl(HWINDOW /*hWnd*/, UINT /*uFlags*/, void* /*pStr*/, BOOL* /*pbHandled*/){
	// pStr 是变化类型字符串 (e.g. "ImmersiveColorSet"). 任何系统设置变化都重新
	// 跑一遍 theme auto: 这里会按当前 AppsUseLightTheme 刷新 tint.
	if (m_theme.load() == xuitool_theme_auto){
		ApplyThemePreset(xuitool_theme_auto);
	}

#if XBLUR_ENABLE_DWM_TRANSIENT
	// DWM_TRANSIENT 路径: 系统 light/dark 切换时重设 DWMWA_USE_IMMERSIVE_DARK_MODE.
	if (m_acrylicApplied && m_hostHwnd){
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(m_hostHwnd);
		if (it != g_hostBlurMap.end() &&
		    it->second.activePath == XBLUR_PATH_DWM_TRANSIENT)
		{
			bool nowDark = XBlur_QuerySystemDarkMode();
			if (nowDark != it->second.darkMode){
				it->second.darkMode = nowDark;
				BOOL bDark = nowDark ? TRUE : FALSE;
				::DwmSetWindowAttribute(m_hostHwnd,
					kXBlur_DWMWA_USE_IMMERSIVE_DARK_MODE,
					&bDark, sizeof(bDark));
			}
		}
	}
#endif
	RedrawSelf();
	return 0;
}

//============================================================================
// 渲染: D2D 路径
//============================================================================
void CXBlur::OnPaintD2D(ID2D1RenderTarget* rt, HDRAW /*hDraw*/){
	RECT rcEle;
	XEle_GetWndClientRectDPI(m_hEle, &rcEle);
	int eleW = rcEle.right  - rcEle.left;
	int eleH = rcEle.bottom - rcEle.top;
	if (eleW <= 0 || eleH <= 0) return;

	D2D1_RECT_F rfEle = D2D1::RectF(
		(FLOAT)rcEle.left,  (FLOAT)rcEle.top,
		(FLOAT)rcEle.right, (FLOAT)rcEle.bottom);

	int tl = m_cornerTL.load();
	int tr = m_cornerTR.load();
	int br = m_cornerBR.load();
	int bl = m_cornerBL.load();
	bool hasCorner = (tl | tr | br | bl) > 0;

	// QI ID2D1DeviceContext: base RT 无 COPY blend / effect graph.
	ID2D1DeviceContext* dc = NULL;
	bool haveDc = SUCCEEDED(rt->QueryInterface(IID_ID2D1DeviceContext, (void**)&dc)) && dc;

	ID2D1Factory* pFac = NULL;
	rt->GetFactory(&pFac);

	COLORREF tc = m_tintColor.load();
	BYTE ta = GetAValue(tc);

	// === 取当前激活的 backdrop 路径, 决定 element 端 paint 怎么 blend.
	int activePath = m_hostHwnd ? XBlur_GetActivePath(m_hostHwnd)
	                             : XBLUR_PATH_DECORATIVE;
	if (haveDc && pFac){
		// 一次构造圆角几何, fill / clip / 都共用
		ID2D1Geometry* pGeom = XBlur_CreateCornerGeometry(pFac, rfEle, tl, tr, br, bl);

		// === 1) tint 填充: 路径互斥的 blend 策略 ===
		//
		//   a) ACCENT_ACRYLIC / DCOMP_WINRT: DWM/dcomp 已合成 tint, element 端
		//      仅 COPY 清 alpha 让 backdrop 透出.
		//
		//   b) ACCENT_BLURBEHIND: DWM 只 blur 不 tint, element COPY 涂 tint+alpha.
		//
		//   c) DECORATIVE: SOURCE_OVER 半透 tint.
		const bool dwmHandlesTint =
		    (activePath == XBLUR_PATH_ACCENT_ACRYLIC) ||
		    (activePath == XBLUR_PATH_DCOMP_WINRT);
		const bool accentBlurBehind =
		    (activePath == XBLUR_PATH_ACCENT_BLURBEHIND);

		if (pGeom){
			if (dwmHandlesTint){
				// 1a/1b: 仅 COPY 清 alpha 到 0, 让 DWM 把 GradientColor 加 backdrop
				// 合成的 acrylic 完整透出.
				//
				// *不再叠 element 端 tint*: DWM 那层已经按 GradientColor (= m_tintColor)
				// 把 tint 混入 acrylic 公式, 这里再 SOURCE_OVER 等于 *双层 tint*, 把
				// DWM blur 盖死成纯色 (用户报的"死灰"问题). 用户改 tint 颜色直接走
				// SetTintColor → reapply 路径同步给 DWM, element 端不再插一脚.
				ID2D1SolidColorBrush* pClear = NULL;
				dc->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &pClear);
				if (pClear){
					D2D1_PRIMITIVE_BLEND oldBlend = dc->GetPrimitiveBlend();
					dc->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
					dc->FillGeometry(pGeom, pClear);
					dc->SetPrimitiveBlend(oldBlend);
				}
				SafeRelease(pClear);
			} else if (accentBlurBehind){
				// 1c: COPY blend 同时写 tint+alpha (RS5- 唯一方案).
				if (ta != 0){
					ID2D1SolidColorBrush* pTint = NULL;
					dc->CreateSolidColorBrush(
						D2D1::ColorF(GetRValue(tc) / 255.0f, GetGValue(tc) / 255.0f,
						             GetBValue(tc) / 255.0f, ta / 255.0f),
						&pTint);
					if (pTint){
						D2D1_PRIMITIVE_BLEND oldBlend = dc->GetPrimitiveBlend();
						dc->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
						dc->FillGeometry(pGeom, pTint);
						dc->SetPrimitiveBlend(oldBlend);
					}
					SafeRelease(pTint);
				}
			} else {
				// 1d: DECORATIVE / shaped 窗户 / 老 OS — SOURCE_OVER 半透 tint, 不动 alpha.
				if (ta != 0){
					ID2D1SolidColorBrush* pTint = NULL;
					dc->CreateSolidColorBrush(
						D2D1::ColorF(GetRValue(tc) / 255.0f, GetGValue(tc) / 255.0f,
						             GetBValue(tc) / 255.0f, ta / 255.0f),
						&pTint);
					if (pTint) dc->FillGeometry(pGeom, pTint);
					SafeRelease(pTint);
				}
			}
		}

		// 2) 圆角 clip (有任一角倒角才推 layer)
		ID2D1Layer* pLayer = NULL;
		if (hasCorner && pGeom){
			if (SUCCEEDED(dc->CreateLayer(NULL, &pLayer)) && pLayer){
				D2D1_LAYER_PARAMETERS lp = D2D1::LayerParameters(rfEle, pGeom);
				dc->PushLayer(lp, pLayer);
			}
		}

		// 3) 噪点层: Turbulence → 去饱和 → 调 alpha → SOURCE_OVER 叠在 tint 上.
		float noise = m_noise.load();
		if (noise > 0.0f){
			int eW = rcEle.right - rcEle.left;
			int eH = rcEle.bottom - rcEle.top;
			ID2D1Effect *turb = NULL, *deSat = NULL, *opac = NULL;
			if (SUCCEEDED(dc->CreateEffect(CLSID_D2D1Turbulence, &turb)) && turb &&
			    SUCCEEDED(dc->CreateEffect(CLSID_D2D1Saturation, &deSat)) && deSat &&
			    SUCCEEDED(dc->CreateEffect(CLSID_D2D1ColorMatrix, &opac)) && opac){
				// Turbulence 默认输出 512x512, element > 512 时只有左上角有噪点;
				// 显式设 SIZE = element 尺寸让噪点铺满.
				turb->SetValue(D2D1_TURBULENCE_PROP_OFFSET, D2D1::Vector2F(0, 0));
				turb->SetValue(D2D1_TURBULENCE_PROP_SIZE,
				               D2D1::Vector2F((FLOAT)eW, (FLOAT)eH));
				// base_freq=0.7 → 颗粒 ~1.4 像素, 接近 Win11 acrylic 砂纸感.
				turb->SetValue(D2D1_TURBULENCE_PROP_BASE_FREQUENCY,
				               D2D1::Vector2F(0.7f, 0.7f));
				turb->SetValue(D2D1_TURBULENCE_PROP_NUM_OCTAVES, (UINT32)1);
				turb->SetValue(D2D1_TURBULENCE_PROP_SEED, (UINT32)0xA5);
				turb->SetValue(D2D1_TURBULENCE_PROP_NOISE,
				               D2D1_TURBULENCE_NOISE_TURBULENCE);
				turb->SetValue(D2D1_TURBULENCE_PROP_STITCHABLE, FALSE);
				deSat->SetInputEffect(0, turb);
				deSat->SetValue(D2D1_SATURATION_PROP_SATURATION, 0.0f);
				opac->SetInputEffect(0, deSat);
				D2D1_MATRIX_5X4_F m = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, Clampf(noise, 0.0f, 1.0f),
					0, 0, 0, 0
				};
				opac->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, m);
				dc->DrawImage(opac,
				              D2D1::Point2F((FLOAT)rcEle.left, (FLOAT)rcEle.top),
				              D2D1::RectF(0, 0, (FLOAT)eW, (FLOAT)eH),
				              D2D1_INTERPOLATION_MODE_LINEAR,
				              D2D1_COMPOSITE_MODE_SOURCE_OVER);
			}
			SafeRelease(opac);
			SafeRelease(deSat);
			SafeRelease(turb);
		}

		if (pLayer){
			dc->PopLayer();
			pLayer->Release();
		}
		SafeRelease(pGeom);
		dc->Release();
	} else {
		// D2D 1.0 / 老 RT fallback: 没法 COPY blend, 也没 effect graph.
		// 直接 SOURCE_OVER 画 tint, 视觉是纯色块 (不出 acrylic, 跟 GDI 模式一致).
		if (ta != 0){
			ID2D1SolidColorBrush* pTint = NULL;
			HRESULT hr = rt->CreateSolidColorBrush(
				D2D1::ColorF(GetRValue(tc) / 255.0f, GetGValue(tc) / 255.0f,
				             GetBValue(tc) / 255.0f, ta / 255.0f),
				&pTint);
			if (SUCCEEDED(hr) && pTint){
				rt->FillRectangle(rfEle, pTint);
				pTint->Release();
			}
		}
	}

	// 边框: 路径中线沿 element 内侧 inset = bw/2, 圆角半径同步缩 bw/2 防角度偏移.
	COLORREF bc = m_borderColor.load();
	BYTE ba = GetAValue(bc);
	float bw = m_borderWidth.load();
	if (ba != 0 && bw > 0.0f && pFac){
		D2D1_RECT_F rfBd = D2D1::RectF(
			rfEle.left + bw * 0.5f, rfEle.top + bw * 0.5f,
			rfEle.right - bw * 0.5f, rfEle.bottom - bw * 0.5f);
		int half = (int)(bw * 0.5f + 0.5f);
		ID2D1Geometry* pBdGeom = XBlur_CreateCornerGeometry(pFac, rfBd,
			(std::max)(0, tl - half), (std::max)(0, tr - half),
			(std::max)(0, br - half), (std::max)(0, bl - half));
		ID2D1SolidColorBrush* pBd = NULL;
		rt->CreateSolidColorBrush(
			D2D1::ColorF(GetRValue(bc) / 255.0f, GetGValue(bc) / 255.0f,
			             GetBValue(bc) / 255.0f, ba / 255.0f),
			&pBd);
		if (pBd && pBdGeom){
			rt->DrawGeometry(pBdGeom, pBd, bw);
		}
		SafeRelease(pBd);
		SafeRelease(pBdGeom);
	}

	if (pFac) pFac->Release();
}

//============================================================================
// 渲染: GDI+ 路径
//============================================================================
void CXBlur::OnPaintGdi(HDC hdc, HDRAW /*hDraw*/){
	RECT rcEle;
	XEle_GetWndClientRectDPI(m_hEle, &rcEle);
	int eleW = rcEle.right  - rcEle.left;
	int eleH = rcEle.bottom - rcEle.top;
	if (eleW <= 0 || eleH <= 0) return;

	int tl = m_cornerTL.load();
	int tr = m_cornerTR.load();
	int br = m_cornerBR.load();
	int bl = m_cornerBL.load();
	bool hasCorner = (tl | tr | br | bl) > 0;

	// GDI 模式 = 没挂 ACCENT_ACRYLIC (见 AttachInternal), 这里只画半透 tint
	// 装饰层 + 边框, 跟"支持但用户关了透明效果"的视觉一致.
	COLORREF tc = m_tintColor.load();
	BYTE ta = GetAValue(tc);
	if (ta != 0){
		Gdiplus::Graphics g(hdc);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		Gdiplus::SolidBrush br_(
			Gdiplus::Color(ta, GetRValue(tc), GetGValue(tc), GetBValue(tc)));
		if (hasCorner){
			Gdiplus::GraphicsPath path;
			XBlur_BuildGdiCornerPath(path,
				(float)rcEle.left, (float)rcEle.top, (float)eleW, (float)eleH,
				tl, tr, br, bl);
			g.FillPath(&br_, &path);
		} else {
			g.FillRectangle(&br_, rcEle.left, rcEle.top, eleW, eleH);
		}
	}

	// 边框: inset = bw/2 同步缩, 圆角半径同步缩 bw/2 防角度偏移.
	COLORREF bc = m_borderColor.load();
	BYTE ba = GetAValue(bc);
	float bw = m_borderWidth.load();
	if (ba != 0 && bw > 0.0f){
		Gdiplus::Graphics g(hdc);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		Gdiplus::Pen pen(
			Gdiplus::Color(ba, GetRValue(bc), GetGValue(bc), GetBValue(bc)),
			(Gdiplus::REAL)bw);

		float inset = bw * 0.5f;
		float bx = (float)rcEle.left + inset;
		float by = (float)rcEle.top  + inset;
		float bW = (float)eleW - bw;
		float bH = (float)eleH - bw;
		int half = (int)(bw * 0.5f + 0.5f);
		int btl = (std::max)(0, tl - half);
		int btr = (std::max)(0, tr - half);
		int bbr = (std::max)(0, br - half);
		int bbl = (std::max)(0, bl - half);
		bool bHasCorner = (btl | btr | bbr | bbl) > 0;
		if (bHasCorner){
			Gdiplus::GraphicsPath path;
			XBlur_BuildGdiCornerPath(path, bx, by, bW, bH, btl, btr, bbr, bbl);
			g.DrawPath(&pen, &path);
		} else {
			g.DrawRectangle(&pen, bx, by, bW, bH);
		}
	}
}

//============================================================================
// DPI / Redraw
//============================================================================
void CXBlur::RefreshDpiScale(){
	// 不用 ::GetDpiForWindow: Win7 user32.dll 没这个导出, 静态导入会让整个 exe
	// 启动失败 (无法定位程序输入点). XCGUI 提供 XWnd_GetDPI 自带跨版本封装,
	// 返回百分比 (系统 150% → 150), 直接除 100 得 scale.
	if (!XC_IsHELE((HXCGUI)m_hEle)){ m_dpiScale = 1.0f; return; }
	HWINDOW hxw = XWidget_GetHWINDOW((HXCGUI)m_hEle);
	if (!hxw){ m_dpiScale = 1.0f; return; }
	int dpi = XWnd_GetDPI(hxw);
	if (dpi <= 0) dpi = 100;
	m_dpiScale = (float)dpi / 100.0f;
}

void CXBlur::RedrawSelf(){
	if (XC_IsHELE((HXCGUI)m_hEle)){
		XEle_Redraw(m_hEle);
	}
}

//============================================================================
// 主题预设
//============================================================================
BOOL CXBlur::IsSystemDarkMode(){
	return XUITool_IsSystemDarkMode();
}

void CXBlur::ApplyThemePresetTintNoise(xuitool_theme_ theme){
	if (theme == xuitool_theme_auto){
		ApplyThemePresetTintNoise(IsSystemDarkMode() ? xuitool_theme_dark : xuitool_theme_light);
		return;
	}
	if (theme != xuitool_theme_light && theme != xuitool_theme_dark) return;
	CXBlurThemeDefaults d = GetThemeDefault(theme);
	m_tintColor.store(d.tintColor);
	m_noise.store(d.noise);
}

void CXBlur::ApplyThemeInternal(xuitool_theme_ theme){
	m_theme.store((int)theme);
	if (m_attachedExDcomp){
		if (theme == xuitool_theme_light || theme == xuitool_theme_dark || theme == xuitool_theme_auto)
			ApplyThemePresetTintNoise(theme);
		ReapplyExEffects();
		return;
	}
	if (theme == xuitool_theme_light || theme == xuitool_theme_dark || theme == xuitool_theme_auto)
		ApplyThemePreset(theme);
	else
		RedrawSelf();
}

void CXBlur::ApplyThemePreset(xuitool_theme_ theme){
	ApplyThemePresetTintNoise(theme);
	xuitool_theme_ resolved = theme;
	if (theme == xuitool_theme_auto){
		resolved = IsSystemDarkMode() ? xuitool_theme_dark : xuitool_theme_light;
	}
	if (resolved != xuitool_theme_light && resolved != xuitool_theme_dark) return;

	CXBlurThemeDefaults d = GetThemeDefault(resolved);

#if XBLUR_ENABLE_DWM_TRANSIENT
	// DWM_TRANSIENT 路径: 显式 light/dark 主题需写 DWMWA_USE_IMMERSIVE_DARK_MODE.
	if (m_acrylicApplied && m_hostHwnd){
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(m_hostHwnd);
		if (it != g_hostBlurMap.end() &&
		    it->second.activePath == XBLUR_PATH_DWM_TRANSIENT)
		{
			bool wantDark = (resolved == xuitool_theme_dark);
			if (wantDark != it->second.darkMode){
				it->second.darkMode = wantDark;
				BOOL bDark = wantDark ? TRUE : FALSE;
				::DwmSetWindowAttribute(m_hostHwnd,
					kXBlur_DWMWA_USE_IMMERSIVE_DARK_MODE,
					&bDark, sizeof(bDark));
			}
		}
	}
#endif
	if (m_acrylicApplied && m_hostHwnd){
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(m_hostHwnd);
		// ACCENT_ACRYLIC: tint 变了, reapply 把新 GradientColor 推给 DWM.
		if (it != g_hostBlurMap.end() &&
		    it->second.activePath == XBLUR_PATH_ACCENT_ACRYLIC &&
		    it->second.activeTint != d.tintColor)
		{
			XBlur_ApplyHostBlur_Locked(m_hostHwnd);
		}
		// DCOMP_WINRT 路径: PoC 默认值由 dcomp case 内部根据 userTheme +
		// userBlurOpacity 决定, m_theme/m_tintColor 改了后必须 reapply 让
		// effect chain 重建. (ApplyHostBlur_Locked 在 dcomp case 里读 GetTheme.)
		if (it != g_hostBlurMap.end() &&
		    it->second.activePath == XBLUR_PATH_DCOMP_WINRT)
		{
			XBlur_ApplyHostBlur_Locked(m_hostHwnd);
		}
	}
	RedrawSelf();
}

// === SetThemeDefault / GetThemeDefault ===
// 修改 light/dark 主题预设的默认参数. 后续 SetTheme(...) 用新值.
// 不立即触发 redraw - 用户期望"先调好默认值再 SetTheme 应用".
void CXBlur::SetThemeDefault(xuitool_theme_ theme, const CXBlurThemeDefaults& d){
	if (theme != xuitool_theme_light && theme != xuitool_theme_dark) return;
	std::lock_guard<std::mutex> lk(g_themeDefaultsMutex);
	if (theme == xuitool_theme_light) g_lightDefaults = d;
	else                            g_darkDefaults  = d;
}
CXBlurThemeDefaults CXBlur::GetThemeDefault(xuitool_theme_ theme){
	std::lock_guard<std::mutex> lk(g_themeDefaultsMutex);
	// auto 按系统当前选 light/dark; 其他无效值返回 light 作为安全默认.
	if (theme == xuitool_theme_auto)
		return IsSystemDarkMode() ? g_darkDefaults : g_lightDefaults;
	return (theme == xuitool_theme_dark) ? g_darkDefaults : g_lightDefaults;
}

//============================================================================
// 公开 setter / getter
//============================================================================
int CXBlur::GetBindMode() const {
	if (m_attachedExDcomp && m_attachToWindow) return xblur_bind_window;
	if (!XC_IsHELE((HXCGUI)m_hEle)) return xblur_bind_none;
	if (m_attachToWindow) return xblur_bind_window;
	return m_owned ? xblur_bind_owned : xblur_bind_attach;
}

// --- 叠加色 ---
// 用户改 tint 时:
//  1) ACCENT_ACRYLIC 路径 (Win10 1803~1809 / Win11 21H2): 重新调用
//     SetWindowCompositionAttribute, 把新 tint 透传给 DWM GradientColor,
//     让 DWM 端的 acrylic 公式立即用新颜色 (否则只 element 端那层 SOURCE_OVER
//     变了, DWM backdrop 仍是老 tint, 视觉错位).
//  2) 其他路径: 仅 RedrawSelf 让 element 端重画.
void CXBlur::SetTintColor(COLORREF color){
	m_tintColor.store(color);
	// AttachToWndEx 路径: 直接刷 dcomp effect chain.
	if (m_attachedExDcomp){
		ReapplyExEffects();
		return;
	}
	if (m_acrylicApplied && m_hostHwnd){
		// 哪些路径需要 reapply:
		//   ACCENT_ACRYLIC : 把新 tint 透传给 DWM GradientColor.
		//   DCOMP_WINRT    : 重建 dcomp effect chain 反映新 tint.
		//   DWM_TRANSIENT  : DWM 用系统主题色, 不接受 element tint, 不 reapply.
		//   ACCENT_BLURBEHIND : 无 GradientColor, 不 reapply.
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(m_hostHwnd);
		if (it != g_hostBlurMap.end()){
			int p = it->second.activePath;
			bool acrylicChanged = (p == XBLUR_PATH_ACCENT_ACRYLIC && it->second.activeTint != color);
			bool dcompPath      = (p == XBLUR_PATH_DCOMP_WINRT);
			if (acrylicChanged || dcompPath){
				XBlur_ApplyHostBlur_Locked(m_hostHwnd);
			}
		}
	}
	RedrawSelf();
}
COLORREF CXBlur::GetTintColor() const { return m_tintColor.load(); }

// --- UniformBrightness (LuminosityBlend, dcomp 路径专属) ---
// 老路径 (ACCENT / DWM) 路径下 setter 仍然成功记录到 m_uniformBrightness, 但不会
// reapply (那些路径没有这个语义). 切到 dcomp 路径后值就生效.
void CXBlur::SetUniformBrightness(BOOL bEnable){
	int newVal = bEnable ? 1 : 0;
	int oldVal = m_uniformBrightness.exchange(newVal);
	if (oldVal == newVal) return;
	// AttachToWndEx 路径: 直接刷 dcomp effect chain.
	if (m_attachedExDcomp){
		ReapplyExEffects();
		return;
	}
	if (m_acrylicApplied && m_hostHwnd){
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(m_hostHwnd);
		if (it != g_hostBlurMap.end() && it->second.activePath == XBLUR_PATH_DCOMP_WINRT){
			XBlur_ApplyHostBlur_Locked(m_hostHwnd);
		}
	}
}
BOOL CXBlur::GetUniformBrightness() const {
	return m_uniformBrightness.load() ? TRUE : FALSE;
}

// --- BlurOpacity / "通透感" (dcomp 路径专属) ---
// 0..1 存进 m_blurOpacityMilli (毫单位整数 0..1000), 负数存 -1 表示 unset.
// 老路径下 setter 仍然成功记录, 但不会 reapply (那些路径用 tintA 控制等价行为).
// 切到 dcomp 路径后值就生效.
void CXBlur::SetBlurOpacity(float fOpacity){
	int newMilli;
	if (fOpacity < 0.0f) {
		newMilli = -1;
	} else {
		if (fOpacity > 1.0f) fOpacity = 1.0f;
		newMilli = (int)(fOpacity * 1000.0f + 0.5f);
	}
	int oldMilli = m_blurOpacityMilli.exchange(newMilli);
	if (oldMilli == newMilli) return;
	// AttachToWndEx 路径: 直接刷 dcomp effect chain.
	if (m_attachedExDcomp){
		ReapplyExEffects();
		return;
	}
	if (m_acrylicApplied && m_hostHwnd){
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(m_hostHwnd);
		if (it != g_hostBlurMap.end() && it->second.activePath == XBLUR_PATH_DCOMP_WINRT){
			XBlur_ApplyHostBlur_Locked(m_hostHwnd);
		}
	}
}
float CXBlur::GetBlurOpacity() const {
	int milli = m_blurOpacityMilli.load();
	return milli < 0 ? -1.0f : (float)milli / 1000.0f;
}

// --- Theme ---
void CXBlur::SetTheme(xuitool_theme_ theme){
	ApplyThemeInternal(theme);
}
xuitool_theme_ CXBlur::GetTheme() const { return (xuitool_theme_)m_theme.load(); }

// SetGlobalTheme 实现: 同步到所有活实例 + 写入全局默认 (供后创建的实例读).
void CXBlur::SetGlobalTheme(xuitool_theme_ theme){
	g_blurGlobalTheme.store((int)theme);
	std::lock_guard<std::mutex> lk(g_blurInstancesMutex);
	for (CXBlur* p : g_blurInstances){
		if (p) p->SetTheme(theme);
	}
}
xuitool_theme_ CXBlur::GetGlobalTheme(){
	return (xuitool_theme_)g_blurGlobalTheme.load();
}

void  CXBlur::SetNoise(float a){
	m_noise.store(Clampf(a, 0.0f, 1.0f));
	if (m_attachedExDcomp){
		ReapplyExEffects();
		return;
	}
	RedrawSelf();
}
float CXBlur::GetNoise() const { return m_noise.load(); }

// --- 能力查询 ---
// 只要 DWM 合成启用 (Vista+) 就认为支持; 实际路线选择在 attach 时 OS 能力降级.
// Win8/8.1 上 SetWindowCompositionAttribute / DwmEnableBlurBehindWindow 都返回透明
// 无 blur, 这里我们仍返 TRUE (CXBlur 会自动退化为仅装饰, 不报错).
BOOL CXBlur::IsSystemAcrylicSupported(){
	BOOL enabled = FALSE;
	return (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled) ? TRUE : FALSE;
}
BOOL CXBlur::IsSystemAcrylicEnabled() const {
	return (m_acrylicApplied || m_attachedExDcomp) ? TRUE : FALSE;
}

//============================================================================
// EnableNativeShadow / EnableNativeRoundedCorner — host 状态机.
//
// 一击式 DwmExtendFrameIntoClientArea / DwmSetWindowAttribute 只能管"此刻",
// 不能管 host 后续状态变迁. 必须装状态机覆盖 3 个真实问题:
//
//   1) Aero snap: 用户拖窗到屏幕边触发系统 snap, 过渡里 DWM 把 frame extension
//      重置. 松手后窗矩形外圈一片空 — 必须 WM_SIZE 重新 apply.
//
//   2) 最大化 (IsZoomed = TRUE): 用户直觉里 "贴满屏幕, 不该有阴影也不该有圆角",
//      DWM 不会主动关 — 状态机识别并切到 margins=0 + DWMWCP_DONOTROUND, 还原后
//      切回用户设置.
//
//   3) 启动白闪: DwmExtendFrameIntoClientArea 触发后 frame 立刻出现 (圆角 + 阴影),
//      但 XCGUI / D2D 首帧落后 50~200 ms, 中间窗体内一格白闪. 解法 = DWMWA_CLOAK
//      在 ShowWindow 前隐窗, 注册 150 ms 定时器揭 cloak.
//
// 实现 = 每个 host HWND 一个 NativeFxState + XWM_WINDPROC 钩子, 仅在用户至少
// 调用过一次 EnableNativeShadow / EnableNativeRoundedCorner 的 HWND 上启用,
// 不影响其它 CXBlur 实例.
//============================================================================
namespace {

struct NativeFxState {
	bool    shadowEnabled  = false;     // 最近一次 EnableNativeShadow 的值
	int     cornerPref     = -1;        // 最近一次 EnableNativeRoundedCorner; -1 = 未启用
	bool    inMaximized    = false;     // IsZoomed 实测; TRUE 时屏蔽 shadow + 强制 donotround
	bool    cloakRequested = false;     // ShowWindow 前已 cloak, 等定时器揭
	bool    hookInstalled  = false;     // XWM_WINDPROC 已注册
	bool    inApply        = false;     // 重入守卫: 防 SetWindowPos(FRAMECHANGED) 触发的递归
	HWINDOW hWindow        = NULL;

	// ===== Snap 控制 (EnableSnap) =====
	// snapEnabled = true (默认) → 系统 snap 行为正常.
	// snapEnabled = false → *字面禁 snap*: strip WS_MAXIMIZEBOX (消除拖边
	//                       snap preview UI + 杀 Snap Layouts 飞出框, 副作用
	//                       是按钮变灰) + WM_WINDOWPOSCHANGING 几何过滤兜底.
	//                       *不* 吞 SC_MAXIMIZE — 保留 Win+Up / API 最大化通路.
	bool    snapEnabled         = true;

	// ===== 最大化控制 (EnableMaximize) =====
	// maxEnabled = true (默认, 跟随窗口 WS_MAXIMIZEBOX 原始状态).
	// maxEnabled = false → strip WS_MAXIMIZEBOX (按钮变灰) + 吞 SC_MAXIMIZE.
	bool    maxEnabled          = true;
	// WS_MAXIMIZEBOX 共享状态 (snapEnabled=false || maxEnabled=false 任一为 true
	// 都 strip; 见 XBlur_UpdateMaxBoxState_Locked).
	bool    maxBoxSaved         = false;    // 是否已 strip WS_MAXIMIZEBOX
	bool    maxBoxOriginallySet = false;    // strip 前 WS_MAXIMIZEBOX 是否本来就置位
};

static std::mutex                     g_nativeFxMutex;
static std::map<HWND, NativeFxState>  g_nativeFxMap;

// 揭 cloak 定时器 ID. 数值需与 XCGUI 内部 timer ID 不撞 — XCGUI 用低位区,
// 这里选高位 magic 值. 150 ms 对 D2D 首帧足够 (D2D init + first FillRectangle
// 一般 < 100 ms).
static const UINT_PTR kCXBlurUncloakTimerId = (UINT_PTR)0xCB10A912;
static const UINT     kCXBlurUncloakDelayMs = 150;
static const DWORD    kDWMWA_CLOAK                     = 13;
static const DWORD    kDWMWA_WINDOW_CORNER_PREFERENCE  = 33;

// 真最大化才屏蔽 effective shadow. snap state 不主动判 — DWM 在 snap 下 by
// design 不画 drop shadow 也不读 corner pref, 我们无论怎么算, 表现都一样,
// 不必加 snap 检测逻辑 (实测 deferred reapply / NCRP override 均无效, 已删).
//   参考 MS 官方: Apply rounded corners in desktop apps
//     https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-rounded-corners
//     原文: "By design, apps are not rounded when maximized, snapped..."
static bool XBlur_IsHostMaximized(HWND raw){
	return raw && ::IsWindow(raw) && (::IsZoomed(raw) != FALSE);
}

// ---------------- Snap / 最大化 禁用机制 (相互独立) ---------------------------
//
// Win 没有 per-window "禁 snap" / "禁最大化" 的官方 API. 拆成两套独立开关,
// 共享 WS_MAXIMIZEBOX strip 状态 (XBlur_UpdateMaxBoxState_Locked 集中管理):
//
//   * EnableSnap(FALSE) — *字面意义* 禁 snap, 含拖边时的 snap preview UI:
//       - strip WS_MAXIMIZEBOX → 消除拖边 snap preview (浮出的半透蒙层 /
//                                  Snap Layouts 飞出框); 副作用: 最大化按钮
//                                  变灰 *不可点*.
//       - WM_WINDOWPOSCHANGING 几何过滤 → target rect 匹配 snap layout (左右
//                                          半屏 / 上下半屏 / 四角 / 全屏) →
//                                          设 SWP_NOMOVE | SWP_NOSIZE 阻止落位.
//                                          *兜底*, 即使 preview 漏出也拦住最
//                                          终落位.
//       - *不* 吞 SC_MAXIMIZE — 保留 *键盘 Win+Up* 和 *API 最大化*
//                                (ShowWindow(SW_MAXIMIZE) / SetWindowPlacement /
//                                 WS_MAXIMIZE 创建属性) 通路.
//
//     用 IsZoomed(hwnd) 在 WINDOWPOSCHANGING 时区分 "真最大化" (放行) vs
//     "snap 全屏" (拦截): Win32 派发本消息之前已更新 WINDOWPLACEMENT.showCmd,
//     IsZoomed=true 时跳过过滤. 覆盖所有真最大化路径.
//
//   * EnableMaximize(FALSE) — 禁最大化:
//       - strip WS_MAXIMIZEBOX → 按钮变灰 (与 EnableSnap 共享 strip 状态).
//       - WM_SYSCOMMAND 吞 SC_MAXIMIZE → 拦键盘 Win+Up + 双击标题栏 + 系统
//                                          菜单 "最大化".
//       (API 路径 ShowWindow(SW_MAXIMIZE) 不走 SYSCOMMAND, 本接口拦不住 —
//        想完全禁 API 路径请调用方层面控制.)
//
// 两套接口可任意组合:
//   EnableSnap(FALSE) + EnableMaximize(TRUE)  ← 字面禁 snap, 保留 Win+↑/API
//                                                 最大化 (按钮灰, 此为典型).
//   EnableSnap(FALSE) + EnableMaximize(FALSE) ← snap+最大化都禁 (API 仍能用).
//   EnableSnap(TRUE)  + EnableMaximize(FALSE) ← 允许 snap 但禁最大化, 罕见.
//   EnableSnap(TRUE)  + EnableMaximize(TRUE)  ← 默认, 系统行为.

// 检测 wp 的目标矩形是否是 snap target geometry.
// 用容差 2 px 防 DPI rounding 误差. 仅匹配 snap layout 标准位置 (左/右半屏 /
// 上/下半屏 / 四角 / 全屏), 用户手动恰好 resize 到 snap 尺寸 *会* 误伤 (但概率
// 极低; 容差小 + 同时要求 4 边对齐).
static bool XBlur_IsSnapTargetGeom(const WINDOWPOS* wp, HWND raw){
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

// strip / restore WS_MAXIMIZEBOX. 持锁 + 锁外 两段:
//   *_Locked  阶段: 仅 GetWindowLong / SetWindowLong (本身不派发消息), 持锁安全.
//             返回是否需要锁外 SetWindowPos(SWP_FRAMECHANGED) 通知 USER32 重画
//             标题栏按钮.
//   锁外 NotifyFrameChanged: SetWindowPos *同步派发* WM_WINDOWPOSCHANGING 回到
//             本 wndproc — 必须在锁外调用, 否则二次 lock g_nativeFxMutex 死锁,
//             DWM hang detection 打 0xc000041d.
static bool XBlur_StripMaxBox_Locked(HWND raw, NativeFxState& st){
	if (st.maxBoxSaved) return false;
	LONG s = ::GetWindowLongW(raw, GWL_STYLE);
	st.maxBoxOriginallySet = (s & WS_MAXIMIZEBOX) != 0;
	st.maxBoxSaved = true;
	if (!st.maxBoxOriginallySet) return false;
	::SetWindowLongW(raw, GWL_STYLE, s & ~WS_MAXIMIZEBOX);
	return true;   // 需锁外 NotifyFrameChanged
}

static bool XBlur_RestoreMaxBox_Locked(HWND raw, NativeFxState& st){
	if (!st.maxBoxSaved) return false;
	bool wasSet = st.maxBoxOriginallySet;
	st.maxBoxSaved         = false;
	st.maxBoxOriginallySet = false;
	if (!wasSet) return false;
	LONG s = ::GetWindowLongW(raw, GWL_STYLE);
	::SetWindowLongW(raw, GWL_STYLE, s | WS_MAXIMIZEBOX);
	return true;   // 需锁外 NotifyFrameChanged
}

// 锁外: 通知 USER32 frame style 变了, 重画标题栏按钮.
// 同步派发 WM_WINDOWPOSCHANGING / WM_WINDOWPOSCHANGED / WM_NCCALCSIZE 给 wndproc.
static void XBlur_NotifyFrameChanged(HWND raw){
	::SetWindowPos(raw, NULL, 0, 0, 0, 0,
	               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
	               SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

// WS_MAXIMIZEBOX 的实际目标状态 = !snapEnabled || !maxEnabled.
// 即 EnableSnap(FALSE) 和 EnableMaximize(FALSE) 任一为禁用都 strip.
//   - EnableSnap(FALSE)  : strip 是 "字面禁 snap" 的核心手段 (消除拖边 snap
//                          preview UI + 杀 Snap Layouts 飞出框). 副作用是
//                          最大化按钮变灰, 但 Win+↑ 和 API 路径仍能最大化
//                          (此函数 *不* 影响 SC_MAXIMIZE 吞 / 几何过滤).
//   - EnableMaximize(FALSE): strip 让最大化按钮变灰, 配合 WM_SYSCOMMAND 吞
//                            SC_MAXIMIZE 一起实现 "禁最大化".
// 返回是否需要锁外 NotifyFrameChanged.
static bool XBlur_UpdateMaxBoxState_Locked(HWND raw, NativeFxState& st){
	bool wantStrip = !st.snapEnabled || !st.maxEnabled;
	if (wantStrip && !st.maxBoxSaved)  return XBlur_StripMaxBox_Locked(raw, st);
	if (!wantStrip && st.maxBoxSaved)  return XBlur_RestoreMaxBox_Locked(raw, st);
	return false;
}

// 算 effective frame/corner → 调 DWM. 调用方持锁.
// 返回 corner 的 hr (用于 EnableNativeRoundedCorner 透传 OS 兼容性).
//
// shadow + frame:
//   shadowEnabled && !inMaximized → margins={1,1,1,1} 触发 DWM frame + shadow.
//   否则 → margins=0 (DWM 不画 frame, 走默认渲染).
// corner:
//   cornerPref >= 0 → 设 DWMWCP_*. 最大化时强制 DONOTROUND (Win11 by design).
//   snap 时 DWM 也不读这个值, 但我们仍然 set (空操作 + 离开 snap 立刻生效).
static HRESULT XBlur_ApplyHostFx_Locked(HWND raw, NativeFxState& st){
	bool effectiveShadow = st.shadowEnabled && !st.inMaximized;
	MARGINS m = effectiveShadow ? MARGINS{1, 1, 1, 1} : MARGINS{0, 0, 0, 0};
	::DwmExtendFrameIntoClientArea(raw, &m);

	HRESULT hrCorner = S_OK;
	if (st.cornerPref >= 0){
		DWORD pref = st.inMaximized
		                ? (DWORD)xblur_corner_donotround
		                : (DWORD)st.cornerPref;
		hrCorner = ::DwmSetWindowAttribute(raw, kDWMWA_WINDOW_CORNER_PREFERENCE,
		                                   &pref, sizeof(pref));
	}
	return hrCorner;
}

// 把 ApplyHostFx 之后, 强制 DWM/USER32 重算 frame & 重新决定阴影.
//   * SetWindowPos(SWP_FRAMECHANGED | NOMOVE | NOSIZE): 让 USER32 重发
//     WM_NCCALCSIZE → DWM 顺势刷新 frame tracking, 保证 snap / DPI 切换后
//     窗外阴影立刻回来.
//   * NOMOVE | NOSIZE 保证不真改大小, 不引发 WM_SIZE 递归.
//   * 但 SetWindowPos 仍会发 WM_WINDOWPOSCHANGING / WM_WINDOWPOSCHANGED,
//     调用方需用 inApply 守卫防止 WM_WINDOWPOSCHANGED 处理器再回来递归.
static void XBlur_RefreshFrame(HWND raw){
	::SetWindowPos(raw, NULL, 0, 0, 0, 0,
	               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
	               SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

// WM_SIZE / WM_WINDOWPOSCHANGED / WM_DPICHANGED 共享的同步路径.
// 调用方 *无* 持锁. 内部自管锁 + inApply 守卫.
static void XBlur_SyncHost(HWND raw){
	bool needRefresh = false;
	{
		std::lock_guard<std::mutex> lk(g_nativeFxMutex);
		auto it = g_nativeFxMap.find(raw);
		if (it == g_nativeFxMap.end()) return;
		if (it->second.inApply) return;             // 重入守卫
		it->second.inApply     = true;
		it->second.inMaximized = XBlur_IsHostMaximized(raw);
		XBlur_ApplyHostFx_Locked(raw, it->second);
		// 仅当 shadow 应该出现时才发 SWP_FRAMECHANGED — 避免 max/无 fx 态白发消息.
		needRefresh = it->second.shadowEnabled && !it->second.inMaximized;
	}
	if (needRefresh){
		XBlur_RefreshFrame(raw);
	}
	{
		std::lock_guard<std::mutex> lk(g_nativeFxMutex);
		auto it = g_nativeFxMap.find(raw);
		if (it != g_nativeFxMap.end()) it->second.inApply = false;
	}
}

static int CALLBACK _CXBlur_NativeFxWndProc(HWINDOW hWnd, UINT msg,
                                            WPARAM wParam, LPARAM lParam,
                                            BOOL* pbHandled)
{
	HWND raw = ::XWnd_GetHWND(hWnd);
	if (!raw) return 0;

	switch (msg){
	case WM_SIZE:
		// SIZE_MAXIMIZED / SIZE_RESTORED → inMaximized 状态切换 + ApplyHostFx.
		// 普通 resize → DWM 在 max/snap 过渡里会重置 frame extension, 必须每次都
		// 重 apply (2 个 DWM 调用, 微小开销).
		XBlur_SyncHost(raw);
		break;

	case WM_WINDOWPOSCHANGING: {
		// EnableSnap(FALSE) 时, 检测 wp 目标矩形是否是 snap target geometry —
		// 若是则覆盖 SWP_NOMOVE | SWP_NOSIZE 阻止落位 (Aero Snap 拖边的拦截点).
		// snapEnabled=true (默认) 时此分支零开销.
		//
		// *IsZoomed 跳过*: 真最大化 (SC_MAXIMIZE / ShowWindow(SW_MAXIMIZE) /
		// SetWindowPlacement / WS_MAXIMIZE 创建属性 / 拖到顶 snap-to-max) 的
		// WINDOWPOSCHANGING 几何也是全工作区, 与 "snap 全屏" 几何相同. Win32
		// 在派发本消息之前已更新 WINDOWPLACEMENT.showCmd → IsZoomed()=true,
		// 用这个区分 "用户最大化" (放行) vs "Aero Snap 落位" (过滤). IsZoomed
		// 是廉价 syscall, 仅在 snapBlocked 路径调一次.
		WINDOWPOS* wp = (WINDOWPOS*)lParam;
		if (!wp) break;
		bool snapBlocked = false;
		{
			std::lock_guard<std::mutex> lk(g_nativeFxMutex);
			auto it = g_nativeFxMap.find(raw);
			if (it != g_nativeFxMap.end() && !it->second.snapEnabled){
				snapBlocked = true;
			}
		}
		if (snapBlocked && !::IsZoomed(raw) && XBlur_IsSnapTargetGeom(wp, raw)){
			wp->flags |= SWP_NOMOVE | SWP_NOSIZE;
		}
		break;
	}

	case WM_WINDOWPOSCHANGED: {
		// snap 完成 / restore-from-snap 部分场景只发 WM_WINDOWPOSCHANGED 不发
		// WM_SIZE (CXShadow 早有此观察). 这里只在 size 真变了才 react —— 避免
		// 拖动期 spam (WM_WINDOWPOSCHANGED 每像素都触发, 不能盲应用).
		WINDOWPOS* wp = (WINDOWPOS*)lParam;
		if (wp && (wp->flags & SWP_NOSIZE) == 0){
			XBlur_SyncHost(raw);
		}
		break;
	}

	case WM_SYSCOMMAND: {
		// EnableMaximize(FALSE) → 吞 SC_MAXIMIZE (Win+Up / 双击标题栏 / 系统菜单).
		// 默认 (maxEnabled=true) 此分支零开销.
		// 注意: 不再设 maximizing 暂态 — WINDOWPOSCHANGING 直接靠 IsZoomed 区分.
		if ((wParam & 0xFFF0) == SC_MAXIMIZE){
			bool maxBlocked = false;
			{
				std::lock_guard<std::mutex> lk(g_nativeFxMutex);
				auto it = g_nativeFxMap.find(raw);
				if (it != g_nativeFxMap.end() && !it->second.maxEnabled){
					maxBlocked = true;
				}
			}
			if (maxBlocked){
				if (pbHandled) *pbHandled = TRUE;
				return 0;
			}
		}
		break;
	}

	case WM_DPICHANGED:
		// DPI 切换会让 DWM 完全重算 frame, 保险再 sync 一次.
		XBlur_SyncHost(raw);
		break;

	case WM_TIMER:
		if (wParam == kCXBlurUncloakTimerId){
			::KillTimer(raw, kCXBlurUncloakTimerId);
			std::lock_guard<std::mutex> lk(g_nativeFxMutex);
			auto it = g_nativeFxMap.find(raw);
			if (it != g_nativeFxMap.end() && it->second.cloakRequested){
				BOOL cloak = FALSE;
				::DwmSetWindowAttribute(raw, kDWMWA_CLOAK, &cloak, sizeof(cloak));
				it->second.cloakRequested = false;
			}
		}
		break;

	case WM_DESTROY: {
		// XCGUI 在窗口销毁时会自动清自己的事件订阅表 (见 CXShadow 注释), 这里
		// 只清理 map. KillTimer 防御性兜底, 已销毁时它静默 no-op.
		::KillTimer(raw, kCXBlurUncloakTimerId);
		std::lock_guard<std::mutex> lk(g_nativeFxMutex);
		g_nativeFxMap.erase(raw);
		break;
	}
	}
	return 0;  // 不拦截, 让 XCGUI 继续派发
}

}  // anonymous namespace

//============================================================================
// EnableNativeShadow(HWINDOW, BOOL)
//============================================================================
BOOL CXBlur::EnableNativeShadow(HWINDOW hWnd, BOOL bEnable){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;
	HWND raw = (HWND)::XWnd_GetHWND(hWnd);
	if (!raw) return FALSE;

	bool needHookInstall = false;
	{
		std::lock_guard<std::mutex> lk(g_nativeFxMutex);
		auto& st = g_nativeFxMap[raw];
		st.shadowEnabled = (bEnable != FALSE);
		st.inMaximized   = XBlur_IsHostMaximized(raw);
		if (!st.hookInstalled){
			st.hookInstalled = true;
			st.hWindow       = hWnd;
			needHookInstall  = true;
		}
		XBlur_ApplyHostFx_Locked(raw, st);

		// Cloak 启动 — bug #3 修复. 仅在 (开启 shadow) + (窗未可见) + (尚未 cloak)
		// 时. cloak 失败 (老 OS / DWM 没启用) 不算错, 跳过.
		if (bEnable && !::IsWindowVisible(raw) && !st.cloakRequested){
			BOOL cloak = TRUE;
			HRESULT hrCloak = ::DwmSetWindowAttribute(raw, kDWMWA_CLOAK,
			                                          &cloak, sizeof(cloak));
			if (SUCCEEDED(hrCloak)){
				st.cloakRequested = true;
				::SetTimer(raw, kCXBlurUncloakTimerId, kCXBlurUncloakDelayMs, NULL);
			}
		}
	}

	// XWnd_RegEventC1 放锁外 — 避免 XCGUI 内部锁与本 mutex 之间形成顺序依赖.
	if (needHookInstall){
		::XWnd_RegEventC1(hWnd, XWM_WINDPROC, (void*)_CXBlur_NativeFxWndProc);
	}
	return TRUE;
}

//============================================================================
// EnableNativeRoundedCorner(HWINDOW, int)
//
// 通过状态机 ApplyHostFx 走 DwmSetWindowAttribute(DWMWA_WINDOW_CORNER_PREFERENCE = 33).
// Win11 21H2+ 接受, 老 OS 返 E_INVALIDARG, 函数透传为 FALSE.
// xblur_corner_* 枚举值与 DWM 原生 DWMWCP_* 二进制一致 (0/1/2/3).
//============================================================================
BOOL CXBlur::EnableNativeRoundedCorner(HWINDOW hWnd, int cornerStyle){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;
	HWND raw = (HWND)::XWnd_GetHWND(hWnd);
	if (!raw) return FALSE;

	bool    needHookInstall = false;
	HRESULT hrCorner        = S_OK;
	{
		std::lock_guard<std::mutex> lk(g_nativeFxMutex);
		auto& st = g_nativeFxMap[raw];
		st.cornerPref  = cornerStyle;
		st.inMaximized = XBlur_IsHostMaximized(raw);
		if (!st.hookInstalled){
			st.hookInstalled = true;
			st.hWindow       = hWnd;
			needHookInstall  = true;
		}
		hrCorner = XBlur_ApplyHostFx_Locked(raw, st);

		// 与 EnableNativeShadow 共享 cloak 路径: 用户启用任一 native fx 在窗未可见
		// 时, 都 cloak 到首帧.
		if (!::IsWindowVisible(raw) && !st.cloakRequested && (st.shadowEnabled || st.cornerPref >= 0)){
			BOOL cloak = TRUE;
			HRESULT hrCloak = ::DwmSetWindowAttribute(raw, kDWMWA_CLOAK,
			                                          &cloak, sizeof(cloak));
			if (SUCCEEDED(hrCloak)){
				st.cloakRequested = true;
				::SetTimer(raw, kCXBlurUncloakTimerId, kCXBlurUncloakDelayMs, NULL);
			}
		}
	}
	if (needHookInstall){
		::XWnd_RegEventC1(hWnd, XWM_WINDPROC, (void*)_CXBlur_NativeFxWndProc);
	}
	return SUCCEEDED(hrCorner) ? TRUE : FALSE;
}

//============================================================================
// EnableSnap(HWINDOW, BOOL)
//
// 启用 / 禁用本窗的 Aero Snap (默认启用). 实现策略详见 anonymous namespace
// "Snap / 最大化 禁用机制" 注释.
//
// bEnable=FALSE 是 *字面禁 snap*: strip WS_MAXIMIZEBOX (消除拖边时浮出的 snap
// preview UI / Snap Layouts 飞出框) + 几何过滤兜底. *不* 吞 SC_MAXIMIZE —
// 用户仍能通过 Win+↑ / ShowWindow(SW_MAXIMIZE) / SetWindowPlacement /
// WS_MAXIMIZE 创建属性 来最大化 (但标题栏最大化按钮变灰不可点).
//
// 想完全禁最大化, 配合 EnableMaximize(hWnd, FALSE).
//============================================================================
BOOL CXBlur::EnableSnap(HWINDOW hWnd, BOOL bEnable){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;
	HWND raw = (HWND)::XWnd_GetHWND(hWnd);
	if (!raw) return FALSE;

	bool needHookInstall  = false;
	bool needFrameChanged = false;
	{
		std::lock_guard<std::mutex> lk(g_nativeFxMutex);
		auto& st = g_nativeFxMap[raw];
		st.snapEnabled = (bEnable != FALSE);
		if (!st.hookInstalled){
			st.hookInstalled = true;
			st.hWindow       = hWnd;
			needHookInstall  = true;
		}
		needFrameChanged = XBlur_UpdateMaxBoxState_Locked(raw, st);
	}
	// SetWindowPos *必须* 在锁外 — 它同步派发 WM_WINDOWPOSCHANGING 回本 wndproc,
	// 持锁时回派会二次 lock g_nativeFxMutex 死锁.
	if (needFrameChanged){
		XBlur_NotifyFrameChanged(raw);
	}
	if (needHookInstall){
		::XWnd_RegEventC1(hWnd, XWM_WINDPROC, (void*)_CXBlur_NativeFxWndProc);
	}
	return TRUE;
}

//============================================================================
// EnableMaximize(HWINDOW, BOOL)
//
// 启用 / 禁用本窗的最大化能力 (默认启用, 跟随窗口 WS_MAXIMIZEBOX 原始状态).
//
// *bEnable=FALSE 双管齐下*:
//   1. strip WS_MAXIMIZEBOX → 标题栏最大化按钮变灰 + Snap Layouts 悬停
//                              飞出框消失.
//   2. WM_SYSCOMMAND 吞 SC_MAXIMIZE → 拦键盘 Win+Up + 双击标题栏 + 系统菜单
//                                       "最大化" + 程序化 ShowWindow(SW_MAXIMIZE).
//
// *bEnable=TRUE (从 FALSE 切回)*: 还原 WS_MAXIMIZEBOX 到 strip 前状态.
//
// *与 EnableSnap 完全独立*: 二者状态机共享同一 NativeFxState + hook, 但
// 互不影响. 默认场景 (snap+max 都允许) 是系统行为.
//============================================================================
BOOL CXBlur::EnableMaximize(HWINDOW hWnd, BOOL bEnable){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;
	HWND raw = (HWND)::XWnd_GetHWND(hWnd);
	if (!raw) return FALSE;

	bool needHookInstall  = false;
	bool needFrameChanged = false;
	{
		std::lock_guard<std::mutex> lk(g_nativeFxMutex);
		auto& st = g_nativeFxMap[raw];
		st.maxEnabled = (bEnable != FALSE);
		if (!st.hookInstalled){
			st.hookInstalled = true;
			st.hWindow       = hWnd;
			needHookInstall  = true;
		}
		needFrameChanged = XBlur_UpdateMaxBoxState_Locked(raw, st);
	}
	// SetWindowPos *必须* 在锁外 — 它同步派发 WM_WINDOWPOSCHANGING 回本 wndproc,
	// 在持锁状态下回派会二次 lock g_nativeFxMutex → 死锁 → DWM hang detection
	// → STATUS_FATAL_USER_CALLBACK_EXCEPTION (0xc000041d) 退出.
	if (needFrameChanged){
		XBlur_NotifyFrameChanged(raw);
	}
	if (needHookInstall){
		::XWnd_RegEventC1(hWnd, XWM_WINDPROC, (void*)_CXBlur_NativeFxWndProc);
	}
	return TRUE;
}

//============================================================================
// ForceSystemTransparencyOn(BOOL) - toggle 强制开启 / 还原 系统透明效果.
//   TRUE  → 保存老值, 写 EnableTransparency=1, 广播本进程, 注册 atexit/Ctrl 还原.
//   FALSE → 还原老值, 广播本进程. 若从未 force 过则 no-op.
//============================================================================
namespace {

static std::mutex          g_forceOnMutex;
static bool                g_forceOnApplied   = false;  // 我们写过 1
static bool                g_forceOnOldExists = false;  // 写之前键值存在过
static DWORD               g_forceOnOldValue  = 0;      // 写之前的老值
static bool                g_forceOnHooked    = false;  // atexit/Ctrl 已注册

// HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\EnableTransparency
static const wchar_t* kPersonalizePath =
	L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
static const wchar_t* kEnableTransparency = L"EnableTransparency";

// 读: outValue 仅在 outExists=true 时有效.
static void XBlur_ReadEnableTransparency(bool* outExists, DWORD* outValue){
	*outExists = false;
	*outValue  = 0;
	HKEY hk = NULL;
	if (::RegOpenKeyExW(HKEY_CURRENT_USER, kPersonalizePath, 0,
	                    KEY_QUERY_VALUE, &hk) != ERROR_SUCCESS) return;
	DWORD val = 0, sz = sizeof(val), type = 0;
	if (::RegQueryValueExW(hk, kEnableTransparency, NULL, &type,
	                       (LPBYTE)&val, &sz) == ERROR_SUCCESS && type == REG_DWORD){
		*outExists = true;
		*outValue  = val;
	}
	::RegCloseKey(hk);
}

// 写: 失败返回 false.
static bool XBlur_WriteEnableTransparency(DWORD value){
	HKEY hk = NULL;
	if (::RegCreateKeyExW(HKEY_CURRENT_USER, kPersonalizePath,
	        0, NULL, 0, KEY_SET_VALUE, NULL, &hk, NULL) != ERROR_SUCCESS){
		return false;
	}
	LSTATUS ls = ::RegSetValueExW(hk, kEnableTransparency, 0, REG_DWORD,
	                              (const BYTE*)&value, sizeof(value));
	::RegCloseKey(hk);
	return ls == ERROR_SUCCESS;
}

// 删: 失败 (不存在 / 没权限) 也不报.
static void XBlur_DeleteEnableTransparency(){
	HKEY hk = NULL;
	if (::RegOpenKeyExW(HKEY_CURRENT_USER, kPersonalizePath,
	                    0, KEY_SET_VALUE, &hk) != ERROR_SUCCESS) return;
	::RegDeleteValueW(hk, kEnableTransparency);
	::RegCloseKey(hk);
}

// 仅向本进程顶层窗口发 WM_SETTINGCHANGE(ImmersiveColorSet),
// 避免 HWND_BROADCAST 打扰其他 app.
static BOOL CALLBACK XBlur_EnumProcessWindowsCb(HWND hWnd, LPARAM lp){
	DWORD myPid = (DWORD)lp;
	DWORD winPid = 0;
	::GetWindowThreadProcessId(hWnd, &winPid);
	if (winPid == myPid){
		::SendMessageTimeoutW(hWnd, WM_SETTINGCHANGE, 0,
		                      (LPARAM)L"ImmersiveColorSet",
		                      SMTO_ABORTIFHUNG, 50, NULL);
	}
	return TRUE;
}

static void XBlur_BroadcastInProcess(){
	::EnumWindows(XBlur_EnumProcessWindowsCb, (LPARAM)::GetCurrentProcessId());
}

// 退出还原 (atexit / Console Ctrl handler 共用).
static void XBlur_RestoreOnExit(){
	std::lock_guard<std::mutex> lk(g_forceOnMutex);
	if (!g_forceOnApplied) return;
	g_forceOnApplied = false;
	if (g_forceOnOldExists){
		XBlur_WriteEnableTransparency(g_forceOnOldValue);
	} else {
		XBlur_DeleteEnableTransparency();
	}
	// atexit 时窗口可能已销毁, 不广播也无所谓.
}

static void XBlur_AtExitTrampoline(){
	XBlur_RestoreOnExit();
}

static BOOL WINAPI XBlur_ConsoleCtrlTrampoline(DWORD /*ctrlType*/){
	XBlur_RestoreOnExit();
	return FALSE;  // 让默认 handler 继续走 (默认会终止进程).
}

}  // namespace

void CXBlur::ForceSystemTransparencyOn(BOOL bForce){
	std::lock_guard<std::mutex> lk(g_forceOnMutex);

	if (bForce){
		if (g_forceOnApplied) return;  // 已开, 幂等返回.

		// 读老值. 若已是 1, 用户本来就开着 → 不掺和.
		XBlur_ReadEnableTransparency(&g_forceOnOldExists, &g_forceOnOldValue);
		if (g_forceOnOldExists && g_forceOnOldValue == 1){
			return;
		}

		if (!XBlur_WriteEnableTransparency(1)) return;
		g_forceOnApplied = true;

		// 注册退出还原 (一次性). atexit 和 Console Ctrl handler 双兜底.
		if (!g_forceOnHooked){
			g_forceOnHooked = true;
			std::atexit(XBlur_AtExitTrampoline);
			::SetConsoleCtrlHandler(XBlur_ConsoleCtrlTrampoline, TRUE);
		}

		XBlur_BroadcastInProcess();
	} else {
		// 还原.
		if (!g_forceOnApplied) return;
		g_forceOnApplied = false;
		if (g_forceOnOldExists){
			XBlur_WriteEnableTransparency(g_forceOnOldValue);
		} else {
			XBlur_DeleteEnableTransparency();
		}
		XBlur_BroadcastInProcess();
	}
}

// --- Invalidate: 仅重画 element 装饰层 ---
void CXBlur::Invalidate() { RedrawSelf(); }

// --- 圆角 / 边框 ---
void CXBlur::SetCornerRadius(int radius){
	int r = (std::max)(0, radius);
	m_cornerTL.store(r);
	m_cornerTR.store(r);
	m_cornerBR.store(r);
	m_cornerBL.store(r);
	RedrawSelf();
}
int  CXBlur::GetCornerRadius() const { return m_cornerTL.load(); }

void CXBlur::SetCornerRadiusEx(int lt, int rt, int rb, int lb){
	m_cornerTL.store((std::max)(0, lt));
	m_cornerTR.store((std::max)(0, rt));
	m_cornerBR.store((std::max)(0, rb));
	m_cornerBL.store((std::max)(0, lb));
	RedrawSelf();
}
void CXBlur::GetCornerRadiusEx(int* pLT, int* pRT, int* pRB, int* pLB) const {
	if (pLT) *pLT = m_cornerTL.load();
	if (pRT) *pRT = m_cornerTR.load();
	if (pRB) *pRB = m_cornerBR.load();
	if (pLB) *pLB = m_cornerBL.load();
}

void CXBlur::SetBorderColor(COLORREF color){ m_borderColor.store(color); RedrawSelf(); }
COLORREF CXBlur::GetBorderColor() const { return m_borderColor.load(); }

void CXBlur::SetBorderWidth(float w){ m_borderWidth.store((std::max)(0.0f, w)); RedrawSelf(); }
float CXBlur::GetBorderWidth() const { return m_borderWidth.load(); }
