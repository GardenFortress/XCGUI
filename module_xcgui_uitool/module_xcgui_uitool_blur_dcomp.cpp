// =============================================================================
// module_xcgui_uitool_blur_dcomp.cpp — XBlurDComp split (原 blur_dcomp.cpp 纯搬运)
//
// DComp + WUC 直接合成路径实现.
//
// 设计要点:
//   * 全部 WinRT / D2D 类型隔离在 .cpp 内, 不外泄.
//   * 每 HWND 独立 Compositor + DesktopWindowTarget + 完整 effect chain. 不共享
//     Compositor (可以共享但收益小, 每窗一个简单清晰).
//   * Effect chain 跟 PoC `poc_dcomp.cpp` 严格一致, PoC 验证过的所有踩坑都已避开.
//
// 链路 (从下到上):
//   backdrop (CompositionBackdropBrush)
//     → GaussianBlur (stdDev=30)
//     → BlendEffect(Luminosity 语义) 与 luminosity 纯色混合 [可关 = 亮度锁定]
//     → ColorMatrix(Saturation, Rec.709 luma)
//     → Opacity (= 1 - tint 层不透明度, 即 backdrop 层可见度)
//     → Composite SOURCE_OVER over tint ColorBrush (不透明底)
//     → [可选] Noise PNG → BorderEffect WRAP → Opacity (3%) → BlendEffect MULTIPLY
//
// 配色算法 (对齐 WinUI AcrylicBrush 19H1+ 配方, 见 ResolveAcrylicRecipe):
//   "剔除黑白灰"靠 *真* Luminosity 混合完成 — 亮度取自 luminosity 纯色, 色度留给
//   桌面. 关键点是 tint 层强度与亮度锁定强度是 *两个独立通道*: 亮度锁定可以很强
//   (深浅主题下都不出现大面积黑/白/灰割裂), 同时 tint 层保持较低不透明度, 桌面
//   色度得以大比例透出 = Win11 开始菜单那种"通透"观感.
//     旧实现把两者绑成同一个 blurOpacity, 且用加性 ColorMatrix 近似 Luminosity,
//   结果是: 色度被整体乘上一个很小的系数 (发淡), 且矩阵结果逐通道 clamp 到 8bpc
//   中间缓冲上限后色相被拉偏 (偏色). 现在改用 D2D BlendEffect, 其 SetLum/ClipColor
//   出界时按亮度整体收敛色度, 保色相.
//
// 已踩坑 (PoC 阶段):
//   1. D2D effect GUID 必须从 d2d1effects.h 字字核对, 别凭印象 (Saturation, Blend
//      都踩过). 错的 GUID → "Unsupported effect type" 0x80070057.
//   2. D2D ColorMatrix 是 v*M, 不是教科书 M*v. 矩阵布局按"input 通道贡献"为行.
//   3. IGraphicsEffect::Name 在同一 graph 内必须唯一 — 同 GUID 多实例必须分名.
//   4. ColorMatrix 带非 0 offset 必须 STRAIGHT alpha 模式 (=2), 否则边缘漏光.
//   5. BlendEffect Source 顺序: Background=0, Foreground=1.
//   6. Win2D Turbulence 不在 WUC 桥白名单 → CPU 撒白噪声 + D2D Bitmap.
//   7. CompositionSurfaceBrush 默认 BitmapInterpolationMode=Linear, 噪声会糊成
//      色斑, 必须 NearestNeighbor.
//   8. WinUI 源码注明 Win2D BlendEffectMode 的 Color / Luminosity 命名对调过.
//      本机实测 D2D 侧符合规范, 但仍不写死枚举 — 由
//      DetectLuminosityBlendMode_Locked 运行时实测, 选错的视觉代价太大.
// =============================================================================

#include "module_xcgui_blur_dcomp.h"

// 独立编译时前向声明; uitool 聚合 TU 内 module_xcgui.h 已定义 HWINDOW.
#ifndef XCGUI_H
typedef void* HWINDOW;
typedef void* HXCGUI;
extern "C" {
HWND  WINAPI XWnd_GetHWND(HWINDOW hWindow);
BOOL  WINAPI XC_IsHWINDOW(HXCGUI hWindow);
}
#endif

#include <wrl.h>
#include <wrl/implements.h>
#include <CommCtrl.h>
#include <dwmapi.h>
#include <DispatcherQueue.h>
#include <windows.ui.composition.h>
#include <windows.ui.composition.interop.h>
#include <Windows.Graphics.Effects.h>
#include <Windows.Graphics.Effects.Interop.h>
#include <d2d1effects_2.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi.h>
#include <random>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <atomic>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.Graphics.Effects.h>
#include <winrt/Windows.Graphics.DirectX.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

#pragma comment(lib, "delayimp.lib")

// ============================================================================
// Windows 7 兼容性火墙：延迟加载 Win8/Win10 特专属 DLL 与 API 接口集
// ============================================================================

// 1. WinRT 核心基础服务 (激活、类工厂等，Win8+)
#pragma comment(linker, "/DELAYLOAD:api-ms-win-core-winrt-l1-1-0.dll")

// 2. HSTRING 字符串处理 (C++/WinRT 传递参数必用，Win8+)
#pragma comment(linker, "/DELAYLOAD:api-ms-win-core-winrt-string-l1-1-0.dll")

// 3. WinRT 错误处理与报告 (Win8+/Win10)
#pragma comment(linker, "/DELAYLOAD:api-ms-win-core-winrt-error-l1-1-0.dll")
#pragma comment(linker, "/DELAYLOAD:api-ms-win-core-winrt-error-l1-1-1.dll")

// 4. 参数化接口 IID 生成器 (C++/WinRT 模板/泛型实例化时依赖，Win10)
#pragma comment(linker, "/DELAYLOAD:api-ms-win-core-winrt-roparameterizediid-l1-1-0.dll")

// 5. COM 核心基础 (WinRT 底层映射的核心库，Win8+)
#pragma comment(linker, "/DELAYLOAD:combase.dll")

// 6. 现代 DPI 与缩放管理 (Win8.1+)
#pragma comment(linker, "/DELAYLOAD:shcore.dll")

// 7. 直接合成核心库 (DirectComposition 运行时，Win8+)
#pragma comment(linker, "/DELAYLOAD:dcomp.dll")

// 8. 现代消息调度通道 (DispatcherQueue 运行依赖，Win10)
#pragma comment(linker, "/DELAYLOAD:CoreMessaging.dll")

namespace ABI_GE = ABI::Windows::Graphics::Effects;
namespace WGE    = winrt::Windows::Graphics::Effects;
namespace WUC    = winrt::Windows::UI::Composition;
namespace WUCD   = winrt::Windows::UI::Composition::Desktop;
namespace WGDX   = winrt::Windows::Graphics::DirectX;
using winrt::com_ptr;
using winrt::check_hresult;

// ---------------------------------------------------------------------------
// D2D effect CLSID (字字核对自 d2d1effects.h / d2d1effects_2.h).
// ---------------------------------------------------------------------------
// {1FEB6D69-2FE6-4AC9-8C58-1D7F93E7A6A5} GaussianBlur (用 SDK 头里的常量也行)
// {921F03D6-641C-47DF-852D-B4BB6153AE11} ColorMatrix
// {811D79A4-DE28-4454-8094-C64685F8BD4C} Opacity
// {48FC9F51-F6AC-48F1-8B58-3B28AC46F76D} Composite
// {81C5B77B-13F8-4CDD-AD20-C890547AC65D} Blend
// {2A2D49C0-4ACF-43c7-8C6A-7C4A27874D27} Border
static constexpr GUID kColorMatrixGuid =
    { 0x921f03d6, 0x641c, 0x47df, { 0x85, 0x2d, 0xb4, 0xbb, 0x61, 0x53, 0xae, 0x11 } };
static constexpr GUID kOpacityGuid =
    { 0x811d79a4, 0xde28, 0x4454, { 0x80, 0x94, 0xc6, 0x46, 0x85, 0xf8, 0xbd, 0x4c } };
static constexpr GUID kCompositeGuid =
    { 0x48fc9f51, 0xf6ac, 0x48f1, { 0x8b, 0x58, 0x3b, 0x28, 0xac, 0x46, 0xf7, 0x6d } };
static constexpr GUID kBlendGuid =
    { 0x81c5b77b, 0x13f8, 0x4cdd, { 0xad, 0x20, 0xc8, 0x90, 0x54, 0x7a, 0xc6, 0x5d } };
static constexpr GUID kBorderGuid =
    { 0x2A2D49C0, 0x4ACF, 0x43c7, { 0x8C, 0x6A, 0x7C, 0x4A, 0x27, 0x87, 0x4D, 0x27 } };

