// DComp 亚克力合成实现；每个 HWND 独立持有 Compositor 和效果链。

#include "xcgui_blur_dcomp.h"

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
#include <dcomp.h>
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
#include <stdio.h>
#include <stdarg.h>

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

// Windows 7 兼容性火墙
// 炫彩 IDE 会把本文件合并为 module_xcgui_uitool.obj。MSVC 不接受从对象
// .drectve 传入 /DELAYLOAD，使用 pragma 会产生 LNK4229 并被忽略。本模块不
// 静态链接 DComp/WindowsApp/CoreMessaging；代码侧在进入 DComp 路径前通过
// LoadLibraryW/GetProcAddress 和 C++/WinRT 自身的运行时解析完成能力探测。

namespace ABI_GE = ABI::Windows::Graphics::Effects;
namespace WGE    = winrt::Windows::Graphics::Effects;
namespace WUC    = winrt::Windows::UI::Composition;
namespace WUCD   = winrt::Windows::UI::Composition::Desktop;
namespace WGDX   = winrt::Windows::Graphics::DirectX;
using winrt::com_ptr;
using winrt::check_hresult;

// D2D effect CLSID (字字核对自 d2d1effects.h / d2d1effects_2.h).
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

// GaussianBlurEffectImpl — 模糊本体. CLSID 用 d2d1effects_2.h 提供的常量.
// 属性: 0=StandardDeviation float, 1=Optimization UInt32, 2=BorderMode UInt32
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

// ColorMatrixEffectImpl — 5x4 矩阵变换 RGBA. v*M 顺序 (每行=输入通道贡献).
// 用 2 次: saturation 矩阵 + luminosity replace 矩阵 + (noise grayscale 矩阵).
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

// OpacityEffectImpl — alpha 乘常数. 用于 blur 半透 + noise 3% dim.
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

// CompositeEffectImpl — Porter-Duff. mode=0 SOURCE_OVER, source[0]=dest, source[1]=src.
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

// BlendEffectImpl — Photoshop 风混合. mode=0 MULTIPLY 用于 noise 叠层.
// *Source 顺序*: 0=Background (底), 1=Foreground (顶) — 跟 Win2D 一致.
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

// BorderEffectImpl — source 边缘扩展. mode 1=WRAP 用于 noise 平铺.
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

// 全局共享: dispatcher queue + D3D/D2D 设备 (可跨 Compositor 共享).
// CompositionGraphicsDevice 必须 per-Compositor, 存 HostState 里.
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

// Per-HWND 状态.
struct HostState {
    WUC::Compositor compositor{nullptr};
    WUC::CompositionGraphicsDevice graphicsDevice{nullptr}; // per-Compositor, 不可跨窗共享
    WUCD::DesktopWindowTarget target{nullptr};
    WUC::ContainerVisual root{nullptr};
    WUC::SpriteVisual sprite{nullptr};
    com_ptr<IDCompositionDevice> uiDevice;
    com_ptr<IDCompositionTarget> uiTarget;
    com_ptr<IDCompositionVisual> uiVisual;
    com_ptr<IUnknown> uiSurface;
    HWND uiSource = NULL;
    WUC::CompositionDrawingSurface noiseSurface{nullptr};  // 长持引用
    WUC::CompositionRoundedRectangleGeometry roundGeom{nullptr};  // 圆角 clip 几何, 持引用方便 Resize 同步 size
    int   currentInset = 0;
    float cornerRadius = 0.0f;
    float borderInset  = 0.0f;  // visual 内缩 px, 让 owned 子窗描边正好压在透明区, Win11 标准描边视觉
};
static std::map<HWND, std::unique_ptr<HostState>> g_blurDcompHostMap;
static std::mutex                                 g_blurDcompMutex;

// 让 acrylic 身份窗的合成树直接引用 XCGUI layered 窗的实时表面。
// 每次还原重建 target 时重建引用；不在窗口消息循环里做像素抓取。
static bool XBlurDComp_AttachUiSurface(HWND host, HWND source){
    if (!host || !source || !::IsWindow(source)) return false;
    std::lock_guard<std::mutex> lk(g_blurDcompMutex);
    auto it = g_blurDcompHostMap.find(host);
    if (it == g_blurDcompHostMap.end()) return false;
    HostState& s = *it->second;
    if (s.uiSource == source && s.uiTarget) return true;
    HMODULE mod = ::GetModuleHandleW(L"dcomp.dll");
    if (!mod) mod = ::LoadLibraryW(L"dcomp.dll");
    if (!mod) return false;
    using CreateDeviceFn = HRESULT (WINAPI*)(IDXGIDevice*, REFIID, void**);
    auto createDevice = reinterpret_cast<CreateDeviceFn>(
        ::GetProcAddress(mod, "DCompositionCreateDevice"));
    if (!createDevice) return false;
    com_ptr<IDCompositionDevice> device;
    com_ptr<IDCompositionTarget> target;
    com_ptr<IDCompositionVisual> visual;
    com_ptr<IUnknown> surface;
    HRESULT hr = createDevice(NULL, __uuidof(IDCompositionDevice), device.put_void());
    if (SUCCEEDED(hr)) hr = device->CreateTargetForHwnd(host, TRUE, target.put());
    if (SUCCEEDED(hr)) hr = device->CreateVisual(visual.put());
    if (SUCCEEDED(hr)) hr = device->CreateSurfaceFromHwnd(source, surface.put());
    if (SUCCEEDED(hr)) hr = visual->SetContent(surface.get());
    RECT sourceRect{}, hostRect{};
    ::GetWindowRect(source, &sourceRect);
    ::GetWindowRect(host, &hostRect);
    if (SUCCEEDED(hr)) hr = visual->SetOffsetX((float)(sourceRect.left - hostRect.left));
    if (SUCCEEDED(hr)) hr = visual->SetOffsetY((float)(sourceRect.top - hostRect.top));
    if (SUCCEEDED(hr)) hr = target->SetRoot(visual.get());
    if (SUCCEEDED(hr)) hr = device->Commit();
    if (SUCCEEDED(hr)) hr = device->WaitForCommitCompletion();
    if (FAILED(hr)) return false;
    s.uiDevice = std::move(device);
    s.uiTarget = std::move(target);
    s.uiVisual = std::move(visual);
    s.uiSurface = std::move(surface);
    s.uiSource = source;
    return true;
}

static void XBlurDComp_DetachUiSurface(HWND host){
    std::lock_guard<std::mutex> lk(g_blurDcompMutex);
    auto it = g_blurDcompHostMap.find(host);
    if (it == g_blurDcompHostMap.end()) return;
    HostState& s = *it->second;
    // 释放 COM 指针并不等于从 DWM 的合成树中移除 visual。若 target 仍引用
    // 上次最小化时的 XCGUI 表面，还原后会把那一帧覆盖在真实窗口上：控件
    // 能收到鼠标事件，但 hover、文字更新都被旧画面遮住。
    if (s.uiTarget && s.uiDevice){
        if (SUCCEEDED(s.uiTarget->SetRoot(nullptr)) &&
            SUCCEEDED(s.uiDevice->Commit()))
            s.uiDevice->WaitForCommitCompletion();
    }
    s.uiTarget = nullptr;
    s.uiVisual = nullptr;
    s.uiSurface = nullptr;
    s.uiDevice = nullptr;
    s.uiSource = NULL;
}

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
static WGE::IGraphicsEffect BuildEffectChain(
    bool  useLuminosity,
    int   tintR, int tintG, int tintB,
    float saturation,
    float blurOpacity,
    float noiseAlphaPct,
    /*out*/ winrt::com_ptr<ColorMatrixEffectImpl>& outSat,
    /*out*/ winrt::com_ptr<ColorMatrixEffectImpl>& outLumi,
    /*out*/ winrt::com_ptr<OpacityEffectImpl>&     outOpacity,
    /*out*/ winrt::com_ptr<CompositeEffectImpl>&   outComposite,
    /*out*/ winrt::com_ptr<BorderEffectImpl>&      outBorder,
    /*out*/ winrt::com_ptr<OpacityEffectImpl>&     outNoiseOpacity,
    /*out*/ winrt::com_ptr<BlendEffectImpl>&       outBlend)
{
    auto backdropParam = WUC::CompositionEffectSourceParameter(L"backdrop");
    auto tintParam     = WUC::CompositionEffectSourceParameter(L"tint");

    // 保留局部色块，再用少量宽核把颜色带入相邻区域。不做整窗取色或壁纸平均。
    // Composition 要求树形效果图；两个分支使用独立节点，绑定同一 backdrop。
    constexpr float localSigma = 36.0f;
    constexpr float diffusionSigma = 88.0f;
    constexpr float diffusionWeight = 0.28f;
    auto blurFx = winrt::make_self<GaussianBlurEffectImpl>();
    blurFx->Name(L"LocalBlur");
    blurFx->StandardDeviation(localSigma);
    blurFx->Source(backdropParam);

    auto diffusionFx = winrt::make_self<GaussianBlurEffectImpl>();
    diffusionFx->Name(L"ColorDiffusion");
    diffusionFx->StandardDeviation(diffusionSigma);
    diffusionFx->Source(WUC::CompositionEffectSourceParameter(L"backdrop"));

    auto diffusionOpacity = winrt::make_self<OpacityEffectImpl>();
    diffusionOpacity->Opacity(diffusionWeight);
    diffusionOpacity->Source(diffusionFx.as<WGE::IGraphicsEffectSource>());

    auto diffusionComposite = winrt::make_self<CompositeEffectImpl>();
    diffusionComposite->Name(L"DiffusedBackdrop");
    diffusionComposite->Destination(blurFx.as<WGE::IGraphicsEffectSource>());
    diffusionComposite->Source(diffusionOpacity.as<WGE::IGraphicsEffectSource>());

    // Saturation: ColorMatrix v*M, Rec.709 luma.
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
    outSat->Source(diffusionComposite.as<WGE::IGraphicsEffectSource>());

    WGE::IGraphicsEffectSource lumiSrc = outSat.as<WGE::IGraphicsEffectSource>();
    if (useLuminosity) {
        float L_tint = (Lr*tintR + Lg*tintG + Lb*tintB) / 255.0f;
        float lumiMat[20] = {
            1.0f - Lr,  -Lr,         -Lr,         0,
            -Lg,         1.0f - Lg,  -Lg,         0,
            -Lb,         -Lb,         1.0f - Lb,  0,
            0,           0,           0,           1,
            L_tint,      L_tint,      L_tint,      0
        };
        outLumi = winrt::make_self<ColorMatrixEffectImpl>();
        outLumi->Name(L"LuminosityMatrix");
        outLumi->SetMatrix(lumiMat);
        outLumi->SetAlphaMode(2);  // STRAIGHT
        outLumi->Source(outSat.as<WGE::IGraphicsEffectSource>());
        lumiSrc = outLumi.as<WGE::IGraphicsEffectSource>();
    }

    outOpacity = winrt::make_self<OpacityEffectImpl>();
    outOpacity->Name(L"BlurOpacity");
    outOpacity->Opacity(blurOpacity);
    outOpacity->Source(lumiSrc);

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

        outBlend = winrt::make_self<BlendEffectImpl>();
        outBlend->Name(L"NoiseBlend");
        outBlend->Mode(0);  // MULTIPLY
        outBlend->Foreground(outNoiseOpacity.as<WGE::IGraphicsEffectSource>());
        outBlend->Background(outComposite.as<WGE::IGraphicsEffectSource>());
        finalRoot = outBlend.as<WGE::IGraphicsEffect>();
    }

    return finalRoot;
}

