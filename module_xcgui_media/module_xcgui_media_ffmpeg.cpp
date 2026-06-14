// module_xcgui_media_ffmpeg.cpp — L0 共享 FFmpeg 内核
#include "module_xcgui_media.h"

#include <algorithm>
#include <cstring>

namespace {

constexpr int kMemIoBufSize = 65536;

static AVPixelFormat MapJpegRangeFmt(AVPixelFormat srcFmt, bool* pIsJpegRange){
	*pIsJpegRange = false;
	switch (srcFmt){
	case AV_PIX_FMT_YUVJ420P: *pIsJpegRange = true; return AV_PIX_FMT_YUV420P;
	case AV_PIX_FMT_YUVJ422P: *pIsJpegRange = true; return AV_PIX_FMT_YUV422P;
	case AV_PIX_FMT_YUVJ444P: *pIsJpegRange = true; return AV_PIX_FMT_YUV444P;
	case AV_PIX_FMT_YUVJ440P: *pIsJpegRange = true; return AV_PIX_FMT_YUV440P;
	case AV_PIX_FMT_YUVJ411P: *pIsJpegRange = true; return AV_PIX_FMT_YUV411P;
	default: return srcFmt;
	}
}

static bool IsNetworkUrl(const char* url){
	if (!url) return false;
	return (strncmp(url, "http://", 7) == 0)
	    || (strncmp(url, "https://", 8) == 0)
	    || (strncmp(url, "rtsp://", 7) == 0)
	    || (strncmp(url, "rtmp://", 7) == 0)
	    || (strncmp(url, "udp://", 6) == 0);
}

} // anonymous namespace

//============================================================================
// L0: MemAvIO
//============================================================================
int _XMedia_MemRead(void* opaque, uint8_t* buf, int bufSize){
	_XMedia_MemBuf* mb = (_XMedia_MemBuf*)opaque;
	int remain = mb->len - mb->pos;
	if (remain <= 0) return AVERROR_EOF;
	int n = (bufSize < remain) ? bufSize : remain;
	memcpy(buf, mb->data + mb->pos, (size_t)n);
	mb->pos += n;
	return n;
}

int64_t _XMedia_MemSeek(void* opaque, int64_t offset, int whence){
	_XMedia_MemBuf* mb = (_XMedia_MemBuf*)opaque;
	if (whence == AVSEEK_SIZE) return mb->len;
	int64_t target = 0;
	switch (whence){
	case SEEK_SET: target = offset; break;
	case SEEK_CUR: target = (int64_t)mb->pos + offset; break;
	case SEEK_END: target = (int64_t)mb->len + offset; break;
	default:       return -1;
	}
	if (target < 0 || target > mb->len) return -1;
	mb->pos = (int)target;
	return target;
}

AVIOContext* _XMedia_FF_CreateMemAvIO(const void* pData, int dataLen, _XMedia_MemBuf** ppMbOut){
	if (!pData || dataLen <= 0 || !ppMbOut) return NULL;
	*ppMbOut = NULL;
	unsigned char* pIoBuf = (unsigned char*)av_malloc(kMemIoBufSize);
	if (!pIoBuf) return NULL;
	_XMedia_MemBuf* pMb = new _XMedia_MemBuf();
	pMb->data = (const uint8_t*)pData;
	pMb->len  = dataLen;
	pMb->pos  = 0;
	AVIOContext* pIO = avio_alloc_context(pIoBuf, kMemIoBufSize, 0, pMb,
	                                      &_XMedia_MemRead, NULL, &_XMedia_MemSeek);
	if (!pIO){
		av_free(pIoBuf);
		delete pMb;
		return NULL;
	}
	*ppMbOut = pMb;
	return pIO;
}

//============================================================================
// L0: 网络 init / 字符串 / 中断
//============================================================================
void _XMedia_FF_EnsureNetworkInit(){
	struct OnceInit{
		OnceInit(){ avformat_network_init(); }
		~OnceInit(){ avformat_network_deinit(); }
	};
	static OnceInit s_once;
	(void)s_once;
}

int _XMedia_FF_InterruptCallback(void* opaque){
	std::atomic<bool>* pAbort = (std::atomic<bool>*)opaque;
	if (pAbort && pAbort->load(std::memory_order_acquire)){
		return 1;
	}
	return 0;
}

