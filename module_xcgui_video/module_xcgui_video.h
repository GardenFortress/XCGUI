#ifndef  XCGUI_VIDEO_H
#define  XCGUI_VIDEO_H
//@模块名称  炫彩界面库-视频播放器
//@版本  1.0.0
//@日期  2026-05-14
//@作者  未闻花名
//@QQ    936599025
//@依赖  module_xcgui_class.h
//@模块备注  基于 FFmpeg (libavformat / libavcodec / libswscale / libswresample) 解码,
//          + WASAPI 音频输出, 渲染走 XCGUI 元素画布: D2D 主路径 + GDI 降级路径
//          (StretchDIBits, 离屏 DIB 合成 + 一次 BitBlt 上屏),
//          兼容 Win7 SP1+ 无 GPU 虚拟机. 多线程架构 (demux / 视频解码 / 音频解码 /
//          音频渲染 4 线程), 以音频为主时钟做 A/V 同步, 在 XE_XC_TIMER 里按时钟驱动
//          视频帧上屏. 自动跟随 XCGUI DPI 缩放系统.
//@模块信息结束

// =================================================================
// 头文件依赖拓扑顺序说明:
//   - @依赖 是 IDE 解析器(智能感知/别名/语法着色)用的, 不会自动注入 #include 给 cl.exe;
//     所以下面要再用真实的 #include 链一次.
//   - d2d1.h 必须先于 module_xcgui.h 完成, 让 d2d1 引入的 POINTF 抢占名字
//     (xcgui 内部用 __IOleControlSite_INTERFACE_DEFINED__ 保护宏跳过重复定义).
//   - module_base.h / module_xcgui.h / module_xcgui_class.h 自身只用 @依赖
//     声明上游, 不带 #include 链, 这里必须按拓扑顺序逐个手动 #include,
//     才能让 HELE / HDRAW / RGBA() 等符号在类声明处可见.
//   - FFmpeg / WASAPI 头不属于 XCGUI 模块, 但要先于 module_xcgui.h 完成,
//     因为 d2d1 / FFmpeg 都对 POINTF, AVRational 等公共名字有约束, 顺序错了会重定义.
// =================================================================

//@复制文件 @当前模块路径 "ffmpeg\bin\avcodec-58.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\avdevice-58.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\avutil-56.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\swscale-5.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\avformat-58.dll"
//@复制文件 @当前模块路径 "ffmpeg\bin\swresample-3.dll"

#include <d2d1.h>

// FFmpeg 头. 用户需要把 FFmpeg dev 包 (4.x / 5.x / 6.x / 7.x / 8.x 都行) include 路径加到工程,
// 并把对应导入库放到链接器输入 (见文件末尾 #pragma comment).
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>   // AVChannelLayout (FFmpeg 5.1+) / 旧版的 av_get_default_channel_layout
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/error.h>
#include <libavutil/hwcontext.h>        // 硬件解码: AV_HWDEVICE_TYPE_*, av_hwdevice_ctx_create, av_hwframe_transfer_data
}

// WASAPI (Windows Vista+ 自带, 无第三方依赖).
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <avrt.h>            // AvSetMmThreadCharacteristics: "Pro Audio" 调高线程优先级

// 标准库.
#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"

//@src "module_xcgui_video.cpp"

// =================================================================
// 第三方依赖: FFmpeg 8.x (gyan.dev / BtbN 预编译 shared dev 包)
// =================================================================
// 目录约定: 解压后 ffmpeg\ 整个目录放到本模块同级:
//     <本模块目录>\ffmpeg\bin\     (avcodec-62.dll ... swresample-6.dll)
//     <本模块目录>\ffmpeg\include\ (libavcodec\ libavformat\ ...)
//     <本模块目录>\ffmpeg\lib\     (avcodec.lib ... swresample.lib)
//
// 链接: 用 #pragma comment(lib, ...) 直接告诉链接器需要哪些 lib. 链接器从默认
// lib 搜索路径里找 (Windows SDK + VS + IDE 配置的附加库目录). FFmpeg 的 lib
// 不在系统路径里, 使用方需要在 IDE / 项目 / cl 命令行里把 ffmpeg\lib 加到
// 附加库目录 (或 /LIBPATH). 64 位与 32 位 lib 文件同名 (avformat.lib 等),
// 由项目平台配置决定挑哪一个. DLL 文件需运行时与 exe 同目录, 使用方负责
// 把 ffmpeg\bin\*.dll 复制到输出目录 (典型做法是在构建脚本里 copy).

//@lib "ffmpeg\lib\avformat.lib"
//@lib "ffmpeg\lib\avcodec.lib"
//@lib "ffmpeg\lib\avutil.lib"
//@lib "ffmpeg\lib\swscale.lib"
//@lib "ffmpeg\lib\swresample.lib"

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")

// WASAPI / GDI / Multimedia 系统 lib (Windows SDK 默认搜索路径下, 链接器能直接找到):
//   - Ole32.lib : CoCreateInstance / CoInitializeEx
//   - Avrt.lib  : AvSetMmThreadCharacteristics
//   - Gdi32.lib : OnPaintGdi 用到 StretchDIBits / SaveDC / RestoreDC /
//                 IntersectClipRect / SetStretchBltMode
//   - User32.lib: MessageBoxW 等 (XCGUI 自身可能间接需要, 显式列出更稳).
// 注意: IMMDeviceEnumerator / IAudioClient / IAudioRenderClient 的 IID 通过 __uuidof()
// 由 mmdeviceapi.h / audioclient.h 头里的 __declspec(uuid()) 注解提供, 无需额外 lib;
// CLSID_MMDeviceEnumerator 同上由头里 __declspec 提供. 不链 Mmdevapi.lib (该 lib
// 在不同 Windows SDK 版本里位置 / 大小写不一致, 容易链接出错).
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Avrt.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

//@隐藏{
// =================================================================
// 队列结构 (在类外是为了避开部分预编译头解析器对类内嵌 struct + std::deque<内嵌类型>
// 的查找顺序问题).
// =================================================================

// 单包队列条目: 持 AVPacket 所有权 (av_packet_alloc / av_packet_free 配对).
// 不直接存 AVPacket 值类型: AVPacket 内部有 buf 引用计数, 浅拷贝有坑;
// 用 av_packet_move_ref 在 push 时把所有权转移进队列, pop 时再转移出来.
//
// 三种角色:
//   (1) 普通包: pkt 非空, flush/eof 都 false.
//   (2) seek flush 标记: pkt = NULL, flush=true, eof=false.
//       解码线程见到要 avcodec_flush_buffers + 把已积累的 frame 丢掉 (其实 demux 线程
//       已经先 Clear 过 frameQ, 这里再 flush 一次以稳妥.).
//   (3) EOF 标记: pkt = NULL, flush=true, eof=true.
//       解码线程见到要 drain decoder (送 NULL 给 avcodec_send_packet 后取出残余帧),
//       然后向 frameQ 推 eof 哨兵帧, 让上层渲染端知道流到底了.
struct _XVideo_PacketNode{
	AVPacket* pkt   = NULL;
	bool      flush = false;
	bool      eof   = false;
};

// 解码后视频帧 (BGRA32, 与 D2D / GDI 兼容, 已 swscale 完毕).
//   - bgra:    像素缓冲, size = pitch * h
//   - pitch:   每行字节数 (= w*4, sws_scale 输出连续无 padding)
//   - w / h:   像素尺寸 (与解码器 width/height 一致, 不随显示区缩放)
//   - ptsSec:  PTS, 秒, 已减去 m_startTimeSec 归零. < 0 表示 unknown.
//   - eof:     true = 流末尾哨兵帧, bgra 可空.
struct _XVideo_VideoFrameNode{
	std::vector<uint8_t> bgra;
	int    pitch  = 0;
	int    w      = 0;
	int    h      = 0;
	double ptsSec = 0.0;
	bool   eof    = false;
};

// 解码后音频帧 (PCM, 已 swr_convert 到目标格式, 与 WASAPI 输出一致).
//   - pcm:     交错 PCM 数据 (S16 / FLT, 看 m_audOutFmt)
//   - frames:  每通道样本数 (= pcm.size() / channels / bytesPerSample)
//   - ptsSec:  PTS, 秒, 已减去 m_startTimeSec.
//   - eof:     true = 流末尾哨兵.
struct _XVideo_AudioFrameNode{
	std::vector<uint8_t> pcm;
	int    frames = 0;
	double ptsSec = 0.0;
	bool   eof    = false;
};

