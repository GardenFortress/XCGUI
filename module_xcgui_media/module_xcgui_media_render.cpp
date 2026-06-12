// module_xcgui_media_render.cpp — L1/L2 共享渲染与窗口几何内核
#include "module_xcgui_media.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

//============================================================================
// L2: SizeMoveGuard — 父窗口 WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE (ref-count 按 HWND)
//============================================================================
namespace {

struct _XMedia_SizeMoveSub{
	void*           user   = NULL;
	void          (*onExit)(void*) = NULL;
};

struct _XMedia_SizeMoveHostInfo{
	int                      refCount   = 0;
	bool                     inSizeMove = false;
	bool                     hooked     = false;
	HWINDOW                  hXWnd      = NULL;
	std::vector<_XMedia_SizeMoveSub> subs;
};

std::mutex g_smMutex;
std::unordered_map<HWND, _XMedia_SizeMoveHostInfo> g_smMap;

bool _XMedia_SizeMove_ResolveHost(void* hEle, HWND* pRaw, HWINDOW* pXWnd){
	if (!hEle || !pRaw || !pXWnd) return false;
	HELE ele = (HELE)hEle;
	if (!XC_IsHELE((HXCGUI)ele)) return false;
	HWINDOW hxw = XWidget_GetHWINDOW((HXCGUI)ele);
	if (!hxw || !XC_IsHWINDOW((HXCGUI)hxw)) return false;
	HWND raw = XWnd_GetHWND(hxw);
	if (!raw) return false;
	*pRaw  = raw;
	*pXWnd = hxw;
	return true;
}

static int CALLBACK _XMedia_SizeMove_WindProcCB(HWINDOW hWnd, UINT msg,
                                                 WPARAM /*wParam*/, LPARAM /*lParam*/,
                                                 BOOL* /*pbHandled*/){
	if (msg != WM_ENTERSIZEMOVE && msg != WM_EXITSIZEMOVE) return 0;
	HWND raw = XWnd_GetHWND(hWnd);
	if (!raw) return 0;

	std::vector<_XMedia_SizeMoveSub> exitSubs;
	{
		std::lock_guard<std::mutex> lk(g_smMutex);
		auto it = g_smMap.find(raw);
		if (it == g_smMap.end()) return 0;
		if (msg == WM_ENTERSIZEMOVE){
			it->second.inSizeMove = true;
		} else {
			it->second.inSizeMove = false;
			exitSubs = it->second.subs;
		}
	}
	if (msg == WM_EXITSIZEMOVE){
		for (auto& s : exitSubs){
			if (s.onExit) s.onExit(s.user);
		}
	}
	return 0;
}

} // namespace

void _XMedia_SizeMoveGuard_Attach(void* hEle, void (*onExit)(void*), void* user){
	HWND raw = NULL;
	HWINDOW hxw = NULL;
	if (!_XMedia_SizeMove_ResolveHost(hEle, &raw, &hxw)) return;

	bool needHook = false;
	{
		std::lock_guard<std::mutex> lk(g_smMutex);
		auto& info = g_smMap[raw];
		info.hXWnd = hxw;
		bool found = false;
		for (auto& s : info.subs){
			if (s.user == user){
				s.onExit = onExit;
				found = true;
				break;
			}
		}
		if (!found){
			_XMedia_SizeMoveSub sub;
			sub.user   = user;
			sub.onExit = onExit;
			info.subs.push_back(sub);
			info.refCount++;
		}
		if (!info.hooked){
			info.hooked = true;
			needHook  = true;
		}
	}
	if (needHook){
		::XWnd_RegEventC1(hxw, XWM_WINDPROC, (void*)_XMedia_SizeMove_WindProcCB);
	}
}

void _XMedia_SizeMoveGuard_Detach(void* hEle, void* user){
	HWND raw = NULL;
	HWINDOW hxw = NULL;
	if (!_XMedia_SizeMove_ResolveHost(hEle, &raw, &hxw)) return;

	bool needUnhook = false;
	{
		std::lock_guard<std::mutex> lk(g_smMutex);
		auto it = g_smMap.find(raw);
		if (it == g_smMap.end()) return;
		auto& info = it->second;
		for (auto sit = info.subs.begin(); sit != info.subs.end(); ++sit){
			if (sit->user == user){
				info.subs.erase(sit);
				if (info.refCount > 0) info.refCount--;
				break;
			}
		}
		if (info.subs.empty()){
			needUnhook = info.hooked;
			g_smMap.erase(it);
		}
	}
	if (needUnhook){
		::XWnd_RemoveEventC(hxw, XWM_WINDPROC, (void*)_XMedia_SizeMove_WindProcCB);
	}
}

