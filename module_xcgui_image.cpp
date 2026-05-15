//============================================================================
// module_xcgui_image.cpp
//
// CXImageEx 实现.
//
// 模块定位: 用 FFmpeg 解码 + 自家 D2D/GDI+ 双路径渲染, 替代 XCGUI 内置的
//           XShapePicture (格式有限 + 最近邻缩放) 和 XShapeGif (仅 GIF).
//
// 架构概览:
//   - 加载: LoadFromFile/Mem -> LoadInternal: avformat_open_input ->
//           find_stream_info -> 找到图像流 (FFmpeg 里不区分 “图像”跟
//           “序列帧”, 它们在容器中都以 AVMEDIA_TYPE_VIDEO 标记) ->
//           av_read_frame 循环 -> avcodec_send_packet/receive_frame 解码 ->
//           av_frame_clone 进 m_frames + 提取 pkt->duration 算 durationSec.
//           整个加载 *同步* (UI 线程内完成). 大图 / 网络图建议自己丢线程池.
//
//   - 缩放: 不在加载阶段就缩到目标尺寸, 而是 *显示阶段* swscale 一步缩.
//           原因 (1) 内存: 1080p × 60 帧 BGRA = 460MB, 太大.
//           (2) 质量: 加载时不知道目标尺寸, 多次缩 = 多次损失.
//           (3) 灵活性: resize 后只需重缩 *一帧* (动画当前帧), O(1) 不是 O(N).
//
//   - 动画: XE_XC_TIMER 16ms tick. 检查 elapsed >= m_frames[curIdx].durationSec
//           就推进 curIdx, 重新缩当前帧 -> 上屏. 用 *真实 per-frame duration*
//           (从 pkt->duration × stream->time_base 算), 不再固定 33ms.
//
//   - 渲染: D2D 主路径 (ID2D1Bitmap CopyFromMemory + DrawBitmap) / GDI+ 兜底路径
//           (离屏 DIB 同尺寸 StretchDIBits 后一次 BitBlt 上屏).
//           BkInfo 协同 XCGUI 标准管线, 让 alpha 透明工作正常.
//============================================================================

#include "module_xcgui_image.h"

#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <chrono>

// 本模块只用 avformat / avcodec / swscale 三家 API. 不读 frame->duration
// (用 pkt->duration 代替, 避开 4.x 跟 5.x+ 之间的字段变化), 不跳转音频代码
// 路径. 结果: FFmpeg 4.4 到 8.x 头文件均可编译, ABI 上只依赖这三家的
// 公开 API.

#ifndef SafeRelease
#define SafeRelease(p) do { if (p) { (p)->Release(); (p) = NULL; } } while (0)
#endif

//============================================================================
// 工具: 内存源 IO (avio_alloc_context 用)
//============================================================================
namespace {

// LoadFromMem 把内存当文件喂给 avformat. avio 内部会按需要调 read / seek.
// 这个 buffer *不* 持有数据 (pData 是调用方传入), 只记录指针 + 长度 + 当前位置.
struct _XImage_MemBuf{
	const uint8_t* data = NULL;
	int            len  = 0;
	int            pos  = 0;
};

int _XImage_MemRead(void* opaque, uint8_t* buf, int bufSize){
	_XImage_MemBuf* mb = (_XImage_MemBuf*)opaque;
	int remain = mb->len - mb->pos;
	if (remain <= 0) return AVERROR_EOF;
	int n = (bufSize < remain) ? bufSize : remain;
	memcpy(buf, mb->data + mb->pos, (size_t)n);
	mb->pos += n;
	return n;
}

int64_t _XImage_MemSeek(void* opaque, int64_t offset, int whence){
	_XImage_MemBuf* mb = (_XImage_MemBuf*)opaque;
	if (whence == AVSEEK_SIZE) return mb->len;
	int64_t target = 0;
	switch (whence){
	case SEEK_SET: target = offset;                break;
	case SEEK_CUR: target = (int64_t)mb->pos + offset; break;
	case SEEK_END: target = (int64_t)mb->len + offset; break;
	default:       return -1;
	}
	if (target < 0 || target > mb->len) return -1;
	mb->pos = (int)target;
	return target;
}

} // anonymous namespace