// 上限有界 MPMC 队列. 满时 push 阻塞 (生产者背压), 空时 pop 阻塞.
// 关闭后 push 立即返 false, pop 把残余消费完再返 false. 这种行为方便上层在 quit
// 时干净退出 - 无需"先唤醒所有线程, 再清队列, 再 join"的复杂顺序.
template<class T>
class _XVideo_BoundedQueue{
public:
	_XVideo_BoundedQueue() : m_cap(64), m_closed(false) {}
	void SetCapacity(size_t cap){
		std::lock_guard<std::mutex> lk(m_mu);
		m_cap = cap;
		m_cvNotFull.notify_all();
	}
	// 阻塞 push. 队列已关闭返回 false, 调用方需销毁 t.
	bool Push(T&& t){
		std::unique_lock<std::mutex> lk(m_mu);
		m_cvNotFull.wait(lk, [&]{ return m_closed || m_q.size() < m_cap; });
		if (m_closed) return false;
		m_q.push_back(std::move(t));
		m_cvNotEmpty.notify_one();
		return true;
	}
	// 非阻塞 push. 满或已关闭返 false (调用方需销毁 t / 释放资源).
	bool TryPush(T&& t){
		std::unique_lock<std::mutex> lk(m_mu);
		if (m_closed)             return false;
		if (m_q.size() >= m_cap)  return false;
		m_q.push_back(std::move(t));
		m_cvNotEmpty.notify_one();
		return true;
	}
	// 带超时的阻塞 push. 超时返 false (调用方丢帧). 用于 video decode 防 "UI 卡 -> frameQ 满
	// -> decode 阻塞 -> pktQ 满 -> demux 阻塞 -> audio pktQ 不喂 -> audio render 静音"
	// 连锁卡死. 正常播放下 UI 消费快 (16ms 一帧), 远小于超时, 永远不丢帧; 仅 UI 真卡 (resize
	// loop / 死循环 >= timeoutMs) 才超时. 比 TryPush 不分场景丢帧温和得多.
	bool PushTimeout(T&& t, int timeoutMs){
		std::unique_lock<std::mutex> lk(m_mu);
		bool ok = m_cvNotFull.wait_for(lk, std::chrono::milliseconds(timeoutMs),
		                                [&]{ return m_closed || m_q.size() < m_cap; });
		if (!ok)         return false;     // 超时
		if (m_closed)    return false;
		m_q.push_back(std::move(t));
		m_cvNotEmpty.notify_one();
		return true;
	}
	// 阻塞 pop. 队列已关闭且空返回 false. 关闭但还有残量则继续返 true.
	bool Pop(T& out){
		std::unique_lock<std::mutex> lk(m_mu);
		m_cvNotEmpty.wait(lk, [&]{ return m_closed || !m_q.empty(); });
		if (m_q.empty()) return false;
		out = std::move(m_q.front());
		m_q.pop_front();
		m_cvNotFull.notify_one();
		return true;
	}
	// 非阻塞 try_pop. 空 / 关闭都返 false.
	bool TryPop(T& out){
		std::lock_guard<std::mutex> lk(m_mu);
		if (m_q.empty()) return false;
		out = std::move(m_q.front());
		m_q.pop_front();
		m_cvNotFull.notify_one();
		return true;
	}
	// 当前长度 (近似, 仅给调度参考用).
	size_t Size(){
		std::lock_guard<std::mutex> lk(m_mu);
		return m_q.size();
	}
	// 清空 (析构 / Seek 用). 已 close 也可调.
	void Clear(){
		std::lock_guard<std::mutex> lk(m_mu);
		m_q.clear();
		m_cvNotFull.notify_all();
	}
	// 关闭. 唤醒所有等待的线程退出.
	void Close(){
		std::lock_guard<std::mutex> lk(m_mu);
		m_closed = true;
		m_cvNotEmpty.notify_all();
		m_cvNotFull.notify_all();
	}
	// 重新打开 (复用同一队列): Stop 后再次 Play 走这里.
	void Reset(size_t cap){
		std::lock_guard<std::mutex> lk(m_mu);
		m_q.clear();
		m_cap = cap;
		m_closed = false;
	}
	bool IsClosed(){
		std::lock_guard<std::mutex> lk(m_mu);
		return m_closed;
	}
private:
	std::deque<T>           m_q;
	std::mutex              m_mu;
	std::condition_variable m_cvNotEmpty;
	std::condition_variable m_cvNotFull;
	size_t                  m_cap;
	bool                    m_closed;
};
//@隐藏}

///视频画面适配模式 (CXVideo::SetFitMode)
//@别名 视频适配模式
enum xvideo_fit_mode_
{
	//@别名 视频适配模式_等比适应
	xvideo_fit_contain  = 0,    ///<等比缩放, 长边贴边, 黑边补足 (默认, letter-box / pillar-box)
	//@别名 视频适配模式_等比裁切
	xvideo_fit_cover    = 1,    ///<等比缩放, 短边贴边, 长边裁切, 不留黑边
	//@别名 视频适配模式_拉伸填满
	xvideo_fit_stretch  = 2,    ///<不保持比例, 直接拉伸到元素客户区
	//@别名 视频适配模式_原始尺寸
	xvideo_fit_original = 3,    ///<不缩放, 居中显示原视频像素 (大于元素时裁切)
};

///播放状态 (CXVideo::GetState)
//@别名 视频播放状态
enum xvideo_state_
{
	//@别名 视频播放状态_未打开
	xvideo_state_closed   = 0,
	//@别名 视频播放状态_已打开未播放
	xvideo_state_stopped  = 1,
	//@别名 视频播放状态_播放中
	xvideo_state_playing  = 2,
	//@别名 视频播放状态_已暂停
	xvideo_state_paused   = 3,
	//@别名 视频播放状态_播放完毕
	xvideo_state_ended    = 4,
	//@别名 视频播放状态_错误
	xvideo_state_error    = 5,
	//@别名 视频播放状态_打开中
	xvideo_state_opening  = 6,   ///<Open() 已调, 后台线程正在 avformat_open_input / find_stream_info / 设备初始化. 成功后转 stopped (或 后续 Play 合并为 playing), 失败转 error.
};

///硬件解码偏好 (CXVideo::SetHwAccel)
//@别名 视频硬解模式
enum xvideo_hwaccel_
{
	//@别名 视频硬解模式_关闭
	xvideo_hwaccel_none    = 0,   ///<强制软解 (CPU). 兼容性最高, 1080p 及以下场景资源占用可接受.
	//@别名 视频硬解模式_自动
	xvideo_hwaccel_auto    = 1,   ///<默认: 优先 D3D11VA -> 退回 DXVA2 -> 退回软解. 推荐.
	//@别名 视频硬解模式_d3d11va
	xvideo_hwaccel_d3d11va = 2,   ///<仅 D3D11VA (Win 8+). Windows 现代主流硬解, NV/AMD/Intel 通吃.
	//@别名 视频硬解模式_dxva2
	xvideo_hwaccel_dxva2   = 3,   ///<仅 DXVA2 (Vista+). Win 7 fallback. 比 D3D11VA 老但兼容范围更广.
};

// 前置声明: 给下面 4 个回调 typedef 使用 (类完整定义在下面).
class CXVideo;

// =================================================================
// 事件回调原型 (C 风格函数指针 + void* 用户数据)
//   - 故意不用 std::function: 炫语言 / C / 旧 C++ 端不能传匿名函数;
//     函数指针 + 用户数据是跨语言通用形态 (Win32, GLFW, libuv 全用这套).
//   - 不写 __stdcall / CALLBACK: 类成员方法默认 __thiscall, 静态/全局函数
//     默认 __cdecl, 这里跟随默认即可; 强制 __stdcall 反而会让 C++ 端写 lambda
//     的人吃 calling convention 编译错.
//   - pUser 是注册时一并存下的不透明指针, 触发时原样回传, 类似 Win32 lParam.
// =================================================================
//@别名  视频回调_视频已打开
typedef void (*XVIDEO_PROC_OPENED)  (CXVideo* pVideo, void* pUser);
//@别名  视频回调_视频播放完毕
typedef void (*XVIDEO_PROC_ENDED)   (CXVideo* pVideo, void* pUser);
//@别名  视频回调_视频错误
typedef void (*XVIDEO_PROC_ERROR)   (CXVideo* pVideo, int errCode, const wchar_t* errMsg, void* pUser);
//@别名  视频回调_视频进度变化
typedef void (*XVIDEO_PROC_POSITION)(CXVideo* pVideo, double seconds, void* pUser);

