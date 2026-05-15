#ifndef  XCGUI_IMAGE_H
#define  XCGUI_IMAGE_H
//@模块名称  炫彩界面库-增强图片元素
//@版本  1.0.0
//@日期  2026-05-15
//@作者  未闻花名
//@QQ    936599025
//@依赖  module_xcgui_class.h
//@模块备注  基于 FFmpeg (libavformat / libavcodec / libswscale) 解码的 *增强版图片元素*.
//          相对 XCGUI 内置 XShapePic / XShapeGif 的优势:
//            (1) 格式支持: 静态 = JPG/PNG/BMP/WEBP/HEIC/AVIF/TIFF/JPEG2000/...
//                          动态 = GIF/APNG/WEBP-anim/AVIF-anim
//            (2) 缩放质量: 用 swscale 在源像素 -> 目标像素一步缩放, 默认 Lanczos
//                          (静态图) / Bilinear (动态图), 远好过 XShapePic 的最近邻
//            (3) 动画时间: 解码后取真实 per-frame duration, 不再固定 33ms
//            (4) GPU 加速: D2D ID2D1Bitmap 上传 BGRA + DrawBitmap, GPU 采样
//            (5) GDI+ 兜底: 离屏 DIB + 单次 BitBlt 上屏, 避免多次 StretchBlt 闪烁
//          继承 CXEle, 不是 Shape - Shape 由 XCGUI 内部画布统一绘制,
//          无法插桩自定义缩放; Element 走 XE_PAINT 完全掌控渲染.
//@模块信息结束

// =================================================================
// 头文件依赖拓扑顺序说明:
//   - @依赖 是 IDE 解析器(智能感知/别名/语法着色)用的, 不会自动注入 #include 给 cl.exe;
//     所以下面要再用真实的 #include 链一次.
//   - d2d1.h 必须先于 module_xcgui.h 完成, 让 d2d1 引入的 POINTF 抢占名字
//     (xcgui 内部用 __IOleControlSite_INTERFACE_DEFINED__ 保护宏跳过重复定义).
//   - module_base.h / module_xcgui.h / module_xcgui_class.h 自身只用 @依赖
//     声明上游, 不带 #include 链, 这里必须按拓扑顺序逐个手动 #include.
//   - FFmpeg 头不属于 XCGUI 模块, 但要先于 module_xcgui.h 完成.
// =================================================================

//@复制文件 @当前模块路径 "ffmpeg\bin\avcodec-58.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\avdevice-58.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\avutil-56.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\swscale-5.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\avformat-58.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\swresample-3.dll"

#include <d2d1.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/error.h>
}

// 标准库.
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"

//@src "module_xcgui_image.cpp"

// =================================================================
// 第三方依赖: FFmpeg 4.4.x ~ 8.x (用 dev 包)
// 链接: avformat / avcodec / avutil / swscale 4 库. avdevice / swresample / postproc
//       本模块不用. 同一个 lib 被多处 #pragma comment 不会重复链接
//       (cl 链接器自动去重).
// =================================================================

//@lib "ffmpeg\lib\avformat.lib"
//@lib "ffmpeg\lib\avcodec.lib"
//@lib "ffmpeg\lib\avutil.lib"
//@lib "ffmpeg\lib\swscale.lib"

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")

#pragma comment(lib, "Gdi32.lib")     // OnPaintGdi: StretchDIBits / FillRect / BitBlt
#pragma comment(lib, "User32.lib")

//@隐藏{
// =================================================================
// 帧节点 (类外声明):
//   - pAvFrame: 解码出来的源帧, 持原始 pix_fmt (yuv420p / pal8 / rgba 等).
//                不在加载阶段统一 swscale 到 BGRA: 那样内存爆炸 (1080p×60帧 ≈ 460MB),
//                而且对动画不友好 - 中间有调色板帧/差分帧需要按帧实时 swscale 到目标尺寸.
//                改为: 加载时只 av_frame_clone 进来, 显示时 sws_scale 一步完成
//                源像素 -> 目标显示尺寸 BGRA (源 pix_fmt -> bgra32, 同步带降色空间).
//   - durationSec: 该帧在动画里的显示时长 (秒). 静态图 N=1 时填 0.
// =================================================================
struct _XImage_Frame{
	AVFrame* pAvFrame    = NULL;
	double   durationSec = 0.0;
};
//@隐藏}

