//============================================================================
// module_xcgui_media_image.cpp — CXImageEx 实现 (合并模块)

#include "module_xcgui_media.h"

#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <chrono>

// 本模块只用 avformat / avcodec / swscale 三家 API. 不读 frame->duration
// (用 pkt->duration 代替, 避开 4.x 跟 5.x+ 之间的字段变化), 不跳转音频代码
// 路径. 结果: FFmpeg 4.4 到 8.x 头文件均可编译, ABI 上只依赖这三家的
// 公开 API.

//============================================================================
// 构造 / 析构
//============================================================================
CXImageEx::CXImageEx(){
	_XMedia_FF_EnsureNetworkInit();
}

CXImageEx::~CXImageEx(){
	// 正常路径: OnDestroyImpl 已先一步释放全部重资源并 `delete this`, dtor 跑到
	// 这里时所有指针都已 NULL, 下面的清理都是 no-op.
	// 异常路径: `new CXImageEx()` 后未调 Create() 又被立即销毁 (理论上 dtor private
	// 已经堵死外部 delete; 唯一可能是构造抛异常时的栈回滚). 这种场景下 OnDestroyImpl
	// 不会触发, 这里做最后兜底.
	Unload();
	if (m_pSws){ sws_freeContext(m_pSws); m_pSws = NULL; }
	SafeRelease(m_d2dCache.pBmp);
	m_d2dCache.pLastRT = NULL;
	m_d2dCache.bmpW = m_d2dCache.bmpH = 0;
	m_d2dCache.uploadedBgraPtr = NULL;
	_XMedia_Render_ReleaseGdiDib(&m_gdiOffscreen);
	m_loadAbort.store(false, std::memory_order_release);
}

//============================================================================
// Create / Attach / Detach / 事件注册
//
// 关系图:
//   外部入口         内部主流程                     备注
//   ---------------------------------------------------
//   Create()    -->  DetachInternal? + AttachInternal(XEle_Create 得到的 HELE)
//   AttachToEle -->  DetachInternal? + AttachInternal(用户传入的 HELE)
//   operator=   -->  AttachToEle / Detach          语法糖 (NULL/非法 -> Detach)
//   Detach()    -->  DetachInternal                仅断开, 不销毁 HELE
//   XE_DESTROY  -->  OnDestroyImpl                 HELE 死亡 -> 清资源 + delete this
//
// DetachInternal vs OnDestroyImpl 差异:
//   - DetachInternal 走 UninstallEvents (反注册 + 关 timer), HELE 仍存活, *不* delete this
//   - OnDestroyImpl  不反注册 (XCGUI 已清自己的事件表), 末尾 `delete this`
//============================================================================
HELE CXImageEx::Create(int x, int y, int cx, int cy, HXCGUI hParent){
	// 已附加旧 HELE -> 先解除附加. 旧 HELE 本身由其父级负责销毁, 这里只断钩子.
	if (XC_IsHELE((HXCGUI)m_hEle)){
		DetachInternal();
	}
	// 直接 XEle_Create: 不需要 CXLayout 那套自动布局 (没有内置控件栏 / 子节点),
	// 普通 XEle 元素就够 - 也避开 CXLayout default mouse-through 等需要额外配置的细节.
	HELE h = XEle_Create(x, y, cx, cy, hParent);
	if (!h) return NULL;
	XUI_EnableCSS(h, FALSE);
	AttachInternal(h);
	return h;
}

BOOL CXImageEx::AttachToEle(HELE hEle){
	// 合法性校验: 必须是 XCGUI 已注册的元素句柄. NULL / 已销毁 / 非元素 句柄都走 FALSE.
	// 注意: XC_IsHELE 形参是 HXCGUI, 这里显式转一下.
	if (!XC_IsHELE((HXCGUI)hEle)){
		// 显式 detach 当前; 与 CXBlur::operator= 对齐 (传非法 = 释放).
		Detach();
		return FALSE;
	}
	// 相同元素 -> no-op, 避免误反注册又重注册触发 timer 抖动.
	if (m_hEle == hEle) return TRUE;
	// 切换前先断旧的.
	if (XC_IsHELE((HXCGUI)m_hEle)){
		DetachInternal();
	}
	AttachInternal(hEle);
	return TRUE;
}

void CXImageEx::Detach(){
	if (!XC_IsHELE((HXCGUI)m_hEle)) {
		// 没附加任何 HELE: 仍清一次内部资源, 防止 LoadFromFile 后又 Detach
		// (LoadFromFile 可填了 m_frames 但 HELE 中途被外部销毁过).
		Unload();
		if (m_pSws){ sws_freeContext(m_pSws); m_pSws = NULL; }
		SafeRelease(m_d2dCache.pBmp);
		m_d2dCache.pLastRT = NULL;
		m_d2dCache.bmpW = m_d2dCache.bmpH = 0;
		m_d2dCache.uploadedBgraPtr = NULL;
		_XMedia_Render_ReleaseGdiDib(&m_gdiOffscreen);
		m_hEle = NULL;
		return;
	}
	DetachInternal();
}

void CXImageEx::operator=(const HELE hEle){
	// IDE 风格糖: `*pImg = hEle`. 合法 -> Attach; 非法/NULL -> Detach.
	if (XC_IsHELE((HXCGUI)hEle)){
		AttachToEle(hEle);
	} else {
		Detach();
	}
}

void CXImageEx::AttachInternal(HELE hEle){
	// 进入前调用方已保证 hEle 合法且本对象未持其它 HELE.
	m_hEle = hEle;
	XUI_EnableCSS(hEle, FALSE);
	RefreshDpiScale();
	RebuildBkInfo();
	InstallEvents();
}