//@分组{ 视频播放器
//@备注  继承: CXEle, CXWidgetUI, CXObjectUI, CXBase. 基础元素 + FFmpeg 解码 + D2D/GDI 渲染.
//@别名  炫彩视频播放器类
class CXVideo : public CXEle
{
public:
	//@隐藏{
	CXVideo();
	virtual ~CXVideo();
	//@隐藏}

//@备注 创建视频播放器元素.
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
//@返回 元素句柄
//@别名  创建()
	HELE Create(int x, int y, int cx, int cy, HXCGUI hParent = NULL);

//@备注 创建视频播放器元素 (构造函数版本).
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
	CXVideo(int x, int y, int cx, int cy, HXCGUI hParent = NULL){ Create(x, y, cx, cy, hParent); }

	// ===== 媒体加载 =====
//@备注 打开本地媒体文件或网络流 URL (rtsp:// / http:// / file:// 等 FFmpeg 支持的协议).
//      *完全异步*: 立即返回, 后台 m_thOpen 线程跑 avformat_open_input + find_stream_info +
//      解码器(含硬解)初始化. 期间 GetState() 返 xvideo_state_opening; 完成后:
//        - 成功: 派发 OnVideoOpened 事件 (UI 线程), GetState() = stopped (或 playing,
//                如果用户在 opening 期间已调 Play()).
//        - 失败: 派发 OnVideoError 事件, GetState() = error.
//      上次的媒体会先 Close (同步等待之前的 m_thOpen / worker 线程退出, 受 FFmpeg
//      interrupt callback 加持, Close 自身一般 < 50ms).
//      在 OnVideoOpened 触发前 GetVideoWidth/Height/Duration 等元数据 API 返 0; 应在事件里取.
//@参数 pPath 媒体路径或 URL (UTF-16 wchar_t*; 内部转 UTF-8 给 avformat_open_input)
//@返回 派发成功返回 TRUE (路径合法且已 spawn m_thOpen); 仅当 pPath 为 NULL/空时返回 FALSE.
//      *不代表打开成功* - 真正成功/失败请监听 OnVideoOpened / OnVideoError.
//@别名  打开()
	BOOL Open(const wchar_t* pPath);

//@备注 关闭当前媒体. 同步等待所有线程退出, 释放 FFmpeg / WASAPI 资源.
//@别名  关闭()
	void Close();

//@备注 是否已打开媒体 (打开成功后到 Close 之前).
//@返回 TRUE / FALSE
//@别名  是否已打开()
	BOOL IsOpen() const;

	// ===== 播放控制 =====
//@备注 开始播放. 若当前是 paused 则恢复; 若是 stopped/ended 则从头开始.
//@别名  播放()
	void Play();

//@备注 暂停播放. 视频帧定格在当前帧, 音频立即静默. Play 后从同一位置继续.
//@别名  暂停()
	void Pause();

//@备注 停止播放. 与 Pause 区别: 同时把播放位置 seek 回 0.
//@别名  停止()
	void Stop();

//@备注 是否正在播放.
//@返回 TRUE / FALSE
//@别名  是否播放中()
	BOOL IsPlaying() const;

//@备注 是否已暂停 (有别于已停止).
//@返回 TRUE / FALSE
//@别名  是否暂停()
	BOOL IsPaused() const;

//@备注 当前播放器状态.
//@返回 取值见 xvideo_state_*
//@别名  取播放状态()
	// 注: 不用 “取状态” 别名 - 会跟父类 CXEle::GetStateFlags() 的 取状态()
	// 冲突 (元素状态位 不同于 播放器状态).
	int  GetState() const;

	// ===== 进度 =====
//@备注 跳转到指定播放位置. 异步, 解码 / 渲染线程会在下一帧前完成.
//@参数 seconds 目标位置, 秒. 超出范围会被 clamp 到 [0, GetDuration()].
//@别名  跳转()
	void Seek(double seconds);

//@备注 媒体总时长. 流媒体 (直播) 不可知时返 0.
//@返回 秒
//@别名  取时长()
	double GetDuration() const;

//@备注 当前播放位置.
//@返回 秒. 未打开时返 0.
//@别名  取播放位置()
	// 注: 不用 “取位置” 别名 - 会跟父类 CXEle::GetPos() 的 取位置()
	// 冲突 (元素坐标 不同于 播放秒数).
	double GetPosition() const;

	// ===== 视频元数据 =====
//@备注 视频原始宽 (像素).
//@返回 像素
//@别名  取视频宽度()
	int  GetVideoWidth() const;

//@备注 视频原始高 (像素).
//@返回 像素
//@别名  取视频高度()
	int  GetVideoHeight() const;

//@备注 视频帧率 (来自容器声明值, 可能与实际略有出入).
//@返回 FPS
//@别名  取帧率()
	double GetFrameRate() const;

	// ===== 音频 =====
//@备注 媒体是否包含音频流. Open 后到 Close 间稳定.
//@返回 TRUE / FALSE
//@别名  是否有音频()
	BOOL HasAudio() const;

//@备注 设置音量 (线性 0..1, 内部应用到 PCM 样本上, 不经系统混音).
//@参数 v01 0..1. <0 / >1 自动 clamp.
//@别名  置音量()
	void SetVolume(float v01);

//@备注 取音量.
//@返回 0..1
//@别名  取音量()
	float GetVolume() const;

//@备注 静音 (与 SetVolume(0) 等效但保留原音量值, Mute(FALSE) 后恢复).
//@参数 bMute TRUE 静音 / FALSE 还原
//@别名  置静音()
	void SetMute(BOOL bMute);

//@备注 是否静音.
//@返回 静音返回 TRUE, 否则返回 FALSE
//@别名  是否静音()
	BOOL IsMuted() const;

//@备注 设置循环播放. TRUE: 播完自动 seek 0 重播 (无 OnVideoEnded 事件); FALSE: 播完转 ended 状态并派发 OnVideoEnded.
//@参数 bLoop TRUE 启用 / FALSE 关闭
//@别名  置循环播放()
	void SetLoop(BOOL bLoop);

//@备注 是否循环播放.
//@返回 TRUE / FALSE
//@别名  是否循环播放()
	BOOL GetLoop() const;

	// ===== 内置控件栏 (Play/Pause, Loop, 进度, 时间, 音量) =====
//@备注 启用/关闭内置控件栏 (覆盖视频底部 ~40 物理像素的半透明操作条).
//      默认 TRUE. 必须在 Create() *之前* 调才生效; Create() 之后改不影响已建好的控件.
//      设为 FALSE 适合纯视频显示场景 (kiosk / 浮窗 / 自定义 UI).
//@参数 bEnable TRUE 启用 / FALSE 关闭
//@别名  启用控件栏()
	void EnableControlBar(BOOL bEnable);

//@备注 是否启用内置控件栏.
//@返回 TRUE / FALSE
//@别名  是否启用控件栏()
	BOOL IsControlBarEnabled() const;

//@备注 启用 *鼠标自动隐藏控件栏* (YouTube 风格). 默认 TRUE.
//      启用后: 鼠标在视频区移动 -> 显示控件栏; N 毫秒无活动 -> 隐藏.
//      鼠标在控件栏或其子控件 (按钮/slider) 上移动也算"活动"; 音量面板可见时不隐藏.
//      关闭后: 控件栏强制保持显示 (除非用户调 XEle_SetVisible 自己隐藏).
//@参数 bEnable TRUE 启用 / FALSE 关闭
//@别名  启用控件栏自动隐藏()
	void EnableControlBarAutoHide(BOOL bEnable);

//@备注 是否启用自动隐藏.
//@别名  是否启用控件栏自动隐藏()
	BOOL IsControlBarAutoHideEnabled() const;

//@备注 设置自动隐藏超时毫秒数. 默认 2500ms. 合法范围 [100, 60000].
//@参数 ms 毫秒
//@别名  置控件栏自动隐藏超时()
	void SetControlBarAutoHideTimeout(int ms);

//@备注 取自动隐藏超时毫秒数.
//@别名  取控件栏自动隐藏超时()
	int  GetControlBarAutoHideTimeout() const;

//@备注 设置控件栏背景色. 默认 0xFF000000 (纯黑不透明).
//      注意 XCGUI 颜色是 *ARGB* (alpha 最高位); Windows 的 RGB() 宏 alpha=0 -> 全透明.
//      请用 XCGUI 的 RGBA(r,g,b,a) 宏或直接给 0xAARRGGBB 形式 (高字节 = alpha).
//      Create() 之前调: 只改成员变量, Create 时生效. Create() 之后调: 立即刷新到现有控件.
//@参数 color ARGB 颜色 (alpha 必须 != 0 否则不可见)
//@别名  置控件栏背景色()
	void SetControlBarColor(COLORREF color);

//@备注 取控件栏背景色 (ARGB).
//@别名  取控件栏背景色()
	COLORREF GetControlBarColor() const;

//@备注 设置控件栏文字色 (Play/Loop/Volume 按钮 + 时间标签). 默认 0xFFFFFFFF (纯白不透明).
//@参数 color ARGB 颜色
//@别名  置控件栏文字色()
	void SetControlBarTextColor(COLORREF color);

//@备注 取控件栏文字色.
//@别名  取控件栏文字色()
	COLORREF GetControlBarTextColor() const;

