//============================================================================
// module_xcgui_video.cpp
//
// CXVideo (FFmpeg + WASAPI) 实现.
//
// 整体架构:
//   - 4 个 worker 线程:
//       (1) Demux       : 读源 -> 拆包 -> 推视频/音频包队列, 处理 seek/eof
//       (2) VideoDecode : 取视频包 -> 解码 -> swscale 转 BGRA32 -> 推视频帧队列
//       (3) AudioDecode : 取音频包 -> 解码 -> swresample 转设备格式 -> 推音频帧队列
//       (4) AudioRender : 取音频帧 -> 写 WASAPI -> 更新 m_audioClock
//
//   - UI 线程在 XE_XC_TIMER (16ms) 里调度视频帧上屏:
//       TryAdvanceFrame: 看 frameQ 顶部帧 PTS 与 主时钟 (m_audioClock) 差:
//         超前 -> 等下一帧 timer
//         差不多 -> 把 BGRA buf 拷进 m_curBgra, dirty=true, RedrawSelf
//         严重落后 -> 丢这帧, 拉下一帧重判
//
//   - OnPaintImpl: 优先 D2D, 拿不到 RT 降级 GDI StretchDIBits.
//     画面起点用 ComputeDestRect 按 m_fitMode 在元素 *物理像素* 客户区里算.
//
//   - DPI: m_dpiScale = 物理像素 / 逻辑像素. 视频源像素是 device-independent, 在
//     ComputeDestRect 里直接映射到物理像素客户区, 不再乘 m_dpiScale.
//
// 所有模块级注释优先解释 *为什么这么写*, 而不是显而易见的 "what".
//============================================================================
#include "module_xcgui_video.h"

#include <cmath>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <new>

//============================================================================
// 匿名命名空间: 文件级辅助
//============================================================================
namespace {

// COM Init 引用计数. CXVideo 的 worker 线程调用 WASAPI / IMMDeviceEnumerator 必须
// 先 CoInitializeEx (MTA). 我们让每个用到 COM 的线程在入口自己 init, 退出前 uninit.
// CoInit 是引用计数的, 多次调用安全 (失败码 RPC_E_CHANGED_MODE 也不致命, 表示
// 当前线程已 init 为另一种模式 - 我们容忍).
struct ComInit_MTA {
	HRESULT hr = E_FAIL;
	ComInit_MTA(){ hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED); }
	~ComInit_MTA(){
		// 失败时 CoUninit 会 underflow ref count, 必须避免.
		if (SUCCEEDED(hr)) ::CoUninitialize();
	}
};

// 关闭一个 IUnknown* (不为 NULL 时), 然后置 NULL. 用宏避免 5 行模板膨胀.
template <class T>
inline void SafeRelease(T*& p){
	if (p){ p->Release(); p = NULL; }
}

// 把 AVPacket 从 src 转移到 dst 全部所有权 (av_packet_move_ref). 转移后 src 是
// "已 unref" 状态, 调用方仍可继续 av_packet_unref / av_packet_free.
inline void MovePacket(AVPacket* dst, AVPacket* src){
	av_packet_unref(dst);
	av_packet_move_ref(dst, src);
}

// Clamp helper. C++17 std::clamp 在某些 SDK 编译选项下还要 #include <algorithm>; 直接写避免依赖.
template<class T> inline T clamp_t(T v, T lo, T hi){ return v < lo ? lo : (v > hi ? hi : v); }

// ===== 控件栏 布局常量 (单位: 元素逻辑像素) =====
// 控件栏总高 + 左右内边距 + 各控件标准尺寸. 控件栏宽度随 m_hEle 自动拉伸.
constexpr int kCtrlBarH       = 40;     // 控件栏高度
constexpr int kCtrlBarPadX    = 8;      // 左右留白
constexpr int kBtnW           = 36;     // 圆形/方形 按钮宽
constexpr int kBtnH           = 28;     // 按钮高
constexpr int kSliderH        = 14;     // 进度条高
constexpr int kTimeLblW       = 110;    // 时间标签宽 (容得下 "0:00 / 0:00" 甚至 "1:23:45 / 2:34:56")
constexpr int kGap            = 6;      // 控件间距
// 进度条 0..kProgressRange 的整型刻度 (越大用户拖动越精细; 1000 ≈ 0.1% 精度).
constexpr int kProgressRange  = 1000;
constexpr int kVolumeRange    = 100;
// scrub 节流: OnSliderProgressChange 距上次实际 Seek < 这个值, 暂存 pending,
// 不打到 demux. 80ms 是经验值: 比一帧 (16ms) 大不少, 让 demux 有空 av_read_frame
// 推几个新包做 preview 解码; 又比人眼对延迟感知阈值 (~150ms) 小, 拖动顺手.
constexpr DWORD kScrubMinIntervalMs = 80;
// slider 视觉锁定: 用户最近 scrub 之后这段时间, UpdateControlBarPosition 不动 slider 视觉位置.
// 给 demux + audio render 足够时间稳定到新位置, 防 keyframe 反向偏差小回弹.
constexpr DWORD kSliderLockMs       = 500;
// 音量 popup 尺寸 (固定; 上方弹出, 不随 m_hEle 缩放).
constexpr int kVolPopupW      = 56;
constexpr int kVolPopupH      = 140;
// 控件栏背景色 (深灰; XCGUI BkFill 不直接支持 alpha 半透, 用纯深色近似 YouTube 风).
// 坑: XCGUI 的颜色是 *ARGB* (0xAARRGGBB 形式, 实际内存 ABGR).
// Windows 的 RGB(r,g,b) 宏产生 0x00BBGGRR -> 高字节 alpha=0 -> 完全透明!
// 必须用 XCGUI 自家的 RGBA(r,g,b,a=255) 宏才正确. 整个控件栏一律 RGBA.
constexpr COLORREF kCtrlBarBg    = RGBA(0,   0,   0,   255);  // 默认 纯黑不透明
constexpr COLORREF kCtrlBarFg    = RGBA(255, 255, 255, 255);  // 默认 白色文字

// ===== Slider 默认样式 (用户 spec) =====
// 轨道底色 35% 白 (alpha = 0.35*255 ≈ 89). XCGUI XDraw_FillRectColor 走 D2D/GDI+,
// 都正常支持 alpha 通道半透混合 (跟 BkFill 不同, BkFill 不支持 alpha).
constexpr COLORREF kSliderTrackBg   = RGBA(255, 255, 255, 89);   // 35% 白
// 已填充段: #0099FF (R=0, G=0x99, B=0xFF), 不透明.
constexpr COLORREF kSliderTrackFill = RGBA(0x00, 0x99, 0xFF, 255);
// 滑块按钮: #F7F7F7 实心圆 (用 XDraw_FillEllipse 而非用户原说的 DrawArcF -
// DrawArcF 只画圆弧轮廓, 不填充; 截图视觉是实心圆 -> FillEllipse 才匹配).
constexpr COLORREF kSliderThumb     = RGBA(0xF7, 0xF7, 0xF7, 255);
// 轨道 视觉厚度 (横向 -> 高, 纵向 -> 宽). 4px 跟截图比例匹配.
constexpr int      kSliderTrackThk  = 4;
// 滑块 直径. 默认 14, kSliderH 一致, 截图也是这种比例.
constexpr int      kSliderThumbDia  = 14;

// SVG 图标 (硬编码 UTF-8 字面量, 用 R"SVG(...)SVG" 避免转义).
// 内容 1:1 来自 ./svg/*.svg, 通过单独的 .inc 文件包含进来防 .cpp 体积无脑膨胀.
#include "module_xcgui_video_svgs.inc"

} // namespace

//============================================================================
// 静态工具: 字符串转换 / FFmpeg 错误码
//============================================================================

std::string CXVideo::WideToUtf8(const std::wstring& w){
	if (w.empty()) return std::string();
	int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
	                              NULL, 0, NULL, NULL);
	if (n <= 0) return std::string();
	std::string s; s.resize((size_t)n);
	::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
	                      &s[0], n, NULL, NULL);
	return s;
}

std::wstring CXVideo::AvErrToWStr(int err){
	char buf[AV_ERROR_MAX_STRING_SIZE];
	::memset(buf, 0, sizeof(buf));
	av_strerror(err, buf, sizeof(buf) - 1);
	int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, -1, NULL, 0);
	if (n <= 0) return std::wstring();
	std::wstring w; w.resize((size_t)n);
	::MultiByteToWideChar(CP_UTF8, 0, buf, -1, &w[0], n);
	if (!w.empty() && w.back() == L'\0') w.pop_back();
	return w;
}

double CXVideo::VPtsToSec(int64_t pts) const{
	if (pts == AV_NOPTS_VALUE) return -1.0;
	if (m_videoTb.den == 0) return -1.0;
	return (double)pts * av_q2d(m_videoTb) - m_startTimeSec;
}

double CXVideo::APtsToSec(int64_t pts) const{
	if (pts == AV_NOPTS_VALUE) return -1.0;
	if (m_audioTb.den == 0) return -1.0;
	return (double)pts * av_q2d(m_audioTb) - m_startTimeSec;
}

//============================================================================
// 构造 / 析构 / Create
//============================================================================

CXVideo::CXVideo(){
	// FFmpeg 4.x 之前需要 av_register_all; 4.0+ 默认自动注册. 网络协议初始化 avformat_network_init
	// 是幂等的, 多次调用安全, 这里调一次让 rtsp:// / http:// 等 URL 可用.
	// 静态局部 + once 避免重复 init 开销.
	struct OnceInit {
		OnceInit(){ avformat_network_init(); }
		~OnceInit(){ avformat_network_deinit(); }
	};
	static OnceInit s_once;
	(void)s_once;
}

CXVideo::~CXVideo(){
	// 析构兜底: 与 OnDestroy 走同一关闭路径. 如果 XE_DESTROY_END 已先触发过 Close,
	// 这里再走一遍是空操作 (各 ptr 已 NULL, 各线程已 join).
	Close();
	// GDI 离屏 DIB 兜底释放. OnDestroyImpl 已经释放过的话, 这里是空操作 (内部句柄都 NULL).
	// 仅在用户跳过 XCGUI destroy 流程直接 delete CXVideo 时这条才做实际工作.
	ReleaseGdiDib();
}

HELE CXVideo::Create(int x, int y, int cx, int cy, HXCGUI hParent){
	// 关键: 用 *CXLayout* 而不是基础 XEle. 这样:
	//   1. 内置控件栏可挂到 m_hEle 下并享受自动布局 (无需手算坐标 + 窗口缩放自动 reflow);
	//   2. 视频自身的 XE_PAINT 仍照常工作 (CXLayout 继承 CXEle, paint 链路不变);
	//   3. layout.alignV=bottom 让单一子节点 (控件栏) 自动贴底部.
	m_hEle = XLayout_Create(x, y, cx, cy, hParent);
	if (!m_hEle) return NULL;
	// CXLayout 默认 EnableMouseThrough=TRUE (鼠标穿透到下层), 我们要让视频区接 LBUTTONUP
	// 触发 暂停/恢复, 必须关闭.
	XEle_EnableMouseThrough(m_hEle, FALSE);
	// 把外层布局设为 *纵向* (Horizon=FALSE), 单一子节点 (控件栏) alignV=bottom 即贴底.
	XLayoutBox_EnableHorizon(m_hEle, FALSE);
	XLayoutBox_SetAlignV(m_hEle, layout_align_bottom);

	RefreshDpiScale();

	// *不* 启用 XE_PAINT_END. 我们走 XE_PAINT + 拦截 (pbHandled=TRUE) 路线,
	// 让 XCGUI 默认的 BkInfo / 边框 / 焦点框 全跳过. 视频播放器不需要 XCGUI 边框,
	// 且每帧都是全量重画 - BkInfo 的背景填充是浪费的一道画出着活. 为保留
	// SetVideoBkColor 语义 (用户调后修改背色), m_videoBkColor 仍在 OnPaintD2D /
	// OnPaintGdi 里被读取作为填充色.
	// (RebuildBkInfo 不再调 - BkInfo 不被使用, 调了也是冗余.)

	InstallEvents();

	// 内置控件栏 (Play/Pause, Loop, 进度, 时间, 音量). 用户在 Create() 之前调
	// EnableControlBar(FALSE) 可关.
	if (m_ctrlBarEnabled){
		CreateControlBar();
	}

	return m_hEle;
}

//============================================================================
// 事件注册
//============================================================================
void CXVideo::InstallEvents(){
	// XE_PAINT: 完全接管绘制. 处理函数里 置 *pbHandled=TRUE 跳过 XCGUI 默认
	// (BkInfo 背景 / 边框 / 焦点框), 自己画: 背色填充 + 视频帧 (如有).
	XEle_RegEventCPP1(m_hEle, XE_PAINT,        &CXVideo::OnPaintImpl);
	// XE_SIZE: 元素尺寸变化, D2D Bitmap 持有的目标矩形不需要重建 (Bitmap 跟源像素绑,
	// 不跟元素客户区绑), 但要触发一次重绘.
	XEle_RegEventCPP1(m_hEle, XE_SIZE,         &CXVideo::OnSizeImpl);
	// XE_XC_TIMER: 帧调度. 全程开启, 即使空闲也只是 16ms 进来一次, 检查 m_state 后
	// 立即返回, CPU 开销可忽略 (~0.01% on idle).
	XEle_RegEventCPP1(m_hEle, XE_XC_TIMER,     &CXVideo::OnTimerImpl);
	// XE_DESTROY: 元素销毁时干净关闭所有线程 / 资源, 避免 *XCGUI 退出后才走 ~CXVideo*
	// 这种顺序 (那时 XCGUI dll 已卸载, XEle_Redraw 等接口可能崩).
	XEle_RegEventCPP1(m_hEle, XE_DESTROY,      &CXVideo::OnDestroyImpl);
	// XE_LBUTTONUP: 视频区域左键弹起 -> 切换 播放/暂停. 控件栏的按钮 / slider 自己消化
	// 鼠标事件, 不会冒泡到这里; 控件栏空白处 (mouse-through) 会透到这里.
	XEle_RegEventCPP1(m_hEle, XE_LBUTTONUP,    &CXVideo::OnLButtonUpVideo);
	// XE_MOUSEMOVE: 用户活动信号 -> 控件栏自动隐藏的 tick 重置 + 显示.
	// 注: 控件栏的子控件 (按钮 / slider) 不会冒泡 mousemove 到这, 它们各自挂 HookMouseActivity.
	HookMouseActivity(m_hEle);

	// 启动定时器, 16ms ≈ 60Hz. 即使没播放也保留, 用来轮询 m_pendingOpened/Ended/Error
	// 把 worker 线程产生的事件转发到 UI 线程. 同时驱动 EvalAutoHide.
	XEle_SetXCTimer(m_hEle, kTimerId_Tick, kTimerInterval_Ms);
}

int CXVideo::OnDestroyImpl(HELE /*hEle*/, BOOL* /*pbHandled*/){
	// XE_DESTROY 在子对象销毁 *之前*. 这里做的事:
	//   1. 关闭定时器 (避免 timer 在 worker 线程被 join 期间还触发)
	//   2. 停所有线程 (含 m_thOpen) 然后才可以释放 FFmpeg/WASAPI 资源, 顺序反了是 UAF
	//   3. D2D / GDI 渲染资源也释放 (D2D Bitmap 跟 RT 绑, RT 析构后不能再 Release)
	//   4. *位* m_hEle = NULL: 防后续 析构->Close->RedrawSelf 拿陈旧句柄调
	//      XEle_Redraw, XCGUI 会弹 "句柄无效 0x13" 错误提示框.
	XEle_KillXCTimer(m_hEle, kTimerId_Tick);

	// 先停线程 (StopThreadsAndJoin 内部会设 m_quit + interrupt cb 才能中断
	// m_thOpen 里可能正在跡的 avformat_open_input/find_stream_info).
	StopThreadsAndJoin();
	CloseInternal();

	// D2D 资源释放. m_pLastRT 不持有 (借自 XCGUI), 不 Release.
	SafeRelease(m_pD2DBmp);
	m_pLastRT = NULL;
	m_d2dBmpW = m_d2dBmpH = 0;
	// GDI 离屏 DIB / DC 释放 (GDI handle 数量上限 10000, 不释会泄漏).
	ReleaseGdiDib();
	// SVG 句柄释放: XSvg_Destroy. XCGUI 卸载后调会 句柄 invalid 弹错框, 赶在这里清掉.
	DestroySvgs();

	// 关键修复: 元素被 XCGUI 销毁后, m_hEle 已成陈旧句柄. 后续 ~CXVideo -> Close()
	// 里的 RedrawSelf 拿这个陈旧值去调 XEle_Redraw, 会报 "句柄无效 0x13" 弹窗.
	// 这里置 NULL, RedrawSelf 的 `if (m_hEle)` 守卫就能安全跳过.
	m_hEle = NULL;
	return 0;
}

int CXVideo::OnLButtonUpVideo(HELE /*hEle*/, UINT /*nFlags*/, POINT* /*pPt*/, BOOL* pbHandled){
	// 视频区域左键弹起.
	// 注意: 控件栏的 *按钮 / slider* 自己消化 LBUTTONUP, 不会冒泡到这里;
	//       控件栏空白处 (CXLayout default 鼠标穿透) 会透到这里 - 跟点视频效果一样.
	// 微交互: 若音量面板可见, 优先关面板, 此次点击 *不* 切换暂停 (避免误触).
	if (IsVolumePanelVisible()){
		HideVolumePanel();
		if (pbHandled) *pbHandled = TRUE;
		return 0;
	}
	int s = m_state.load(std::memory_order_acquire);
	switch (s){
	case xvideo_state_playing:
		Pause();
		break;
	case xvideo_state_paused:
	case xvideo_state_stopped:
	case xvideo_state_ended:
	case xvideo_state_opening:    // opening 期间点击 -> 标 Play, 异步打开成功后自动播
		Play();
		break;
	case xvideo_state_closed:
	case xvideo_state_error:
	default:
		// 没媒体 / 出错: 点击无效. 用户得先 Open().
		return 0;
	}
	// 同步控件栏 (如果有) 的播放按钮显示.
	UpdateControlBarPlayState();
	return 0;
}

int CXVideo::OnSizeImpl(HELE /*hEle*/, int /*nFlags*/, UINT /*nAdjustNo*/, BOOL* /*pbHandled*/){
	// 元素尺寸变化, 视频帧尺寸不变 (D2D Bitmap 跟源像素绑), 仅触发重绘以更新 ComputeDestRect.
	// 同时 DPI 可能因父窗口跨屏而变化, 刷新一次.
	RefreshDpiScale();
	// 控件栏现在用 CXLayout 自动布局, 但 XCGUI default XC_EnableAutoRedrawUI=FALSE 时
	// 单纯尺寸变化不会自动 reflow + 重绘子节点. ReflowControlBar 强制走一遍 layout +
	// redraw 链, 防止 bar / 子控件停留在旧位置.
	ReflowControlBar();
	// 音量面板用 SetRect 绝对定位 (LayoutItem disable), 不会跟着自动 reflow. 窗口缩放期间
	// 面板若可见, 位置会停在旧的按钮坐标, 跟 vol 按钮脱节. 这里关掉它最省事 (用户重开即可),
	// 这样不用再写一份重定位代码.
	if (IsVolumePanelVisible()) HideVolumePanel();
	// 仅重画自己, 不调 XWnd_Redraw (整窗 redraw). 之前那条用来擦 GDI+ 模式 resize 后元素
	// 之外的旧像素, 但实测拖拽边缘时高频 OnSize -> XWnd_Redraw 让整窗高频 paint -> UI 线程
	// 吃 CPU -> video timer 推不动 -> 画面/音频感觉卡死. 现在改为靠 XDraw_ConvRect 让 CXVideo
	// 元素内永远画对位置, 元素外的旧像素是 XCGUI 自己 paint 链的责任 (典型场景下元素外只是
	// 标题栏 / layout padding, 即便偶尔残留也很小, 用户后续操作会触发 XCGUI 自然重绘擦掉).
	// DIB 重建标记 dirty: 元素尺寸变了, 下次 paint 必须重画 DIB 内容.
	m_gdiDibDirty = true;
	RedrawSelf();
	return 0;
}

//============================================================================
// DPI / BkInfo / 重绘
//============================================================================

void CXVideo::RefreshDpiScale(){
	if (!m_hEle) return;
	HWINDOW hWnd = (HWINDOW)XWidget_GetHWINDOW((HXCGUI)m_hEle);
	int dpi = hWnd ? XWnd_GetDPI(hWnd) : 96;
	if (dpi <= 0) dpi = 96;
	m_dpiScale = (float)dpi / 96.0f;
}

void CXVideo::RebuildBkInfo(){
	if (!m_hEle) return;
	XEle_ClearBkInfo(m_hEle);
	// 单层 BkFill, 颜色 = m_videoBkColor. SetVideoBkColor 改色后会再次 RebuildBkInfo.
	// 用户也可以自己 XEle_AddBkXxx 增层 (例如 hover 边框); BkInfo 系统支持多层叠加.
	XEle_AddBkFill(m_hEle, element_state_flag_focus_no, m_videoBkColor);
	RedrawSelf();
}

void CXVideo::ComputeDestRect(int eleW, int eleH, RECT* pDst) const{
	pDst->left = pDst->top = pDst->right = pDst->bottom = 0;
	if (eleW <= 0 || eleH <= 0 || m_videoW <= 0 || m_videoH <= 0) return;

	if (m_fitMode == xvideo_fit_stretch){
		pDst->left = 0; pDst->top = 0;
		pDst->right = eleW; pDst->bottom = eleH;
		return;
	}
	if (m_fitMode == xvideo_fit_original){
		// 居中, 不缩放. 大于元素时让 D2D / GDI clip.
		int dx = (eleW - m_videoW) / 2;
		int dy = (eleH - m_videoH) / 2;
		pDst->left   = dx;
		pDst->top    = dy;
		pDst->right  = dx + m_videoW;
		pDst->bottom = dy + m_videoH;
		return;
	}

	// contain / cover: 等比缩放. contain 取 min(scale), cover 取 max(scale).
	double sx = (double)eleW / (double)m_videoW;
	double sy = (double)eleH / (double)m_videoH;
	double s  = (m_fitMode == xvideo_fit_cover) ? (std::max)(sx, sy) : (std::min)(sx, sy);

	int w = (int)(m_videoW * s + 0.5);
	int h = (int)(m_videoH * s + 0.5);
	int dx = (eleW - w) / 2;
	int dy = (eleH - h) / 2;
	pDst->left   = dx;
	pDst->top    = dy;
	pDst->right  = dx + w;
	pDst->bottom = dy + h;
}

void CXVideo::RedrawSelf(){
	// 三层防御:
	//   (1) m_hEle != NULL          - OnDestroyImpl 末尾置 NULL, 防 ~CXVideo->Close 期间访问.
	//   (2) XC_IsHELE(m_hEle) == TRUE - XCGUI 自带的句柄校验, 防 race: 关窗时 XCGUI 内部
	//                                   已把元素标记为正在销毁, 但 OnDestroyImpl 还没轮到我们
	//                                   置 NULL. 这时 m_hEle 不为 NULL 但已是陈旧值, XEle_Redraw
	//                                   会弹 "句柄无效 0x13" 错误框. XC_IsHELE 在 (HEle is invalid)
	//                                   时返 FALSE, 让我们静默跳过.
	//   (3) 顺手用 SuspendError API 也行, 但 XC_IsHELE 已足够干净.
	if (m_hEle && XC_IsHELE((HXCGUI)m_hEle)) XEle_Redraw(m_hEle, FALSE);
}

