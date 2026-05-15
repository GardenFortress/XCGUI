# XCGUI 模块扩展库

基于 [炫彩界面库 (XCGUI)](https://www.xcgui.com) 的高质量扩展模块集合，提供 **DirectWrite 富文本编辑器**、**FFmpeg 视频播放器**、**FFmpeg 增强图片元素** 和 **DWM 亚克力磨砂玻璃元素**。模块都做了 D2D 主路径 + GDI/GDI+ 兜底，**兼容 Windows 7 SP1+ 无 GPU 虚拟机环境**。

---

## 模块列表

### `module_xcgui_editdw` —— DirectWrite 编辑框
| | |
|---|---|
| **类名** | `CXEditDW`（继承 `CXScrollView`） |
| **别名** | 炫彩DW编辑框类 |
| **依赖** | `d2d1.lib` / `dwrite.lib` / `user32.lib`（全部 Windows SDK 自带） |

**特性**

- **D2D + DirectWrite** 文本布局，渲染质量优于 GDI ExtTextOut
- **彩色 Emoji**：Segoe UI Emoji 自动回退，支持 SMP 代理对（💰 U+1F4B0 / 😀 U+1F600）真实码位渲染
- **分段 layout 架构**：每段一个 `IDWriteTextLayout`，200K+ 字符仍可流畅编辑（单 layout 全文 shape 在大文本下要 1s+，分段后改一段只 1~5ms）
- **完整编辑能力**：鼠标拖选 / 双击选词 / Shift+方向键扩选 / 自绘闪烁光标 / 撤销重做 / 1:1 文本复制粘贴
- **样式系统**：字符级字体 / 颜色 / 图片 / UI 对象嵌入
- **D2D → GDI 兜底**：无 GPU 时自动降级 GDI 渲染（VMware Win7 默认配置可用）

### `module_xcgui_video` —— FFmpeg 视频播放器
| | |
|---|---|
| **类名** | `CXVideo`（继承 `CXEle`） |
| **别名** | 炫彩视频播放器类 |
| **依赖** | FFmpeg dev 包（4.x ~ 8.x 任一）、Windows SDK |

**特性**

- **解码**：FFmpeg `libavformat` + `libavcodec` + `libswscale` + `libswresample`
- **版本兼容**：FFmpeg 4.x / 5.x / 6.x / 7.x / 8.x 头文件全部可编译（用 `LIBAVCODEC_VERSION_INT` 做 API 分支，新旧 `channel_layout` API 都支持）
- **硬解**：D3D11VA / DXVA2，失败自动回退软解
- **音频**：WASAPI 共享模式输出
- **渲染**：D2D 主路径（`ID2D1Bitmap` 上传 + `DrawBitmap`）+ GDI+ 离屏 DIB 降级路径（`StretchDIBits` to memory DIB → 单次 `BitBlt` 上屏，解决偏移和抽搐）
- **4 线程架构**：demux / 视频解码 / 音频解码 / 音频渲染，全部用 `BoundedQueue` 限流（带 `PushTimeout` 防止 resize 卡死）
- **A/V 同步**：音频时钟为主时钟，在 `XE_XC_TIMER` (16ms) 里推视频帧上屏
- **DPI 自适应**：跟随 XCGUI DPI 缩放系统
- **抗阻塞**：网络流 / debug heap 慢分配场景下，靠 `interrupt_callback` 让 `avformat_open_input` / `av_read_frame` 在几十毫秒内中断退出

### `module_xcgui_image` —— FFmpeg 增强图片元素
| | |
|---|---|
| **类名** | `CXImageEx`（继承 `CXEle`） |
| **别名** | 炫彩增强图片类 |
| **依赖** | FFmpeg dev 包（4.x ~ 8.x 任一)、Windows SDK |

**特性**