///图片适配模式 (CXImageEx::SetFitMode)
//@别名 图片适配模式
enum ximage_fit_mode_
{
	//@别名 图片适配模式_等比适应
	ximage_fit_contain  = 0,    ///<等比缩放, 长边贴边, 边距留底色 (默认, letter-box)
	//@别名 图片适配模式_等比裁切
	ximage_fit_cover    = 1,    ///<等比缩放, 短边贴边, 长边裁切, 不留底色
	//@别名 图片适配模式_拉伸填满
	ximage_fit_stretch  = 2,    ///<不保持比例, 直接拉伸到元素客户区
	//@别名 图片适配模式_原始尺寸
	ximage_fit_original = 3,    ///<不缩放, 居中显示原图像素 (大于元素时裁切)
};

///缩放算法 (CXImageEx::SetInterpolation). 数值与 swscale 的 SWS_* flag 一致, 直接传入.
//@别名 图片插值算法
enum ximage_interp_
{
	//@别名 图片插值算法_最近邻
	ximage_interp_nearest    = 0x01,    ///<SWS_FAST_BILINEAR 兜底; 实际等效最近邻; 最快, 锯齿明显
	//@别名 图片插值算法_双线性
	ximage_interp_bilinear   = 0x02,    ///<SWS_BILINEAR; 平衡; 适合动画 (60fps 实时缩放)
	//@别名 图片插值算法_双三次
	ximage_interp_bicubic    = 0x04,    ///<SWS_BICUBIC; 偏锐, 适合放大
	//@别名 图片插值算法_Lanczos
	ximage_interp_lanczos    = 0x200,   ///<SWS_LANCZOS; 高质量, 适合静态图 / 大图缩小
	//@别名 图片插值算法_自动
	ximage_interp_auto       = -1,      ///<静态图自动 Lanczos, 动态图自动 Bilinear (默认)
};

///图片元素状态 (CXImageEx::GetState)
//@别名 图片状态
enum ximage_state_
{
	//@别名 图片状态_未加载
	ximage_state_empty     = 0,
	//@别名 图片状态_已加载未播放
	ximage_state_stopped   = 1,
	//@别名 图片状态_播放中
	ximage_state_playing   = 2,
	//@别名 图片状态_已暂停
	ximage_state_paused    = 3,
	//@别名 图片状态_播放完毕
	ximage_state_ended     = 4,         ///<动画播完且未循环
	//@别名 图片状态_错误
	ximage_state_error     = 5,
};

// 前置声明: 给下面回调 typedef 使用.
class CXImageEx;

// =================================================================
// 事件回调原型 (C 风格函数指针 + void* 用户数据, 炫语言侧可直接传函数名)
// =================================================================
//@别名  图片回调_加载完成
typedef void (*XIMAGE_PROC_LOADED)      (CXImageEx* pImg, int srcW, int srcH,
                                          int frameCount, double totalDurationSec, void* pUser);
//@别名  图片回调_加载/解码错误
typedef void (*XIMAGE_PROC_ERROR)       (CXImageEx* pImg, int errCode,
                                          const wchar_t* errMsg, void* pUser);
//@别名  图片回调_帧切换
typedef void (*XIMAGE_PROC_FRAMECHANGE) (CXImageEx* pImg, int frameIndex, void* pUser);
//@别名  图片回调_动画播完
typedef void (*XIMAGE_PROC_ENDED)       (CXImageEx* pImg, void* pUser);

//@分组{ 增强图片元素
//@备注  继承: CXEle, CXWidgetUI, CXObjectUI, CXBase. 基于 FFmpeg 解码 + D2D/GDI 渲染的图片元素.
//       覆盖 XShapePic + XShapeGif 全部场景, 并扩展 HEIC/AVIF/APNG/WEBP-anim 等现代格式.
//@别名  炫彩增强图片类
class CXImageEx : public CXEle
{
public:
	//@隐藏{
	CXImageEx();
	virtual ~CXImageEx();
	//@隐藏}

//@备注 创建图片元素.
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
//@返回 元素句柄
//@别名  创建()
	HELE Create(int x, int y, int cx, int cy, HXCGUI hParent = NULL);

//@备注 创建图片元素 (构造函数版本).
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
	CXImageEx(int x, int y, int cx, int cy, HXCGUI hParent = NULL){ Create(x, y, cx, cy, hParent); }

