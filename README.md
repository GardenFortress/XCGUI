# XCGUI 模块扩展库

[炫彩界面库 (XCGUI)](https://www.xcgui.com) 的扩展模块集合. D2D 主路径 + GDI/GDI+ 兜底, 兼容 Windows 7 SP1+.

## 模块

| 模块 | 类 | 用途 |
|---|---|---|
| [`module_xcgui_editdw`](./module_xcgui_editdw/) | `CXEditDW : CXScrollView` | DirectWrite 富文本编辑框, 彩色 Emoji, 分段 layout 支持 200K+ 字符 |
| [`module_xcgui_video`](./module_xcgui_video/) | `CXVideo : CXEle` | FFmpeg 视频播放器, D3D11VA/DXVA2 硬解, WASAPI 音频 |
| [`module_xcgui_image`](./module_xcgui_image/) | `CXImageEx : CXEle` | FFmpeg 图片元素, 静/动态 jpg/png/webp/heic/avif/gif/apng 等 |
| [`module_xcgui_blur`](./module_xcgui_blur/) | `CXBlur : CXEle` | DWM 亚克力 (`ACCENT_ACRYLIC`), 元素级 alpha 控制, per-corner 圆角 |

详细 API 见各 `.h` 文件中的 `//@别名` 注释.

## 兼容性

| 系统 | editdw / image | video | blur |
|---|---|---|---|
| Win11 / Win10 1803+ | D2D | D2D + 硬解 | **ACCENT_ACRYLIC** |
| Win10 1607~1709 | D2D | D2D + 硬解 | ACCENT_BLURBEHIND |
| Win8 / 8.1 | D2D | D2D + 硬解 | 装饰层 |
| Win7 SP1 (有 GPU) | D2D | D2D + 软解 | 装饰层 |
| Win7 SP1 (VMware 默认) | GDI 降级 | GDI+ 降级 + 软解 | 装饰层 |

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
#include <dwrite.h>                  // editdw 需要
#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"
#include "module_xcgui_blur.h"       // 或其他模块

XInitXCGUI(TRUE);                    // D2D 主渲染; FALSE = GDI+ 兜底

CXBlur* p = new CXBlur();
p->AttachToWnd(hWnd);
p->SetTheme(xblur_theme_auto);
p->SetCornerRadiusEx(16, 16, 0, 0);  // per-corner: 左上 / 右上 / 右下 / 左下
```

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

## 许可证

[Unlicense](LICENSE)

## 作者

**未闻花名** · QQ 936599025