//============================================================================
// 构造 / 析构
//============================================================================
CXImageEx::CXImageEx(){
	// avformat_network_init 让 http(s):// 等 URL 可用. 幂等, 多次调用安全,
	// 静态局部 once 模式避免重复 init.
	struct OnceInit{
		OnceInit(){ avformat_network_init(); }
		~OnceInit(){ avformat_network_deinit(); }
	};
	static OnceInit s_once;
	(void)s_once;
}

CXImageEx::~CXImageEx(){
	// 析构路径走 Unload (释放帧 / sws), 渲染资源由 OnDestroyImpl 释放.
	// 析构发生时 XCGUI 元素可能已被销毁 (m_hEle = NULL by OnDestroyImpl), 这里
	// 只清不依赖元素的资源.
	Unload();
	// 兜底: 如果 OnDestroyImpl 没轮到 (例如对象创建后从未 Create), 这里也释放.
	if (m_pSws){ sws_freeContext(m_pSws); m_pSws = NULL; }
	SafeRelease(m_pD2DBmp);
	m_pLastRT = NULL;
	ReleaseGdiDib();
}

//============================================================================
// Create / 事件注册
//============================================================================
HELE CXImageEx::Create(int x, int y, int cx, int cy, HXCGUI hParent){
	// 直接 XEle_Create: 不需要 CXLayout 那套自动布局 (没有内置控件栏 / 子节点),
	// 普通 XEle 元素就够 - 也避开 CXLayout default mouse-through 等需要额外配置的细节.
	m_hEle = XEle_Create(x, y, cx, cy, hParent);
	if (!m_hEle) return NULL;
	// 我们走 BkInfo 路径填底色 (RebuildBkInfo 里 XEle_AddBkFill), 必须显式关掉
	// 透明传导, 否则父背景会透过 alpha=0 的图片像素显出来 (用户期待的是 m_bkColor).
	XEle_EnableBkTransparent(m_hEle, FALSE);

	RefreshDpiScale();
	RebuildBkInfo();
	InstallEvents();
	return m_hEle;
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
	// XE_DESTROY: 释放 D2D / GDI / FFmpeg 资源.
	XEle_RegEventCPP1(m_hEle, XE_DESTROY,  &CXImageEx::OnDestroyImpl);

	XEle_SetXCTimer(m_hEle, kTimerId_Tick, kTimerInterval_Ms);
}

int CXImageEx::OnDestroyImpl(HELE /*hEle*/, BOOL* /*pbHandled*/){
	// XE_DESTROY 在子对象销毁 *之前*. 顺序:
	//   (1) 关 timer (避免 join 期间 timer 还触发).
	//   (2) Unload 释放帧 / sws.
	//   (3) D2D / GDI 渲染资源也释放 (D2D Bitmap 跟 RT 绑, RT 析构后不能再 Release).
	//   (4) m_hEle = NULL: 防后续 ~CXImageEx -> RedrawSelf 拿陈旧句柄崩 / 弹"句柄无效".
	XEle_KillXCTimer(m_hEle, kTimerId_Tick);
	Unload();
	if (m_pSws){ sws_freeContext(m_pSws); m_pSws = NULL; }
	SafeRelease(m_pD2DBmp);
	m_pLastRT = NULL;
	m_d2dBmpW = m_d2dBmpH = 0;
	ReleaseGdiDib();
	m_hEle = NULL;
	return 0;
}

//============================================================================
// 尺寸 / 定时器
//============================================================================
int CXImageEx::OnSizeImpl(HELE /*hEle*/, int /*nFlags*/, UINT /*nAdjustNo*/, BOOL* /*pbHandled*/){
	RefreshDpiScale();
	// 元素尺寸变了 -> 显示目标矩形变 -> 必须重缩当前帧. 标记 dirty 让 OnPaint 触发重缩.
	// 注意不在这里直接调 RescaleCurrentFrameIfNeeded: resize 高频, 让 OnPaint 在真正需要画
	// 之前才缩, 避免 resize 一帧未到 paint 又被 resize 覆盖的浪费.
	m_gdiDibDirty = true;
	// m_curW = 0 是给 RescaleCurrentFrameIfNeeded 的"未初始化"哨兵, 设 0 强制下次 paint 必重缩.
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_curW = m_curH = 0;
		m_curDirty = true;
	}
	RedrawSelf();
	return 0;
}