- **格式支持**：静态 JPG / PNG / BMP / WEBP / HEIC / AVIF / TIFF / JPEG2000 等，动态 GIF / APNG / WEBP-anim / AVIF-anim
- **高质量缩放**：显示阶段用 `swscale` 一步缩放，支持 Nearest / Bilinear / Bicubic / Lanczos，默认静态图 Lanczos、动态图 Bilinear
- **动画时长**：从 FFmpeg packet duration 提取真实 per-frame duration，不再固定 33ms
- **渲染**：D2D 主路径（`ID2D1Bitmap` 上传 + `DrawBitmap`）+ GDI+ 离屏 DIB 降级路径
- **内存策略**：加载阶段保留源 `AVFrame`，显示阶段只缩放当前帧，避免多帧动图预转 BGRA 导致内存爆炸
- **控制接口**：播放 / 暂停 / 停止 / seek frame / loop / fit mode / interpolation / 背景色 / 加载完成与错误回调

### `module_xcgui_blur` —— DWM 亚克力磨砂玻璃元素
| | |
|---|---|
| **类名** | `CXBlur`（继承 `CXEle`） |
| **别名** | 炫彩亚克力模糊类 |
| **依赖** | `d2d1.lib` / `gdiplus.lib` / `dwmapi.lib` / `user32.lib`（全部 Windows SDK 自带） |

**特性**

- **真 DWM acrylic 后景**：调 `SetWindowCompositionAttribute(ACCENT_ENABLE_ACRYLICBLURBEHIND)` 让系统合成器在 host 客户区下层做 backdrop + blur，**跟手 0 帧延迟，无抓帧**
- **元素级 alpha 控制**：CXBlur 元素之外的内容默认 alpha=255 完全覆盖 acrylic，仅 element 区用 D2D `D2D1_PRIMITIVE_BLEND_COPY` 写 alpha=tint.alpha 透出 backdrop（经典 Win11 InAppAcrylic 实现）
- **三种绑定模式**：`Create` 自有元素 / `AttachToEle` 接管已存在元素 / `AttachToWnd` 整窗背板
- **per-corner 圆角**：`SetCornerRadiusEx(左上, 右上, 右下, 左下)` 四角独立配置，可做"半圆角矩形" / "上圆下方"
- **主题预设**：light / dark / auto（跟随系统）/ custom，全局 `SetGlobalTheme` 同步所有活实例
- **D2D 噪点层**：`D2D1Turbulence → Saturation → ColorMatrix` 三级 effect graph 出真砂纸感
- **OS 自动降级**：Win10 1607~1709 → BLURBEHIND，Win7/Win8/8.1 → 仅装饰层（tint+border+圆角），不报错
- **GDI / D2D 双路径**：GDI 模式不挂 ACCENT（避免 alpha halo 伪影），自动退化为装饰层
- **进程退出还原**：`atexit` + `Console Ctrl handler` 双兜底，正常退出时还原系统状态

---

## 兼容性矩阵
| 系统 | editdw | video | image | blur | 备注 |
|---|---|---|---|---|---|
| Windows 11 / 10 1803+ | ✅ D2D 渲染 | ✅ D2D + 硬解 | ✅ D2D | ✅ ACCENT_ACRYLIC | 推荐配置 |
| Windows 10 1607 ~ 1709 | ✅ D2D 渲染 | ✅ D2D + 硬解 | ✅ D2D | ✅ ACCENT_BLURBEHIND（无 tint） |  |
| Windows 8 / 8.1 | ✅ D2D 渲染 | ✅ D2D + 硬解 | ✅ D2D | ⚠️ 仅装饰层（tint+border） |  |
| Windows 7 SP1（有 GPU 驱动） | ✅ D2D | ✅ D2D + 软解 | ✅ D2D | ⚠️ 仅装饰层 | DXVA2 可能可用 |
| Windows 7 SP1（无 GPU 驱动，VMware 默认） | ✅ GDI 降级 | ✅ GDI+ 降级 + 软解 | ✅ GDI+ 降级 | ⚠️ 仅装饰层 | 必须用 ffmpeg-4.4 DLL |

> ⚠️ **Win7 必须用 FFmpeg 4.4.x**。FFmpeg 5+ 引入了 `WaitOnAddress` 等 Win8+ API，Win7 上 `LoadLibrary` 会 `0xC0000005` 闪退。本仓库的 `@复制文件` 注解默认指向 4.4.x DLL（`avcodec-58.dll` 等）。

---

## 运行时 DLL 清单
**`module_xcgui_editdw`** —— 无外部 DLL（`dwrite.dll` Win7 SP1+ 系统自带）