bool _XMedia_SizeMoveGuard_IsActive(void* hEle){
	HWND raw = NULL;
	HWINDOW hxw = NULL;
	if (!_XMedia_SizeMove_ResolveHost(hEle, &raw, &hxw)) return false;
	std::lock_guard<std::mutex> lk(g_smMutex);
	auto it = g_smMap.find(raw);
	if (it == g_smMap.end()) return false;
	return it->second.inSizeMove;
}

void _XMedia_Render_ComputeDestRect(int fitMode, int srcW, int srcH,
                                     int eleW, int eleH, RECT* pDst){
	pDst->left = pDst->top = pDst->right = pDst->bottom = 0;
	if (eleW <= 0 || eleH <= 0 || srcW <= 0 || srcH <= 0) return;

	if (fitMode == xmedia_fit_stretch){
		pDst->left = 0; pDst->top = 0;
		pDst->right = eleW; pDst->bottom = eleH;
		return;
	}
	if (fitMode == xmedia_fit_original){
		int dx = (eleW - srcW) / 2;
		int dy = (eleH - srcH) / 2;
		pDst->left   = dx;
		pDst->top    = dy;
		pDst->right  = dx + srcW;
		pDst->bottom = dy + srcH;
		return;
	}

	double sx = (double)eleW / (double)srcW;
	double sy = (double)eleH / (double)srcH;
	double s  = (fitMode == xmedia_fit_cover) ? (std::max)(sx, sy) : (std::min)(sx, sy);

	int w = (int)(srcW * s + 0.5);
	int h = (int)(srcH * s + 0.5);
	int dx = (eleW - w) / 2;
	int dy = (eleH - h) / 2;
	pDst->left   = dx;
	pDst->top    = dy;
	pDst->right  = dx + w;
	pDst->bottom = dy + h;
}