//============================================================================
// 公开: 打开 / 关闭
//============================================================================

BOOL CXVideo::Open(const wchar_t* pPath){
	if (!pPath || !*pPath) return FALSE;

	// 先关掉上一次. Close 同步 join m_thOpen + worker 线程. 受 FFmpeg interrupt cb
	// 加持, 即使上次 Open 在跡 avformat_open_input (网络流 / debug heap), 这里也
	// 能几十毫秒返回.
	Close();

	// Close() 末尾会把 m_quit 留为 true. 这里重置为 false, 不然新一轮 worker
	// (含 m_thOpen 本身) 一启动就会看到 quit 立马退出.
	m_quit.store(false, std::memory_order_release);

	// 标记 opening, 让 GetState() 反映真实状态. OpenWorkerFn 末尾会 CAS opening->stopped;
	// 如果用户在打开期间已调 Play(), state 已是 playing, CAS 不成功, 保持 playing.
	m_state.store(xvideo_state_opening, std::memory_order_release);

	// 派到 m_thOpen. 拷贝 path 进线程体避免悬空原始 wchar_t*.
	std::wstring path(pPath);
	m_thOpen = std::thread(&CXVideo::OpenWorkerFn, this, std::move(path));
	return TRUE;
}

//============================================================================
// 异步 Open 工作体: 在 m_thOpen 上跡 avformat_open_input + find_stream_info +
// 解码器初始化 (含硬解). 是 Plan C 的核心 - 把 UI 线程从这三件同步阻塞事里抢出来.
//============================================================================
void CXVideo::OpenWorkerFn(std::wstring path){
	// 刚启动就被要求退出 (调用方在派发后立马调了 Close). 静默返.
	if (m_quit.load(std::memory_order_acquire)) return;

	if (!OpenInternal(path)){
		// OpenInternal 内部已 PostError. 区分两种失败:
		//   (a) m_quit=true 导致的中断 - 这是 Close 主动取消, 不该设 state=error,
		//       让 Close 后续把 state 设为 closed.
		//   (b) 真实错误 (路径不存在/格式不识别等) - state 走 error, OnVideoError 会被
		//       UI timer 派发.
		if (!m_quit.load(std::memory_order_acquire)){
			m_state.store(xvideo_state_error, std::memory_order_release);
		}
		return;
	}

	// 开间被 Close 拦截: OpenInternal 走完了但 m_quit 已置位. 不起 worker 线程,
	// 资源留给 Close()->StopThreadsAndJoin()->CloseInternal() 释放 (都是 NULL-safe).
	if (m_quit.load(std::memory_order_acquire)) return;

	StartThreads();
	m_pendingOpened.store(true, std::memory_order_release);

	// CAS opening->stopped. 若用户在 opening 期间已 Play(), state 已是 playing,
	// CAS 不成功 - 保持 playing, render 线程立刻开始出帧.
	int expected = xvideo_state_opening;
	m_state.compare_exchange_strong(expected, xvideo_state_stopped,
	                                std::memory_order_acq_rel,
	                                std::memory_order_acquire);
}

//============================================================================
// FFmpeg 中断回调: 让 Close()/m_quit 能取消跡在 avformat_open_input /
// find_stream_info / av_read_frame 里的阻塞 I/O. 所有需要取消能力的 AVFormatContext
// 都要在创建后立刻设这个 callback (见 OpenInternal).
//============================================================================
int CXVideo::AvInterruptCb(void* opaque){
	CXVideo* self = (CXVideo*)opaque;
	if (!self) return 0;
	return self->m_quit.load(std::memory_order_acquire) ? 1 : 0;
}

void CXVideo::Close(){
	// 设 quit 在最前, 让 m_thOpen 里可能正在跡的 avformat_open_input 被 interrupt cb
	// 立刻中断; 同时让 worker 线程的 hot loop 发现 quit 后退出.
	m_quit.store(true, std::memory_order_release);

	if (!m_hEle){
		// 析构路径: m_hEle 已被 XCGUI 销毁 (OnDestroyImpl 已置 NULL). 仍要清
		// FFmpeg / WASAPI 资源. 不能调任何 XEle_*.
		StopThreadsAndJoin();
		CloseInternal();
		return;
	}
	StopThreadsAndJoin();
	CloseInternal();
	m_state.store(xvideo_state_closed, std::memory_order_release);

	// 清掉 pending 事件, 不然下一轮 Open 还没 spawn UI 定时器已经派发过期 error/ended.
	// (例如 m_thOpen 被中断后 PostError 在队里, 不清会让 OnVideoError 在下一轮误报.)
	m_pendingOpened.store(false, std::memory_order_release);
	m_pendingEnded.store(false, std::memory_order_release);
	m_pendingError.store(false, std::memory_order_release);

	// 清 *当前帧* 缓存, 让下次 Open 之前 UI 上画的还是黑底背景.
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_curBgra.clear();
		m_curW = m_curH = m_curPitch = 0;
		m_curDirty = false;
		m_curFramePts = 0.0;
	}
	// 释放 D2D 缓存 bitmap (跟源像素绑, 下次 Open 不同尺寸要重建).
	SafeRelease(m_pD2DBmp);
	m_pLastRT = NULL;
	m_d2dBmpW = m_d2dBmpH = 0;
	RedrawSelf();
}

BOOL CXVideo::IsOpen() const{
	int s = m_state.load(std::memory_order_acquire);
	return (s != xvideo_state_closed && s != xvideo_state_error) ? TRUE : FALSE;
}

//============================================================================
// 内部: 打开 / 关闭 / 启停线程
//============================================================================

BOOL CXVideo::OpenInternal(const std::wstring& path){
	std::string utf8 = WideToUtf8(path);
	if (utf8.empty()){
		PostError(AVERROR(EINVAL), L"路径转换 UTF-8 失败");
		return FALSE;
	}

	// 1) 预分配 AVFormatContext + 装中断回调. avformat_open_input 可以接受一个已预设
	//    interrupt_callback 的 ctx, 这样在探测阶段就已可被 Close()/m_quit 取消, 防 rtsp/http
	//    的所谓 30s 默认超时、或 debug heap 下几十秒的堆分配.
	AVFormatContext* pFmt = avformat_alloc_context();
	if (!pFmt){
		PostError(AVERROR(ENOMEM), L"avformat_alloc_context 失败");
		return FALSE;
	}
	pFmt->interrupt_callback.callback = &CXVideo::AvInterruptCb;
	pFmt->interrupt_callback.opaque   = this;

	// 2) avformat_open_input: 探测容器, 解析头. 失败常见: 路径错 / 网络超时 / 格式不识别 /
	//    被 interrupt cb 取消 (返 AVERROR_EXIT).
	int ret = avformat_open_input(&pFmt, utf8.c_str(), NULL, NULL);
	if (ret < 0){
		PostError(ret, AvErrToWStr(ret).c_str());
		return FALSE;
	}
	// 3) avformat_find_stream_info: 探测解码器需要的更详细信息 (码率 / 帧率 / 通道布局).
	//    某些容器 (mpegts, rtsp) 头太薄, 必须读几秒数据才能确定. 是阻塞调用.
	ret = avformat_find_stream_info(pFmt, NULL);
	if (ret < 0){
		avformat_close_input(&pFmt);
		PostError(ret, AvErrToWStr(ret).c_str());
		return FALSE;
	}

	// 4) 找视频 / 音频流. 选 best 流 (FFmpeg 启发式: 码率 / 分辨率 / 通道数高的优先).
	int vIdx = av_find_best_stream(pFmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	int aIdx = av_find_best_stream(pFmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	// 至少要有一路. 纯字幕 / 数据流不算媒体.
	if (vIdx < 0 && aIdx < 0){
		avformat_close_input(&pFmt);
		PostError(AVERROR_STREAM_NOT_FOUND, L"未找到视频或音频流");
		return FALSE;
	}

	// 5) 视频解码器初始化.
	AVCodecContext* pVCtx = NULL;
	if (vIdx >= 0){
		AVStream* st = pFmt->streams[vIdx];
		const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec){
			avformat_close_input(&pFmt);
			PostError(AVERROR_DECODER_NOT_FOUND, L"找不到视频解码器");
			return FALSE;
		}
		pVCtx = avcodec_alloc_context3(dec);
		if (!pVCtx){
			avformat_close_input(&pFmt);
			PostError(AVERROR(ENOMEM), L"视频解码器内存不足");
			return FALSE;
		}
		avcodec_parameters_to_context(pVCtx, st->codecpar);
		// 多线程解码, 让 FFmpeg 自动按 CPU 核数选 frame/slice 线程数. H264/HEVC/VP9 都
		// 内部支持线程, 提升 1080p+ 性能 1.5~3x. 硬解模式下重活在 GPU, 这里设 0 无副作用.
		//
		// *调试器规避*: FFmpeg 内部线程池启动时, 每个 worker 会 RaiseException(0x406D1388,
		// MS_VC_EXCEPTION) 自命名给调试器看. VS 调试器默认会安静吞掉, 但 炫语言 IDE 等
		// 部分调试器把它当未捕获异常 -> 进程闪退. 检测到调试器附加时改用单线程解码,
		// 牺牲一点 SW 解码性能(HW 解码场景几乎无影响) 换调试期可用. 用户双击运行 /
		// 不挂调试器时 IsDebuggerPresent=FALSE, 仍走多线程满速.
		if (IsDebuggerPresent()){
			pVCtx->thread_count = 1;
			pVCtx->thread_type  = 0;
		} else {
			pVCtx->thread_count = 0;
			pVCtx->thread_type  = FF_THREAD_FRAME | FF_THREAD_SLICE;
		}

		// *硬解尝试* (必须在 avcodec_open2 之前). 成功后 m_hwAccelActive=true, 装好
		// pVCtx->hw_device_ctx + get_format. 失败静默, 软解继续.
		TryInitHwAccel(pVCtx, dec);

		ret = avcodec_open2(pVCtx, dec, NULL);
		if (ret < 0){
			// 硬解 + avcodec_open2 失败时回退一次到软解 (清掉 hw_device_ctx 再开).
			// 常见原因: 该 codec 不支持选中的硬解类型 (例如 AV1 硬解需要 dav1d 而不是 d3d11va).
			if (m_hwAccelActive){
				m_hwAccelActive = false;
				if (pVCtx->hw_device_ctx){ av_buffer_unref(&pVCtx->hw_device_ctx); }
				pVCtx->get_format = NULL;
				pVCtx->opaque     = NULL;
				m_hwPixFmt        = AV_PIX_FMT_NONE;
				m_hwActiveType    = AV_HWDEVICE_TYPE_NONE;
				ret = avcodec_open2(pVCtx, dec, NULL);
			}
			if (ret < 0){
				avcodec_free_context(&pVCtx);
				avformat_close_input(&pFmt);
				PostError(ret, AvErrToWStr(ret).c_str());
				return FALSE;
			}
		}
		// SwsContext 在第一次解码出 frame 时按 frame 实际尺寸 / 像素格式建; 这里不预建,
		// 因为有些 codec (HEVC) 在 *第一帧之前* width/height/pix_fmt 还可能为 0.
		// 硬解时 frame->format = m_hwPixFmt (硬件像素), 走 av_hwframe_transfer_data
		// 到一个 SW frame (NV12), 再由 sws_scale 转 BGRA32; 详见 VideoDecodeThreadFn.
	}

	// 6) 音频解码器初始化.
	AVCodecContext* pACtx = NULL;
	if (aIdx >= 0){
		AVStream* st = pFmt->streams[aIdx];
		const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec){
			// 视频还能播, 音频降级成无声. 不算致命.
			aIdx = -1;
		} else {
			pACtx = avcodec_alloc_context3(dec);
			if (pACtx){
				avcodec_parameters_to_context(pACtx, st->codecpar);
				pACtx->thread_count = 1;   // 音频通常单线程足够
				ret = avcodec_open2(pACtx, dec, NULL);
				if (ret < 0){
					avcodec_free_context(&pACtx);
					aIdx = -1;
				}
			} else {
				aIdx = -1;
			}
		}
	}

	// 全部 OK, 提交到成员.
	m_pFmt    = pFmt;
	m_pVCtx   = pVCtx;
	m_pACtx   = pACtx;
	m_videoIdx = vIdx;
	m_audioIdx = aIdx;
	m_hasVideo = (vIdx >= 0 && pVCtx != NULL);
	m_hasAudio = (aIdx >= 0 && pACtx != NULL);

	if (m_hasVideo){
		m_videoTb = pFmt->streams[vIdx]->time_base;
		m_videoW = pVCtx->width;
		m_videoH = pVCtx->height;
		// 帧率优先用 r_frame_rate (容器声明), 回退到 avg_frame_rate.
		AVRational fr = pFmt->streams[vIdx]->r_frame_rate;
		if (fr.den == 0 || fr.num == 0) fr = pFmt->streams[vIdx]->avg_frame_rate;
		m_frameRate = (fr.den != 0) ? av_q2d(fr) : 0.0;
	} else {
		m_videoTb = AVRational{0, 1};
		m_videoW = m_videoH = 0;
		m_frameRate = 0.0;
	}
	if (m_hasAudio){
		m_audioTb = pFmt->streams[aIdx]->time_base;
	} else {
		m_audioTb = AVRational{0, 1};
	}

	// duration / start_time. AV_NOPTS_VALUE 视为未知.
	if (pFmt->duration != (int64_t)AV_NOPTS_VALUE && pFmt->duration > 0){
		m_durationSec = (double)pFmt->duration / (double)AV_TIME_BASE;
	} else {
		m_durationSec = 0.0;
	}
	if (pFmt->start_time != (int64_t)AV_NOPTS_VALUE){
		m_startTimeSec = (double)pFmt->start_time / (double)AV_TIME_BASE;
	} else {
		m_startTimeSec = 0.0;
	}

	// 队列重置 (上一次 Stop 后 Close 状态; 这里重新打开).
	m_videoPktQ.Reset((size_t)m_packetCap);
	m_audioPktQ.Reset((size_t)m_packetCap);
	m_videoFrameQ.Reset((size_t)m_frameCap);
	m_audioFrameQ.Reset((size_t)m_frameCap);

	// 注意: m_quit 由 Open() 在 spawn m_thOpen *之前* 已 store(false), 这里 *不要再写*.
	// 否则会跟 Close() 主线程的 m_quit.store(true) 起 race: Close 走到 StopThreadsAndJoin
	// 等 m_thOpen.join() 时, OpenInternal 已过中断点 + 这里把 quit 覆盖回 false ->
	// OpenWorkerFn 检 m_quit==false -> StartThreads -> Close 再次 stop, 浪费一次启停.
	m_paused.store(false, std::memory_order_release);
	m_demuxEof.store(false, std::memory_order_release);
	m_seekRequest.store(false, std::memory_order_release);
	m_seekInFlight.store(false, std::memory_order_release);
	// 进度条 拖动节流 / 锁定状态: 跨 session 不能残留, 否则 UI 在新 session 开头会被
	// 上一次的 m_lastScrubTick 锁住 ~500ms 不更新 slider.
	m_pendingScrubSec = -1.0;
	m_lastScrubSeekTick = 0;
	m_lastScrubTick = 0;
	m_audioClock.store(0.0, std::memory_order_release);
	m_videoClock.store(0.0, std::memory_order_release);
	m_clockBaseTick.store(0, std::memory_order_release);
	m_clockBasePts.store(0.0, std::memory_order_release);

	// 残留 buffer 清掉, 防止上次 Close 没耗尽的样本污染本次播放.
	m_audResidualPcm.clear();
	m_audResidualFrames = 0;
	m_audResidualPtsSec = 0.0;

	return TRUE;
}

void CXVideo::CloseInternal(){
	// FFmpeg 资源释放. 顺序很关键:
	//   1) AVCodecContext (avcodec_free_context) 会自动 unref 内部持有的 hw_device_ctx,
	//      所以本地的 m_pHwDeviceCtx 是独立的一份 ref, 单独释放.
	//   2) avformat_close_input 会释放 streams.
	// 全部为 NULL safe.
	if (m_pSws){ sws_freeContext(m_pSws); m_pSws = NULL; }
	if (m_pSwr){
		swr_free(&m_pSwr);   // swr_free 接受 SwrContext** 并置 NULL
	}
	if (m_pVCtx){ avcodec_free_context(&m_pVCtx); m_pVCtx = NULL; }
	if (m_pACtx){ avcodec_free_context(&m_pACtx); m_pACtx = NULL; }
	if (m_pHwDeviceCtx){
		av_buffer_unref(&m_pHwDeviceCtx);    // 接受 AVBufferRef** 并置 NULL
	}
	m_hwAccelActive = false;
	m_hwPixFmt      = AV_PIX_FMT_NONE;
	m_hwActiveType  = AV_HWDEVICE_TYPE_NONE;

	if (m_pFmt) { avformat_close_input(&m_pFmt); m_pFmt  = NULL; }

	m_videoIdx = m_audioIdx = -1;
	m_hasVideo = m_hasAudio = false;
	m_videoW = m_videoH = 0;
	m_frameRate = m_durationSec = m_startTimeSec = 0.0;

	// WASAPI 释放. 跟 FFmpeg 解耦.
	ShutdownWasapi();
	// 复位 WASAPI init-failed 标志: 下次 Open 重新尝试 (用户可能这期间插上耳机 / 关掉占用应用).
	m_wasapiInitFailed = false;
}

void CXVideo::StartThreads(){
	// quit 已经在 Open() spawn m_thOpen *之前* 置 false (不在 OpenInternal 里, 避免覆盖
	// Close 主线程并发设的 true). 各 worker 线程从此处开始独立运行.
	if (m_hasVideo){
		m_thVDecode = std::thread(&CXVideo::VideoDecodeThreadFn, this);
	}
	if (m_hasAudio){
		m_thADecode = std::thread(&CXVideo::AudioDecodeThreadFn, this);
		// 音频 render 线程会 lazy InitWasapi (头一帧时建); 失败则降级无声 + 静默 join.
		m_thARender = std::thread(&CXVideo::AudioRenderThreadFn, this);
	}
	// Demux 线程必须最后启动: 它一开始就推包到队列, 解码线程没就位的话会浪费
	// 一段冷启动时间, 包堆队列里. 顺序倒是反过来不影响功能, 只影响首帧延迟.
	m_thDemux = std::thread(&CXVideo::DemuxThreadFn, this);
}

void CXVideo::StopThreadsAndJoin(){
	// 标志 quit. worker 线程下次循环检查后退出. 队列 Close 唤醒所有阻塞在
	// Push/Pop 的线程; 另 m_thOpen / DemuxThreadFn 里的 avformat_open_input /
	// av_read_frame 等阻塞 I/O 靠 AvInterruptCb 中断 (见 OpenInternal 里装的
	// interrupt_callback). 这样跨线程的网络超时 / debug heap 几十秒都能几十毫秒退出.
	m_quit.store(true, std::memory_order_release);
	m_videoPktQ.Close();
	m_audioPktQ.Close();
	m_videoFrameQ.Close();
	m_audioFrameQ.Close();

	// m_thOpen 先 join: 它可能正在 avformat_open_input/find_stream_info, AvInterruptCb 返 1
	// 后 FFmpeg 会返 AVERROR_EXIT. OpenWorkerFn 检出 m_quit 后不会走 StartThreads, 所以
	// 下面几个 worker 线程 joinable() 可能为 false - 那也正常.
	if (m_thOpen.joinable())    m_thOpen.join();
	if (m_thDemux.joinable())   m_thDemux.join();
	if (m_thVDecode.joinable()) m_thVDecode.join();
	if (m_thADecode.joinable()) m_thADecode.join();
	if (m_thARender.joinable()) m_thARender.join();

	// 清空残余包 / 帧, 释放 AVPacket / 缓冲. 队列已 close, Pop 会把残量消费完返回 false.
	_XVideo_PacketNode pkt;
	while (m_videoPktQ.Pop(pkt)){ if (pkt.pkt) av_packet_free(&pkt.pkt); }
	while (m_audioPktQ.Pop(pkt)){ if (pkt.pkt) av_packet_free(&pkt.pkt); }
	_XVideo_VideoFrameNode vf; while (m_videoFrameQ.Pop(vf)){}
	_XVideo_AudioFrameNode af; while (m_audioFrameQ.Pop(af)){}
}