	// ----- 控件栏 子控件 getter (供外部定制外观: 加 bkInfoM / 改字体 / 注册更多事件) -----
//@备注 取控件栏容器 HELE (CXLayout). Create() 之后才有效, 否则返 NULL.
//@返回 HELE
//@别名  取控件栏()
	HELE   GetControlBar()    const;
//@备注 取播放/暂停按钮 HELE (CXButton).
//@别名  取按钮播放()
	HELE   GetBtnPlay()       const;
//@备注 取循环按钮 HELE (CXButton).
//@别名  取按钮循环()
	HELE   GetBtnLoop()       const;
//@备注 取进度条 HELE (CXSliderBar, 横向).
//@别名  取进度条()
	HELE   GetSliderProgress() const;
//@备注 取时间标签 HXCGUI (CXShapeText). 注意: 是 *形状* 不是元素, 用 XShapeText_* API.
//@别名  取时间标签()
	HXCGUI GetLblTime()       const;
//@备注 取音量按钮 HELE (CXButton).
//@别名  取按钮音量()
	HELE   GetBtnVolume()     const;
//@备注 取音量面板 HELE (CXLayout). 随控件栏一起提前建, 初始隐藏;
//      EnableControlBar(FALSE) 下未建控件栏时为 NULL.
//      面板是 m_hEle 的 *子元素* (不是独立窗口),
//      避开 DPI 位置计算 + 焦点夺取 两个问题.
//@别名  取音量面板()
	HELE   GetVolumePanel()   const;
//@备注 取音量面板内的垂直 slider HELE. 随面板一起提前建.
//@别名  取音量滑块()
	HELE   GetSliderVolume()  const;

	// ===== 显示选项 =====
//@备注 设置画面适配模式. 默认 contain (等比适应, 黑边补足).
//@参数 mode 见 xvideo_fit_mode_
//@别名  置适配模式()
	void SetFitMode(int mode);

//@备注 取适配模式.
//@返回 取值见 xvideo_fit_mode_
//@别名  取适配模式()
	int  GetFitMode() const;

//@备注 设置 letter-box / 视频区背景填充色 (适配模式留出黑边时填这个色).
//@参数 color 颜色, 用 RGBA() 宏构造
//@别名  置视频背景色()
	void SetVideoBkColor(COLORREF color);

//@备注 取视频背景色.
//@返回 颜色值, RGBA
//@别名  取视频背景色()
	COLORREF GetVideoBkColor() const;

	// ===== 性能调优 =====
//@备注 设置内部队列上限. 默认 packet 64 / frame 8. 加大可抗网络抖动 / 大 GOP, 但会
//      占更多内存. 帧队列每帧约 w*h*4 字节 (1080p 约 8MB), 队列 8 帧 = 64MB.
//@参数 packetCap  解封装包队列上限
//@参数 frameCap   解码帧队列上限
//@别名  置队列上限()
	void SetMaxQueueSize(int packetCap, int frameCap);

	// ===== 硬件解码 =====
//@备注 设置硬件解码偏好. 必须在 Open() 之前调用; Open 之后改不影响当前流, 重新 Open 才生效.
//      硬解流程: 创建 AV_HWDEVICE_TYPE_D3D11VA / DXVA2 设备上下文 -> 装到 AVCodecContext::hw_device_ctx
//      -> get_format 回调返硬解 pix_fmt (AV_PIX_FMT_D3D11 / AV_PIX_FMT_DXVA2_VLD) -> 解码出
//      硬件 frame -> av_hwframe_transfer_data 拷回 CPU NV12 -> sws_scale 到 BGRA32.
//      *硬解收益*: 1080p H.264 CPU 占用降 6% -> 2%; 4K HEVC 显著, 18% -> 6%. 但 GPU->CPU 回传
//      占 PCIe 带宽, 1080p ≈ 8MB/帧, 4K ≈ 32MB/帧, 多路同播或带宽紧时反而是劣势.
//      *降级*: 任意一步失败都自动回到软解, 不影响 Open 成功.
//@参数 type 见 xvideo_hwaccel_
//@别名  置硬解模式()
	void SetHwAccel(int type);

//@备注 取硬解偏好 (设置值, 不一定真激活, 见 IsHwAccelActive).
//@返回 取值见 xvideo_hwaccel_
//@别名  取硬解模式()
	int  GetHwAccel() const;

//@备注 当前是否真正激活了硬解. Open 成功且硬解上下文创建成功后为 TRUE; 软解 / Open 前 / Close 后为 FALSE.
//@返回 TRUE / FALSE
//@别名  是否硬解中()
	BOOL IsHwAccelActive() const;

	// ===== 事件回调 (按 .cursor/rules/xcgui-events.mdc 命名: 控件名+动作) =====
	// 设计选择: 视频特有事件不映射到 XCGUI 自定义事件 ID 体系 (XCGUI 内置事件 ID 不可
	// 由用户扩展, 见 module_xcgui.h XE_* 定义); 改用 *C 风格函数指针 + void* pUser*
	// 注册 (上方 XVIDEO_PROC_* typedef). 不用 std::function 是为了让炫语言 / C / 旧 C++
	// 端可调 (那些环境不支持匿名函数). XCGUI 标准事件 (XE_PAINT / XE_SIZE 等)
	// 还是走 RegEventCPP1; 本事件类别仅限于 *自定义业务事件*.

//@备注 媒体打开成功事件. 此时元数据 (尺寸/时长/帧率) 已就绪.
//@参数 fn 事件回调函数指针; 传 NULL 取消注册. 回调里收到的 CXVideo* 与本对象相同.
//@参数 pUser 用户数据指针; 触发时原样回传给 fn 的最后一个参数. 不需要可填 NULL.
//@别名  注册事件_视频已打开()
	void OnVideoOpened(XVIDEO_PROC_OPENED fn, void* pUser = NULL);

//@备注 播放到媒体末尾事件 (非 Stop/Close 触发).
//@参数 fn 事件回调函数指针; 传 NULL 取消注册.
//@参数 pUser 用户数据指针; 触发时原样回传. 不需要可填 NULL.
//@别名  注册事件_视频播放完毕()
	void OnVideoEnded(XVIDEO_PROC_ENDED fn, void* pUser = NULL);

//@备注 错误事件. errCode 来自 FFmpeg AVERROR 或本类内部错误码; errMsg 给 UI 提示用.
//      worker 线程产生的错误标志会被定时器派发到 UI 线程后再调 fn, 因此 fn 内可直接
//      操作 XCGUI (例如 XLabel_SetText 提示用户).
//@参数 fn 事件回调函数指针; 参数 errCode = AVERROR 值, errMsg = 友好提示字符串 (UTF-16).
//@参数 pUser 用户数据指针; 触发时原样回传. 不需要可填 NULL.
//@别名  注册事件_视频错误()
	void OnVideoError(XVIDEO_PROC_ERROR fn, void* pUser = NULL);

//@备注 播放进度变化事件. 仅在状态机意义上的"明显进度变化"时触发 (默认每 200ms / 走完
//      一个视频帧上屏后), 不是每帧都触发, 避免频繁回调拖累 UI 线程.
//@参数 fn 事件回调函数指针; 参数 seconds = 当前播放位置, 与 GetPosition() 同值.
//@参数 pUser 用户数据指针; 触发时原样回传. 不需要可填 NULL.
//@别名  注册事件_视频进度变化()
	void OnVideoPositionChanged(XVIDEO_PROC_POSITION fn, void* pUser = NULL);

	//@隐藏{
private:
	// =================================================================
	// 内部成员 (大量, 注释优先解释 *为什么是这个值/类型*, 而不是显而易见的 "what")
	// =================================================================

