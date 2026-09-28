// AttachToWndEx 的内容圆角：在 XCGUI 提交 layered 表面前清除四角。
// DWM 的圆角只作用于 acrylic HWND，不能裁剪上方的独立 XCGUI HWND。
namespace XBlurContentClip {
static const wchar_t kProperty[] = L"XCGUI.CXBlur.ContentClip";
static const UINT_PTR kSubclass = 0xACBE;

struct State {
    ID2D1DeviceContext* device = nullptr;
    ID2D1Bitmap1* mask = nullptr;
    float radius = -1.0f;
    HDC patchDC = nullptr;
    HBITMAP patchBitmap = nullptr;
    HGDIOBJ patchOld = nullptr;
    BYTE* patchBits = nullptr;
    int extent = 0;
    std::vector<BYTE> coverage;
    void ResetGdi() {
        if (patchDC && patchOld) ::SelectObject(patchDC, patchOld);
        if (patchBitmap) ::DeleteObject(patchBitmap);
        if (patchDC) ::DeleteDC(patchDC);
        patchDC = nullptr; patchBitmap = nullptr; patchOld = nullptr; patchBits = nullptr;
        coverage.clear();
    }
    ~State() { ResetDevice(); ResetGdi(); }
    void ResetDevice() {
        SafeRelease(mask);
        SafeRelease(device);
    }
};

static float RadiusPx(HWND content, HWND acrylic) {
    if (!acrylic || ::IsZoomed(content)) return 0.0f;
    DWORD preference = 0;
    if (FAILED(::DwmGetWindowAttribute(acrylic, 33, &preference, sizeof(preference))) ||
        preference == 1 /* DWMWCP_DONOTROUND */) return 0.0f;
    // DWM 提供偏好而不提供半径查询。Win11 ROUND / ROUNDSMALL 分别以 8 / 4 DIP 对齐。
    // 必须用背板实际 DPI；XWnd_SetDPI 的布局 DPI 可能与系统 DPI 不同。
    using GetDpiFn = UINT (WINAPI*)(HWND);
    static auto getDpi = reinterpret_cast<GetDpiFn>(::GetProcAddress(
        ::GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    const UINT dpi = getDpi ? getDpi(acrylic) : 96;
    UINT border = 0;
    ::DwmGetWindowAttribute(acrylic, 37 /* VISIBLE_FRAME_BORDER_THICKNESS */,
        &border, sizeof(border));
    return (std::max)(0.0f, (preference == 3 ? 4.0f : 8.0f) * dpi / 96.0f - border);
}

static void PrepareCoverage(State& s, float radius) {
    if (s.radius == radius && !s.coverage.empty()) return;
    SafeRelease(s.mask);
    s.ResetGdi();
    s.radius = radius;
    s.extent = (int)std::ceil(radius);
    s.coverage.resize(s.extent*s.extent);
    // 固定的 16x16 亚像素覆盖率，避免 D2D 曲线细分随 dirty clip 改变边缘像素。
    // 缓存只随半径变化重建；D2D 上传一个小角区，GDI 使用同一覆盖率。
    const int samples = 16;
    for (int y=0; y<s.extent; ++y) for (int x=0; x<s.extent; ++x) {
        int inside = 0;
        for (int sy=0; sy<samples; ++sy) for (int sx=0; sx<samples; ++sx) {
            const float dx=radius-(x+(sx+.5f)/samples), dy=radius-(y+(sy+.5f)/samples);
            if (dx<=0 || dy<=0 || dx*dx+dy*dy<=radius*radius) ++inside;
        }
        s.coverage[y*s.extent+x]=(BYTE)((inside*255+samples*samples/2)/(samples*samples));
    }
}

static bool PrepareD2D(State& s, ID2D1DeviceContext* dc, float radius) {
    if (s.device != dc) { s.ResetDevice(); s.device=dc; dc->AddRef(); }
    PrepareCoverage(s, radius);
    if (s.mask) return true;
    std::vector<BYTE> erase(s.extent*s.extent*4, 0);
    for (size_t i=0; i<s.coverage.size(); ++i) erase[i*4+3]=255-s.coverage[i];
    const auto props=D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
    return SUCCEEDED(dc->CreateBitmap(D2D1::SizeU(s.extent, s.extent), erase.data(),
        s.extent*4, &props, &s.mask));
}
static void PaintD2D(State& s, ID2D1DeviceContext* dc, int width, int height,
    const RECT& dirty, float radius) {
    if (!PrepareD2D(s, dc, radius)) return;
    const auto blend = dc->GetPrimitiveBlend();
    const auto units = dc->GetUnitMode();
    D2D1_MATRIX_3X2_F transform;
    dc->GetTransform(&transform);
    dc->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    dc->SetTransform(D2D1::Matrix3x2F::Identity());
    // 只处理刚被框架清理并重画的像素；否则局部刷新会反复衰减未重画的圆角。
    dc->PushAxisAlignedClip(D2D1::RectF((float)dirty.left, (float)dirty.top,
        (float)dirty.right, (float)dirty.bottom), D2D1_ANTIALIAS_MODE_ALIASED);
    dc->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
    for (int i = 0; i < 4; ++i) {
        const bool right = (i & 1) != 0, bottom = (i & 2) != 0;
        const float x = right ? (float)width : 0, y = bottom ? (float)height : 0;
        if ((right ? dirty.right <= width-radius-1 : dirty.left >= radius+1) ||
            (bottom ? dirty.bottom <= height-radius-1 : dirty.top >= radius+1)) continue;
        dc->SetTransform(D2D1::Matrix3x2F(right ? -1.0f : 1.0f, 0, 0,
            bottom ? -1.0f : 1.0f, x, y));
        // DESTINATION_OUT 将预乘 RGB 和 alpha 同乘覆盖率，不需要读取整窗表面。
        dc->DrawImage(s.mask, D2D1::Point2F(0, 0),
            D2D1::RectF(0, 0, (float)s.extent, (float)s.extent),
            D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, D2D1_COMPOSITE_MODE_DESTINATION_OUT);
    }
    dc->PopAxisAlignedClip();
    dc->SetTransform(transform);
    dc->SetUnitMode(units);
    dc->SetPrimitiveBlend(blend);
}

static void PaintGdi(State& s, HDC dc, int width, int height, const RECT& dirty, float radius) {
    if (!dc) return;
    PrepareCoverage(s, radius);
    if (!s.patchBits) {
        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = s.extent;
        info.bmiHeader.biHeight = -s.extent;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        s.patchDC = ::CreateCompatibleDC(dc);
        s.patchBitmap = ::CreateDIBSection(dc, &info, DIB_RGB_COLORS,
            (void**)&s.patchBits, nullptr, 0);
        if (!s.patchDC || !s.patchBitmap) { s.ResetGdi(); return; }
        s.patchOld = ::SelectObject(s.patchDC, s.patchBitmap);
    }
    // GDI+ 在 HDC 上的透明 SourceCopy 会丢失边缘覆盖率。
    // 只搬运四个小角区的预乘 BGRA，以同一覆盖率乘 RGB/A，再原样复制回去。
    for (int i=0; i<4; ++i) {
        const int left = (i&1) ? width-s.extent : 0, top = (i&2) ? height-s.extent : 0;
        RECT corner = {left, top, left+s.extent, top+s.extent}, update;
        if (!::IntersectRect(&update, &corner, &dirty)) continue;
        if (!::BitBlt(s.patchDC, 0, 0, s.extent, s.extent, dc, left, top, SRCCOPY)) continue;
        ::GdiFlush();
        for (int py=update.top; py<update.bottom; ++py) for (int px=update.left; px<update.right; ++px) {
            const int x=px-left, y=py-top;
            const int mx=(i&1) ? s.extent-1-x : x, my=(i&2) ? s.extent-1-y : y;
            const BYTE alpha=s.coverage[my*s.extent+mx];
            BYTE* pixel=s.patchBits+(y*s.extent+x)*4;
            for (int c=0; c<4; ++c) pixel[c]=(BYTE)((pixel[c]*alpha+127)/255);
        }
        ::BitBlt(dc, update.left, update.top, update.right-update.left, update.bottom-update.top,
            s.patchDC, update.left-left, update.top-top, SRCCOPY);
        ::GdiFlush(); // 下一角复用 DIB 前完成复制。
    }
}
static int CALLBACK OnPaintEnd(HWINDOW window, HDRAW draw, BOOL*) {
    HWND hwnd = XBlur_GetLiveWindowHwnd(window);
    if (!hwnd) return 0;
    auto s = reinterpret_cast<State*>(::GetPropW(hwnd, kProperty));
    if (!s) return 0;
    RECT client = {}, dirty = {};
    ::GetClientRect(hwnd, &client);
    XWnd_GetDrawRect(window, &dirty);
    XDraw_ConvRect(draw, &dirty); // 局部重绘使用 XCGUI 坐标，转换到物理像素画布。
    if (!::IntersectRect(&dirty, &dirty, &client)) return 0;
    const float radius = (std::min)(RadiusPx(hwnd, XBlurDComp::GetAcrylicHwnd(window)),
        (std::min)(client.right, client.bottom) * .5f);
    if (radius <= 0) return 0;
    if (XC_IsEnableD2D()) {
        auto rt = reinterpret_cast<ID2D1RenderTarget*>(XDraw_GetD2dRenderTarget(draw));
        ID2D1DeviceContext* dc = nullptr;
        if (rt && SUCCEEDED(rt->QueryInterface(IID_PPV_ARGS(&dc)))) {
            PaintD2D(*s, dc, client.right, client.bottom, dirty, radius);
            dc->Release();
        }
    } else PaintGdi(*s, XDraw_GetHDC(draw), client.right, client.bottom, dirty, radius);
    return 0;
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (msg == WM_NCDESTROY) {
        auto s = reinterpret_cast<State*>(::RemovePropW(hwnd, kProperty));
        ::RemoveWindowSubclass(hwnd, WindowProc, kSubclass);
        delete s;
    }
    return ::DefSubclassProc(hwnd, msg, wp, lp);
}

static void Detach(HWINDOW window) {
    HWND hwnd = XBlur_GetLiveWindowHwnd(window);
    if (!hwnd) return;
    auto s = reinterpret_cast<State*>(::RemovePropW(hwnd, kProperty));
    if (!s) return;
    XWnd_RemoveEventC(window, XWM_PAINT_END, (void*)OnPaintEnd);
    ::RemoveWindowSubclass(hwnd, WindowProc, kSubclass);
    delete s;
    XWnd_Redraw(window);
}

static bool Attach(HWINDOW window) {
    Detach(window);
    HWND hwnd = XBlur_GetLiveWindowHwnd(window);
    if (!hwnd) return false;
    auto s = new(std::nothrow) State;
    if (!s) return false;
    if (!::SetPropW(hwnd, kProperty, s)) { delete s; return false; }
    if (!::SetWindowSubclass(hwnd, WindowProc, kSubclass, 0) ||
        !XWnd_RegEventC1(window, XWM_PAINT_END, (void*)OnPaintEnd)) {
        Detach(window);
        return false;
    }
    XWnd_Redraw(window);
    return true;
}
} // namespace XBlurContentClip