**`module_xcgui_blur`** —— 无外部 DLL（`dwmapi.dll` / `user32.dll` 系统自带）

**`module_xcgui_video` / `module_xcgui_image`**（默认 FFmpeg 4.4.x，Win7 兼容）

```
avcodec-58.dll
avdevice-58.dll
avformat-58.dll
avutil-56.dll
swresample-3.dll
swscale-5.dll
```

下载地址（Win7 可用的 4.4.3 shared build）：<https://github.com/GyanD/codexffmpeg/releases/tag/4.4.3>

> 如果只跑 Win10/11 且不需要 AV1/VVC，**用 4.4.x 是最优选择**：性能差距 ≤ 5%，DLL 体积少 20MB+，LTS 更稳。

---

## ⚠️ 重要警告：`CXBlur::ForceSystemTransparencyOn` 入侵性注册表改动

`module_xcgui_blur` 提供了一个 **修改 Windows 系统注册表** 的工具方法，用来在用户关掉 *个性化 - 颜色 - 透明效果* 时仍能让 acrylic 出真 blur。**因为它是用户级系统设置（写下去整个用户帐户的所有 app 都生效），这里特别醒目地标注**：

| 项 | 说明 |
|---|---|
| **作用范围** | **HKCU 当前用户级**，写下去后该用户所有 app 都用上透明效果（**不只你的程序**） |
| **注册表路径** | `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\EnableTransparency` |
| **写入值** | `REG_DWORD = 1` |
| **管理员权限** | 不需要（HKCU 当前用户即可） |
| **OS 适用** | 仅 Win10 1803+ / Win11（老系统没 acrylic） |

### 默认 = 关闭 ✅

```cpp
CXBlur::ForceSystemTransparencyOn();          // = ForceSystemTransparencyOn(FALSE)，默认 no-op
```

**默认调用什么也不做**，必须显式传 `TRUE` 才会改注册表。**这是默认行为，避免无意中污染用户系统**。

### 启用（写注册表 = 1）

```cpp
// 建议放在 XInitXCGUI 之后，创建第一个 CXBlur 之前
CXBlur::ForceSystemTransparencyOn(TRUE);
```

调用后：
1. 保存原值（老值已是 1 时不写也不还原，用户本来就开着）
2. 写入 `EnableTransparency = 1`
3. 仅向 **本进程顶层窗口** 广播 `WM_SETTINGCHANGE("ImmersiveColorSet")`（避免打扰其他 app）
4. 注册 `atexit` + `SetConsoleCtrlHandler` 双兜底，**进程正常退出时自动还原老值**

### 关闭 / 还原

```cpp
CXBlur::ForceSystemTransparencyOn(FALSE);     // 立即还原老值，广播本进程
```

或 **进程正常退出**（atexit 自动跑）：
- ✅ `wWinMain` 返回
- ✅ `XExitXCGUI` 后退出
- ✅ Ctrl-C / Ctrl-Break / 控制台关闭
- ✅ `std::exit` / `_exit`

### 还原失败的情况（无解）

- ❌ 任务管理器 → 结束任务（`TerminateProcess`）
- ❌ 蓝屏 / 强制断电
- ❌ 调试器中止

→ 这些路径下 atexit 和 Ctrl handler 都不会跑，注册表会停留在 `1`。下次启动应用再调一次 `(TRUE)` 会重新跟踪当前值，影响很小。

### EULA 提示建议

如果你的产品调用了 `ForceSystemTransparencyOn(TRUE)`，**请务必在用户协议 / 启动提示里告知**：本程序会临时修改系统 *透明效果* 开关（用户级，对当前用户全局生效），进程正常退出时自动还原。

---

## 使用方法

### 1. 加入 XCGUI 工程
把 `module_xcgui_editdw.{h,cpp}`、`module_xcgui_video.{h,cpp}`、`module_xcgui_image.{h,cpp}` 和/或 `module_xcgui_blur.{h,cpp}` 加入炫彩 IDE 工程。模块头文件顶部的 `@依赖` / `@复制文件` 注解会被炫彩 IDE 解析（自动 include / 自动复制 DLL）。