	// ===== 加载 =====
//@备注 同步加载图片文件. FFmpeg 自动识别格式 (静态 / 动态都支持). 加载完成后:
//        - 若是动态图 (帧数>1): 默认 *自动播放* + *循环*. 用 SetLoop / Stop 改变.
//        - 若是静态图 (帧数=1): 显示该帧, 状态置 stopped.
//      失败时派发 OnImageError 事件; 成功时派发 OnImageLoaded 事件 (UI 线程).
//      会先 Unload() 清掉上次加载的内容.
//@参数 pPath 图片路径 (UTF-16 wchar_t*; 内部转 UTF-8 给 avformat_open_input).
//      支持 file:// / http:// / https:// / ftp:// 等 FFmpeg 协议.
//@返回 派发成功返回 TRUE; pPath 非法返回 FALSE.
//      *不代表加载成功* - 真正加载结果靠 OnImageLoaded / OnImageError 通知.
//@别名  从文件加载()
	BOOL LoadFromFile(const wchar_t* pPath);

//@备注 从内存加载图片. data 在加载期间需保持有效 (函数返回前会被解析).
//@参数 pData 数据指针
//@参数 dataLen 数据长度
//@返回 TRUE / FALSE
//@别名  从内存加载()
	BOOL LoadFromMem(const void* pData, int dataLen);

//@备注 卸载当前图片, 释放所有帧 / swscale 上下文 / 渲染缓冲.
//@别名  卸载()
	void Unload();

//@备注 是否已加载图片 (帧数 ≥ 1).
//@别名  是否已加载()
	BOOL IsLoaded() const;

	// ===== 动画控制 (静态图调这些是 no-op) =====
//@备注 开始播放动画. 静态图无效. paused 时从当前帧继续, stopped/ended 时从第 0 帧开始.
//@别名  播放()
	void Play();

//@备注 暂停动画. 当前帧停留显示. Play 后从同一帧继续.
//@别名  暂停()
	void Pause();

//@备注 停止动画. 跳回第 0 帧并停留, 状态置 stopped.
//@别名  停止()
	void Stop();

//@备注 是否正在播放.
//@别名  是否播放中()
	BOOL IsPlaying() const;

//@备注 设置循环播放. 默认 TRUE.
//@别名  设置循环()
	void SetLoop(BOOL bLoop);

//@备注 获取循环播放状态.
//@别名  取循环()
	BOOL GetLoop() const;

//@备注 跳到指定帧 (0-based). 越界自动 clamp. 触发 OnImageFrameChange.
//@别名  跳转帧()
	void SeekFrame(int frameIndex);

//@备注 当前帧索引 (0-based). 未加载返 -1.
//@别名  取当前帧()
	int  GetCurrentFrame() const;

	// ===== 元数据 =====
//@备注 图片源宽度 (像素). 未加载返 0.
//@别名  取源宽度()
	int  GetSrcWidth() const;

//@备注 图片源高度 (像素). 未加载返 0.
//@别名  取源高度()
	int  GetSrcHeight() const;

//@备注 帧数. 静态图返 1, 动态图返 ≥ 2. 未加载返 0.
//@别名  取帧数()
	int  GetFrameCount() const;

//@备注 是否动画 (帧数 > 1).
//@别名  是否动画()
	BOOL IsAnimated() const;

//@备注 动画总时长 (秒). 静态图返 0.
//@别名  取总时长()
	double GetTotalDuration() const;

//@备注 指定帧时长 (秒). 越界返 0.
//@别名  取帧时长()
	double GetFrameDuration(int frameIndex) const;

//@备注 当前状态 (ximage_state_*).
//@别名  取状态()
	int  GetState() const;

	// ===== 显示 =====
//@备注 设置适配模式 (ximage_fit_*). 默认 contain.
//@别名  设置适配模式()
	void SetFitMode(int mode);

//@备注 获取适配模式.
//@别名  取适配模式()
	int  GetFitMode() const;

//@备注 设置缩放算法 (ximage_interp_*). 默认 auto (静态图 Lanczos / 动态图 Bilinear).
//      切换后下一次重绘 (SetSize / 帧切换) 才生效.
//@别名  设置插值算法()
	void SetInterpolation(int interp);

//@备注 获取当前生效的插值算法.
//@别名  取插值算法()
	int  GetInterpolation() const;

//@备注 设置背景色 (元素未被图片占满时填充用). 默认透明 (RGBA(0,0,0,0)).
//      支持 alpha; XCGUI RGBA 宏布局 0xAABBGGRR.
//@别名  设置背景色()
	void SetBkColor(COLORREF color);

//@备注 获取背景色.
//@别名  取背景色()
	COLORREF GetBkColor() const;

