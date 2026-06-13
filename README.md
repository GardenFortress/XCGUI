# XCGUI 扩展模块

[炫彩界面库 (XCGUI)](https://www.xcgui.com) 的扩展模块集合。D2D 主路径 + GDI/GDI+ 兜底，兼容 Windows 7 SP1+。

## 模块

| 模块 | 类 | 用途 |
|---|---|---|
| [`module_xcgui_media`](./module_xcgui_media/) | `CXImageEx` / `CXVideo` | 图片 + 视频播放，共享 FFmpeg / 渲染内核 |
| [`module_xcgui_uitool`](./module_xcgui_uitool/) | `CXTooltip` / `CXLoading` / `CXCalendarCard` / `CXShadow` / `CXEditDW` / `CXBlur` / `CXChatBubbleBox` / `CXAccordion` | UI 工具集：提示、加载、日历、阴影、富文本编辑、亚克力、聊天气泡、折叠面板 |

旧独立头文件（`module_xcgui_image.h`、`module_xcgui_video.h`、`module_xcgui_editdw.h` 等）保留为兼容 shim，转发到新模块。

详细 API 见各 `.h` 文件中的 `//@别名` 注释；`module_xcgui_uitool/示例/` 提供完整 demo。

## 环境要求

- **C++ 标准：C++17**（`/std:c++17`）
- MSVC 2017+（推荐 VS 2022）
- Windows SDK ≥ 10.0.17763
- XCGUI ≥ 2025-12
- FFmpeg dev 包（仅 `module_xcgui_media` 需要；头文件见 `module_xcgui_media/ffmpeg/include`）

## 集成

`d2d1.h` 的 `POINTF` 与 XCGUI 内部同名，**必须先 include**：

```cpp
#include <d2d1.h>
#include <dwrite.h>                       // uitool 需要
#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"
#include "module_xcgui_uitool.h"          // 或 module_xcgui_media.h

XInitXCGUI(TRUE);                         // TRUE = D2D; FALSE = GDI+ 兜底
```

### 半透明背景文本抗锯齿

半透明 / 亚克力背景下，ClearType 与 alpha 混合异常，需使用灰度抗锯齿：

```cpp
XC_SetD2dTextAntialiasMode(2);            // 2 = 灰度抗锯齿
// 0 = 系统默认  1 = ClearType  2 = 灰度抗锯齿  3 = 不抗锯齿
```

`CXBlur` 构造时内部已自动调用 `XC_SetD2dTextAntialiasMode(2)`。

## 快速示例

### CXImageEx

```cpp
CXImageEx* pImg = new CXImageEx();
pImg->Create(0, 0, 400, 300, hParent);
pImg->LoadFromFile(L"D:\\test.avif");
```

### CXVideo

```cpp
CXVideo* pVideo = new CXVideo();
pVideo->Create(0, 0, 800, 600, hParent);
pVideo->Open(L"D:\\test.mp4");
pVideo->Play();
```

### CXEditDW

```cpp
CXEditDW* pEdit = new CXEditDW();
pEdit->Create(0, 0, 400, 300, hParent);
pEdit->SetText(L"Hello 💰 世界 😀");
```

### CXBlur

```cpp
CXBlur* pBlur = new CXBlur();
pBlur->AttachToWndEx(hWnd);               // Win10 1803+ DComp 路径; 不支持时退回 AttachToWnd
pBlur->SetTheme(xblur_theme_auto);
```

### CXShadow

```cpp
CXShadow* pShadow = new CXShadow();
pShadow->AttachToWnd(hWnd);
pShadow->SetCornerRadius(8);
```

### CXTooltip / CXLoading / CXCalendarCard

```cpp
CXTooltip::AddEleTip(hBtn, L"保存当前文档");
CXLoading::AttachEle(hPanel);
CXLoading::SetStyle(hPanel, xloading_style_spinner);
CXCalendarCard::PopupSingle(hWnd, &date, TRUE, xcalendar_theme_auto);
```

### CXChatBubbleBox

```cpp
CXChatBubbleBox* pChat = new CXChatBubbleBox();
pChat->Create(0, 0, 480, 600, hParent);
pChat->SetInsertType(chat_insert_type_receiver);
pChat->InsertBubbleBegin();
pChat->InsertText(L"你好");
pChat->InsertBubbleEnd();
```

### CXAccordion

FAQ / 设置分组 / 引导清单用折叠面板，支持分组、图标、徽章、文本或元素内容、展开动画与 `xuitool_theme_` 主题。

```cpp
CXAccordion* pAcc = new CXAccordion();
pAcc->Create(hWnd);
pAcc->SetTheme(xuitool_theme_auto);
int g = pAcc->AddGroup(L"常见问题");
int item = pAcc->AddItem(g, L"如何开始使用?", xaccordion_content_text);
pAcc->SetItemBodyText(item, L"创建账号后按引导完成基础设置即可。");
pAcc->AdjustLayout();
pAcc->ExpandItem(item, TRUE);
// 关闭前: pAcc->DestroyAccordion(); delete pAcc;
```

完整 API 与事件示例见 [`module_xcgui_uitool/示例/demo_accordion.cpp`](./module_xcgui_uitool/示例/demo_accordion.cpp)。

## 兼容性

| 系统 | media | uitool |
|---|---|---|
| Win11 / Win10 1803+ | D2D + 硬解 | D2D + DComp 亚克力 |
| Win10 1607~1709 | D2D + 硬解 | D2D + BLURBEHIND |
| Win7 SP1 | D2D/GDI+ 降级 + 软解 | D2D/GDI+ 兜底 |

> Win7 须使用 **FFmpeg 4.4.x**（5.x+ 含 `WaitOnAddress`，Win7 加载即崩）。

## 运行时 DLL

仅 `module_xcgui_media` 依赖 FFmpeg。**DLL 不随仓库分发**，请自行下载后放到 `module_xcgui_media/ffmpeg/bin/`（与工程 `@复制文件` 路径一致）。

```
avcodec-*.dll  avformat-*.dll  avutil-*.dll
swresample-*.dll  swscale-*.dll  avdevice-*.dll
```

- 新版 FFmpeg：<https://www.gyan.dev/ffmpeg/builds/>
- Win7 须用 **4.4.x**：<https://github.com/GyanD/codexffmpeg/releases/tag/4.4.3>

## 许可证

[Unlicense](LICENSE)

## 作者

**未闻花名** · QQ 936599025
