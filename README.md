# XCGUI 模块扩展库

[炫彩界面库 (XCGUI)](https://www.xcgui.com) 的扩展模块集合. D2D 主路径 + GDI/GDI+ 兜底, 兼容 Windows 7 SP1+.

## 模块

| 模块 | 类 | 用途 |
|---|---|---|
| [`module_xcgui_editdw`](./module_xcgui_editdw/) | `CXEditDW : CXScrollView` | DirectWrite 富文本编辑框, 彩色 Emoji, 分段 layout 支持 200K+ 字符 |
| [`module_xcgui_video`](./module_xcgui_video/) | `CXVideo : CXEle` | FFmpeg 视频播放器, D3D11VA/DXVA2 硬解, WASAPI 音频 |
| [`module_xcgui_image`](./module_xcgui_image/) | `CXImageEx : CXEle` | FFmpeg 图片元素, 静/动态 jpg/png/webp/heic/avif/gif/apng 等 |
| [`module_xcgui_blur`](./module_xcgui_blur/) | `CXBlur : CXEle` | DWM 亚克力 (`ACCENT_ACRYLIC`), 元素级 alpha 控制, per-corner 圆角 |
| [`module_xcgui_shadow`](./module_xcgui_shadow/) | `CXShadow` | Win11 风格窗口外阴影 + AA 圆角描边 + 圆角内圈背景, DPI 自适应, 不占客户区 |
| [`module_xcgui_uitool`](./module_xcgui_uitool/) | `CXTooltip` / `CXLoading` | UI 工具集: 悬停气泡提示 (5 种语义 + 三角指针 + 渐显渐隐), 加载动画 (5 风格 + 元素/窗口附加) |

详细 API 见各 `.h` 文件中的 `//@别名` 注释.

## 兼容性

| 系统 | editdw / image | video | blur | shadow | uitool |
|---|---|---|---|---|---|
| Win11 / Win10 1803+ | D2D | D2D + 硬解 | **ACCENT_ACRYLIC** | D2D + GDI+ DIB | D2D |
| Win10 1607~1709 | D2D | D2D + 硬解 | ACCENT_BLURBEHIND | D2D + GDI+ DIB | D2D |
| Win8 / 8.1 | D2D | D2D + 硬解 | 装饰层 | D2D + GDI+ DIB | D2D |
| Win7 SP1 (有 GPU) | D2D | D2D + 软解 | 装饰层 | D2D + GDI+ DIB | D2D |
| Win7 SP1 (VMware 默认) | GDI 降级 | GDI+ 降级 + 软解 | 装饰层 | GDI+ DIB | GDI+ 兜底 |

> Win7 必须用 **FFmpeg 4.4.x**. 5.x+ 引入 Win8 API `WaitOnAddress`, 在 Win7 上加载即崩.

## 运行时 DLL

仅 `module_xcgui_video` / `module_xcgui_image` 依赖 FFmpeg, 其余模块均走 Windows SDK 自带 DLL.

```
avcodec-58.dll  avformat-58.dll  avutil-56.dll
swresample-3.dll  swscale-5.dll  avdevice-58.dll
```

Win7 兼容包: <https://github.com/GyanD/codexffmpeg/releases/tag/4.4.3>

## 集成

D2D 的 `POINTF` 与 XCGUI 内部 `POINTF` 同名, **`d2d1.h` 必须先 include**:

```cpp
#include <d2d1.h>
#include <dwrite.h>                       // editdw 需要
#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"
#include "module_xcgui_editdw.h"          // 按需 include: editdw / video / image / blur / shadow / uitool

XInitXCGUI(TRUE);                         // D2D 主渲染; FALSE = GDI+ 兜底
```

### `CXEditDW`

```cpp
CXEditDW* pEdit = new CXEditDW();
pEdit->Create(0, 0, 400, 300, hParent);
pEdit->SetText(L"Hello 💰 世界 😀");
```

### `CXVideo`

```cpp
CXVideo* pVideo = new CXVideo();
pVideo->Create(0, 0, 800, 600, hParent);
pVideo->Open(L"D:\\test.mp4");
pVideo->Play();
```

### `CXImageEx`

```cpp
CXImageEx* pImg = new CXImageEx();
pImg->Create(0, 0, 400, 300, hParent);
pImg->SetFitMode(ximage_fit_contain);
pImg->SetInterpolation(ximage_interp_lanczos);
pImg->SetLoop(TRUE);
pImg->LoadFromFile(L"D:\\test.avif");
```

### `CXBlur`

```cpp
CXBlur* pBlur = new CXBlur();
pBlur->AttachToWnd(hWnd);
pBlur->SetTheme(xblur_theme_auto);
pBlur->SetCornerRadiusEx(16, 16, 0, 0);   // per-corner: 左上 / 右上 / 右下 / 左下
// 可选: 强制启用系统透明效果 (会写 HKCU 注册表, 详见下文)
// CXBlur::ForceSystemTransparencyOn(TRUE);
```

### `CXShadow`

```cpp
CXShadow* pShadow = new CXShadow();
pShadow->AttachToWnd(hWnd);                  // 接管 padding / 透明 / WM_PAINT / NCHITTEST
pShadow->SetTheme(xshadow_theme_auto);       // 浅/深 跟随系统
pShadow->SetCornerRadius(8);                 // 圆角逻辑像素 @96 DPI
// 可选: 调阴影几何
// pShadow->SetShadowRadius(24);             // blur
// pShadow->SetShadowOffset(0, 6);           // key 层 dy
// pShadow->SetBorderWidth(1.0f);            // 1px AA 描边
```

- AttachToWnd 之外不需任何前置步骤 (透明属性 / padding / 圆角内圈填充由本类负责)
- 不要再给主窗设 `XWnd_SetRound` / `SetWindowRgn` (HRGN 1-bit 必锯齿, 与 AA 描边冲突)
- 最大化 / aero snap 自动 ClearPadding 隐藏阴影, 还原后自动恢复
- `WM_GETMINMAXINFO` 自动把 padding 加到 ptMinTrackSize, 用户配置的最小尺寸不被阴影框吃掉

### `CXTooltip`

鼠标悬停气泡提示 — 全 static API, 内部维护单例共享气泡窗口 + 全局源元素注册表, 同一时刻至多 1 个气泡.

```cpp
// 最简: 给一个按钮挂提示
CXTooltip::AddEleTip(hBtn, L"保存当前文档");

// 带语义图标 + 浅色主题 + 强制下方弹出
CXTooltip::AddEleTipEx(hBtn, L"操作成功",
    xtooltip_type_success,
    xtooltip_theme_light);
CXTooltip::SetArrowSide(hBtn, xtooltip_arrow_side_bottom);

// 行为调优 (全局, 不分元素)
CXTooltip::SetShowDelay(300);    // 鼠标停留多久后弹 (ms)
CXTooltip::SetAutoCloseMs(3000); // 弹出后多久自动消失 (0 = 不自动)
CXTooltip::SetFadeMs(150);       // 渐显/渐隐时长

// 退出前一次清理 (反挂所有源元素事件 + 销毁气泡窗口)
CXTooltip::Cleanup();
```

- 弹出窗口设 `WS_EX_TRANSPARENT` 鼠标穿透 + `XWnd_EnableNcaActive(FALSE)` 不抢焦点
- `xtooltip_arrow_side_auto` (默认): 按鼠标进入源元素哪一边自动选反向; 4 个固定枚举强制锚定
- 源元素销毁时自动反注册, 不需手动 `DelEleTip`

### `CXLoading`

加载动画 / 旋转指示器 — 3 种宿主形态 × 5 种动画风格 × 4 套主题.

```cpp
// 形态 1: 元素附加 (在已有元素上覆盖渲染 loading)
CXLoading::AttachEle(hPanel);
CXLoading::SetStyle(hPanel, xloading_style_spinner);
CXLoading::SetText (hPanel, L"加载中...");
// ...
CXLoading::Stop  (hPanel);   // 让出 paint, 宿主原内容恢复显示
CXLoading::Detach(hPanel);   // 完全反附加, 反挂事件

// 形态 2: 窗口附加 (建子元素填满窗口客户区, 自动置顶 + layout_fill)
CXLoading::AttachWnd(hWnd);  // 会减 XWnd_SetPadding 的内填充; resize 自适应
CXLoading::SetStyle (hWnd, xloading_style_bars);
CXLoading::SetTheme (hWnd, xloading_theme_auto);
// ...
CXLoading::Stop  (hWnd);     // 自建子元素被 XWidget_Show(FALSE) 隐藏, 不遮窗口内容
CXLoading::Start (hWnd);     // 再次显示 + 重启动画
CXLoading::Detach(hWnd);     // 销毁内部子元素, 完全释放

// 形态 3: 自建元素 (返回 HELE, 用户自行控制坐标, 适合做模态遮罩)
HELE hLoad = CXLoading::Create(0, 0, 200, 200, hParent);
CXLoading::SetSize         (hLoad, 60, 60);
CXLoading::SetCornerRadius (hLoad, 12);
CXLoading::SetAccentColor  (hLoad, RGB(0xFF, 0x99, 0x00));
```

- 5 风格: `spinner` (圆环 ease 振荡) / `dots` (5 点错峰旋转) / `spokes` (12 辐条衰减) / `pulse` (单圈脉冲) / `bars` (4 胶囊条波动)
- D2D 主路径 (PathGeometry + round-cap StrokeStyle) — 端帽与笔画严格对齐, sub-pixel 精度
- GDI+ 兜底自动启用 (`XInitXCGUI(FALSE)` 或低端机), 笔画宽手动 ×dpi 物理化, cap 圆直径与笔画严格等宽
- `Stop` 对自建元素 (`Create` / `AttachWnd`) 走 `XWidget_Show(FALSE)`, 对外部元素 (`AttachEle`) 走让出 paint
- 元素/窗口销毁时自动释放 entry (无需显式 `Detach`); 进程退出前调 `CXLoading::Cleanup()` 兜底

## ⚠️ `CXBlur::ForceSystemTransparencyOn` 修改注册表

强制启用系统 *透明效果* 开关, 让 acrylic 在用户关掉个性化透明时仍能出 blur. **默认 `FALSE` = no-op**, 必须显式传 `TRUE` 才写注册表.

| 项 | 内容 |
|---|---|
| 注册表路径 | `HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\EnableTransparency` |
| 写入值 | `REG_DWORD = 1` |
| 作用范围 | **当前用户所有 app**, 非进程级 |
| 退出还原 | `atexit` + `SetConsoleCtrlHandler` 双兜底; `TerminateProcess` / 蓝屏不还原 |
| OS 要求 | Win10 1803+ / Win11 |

```cpp
CXBlur::ForceSystemTransparencyOn(TRUE);    // 开 (写注册表)
CXBlur::ForceSystemTransparencyOn(FALSE);   // 关 / 还原 (默认值)
```

> 调用 `(TRUE)` 的产品应在用户协议中告知该行为.

## 编译

- MSVC 2015+ (推荐 VS 2022)
- Windows SDK ≥ 10.0.17763
- XCGUI ≥ 2025-12 (`module_xcgui_video` 需要 `XDraw_ConvRect`)
- FFmpeg dev 包 (仅 `video` / `image` 模块需要)

## 模块封装规范

新增封装模块前请阅读 [`炫彩界面库模块封装规范/模块封装规范.md`](./炫彩界面库模块封装规范/模块封装规范.md). 该文档从现有 5 个生产模块 (`editdw` / `video` / `image` / `blur` / `shadow`) 反向提炼, 后续 AI 协作或人工开发新模块时只读此文即可直接动手.

涵盖内容:

- 文件命名 / 头守卫 / `class CX<功能>` 大驼峰约定
- 头文件骨架与全套 `@` 标签 (`@模块名称` / `@别名` / `@分组` / `@隐藏` / `@复制文件` / `@lib` / `@src` / `@依赖`)
- 类层级 (`CXBase` → `CXObjectUI` → `CXEle` / `CXShape` / `CXScrollView`) 与必须实现的 5 段样板 (`GetHXCGUI` override / `operator HELE` / `operator HXCGUI` / `operator=`)
- **DPI 与坐标转换** (新模块开发必读, 5 模块踩坑总结的全部 XCGUI 坐标 API 与高频规则)
- D2D 主路径 + GDI 兜底的 `OnPaintImpl` 标准分流代码
- `BkInfo` / 多线程 (`BoundedQueue` + `PushTimeout` 防 UI 卡死锁)
- 事件系统: 通用走 `XEle_RegEventCPP1`, 业务走 C 风格函数指针 + `void*` 用户数据
- 头文件依赖陷阱 (`d2d1.h` 必须先于 `module_xcgui.h` 否则 `POINTF` 重定义)
- 资源链接 (`@lib` + `#pragma comment(lib)` 双写) 与运行时 DLL 拷贝
- 完整 `module_xcgui_demo.h/.cpp` 模板 (可直接 copy-paste 改名)
- 中文 `@别名` 命名速查表 (避免与父类同名冲突, 例如 `取播放状态` vs `取状态`)

附带依赖 (新模块编译必需, 已打包同目录):

- `基础模块/` — `module_base.h/.cpp` + `xc_mkStr` 字符串工具
- `炫彩界面库/` — `module_xcgui.h` / `module_xcgui_class.h` / `xcgui_event.h` + `XCGUI.dll/lib` (x86 / x64)

## 许可证

[Unlicense](LICENSE)

## 作者

**未闻花名** · QQ 936599025