	// ===== FFmpeg 上下文 =====
	// 全部由 OpenInternal 在 demux 线程上首帧前一次性建好; CloseInternal 释放.
	// 不同生命周期之外不应被外部线程读 (UI 线程读 GetVideoWidth/Height 走的是缓存的
	// m_videoW / m_videoH 不是直接读 m_pVCtx).
	AVFormatContext* m_pFmt   = NULL;   // 容器解封装上下文
	int m_videoIdx = -1;                // 视频流在 m_pFmt->streams 里的下标 (无视频则 -1)
	int m_audioIdx = -1;                // 音频流下标 (无音频则 -1)
	AVCodecContext*  m_pVCtx  = NULL;   // 视频解码器
	AVCodecContext*  m_pACtx  = NULL;   // 音频解码器
	SwsContext*      m_pSws   = NULL;   // 视频像素格式 / 尺寸转换 (any -> BGRA32)
	SwrContext*      m_pSwr   = NULL;   // 音频重采样 (任意 -> 设备格式)
	AVRational       m_videoTb{0, 1};   // 视频时基 (stream->time_base, 转秒用)
	AVRational       m_audioTb{0, 1};   // 音频时基

	// ===== 缓存的元数据 (UI 线程可读, demux/decode 线程不写) =====
	// 在 OpenInternal 末尾、StartThreads 之前写一次, 之后只读, 不需要锁.
	int    m_videoW = 0, m_videoH = 0;
	double m_frameRate = 0.0;           // 视频帧率 (FPS)
	double m_durationSec = 0.0;         // 总时长 (秒). 流媒体未知时为 0.
	double m_startTimeSec = 0.0;        // 流的起始 PTS (m_pFmt->start_time / AV_TIME_BASE),
	                                    // 后续所有 ptsSec 都减它归零, UI 进度从 0 起.
	bool   m_hasVideo = false;
	bool   m_hasAudio = false;

	// ===== 队列 =====
	_XVideo_BoundedQueue<_XVideo_PacketNode>     m_videoPktQ;
	_XVideo_BoundedQueue<_XVideo_PacketNode>     m_audioPktQ;
	_XVideo_BoundedQueue<_XVideo_VideoFrameNode> m_videoFrameQ;
	_XVideo_BoundedQueue<_XVideo_AudioFrameNode> m_audioFrameQ;
	int m_packetCap = 64;
	int m_frameCap  = 8;

	// ===== 线程 =====
	// m_thOpen: 异步 Open 专用. 干三件事 - avformat_open_input + find_stream_info + 解码器
	// (含硬解) 初始化. 这三件都是同步阻塞调用 (本地 mp4 ~50ms, 网络流数秒, debug
	// heap 下几十秒), 不能在 UI 线程跱.
	std::thread m_thOpen;
	std::thread m_thDemux;
	std::thread m_thVDecode;
	std::thread m_thADecode;
	std::thread m_thARender;
	// quit: 终止信号. StopThreads 设为 true 并 Close() 所有队列, 各线程在循环里检查
	// 后退出. 用 atomic 避免锁争用.
	std::atomic<bool> m_quit{false};

	// loop: 循环播放标志. UI 线程 (SetLoop) 写, TryAdvanceFrame 在 EOF 时读: 若 true
	// 则 seek 回 0 + 重置时钟基准, 不发 OnVideoEnded; 若 false 走原 ended 流程.
	std::atomic<bool> m_loop{false};

	// ===== 控件栏 (内置 UI: Play/Pause, Loop, 进度, 时间, 音量) =====
	// 整体结构 (CXLayout 自动布局, 不再手算坐标):
	//   m_hEle (CXLayout, vertical, alignV=bottom, mouseThrough=FALSE)
	//     └── m_hCtrlBar (CXLayout, horizontal, alignV=center, space=8, padding=12,0,12,0)
	//            ├── m_hBtnPlay        (CXButton, button_type_check, 40x40, transparent)
	//            │     check=TRUE  -> 播放中 (UI 由 UpdateControlBarPlayState 同步 m_state);
	//            │     check=FALSE -> 暂停/停止/已结束/未打开. XE_BUTTON_CHECK 驱动 Play/Pause.
	//            ├── m_hBtnLoop        (CXButton, button_type_check, 40x40, transparent)
	//            │     check=TRUE/FALSE 直接对应 SetLoop/GetLoop, XE_BUTTON_CHECK 驱动.
	//            ├── m_hSliderProgress (CXSliderBar, layout.width=weight:1, transparent)
	//            ├── m_hLblTime        (CXShapeText, layout.width=auto, transparent)
	//            └── m_hBtnVolume      (CXButton, 40x40, transparent)
	BOOL    m_ctrlBarEnabled = TRUE;
	// 自动隐藏 (YouTube 风格): 鼠标活动显示, 超时无活动隐藏. 默认开启 2.5s.
	BOOL    m_autoHideCtrlBar     = TRUE;
	DWORD   m_autoHideTimeoutMs   = 2500;
	DWORD   m_lastUserActivityTick = 0;       // GetTickCount(), 16ms 定时器里跟 timeout 比
	// 颜色: XCGUI 颜色是 ARGB, alpha 必须显式给 (Windows RGB 宏 alpha=0 -> 全透明).
	// 默认: 黑底白字. 用户可在 Create() 之前 或 之后调 SetControlBarColor / SetControlBarTextColor
	// 修改. Create() 之前调只改成员变量; Create 之后调会同步刷新现有控件.
	COLORREF m_ctrlBarBg      = 0xFF000000;   // RGBA(0,0,0,255)   纯黑
	COLORREF m_ctrlBarFg      = 0xFFFFFFFF;   // RGBA(255,255,255,255) 纯白
	HELE    m_hCtrlBar        = NULL;
	HELE    m_hBtnPlay        = NULL;
	HELE    m_hBtnLoop        = NULL;
	HELE    m_hSliderProgress = NULL;
	HXCGUI  m_hLblTime        = NULL;     // CXShapeText -> HXCGUI (shape, 不是 element)
	HELE    m_hBtnVolume      = NULL;
	// 音量 面板: CreateControlBar() 后期 一起创建 (初始隐藏), 不再懒建.
	// 创建后 才能 给 GetVolumePanel / GetSliderVolume 返回有效句柄 供外部改样式 / 注事件.
	// 是 m_hEle 的子 (不是独立窗口), LayoutItem.width/height = disable -> 不参与父布局,
	// 用 XEle_SetRect 绝对定位. 这样避开 独立窗口的 DPI 位置计算 + 焦点被夺 两个问题.
	HELE    m_hVolPanel       = NULL;
	HELE    m_hSliderVolume   = NULL;
	// SVG 图标句柄 (硬编码, EnsureSvgsLoaded 里 XSvg_LoadStringUtf8 加载, OnDestroyImpl 里 XSvg_Destroy).
	// Play/暂停 互换; 循环开/关 互换; 有声/静音 互换.
	HSVG    m_hSvgPlay        = NULL;     // play.svg       (当前未在播)
	HSVG    m_hSvgSuspend     = NULL;     // suspend.svg    (当前在播, 显示暂停图标)
	HSVG    m_hSvgLoop        = NULL;     // loop.svg       (循环开)
	HSVG    m_hSvgLoopClose   = NULL;     // loop_close.svg (循环关)
	HSVG    m_hSvgVoice       = NULL;     // voice.svg      (有声)
	HSVG    m_hSvgVoiceMute   = NULL;     // voice_mute.svg (静音 / volume==0)
	BOOL    m_programmaticSliderUpdate = FALSE;
	// 程式化 XBtn_SetCheck 反弹保护: m_hBtnPlay / m_hBtnLoop 是 button_type_check, 调
	// XBtn_SetCheck 会触发 XE_BUTTON_CHECK -> OnBtnPlayCheck / OnBtnLoopCheck. 没有这个
	// 标志的话: UpdateControlBarPlayState -> XBtn_SetCheck -> OnBtnPlayCheck -> Play/Pause
	// -> UpdateControlBarPlayState -> ... 死循环. 与 m_programmaticSliderUpdate 同模式.
	BOOL    m_programmaticBtnCheck = FALSE;
	int     m_lastProgressPos = -1;
	std::wstring m_lastTimeStr;
	// ===== 进度条 拖动节流 + slider 锁定 =====
	// 用户高频拖动 (mouse move 一帧 1 次, 60Hz) 时, 不每次都 Seek, 否则 demux 被打爆,
	// 永远稳不下来. OnSliderProgressChange 距上次 Seek < kScrubMinIntervalMs 时只暂存
	// pending, OnTimerImpl 在 16ms tick 里检查 pending 节流后 commit. mouseup 后用户最
	// 后那次微小拖动 100ms 内会被 commit, 体感不出延迟.
	double  m_pendingScrubSec    = -1.0;     // < 0 表示无 pending; >= 0 是待 commit 的目标秒
	DWORD   m_lastScrubSeekTick  = 0;        // 上次实际 Seek 的 tick (节流参考)
	// slider 锁定: scrub 之后 kSliderLockMs 内, UpdateControlBarPosition 不动 slider 视觉
	// 位置. 防 av_seek_frame BACKWARD 跳到 keyframe 后 m_audioClock 反向回退, 把 slider
	// 推到 keyframe 位置 (= 比用户拖到的位置往前几秒) 的 "小回弹".
	DWORD   m_lastScrubTick      = 0;        // 用户最近一次 OnSliderProgressChange 的 tick

