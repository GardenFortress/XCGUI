// CXMask: window paint-end snapshot -> premultiplied BGRA Stack Blur -> source copy.
// Included by the uitool aggregate; no screen capture, CXBlur or compositor effects.
#ifndef _XCGUI_UITOOL_AGGREGATED_
#include "module_xcgui_uitool.h"
#endif
#include "xcgui_uitool_shadow_internal.h"
#include <memory>
#include <limits>

namespace _XMask {

constexpr COLORREF kDefaultColor = 0x80000000;
constexpr int kDefaultRadius = 12;
constexpr int kDefaultFadeMs = 240;
constexpr UINT kFadeTimerId = 0x734D;
constexpr UINT kFadeTickMs = 16;

template<class T> struct ComPtr {
    T* p = nullptr;
    ~ComPtr() { if (p) p->Release(); }
    ComPtr() = default;
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    T* operator->() const { return p; }
};

struct Pixels {
    int width = 0, height = 0;
    std::vector<BYTE> data;
    void Allocate(int w, int h) {
        if (w <= 0 || h <= 0 || (size_t)w > (std::numeric_limits<size_t>::max)() / 4 / (size_t)h)
            throw std::bad_alloc();
        data.resize((size_t)w * h * 4);
        width = w; height = h;
    }
};

struct Entry {
    HWINDOW window = nullptr;
    HWND hwnd = nullptr;
    COLORREF color = kDefaultColor;
    int radius = kDefaultRadius;
    int fadeMs = kDefaultFadeMs;
    bool closing = false, fading = false, timerHook = false;
    double progress = 0, fadeFrom = 0, fadeTo = 1;
    ULONGLONG fadeStart = 0;
    DWORD fadeSpan = 0;
    bool closeOnClick = false, open = false, ready = false, sourceCurrent = false;
    bool capture = true, blurDirty = true, tintDirty = true, uploadDirty = true;
    bool baseDirty = true, presentDirty = true;
    bool sizing = false, painting = false, leftDown = false;
    bool paintHook = false, inputHook = false;
    RECT frameRect{};
    RECT observedRect{};
    int frameDpi = 96;
    int observedDpi = 0;
    float frameCorner = 0;
    float observedCorner = 0;
    Pixels source, blurred, tinted, base, presented;
    ID2D1Bitmap1* bitmap = nullptr;
    ID2D1RenderTarget* bitmapTarget = nullptr;
    ~Entry() { ReleaseBitmap(); }
    void ReleaseBitmap() {
        if (bitmap) bitmap->Release();
        if (bitmapTarget) bitmapTarget->Release();
        bitmap = nullptr; bitmapTarget = nullptr; uploadDirty = true;
    }
    void ReleaseFrame() {
        ReleaseBitmap();
        source = Pixels(); blurred = Pixels(); tinted = Pixels();
        base = Pixels(); presented = Pixels(); baseDirty = presentDirty = true;
        ready = sourceCurrent = false; capture = blurDirty = tintDirty = true;
    }
};

// Like the other uitool registries, all access is on the owning UI thread.
// Thread-local storage also keeps independent UI threads out of each other's maps.
using Registry = std::unordered_map<HWINDOW, std::shared_ptr<Entry>>;
static Registry& Entries() { static thread_local Registry entries; return entries; }
static bool Valid(HWINDOW w) {
    return w && XC_IsHWINDOW(w) &&
        ::GetWindowThreadProcessId(XWnd_GetHWND(w), nullptr) == ::GetCurrentThreadId();
}
static std::shared_ptr<Entry> Find(HWINDOW w) {
    auto it = Entries().find(w);
    return it == Entries().end() ? nullptr : it->second;
}
static bool Visible(const Entry& e) {
    return ::IsWindowVisible(e.hwnd) && !::IsIconic(e.hwnd);
}

static int CALLBACK OnPaint(HWINDOW, HDRAW, BOOL*);
static int CALLBACK OnInput(HWINDOW, UINT, WPARAM, LPARAM, BOOL*);
static int CALLBACK OnDestroy(HWINDOW, BOOL*);
static int CALLBACK OnTimer(HWINDOW, UINT, BOOL*);
static void FinishClose(Entry&);

static void SetProgress(Entry& e, double value) {
    if (e.progress == value) return;
    e.progress = value; e.presentDirty = e.uploadDirty = true;
}
static void StopFade(Entry& e) {
    if (e.fading && XC_IsHWINDOW(e.window)) XWnd_KillXCTimer(e.window, kFadeTimerId);
    e.fading = false;
}
static void BeginFade(Entry& e, double target) {
    StopFade(e);
    e.fadeFrom = e.progress; e.fadeTo = target;
    if (e.fadeMs > 0 && e.progress != target && Visible(e)) {
        // Reversals start at the last displayed strength and use the remaining distance.
        e.fadeSpan = (DWORD)(std::max)(1.0, e.fadeMs * fabs(target - e.progress));
        e.fadeStart = ::GetTickCount64();
        if (!e.timerHook) e.timerHook = XWnd_RegEventC1(e.window, XWM_XC_TIMER, (void*)&OnTimer) != FALSE;
        if (e.timerHook && XWnd_SetXCTimer(e.window, kFadeTimerId, kFadeTickMs)) {
            e.fading = true; return;
        }
    }
    SetProgress(e, target); // Hidden hosts / disabled animation / timer failure complete immediately.
}

static std::shared_ptr<Entry> Ensure(HWINDOW w) {
    if (!Valid(w)) return nullptr;
    auto e = Find(w);
    if (e) return e;
    try {
        e = std::make_shared<Entry>();
        e->window = w; e->hwnd = XWnd_GetHWND(w);
        Entries().emplace(w, e);
        if (!XWnd_RegEventC1(w, WM_DESTROY, (void*)&OnDestroy)) {
            Entries().erase(w); return nullptr;
        }
        return e;
    } catch (...) { return nullptr; }
}

static RECT BodyRect(HWINDOW w, int* dpi, float* corner) {
    RECT r{};
    CXShadow::GetBodyClientRect(w, &r);
    int d = XWnd_GetDPI(w);
    if (d <= 0) d = 96;
    *dpi = d; *corner = 0;
    {
        CXShadowModule::HostLock lock;
        auto& hosts = CXShadowModule::HostMap();
        auto it = hosts.find(w);
        if (it != hosts.end() && !it->second->IsMaximized()) {
            RECT client{}; XWnd_GetClientRect(w, &client);
            // Full client means maximized/snap/no outer inset: keep the square body.
            if (!::EqualRect(&r, &client)) *corner = it->second->GetCornerRadius() * d / 96.0f;
        }
    }
    XWnd_RectToDPI(w, &r);
    return r;
}

static bool ReadD2D(ID2D1RenderTarget* rt, const RECT& r, Pixels& out) {
    ComPtr<ID2D1DeviceContext> dc;
    if (FAILED(rt->QueryInterface(__uuidof(ID2D1DeviceContext), (void**)&dc.p))) return false;
    ComPtr<ID2D1Image> target;
    dc->GetTarget(&target.p);
    if (!target.p) return false;
    ComPtr<ID2D1Bitmap1> source;
    if (FAILED(target->QueryInterface(__uuidof(ID2D1Bitmap1), (void**)&source.p))) return false;
    const auto size = source->GetPixelSize();
    if (r.left < 0 || r.top < 0 || r.right > (int)size.width || r.bottom > (int)size.height) return false;
    auto format = source->GetPixelFormat();
    bool rgba = format.format == DXGI_FORMAT_R8G8B8A8_UNORM || format.format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    if (!rgba && format.format != DXGI_FORMAT_B8G8R8A8_UNORM && format.format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
        return false;
    out.Allocate(r.right - r.left, r.bottom - r.top);
    ComPtr<ID2D1Bitmap1> staging;
    auto props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, format);
    if (FAILED(dc->CreateBitmap(D2D1::SizeU(out.width, out.height), nullptr, 0, &props, &staging.p))) return false;
    if (FAILED(dc->Flush())) return false;
    const D2D1_RECT_U srcRect = D2D1::RectU(r.left, r.top, r.right, r.bottom);
    if (FAILED(staging->CopyFromBitmap(nullptr, source.p, &srcRect))) return false;
    D2D1_MAPPED_RECT mapped{};
    if (FAILED(staging->Map(D2D1_MAP_OPTIONS_READ, &mapped))) return false;
    for (int y = 0; y < out.height; ++y)
        memcpy(out.data.data() + (size_t)y * out.width * 4, mapped.bits + (size_t)y * mapped.pitch, (size_t)out.width * 4);
    staging->Unmap();
    for (size_t i = 0; i < out.data.size(); i += 4) {
        if (rgba) std::swap(out.data[i], out.data[i + 2]);
        if (format.alphaMode == D2D1_ALPHA_MODE_IGNORE) out.data[i + 3] = 255;
    }
    return true;
}

struct Dib {
    HDC dc = nullptr; HBITMAP bitmap = nullptr; HGDIOBJ old = nullptr; void* bits = nullptr;
    ~Dib() {
        if (old) ::SelectObject(dc, old);
        if (bitmap) ::DeleteObject(bitmap);
        if (dc) ::DeleteDC(dc);
    }
    bool Create(HDC ref, int w, int h) {
        BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = w; info.bmiHeader.biHeight = -h;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        dc = ::CreateCompatibleDC(ref);
        bitmap = ::CreateDIBSection(ref, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!dc || !bitmap) return false;
        old = ::SelectObject(dc, bitmap);
        if (old == HGDI_ERROR) old = nullptr;
        return old != nullptr;
    }
};
struct SavedDC {
    HDC dc; int saved;
    explicit SavedDC(HDC d) : dc(d), saved(::SaveDC(d)) {
        if (saved) { ::SetMapMode(dc, MM_TEXT); ::SetWindowOrgEx(dc, 0, 0, nullptr); ::SetViewportOrgEx(dc, 0, 0, nullptr); }
    }
    ~SavedDC() { if (saved) ::RestoreDC(dc, saved); }
};

static bool ReadGDI(Entry& e, HDRAW draw, const RECT& r, Pixels& out) {
    HDC dc = XDraw_GetHDC(draw);
    if (!dc) return false;
    int offsetX = 0, offsetY = 0;
    XDraw_GetOffset(draw, &offsetX, &offsetY);
    BITMAP surface{};
    if (!::GetObjectW(::GetCurrentObject(dc, OBJ_BITMAP), sizeof(surface), &surface) ||
        r.left < offsetX || r.top < offsetY || r.right - offsetX > surface.bmWidth || r.bottom - offsetY > surface.bmHeight)
        return false;
    out.Allocate(r.right - r.left, r.bottom - r.top);
    Dib dib;
    if (!dib.Create(dc, out.width, out.height)) return false;
    SavedDC saved(dc);
    if (!saved.saved) return false;
    if (!::BitBlt(dib.dc, 0, 0, out.width, out.height, dc, r.left - offsetX, r.top - offsetY, SRCCOPY)) return false;
    ::GdiFlush();
    memcpy(out.data.data(), dib.bits, out.data.size());
    const auto mode = XWnd_GetTransparentType(e.window);
    if (mode != window_transparent_shaped && mode != window_transparent_shadow)
        for (size_t i = 3; i < out.data.size(); i += 4) out.data[i] = 255;
    return true;
}

// Triangular (Stack Blur) kernel, K(i)=r+1-|i|. Running sums make each pass O(pixels).
// All four channels stay premultiplied; clamped edges avoid dark borders.
static void BlurPass(const Pixels& src, Pixels& dst, int radius, bool vertical) {
    dst.Allocate(src.width, src.height);
    const int length = vertical ? src.height : src.width;
    const int lines = vertical ? src.width : src.height;
    const size_t step = vertical ? (size_t)src.width * 4 : 4;
    const int64_t divisor = (int64_t)(radius + 1) * (radius + 1);
    for (int line = 0; line < lines; ++line) {
        const size_t base = vertical ? (size_t)line * 4 : (size_t)line * src.width * 4;
        auto at = [&](int x) { return src.data.data() + base + (size_t)(std::max)(0, (std::min)(length - 1, x)) * step; };
        int64_t weighted[4]{}, left[4]{}, right[4]{};
        for (int k = -radius; k <= radius; ++k)
            for (int c = 0; c < 4; ++c) weighted[c] += (int64_t)at(k)[c] * (radius + 1 - abs(k));
        for (int k = -radius; k <= 0; ++k)
            for (int c = 0; c < 4; ++c) left[c] += at(k)[c];
        for (int k = 1; k <= radius + 1; ++k)
            for (int c = 0; c < 4; ++c) right[c] += at(k)[c];
        for (int x = 0; x < length; ++x) {
            BYTE* dstPixel = dst.data.data() + base + (size_t)x * step;
            const BYTE* leave = at(x - radius);
            const BYTE* middle = at(x + 1);
            const BYTE* enter = at(x + radius + 2);
            for (int c = 0; c < 4; ++c) {
                dstPixel[c] = (BYTE)((weighted[c] + divisor / 2) / divisor);
                weighted[c] += right[c] - left[c];
                left[c] += (int)middle[c] - leave[c];
                right[c] += (int)enter[c] - middle[c];
            }
        }
    }
}

static void Blur(const Pixels& source, Pixels& result, int radius) {
    if (radius == 0) { result = source; return; }
    Pixels intermediate;
    BlurPass(source, intermediate, radius, false);
    BlurPass(intermediate, result, radius, true);
}

static void Tint(Entry& e, Pixels& result) {
    result.Allocate(e.blurred.width, e.blurred.height);
    const unsigned a = (e.color >> 24) & 255;
    const unsigned tint[4] = {GetBValue(e.color) * a, GetGValue(e.color) * a, GetRValue(e.color) * a, 255 * a};
    const float r = (std::min)(e.frameCorner, (std::min)(result.width, result.height) * 0.5f);
    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            const size_t i = ((size_t)y * result.width + x) * 4;
            unsigned cover = 255;
            if (r > 0) {
                float dx = (std::max)(r - (x + 0.5f), x + 0.5f - (result.width - r));
                float dy = (std::max)(r - (y + 0.5f), y + 0.5f - (result.height - r));
                if (dx > 0 && dy > 0)
                    cover = (unsigned)((std::max)(0.0f, (std::min)(1.0f, r + 0.5f - sqrtf(dx * dx + dy * dy))) * 255 + 0.5f);
            }
            for (int c = 0; c < 4; ++c) {
                unsigned value = (tint[c] + e.blurred.data[i + c] * (255 - a) + 127) / 255;
                // Preserve the original rounded exterior, including its alpha, rather than filling it black.
                result.data[i + c] = (BYTE)((value * cover + e.source.data[i + c] * (255 - cover) + 127) / 255);
            }
        }
    }
}