//============================================================================
// Demux 线程: 读源 -> 推包队列, 处理 seek
//============================================================================
void CXVideo::DemuxThreadFn(){
	// 这个线程不调 COM, 不需要 CoInit.
	AVPacket* pkt = av_packet_alloc();
	if (!pkt){
		PostError(AVERROR(ENOMEM), L"av_packet_alloc 失败");
		return;
	}

	while (!m_quit.load(std::memory_order_acquire)){
		// (a) 处理 seek 请求.
		if (m_seekRequest.exchange(false, std::memory_order_acq_rel)){
			// 双保险: Seek() 已设过 m_seekInFlight=true, 但 *连续 seek* 场景下,
			// 上一轮 demux 处理完末尾把 m_seekInFlight 清了, 此刻第 2 次 seek 进
			// 处理之前 in-flight=false, UI 会短暂跟到第 1 次 seek 位置后才被第 2 次
			// 拉走 -> 仍旧闪一下. 这里再次置 true 把第 2 次的 in-flight 窗口接续上.
			m_seekInFlight.store(true, std::memory_order_release);
			double tgt = m_seekTargetSec.load(std::memory_order_acquire);
			if (tgt < 0) tgt = 0;
			if (m_durationSec > 0 && tgt > m_durationSec) tgt = m_durationSec;

			// av_seek_frame: AVSEEK_FLAG_BACKWARD 找最近的关键帧 (向前). FFmpeg 不保证
			// 跳到精确 PTS, 解码会从最近 keyframe 开始, 中间帧解出来后丢掉直到 >= tgt.
			// 简化处理: 我们不做 keyframe-to-target 之间的精确丢弃, 用户感知是首帧
			// 可能比 tgt 早几十毫秒到几秒不等 (取决于 GOP 大小). 工业级播放器会做
			// 二级精确 seek, 这里先满足通用需求.
			int64_t targetPts = (int64_t)((tgt + m_startTimeSec) * AV_TIME_BASE);
			int ret = av_seek_frame(m_pFmt, -1, targetPts, AVSEEK_FLAG_BACKWARD);
			if (ret < 0){
				// seek 失败常见: 不支持 seek 的格式 (live stream). 不致命, 继续读.
			}

			// 清队列 + 推 flush 标记给两路解码线程.
			m_videoPktQ.Clear();
			m_audioPktQ.Clear();
			m_videoFrameQ.Clear();
			m_audioFrameQ.Clear();

			if (m_hasVideo){
				_XVideo_PacketNode flushNode;
				flushNode.flush = true;
				m_videoPktQ.Push(std::move(flushNode));
			}
			if (m_hasAudio){
				_XVideo_PacketNode flushNode;
				flushNode.flush = true;
				m_audioPktQ.Push(std::move(flushNode));
			}
			// 重置时钟基准, 让 GetMasterClock 重新对齐.
			m_audioClock.store(tgt, std::memory_order_release);
			m_videoClock.store(tgt, std::memory_order_release);
			m_clockBaseTick.store(::GetTickCount(), std::memory_order_release);
			m_clockBasePts.store(tgt, std::memory_order_release);
			m_demuxEof.store(false, std::memory_order_release);
			// Seek 后 residual 是旧位置数据, 直接丢. (这里写 demux 线程 -> render 线程
			// 共享变量, 严格说要锁, 但简化起见: seek 期间 render 应该被 paused 暂停拉取
			// audioFrameQ. 此处只 clear 不 push, 是 SC 安全的.)
			m_audResidualPcm.clear();
			m_audResidualFrames = 0;
			m_audResidualPtsSec = 0.0;
			// seek 处理收尾: 清 in-flight 标志, 让 UI 进度条恢复跟随 master clock.
			// 必须放在 m_audioClock / m_clockBasePts 设完之后, 不然 UI 短暂解锁的瞬间
			// 仍可能读到旧时钟.
			m_seekInFlight.store(false, std::memory_order_release);
		}

		// (b) 读一包.
		int ret = av_read_frame(m_pFmt, pkt);
		if (ret == AVERROR_EOF){
			// 流末尾. 推 EOF 标记给两路解码线程, 自己退出.
			if (m_hasVideo){
				_XVideo_PacketNode eofNode;
				eofNode.flush = true; eofNode.eof = true;
				m_videoPktQ.Push(std::move(eofNode));
			}
			if (m_hasAudio){
				_XVideo_PacketNode eofNode;
				eofNode.flush = true; eofNode.eof = true;
				m_audioPktQ.Push(std::move(eofNode));
			}
			m_demuxEof.store(true, std::memory_order_release);
			// EOF 后线程不直接退出: 用户可能 seek 回中间. 等待 quit 或 seek.
			// 用条件变量更省 CPU 但需要额外同步. 简单起见, 让 demux 线程在 EOF 后
			// 进入轻量等待, 检查 m_seekRequest / m_quit.
			while (!m_quit.load(std::memory_order_acquire) &&
			       !m_seekRequest.load(std::memory_order_acquire)){
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
			continue;
		}
		if (ret < 0){
			// 其他错误: 网络断开 / 文件损坏. 报告 + 视为 EOF 对待 (不再读, 等 seek).
			PostError(ret, AvErrToWStr(ret).c_str());
			m_demuxEof.store(true, std::memory_order_release);
			while (!m_quit.load(std::memory_order_acquire) &&
			       !m_seekRequest.load(std::memory_order_acquire)){
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			continue;
		}

		// (c) 按流类型派发到队列. 转移 ref ownership, 然后 av_packet_unref(pkt) 让 pkt
		//     回到 fresh 状态以便下次 read. (av_packet_move_ref 已让 pkt 干净, 但 unref
		//     是显式语义稳妥.)
		if (m_hasVideo && pkt->stream_index == m_videoIdx){
			AVPacket* dup = av_packet_alloc();
			if (dup){
				MovePacket(dup, pkt);
				_XVideo_PacketNode node;
				node.pkt = dup;
				if (!m_videoPktQ.Push(std::move(node))){
					// queue closed
					av_packet_free(&dup);
					break;
				}
			}
		} else if (m_hasAudio && pkt->stream_index == m_audioIdx){
			AVPacket* dup = av_packet_alloc();
			if (dup){
				MovePacket(dup, pkt);
				_XVideo_PacketNode node;
				node.pkt = dup;
				if (!m_audioPktQ.Push(std::move(node))){
					av_packet_free(&dup);
					break;
				}
			}
		} else {
			// 字幕 / 数据流, 暂不处理.
			av_packet_unref(pkt);
		}
	}

	if (pkt) av_packet_free(&pkt);
}

//============================================================================
// 硬件解码: TryInitHwAccel / OnGetFormatImpl + 静态 trampoline
//============================================================================
//
// 硬解架构: FFmpeg 用一个 *设备无关* 的 AVCodecContext, 通过 hw_device_ctx 字段挂上具体
// 硬解设备 (D3D11VA / DXVA2 / CUDA / QSV ...). 一些细节:
//   1) get_format 回调时机: avcodec 在解析到 codec 头后, 用 get_format 询问 "我能给这些
//      候选 pix_fmt 你要哪个". 我们看里面有没有 cfg->pix_fmt (硬解格式 e.g.
//      AV_PIX_FMT_D3D11), 有就返它, codec 后续解出来的 frame 就是 GPU 帧.
//   2) 候选格式列表里通常 *最后* 一个是软解 fallback (e.g. AV_PIX_FMT_YUV420P). 找不到
//      硬解格式时返 fmts[0] 让 codec 走软解, 不能返 AV_PIX_FMT_NONE (返 NONE 会让 codec
//      报错 "decoding failed").
//   3) get_format 是 C 回调签名, 不能直接绑成员函数. 用 AVCodecContext::opaque 存 this 指针,
//      静态 trampoline 转发到成员函数.
//   4) av_hwdevice_ctx_create 返回的 AVBufferRef 是 *第一个* ref; 装到 pVCtx->hw_device_ctx
//      之前要 av_buffer_ref 取一份. avcodec_free_context 会 unref pVCtx 里那份;
//      CloseInternal 再 unref 我们自己的那份. 不重复释放.

// 静态 trampoline: avcodec 的 C 回调签名 (enum AVPixelFormat (*)(AVCodecContext*, const AVPixelFormat*))
// 不能直接绑成员函数. 通过 ctx->opaque 拿到 this 后再走成员路径.
static enum AVPixelFormat XVideo_GetFormatTrampoline(struct AVCodecContext* s,
                                                     const enum AVPixelFormat* fmts){
	CXVideo* self = (CXVideo*)s->opaque;
	if (!self){
		// 没 this 的极端情况 (理论不该发生): 返第一个候选, 多半是软解 fallback.
		return fmts ? fmts[0] : AV_PIX_FMT_NONE;
	}
	return self->OnGetFormatImpl(s, fmts);
}

AVPixelFormat CXVideo::OnGetFormatImpl(AVCodecContext* /*s*/, const AVPixelFormat* fmts){
	// 在候选格式数组里找我们 TryInitHwAccel 锁定的 m_hwPixFmt.
	// fmts 以 AV_PIX_FMT_NONE 结尾.
	if (m_hwPixFmt != AV_PIX_FMT_NONE){
		for (const AVPixelFormat* p = fmts; p && *p != AV_PIX_FMT_NONE; ++p){
			if (*p == m_hwPixFmt) return *p;
		}
	}
	// 找不到: 回到第一个候选 (一般是软解 SW fmt). 不要返 AV_PIX_FMT_NONE.
	return fmts ? fmts[0] : AV_PIX_FMT_NONE;
}

void CXVideo::TryInitHwAccel(AVCodecContext* pVCtx, const AVCodec* dec){
	// 状态预重置 (Open 可以多次调, 重新进 OpenInternal 时残值要清).
	m_hwAccelActive = false;
	m_hwPixFmt      = AV_PIX_FMT_NONE;
	m_hwActiveType  = AV_HWDEVICE_TYPE_NONE;
	if (m_pHwDeviceCtx){ av_buffer_unref(&m_pHwDeviceCtx); }

	if (m_hwAccelPref == xvideo_hwaccel_none) return;
	if (!pVCtx || !dec) return;

	// 按偏好排尝试列表. auto: D3D11VA > DXVA2 (D3D11VA 是 Win8+ 新接口, 性能与稳定性更好;
	// DXVA2 兼容 Win7 老机).
	AVHWDeviceType tryList[2] = { AV_HWDEVICE_TYPE_NONE, AV_HWDEVICE_TYPE_NONE };
	int nTry = 0;
	switch (m_hwAccelPref){
	case xvideo_hwaccel_d3d11va:
		tryList[nTry++] = AV_HWDEVICE_TYPE_D3D11VA;
		break;
	case xvideo_hwaccel_dxva2:
		tryList[nTry++] = AV_HWDEVICE_TYPE_DXVA2;
		break;
	case xvideo_hwaccel_auto:
	default:
		tryList[nTry++] = AV_HWDEVICE_TYPE_D3D11VA;
		tryList[nTry++] = AV_HWDEVICE_TYPE_DXVA2;
		break;
	}

	for (int i = 0; i < nTry; ++i){
		AVHWDeviceType type = tryList[i];

		// (1) 查 codec 是否声明支持本硬解类型. avcodec_get_hw_config 列出 codec 的所有
		//     硬解 config (pix_fmt + device_type 配对), 我们要 method == HW_DEVICE_CTX
		//     (现代 hw_device_ctx 路径, 而非老的 hwaccel callback).
		AVPixelFormat hwPixFmt = AV_PIX_FMT_NONE;
		for (int j = 0;; ++j){
			const AVCodecHWConfig* cfg = avcodec_get_hw_config(dec, j);
			if (!cfg) break;
			if ((cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
			    cfg->device_type == type){
				hwPixFmt = cfg->pix_fmt;
				break;
			}
		}
		if (hwPixFmt == AV_PIX_FMT_NONE) continue;   // 该 codec 不支持本硬解类型 (e.g. AV1 + DXVA2)

		// (2) 创建硬解设备上下文. 设备名 NULL = 用系统默认 GPU. 多 GPU 系统想指定的话传
		//     "0" / "1" 字符串, 但需要 Adapter LUID, 太麻烦, 这里走默认.
		AVBufferRef* hwCtx = NULL;
		int r = av_hwdevice_ctx_create(&hwCtx, type, NULL, NULL, 0);
		if (r < 0 || !hwCtx){
			// 该硬解类型创建失败 (常见: D3D11VA 在 Win7 没装更新; DXVA2 在 server 版被禁).
			// 不上报错误, 静默 fallback 到下一个 type 或软解.
			if (hwCtx) av_buffer_unref(&hwCtx);
			continue;
		}

		// (3) 装到 codec ctx. pVCtx->hw_device_ctx 也走 ref-counted, 用 av_buffer_ref
		//     取一份给 codec; 我们自己留 m_pHwDeviceCtx 这份, CloseInternal 释放.
		pVCtx->hw_device_ctx = av_buffer_ref(hwCtx);
		if (!pVCtx->hw_device_ctx){
			av_buffer_unref(&hwCtx);
			continue;
		}
		pVCtx->opaque     = this;
		pVCtx->get_format = XVideo_GetFormatTrampoline;

		m_pHwDeviceCtx  = hwCtx;     // 持本地 ref
		m_hwPixFmt      = hwPixFmt;
		m_hwActiveType  = type;
		m_hwAccelActive = true;
		return;   // 成功, 用本硬解.
	}
	// 所有 try 都失败. m_hwAccelActive 保持 false, 调用方走软解.
}

//============================================================================
// 视频解码线程
//============================================================================
void CXVideo::VideoDecodeThreadFn(){
	if (!m_pVCtx) return;
	AVFrame* frame   = av_frame_alloc();
	AVFrame* swFrame = av_frame_alloc();   // 硬解时承接 av_hwframe_transfer_data 输出
	if (!frame || !swFrame){
		if (frame)   av_frame_free(&frame);
		if (swFrame) av_frame_free(&swFrame);
		PostError(AVERROR(ENOMEM), L"av_frame_alloc 失败");
		return;
	}

	auto flushPushEof = [&](){
		_XVideo_VideoFrameNode eof; eof.eof = true; eof.w = 0; eof.h = 0;
		eof.pitch = 0; eof.ptsSec = -1.0;
		m_videoFrameQ.Push(std::move(eof));
	};

	while (!m_quit.load(std::memory_order_acquire)){
		_XVideo_PacketNode node;
		if (!m_videoPktQ.Pop(node)) break;

		if (node.flush){
			avcodec_flush_buffers(m_pVCtx);
			// seek flush 时清掉自己手上 frame ref 残余.
			av_frame_unref(frame);
			if (node.eof){
				// drain decoder
				avcodec_send_packet(m_pVCtx, NULL);
				while (avcodec_receive_frame(m_pVCtx, frame) >= 0){
					// 复用下面 sws 路径转 BGRA 推队列.
					// 这里走简化: drain 时偷懒不出帧, 直接推 EOF. 因为 drain 出来的
					// 几帧通常都接近流末尾, 用户视觉上少看几十毫秒可接受.
					av_frame_unref(frame);
				}
				flushPushEof();
				// 不 break - 等 demux 线程后续 seek 重启喂包.
				continue;
			}
			// seek flush, 等下一批包.
			continue;
		}

		// 普通包.
		if (!node.pkt){ continue; }
		int ret = avcodec_send_packet(m_pVCtx, node.pkt);
		av_packet_free(&node.pkt);
		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF){
			PostError(ret, AvErrToWStr(ret).c_str());
			continue;
		}
		// 一包可能解出多帧 (B-frame 重排序后).
		while (true){
			ret = avcodec_receive_frame(m_pVCtx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
			if (ret < 0){
				PostError(ret, AvErrToWStr(ret).c_str());
				break;
			}

			// (a) 硬解 frame 转 SW frame. 硬解模式下 frame->format = m_hwPixFmt (AV_PIX_FMT_D3D11
			//     / DXVA2_VLD), data[] 指 GPU 资源, sws_scale 不认; av_hwframe_transfer_data
			//     把 GPU 像素拷回系统内存, 产生一个 SW frame (典型 AV_PIX_FMT_NV12), 然后照常 sws_scale.
			AVFrame* useFrame = frame;
			if (m_hwAccelActive && m_hwPixFmt != AV_PIX_FMT_NONE &&
			    (AVPixelFormat)frame->format == m_hwPixFmt){
				av_frame_unref(swFrame);
				int rt = av_hwframe_transfer_data(swFrame, frame, 0);
				if (rt < 0){
					// HW->SW 拷贝失败 (常见: GPU 显存不足 / 设备丢失). 跳过此帧,
					// 不上报错误 - 单帧失败不致命, 多帧持续失败用户可通过 OnVideoError
					// 看到 sws_scale 失败而间接察觉. 真正持续失败的话应该上层重 Open 切软解.
					av_frame_unref(frame);
					continue;
				}
				// 保留 frame 的 best_effort_timestamp / pts (transfer_data 已自动 copy
				// metadata, 但保险起见手动同步关键字段).
				swFrame->pts = frame->pts;
				swFrame->best_effort_timestamp = frame->best_effort_timestamp;
				useFrame = swFrame;
			}

			// (b) 建立 / 复用 SwsContext. 第一次见到 useFrame 真实尺寸 / 像素格式时建,
			//     后续若 useFrame 尺寸 / fmt 变 (动态码流 / 硬软切换) 也重建.
			int srcW = useFrame->width;
			int srcH = useFrame->height;
			AVPixelFormat srcFmt = (AVPixelFormat)useFrame->format;
			static thread_local int s_lastSrcW = 0, s_lastSrcH = 0, s_lastSrcFmt = -1;
			if (!m_pSws || s_lastSrcW != srcW || s_lastSrcH != srcH || s_lastSrcFmt != (int)srcFmt){
				if (m_pSws){ sws_freeContext(m_pSws); m_pSws = NULL; }
				m_pSws = sws_getContext(srcW, srcH, srcFmt,
				                        srcW, srcH, AV_PIX_FMT_BGRA,
				                        SWS_BILINEAR, NULL, NULL, NULL);
				s_lastSrcW = srcW;
				s_lastSrcH = srcH;
				s_lastSrcFmt = (int)srcFmt;
				// 视频原始尺寸更新 (避免容器声明值与解码器实际不一致, 比如 SAR/DAR).
				m_videoW = srcW;
				m_videoH = srcH;
			}
			if (!m_pSws){
				av_frame_unref(frame);
				if (useFrame == swFrame) av_frame_unref(swFrame);
				break;
			}

			// (c) sws_scale. 输出连续 BGRA32, pitch = w*4.
			int outW = srcW;
			int outH = srcH;
			int pitch = outW * 4;
			std::vector<uint8_t> bgraBuf((size_t)pitch * outH);
			uint8_t* dst[1] = { bgraBuf.data() };
			int      dstStride[1] = { pitch };
			sws_scale(m_pSws, useFrame->data, useFrame->linesize, 0, srcH, dst, dstStride);

			// (d) 算 PTS 秒. PTS 用 useFrame 的字段 (HW transfer 后已同步).
			int64_t pts = useFrame->best_effort_timestamp;
			if (pts == AV_NOPTS_VALUE) pts = useFrame->pts;
			double sec = VPtsToSec(pts);
			if (sec < 0) sec = 0;

			// (d) 推帧队列. 帧大小可能很大 (1080p ≈ 8MB), 队列容量上限保护内存.
			_XVideo_VideoFrameNode out;
			out.bgra = std::move(bgraBuf);
			out.pitch = pitch;
			out.w = outW;
			out.h = outH;
			out.ptsSec = sec;
			out.eof = false;
			// PushTimeout: 等 frameQ 有空位最多 100ms, 超时丢这帧. 关键设计 -
			//   * 正常播放: UI 60Hz (16ms) 消费 frame, 100ms 内永远 push 成功, 永不丢帧;
			//   * UI 卡死 (resize loop / 死循环): 100ms 后超时丢帧, 防止 frameQ 满 ->
			//     decode 阻塞 -> pktQ 满 -> demux 阻塞 -> audio pktQ 不喂 -> audio render
			//     静音的连锁卡死. video 帧丢失, 但 *已解码* 帧丢, decoder 内部参考链
			//     完整, 后续 P/B 帧解码不受影响 (跟 skip_frame=NONKEY 这种破坏 GOP
			//     的方案根本不同). UI 恢复后画面立即跳到当前位置 (master clock 由 audio
			//     线程独立推进, 跟 UI 卡无关).
			//   * IsClosed 区分 "队列关闭 (Close 流程)" 与 "队列满 (UI 卡)" 两种 false.
			bool pushed = m_videoFrameQ.PushTimeout(std::move(out), 100);
			av_frame_unref(frame);
			// 硬解路径下 swFrame 也要 unref, 否则其内部 buf 引用计数泄漏 (会占用 GPU 资源).
			if (useFrame == swFrame) av_frame_unref(swFrame);
			if (!pushed && m_videoFrameQ.IsClosed()){
				goto exit_loop;
			}
			// (pushed=false 但队列未关闭) = UI 卡 100ms+, 丢这帧, 继续解下一帧.
		}
	}
exit_loop:
	if (frame)   av_frame_free(&frame);
	if (swFrame) av_frame_free(&swFrame);
}

//============================================================================
// 音频解码线程
//============================================================================
void CXVideo::AudioDecodeThreadFn(){
	if (!m_pACtx) return;
	AVFrame* frame = av_frame_alloc();
	if (!frame){
		PostError(AVERROR(ENOMEM), L"av_frame_alloc(audio) 失败");
		return;
	}

	auto flushPushEof = [&](){
		_XVideo_AudioFrameNode eof; eof.eof = true; eof.frames = 0; eof.ptsSec = -1.0;
		m_audioFrameQ.Push(std::move(eof));
	};

	while (!m_quit.load(std::memory_order_acquire)){
		_XVideo_PacketNode node;
		if (!m_audioPktQ.Pop(node)) break;

		if (node.flush){
			avcodec_flush_buffers(m_pACtx);
			av_frame_unref(frame);
			if (node.eof){
				avcodec_send_packet(m_pACtx, NULL);
				while (avcodec_receive_frame(m_pACtx, frame) >= 0){
					av_frame_unref(frame);
				}
				flushPushEof();
				continue;
			}
			continue;
		}
		if (!node.pkt) continue;

		int ret = avcodec_send_packet(m_pACtx, node.pkt);
		av_packet_free(&node.pkt);
		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF){
			PostError(ret, AvErrToWStr(ret).c_str());
			continue;
		}

		while (true){
			ret = avcodec_receive_frame(m_pACtx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
			if (ret < 0){
				PostError(ret, AvErrToWStr(ret).c_str());
				break;
			}

			// (a) lazy InitWasapi & SwrContext. WASAPI 的目标格式由设备决定 (m_audSampleRate /
			//     m_audChannels / m_audOutFmt), 我们要把解码出来的 frame 从源 rate/ch/fmt
			//     重采样到目标. 第一帧时初始化.
			if (!m_pAudClient){
				// AVCodecContext::ch_layout 在 libavcodec 59.24+ (FFmpeg 5.1) 引入,
				// 旧版用 channels 字段. 用 LIBAVCODEC_VERSION_INT 区分.
				if (!InitWasapi(m_pACtx->sample_rate,
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 24, 100)
				                m_pACtx->ch_layout.nb_channels,
#else
				                m_pACtx->channels,
#endif
				                m_pACtx->sample_fmt)){
					// WASAPI 失败: 降级为无声 (后续帧直接丢, 不耽误视频). 不报错给用户,
					// 现实中常见原因是 *无音频设备* 或 *被独占占用*, 不影响视频体验.
					av_frame_unref(frame);
					// 把队列后续的 audio 帧消费掉避免队列堵塞 demux.
					// 简单方案: 这里 break 后线程下次拿帧仍会进来, 反复尝试 WASAPI.
					// 为避免循环冲击, 加 100ms sleep.
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					break;
				}
			}
			if (!m_pSwr){
				m_pSwr = swr_alloc();
				if (!m_pSwr){
					PostError(AVERROR(ENOMEM), L"swr_alloc 失败");
					av_frame_unref(frame);
					break;
				}
				// 输入参数取自 frame (有些 codec 在中途会变 layout, 这里用 frame 直接最稳).
				// swr_ret 在 #if/#else 两个分支都用得到, 提到外部作用域避免重定义.
				int swr_ret = 0;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 24, 100)
				// AVChannelLayout 内部可能含堆数据 (custom layout), 不能 = 浅拷贝.
				// 用 av_channel_layout_copy 做深拷贝, 用完 av_channel_layout_uninit 释放.
				AVChannelLayout in_ch{};
				av_channel_layout_copy(&in_ch, &frame->ch_layout);
				AVChannelLayout out_ch{};
				av_channel_layout_default(&out_ch, m_audChannels);
				swr_ret = swr_alloc_set_opts2(&m_pSwr,
				                               &out_ch, m_audOutFmt, m_audSampleRate,
				                               &in_ch,  (AVSampleFormat)frame->format, frame->sample_rate,
				                               0, NULL);
				av_channel_layout_uninit(&out_ch);
				av_channel_layout_uninit(&in_ch);
				if (swr_ret < 0){
					swr_free(&m_pSwr);
					PostError(swr_ret, L"swr_alloc_set_opts2 失败");
					av_frame_unref(frame); break;
				}
#else
				int64_t in_layout  = frame->channel_layout
				                     ? (int64_t)frame->channel_layout
				                     : av_get_default_channel_layout(frame->channels);
				int64_t out_layout = av_get_default_channel_layout(m_audChannels);
				av_opt_set_int       (m_pSwr, "in_channel_layout",  in_layout, 0);
				av_opt_set_int       (m_pSwr, "out_channel_layout", out_layout, 0);
				av_opt_set_int       (m_pSwr, "in_sample_rate",  frame->sample_rate, 0);
				av_opt_set_int       (m_pSwr, "out_sample_rate", m_audSampleRate, 0);
				av_opt_set_sample_fmt(m_pSwr, "in_sample_fmt",  (AVSampleFormat)frame->format, 0);
				av_opt_set_sample_fmt(m_pSwr, "out_sample_fmt", m_audOutFmt, 0);
#endif
				swr_ret = swr_init(m_pSwr);
				if (swr_ret < 0){
					swr_free(&m_pSwr);
					PostError(swr_ret, L"swr_init 失败");
					av_frame_unref(frame); break;
				}
			}

			// (b) 重采样. swr_convert 输出帧数 = ceil(in * out_rate / in_rate) 上界
			//     (留点余量给可能存在的 SWR 内部 buffer).
			int srcSamples = frame->nb_samples;
			int dstSamples = (int)av_rescale_rnd(
			                    swr_get_delay(m_pSwr, frame->sample_rate) + srcSamples,
			                    m_audSampleRate, frame->sample_rate, AV_ROUND_UP);
			int dstBytes = dstSamples * m_audBytesPerFrame;
			std::vector<uint8_t> pcm((size_t)dstBytes);
			uint8_t* outPlanes[1] = { pcm.data() };
			int converted = swr_convert(m_pSwr, outPlanes, dstSamples,
			                             (const uint8_t**)frame->extended_data, srcSamples);
			if (converted < 0){
				PostError(converted, L"swr_convert 失败");
				av_frame_unref(frame); break;
			}
			pcm.resize((size_t)converted * m_audBytesPerFrame);

			int64_t pts = frame->best_effort_timestamp;
			if (pts == AV_NOPTS_VALUE) pts = frame->pts;
			double sec = APtsToSec(pts);
			if (sec < 0) sec = 0;

			_XVideo_AudioFrameNode out;
			out.pcm     = std::move(pcm);
			out.frames  = converted;
			out.ptsSec  = sec;
			out.eof     = false;
			if (!m_audioFrameQ.Push(std::move(out))){
				av_frame_unref(frame);
				goto exit_loop;
			}
			av_frame_unref(frame);
		}
	}
exit_loop:
	if (frame) av_frame_free(&frame);
}

//============================================================================
// WASAPI 初始化 / 关闭 / 写入
//============================================================================

BOOL CXVideo::InitWasapi(int /*srcSampleRate*/, int /*srcChannels*/, AVSampleFormat /*srcFmt*/){
	// 本 session 之前已 init 失败过 -> 静默返 FALSE, 不再 PostError 刷屏.
	// 否则音频解码线程会每 100ms 反复 retry, 每次都 PostError 一次, 用户的 OnVideoError
	// 回调被刷爆. 现实场景: 无音频设备 / 设备被独占占用 -> 失败稳定不会恢复 -> 一次报告够了.
	// (m_wasapiInitFailed 在 CloseInternal 复位, 下次 Open 重新尝试.)
	if (m_wasapiInitFailed) return FALSE;
	// 用 lambda 包 PostError + 设 fail 标志, 替代每个失败路径手写两行. retVal 永远是 FALSE,
	// 方便 `return PostFail(...)` 一行返回.
	auto PostFail = [this](int code, const wchar_t* msg) -> BOOL {
		m_wasapiInitFailed = true;
		PostError(code, msg);
		return FALSE;
	};

	// COM Init: WASAPI 调用必须在已 CoInitialize 的线程上. AudioRender / AudioDecode 线程
	// 都会需要; 这里在第一次 InitWasapi 的线程上 init, 不 uninit (线程退出时再 uninit
	// 也来得及, 但 thread-local 析构不可靠). 我们在 AudioRender 线程入口做 ComInit_MTA.
	HRESULT hr;

	hr = ::CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
	                        __uuidof(IMMDeviceEnumerator), (void**)&m_pAudEnum);
	if (FAILED(hr) || !m_pAudEnum){
		return PostFail((int)hr, L"创建 MMDeviceEnumerator 失败");
	}
	hr = m_pAudEnum->GetDefaultAudioEndpoint(eRender, eConsole, &m_pAudDevice);
	if (FAILED(hr) || !m_pAudDevice){
		return PostFail((int)hr, L"获取默认音频输出设备失败");
	}
	hr = m_pAudDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&m_pAudClient);
	if (FAILED(hr) || !m_pAudClient){
		return PostFail((int)hr, L"IAudioClient::Activate 失败");
	}

	// 取设备首选格式. 共享模式下必须用这个 (或者用 IsFormatSupported 找最近匹配).
	WAVEFORMATEX* pMix = NULL;
	hr = m_pAudClient->GetMixFormat(&pMix);
	if (FAILED(hr) || !pMix){
		return PostFail((int)hr, L"GetMixFormat 失败");
	}

	// 优先用设备 native 格式 (WAVEFORMATEXTENSIBLE FLOAT 48000 stereo 是常态).
	// 把 FFmpeg 端目标 fmt 设为 FLT (与 IEEE_FLOAT 对应); 若设备是 PCM 16, 用 S16.
	// KSDATAFORMAT_SUBTYPE_IEEE_FLOAT GUID = {00000003-0000-0010-8000-00aa00389b71},
	// 直接硬编码以避开 INITGUID / ksuser.lib 链接依赖 (有些 SDK 没默认引这一套 lib).
	static const GUID kSubFmtIeeeFloat = {
		0x00000003, 0x0000, 0x0010,
		{ 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
	AVSampleFormat targetFmt = AV_SAMPLE_FMT_S16;
	if (pMix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT){
		targetFmt = AV_SAMPLE_FMT_FLT;
	} else if (pMix->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
		WAVEFORMATEXTENSIBLE* pExt = (WAVEFORMATEXTENSIBLE*)pMix;
		if (::IsEqualGUID(pExt->SubFormat, kSubFmtIeeeFloat)){
			targetFmt = AV_SAMPLE_FMT_FLT;
		} else {
			targetFmt = AV_SAMPLE_FMT_S16;
		}
	}

	m_audSampleRate = pMix->nSamplesPerSec;
	m_audChannels   = pMix->nChannels;
	m_audOutFmt     = targetFmt;
	m_audBytesPerFrame = m_audChannels *
	                     ((targetFmt == AV_SAMPLE_FMT_FLT) ? sizeof(float) : sizeof(int16_t));

	// 建一个匹配 targetFmt + 采样率 + 通道数的 WAVEFORMATEX (设备首选格式直接用).
	// 为了避免 IsFormatSupported 查不到最近匹配的麻烦, 我们直接走 GetMixFormat 返回的格式.
	// 所以 targetFmt 必须和 pMix subformat 严格一致 - 上面已根据 subformat 选了.
	// 缓冲: 100ms (1,000,000 hns 单位 = 1 sec). 共享模式最小 buffer ~10ms, 100ms 足够.
	REFERENCE_TIME hnsBuf = 1000 * 10000; // 1s = 10,000,000 hns; 100ms = 1,000,000
	hnsBuf = 100 * 10000;

	hr = m_pAudClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
	                              AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
	                              hnsBuf, 0, pMix, NULL);
	::CoTaskMemFree(pMix);
	pMix = NULL;
	if (FAILED(hr)){
		// 常见: EXCLUSIVE_MODE 已被占用, 或缓冲值无效. 这里不做 retry, 直接报错.
		return PostFail((int)hr, L"IAudioClient::Initialize 失败");
	}

	hr = m_pAudClient->GetBufferSize(&m_audBufFrames);
	if (FAILED(hr)){
		return PostFail((int)hr, L"IAudioClient::GetBufferSize 失败");
	}

	m_hAudioEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!m_hAudioEvent){
		return PostFail((int)::GetLastError(), L"CreateEvent 失败");
	}
	hr = m_pAudClient->SetEventHandle(m_hAudioEvent);
	if (FAILED(hr)){
		return PostFail((int)hr, L"SetEventHandle 失败");
	}
	hr = m_pAudClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_pAudRender);
	if (FAILED(hr) || !m_pAudRender){
		return PostFail((int)hr, L"GetService(AudioRenderClient) 失败");
	}
	// Start: 此时 buffer 是空的, 设备启动后会立刻触发一次事件让我们填. 不预填静音也可以,
	// 共享模式 OS 会用 0 填充直到我们提供数据.
	hr = m_pAudClient->Start();
	if (FAILED(hr)){
		return PostFail((int)hr, L"IAudioClient::Start 失败");
	}
	return TRUE;
}

