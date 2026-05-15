//============================================================================
// module_xcgui_blur.cpp
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
// 系统 acrylic 启用路径 (运行时按 OS 能力降级):
//   1. Win10 1803+ / Win11: SetWindowCompositionAttribute
//                            ACCENT_ENABLE_ACRYLICBLURBEHIND  (真 acrylic + tint)
//   2. Win10 1607 ~ 1709 : ACCENT_ENABLE_BLURBEHIND           (Win10 风 blur)
//   3. Win7 / Win8 / 8.1 : 不启用 backdrop blur, CXBlur 退化为“仅装饰”.
//
// Win7 / Win8 / 8.1 不走 DwmEnableBlurBehindWindow + BLURREGION:
//   BLURREGION blur 生效要 host pixel alpha=0, XCGUI 渲染 pipeline
//   输出 alpha=255 → 出不了 blur. 变成仅装饰层 (tint+border).
//============================================================================

#include "module_xcgui_blur.h"

#include <dwmapi.h>
#include <algorithm>
#include <map>
#include <set>
#include <mutex>
#include <vector>
#include <atomic>

#pragma comment(lib, "dwmapi.lib")

//============================================================================
// 工具函数 / 颜色解码
//============================================================================
static inline BYTE GetRGBA_R(COLORREF c){ return (BYTE)((c) & 0xFF); }
static inline BYTE GetRGBA_G(COLORREF c){ return (BYTE)(((c) >> 8) & 0xFF); }
static inline BYTE GetRGBA_B(COLORREF c){ return (BYTE)(((c) >> 16) & 0xFF); }
static inline BYTE GetRGBA_A(COLORREF c){ return (BYTE)(((c) >> 24) & 0xFF); }
static inline COLORREF MakeRGBA(BYTE r, BYTE g, BYTE b, BYTE a){
	return (COLORREF)((a << 24) | (b << 16) | (g << 8) | r);
}

static inline float Clampf(float v, float lo, float hi){
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
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
	XBLUR_ACCENT_ENABLE_ACRYLICBLURBEHIND   = 4,  // Win11 风格 acrylic
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

namespace {

// host HWND 状态: 订阅的 element + hook 前的 WndProc + 标志位.
struct _XBlur_HostBlurState {
	std::set<HELE> subs;
	WNDPROC origWndProc = NULL;
	bool    enabled     = false;
	bool    destroying  = false;
};
static std::mutex                                g_hostBlurMutex;
static std::map<HWND, _XBlur_HostBlurState>      g_hostBlurMap;

// subclass WndProc:
//   WM_ERASEBKGND: 返回 1 阻止 GDI 擦背景 (否则盖掉 acrylic).
//   WM_NCDESTROY : 标 destroying, 让 release 跳过 disable accent
//                  (destroy 路径调 SetWindowCompositionAttribute(DISABLED)
//                  会触发画面定格 + 残留窗口).
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
	WNDPROC orig = NULL;
	{
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(h);
		if (it != g_hostBlurMap.end()) orig = it->second.origWndProc;
	}
	return orig ? CallWindowProcW(orig, h, m, w, l)
	            : DefWindowProcW(h, m, w, l);
}

// 给 host 应用 ACCENT_ACRYLIC. 调用方持锁.
// Win10 1803+: ACCENT_ENABLE_ACRYLICBLURBEHIND (state=4) = 真 acrylic.
// 老系统不支持时 fall back 到 ACCENT_ENABLE_BLURBEHIND (state=3) Win10 风格 blur.
static bool XBlur_ApplyAccentBlur(HWND host, DWORD accentState, DWORD tintRgba){
	HMODULE u32 = ::GetModuleHandleW(L"user32.dll");
	if (!u32) return false;
	auto pSet = (PFN_SetWindowCompositionAttribute)
		::GetProcAddress(u32, "SetWindowCompositionAttribute");
	if (!pSet) return false;
	_XBlur_ACCENT_POLICY policy = {};
	policy.AccentState   = accentState;
	policy.AccentFlags   = 0;  // 注: ACCENT_FLAGS 2 会去掉 GradientColor (无 tint)
	policy.GradientColor = tintRgba;  // 仅 ACRYLIC 用; BLURBEHIND 忽略
	policy.AnimationId   = 0;
	_XBlur_WCA_DATA data = {};
	data.Attribute = XBLUR_WCA_ACCENT_POLICY;
	data.pvData    = &policy;
	data.cbData    = sizeof(policy);
	return pSet(host, &data) ? true : false;
}

// 应用 host blur. 按 OS 能力降级:
//   1. Win10 1803+ : ACCENT_ENABLE_ACRYLICBLURBEHIND (真 acrylic + tint).
//   2. Win10 1607- : ACCENT_ENABLE_BLURBEHIND        (Win10 风格 blur, 无 tint).
//   3. 其他 OS    : 不走 DwmEnableBlurBehindWindow. 原因见文件头注释:
//                     XCGUI 渲染 pipeline 拉 alpha 为 255, BLURREGION 造不出
//                     glass; 只会警示并重绘 tint+border (装饰层).
static void XBlur_ApplyHostBlur_Locked(HWND host){
	auto it = g_hostBlurMap.find(host);
	if (it == g_hostBlurMap.end()) return;
	if (it->second.subs.empty()){
		XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_DISABLED, 0);
		it->second.enabled = false;
		return;
	}

	// 路径 1: ACCENT_ACRYLIC (Win10 1803+)
	if (XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_ENABLE_ACRYLICBLURBEHIND, 0)){
		it->second.enabled = true;
		return;
	}
	// 路径 2: ACCENT_BLURBEHIND (Win10 1607+)
	if (XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_ENABLE_BLURBEHIND, 0)){
		it->second.enabled = true;
		return;
	}
	// 路径 3: 退化为仅装饰层 (Vista / Win7 / Win8 / 8.1 等 SetWindowCompositionAttribute
	// 不可用的 OS). Aero 的 BLURREGION + alpha=0 机制需要 HWND surface 保留
	// 逐像素 alpha, 这与 XCGUI 默认渲染模式冲突 → blur 不会出现, 不上.
	it->second.enabled = false;
}