// 公共 API 实现
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
            // DComp visual 的 z 序不影响 DWM 离屏预览。
            static const BOOL s_topmost = []{
                wchar_t v[8] = {};
                return (::GetEnvironmentVariableW(L"XBLUR_DCOMP_TOPMOST", v, 7) != 0 && _wtoi(v) != 0) ? TRUE : FALSE;
            }();
            namespace abid = ABI::Windows::UI::Composition::Desktop;
            auto interop = state->compositor.as<abid::ICompositorDesktopInterop>();
            HRESULT hr = interop->CreateDesktopWindowTarget(
                host, s_topmost,
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

        // 重建 effect chain (反映新参数).
        winrt::com_ptr<ColorMatrixEffectImpl> sat, lumi;
        winrt::com_ptr<OpacityEffectImpl>     blurOp, noiseOp;
        winrt::com_ptr<CompositeEffectImpl>   composite;
        winrt::com_ptr<BorderEffectImpl>      border;
        winrt::com_ptr<BlendEffectImpl>       blend;
        auto root = BuildEffectChain(
            uniformBrightness ? true : false,
            tintR, tintG, tintB,
            saturation,
            blurOpacity,
            noiseAlphaPct,
            sat, lumi, blurOp, composite, border, noiseOp, blend);

        auto factory = s.compositor.CreateEffectFactory(root);
        auto effectBrush = factory.CreateBrush();

        // HostBackdrop 从 HWND 外侧取样；普通 Backdrop 只取 visual 后方。
        // Win32 使用 HostBackdrop 前需要先启用对应的 DWM 窗口属性。
        // 属性不可用时继续使用原来的 CompositionBackdropBrush。
        BOOL hostBackdrop = TRUE;
        const bool useHostBackdrop = SUCCEEDED(::DwmSetWindowAttribute(
            host, 17 /* DWMWA_USE_HOSTBACKDROPBRUSH */, &hostBackdrop, sizeof(hostBackdrop)));
        auto backdrop = useHostBackdrop ? s.compositor.CreateHostBackdropBrush()
                                        : s.compositor.CreateBackdropBrush();
        effectBrush.SetSourceParameter(L"backdrop", backdrop);

        // tint 当作 Solid 顶层不透明 (alpha=255), 透明度由 OpacityEffect 控制.
        auto tintBrush = s.compositor.CreateColorBrush(
            winrt::Windows::UI::Color{ 255, (uint8_t)tintR, (uint8_t)tintG, (uint8_t)tintB });
        effectBrush.SetSourceParameter(L"tint", tintBrush);

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


namespace {

struct AcrylicHostEntry {
	HWND acrylicHwnd = NULL;
	HWND xcguiHwnd   = NULL;
	HWND originalOwner = NULL;
	LONG_PTR originalExStyle = 0;
	// 任务栏 / peek 身份是否挂在 acrylic 窗上 (默认 true, 见下面对 peek 的说明).
	// 为 false 时是旧行为: WS_EX_APPWINDOW 在 XCGUI 主窗上, acrylic 是 TOOLWINDOW.
	bool taskbarOnAcrylic = false;
	bool zOrderSyncPending = false;
	// 效果链参数 —— 最小化后 DWM 会丢掉 WS_EX_NOREDIRECTIONBITMAP 窗的
	// DesktopWindowTarget 合成 (实测还原后缩略图整片透明), 还原时必须用同一组
	// 参数把合成重建出来, 所以这里存一份。
	int   tintR = 0, tintG = 0, tintB = 0, tintA = 255;
	float blurOpacity = 0.0f;
	float saturation  = 1.0f;
	BOOL  uniformBrightness = FALSE;
	float noiseAlphaPct = 0.0f;
	// acrylic 是否处于最小化态 —— 用来只在"最小化 → 还原"这一跳重建合成,
	// 而不是每次 WM_SIZE (拖拽改尺寸) 都重建。
	bool  acrylicIconic = false;
	// 嵌套计数: "正在把 acrylic 推进最小化态"。
	//   为什么需要: 最小化 owner 会让 Win32 **自动隐藏** owned 的 XCGUI 主窗, 那一刻
	//   主窗收到的 WM_SHOWWINDOW(FALSE) 会被可见性同步逻辑反向打成 acrylic 的
	//   SW_HIDE。而 acrylic 是任务栏身份窗 —— 隐藏窗没有任务栏按钮, Explorer 会
	//   立刻**移除**按钮, 随后 acrylic 真正进入 iconic 态按钮又被**重建**, 表现为
	//   任务栏图标"先消失再出现"的过渡动画, 过渡期内点任务栏还会被吞掉。
	//   此时 acrylic 的 IsIconic() 尚未成立, 光靠 IsIconic 守卫拦不住, 必须靠这个
	//   标记。见 XcguiToAcrylicSyncProc 的 WM_SHOWWINDOW 分支。
	int   acrylicMinInFlight = 0;
	// ---- 最小化后的 peek / Alt+Tab 缩略图快照 --------------------------------
	// acrylic 是 WS_EX_NOREDIRECTIONBITMAP 窗: DWM 不给它建重定向表面。窗口可见
	// 时 DWM 直接合成 DComp visual, 缩略图正常; 一旦最小化, DWM 需要一张**静态
	// 位图**来画缩略图, NOREDIRECTIONBITMAP 窗没有可用表面 → 只能画空框(用户
	// 看到的"忙碌"态)。
	// 修法: 最小化**之前**从屏幕 DC 抓一帧存这里 (BitBlt+CAPTUREBLT), 等 DWM 用
	// WM_DWMSENDICONIC* 来要的时候交回去。抓帧每次最小化只做一次, 不是每帧。
	HBITMAP snapBmp = NULL;      // 32bpp top-down DIB (BGRA, 非预乘=预乘因 alpha 全 255)
	int     snapW   = 0;
	int     snapH   = 0;
	DWORD   snapTick = 0;        // 抓帧时刻, 用来抑制同一次最小化里的重复抓帧
	// acrylic 是否被我们"因虚拟桌面 cloak 而挪出屏幕"。只有它为 true 时
	// uncloak 才把 acrylic 搬回去 (否则会把最小化态 / 用户主动隐藏搞坏)。
	bool    cloakHidden = false;
	RECT    cloakSavedRect = {0, 0, 0, 0};  // 挪走前的位置, uncloak 时原样搬回
};
static std::map<HWND, AcrylicHostEntry> s_acrylicByXcgui; // key = xcgui HWND
static std::mutex                       s_acrylicMutex;

// WINDOWPOSCHANGED 仍可能处于 USER32 的 owner/owned 批量重排中。在其中
// 再次 SetWindowPos 背板，结果会被外层重排覆盖；应在这轮消息处理完后同步。
static UINT XBlurDComp_ZOrderSyncMessage(){
	static const UINT msg = ::RegisterWindowMessageW(L"XCGUI.CXBlur.SyncAcrylicZOrder");
	return msg;
}

static void XBlurDComp_ScheduleZOrderSync(HWND hwnd){
	const UINT msg = XBlurDComp_ZOrderSyncMessage();
	if (!msg) return;
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(hwnd);
	if (it == s_acrylicByXcgui.end() || it->second.zOrderSyncPending) return;
	if (::PostMessageW(hwnd, msg, (WPARAM)it->second.acrylicHwnd, 0))
		it->second.zOrderSyncPending = true;
}

static bool XBlurDComp_IsAbove(HWND content, HWND owner){
	for (HWND next = ::GetWindow(content, GW_HWNDNEXT); next; next = ::GetWindow(next, GW_HWNDNEXT)){
		if (next == owner) return true;
	}
	return false;
}

static void XBlurDComp_RestoreOwnedOrder(HWND owner, unsigned depth = 0){
	if (depth >= 64) return;
	// 保存当前兄弟窗口顺序，再只修复位于 owner 下方的窗口。包括没有附加
	// 模糊的炫彩弹窗；不改 owner、不抢焦点，也不把整组窗口抬到屏幕最前。
	std::vector<HWND> owned;
	for (HWND w = ::GetTopWindow(NULL); w; w = ::GetWindow(w, GW_HWNDNEXT)){
		if (::GetWindow(w, GW_OWNER) == owner && ::IsWindowVisible(w) &&
		    !::IsIconic(w) && ::GetWindowThreadProcessId(w, NULL) == ::GetCurrentThreadId())
			owned.push_back(w);
	}
	for (HWND w : owned){
		if (!::IsWindow(w) || ::GetWindow(w, GW_OWNER) != owner) continue;
		if (!XBlurDComp_IsAbove(w, owner)){
			HWND after = ::GetWindow(owner, GW_HWNDPREV);
			// 非置顶组紧邻置顶区间时，用 HWND_TOP 保持其非置顶属性。
			if (!after || (!(::GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOPMOST) &&
			    (::GetWindowLongPtrW(after, GWL_EXSTYLE) & WS_EX_TOPMOST))) after = HWND_TOP;
			::SetWindowPos(w, after, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
		}
		XBlurDComp_RestoreOwnedOrder(w, depth + 1);
	}
}

static void XBlurDComp_SyncZOrder(HWND hwnd, HWND acrylic){
	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(hwnd);
		if (it == s_acrylicByXcgui.end() || it->second.acrylicHwnd != acrylic) return;
		// 同步引起的嵌套 WINDOWPOS 通知不再重复投递。
		it->second.zOrderSyncPending = true;
	}
	if (::IsWindow(hwnd) && ::IsWindow(acrylic)){
		const UINT flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER;
		const bool mainTop = (::GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
		const bool acrylicTop = (::GetWindowLongPtrW(acrylic, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
		if (mainTop != acrylicTop)
			::SetWindowPos(acrylic, mainTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, flags);
		// 同一置顶区间也可能发生 owner 在 owned 之上的反转。只修复反转，
		// 不把窗口无条件抬到最前，保留其他应用和业务弹窗的相对顺序。
		if (!XBlurDComp_IsAbove(hwnd, acrylic))
			::SetWindowPos(acrylic, hwnd, 0, 0, 0, 0, flags);
		XBlurDComp_RestoreOwnedOrder(acrylic);
	}
	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(hwnd);
		if (it != s_acrylicByXcgui.end() && it->second.acrylicHwnd == acrylic)
			it->second.zOrderSyncPending = false;
	}
}

// Acrylic 外扩 device pixels — acrylic HWND 比 XCGUI HWND 4 边各大 N 像素, 让 acrylic
// 系统描边 (DWM 1px BORDER) 露在 XCGUI 边外, 不被 XCGUI alpha 客户区覆盖.
static constexpr int kAcrylicOuterPx = 1;

// 读布尔型实验开关 (值为 "1" 视为开).
static bool XBlurDComp_ReadEnvFlag(const wchar_t* name){
	wchar_t buf[8] = {};
	DWORD got = ::GetEnvironmentVariableW(name, buf, _countof(buf) - 1);
	return got > 0 && buf[0] == L'1';
}

// 读整型实验开关, 没设时用默认值。(原来是放在文件靠后位置, 挪到这儿是为了能被
// 上面几处早期 helper 直接用 —— 纯函数, 挪动不影响语义。)
static int XBlurDComp_ReadEnvInt(const wchar_t* name, int def){
	wchar_t buf[32] = {};
	DWORD n = ::GetEnvironmentVariableW(name, buf, 31);
	if (!n) return def;
	return _wtoi(buf);
}

// 自定义 DWM 缩略图通道使用的属性值。
static const DWORD kXBlurDcomp_DWMWA_HAS_ICONIC_BITMAP = 10;
static const DWORD kXBlurDcomp_DWMWA_FORCE_ICONIC_REPRESENTATION = 7;
static const UINT_PTR kXBlurDcompSnapshotTimer = 0xACD1;

// 默认使用 DWM 自身预览；设置 XBLUR_ICONIC_BITMAP 可启用应用提供的位图。
static bool XBlurDComp_IconicBitmapOn(){
	static const bool s_on = (XBlurDComp_ReadEnvInt(L"XBLUR_ICONIC_BITMAP", 0) != 0);
	return s_on;
}

// DWM 那三条缩略图 API 在 SDK 头里被 `#if(_WIN32_WINNT >= 0x0601)` 包着, 而本项目
// 聚合 TU 从没定义过 _WIN32_WINNT —— 直接调用会"未声明的标识符"。
// 与其去改上层 TU 的编译宏 (会牵连别的模块), 这里改成**运行时取函数指针**:
// dwmapi.dll 从 Vista 起就带着它们, 拿得到就调; 拿不到就退化成"没有缩略图"
// (=改动前的行为), 不会崩。两条消息同理自带一份常量 (值与 winuser.h 一致)。
#ifndef WM_DWMSENDICONICTHUMBNAIL
#define WM_DWMSENDICONICTHUMBNAIL            0x0323
#endif
#ifndef WM_DWMSENDICONICLIVEPREVIEWBITMAP
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP    0x0326
#endif

typedef HRESULT (WINAPI* XBlurDcompPfnSetIconicThumbnail)(HWND, HBITMAP, DWORD);
typedef HRESULT (WINAPI* XBlurDcompPfnSetIconicLivePreview)(HWND, HBITMAP, POINT*, DWORD);
typedef HRESULT (WINAPI* XBlurDcompPfnInvalidateIconicBitmaps)(HWND);

static HMODULE XBlurDComp_DwmApiModule(){
	static HMODULE s_mod = ::LoadLibraryW(L"dwmapi.dll");
	return s_mod;
}
static XBlurDcompPfnSetIconicThumbnail XBlurDComp_pfnSetIconicThumbnail(){
	static XBlurDcompPfnSetIconicThumbnail s_p = (XBlurDcompPfnSetIconicThumbnail)
		::GetProcAddress(XBlurDComp_DwmApiModule(), "DwmSetIconicThumbnail");
	return s_p;
}
static XBlurDcompPfnSetIconicLivePreview XBlurDComp_pfnSetIconicLivePreview(){
	static XBlurDcompPfnSetIconicLivePreview s_p = (XBlurDcompPfnSetIconicLivePreview)
		::GetProcAddress(XBlurDComp_DwmApiModule(), "DwmSetIconicLivePreviewBitmap");
	return s_p;
}
static XBlurDcompPfnInvalidateIconicBitmaps XBlurDComp_pfnInvalidateIconicBitmaps(){
	static XBlurDcompPfnInvalidateIconicBitmaps s_p = (XBlurDcompPfnInvalidateIconicBitmaps)
		::GetProcAddress(XBlurDComp_DwmApiModule(), "DwmInvalidateIconicBitmaps");
	return s_p;
}
// 让 DWM 丢弃已缓存的缩略图位图 (换了新快照 / 从最小化还原后必须调)。
static void XBlurDComp_DwmInvalidateIconic(HWND hwnd){
	if (!XBlurDComp_IconicBitmapOn()) return;
	XBlurDcompPfnInvalidateIconicBitmaps p = XBlurDComp_pfnInvalidateIconicBitmaps();
	if (p && hwnd && ::IsWindow(hwnd)) p(hwnd);
}

// 任务栏身份由 acrylic 持有，使 DWM 预览包含其 owned XCGUI 内容。
static bool XBlurDComp_TaskbarOnAcrylicWindow(){
	// 默认 true = 身份挂 acrylic。
	static const bool s_xcgui =
	    (XBlurDComp_ReadEnvInt(L"XBLUR_ACRYLIC_TASKBAR_XCGUI", 0) != 0);
	return !s_xcgui;
}

// 本次 attach 的任务栏身份是否在 acrylic 窗上 (默认 false = 在 XCGUI 主窗上)。
// 最小化/还原/关闭这些窗口状态操作要按它决定作用到哪个窗, 见 XcguiToAcrylicSyncProc。
static bool XBlurDComp_TaskbarOnAcrylicFor(HWND xcguiHwnd){
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	return it != s_acrylicByXcgui.end() && it->second.taskbarOnAcrylic;
}

// 任务栏身份窗 = 拿任务栏按钮 / Alt+Tab / peek 的那一个窗。
//   taskbarOnAcrylic=true  → acrylic 窗
//   默认                    → XCGUI 主窗
// 自定义缩略图 (DWMWA_HAS_ICONIC_BITMAP) 必须设在**这个窗**上, 消息也由它收。
static HWND XBlurDComp_TaskbarIdentityOf(HWND xcguiHwnd){
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	if (it == s_acrylicByXcgui.end()) return NULL;
	return it->second.taskbarOnAcrylic ? it->second.acrylicHwnd : it->second.xcguiHwnd;
}

// 仅在任务栏身份窗需要应用提供位图时启用。
static bool XBlurDComp_IconicBitmapNeededFor(HWND xcguiHwnd){
	if (!XBlurDComp_IconicBitmapOn()) return false;
	return XBlurDComp_TaskbarOnAcrylicFor(xcguiHwnd);
}

// 在任务栏身份窗上开启 DWM 自定义缩略图请求。
static void XBlurDComp_Trace(const char* fmt, ...);

static void XBlurDComp_EnableIconicBitmapFor(HWND xcguiHwnd){
	if (!XBlurDComp_IconicBitmapNeededFor(xcguiHwnd)){
		XBlurDComp_Trace("[ICONIC] arm skipped (need=%d taskbarOnAcrylic=%d iconicOn=%d)",
		                 XBlurDComp_IconicBitmapNeededFor(xcguiHwnd) ? 1 : 0,
		                 XBlurDComp_TaskbarOnAcrylicFor(xcguiHwnd) ? 1 : 0,
		                 XBlurDComp_IconicBitmapOn() ? 1 : 0);
		return;
	}
	HWND idw = XBlurDComp_TaskbarIdentityOf(xcguiHwnd);
	if (!idw || !::IsWindow(idw)) return;
	BOOL on = TRUE;
	HRESULT forceHr = ::DwmSetWindowAttribute(idw, kXBlurDcomp_DWMWA_FORCE_ICONIC_REPRESENTATION,
	                        &on, sizeof(on));
	HRESULT hr = ::DwmSetWindowAttribute(idw, kXBlurDcomp_DWMWA_HAS_ICONIC_BITMAP,
	                        &on, sizeof(on));
	XBlurDComp_Trace("[ICONIC] arm FORCE/HAS on identity 0x%p hr=0x%08X/0x%08X",
	                 (void*)idw, (unsigned)forceHr, (unsigned)hr);
}

// 旧 TOOLWINDOW 背板不随主窗切换虚拟桌面；监听 cloak 事件同步可见性。
namespace {
HWINEVENTHOOK    s_cloakHook = NULL;
std::atomic<int> s_cloakHookRefs{0};
} // namespace

static void CALLBACK XBlurDComp_CloakEventProc(HWINEVENTHOOK, DWORD ev, HWND hwnd,
                                               LONG idObject, LONG, DWORD, DWORD){
	if (idObject != OBJID_WINDOW || !hwnd) return;
	if (ev != EVENT_OBJECT_CLOAKED && ev != EVENT_OBJECT_UNCLOAKED) return;

	HWND acrylic = NULL;
	bool hideIt = false, showIt = false;
	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(hwnd);
		if (it == s_acrylicByXcgui.end()) return;
		acrylic = it->second.acrylicHwnd;
		if (!acrylic || !::IsWindow(acrylic)) return;
		// 当前默认结构由 acrylic 持有 APPWINDOW/任务栏身份。实测两个 HWND
		// 在切桌面时都会被 Shell cloak=2，Shell 自己负责同步合成过渡。
		// 旧 TOOLWINDOW 背板的搬出/搬回补丁在这里反而制造二次显示过程。
		if (it->second.taskbarOnAcrylic) return;

		if (ev == EVENT_OBJECT_CLOAKED){
			if (it->second.cloakHidden) return;              // 幂等
			// 最小化态本来就不在屏上, 不用动它 (动了反而会把最小化状态搞坏)。
			if (!::IsWindowVisible(acrylic) || ::IsIconic(acrylic)) return;
			it->second.cloakHidden = true;
			::GetWindowRect(acrylic, &it->second.cloakSavedRect);
			hideIt = true;
		} else {
			if (!it->second.cloakHidden) return;             // 不是我们挪走的, 别擅自动
			it->second.cloakHidden = false;
			showIt = true;
		}
	}

	// 移出屏幕可保持 owner 的可见性和任务栏身份，uncloak 时再移回。
	if (hideIt){
		XBlurDComp_Trace("[CLOAK] 身份窗 0x%p 被 cloak (切走虚拟桌面) -> acrylic 0x%p 移出屏幕",
		                 (void*)hwnd, (void*)acrylic);
		::SetWindowPos(acrylic, NULL, -32000, -32000, 0, 0,
		               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	} else if (showIt){
		RECT rc = {0, 0, 0, 0};
		{
			std::lock_guard<std::mutex> lk(s_acrylicMutex);
			auto it = s_acrylicByXcgui.find(hwnd);
			if (it == s_acrylicByXcgui.end()) return;
			rc = it->second.cloakSavedRect;
		}
		XBlurDComp_Trace("[CLOAK] 身份窗 0x%p uncloak (切回虚拟桌面) -> acrylic 0x%p 搬回 (%ld,%ld)",
		                 (void*)hwnd, (void*)acrylic, rc.left, rc.top);
		::SetWindowPos(acrylic, NULL, rc.left, rc.top, 0, 0,
		               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		if (!::IsWindowVisible(acrylic)) ::ShowWindow(acrylic, SW_SHOWNA);
	}
}

// 整进程只装一个 hook (按进程 id 过滤, 不接收别家的事件), 引用计数挂在 attach 上。
static void XBlurDComp_EnsureCloakHook(){
	if (s_cloakHookRefs.fetch_add(1) == 0){
		s_cloakHook = ::SetWinEventHook(
			EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED, NULL,
			XBlurDComp_CloakEventProc,
			::GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
		XBlurDComp_Trace("[CLOAK] SetWinEventHook(CLOAKED/UNCLOAKED) hook=0x%p",
		                 (void*)s_cloakHook);
	}
}
static void XBlurDComp_ReleaseCloakHook(){
	int cur = s_cloakHookRefs.load();
	for (;;){
		if (cur <= 0) return;
		if (s_cloakHookRefs.compare_exchange_weak(cur, cur - 1)) break;
	}
	if (cur == 1 && s_cloakHook){
		::UnhookWinEvent(s_cloakHook);
		s_cloakHook = NULL;
		XBlurDComp_Trace("[CLOAK] UnhookWinEvent");
	}
}

// 重建 acrylic 窗的 DComp 合成层 + 重新同步几何。
// 触发时机: acrylic 从最小化态还原。WS_EX_NOREDIRECTIONBITMAP 的窗在最小化时
// 被 DWM 停掉合成, 还原后 DesktopWindowTarget 不再出内容 —— 实测此时对该窗做
// DwmRegisterThumbnail 得到的是整片透明 (等于 peek 又回到透明洞)。所以还原后必须
// Disable + Apply 重建 target, 不能只重刷 effect chain。
static void XBlurDComp_RebuildAcrylicFor(HWND xcguiHwnd){
	AcrylicHostEntry e;
	bool found = false;
	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(xcguiHwnd);
		if (it == s_acrylicByXcgui.end()) return;
		e = it->second;
		found = true;
	}
	if (!found || !e.acrylicHwnd || !::IsWindow(e.acrylicHwnd)) return;

	XBlurDComp::Disable(e.acrylicHwnd);
	XBlurDComp::Apply(e.acrylicHwnd,
	                  e.tintR, e.tintG, e.tintB, e.tintA,
	                  e.blurOpacity, e.saturation,
	                  e.uniformBrightness, e.noiseAlphaPct, 0);
	// 重建后 root/sprite 是新对象, 尺寸要重新给一遍。
	XBlurDComp::Resize(e.acrylicHwnd, 0);
	// DWM 的 window attribute 在"停合成 → 重建"这一段里不保证还在, 补一次。
	// 少了它, 还原后再最小化就会退回"peek 空框 / 透明洞"。
	XBlurDComp_EnableIconicBitmapFor(xcguiHwnd);
	// 上一次最小化时抓的帧已经过期 (窗口内容/尺寸都可能变了) → 让 DWM 丢弃缓存,
	// 下次最小化重新抓、重新要。
	XBlurDComp_DwmInvalidateIconic(XBlurDComp_TaskbarIdentityOf(xcguiHwnd));
}

static void XBlurDComp_SetAcrylicIconic(HWND xcguiHwnd, bool iconic){
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	if (it != s_acrylicByXcgui.end()) it->second.acrylicIconic = iconic;
}

// 返回"刚才是否处于最小化态", 并清标记。只在真还原时返回 true。
static bool XBlurDComp_TakeAcrylicIconic(HWND xcguiHwnd){
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	if (it == s_acrylicByXcgui.end()) return false;
	bool was = it->second.acrylicIconic;
	it->second.acrylicIconic = false;
	return was;
}

// "acrylic 正在被推进最小化态" 的嵌套标记。见 AcrylicHostEntry::acrylicMinInFlight。
static void XBlurDComp_PushAcrylicMinInFlight(HWND xcguiHwnd){
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	if (it != s_acrylicByXcgui.end()) ++it->second.acrylicMinInFlight;
}
static void XBlurDComp_PopAcrylicMinInFlight(HWND xcguiHwnd){
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	if (it != s_acrylicByXcgui.end() && it->second.acrylicMinInFlight > 0)
		--it->second.acrylicMinInFlight;
}
static bool XBlurDComp_AcrylicMinInFlight(HWND xcguiHwnd){
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	return it != s_acrylicByXcgui.end() && it->second.acrylicMinInFlight > 0;
}

// 最小化后的 peek / Alt+Tab 缩略图 —— 为什么必须自己提供像素
// (诊断日志实现在本段之后, 先声明。)
static void XBlurDComp_Trace(const char* fmt, ...);
// 最小化前保存合成画面，供 DWM 的缩略图和实时预览使用。

// 可选的重定向表面绘制路径；默认使用自定义缩略图通道。
static void XBlurDComp_PaintComposedToWindowSurface(HWND acrylicHwnd, HWND xcguiHwnd){
	if (!acrylicHwnd || !::IsWindow(acrylicHwnd)) return;

	RECT rc{};
	::GetWindowRect(acrylicHwnd, &rc);
	{
		RECT xr{};
		if (xcguiHwnd && ::IsWindow(xcguiHwnd) && ::IsWindowVisible(xcguiHwnd) &&
		    ::GetWindowRect(xcguiHwnd, &xr)){
			if (xr.left   < rc.left)   rc.left   = xr.left;
			if (xr.top    < rc.top)    rc.top    = xr.top;
			if (xr.right  > rc.right)  rc.right  = xr.right;
			if (xr.bottom > rc.bottom) rc.bottom = xr.bottom;
		}
	}
	const int w = rc.right - rc.left, h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return;

	HDC screen = ::GetDC(NULL);
	if (!screen) return;

	// 先抓合成画面，再改变 DComp target。
	BITMAPINFO bi{};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = w;
	bi.bmiHeader.biHeight      = -h;
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	void* bits = NULL;
	HBITMAP dib = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
	HDC mem = ::CreateCompatibleDC(screen);
	bool grabbed = false;
	if (dib && mem){
		HGDIOBJ old = ::SelectObject(mem, dib);
		grabbed = (::BitBlt(mem, 0, 0, w, h, screen, rc.left, rc.top,
		                    SRCCOPY | CAPTUREBLT) != FALSE);
		::GdiFlush();
		::SelectObject(mem, old);
	}

	// target 仍在时，DWM 不会使用写入窗口 DC 的内容。
	if (XBlurDComp_ReadEnvInt(L"XBLUR_SURF_DISABLE_DCOMP", 1) != 0)
		XBlurDComp::Disable(acrylicHwnd);

	// 3) 把抓到的帧写回 acrylic 自己的窗口 DC = 写进它的重定向表面。
	if (grabbed){
		HDC wdc = ::GetWindowDC(acrylicHwnd);
		if (wdc){
			::BitBlt(wdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
			::GdiFlush();
			::ReleaseDC(acrylicHwnd, wdc);
		}
	}
	if (mem) ::DeleteDC(mem);
	if (dib) ::DeleteObject(dib);
	::ReleaseDC(NULL, screen);
	XBlurDComp_Trace("[SURF] composed %dx%d grabbed=%d, DComp target disabled, painted to wnd DC",
	                 w, h, grabbed ? 1 : 0);
}

// 必须在窗口进入最小化停靠位置之前抓取。
static void XBlurDComp_CaptureAcrylicSnapshot(HWND xcguiHwnd){
	if (!xcguiHwnd || !::IsWindow(xcguiHwnd)) return;

	HWND acrylicHwnd = NULL;
	bool cloakHidden = false;
	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(xcguiHwnd);
		if (it == s_acrylicByXcgui.end()) return;
		acrylicHwnd = it->second.acrylicHwnd;
		cloakHidden = it->second.cloakHidden;
	}
	if (!acrylicHwnd || !::IsWindow(acrylicHwnd)) return;
	// 已被移出屏幕 (= 切到了别的虚拟桌面): 此刻抓屏只能抓到屏幕外的垃圾, 直接跳过。
	if (cloakHidden) return;
	if (!::IsWindowVisible(acrylicHwnd) || ::IsIconic(acrylicHwnd)) return;

	// XBLUR_SURF_PAINT 仅用于启用重定向表面实验路径。
	if (XBlurDComp_ReadEnvInt(L"XBLUR_SURF_PAINT", 0) != 0)
		XBlurDComp_PaintComposedToWindowSurface(acrylicHwnd, xcguiHwnd);

	// (2) 以下是自定义缩略图通道那份快照。
	if (!XBlurDComp_IconicBitmapNeededFor(xcguiHwnd)) return;

	// 抓 acrylic 与 XCGUI 主窗的**并集**矩形。两者几乎重合 (acrylic 比 XCGUI 四边各
	// 大 1px), 取并集是为了不依赖"谁大谁小"这个实现细节。
	RECT rc{};
	RECT xr{};
	if (!::GetWindowRect(acrylicHwnd, &rc) || !::IsWindowVisible(xcguiHwnd) ||
	    !::GetWindowRect(xcguiHwnd, &xr)) return;
	// XCGUI 的自定义最小化会先把自身缩成仅有标题栏的停靠窗。此时不能用
	// acrylic 的完整矩形去抓屏，否则会把桌面/别的窗当成预览内容。
	const int aw = rc.right - rc.left, ah = rc.bottom - rc.top;
	const int xw = xr.right - xr.left, xh = xr.bottom - xr.top;
	if (xw < aw - 8 || xh < ah - 8 || xw > aw + 8 || xh > ah + 8 ||
	    abs(xr.left - rc.left) > 8 || abs(xr.top - rc.top) > 8){
		XBlurDComp_Trace("[SNAP] skip incomplete geometry acrylic=%dx%d main=%dx%d", aw, ah, xw, xh);
		return;
	}
	if (xr.left   < rc.left)   rc.left   = xr.left;
	if (xr.top    < rc.top)    rc.top    = xr.top;
	if (xr.right  > rc.right)  rc.right  = xr.right;
	if (xr.bottom > rc.bottom) rc.bottom = xr.bottom;
	int w = rc.right - rc.left, h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return;
	const int vx = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	const int vy = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	const int vw = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const int vh = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
	if (rc.left < vx || rc.top < vy || rc.right > vx + vw || rc.bottom > vy + vh) return;

	HDC screen = ::GetDC(NULL);
	if (!screen) return;

	BITMAPINFO bi{};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = w;
	bi.bmiHeader.biHeight      = -h;          // top-down: 与屏幕扫描顺序一致
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	void* bits = NULL;
	HBITMAP dib = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
	HDC mem = ::CreateCompatibleDC(screen);
	BOOL ok = FALSE;
	if (dib && mem){
		HGDIOBJ old = ::SelectObject(mem, dib);
		// CAPTUREBLT 必须带: 不带就抓不到 layered (XCGUI 主窗) 和 DWM 合成层,
		// 只会拿到一片底层桌面 —— 这正是"缩略图空白"的另一种翻版。
		ok = ::BitBlt(mem, 0, 0, w, h, screen, rc.left, rc.top, SRCCOPY | CAPTUREBLT);
		::GdiFlush();
		::SelectObject(mem, old);
	}
	if (mem)  ::DeleteDC(mem);
	::ReleaseDC(NULL, screen);

	if (!dib){ return; }
	if (!ok || !bits){ ::DeleteObject(dib); return; }

	{
		unsigned char* p = (unsigned char*)bits;
		const size_t n = (size_t)w * (size_t)h;
		// BitBlt 不写 alpha 通道 (DIB 里全是 0)。alpha 全 0 = DWM 眼里整张图
		// 全透明, 交上去照样是空框 —— 必须显式补成 255 (不透明, 同时也是合法
		// 的 premultiplied alpha, 因为乘不上任何变化)。
		for (size_t i = 0; i < n; ++i) p[i * 4 + 3] = 255;

		// 四角抹成透明: 窗口是 DWM 圆角矩形, 圆角外面那几个像素其实是 BitBlt
		// 从桌面抓来的, 不抹掉会在缩略图四角露出桌面色块。半径跟 DWM 的
		// 8 逻辑 px 对齐 (按窗口 DPI 缩放)。
		UINT dpi = ::GetDpiForWindow(acrylicHwnd);
		if (dpi == 0) dpi = 96;
		int r = (int)((dpi * 8 + 48) / 96);
		if (r > 0 && r * 2 < w && r * 2 < h){
			const int rr = r * r;
			for (int y = 0; y < r; ++y){
				for (int x = 0; x < r; ++x){
					const int dx = r - 1 - x, dy = r - 1 - y;
					if (dx * dx + dy * dy <= rr) continue;
					const size_t i0 = (size_t)y         * w + x;
					const size_t i1 = (size_t)y         * w + (w - 1 - x);
					const size_t i2 = (size_t)(h - 1 - y) * w + x;
					const size_t i3 = (size_t)(h - 1 - y) * w + (w - 1 - x);
					p[i0 * 4 + 3] = 0; p[i0 * 4 + 0] = 0; p[i0 * 4 + 1] = 0; p[i0 * 4 + 2] = 0;
					p[i1 * 4 + 3] = 0; p[i1 * 4 + 0] = 0; p[i1 * 4 + 1] = 0; p[i1 * 4 + 2] = 0;
					p[i2 * 4 + 3] = 0; p[i2 * 4 + 0] = 0; p[i2 * 4 + 1] = 0; p[i2 * 4 + 2] = 0;
					p[i3 * 4 + 3] = 0; p[i3 * 4 + 0] = 0; p[i3 * 4 + 1] = 0; p[i3 * 4 + 2] = 0;
				}
			}
		}
	}

	bool stored = false;
	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(xcguiHwnd);
		if (it != s_acrylicByXcgui.end()){
			if (it->second.snapBmp) ::DeleteObject(it->second.snapBmp);
			it->second.snapBmp  = dib;
			it->second.snapW    = w;
			it->second.snapH    = h;
			it->second.snapTick = ::GetTickCount();
			stored = true;
		}
	}
	if (stored){
		// 新图就绪 → 通知 DWM 别再用旧图。
		XBlurDComp_DwmInvalidateIconic(acrylicHwnd);
		XBlurDComp_Trace("[SNAP] captured acrylic snapshot %dx%d (capture=ok)", w, h);
	} else {
		::DeleteObject(dib);
	}
}
static bool XBlurDComp_PeekAcrylicIconic(HWND xcguiHwnd){
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	return it != s_acrylicByXcgui.end() && it->second.acrylicIconic;
}

// Paint/resize/DPI 事件只重置一个单次定时器；连续重绘时不会每帧抓屏。
// 真正的 BitBlt 在绘制结束、窗口尺寸稳定后运行。
static void XBlurDComp_ScheduleAcrylicSnapshot(HWND xcguiHwnd){
	if (!XBlurDComp_IconicBitmapNeededFor(xcguiHwnd)) return;
	HWND acrylicHwnd = XBlurDComp_TaskbarIdentityOf(xcguiHwnd);
	if (acrylicHwnd && ::IsWindow(acrylicHwnd) && !::IsIconic(acrylicHwnd))
		::SetTimer(acrylicHwnd, kXBlurDcompSnapshotTimer, 150, NULL);
}

// 释放快照位图。调用方必须已持有 s_acrylicMutex (或该 entry 已不在 map 里)。
static void XBlurDComp_FreeAcrylicSnapshot(AcrylicHostEntry& e){
	if (e.snapBmp){ ::DeleteObject(e.snapBmp); e.snapBmp = NULL; }
	e.snapW = 0; e.snapH = 0; e.snapTick = 0;
}

// 从 AcrylicHostEntry 的快照里按 (maxW, maxH) 等比缩出一张新的 DIB 交给调用方。
// 返回 HBITMAP (调用方负责 DeleteObject), 没有快照时返回 NULL。
static HBITMAP XBlurDComp_MakeScaledSnap(HWND xcguiHwnd, int maxW, int maxH,
                                         int* outW, int* outH){
	if (outW) *outW = 0;
	if (outH) *outH = 0;
	if (maxW <= 0 || maxH <= 0) return NULL;

	// 保持锁直到 StretchBlt 完成；另一个线程若替换/释放快照，源 HBITMAP
	// 不能在 SelectObject 使用期间被 DeleteObject。
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	if (it == s_acrylicByXcgui.end()) return NULL;
	HBITMAP src = it->second.snapBmp;
	int sw = it->second.snapW, sh = it->second.snapH;
	if (!src || sw <= 0 || sh <= 0) return NULL;

	// 等比缩放到不超过请求的框; 只缩不放 (放大只会糊, 且 DWM 自己会再缩)。
	double s = 1.0;
	if (sw > maxW || sh > maxH){
		const double sx = (double)maxW / (double)sw;
		const double sy = (double)maxH / (double)sh;
		s = (sx < sy) ? sx : sy;
	}
	int dw = (int)(sw * s + 0.5);
	int dh = (int)(sh * s + 0.5);
	if (dw < 1) dw = 1;
	if (dh < 1) dh = 1;

	HDC screen = ::GetDC(NULL);
	if (!screen) return NULL;
	BITMAPINFO bi{};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = dw;
	bi.bmiHeader.biHeight      = -dh;
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	void* bits = NULL;
	HBITMAP dst = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
	HDC mem = ::CreateCompatibleDC(screen);
	if (dst && mem){
		HGDIOBJ old = ::SelectObject(mem, dst);
		::SetStretchBltMode(mem, HALFTONE);
		::SetBrushOrgEx(mem, 0, 0, NULL);
		HDC srcDc = ::CreateCompatibleDC(screen);
		if (srcDc){
			HGDIOBJ oldSrc = ::SelectObject(srcDc, src);
			::StretchBlt(mem, 0, 0, dw, dh, srcDc, 0, 0, sw, sh, SRCCOPY);
			::SelectObject(srcDc, oldSrc);
			::DeleteDC(srcDc);
		}
		::GdiFlush();
		::SelectObject(mem, old);
	}
	if (mem) ::DeleteDC(mem);
	::ReleaseDC(NULL, screen);
	if (!dst) return NULL;
	if (outW) *outW = dw;
	if (outH) *outH = dh;
	return dst;
}

// DWM 分别请求缩略图与实时预览；尺寸只在缩略图请求的 lParam 中提供。
static LRESULT XBlurDComp_ProvideIconBitmap(HWND targetWnd, HWND xcguiHwnd,
                                            bool livePreview, LPARAM lParam){
	if (!targetWnd || !::IsWindow(targetWnd)) return 0;

	int maxW = 0, maxH = 0;
	if (livePreview){
		// 最小化后 GetWindowRect 可能只剩标题栏尺寸；使用完整快照尺寸。
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(xcguiHwnd);
		if (it != s_acrylicByXcgui.end()){
			maxW = it->second.snapW;
			maxH = it->second.snapH;
		}
	} else {
		maxW = (int)(short)HIWORD(lParam);
		maxH = (int)(short)LOWORD(lParam);
	}
	if (maxW <= 0) maxW = 1;
	if (maxH <= 0) maxH = 1;

	int bw = 0, bh = 0;
	HBITMAP hbm = XBlurDComp_MakeScaledSnap(xcguiHwnd, maxW, maxH, &bw, &bh);
	if (!hbm){
		XBlurDComp_Trace("[ICONIC] %s request (%dx%d) but no snapshot -> empty",
		                 livePreview ? "livepreview" : "thumbnail", maxW, maxH);
		return 0;
	}
	HRESULT hr = E_FAIL;
	if (livePreview){
		XBlurDcompPfnSetIconicLivePreview fp = XBlurDComp_pfnSetIconicLivePreview();
		if (fp){
			// 快照里已经含窗口自身那圈系统描边, 所以不再让 DWM 加画 frame。
			hr = fp(targetWnd, hbm, NULL, 0);
		}
	} else {
		XBlurDcompPfnSetIconicThumbnail fp = XBlurDComp_pfnSetIconicThumbnail();
		if (fp) hr = fp(targetWnd, hbm, 0);
	}
	::DeleteObject(hbm);
	XBlurDComp_Trace("[ICONIC] %s request (%dx%d) -> %dx%d hr=0x%08X",
	                 livePreview ? "livepreview" : "thumbnail", maxW, maxH, bw, bh, (unsigned)hr);
	return 0;
}

// 进入"把 acrylic 推进最小化态"的窗口期。
//   标记的**结束时间点很重要**: owner 最小化连带隐藏 owned 主窗、挪动位置这些
//   副作用, 有的是在 ShowWindow 内部同步重入进来的, 有的会晚一拍。与其赌
//   "ShowWindow 什么时候返回", 不如让标记一直有效到**下一条消息**(下个消息循环
//   回合) —— 也就是这里 PostMessage 一个结束消息, 由 XcguiToAcrylicSyncProc 处理。
//   这期间 acrylic 的可见性**绝不能**被同步逻辑改动: 一隐藏 Explorer 就移除任务栏
//   按钮, 随后 acrylic 进入 iconic 态按钮又被重建 = 用户看到的"图标先消失再出现"。
static const UINT kXBlurMsgEndAcrylicMinimize = WM_APP + 0x101;

static void XBlurDComp_BeginAcrylicMinimize(HWND xcguiHwnd){
	HWND acrylic = NULL;
	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(xcguiHwnd);
		if (it != s_acrylicByXcgui.end()) acrylic = it->second.acrylicHwnd;
	}
	if (acrylic && XBlurDComp_ReadEnvInt(L"XBLUR_UI_SURFACE", 1) != 0)
		XBlurDComp_AttachUiSurface(acrylic, xcguiHwnd);
	XBlurDComp_PushAcrylicMinInFlight(xcguiHwnd);
	::PostMessageW(xcguiHwnd, kXBlurMsgEndAcrylicMinimize, 0, 0);
}

// 设置 XBLUR_ACRYLIC_TRACE 后，将窗口状态写入指定文件。
static FILE* XBlurDComp_TraceFile(){
	static FILE* s_trace = (FILE*)(intptr_t)-1;   // -1 = 还没初始化
	if (s_trace == (FILE*)(intptr_t)-1){
		s_trace = NULL;
		wchar_t p[MAX_PATH] = {};
		if (::GetEnvironmentVariableW(L"XBLUR_ACRYLIC_TRACE", p, MAX_PATH - 1)){
			FILE* f = NULL;
			if (_wfopen_s(&f, p, L"a") == 0) s_trace = f;
		}
	}
	return s_trace;
}
static void XBlurDComp_Trace(const char* fmt, ...){
	FILE* f = XBlurDComp_TraceFile();
	if (!f) return;
	va_list ap; va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	fputc('\n', f);
	fflush(f);
}

static void XBlurDComp_SetAcrylicCornerPref(HWND acrylicHwnd, bool maximized){
	DWORD pref = maximized ? kDWMWCP_DONOTROUND : kDWMWCP_ROUND;
	::DwmSetWindowAttribute(acrylicHwnd, kXBlurDcomp_DWMWA_WINDOW_CORNER_PREFERENCE,
	                        &pref, sizeof(pref));
}

// 诊断/行为开关: acrylic 激活时怎么把键盘焦点交给 XCGUI 主窗。
//   0 = 完全不转 (焦点留在 acrylic)
//   1 = WM_ACTIVATE/WM_SETFOCUS 里**立即** SetFocus(主窗)   <-- 历史行为
//   2 = 延后到 Explorer 的激活流程走完再转 (PostMessage)
// 读一次缓存 (XBLUR_ACRYLIC_FOCUS_FWD)。用于定位「点任务栏按钮第一次被吃掉」
// 到底是 fg!=acrylic 还是"激活过程中抢焦点打扰了 Explorer"。

static const UINT kXBlurMsgTakeFocus = WM_APP + 0x100;

// Acrylic backdrop 自己的 subclass。
//   dwRefData = XCGUI 主窗 (仅 taskbarOnAcrylic 模式下非 0)。
//
// 为什么需要它: 默认模式下任务栏按钮挂在这个 acrylic 窗上, 于是"点任务栏按钮 /
// Alt+Tab"激活的是 acrylic(一个 DefWindowProc 空窗)。实测此时该 GUI 线程的
//   GetGUIThreadInfo().hwndActive / hwndFocus 都变成 acrylic, XCGUI 主窗
// 既不是活动窗也不是焦点窗 → **键盘输入 (快捷键/输入法/Tab/ESC) 会打到空窗上,
// 主窗收不到 WM_KEYDOWN** (鼠标点击不受影响, 因为点击落在上面的主窗上)。
// 这里把激活与焦点转回主窗, 恢复"任务栏按钮=应用主窗"的语义。
static LRESULT CALLBACK AcrylicWndSubclassProc(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	HWND owned = (HWND)dwRefData;   // XCGUI 主窗, 0 = 旧模式(不转发)
	// 焦点转发策略 (见 XBlurDComp_ReadEnvInt 注释)。
	static const int s_focusFwd = XBlurDComp_ReadEnvInt(L"XBLUR_ACRYLIC_FOCUS_FWD", 1);
	auto handFocusToOwned = [&](bool defer){
		if (!owned || !::IsWindow(owned) || ::GetFocus() == owned) return;
		if (defer) ::PostMessageW(hwnd, kXBlurMsgTakeFocus, 0, 0);
		else       ::SetFocus(owned);
	};

	switch (msg){
	case WM_TIMER:
		if (wParam == kXBlurDcompSnapshotTimer){
			::KillTimer(hwnd, kXBlurDcompSnapshotTimer);
			if (owned && ::IsWindow(owned)) XBlurDComp_CaptureAcrylicSnapshot(owned);
			return 0;
		}
		break;
	case kXBlurMsgTakeFocus:
		// 延后执行: 此刻 Explorer 的激活流程已经走完, 再把键盘焦点交给主窗,
		// 不会中途把 acrylic 的激活状态拽走。
		if (owned && ::IsWindow(owned) && ::GetFocus() != owned) ::SetFocus(owned);
		return 0;
	case WM_ACTIVATE:
		// 键盘 (快捷键/输入法/Tab/ESC) 必须落到 XCGUI 主窗, 但这个转移**不能**
		// 在 Explorer 处理任务栏按钮点击的激活流程中间做 —— 实测那样会把点击
		// 这一下"吃掉"(按钮所属窗刚被激活就被我们切成非激活)。
		if (LOWORD(wParam) != WA_INACTIVE && s_focusFwd != 0) handFocusToOwned(s_focusFwd == 2);
		break;
	case WM_SETFOCUS:
		// 焦点落到空窗上 → 转给主窗 (SetFocus 前提: 同线程 + 是前台线程)
		if (s_focusFwd != 0) handFocusToOwned(s_focusFwd == 2);
		break;
	case WM_CLOSE:
		// 任务栏右键"关闭窗口" / 系统菜单关闭 会送到 acrylic 上。acrylic 是 owner,
		// 直接 DestroyWindow 会连带把 owned 的 XCGUI 主窗一起销毁, 而且**绕过主窗的
		// WM_CLOSE 流程** (调用方的 Detach 清理 / 用户退出回调全部跳过)。转发给主窗。
		if (owned && ::IsWindow(owned)){
			::PostMessageW(owned, WM_CLOSE, 0, 0);
			return 0;
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE && owned && ::IsWindow(owned)){
			::PostMessageW(owned, WM_CLOSE, 0, 0);
			return 0;
		}
		// Win+Up/Down 可能把系统命令送到任务栏身份窗。最大化及普通还原
		// 必须由 XCGUI 主窗执行，才能产生它的 WM_SIZE/布局流程。
		if (owned && ::IsWindow(owned)){
			const UINT cmd = (UINT)(wParam & 0xFFF0);
			if (cmd == SC_MAXIMIZE || (cmd == SC_RESTORE && !::IsIconic(hwnd))){
				if (cmd == SC_MAXIMIZE && ::IsIconic(hwnd)){
					::ShowWindow(hwnd, SW_RESTORE);
					return 0; // 最小化后的第一次 Win+Up 只还原
				}
				::SendMessageW(owned, WM_SYSCOMMAND, wParam, lParam);
				return 0;
			}
		}
		// 点任务栏按钮最小化时, 系统把 SC_MINIMIZE **直接发给任务栏身份窗** (= 这个
		// acrylic), 完全不经过 XCGUI 侧的任何 handler。
		if ((wParam & 0xFFF0) == SC_MINIMIZE && owned && ::IsWindow(owned) &&
		    !::IsIconic(hwnd) && !XBlurDComp_AcrylicMinInFlight(owned)){
			if (XBlurDComp_ReadEnvInt(L"XBLUR_UI_SURFACE", 1) != 0)
				XBlurDComp_AttachUiSurface(hwnd, owned);
			XBlurDComp_CaptureAcrylicSnapshot(owned);
			// 标记最小化过程，避免 owner 隐藏 owned 主窗时反向隐藏任务栏身份窗。
			XBlurDComp_BeginAcrylicMinimize(owned);
		}
		break;
	// ---- DWM 来要自定义缩略图 / live preview 位图 ---------------------------
	// 必须置了 DWMWA_HAS_ICONIC_BITMAP 才会收到; 不响应 (或回空图) 就是用户看到的
	// "最小化后 peek 只有一个空框, 要点第二次才出内容"。
	case WM_DWMSENDICONICTHUMBNAIL:
		return XBlurDComp_ProvideIconBitmap(hwnd, owned, false, lParam);
	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		return XBlurDComp_ProvideIconBitmap(hwnd, owned, true, lParam);
	case WM_WINDOWPOSCHANGING:
		if (owned && XBlurDComp_PeekAcrylicIconic(owned) &&
		    !(::GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_MINIMIZE)){
			XBlurDComp_Trace("[ACRYLIC_POS] early detach");
			XBlurDComp_DetachUiSurface(hwnd);
		}
		// 兜底抓帧: 任何"把自己最小化"的路径都会先在这里冒头, 且此刻窗口**还没**
		// 被挪走 (真正的位移发生在 WM_WINDOWPOSCHANGED)。
		if (owned && ::IsWindow(owned) && !::IsIconic(hwnd) &&
		    (::GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_MINIMIZE)){
			if (XBlurDComp_ReadEnvInt(L"XBLUR_UI_SURFACE", 1) != 0)
				XBlurDComp_AttachUiSurface(hwnd, owned);
			XBlurDComp_CaptureAcrylicSnapshot(owned);
		}
		break;
	case WM_SIZE:
		if (owned && ::IsWindow(owned)){
			if (wParam == SIZE_MINIMIZED){
				XBlurDComp_SetAcrylicIconic(owned, true);
			} else if (XBlurDComp_TakeAcrylicIconic(owned)){
				XBlurDComp_DetachUiSurface(hwnd);
				// 刚从最小化还原: DWM 在最小化期间停掉了这个窗的合成,
				// DesktopWindowTarget 已失效 → 必须重建。
				// 不重建的话: 窗口本身没有模糊层, 而且 peek / 任务栏预览退回
				// 透明洞 (实测对还原后的 acrylic 注册缩略图 = 整片透明)。
				// 只在"最小化→还原"这一跳重建, 不在每次 WM_SIZE 重建
				// (拖拽改尺寸会频繁 WM_SIZE, 重建代价太大)。
				XBlurDComp_RebuildAcrylicFor(owned);
				// XCGUI 自身按钮可能也把 owned 窗口置为 iconic。保持这一状态到
				// owner 真正还原，再让 XCGUI 收到正常的 restore/layout 消息。
				if (::IsIconic(owned)) ::ShowWindow(owned, SW_RESTORE);
			}
		}
		break;
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
	if (msg == XBlurDComp_ZOrderSyncMessage()){
		if ((HWND)wParam == acrylicBackdrop) XBlurDComp_SyncZOrder(hwnd, acrylicBackdrop);
		return 0;
	}

	// 仅在需要任务栏身份的消息分支查表，避免给每条窗口消息加锁。

	// 诊断: XBLUR_ACRYLIC_TRACE 打开时, 记录一小撮"窗口状态相关"消息 (白名单, 免得刷屏)。
	// 用途: 定位"点窗口自己的最小化按钮"这类路径到底走了哪些消息 —— 例如判断能否在
	// 窗口被挪到左下角**之前**就把它拦下来。不开 trace 时只多一次静态指针判断。
	if (XBlurDComp_TraceFile()){
		switch (msg){
		case WM_SHOWWINDOW: case WM_WINDOWPOSCHANGING: case WM_WINDOWPOSCHANGED:
		case WM_SIZE: case WM_SYSCOMMAND: case WM_CLOSE:
		case WM_NCLBUTTONDOWN: case WM_NCLBUTTONUP:
		case WM_ACTIVATE: case WM_STYLECHANGING: case WM_STYLECHANGED:
		{
			LONG_PTR st = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
			RECT rc{}; ::GetWindowRect(hwnd, &rc);
			XBlurDComp_Trace("[MSG] 0x%04X wP=0x%llX lP=0x%llX vis=%d ico=%d rect=(%d,%d,%dx%d)",
			                 (unsigned)msg, (unsigned long long)wParam, (unsigned long long)lParam,
			                 ::IsWindowVisible(hwnd) ? 1 : 0, (st & WS_MINIMIZE) ? 1 : 0,
			                 rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
		}
		break;
		}
	}

	// 主窗是否处于/正在进入最小化态。
	auto contentMinimized = [&]() -> bool {
		return ::IsIconic(hwnd) || (::GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_MINIMIZE) != 0;
	};
	switch (msg){
	case WM_PAINT:
		{
			LRESULT r = ::DefSubclassProc(hwnd, msg, wParam, lParam);
			XBlurDComp_ScheduleAcrylicSnapshot(hwnd);
			return r;
		}
	case kXBlurMsgEndAcrylicMinimize:
		// 见 XBlurDComp_BeginAcrylicMinimize: 最小化窗口期到此结束。
		XBlurDComp_PopAcrylicMinInFlight(hwnd);
		return 0;
	// 身份挂在 acrylic 上时, 自定义缩略图请求由 **acrylic 的 subclass** 处理; 这两条
	// 是"身份挂 XCGUI 主窗"(XBLUR_ACRYLIC_TASKBAR_XCGUI=1) 那套对照配置的入口。
	// 通道默认关闭 (XBlurDComp_IconicBitmapOn 默认 0), 所以平时都收不到。
	case WM_DWMSENDICONICTHUMBNAIL:
		return XBlurDComp_ProvideIconBitmap(hwnd, hwnd, false, lParam);
	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		return XBlurDComp_ProvideIconBitmap(hwnd, hwnd, true, lParam);
	case WM_SHOWWINDOW:
		if (wParam){
			XBlurDComp_ScheduleAcrylicSnapshot(hwnd);
		}
		// acrylic 是 XCGUI 主窗的 owner。隐藏 owned 主窗不会自动隐藏 owner，
		// 托盘模式因此会只留下空白亚克力背板；这里显式同步普通显示/隐藏。
		//
		// 忽略 owner 最小化引起的连带隐藏，避免任务栏按钮被移除。
		if (lParam == SW_PARENTCLOSING || lParam == SW_PARENTOPENING){
			XBlurDComp_Trace("[WM_SHOWWINDOW] wParam=%d lParam=%d (%s) -> skip acrylic vis sync",
			                 (int)wParam, (int)lParam,
			                 (lParam == SW_PARENTCLOSING) ? "SW_PARENTCLOSING" : "SW_PARENTOPENING");
			break;
		}
		if (XBlurDComp_AcrylicMinInFlight(hwnd)){
			XBlurDComp_Trace("[WM_SHOWWINDOW] wParam=%d lParam=%d minimize-in-flight -> skip acrylic vis sync",
			                 (int)wParam, (int)lParam);
			break;
		}
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop) && !::IsIconic(acrylicBackdrop)){
			if (wParam && !::IsIconic(hwnd))
				::ShowWindow(acrylicBackdrop, SW_SHOWNA);
			else
				::ShowWindow(acrylicBackdrop, SW_HIDE);
		}
		break;
	case WM_SYSCOMMAND: {
		// 默认模式 (任务栏身份在主窗上) 下点任务栏按钮, 系统把 SC_MINIMIZE 直接发给
		// 主窗。此刻窗口还完整可见 —— 这是给 peek / Alt+Tab 抓缩略图位图的最佳时机,
		// 晚一步(等 WM_SIZE)窗口就已经在停靠位上了。
		if ((wParam & 0xFFF0) == SC_MINIMIZE && !::IsIconic(hwnd) &&
		    ::IsWindowVisible(hwnd) && !XBlurDComp_AcrylicMinInFlight(hwnd)){
			XBlurDComp_CaptureAcrylicSnapshot(hwnd);
		}
		// 最小化 / 还原作用在 acrylic (任务栏身份窗) 上, 主窗自己不动。
		// 若放任主窗自己最小化: 主窗是 owned 窗 → 没有任务栏按钮; acrylic 又被
		// 上面 WM_SIZE 的旧逻辑 SW_HIDE → 隐藏窗也没有任务栏按钮 ⇒ 应用整个从
		// 任务栏消失, 主窗还会以一个 iconic 小窗停在屏幕左下角
		// (实测 rect=(0, 屏幕高-111) 237x39)。
		if (XBlurDComp_TaskbarOnAcrylicFor(hwnd) && acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
			UINT cmd = (UINT)(wParam & 0xFFF0);
			// Win+Up 也可能直接作用于 owned 主窗。任务栏 owner 为 iconic
			// 时这次按键只还原；下一次 Win+Up 才由 XCGUI 执行最大化。
			if (cmd == SC_MAXIMIZE && ::IsIconic(acrylicBackdrop)){
				::ShowWindow(acrylicBackdrop, SW_RESTORE);
				return 0;
			}
			if (cmd == SC_MINIMIZE && !::IsIconic(acrylicBackdrop) &&
			    !XBlurDComp_AcrylicMinInFlight(hwnd)){
				// 标记"最小化进行中": 这一步会连带触发系统自动隐藏 owned 主窗,
				// 那次 WM_SHOWWINDOW(FALSE) 必须被 WM_SHOWWINDOW 分支忽略,
				// 否则 acrylic 会被反向 SW_HIDE → 任务栏按钮先移除再重建。
				XBlurDComp_Trace("[WM_SYSCOMMAND] SC_MINIMIZE -> ShowWindow(acrylic, SW_MINIMIZE)");
				XBlurDComp_BeginAcrylicMinimize(hwnd);
				::ShowWindow(acrylicBackdrop, SW_MINIMIZE);
				return 0;
			}
			if (cmd == SC_RESTORE && ::IsIconic(acrylicBackdrop)){
				::ShowWindow(acrylicBackdrop, SW_RESTORE);
				return 0;
			}
		}
		break;
	}
	case WM_WINDOWPOSCHANGING: {
		// 在主窗移到停靠位置前最小化 owner，避免窗口闪现。
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop) &&
		    (::GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_MINIMIZE) != 0 &&
		    ::IsWindowVisible(hwnd) && !XBlurDComp_AcrylicMinInFlight(hwnd)){
			// (1) 抓一帧 peek / Alt+Tab 缩略图要用的位图。两种身份归属模式都要抓:
			//     默认模式下身份是 XCGUI 主窗, 它最小化后 DWM 只有**它自己**的重定向
			//     表面 (没有 owner 的 acrylic 合成层) → peek 就是那个背景透明洞。
			XBlurDComp_CaptureAcrylicSnapshot(hwnd);

			// (2) 只有在"任务栏身份挂在 acrylic 上"这种旧模式里才需要接管: 那时主窗
			//     只是 owned 子窗, 让它自己最小化会先在屏幕左下角可见地停一下。
			if (XBlurDComp_TaskbarOnAcrylicFor(hwnd) && !::IsIconic(acrylicBackdrop)){
				XBlurDComp_Trace("[WM_WINDOWPOSCHANGING] XCGUI self-minimize -> capture + hand to acrylic");
				XBlurDComp_BeginAcrylicMinimize(hwnd);
				::ShowWindow(acrylicBackdrop, SW_MINIMIZE);
			}
		}
		LRESULT r = ::DefSubclassProc(hwnd, msg, wParam, lParam);
		// 主窗最小化/还原时系统会把它挪到 (-32000,-32000) 或 (0, 屏幕高-111)
		// 这类"停靠位"。绝不能跟着同步给 acrylic, 否则 acrylic 会被拽跑并在
		// 还原后错位。
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop) && !contentMinimized()){
			WINDOWPOS* wp = (WINDOWPOS*)lParam;
			// Shell 的快捷键路径有时直接 ShowWindow/MAXIMIZE，绕过 SC_MAXIMIZE。
			if (::IsIconic(acrylicBackdrop) && ::IsWindowVisible(hwnd) &&
			    wp && (::IsZoomed(hwnd) || (wp->flags & SWP_SHOWWINDOW)))
				::ShowWindow(acrylicBackdrop, SW_RESTORE);
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
				if (XBlurDComp_TaskbarOnAcrylicFor(hwnd)){
					// 主窗还是自己 iconic 了 (XCGUI 直接调 ShowWindow(SW_MINIMIZE),
					// 绕过了 WM_SYSCOMMAND)。任务栏身份在 acrylic 上, 所以:
					//   1) 让 acrylic 真正进入最小化态 → 任务栏按钮保留;
					//   2) 隐藏 XCGUI 的 iconic 停靠小窗，但保留其最小化状态。
					//      owner 还原时再还原 XCGUI；若这里立刻 RESTORE+HIDE，
					//      XCGUI 会停止提交控件画面，直到下一次布局尺寸改变。
					if (!::IsIconic(acrylicBackdrop) && !XBlurDComp_AcrylicMinInFlight(hwnd)){
						// 同 WM_SYSCOMMAND 的 SC_MINIMIZE 分支: 标记最小化进行中,
						// 免得连带隐藏 owned 主窗时把 acrylic 反向打成 SW_HIDE。
						XBlurDComp_Trace("[WM_SIZE] SIZE_MINIMIZED -> ShowWindow(acrylic, SW_MINIMIZE), then main HIDE");
						XBlurDComp_BeginAcrylicMinimize(hwnd);
						::ShowWindow(acrylicBackdrop, SW_MINIMIZE);
					}
					::ShowWindow(hwnd, SW_HIDE);
				} else {
					::ShowWindow(acrylicBackdrop, SW_HIDE);
				}
			} else if (!contentMinimized()){
				if (!::IsWindowVisible(acrylicBackdrop) && !::IsIconic(acrylicBackdrop)){
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
		XBlurDComp_ScheduleAcrylicSnapshot(hwnd);
		break;
	case WM_WINDOWPOSCHANGED:
		if (acrylicBackdrop && ::IsWindow(acrylicBackdrop)){
			WINDOWPOS* wp = (WINDOWPOS*)lParam;
			if (wp && (!(wp->flags & SWP_NOSIZE) || !(wp->flags & SWP_NOMOVE)) &&
			    !contentMinimized()) XBlurDComp_ScheduleAcrylicSnapshot(hwnd);
			// 部分框架路径只通过 WINDOWPOS 标志改变可见性，未必单独发送 WM_SHOWWINDOW。
			// acrylic 最小化期间跳过: 那时主窗的可见性变化是 owner 最小化引起的连带
			// 效果, 不能拿来反向 show/hide acrylic。
			//
			// **"最小化进行中"也必须跳过** (与 WM_SHOWWINDOW 分支同理): owner 最小化
			// 会连带隐藏 owned 主窗, 该隐藏会带着 SWP_HIDEWINDOW 走到这里。若照做把
			// acrylic SW_HIDE, Explorer 立刻移除任务栏按钮(隐藏窗没有按钮), 随后
			// acrylic 进入 iconic 态按钮又被重建 = "图标先消失再出现"。
			// 这一刻 acrylic 的 IsIconic() 尚未成立, 单靠 IsIconic 守卫拦不住。
			if (wp && (wp->flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW)) &&
			    XBlurDComp_AcrylicMinInFlight(hwnd)){
				XBlurDComp_Trace("[WM_WINDOWPOSCHANGED] flags=0x%08X minimize-in-flight -> skip acrylic vis sync",
				                 (unsigned)wp->flags);
			}
			else if (wp && !::IsIconic(acrylicBackdrop)){
				if (wp->flags & SWP_HIDEWINDOW)
					::ShowWindow(acrylicBackdrop, SW_HIDE);
				else if ((wp->flags & SWP_SHOWWINDOW) && !::IsIconic(hwnd))
					::ShowWindow(acrylicBackdrop, SW_SHOWNA);
			}

			if (wp && !(wp->flags & SWP_NOZORDER)) XBlurDComp_ScheduleZOrderSync(hwnd);
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
			auto itDel = s_acrylicByXcgui.find(hwnd);
			if (itDel != s_acrylicByXcgui.end()){
				XBlurDComp_FreeAcrylicSnapshot(itDel->second);
				s_acrylicByXcgui.erase(itDel);
			}
		}
		XBlurDComp_ReleaseCloakHook();
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
				it->second.tintR = tintR; it->second.tintG = tintG;
				it->second.tintB = tintB; it->second.tintA = tintA;
				it->second.blurOpacity = blurOpacity;
				it->second.saturation = saturation;
				it->second.uniformBrightness = uniformBrightness;
				it->second.noiseAlphaPct = noiseAlphaPct;
				XBlurDComp::Apply(existing,
					tintR, tintG, tintB, tintA,
					blurOpacity, saturation, uniformBrightness, noiseAlphaPct, 0);
				return existing;
			}
			XBlurDComp_FreeAcrylicSnapshot(it->second);
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

	// 任务栏身份默认由 acrylic 持有；预览需要它保留重定向表面。
	const bool taskbarOnAcrylic = XBlurDComp_TaskbarOnAcrylicWindow();
	static const bool acrylicNoRedir =
	    (XBlurDComp_ReadEnvInt(L"XBLUR_ACRYLIC_NOREDIRECTION", 0) != 0);
	LONG_PTR acrylicExStyle;
	if (taskbarOnAcrylic)
		acrylicExStyle = (acrylicNoRedir ? WS_EX_NOREDIRECTIONBITMAP : 0) | WS_EX_APPWINDOW;
	else
		acrylicExStyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;

	// WS_MINIMIZEBOX 使 Explorer 发给身份窗的最小化命令生效。
	DWORD acrylicStyle = WS_POPUP | WS_MINIMIZEBOX;

	RECT xcRect; ::GetWindowRect(xcguiHwnd, &xcRect);
	HWND acrylicBackdrop = ::CreateWindowExW(
		(DWORD)acrylicExStyle,
		kAcCls, L"",
		acrylicStyle,
		xcRect.left - kAcrylicOuterPx, xcRect.top - kAcrylicOuterPx,
		(xcRect.right - xcRect.left) + 2 * kAcrylicOuterPx,
		(xcRect.bottom - xcRect.top) + 2 * kAcrylicOuterPx,
		originalOwner, NULL, GetModuleHandleW(NULL), NULL);

	if (pfnSetThreadDpiAwarenessContext && prevDpiCtx) {
		pfnSetThreadDpiAwarenessContext(prevDpiCtx);
	}
	if (!acrylicBackdrop) return NULL;

	// 2b. taskbarOnAcrylic 时 acrylic 是"应用主窗", Alt+Tab / 任务栏 tooltip 会读它的
	//     标题与图标。抄过去, 否则这些地方显示空标题 + 默认图标。
	if (taskbarOnAcrylic){
		wchar_t title[512] = {};
		::GetWindowTextW(xcguiHwnd, title, _countof(title) - 1);
		if (title[0]) ::SetWindowTextW(acrylicBackdrop, title);
		HICON icBig = (HICON)::SendMessageW(xcguiHwnd, WM_GETICON, ICON_BIG,   0);
		HICON icSm  = (HICON)::SendMessageW(xcguiHwnd, WM_GETICON, ICON_SMALL, 0);
		if (!icBig) icBig = (HICON)::GetClassLongPtrW(xcguiHwnd, GCLP_HICON);
		if (!icSm)  icSm  = (HICON)::GetClassLongPtrW(xcguiHwnd, GCLP_HICONSM);
		if (icBig) ::SendMessageW(acrylicBackdrop, WM_SETICON, ICON_BIG,   (LPARAM)icBig);
		if (icSm)  ::SendMessageW(acrylicBackdrop, WM_SETICON, ICON_SMALL, (LPARAM)icSm);
	}

	XBlurDComp::Apply(acrylicBackdrop,
		tintR, tintG, tintB, tintA,
		blurOpacity, saturation, uniformBrightness, noiseAlphaPct, 0);

	// 调用方需设置少量非零 alpha，以保留 layered 窗口的鼠标命中区域。

	// 5. 建立 owner-owned 关系。若 XCGUI 原来已有 owner，CreateWindowExW 已先把
	//    acrylic 挂到原 owner；现在再把 XCGUI 挂到 acrylic，完整保留模态/owned 链。
	::SetWindowLongPtrW(xcguiHwnd, GWLP_HWNDPARENT, (LONG_PTR)acrylicBackdrop);

	// 6. taskbar 身份归属。
	//    taskbarOnAcrylic (默认): 主窗摘掉 APPWINDOW, 由 acrylic 窗持有 →
	//      任务栏按钮 / Alt+Tab / peek 全部指向 acrylic 窗; 而 DWM 对 owner 窗做
	//      derivate 缩略图时会连同 owned 子窗 (XCGUI 主窗) 内容一起合成, 于是 peek
	//      拿到「acrylic 合成层 + XCGUI UI」的完整画面 (见文件上方根因说明)。
	//    旧行为 (XBLUR_ACRYLIC_TASKBAR_XCGUI=1): 仅原本无 owner 的普通顶层窗口强制
	//      显示到 taskbar, 模态/owned 窗口保持原扩展样式, 避免多出独立任务栏按钮。
	bool xcguiExChanged = false;
	if (taskbarOnAcrylic){
		LONG_PTR wx = originalExStyle & ~((LONG_PTR)WS_EX_APPWINDOW);
		if (wx != originalExStyle){
			::SetWindowLongPtrW(xcguiHwnd, GWL_EXSTYLE, wx);
			xcguiExChanged = true;
		}
	}
	else if (!originalOwner){
		::SetWindowLongPtrW(xcguiHwnd, GWL_EXSTYLE, originalExStyle | WS_EX_APPWINDOW);
		xcguiExChanged = true;
	}

	// owner 变更后刷新任务栏按钮；须在同步 subclass 安装前完成。
	if (taskbarOnAcrylic && ::IsWindowVisible(xcguiHwnd)){
		::ShowWindow(xcguiHwnd, SW_HIDE);
		::ShowWindow(xcguiHwnd, SW_SHOW);
		xcguiExChanged = false;   // 已在正确时机刷过, 第 10 步不要再刷
	}

	// 7. acrylic 用系统默认 frame: Win11 自动加 round corner + BORDER_COLOR + frame shadow.
	//    XCGUI 主窗这边由调用方自己控.

	// 8a. acrylic 自己装 subclass 处理 WM_DPICHANGED + 激活/焦点转发 — acrylic 是
	//     独立 top-level, 系统直接给它发 DPI 改变, 但 DefWindowProc 不刷新 frame
	//     attribute. 必须 hook. refData 传主窗 hwnd, 供 WM_ACTIVATE/WM_SETFOCUS 转发.
	::SetWindowSubclass(acrylicBackdrop, AcrylicWndSubclassProc, 0xACDC, (DWORD_PTR)xcguiHwnd);

	// 8. Subclass XCGUI 同步 acrylic backdrop 的位置 / 大小 / 可见性 / topmost / 销毁.
	::SetWindowSubclass(xcguiHwnd, XcguiToAcrylicSyncProc, 0xACBD, (DWORD_PTR)acrylicBackdrop);

	{
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		AcrylicHostEntry e;
		e.acrylicHwnd        = acrylicBackdrop;
		e.xcguiHwnd          = xcguiHwnd;
		e.originalOwner      = originalOwner;
		e.originalExStyle    = originalExStyle;
		e.taskbarOnAcrylic   = taskbarOnAcrylic;
		e.tintR = tintR; e.tintG = tintG; e.tintB = tintB; e.tintA = tintA;
		e.blurOpacity = blurOpacity;
		e.saturation  = saturation;
		e.uniformBrightness = uniformBrightness;
		e.noiseAlphaPct = noiseAlphaPct;
		s_acrylicByXcgui[xcguiHwnd] = e;
	}

	// 须在 map 登记后为任务栏身份窗配置可选的自定义缩略图。
	XBlurDComp_EnableIconicBitmapFor(xcguiHwnd);

	// 旧 TOOLWINDOW 背板需要跟随主窗的虚拟桌面 cloak 状态。
	XBlurDComp_EnsureCloakHook();

	// 9. 显示 acrylic backdrop (SW_SHOWNA 不抢焦点).
	::ShowWindow(acrylicBackdrop, SW_SHOWNA);
	XBlurDComp_ScheduleZOrderSync(xcguiHwnd);
	XBlurDComp_ScheduleAcrylicSnapshot(xcguiHwnd);

	// 先激活身份窗，让首次任务栏点击触发最小化。
	if (taskbarOnAcrylic && !::IsIconic(acrylicBackdrop)){
		::SetForegroundWindow(acrylicBackdrop);
	}

	// 仅刷新扩展样式实际改变的窗口；隐藏 owner 会连带隐藏 XCGUI。
	if (xcguiExChanged && ::IsWindowVisible(xcguiHwnd)){
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

void UpdateAcrylicEffectArgs(void* hxwOpaque,
                             int tintR, int tintG, int tintB, int tintA,
                             float blurOpacity, float saturation,
                             BOOL uniformBrightness, float noiseAlphaPct){
	HWINDOW hWnd = (HWINDOW)hxwOpaque;
	if (!hWnd || !::XC_IsHWINDOW((HXCGUI)hWnd)) return;
	HWND xcguiHwnd = ::XWnd_GetHWND(hWnd);
	if (!xcguiHwnd) return;
	std::lock_guard<std::mutex> lk(s_acrylicMutex);
	auto it = s_acrylicByXcgui.find(xcguiHwnd);
	if (it == s_acrylicByXcgui.end()) return;
	AcrylicHostEntry& e = it->second;
	e.tintR = tintR; e.tintG = tintG; e.tintB = tintB; e.tintA = tintA;
	e.blurOpacity = blurOpacity;
	e.saturation = saturation;
	e.uniformBrightness = uniformBrightness;
	e.noiseAlphaPct = noiseAlphaPct;
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
	bool taskbarOnAcrylic = false;
	if (xcguiHwnd){
		std::lock_guard<std::mutex> lk(s_acrylicMutex);
		auto it = s_acrylicByXcgui.find(xcguiHwnd);
		if (it != s_acrylicByXcgui.end()){
			acrylicBackdrop = it->second.acrylicHwnd;
			originalOwner = it->second.originalOwner;
			originalExStyle = it->second.originalExStyle;
			taskbarOnAcrylic = it->second.taskbarOnAcrylic;
			hasEntry = true;
			XBlurDComp_FreeAcrylicSnapshot(it->second);
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
		XBlurDComp_ReleaseCloakHook();
	}

	// taskbarOnAcrylic 下 acrylic 曾持有任务栏身份; 销毁它之后要把 XCGUI 主窗的
	// APPWINDOW 生效回来 (上面的 exstyle 恢复只改了位, taskbar 需要 hide→show 刷新).
	if (hasEntry && taskbarOnAcrylic && xcguiHwnd && ::IsWindow(xcguiHwnd) &&
	    ::IsWindowVisible(xcguiHwnd)){
		::ShowWindow(xcguiHwnd, SW_HIDE);
		::ShowWindow(xcguiHwnd, SW_SHOW);
	}
}

} // namespace XBlurDComp