void CXVideo::ShutdownWasapi(){
	if (m_pAudClient){
		// Stop 后 GetCurrentPadding 可能仍 > 0, 但没关系 - 我们要 Release 整个 client.
		m_pAudClient->Stop();
	}
	SafeRelease(m_pAudRender);
	SafeRelease(m_pAudClient);
	SafeRelease(m_pAudDevice);
	SafeRelease(m_pAudEnum);
	if (m_hAudioEvent){
		::CloseHandle(m_hAudioEvent);
		m_hAudioEvent = NULL;
	}
	m_audBufFrames = 0;
	m_audSampleRate = 0;
	m_audChannels = 0;
	m_audBytesPerFrame = 0;
}

//============================================================================
// 音频渲染线程: 等 WASAPI buffer 空, 写一段 PCM
//============================================================================
void CXVideo::AudioRenderThreadFn(){
	ComInit_MTA com;
	if (FAILED(com.hr) && com.hr != RPC_E_CHANGED_MODE){
		// 几乎不可能, 但严格起见.
		return;
	}

	// 按 Windows 推荐, 调高音频线程 MMCSS 优先级 (Pro Audio).
	DWORD taskIdx = 0;
	HANDLE hMmcss = ::AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIdx);

	while (!m_quit.load(std::memory_order_acquire)){
		// 暂停: 不写 buffer, 让设备自然 underrun (共享模式下 OS 用 0 填充, 听起来就是静音,
		// 不会爆音). 主动 IAudioClient::Stop 也行但 Start/Stop 频繁切换有 OS 开销.
		if (m_paused.load(std::memory_order_acquire)){
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			continue;
		}

		// WASAPI 还没 init (无音频文件 / 初始化失败) -> 仅作 EOF/退出探测.
		if (!m_pAudClient || !m_pAudRender || !m_hAudioEvent){
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		// 等设备需要数据.
		DWORD wait = ::WaitForSingleObject(m_hAudioEvent, 100);
		if (m_quit.load(std::memory_order_acquire)) break;
		if (wait != WAIT_OBJECT_0) continue;   // timeout, 检查 quit / paused

		// 算可写帧数 = bufFrames - padding.
		UINT32 padding = 0;
		HRESULT hr = m_pAudClient->GetCurrentPadding(&padding);
		if (FAILED(hr)) continue;
		UINT32 avail = m_audBufFrames - padding;
		if (avail == 0) continue;

		BYTE* devBuf = NULL;
		hr = m_pAudRender->GetBuffer(avail, &devBuf);
		if (FAILED(hr)) continue;

		// 从 (1) m_audResidualPcm + (2) audioFrameQ 取数据填进设备 buffer.
		// 处理顺序: 先消费上次写不下的残留, 再 pop 新帧. 关键约束: 音频 *样本不能丢*,
		// 否则 ptsSec 推进与实际播放的样本数失配, 导致 audioClock 跑得比 wall 时间快
		// (历史 bug: 一帧 1024 样本只 copy 480, 丢 544 样本但 ptsSec 跳到下一帧 -> 2x 速).
		UINT32 written = 0;
		while (written < avail){
			// 1) 优先消费残留.
			if (m_audResidualFrames > 0){
				UINT32 needFrames = avail - written;
				UINT32 haveFrames = (UINT32)m_audResidualFrames;
				UINT32 copyFrames = (haveFrames < needFrames) ? haveFrames : needFrames;
				ApplyVolumeAndWrite(m_audResidualPcm.data(), (int)copyFrames);
				::memcpy(devBuf + (size_t)written * m_audBytesPerFrame,
				         m_audResidualPcm.data(),
				         (size_t)copyFrames * m_audBytesPerFrame);
				written += copyFrames;

				double sec = m_audResidualPtsSec
				             + (double)copyFrames / (double)m_audSampleRate;
				m_audioClock.store(sec, std::memory_order_release);

				if (haveFrames > copyFrames){
					// 还有剩余, 把头 copyFrames 字节挤掉, 推进 PtsSec.
					int leftFrames = (int)(haveFrames - copyFrames);
					size_t leftBytes = (size_t)leftFrames * m_audBytesPerFrame;
					::memmove(m_audResidualPcm.data(),
					          m_audResidualPcm.data() + (size_t)copyFrames * m_audBytesPerFrame,
					          leftBytes);
					m_audResidualPcm.resize(leftBytes);
					m_audResidualFrames = leftFrames;
					m_audResidualPtsSec = sec;
				} else {
					m_audResidualPcm.clear();
					m_audResidualFrames = 0;
				}
				continue;
			}

			// 2) 残留耗尽, 从队列 pop 新帧.
			_XVideo_AudioFrameNode af;
			if (!m_audioFrameQ.TryPop(af)){
				// 没数据了. 用静音填剩余, 标志 SILENT 让 OS 知道无需混音.
				::memset(devBuf + (size_t)written * m_audBytesPerFrame, 0,
				         (size_t)(avail - written) * m_audBytesPerFrame);
				written = avail;
				m_pAudRender->ReleaseBuffer(written, AUDCLNT_BUFFERFLAGS_SILENT);
				goto next_iter;
			}
			if (af.eof){
				// 写到流末尾. 后续不再有音频, 用静音补完本次. 不退出 - 用户可能 seek 回头.
				::memset(devBuf + (size_t)written * m_audBytesPerFrame, 0,
				         (size_t)(avail - written) * m_audBytesPerFrame);
				written = avail;
				break;
			}

			UINT32 needFrames = avail - written;
			UINT32 haveFrames = (UINT32)af.frames;
			UINT32 copyFrames = (haveFrames < needFrames) ? haveFrames : needFrames;

			// 应用 volume / mute 仅给 *要拷贝的部分*; 多出来的留到 residual 下次再 apply.
			ApplyVolumeAndWrite(af.pcm.data(), (int)copyFrames);
			::memcpy(devBuf + (size_t)written * m_audBytesPerFrame,
			         af.pcm.data(),
			         (size_t)copyFrames * m_audBytesPerFrame);
			written += copyFrames;

			double sec = af.ptsSec + (double)copyFrames / (double)m_audSampleRate;
			m_audioClock.store(sec, std::memory_order_release);

			if (haveFrames > copyFrames){
				// 残留: 多出来的样本搬到 m_audResidualPcm.
				int leftFrames = (int)(haveFrames - copyFrames);
				m_audResidualPcm.assign(
				    af.pcm.data() + (size_t)copyFrames * m_audBytesPerFrame,
				    af.pcm.data() + (size_t)haveFrames * m_audBytesPerFrame);
				m_audResidualFrames = leftFrames;
				m_audResidualPtsSec = sec;   // 残留首样本对应 *已写入完毕之后* 的 PTS
			}
		}
		hr = m_pAudRender->ReleaseBuffer(written, 0);
		(void)hr;
next_iter:
		;
	}

	if (hMmcss){
		::AvRevertMmThreadCharacteristics(hMmcss);
	}
}

void CXVideo::ApplyVolumeAndWrite(uint8_t* pcm, int frames){
	float v = m_muted ? 0.0f : m_volume;
	if (v == 1.0f) return;   // no-op
	if (m_audOutFmt == AV_SAMPLE_FMT_FLT){
		float* p = (float*)pcm;
		int n = frames * m_audChannels;
		for (int i = 0; i < n; ++i) p[i] *= v;
	} else {   // S16
		int16_t* p = (int16_t*)pcm;
		int n = frames * m_audChannels;
		// 整数乘除避免浮点开销. v 量化为 0..256.
		int vi = (int)(v * 256.0f + 0.5f);
		for (int i = 0; i < n; ++i){
			int s = (int)p[i] * vi >> 8;
			if (s >  32767) s =  32767;
			if (s < -32768) s = -32768;
			p[i] = (int16_t)s;
		}
	}
}

//============================================================================
// 错误 -> UI 异步派发
//============================================================================
void CXVideo::PostError(int code, const wchar_t* msg){
	{
		std::lock_guard<std::mutex> lk(m_pendingErrMutex);
		m_pendingErrCode = code;
		m_pendingErrMsg  = msg ? msg : L"";
	}
	m_pendingError.store(true, std::memory_order_release);
	// state 也更新, 让 IsOpen / GetState 立即看到错误.
	// 注意: 错误期间不强制关闭流 - 有些错误是局部的 (单帧解码失败), 流仍可继续.
	// 致命错误 (Open 失败) 的状态置 error 是 Open 在调用 PostError 之后做的.
}

//============================================================================
// 主时钟 / 帧调度 (在 UI 线程的 XE_XC_TIMER 里调度)
//============================================================================

double CXVideo::GetMasterClock() const{
	if (m_hasAudio){
		return m_audioClock.load(std::memory_order_acquire);
	}
	// 无音频: 墙钟. 暂停 / 停止时基准会被 Pause/Play 同步重置.
	if (m_paused.load(std::memory_order_acquire)){
		return m_clockBasePts.load(std::memory_order_acquire);
	}
	DWORD base = m_clockBaseTick.load(std::memory_order_acquire);
	if (base == 0){
		// 还没 Play 过, 时钟保持 0.
		return m_clockBasePts.load(std::memory_order_acquire);
	}
	DWORD now = ::GetTickCount();
	double elapsed = (double)(now - base) / 1000.0;
	return m_clockBasePts.load(std::memory_order_acquire) + elapsed;
}

void CXVideo::TryAdvanceFrame(){
	// 仅当 playing 时才推进. paused / stopped / closed / ended / error 都跳过.
	int s = m_state.load(std::memory_order_acquire);
	if (s != xvideo_state_playing) return;

	// 纯音频文件 (m_hasVideo=false): 没有视频帧队列, 整个原视频路径不适用.
	// 但 position 派发 + EOF 检测 + loop 仍要工作 - 走 master clock (=audio clock) 单独
	// 一条 mini 路径. master clock 由 audio render 线程在写设备 buffer 时同步推进.
	if (!m_hasVideo){
		double pos = GetMasterClock();
		if (pos < 0) pos = 0;
		// Seek in-flight: master clock + m_demuxEof 都可能滞后于真实状态 (demux 还没
		// 处理 seek 请求). 整个 EOF / emit 路径跳过, 等 demux 完成 seek 再继续.
		// 否则: 用户从末尾向前拖动 (pos=duration->30) 时 m_demuxEof 仍是上一轮的 true,
		// 这里会误触发 ended/loop.
		if (m_seekInFlight.load(std::memory_order_acquire)) return;
		// EOF: demux 已读完源, 且 master clock 抵达 duration 附近.
		// audio render 线程在 EOF 后会用静音填 buffer 但不再推 m_audioClock,
		// 最后一次更新的 audioClock = 末帧 ptsSec + frames/sr, 容差 0.2s 兜底.
		// (live stream / 不可识别 duration: m_durationSec=0, 这里不会触发 ended.)
		if (m_demuxEof.load(std::memory_order_acquire) &&
		    m_durationSec > 0 && pos >= m_durationSec - 0.2){
			if (m_loop.load(std::memory_order_acquire)){
				// 跟视频路径同样的 loop 重置: 触发 demux 线程 seek 回 0 + 时钟归零.
				m_seekInFlight.store(true, std::memory_order_release);
				m_seekTargetSec.store(0.0, std::memory_order_release);
				m_seekRequest.store(true, std::memory_order_release);
				m_clockBasePts.store(0.0, std::memory_order_release);
				m_clockBaseTick.store(::GetTickCount(), std::memory_order_release);
				m_audioClock.store(0.0, std::memory_order_release);
				m_lastEmittedPosSec = -1.0;
				m_pendingEnded.store(false, std::memory_order_release);
				return;
			}
			// 非循环: 标 ended + 派发 OnVideoEnded. 不强制清流, 用户可 Seek 回头重播.
			m_state.store(xvideo_state_ended, std::memory_order_release);
			m_pendingEnded.store(true, std::memory_order_release);
			// 落进下面 派发最后一次 position 让 UI 显示到末尾.
		}
		// 节流派发 position 给 UI 控件栏 + 用户回调.
		if (pos - m_lastEmittedPosSec >= kPositionEmitInterval){
			m_lastEmittedPosSec = pos;
			UpdateControlBarPosition(pos);
			if (m_cbPosition) m_cbPosition(this, pos, m_userPosition);
		}
		return;
	}

	double mclock = GetMasterClock();

	// A/V 同步策略: 取一帧, 比较 PTS:
	//   PTS - mclock <  -kDropThreshold   : 严重落后, 丢这帧, 拉下一帧
	//   PTS - mclock <=  kAheadThreshold  : 该上屏 (含早到几毫秒)
	//   PTS - mclock >   kAheadThreshold  : 太早, 不动, 等下次 timer
	// kAheadThreshold 取一个帧间隔 (1/fps) 上限, 视觉无感的早到. kDropThreshold = 0.1s.
	double frameDur = (m_frameRate > 1.0) ? (1.0 / m_frameRate) : (1.0 / 30.0);
	const double kDrop  = 0.10;
	const double kAhead = frameDur;

	// 拉队列, 最多丢 N 帧防止饥饿循环.
	int dropMax = 8;
	for (int i = 0; i < dropMax; ++i){
		_XVideo_VideoFrameNode vf;
		if (!m_videoFrameQ.TryPop(vf)) return;   // 没帧, 等下次 timer
		if (vf.eof){
			// 流尾分两路:
			//   (a) m_loop=true : 循环播放 - 直接 seek 回 0 + 重置时钟, *不* 发 OnVideoEnded.
			//       seek 走 demux 线程的标准 seek 协议 (m_seekTargetSec + m_seekRequest);
			//       同时清掉 m_pendingEnded 防 timer 把上一轮的 ended 派发出去.
			//   (b) m_loop=false: 原行为 - 标 ended, 派发 OnVideoEnded.
			if (m_loop.load(std::memory_order_acquire)){
				m_seekInFlight.store(true, std::memory_order_release);
				m_seekTargetSec.store(0.0, std::memory_order_release);
				m_seekRequest.store(true, std::memory_order_release);
				m_clockBasePts.store(0.0, std::memory_order_release);
				m_clockBaseTick.store(::GetTickCount(), std::memory_order_release);
				m_audioClock.store(0.0, std::memory_order_release);
				m_videoClock.store(0.0, std::memory_order_release);
				m_lastEmittedPosSec = -1.0;
				m_pendingEnded.store(false, std::memory_order_release);
				return;
			}
			m_state.store(xvideo_state_ended, std::memory_order_release);
			m_pendingEnded.store(true, std::memory_order_release);
			return;
		}
		double diff = vf.ptsSec - mclock;
		if (diff < -kDrop && i < dropMax - 1){
			// 丢帧, 继续.
			continue;
		}
		if (diff > kAhead){
			// 太早: 把它塞回去? BoundedQueue 没 push_front. 简化方案: 直接上屏 (视觉
			// 提前几十毫秒, 通常无感). 工业播放器会精确等待, 这里折中.
			// 实际测试 contain 路径下提前 1 frameDur 用户察觉不到.
		}
		// 上屏.
		{
			std::lock_guard<std::mutex> lk(m_curFrameMutex);
			m_curBgra  = std::move(vf.bgra);
			m_curW     = vf.w;
			m_curH     = vf.h;
			m_curPitch = vf.pitch;
			m_curFramePts = vf.ptsSec;
			m_curDirty = true;
		}
		m_videoClock.store(vf.ptsSec, std::memory_order_release);

		// 节流派发进度回调. Seek in-flight 期间 vf 可能是 *seek 前* 残留旧帧, 派发会让
		// UI / 用户回调看到旧位置 ("回弹"). UpdateControlBarPosition 入口已 short-circuit
		// 进度条 + 时间标签, 但 m_cbPosition 用户回调要在这里截掉.
		if (vf.ptsSec - m_lastEmittedPosSec >= kPositionEmitInterval &&
		    !m_seekInFlight.load(std::memory_order_acquire)){
			m_lastEmittedPosSec = vf.ptsSec;
			// 内置控件栏 (如有) 同步进度条 / 时间标签. 用户拖动期间会内部 short-circuit.
			UpdateControlBarPosition(vf.ptsSec);
			// PositionChanged 直接在 UI 线程里调用回调 (我们就是 UI 线程).
			if (m_cbPosition) m_cbPosition(this, vf.ptsSec, m_userPosition);
		}
		RedrawSelf();
		return;
	}
}