// 加 sub element 到 host 订阅集合, 第一次启用 + subclass WndProc, 然后应用 region.
static bool XBlur_AcquireHostBlur(HWND host, HELE hEle){
	if (!host || !::IsWindow(host) || !hEle) return false;
	std::lock_guard<std::mutex> lk(g_hostBlurMutex);
	_XBlur_HostBlurState& s = g_hostBlurMap[host];
	bool firstTime = s.subs.empty();
	s.subs.insert(hEle);
	if (firstTime){
		// subclass WndProc 处理 WM_ERASEBKGND.
		s.origWndProc = (WNDPROC)::SetWindowLongPtrW(host, GWLP_WNDPROC,
			(LONG_PTR)XBlur_HostSubclassWndProc);
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
		// 最后一个: host 还活着 → disable accent + 还原 WndProc.
		// host 销毁中 (destroying=true) → 跳过 disable, 让 DWM 自动清理,
		// 避免 SetWindowCompositionAttribute 在 destroy 路径触发画面定格.
		bool destroying = it->second.destroying;
		if (!destroying && ::IsWindow(host)){
			XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_DISABLED, 0);
			if (it->second.origWndProc){
				::SetWindowLongPtrW(host, GWLP_WNDPROC,
				                    (LONG_PTR)it->second.origWndProc);
			}
		}
		g_hostBlurMap.erase(it);
	} else {
		// 还有其他 sub → 重新应用 acrylic.
		XBlur_ApplyHostBlur_Locked(host);
	}
}

// 通知 host: 某个 sub 的 element 位置/大小变化. ACCENT 路径由 DWM 自己跟,
// 不需重算; 保留函数仅为 OnSizeImpl 预留 hook 点.
static void XBlur_UpdateHostBlur(HWND host){
	if (!host || !::IsWindow(host)) return;
	std::lock_guard<std::mutex> lk(g_hostBlurMutex);
	XBlur_ApplyHostBlur_Locked(host);
}

}  // anonymous namespace

//============================================================================
// 全局实例 + 主题预设 (DWM acrylic 路径下需要)
//============================================================================
namespace {

// 全局活实例 + 全局主题 (CXBlur::SetGlobalTheme). attach/detach 维护.
static std::mutex            g_blurInstancesMutex;
static std::set<CXBlur*>     g_blurInstances;
static std::atomic<int>      g_globalTheme{xblur_theme_custom};

// === 主题预设默认参数 (CXBlur::SetThemeDefault / GetThemeDefault) ===
// 用户调好后可 SetThemeDefault 保存, 后续 SetTheme(light/dark) 自动用此值.
// DWM 路径下只剩 tintColor + noise + cornerRadius + border* 有效, 其他字段
// 已废弃 (DWM acrylic 算法不让应用层控制 blurRadius/saturation/brightness/contrast).
static std::mutex            g_themeDefaultsMutex;
static CXBlurThemeDefaults   g_lightDefaults = {
	/*tintColor */ 0x33FCFCFC,   // 0xAABBGGRR: A=51 (20%), B=G=R=252
	/*noise     */ 0.06f
};
static CXBlurThemeDefaults   g_darkDefaults = {
	/*tintColor */ 0x40161414,   // 0xAABBGGRR: A=64 (25%), B=22, G=20, R=20
	/*noise     */ 0.06f
};

}  // anonymous namespace

