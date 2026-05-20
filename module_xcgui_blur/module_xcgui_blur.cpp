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
// 系统 acrylic 启用路径 (运行时按 OS build number 显式选, 见 XBlur_PickAccentState):
//   1. Win11 (>= 22000)               : ACCENT_ENABLE_ACRYLICBLURBEHIND (真 acrylic)
//   2. Win10 1803~1809 (17134~17763)  : ACCENT_ENABLE_ACRYLICBLURBEHIND (真 acrylic)
//   3. Win10 1903~22H2 (18362~21999)  : ACCENT_ENABLE_BLURBEHIND        (绕开 ACRYLIC 阉割)
//   4. Win10 1607~1709 (14393~16299)  : ACCENT_ENABLE_BLURBEHIND        (Win10 风 blur)
//   5. Win7 / Win8 / 8.1 / 老 Win10   : 不启用 backdrop blur, CXBlur 退化为"仅装饰".
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

// 用 RtlGetVersion 拿真实 OS build (GetVersionEx 在没 manifest 的进程上会撒谎封顶在 6.2).
// 缓存一次, 进程内不变.
static DWORD XBlur_GetOsBuild(){
	static DWORD s_build = 0;
	if (s_build) return s_build;
	HMODULE nt = ::GetModuleHandleW(L"ntdll.dll");
	if (!nt){ s_build = 1; return s_build; }
	typedef LONG (WINAPI* PFN_RtlGetVersion)(OSVERSIONINFOEXW*);
	auto pRtl = (PFN_RtlGetVersion)::GetProcAddress(nt, "RtlGetVersion");
	if (!pRtl){ s_build = 1; return s_build; }
	OSVERSIONINFOEXW vi = {};
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (pRtl(&vi) != 0){ s_build = 1; return s_build; }
	s_build = vi.dwBuildNumber ? vi.dwBuildNumber : 1;
	return s_build;
}
	
// 按 OS build 挑合适的 ACCENT_STATE.
//
// Win10 1903 (build 18362) 起微软 *阉割* 了 ACCENT_ENABLE_ACRYLICBLURBEHIND:
// SetWindowCompositionAttribute 仍返 TRUE, 但 DWM 不再跑 blur kernel, 只剩
// 透明 + tint, 而且 resize/move 时窗口刷新有 ~500ms~1s 延迟. 这是系统级
// regression, Win10 22H2 仍未修. 修复策略: 该 build 段降级到 BLURBEHIND
// (state=3, Win10 Aero 风格 blur), 它在 22H2 上仍能跑真 blur 且无明显延迟.
//
// build 表:
//   >= 22000           Win11+      → ACRYLIC (真 acrylic)
//   17134 ~ 17763      Win10 1803~1809 → ACRYLIC (真 acrylic)
//   18362 ~ 21999      Win10 1903~22H2 → BLURBEHIND (绕开阉割)
//   14393 ~ 16299      Win10 1607~1709 → BLURBEHIND (无 acrylic)
//   < 14393            Win10 < 1607 / Win8 / 8.1 / Win7 → DISABLED (装饰层)
//
// 返 0 表示走装饰层不调 SetWindowCompositionAttribute.
static DWORD XBlur_PickAccentState(){
	DWORD b = XBlur_GetOsBuild();
	if (b >= 22000)              return XBLUR_ACCENT_ENABLE_ACRYLICBLURBEHIND;
	if (b >= 17134 && b <= 17763) return XBLUR_ACCENT_ENABLE_ACRYLICBLURBEHIND;
	if (b >= 18362)              return XBLUR_ACCENT_ENABLE_BLURBEHIND;  // 22H2 等
	if (b >= 14393)              return XBLUR_ACCENT_ENABLE_BLURBEHIND;  // 1607~1709
	return 0;
}