static bool Process(Entry& e) {
    if (e.source.data.empty()) return false;
    try {
        if (e.blurDirty) {
            Pixels next;
            const int radius = (int)(((int64_t)e.radius * e.frameDpi + 48) / 96);
            Blur(e.source, next, radius);
            e.blurred = std::move(next); e.blurDirty = false; e.tintDirty = true;
        }
        if (e.tintDirty) {
            Pixels next, nextBase;
            if (e.baseDirty) nextBase = e.source;
            Tint(e, next);
            // Commit the matching original and tinted endpoints together. A failed
            // resize must not mix an old mask with a differently sized new source.
            if (e.baseDirty) { e.base = std::move(nextBase); e.baseDirty = false; }
            e.tinted = std::move(next); e.tintDirty = false; e.presentDirty = e.uploadDirty = true;
        }
        return true;
    } catch (...) { return false; }
}

static const Pixels& Presentation(Entry& e) {
    if (e.progress >= 1) return e.tinted;
    if (e.progress <= 0) return e.base;
    if (e.presentDirty) {
        e.presented.Allocate(e.tinted.width, e.tinted.height);
        const unsigned weight = (unsigned)(e.progress * 65535 + 0.5);
        for (size_t i = 0; i < e.tinted.data.size(); ++i)
            e.presented.data[i] = (BYTE)((e.base.data[i] * (65535 - weight) + e.tinted.data[i] * weight + 32767) / 65535);
        e.presentDirty = false;
    }
    return e.presented;
}