int CXImageEx::OnTimerImpl(HELE /*hEle*/, UINT nTimerId, BOOL* /*pbHandled*/){
	if (nTimerId != kTimerId_Tick) return 0;

	// 1) 派发 pending 事件 (Loaded / Error / Ended). 这条路径让 LoadInternal 等同步代码
	//    能"产事件不直接调用回调", 把回调延后到 UI 线程下一个 16ms tick - 用户在回调里
	//    再次调 LoadFromFile 也不会撞重入.
	DispatchPendingCallbacks();

	// 2) 动画推进. 只在 playing 状态时做.
	if (m_state.load(std::memory_order_acquire) == ximage_state_playing){
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
	pDst->left = pDst->top = pDst->right = pDst->bottom = 0;
	if (eleW <= 0 || eleH <= 0 || m_srcW <= 0 || m_srcH <= 0) return;

	if (m_fitMode == ximage_fit_stretch){
		pDst->left = 0; pDst->top = 0;
		pDst->right = eleW; pDst->bottom = eleH;
		return;
	}
	if (m_fitMode == ximage_fit_original){
		// 居中, 不缩放. 大于元素时让 swscale 输出尺寸 = 元素尺寸 (= 自然裁切).
		// 小于元素时居中, 周围露出背景.
		int dx = (eleW - m_srcW) / 2;
		int dy = (eleH - m_srcH) / 2;
		pDst->left   = dx;
		pDst->top    = dy;
		pDst->right  = dx + m_srcW;
		pDst->bottom = dy + m_srcH;
		return;
	}

	// contain / cover: 等比缩放. contain 取 min(scale), cover 取 max(scale).
	double sx = (double)eleW / (double)m_srcW;
	double sy = (double)eleH / (double)m_srcH;
	double s  = (m_fitMode == ximage_fit_cover) ? (std::max)(sx, sy) : (std::min)(sx, sy);

	int w = (int)(m_srcW * s + 0.5);
	int h = (int)(m_srcH * s + 0.5);
	int dx = (eleW - w) / 2;
	int dy = (eleH - h) / 2;
	pDst->left   = dx;
	pDst->top    = dy;
	pDst->right  = dx + w;
	pDst->bottom = dy + h;
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
	ClearFrames();
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_curBgra.clear();
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
	// 先卸载上次的, 防止追加.
	Unload();

	AVFormatContext* pFmt     = NULL;
	AVCodecContext*  pCodecCtx = NULL;
	AVIOContext*     pIO   = NULL;
	_XImage_MemBuf*  pMb   = NULL;
	unsigned char*   pIoBuf = NULL;
	AVPacket*        pkt    = NULL;
	AVFrame*         frame  = NULL;
	const int kIoBufSize = 4096;
	BOOL ok = FALSE;
	int  ret = 0;
	std::string utf8;
	int            streamIdx = -1;
	AVStream*      st        = NULL;
	const AVCodec* dec       = NULL;

	// === 1) 打开容器 ===
	pFmt = avformat_alloc_context();
	if (!pFmt){ PostError(AVERROR(ENOMEM), L"avformat_alloc_context 失败"); goto cleanup; }

	if (pData){
		// 内存源: 用 avio 自定义 IO. avformat_open_input 第二个参数传 NULL.
		pIoBuf = (unsigned char*)av_malloc(kIoBufSize);
		if (!pIoBuf){ PostError(AVERROR(ENOMEM), L"av_malloc IO buf 失败"); goto cleanup; }
		pMb = new _XImage_MemBuf();
		pMb->data = (const uint8_t*)pData;
		pMb->len  = dataLen;
		pMb->pos  = 0;
		pIO = avio_alloc_context(pIoBuf, kIoBufSize, 0, pMb,
		                          &_XImage_MemRead, NULL, &_XImage_MemSeek);
		if (!pIO){
			av_free(pIoBuf); pIoBuf = NULL;
			delete pMb; pMb = NULL;
			PostError(AVERROR(ENOMEM), L"avio_alloc_context 失败");
			goto cleanup;
		}
		pFmt->pb = pIO;
		// avformat_open_input 在内存源下接管 pIoBuf 所有权, 失败时它会把 pIoBuf 释放
		// (留 pIO 的 buffer 字段为 NULL). 我们后面只 free pIO 即可.
		ret = avformat_open_input(&pFmt, NULL, NULL, NULL);
	} else {
		utf8 = WideToUtf8(pPath);
		ret = avformat_open_input(&pFmt, utf8.c_str(), NULL, NULL);
	}
	if (ret < 0){
		PostError(ret, AvErrToWStr(ret));
		// 失败时 avformat_open_input 已释放 pFmt, 这里置 NULL 防 cleanup 重复 close.
		pFmt = NULL;
		goto cleanup;
	}

	// === 2) 探测流信息 ===
	ret = avformat_find_stream_info(pFmt, NULL);
	if (ret < 0){
		PostError(ret, AvErrToWStr(ret));
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
		PostError(ret, AvErrToWStr(ret));
		goto cleanup;
	}
	// 单线程解码: 图片解码量小, 多线程反而 setup 开销更大.
	pCodecCtx->thread_count = 1;
	ret = avcodec_open2(pCodecCtx, dec, NULL);
	if (ret < 0){
		PostError(ret, AvErrToWStr(ret));
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
		if (pkt->stream_index == streamIdx){
			ret = avcodec_send_packet(pCodecCtx, pkt);
			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF){
				av_packet_unref(pkt);
				PostError(ret, AvErrToWStr(ret));
				goto cleanup;
			}
			while (true){
				ret = avcodec_receive_frame(pCodecCtx, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
				if (ret < 0){
					av_packet_unref(pkt);
					PostError(ret, AvErrToWStr(ret));
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
	avcodec_send_packet(pCodecCtx, NULL);
	while (true){
		ret = avcodec_receive_frame(pCodecCtx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
		if (ret < 0){
			PostError(ret, AvErrToWStr(ret));
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

	// 命中缓存 -> 直接返.
	if (m_scaledForFrame  == idx
	 && m_scaledForW      == dstW
	 && m_scaledForH      == dstH
	 && m_scaledForInterp == interp){
		return;
	}

	// JPEG 走 yuvj420p (J=full range), 但 ffmpeg 7.x 对 yuvj* 输出 deprecated 警告
	// (`deprecated pixel format used, make sure you did set range correctly`).
	// 正确做法: 把 yuvj* 重映射到对应 yuv*, 然后 sws_setColorspaceDetails 显式
	// 标 src range = JPEG (full). yuvj* 跟 yuv* 数据布局完全相同, 仅 range 标记差异.
	AVPixelFormat srcFmt = (AVPixelFormat)src->format;
	bool isJpegRange = false;
	switch (srcFmt){
	case AV_PIX_FMT_YUVJ420P: srcFmt = AV_PIX_FMT_YUV420P; isJpegRange = true; break;
	case AV_PIX_FMT_YUVJ422P: srcFmt = AV_PIX_FMT_YUV422P; isJpegRange = true; break;
	case AV_PIX_FMT_YUVJ444P: srcFmt = AV_PIX_FMT_YUV444P; isJpegRange = true; break;
	case AV_PIX_FMT_YUVJ440P: srcFmt = AV_PIX_FMT_YUV440P; isJpegRange = true; break;
	case AV_PIX_FMT_YUVJ411P: srcFmt = AV_PIX_FMT_YUV411P; isJpegRange = true; break;
	default: break;
	}

	// sws_getCachedContext: 输入参数变了它内部 free + 新建; 没变直接复用.
	// 我们额外维护 m_swsLastXxx 是为了"已经知道要变"的早退路径 (避免每次都进
	// sws_getCachedContext, 它内部锁 + 比较有小开销).
	bool needNew = (m_pSws == NULL)
	            || (m_swsLastSrcW   != src->width)
	            || (m_swsLastSrcH   != src->height)
	            || (m_swsLastSrcFmt != srcFmt)
	            || (m_swsLastDstW   != dstW)
	            || (m_swsLastDstH   != dstH)
	            || (m_swsLastFlags  != interp);
	if (needNew){
		m_pSws = sws_getCachedContext(m_pSws,
		                               src->width, src->height, srcFmt,
		                               dstW, dstH, AV_PIX_FMT_BGRA,
		                               interp,
		                               NULL, NULL, NULL);
		if (!m_pSws){
			// 极少出现: pix_fmt 不被 swscale 支持 (奇异 codec). 静默失败, 这一帧不画.
			return;
		}
		// JPEG full range 显式告诉 swscale, 否则它会按 limited range 反 unpack ->
		// 输出 BGRA 偏暗 (0~255 区间被压到 16~235). dst BGRA 是 RGB, range 概念不
		// 适用, 写 0 即可. 矩阵用默认 BT601/BT709 (sws_getCoefficients(SWS_CS_DEFAULT)).
		if (isJpegRange){
			const int* coeff = sws_getCoefficients(SWS_CS_DEFAULT);
			sws_setColorspaceDetails(m_pSws,
			                          coeff, 1,            // src: full range
			                          coeff, 0,            // dst: doesn't matter for RGB
			                          0,                    // brightness
			                          1 << 16,              // contrast = 1.0
			                          1 << 16);             // saturation = 1.0
		}
		m_swsLastSrcW   = src->width;
		m_swsLastSrcH   = src->height;
		m_swsLastSrcFmt = srcFmt;
		m_swsLastDstW   = dstW;
		m_swsLastDstH   = dstH;
		m_swsLastFlags  = interp;
	}

	// 准备目标 buffer + 一次 sws_scale. 输出连续无 padding (linesize = 4*w).
	std::vector<uint8_t> tmp((size_t)dstW * dstH * 4);
	uint8_t* dstData[1]    = { tmp.data() };
	int      dstLinesize[1] = { dstW * 4 };
	int rh = sws_scale(m_pSws,
	                    (const uint8_t* const*)src->data, src->linesize,
	                    0, src->height,
	                    dstData, dstLinesize);
	if (rh <= 0) return;

	// 锁内 swap-in: OnPaint 那边 lock 后拿走 m_curBgra; 这里写新 BGRA. 锁粒度小.
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		m_curBgra = std::move(tmp);
		m_curW    = dstW;
		m_curH    = dstH;
		m_curDirty = true;          // OnPaint 上传后清
		m_scaledForFrame  = idx;
		m_scaledForW      = dstW;
		m_scaledForH      = dstH;
		m_scaledForInterp = interp;
	}
	m_gdiDibDirty = true;            // GDI 路径需要重画 DIB
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
	// === 元素客户区 (物理像素, 与 D2D RT face-value 同坐标系) ===
	RECT rcEle;
	XEle_GetWndClientRectDPI(m_hEle, &rcEle);
	int eleW = rcEle.right  - rcEle.left;
	int eleH = rcEle.bottom - rcEle.top;
	if (eleW <= 0 || eleH <= 0) return;

	// === 1) 背景 ===
	BYTE bgA = (BYTE)((m_bkColor >> 24) & 0xFF);
	if (bgA != 0){
		// XCGUI RGBA 宏布局 0xAABBGGRR; D2D ColorF 需要 R,G,B,A floats.
		BYTE r = (BYTE)((m_bkColor      ) & 0xFF);
		BYTE g = (BYTE)((m_bkColor >>  8) & 0xFF);
		BYTE b = (BYTE)((m_bkColor >> 16) & 0xFF);
		ID2D1SolidColorBrush* pBrush = NULL;
		HRESULT hr = rt->CreateSolidColorBrush(
			D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, bgA / 255.0f),
			&pBrush);
		if (SUCCEEDED(hr) && pBrush){
			D2D1_RECT_F rc = D2D1::RectF(
				(FLOAT)rcEle.left, (FLOAT)rcEle.top,
				(FLOAT)rcEle.right, (FLOAT)rcEle.bottom);
			rt->FillRectangle(rc, pBrush);
			pBrush->Release();
		}
	}

	if (!IsLoaded()) return;

	// === 2) 算目标矩形 (物理像素, 元素本地坐标 0..eleW/eleH) ===
	RECT rcDst;
	ComputeDestRect(eleW, eleH, &rcDst);
	int dstW = rcDst.right - rcDst.left;
	int dstH = rcDst.bottom - rcDst.top;
	if (dstW <= 0 || dstH <= 0) return;

	// === 3) 重缩当前帧 (按需) ===
	RescaleCurrentFrameIfNeeded(dstW, dstH);

	// === 4) D2D Bitmap: RT 变化 / 尺寸变化重建; 帧变化只 CopyFromMemory ===
	if (rt != m_pLastRT){
		SafeRelease(m_pD2DBmp);
		m_pLastRT = rt;
		m_d2dBmpW = m_d2dBmpH = 0;
	}

	int bmpW = 0, bmpH = 0;
	std::vector<uint8_t> localCopy;
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		bmpW = m_curW;
		bmpH = m_curH;
		if (m_curDirty && bmpW > 0 && bmpH > 0 && !m_curBgra.empty()){
			localCopy = m_curBgra;
			m_curDirty = false;
		}
	}
	if (bmpW <= 0 || bmpH <= 0) return;

	bool needCreate = (m_pD2DBmp == NULL)
	               || (m_d2dBmpW != bmpW)
	               || (m_d2dBmpH != bmpH);
	if (needCreate){
		SafeRelease(m_pD2DBmp);
		D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
		HRESULT hr = rt->CreateBitmap(D2D1::SizeU((UINT32)bmpW, (UINT32)bmpH),
		                              NULL, 0, props, &m_pD2DBmp);
		if (FAILED(hr) || !m_pD2DBmp){ m_pD2DBmp = NULL; return; }
		m_d2dBmpW = bmpW;
		m_d2dBmpH = bmpH;
		// 新 bitmap 还是空像素, 必须强制上传一次. 上面 m_curDirty 可能已被清,
		// 这种情况下 localCopy 也是空, 我们重新拿一份只读副本.
		if (localCopy.empty()){
			std::lock_guard<std::mutex> lk(m_curFrameMutex);
			localCopy = m_curBgra;
		}
	}

	if (!localCopy.empty()){
		D2D1_RECT_U dstRc = D2D1::RectU(0, 0, (UINT32)bmpW, (UINT32)bmpH);
		m_pD2DBmp->CopyFromMemory(&dstRc, localCopy.data(), (UINT32)bmpW * 4);
	}

	// === 5) 上屏: D2D 默认 D2D1_BITMAP_INTERPOLATION_MODE_LINEAR (双线性).
	//        我们 src bitmap 已经是目标尺寸, 这里不再触发实质缩放, 只是 1:1 blit.
	//        D2D 的 alpha 混合会自动做 (PREMULTIPLIED), 透明 PNG / WEBP 直接对.
	D2D1_RECT_F rcDstF = D2D1::RectF(
		(FLOAT)(rcEle.left + rcDst.left),
		(FLOAT)(rcEle.top  + rcDst.top),
		(FLOAT)(rcEle.left + rcDst.right),
		(FLOAT)(rcEle.top  + rcDst.bottom));
	rt->DrawBitmap(m_pD2DBmp, rcDstF, 1.0f,
	               D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
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
	if (IsLoaded() && dstW > 0 && dstH > 0){
		RescaleCurrentFrameIfNeeded(dstW, dstH);
	}

	int srcW = 0, srcH = 0;
	std::vector<uint8_t> localCopy;
	{
		std::lock_guard<std::mutex> lk(m_curFrameMutex);
		srcW = m_curW;
		srcH = m_curH;
		if (srcW > 0 && srcH > 0 && !m_curBgra.empty()){
			localCopy = m_curBgra;
			m_curDirty = false;
		}
	}

	Gdiplus::Graphics graphics(hdc);
	graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
	graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
	graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);

	BYTE bgA = (BYTE)((m_bkColor >> 24) & 0xFF);
	if (bgA != 0){
		BYTE r = (BYTE)((m_bkColor      ) & 0xFF);
		BYTE g = (BYTE)((m_bkColor >>  8) & 0xFF);
		BYTE b = (BYTE)((m_bkColor >> 16) & 0xFF);
		Gdiplus::SolidBrush brush(Gdiplus::Color(bgA, r, g, b));
		graphics.FillRectangle(&brush, rcDstHdc.left, rcDstHdc.top, eleWPhys, eleHPhys);
	}

	if (!localCopy.empty() && srcW > 0 && srcH > 0 && dstW > 0 && dstH > 0){
		Gdiplus::Bitmap bmp(srcW, srcH, srcW * 4, PixelFormat32bppARGB, localCopy.data());
		graphics.DrawImage(&bmp,
		                   rcDstHdc.left + rcDst.left,
		                   rcDstHdc.top  + rcDst.top,
		                   dstW,
		                   dstH);
	}

	m_gdiDibDirty = false;
}

//============================================================================
// GDI 离屏 DIB (CreateCompatibleDC + CreateDIBSection BGRA32 top-down)
//============================================================================
bool CXImageEx::EnsureGdiDib(int w, int h){
	if (w <= 0 || h <= 0) return false;
	if (m_gdiDib && m_gdiMemDC && m_gdiDibW == w && m_gdiDibH == h) return true;
	ReleaseGdiDib();

	HDC hdcRef = ::GetDC(NULL);
	HDC dcMem  = ::CreateCompatibleDC(hdcRef);
	::ReleaseDC(NULL, hdcRef);
	if (!dcMem) return false;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = w;
	bmi.bmiHeader.biHeight      = -h;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* pBits = NULL;
	HBITMAP dib = ::CreateDIBSection(dcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
	if (!dib){ ::DeleteDC(dcMem); return false; }

	HBITMAP oldBmp = (HBITMAP)::SelectObject(dcMem, dib);
	m_gdiMemDC  = dcMem;
	m_gdiDib    = dib;
	m_gdiOldBmp = oldBmp;
	m_gdiDibW   = w;
	m_gdiDibH   = h;
	return true;
}

void CXImageEx::ReleaseGdiDib(){
	if (m_gdiMemDC && m_gdiOldBmp){
		::SelectObject(m_gdiMemDC, m_gdiOldBmp);
	}
	if (m_gdiDib){
		::DeleteObject(m_gdiDib);
		m_gdiDib = NULL;
	}
	if (m_gdiMemDC){
		::DeleteDC(m_gdiMemDC);
		m_gdiMemDC = NULL;
	}
	m_gdiOldBmp = NULL;
	m_gdiDibW = m_gdiDibH = 0;
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
	if (mode < ximage_fit_contain || mode > ximage_fit_original) return;
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

//============================================================================
// 工具: 编码转换 / 错误码翻译
//============================================================================
std::string CXImageEx::WideToUtf8(const std::wstring& w){
	if (w.empty()) return std::string();
	int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
	                              NULL, 0, NULL, NULL);
	if (n <= 0) return std::string();
	std::string s((size_t)n, '\0');
	::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
	                      &s[0], n, NULL, NULL);
	return s;
}

std::wstring CXImageEx::AvErrToWStr(int err){
	char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
	av_strerror(err, buf, sizeof(buf));
	int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, -1, NULL, 0);
	if (n <= 0) return std::wstring();
	std::wstring w((size_t)n, L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, buf, -1, &w[0], n);
	if (!w.empty() && w.back() == L'\0') w.pop_back();
	return w;
}