int _XMedia_FF_InterruptCallbackEx(void* opaque){
	_XMedia_FF_InterruptCtx* pCtx = (_XMedia_FF_InterruptCtx*)opaque;
	if (!pCtx) return 0;
	if (pCtx->pUserAbort && pCtx->pUserAbort->load(std::memory_order_acquire)){
		return 1;
	}
	if (pCtx->deadlineMs != 0){
		unsigned long long now = ::GetTickCount64();
		if (now >= pCtx->deadlineMs){
			return 1;
		}
	}
	return 0;
}

std::string _XMedia_FF_WideToUtf8(const std::wstring& w){
	if (w.empty()) return std::string();
	// UI / worker: 短路径走 XC_wtoutf8 共享缓冲, 立即 copy 到 std::string (XC_* 非线程安全).
	if ((int)w.size() < TEXT_BUFFER_SIZE){
		const char* p = XC_wtoutf8(w.c_str());
		if (p && *p) return std::string(p);
	}
	// 超长路径 (>10240 wchar) 保留堆分配.
	int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
	                              NULL, 0, NULL, NULL);
	if (n <= 0) return std::string();
	std::string s;
	s.resize((size_t)n);
	::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
	                      &s[0], n, NULL, NULL);
	return s;
}

std::wstring _XMedia_FF_ErrToWide(int err){
	char buf[AV_ERROR_MAX_STRING_SIZE];
	::memset(buf, 0, sizeof(buf));
	av_strerror(err, buf, sizeof(buf) - 1);
	const wchar_t* ws = XC_utf8tow(buf);
	if (ws && *ws) return std::wstring(ws);
	return std::wstring();
}

int _XMedia_FF_OpenWithOptions(AVFormatContext** ppFmt, const char* url,
                                _XMedia_OpenProfile_ profile,
                                std::atomic<bool>* pAbort,
                                _XMedia_FF_InterruptCtx* pIntrCtx){
	if (!ppFmt || !*ppFmt || !url) return AVERROR(EINVAL);
	auto bindIntr = [&](AVFormatContext* pCtx){
		if (!pCtx) return;
		if (pAbort){
			pCtx->interrupt_callback.callback = &_XMedia_FF_InterruptCallback;
			pCtx->interrupt_callback.opaque   = (void*)pAbort;
		} else if (pIntrCtx){
			pCtx->interrupt_callback.callback = &_XMedia_FF_InterruptCallbackEx;
			pCtx->interrupt_callback.opaque   = (void*)pIntrCtx;
		}
	};
	bindIntr(*ppFmt);
	AVDictionary* opts = NULL;
	const char* probesize = "5M";
	const char* analyzeduration = "3M";
	switch (profile){
	case _xmedia_open_image_local:
		probesize = "256K";
		analyzeduration = "500000";
		break;
	case _xmedia_open_thumbnail:
		probesize = "128K";
		analyzeduration = "250000";
		break;
	case _xmedia_open_video_net:
		probesize = "5M";
		analyzeduration = "5M";
		break;
	case _xmedia_open_video_local:
	default:
		break;
	}
	av_dict_set(&opts, "probesize", probesize, 0);
	av_dict_set(&opts, "analyzeduration", analyzeduration, 0);
	av_dict_set(&opts, "fflags", "+genpts", 0);
	if (profile == _xmedia_open_video_local && !IsNetworkUrl(url)){
		av_dict_set(&opts, "fflags", "+genpts+fastseek", 0);
	}
	int ret = avformat_open_input(ppFmt, url, NULL, &opts);
	av_dict_free(&opts);
	if (ret >= 0) return ret;

	// 失败回退: 默认 open
	avformat_free_context(*ppFmt);
	*ppFmt = avformat_alloc_context();
	if (!*ppFmt) return AVERROR(ENOMEM);
	bindIntr(*ppFmt);
	return avformat_open_input(ppFmt, url, NULL, NULL);
}