	// ===== 播放状态 =====
	std::atomic<int> m_state{xvideo_state_closed};
	// pause: 与 m_state 协同. m_state 是给外部 / 事件用的离散状态; m_paused 是热路径上
	// 渲染线程 / 音频线程的快速判定.
	std::atomic<bool> m_paused{false};
	// EOF: demux 线程读完源时设. 解码线程把残余帧 flush 后向各 frameQ 推 eof 哨兵.
	std::atomic<bool> m_demuxEof{false};

	// ===== Seek 协议 =====
	// 步骤 (在 demux 线程上完成):
	//   (1) UI 线程 Seek(): m_seekRequest = true; m_seekTargetSec = target;
	//   (2) demux 线程检测 m_seekRequest, 把 packetQ / frameQ 全部 Clear,
	//       推 PacketNode{flush=true} 给两路解码线程, 调 av_seek_frame, 设
	//       m_seekFlushBaseline = ++m_seekFlushSeq, 清 m_seekRequest;
	//   (3) 解码线程拿到 flush 包: avcodec_flush_buffers, 自己也产 eof 哨兵推帧队列;
	//   (4) UI 线程 OnPaint 看到帧 PTS 跳到新位置就视觉上 seek 完成了.
	std::atomic<bool>   m_seekRequest{false};
	std::atomic<double> m_seekTargetSec{0.0};
	// m_seekInFlight 覆盖整个 "Seek() 被调 -> demux 完成 seek 处理" 窗口,
	// 比 m_seekRequest 寿命长. m_seekRequest 在 demux loop top 立刻被 exchange(false),
	// 但此时队列还没清完 / 时钟没归位; 而 audio render 仍可能在拉 *seek 之前* 残留的旧帧
	// -> m_audioClock 被推到旧位置 -> UpdateControlBarPosition 把 slider 倒灌回旧位置 ->
	// 用户看到 "回弹" 闪一下. 解法: 这个标志在 Seek() 时置 true, demux 把 seek 全部处理
	// 完 (清队列 + flush + 时钟归位) 后才置 false; UpdateControlBarPosition 期间整体跳过.
	std::atomic<bool>   m_seekInFlight{false};

	// ===== 时钟 (A/V 同步) =====
	// 主时钟: 有音频时 = m_audioClock (音频渲染线程在每次 WASAPI 写入后更新);
	//          无音频时 = 墙钟换算 (Play 时刻起算 m_clockBaseTick, 加上偏移).
	// 视频帧调度路径在 OnTimerImpl 里读主时钟, 拉 frameQ 顶部帧, 若帧 PTS 落后于主时钟
	// 太多则丢帧, 落后少则立即上屏, 超前则等下一个 timer tick. 详见 cpp 里 TryAdvanceFrame.
	std::atomic<double> m_audioClock{0.0};   // 秒, 音频已写入 device 的 PTS
	std::atomic<double> m_videoClock{0.0};   // 秒, 上一帧上屏时记录, 给 GetPosition 备用
	std::atomic<DWORD>  m_clockBaseTick{0};  // GetTickCount() 基准, 无音频时用
	std::atomic<double> m_clockBasePts{0.0}; // 基准对应的 PTS 偏移

	// ===== 当前要显示的帧 (双缓冲, 渲染线程写, UI 线程读) =====
	// 与 D2D 渲染锁分离 - 锁粒度小, 60fps 下争用可以忽略.
	std::mutex           m_curFrameMutex;
	std::vector<uint8_t> m_curBgra;
	int    m_curW = 0, m_curH = 0;
	int    m_curPitch = 0;
	bool   m_curDirty = false;        // true = 自上次上屏后被刷新过, 需 ID2D1Bitmap::CopyFromMemory
	double m_curFramePts = 0.0;

	// ===== 硬件解码 =====
	// m_hwAccelPref: 用户偏好 (xvideo_hwaccel_*), Open 时读一次. 之后改不影响当前流.
	// m_hwAccelActive: Open 时硬解上下文创建成功才置 true; 任意一步失败回到软解,
	//                  本字段保持 false. 渲染线程靠这个判断要不要走 av_hwframe_transfer_data.
	// m_pHwDeviceCtx: AV_HWDEVICE_TYPE_D3D11VA / DXVA2 设备上下文. 装到 AVCodecContext::hw_device_ctx
	//                 之前要做 av_buffer_ref. CloseInternal 里 av_buffer_unref 释放本地 ref.
	// m_hwPixFmt: 硬解出来的 pix_fmt (AV_PIX_FMT_D3D11 / AV_PIX_FMT_DXVA2_VLD), get_format 回调返这个.
	// m_hwActiveType: 真正激活的设备类型 (用户选 auto 时记录最终落到哪种), 给日志 / 调试用.
	int                m_hwAccelPref   = xvideo_hwaccel_auto;
	bool               m_hwAccelActive = false;
	AVBufferRef*       m_pHwDeviceCtx  = NULL;
	AVPixelFormat      m_hwPixFmt      = AV_PIX_FMT_NONE;
	AVHWDeviceType     m_hwActiveType  = AV_HWDEVICE_TYPE_NONE;

	// ===== WASAPI =====
	IMMDeviceEnumerator* m_pAudEnum   = NULL;
	IMMDevice*           m_pAudDevice = NULL;
	IAudioClient*        m_pAudClient = NULL;
	IAudioRenderClient*  m_pAudRender = NULL;
	HANDLE               m_hAudioEvent = NULL;   // 共享模式事件驱动: 设备就绪通知
	// 防 InitWasapi 失败时被 audio decode 线程每 100ms 反复调用 -> PostError 刷屏.
	// 一旦本 session 首次 init 失败, 置 true, 后续 InitWasapi 静默 fail (PostError 跳过).
	// 由 CloseInternal 复位为 false, 让下一次 Open 重新尝试.
	bool                 m_wasapiInitFailed = false;
	UINT32               m_audBufFrames = 0;     // 设备 buffer 容量 (帧数)
	int                  m_audSampleRate = 0;
	int                  m_audChannels = 0;
	AVSampleFormat       m_audOutFmt = AV_SAMPLE_FMT_S16;   // 与 WASAPI WAVEFORMATEX 对齐
	int                  m_audBytesPerFrame = 0; // = channels * bytesPerSample
	float                m_volume = 1.0f;
	BOOL                 m_muted  = FALSE;

	// ===== 音频残留缓冲 =====
	// 解码 PCM 一帧 (常见 1024 样本 / AAC) 比 WASAPI 一次能填的 buffer slot
	// (常见 480 样本 = 10ms@48kHz device period) 大. 之前简化版直接丢弃多余
	// 样本 -> 音频 2x 速度播放 (af.ptsSec 跳到下一帧但只播了 1/2). 修: 把多出来
	// 的部分缓存在这里, 下次 WASAPI 要数据时优先消费.
	//   m_audResidualPcm:    剩余样本字节流, 与 device 格式一致 (FLT/S16 + nch * frames).
	//   m_audResidualFrames: per-channel 样本数.
	//   m_audResidualPtsSec: 该残留首样本对应的 *源* PTS, 给 audio clock 计算用.
	std::vector<uint8_t> m_audResidualPcm;
	int                  m_audResidualFrames = 0;
	double               m_audResidualPtsSec = 0.0;