void CXImageEx::DetachInternal(){
	// 顺序与 OnDestroyImpl 几乎一致, 但 *多* UninstallEvents (HELE 还活着, 必须主动反注册);
	// *少* 末尾 delete this (包装对象继续存活, 待 Re-Attach 或用户决定生命周期).
	UninstallEvents();
	Unload();
	if (m_pSws){ sws_freeContext(m_pSws); m_pSws = NULL; }
	SafeRelease(m_d2dCache.pBmp);
	m_d2dCache.pLastRT = NULL;
	m_d2dCache.bmpW = m_d2dCache.bmpH = 0;
	m_d2dCache.uploadedBgraPtr = NULL;
	_XMedia_Render_ReleaseGdiDib(&m_gdiOffscreen);
	m_hEle = NULL;
}

void CXImageEx::InstallEvents(){
	// XE_PAINT: 完全接管绘制. 处理函数里 *pbHandled=TRUE 跳过 XCGUI 默认背景/边框,
	// 自己画: 背景色 + 当前帧 BGRA.
	XEle_RegEventCPP1(m_hEle, XE_PAINT,    &CXImageEx::OnPaintImpl);
	// XE_SIZE: 重缩当前帧到新目标尺寸 (dst 改, src 不变).
	XEle_RegEventCPP1(m_hEle, XE_SIZE,     &CXImageEx::OnSizeImpl);
	// XE_XC_TIMER: 16ms tick 驱动动画 + 派发 pending 事件.
	// 全程开启即使空闲也只 ~0.01% CPU, 比 lazy start/stop 简单可靠.
	XEle_RegEventCPP1(m_hEle, XE_XC_TIMER, &CXImageEx::OnTimerImpl);
	// XE_DESTROY: 释放 D2D / GDI / FFmpeg 资源 + delete this.
	XEle_RegEventCPP1(m_hEle, XE_DESTROY,  &CXImageEx::OnDestroyImpl);

	XEle_SetXCTimer(m_hEle, kTimerId_Tick, kTimerInterval_Ms);
	_XMedia_SizeMoveGuard_Attach((void*)m_hEle, &CXImageEx::OnSizeMoveExitThunk, this);
}

void CXImageEx::UninstallEvents(){
	if (!XC_IsHELE((HXCGUI)m_hEle)) return;
	_XMedia_SizeMoveGuard_Detach((void*)m_hEle, this);
	// 关 timer 必须先于 RemoveEvent: 否则窗口下一个 tick 仍可能进 OnTimerImpl.
	XEle_KillXCTimer(m_hEle, kTimerId_Tick);
	// CPP1 注册的回调用 XEle_RemoveEventCPP 反注册 (XCGUI 内部按函数名字符串
	// 做去重 / 卸载键, RegEventCPP 与 RegEventCPP1 共享同一张表, 移除用同一宏).
	// 反注册顺序与 InstallEvents 镜像.
	XEle_RemoveEventCPP(m_hEle, XE_PAINT,    &CXImageEx::OnPaintImpl);
	XEle_RemoveEventCPP(m_hEle, XE_SIZE,     &CXImageEx::OnSizeImpl);
	XEle_RemoveEventCPP(m_hEle, XE_XC_TIMER, &CXImageEx::OnTimerImpl);
	XEle_RemoveEventCPP(m_hEle, XE_DESTROY,  &CXImageEx::OnDestroyImpl);
}

int CXImageEx::OnDestroyImpl(HELE /*hEle*/, BOOL* /*pbHandled*/){
	// XE_DESTROY 在子对象销毁 *之前*. 顺序:
	//   (1) 关 timer (避免 join 期间 timer 还触发).
	//   (2) Unload 释放帧 / sws.
	//   (3) D2D / GDI 渲染资源也释放 (D2D Bitmap 跟 RT 绑, RT 析构后不能再 Release).
	//   (4) m_hEle = NULL: 防 `delete this` 触发 dtor 兜底路径里再走元素相关代码.
	//   (5) `delete this`: 与 XCGUI 设计对齐 - C++ 包装的生命周期绑定到 HELE.
	//        父窗口/父元素销毁 -> XE_DESTROY -> 这里释放所有资源 + 释放包装本身,
	//        外部既不需要也不允许手动 delete (dtor 已 protected).
	//        delete this 必须在最后一行: 之后任何对成员的访问都是 use-after-free.
	//   注: 这里 *不* 调 UninstallEvents - XE_DESTROY 触发时 XCGUI 已在拆事件表,
	//   主动 RemoveEvent 多此一举且可能与拆表过程竞争.
	XEle_KillXCTimer(m_hEle, kTimerId_Tick);
	_XMedia_SizeMoveGuard_Detach((void*)m_hEle, this);
	Unload();
	if (m_pSws){ sws_freeContext(m_pSws); m_pSws = NULL; }
	SafeRelease(m_d2dCache.pBmp);
	m_d2dCache.pLastRT = NULL;
	m_d2dCache.bmpW = m_d2dCache.bmpH = 0;
	m_d2dCache.uploadedBgraPtr = NULL;
	_XMedia_Render_ReleaseGdiDib(&m_gdiOffscreen);
	m_hEle = NULL;
	delete this;
	return 0;
}

//============================================================================
// 尺寸 / 定时器
//============================================================================
int CXImageEx::OnSizeImpl(HELE /*hEle*/, int /*nFlags*/, UINT /*nAdjustNo*/, BOOL* /*pbHandled*/){
	RefreshDpiScale();
	if (_XMedia_SizeMoveGuard_IsActive((void*)m_hEle)){
		m_gdiDibDirty = true;
		RedrawSelf();
		return 0;
	}
	// 元素尺寸变了 -> 显示目标矩形变 -> 必须重缩当前帧. 标记 dirty 让 OnPaint 触发重缩.
	// 注意不在这里直接调 RescaleCurrentFrameIfNeeded: resize 高频, 让 OnPaint 在真正需要画
	// 之前才缩, 避免 resize 一帧未到 paint 又被 resize 覆盖的浪费.
	m_gdiDibDirty = true;
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_scaledForFrame = -1;
		m_curDirty = true;
	}
	RedrawSelf();
	return 0;
}