//============================================================================
// L0: SwsToBgra (yuvj + cached context)
//============================================================================
bool _XMedia_FF_SwsAvFrameToBgra(_XMedia_FF_SwsCache* pCache,
                                  AVFrame* src, int dstW, int dstH, int swsFlags,
                                  std::vector<uint8_t>* outBgra, int* outW, int* outH){
	if (!pCache || !src || !outBgra || dstW <= 0 || dstH <= 0) return false;
	if (src->width <= 0 || src->height <= 0) return false;

	bool isJpegRange = false;
	AVPixelFormat srcFmt = MapJpegRangeFmt((AVPixelFormat)src->format, &isJpegRange);

	bool needNew = (pCache->pSws == NULL)
	            || (pCache->lastSrcW   != src->width)
	            || (pCache->lastSrcH   != src->height)
	            || (pCache->lastSrcFmt != srcFmt)
	            || (pCache->lastDstW   != dstW)
	            || (pCache->lastDstH   != dstH)
	            || (pCache->lastFlags  != swsFlags);
	if (needNew){
		pCache->pSws = sws_getCachedContext(pCache->pSws,
		                                     src->width, src->height, srcFmt,
		                                     dstW, dstH, AV_PIX_FMT_BGRA,
		                                     swsFlags, NULL, NULL, NULL);
		if (!pCache->pSws) return false;
		if (isJpegRange){
			const int* coeff = sws_getCoefficients(SWS_CS_DEFAULT);
			sws_setColorspaceDetails(pCache->pSws,
			                          coeff, 1, coeff, 0, 0, 1 << 16, 1 << 16);
		}
		pCache->lastSrcW   = src->width;
		pCache->lastSrcH   = src->height;
		pCache->lastSrcFmt = srcFmt;
		pCache->lastDstW   = dstW;
		pCache->lastDstH   = dstH;
		pCache->lastFlags  = swsFlags;
	}

	std::vector<uint8_t> tmp((size_t)dstW * dstH * 4);
	uint8_t* dstData[1]     = { tmp.data() };
	int      dstLinesize[1] = { dstW * 4 };
	int rh = sws_scale(pCache->pSws,
	                    (const uint8_t* const*)src->data, src->linesize,
	                    0, src->height, dstData, dstLinesize);
	if (rh <= 0) return false;

	*outBgra = std::move(tmp);
	if (outW) *outW = dstW;
	if (outH) *outH = dstH;
	return true;
}

bool _XMedia_FF_GrabFirstVideoFrameBgra(AVFormatContext* pFmt, int videoStreamIdx,
                                         double coverTimeSec,
                                         std::vector<uint8_t>* outBgra, int* outW, int* outH){
	if (!pFmt || videoStreamIdx < 0 || !outBgra) return false;
	AVStream* st = pFmt->streams[videoStreamIdx];
	const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
	if (!dec) return false;
	AVCodecContext* pCtx = avcodec_alloc_context3(dec);
	if (!pCtx) return false;

	BOOL ok = FALSE;
	AVFrame* frame = NULL;
	AVPacket* pkt = NULL;
	_XMedia_FF_SwsCache swsCache;
	do{
		if (avcodec_parameters_to_context(pCtx, st->codecpar) < 0) break;
		pCtx->thread_count = 1;
		if (avcodec_open2(pCtx, dec, NULL) < 0) break;

		if (coverTimeSec > 0.0 && pFmt->duration > 0){
			double durSec = (double)pFmt->duration / (double)AV_TIME_BASE;
			double t = coverTimeSec;
			if (t > durSec - 0.05) t = (durSec > 0.2) ? (durSec * 0.1) : 0.0;
			int64_t ts = (int64_t)(t * AV_TIME_BASE);
			av_seek_frame(pFmt, -1, ts, AVSEEK_FLAG_BACKWARD);
			avcodec_flush_buffers(pCtx);
		}

		frame = av_frame_alloc();
		pkt = av_packet_alloc();
		if (!frame || !pkt) break;

		int gotFrame = 0;
		while (!gotFrame){
			int rr = av_read_frame(pFmt, pkt);
			if (rr < 0){
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

		ok = _XMedia_FF_SwsAvFrameToBgra(&swsCache, frame,
		                                  frame->width, frame->height, SWS_BILINEAR,
		                                  outBgra, outW, outH) ? TRUE : FALSE;
	} while (0);

	if (swsCache.pSws) sws_freeContext(swsCache.pSws);
	if (frame) av_frame_free(&frame);
	if (pkt)   av_packet_free(&pkt);
	if (pCtx)  avcodec_free_context(&pCtx);
	return ok;
}
