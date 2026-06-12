#ifndef XCGUI_MEDIA_INTERNAL_H
#define XCGUI_MEDIA_INTERNAL_H
//@隐藏{

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libavutil/error.h>
}

#ifndef SafeRelease
#define SafeRelease(p) do { if (p) { (p)->Release(); (p) = NULL; } } while (0)
#endif

// =================================================================
// 适配模式 (image / video 数值一致, 内部统一)
// =================================================================
enum xmedia_fit_mode_
{
	xmedia_fit_contain  = 0,
	xmedia_fit_cover    = 1,
	xmedia_fit_stretch  = 2,
	xmedia_fit_original = 3,
};

// =================================================================
// 图片帧节点 (CXImageEx 加载期持有)
// =================================================================
struct _XImage_Frame{
	AVFrame* pAvFrame    = NULL;
	double   durationSec = 0.0;
};

// =================================================================
// L0 FFmpeg: 内存 AVIO
// =================================================================
struct _XMedia_MemBuf{
	const uint8_t* data = NULL;
	int            len  = 0;
	int            pos  = 0;
};

int      _XMedia_MemRead(void* opaque, uint8_t* buf, int bufSize);
int64_t  _XMedia_MemSeek(void* opaque, int64_t offset, int whence);
AVIOContext* _XMedia_FF_CreateMemAvIO(const void* pData, int dataLen, _XMedia_MemBuf** ppMbOut);

// =================================================================
// L0 FFmpeg: 网络 / 打开 / 中断 / 错误
// =================================================================
void _XMedia_FF_EnsureNetworkInit();

enum _XMedia_OpenProfile_
{
	_xmedia_open_default     = 0,
	_xmedia_open_image_local = 1,
	_xmedia_open_video_local = 2,
	_xmedia_open_video_net   = 3,
	_xmedia_open_thumbnail   = 4,
};

int _XMedia_FF_InterruptCallback(void* opaque);

int _XMedia_FF_OpenWithOptions(AVFormatContext** ppFmt, const char* url,
                               _XMedia_OpenProfile_ profile,
                               std::atomic<bool>* pAbort);

std::string  _XMedia_FF_WideToUtf8(const std::wstring& w);
std::wstring _XMedia_FF_ErrToWide(int err);

// =================================================================
// L0 FFmpeg: swscale -> BGRA (colorspace 权威实现, yuvj 处理)
// =================================================================
struct _XMedia_FF_SwsCache{
	SwsContext*   pSws         = NULL;
	int           lastSrcW     = 0;
	int           lastSrcH     = 0;
	AVPixelFormat lastSrcFmt   = AV_PIX_FMT_NONE;
	int           lastDstW     = 0;
	int           lastDstH     = 0;
	int           lastFlags    = 0;
};

bool _XMedia_FF_SwsAvFrameToBgra(_XMedia_FF_SwsCache* pCache,
                                AVFrame* src, int dstW, int dstH, int swsFlags,
                                std::vector<uint8_t>* outBgra, int* outW, int* outH);

bool _XMedia_FF_GrabFirstVideoFrameBgra(AVFormatContext* pFmt, int videoStreamIdx,
                                         double coverTimeSec,
                                         std::vector<uint8_t>* outBgra, int* outW, int* outH);

// =================================================================
// L1 渲染: 目标矩形 / GDI 离屏 DIB
// =================================================================
void _XMedia_Render_ComputeDestRect(int fitMode, int srcW, int srcH,
                                     int eleW, int eleH, RECT* pDst);

struct _XMedia_GdiDib{
	HDC     memDC    = NULL;
	HBITMAP dib      = NULL;
	HBITMAP oldBmp   = NULL;
	int     dibW     = 0;
	int     dibH     = 0;
};

struct ID2D1Bitmap;
struct ID2D1RenderTarget;