namespace {

// ---------------------------------------------------------------------------
// GaussianBlurEffectImpl — 模糊本体. CLSID 用 d2d1effects_2.h 提供的常量.
// 属性: 0=StandardDeviation float, 1=Optimization UInt32, 2=BorderMode UInt32
// ---------------------------------------------------------------------------
struct GaussianBlurEffectImpl : winrt::implements<
    GaussianBlurEffectImpl,
    WGE::IGraphicsEffect,
    WGE::IGraphicsEffectSource,
    ABI_GE::IGraphicsEffectD2D1Interop>
{
    winrt::hstring m_name = L"GaussianBlur";
    float  m_stdDev = 30.0f;
    UINT32 m_optimization = 1;  // BALANCED
    UINT32 m_borderMode   = 0;  // SOFT
    WGE::IGraphicsEffectSource m_source{nullptr};

    void Source(WGE::IGraphicsEffectSource const& s) { m_source = s; }
    void StandardDeviation(float v) { m_stdDev = v; }
    void Name(winrt::hstring const& n) { m_name = n; }
    winrt::hstring Name() { return m_name; }

    HRESULT __stdcall GetEffectId(GUID* id) noexcept override {
        *id = CLSID_D2D1GaussianBlur; return S_OK;
    }
    HRESULT __stdcall GetSourceCount(UINT* c) noexcept override { *c = 1; return S_OK; }
    HRESULT __stdcall GetSource(UINT idx, ABI_GE::IGraphicsEffectSource** out) noexcept override {
        if (idx != 0 || !m_source) return E_INVALIDARG;
        winrt::copy_to_abi(m_source, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetPropertyCount(UINT* c) noexcept override { *c = 3; return S_OK; }
    HRESULT __stdcall GetProperty(UINT idx, ABI::Windows::Foundation::IPropertyValue** out) noexcept override {
        using namespace winrt::Windows::Foundation;
        IPropertyValue v{nullptr};
        switch (idx) {
        case 0: v = PropertyValue::CreateSingle(m_stdDev).as<IPropertyValue>(); break;
        case 1: v = PropertyValue::CreateUInt32(m_optimization).as<IPropertyValue>(); break;
        case 2: v = PropertyValue::CreateUInt32(m_borderMode).as<IPropertyValue>(); break;
        default: return E_INVALIDARG;
        }
        winrt::copy_to_abi(v, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR, UINT*, ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING*) noexcept override {
        return E_INVALIDARG;
    }
};

// ---------------------------------------------------------------------------
// ColorMatrixEffectImpl — 5x4 矩阵变换 RGBA. v*M 顺序 (每行=输入通道贡献).
// 用 2 次: saturation 矩阵 + luminosity replace 矩阵 + (noise grayscale 矩阵).
// ---------------------------------------------------------------------------
struct ColorMatrixEffectImpl : winrt::implements<
    ColorMatrixEffectImpl,
    WGE::IGraphicsEffect,
    WGE::IGraphicsEffectSource,
    ABI_GE::IGraphicsEffectD2D1Interop>
{
    winrt::hstring m_name = L"ColorMatrix";
    float m_matrix[20] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
        0, 0, 0, 0
    };
    UINT m_alphaMode = 1;  // 1=PREMULTIPLIED 默认, 2=STRAIGHT (offset 矩阵需要)
    WGE::IGraphicsEffectSource m_source{nullptr};

    void Source(WGE::IGraphicsEffectSource const& s) { m_source = s; }
    void SetMatrix(float const* m) { memcpy(m_matrix, m, sizeof(m_matrix)); }
    void SetAlphaMode(UINT m) { m_alphaMode = m; }
    void Name(winrt::hstring const& n) { m_name = n; }
    winrt::hstring Name() { return m_name; }

    HRESULT __stdcall GetEffectId(GUID* id) noexcept override { *id = kColorMatrixGuid; return S_OK; }
    HRESULT __stdcall GetSourceCount(UINT* c) noexcept override { *c = 1; return S_OK; }
    HRESULT __stdcall GetSource(UINT idx, ABI_GE::IGraphicsEffectSource** out) noexcept override {
        if (idx != 0 || !m_source) return E_INVALIDARG;
        winrt::copy_to_abi(m_source, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetPropertyCount(UINT* c) noexcept override { *c = 3; return S_OK; }
    HRESULT __stdcall GetProperty(UINT idx, ABI::Windows::Foundation::IPropertyValue** out) noexcept override {
        using namespace winrt::Windows::Foundation;
        IPropertyValue v{nullptr};
        switch (idx) {
        case 0: {
            winrt::array_view<float const> arr{ m_matrix, m_matrix + 20 };
            v = PropertyValue::CreateSingleArray(arr).as<IPropertyValue>(); break;
        }
        case 1: v = PropertyValue::CreateUInt32(m_alphaMode).as<IPropertyValue>(); break;
        case 2: v = PropertyValue::CreateBoolean(false).as<IPropertyValue>(); break;  // ClampOutput=false
        default: return E_INVALIDARG;
        }
        winrt::copy_to_abi(v, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetNamedPropertyMapping(
        LPCWSTR name, UINT* idx, ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) noexcept override {
        if (!name || !idx || !mapping) return E_POINTER;
        if (wcscmp(name, L"ColorMatrix") == 0) {
            *idx = 0; *mapping = ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT; return S_OK;
        }
        if (wcscmp(name, L"AlphaMode") == 0) {
            *idx = 1; *mapping = ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLORMATRIX_ALPHA_MODE; return S_OK;
        }
        if (wcscmp(name, L"ClampOutput") == 0) {
            *idx = 2; *mapping = ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT; return S_OK;
        }
        return E_INVALIDARG;
    }
};

// ---------------------------------------------------------------------------
// OpacityEffectImpl — alpha 乘常数. 用于 blur 半透 + noise 3% dim.
// ---------------------------------------------------------------------------
struct OpacityEffectImpl : winrt::implements<
    OpacityEffectImpl,
    WGE::IGraphicsEffect,
    WGE::IGraphicsEffectSource,
    ABI_GE::IGraphicsEffectD2D1Interop>
{
    winrt::hstring m_name = L"Opacity";
    float m_opacity = 1.0f;
    WGE::IGraphicsEffectSource m_source{nullptr};

    void Source(WGE::IGraphicsEffectSource const& s) { m_source = s; }
    void Opacity(float v) { m_opacity = v; }
    void Name(winrt::hstring const& n) { m_name = n; }
    winrt::hstring Name() { return m_name; }

    HRESULT __stdcall GetEffectId(GUID* id) noexcept override { *id = kOpacityGuid; return S_OK; }
    HRESULT __stdcall GetSourceCount(UINT* c) noexcept override { *c = 1; return S_OK; }
    HRESULT __stdcall GetSource(UINT idx, ABI_GE::IGraphicsEffectSource** out) noexcept override {
        if (idx != 0 || !m_source) return E_INVALIDARG;
        winrt::copy_to_abi(m_source, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetPropertyCount(UINT* c) noexcept override { *c = 1; return S_OK; }
    HRESULT __stdcall GetProperty(UINT idx, ABI::Windows::Foundation::IPropertyValue** out) noexcept override {
        using namespace winrt::Windows::Foundation;
        if (idx != 0) return E_INVALIDARG;
        auto v = PropertyValue::CreateSingle(m_opacity).as<IPropertyValue>();
        winrt::copy_to_abi(v, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR, UINT*, ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING*) noexcept override {
        return E_INVALIDARG;
    }
};

// ---------------------------------------------------------------------------
// CompositeEffectImpl — Porter-Duff. mode=0 SOURCE_OVER, source[0]=dest, source[1]=src.
// ---------------------------------------------------------------------------
struct CompositeEffectImpl : winrt::implements<
    CompositeEffectImpl,
    WGE::IGraphicsEffect,
    WGE::IGraphicsEffectSource,
    ABI_GE::IGraphicsEffectD2D1Interop>
{
    winrt::hstring m_name = L"Composite";
    UINT m_mode = 0;
    WGE::IGraphicsEffectSource m_dest{nullptr};
    WGE::IGraphicsEffectSource m_source{nullptr};

    void Destination(WGE::IGraphicsEffectSource const& s) { m_dest = s; }
    void Source(WGE::IGraphicsEffectSource const& s) { m_source = s; }
    void Mode(UINT m) { m_mode = m; }
    void Name(winrt::hstring const& n) { m_name = n; }
    winrt::hstring Name() { return m_name; }

    HRESULT __stdcall GetEffectId(GUID* id) noexcept override { *id = kCompositeGuid; return S_OK; }
    HRESULT __stdcall GetSourceCount(UINT* c) noexcept override { *c = 2; return S_OK; }
    HRESULT __stdcall GetSource(UINT idx, ABI_GE::IGraphicsEffectSource** out) noexcept override {
        WGE::IGraphicsEffectSource src{nullptr};
        if      (idx == 0) src = m_dest;
        else if (idx == 1) src = m_source;
        else return E_INVALIDARG;
        if (!src) return E_INVALIDARG;
        winrt::copy_to_abi(src, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetPropertyCount(UINT* c) noexcept override { *c = 1; return S_OK; }
    HRESULT __stdcall GetProperty(UINT idx, ABI::Windows::Foundation::IPropertyValue** out) noexcept override {
        using namespace winrt::Windows::Foundation;
        if (idx != 0) return E_INVALIDARG;
        auto v = PropertyValue::CreateUInt32(m_mode).as<IPropertyValue>();
        winrt::copy_to_abi(v, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR, UINT*, ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING*) noexcept override {
        return E_INVALIDARG;
    }
};

// ---------------------------------------------------------------------------
// BlendEffectImpl — Photoshop 风混合. mode=0 MULTIPLY 用于 noise 叠层.
// *Source 顺序*: 0=Background (底), 1=Foreground (顶) — 跟 Win2D 一致.
// ---------------------------------------------------------------------------
struct BlendEffectImpl : winrt::implements<
    BlendEffectImpl,
    WGE::IGraphicsEffect,
    WGE::IGraphicsEffectSource,
    ABI_GE::IGraphicsEffectD2D1Interop>
{
    winrt::hstring m_name = L"Blend";
    UINT m_mode = 0;  // MULTIPLY 默认
    WGE::IGraphicsEffectSource m_fg{nullptr};
    WGE::IGraphicsEffectSource m_bg{nullptr};

    void Foreground(WGE::IGraphicsEffectSource const& s) { m_fg = s; }
    void Background(WGE::IGraphicsEffectSource const& s) { m_bg = s; }
    void Mode(UINT m) { m_mode = m; }
    void Name(winrt::hstring const& n) { m_name = n; }
    winrt::hstring Name() { return m_name; }

    HRESULT __stdcall GetEffectId(GUID* id) noexcept override { *id = kBlendGuid; return S_OK; }
    HRESULT __stdcall GetSourceCount(UINT* c) noexcept override { *c = 2; return S_OK; }
    HRESULT __stdcall GetSource(UINT idx, ABI_GE::IGraphicsEffectSource** out) noexcept override {
        WGE::IGraphicsEffectSource src{nullptr};
        if      (idx == 0) src = m_bg;  // Background
        else if (idx == 1) src = m_fg;  // Foreground
        else return E_INVALIDARG;
        if (!src) return E_INVALIDARG;
        winrt::copy_to_abi(src, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetPropertyCount(UINT* c) noexcept override { *c = 1; return S_OK; }
    HRESULT __stdcall GetProperty(UINT idx, ABI::Windows::Foundation::IPropertyValue** out) noexcept override {
        using namespace winrt::Windows::Foundation;
        if (idx != 0) return E_INVALIDARG;
        auto v = PropertyValue::CreateUInt32(m_mode).as<IPropertyValue>();
        winrt::copy_to_abi(v, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR, UINT*, ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING*) noexcept override {
        return E_INVALIDARG;
    }
};

// ---------------------------------------------------------------------------
// BorderEffectImpl — source 边缘扩展. mode 1=WRAP 用于 noise 平铺.
// ---------------------------------------------------------------------------
struct BorderEffectImpl : winrt::implements<
    BorderEffectImpl,
    WGE::IGraphicsEffect,
    WGE::IGraphicsEffectSource,
    ABI_GE::IGraphicsEffectD2D1Interop>
{
    winrt::hstring m_name = L"Border";
    UINT32 m_extendX = 1;  // WRAP
    UINT32 m_extendY = 1;
    WGE::IGraphicsEffectSource m_source{nullptr};

    void Source(WGE::IGraphicsEffectSource const& s) { m_source = s; }
    void SetExtendModes(UINT32 x, UINT32 y) { m_extendX = x; m_extendY = y; }
    void Name(winrt::hstring const& n) { m_name = n; }
    winrt::hstring Name() { return m_name; }

    HRESULT __stdcall GetEffectId(GUID* id) noexcept override { *id = kBorderGuid; return S_OK; }
    HRESULT __stdcall GetSourceCount(UINT* c) noexcept override { *c = 1; return S_OK; }
    HRESULT __stdcall GetSource(UINT idx, ABI_GE::IGraphicsEffectSource** out) noexcept override {
        if (idx != 0 || !m_source) return E_INVALIDARG;
        winrt::copy_to_abi(m_source, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetPropertyCount(UINT* c) noexcept override { *c = 2; return S_OK; }
    HRESULT __stdcall GetProperty(UINT idx, ABI::Windows::Foundation::IPropertyValue** out) noexcept override {
        using namespace winrt::Windows::Foundation;
        IPropertyValue v{nullptr};
        if (idx == 0)      v = PropertyValue::CreateUInt32(m_extendX).as<IPropertyValue>();
        else if (idx == 1) v = PropertyValue::CreateUInt32(m_extendY).as<IPropertyValue>();
        else return E_INVALIDARG;
        winrt::copy_to_abi(v, *reinterpret_cast<void**>(out));
        return S_OK;
    }
    HRESULT __stdcall GetNamedPropertyMapping(
        LPCWSTR name, UINT* idx, ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) noexcept override {
        if (!name || !idx || !mapping) return E_POINTER;
        if (wcscmp(name, L"ExtendX") == 0) { *idx = 0; *mapping = ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT; return S_OK; }
        if (wcscmp(name, L"ExtendY") == 0) { *idx = 1; *mapping = ABI_GE::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT; return S_OK; }
        return E_INVALIDARG;
    }
};

} // anonymous namespace

// ===========================================================================
// 全局共享: dispatcher queue + D3D/D2D 设备 (可跨 Compositor 共享).
// CompositionGraphicsDevice 必须 per-Compositor, 存 HostState 里.
// ===========================================================================
struct GlobalShared {
    bool initialized = false;
    winrt::Windows::System::DispatcherQueueController dispatcher{nullptr};
    com_ptr<ID3D11Device> d3dDevice;
    com_ptr<ID2D1Device>  d2dDevice;
};
static GlobalShared g_shared;
static std::mutex   g_sharedMutex;

struct HostState; // forward — per-HWND 完整定义见下方

// 进程级 DispatcherQueue. Compositor 必须挂在 dispatcher queue 上.
// 调多次安全 (winrt::Windows::System::DispatcherQueue::GetForCurrentThread()
// 在已存在时直接返回, CreateDispatcherQueueController 也会 fail-safe).
static bool EnsureDispatcherQueue_Locked() {
    if (g_shared.dispatcher) return true;
    DispatcherQueueOptions opts{};
    opts.dwSize        = sizeof(opts);
    opts.threadType    = DQTYPE_THREAD_CURRENT;
    opts.apartmentType = DQTAT_COM_NONE;

    // Win7 无静态导入 CoreMessaging.dll, 运行时加载避免进程启动失败.
    typedef HRESULT(WINAPI* PFN_CreateDispatcherQueueController)(DispatcherQueueOptions, ABI::Windows::System::IDispatcherQueueController**);
    static HMODULE hCoreMessaging = ::LoadLibraryW(L"CoreMessaging.dll");
    if (!hCoreMessaging) return false;

    static auto pfnCreate = (PFN_CreateDispatcherQueueController)::GetProcAddress(hCoreMessaging, "CreateDispatcherQueueController");
    if (!pfnCreate) return false;

    ABI::Windows::System::IDispatcherQueueController* raw = nullptr;
    HRESULT hr = pfnCreate(opts, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(&raw));

    if (FAILED(hr)) return false;
    winrt::Windows::System::DispatcherQueueController dq{nullptr};
    winrt::copy_from_abi(dq, raw);
    g_shared.dispatcher = dq;
    raw->Release();
    return true;
}

// 单例 D3D11 + D2D. CompositionGraphicsDevice 按 Compositor 分别创建.
static bool EnsureSharedD3D_Locked() {
    if (g_shared.d3dDevice) return true;

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL fl;
    HRESULT hr = ::D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        nullptr, 0, D3D11_SDK_VERSION,
        g_shared.d3dDevice.put(), &fl, nullptr);
    if (FAILED(hr)) return false;

    auto dxgiDevice = g_shared.d3dDevice.as<IDXGIDevice>();
    com_ptr<ID2D1Factory1> d2dFactory;
    D2D1_FACTORY_OPTIONS opts{};
    hr = ::D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory1), &opts,
        d2dFactory.put_void());
    if (FAILED(hr)) return false;
    hr = d2dFactory->CreateDevice(dxgiDevice.get(), g_shared.d2dDevice.put());
    return SUCCEEDED(hr);
}

// ---------------------------------------------------------------------------
// 挑出"亮度取 foreground, 色度留给 background"那一档混合模式.
//
// 按 W3C / PDF 规范这是 LUMINOSITY, 本机 (Win11 26100) 实测 D2D 也确实如此:
//   backdrop=纯蓝 + foreground=中灰 → mode 23 得 (112,112,255), mode 22 得灰.
// 但 WinUI AcrylicBrush 源码注明它用的 Win2D BlendEffectMode 里 Color / Luminosity
// 两个名字是对调的, 说明这层命名历史上出过错. 一旦选错, 窗口会变成"桌面亮度灰度
// 图 + tint 色度", 正好是本模块要消灭的割裂感, 所以不写死, 运行时实测一次并缓存.
//
// 判据: Background = 饱和蓝, Foreground = 不透明中灰.
//   Luminosity 语义 → 亮度换成中灰, 色度仍来自 backdrop → 输出明显偏蓝 (极差大)
//   Color 语义      → 色度来自 foreground 的灰 = 无色 → 输出灰 (极差 ≈ 0)
// 探测失败 (D2D 资源建不出来) 回退规范值 LUMINOSITY.
//
// 调用方须持 g_sharedMutex 且已 EnsureSharedD3D_Locked.
// ---------------------------------------------------------------------------
static UINT DetectLuminosityBlendMode_Locked() {
    static int sCached = -1;
    if (sCached >= 0) return (UINT)sCached;

    UINT picked = (UINT)D2D1_BLEND_MODE_LUMINOSITY;
    com_ptr<ID2D1DeviceContext> ctx;
    if (g_shared.d2dDevice &&
        SUCCEEDED(g_shared.d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, ctx.put())))
    {
        // B8G8R8A8 内存序 B,G,R,A → uint32 (小端) 为 0xAARRGGBB.
        const uint32_t kBackdropBlue = 0xFF0000FFu;
        const uint32_t kForegroundGray = 0xFF808080u;

        D2D1_BITMAP_PROPERTIES1 props{};
        props.pixelFormat = { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED };
        props.dpiX = 96.0f;
        props.dpiY = 96.0f;

        com_ptr<ID2D1Bitmap1> bg, fg, target, staging;
        HRESULT hr = ctx->CreateBitmap(D2D1::SizeU(1, 1), &kBackdropBlue, 4, props, bg.put());
        if (SUCCEEDED(hr)) hr = ctx->CreateBitmap(D2D1::SizeU(1, 1), &kForegroundGray, 4, props, fg.put());

        D2D1_BITMAP_PROPERTIES1 targetProps = props;
        targetProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
        if (SUCCEEDED(hr)) hr = ctx->CreateBitmap(D2D1::SizeU(1, 1), nullptr, 0, targetProps, target.put());

        D2D1_BITMAP_PROPERTIES1 stagingProps = props;
        stagingProps.bitmapOptions = D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        if (SUCCEEDED(hr)) hr = ctx->CreateBitmap(D2D1::SizeU(1, 1), nullptr, 0, stagingProps, staging.put());

        // 用本文件核对过的 kBlendGuid, 不用 SDK 的 CLSID_D2D1Blend 符号 —— 后者要额外
        // 链 dxguid.lib, 会给使用方增加链接依赖.
        com_ptr<ID2D1Effect> blend;
        if (SUCCEEDED(hr)) hr = ctx->CreateEffect(kBlendGuid, blend.put());
        if (SUCCEEDED(hr)) {
            blend->SetInput(0, bg.get());
            blend->SetInput(1, fg.get());
            hr = blend->SetValue(D2D1_BLEND_PROP_MODE, (UINT32)D2D1_BLEND_MODE_COLOR);
        }
        if (SUCCEEDED(hr)) {
            ctx->SetTarget(target.get());
            ctx->BeginDraw();
            ctx->Clear(D2D1::ColorF(0, 0, 0, 0));
            ctx->DrawImage(blend.get());
            hr = ctx->EndDraw();
            ctx->SetTarget(nullptr);
        }
        if (SUCCEEDED(hr)) hr = staging->CopyFromBitmap(nullptr, target.get(), nullptr);

        D2D1_MAPPED_RECT mapped{};
        if (SUCCEEDED(hr) && SUCCEEDED(staging->Map(D2D1_MAP_OPTIONS_READ, &mapped)) && mapped.bits) {
            int b = mapped.bits[0], g = mapped.bits[1], r = mapped.bits[2];
            staging->Unmap();
            int mx = (r > g) ? r : g; if (b > mx) mx = b;
            int mn = (r < g) ? r : g; if (b < mn) mn = b;
            // 实测极差: 符合 Luminosity 语义时 143 (= SetLum+ClipColor 理论值), 否则 0.
            picked = (mx - mn > 38) ? (UINT)D2D1_BLEND_MODE_COLOR
                                    : (UINT)D2D1_BLEND_MODE_LUMINOSITY;
        }
    }
    sCached = (int)picked;
    return picked;
}

// ---------------------------------------------------------------------------
// Win11 acrylic 配色推导 — 把对外单参数 blurOpacity ("通透感") 折算成 WinUI
// AcrylicBrush 的两个内部通道: tint 层不透明度 + luminosity 混合色 (含 alpha).
// 算法与 WinUI GetTintOpacityModifier / GetLuminosityColor 一致, 常量同源.
// ---------------------------------------------------------------------------
struct AcrylicRecipe {
    float tintAlpha;   // tint 纯色层 SOURCE_OVER 不透明度 0..1
    float lumiR, lumiG, lumiB;  // luminosity 混合色 (straight, 0..1)
    float lumiA;       // 亮度锁定强度 0..1
};

static float XBlurDComp_Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// HSV: V = max 通道, S = (max-min)/max. 与 WinUI ColorConversion 同定义.
static void XBlurDComp_RgbToHsv(float r, float g, float b, float& h, float& s, float& v) {
    float mx = (r > g) ? r : g; if (b > mx) mx = b;
    float mn = (r < g) ? r : g; if (b < mn) mn = b;
    float d = mx - mn;
    v = mx;
    s = (mx <= 0.0f) ? 0.0f : (d / mx);
    if (d <= 0.0f) { h = 0.0f; return; }
    if      (mx == r) h = (g - b) / d;
    else if (mx == g) h = (b - r) / d + 2.0f;
    else              h = (r - g) / d + 4.0f;
    h *= 60.0f;
    if (h < 0.0f) h += 360.0f;
}

static void XBlurDComp_HsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
    if (s <= 0.0f) { r = g = b = v; return; }
    float hh = h / 60.0f;
    int   sector = (int)hh;
    float f = hh - (float)sector;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    switch (sector % 6) {
    case 0:  r = v; g = t; b = p; break;
    case 1:  r = q; g = v; b = p; break;
    case 2:  r = p; g = v; b = t; break;
    case 3:  r = p; g = q; b = v; break;
    case 4:  r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
}

// WinUI GetTintOpacityModifier: 按 tint 明度压制 tint 层最大不透明度 —
// 纯白 45%, 中灰 90%, 纯黑 85%, 饱和度越高压制越弱. 这条曲线是浅色主题"通透"
// 而深色主题"厚实"的来源: 近白 tint 只能盖住四成多, 桌面色度自然透出来.
static float XBlurDComp_TintOpacityModifier(float hsvV, float hsvS) {
    const float kMidPoint = 0.50f;
    const float kWhiteMaxOpacity = 0.45f;
    const float kMidPointMaxOpacity = 0.90f;
    const float kBlackMaxOpacity = 0.85f;

    if (hsvV == kMidPoint) return kMidPointMaxOpacity;

    float lowestMaxOpacity = (hsvV > kMidPoint) ? kWhiteMaxOpacity : kBlackMaxOpacity;
    float maxDeviation     = (hsvV > kMidPoint) ? (1.0f - kMidPoint) : kMidPoint;
    float maxSuppression   = kMidPointMaxOpacity - lowestMaxOpacity;
    if (hsvS > 0.0f) {
        float damp = 1.0f - hsvS * 2.0f;
        maxSuppression *= (damp > 0.0f) ? damp : 0.0f;
    }
    float deviation = (hsvV > kMidPoint) ? (hsvV - kMidPoint) : (kMidPoint - hsvV);
    return kMidPointMaxOpacity - maxSuppression * (deviation / maxDeviation);
}

static AcrylicRecipe ResolveAcrylicRecipe(int tintR, int tintG, int tintB, float blurOpacity) {
    float visibility  = XBlurDComp_Clamp01(blurOpacity);  // 对外"通透感"
    float tintOpacity = 1.0f - visibility;                // tint 层主导度

    float r = tintR / 255.0f, g = tintG / 255.0f, b = tintB / 255.0f;
    float h = 0.0f, s = 0.0f, v = 0.0f;
    XBlurDComp_RgbToHsv(r, g, b, h, s, v);

    float modifier = XBlurDComp_TintOpacityModifier(v, s);
    // 通透感取 0 时接口语义是"完全不通透 = 纯 tint", Win11 压制曲线会让近白 tint
    // 停在四成多, 与该语义冲突. 只在 0 附近线性放开压制, 保住端点又不产生跳变.
    const float kNoSuppressRamp = 0.08f;
    if (visibility < kNoSuppressRamp) {
        float t = visibility / kNoSuppressRamp;
        modifier = 1.0f + (modifier - 1.0f) * t;
    }

    AcrylicRecipe out{};
    out.tintAlpha = XBlurDComp_Clamp01(tintOpacity * modifier);

    // luminosity 混合色: 取 tint 的色相/饱和度, 明度夹到 [0.125, 0.965] —
    // 避免纯白/纯黑 tint 把 backdrop 亮度压成死白 / 死黑而彻底吃掉层次.
    float lumiV = v;
    if (lumiV < 0.125f) lumiV = 0.125f;
    if (lumiV > 0.965f) lumiV = 0.965f;
    XBlurDComp_HsvToRgb(h, s, lumiV, out.lumiR, out.lumiG, out.lumiB);

    // 亮度锁定强度由 tintOpacity 线性映射到 [0.15, 1.03] 后夹 1 (WinUI 同映射):
    // tint 越主导, 亮度越彻底交给 tint, 桌面明暗起伏残留越少.
    out.lumiA = XBlurDComp_Clamp01(0.15f + tintOpacity * (1.03f - 0.15f));
    return out;
}

static const DWORD kXBlurDcomp_DWMWA_WINDOW_CORNER_PREFERENCE = 33;
static const DWORD kDWMWCP_DONOTROUND            = 1;
static const DWORD kDWMWCP_ROUND                   = 2;

// 烤一张 256x256 CPU 白噪声进 CompositionDrawingSurface.
// NearestNeighbor 采样 + Stretch=None, 由 BorderEffect WRAP 平铺.
static void BakeNoiseSurface(WUC::CompositionGraphicsDevice const& graphicsDevice,
                             WUC::CompositionDrawingSurface& outSurface) {
    constexpr int   kSizeI = 256;
    constexpr float kSizeF = (float)kSizeI;

    std::vector<uint32_t> pixels((size_t)kSizeI * kSizeI);
    std::mt19937 rng(1u);
    for (auto& px : pixels) {
        uint32_t g = rng() & 0xFF;
        px = (0xFFu << 24) | (g << 16) | (g << 8) | g;
    }

    outSurface = graphicsDevice.CreateDrawingSurface(
        {kSizeF, kSizeF},
        WGDX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        WGDX::DirectXAlphaMode::Premultiplied);
    auto surfaceInterop = outSurface.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
    com_ptr<ID2D1DeviceContext> ctx;
    POINT offset{};
    check_hresult(surfaceInterop->BeginDraw(
        nullptr, __uuidof(ID2D1DeviceContext), ctx.put_void(), &offset));

    D2D1_BITMAP_PROPERTIES bmpProps = {};
    bmpProps.pixelFormat = { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED };
    bmpProps.dpiX = 96.0f;
    bmpProps.dpiY = 96.0f;
    com_ptr<ID2D1Bitmap> bitmap;
    check_hresult(ctx->CreateBitmap(
        D2D1::SizeU(kSizeI, kSizeI), pixels.data(), kSizeI * 4, &bmpProps, bitmap.put()));

    ctx->Clear(D2D1::ColorF(0, 0, 0, 0));
    D2D1_RECT_F dst = D2D1::RectF(
        (float)offset.x, (float)offset.y,
        (float)offset.x + kSizeF, (float)offset.y + kSizeF);
    ctx->DrawBitmap(bitmap.get(), &dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

    check_hresult(surfaceInterop->EndDraw());
}

// ===========================================================================
// Per-HWND 状态.
// ===========================================================================
struct HostState {
    WUC::Compositor compositor{nullptr};
    WUC::CompositionGraphicsDevice graphicsDevice{nullptr}; // per-Compositor, 不可跨窗共享
    WUCD::DesktopWindowTarget target{nullptr};
    WUC::ContainerVisual root{nullptr};
    WUC::SpriteVisual sprite{nullptr};
    WUC::CompositionDrawingSurface noiseSurface{nullptr};  // 长持引用
    WUC::CompositionRoundedRectangleGeometry roundGeom{nullptr};  // 圆角 clip 几何, 持引用方便 Resize 同步 size
    int   currentInset = 0;
    float cornerRadius = 0.0f;
    float borderInset  = 0.0f;  // visual 内缩 px, 让 owned 子窗描边正好压在透明区, Win11 标准描边视觉
};
static std::map<HWND, std::unique_ptr<HostState>> g_blurDcompHostMap;
static std::mutex                                 g_blurDcompMutex;

static bool EnsureHostGraphicsDevice_Locked(WUC::Compositor const& compositor, HostState& host) {
    if (host.graphicsDevice) return true;
    if (!EnsureSharedD3D_Locked()) return false;

    auto compInterop = compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
    com_ptr<ABI::Windows::UI::Composition::ICompositionGraphicsDevice> rawCG;
    HRESULT hr = compInterop->CreateGraphicsDevice(g_shared.d2dDevice.get(), rawCG.put());
    if (FAILED(hr)) return false;
    winrt::copy_from_abi(host.graphicsDevice, rawCG.get());
    return host.graphicsDevice != nullptr;
}

// 首次 Apply 时烤制 noise surface, 后续 Apply 复用 (避免每次重建 effect chain 重烤).
static WUC::CompositionSurfaceBrush GetOrCreateNoiseBrush(HostState& s) {
    if (!s.noiseSurface) {
        if (!s.graphicsDevice) return WUC::CompositionSurfaceBrush{nullptr};
        BakeNoiseSurface(s.graphicsDevice, s.noiseSurface);
    }
    auto brush = s.compositor.CreateSurfaceBrush(s.noiseSurface);
    brush.BitmapInterpolationMode(WUC::CompositionBitmapInterpolationMode::NearestNeighbor);
    brush.Stretch(WUC::CompositionStretch::None);
    return brush;
}

// 计算给定 HWND 应用 inset 后的 visual 矩形 (DIPs).
static void GetClientRectInset(HWND host, int inset, float& w, float& h) {
    RECT rc{};
    ::GetClientRect(host, &rc);
    int cw = rc.right - rc.left - inset * 2;
    int ch = rc.bottom - rc.top - inset * 2;
    if (cw < 0) cw = 0;
    if (ch < 0) ch = 0;
    w = (float)cw;
    h = (float)ch;
}

// 创建完整 effect chain, 返回 root effect (可拿去做 EffectFactory).
// 根据 useLuminosity / noiseAlphaPct 决定是否插入对应层.
//
// blurLayerOpacity = 1 - tint 层不透明度. 这里保留"不透明 tint 作 Composite 底 +
// backdrop 层带 alpha 叠上去"的方向 (与 WinUI"半透 tint 叠在不透明 backdrop 层上"
// 代数等价), 因为不透明底能保证输出 alpha 恒为 1 — GaussianBlur SOFT 边缘的 alpha
// 衰减不会漏成半透窗边.
static WGE::IGraphicsEffect BuildEffectChain(
    bool  useLuminosity,
    UINT  luminosityBlendMode,
    float saturation,
    float blurLayerOpacity,
    float noiseAlphaPct,
    /*out*/ winrt::com_ptr<BlendEffectImpl>&       outLumiBlend,
    /*out*/ winrt::com_ptr<ColorMatrixEffectImpl>& outSat,
    /*out*/ winrt::com_ptr<OpacityEffectImpl>&     outOpacity,
    /*out*/ winrt::com_ptr<CompositeEffectImpl>&   outComposite,
    /*out*/ winrt::com_ptr<BorderEffectImpl>&      outBorder,
    /*out*/ winrt::com_ptr<OpacityEffectImpl>&     outNoiseOpacity,
    /*out*/ winrt::com_ptr<BlendEffectImpl>&       outNoiseBlend)
{
    auto backdropParam = WUC::CompositionEffectSourceParameter(L"backdrop");
    auto tintParam     = WUC::CompositionEffectSourceParameter(L"tint");

    auto blurFx = winrt::make_self<GaussianBlurEffectImpl>();
    blurFx->StandardDeviation(30.0f);
    blurFx->Source(backdropParam);

    WGE::IGraphicsEffectSource backdropLayer = blurFx.as<WGE::IGraphicsEffectSource>();

    // 亮度锁定 = 真 Luminosity 混合: 亮度取 luminosity 纯色 (强度写在该色 alpha 上),
    // 色度留给桌面. 出界色由 D2D 的 ClipColor 按亮度整体收敛, 不会像加性矩阵那样
    // 逐通道 clamp 出偏色.
    if (useLuminosity) {
        auto lumiParam = WUC::CompositionEffectSourceParameter(L"luminosity");
        outLumiBlend = winrt::make_self<BlendEffectImpl>();
        outLumiBlend->Name(L"LuminosityBlend");
        outLumiBlend->Mode(luminosityBlendMode);
        outLumiBlend->Background(backdropLayer);
        outLumiBlend->Foreground(lumiParam);
        backdropLayer = outLumiBlend.as<WGE::IGraphicsEffectSource>();
    }

    // Saturation: ColorMatrix v*M, Rec.709 luma. 放在亮度锁定 *之后* —
    // 此时色度已被压到 tint 亮度附近, 提饱和不会撞 8bpc 中间缓冲上限;
    // 放在 blur 之后会先被 clamp 掉一部分, 提亮饱和反而丢色相.
    constexpr float Lr = 0.2126f, Lg = 0.7152f, Lb = 0.0722f;
    float inv = 1.0f - saturation;
    float satMat[20] = {
        saturation + inv*Lr,  inv*Lr,                inv*Lr,                0,
        inv*Lg,                saturation + inv*Lg,  inv*Lg,                0,
        inv*Lb,                inv*Lb,                saturation + inv*Lb,  0,
        0, 0, 0, 1,
        0, 0, 0, 0
    };
    outSat = winrt::make_self<ColorMatrixEffectImpl>();
    outSat->Name(L"SaturationMatrix");
    outSat->SetMatrix(satMat);
    outSat->Source(backdropLayer);

    outOpacity = winrt::make_self<OpacityEffectImpl>();
    outOpacity->Name(L"BlurOpacity");
    outOpacity->Opacity(blurLayerOpacity);
    outOpacity->Source(outSat.as<WGE::IGraphicsEffectSource>());

    outComposite = winrt::make_self<CompositeEffectImpl>();
    outComposite->Name(L"TintComposite");
    outComposite->Mode(0);
    outComposite->Destination(tintParam);
    outComposite->Source(outOpacity.as<WGE::IGraphicsEffectSource>());

    WGE::IGraphicsEffect finalRoot = outComposite.as<WGE::IGraphicsEffect>();

    if (noiseAlphaPct > 0.001f) {
        auto noiseParam = WUC::CompositionEffectSourceParameter(L"noise");

        outBorder = winrt::make_self<BorderEffectImpl>();
        outBorder->Name(L"NoiseTile");
        outBorder->SetExtendModes(1, 1);
        outBorder->Source(noiseParam);

        outNoiseOpacity = winrt::make_self<OpacityEffectImpl>();
        outNoiseOpacity->Name(L"NoiseOpacity");
        outNoiseOpacity->Opacity(noiseAlphaPct / 100.0f);
        outNoiseOpacity->Source(outBorder.as<WGE::IGraphicsEffectSource>());

        outNoiseBlend = winrt::make_self<BlendEffectImpl>();
        outNoiseBlend->Name(L"NoiseBlend");
        outNoiseBlend->Mode((UINT)D2D1_BLEND_MODE_MULTIPLY);
        outNoiseBlend->Foreground(outNoiseOpacity.as<WGE::IGraphicsEffectSource>());
        outNoiseBlend->Background(outComposite.as<WGE::IGraphicsEffectSource>());
        finalRoot = outNoiseBlend.as<WGE::IGraphicsEffect>();
    }

    return finalRoot;
}

// ===========================================================================
// 公共 API 实现
// ===========================================================================
namespace XBlurDComp {

bool IsSupported() {
    static std::atomic<int> sTriState{-1};
    int cached = sTriState.load(std::memory_order_relaxed);
    if (cached >= 0) return cached != 0;
    bool ok = false;
    if (XUITool_GetOsBuild() >= 17134) {
        // build + 运行时 DLL 探测 (DELAYLOAD 下 LoadLibrary 安全).
        HMODULE hDcomp = ::LoadLibraryW(L"dcomp.dll");
        HMODULE hCoreMsg = ::LoadLibraryW(L"CoreMessaging.dll");
        if (hDcomp && hCoreMsg) {
            ok = true;
        }
        if (hDcomp) ::FreeLibrary(hDcomp);
        if (hCoreMsg) ::FreeLibrary(hCoreMsg);
    }
    sTriState.store(ok ? 1 : 0, std::memory_order_relaxed);
    return ok;
}

bool Apply(HWND host,
           int tintR, int tintG, int tintB, int tintA,
           float blurOpacity,
           float saturation,
           BOOL uniformBrightness,
           float noiseAlphaPct,
           int shadowFrameInset)
{
    if (!host || !::IsWindow(host)) return false;
    if (!IsSupported()) return false;

    try {
        // blurOpacity<0 视为"未指定", 按 tintA 反算 (跟 ACCENT_ACRYLIC GradientColor.A
        // 语义一致: A=255 完全 tint, A=0 完全 blur).
        // blurOpacity>=0 视为用户显式 setter, 优先级高于 tintA 反算.
        if (blurOpacity < 0.0f) {
            blurOpacity = 1.0f - (tintA / 255.0f);
        }
        if (blurOpacity < 0.0f) blurOpacity = 0.0f;
        if (blurOpacity > 1.0f) blurOpacity = 1.0f;

        std::lock_guard<std::mutex> sharedLock(g_sharedMutex);
        if (!EnsureDispatcherQueue_Locked()) return false;

        std::lock_guard<std::mutex> hostLock(g_blurDcompMutex);
        auto it = g_blurDcompHostMap.find(host);
        bool firstAttach = (it == g_blurDcompHostMap.end());

        if (firstAttach) {
            auto state = std::make_unique<HostState>();
            state->compositor = WUC::Compositor();

            // DesktopWindowTarget 挂在现有 HWND (非 topmost), visual 在
            // redirection bitmap *下方*. 客户区被 paint 写 alpha=0 的地方就透出 visual.
            // 用 winrt::put_abi 让 winrt 项目自动管理引用计数, 避免手写 Release.
            namespace abid = ABI::Windows::UI::Composition::Desktop;
            auto interop = state->compositor.as<abid::ICompositorDesktopInterop>();
            HRESULT hr = interop->CreateDesktopWindowTarget(
                host, /*isTopmost*/FALSE,
                reinterpret_cast<abid::IDesktopWindowTarget**>(winrt::put_abi(state->target)));
            if (FAILED(hr) || !state->target) return false;

            state->root = state->compositor.CreateContainerVisual();
            state->sprite = state->compositor.CreateSpriteVisual();
            state->root.Children().InsertAtTop(state->sprite);
            state->target.Root(state->root);

            it = g_blurDcompHostMap.emplace(host, std::move(state)).first;
        }

        HostState& s = *it->second;

        if (!EnsureHostGraphicsDevice_Locked(s.compositor, s)) {
            // 装不出来 D2D/CompositionGraphics — 直接卸 (避免半成品).
            if (firstAttach) g_blurDcompHostMap.erase(it);
            return false;
        }

        // 通透感 → tint 层不透明度 + 亮度锁定强度 (两个独立通道, 见 ResolveAcrylicRecipe).
        AcrylicRecipe recipe = ResolveAcrylicRecipe(tintR, tintG, tintB, blurOpacity);
        bool useLuminosity = (uniformBrightness != FALSE);
        UINT lumiMode = useLuminosity ? DetectLuminosityBlendMode_Locked()
                                      : (UINT)D2D1_BLEND_MODE_COLOR;

        // 重建 effect chain (反映新参数).
        winrt::com_ptr<ColorMatrixEffectImpl> sat;
        winrt::com_ptr<OpacityEffectImpl>     blurOp, noiseOp;
        winrt::com_ptr<CompositeEffectImpl>   composite;
        winrt::com_ptr<BorderEffectImpl>      border;
        winrt::com_ptr<BlendEffectImpl>       lumiBlend, noiseBlend;
        auto root = BuildEffectChain(
            useLuminosity,
            lumiMode,
            saturation,
            1.0f - recipe.tintAlpha,
            noiseAlphaPct,
            lumiBlend, sat, blurOp, composite, border, noiseOp, noiseBlend);

        auto factory = s.compositor.CreateEffectFactory(root);
        auto effectBrush = factory.CreateBrush();

        auto backdrop = s.compositor.CreateBackdropBrush();
        effectBrush.SetSourceParameter(L"backdrop", backdrop);

        // tint 作 Composite 不透明底 (alpha=255), 可见比例由 backdrop 层的
        // OpacityEffect(1 - tintAlpha) 决定.
        auto tintBrush = s.compositor.CreateColorBrush(
            winrt::Windows::UI::Color{ 255, (uint8_t)tintR, (uint8_t)tintG, (uint8_t)tintB });
        effectBrush.SetSourceParameter(L"tint", tintBrush);

        if (useLuminosity) {
            auto toByte = [](float v) -> uint8_t {
                int i = (int)(v * 255.0f + 0.5f);
                if (i < 0)   i = 0;
                if (i > 255) i = 255;
                return (uint8_t)i;
            };
            auto lumiBrush = s.compositor.CreateColorBrush(
                winrt::Windows::UI::Color{ toByte(recipe.lumiA), toByte(recipe.lumiR),
                                           toByte(recipe.lumiG), toByte(recipe.lumiB) });
            effectBrush.SetSourceParameter(L"luminosity", lumiBrush);
        }

        if (noiseAlphaPct > 0.001f) {
            auto noiseBrush = GetOrCreateNoiseBrush(s);
            if (noiseBrush) {
                effectBrush.SetSourceParameter(L"noise", noiseBrush);
            }
        }

        s.sprite.Brush(effectBrush);
        s.currentInset = shadowFrameInset;

        // 同步 visual 大小 & inset 偏移.
        float w, h;
        GetClientRectInset(host, shadowFrameInset, w, h);
        s.root.Size({w + shadowFrameInset * 2.0f, h + shadowFrameInset * 2.0f});
        s.sprite.Offset({(float)shadowFrameInset, (float)shadowFrameInset, 0.0f});
        s.sprite.Size({w, h});

        return true;
    }
    catch (winrt::hresult_error const&) {
        return false;
    }
    catch (...) {
        return false;
    }
}

void SetCornerRadius(HWND host, float radius, float borderInset) {
    if (!host) return;
    std::lock_guard<std::mutex> hostLock(g_blurDcompMutex);
    auto it = g_blurDcompHostMap.find(host);
    if (it == g_blurDcompHostMap.end() || !it->second) return;
    HostState& s = *it->second;
    if (!s.root || !s.compositor) return;

    s.cornerRadius = radius;
    s.borderInset  = borderInset;
    try {
        if (radius <= 0.0f && borderInset <= 0.0f) {
            // 关闭裁切 + 内缩, 还原原状.
            s.root.Clip(nullptr);
            s.roundGeom = nullptr;
            s.sprite.Offset({(float)s.currentInset, (float)s.currentInset, 0.0f});
            return;
        }
        // 圆角 clip 走 root.Clip — visual 之外 4 角不渲染 = 透明.
        if (radius > 0.0f) {
            if (!s.roundGeom) {
                s.roundGeom = s.compositor.CreateRoundedRectangleGeometry();
                auto clip = s.compositor.CreateGeometricClip(s.roundGeom);
                s.root.Clip(clip);
                // BorderMode::Hard 关 visual 边缘 antialiasing, 防跟 GeometricClip 抗锯齿叠加变粗.
                s.root.BorderMode(WUC::CompositionBorderMode::Hard);
            }
        }
        // 重新算 visual rect 加上 borderInset (在 shadowFrameInset 之上再缩).
        float w, h;
        GetClientRectInset(host, s.currentInset, w, h);
        float spriteW = w - 2.0f * borderInset; if (spriteW < 0.0f) spriteW = 0.0f;
        float spriteH = h - 2.0f * borderInset; if (spriteH < 0.0f) spriteH = 0.0f;
        // sprite 装 effect 内容, offset 加上 borderInset, size 缩 2*borderInset.
        s.sprite.Offset({(float)s.currentInset + borderInset,
                         (float)s.currentInset + borderInset, 0.0f});
        s.sprite.Size({spriteW, spriteH});
        // root 不变 (root 是整个 acrylic HWND 大小), clip geom 用 sprite 同 rect.
        if (s.roundGeom) {
            // GeometricClip 是相对 root visual 坐标系, root 没动 offset. 让 geom 匹配 sprite
            // 区域. 但 RoundedRectangleGeometry 没 Offset, 我们改 root.Clip 为带 inset 的 clip.
            // 简化: 直接给 geom Size 用 sprite size, 然后 clip 加在 sprite 上而不是 root.
            // 实际: 把 clip 从 root 移到 sprite, geom 跟 sprite 同 size.
            s.root.Clip(nullptr);
            s.sprite.Clip(s.compositor.CreateGeometricClip(s.roundGeom));
            s.sprite.BorderMode(WUC::CompositionBorderMode::Hard);
            s.roundGeom.Size({spriteW, spriteH});
            s.roundGeom.CornerRadius({radius, radius});
        }
    } catch (...) {}
}

void Resize(HWND host, int shadowFrameInset) {
    if (!host) return;
    std::lock_guard<std::mutex> hostLock(g_blurDcompMutex);
    auto it = g_blurDcompHostMap.find(host);
    if (it == g_blurDcompHostMap.end() || !it->second) return;
    HostState& s = *it->second;
    if (!s.sprite) return;

    s.currentInset = shadowFrameInset;
    float w, h;
    GetClientRectInset(host, shadowFrameInset, w, h);
    try {
        s.root.Size({w + shadowFrameInset * 2.0f, h + shadowFrameInset * 2.0f});
        // sprite 内缩 borderInset (在 shadowFrameInset 之上).
        float spriteW = w - 2.0f * s.borderInset; if (spriteW < 0.0f) spriteW = 0.0f;
        float spriteH = h - 2.0f * s.borderInset; if (spriteH < 0.0f) spriteH = 0.0f;
        s.sprite.Offset({(float)shadowFrameInset + s.borderInset,
                         (float)shadowFrameInset + s.borderInset, 0.0f});
        s.sprite.Size({spriteW, spriteH});
        // 圆角 clip geometry 跟随 sprite size (CornerRadius 不变).
        if (s.roundGeom) {
            s.roundGeom.Size({spriteW, spriteH});
        }
    } catch (...) {}
}

void Disable(HWND host) {
    if (!host) return;
    std::lock_guard<std::mutex> hostLock(g_blurDcompMutex);
    auto it = g_blurDcompHostMap.find(host);
    if (it == g_blurDcompHostMap.end()) return;
    // 析构顺序: target → root → sprite. winrt 智能指针自动 Release.
    g_blurDcompHostMap.erase(it);
}

// ============================================================================
// AttachAcrylicHost — 1:1 移植自 test_blur_main PoC wWinMain 里手写的 acrylic 块.
// ----------------------------------------------------------------------------
// 全部内部状态用 static (TU-local) 私有, subclass procs 也是 static. 跟 test_blur_main
// 里的同名 static 不冲突 (不同 TU 各有一份, link 不见).
// ============================================================================

namespace {

struct AcrylicHostEntry {
	HWND acrylicHwnd = NULL;
	HWND xcguiHwnd   = NULL;
	HWND originalOwner = NULL;
	LONG_PTR originalExStyle = 0;
};
static std::map<HWND, AcrylicHostEntry> s_acrylicByXcgui; // key = xcgui HWND
static std::mutex                       s_acrylicMutex;

// Acrylic 外扩 device pixels — acrylic HWND 比 XCGUI HWND 4 边各大 N 像素, 让 acrylic
// 系统描边 (DWM 1px BORDER) 露在 XCGUI 边外, 不被 XCGUI alpha 客户区覆盖.
static constexpr int kAcrylicOuterPx = 1;

static void XBlurDComp_SetAcrylicCornerPref(HWND acrylicHwnd, bool maximized){
	DWORD pref = maximized ? kDWMWCP_DONOTROUND : kDWMWCP_ROUND;
	::DwmSetWindowAttribute(acrylicHwnd, kXBlurDcomp_DWMWA_WINDOW_CORNER_PREFERENCE,
	                        &pref, sizeof(pref));
}

// Acrylic backdrop 自己的 subclass — hook 它收到的 WM_DPICHANGED.
static LRESULT CALLBACK AcrylicWndSubclassProc(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
	switch (msg){
	case WM_DPICHANGED:
		{
			HWND ownedTop = ::GetWindow(hwnd, GW_HWNDPREV); // owned 在 owner 之上
			bool maxd = false;
			if (ownedTop) maxd = (::GetWindowLongPtrW(ownedTop, GWL_STYLE) & WS_MAXIMIZE) != 0;
			XBlurDComp_SetAcrylicCornerPref(hwnd, maxd);
			::SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
		break;
	case WM_NCDESTROY:
		::RemoveWindowSubclass(hwnd, AcrylicWndSubclassProc, uIdSubclass);
		break;
	}
	return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// XCGUI WndProc subclass — 同步 acrylic backdrop 位置/大小/可见性/topmost/销毁.
// dwRefData = acrylic backdrop HWND.
static LRESULT CALLBACK XcguiToAcrylicSyncProc(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	HWND acrylicBackdrop = (HWND)dwRefData;
	switch (msg){
	case WM_SHOWWINDOW:
		// acrylic 是 XCGUI 主窗的 owner。隐藏 owned 主窗不会自动隐藏 owner，
		// 托盘模式因此会只留下空白亚克力背板；这里显式同步普通显示/隐藏。
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
			if (wParam && !::IsIconic(hwnd))
				::ShowWindow(acrylicBackdrop, SW_SHOWNA);
			else
				::ShowWindow(acrylicBackdrop, SW_HIDE);
		}
		break;
	case WM_WINDOWPOSCHANGING: {
		LRESULT r = ::DefSubclassProc(hwnd, msg, wParam, lParam);
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
			WINDOWPOS* wp = (WINDOWPOS*)lParam;
			if (wp && !((wp->flags & SWP_NOSIZE) && (wp->flags & SWP_NOMOVE))){
				UINT flags = SWP_NOACTIVATE | SWP_NOZORDER;
				if (wp->flags & SWP_NOMOVE) flags |= SWP_NOMOVE;
				if (wp->flags & SWP_NOSIZE) flags |= SWP_NOSIZE;
				int aw = wp->cx + 2 * kAcrylicOuterPx; if (aw < 1) aw = 1;
				int ah = wp->cy + 2 * kAcrylicOuterPx; if (ah < 1) ah = 1;
				::SetWindowPos(acrylicBackdrop, NULL,
					wp->x - kAcrylicOuterPx, wp->y - kAcrylicOuterPx,
					aw, ah, flags);
				if (!(wp->flags & SWP_NOSIZE)){
					XBlurDComp::Resize(acrylicBackdrop, 0);
				}
			}
		}
		return r;
	}
	case WM_SIZE:
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
			if (wParam == SIZE_MINIMIZED){
				::ShowWindow(acrylicBackdrop, SW_HIDE);
			} else {
				if (!::IsWindowVisible(acrylicBackdrop)){
					::ShowWindow(acrylicBackdrop, SW_SHOWNA);
				}
				XBlurDComp_SetAcrylicCornerPref(acrylicBackdrop, wParam == SIZE_MAXIMIZED);
			}
		}
		break;
	case WM_DPICHANGED:
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
			bool maxd = (::GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_MAXIMIZE) != 0;
			XBlurDComp_SetAcrylicCornerPref(acrylicBackdrop, maxd);
			::SetWindowPos(acrylicBackdrop, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
		break;
	case WM_WINDOWPOSCHANGED:
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
			WINDOWPOS* wp = (WINDOWPOS*)lParam;
			// 部分框架路径只通过 WINDOWPOS 标志改变可见性，未必单独发送 WM_SHOWWINDOW。
			if (wp && (wp->flags & SWP_HIDEWINDOW))
				::ShowWindow(acrylicBackdrop, SW_HIDE);
			else if (wp && (wp->flags & SWP_SHOWWINDOW) && !::IsIconic(hwnd))
				::ShowWindow(acrylicBackdrop, SW_SHOWNA);

			LONG_PTR exXcgui   = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
			LONG_PTR exAcrylic = ::GetWindowLongPtrW(acrylicBackdrop, GWL_EXSTYLE);
			bool xcguiTop   = (exXcgui   & WS_EX_TOPMOST) != 0;
			bool acrylicTop = (exAcrylic & WS_EX_TOPMOST) != 0;
			if (xcguiTop != acrylicTop){
				::SetWindowPos(acrylicBackdrop,
					xcguiTop ? HWND_TOPMOST : HWND_NOTOPMOST,
					0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}
		}
		break;
	case WM_NCDESTROY:
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
			::RemoveWindowSubclass(acrylicBackdrop, AcrylicWndSubclassProc, 0xACDC);
			XBlurDComp::Disable(acrylicBackdrop);
			::DestroyWindow(acrylicBackdrop);
		}
		{
			std::lock_guard<std::mutex> lk(s_acrylicMutex);
			s_acrylicByXcgui.erase(hwnd);
		}
		::RemoveWindowSubclass(hwnd, XcguiToAcrylicSyncProc, uIdSubclass);
		break;
	}
	return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

} // anonymous namespace

HWND AttachAcrylicHost(void* hxwOpaque,
                       int tintR, int tintG, int tintB, int tintA,
                       float blurOpacity,
                       float saturation,
                       BOOL  uniformBrightness,
                       float noiseAlphaPct)
{
	HWINDOW hWnd = (HWINDOW)hxwOpaque;
	if (!hWnd || !::XC_IsHWINDOW((HXCGUI)hWnd)) return NULL;
	HWND xcguiHwnd = ::XWnd_GetHWND(hWnd);
	if (!xcguiHwnd) return NULL;
	// 保存 Attach 前的窗口关系。XModalWnd / owned popup 已经在这里携带业务 owner，
	// acrylic 必须继承它，不能让后续 owner-owned 架构把模态关系截断。
	HWND originalOwner = (HWND)::GetWindowLongPtrW(xcguiHwnd, GWLP_HWNDPARENT);
	if (originalOwner && !::IsWindow(originalOwner)) originalOwner = NULL;
	LONG_PTR originalExStyle = ::GetWindowLongPtrW(xcguiHwnd, GWL_EXSTYLE);

	// 已 attach 过 → 仅刷新 effect chain, 不重复建窗.
	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(xcguiHwnd);
		if (it != s_acrylicByXcgui.end()){
			HWND existing = it->second.acrylicHwnd;
			if (existing && ::IsWindow(existing)){
				XBlurDComp::Apply(existing,
					tintR, tintG, tintB, tintA,
					blurOpacity, saturation, uniformBrightness, noiseAlphaPct, 0);
				return existing;
			}
			s_acrylicByXcgui.erase(it);
		}
	}

	// 1. 注册 acrylic backdrop 窗口类
	static const wchar_t* kAcCls = L"XBlurAcrylicBackdrop_PoC";
	static std::atomic<bool> sClsRegistered{false};
	if (!sClsRegistered.exchange(true)){
		WNDCLASSW wc = {};
		wc.lpfnWndProc = ::DefWindowProcW;
		wc.hInstance = GetModuleHandleW(NULL);
		wc.lpszClassName = kAcCls;
		wc.hbrBackground = NULL;
		wc.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
		::RegisterClassW(&wc);
	}

	// 2. 创建 acrylic backdrop (Win7 无 SetThreadDpiAwarenessContext, 动态解析)
	typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
	static auto pfnSetThreadDpiAwarenessContext = (PFN_SetThreadDpiAwarenessContext)
		::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext");

	DPI_AWARENESS_CONTEXT prevDpiCtx = NULL;
	if (pfnSetThreadDpiAwarenessContext) {
		prevDpiCtx = pfnSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}

	RECT xcRect; ::GetWindowRect(xcguiHwnd, &xcRect);
	HWND acrylicBackdrop = ::CreateWindowExW(
		WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		kAcCls, L"",
		WS_POPUP,
		xcRect.left - kAcrylicOuterPx, xcRect.top - kAcrylicOuterPx,
		(xcRect.right - xcRect.left) + 2 * kAcrylicOuterPx,
		(xcRect.bottom - xcRect.top) + 2 * kAcrylicOuterPx,
		originalOwner, NULL, GetModuleHandleW(NULL), NULL);

	if (pfnSetThreadDpiAwarenessContext && prevDpiCtx) {
		pfnSetThreadDpiAwarenessContext(prevDpiCtx);
	}
	if (!acrylicBackdrop) return NULL;

	// 3. 装 PoC dark effect on acrylic backdrop.
	XBlurDComp::Apply(acrylicBackdrop,
		tintR, tintG, tintB, tintA,
		blurOpacity, saturation, uniformBrightness, noiseAlphaPct, 0);

	// 4. XCGUI 整窗叠 1% 不透明度底色 — 视觉看不见但 layered hit-test 命中, 边缘 resize / 拖动
	//    才能触发. EnableDrawBk(FALSE) 让 alpha=0 layered 全透 → 整窗鼠标穿透 → 不能 resize.
	//    *这 4 个 XCGUI API 必须由调用方 (test_blur_main TU) 调用* — 实测发现把它们放在 dcomp.cpp
	//    TU 调时 XCGUI 内部行为有微妙差异 (高 DPI 下视觉错位), 在调用方 TU 调则正确.
	//    本接口约定调用方在调本接口 *之后* 紧跟下面 4 行.
	//
	//    XWnd_SetTransparentType(hWnd, window_transparent_shaped);
	//    XWnd_SetTransparentAlpha(hWnd, 255);
	//    XWnd_SetBkInfo(hWnd, L"{99:1.9.9;98:1(0);5:2(15)20(1)21(3)26(1)22(16777216)23(1)9(8,8,8,8);}");
	//    XWnd_EnableDragWindow(hWnd, TRUE);

	// 5. 建立 owner-owned 关系。若 XCGUI 原来已有 owner，CreateWindowExW 已先把
	//    acrylic 挂到原 owner；现在再把 XCGUI 挂到 acrylic，完整保留模态/owned 链。
	::SetWindowLongPtrW(xcguiHwnd, GWLP_HWNDPARENT, (LONG_PTR)acrylicBackdrop);

	// 6. 仅原本无 owner 的普通顶层窗口需要强制显示到 taskbar。模态/owned 窗口
	//    保持原扩展样式，避免附加模糊后意外多出独立任务栏按钮。
	if (!originalOwner){
		::SetWindowLongPtrW(xcguiHwnd, GWL_EXSTYLE, originalExStyle | WS_EX_APPWINDOW);
	}

	// 7. acrylic 用系统默认 frame: Win11 自动加 round corner + BORDER_COLOR + frame shadow.
	//    XCGUI 主窗这边由调用方自己控.

	// 8a. acrylic 自己装 subclass 处理 WM_DPICHANGED — acrylic 是独立 top-level,
	//     系统直接给它发 DPI 改变, 但 DefWindowProc 不刷新 frame attribute. 必须 hook.
	::SetWindowSubclass(acrylicBackdrop, AcrylicWndSubclassProc, 0xACDC, 0);

	// 8. Subclass XCGUI 同步 acrylic backdrop 的位置 / 大小 / 可见性 / topmost / 销毁.
	::SetWindowSubclass(xcguiHwnd, XcguiToAcrylicSyncProc, 0xACBD, (DWORD_PTR)acrylicBackdrop);

	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		s_acrylicByXcgui[xcguiHwnd] = {
			acrylicBackdrop, xcguiHwnd, originalOwner, originalExStyle
		};
	}

	// 9. 显示 acrylic backdrop (SW_SHOWNA 不抢焦点).
	::ShowWindow(acrylicBackdrop, SW_SHOWNA);

	// 10. 触发 XCGUI 的 taskbar flag 刷新 (hide → show), 不然 WS_EX_APPWINDOW 可能不立即生效.
	if (::IsWindowVisible(xcguiHwnd)){
		::ShowWindow(xcguiHwnd, SW_HIDE);
		::ShowWindow(xcguiHwnd, SW_SHOW);
	}

	return acrylicBackdrop;
}

HWND GetAcrylicHwnd(void* hxwOpaque){
	if (!hxwOpaque) return NULL;
	HWINDOW hWnd = (HWINDOW)hxwOpaque;
	if (!::XC_IsHWINDOW((HXCGUI)hWnd)) return NULL;
	HWND xcguiHwnd = ::XWnd_GetHWND(hWnd);
	if (!xcguiHwnd) return NULL;
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	if (it == s_acrylicByXcgui.end()) return NULL;
	HWND ac = it->second.acrylicHwnd;
	return (ac && ::IsWindow(ac)) ? ac : NULL;
}

void DetachAcrylicHost(void* hxwOpaque){
	HWND xcguiHwnd = NULL;
	HWINDOW hWnd = (HWINDOW)hxwOpaque;
	if (hWnd && ::XC_IsHWINDOW((HXCGUI)hWnd)){
		xcguiHwnd = ::XWnd_GetHWND(hWnd);
	}

	HWND acrylicBackdrop = NULL;
	HWND originalOwner = NULL;
	LONG_PTR originalExStyle = 0;
	bool hasEntry = false;
	if (xcguiHwnd){
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(xcguiHwnd);
		if (it != s_acrylicByXcgui.end()){
			acrylicBackdrop = it->second.acrylicHwnd;
			originalOwner = it->second.originalOwner;
			originalExStyle = it->second.originalExStyle;
			hasEntry = true;
			s_acrylicByXcgui.erase(it);
		}
		::RemoveWindowSubclass(xcguiHwnd, XcguiToAcrylicSyncProc, 0xACBD);
		// 只在 owner 仍是本实例创建的 acrylic 时恢复，避免覆盖调用方在附加后
		// 主动设置的新 owner。扩展样式也只恢复本模块改动的 APPWINDOW 位。
		if (hasEntry){
			HWND currentOwner = (HWND)::GetWindowLongPtrW(xcguiHwnd, GWLP_HWNDPARENT);
			if (currentOwner == acrylicBackdrop){
				HWND restoreOwner = (originalOwner && ::IsWindow(originalOwner))
					? originalOwner : NULL;
				::SetWindowLongPtrW(xcguiHwnd, GWLP_HWNDPARENT, (LONG_PTR)restoreOwner);
			}
			LONG_PTR currentExStyle = ::GetWindowLongPtrW(xcguiHwnd, GWL_EXSTYLE);
			LONG_PTR restoredExStyle = (currentExStyle & ~((LONG_PTR)WS_EX_APPWINDOW))
				| (originalExStyle & WS_EX_APPWINDOW);
			::SetWindowLongPtrW(xcguiHwnd, GWL_EXSTYLE, restoredExStyle);
		}
	}

	if (acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
		::RemoveWindowSubclass(acrylicBackdrop, AcrylicWndSubclassProc, 0xACDC);
		Disable(acrylicBackdrop);
		::DestroyWindow(acrylicBackdrop);
	}
}

} // namespace XBlurDComp