// host HWND 状态: 订阅的 element + hook 前的 WndProc + 标志位.
struct _XBlur_HostBlurState {
	std::set<HELE> subs;
	WNDPROC origWndProc  = NULL;
	bool    enabled      = false;
	bool    destroying   = false;
	bool    pendingApply = false;  // 待首次 WM_PAINT 后再装 accent (冷启动防 flash)
};
static std::mutex                                g_hostBlurMutex;
static std::map<HWND, _XBlur_HostBlurState>      g_hostBlurMap;

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
		std::lock_guard<std::mutex> lk(g_hostBlurMutex);
		auto it = g_hostBlurMap.find(h);
		if (it != g_hostBlurMap.end() && !it->second.destroying){
			XBlur_ApplyHostBlur_Locked(h);
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

// 调 SetWindowCompositionAttribute 给 host 装一个 ACCENT_STATE.
// state 由 XBlur_PickAccentState 决定; tintRgba 仅 ACRYLIC 用, BLURBEHIND 忽略.
// 调用方持 g_hostBlurMutex 锁 (修改 g_hostBlurMap 的路径).
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

// 应用 host blur. 按 XBlur_PickAccentState 决定的 state 装. 不再"先 ACRYLIC
// 再 BLURBEHIND" 试探 — Win10 1903+ ACRYLIC 调用会成功返 TRUE 但不出 blur,
// 试探机制无法察觉, 必须按 build number 直接选.
static void XBlur_ApplyHostBlur_Locked(HWND host){
	auto it = g_hostBlurMap.find(host);
	if (it == g_hostBlurMap.end()) return;
	if (it->second.subs.empty()){
		XBlur_ApplyAccentBlur(host, XBLUR_ACCENT_DISABLED, 0);
		it->second.enabled = false;
		return;
	}

	DWORD state = XBlur_PickAccentState();
	if (state == 0){
		// 老 OS (Win8/8.1/Win7) / 不支持 SetWindowCompositionAttribute → 装饰层.
		// XCGUI 渲染 pipeline 拉 alpha 为 255, Aero BLURREGION 造不出 glass.
		it->second.enabled = false;
		return;
	}
	if (XBlur_ApplyAccentBlur(host, state, 0)){
		it->second.enabled = true;
		return;
	}
	it->second.enabled = false;
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
int CXBlur::OnPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled){
	if (!hDraw) return 0;

	// === CXShadow 兼容性 runtime 检测 ===========================================
	// 场景: 用户先 CXBlur::AttachToEle (装上 DWM ACCENT_ACRYLIC) → 之后 CXShadow::
	// AttachToWnd 把宿主窗切到 window_transparent_shaped. 两个模块都在用宿主窗的
	// alpha 通道, 但语义完全冲突:
	//   * shaped 模式: alpha<255 像素 = "透出桌面" (shadow halo 渐变化淡出)
	//   * DWM acrylic: alpha<255 像素 = "透出 blur backdrop" (DWM 把同一通道复用)
	// 结果: shadow halo 区被 DWM 当成请求 acrylic blur, 用户视觉上看到
	// "shadow 区也带半透模糊"的 bug.
	//
	// DWM acrylic 是 HWND 级, 无 per-region 控制, 没法只让 inner 元素区透 blur
	// 而 shadow halo 区不透. 唯一可行 (无新架构) 方案是 host 变 shaped 后立即
	// 释放 acrylic, 让 CXBlur 退化为 tint+border 装饰层 (与已 shaped 窗口 attach
	// 顺序 A / GDI 模式 / 老 OS 一致).
	//
	// 顺序无关: attach 时已经检查过一次, 这里再每帧检查一次, 让 "后 attach
	// CXShadow" 也能正确触发释放. 检查极轻量 (一次原子读 + 一次 mutex map 查询).
	if (m_acrylicApplied && m_hostHwnd){
		HWINDOW hxw = XWidget_GetHWINDOW((HXCGUI)hEle);
		if (hxw){
			window_transparent_ wt = XWnd_GetTransparentType(hxw);
			if (wt != window_transparent_false){
				XBlur_ReleaseHostBlur(m_hostHwnd, hEle);
				m_acrylicApplied = false;
				// 不清 m_hostHwnd: Detach / OnDestroy 仍要它做 graceful cleanup.
			}
		}
	}
	// ============================================================================

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

		// 1) tint 填充. 两条路径:
		//    a) m_acrylicApplied = TRUE  (普通不透明窗 + D2D + 系统支持 acrylic):
		//       用 D2D1_PRIMITIVE_BLEND_COPY 把元素圆角内像素 alpha 直接覆盖
		//       为 tint.alpha (~51 / 20%), 让 DWM 经典 Win11 acrylic 公式
		//       "displayed = tint_rgb*α + backdrop_blur*(1-α)" 正确出后景.
		//    b) m_acrylicApplied = FALSE (shaped/shadow/simple 透明窗, 或老 OS,
		//       或 D2D 1.0 fallback): 用默认 SOURCE_OVER 把 tint 半透明叠在
		//       元素客户区上, 不动 alpha 通道. 关键: 不能再用 COPY blend, 否则
		//       会把圆角 path 内 alpha 强制写成 51 (=20%), 在 shaped 窗口里
		//       这一片就变 80% 透明 (能透出桌面), 而圆角外角落仍然保留 XCGUI
		//       原 alpha → 角落看上去半透矩形, 圆角内"亚克力"反而是空洞,
		//       视觉上正好是用户报的 issue.
		ID2D1SolidColorBrush* pTint = NULL;
		dc->CreateSolidColorBrush(
			D2D1::ColorF(GetRGBA_R(tc) / 255.0f, GetRGBA_G(tc) / 255.0f,
			             GetRGBA_B(tc) / 255.0f, ta / 255.0f),
			&pTint);
		if (pTint && pGeom){
			if (m_acrylicApplied){
				D2D1_PRIMITIVE_BLEND oldBlend = dc->GetPrimitiveBlend();
				dc->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
				dc->FillGeometry(pGeom, pTint);
				dc->SetPrimitiveBlend(oldBlend);
			} else {
				// SOURCE_OVER (D2D 默认), 不改 alpha 通道, 与 OnPaintGdi 同语义.
				dc->FillGeometry(pGeom, pTint);
			}
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
	// snapEnabled = false → 仅拦 *snap* 入口 (Aero Snap 拖边 / Win+方向键 / Snap
	//                       Layouts 选 half/quarter), 通过 WM_WINDOWPOSCHANGING
	//                       几何过滤实现. 最大化按钮 / SC_MAXIMIZE 不受影响 —
	//                       如果你也想禁最大化, 用独立 EnableMaximize 接口.
	bool    snapEnabled         = true;

	// ===== 最大化控制 (EnableMaximize) =====
	// maxEnabled = true (默认, 跟随窗口 WS_MAXIMIZEBOX 原始状态).
	// maxEnabled = false → strip WS_MAXIMIZEBOX (按钮变灰) + 吞 SC_MAXIMIZE.
	bool    maxEnabled          = true;
	bool    maxBoxSaved         = false;    // 是否已 strip WS_MAXIMIZEBOX
	bool    maxBoxOriginallySet = false;    // strip 前 WS_MAXIMIZEBOX 是否本来就置位

	// ===== Snap / 最大化 解耦的暂态 flag =====
	// SC_MAXIMIZE 经 WM_SYSCOMMAND 放行后会触发 WM_WINDOWPOSCHANGING, 目标矩形
	// = 全工作区, 几何上跟 "snap 全屏 (拖到顶边)" 完全一样. 没法靠几何区分
	// "用户主动最大化" 和 "Aero Snap 全屏". 用一个暂态标记: SYSCOMMAND
	// 处理 SC_MAXIMIZE 时把它拨为 true, 紧随其后的 WM_WINDOWPOSCHANGING 就跳
	// 过 snap 几何过滤; WM_WINDOWPOSCHANGED 处理完后立即清回 false.
	// 不需要 atomic — 全部在 UI 线程 (Win32 派发线程) 顺序读写, 锁内访问.
	bool    maximizing          = false;
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
// Win 没有 per-window "禁 snap" / "禁最大化" 的官方 API. 拆成两套独立实现:
//
//   * EnableSnap(FALSE) — 仅拦 snap 拖边落位. 不动 WS_MAXIMIZEBOX, 不动
//     SC_MAXIMIZE. 标题栏最大化按钮 / 双击 / Win+Up 仍能正常最大化.
//
//       WM_WINDOWPOSCHANGING 几何过滤 → target rect 匹配 snap layout (左右
//       半屏 / 上下半屏 / 四角 / 全屏) → 设 SWP_NOMOVE | SWP_NOSIZE 阻止落位.
//
//       但 SC_MAXIMIZE 转化的 WM_WINDOWPOSCHANGING 目标 rect = 全工作区, 几
//       何上和 "snap 全屏" 完全一样, 必须区分. 用 maximizing 暂态 flag (见
//       NativeFxState), SC_MAXIMIZE 放行时立刻置位, 下一帧 WM_WINDOWPOSCHANGED
//       清掉 — 这之间的 WM_WINDOWPOSCHANGING 跳过过滤.
//
//   * EnableMaximize(FALSE) — 同时禁掉最大化:
//       - strip WS_MAXIMIZEBOX → 杀 Snap Layouts 悬停飞出框 + 标题栏最大化
//                                  按钮变灰.
//       - WM_SYSCOMMAND 吞 SC_MAXIMIZE → 拦键盘 Win+Up + 双击标题栏 + 系统
//                                          菜单 "最大化".
//
// 两套接口可任意组合:
//   EnableSnap(FALSE) + EnableMaximize(TRUE)  ← 典型场景: 不喜欢 snap 但保留
//                                                最大化按钮.
//   EnableSnap(FALSE) + EnableMaximize(FALSE) ← 完全锁死窗口尺寸入口.
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
		// *maximizing flag 跳过*: 紧随 SC_MAXIMIZE 的 WINDOWPOSCHANGING 几何
		// = 全工作区, 用户的本意是最大化不是 snap, 必须放行. 用 maximizing
		// 暂态 (SYSCOMMAND 处置位, WINDOWPOSCHANGED 清回) 区分.
		WINDOWPOS* wp = (WINDOWPOS*)lParam;
		if (!wp) break;
		bool snapBlocked = false;
		{
			std::lock_guard<std::mutex> lk(g_nativeFxMutex);
			auto it = g_nativeFxMap.find(raw);
			if (it != g_nativeFxMap.end() && !it->second.snapEnabled
			    && !it->second.maximizing){
				snapBlocked = true;
			}
		}
		if (snapBlocked && XBlur_IsSnapTargetGeom(wp, raw)){
			wp->flags |= SWP_NOMOVE | SWP_NOSIZE;
		}
		break;
	}

	case WM_WINDOWPOSCHANGED: {
		// snap 完成 / restore-from-snap 部分场景只发 WM_WINDOWPOSCHANGED 不发
		// WM_SIZE (CXShadow 早有此观察). 这里只在 size 真变了才 react —— 避免
		// 拖动期 spam (WM_WINDOWPOSCHANGED 每像素都触发, 不能盲应用).
		// *清 maximizing 暂态* — SC_MAXIMIZE 触发的 WINDOWPOSCHANGING 已处理完.
		WINDOWPOS* wp = (WINDOWPOS*)lParam;
		{
			std::lock_guard<std::mutex> lk(g_nativeFxMutex);
			auto it = g_nativeFxMap.find(raw);
			if (it != g_nativeFxMap.end()) it->second.maximizing = false;
		}
		if (wp && (wp->flags & SWP_NOSIZE) == 0){
			XBlur_SyncHost(raw);
		}
		break;
	}

	case WM_SYSCOMMAND: {
		// SC_MAXIMIZE 处理 (mask 0xFFF0 拿主命令, 低 4 位是 system reserved).
		// 两条路径:
		//   * EnableMaximize(FALSE)  → 吞掉 (pbHandled=TRUE), 不下发 DefWindowProc.
		//   * EnableMaximize(TRUE)   → 放行, 但置 maximizing=true 让紧接的
		//                                WINDOWPOSCHANGING 跳过 snap 几何过滤
		//                                (即使 EnableSnap(FALSE)).
		// 默认 (maxEnabled=true, snapEnabled=true) 此分支只置个 flag, 几乎零开销.
		if ((wParam & 0xFFF0) == SC_MAXIMIZE){
			bool maxBlocked = false;
			{
				std::lock_guard<std::mutex> lk(g_nativeFxMutex);
				auto it = g_nativeFxMap.find(raw);
				if (it != g_nativeFxMap.end()){
					if (!it->second.maxEnabled){
						maxBlocked = true;
					} else {
						it->second.maximizing = true;
					}
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
// 启用 / 禁用本窗的 Aero Snap 拖边落位 (默认启用). 实现策略详见 anonymous
// namespace "Snap / 最大化 禁用机制" 注释.
//
// *与 EnableMaximize 解耦*: 本接口仅拦 snap 几何, 不动 WS_MAXIMIZEBOX, 不吞
// SC_MAXIMIZE — 最大化按钮 / 双击 / Win+Up 仍能正常最大化. 想同时禁最大化
// 请额外调 EnableMaximize(hWnd, FALSE).
//============================================================================
BOOL CXBlur::EnableSnap(HWINDOW hWnd, BOOL bEnable){
	if (!XC_IsHWINDOW((HXCGUI)hWnd)) return FALSE;
	HWND raw = (HWND)::XWnd_GetHWND(hWnd);
	if (!raw) return FALSE;

	bool needHookInstall = false;
	{
		std::lock_guard<std::mutex> lk(g_nativeFxMutex);
		auto& st = g_nativeFxMap[raw];
		st.snapEnabled = (bEnable != FALSE);
		if (!st.hookInstalled){
			st.hookInstalled = true;
			st.hWindow       = hWnd;
			needHookInstall  = true;
		}
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
		bool prev = st.maxEnabled;
		st.maxEnabled = (bEnable != FALSE);
		if (!st.hookInstalled){
			st.hookInstalled = true;
			st.hWindow       = hWnd;
			needHookInstall  = true;
		}
		if (prev != st.maxEnabled){
			if (!st.maxEnabled){
				needFrameChanged = XBlur_StripMaxBox_Locked(raw, st);
			} else {
				needFrameChanged = XBlur_RestoreMaxBox_Locked(raw, st);
			}
		}
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