bool _XMedia_Render_EnsureGdiDib(_XMedia_GdiDib* pDib, int w, int h){
	if (!pDib || w <= 0 || h <= 0) return false;
	if (pDib->memDC && pDib->dib && pDib->dibW == w && pDib->dibH == h) return true;
	_XMedia_Render_ReleaseGdiDib(pDib);

	HDC dcMem = ::CreateCompatibleDC(NULL);
	if (!dcMem) return false;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = w;
	bmi.bmiHeader.biHeight      = -h;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* pBits = NULL;
	HBITMAP dib = ::CreateDIBSection(dcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
	if (!dib){
		::DeleteDC(dcMem);
		return false;
	}
	HBITMAP oldBmp = (HBITMAP)::SelectObject(dcMem, dib);
	pDib->memDC  = dcMem;
	pDib->dib    = dib;
	pDib->oldBmp = oldBmp;
	pDib->dibW   = w;
	pDib->dibH   = h;
	return true;
}

void _XMedia_Render_ReleaseGdiDib(_XMedia_GdiDib* pDib){
	if (!pDib) return;
	if (pDib->memDC && pDib->oldBmp){
		::SelectObject(pDib->memDC, pDib->oldBmp);
	}
	pDib->oldBmp = NULL;
	if (pDib->dib){
		::DeleteObject(pDib->dib);
		pDib->dib = NULL;
	}
	if (pDib->memDC){
		::DeleteDC(pDib->memDC);
		pDib->memDC = NULL;
	}
	pDib->dibW = pDib->dibH = 0;
}

void _XMedia_Render_FillGdiDibBk(_XMedia_GdiDib* pDib, int eleW, int eleH, COLORREF bkColor){
	if (!pDib || !pDib->memDC) return;
	RECT rcAll = { 0, 0, eleW, eleH };
	COLORREF gdiBg = bkColor & 0x00FFFFFF;
	HBRUSH hBg = ::CreateSolidBrush(gdiBg);
	if (hBg){
		::FillRect(pDib->memDC, &rcAll, hBg);
		::DeleteObject(hBg);
	}
}

void _XMedia_Render_StretchBgraToDib(HDC dcMem, const uint8_t* bgra, int srcW, int srcH,
                                      int dstX, int dstY, int dstW, int dstH){
	if (!dcMem || !bgra || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;
	BITMAPINFO sbmi = {};
	sbmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	sbmi.bmiHeader.biWidth       = srcW;
	sbmi.bmiHeader.biHeight      = -srcH;
	sbmi.bmiHeader.biPlanes      = 1;
	sbmi.bmiHeader.biBitCount    = 32;
	sbmi.bmiHeader.biCompression = BI_RGB;
	::SetStretchBltMode(dcMem, COLORONCOLOR);
	::StretchDIBits(dcMem,
	                dstX, dstY, dstW, dstH,
	                0, 0, srcW, srcH,
	                (void*)bgra, &sbmi, DIB_RGB_COLORS, SRCCOPY);
}

void _XMedia_Render_BlitGdiDibStretch(HDC hdcDest, const _XMedia_GdiDib* pDib,
                                       int destX, int destY, int destW, int destH){
	if (!hdcDest || !pDib || !pDib->memDC || !pDib->dib) return;
	::SetStretchBltMode(hdcDest, COLORONCOLOR);
	::StretchBlt(hdcDest, destX, destY, destW, destH,
	             pDib->memDC, 0, 0, pDib->dibW, pDib->dibH, SRCCOPY);
}

namespace {

void _XMedia_Render_FillD2DBk(ID2D1RenderTarget* rt, const RECT& rcEle,
                               COLORREF bkColor, bool respectAlpha){
	if (!rt) return;
	if (respectAlpha){
		BYTE bgA = GetAValue(bkColor);
		if (bgA == 0) return;
		ID2D1SolidColorBrush* pBrush = NULL;
		HRESULT hr = rt->CreateSolidColorBrush(
			D2D1::ColorF(GetRValue(bkColor) / 255.0f, GetGValue(bkColor) / 255.0f,
			             GetBValue(bkColor) / 255.0f, bgA / 255.0f),
			&pBrush);
		if (SUCCEEDED(hr) && pBrush){
			D2D1_RECT_F rc = D2D1::RectF(
				(FLOAT)rcEle.left, (FLOAT)rcEle.top,
				(FLOAT)rcEle.right, (FLOAT)rcEle.bottom);
			rt->FillRectangle(rc, pBrush);
			pBrush->Release();
		}
	} else {
		ID2D1SolidColorBrush* bg = NULL;
		D2D1_COLOR_F d2dC = D2D1::ColorF(
			GetRValue(bkColor) / 255.0f, GetGValue(bkColor) / 255.0f,
			GetBValue(bkColor) / 255.0f, 1.0f);
		if (SUCCEEDED(rt->CreateSolidColorBrush(d2dC, &bg)) && bg){
			D2D1_RECT_F rfEle = D2D1::RectF(
				(float)rcEle.left, (float)rcEle.top,
				(float)rcEle.right, (float)rcEle.bottom);
			rt->FillRectangle(rfEle, bg);
			bg->Release();
		}
	}
}

} // namespace

bool _XMedia_Render_PaintD2D_Bgra(const _XMedia_Render_PaintD2DParams* p){
	if (!p || !p->rt || !p->rcEle || !p->cache) return false;
	const RECT& rcEle = *p->rcEle;
	int eleW = rcEle.right - rcEle.left;
	int eleH = rcEle.bottom - rcEle.top;
	if (eleW <= 0 || eleH <= 0) return false;

	_XMedia_Render_FillD2DBk(p->rt, rcEle, p->bkColor, p->bkRespectAlpha);

	if (!p->bgra || p->srcW <= 0 || p->srcH <= 0 || !p->rcDstLocal) return true;

	int pitch = (p->srcPitch > 0) ? p->srcPitch : (p->srcW * 4);
	bool dirty = p->pDirty ? *p->pDirty : false;
	_XMedia_Render_D2DBmpCache* cache = p->cache;

	if (p->rt != cache->pLastRT){
		SafeRelease(cache->pBmp);
		cache->pLastRT = p->rt;
		cache->bmpW = cache->bmpH = 0;
		cache->uploadedBgraPtr = NULL;
	}
	if (cache->pBmp && (cache->bmpW != p->srcW || cache->bmpH != p->srcH)){
		SafeRelease(cache->pBmp);
		cache->bmpW = cache->bmpH = 0;
		cache->uploadedBgraPtr = NULL;
	}
	bool bitmapJustCreated = false;
	if (!cache->pBmp){
		D2D1_BITMAP_PROPERTIES props = {};
		props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		props.pixelFormat.alphaMode = (p->alphaMode == _xmedia_d2d_alpha_premul)
		                            ? D2D1_ALPHA_MODE_PREMULTIPLIED
		                            : D2D1_ALPHA_MODE_IGNORE;
		props.dpiX = 96.0f;
		props.dpiY = 96.0f;
		HRESULT hr = p->rt->CreateBitmap(
			D2D1::SizeU((UINT32)p->srcW, (UINT32)p->srcH),
			NULL, 0, &props, &cache->pBmp);
		if (FAILED(hr) || !cache->pBmp) return false;
		cache->bmpW = p->srcW;
		cache->bmpH = p->srcH;
		bitmapJustCreated = true;
	}

	bool needUpload = bitmapJustCreated || dirty;
	if (!needUpload && p->bgra && cache->pBmp){
		if (p->bgra != cache->uploadedBgraPtr)
			needUpload = true;
	}
	if (needUpload && cache->pBmp && p->bgra){
		D2D1_RECT_U dstRc = D2D1::RectU(0, 0, (UINT32)p->srcW, (UINT32)p->srcH);
		HRESULT hr = cache->pBmp->CopyFromMemory(&dstRc, p->bgra, (UINT32)pitch);
		if (FAILED(hr)) return false;
		cache->uploadedBgraPtr = p->bgra;
		if (p->pDirty) *p->pDirty = false;
	}

	const RECT& rcDst = *p->rcDstLocal;
	D2D1_RECT_F dstRect = D2D1::RectF(
		(float)(rcEle.left + rcDst.left),
		(float)(rcEle.top  + rcDst.top),
		(float)(rcEle.left + rcDst.right),
		(float)(rcEle.top  + rcDst.bottom));

	bool needClip = (p->fitMode == xmedia_fit_cover) || (p->fitMode == xmedia_fit_original);
	if (needClip){
		D2D1_RECT_F clipRect = D2D1::RectF(
			(float)rcEle.left, (float)rcEle.top,
			(float)rcEle.right, (float)rcEle.bottom);
		p->rt->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
	}

	if (p->stretchFromSrc){
		D2D1_RECT_F srcRect = D2D1::RectF(0.0f, 0.0f, (float)p->srcW, (float)p->srcH);
		p->rt->DrawBitmap(cache->pBmp, &dstRect, 1.0f,
		                  D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &srcRect);
	} else {
		p->rt->DrawBitmap(cache->pBmp, dstRect, 1.0f,
		                  D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	}

	if (needClip){
		p->rt->PopAxisAlignedClip();
	}
	return true;
}

bool _XMedia_Render_PaintGdi_Bgra(const _XMedia_Render_PaintGdiParams* p){
	if (!p || !p->hdcDest || !p->pDib || !p->rcDstHdc || !p->rcDstLocal) return false;
	if (p->eleWPhys <= 0 || p->eleHPhys <= 0) return false;
	if (!_XMedia_Render_EnsureGdiDib(p->pDib, p->eleWPhys, p->eleHPhys)) return false;

	if (p->needRedraw){
		_XMedia_Render_FillGdiDibBk(p->pDib, p->eleWPhys, p->eleHPhys, p->bkColor);
		const RECT& rcDst = *p->rcDstLocal;
		int dstW = rcDst.right - rcDst.left;
		int dstH = rcDst.bottom - rcDst.top;
		if (p->bgra && p->srcW > 0 && p->srcH > 0 && dstW > 0 && dstH > 0){
			_XMedia_Render_StretchBgraToDib(p->pDib->memDC, p->bgra, p->srcW, p->srcH,
			                                 rcDst.left, rcDst.top, dstW, dstH);
		}
		if (p->pDibDirty) *p->pDibDirty = false;
	}

	_XMedia_Render_BlitGdiDibStretch(p->hdcDest, p->pDib,
	                                  p->rcDstHdc->left, p->rcDstHdc->top,
	                                  p->eleWPhys, p->eleHPhys);
	return true;
}