void CXImageEx::OnSizeMoveExitThunk(void* user){
	reinterpret_cast<CXImageEx*>(user)->OnExitSizeMoveImpl();
}

void CXImageEx::OnExitSizeMoveImpl(){
	m_gdiDibDirty = true;
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_scaledForFrame = -1;
		m_curDirty = true;
	}
	RedrawSelf();
}

int CXImageEx::OnTimerImpl(HELE /*hEle*/, UINT nTimerId, BOOL* /*pbHandled*/){
	if (nTimerId != kTimerId_Tick) return 0;

	// 1) 派发 pending 事件 (Loaded / Error / Ended). 这条路径让 LoadInternal 等同步代码
	//    能"产事件不直接调用回调", 把回调延后到 UI 线程下一个 16ms tick - 用户在回调里
	//    再次调 LoadFromFile 也不会撞重入.
	DispatchPendingCallbacks();

	// 2) 动画推进. 只在 playing 且宿主可见时做 (最小化时不浪费 CPU swscale/redraw).
	if (m_state.load(std::memory_order_acquire) == ximage_state_playing
	    && _XMedia_IsHostVisible((void*)m_hEle)){
		AdvanceFrameIfDue();
	}
	return 0;
}

//============================================================================
// DPI / BkInfo / 重绘 / 目标矩形
//============================================================================
void CXImageEx::RefreshDpiScale(){
	if (!m_hEle) return;
	HWINDOW hWnd = (HWINDOW)XWidget_GetHWINDOW((HXCGUI)m_hEle);
	int dpi = hWnd ? XWnd_GetDPI(hWnd) : 96;
	if (dpi <= 0) dpi = 96;
	m_dpiScale = (float)dpi / 96.0f;
}

void CXImageEx::RebuildBkInfo(){
	if (!m_hEle) return;
	XEle_ClearBkInfo(m_hEle);
	// 默认 m_bkColor = RGBA(0,0,0,0) 全透明; XCGUI BkFill 里 alpha=0 时跳过 (相当于不画背景).
	// 用户调 SetBkColor 给个不透明色 (e.g. 黑) 就能在没有图 / 图比元素小时看到背景.
	XEle_AddBkFill(m_hEle, element_state_flag_focus_no, m_bkColor);
	RedrawSelf();
}

void CXImageEx::ComputeDestRect(int eleW, int eleH, RECT* pDst) const{
	_XMedia_Render_ComputeDestRect(m_fitMode, m_srcW, m_srcH, eleW, eleH, pDst);
}

void CXImageEx::RedrawSelf(){
	if (m_hEle && XC_IsHELE((HXCGUI)m_hEle)) XEle_Redraw(m_hEle, FALSE);
}

//============================================================================
// 加载主流程
//============================================================================
BOOL CXImageEx::LoadFromFile(const wchar_t* pPath){
	if (!pPath || !*pPath) return FALSE;
	return LoadInternal(pPath, NULL, 0);
}

BOOL CXImageEx::LoadFromMem(const void* pData, int dataLen){
	if (!pData || dataLen <= 0) return FALSE;
	return LoadInternal(NULL, pData, dataLen);
}

void CXImageEx::Unload(){
	m_loadAbort.store(true, std::memory_order_release);
	ClearFrames();
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_curBgra.reset();
		m_curW = m_curH = 0;
		m_curDirty = false;
		m_scaledForFrame  = -1;
		m_scaledForW      = 0;
		m_scaledForH      = 0;
		m_scaledForInterp = -1;
	}
	m_curIdx.store(0, std::memory_order_release);
	m_curFrameStartTick = 0;
	m_state.store(ximage_state_empty, std::memory_order_release);
	m_gdiDibDirty = true;
	RedrawSelf();
}

BOOL CXImageEx::IsLoaded() const{
	return m_srcW > 0 && m_srcH > 0 && !m_frames.empty();
}

void CXImageEx::ClearFrames(){
	for (auto& f : m_frames){
		if (f.pAvFrame) av_frame_free(&f.pAvFrame);
	}
	m_frames.clear();
	m_srcW = m_srcH = 0;
	m_srcPixFmt = AV_PIX_FMT_NONE;
	m_totalDurationSec = 0.0;
	m_animated = false;
}