//============================================================================
// 定时器: 帧调度 + 事件分发
//============================================================================
int CXVideo::OnTimerImpl(HELE /*hEle*/, UINT timerId, BOOL* /*pbHandled*/){
	if (timerId != kTimerId_Tick) return 0;

	// 1) worker 线程产生的事件 -> UI 线程派发.
	DispatchPendingCallbacks();

	// 2) 视频帧调度.
	TryAdvanceFrame();

	// 3) Scrub pending commit: 用户快速拖动节流期间 Seek 被攒到 m_pendingScrubSec,
	//    每个 tick (16ms) 检查一次, 距上次实际 Seek 超过 kScrubMinIntervalMs 就 commit.
	//    用户停止拖动后, 最后那次微小拖动会在 ≤ kScrubMinIntervalMs + 16ms 内 commit.
	if (m_pendingScrubSec >= 0.0){
		DWORD now = ::GetTickCount();
		if (now - m_lastScrubSeekTick >= kScrubMinIntervalMs){
			double tgt = m_pendingScrubSec;
			m_pendingScrubSec = -1.0;
			m_lastScrubSeekTick = now;
			Seek(tgt);
		}
	}

	// 4) 控件栏 自动隐藏: 看用户活动 tick 是否超 timeout, 是则 hide.
	//    在所有自动隐藏开关里都做了 guard, 全开销 ≈ 几条字段读取 + 一次 GetTickCount, 可忽略.
	EvalAutoHide();
	return 0;
}

void CXVideo::DispatchPendingCallbacks(){
	// stateChanged: 任一 pending 跳变 都代表 m_state 可能变了
	// (opened: opening->stopped/playing; ended: ?->ended; error: ?->error).
	// 脚本末尾 同步 控件栏播放按钮 的 check 状态与 文本, 避免 视频自然结束 后
	// m_hBtnPlay 还停在 ❚❚ / check=TRUE.
	bool stateChanged = false;
	if (m_pendingOpened.exchange(false, std::memory_order_acq_rel)){
		stateChanged = true;
		if (m_cbOpened) m_cbOpened(this, m_userOpened);
	}
	if (m_pendingEnded.exchange(false, std::memory_order_acq_rel)){
		stateChanged = true;
		if (m_cbEnded) m_cbEnded(this, m_userEnded);
	}
	if (m_pendingError.exchange(false, std::memory_order_acq_rel)){
		stateChanged = true;
		int code; std::wstring msg;
		{
			std::lock_guard<std::mutex> lk(m_pendingErrMutex);
			code = m_pendingErrCode;
			msg  = m_pendingErrMsg;
		}
		if (m_cbError) m_cbError(this, code, msg.c_str(), m_userError);
	}
	if (stateChanged) UpdateControlBarPlayState();
}

//============================================================================
// 绘制: 优先 D2D, 拿不到 RT 走 GDI 回退
// XE_PAINT 完全接管: pbHandled=TRUE 跳过 XCGUI 默认绘制 (背景 / 边框 / 焦点框).
// 背色填充 + 视频帧 都由 OnPaintD2D / OnPaintGdi 负责. 没帧也要填背色 (避免
// 下面的其他元素透出).
//============================================================================
int CXVideo::OnPaintImpl(HELE /*hEle*/, HDRAW hDraw, BOOL* pbHandled){
	if (pbHandled) *pbHandled = TRUE;

	ID2D1RenderTarget* rt = (ID2D1RenderTarget*)XDraw_GetD2dRenderTarget(hDraw);
	if (rt){
		OnPaintD2D(rt, hDraw);
	} else {
		HDC hdc = XDraw_GetHDC(hDraw);
		if (hdc) OnPaintGdi(hdc, hDraw);
	}
	return 0;
}

void CXVideo::OnPaintD2D(ID2D1RenderTarget* rt, HDRAW /*hDraw*/){
	// 元素客户区 (物理像素, 已含 DPI). XEle_GetWndClientRectDPI 返窗口客户区为参考系
	// 的元素矩形 (与 D2D RT 同坐标系).
	RECT rcEle;
	XEle_GetWndClientRectDPI(m_hEle, &rcEle);
	int eleW = rcEle.right  - rcEle.left;
	int eleH = rcEle.bottom - rcEle.top;
	if (eleW <= 0 || eleH <= 0) return;

	// *背色填充* (XE_PAINT 接管后手动填, 代替 XCGUI BkInfo). 填整块元素区, 覆盖
	// “续帧未到 / letterbox 边 / 暂停中”等场景. 如果后面的视频帧部分覆盖了,
	// 被覆盖那部分的背色 fillRectangle 也被覆, 不会闪烁 (D2D 为同一帧内部分批提交).
	{
		ID2D1SolidColorBrush* bg = NULL;
		COLORREF c = m_videoBkColor;
		D2D1_COLOR_F d2dC = D2D1::ColorF(
		    GetRValue(c) / 255.0f, GetGValue(c) / 255.0f, GetBValue(c) / 255.0f, 1.0f);
		if (SUCCEEDED(rt->CreateSolidColorBrush(d2dC, &bg)) && bg){
			D2D1_RECT_F rfEle = D2D1::RectF(
			    (float)rcEle.left,  (float)rcEle.top,
			    (float)rcEle.right, (float)rcEle.bottom);
			rt->FillRectangle(rfEle, bg);
			bg->Release();
		}
	}

	// 没帧时 (Closed / Error / 第一帧未到 / 尺寸变零) 背景已填, 直接返回.
	bool hasFrame;
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		hasFrame = (m_curW > 0 && m_curH > 0 && !m_curBgra.empty());
	}
	if (!hasFrame) return;

	// 1) RT 变化 / 尺寸不匹配时重建 D2D Bitmap.
	int srcW, srcH, srcPitch;
	std::vector<uint8_t> localCopy;
	bool dirty;
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		srcW = m_curW;
		srcH = m_curH;
		srcPitch = m_curPitch;
		dirty = m_curDirty;
		// 不在锁里拷整张: 1080p ≈ 8MB, 占锁太久. 先 swap 到 local, 释放锁后再用.
		if (dirty){
			localCopy = m_curBgra;   // 拷一份, 让 worker 线程后续可以更新 m_curBgra
			m_curDirty = false;
		}
	}

	if (rt != m_pLastRT){
		// RT 变了: 旧 Bitmap 不能跨 RT 用, 释放重建.
		SafeRelease(m_pD2DBmp);
		m_pLastRT = rt;
		m_d2dBmpW = m_d2dBmpH = 0;
	}
	if (m_pD2DBmp && (m_d2dBmpW != srcW || m_d2dBmpH != srcH)){
		SafeRelease(m_pD2DBmp);
		m_d2dBmpW = m_d2dBmpH = 0;
	}
	if (!m_pD2DBmp){
		D2D1_BITMAP_PROPERTIES props = {};
		props.pixelFormat.format    = DXGI_FORMAT_B8G8R8A8_UNORM;
		props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
		props.dpiX = 96.0f;
		props.dpiY = 96.0f;
		D2D1_SIZE_U size = { (UINT32)srcW, (UINT32)srcH };
		HRESULT hr = rt->CreateBitmap(size, NULL, 0, &props, &m_pD2DBmp);
		if (FAILED(hr) || !m_pD2DBmp){
			// 创建失败: 不画就好, 不报错给用户.
			return;
		}
		m_d2dBmpW = srcW;
		m_d2dBmpH = srcH;
		// 第一次创建: 必须 CopyFromMemory 一次, 即使 dirty 是 false.
		// 走下面的 dirty 路径 (强制 dirty=true).
		dirty = true;
		if (localCopy.empty()){
			std::lock_guard<std::mutex> lk(m_curFrameMutex);
			localCopy = m_curBgra;
		}
	}

	// 2) 上传新像素 (只在 dirty 时).
	if (dirty && !localCopy.empty() && m_pD2DBmp){
		D2D1_RECT_U dst = { 0, 0, (UINT32)srcW, (UINT32)srcH };
		HRESULT hr = m_pD2DBmp->CopyFromMemory(&dst, localCopy.data(), (UINT32)srcPitch);
		if (FAILED(hr)){
			// 拷贝失败一般是 RT 设备丢失, 静默跳过.
			return;
		}
	}

	// 3) 算目标矩形 (在 物理 像素 元素客户区里, 与 D2D RT 坐标一致).
	RECT rcDstLocal;
	ComputeDestRect(eleW, eleH, &rcDstLocal);
	D2D1_RECT_F dstRect = D2D1::RectF(
	    (float)(rcEle.left + rcDstLocal.left),
	    (float)(rcEle.top  + rcDstLocal.top),
	    (float)(rcEle.left + rcDstLocal.right),
	    (float)(rcEle.top  + rcDstLocal.bottom));
	D2D1_RECT_F srcRect = D2D1::RectF(0.0f, 0.0f, (float)srcW, (float)srcH);

	// cover 模式下 dst 可能比 ele 大 (extends out), 用 PushAxisAlignedClip 限制到元素区.
	bool needClip = (m_fitMode == xvideo_fit_cover) || (m_fitMode == xvideo_fit_original);
	D2D1_RECT_F clipRect;
	if (needClip){
		clipRect = D2D1::RectF(
		    (float)rcEle.left,  (float)rcEle.top,
		    (float)rcEle.right, (float)rcEle.bottom);
		rt->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
	}

	// 4) 画.
	rt->DrawBitmap(m_pD2DBmp, &dstRect, 1.0f,
	               D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &srcRect);

	if (needClip){
		rt->PopAxisAlignedClip();
	}
}

// GDI 离屏 DIB 管理: 同尺寸复用, 异尺寸重建.
// 失败时把所有句柄归零返 false, 调用方 (OnPaintGdi) 直接退出该帧 (不画总比错画好).
bool CXVideo::EnsureGdiDib(int w, int h){
	if (w <= 0 || h <= 0) return false;
	if (m_gdiMemDC && m_gdiDib && m_gdiDibW == w && m_gdiDibH == h){
		return true;   // 命中: 复用
	}
	// 尺寸不一致 -> 释放再重建. (注意: 不在这里 TryRecreate-without-release 偷懒,
	// CreateDIBSection 失败时若已 select 旧 bmp 进新 DC 会泄漏旧 bmp.)
	ReleaseGdiDib();

	m_gdiMemDC = ::CreateCompatibleDC(NULL);
	if (!m_gdiMemDC) return false;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = w;
	bmi.bmiHeader.biHeight      = -h;        // top-down (与视频帧 BGRA 数据布局对齐)
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* pBits = NULL;
	m_gdiDib = ::CreateDIBSection(m_gdiMemDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
	if (!m_gdiDib){
		::DeleteDC(m_gdiMemDC);
		m_gdiMemDC = NULL;
		return false;
	}
	m_gdiOldBmp = (HBITMAP)::SelectObject(m_gdiMemDC, m_gdiDib);
	m_gdiDibW = w;
	m_gdiDibH = h;
	return true;
}

void CXVideo::ReleaseGdiDib(){
	if (m_gdiMemDC && m_gdiOldBmp){
		::SelectObject(m_gdiMemDC, m_gdiOldBmp);
	}
	m_gdiOldBmp = NULL;
	if (m_gdiDib){
		::DeleteObject(m_gdiDib);
		m_gdiDib = NULL;
	}
	if (m_gdiMemDC){
		::DeleteDC(m_gdiMemDC);
		m_gdiMemDC = NULL;
	}
	m_gdiDibW = m_gdiDibH = 0;
}

void CXVideo::OnPaintGdi(HDC hdc, HDRAW hDraw){
	// GDI+ 模式 (XInitXCGUI(FALSE)) 下走这条. 坐标系通过 XCGUI 2025-12 新增的 XDraw_ConvRect
	// 处理 - 它把 "元素客户区坐标 (0..eleW)" 自动转成 "当前 hdc 应该用的坐标", 自带 DPI 适配,
	// 解决 "画布偏移问题": 手动用 XDraw_GetOffset + 元素本地原点假设, 在 XCGUI 内部 hdc viewport
	// transform 状态变化 (例如 resize 瞬间 / 最小化还原 第一次 paint) 时会画错位置 (用户实测画面
	// 跑到 "比 0,0 还靠上"). XDraw_ConvRect 由 XCGUI 自己读 hDraw 内部状态算坐标, 永远对.
	//
	// 直接在 XCGUI 给的 hdc 上多步 GDI 操作 (FillRect + StretchDIBits) 还会出两类问题:
	//   1) "FillRect 已画 / StretchDIBits 还没画" 的中间态被 XCGUI 的 GDI+ 合成抓取
	//      -> 鼠标移动 (高频局部重绘) 时画面上下抽搐.
	//   2) GDI 路径下 ::CreateSolidBrush 拒绝高字节 alpha != 0 的 COLORREF (XCGUI RGBA
	//      宏布局 0xAABBGGRR), 默认 m_videoBkColor=RGBA(0,0,0,0xFF) -> brush 创建失败,
	//      FillRect 实际不擦背景 -> 旧帧像素残留.
	// 解法: 用 *离屏 DIB* 做合成缓冲, 所有绘制在 DIB 内
	// 完成, 最后一次 BitBlt 回屏幕 hdc. 屏幕 hdc 上每帧只看到 *一次* 原子化 BitBlt.

	int eleW = XEle_GetWidth(m_hEle);
	int eleH = XEle_GetHeight(m_hEle);
	int eleWPhys = (int)((float)eleW * m_dpiScale + 0.5f);
	int eleHPhys = (int)((float)eleH * m_dpiScale + 0.5f);
	if (eleWPhys <= 0 || eleHPhys <= 0) return;

	// 元素客户区 (0,0,W,H) -> 画布对齐坐标 (hdc 实际坐标系下的位置). XDraw_ConvRect 是
	// const RECT* 签名但内部 const_cast 原地修改 (XCGUI 风格).
	RECT rcDstHdc = { 0, 0, eleWPhys, eleHPhys };
	XDraw_ConvRect(hDraw, &rcDstHdc);

	// 离屏 DIB. 失败直接退出 (这一帧不画), 屏幕保留上次像素 - 视觉上是 "卡了 1 帧",
	// 用户基本无感; 总比 race 状态画到屏幕好.
	// EnsureGdiDib 内部命中缓存返 true 不重建; 异尺寸时重建并隐式置 m_gdiDibDirty=true,
	// 让本帧重画 DIB.
	bool dibRecreated = (m_gdiDibW != eleWPhys || m_gdiDibH != eleHPhys || !m_gdiDib);
	if (!EnsureGdiDib(eleWPhys, eleHPhys)) return;
	if (dibRecreated) m_gdiDibDirty = true;
	HDC dcMem = m_gdiMemDC;

	// 性能关键: 视频帧 (1080p ≈ 8MB) 每次 OnPaint 都 copy + StretchDIBits 进 DIB, 频繁
	// 拖拽窗口时 paint 高频触发 -> 480MB/s 拷贝 -> UI 线程被吃光, video timer 推不动 ->
	// 视觉上 "视频卡住". 优化: 仅当 *源帧变了* (m_curDirty=true 由 TryAdvanceFrame 上屏
	// 新帧时设) 或 *DIB 尺寸变了* 时才重画 DIB; 否则直接 BitBlt 已画好的 DIB 到屏幕.
	int srcW = 0, srcH = 0;
	std::vector<uint8_t> localCopy;
	bool haveFrameToPaint = false;
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		srcW = m_curW;
		srcH = m_curH;
		if (m_curDirty && srcW > 0 && srcH > 0 && !m_curBgra.empty()){
			localCopy = m_curBgra;        // 锁里 swap-out, 锁外渲染
			m_curDirty = false;
			haveFrameToPaint = true;
		}
	}
	bool needRedrawDib = m_gdiDibDirty || haveFrameToPaint;

	if (needRedrawDib){
		// 1) 背景填充 (DIB 本地坐标 (0,0)~(W,H)).
		//    GDI ::CreateSolidBrush 要求 COLORREF 高字节为 0 (BI_RGB 语义). XCGUI 的
		//    RGBA 宏布局是 0xAABBGGRR, alpha 在高字节. 不 mask 掉 alpha, GDI 把 COLORREF
		//    当 PALETTERGB 解释 -> 行为未定义 (多数情况下返 NULL brush, FillRect 实际不擦).
		RECT rcAll = { 0, 0, eleWPhys, eleHPhys };
		COLORREF gdiBg = m_videoBkColor & 0x00FFFFFF;
		HBRUSH hBg = ::CreateSolidBrush(gdiBg);
		if (hBg){
			::FillRect(dcMem, &rcAll, hBg);
			::DeleteObject(hBg);
		}

		// 2) 视频帧. 没新帧时 (haveFrameToPaint=false 但 m_gdiDibDirty=true, e.g. 元素
		//    刚 resize 过, DIB 重建但当前帧没变), 用 *上次缓存* 的帧重画 DIB - 取一份
		//    锁内只读拷贝 (此时 m_curDirty 仍是 false).
		if (!haveFrameToPaint){
			std::lock_guard<std::mutex> lk(m_curFrameMutex);
			if (srcW > 0 && srcH > 0 && !m_curBgra.empty()){
				localCopy = m_curBgra;
			}
		}
		if (!localCopy.empty()){
			RECT rcDst;
			ComputeDestRect(eleWPhys, eleHPhys, &rcDst);
			int dstX = rcDst.left;
			int dstY = rcDst.top;
			int dstW = rcDst.right  - rcDst.left;
			int dstH = rcDst.bottom - rcDst.top;
			if (dstW > 0 && dstH > 0){
				BITMAPINFO sbmi = {};
				sbmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
				sbmi.bmiHeader.biWidth       = srcW;
				sbmi.bmiHeader.biHeight      = -srcH;
				sbmi.bmiHeader.biPlanes      = 1;
				sbmi.bmiHeader.biBitCount    = 32;
				sbmi.bmiHeader.biCompression = BI_RGB;
				::SetStretchBltMode(dcMem, COLORONCOLOR);
				// cover / original 模式下 dst 可能溢出元素区, DIB 自带边界裁剪 (像素超出
				// DIB 的部分直接丢, 不会穿透到屏幕). 不再需要 IntersectClipRect.
				::StretchDIBits(dcMem,
				                dstX, dstY, dstW, dstH,
				                0, 0, srcW, srcH,
				                localCopy.data(), &sbmi, DIB_RGB_COLORS, SRCCOPY);
			}
		}
		m_gdiDibDirty = false;
	}

	// 3) 整块 BitBlt 回屏幕 hdc. 用 XDraw_ConvRect 已转好的画布坐标 (left/top); 尺寸保持
	//    原始物理像素 (Conv 仅平移, 宽高不变). 单一 BitBlt 是原子的, 解决抽搐 + 旧像素覆盖.
	::BitBlt(hdc,
	         rcDstHdc.left, rcDstHdc.top, eleWPhys, eleHPhys,
	         dcMem, 0, 0, SRCCOPY);
}

//============================================================================
// 公开: 播放控制
//============================================================================

void CXVideo::Play(){
	int s = m_state.load(std::memory_order_acquire);
	if (s == xvideo_state_closed || s == xvideo_state_error) return;
	if (s == xvideo_state_ended){
		// 已播完, 从头开始.
		Seek(0.0);
	}
	m_paused.store(false, std::memory_order_release);
	// 无音频时墙钟基准要更新到 *此刻*, 让暂停期间累积的时间不算进去.
	if (!m_hasAudio){
		double basePts = m_clockBasePts.load(std::memory_order_acquire);
		m_clockBaseTick.store(::GetTickCount(), std::memory_order_release);
		m_clockBasePts.store(basePts, std::memory_order_release);
	}
	m_state.store(xvideo_state_playing, std::memory_order_release);
	UpdateControlBarPlayState();
}

void CXVideo::Pause(){
	int s = m_state.load(std::memory_order_acquire);
	if (s != xvideo_state_playing) return;
	m_paused.store(true, std::memory_order_release);
	// 锁定墙钟基准到当前播放位置. 后续 Play 从这里继续.
	if (!m_hasAudio){
		m_clockBasePts.store(GetMasterClock(), std::memory_order_release);
		m_clockBaseTick.store(0, std::memory_order_release);
	}
	m_state.store(xvideo_state_paused, std::memory_order_release);
	UpdateControlBarPlayState();
}

void CXVideo::Stop(){
	int s = m_state.load(std::memory_order_acquire);
	if (s == xvideo_state_closed || s == xvideo_state_error) return;
	m_paused.store(true, std::memory_order_release);
	m_state.store(xvideo_state_stopped, std::memory_order_release);
	// Seek 回 0 (异步, demux 线程处理).
	m_seekInFlight.store(true, std::memory_order_release);
	m_seekTargetSec.store(0.0, std::memory_order_release);
	m_seekRequest.store(true, std::memory_order_release);
	m_clockBasePts.store(0.0, std::memory_order_release);
	m_clockBaseTick.store(0, std::memory_order_release);
	m_audioClock.store(0.0, std::memory_order_release);
	m_videoClock.store(0.0, std::memory_order_release);
	m_lastEmittedPosSec = -1.0;
	UpdateControlBarPlayState();
	// bForce=true: Stop 设了 m_seekInFlight=true (覆盖 demux 处理 seek 的窗口),
	// 这里要绕过该 short-circuit 立刻把 UI 归零.
	UpdateControlBarPosition(0.0, true);
}

BOOL CXVideo::IsPlaying() const{
	return m_state.load(std::memory_order_acquire) == xvideo_state_playing ? TRUE : FALSE;
}

BOOL CXVideo::IsPaused() const{
	return m_state.load(std::memory_order_acquire) == xvideo_state_paused ? TRUE : FALSE;
}

int CXVideo::GetState() const{
	return m_state.load(std::memory_order_acquire);
}

void CXVideo::Seek(double seconds){
	if (!IsOpen()) return;
	// NaN / Inf 防御: NaN 通过所有 < > 比较, 然后被强转 int64 -> UB. 归零.
	if (!(seconds == seconds)) seconds = 0;
	if (seconds < 0) seconds = 0;
	if (m_durationSec > 0 && seconds > m_durationSec) seconds = m_durationSec;
	// 顺序: 先 m_seekInFlight, 再 m_seekTargetSec, 最后 m_seekRequest.
	// m_seekInFlight 用来截断 UI 倒灌 (UpdateControlBarPosition short-circuit), 必须先于
	// m_seekRequest 才能保证 demux 在看到 m_seekRequest=true 之前 UI 已停止反弹.
	m_seekInFlight.store(true, std::memory_order_release);
	m_seekTargetSec.store(seconds, std::memory_order_release);
	m_seekRequest.store(true, std::memory_order_release);
	m_lastEmittedPosSec = -1.0;
}

double CXVideo::GetDuration() const{
	return m_durationSec;
}

double CXVideo::GetPosition() const{
	if (!IsOpen()) return 0.0;
	// 优先用主时钟 (与音视频同步保持一致). seek 期间 master clock 暂时是旧值,
	// 不影响 UI 视觉 - 用户拖进度条松手后第一次重画进度自然回正.
	double pos = GetMasterClock();
	if (pos < 0) pos = 0;
	return pos;
}

int    CXVideo::GetVideoWidth() const  { return m_videoW; }
int    CXVideo::GetVideoHeight() const { return m_videoH; }
double CXVideo::GetFrameRate() const   { return m_frameRate; }
BOOL   CXVideo::HasAudio() const       { return m_hasAudio ? TRUE : FALSE; }