	// ===== 事件 =====
//@备注 注册"加载完成"回调. UI 线程触发. cb=NULL 取消.
//@参数 cb 回调函数指针
//@参数 pUser 用户数据 (回调时原样回传)
//@别名  设置回调_加载完成()
	void SetOnLoaded(XIMAGE_PROC_LOADED cb, void* pUser);

//@备注 注册"加载/解码错误"回调.
//@别名  设置回调_错误()
	void SetOnError(XIMAGE_PROC_ERROR cb, void* pUser);

//@备注 注册"帧切换"回调. 仅动画图触发. 高频, 回调里别做重活.
//@别名  设置回调_帧切换()
	void SetOnFrameChange(XIMAGE_PROC_FRAMECHANGE cb, void* pUser);

//@备注 注册"动画播完"回调 (loop=FALSE 时). loop=TRUE 时永不触发.
//@别名  设置回调_播完()
	void SetOnEnded(XIMAGE_PROC_ENDED cb, void* pUser);

	//@隐藏{
private:
	// ===== 元素 / DPI =====
	HELE  m_hEle      = NULL;
	float m_dpiScale  = 1.0f;

	// ===== 源帧数据 (加载阶段填充, 之后只读) =====
	// m_frames[i].pAvFrame: 解码出的源帧, 持原始 pix_fmt. av_frame_free 配对释放.
	// m_frames[i].durationSec: 该帧时长. GIF/APNG/WEBP-anim 在解码时通过
	//   stream->time_base + pkt_duration 算出来; 静态图填 0.
	std::vector<_XImage_Frame> m_frames;
	int    m_srcW = 0, m_srcH = 0;
	AVPixelFormat m_srcPixFmt = AV_PIX_FMT_NONE;
	double m_totalDurationSec = 0.0;
	bool   m_animated         = false;

	// ===== 当前显示帧 (UI 线程读, 加载/动画线程写; 实际只在 UI 线程, 锁是给 GetCurrentFrame
	//                  这种公开 const 接口保险用) =====
	std::atomic<int> m_curIdx{0};
	// 动画时序: 进入当前帧的 tick. 16ms 定时器里检查 elapsed >= m_frames[m_curIdx].durationSec
	// 就推进. 用 GetTickCount64 防 49.7 天回绕 (虽然实际场景不会, 但 unsigned 32 减法
	// 在某些 corner case 下编译器警告).
	ULONGLONG m_curFrameStartTick = 0;

	// ===== 缩放后的显示缓冲 =====
	// 把 m_frames[curIdx].pAvFrame 用 sws_scale 一步缩到 *目标显示尺寸 BGRA*, 缓存起来.
	// OnPaint 直接拿这个 BGRA 上传 D2D Bitmap / GDI DIB - 1:1 不再二次缩放, 质量由 swscale
	// 保证 (Lanczos / Bicubic / Bilinear 用户配置).
	std::mutex           m_curFrameMutex;       // 给 OnPaint 拷贝 m_curBgra 到 localCopy 用
	std::vector<uint8_t> m_curBgra;             // size = m_curW * m_curH * 4
	int  m_curW = 0, m_curH = 0;
	bool m_curDirty = false;                    // OnPaint 上传后清; 帧切换/resize/重缩放后置位
	int  m_scaledForFrame   = -1;               // m_curBgra 当前是哪一帧缩出来的
	int  m_scaledForW       = 0;                // m_curBgra 当前的目标宽 (= ComputeDestRect.w)
	int  m_scaledForH       = 0;                // 同上, 高
	int  m_scaledForInterp  = -1;               // 同上, 插值算法

	// ===== swscale =====
	SwsContext* m_pSws = NULL;
	// 上次 sws_getCachedContext 的输入参数, 用来判断是否需要 freeContext.
	// (sws_getCachedContext 内部也会判断, 这里冗余一份给 ResetSwsIfNeeded 早退路径用).
	int           m_swsLastSrcW = 0, m_swsLastSrcH = 0;
	AVPixelFormat m_swsLastSrcFmt = AV_PIX_FMT_NONE;
	int           m_swsLastDstW = 0, m_swsLastDstH = 0;
	int           m_swsLastFlags = 0;

