# XCGUI 扩展模块

[炫彩界面库 (XCGUI)](https://www.xcgui.com) 的扩展模块集合。D2D 主路径 + GDI/GDI+ 兜底，兼容 Windows 7 SP1+。

## 模块

| 模块 | 类 | 用途 |
|---|---|---|
| [`module_xcgui_media`](./module_xcgui_media/) | `CXImageEx` / `CXVideo` | 图片 + 视频播放，共享 FFmpeg / 渲染内核 |
| [`module_xcgui_uitool`](./module_xcgui_uitool/) | `CXTooltip` / `CXNotify` / `CXLoading` / `CXCalendarCard` / `CXShadow` / `CXEditDW` / `CXBlur` / `CXChatBubbleBox` / `CXAccordion` / `CXCardPanel` / `CXSteps` / `CXColorPicker` / `CXCheckAnim` | UI 工具集：提示、系统/炫彩通知、加载、日历、阴影、富文本编辑、亚克力、聊天气泡、折叠面板、卡片面板、步骤条、颜色选择器、多选框动画 |

旧独立头文件（`module_xcgui_image.h`、`module_xcgui_video.h`、`module_xcgui_editdw.h` 等）保留为兼容 shim，转发到新模块。

`module_xcgui_uitool` 内 **CXTooltip / CXNotify / CXLoading / CXCalendarCard / CXShadow / CXBlur / CXAccordion / CXCardPanel / CXSteps / CXColorPicker / CXCheckAnim** 共用 `xuitool_theme_`（深/浅/自定义/跟随系统）；旧 `xshadow_theme_*`、`xblur_theme_*` 已移除。

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

各模块完整用法见 [`module_xcgui_uitool/示例/`](./module_xcgui_uitool/示例/) 下对应 `demo_*.cpp`。

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
pBlur->AttachToWndEx(hWnd, xblur_path_auto);   // Win10 1803+ DComp; 不支持时自动降级
pBlur->SetTheme(xuitool_theme_auto);
// 关闭前: pBlur->Detach(); delete pBlur;
```

### CXShadow

```cpp
CXShadow* pShadow = CXShadow::Create();
pShadow->AttachToWnd(hWnd);
pShadow->SetCornerRadius(10);
pShadow->SetTheme(xuitool_theme_auto);
// 关闭前: pShadow->Detach(); CXShadow::Destroy(pShadow);
```

### CXTooltip

```cpp
CXTooltip::AddEleTip(hBtn, L"保存当前文档");
CXTooltip::SetType(hBtn, xtooltip_type_success);
CXTooltip::SetTheme(hBtn, xuitool_theme_auto);
```

### CXNotify

`ShowTray` 应在 `XTrayIcon_Add()` 成功后调用。Win10/11 优先提交系统通知；系统提交失败或老系统自动降级为右下角 XCGUI 非模态通知窗。

```cpp
XTrayIcon_Reset();
XTrayIcon_SetTips(L"应用名称");
if (XTrayIcon_Add(hWnd, 43001)) {
    CXNotify::ShowTray(hWnd, L"应用名称",
        L"程序仍在后台运行，点击托盘图标可以恢复主窗口。",
        xuitool_theme_auto, 4500);
}
// XExitXCGUI() 前:
CXNotify::Cleanup();
```

### CXLoading

```cpp
CXLoading::AttachEle(hPanel);              // AttachEle 自动继承宿主元素字体族名与字号
CXLoading::SetStyle(hPanel, xloading_style_spinner);
CXLoading::SetTheme(hPanel, xuitool_theme_dark);
CXLoading::Start(hPanel);
```

### CXCalendarCard

```cpp
xcalendar_datetime_ date = CXCalendarCard::GetToday();
CXCalendarCard::SetBindEle(hBtn, 0, 4);
if (CXCalendarCard::PopupSingle(hWnd, &date, TRUE, xuitool_theme_auto, NULL, 10)) {
    // date 已更新
}
```

### CXChatBubbleBox

```cpp
CXChatBubbleBox* pChat = new CXChatBubbleBox();
pChat->Create(0, 0, 480, 600, hParent);
pChat->SetInsertType(chat_insert_type_receiver);
pChat->InsertBubbleBegin();
pChat->InsertText(L"你好");
pChat->InsertBubbleEnd();
// 关闭前: pChat->DestroyChat(); delete pChat;
```

### CXAccordion

FAQ / 设置分组 / 引导清单用折叠面板，支持分组、图标、徽章、文本或元素内容、展开动画与 `xuitool_theme_` 主题。

```cpp
CXAccordion* pAcc = new CXAccordion();
pAcc->Create(hWnd);
pAcc->SetTheme(xuitool_theme_light);
pAcc->SetExpandMode(xaccordion_expand_mode_single);
int g = pAcc->AddGroup(L"常见问题");
int item = pAcc->AddItem(g, L"如何开始使用?", xaccordion_content_text);
pAcc->SetItemBodyText(item, L"创建账号后按引导完成基础设置即可。");
pAcc->AdjustLayout();
pAcc->ExpandItem(item, FALSE);
// 关闭前: pAcc->DestroyAccordion(); delete pAcc;
```

详见 [`demo_accordion.cpp`](./module_xcgui_uitool/示例/demo_accordion.cpp)。

### CXCardPanel

设置页风格卡片面板，支持分组标题、圆角卡片、开关行与内容区布局。

```cpp
CXCardPanel* pPanel = new CXCardPanel();
if (pPanel->Create(hWnd)) {
    pPanel->SetTheme(xuitool_theme_light);
    int g = pPanel->AddGroup(L"系统设置");
    pPanel->SetGroupContentEle(g, hToggleRow);   // 单内容; 多项连续添加用 AddGroupContentEle
    pPanel->AdjustLayout();
}
// 关闭前: pPanel->DestroyCardPanel(); delete pPanel;
```

详见 [`demo_cardpanel.cpp`](./module_xcgui_uitool/示例/demo_cardpanel.cpp)。

### CXSteps

水平/垂直步骤条，支持深浅主题、标签顺序调换与切换动画。

```cpp
CXSteps* pSteps = new CXSteps();
if (pSteps->Create(hWnd)) {
    pSteps->SetTheme(xuitool_theme_dark);
    pSteps->SetOrientation(xsteps_orient_horizontal);
    pSteps->SetContentOrder(xsteps_content_label_first);
    pSteps->SetAnimEnabled(TRUE);
    pSteps->AddStep(L"Register");
    pSteps->AddStep(L"Choose plan");
    pSteps->SetCurrentStep(1);
    pSteps->AdjustLayout();
}
// 关闭前: pSteps->DestroySteps(); delete pSteps;
```

详见 [`demo_steps.cpp`](./module_xcgui_uitool/示例/demo_steps.cpp)。

### CXColorPicker

现代颜色选择器，支持 RGBA/HEX/HSL、吸管取色、实时预览与元素绑定；可选非模态弹出、拖动窗口与置顶。

```cpp
xcolor_rgba_ color = { 255, 0, 0, 255 };
CXColorPicker::SetBindEle(hBtn, 0, 6);              // 绑定保持到下次 SetPopupPosition 或重新 SetBindEle
CXColorPicker::SetEnableModal(FALSE);               // 非模态，父窗口仍可操作
CXColorPicker::SetEnableDrag(TRUE);                 // 允许拖动选择器窗口
CXColorPicker::SetEnableTopmost(TRUE);              // 置顶显示
if (CXColorPicker::Popup(hWnd, &color, TRUE, xuitool_theme_auto, 10, TRUE, xcolor_input_hex)) {
    // color 已更新
}
CXColorPicker::SetEnableModal(TRUE);                // 恢复默认
CXColorPicker::SetEnableDrag(FALSE);
CXColorPicker::SetEnableTopmost(FALSE);
```

详见 [`demo_colorpicker.cpp`](./module_xcgui_uitool/示例/demo_colorpicker.cpp)。

### CXCheckAnim

WinUI3 风格 Toggle 多选框动画，附加到已有 `XC_BUTTON` 按钮，支持深/浅主题与文本左右对齐。

```cpp
HELE hBtn = XBtn_Create(16, 40, 200, 20, L"启用通知", hParent);
XUI_EnableCSS(hBtn, FALSE);
XBtn_SetCheck(hBtn, TRUE);
CXCheckAnim::AttachBtn(hBtn, 0, 20, xuitool_theme_dark);
CXCheckAnim::SetTextAlign(hBtn, xcheckanim_text_align_right);
CXCheckAnim::SetAnimEnabled(hBtn, TRUE);
// 进程退出前: CXCheckAnim::Cleanup();
```

详见 [`demo_checkanim.cpp`](./module_xcgui_uitool/示例/demo_checkanim.cpp)。

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
- Win7 须用 **4.4.x**：<https://github.com/GyanD/codexffmpeg/releases/tag/4.4.1>

## 许可证

[Unlicense](LICENSE)

## 作者

**未闻花名** · QQ 936599025