struct _XMedia_Render_D2DBmpCache{
	ID2D1Bitmap*       pBmp    = NULL;
	ID2D1RenderTarget* pLastRT = NULL;
	int                bmpW    = 0;
	int                bmpH    = 0;
	const void*        uploadedBgraPtr = NULL;  // 上次 CopyFromMemory 的像素指针; resize 仅改 dstRect 时跳过上传
};

// OnPaint 快照: shared_ptr 引用计数代替 vector 全量拷贝, 绘制期间持有 pixels 保活.
struct _XMedia_FramePaintSnap{
	std::shared_ptr<const std::vector<uint8_t>> pixels;
	int  w     = 0;
	int  h     = 0;
	int  pitch = 0;
	bool contentDirty = false;
};

inline _XMedia_FramePaintSnap _XMedia_FrameSnapForPaint(
	std::mutex& mu,
	std::shared_ptr<const std::vector<uint8_t>>& pixels,
	int& w, int& h, int& pitch, bool& dirty,
	bool pitchIsStored = true)
{
	std::lock_guard<std::mutex> lk(mu);
	_XMedia_FramePaintSnap s;
	s.w = w;
	s.h = h;
	s.pitch = pitchIsStored ? pitch : ((w > 0) ? w * 4 : 0);
	if (pixels && !pixels->empty()){
		s.pixels = pixels;
		s.contentDirty = dirty;
		if (dirty) dirty = false;
	}
	return s;
}

enum _XMedia_Render_D2dAlpha_{
	_xmedia_d2d_alpha_premul = 0,
	_xmedia_d2d_alpha_ignore = 1,
};

struct _XMedia_Render_PaintD2DParams{
	ID2D1RenderTarget*     rt;
	const RECT*            rcEle;
	COLORREF               bkColor;
	bool                   bkRespectAlpha;
	int                    fitMode;
	const RECT*            rcDstLocal;
	int                    srcW;
	int                    srcH;
	int                    srcPitch;
	const uint8_t*         bgra;
	bool*                  pDirty;
	_XMedia_Render_D2dAlpha_ alphaMode;
	bool                   stretchFromSrc;
	_XMedia_Render_D2DBmpCache* cache;
};

struct _XMedia_Render_PaintGdiParams{
	HDC              hdcDest;
	const RECT*      rcDstHdc;
	int              eleWPhys;
	int              eleHPhys;
	COLORREF         bkColor;
	const RECT*      rcDstLocal;
	int              srcW;
	int              srcH;
	const uint8_t*   bgra;
	bool             needRedraw;
	bool*            pDibDirty;
	_XMedia_GdiDib*  pDib;
};

bool _XMedia_Render_EnsureGdiDib(_XMedia_GdiDib* pDib, int w, int h);
void _XMedia_Render_ReleaseGdiDib(_XMedia_GdiDib* pDib);

bool _XMedia_Render_PaintD2D_Bgra(const _XMedia_Render_PaintD2DParams* p);
bool _XMedia_Render_PaintGdi_Bgra(const _XMedia_Render_PaintGdiParams* p);

void _XMedia_Render_FillGdiDibBk(_XMedia_GdiDib* pDib, int eleW, int eleH, COLORREF bkColor);
void _XMedia_Render_BlitGdiDibStretch(HDC hdcDest, const _XMedia_GdiDib* pDib,
                                       int destX, int destY, int destW, int destH);

void _XMedia_Render_StretchBgraToDib(HDC dcMem, const uint8_t* bgra, int srcW, int srcH,
                                      int dstX, int dstY, int dstW, int dstH);

// =================================================================
// L2 窗口几何: size-move 期间 defer 重缩放 / layout (ref-count 按 HWND)
// =================================================================
void _XMedia_SizeMoveGuard_Attach(void* hEle, void (*onExit)(void*), void* user);
void _XMedia_SizeMoveGuard_Detach(void* hEle, void* user);
bool _XMedia_SizeMoveGuard_IsActive(void* hEle);

//@隐藏}
#endif // XCGUI_MEDIA_INTERNAL_H
