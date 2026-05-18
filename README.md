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

详细 API 见各 `.h` 文件中的 `//@别名` 注释.

## 兼容性

| 系统 | editdw / image | video | blur | shadow |
|---|---|---|---|---|
| Win11 / Win10 1803+ | D2D | D2D + 硬解 | **ACCENT_ACRYLIC** | D2D + GDI+ DIB |
| Win10 1607~1709 | D2D | D2D + 硬解 | ACCENT_BLURBEHIND | D2D + GDI+ DIB |
| Win8 / 8.1 | D2D | D2D + 硬解 | 装饰层 | D2D + GDI+ DIB |
| Win7 SP1 (有 GPU) | D2D | D2D + 软解 | 装饰层 | D2D + GDI+ DIB |
| Win7 SP1 (VMware 默认) | GDI 降级 | GDI+ 降级 + 软解 | 装饰层 | GDI+ DIB |

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
#include "module_xcgui_editdw.h"          // 按需 include: editdw / video / image / blur

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
