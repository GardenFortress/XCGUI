// DComp 亚克力合成接口。

#pragma once
#include <windows.h>

namespace XBlurDComp {

// 检查当前系统是否支持 DComp 合成路径。
bool IsSupported();

// 首次调用创建合成树，后续调用更新效果参数；blurOpacity < 0 时从 tintA 推导。
// tint 分量为 0..255，blurOpacity 为 0..1，noiseAlphaPct 为 0..100。
// uniformBrightness 锁定背景亮度；shadowFrameInset 为原生阴影留出像素边距。
// 返回 false 时调用方应回退到其他模糊路径。
bool Apply(HWND host,
           int tintR, int tintG, int tintB, int tintA,
           float blurOpacity,
           float saturation,
           BOOL uniformBrightness,
           float noiseAlphaPct,
           int shadowFrameInset);

// 同步合成层尺寸；未绑定的 host 会被忽略。
void Resize(HWND host, int shadowFrameInset);

// 裁切圆角并按 borderInset 内缩合成层，为 XCGUI 描边留出区域。
// radius 为 0 时关闭裁切；Resize 会同步裁切区域尺寸。
void SetCornerRadius(HWND host, float radius, float borderInset = 0.0f);

// 释放指定窗口的合成资源；重复调用安全。
void Disable(HWND host);

// 为 XCGUI 主窗创建 acrylic owner；调用方负责设置透明背景和拖动行为。
// 调用前设 window_transparent_shaped 并关闭背景绘制；调用后设透明度 255、
// 含非零 alpha 的背景信息及拖动行为。相关 XCGUI API 在调用方编译单元执行。
// 原有 owner 保留在窗口链中；主窗销毁时自动解绑；创建失败返回 NULL。
HWND AttachAcrylicHost(void* hxw,
                       int tintR, int tintG, int tintB, int tintA,
                       float blurOpacity,
                       float saturation,
                       BOOL  uniformBrightness,
                       float noiseAlphaPct);

// 返回指定主窗对应的 acrylic HWND；未绑定时返回 NULL。
HWND GetAcrylicHwnd(void* hxwOpaque = nullptr);

// 更新还原时重建 acrylic 所需的主题参数。运行时切换浅色、深色或自定义 tint 后调用。
void UpdateAcrylicEffectArgs(void* hxwOpaque,
                             int tintR, int tintG, int tintB, int tintA,
                             float blurOpacity, float saturation,
                             BOOL uniformBrightness, float noiseAlphaPct);

// 显式解绑 acrylic 并恢复主窗 owner 与扩展样式。
void DetachAcrylicHost(void* hxwOpaque);

} // namespace XBlurDComp