BOOL CXImageEx::LoadInternal(const wchar_t* pPath, const void* pData, int dataLen){
	Unload();
	m_loadAbort.store(false, std::memory_order_release);

	AVFormatContext* pFmt     = NULL;
	AVCodecContext*  pCodecCtx = NULL;
	AVIOContext*     pIO   = NULL;
	_XMedia_MemBuf*  pMb   = NULL;
	AVPacket*        pkt    = NULL;
	AVFrame*         frame  = NULL;
	BOOL ok = FALSE;
	int  ret = 0;
	std::string utf8;
	int            streamIdx = -1;
	AVStream*      st        = NULL;
	const AVCodec* dec       = NULL;

	pFmt = avformat_alloc_context();
	if (!pFmt){ PostError(AVERROR(ENOMEM), L"avformat_alloc_context 失败"); goto cleanup; }
	pFmt->interrupt_callback.callback = &_XMedia_FF_InterruptCallback;
	pFmt->interrupt_callback.opaque   = (void*)&m_loadAbort;

	if (pData){
		pIO = _XMedia_FF_CreateMemAvIO(pData, dataLen, &pMb);
		if (!pIO){
			PostError(AVERROR(ENOMEM), L"avio_alloc_context 失败");
			goto cleanup;
		}
		pFmt->pb = pIO;
		ret = avformat_open_input(&pFmt, NULL, NULL, NULL);
	} else {
		utf8 = _XMedia_FF_WideToUtf8(pPath);
		if (utf8.empty()){
			PostError(AVERROR(EINVAL), L"路径转换 UTF-8 失败");
			goto cleanup;
		}
		_XMedia_OpenProfile_ profile = _xmedia_open_image_local;
	if (pPath && (wcsncmp(pPath, L"http://", 7) == 0 || wcsncmp(pPath, L"https://", 8) == 0
	           || wcsncmp(pPath, L"rtsp://", 7) == 0)){
			profile = _xmedia_open_video_net;
		}
		ret = _XMedia_FF_OpenWithOptions(&pFmt, utf8.c_str(), profile, &m_loadAbort);
	}
	if (ret < 0){
		PostError(ret, _XMedia_FF_ErrToWide(ret));
		pFmt = NULL;
		goto cleanup;
	}

	// === 2) 探测流信息 ===
	if (m_loadAbort.load(std::memory_order_acquire)) goto cleanup;
	ret = avformat_find_stream_info(pFmt, NULL);
	if (ret < 0){
		if (m_loadAbort.load(std::memory_order_acquire)) goto cleanup;
		PostError(ret, _XMedia_FF_ErrToWide(ret));
		goto cleanup;
	}

	// === 3) 找图像流 (FFmpeg 不区分“图像”跟“序列帧”, 均以 AVMEDIA_TYPE_VIDEO 标记) ===
	// 注意: 第 5 个参数传 NULL 而不是 &dec, 避开 ffmpeg 4.x (AVCodec**) 跟
	// ffmpeg 7.x (const AVCodec**) 的签名差异. 然后手动 avcodec_find_decoder.
	streamIdx = av_find_best_stream(pFmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (streamIdx < 0){
		PostError(AVERROR_STREAM_NOT_FOUND,
		          L"未找到图像流 (可能是不支持的格式或文件损坏)");
		goto cleanup;
	}
	st = pFmt->streams[streamIdx];
	dec = avcodec_find_decoder(st->codecpar->codec_id);
	if (!dec){
		PostError(AVERROR_DECODER_NOT_FOUND, L"找不到解码器");
		goto cleanup;
	}

	// === 4) 打开解码器 ===
	pCodecCtx = avcodec_alloc_context3(dec);
	if (!pCodecCtx){
		PostError(AVERROR(ENOMEM), L"avcodec_alloc_context3 失败");
		goto cleanup;
	}
	ret = avcodec_parameters_to_context(pCodecCtx, st->codecpar);
	if (ret < 0){
		PostError(ret, _XMedia_FF_ErrToWide(ret));
		goto cleanup;
	}
	// 单线程解码: 图片解码量小, 多线程反而 setup 开销更大.
	pCodecCtx->thread_count = 1;
	ret = avcodec_open2(pCodecCtx, dec, NULL);
	if (ret < 0){
		PostError(ret, _XMedia_FF_ErrToWide(ret));
		goto cleanup;
	}

	// === 5) 读包 + 解码循环 ===
	pkt   = av_packet_alloc();
	frame = av_frame_alloc();
	if (!pkt || !frame){
		PostError(AVERROR(ENOMEM), L"av_packet_alloc / av_frame_alloc 失败");
		goto cleanup;
	}

	while ((ret = av_read_frame(pFmt, pkt)) >= 0){
		if (m_loadAbort.load(std::memory_order_acquire)) goto cleanup;
		if (pkt->stream_index == streamIdx){
			ret = avcodec_send_packet(pCodecCtx, pkt);
			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF){
				av_packet_unref(pkt);
				PostError(ret, _XMedia_FF_ErrToWide(ret));
				goto cleanup;
			}
			while (true){
				ret = avcodec_receive_frame(pCodecCtx, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
				if (ret < 0){
					av_packet_unref(pkt);
					PostError(ret, _XMedia_FF_ErrToWide(ret));
					goto cleanup;
				}
				// av_frame_clone: 内部走引用计数, 浅拷贝 + 共享 data buffer. 解码器
				// 下一次写 frame 不会破坏我们存的副本.
				AVFrame* cloned = av_frame_clone(frame);
				if (!cloned){
					av_frame_unref(frame);
					av_packet_unref(pkt);
					PostError(AVERROR(ENOMEM), L"av_frame_clone 失败");
					goto cleanup;
				}
				_XImage_Frame fEntry;
				fEntry.pAvFrame = cloned;
				// duration: pkt->duration 是 stream 时基里的 ticks, 乘 av_q2d(time_base)
				// 得秒. GIF 的 time_base 通常是 1/100, pkt->duration=10 -> 0.1s, 与 GIF 头里
				// 的 delay (单位 1/100s) 直接对得上.
				if (pkt->duration > 0){
					fEntry.durationSec = (double)pkt->duration * av_q2d(st->time_base);
				} else {
					// 兜底: 没 duration (单帧静态图通常是这种), 给 0; 多帧但没 duration 极少见,
					// 给 1/30 当备份.
					fEntry.durationSec = 0.0;
				}
				m_frames.push_back(fEntry);
				av_frame_unref(frame);
			}
		}
		av_packet_unref(pkt);
	}
	// 流末尾 -> drain decoder.
	if (m_loadAbort.load(std::memory_order_acquire)) goto cleanup;
	avcodec_send_packet(pCodecCtx, NULL);
	while (true){
		if (m_loadAbort.load(std::memory_order_acquire)) goto cleanup;
		ret = avcodec_receive_frame(pCodecCtx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
		if (ret < 0){
			PostError(ret, _XMedia_FF_ErrToWide(ret));
			goto cleanup;
		}
		AVFrame* cloned = av_frame_clone(frame);
		if (!cloned){
			PostError(AVERROR(ENOMEM), L"av_frame_clone 失败");
			goto cleanup;
		}
		_XImage_Frame fEntry;
		fEntry.pAvFrame = cloned;
		fEntry.durationSec = 0.0;   // drain 出来的残余帧, 基本上是单帧场景, duration 不重要
		m_frames.push_back(fEntry);
		av_frame_unref(frame);
	}

	if (m_frames.empty()){
		PostError(AVERROR_INVALIDDATA, L"解码完成但 0 帧 (文件可能损坏)");
		goto cleanup;
	}

	// === 6) 元数据补足 ===
	m_srcW = pCodecCtx->width;
	m_srcH = pCodecCtx->height;
	m_srcPixFmt = pCodecCtx->pix_fmt;
	if (m_srcW <= 0 || m_srcH <= 0){
		// 一些容器里 codecpar 没填尺寸, 用第一帧的尺寸兜底.
		m_srcW = m_frames[0].pAvFrame->width;
		m_srcH = m_frames[0].pAvFrame->height;
	}
	if (m_srcPixFmt == AV_PIX_FMT_NONE){
		m_srcPixFmt = (AVPixelFormat)m_frames[0].pAvFrame->format;
	}
	m_animated = (m_frames.size() > 1);
	// 单帧图但 duration 给了非零 (极少见), 也归为静态 (动画语义不成立).
	if (!m_animated){
		m_frames[0].durationSec = 0.0;
	}
	// 总时长.
	m_totalDurationSec = 0.0;
	for (auto& f : m_frames){
		double d = f.durationSec;
		// 兜底: 多帧 duration=0 给 1/30 防 elapsed 永远满足.
		if (m_animated && d <= 0.0){ d = 1.0 / 30.0; f.durationSec = d; }
		m_totalDurationSec += d;
	}

	// === 7) 状态 ===
	m_curIdx.store(0, std::memory_order_release);
	m_curFrameStartTick = ::GetTickCount64();
	if (m_animated){
		// 动画图: 默认自动播放. 用户在 OnLoaded 回调里调 Stop / Pause 可以拦回去.
		m_state.store(ximage_state_playing, std::memory_order_release);
	} else {
		m_state.store(ximage_state_stopped, std::memory_order_release);
	}
	m_pendingLoaded.store(true, std::memory_order_release);
	m_gdiDibDirty = true;
	RedrawSelf();

	ok = TRUE;

cleanup:
	if (pkt)       av_packet_free(&pkt);
	if (frame)     av_frame_free(&frame);
	if (pCodecCtx) avcodec_free_context(&pCodecCtx);
	if (pFmt)      avformat_close_input(&pFmt);   // 内存源时也调, 它会负责清 pb / pIoBuf
	if (pIO){
		// avformat_close_input 已把 pIO->buffer (= pIoBuf) 设为 NULL or 释放.
		// 直接 av_free pIO 即可. 把 buffer 字段先取出再释放, 防 av_freep 漏 / 重复释放.
		if (pIO->buffer) av_freep(&pIO->buffer);
		avio_context_free(&pIO);
	}
	if (pMb) delete pMb;
	if (!ok){
		// 失败: 清掉可能已经 push 进 m_frames 的部分帧, 状态置 error.
		ClearFrames();
		m_state.store(ximage_state_error, std::memory_order_release);
	}
	return ok;
}

//============================================================================
// 动画控制
//============================================================================
void CXImageEx::Play(){
	if (!IsLoaded()) return;
	int s = m_state.load(std::memory_order_acquire);
	if (s == ximage_state_playing) return;
	if (s == ximage_state_paused){
		// 从暂停恢复: 不重置 curFrameStartTick (保持已经进入的进度).
		// 但需要把 tick 基线挪到"现在 - 已经走过的时间", 否则 elapsed 一进来就大跳.
		// 简化处理: 直接重置 tick - 等同于"暂停后下一帧的时序从恢复点重新算",
		// 视觉上最多差一帧, 用户不会感知.
		m_curFrameStartTick = ::GetTickCount64();
		m_state.store(ximage_state_playing, std::memory_order_release);
		return;
	}
	// stopped / ended / empty(非法): 从第 0 帧开始.
	m_curIdx.store(0, std::memory_order_release);
	m_curFrameStartTick = ::GetTickCount64();
	m_state.store(ximage_state_playing, std::memory_order_release);
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_scaledForFrame = -1;   // 强制重缩到第 0 帧
		m_curDirty = true;
	}
	RedrawSelf();
}

void CXImageEx::Pause(){
	if (m_state.load(std::memory_order_acquire) == ximage_state_playing){
		m_state.store(ximage_state_paused, std::memory_order_release);
	}
}

void CXImageEx::Stop(){
	if (!IsLoaded()) return;
	m_state.store(ximage_state_stopped, std::memory_order_release);
	m_curIdx.store(0, std::memory_order_release);
	m_curFrameStartTick = ::GetTickCount64();
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_scaledForFrame = -1;
		m_curDirty = true;
	}
	RedrawSelf();
}