void CXVideo::SetVolume(float v01){
	float prev = m_volume;
	// NaN / Inf 防御: 用户调 SetVolume(0.0/0.0) 之类拿到 NaN, NaN 的所有 < > 比较都 false,
	// 会直接落进 m_volume, 乘到样本上 -> 输出爆音 / 全静音 (取决于 FPU 状态). 一次性 fix.
	if (!(v01 == v01) || v01 < 0.0f) v01 = 0.0f;  // (v != v) 是 NaN 唯一可移植判定
	if (v01 > 1.0f) v01 = 1.0f;
	m_volume = v01;
	// 跨 0 边界 -> voice.svg / voice_mute.svg 需切换, 重绘音量按钮.
	// 进一步: 选择背后画的 svg 是 逻辑 m_muted || m_volume<=0, 跨边界 才需重绘.
	BOOL wasMute = (prev <= 0.0f) ? TRUE : FALSE;
	BOOL nowMute = (m_volume <= 0.0f) ? TRUE : FALSE;
	if (wasMute != nowMute && m_hBtnVolume){
		XEle_Redraw(m_hBtnVolume, FALSE);
	}
}
float CXVideo::GetVolume() const { return m_volume; }
void  CXVideo::SetMute(BOOL bMute){
	BOOL prev = m_muted;
	m_muted = bMute ? TRUE : FALSE;
	// 静音状态变了 -> voice.svg <-> voice_mute.svg 互换, 重绘音量按钮.
	if (prev != m_muted && m_hBtnVolume){
		XEle_Redraw(m_hBtnVolume, FALSE);
	}
}
BOOL  CXVideo::IsMuted() const { return m_muted; }

void CXVideo::SetLoop(BOOL bLoop){
	m_loop.store(bLoop ? true : false, std::memory_order_release);
	UpdateControlBarLoopState();
}
BOOL CXVideo::GetLoop() const {
	return m_loop.load(std::memory_order_acquire) ? TRUE : FALSE;
}

void CXVideo::EnableControlBar(BOOL bEnable){
	m_ctrlBarEnabled = bEnable ? TRUE : FALSE;
	// Create() 之前调: 仅记 flag, 后续 Create 用. Create() 之后调: 不动态 add/remove
	// 控件 (复杂度不值得), 调用方应在 Create 前设定.
}
BOOL CXVideo::IsControlBarEnabled() const { return m_ctrlBarEnabled; }

// ===== 控件栏 自动隐藏 (YouTube 风格) =====
// 默认开启 2.5s. 鼠标活动 (移动 / 点击) 重置 tick + 显示; 16ms 定时器里 tick 超
// timeout 即隐藏. 音量面板可见时不隐藏 (用户正在调音量).
void CXVideo::EnableControlBarAutoHide(BOOL bEnable){
	BOOL b = bEnable ? TRUE : FALSE;
	if (b == m_autoHideCtrlBar) return;
	m_autoHideCtrlBar = b;
	if (!b){
		// 关闭自动隐藏 -> 强制显示 bar (用户已显式禁用 隐藏行为).
		if (m_hCtrlBar){
			XWidget_Show((HXCGUI)m_hCtrlBar, TRUE);
			// 如果 bar 之前隐藏, 重新显示要走一遍 reflow + redraw.
			ReflowControlBar();
		}
	} else {
		// 重新启用 -> 重置 tick, 让 timer 自然 tick down 到隐藏.
		m_lastUserActivityTick = GetTickCount();
	}
}
BOOL CXVideo::IsControlBarAutoHideEnabled() const { return m_autoHideCtrlBar; }

void CXVideo::SetControlBarAutoHideTimeout(int ms){
	if (ms < 100)   ms = 100;     // 太短不可用 (鼠标动一下立刻消失)
	if (ms > 60000) ms = 60000;   // 太长意义不大 (>1min)
	m_autoHideTimeoutMs = (DWORD)ms;
}
int CXVideo::GetControlBarAutoHideTimeout() const { return (int)m_autoHideTimeoutMs; }

void CXVideo::NotifyUserActivity(){
	m_lastUserActivityTick = GetTickCount();
	if (m_autoHideCtrlBar && m_hCtrlBar && !XWidget_IsShow((HXCGUI)m_hCtrlBar)){
		XWidget_Show((HXCGUI)m_hCtrlBar, TRUE);
		// bar 隐藏期间窗口可能 resize 过, layout 引擎对隐藏子节点可能跳过 reflow.
		// 重新显示时强制走一遍 reflow + redraw, 不然 bar / 子控件位置是 stale 的.
		ReflowControlBar();
	}
}

void CXVideo::EvalAutoHide(){
	if (!m_autoHideCtrlBar) return;
	if (!m_hCtrlBar)        return;
	// 音量面板可见时保持 bar 可见 (避免 bar 一消失, 用户拖音量 slider 失去参照).
	if (IsVolumePanelVisible()) return;
	// 进度条 scrub 锁定窗口内绝不隐藏 (双保险, 跟 OnSliderProgressChange 里 Notify-
	// UserActivity 互补): 即便有 race 让 activity tick 没及时刷, 这里也兜底.
	if (m_lastScrubTick != 0 &&
	    (GetTickCount() - m_lastScrubTick) < kSliderLockMs){
		return;
	}
	if (!XWidget_IsShow((HXCGUI)m_hCtrlBar)) return;
	DWORD elapsed = GetTickCount() - m_lastUserActivityTick;
	if (elapsed > m_autoHideTimeoutMs){
		XWidget_Show((HXCGUI)m_hCtrlBar, FALSE);
		// XC_EnableAutoRedrawUI=FALSE 时 Show 不带重绘, bar 原区域留旧像素. 强制重绘.
		if (m_hEle) XEle_Redraw(m_hEle, FALSE);
	}
}

void CXVideo::HookMouseActivity(HELE h){
	if (!h) return;
	XEle_RegEventCPP1(h, XE_MOUSEMOVE, &CXVideo::OnMouseMoveActivity);
}

void CXVideo::ReflowControlBar(){
	if (!m_hEle) return;
	// AdjustLayout 把 m_hEle 内部的 CXLayout 引擎重新跑一遍, 把 m_hCtrlBar 摆到当前
	// m_hEle 尺寸对应的正确位置 + 给 bar 内部的 5 个子控件再排一次.
	// (注: m_hEle 是 CXLayout, 调它的 AdjustLayout 等于让自己 + 所有 layout 子节点 reflow.)
	XEle_AdjustLayout(m_hEle);
	// XC_EnableAutoRedrawUI=FALSE 时 AdjustLayout 不带重绘, 必须手动 Redraw, 否则像素是旧的.
	XEle_Redraw(m_hEle, FALSE);
}

int CXVideo::OnMouseMoveActivity(HELE /*hEle*/, UINT /*nFlags*/, POINT* /*pPt*/, BOOL* /*pbHandled*/){
	NotifyUserActivity();
	return 0;
}

// ===== 控件栏 样式 (颜色) =====
// XCGUI 颜色 = ARGB (高字节 alpha). 用户传 Windows RGB() 会得到 alpha=0 全透明.
// 务必通过 XCGUI 的 RGBA(r,g,b,a) 或直接 0xAARRGGBB 形式给值.
void CXVideo::SetControlBarColor(COLORREF color){
	m_ctrlBarBg = color;
	if (!m_hCtrlBar) return;
	// AddBkFill 是 *追加* 一层填充, 多次调会叠 N 层. 这里先 ClearBkInfo 清掉之前的, 再加.
	XEle_ClearBkInfo(m_hCtrlBar);
	XEle_AddBkFill(m_hCtrlBar, element_state_flag_focus_no, m_ctrlBarBg);
	XEle_Redraw(m_hCtrlBar, FALSE);
}
COLORREF CXVideo::GetControlBarColor() const { return m_ctrlBarBg; }

void CXVideo::SetControlBarTextColor(COLORREF color){
	m_ctrlBarFg = color;
	if (m_hBtnPlay)   XEle_SetTextColor(m_hBtnPlay,   m_ctrlBarFg);
	if (m_hBtnLoop)   XEle_SetTextColor(m_hBtnLoop,   m_ctrlBarFg);
	if (m_hBtnVolume) XEle_SetTextColor(m_hBtnVolume, m_ctrlBarFg);
	if (m_hLblTime)   XShapeText_SetTextColor(m_hLblTime, m_ctrlBarFg);
	if (m_hCtrlBar)   XEle_Redraw(m_hCtrlBar, FALSE);
}
COLORREF CXVideo::GetControlBarTextColor() const { return m_ctrlBarFg; }

// ===== 控件栏 子控件 getter =====
// 外部拿到 HELE 后可调 XCGUI 任意 API 自由定制 (字体, 颜色, bkInfoM, 注册更多事件).
// 全部 thread-unsafe (只读 UI 线程私有成员), 用户应在 UI 线程访问.
HELE   CXVideo::GetControlBar()    const { return m_hCtrlBar;        }
HELE   CXVideo::GetBtnPlay()       const { return m_hBtnPlay;        }
HELE   CXVideo::GetBtnLoop()       const { return m_hBtnLoop;        }
HELE   CXVideo::GetSliderProgress()const { return m_hSliderProgress; }
HXCGUI CXVideo::GetLblTime()       const { return m_hLblTime;        }
HELE   CXVideo::GetBtnVolume()     const { return m_hBtnVolume;      }
HELE   CXVideo::GetVolumePanel()   const { return m_hVolPanel;       }
HELE   CXVideo::GetSliderVolume()  const { return m_hSliderVolume;   }

void CXVideo::SetFitMode(int mode){
	if (mode < 0 || mode > xvideo_fit_original) return;
	if (mode == m_fitMode) return;
	m_fitMode = mode;
	RedrawSelf();
}
int CXVideo::GetFitMode() const { return m_fitMode; }

void CXVideo::SetVideoBkColor(COLORREF color){
	if (m_videoBkColor == color) return;
	m_videoBkColor = color;
	RebuildBkInfo();
}
COLORREF CXVideo::GetVideoBkColor() const { return m_videoBkColor; }

void CXVideo::SetMaxQueueSize(int packetCap, int frameCap){
	if (packetCap < 1)  packetCap = 1;
	if (frameCap  < 1)  frameCap  = 1;
	m_packetCap = packetCap;
	m_frameCap  = frameCap;
	m_videoPktQ.SetCapacity((size_t)packetCap);
	m_audioPktQ.SetCapacity((size_t)packetCap);
	m_videoFrameQ.SetCapacity((size_t)frameCap);
	m_audioFrameQ.SetCapacity((size_t)frameCap);
}

//============================================================================
// 硬件解码 公开 API
//============================================================================
void CXVideo::SetHwAccel(int type){
	if (type < xvideo_hwaccel_none || type > xvideo_hwaccel_dxva2) return;
	m_hwAccelPref = type;
	// 不在此处直接做硬解初始化: 当前若已 Open 也不重建解码器 (避免抢锁), 留给下次 Open
	// 时 TryInitHwAccel 读取本字段并生效.
}

int CXVideo::GetHwAccel() const {
	return m_hwAccelPref;
}

BOOL CXVideo::IsHwAccelActive() const {
	return m_hwAccelActive ? TRUE : FALSE;
}

//============================================================================
// 事件回调注册
//============================================================================
void CXVideo::OnVideoOpened(XVIDEO_PROC_OPENED fn, void* pUser){
	m_cbOpened   = fn;
	m_userOpened = pUser;
}
void CXVideo::OnVideoEnded(XVIDEO_PROC_ENDED fn, void* pUser){
	m_cbEnded    = fn;
	m_userEnded  = pUser;
}
void CXVideo::OnVideoError(XVIDEO_PROC_ERROR fn, void* pUser){
	m_cbError    = fn;
	m_userError  = pUser;
}
void CXVideo::OnVideoPositionChanged(XVIDEO_PROC_POSITION fn, void* pUser){
	m_cbPosition   = fn;
	m_userPosition = pUser;
}

//============================================================================
// 控件栏 实现
//
// 设计目标:
//   1. 视觉: 元素底部 ~40px 深色覆盖条, 5 个控件横排 (Play | Loop | Progress | Time | Vol).
//   2. 行为: 点 Play -> 切播放/暂停; 点 Loop -> 切循环模式; 拖 Progress -> Seek;
//            点 Vol -> 弹 *上方* popup 窗口 + 垂直 slider 调音量.
//   3. 复用 CXVideo 现有 Play/Pause/Seek/SetVolume/SetLoop API, 不复制状态机.
//   4. 用户拖进度条期间, UpdateControlBarPosition 短路, 不被 frame timer 倒灌干扰.
//   5. 程式化 SetPos 期间 m_programmaticSliderUpdate=TRUE, 让 OnSliderProgressChange
//      跳过, 避免 SetPos -> XE_SLIDERBAR_CHANGE -> OnSliderProgressChange -> Seek
//      -> position 回来 -> SetPos 死循环.
//   6. 所有 Update* 对 m_hCtrlBar=NULL 都安全短路 - 调 EnableControlBar(FALSE) 用户
//      或 Create() 前控件栏未建时, Play/Pause/Seek 等仍可正常用, 只是没 UI 反馈.
//============================================================================

std::wstring CXVideo::FormatDuration(double seconds){
	if (seconds < 0 || !(seconds == seconds)) seconds = 0;  // 负数 / NaN 归零
	int total = (int)(seconds + 0.5);
	int h = total / 3600;
	int m = (total % 3600) / 60;
	int s = total % 60;
	wchar_t buf[32];
	if (h > 0){
		swprintf_s(buf, 32, L"%d:%02d:%02d", h, m, s);
	} else {
		swprintf_s(buf, 32, L"%d:%02d", m, s);
	}
	return std::wstring(buf);
}

void CXVideo::CreateControlBar(){
	if (!m_hEle || m_hCtrlBar) return;
	// 注意: 此时 m_hEle 可能是 0x0 大小! 常见场景:
	//     video.Create(0, 0, 0, 0, layoutParent);
	//     video.LayoutItem_SetWidth(fill, 0);  // 大小在后续 reflow 才有值
	// 所以 *不能* 因为 GetWidth/Height <= 0 就 return. 控件栏照建, 初始占位, 等
	// XE_ADJUSTLAYOUT 来了 layout 引擎会把所有子节点 reflow 到正确尺寸.
	int eleW = XEle_GetWidth(m_hEle);
	if (eleW <= 0) eleW = 100;   // 占位宽度, 后续 reflow 会覆盖

	// 控件栏 = CXLayout. 初始 x/y/w/h 只是占位; XCGUI 的 layout 引擎按下面
	// 设置的 LayoutItem_SetWidth(fill) / LayoutItem_SetHeight(40) 自动 reflow.
	m_hCtrlBar = XLayout_Create(0, 0, eleW, kCtrlBarH, (HXCGUI)m_hEle);
	if (!m_hCtrlBar) return;
	// 控件栏本身 *默认鼠标穿透* (CXLayout default TRUE), 我们不主动关闭. 这样:
	//   - 子控件 (按钮 / slider) 仍接到自己的鼠标事件 (穿透只影响容器本身);
	//   - 控件栏空白处点击会透到 m_hEle, 触发 OnLButtonUpVideo 切暂停 - 跟 youtube 一致.
	// 控件栏 *自己* 不需要任何事件处理 (无需 RegEvent XE_LBUTTONUP).
	XLayoutBox_EnableHorizon(m_hCtrlBar, TRUE);
	XLayoutBox_SetAlignV(m_hCtrlBar, layout_align_center);
	XLayoutBox_SetSpace(m_hCtrlBar, kGap);
	XEle_SetPadding(m_hCtrlBar, 12, 0, 12, 0);
	// 控件栏作为 m_hEle 的 layout 项: 宽 fill 父宽, 高固定 40.
	// m_hEle 是 vertical + alignV=bottom 布局, 自然把这唯一子节点贴底部.
	XWidget_LayoutItem_SetWidth ((HXCGUI)m_hCtrlBar, layout_size_fill,  0);
	XWidget_LayoutItem_SetHeight((HXCGUI)m_hCtrlBar, layout_size_fixed, kCtrlBarH);
	// 控件栏 bg. m_ctrlBarBg 由用户通过 SetControlBarColor 修改, 默认深色不透明.
	// 注意: CXLayout 默认可能 EnableBkTransparent=TRUE, 必须显式关掉, 否则 AddBkFill 不显.
	XEle_EnableBkTransparent(m_hCtrlBar, FALSE);
	XEle_AddBkFill(m_hCtrlBar, element_state_flag_focus_no, m_ctrlBarBg);
	// 控件栏自身不画边框 (CXLayout default 可能画一圈细线).
	XEle_EnableDrawBorder(m_hCtrlBar, FALSE);

	// ---- 子控件 ---- (所有控件: transparent bg + 白色文字 + 不画默认边框 + tooltip;
	//      用 LayoutItem 声明尺寸, 不再传 x/y, 由 layout 引擎按声明顺序左->右排列)

	// XCGUI 的 COLORREF 是 ARGB, 透明像素 alpha=0; 不能用 Windows RGB(255,255,255) (alpha=0).
	// 用 kCtrlBarFg = RGBA(255,255,255,255) 保证 *不透明白*.
	// IDC_HAND: 鼠标停在按钮上变小手指, 提示 "可点". 系统共享句柄, 不需 DestroyCursor.
	HCURSOR hCurHand = ::LoadCursorW(NULL, IDC_HAND);
	auto styleButton = [this, hCurHand](HELE h, const wchar_t* tip){
		XEle_EnableBkTransparent(h, TRUE);
		XEle_EnableDrawBorder(h, FALSE);              // 关默认边框
		XEle_EnableDrawFocus(h, FALSE);               // 关 焦点虚线框 (XCGUI 默认会画一圈虚线)
		XEle_SetTextColor(h, m_ctrlBarFg);
		if (hCurHand) XEle_SetCursor(h, hCurHand);    // 小手指光标 (LoadCursor 失败 fallback 默认箭头)
		if (tip) XEle_SetToolTip(h, tip);             // 悬停提示
	};

	// Play / Pause (40x40 固定). button_type_check: 点击自动 toggle check 状态,
	// XCGUI 在重绘时会同时走 选中 / 未选中 两套背景, 能直接表达 "正在播放" 的状态;
	// 事件走 XE_BUTTON_CHECK 拿到新的 bCheck, 不需再去 load m_state 翻译.
	m_hBtnPlay = XBtn_Create(0, 0, kBtnW, kBtnH, L"▶", (HXCGUI)m_hCtrlBar);
	XBtn_SetTypeEx(m_hBtnPlay, button_type_check);
	styleButton(m_hBtnPlay, L"播放/暂停");
	XWidget_LayoutItem_SetWidth ((HXCGUI)m_hBtnPlay, layout_size_fixed, kBtnW);
	XWidget_LayoutItem_SetHeight((HXCGUI)m_hBtnPlay, layout_size_fixed, kBtnH);
	XEle_RegEventCPP1(m_hBtnPlay, XE_BUTTON_CHECK, &CXVideo::OnBtnPlayCheck);
	// XE_PAINT: 接管绘制, 根据 m_state 在中心画 play.svg / suspend.svg.
	XEle_RegEventCPP1(m_hBtnPlay, XE_PAINT, &CXVideo::OnPaintBtnPlay);

	// Loop (40x40 固定). 同上用 button_type_check, check==SetLoop 完全一一对应.
	m_hBtnLoop = XBtn_Create(0, 0, kBtnW, kBtnH, L"↻", (HXCGUI)m_hCtrlBar);
	XBtn_SetTypeEx(m_hBtnLoop, button_type_check);
	styleButton(m_hBtnLoop, L"循环播放");
	XWidget_LayoutItem_SetWidth ((HXCGUI)m_hBtnLoop, layout_size_fixed, kBtnW);
	XWidget_LayoutItem_SetHeight((HXCGUI)m_hBtnLoop, layout_size_fixed, kBtnH);
	XEle_RegEventCPP1(m_hBtnLoop, XE_BUTTON_CHECK, &CXVideo::OnBtnLoopCheck);
	XEle_RegEventCPP1(m_hBtnLoop, XE_PAINT, &CXVideo::OnPaintBtnLoop);

	// Progress slider (weight=1 弹性, 吃掉中间剩余宽度)
	m_hSliderProgress = XSliderBar_Create(0, 0, 100, kSliderH, (HXCGUI)m_hCtrlBar);
	XSliderBar_EnableHorizon(m_hSliderProgress, TRUE);
	XSliderBar_SetRange(m_hSliderProgress, kProgressRange);
	// 滑块 固定圆形 直径 = kSliderThumbDia. 默认 XCGUI 给的尺寸跳跳, 手动锁为正方形.
	XSliderBar_SetButtonWidth (m_hSliderProgress, kSliderThumbDia);
	XSliderBar_SetButtonHeight(m_hSliderProgress, kSliderThumbDia);
	XEle_EnableBkTransparent(m_hSliderProgress, TRUE);
	// XCGUI 的 slider: 单纯 EnableDrawBorder(FALSE) 不够, 还得 EnableDrawFocus(FALSE)
	// 才能完全去掉默认的 "焦点虚线框" (XCGUI 的一个小怪异行为, 但既然 API 在就调).
	XEle_EnableDrawBorder(m_hSliderProgress, FALSE);
	XEle_EnableDrawFocus (m_hSliderProgress, FALSE);
	XWidget_LayoutItem_SetWidth ((HXCGUI)m_hSliderProgress, layout_size_weight, 1);
	XWidget_LayoutItem_SetHeight((HXCGUI)m_hSliderProgress, layout_size_fixed, kSliderH);
	// 视觉补偿: 按钮 SVG 居中在 36px 框里自带留白, 而 slider 轨道画到客户区两端 ->
	// 看上去 slider 与 邻居 比 按钮-按钮 间隙小. 加 10px 外边距把 slider 缩进, 视觉统一.
	XWidget_LayoutItem_SetMargin((HXCGUI)m_hSliderProgress, 10, 0, 10, 0);
	XEle_RegEventCPP1(m_hSliderProgress, XE_SLIDERBAR_CHANGE,
	                  &CXVideo::OnSliderProgressChange);
	// XE_PAINT: 接管轨道绘制; 滑块 走 m_hSliderProgress 的 button 子元素的 paint.
	XEle_RegEventCPP1(m_hSliderProgress, XE_PAINT, &CXVideo::OnPaintSlider);
	if (HELE hThumb = XSliderBar_GetButton(m_hSliderProgress)){
		XEle_EnableBkTransparent(hThumb, TRUE);
		XEle_EnableDrawBorder   (hThumb, FALSE);
		XEle_EnableDrawFocus    (hThumb, FALSE);
		XEle_RegEventCPP1(hThumb, XE_PAINT, &CXVideo::OnPaintSliderThumb);
	}

	// Time 标签 (CXShapeText, auto-size 跟文字宽). shape 没 EnableBkTransparent -
	// 它本身就是绘图形状, 不带背景. ARGB 颜色: alpha 必须 255.
	m_hLblTime = XShapeText_Create(0, 0, 100, kBtnH, L"0:00 / 0:00", (HXCGUI)m_hCtrlBar);
	if (m_hLblTime){
		XShapeText_SetTextColor(m_hLblTime, m_ctrlBarFg);
		XWidget_LayoutItem_SetWidth (m_hLblTime, layout_size_auto, 0);
		XWidget_LayoutItem_SetHeight(m_hLblTime, layout_size_auto, 0);
	}

	// Volume (40x40 固定)
	m_hBtnVolume = XBtn_Create(0, 0, kBtnW, kBtnH, L"🔊", (HXCGUI)m_hCtrlBar);
	styleButton(m_hBtnVolume, L"音量");
	XWidget_LayoutItem_SetWidth ((HXCGUI)m_hBtnVolume, layout_size_fixed, kBtnW);
	XWidget_LayoutItem_SetHeight((HXCGUI)m_hBtnVolume, layout_size_fixed, kBtnH);
	XEle_RegEventCPP1(m_hBtnVolume, XE_BNCLICK, &CXVideo::OnBtnVolumeClick);
	XEle_RegEventCPP1(m_hBtnVolume, XE_PAINT, &CXVideo::OnPaintBtnVolume);

	// 触发一次 reflow, 让子控件立即就位 (后续窗口缩放 XE_ADJUSTLAYOUT 自动再算).
	XEle_AdjustLayout(m_hEle);

	// 自动隐藏: 每个控件栏元素都挂上 XE_MOUSEMOVE -> NotifyUserActivity,
	// 这样鼠标悬停按钮 / slider / 拖进度条 时 bar 也不会消失.
	// (m_hEle 已在 InstallEvents 挂过, 不重复.)
	HookMouseActivity(m_hCtrlBar);
	HookMouseActivity(m_hBtnPlay);
	HookMouseActivity(m_hBtnLoop);
	HookMouseActivity(m_hSliderProgress);
	HookMouseActivity(m_hBtnVolume);
	// 时间标签 m_hLblTime 是 HXCGUI shape 不是 HELE; shape 不接收鼠标事件, 跳过.

	// SVG 图标 及时加载 (幂等, 安全). 需要在首次 paint 前准备好, 否则首帧画个空.
	EnsureSvgsLoaded();

	// 音量面板 + slider 提前建好 (初始隐藏), 避免 "首次点 vol" 才能拿句柄.
	// 这样用户在 Create() 后就能 GetVolumePanel/GetSliderVolume + 改颜色/添额外事件.
	CreateVolumePanel();

	// bar 初始可见, tick = now. 用户 timeout 内有任何活动会续命, 否则隐藏.
	m_lastUserActivityTick = GetTickCount();

	// 初始状态同步.
	UpdateControlBarPlayState();
	UpdateControlBarLoopState();
	UpdateControlBarPosition(0.0);
}