//============================================================================
// CXBlur 构造 / 析构
//============================================================================
CXBlur::CXBlur(){
	// DWM 接管 blur 强度/饱和/亮度/对比度, 我们只控 tint + noise + 边框等装饰层.
	m_tintColor.store(MakeRGBA(252, 252, 252, 51));    // 20% 白 tint (acrylic 标准)
	m_theme.store(xblur_theme_custom);
	m_noise.store(0.06f);                              // 6% 灰度噪点纹理
	m_cornerTL.store(0);
	m_cornerTR.store(0);
	m_cornerBR.store(0);
	m_cornerBL.store(0);
	m_borderColor.store(0);
	m_borderWidth.store(0.0f);
}

CXBlur::~CXBlur(){
	if (XC_IsHELE((HXCGUI)m_hEle)){
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

	RECT rcCli;
	XWnd_GetClientRect(hWnd, &rcCli);
	int cx = rcCli.right  - rcCli.left;
	int cy = rcCli.bottom - rcCli.top;

	HELE hEle = XEle_Create(0, 0, cx, cy, (HXCGUI)hWnd);
	if (!hEle) return FALSE;

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
// Detach
//============================================================================
void CXBlur::Detach(){
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
	HWND host = FindHostHwnd();
	if (host){
		m_hostHwnd = host;  // 记录 host, Detach 时释放
		if (XC_IsEnableD2D()){
			m_acrylicApplied = XBlur_AcquireHostBlur(host, hEle);
		}
	}

	// 注册到全局活实例集合, 让 SetGlobalTheme 能广播到本实例.
	{
		std::lock_guard<std::mutex> lk(g_blurInstancesMutex);
		g_blurInstances.insert(this);
	}
	// 如果用户在 attach 之前已 SetGlobalTheme(...), 默认采用之.
	int gt = g_globalTheme.load();
	if (gt != xblur_theme_custom){
		m_theme.store(gt);
		ApplyThemePreset(gt);
	}

	RedrawSelf();
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

	if (m_attachToWindow && m_attachedWnd){
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
static std::mutex                    g_hostMutex;
static std::map<HWND, _CXBlurHostInfo> g_hostMap;

// 快照订阅者 (调用者不持锁遵循, 避免回调里反咬锁)
static std::vector<CXBlur*> SnapshotSubs(HWND raw) {
	std::vector<CXBlur*> v;
	std::lock_guard<std::mutex> lk(g_hostMutex);
	auto it = g_hostMap.find(raw);
	if (it != g_hostMap.end()) v.assign(it->second.subs.begin(), it->second.subs.end());
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
	HWND raw = XWnd_GetHWND(hWnd);
	if (!raw) return 0;
	auto subs = SnapshotSubs(raw);
	for (auto* s : subs){
		s->OnWndSettingChangeImpl(hWnd, uFlags, pStr, pbHandled);
	}
	return 0;
}
static int CALLBACK _CXBlur_GlobalWndSizeCB(HWINDOW hWnd, UINT nFlags, SIZE* pSize, BOOL* pbHandled){
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
	if (!hxw) return;
	HWND raw = XWnd_GetHWND(hxw);
	if (!raw) return;

	bool needRegSetting = false;
	{
		std::lock_guard<std::mutex> lk(g_hostMutex);
		auto& info = g_hostMap[raw];
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
	if (!hxw) return;
	HWND raw = XWnd_GetHWND(hxw);
	if (!raw) return;

	bool needUnregSetting = false;
	bool needUnregSize    = false;
	{
		std::lock_guard<std::mutex> lk(g_hostMutex);
		auto it = g_hostMap.find(raw);
		if (it != g_hostMap.end()){
			it->second.subs.erase(this);
			if (it->second.subs.empty()){
				needUnregSetting = it->second.regSetting;
				needUnregSize    = it->second.regSize;
				g_hostMap.erase(it);
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
	HWND raw = XWnd_GetHWND(hWnd);
	if (!raw) return;

	bool needRegSize = false;
	{
		std::lock_guard<std::mutex> lk(g_hostMutex);
		auto& info = g_hostMap[raw];
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
	if (!hxw) return NULL;
	HWND raw = XWnd_GetHWND(hxw);
	if (!raw) return NULL;
	return raw;
}

//============================================================================
// 元素事件实现
//============================================================================
int CXBlur::OnPaintImpl(HELE /*hEle*/, HDRAW hDraw, BOOL* pbHandled){
	if (!hDraw) return 0;

	BOOL useD2D = XC_IsEnableD2D();
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
	return 0;
}

int CXBlur::OnWndSettingChangeImpl(HWINDOW /*hWnd*/, UINT /*uFlags*/, void* /*pStr*/, BOOL* /*pbHandled*/){
	// pStr 是变化类型字符串 (e.g. "ImmersiveColorSet"). 任何系统设置变化都重新
	// 跑一遍 theme auto: 这里会按当前 AppsUseLightTheme 刷新 tint.
	if (m_theme.load() == xblur_theme_auto){
		ApplyThemePreset(xblur_theme_auto);
	}
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

	// QI ID2D1DeviceContext: 比 base RT 多 effect graph + COPY blend, 是让
	// ACCENT_ACRYLIC 实际透出 backdrop 的前提 (base RT 默认 SOURCE_OVER 写
	// alpha=255 会把 backdrop 整个盖掉).
	static const GUID IID_ID2D1DeviceContext_local =
		{ 0xe8f7fe7a, 0x191c, 0x466d, { 0xad, 0x95, 0x97, 0x56, 0x78, 0xbd, 0xa9, 0x98 } };
	ID2D1DeviceContext* dc = NULL;
	bool haveDc = SUCCEEDED(rt->QueryInterface(IID_ID2D1DeviceContext_local, (void**)&dc)) && dc;

	ID2D1Factory* pFac = NULL;
	rt->GetFactory(&pFac);

	COLORREF tc = m_tintColor.load();
	BYTE ta = GetRGBA_A(tc);
	if (haveDc && pFac){
		// 一次构造圆角几何, fill / clip / 都共用
		ID2D1Geometry* pGeom = XBlur_CreateCornerGeometry(pFac, rfEle, tl, tr, br, bl);

		// 1) COPY blend 把 element 区域像素 alpha 设成 tint.alpha (~51 / 20%):
		//    displayed = tint_rgb*α + backdrop_blur*(1-α), 经典 Win11 acrylic 公式.
		ID2D1SolidColorBrush* pTint = NULL;
		dc->CreateSolidColorBrush(
			D2D1::ColorF(GetRGBA_R(tc) / 255.0f, GetRGBA_G(tc) / 255.0f,
			             GetRGBA_B(tc) / 255.0f, ta / 255.0f),
			&pTint);
		if (pTint && pGeom){
			D2D1_PRIMITIVE_BLEND oldBlend = dc->GetPrimitiveBlend();
			dc->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
			dc->FillGeometry(pGeom, pTint);
			dc->SetPrimitiveBlend(oldBlend);
		}
		SafeRelease(pTint);

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
				D2D1::ColorF(GetRGBA_R(tc) / 255.0f, GetRGBA_G(tc) / 255.0f,
				             GetRGBA_B(tc) / 255.0f, ta / 255.0f),
				&pTint);
			if (SUCCEEDED(hr) && pTint){
				rt->FillRectangle(rfEle, pTint);
				pTint->Release();
			}
		}
	}

	// 边框: 路径中线沿 element 内侧 inset = bw/2, 圆角半径同步缩 bw/2 防角度偏移.
	COLORREF bc = m_borderColor.load();
	BYTE ba = GetRGBA_A(bc);
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
			D2D1::ColorF(GetRGBA_R(bc) / 255.0f, GetRGBA_G(bc) / 255.0f,
			             GetRGBA_B(bc) / 255.0f, ba / 255.0f),
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
	BYTE ta = GetRGBA_A(tc);
	if (ta != 0){
		Gdiplus::Graphics g(hdc);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		Gdiplus::SolidBrush br_(
			Gdiplus::Color(ta, GetRGBA_R(tc), GetRGBA_G(tc), GetRGBA_B(tc)));
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
	BYTE ba = GetRGBA_A(bc);
	float bw = m_borderWidth.load();
	if (ba != 0 && bw > 0.0f){
		Gdiplus::Graphics g(hdc);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		Gdiplus::Pen pen(
			Gdiplus::Color(ba, GetRGBA_R(bc), GetRGBA_G(bc), GetRGBA_B(bc)),
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
	HKEY hk = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER,
	                  L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
	                  0, KEY_READ, &hk) != ERROR_SUCCESS) return FALSE;
	DWORD val = 1, sz = sizeof(val);
	BOOL dark = FALSE;
	if (RegQueryValueExW(hk, L"AppsUseLightTheme", NULL, NULL,
	                     (LPBYTE)&val, &sz) == ERROR_SUCCESS){
		dark = (val == 0);
	}
	RegCloseKey(hk);
	return dark;
}

void CXBlur::ApplyThemePreset(int theme){
	if (theme == xblur_theme_auto){
		ApplyThemePreset(IsSystemDarkMode() ? xblur_theme_dark : xblur_theme_light);
		return;
	}
	if (theme != xblur_theme_light && theme != xblur_theme_dark){
		// custom / 其他 = 不应用任何预设, 保留用户已 Set... 的参数.
		return;
	}
	CXBlurThemeDefaults d;
	{
		std::lock_guard<std::mutex> lk(g_themeDefaultsMutex);
		d = (theme == xblur_theme_light) ? g_lightDefaults : g_darkDefaults;
	}
	m_tintColor.store(d.tintColor);
	m_noise    .store(d.noise);
	// tint 变了只影响 element 装饰层 (host acrylic 使用固定的 DWM tint).
	RedrawSelf();
}

// === SetThemeDefault / GetThemeDefault ===
// 修改 light/dark 主题预设的默认参数. 后续 SetTheme(...) 用新值.
// 不立即触发 redraw - 用户期望"先调好默认值再 SetTheme 应用".
void CXBlur::SetThemeDefault(int theme, const CXBlurThemeDefaults& d){
	if (theme != xblur_theme_light && theme != xblur_theme_dark) return;
	std::lock_guard<std::mutex> lk(g_themeDefaultsMutex);
	if (theme == xblur_theme_light) g_lightDefaults = d;
	else                            g_darkDefaults  = d;
}
CXBlurThemeDefaults CXBlur::GetThemeDefault(int theme){
	std::lock_guard<std::mutex> lk(g_themeDefaultsMutex);
	// auto 按系统当前选 light/dark; 其他无效值返回 light 作为安全默认.
	if (theme == xblur_theme_auto)
		return IsSystemDarkMode() ? g_darkDefaults : g_lightDefaults;
	return (theme == xblur_theme_dark) ? g_darkDefaults : g_lightDefaults;
}

//============================================================================
// 公开 setter / getter
//============================================================================
int CXBlur::GetBindMode() const {
	if (!XC_IsHELE((HXCGUI)m_hEle)) return xblur_bind_none;
	if (m_attachToWindow) return xblur_bind_window;
	return m_owned ? xblur_bind_owned : xblur_bind_attach;
}

// --- 叠加色 ---
void CXBlur::SetTintColor(COLORREF color){
	m_tintColor.store(color);
	RedrawSelf();
}
COLORREF CXBlur::GetTintColor() const { return m_tintColor.load(); }

// --- Theme ---
void CXBlur::SetTheme(int theme){
	m_theme.store(theme);
	ApplyThemePreset(theme);
	RedrawSelf();
}
int  CXBlur::GetTheme() const { return m_theme.load(); }

// SetGlobalTheme 实现: 同步到所有活实例 + 写入全局默认 (供后创建的实例读).
void CXBlur::SetGlobalTheme(int theme){
	g_globalTheme.store(theme);
	std::lock_guard<std::mutex> lk(g_blurInstancesMutex);
	for (CXBlur* p : g_blurInstances){
		if (p) p->SetTheme(theme);
	}
}
int CXBlur::GetGlobalTheme(){
	return g_globalTheme.load();
}

void  CXBlur::SetNoise(float a){ m_noise.store(Clampf(a, 0.0f, 1.0f)); RedrawSelf(); }
float CXBlur::GetNoise() const { return m_noise.load(); }

// --- 能力查询 ---
// 只要 DWM 合成启用 (Vista+) 就认为支持; 实际路线选择在 attach 时 OS 能力降级.
// Win8/8.1 上 SetWindowCompositionAttribute / DwmEnableBlurBehindWindow 都返回透明
// 无 blur, 这里我们仍返 TRUE (CXBlur 会自动退化为仅装饰, 不报错).
BOOL CXBlur::IsSystemAcrylicSupported(){
	BOOL enabled = FALSE;
	return (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled) ? TRUE : FALSE;
}
BOOL CXBlur::IsSystemAcrylicEnabled() const { return m_acrylicApplied ? TRUE : FALSE; }

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