	// ===== 渲染资源 =====
	// D2D 路径
	ID2D1Bitmap*       m_pD2DBmp   = NULL;
	ID2D1RenderTarget* m_pLastRT   = NULL;
	int                m_d2dBmpW   = 0;
	int                m_d2dBmpH   = 0;
	// GDI+ 离屏 DIB 路径
	HDC     m_gdiMemDC    = NULL;
	HBITMAP m_gdiDib      = NULL;
	HBITMAP m_gdiOldBmp   = NULL;
	int     m_gdiDibW     = 0;
	int     m_gdiDibH     = 0;
	bool    m_gdiDibDirty = true;

	// ===== 状态机 =====
	std::atomic<int>  m_state{ximage_state_empty};
	std::atomic<bool> m_loop{true};

	// ===== 显示选项 =====
	int      m_fitMode  = ximage_fit_contain;
	int      m_interp   = ximage_interp_auto;       // auto = 动静图各自映射
	COLORREF m_bkColor  = RGBA(0, 0, 0, 0);          // 默认透明; 与 XCGUI BkInfo 协同

	// ===== 定时器 =====
	// 16ms tick 同时驱动:
	//   (1) 动画帧推进 (检查 elapsed >= m_frames[curIdx].durationSec)
	//   (2) UI 线程派发 m_pendingLoaded / Error / Ended 回调
	static const UINT kTimerId_Tick    = 0xCB1E0001;
	static const UINT kTimerInterval_Ms = 16;

	// ===== 用户事件回调 =====
	XIMAGE_PROC_LOADED      m_cbLoaded      = NULL;   void* m_userLoaded      = NULL;
	XIMAGE_PROC_ERROR       m_cbError       = NULL;   void* m_userError       = NULL;
	XIMAGE_PROC_FRAMECHANGE m_cbFrameChange = NULL;   void* m_userFrameChange = NULL;
	XIMAGE_PROC_ENDED       m_cbEnded       = NULL;   void* m_userEnded       = NULL;

	// 同步加载完成后 PostX 模式. 保证回调永远在 UI 线程触发.
	std::atomic<bool> m_pendingLoaded{false};
	std::atomic<bool> m_pendingError{false};
	std::atomic<bool> m_pendingEnded{false};
	int               m_pendingErrCode = 0;
	std::wstring      m_pendingErrMsg;
	std::mutex        m_pendingErrMutex;

	// =================================================================
	// 内部辅助 (declare here, define in cpp)
	// =================================================================
	void InstallEvents();
	int  OnPaintImpl   (HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int  OnSizeImpl    (HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled);
	int  OnTimerImpl   (HELE hEle, UINT nTimerId, BOOL* pbHandled);
	int  OnDestroyImpl (HELE hEle, BOOL* pbHandled);

	// 渲染分流: D2D 主路径 / GDI+ 兜底.
	void OnPaintD2D(ID2D1RenderTarget* rt, HDRAW hDraw);
	void OnPaintGdi(HDC hdc, HDRAW hDraw);
	bool EnsureGdiDib(int w, int h);
	void ReleaseGdiDib();

	// DPI / BkInfo / 重绘.
	void RefreshDpiScale();
	void RebuildBkInfo();
	void ComputeDestRect(int eleW, int eleH, RECT* pDst) const;
	void RedrawSelf();

	// 加载主流程: 内部把 LoadFromFile / LoadFromMem 都收敛到 LoadInternal.
	// pCustomIO != NULL 时走 avio_alloc_context (内存源); pPath != NULL 时走文件源.
	BOOL LoadInternal(const wchar_t* pPath, const void* pData, int dataLen);
	void ClearFrames();

	// 帧切换 / 缩放. 由 OnTimerImpl 推进, 由 SeekFrame 直接跳, 由 OnSizeImpl 强制重缩.
	void AdvanceFrameIfDue();          // 动画 tick: elapsed >= duration -> 推进
	void RescaleCurrentFrameIfNeeded(int dstW, int dstH);   // 把 m_frames[curIdx] 缩到 (dstW,dstH) BGRA
	int  EffectiveInterp() const;      // ximage_interp_auto -> 实际值

	// 错误派发 (供 LoadInternal 调).
	void PostError(int code, const wchar_t* msg);
	void PostError(int code, const std::wstring& msg);
	// UI 线程 timer 里轮询 pending flag 并触发用户回调.
	void DispatchPendingCallbacks();

	// UTF-16 -> UTF-8 (FFmpeg avformat_open_input 需要 UTF-8 路径).
	static std::string  WideToUtf8(const std::wstring& w);
	// AVERROR -> 中文友好串.
	static std::wstring AvErrToWStr(int err);
	//@隐藏}
};
//@分组}

#endif // XCGUI_IMAGE_H