BOOL CXImageEx::IsPlaying() const{
	return m_state.load(std::memory_order_acquire) == ximage_state_playing;
}

void CXImageEx::SetLoop(BOOL bLoop){
	m_loop.store(bLoop != FALSE, std::memory_order_release);
}

BOOL CXImageEx::GetLoop() const{
	return m_loop.load(std::memory_order_acquire) ? TRUE : FALSE;
}

void CXImageEx::SeekFrame(int frameIndex){
	if (!IsLoaded()) return;
	int n = (int)m_frames.size();
	if (frameIndex < 0)  frameIndex = 0;
	if (frameIndex >= n) frameIndex = n - 1;
	int prev = m_curIdx.exchange(frameIndex, std::memory_order_acq_rel);
	m_curFrameStartTick = ::GetTickCount64();
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_scaledForFrame = -1;
		m_curDirty = true;
	}
	if (prev != frameIndex && m_cbFrameChange){
		// 用户主动 Seek 也算帧切换, 直接在调用线程里同步派发回调没问题
		// (UI 线程 -> UI 线程, 无 reentrancy 风险, 因为 Seek 不会被自动调).
		m_cbFrameChange(this, frameIndex, m_userFrameChange);
	}
	RedrawSelf();
}

int CXImageEx::GetCurrentFrame() const{
	if (!IsLoaded()) return -1;
	return m_curIdx.load(std::memory_order_acquire);
}