	// ===== 渲染资源 =====
	// D2D 路径: ID2D1Bitmap 每次 RT 变化时重建 (RT 由 XCGUI 持有, 元素 resize / 父窗口
	// 切换 / 模式切换都会重建). RT 指针变了 就丢旧 bitmap 用新 RT 重创.
	ID2D1Bitmap*       m_pD2DBmp   = NULL;
	ID2D1RenderTarget* m_pLastRT   = NULL;
	int                m_d2dBmpW = 0;
	int                m_d2dBmpH = 0;
	// GDI 路径: 用 *离屏 DIB* 做合成缓冲. 所有 GDI 操作
	// (背景 FillRect + 视频帧 StretchDIBits) 在 DIB 内完成, 最后一次 BitBlt 回 XCGUI
	// 给的屏幕 HDC. 这样:
	//   1) 屏幕 HDC 上一次只看到 *一次* BitBlt - 避免 "FillRect 已画 / StretchDIBits
	//      还没画" 的中间态被 XCGUI/GDI+ 合成抓取, 解决鼠标移动重绘时画面抽搐.
	//   2) DIB 是新缓冲, 不受 XCGUI paint 阶段已合成像素 / clip 状态干扰, resize 后
	//      残留的旧帧像素不会再 "漏" 出来 (整块 BitBlt 把 DIB 像素 1:1 覆盖元素客户区).
	// 缓存策略: 按 *物理像素 元素客户区尺寸* 重建. 同尺寸下复用, 避免每帧 alloc.
	HDC     m_gdiMemDC    = NULL;
	HBITMAP m_gdiDib      = NULL;
	HBITMAP m_gdiOldBmp   = NULL;   // SelectObject 返的原 1x1 mono bmp, 释放前要 select 回去
	int     m_gdiDibW     = 0;
	int     m_gdiDibH     = 0;
	// DIB 内容是否需要重画. true = 必须重画 (FillRect + StretchDIBits) 才能 BitBlt 出新画面.
	// 触发置 true 的时机: DIB 重建 (尺寸变化) / 视频帧推进 (m_curDirty 信号). 避免高频 paint
	// 时反复拷贝 1080p ≈ 8MB 视频数据, UI 线程被吃光导致 video timer 推不动.
	bool    m_gdiDibDirty = true;

	// ===== DPI 缩放 =====
	// 96 = 1.0; 144 = 1.5; 192 = 2.0. 元素客户区物理像素 = 逻辑像素 * dpiScale.
	// XEle_GetWndClientRectDPI 返物理像素, 与 D2D RT face-value 同坐标系, 直接用即可;
	// XDraw_GetOffset (GDI 路径) 返物理像素, 也是 face-value. 视频帧自身像素是设备无关的
	// (源像素), 所以 ComputeDestRect 在物理像素坐标系里算映射区, 内部不再乘 dpiScale.
	float m_dpiScale = 1.0f;

	// ===== 显示选项 =====
	int      m_fitMode = xvideo_fit_contain;
	COLORREF m_videoBkColor = RGBA(0x00, 0x00, 0x00, 0xFF);

	// ===== 定时器 ID =====
	// XEle_SetXCTimer 接受任意非 0 UINT, 与该元素其他定时器 ID 不冲突即可. 取一个
	// 视觉上能看出"是 video 帧调度"的魔数, 便于断点 / 日志辨认.
	static const UINT kTimerId_Tick = 0xCB1D0001;
	// 定时器周期: 太短空转烧 CPU; 太长帧抖动. 16ms ≈ 60fps, 与 Windows DWM 帧率匹配.
	static const UINT kTimerInterval_Ms = 16;

	// ===== 进度回调节流 =====
	// OnVideoPositionChanged 只在 *帧上屏* + *距上次回调 >= kPositionEmitInterval* 时触发.
	double m_lastEmittedPosSec = -1.0;
	static constexpr double kPositionEmitInterval = 0.2;   // 200ms

	// ===== 用户事件回调 =====
	// 函数指针本身是 trivially-copyable, 跨线程读写仍需保护; 我们规定: 注册仅在 UI 线程发生;
	// 触发也仅在 UI 线程 (worker 线程通过 PostMessage / 元素定时器 hook 转回 UI 线程).
	// pUser 与回调指针成对存储, 注册时一并设, 触发时原样回传.
	XVIDEO_PROC_OPENED   m_cbOpened     = NULL;   void* m_userOpened   = NULL;
	XVIDEO_PROC_ENDED    m_cbEnded      = NULL;   void* m_userEnded    = NULL;
	XVIDEO_PROC_ERROR    m_cbError      = NULL;   void* m_userError    = NULL;
	XVIDEO_PROC_POSITION m_cbPosition   = NULL;   void* m_userPosition = NULL;

	// 跨线程标志: worker 线程产事件后只设 atomic flag, UI 线程定时器轮询并触发回调.
	// 这样避免在解码 / 音频线程里直接调用回调 (调用方可能从回调里 SendMessage,
	// 反向到锁顺序倒置).
	std::atomic<bool> m_pendingOpened{false};
	std::atomic<bool> m_pendingEnded{false};
	std::atomic<bool> m_pendingError{false};
	int               m_pendingErrCode = 0;
	std::wstring      m_pendingErrMsg;
	std::mutex        m_pendingErrMutex;