### 2. 头文件 include 顺序约束
由于 D2D 的 `POINTF` 和 XCGUI 内部的 `POINTF` 同名冲突，**必须先 include `d2d1.h` 抢占符号，再 include XCGUI 系列头**：

```cpp
#include <d2d1.h>           // 先
#include <dwrite.h>         // 仅 editdw 需要
#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"
#include "module_xcgui_editdw.h"  // 或 video / image / blur
```

### 3. 选择渲染后端
```cpp
XInitXCGUI(TRUE);   // D2D 主渲染（Win8+ 或 Win7 有 GPU），blur 模块出真 acrylic
// 或
XInitXCGUI(FALSE);  // GDI+ 渲染（Win7 无 GPU / 虚拟机 / 老硬件），blur 模块退化为装饰层
```

四个模块都自动适配两种后端，**业务代码不用改**。

### 4. 实例化
```cpp
// editdw
CXEditDW* pEdit = new CXEditDW();
pEdit->Create(0, 0, 400, 300, hParent);
pEdit->SetText(L"Hello 💰 世界 😀");

// video
CXVideo* pVideo = new CXVideo();
pVideo->Create(0, 0, 800, 600, hParent);
pVideo->Open(L"D:\\test.mp4");
pVideo->Play();

// image
CXImageEx* pImg = new CXImageEx();
pImg->Create(0, 0, 400, 300, hParent);
pImg->SetFitMode(ximage_fit_contain);
pImg->SetInterpolation(ximage_interp_lanczos);
pImg->SetLoop(TRUE);
pImg->LoadFromFile(L"D:\\test.avif");

// blur (整窗 acrylic 背板)
CXBlur* pBlur = new CXBlur();
pBlur->AttachToWnd(hWnd);
pBlur->SetTheme(xblur_theme_auto);                     // 跟随系统亮 / 暗
pBlur->SetCornerRadiusEx(16, 16, 0, 0);                // 上圆下方
// 可选: 强制开启系统透明效果 (会改 HKCU 注册表, 见上方警告)
// CXBlur::ForceSystemTransparencyOn(TRUE);
```

详细 API 见各 `.h` 文件中的 `//@注释`（中文友好，配合炫彩 IDE 智能感知体验最佳）。

---

## 编译环境
- **MSVC 2015 +**（推荐 VS 2019 / 2022）
- **Windows SDK** ≥ 10.0.17763
- **XCGUI 界面库** ≥ 2025-12 版本（`module_xcgui_video` 需要 `XDraw_ConvRect` 处理 GDI+ 画布坐标）
- **FFmpeg dev 包**（`video` / `image` 模块需要）：把 `include/` / `lib/` 路径加到工程，`bin/` 下的 DLL 放到 EXE 同目录

---

## 设计亮点
- **D2D / GDI 双路径都打磨过**：不是简单的 "走不通就 throw"，是两条路径都仔细对齐了渲染细节（GDI+ 模式下用 `XDraw_ConvRect` 解决画布偏移，离屏 DIB 解决抽搐）
- **大文本 / 媒体性能**：editdw 分段 layout + 懒构建，video 限流队列 + 帧丢弃策略，image 显示阶段只缩当前帧
- **可中断 I/O**：FFmpeg `interrupt_callback` 配合 `m_quit` 原子量，网络流 30s 超时被压到几十毫秒退出
- **零运行时崩溃**：硬解失败 / GPU 驱动缺失 / DPI 变化 / 窗口 resize 全部走 fallback，不抛异常
- **DWM 系统合成路径**：blur 模块走 `SetWindowCompositionAttribute` 让 DWM 在合成 pipeline 内做 backdrop + blur，永远跟手 0 延迟，无抓帧；与 NTQQ / 微信 / 钉钉 等所有主流 IM 的 acrylic 是同一套机制

---

## 许可证
[Unlicense](LICENSE) —— 公共领域，商用 / 修改 / 转发都自由。

---

## 致谢
- [炫彩界面库 (XCGUI)](https://www.xcgui.com)
- [FFmpeg](https://ffmpeg.org)
- Microsoft DirectWrite / Direct2D / WASAPI / DWM

---

## 作者
**未闻花名** · QQ: 936599025