void CXImageEx::AdvanceFrameIfDue(){
	int idx = m_curIdx.load(std::memory_order_acquire);
	if (idx < 0 || idx >= (int)m_frames.size()) return;

	double dur = m_frames[idx].durationSec;
	if (dur <= 0.0) return;   // 静态帧不推进 (理论上 m_state 不会是 playing, 但兜底)

	ULONGLONG now = ::GetTickCount64();
	ULONGLONG elapsedMs = now - m_curFrameStartTick;
	double elapsedSec = (double)elapsedMs / 1000.0;
	if (elapsedSec < dur) return;

	// 推进. 跨多帧 (elapsedSec > 单帧 duration 多倍) 时一次性多跳, 防止 UI 卡了一段后
	// 动画"补回去" - 视觉上看起来快速倒带不如直接跳到对应位置.
	int n = (int)m_frames.size();
	int newIdx = idx;
	double consumed = 0.0;
	while (elapsedSec - consumed >= m_frames[newIdx].durationSec
	       && newIdx + 1 < n){
		consumed += m_frames[newIdx].durationSec;
		newIdx++;
	}
	// 还差最后一段没消化完: 看是否到尾了.
	if (newIdx == n - 1 && elapsedSec - consumed >= m_frames[newIdx].durationSec){
		// 走到末帧且消化完最后一帧 = 整轮播放完成.
		consumed += m_frames[newIdx].durationSec;
		if (m_loop.load(std::memory_order_acquire)){
			// 循环: 回 0, 把多余的 elapsed (consumed 之后的部分) 当下一轮已走时间.
			newIdx = 0;
			// 重置 tick 基线: 以 "现在 - 残余" 为新的 start, 下一轮第 0 帧从这个点起算.
			double residualSec = elapsedSec - consumed;
			ULONGLONG residualMs = (ULONGLONG)(residualSec * 1000.0);
			m_curFrameStartTick = now - residualMs;
		} else {
			// 非循环: 停在末帧, 状态 ended, 派发回调.
			m_state.store(ximage_state_ended, std::memory_order_release);
			m_pendingEnded.store(true, std::memory_order_release);
			// 不更新 curIdx (停在末帧).
			return;
		}
	} else {
		// 普通推进: 把消化掉的 consumed 折回 tick.
		ULONGLONG consumedMs = (ULONGLONG)(consumed * 1000.0);
		m_curFrameStartTick = now;
		// 减去 (elapsed - consumed) 的 ms: 让 tick 基线退一点点, 反映"我们还没用完
		// 这一帧的剩余时间 (residual)". 但 GetTickCount64 是 unsigned, 减法可能下溢;
		// 简单处理: 直接用 now - residualMs.
		double residualSec = elapsedSec - consumed;
		ULONGLONG residualMs = (ULONGLONG)(residualSec * 1000.0);
		m_curFrameStartTick = now - residualMs;
		(void)consumedMs;
	}

	if (newIdx != idx){
		m_curIdx.store(newIdx, std::memory_order_release);
		{
			std::lock_guard<std::mutex> lk(m_curFrameMutex);
			m_scaledForFrame = -1;   // 帧变了, 让 OnPaint 重缩
			m_curDirty = true;
		}
		if (m_cbFrameChange){
			m_cbFrameChange(this, newIdx, m_userFrameChange);
		}
		RedrawSelf();
	}
}

//============================================================================
// 缩放: 把 m_frames[curIdx] 缩到 (dstW, dstH) BGRA, 写进 m_curBgra.
// 调用时机: OnPaint 拿到目标矩形后, 检查 m_scaledForFrame / W / H / Interp 任一不
//           匹配就调用本函数. AdvanceFrameIfDue / OnSizeImpl / SeekFrame 只 *标记*
//           dirty (置 m_scaledForFrame=-1), 不在那里直接缩, 避免 resize 高频触发浪费.
//============================================================================
int CXImageEx::EffectiveInterp() const{
	if (m_interp != ximage_interp_auto) return m_interp;
	// 自动: 静态图 Lanczos (一次性, 高质量), 动画图 Bilinear (60fps 实时, 平衡).
	return m_animated ? ximage_interp_bilinear : ximage_interp_lanczos;
}