static bool DrawD2D(Entry& e, ID2D1RenderTarget* rt, const RECT& rect, const Pixels& pixels) {
    ComPtr<ID2D1DeviceContext> dc;
    if (FAILED(rt->QueryInterface(__uuidof(ID2D1DeviceContext), (void**)&dc.p))) return false;
    if (e.bitmapTarget != rt) e.ReleaseBitmap();
    if (e.uploadDirty) {
        ComPtr<ID2D1Bitmap1> next;
        auto props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
        if (SUCCEEDED(dc->CreateBitmap(D2D1::SizeU(pixels.width, pixels.height), pixels.data.data(),
            pixels.width * 4, &props, &next.p))) {
            e.ReleaseBitmap(); e.bitmap = next.p; next.p = nullptr;
            e.bitmapTarget = rt; rt->AddRef(); e.uploadDirty = false;
        } else if (!e.bitmap) return false;
        // A failed upload may still draw the last bitmap from this target.
    }
    const auto bitmapSize = e.bitmap->GetPixelSize();
    D2D1_MATRIX_3X2_F old; dc->GetTransform(&old);
    auto units = dc->GetUnitMode();
    dc->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    dc->SetTransform(D2D1::Matrix3x2F::Identity());
    dc->PushAxisAlignedClip(D2D1::RectF((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom), D2D1_ANTIALIAS_MODE_ALIASED);
    dc->SetTransform(D2D1::Matrix3x2F::Scale(
        (float)(rect.right - rect.left) / bitmapSize.width,
        (float)(rect.bottom - rect.top) / bitmapSize.height) * D2D1::Matrix3x2F::Translation((float)rect.left, (float)rect.top));
    dc->DrawImage(e.bitmap, nullptr, nullptr, D2D1_INTERPOLATION_MODE_LINEAR, D2D1_COMPOSITE_MODE_SOURCE_COPY);
    dc->SetTransform(D2D1::Matrix3x2F::Identity()); dc->PopAxisAlignedClip();
    dc->SetTransform(old); dc->SetUnitMode(units);
    return true;
}

static bool DrawGDI(HDRAW draw, const RECT& r, const Pixels& pixels) {
    HDC dc = XDraw_GetHDC(draw);
    if (!dc) return false;
    // GDI uses a temporary dirty-rectangle DIB. Its origin is supplied by HDRAW,
    // not by the HDC viewport; D2D, in contrast, keeps a window-sized target.
    int offsetX = 0, offsetY = 0;
    XDraw_GetOffset(draw, &offsetX, &offsetY);
    BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = pixels.width; info.bmiHeader.biHeight = -pixels.height;
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
    SavedDC saved(dc);
    if (!saved.saved) return false;
    ::SetStretchBltMode(dc, COLORONCOLOR);
    int lines = ::StretchDIBits(dc, r.left - offsetX, r.top - offsetY, r.right - r.left, r.bottom - r.top,
        0, 0, pixels.width, pixels.height, pixels.data.data(), &info, DIB_RGB_COLORS, SRCCOPY);
    return lines != 0 && lines != GDI_ERROR;
}

static void DrawFallback(Entry& e, HDRAW draw, const RECT& rect, float corner) {
    const BYTE alpha = (BYTE)((e.color >> 24) * e.progress + 0.5);
    const float width = (float)(rect.right - rect.left), height = (float)(rect.bottom - rect.top);
    corner = (std::min)(corner, (std::min)(width, height) * 0.5f);
    auto rt = reinterpret_cast<ID2D1RenderTarget*>(XDraw_GetD2dRenderTarget(draw));
    if (rt) {
        ComPtr<ID2D1SolidColorBrush> brush;
        if (FAILED(rt->CreateSolidColorBrush(D2D1::ColorF(GetRValue(e.color) / 255.0f,
            GetGValue(e.color) / 255.0f, GetBValue(e.color) / 255.0f, alpha / 255.0f), &brush.p))) return;
        D2D1_MATRIX_3X2_F transform; rt->GetTransform(&transform);
        ComPtr<ID2D1DeviceContext> dc;
        D2D1_UNIT_MODE units = D2D1_UNIT_MODE_DIPS;
        if (SUCCEEDED(rt->QueryInterface(__uuidof(ID2D1DeviceContext), (void**)&dc.p))) {
            units = dc->GetUnitMode(); dc->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
            rt->SetTransform(D2D1::Matrix3x2F::Identity());
        } else {
            float dpiX = 96, dpiY = 96; rt->GetDpi(&dpiX, &dpiY);
            rt->SetTransform(D2D1::Matrix3x2F::Scale(96.0f / dpiX, 96.0f / dpiY));
        }
        auto r = D2D1::RectF((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
        rt->FillRoundedRectangle(D2D1::RoundedRect(r, corner, corner), brush.p);
        rt->SetTransform(transform);
        if (dc.p) dc->SetUnitMode(units);
    } else if (HDC dc = XDraw_GetHDC(draw)) {
        SavedDC saved(dc); if (!saved.saved) return;
        int offsetX = 0, offsetY = 0;
        XDraw_GetOffset(draw, &offsetX, &offsetY);
        Gdiplus::Graphics graphics(dc);
        graphics.TranslateTransform((float)-offsetX, (float)-offsetY);
        Gdiplus::SolidBrush brush(Gdiplus::Color(alpha, GetRValue(e.color), GetGValue(e.color), GetBValue(e.color)));
        if (corner <= 0) graphics.FillRectangle(&brush, (float)rect.left, (float)rect.top, width, height);
        else {
            Gdiplus::GraphicsPath path; float diameter = corner * 2;
            path.AddArc((float)rect.left, (float)rect.top, diameter, diameter, 180, 90);
            path.AddArc(rect.right - diameter, (float)rect.top, diameter, diameter, 270, 90);
            path.AddArc(rect.right - diameter, rect.bottom - diameter, diameter, diameter, 0, 90);
            path.AddArc((float)rect.left, rect.bottom - diameter, diameter, diameter, 90, 90);
            path.CloseFigure(); graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            graphics.FillPath(&brush, &path);
        }
    }
}

static void Redraw(Entry& e, bool capture, bool sync = false) {
    if (capture) { e.capture = true; e.ready = e.sourceCurrent = false; }
    if (e.open && Visible(e)) XWnd_Redraw(e.window, sync && !e.painting);
}

static int CALLBACK OnTimer(HWINDOW w, UINT id, BOOL* handled) {
    auto e = Find(w);
    if (id != kFadeTimerId || !e || !e->open || !e->fading) return 0;
    if (handled) *handled = TRUE;
    const double t = (std::min)(1.0, (double)(::GetTickCount64() - e->fadeStart) / e->fadeSpan);
    const double eased = t * t * (3.0 - 2.0 * t);
    SetProgress(*e, e->fadeFrom + (e->fadeTo - e->fadeFrom) * eased);
    if (t >= 1 || !Visible(*e)) {
        StopFade(*e); SetProgress(*e, e->fadeTo);
        if (e->closing) { FinishClose(*e); return 0; }
    }
    Redraw(*e, false);
    return 0;
}

static int CALLBACK OnPaint(HWINDOW w, HDRAW draw, BOOL*) {
    auto e = Find(w);
    if (!e || !e->open || e->painting || !draw || !Visible(*e)) return 0;
    e->painting = true;
    struct Reset { bool& flag; ~Reset() { flag = false; } } reset{e->painting};
    RECT rect{};
    int dpi = 96; float corner = 0;
    try {
        rect = BodyRect(w, &dpi, &corner);
        if (rect.right <= rect.left || rect.bottom <= rect.top) { e->ready = false; return 0; }
        auto rt = reinterpret_cast<ID2D1RenderTarget*>(XDraw_GetD2dRenderTarget(draw));
        bool changed = !::EqualRect(&rect, &e->observedRect) || dpi != e->observedDpi || corner != e->observedCorner;
        e->observedRect = rect; e->observedDpi = dpi; e->observedCorner = corner;
        // Unexpected geometry changes require a complete underlying repaint before capture.
        if (changed && !e->capture) Redraw(*e, true);
        else if (e->capture && !e->sizing) {
            Pixels source;
            bool ok = rt ? ReadD2D(rt, rect, source) : ReadGDI(*e, draw, rect, source);
            if (ok) {
                e->source = std::move(source); e->frameRect = rect; e->frameDpi = dpi; e->frameCorner = corner;
                e->blurDirty = e->tintDirty = e->baseDirty = true;
            }
            e->capture = false; e->sourceCurrent = ok; e->ready = ok && Process(*e);
        } else if (e->blurDirty || e->tintDirty) e->ready = Process(*e);
        if (!e->tinted.data.empty()) {
            const Pixels& pixels = Presentation(*e);
            bool ok = rt ? DrawD2D(*e, rt, rect, pixels) : DrawGDI(draw, rect, pixels);
            if (ok) {
                e->ready = e->sourceCurrent && !e->blurDirty && !e->tintDirty && (!rt || !e->uploadDirty);
                return 0;
            }
            e->ready = false; e->ReleaseBitmap();
        }
        // Allocation/readback failures still leave a usable input-blocking color mask.
        DrawFallback(*e, draw, rect, corner);
    } catch (...) {
        e->ready = false;
        if (e->capture) e->sourceCurrent = false;
        e->capture = false;
        if (rect.right > rect.left && rect.bottom > rect.top) {
            auto rt = reinterpret_cast<ID2D1RenderTarget*>(XDraw_GetD2dRenderTarget(draw));
            if (!e->tinted.data.empty() &&
                (rt ? DrawD2D(*e, rt, rect, e->tinted) : DrawGDI(draw, rect, e->tinted))) {
                // The emergency frame bypassed the transition; retry its upload next paint.
                e->uploadDirty = true; return 0;
            }
            DrawFallback(*e, draw, rect, corner);
        }
    }
    return 0;
}

static bool ClientPointInside(Entry& e, LPARAM lp) {
    POINT pt{(short)LOWORD(lp), (short)HIWORD(lp)};
    int dpi; float corner; RECT rect = BodyRect(e.window, &dpi, &corner);
    return ::PtInRect(&rect, pt) != FALSE;
}

static int CALLBACK OnInput(HWINDOW w, UINT msg, WPARAM wp, LPARAM lp, BOOL* handled) {
    auto e = Find(w);
    if (!e || !e->open) return 0;
    switch (msg) {
    case WM_ENTERSIZEMOVE: e->sizing = true; break;
    case WM_EXITSIZEMOVE: e->sizing = false; Redraw(*e, true); break;
    case WM_SIZE:
        if (wp != SIZE_MINIMIZED) Redraw(*e, true);
        else {
            e->ready = false; StopFade(*e); SetProgress(*e, e->closing ? 0 : 1);
            if (e->closing) FinishClose(*e);
        }
        break;
    case WM_DPICHANGED: Redraw(*e, true); break;
    case WM_SHOWWINDOW:
        if (wp) { e->capture = true; e->ready = false; XWnd_Redraw(w, FALSE); }
        else {
            e->leftDown = false; e->ready = false;
            StopFade(*e); SetProgress(*e, e->closing ? 0 : 1);
            if (e->closing) FinishClose(*e);
        }
        break;
    case WM_CANCELMODE: case WM_CAPTURECHANGED: e->leftDown = false; break;
    case WM_TOUCH:
        // Consuming WM_TOUCH transfers responsibility for closing its handle.
        ::CloseTouchInputHandle(reinterpret_cast<HTOUCHINPUT>(lp));
        if (handled) *handled = TRUE;
        return 0;
    case WM_LBUTTONDOWN:
        e->leftDown = ClientPointInside(*e, lp);
        if (e->leftDown) ::SetCapture(e->hwnd);
        if (handled) *handled = TRUE;
        return 0;
    case WM_LBUTTONUP: {
        bool close = e->leftDown && e->closeOnClick && ClientPointInside(*e, lp);
        e->leftDown = false;
        if (::GetCapture() == e->hwnd) ::ReleaseCapture();
        if (handled) *handled = TRUE;
        if (close) CXMask::Close(w);
        return 0;
    }
    default: break;
    }
    // Keep sizing, system commands, activation and other window management intact.
    bool mouse = msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST;
    bool keyboard = msg >= WM_KEYFIRST && msg <= WM_KEYLAST;
    bool ime = msg == WM_IME_CHAR || msg == WM_IME_COMPOSITION || msg == WM_IME_STARTCOMPOSITION;
    bool pointer = msg == WM_TOUCH || (msg >= WM_POINTERUPDATE && msg <= WM_POINTERHWHEEL);
    if (mouse || keyboard || ime || pointer || msg == WM_CONTEXTMENU) {
        if (msg == WM_SYSKEYDOWN && wp == VK_F4) return 0; // retain normal window close
        if (handled) *handled = TRUE;
    }
    return 0;
}

static void Unhook(Entry& e) {
    StopFade(e);
    if (XC_IsHWINDOW(e.window)) {
        if (e.paintHook) XWnd_RemoveEventC(e.window, XWM_PAINT_END, (void*)&OnPaint);
        if (e.inputHook) XWnd_RemoveEventC(e.window, XWM_WINDPROC, (void*)&OnInput);
        if (e.timerHook) XWnd_RemoveEventC(e.window, XWM_XC_TIMER, (void*)&OnTimer);
    }
    e.paintHook = e.inputHook = e.timerHook = false;
}
static void FinishClose(Entry& e) {
    e.open = e.closing = false;
    if (e.leftDown && ::GetCapture() == e.hwnd) ::ReleaseCapture();
    e.leftDown = e.sizing = false;
    Unhook(e); e.ReleaseFrame(); SetProgress(e, 0);
    XWnd_Redraw(e.window, !e.painting);
}
static int CALLBACK OnDestroy(HWINDOW w, BOOL*) {
    auto e = Find(w);
    if (!e) return 0;
    e->open = false; Unhook(*e); e->ReleaseFrame(); Entries().erase(w);
    return 0;
}

} // namespace _XMask

BOOL CXMask::Open(HWINDOW w) {
    auto e = _XMask::Ensure(w);
    if (!e) return FALSE;
    if (e->open) {
        if (e->closing) {
            e->closing = false; _XMask::BeginFade(*e, 1); _XMask::Redraw(*e, false, true);
        }
        return TRUE;
    }
    e->paintHook = XWnd_RegEventC1(w, XWM_PAINT_END, (void*)&_XMask::OnPaint) != FALSE;
    e->inputHook = XWnd_RegEventC1(w, XWM_WINDPROC, (void*)&_XMask::OnInput) != FALSE;
    if (!e->paintHook || !e->inputHook) { _XMask::Unhook(*e); return FALSE; }
    ::SendMessageW(e->hwnd, WM_CANCELMODE, 0, 0);
    if (!_XMask::Valid(w) || _XMask::Find(w) != e) return FALSE;
    if (::GetCapture() == e->hwnd) ::ReleaseCapture();
    e->open = true; e->closing = e->leftDown = false;
    _XMask::SetProgress(*e, e->fadeMs > 0 && _XMask::Visible(*e) ? 0 : 1);
    _XMask::Redraw(*e, true, true);
    // Start the clock after the synchronous first snapshot, so blur preparation
    // does not consume the visible fade duration. Modal loops keep ticking it.
    if (_XMask::Find(w) == e && e->open && !e->closing) {
        const double before = e->progress;
        _XMask::BeginFade(*e, 1);
        if (e->progress != before) _XMask::Redraw(*e, false, true);
    }
    return _XMask::Find(w) == e && e->open;
}
BOOL CXMask::Close(HWINDOW w) {
    if (!_XMask::Valid(w)) return FALSE;
    auto e = _XMask::Find(w);
    if (!e || !e->open || e->closing) return TRUE;
    e->closing = true; _XMask::BeginFade(*e, 0);
    if (!e->fading) _XMask::FinishClose(*e);
    return TRUE;
}
BOOL CXMask::IsOpen(HWINDOW w) { auto e = _XMask::Find(w); return e && e->open; }
BOOL CXMask::IsBlurReady(HWINDOW w) { auto e = _XMask::Find(w); return e && e->open && e->ready && e->sourceCurrent; }
BOOL CXMask::SetBkColor(HWINDOW w, COLORREF color) {
    auto e = _XMask::Ensure(w); if (!e) return FALSE;
    if (e->color != color) { e->color = color; e->tintDirty = true; _XMask::Redraw(*e, false, true); }
    return TRUE;
}
COLORREF CXMask::GetBkColor(HWINDOW w) { auto e = _XMask::Find(w); return e ? e->color : _XMask::kDefaultColor; }
BOOL CXMask::SetBlurRadius(HWINDOW w, int radius) {
    auto e = _XMask::Ensure(w); if (!e) return FALSE;
    radius = (std::max)(0, (std::min)(64, radius));
    if (e->radius != radius) { e->radius = radius; e->blurDirty = true; _XMask::Redraw(*e, false, true); }
    return TRUE;
}
int CXMask::GetBlurRadius(HWINDOW w) { auto e = _XMask::Find(w); return e ? e->radius : _XMask::kDefaultRadius; }
BOOL CXMask::SetFadeDuration(HWINDOW w, int ms) {
    auto e = _XMask::Ensure(w); if (!e) return FALSE;
    ms = (std::max)(0, ms);
    if (e->fadeMs == ms) return TRUE;
    e->fadeMs = ms;
    if (e->fading) {
        _XMask::BeginFade(*e, e->closing ? 0 : 1);
        if (e->closing && !e->fading) _XMask::FinishClose(*e);
        else _XMask::Redraw(*e, false, true);
    }
    return TRUE;
}
int CXMask::GetFadeDuration(HWINDOW w) { auto e = _XMask::Find(w); return e ? e->fadeMs : _XMask::kDefaultFadeMs; }
BOOL CXMask::SetCloseOnClick(HWINDOW w, BOOL enable) {
    auto e = _XMask::Ensure(w); if (!e) return FALSE; e->closeOnClick = enable != FALSE; return TRUE;
}
BOOL CXMask::GetCloseOnClick(HWINDOW w) { auto e = _XMask::Find(w); return e && e->closeOnClick; }
void CXMask::Cleanup() {
    auto entries = std::move(_XMask::Entries());
    _XMask::Entries().clear();
    for (auto& kv : entries) {
        auto& e = *kv.second;
        bool open = e.open; e.open = false; _XMask::Unhook(e);
        if (XC_IsHWINDOW(e.window)) {
            XWnd_RemoveEventC(e.window, WM_DESTROY, (void*)&_XMask::OnDestroy);
            if (e.leftDown && ::GetCapture() == e.hwnd) ::ReleaseCapture();
            if (open) XWnd_Redraw(e.window, FALSE);
        }
    }
}