	// =================================================================
	// 内部辅助 (declare here, define in cpp)
	// =================================================================
	// 事件 / 定时器 / 销毁 hook 安装. Create() 末尾调一次.
	void InstallEvents();
	int  OnPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int  OnSizeImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled);
	int  OnTimerImpl(HELE hEle, UINT nTimerId, BOOL* pbHandled);
	int  OnDestroyImpl(HELE hEle, BOOL* pbHandled);
	// 视频区域左键弹起 -> 切换 播放/暂停.
	// 控件栏是 *默认鼠标穿透* (CXLayout default), 点控件栏空白会透透过到视频区
	// 触发这里 - 有点 youtube "点视频 = 暂停" 的体验. 按钮的 XE_BNCLICK 被消费, 不冲突.
	// 额外: 若音量面板可见, 优先关面板不切换暂停 (微交互).
	int  OnLButtonUpVideo(HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled);

	// ===== 控件栏 子系统 =====
	// 在 Create() 末尾若 m_ctrlBarEnabled=TRUE 则调. 创建 m_hCtrlBar (CXLayout 容器) + 5 个子控件
	// (play/loop btn + progress slider + time label + volume btn). 注册各自事件回调.
	// 布局靠 XCGUI 的 CXLayout 引擎完成 - 用 LayoutItem_SetWidth/Height/Align 给每个子控件
	// 声明 fill/auto/weight/fixed, 然后 XCGUI 自动按窗口尺寸 reflow, 不需要手算坐标.
	void CreateControlBar();
	// 帧推进 / 位置回调时调. 用当前 GetPosition() 更新进度条 + 时间标签.
	// 若 m_programmaticSliderUpdate=TRUE 或 m_userSeeking, 跳过 (避免反向干扰用户拖动).
	// bForce=true 跳过 m_seekInFlight short-circuit, 给 Stop() / 强制刷新场景用.
	void UpdateControlBarPosition(double posSec, bool bForce = false);
	// 播放状态变化 (Play/Pause/Stop/Ended/Error) 时调. 切换 m_hBtnPlay 显示的 ▶ / ❚❚
	// 同时把 button_type_check 的 check 状态同步到 m_state==playing.
	// 内部用 m_programmaticBtnCheck 屏蔽自身触发的 XE_BUTTON_CHECK.
	void UpdateControlBarPlayState();
	// SetLoop 调用时同步循环按钮的 "已选中" 视觉状态. 内部用 m_programmaticBtnCheck 屏蔽
	// XBtn_SetCheck 触发的 XE_BUTTON_CHECK 回环.
	void UpdateControlBarLoopState();
	// CreateControlBar() 末尾调: 提前建好 音量面板 + 垂直 slider, 初始隐藏.
	// 这样 GetVolumePanel / GetSliderVolume 在 Create() 后就能拿到有效句柄, 用户能提前 改样式 / 加额外事件.
	void CreateVolumePanel();
	// 音量面板 切换可见. 面板在 CreateVolumePanel 里提前建好, 这里只重新定位 + show/hide.
	void ToggleVolumePanel();
	// SVG 图标资源: 首次调时 XSvg_LoadStringUtf8 从硬编码字串建好, 后续成幂等.
	void EnsureSvgsLoaded();
	// 释放 所有 SVG 句柄. OnDestroyImpl 里调, 避免 XCGUI 卸载后句柄变陆离.
	void DestroySvgs();
	// 隐藏面板. 供 OnLButtonUpVideo 点外部关闭 / 他按钮 click 时调.
	void HideVolumePanel();
	// 面板当前是否可见.
	BOOL IsVolumePanelVisible() const;
	// 自动隐藏: 任何鼠标活动 (移动 / 点击) 都该调. 重置 tick + 把 bar 拉回可见.
	void NotifyUserActivity();
	// 16ms 定时器里调: 若 timeout 已到且面板不可见, 隐藏 bar.
	void EvalAutoHide();
	// 给 m_hEle / 控件栏 / 子控件 全挂上 XE_MOUSEMOVE -> NotifyUserActivity.
	// 抽出作公共 helper, 避免 CreateControlBar / CreateVolumePanel 重复挂事件逻辑.
	void HookMouseActivity(HELE h);
	// 强制重排 + 重绘 控件栏. 在以下路径调:
	//   - OnSizeImpl (m_hEle 尺寸变了)
	//   - bar 从隐藏->显示 (隐藏期间可能跳过 layout, 重新显示时位置/尺寸是 stale)
	//   - 任何外部 API 改变 bar 子节点属性
	// XCGUI 默认 XC_EnableAutoRedrawUI=FALSE -> 仅调 AdjustLayout 不会自动重绘, 需手动 Redraw.
	void ReflowControlBar();
	// 把秒数格式化成 "M:SS" 或 "H:MM:SS".
	static std::wstring FormatDuration(double seconds);

	// ===== 控件栏 事件 回调 =====
	// (OnLButtonUpCtrlBar 已删 - 控件栏默认鼠标穿透, 不需要点击处理)
	// Play / Loop 按钮是 button_type_check, 走 XE_BUTTON_CHECK (带 bCheck 参数);
	// Volume 按钮是默认 push 型, 走 XE_BNCLICK.
	int  OnBtnPlayCheck        (HELE hEle, BOOL bCheck, BOOL* pbHandled);
	int  OnBtnLoopCheck        (HELE hEle, BOOL bCheck, BOOL* pbHandled);
	int  OnBtnVolumeClick      (HELE hEle, BOOL* pbHandled);
	int  OnSliderProgressChange(HELE hEle, int pos, BOOL* pbHandled);
	int  OnSliderVolumeChange  (HELE hEle, int pos, BOOL* pbHandled);
	// XE_MOUSEMOVE 公共 handler: 所有挂了 HookMouseActivity 的元素共用,
	// 不区分来源, 都视作"用户活动".
	int  OnMouseMoveActivity   (HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled);

	// ===== 控件栏 自绘 (XE_PAINT, pbHandled=TRUE 完全接管) =====
	// 三个按钮 按 当前逻辑状态 选不同 SVG 画到中心:
	//   Play  : m_state==playing -> suspend.svg (表示按下这个就暂停); 否则 play.svg.
	//   Loop  : m_loop==TRUE     -> loop.svg     (已开循环);             否则 loop_close.svg.
	//   Volume: m_muted || m_volume<=0 -> voice_mute.svg;                  否则 voice.svg.
	// 布局公式与用户 spec 一致: x = rc.right/2 - w/2, y = rc.bottom/2 - h/2.
	int  OnPaintBtnPlay        (HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int  OnPaintBtnLoop        (HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int  OnPaintBtnVolume      (HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	// Slider 轨道 (进度 + 音量 共用): 画 35% 白 底 + #0099FF 填充.
	// 水平 vs 垂直 从 client rect 宽高比例推断. 垂直下填充从底朝上 (XSliderBar 0=底).
	int  OnPaintSlider         (HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	// Slider 滑块 (XSliderBar_GetButton 返回的子元素): 画 #F7F7F7 实心圆.
	int  OnPaintSliderThumb    (HELE hEle, HDRAW hDraw, BOOL* pbHandled);

	// 渲染分流: D2D 主路径 / GDI 兜底.
	void OnPaintD2D(ID2D1RenderTarget* rt, HDRAW hDraw);
	void OnPaintGdi(HDC hdc, HDRAW hDraw);
	// GDI 离屏 DIB 管理: w/h = 元素物理像素客户区. 与现有 m_gdiDibW/H 不一致就重建.
	// 同尺寸时复用. 失败返 false (调用方退出 OnPaintGdi).
	bool EnsureGdiDib(int w, int h);
	// OnDestroy / 元素销毁时释放 DIB / DC. 注意 SelectObject 顺序: 先 select 回原 bmp,
	// 再 DeleteObject DIB, 最后 DeleteDC. 漏哪一步都会泄漏 GDI handle.
	void ReleaseGdiDib();

	// DPI 同步: 读 XWnd_GetDPI 刷新 m_dpiScale.
	void RefreshDpiScale();
	// XEle_ClearBkInfo + AddBkFill, 让背景色与 XCGUI 标准管线协同.
	void RebuildBkInfo();
	// 把视频源像素映射到元素客户区物理像素的目标矩形 (按 m_fitMode 算).
	// eleW / eleH 是物理像素; 输出 pDst 也是物理像素.
	void ComputeDestRect(int eleW, int eleH, RECT* pDst) const;
	// 立即标记重绘.
	void RedrawSelf();

	// FFmpeg / WASAPI lifecycle.
	BOOL OpenInternal(const std::wstring& path);
	void CloseInternal();
	void StartThreads();
	void StopThreadsAndJoin();

	// 异步 Open 工作体. 由 Open() 在 m_thOpen 上 spawn; 完成后 成功转
	// state=stopped (或 保持用户不选心设的 playing), 失败转 state=error.
	void OpenWorkerFn(std::wstring path);

	// FFmpeg 中断回调: 让 avformat_open_input / find_stream_info / av_read_frame 能被
	// Close() / m_quit 取消. 不设的话, Close() 遇上正在超时重试的网络流会被卡到超时
	// 返回为止 (rtsp 默认 30s). 返 1 = 要求中断, 0 = 继续.
	static int AvInterruptCb(void* opaque);

	// 硬解上下文初始化. 在 avcodec_open2 之前调; 成功后置 m_hwAccelActive=true,
	// 装好 pVCtx->hw_device_ctx / get_format / opaque. 失败静默返 (调用方继续走软解).
	// pVCtx 由调用方传入, dec 是 codec, 不被持有.
	void TryInitHwAccel(AVCodecContext* pVCtx, const AVCodec* dec);
	// FFmpeg get_format 回调实例方法. 静态 trampoline 在 cpp 里通过 AVCodecContext::opaque
	// 拿到 this 后调本方法; 由于 trampoline 是 C-linkage 自由函数, 必须把它声明为 friend
	// 才能访问 *private* OnGetFormatImpl.
	// 候选格式里有 m_hwPixFmt 就返它, 否则回到第一个 SW fmt 让 codec 走软解.
	AVPixelFormat OnGetFormatImpl(AVCodecContext* s, const AVPixelFormat* fmts);
	friend enum AVPixelFormat XVideo_GetFormatTrampoline(struct AVCodecContext* s,
	                                                     const enum AVPixelFormat* fmts);

	// 4 个 worker 线程入口. 都是私有, ::std::thread 直接绑成员函数.
	void DemuxThreadFn();
	void VideoDecodeThreadFn();
	void AudioDecodeThreadFn();
	void AudioRenderThreadFn();

	// WASAPI 子流程.
	BOOL InitWasapi(int srcSampleRate, int srcChannels, AVSampleFormat srcFmt);
	void ShutdownWasapi();
	// 把一段 PCM 写到 device. blocking 直到全部写完或 m_quit. 返实际写入帧数.
	int  WriteWasapi(const uint8_t* pcm, int frames);

	// 帧调度: OnTimerImpl 里调, 按主时钟从 m_videoFrameQ 拉帧塞进 m_curBgra.
	void TryAdvanceFrame();
	// 主时钟: 有音频走 m_audioClock; 无音频走墙钟. 单调递增 (除非 seek).
	double GetMasterClock() const;
	// 把 audio decode 出来的帧应用音量 / 静音并写 WASAPI.
	void ApplyVolumeAndWrite(uint8_t* pcm, int frames);

	// 错误派发 (供 worker 线程调).
	void PostError(int code, const wchar_t* msg);
	// UI 线程 timer 里轮询 pending flag 并触发用户回调.
	void DispatchPendingCallbacks();

	// UTF-16 -> UTF-8 (FFmpeg avformat_open_input 需要 UTF-8 路径).
	static std::string WideToUtf8(const std::wstring& w);
	// AVERROR -> 中文友好串.
	static std::wstring AvErrToWStr(int err);

	// 取 video stream / audio stream 的 PTS 转秒. 容错 nopts.
	double VPtsToSec(int64_t pts) const;
	double APtsToSec(int64_t pts) const;
	//@隐藏}
};
//@分组}

#endif // XCGUI_VIDEO_H