void CXVideo::UpdateControlBarPosition(double posSec, bool bForce){
	if (!m_hCtrlBar) return;
	// Seek in-flight: demux 还没处理完 seek 请求, audio render 可能仍在消费旧帧 ->
	// posSec 是 *seek 之前* 的旧时钟. 这时候去更新进度条 / 时间标签, 用户会看到 slider
	// 弹回旧位置 + 时间数字闪一下旧值, 也就是 "回弹" 现象. 整体跳过这一帧, 等 demux
	// 完成 seek 后下一次 emit 自然显示新位置.
	// bForce=true 时强制更新 (Stop 立刻把 UI 归零, 不能被 short-circuit 截掉).
	if (!bForce && m_seekInFlight.load(std::memory_order_acquire)) return;
	// Slider + 时间标签 锁定: 用户最近一次 OnSliderProgressChange 之后 kSliderLockMs 内,
	// 整个 UI 不被主时钟反推. 修一个细回弹: av_seek_frame BACKWARD 落在最近 keyframe (可能
	// 比 tgt 早几秒), audio render 后续推 m_audioClock = keyframe ptsSec, UpdateControl-
	// BarPosition 把 slider/标签拉回 keyframe 位置 -> 跟用户拖到的位置不一致. 锁定窗口期间
	// 标签已由 OnSliderProgressChange 直接设到用户拖到的秒数, slider 也已 SetPos 完成,
	// 直接 return 让它们保持. bForce 仍然能强制刷 (Stop 路径).
	if (!bForce && m_lastScrubTick != 0 &&
	    (::GetTickCount() - m_lastScrubTick) < kSliderLockMs){
		return;
	}
	double dur = GetDuration();

	// 进度条: 仅在 *非* 用户拖动期间更新. 用户拖动时 OnSliderProgressChange 已收
	// 真实位置, 不需要再倒灌.
	if (m_hSliderProgress && !m_programmaticSliderUpdate){
		int newPos = 0;
		if (dur > 0.0){
			double ratio = posSec / dur;
			if (ratio < 0.0) ratio = 0.0;
			if (ratio > 1.0) ratio = 1.0;
			newPos = (int)(ratio * kProgressRange + 0.5);
		}
		if (newPos != m_lastProgressPos){
			m_lastProgressPos = newPos;
			// 程式化更新: 置位防止 XE_SLIDERBAR_CHANGE 回调把它误识为用户操作.
			m_programmaticSliderUpdate = TRUE;
			XSliderBar_SetPos(m_hSliderProgress, newPos);
			m_programmaticSliderUpdate = FALSE;
		}
	}

	// 时间标签: "M:SS / M:SS".
	if (m_hLblTime){
		std::wstring nowStr = FormatDuration(posSec);
		std::wstring durStr = FormatDuration(dur);
		std::wstring txt = nowStr + L" / " + durStr;
		if (txt != m_lastTimeStr){
			m_lastTimeStr = txt;
			// CXShapeText 用 XShapeText_SetText (不是 XBtn_SetText).
			XShapeText_SetText(m_hLblTime, txt.c_str());
			// 文字长度变了, 触发一次 reflow 让 auto 宽度的标签重新算尺寸.
			if (m_hCtrlBar) XEle_AdjustLayout(m_hCtrlBar);
		}
	}
}

void CXVideo::UpdateControlBarPlayState(){
	if (!m_hBtnPlay) return;
	int s = m_state.load(std::memory_order_acquire);
	BOOL playing = (s == xvideo_state_playing) ? TRUE : FALSE;
	const wchar_t* glyph = playing ? L"❚❚" : L"▶";
	// XBtn_SetCheck 会触发 XE_BUTTON_CHECK -> OnBtnPlayCheck, 后者看到 程式化 标志
	// 会直接 return, 避免 Play/Pause -> UpdateControlBarPlayState 死循环.
	m_programmaticBtnCheck = TRUE;
	XBtn_SetCheck(m_hBtnPlay, playing);
	XBtn_SetText (m_hBtnPlay, glyph);
	m_programmaticBtnCheck = FALSE;
}

void CXVideo::UpdateControlBarLoopState(){
	if (!m_hBtnLoop) return;
	BOOL on = m_loop.load(std::memory_order_acquire) ? TRUE : FALSE;
	// button_type_check: check 状态是按钮原生表达, XCGUI 重绘时同步带上 选中 背景.
	// 这一调会触发 XE_BUTTON_CHECK -> OnBtnLoopCheck, 用 m_programmaticBtnCheck 拦住.
	m_programmaticBtnCheck = TRUE;
	XBtn_SetCheck(m_hBtnLoop, on);
	m_programmaticBtnCheck = FALSE;
}

// (OnLButtonUpCtrlBar 已删 - 控件栏默认鼠标穿透, 空白处点击直接透到视频区,
//  没有需要拦截的事件; 按钮们的 XE_BUTTON_CHECK / XE_BNCLICK 在子级被消费, 不冲突.)

// Play 按钮 XE_BUTTON_CHECK: bCheck=TRUE -> 用户要求播放; bCheck=FALSE -> 用户要求暂停.
// 由 button_type_check 自动 toggle, 不用手工 load m_state 反推动作.
int CXVideo::OnBtnPlayCheck(HELE /*hEle*/, BOOL bCheck, BOOL* /*pbHandled*/){
	// 程式化 XBtn_SetCheck (UpdateControlBarPlayState) 触发的这路回调 直接忽略,
	// 避免 UI 同步 -> Play/Pause -> UI 同步 死循环.
	if (m_programmaticBtnCheck) return 0;
	int s = m_state.load(std::memory_order_acquire);
	if (bCheck){
		// 要求播放. closed/error 无媒体 - 下面 fall through 走 UI 回滚.
		switch (s){
		case xvideo_state_paused:
		case xvideo_state_stopped:
		case xvideo_state_ended:
		case xvideo_state_opening:
			Play();           // 内部会 UpdateControlBarPlayState
			return 0;
		case xvideo_state_playing:
			// 已经在播 - check 与实际 一致, 什么都不必做.
			return 0;
		default:
			break;            // closed / error: 走下面 回滚 check 状态
		}
	} else {
		// 要求暂停. 仅 playing 状态有意义.
		if (s == xvideo_state_playing){
			Pause();              // 内部会 UpdateControlBarPlayState
			return 0;
		}
		// 其他状态 (已经不在播): 走下面同步一下 check 以防不一致.
	}
	// 到这里表示 当前 state 不允许 该动作 (如 closed/error 下点击), 把 button 视觉 回滚 到 state.
	UpdateControlBarPlayState();
	return 0;
}

// Loop 按钮 XE_BUTTON_CHECK: 直接把 bCheck 同步到 SetLoop, 不需 toggle 逻辑.
int CXVideo::OnBtnLoopCheck(HELE /*hEle*/, BOOL bCheck, BOOL* /*pbHandled*/){
	if (m_programmaticBtnCheck) return 0;
	SetLoop(bCheck);
	// SetLoop 内部已调 UpdateControlBarLoopState, 这里无需重复.
	return 0;
}

int CXVideo::OnSliderProgressChange(HELE /*hEle*/, int pos, BOOL* /*pbHandled*/){
	// 程式化 SetPos 引发的回调直接忽略.
	if (m_programmaticSliderUpdate) return 0;
	double dur = GetDuration();
	if (dur <= 0.0) return 0;
	double targetSec = ((double)pos / (double)kProgressRange) * dur;
	if (targetSec < 0.0) targetSec = 0.0;
	if (targetSec > dur)  targetSec = dur;
	m_lastProgressPos = pos;            // UI 当前实际位置, 防 UpdateControlBarPosition 反弹
	DWORD now = ::GetTickCount();
	m_lastScrubTick = now;              // 进入 slider 锁定窗口 (kSliderLockMs)
	// 刷 user activity: 拖 slider 时 Windows mouse-capture 把 mouse-move 锁在 slider
	// 内部, 一旦光标拖出 bar 边界, HookMouseActivity 注册的 XE_MOUSEMOVE 就收不到了,
	// 但 XE_SLIDERBAR_CHANGE 仍会持续触发. 不在这里刷 activity tick 的话, m_autoHide
	// timeout 一到 bar 就会消失 (用户还在拖!). slider 值变化本身就是用户活动.
	NotifyUserActivity();
	// 时间标签立即跟手 (即使 Seek 被节流暂存, 数字也实时显示用户拖到的位置).
	if (m_hLblTime){
		std::wstring nowStr = FormatDuration(targetSec);
		std::wstring durStr = FormatDuration(dur);
		std::wstring txt = nowStr + L" / " + durStr;
		if (txt != m_lastTimeStr){
			m_lastTimeStr = txt;
			XShapeText_SetText(m_hLblTime, txt.c_str());
			if (m_hCtrlBar) XEle_AdjustLayout(m_hCtrlBar);
		}
	}
	// 节流: 距上次实际 Seek < kScrubMinIntervalMs 仅暂存 pending, 由 OnTimerImpl
	// 在 16ms tick 里检查节流后 commit. 防快速拖动每帧都打 demux -> demux 反复 av_seek
	// + 清队列, 永远稳不下来一帧 (拖动卡顿).
	if (now - m_lastScrubSeekTick >= kScrubMinIntervalMs){
		m_lastScrubSeekTick = now;
		m_pendingScrubSec = -1.0;
		Seek(targetSec);
	} else {
		m_pendingScrubSec = targetSec;  // 留给 OnTimerImpl
	}
	return 0;
}

//============================================================================
// 音量 *面板* (inline, 非独立窗口): CreateControlBar() 末尾 一起建, 初始隐藏.
// CreateVolumePanel 负责创建 m_hVolPanel (CXLayout 子元素, 不是 HWINDOW) +
// 内嵌 vertical CXSliderBar; ToggleVolumePanel / HideVolumePanel 只负责 重新定位 +
// show/hide 切换.
//
// 提前建 (而不是懒建) 的原因: 用户拿 GetVolumePanel / GetSliderVolume 句柄 改样式 /
// 注额外事件的场景 并不罕见, 懒建会造成 Create() 后拿到 NULL, 体验差.
//
// 为啥不用 popup 窗口: ① 独立窗口要做 DPI 物理像素 -> 逻辑像素换算 (XCGUI 客户区
// 已是 DPI 缩放后的逻辑像素, 跟独立顶层窗口的物理像素不一致); ② 独立窗口会夺焦点
// (用户拖完 slider, 焦点不在视频上, 键盘快捷键就接不到). 改成 m_hEle 的子元素后
// 这两个问题自然没了, 同时 GetVolumePanel / GetSliderVolume getter 的生命周期跟
// CXVideo 完全绑定, 更干净.
//
// 位置: 在 vol 按钮上方 8px, 与按钮水平居中.
// 关闭: 再点 vol 按钮 (toggle) / 点视频空白处 (OnLButtonUpVideo 优先关面板).
//============================================================================

int CXVideo::OnBtnVolumeClick(HELE /*hEle*/, BOOL* /*pbHandled*/){
	ToggleVolumePanel();
	return 0;
}

BOOL CXVideo::IsVolumePanelVisible() const {
	if (!m_hVolPanel) return FALSE;
	return XWidget_IsShow((HXCGUI)m_hVolPanel);
}

void CXVideo::HideVolumePanel(){
	if (m_hVolPanel) XWidget_Show((HXCGUI)m_hVolPanel, FALSE);
}

// 创建 音量面板 + 内嵌垂直 slider, 初始隐藏. CreateControlBar() 末尾调 一次.
// 拆出独立函数 是为了 不肥 CreateControlBar; 逻辑上是控件栏的子体系.
void CXVideo::CreateVolumePanel(){
	if (!m_hEle || m_hVolPanel) return;
	// 面板挂到 m_hEle (视频元素) 下, 不参与父 layout (size=disable).
	// 初始 rect 占位, ToggleVolumePanel 里显示前会重新定位.
	m_hVolPanel = XLayout_Create(0, 0, kVolPopupW, kVolPopupH, (HXCGUI)m_hEle);
	if (!m_hVolPanel) return;
	// 不参与 m_hEle 的纵向 layout (否则会跟控件栏一起被排进 vertical stack).
	XWidget_LayoutItem_SetWidth ((HXCGUI)m_hVolPanel, layout_size_disable, 0);
	XWidget_LayoutItem_SetHeight((HXCGUI)m_hVolPanel, layout_size_disable, 0);
	// 面板自己是 vertical layout, 内边距 8, 子节点 (slider) 横向居中.
	XLayoutBox_EnableHorizon(m_hVolPanel, FALSE);
	XLayoutBox_SetAlignH(m_hVolPanel, layout_align_center);
	XEle_SetPadding(m_hVolPanel, 8, 8, 8, 8);
	// 用户 spec: 面板背景完全透明 + 鼠标穿透. 最终 "只看到滑动条".
	// 鼠标穿透 后: slider 区外的点击 透到 m_hEle (视频区),
	// OnLButtonUpVideo 看到面板可见 会优先 HidePanel - 点另外区关面板, 符合直觉.
	XEle_EnableBkTransparent(m_hVolPanel, TRUE);
	XEle_EnableDrawBorder   (m_hVolPanel, FALSE);
	XEle_EnableDrawFocus    (m_hVolPanel, FALSE);
	XEle_EnableMouseThrough (m_hVolPanel, TRUE);

	// 嵌套 vertical slider. SliderBar 0=底, max=顶 (XCGUI 默认从下到上, 跟音量直觉一致).
	m_hSliderVolume = XSliderBar_Create(0, 0, 20, kVolPopupH - 16, (HXCGUI)m_hVolPanel);
	if (m_hSliderVolume){
		XSliderBar_EnableHorizon(m_hSliderVolume, FALSE);
		XSliderBar_SetRange(m_hSliderVolume, kVolumeRange);
		// 滑块 同水平 slider 一致 固定正方形.
		XSliderBar_SetButtonWidth (m_hSliderVolume, kSliderThumbDia);
		XSliderBar_SetButtonHeight(m_hSliderVolume, kSliderThumbDia);
		XEle_EnableBkTransparent(m_hSliderVolume, TRUE);
		// 跟进度 slider 一样, 去边框 + 去焦点虚线.
		XEle_EnableDrawBorder(m_hSliderVolume, FALSE);
		XEle_EnableDrawFocus (m_hSliderVolume, FALSE);
		// slider 在面板里: 宽度固定 20, 高度 fill 余下空间.
		XWidget_LayoutItem_SetWidth ((HXCGUI)m_hSliderVolume, layout_size_fixed, 20);
		XWidget_LayoutItem_SetHeight((HXCGUI)m_hSliderVolume, layout_size_fill,  0);
		int volPos = (int)(m_volume * kVolumeRange + 0.5);
		XSliderBar_SetPos(m_hSliderVolume, volPos);
		XEle_RegEventCPP1(m_hSliderVolume, XE_SLIDERBAR_CHANGE,
		                  &CXVideo::OnSliderVolumeChange);
		// XE_PAINT: 轨道 + 滑块 两套 跟进度 slider 同样逻辑 (函数内部 随 client rect 横/纵自适).
		XEle_RegEventCPP1(m_hSliderVolume, XE_PAINT, &CXVideo::OnPaintSlider);
		if (HELE hThumb = XSliderBar_GetButton(m_hSliderVolume)){
			XEle_EnableBkTransparent(hThumb, TRUE);
			XEle_EnableDrawBorder   (hThumb, FALSE);
			XEle_EnableDrawFocus    (hThumb, FALSE);
			XEle_RegEventCPP1(hThumb, XE_PAINT, &CXVideo::OnPaintSliderThumb);
		}
	}
	// 自动隐藏: 面板 鼠标穿透 拿不到 mousemove, 不用挂; 但内部 slider 仍需要 用户拖动时算 "活动".
	HookMouseActivity(m_hSliderVolume);
	// 面板初始隐藏, 等 ToggleVolumePanel 控着 show/hide.
	XWidget_Show((HXCGUI)m_hVolPanel, FALSE);
}

void CXVideo::ToggleVolumePanel(){
	// 面板在 CreateVolumePanel 里已提前建好; 这里只需 toggle + 重新定位.
	// EnableControlBar(FALSE) 场景下 m_hBtnVolume / m_hVolPanel 均为 NULL, 直接返.
	if (!m_hEle || !m_hBtnVolume || !m_hVolPanel) return;

	// ---- 1. 切换可见 ----
	BOOL nowShow = XWidget_IsShow((HXCGUI)m_hVolPanel);
	if (nowShow){
		XWidget_Show((HXCGUI)m_hVolPanel, FALSE);
		return;
	}

	// ---- 2. 显示前重新定位 (按钮位置可能因 reflow 变了) ----
	// 都是 XCGUI 元素坐标 (已 DPI-aware 的逻辑像素), 无需手动 DPI 换算.
	// 先强制 reflow 一次: bar 可能是刚从隐藏被 NotifyUserActivity 拉回来的,
	// 此时 XEle_GetRect 拿到的可能还是 stale 的旧坐标.
	ReflowControlBar();
	RECT rcBtn = {0};  XEle_GetRect(m_hBtnVolume, &rcBtn);   // 相对 m_hCtrlBar
	RECT rcBar = {0};  XEle_GetRect(m_hCtrlBar,   &rcBar);   // 相对 m_hEle
	int btnX_inVideo = rcBar.left + rcBtn.left;
	int btnY_inVideo = rcBar.top  + rcBtn.top;
	int btnW         = rcBtn.right - rcBtn.left;
	// 面板左上角: 与按钮水平居中, 底边贴按钮顶边 - 8px gap.
	int panelX = btnX_inVideo + (btnW - kVolPopupW) / 2;
	int panelY = btnY_inVideo - kVolPopupH - 8;
	// clamp 到 m_hEle 客户区内.
	int eleW = XEle_GetWidth (m_hEle);
	if (panelX < 0)                          panelX = 0;
	if (panelX + kVolPopupW > eleW)          panelX = eleW - kVolPopupW;
	if (panelY < 0)                          panelY = 0;
	RECT rcPanel = { panelX, panelY, panelX + kVolPopupW, panelY + kVolPopupH };
	XEle_SetRect(m_hVolPanel, &rcPanel, FALSE);

	// 把面板置顶 (在 m_hEle 的子列表里). XCGUI 后建的子默认就在前面建的之上,
	// 但 SetRect 后做一次 Redraw 保险.
	XWidget_Show((HXCGUI)m_hVolPanel, TRUE);
	XEle_Redraw(m_hVolPanel, FALSE);
}

int CXVideo::OnSliderVolumeChange(HELE /*hEle*/, int pos, BOOL* /*pbHandled*/){
	// 用户在音量 popup 里拖. pos 0..kVolumeRange -> 0..1.
	float v = (float)pos / (float)kVolumeRange;
	if (v < 0.0f) v = 0.0f;
	if (v > 1.0f) v = 1.0f;
	SetVolume(v);
	// 重绘 音量按钮: v 跨 0 边界时 voice.svg <-> voice_mute.svg 需切换.
	// SetVolume 内部已在跨界时 redraw 了, 这里是 用户 spec 要求 补上的 (作 事件处理末尾 多一道保险).
	if (m_hBtnVolume) XEle_Redraw(m_hBtnVolume, FALSE);
	return 0;
}

//============================================================================
// SVG 图标 资源管理
// 6 个图标 (play / suspend / loop / loop_close / voice / voice_mute) 全部硬编
// 码在 module_xcgui_video_svgs.inc, 通过 XSvg_LoadStringUtf8 一次性建好.
// XSvg 在 XCGUI 内部是引用计数的; 我们持有的句柄要在 OnDestroyImpl 里 XSvg_Destroy.
// 调用方不应在 XCGUI 卸载后调 XSvg_Destroy - 那时句柄表已销毁, 会弹无效句柄错框.
//============================================================================

void CXVideo::EnsureSvgsLoaded(){
	// 幂等: 已加载就跳过. 各句柄独立判断, 避免某次 LoadStringUtf8 失败后续永远跳过.
	if (!m_hSvgPlay)      m_hSvgPlay      = XSvg_LoadStringUtf8(kSvgPlay);
	if (!m_hSvgSuspend)   m_hSvgSuspend   = XSvg_LoadStringUtf8(kSvgSuspend);
	if (!m_hSvgLoop)      m_hSvgLoop      = XSvg_LoadStringUtf8(kSvgLoop);
	if (!m_hSvgLoopClose) m_hSvgLoopClose = XSvg_LoadStringUtf8(kSvgLoopClose);
	if (!m_hSvgVoice)     m_hSvgVoice     = XSvg_LoadStringUtf8(kSvgVoice);
	if (!m_hSvgVoiceMute) m_hSvgVoiceMute = XSvg_LoadStringUtf8(kSvgVoiceMute);
}

void CXVideo::DestroySvgs(){
	// 顺序释放. XSvg_Destroy 容许 NULL? 保守起见自己 if 守.
	if (m_hSvgPlay)     { XSvg_Destroy(m_hSvgPlay);      m_hSvgPlay      = NULL; }
	if (m_hSvgSuspend)  { XSvg_Destroy(m_hSvgSuspend);   m_hSvgSuspend   = NULL; }
	if (m_hSvgLoop)     { XSvg_Destroy(m_hSvgLoop);      m_hSvgLoop      = NULL; }
	if (m_hSvgLoopClose){ XSvg_Destroy(m_hSvgLoopClose); m_hSvgLoopClose = NULL; }
	if (m_hSvgVoice)    { XSvg_Destroy(m_hSvgVoice);     m_hSvgVoice     = NULL; }
	if (m_hSvgVoiceMute){ XSvg_Destroy(m_hSvgVoiceMute); m_hSvgVoiceMute = NULL; }
}