void CXImageEx::RescaleCurrentFrameIfNeeded(int dstW, int dstH){
	if (dstW <= 0 || dstH <= 0) return;
	if (!IsLoaded()) return;
	int idx = m_curIdx.load(std::memory_order_acquire);
	if (idx < 0 || idx >= (int)m_frames.size()) return;
	AVFrame* src = m_frames[idx].pAvFrame;
	if (!src) return;
	int interp = EffectiveInterp();

	if (m_scaledForFrame  == idx
	 && m_scaledForW      == dstW
	 && m_scaledForH      == dstH
	 && m_scaledForInterp == interp){
		return;
	}

	_XMedia_FF_SwsCache cache;
	cache.pSws       = m_pSws;
	cache.lastSrcW   = m_swsLastSrcW;
	cache.lastSrcH   = m_swsLastSrcH;
	cache.lastSrcFmt = m_swsLastSrcFmt;
	cache.lastDstW   = m_swsLastDstW;
	cache.lastDstH   = m_swsLastDstH;
	cache.lastFlags  = m_swsLastFlags;

	std::vector<uint8_t> tmp;
	int outW = 0, outH = 0;
	if (!_XMedia_FF_SwsAvFrameToBgra(&cache, src, dstW, dstH, interp, &tmp, &outW, &outH)){
		return;
	}

	m_pSws          = cache.pSws;
	m_swsLastSrcW   = cache.lastSrcW;
	m_swsLastSrcH   = cache.lastSrcH;
	m_swsLastSrcFmt = cache.lastSrcFmt;
	m_swsLastDstW   = cache.lastDstW;
	m_swsLastDstH   = cache.lastDstH;
	m_swsLastFlags  = cache.lastFlags;

	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_curBgra = std::make_shared<std::vector<uint8_t>>(std::move(tmp));
		m_curW    = outW;
		m_curH    = outH;
		m_curDirty = true;
		m_scaledForFrame  = idx;
		m_scaledForW      = dstW;
		m_scaledForH      = dstH;
		m_scaledForInterp = interp;
	}
	m_gdiDibDirty = true;
}

//============================================================================
// 渲染分流 (XE_PAINT 入口)
//============================================================================
int CXImageEx::OnPaintImpl(HELE /*hEle*/, HDRAW hDraw, BOOL* pbHandled){
	if (!hDraw){
		// 无 hDraw 不该发生 (XCGUI 保证), 防御性 fallback 到默认绘制.
		return 0;
	}
	// 我们完全接管: 跳过 XCGUI 默认 BkInfo 之外的边框 / 焦点框.
	// BkInfo 还是要 XCGUI 画 (我们调了 XEle_AddBkFill), 所以让 XCGUI 默认逻辑先跑,
	// 然后我们在 *默认绘制完之后* 再画图. 这就需要 *不* 设 pbHandled=TRUE - 而是
	// 让 XCGUI 走完默认链 (画 BkInfo), 之后在 XCGUI 把 hDraw 还给我们的窗口画完时
	// 自动覆盖. 但 XCGUI 的 paint 链是单次 -> 一旦 *pbHandled=FALSE 我们这里画完了
	// 还会被 XCGUI 默认链覆盖.
	//
	// 解法: 我们 *接管* 所有绘制, 自己手画 BkInfo 等价物 (FillRect
	// 用 m_bkColor), 然后画图. 这样画一次 OK.
	*pbHandled = TRUE;

	HDC hdc = (HDC)XDraw_GetHDC(hDraw);
	ID2D1RenderTarget* rt = (ID2D1RenderTarget*)XDraw_GetD2dRenderTarget(hDraw);
	if (rt){
		OnPaintD2D(rt, hDraw);
	} else if (hdc){
		OnPaintGdi(hdc, hDraw);
	}
	return 0;
}

void CXImageEx::OnPaintD2D(ID2D1RenderTarget* rt, HDRAW /*hDraw*/){
	RECT rcEle;
	XEle_GetWndClientRectDPI(m_hEle, &rcEle);
	int eleW = rcEle.right  - rcEle.left;
	int eleH = rcEle.bottom - rcEle.top;
	if (eleW <= 0 || eleH <= 0) return;

	RECT rcDst = {};
	const uint8_t* bgraPtr = NULL;
	int bmpW = 0, bmpH = 0;
	bool dirty = false;
	_XMedia_FramePaintSnap snap;

	if (IsLoaded()){
		ComputeDestRect(eleW, eleH, &rcDst);
		int dstW = rcDst.right - rcDst.left;
		int dstH = rcDst.bottom - rcDst.top;
		if (dstW > 0 && dstH > 0){
			if (!_XMedia_SizeMoveGuard_IsActive((void*)m_hEle)){
				RescaleCurrentFrameIfNeeded(dstW, dstH);
			}
			int pitch = 0;
			snap = _XMedia_FrameSnapForPaint(m_curFrameMutex, m_curBgra,
			                                 m_curW, m_curH, pitch, m_curDirty, false);
			bmpW = snap.w;
			bmpH = snap.h;
			dirty = snap.contentDirty;
			if (!m_d2dCache.pBmp && snap.pixels && !dirty)
				dirty = true;
			if (snap.pixels)
				bgraPtr = snap.pixels->data();
		}
	}

	_XMedia_Render_PaintD2DParams params = {};
	params.rt               = rt;
	params.rcEle            = &rcEle;
	params.bkColor          = m_bkColor;
	params.bkRespectAlpha   = true;
	params.fitMode          = m_fitMode;
	params.rcDstLocal       = &rcDst;
	params.srcW             = bmpW;
	params.srcH             = bmpH;
	params.bgra             = bgraPtr;
	params.pDirty           = &dirty;
	params.alphaMode        = _xmedia_d2d_alpha_premul;
	params.stretchFromSrc   = false;
	params.cache            = &m_d2dCache;
	_XMedia_Render_PaintD2D_Bgra(&params);
}

