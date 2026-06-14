//============================================================================
// module_xcgui_uitool_common.inc — uitool 公共运行时 (系统环境 + _XUITool 主题层)
// 由 module_xcgui_uitool.cpp 聚合 TU #include; split 可条件 #include (见各 split 顶部).
//============================================================================
#ifndef _XCGUI_UITOOL_COMMON_INC_
#define _XCGUI_UITOOL_COMMON_INC_

DWORD XUITool_GetOsBuild()
{
	static DWORD s_build = 0;
	if (s_build) return s_build;
	HMODULE nt = ::GetModuleHandleW(L"ntdll.dll");
	if (!nt){ s_build = 1; return s_build; }
	typedef LONG (WINAPI* PFN_RtlGetVersion)(OSVERSIONINFOEXW*);
	auto pRtl = (PFN_RtlGetVersion)::GetProcAddress(nt, "RtlGetVersion");
	if (!pRtl){ s_build = 1; return s_build; }
	OSVERSIONINFOEXW vi = {};
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (pRtl(&vi) != 0){ s_build = 1; return s_build; }
	s_build = vi.dwBuildNumber ? vi.dwBuildNumber : 1;
	return s_build;
}

int GetCurrentVersion()
{
	DWORD build = XUITool_GetOsBuild();
	if (build >= 22000) return 11;
	if (build >= 10240) return 10;
	// build 无法区分 Win7/8/8.1, 回退 RtlGetVersion 读 major/minor
	HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
	if (hMod) {
		typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
		RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
		if (RtlGetVersion) {
			RTL_OSVERSIONINFOW osvi = { 0 };
			osvi.dwOSVersionInfoSize = sizeof(osvi);
			if (RtlGetVersion(&osvi) == 0) {
				DWORD major = osvi.dwMajorVersion;
				DWORD minor = osvi.dwMinorVersion;
				if (major == 10) return 10;
				if (major == 6 && minor >= 2) return 8;
				if (major == 6 && minor == 1) return 7;
			}
		}
	}
	return 0;
}

namespace _XUITool {
constexpr int kCornerRadius = 8;

// 预设主题色 (XCGUI COLORREF = ARGB, 0xAABBGGRR)
constexpr COLORREF kDarkText    = RGBA(245, 245, 245, 255);
constexpr COLORREF kDarkBg      = RGBA(23, 23, 23, 255);
constexpr COLORREF kDarkAccent  = RGBA(255, 255, 255, 255);
constexpr COLORREF kLightText   = RGBA(23, 23, 23, 255);
constexpr COLORREF kLightBg     = RGBA(255, 255, 255, 255);
constexpr COLORREF kLightAccent = RGBA(23, 23, 23, 255);

struct ThemePalette
{
	COLORREF text;
	COLORREF bg;
	COLORREF accent;
};

inline COLORREF WithAlpha(COLORREF c, BYTE a)
{
	return (c & 0x00FFFFFF) | ((COLORREF)a << 24);
}

// 读 HKCU\...\Personalize\AppsUseLightTheme. 0 = 深色, 1 = 浅色.
// 失败时默认深色 (匹配本模块默认主题).
BOOL DetectSystemDarkMode()
{
	HKEY hKey = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		0, KEY_READ, &hKey) != ERROR_SUCCESS) return TRUE;
	DWORD val = 1, size = sizeof(val), type = REG_DWORD;
	BOOL dark = TRUE;
	if (RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, &type,
		(LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD){
		dark = (val == 0);
	}
	RegCloseKey(hKey);
	return dark;
}

inline xuitool_theme_ EffectiveTheme(xuitool_theme_ mode)
{
	if (mode == xuitool_theme_auto)
		return DetectSystemDarkMode() ? xuitool_theme_dark : xuitool_theme_light;
	return mode;
}

inline BOOL IsLightTheme(xuitool_theme_ mode)
{
	return EffectiveTheme(mode) == xuitool_theme_light;
}
void ResolvePalette(xuitool_theme_ mode, COLORREF customText, COLORREF customBg,
	COLORREF customAccent, ThemePalette* out)
{
	xuitool_theme_ eff = EffectiveTheme(mode);
	switch (eff){
	case xuitool_theme_light:
		out->text   = kLightText;
		out->bg     = kLightBg;
		out->accent = kLightAccent;
		break;
	case xuitool_theme_custom:
		out->text   = customText;
		out->bg     = customBg;
		out->accent = customAccent;
		break;
	case xuitool_theme_dark:
	default:
		out->text   = kDarkText;
		out->bg     = kDarkBg;
		out->accent = kDarkAccent;
		break;
	}
}

} // namespace _XUITool
#endif // _XCGUI_UITOOL_COMMON_INC_