//============================================================================
// 控件栏 自绘 (XE_PAINT)
//
// 三个按钮 + 两个 slider (轨道 + 滑块) 各自接管绘制. pbHandled=TRUE 跳过 XCGUI
// 默认背景/边框/焦点框 + 文字, 完全由我们画.
//
// 内部用 lambda DrawSvgCentered 复用 "中心画 SVG" 逻辑, 跟用户 spec 公式一致:
//   x = rc.right/2 - w/2, y = rc.bottom/2 - h/2
// (XEle_GetClientRect 返 rc 起点 (0,0), 所以这等价于 (rc.right-rc.left-w)/2 + rc.left).
//============================================================================

namespace {
// 中心绘制 helper: 加载尺寸 -> 算居中坐标 -> XDraw_DrawSvg.
inline void DrawSvgCentered(HDRAW hDraw, HSVG hSvg, const RECT& rc){
	if (!hDraw || !hSvg) return;
	int sw = 0, sh = 0;
	XSvg_GetSize(hSvg, &sw, &sh);
	// 容错: 若 SVG 解析失败 GetSize 给 0, 直接放弃画.
	if (sw <= 0 || sh <= 0) return;
	// 用户 spec 公式. 假设 rc.left/top == 0 (XEle_GetClientRect 通常如此).
	int x = rc.right  / 2 - sw / 2;
	int y = rc.bottom / 2 - sh / 2;
	XDraw_DrawSvg(hDraw, hSvg, x, y);
}
} // namespace

int CXVideo::OnPaintBtnPlay(HELE hEle, HDRAW hDraw, BOOL* pbHandled){
	// pbHandled=TRUE: 跳过 XCGUI 默认 (背景 / 边框 / 文本); 完全自绘.
	if (pbHandled) *pbHandled = TRUE;
	EnsureSvgsLoaded();
	int s = m_state.load(std::memory_order_acquire);
	// 在播 -> 显示 暂停 图标 (按下就暂停); 否则显示 播放 图标.
	HSVG hSvg = (s == xvideo_state_playing) ? m_hSvgSuspend : m_hSvgPlay;
	RECT rc{};  XEle_GetClientRect(hEle, &rc);
	DrawSvgCentered(hDraw, hSvg, rc);
	return 0;
}

int CXVideo::OnPaintBtnLoop(HELE hEle, HDRAW hDraw, BOOL* pbHandled){
	if (pbHandled) *pbHandled = TRUE;
	EnsureSvgsLoaded();
	BOOL on = m_loop.load(std::memory_order_acquire) ? TRUE : FALSE;
	HSVG hSvg = on ? m_hSvgLoop : m_hSvgLoopClose;
	RECT rc{};  XEle_GetClientRect(hEle, &rc);
	DrawSvgCentered(hDraw, hSvg, rc);
	return 0;
}

int CXVideo::OnPaintBtnVolume(HELE hEle, HDRAW hDraw, BOOL* pbHandled){
	if (pbHandled) *pbHandled = TRUE;
	EnsureSvgsLoaded();
	// 静音判定: 显式 mute 或 音量降到 0 都视为静音.
	BOOL muted = (m_muted || m_volume <= 0.0f) ? TRUE : FALSE;
	HSVG hSvg = muted ? m_hSvgVoiceMute : m_hSvgVoice;
	RECT rc{};  XEle_GetClientRect(hEle, &rc);
	DrawSvgCentered(hDraw, hSvg, rc);
	return 0;
}

int CXVideo::OnPaintSlider(HELE hEle, HDRAW hDraw, BOOL* pbHandled){
	// 接管: 不要 XCGUI 默认 track 图. 我们画 35% 白底 + #0099FF 填充.
	if (pbHandled) *pbHandled = TRUE;
	RECT rc{};  XEle_GetClientRect(hEle, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0) return 0;

	int pos   = XSliderBar_GetPos  (hEle);
	int range = XSliderBar_GetRange(hEle);
	if (range <= 0) return 0;
	float frac = (float)pos / (float)range;
	if (frac < 0.0f) frac = 0.0f;
	if (frac > 1.0f) frac = 1.0f;

	// 方向: 宽 >= 高 走水平, 否则垂直. 跟 XSliderBar_EnableHorizon 实际值一致.
	bool horizontal = (w >= h);
	const int  thk = kSliderTrackThk;
	const float radius = thk * 0.5f;  // pill 圆角 = 厚度一半 -> 完美胶囊形

	if (horizontal){
		// 轨道在 垂直中央, 高 thk, 跨 整宽. 圆角 = thk/2 -> 两端半圆.
		int trackY = rc.top + (h - thk) / 2;
		RECTF rcTrackF = { (float)rc.left, (float)trackY,
		                   (float)rc.right, (float)(trackY + thk) };
		XDraw_SetBrushColor(hDraw, kSliderTrackBg);
		XDraw_FillRoundRectF(hDraw, &rcTrackF, radius, radius);
		// 填充段: 左 -> 当前 pos. frac=0 时 fillW=0, 直接跳过避免 0 宽 round rect.
		int fillW = (int)((float)w * frac + 0.5f);
		if (fillW > 0){
			RECTF rcFillF = { (float)rc.left, (float)trackY,
			                  (float)(rc.left + fillW), (float)(trackY + thk) };
			XDraw_SetBrushColor(hDraw, kSliderTrackFill);
			XDraw_FillRoundRectF(hDraw, &rcFillF, radius, radius);
		}
	} else {
		// 垂直: 轨道在 水平中央, 宽 thk, 跨 整高. fill 从 *底* 向上 (XSliderBar 0=底).
		int trackX = rc.left + (w - thk) / 2;
		RECTF rcTrackF = { (float)trackX,         (float)rc.top,
		                   (float)(trackX + thk), (float)rc.bottom };
		XDraw_SetBrushColor(hDraw, kSliderTrackBg);
		XDraw_FillRoundRectF(hDraw, &rcTrackF, radius, radius);
		int fillH = (int)((float)h * frac + 0.5f);
		if (fillH > 0){
			RECTF rcFillF = { (float)trackX,         (float)(rc.bottom - fillH),
			                  (float)(trackX + thk), (float)rc.bottom };
			XDraw_SetBrushColor(hDraw, kSliderTrackFill);
			XDraw_FillRoundRectF(hDraw, &rcFillF, radius, radius);
		}
	}
	return 0;
}

int CXVideo::OnPaintSliderThumb(HELE hEle, HDRAW hDraw, BOOL* pbHandled){
	// 滑块 #F7F7F7 实心圆. 用户 spec 提到 XDraw_DrawArcF, 但那是只画弧线轮廓 -
	// 截图视觉是实心圆, 这里用 XDraw_FillEllipse 才匹配. 先 SetBrushColor 再 fill.
	if (pbHandled) *pbHandled = TRUE;
	RECT rc{};  XEle_GetClientRect(hEle, &rc);
	if (rc.right <= rc.left || rc.bottom <= rc.top) return 0;
	XDraw_SetBrushColor(hDraw, kSliderThumb);
	XDraw_FillEllipse(hDraw, &rc);
	return 0;
}

//============================================================================
// 静态工具: 视频封面 / 视频信息  (从 module_xcgui_image 迁移过来)
//
// 设计:
//   - 不复用 CXVideo 实例字段 (m_pVCtx 等): 静态接口需在无实例时也能工作, 而且
//     调用方可能并发多次 (列表里给一堆视频抓缩略图).
//   - 也不依赖 CXVideo 的 worker 线程模型 / WASAPI / 硬件解码上下文: 这里只做
//     一次性 *软解 1 帧 + 编码 PNG*, 路径远比播放器轻.
//   - GDI+ 编码 PNG. WIC 路径更可控但要 COM 模板, 编译时间贵; GDI+ 已在系统里,
//     用 GdiplusStartup 静态 once 启动 (token 与 XCGUI 自身 startup 互不干扰).
//   - 抓帧策略: avformat_seek_file 到 coverTimeSec 对应的 pts ->
//     循环 av_read_frame + avcodec_send/receive_frame, 拿到 *第一帧* (≥ seek 点).
//     若 seek 失败或位置超时长, 自动回退到 0 + 从头解.
//   - 颜色空间: sws_scale 直接转 BGRA (PixelFormat32bppARGB GDI+ 兼容).
//   - hash: 路径走 FNV-1a 64bit, 拼成 16 位十六进制做文件名, 同一视频反复
//          抓取直接命中缓存 (除非用户改了 coverTimeSec).
//============================================================================

#include <gdiplus.h>
#include <shlwapi.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

namespace {

// GDI+ 静态启动 (与 XCGUI 自身的 startup 互不干扰: token 各自独立).
struct _XVideoCover_GdiplusOnce{
	ULONG_PTR token = 0;
	_XVideoCover_GdiplusOnce(){
		Gdiplus::GdiplusStartupInput in;
		Gdiplus::GdiplusStartup(&token, &in, NULL);
	}
	~_XVideoCover_GdiplusOnce(){
		if (token) Gdiplus::GdiplusShutdown(token);
	}
};
static void _XVideoCover_EnsureGdiplus(){
	static _XVideoCover_GdiplusOnce s_once;
	(void)s_once;
}

// 取 PNG 编码器 CLSID. 失败返 FALSE.
static BOOL _XVideoCover_GetPngEncoderClsid(CLSID* pOut){
	UINT num = 0, size = 0;
	if (Gdiplus::GetImageEncodersSize(&num, &size) != Gdiplus::Ok || size == 0) return FALSE;
	std::vector<BYTE> buf(size);
	Gdiplus::ImageCodecInfo* info = (Gdiplus::ImageCodecInfo*)buf.data();
	if (Gdiplus::GetImageEncoders(num, size, info) != Gdiplus::Ok) return FALSE;
	for (UINT i = 0; i < num; ++i){
		if (wcscmp(info[i].MimeType, L"image/png") == 0){
			*pOut = info[i].Clsid;
			return TRUE;
		}
	}
	return FALSE;
}

// FNV-1a 64bit, 对 wchar 缓冲整段做; 大小写不区分 (路径 Windows 不敏感).
static uint64_t _XVideoCover_HashPathW(const wchar_t* p){
	uint64_t h = 0xcbf29ce484222325ULL;
	for (; *p; ++p){
		wchar_t c = *p;
		if (c >= L'A' && c <= L'Z') c = (wchar_t)(c + 32);
		h ^= (uint64_t)(uint16_t)c;
		h *= 0x100000001b3ULL;
	}
	return h;
}

// 解码视频流, 抓 *seek 后的第 1 帧*, 输出为 (BGRA, W, H).
// 成功填 outBgra (size = W*H*4) 返 TRUE. 失败返 FALSE.
static BOOL _XVideoCover_GrabFrameBgra(AVFormatContext* pFmt, int videoStreamIdx,
                                       double coverTimeSec,
                                       std::vector<uint8_t>* outBgra, int* outW, int* outH){
	if (!pFmt || videoStreamIdx < 0) return FALSE;
	AVStream* st = pFmt->streams[videoStreamIdx];
	const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
	if (!dec) return FALSE;
	AVCodecContext* pCtx = avcodec_alloc_context3(dec);
	if (!pCtx) return FALSE;
	BOOL ok = FALSE;
	AVFrame* frame = NULL;
	AVPacket* pkt = NULL;
	SwsContext* sws = NULL;
	do{
		if (avcodec_parameters_to_context(pCtx, st->codecpar) < 0) break;
		pCtx->thread_count = 1;
		if (avcodec_open2(pCtx, dec, NULL) < 0) break;

		// Seek: 若 coverTimeSec > 0 且容器可 seek.
		if (coverTimeSec > 0.0 && pFmt->duration > 0){
			double durSec = (double)pFmt->duration / (double)AV_TIME_BASE;
			double t = coverTimeSec;
			if (t > durSec - 0.05) t = (durSec > 0.2) ? (durSec * 0.1) : 0.0;
			int64_t ts = (int64_t)(t * AV_TIME_BASE);
			// 用 AVSEEK_FLAG_BACKWARD 拿前一个关键帧, 解码到目标时间稳.
			av_seek_frame(pFmt, -1, ts, AVSEEK_FLAG_BACKWARD);
			avcodec_flush_buffers(pCtx);
		}

		frame = av_frame_alloc();
		pkt = av_packet_alloc();
		if (!frame || !pkt) break;

		// 读包 -> 解码 -> 拿到第一帧立刻退出.
		int gotFrame = 0;
		while (!gotFrame){
			int rr = av_read_frame(pFmt, pkt);
			if (rr < 0){
				// EOF: flush 解码器
				avcodec_send_packet(pCtx, NULL);
				while (avcodec_receive_frame(pCtx, frame) == 0){ gotFrame = 1; break; }
				break;
			}
			if (pkt->stream_index != videoStreamIdx){
				av_packet_unref(pkt);
				continue;
			}
			if (avcodec_send_packet(pCtx, pkt) >= 0){
				while (avcodec_receive_frame(pCtx, frame) == 0){
					gotFrame = 1;
					break;
				}
			}
			av_packet_unref(pkt);
		}
		if (!gotFrame || frame->width <= 0 || frame->height <= 0) break;

		// 缩放 + 颜色空间转换到 BGRA. 不缩, 保留原始像素.
		int srcW = frame->width, srcH = frame->height;
		sws = sws_getContext(srcW, srcH, (AVPixelFormat)frame->format,
		                     srcW, srcH, AV_PIX_FMT_BGRA,
		                     SWS_BILINEAR, NULL, NULL, NULL);
		if (!sws) break;

		std::vector<uint8_t> bgra((size_t)srcW * srcH * 4);
		uint8_t* dstData[4] = { bgra.data(), NULL, NULL, NULL };
		int dstStride[4] = { srcW * 4, 0, 0, 0 };
		sws_scale(sws, frame->data, frame->linesize, 0, srcH, dstData, dstStride);

		*outBgra = std::move(bgra);
		*outW = srcW;
		*outH = srcH;
		ok = TRUE;
	} while (0);

	if (sws)   sws_freeContext(sws);
	if (frame) av_frame_free(&frame);
	if (pkt)   av_packet_free(&pkt);
	if (pCtx)  avcodec_free_context(&pCtx);
	return ok;
}

// 把 BGRA 缓冲存为 PNG (path = UTF-16 全路径). 成功返 TRUE.
static BOOL _XVideoCover_SaveBgraToPng(const uint8_t* bgra, int w, int h, const wchar_t* path){
	_XVideoCover_EnsureGdiplus();
	CLSID clsid;
	if (!_XVideoCover_GetPngEncoderClsid(&clsid)) return FALSE;
	// PixelFormat32bppARGB 在 GDI+ 里就是 BGRA byte order (低字节 B, 高字节 A),
	// 跟 sws AV_PIX_FMT_BGRA 输出一致, 不需要再翻通道.
	Gdiplus::Bitmap bmp(w, h, w * 4, PixelFormat32bppARGB, (BYTE*)bgra);
	if (bmp.GetLastStatus() != Gdiplus::Ok) return FALSE;
	return bmp.Save(path, &clsid, NULL) == Gdiplus::Ok ? TRUE : FALSE;
}

} // anonymous namespace

BOOL CXVideo::GetVideoCacheDir(wchar_t* pOutDir, int outBufLen){
	if (!pOutDir || outBufLen < 32) return FALSE;
	wchar_t tmp[MAX_PATH] = { 0 };
	DWORD n = ::GetTempPathW(MAX_PATH, tmp);
	if (n == 0 || n >= MAX_PATH) return FALSE;
	// tmp 末尾保证带反斜杠 (GetTempPath 规范). 拼上子目录.
	std::wstring dir(tmp);
	dir += L"xcgui_video_cover\\";
	// CreateDirectoryW 已存在返回 ERROR_ALREADY_EXISTS, 视为成功.
	if (!::CreateDirectoryW(dir.c_str(), NULL)){
		DWORD e = ::GetLastError();
		if (e != ERROR_ALREADY_EXISTS) return FALSE;
	}
	if ((int)dir.size() + 1 > outBufLen) return FALSE;
	wcscpy_s(pOutDir, (size_t)outBufLen, dir.c_str());
	return TRUE;
}

BOOL CXVideo::GetVideoCover(const wchar_t* pVideoPath, const wchar_t* pSaveDir,
                            double coverTimeSec,
                            wchar_t* pOutCoverPath, int outBufLen){
	if (!pVideoPath || !*pVideoPath) return FALSE;

	// 1) 决定保存目录.
	wchar_t dirBuf[MAX_PATH] = { 0 };
	if (pSaveDir && *pSaveDir){
		wcscpy_s(dirBuf, MAX_PATH, pSaveDir);
		size_t L = wcslen(dirBuf);
		if (L > 0 && dirBuf[L - 1] != L'\\' && dirBuf[L - 1] != L'/'){
			if (L + 1 < MAX_PATH){ dirBuf[L] = L'\\'; dirBuf[L + 1] = 0; }
		}
		// 尽力创建 (不递归; 多级目录请调用方自行保证).
		::CreateDirectoryW(dirBuf, NULL);
	} else {
		if (!GetVideoCacheDir(dirBuf, MAX_PATH)) return FALSE;
	}

	// 2) 拼输出 PNG 全路径.
	uint64_t h = _XVideoCover_HashPathW(pVideoPath);
	wchar_t fullPath[MAX_PATH] = { 0 };
	swprintf_s(fullPath, MAX_PATH, L"%s%016llx.png", dirBuf, (unsigned long long)h);

	// 3) 命中缓存 (同路径 + 已存在 PNG): 直接返回, 不重复抓.
	if (::PathFileExistsW(fullPath)){
		if (pOutCoverPath && outBufLen > 0){
			wcscpy_s(pOutCoverPath, (size_t)outBufLen, fullPath);
		}
		return TRUE;
	}

	// 4) FFmpeg 抓帧.
	AVFormatContext* pFmt = NULL;
	std::string utf8 = WideToUtf8(pVideoPath);
	if (avformat_open_input(&pFmt, utf8.c_str(), NULL, NULL) < 0) return FALSE;
	BOOL ok = FALSE;
	do{
		if (avformat_find_stream_info(pFmt, NULL) < 0) break;
		int vsi = -1;
		for (unsigned i = 0; i < pFmt->nb_streams; ++i){
			if (pFmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
				vsi = (int)i; break;
			}
		}
		if (vsi < 0) break;
		std::vector<uint8_t> bgra;
		int w = 0, hgt = 0;
		if (!_XVideoCover_GrabFrameBgra(pFmt, vsi, coverTimeSec, &bgra, &w, &hgt)) break;
		if (!_XVideoCover_SaveBgraToPng(bgra.data(), w, hgt, fullPath)) break;
		ok = TRUE;
	} while (0);
	avformat_close_input(&pFmt);

	if (ok && pOutCoverPath && outBufLen > 0){
		wcscpy_s(pOutCoverPath, (size_t)outBufLen, fullPath);
	}
	return ok;
}

BOOL CXVideo::GetVideoInfo(const wchar_t* pVideoPath, XVideoInfo* pOutInfo, double coverTimeSec){
	if (!pVideoPath || !*pVideoPath || !pOutInfo) return FALSE;
	memset(pOutInfo, 0, sizeof(*pOutInfo));
	wcscpy_s(pOutInfo->videoPath, pVideoPath);

	AVFormatContext* pFmt = NULL;
	std::string utf8 = WideToUtf8(pVideoPath);
	if (avformat_open_input(&pFmt, utf8.c_str(), NULL, NULL) < 0) return FALSE;
	BOOL ok = FALSE;
	do{
		if (avformat_find_stream_info(pFmt, NULL) < 0) break;

		// 找首个视频流 + 首个音频流.
		int vsi = -1, asi = -1;
		for (unsigned i = 0; i < pFmt->nb_streams; ++i){
			AVCodecParameters* cp = pFmt->streams[i]->codecpar;
			if (vsi < 0 && cp->codec_type == AVMEDIA_TYPE_VIDEO) vsi = (int)i;
			else if (asi < 0 && cp->codec_type == AVMEDIA_TYPE_AUDIO) asi = (int)i;
		}
		if (vsi < 0) break;
		AVStream* vst = pFmt->streams[vsi];
		AVCodecParameters* vcp = vst->codecpar;

		// 容器格式名 (取 long_name 前的短名, 例 "mov,mp4,m4a,3gp,3g2,mj2" -> 取首段)
		const char* fmtName = (pFmt->iformat && pFmt->iformat->name) ? pFmt->iformat->name : "";
		{
			const char* comma = strchr(fmtName, ',');
			size_t n = comma ? (size_t)(comma - fmtName) : strlen(fmtName);
			if (n >= 64) n = 63;
			char tmp[64] = { 0 };
			memcpy(tmp, fmtName, n);
			::MultiByteToWideChar(CP_UTF8, 0, tmp, -1, pOutInfo->formatName, 64);
		}

		// 编码名
		const char* vcodec = avcodec_get_name(vcp->codec_id);
		if (vcodec) ::MultiByteToWideChar(CP_UTF8, 0, vcodec, -1, pOutInfo->videoCodec, 64);
		if (asi >= 0){
			const char* acodec = avcodec_get_name(pFmt->streams[asi]->codecpar->codec_id);
			if (acodec) ::MultiByteToWideChar(CP_UTF8, 0, acodec, -1, pOutInfo->audioCodec, 64);
			pOutInfo->hasAudio = 1;
		}

		// 尺寸
		pOutInfo->width  = vcp->width;
		pOutInfo->height = vcp->height;

		// 时长
		pOutInfo->durationSec = (pFmt->duration > 0) ? (double)pFmt->duration / (double)AV_TIME_BASE : 0.0;
		// 比特率
		pOutInfo->bitrate = pFmt->bit_rate;
		// 帧率
		AVRational fr = vst->avg_frame_rate;
		if (fr.num > 0 && fr.den > 0) pOutInfo->fps = (double)fr.num / (double)fr.den;
		// 帧数估算
		if (pOutInfo->fps > 0.0 && pOutInfo->durationSec > 0.0){
			pOutInfo->frameCount = (int64_t)(pOutInfo->fps * pOutInfo->durationSec);
		}
		// 文件大小 (本地文件) -- avio_size 也行, 这里直接 stat 防网络源阻塞.
		{
			WIN32_FILE_ATTRIBUTE_DATA fad;
			if (::GetFileAttributesExW(pVideoPath, GetFileExInfoStandard, &fad) &&
			    !(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
				LARGE_INTEGER li;
				li.HighPart = (LONG)fad.nFileSizeHigh;
				li.LowPart  = fad.nFileSizeLow;
				pOutInfo->fileSize = li.QuadPart;
			}
		}

		// 抓封面 (复用 GetVideoCover 的逻辑, 但不重复 open: 这里手动跑一次 grab + save).
		wchar_t dirBuf[MAX_PATH] = { 0 };
		if (GetVideoCacheDir(dirBuf, MAX_PATH)){
			uint64_t hsh = _XVideoCover_HashPathW(pVideoPath);
			wchar_t fullPath[MAX_PATH] = { 0 };
			swprintf_s(fullPath, MAX_PATH, L"%s%016llx.png", dirBuf, (unsigned long long)hsh);

			BOOL coverOk = FALSE;
			if (::PathFileExistsW(fullPath)){
				coverOk = TRUE;
			} else {
				std::vector<uint8_t> bgra;
				int gw = 0, gh = 0;
				if (_XVideoCover_GrabFrameBgra(pFmt, vsi, coverTimeSec, &bgra, &gw, &gh)){
					coverOk = _XVideoCover_SaveBgraToPng(bgra.data(), gw, gh, fullPath);
				}
			}
			if (coverOk){
				wcscpy_s(pOutInfo->coverPath, fullPath);
			}
		}
		ok = TRUE;
	} while (0);
	avformat_close_input(&pFmt);
	return ok;
}