void CXImageEx::OnPaintGdi(HDC hdc, HDRAW hDraw){
	int eleW = XEle_GetWidth(m_hEle);
	int eleH = XEle_GetHeight(m_hEle);
	int eleWPhys = (int)((float)eleW * m_dpiScale + 0.5f);
	int eleHPhys = (int)((float)eleH * m_dpiScale + 0.5f);
	if (eleWPhys <= 0 || eleHPhys <= 0) return;

	RECT rcDstHdc = { 0, 0, eleWPhys, eleHPhys };
	XDraw_ConvRect(hDraw, &rcDstHdc);

	RECT rcDst;
	ComputeDestRect(eleWPhys, eleHPhys, &rcDst);
	int dstW = rcDst.right - rcDst.left;
	int dstH = rcDst.bottom - rcDst.top;
	if (IsLoaded() && dstW > 0 && dstH > 0
	    && !_XMedia_SizeMoveGuard_IsActive((void*)m_hEle)){
		RescaleCurrentFrameIfNeeded(dstW, dstH);
	}

	if (m_gdiOffscreen.dibW != eleWPhys || m_gdiOffscreen.dibH != eleHPhys || !m_gdiOffscreen.dib){
		m_gdiDibDirty = true;
	}

	int pitch = 0;
	_XMedia_FramePaintSnap snap = _XMedia_FrameSnapForPaint(m_curFrameMutex, m_curBgra,
	                                                        m_curW, m_curH, pitch, m_curDirty, false);

	_XMedia_Render_PaintGdiParams params = {};
	params.hdcDest      = hdc;
	params.rcDstHdc     = &rcDstHdc;
	params.eleWPhys     = eleWPhys;
	params.eleHPhys     = eleHPhys;
	params.bkColor      = m_bkColor;
	params.rcDstLocal   = &rcDst;
	params.srcW         = snap.w;
	params.srcH         = snap.h;
	params.bgra         = snap.pixels ? snap.pixels->data() : NULL;
	params.needRedraw   = m_gdiDibDirty || snap.contentDirty;
	params.pDibDirty    = &m_gdiDibDirty;
	params.pDib         = &m_gdiOffscreen;
	_XMedia_Render_PaintGdi_Bgra(&params);
}

//============================================================================
// 元数据 / 显示属性 setter / getter
//============================================================================
int    CXImageEx::GetSrcWidth()       const { return m_srcW; }
int    CXImageEx::GetSrcHeight()      const { return m_srcH; }
int    CXImageEx::GetFrameCount()     const { return (int)m_frames.size(); }
BOOL   CXImageEx::IsAnimated()        const { return m_animated ? TRUE : FALSE; }
double CXImageEx::GetTotalDuration()  const { return m_totalDurationSec; }

double CXImageEx::GetFrameDuration(int frameIndex) const{
	if (frameIndex < 0 || frameIndex >= (int)m_frames.size()) return 0.0;
	return m_frames[frameIndex].durationSec;
}

int CXImageEx::GetState() const{
	return m_state.load(std::memory_order_acquire);
}

void CXImageEx::SetFitMode(int mode){
	if (mode < xmedia_fit_contain || mode > xmedia_fit_original) return;
	if (m_fitMode == mode) return;
	m_fitMode = mode;
	// 适配模式变了 -> 目标矩形变 -> 重缩.
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_scaledForFrame = -1;
		m_curDirty = true;
	}
	m_gdiDibDirty = true;
	RedrawSelf();
}

int CXImageEx::GetFitMode() const{ return m_fitMode; }

void CXImageEx::SetInterpolation(int interp){
	if (m_interp == interp) return;
	m_interp = interp;
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_scaledForFrame = -1;
		m_curDirty = true;
	}
	m_gdiDibDirty = true;
	RedrawSelf();
}

int CXImageEx::GetInterpolation() const{ return m_interp; }

void CXImageEx::SetBkColor(COLORREF color){
	if (m_bkColor == color) return;
	m_bkColor = color;
	RebuildBkInfo();
	m_gdiDibDirty = true;
	RedrawSelf();
}

COLORREF CXImageEx::GetBkColor() const{ return m_bkColor; }

//============================================================================
// 事件回调注册
//============================================================================
void CXImageEx::SetOnLoaded(XIMAGE_PROC_LOADED cb, void* pUser){
	m_cbLoaded = cb; m_userLoaded = pUser;
}
void CXImageEx::SetOnError(XIMAGE_PROC_ERROR cb, void* pUser){
	m_cbError = cb; m_userError = pUser;
}
void CXImageEx::SetOnFrameChange(XIMAGE_PROC_FRAMECHANGE cb, void* pUser){
	m_cbFrameChange = cb; m_userFrameChange = pUser;
}
void CXImageEx::SetOnEnded(XIMAGE_PROC_ENDED cb, void* pUser){
	m_cbEnded = cb; m_userEnded = pUser;
}

//============================================================================
// 错误派发 / pending 回调分发
//============================================================================
void CXImageEx::PostError(int code, const wchar_t* msg){
	{
		std::lock_guard<std::mutex> lk(m_pendingErrMutex);
		m_pendingErrCode = code;
		m_pendingErrMsg  = msg ? msg : L"";
	}
	m_pendingError.store(true, std::memory_order_release);
}

void CXImageEx::PostError(int code, const std::wstring& msg){
	PostError(code, msg.c_str());
}

void CXImageEx::DispatchPendingCallbacks(){
	// Loaded
	if (m_pendingLoaded.exchange(false, std::memory_order_acq_rel)){
		if (m_cbLoaded){
			m_cbLoaded(this, m_srcW, m_srcH,
			           (int)m_frames.size(), m_totalDurationSec, m_userLoaded);
		}
	}
	// Error
	if (m_pendingError.exchange(false, std::memory_order_acq_rel)){
		int code; std::wstring msg;
		{
			std::lock_guard<std::mutex> lk(m_pendingErrMutex);
			code = m_pendingErrCode;
			msg  = m_pendingErrMsg;
		}
		if (m_cbError){
			m_cbError(this, code, msg.c_str(), m_userError);
		}
	}
	// Ended
	if (m_pendingEnded.exchange(false, std::memory_order_acq_rel)){
		if (m_cbEnded){
			m_cbEnded(this, m_userEnded);
		}
	}
}

