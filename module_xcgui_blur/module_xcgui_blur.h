#ifndef  XCGUI_BLUR_H
#define  XCGUI_BLUR_H
//@模块名称  炫彩界面库-亚克力高斯模糊
//@版本  3.0.0
//@日期  2026-05-15
//@作者  未闻花名
//@QQ    936599025
//@依赖  module_xcgui_class.h
//@模块备注  亚克力 / 磨砂玻璃元素. 走 DWM 系统合成路径 (HWND 级 acrylic 后
//          element 用 alpha 控制透出 backdrop blur), 跟手, 0 帧延迟, 无抓帧.
//
//          路线选择 (运行时按 OS 能力降级, 无需调用方关心):
//            1) Win10 1803+ / Win11    : SetWindowCompositionAttribute
//                                        ACCENT_ENABLE_ACRYLICBLURBEHIND
//            2) Win10 1607 ~ 1709      : ACCENT_ENABLE_BLURBEHIND (无 tint)
//            3) Win7 / Win8 / 8.1 / Vista: 退化为"仅装饰" (tint + border + 圆角).
//                                          XCGUI 渲染管线不保留 per-pixel alpha,
//                                          Win7 Aero BLURREGION 路径在实际场景下
//                                          不会出 blur, 故跟 Win8/8.1 一档处理.
//
//          重要权衡 (主动声明给调用者):
//            * 受 Windows 个性化 - 颜色 - 透明效果 开关影响. 用户关闭时
//              系统会自动把 acrylic 退化成纯色, 这是 OS 行为, 应用层无法覆盖.
//              WinUI3 / Office / Edge / VS Code 都受这个影响.
//            * DWM acrylic 是 HWND 级. CXBlur 元素 (Create/AttachToEle) 也会导致
//              它所在的整个窗口客户区开 acrylic. 这与 NTQQ / 微信 / 钉钉 等所有
//              主流 IM 一致.
//
//          支持 三种 绑定模式:
//            * Create        : 自有元素, 用户指定坐标. 启用所在窗口 acrylic.
//            * AttachToEle   : 接管已存在的元素. 启用所在窗口 acrylic.
//            * AttachToWnd   : 创建覆盖整个窗口客户区的子元素. 启用窗口 acrylic.
//          也支持 m_hEle = 用户元素 这种 IDE 式的句柄赋值 (operator=).
//@模块信息结束

// =================================================================
// 头文件依赖拓扑顺序说明:
//   - @依赖 是 IDE 解析器(智能感知/别名/语法着色)用的, 不会自动注入 #include 给 cl.exe;
//     所以下面要再用真实的 #include 链一次.
//   - d2d1.h 必须先于 module_xcgui.h 完成.
// =================================================================

#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <gdiplus.h>

#include <atomic>
#include <cstdint>

#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"

//@src "module_xcgui_blur.cpp"

// =================================================================
// 第三方依赖: GDI / GDI+ / D2D / User32 / DWM.
// =================================================================

//@lib "Gdiplus.lib"
//@lib "Gdi32.lib"
//@lib "User32.lib"
//@lib "D2d1.lib"
//@lib "Dxguid.lib"
//@lib "Dwmapi.lib"

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "D2d1.lib")
#pragma comment(lib, "Dxguid.lib")
#pragma comment(lib, "Dwmapi.lib")

//@隐藏{
class CXBlur;
//@隐藏}

///模糊效果主题预设 (CXBlur::SetTheme)
//@别名 模糊主题
enum xblur_theme_
{
	//@别名 模糊主题_自定义
	xblur_theme_custom    = 0,
	//@别名 模糊主题_浅色
	xblur_theme_light     = 1,
	//@别名 模糊主题_深色
	xblur_theme_dark      = 2,
	//@别名 模糊主题_跟随系统
	xblur_theme_auto      = 3,
};

///主题预设默认参数 (CXBlur::SetThemeDefault / GetThemeDefault)
///v3.0 DWM acrylic 路径下只保留 tint + noise (两个应用可控参数).
///blur 强度/饱和/亮度/对比度 都由 DWM 系统决定, 应用层不控。
//@别名 模糊主题默认参数
struct CXBlurThemeDefaults
{
	//@别名 叠加色 (0xAABBGGRR)
	COLORREF tintColor;
	//@别名 噪点
	float    noise;
};

///绑定模式 (CXBlur::GetBindMode 返回值)
//@别名 模糊绑定模式
enum xblur_bind_
{
	//@别名 模糊绑定_未绑定
	xblur_bind_none      = 0,
	//@别名 模糊绑定_自有元素
	xblur_bind_owned     = 1,
	//@别名 模糊绑定_附加元素
	xblur_bind_attach    = 2,
	//@别名 模糊绑定_附加窗口
	xblur_bind_window    = 3,
};

//@分组{ 亚克力模糊元素
//@备注  继承: CXEle, CXWidgetUI, CXObjectUI, CXBase. 走 Windows DWM 系统级 acrylic.
//@别名  炫彩亚克力模糊类
class CXBlur : public CXEle
{
public:
	//@隐藏{
	CXBlur();
	virtual ~CXBlur();
	virtual HXCGUI GetHXCGUI() override { return m_hEle; }
	operator HELE() const { return m_hEle; }
	operator HXCGUI(){ return m_hEle; }
	virtual void operator=(const HELE hEle) override;
	//@隐藏}

//@备注 创建模糊元素 (自有). 自动启用所在窗口的 DWM acrylic.
//@参数 x 元素x坐标 (相对父元素客户区).
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄.
//@别名  创建()
	HELE Create(int x, int y, int cx, int cy, HXCGUI hParent = NULL);

//@备注 创建模糊元素 (构造函数版本).
	CXBlur(int x, int y, int cx, int cy, HXCGUI hParent = NULL){ Create(x, y, cx, cy, hParent); }

//@备注 把模糊效果 *附加* 到一个已存在的元素. 启用所在窗口的 DWM acrylic.
//      副作用 (Detach 时还原):
//        1) 强制 XEle_EnableBkTransparent(hUserEle, TRUE).
//        2) 注册 XE_PAINT 自绘 tint+border+圆角.
//@参数 hUserEle 用户已创建好的元素句柄.
//@返回 TRUE 成功, FALSE 句柄非法.
//@别名  附加元素()
	BOOL AttachToEle(HELE hUserEle);

//@备注 创建一个 *覆盖整个窗口客户区* 的模糊背板, 作为窗口级亚克力背景使用.
//@参数 hWnd 目标窗口.
//@返回 TRUE 成功, FALSE 句柄非法.
//@别名  附加窗口()
	BOOL AttachToWnd(HWINDOW hWnd);

//@备注 解除当前绑定. 还原元素的 EnableBkTransparent 状态.
//      *不会* 主动关闭窗口的 DWM acrylic, 因为同一窗口可能有多个 CXBlur 共享, 也可能
//      用户希望 CXBlur 销毁后窗口仍保持 acrylic 视觉. 用户需要彻底关 acrylic
//      请直接销毁窗口.
//@别名  解除绑定()
	void Detach();

//@备注 当前绑定模式 (xblur_bind_*).
//@别名  取绑定模式()
	int  GetBindMode() const;

	// ===== 着色叠加 =====
//@备注 设置 *叠加色* (Tint). 直接传给 DWM acrylic 的 GradientColor.
//      XCGUI RGBA 0xAABBGGRR 与 ACCENT_POLICY.GradientColor 编码完全对齐, 直接透传.
//@别名  置叠加色()
	void SetTintColor(COLORREF color);
//@别名  取叠加色()
	COLORREF GetTintColor() const;

//@备注 应用主题预设 (xblur_theme_*) 到当前实例.
//@别名  置主题()
	void SetTheme(int theme);
//@别名  取主题()
	int  GetTheme() const;

//@备注 全局主题: 设置后会同步到当前所有 CXBlur 实例, 之后新创建的实例
//      默认也使用此主题. 适用于"整个应用统一 acrylic 风格"的场景.
//      个别 element 仍可通过 SetTheme(...) 单独 override.
//@别名  置全局主题()
	static void SetGlobalTheme(int theme);
//@别名  取全局主题()
	static int  GetGlobalTheme();

//@备注 修改 light/dark 主题预设的默认参数. 后续 SetTheme(light/dark) 会用
//      你设的值而非硬编码默认. 用于"调好参数后保存, 避免每次启动重设".
//      theme 只接受 xblur_theme_light 或 xblur_theme_dark (其他忽略).
//      auto 主题运行时根据系统设置选 light/dark 调用此值.
//@别名  置主题默认参数()
	static void SetThemeDefault(int theme, const CXBlurThemeDefaults& d);

//@备注 读取 light/dark 主题预设当前默认参数. 配合 SetThemeDefault 可"读出
//      → 改单字段 → 写回"实现细粒度调整.
//@别名  取主题默认参数()
	static CXBlurThemeDefaults GetThemeDefault(int theme);

//@备注 设置噪点强度. 增加 acrylic 砂质感, 缓解纯色块感.
//      0 = 关闭, 1 = 满, 推荐 0 ~ 0.15. 默认 0.06.
//@别名  置噪点()
	void SetNoise(float amount);
//@别名  取噪点()
	float GetNoise() const;

//@备注 当前运行环境是否支持系统级 acrylic / blur (Vista+ 任意一档即视为支持).
//      不支持时 (Win8/8.1) CXBlur 仅画 tint+border+圆角.
//@别名  是否支持系统亚克力()
	static BOOL IsSystemAcrylicSupported();

//@备注 当前 CXBlur 实例的系统 acrylic / blur 是否成功启用.
//@别名  是否已启用系统亚克力()
	BOOL IsSystemAcrylicEnabled() const;

//@备注 强制开启 / 关闭 *系统* 透明效果 (写 HKCU 注册表 EnableTransparency).
//      让 ACCENT_ACRYLIC 在用户关了"个性化-颜色-透明效果"时仍能出真 blur.
//
//      bForce = TRUE  → 保存老值, 写入 1, 广播本进程窗口, 注册退出还原.
//      bForce = FALSE → 还原老值, 广播本进程窗口 (若未启用则 no-op).
//
//      *重要权衡*:
//        * 这是 *用户级* 系统设置, 写下去整个用户帐户的所有 app 都开.
//          务必在 EULA / 启动提示里告知用户.
//        * Win7 / Win8 / 8.1 上没用 (它们没有 ACCENT_ACRYLIC).
//        * 进程正常退出 atexit / Ctrl-C / Ctrl-Break / Close 会自动还原;
//          被强杀 (任务管理器 / TerminateProcess / 蓝屏) 时还原不上.
//        * 老值已经是 1 时不写也不还原 (用户本来就开着).
//        * 重复调用幂等: 第一次 TRUE 真正写入并注册 atexit, 后续 TRUE 直接返回;
//          FALSE 仅在已 forced 时执行还原, 否则 no-op.
//        * 广播只发本进程顶层窗口 (避免打扰其他 app).
//        * 不需要管理员权限 (HKCU 当前用户就能写).
//
//      调用时机: 建议在 XInitXCGUI 之后, 创建第一个 CXBlur 之前.
//@参数 bForce  TRUE=开启, FALSE=还原. 默认 FALSE.
//@别名  强制开启系统透明效果()
	static void ForceSystemTransparencyOn(BOOL bForce = FALSE);

//@别名  立即刷新()
	void Invalidate();

	// ===== 圆角 / 边框 =====
//@备注 统一圆角半径. 四个角设为同一值. 单位 = 元素逻辑像素.
//@别名  置圆角()
	void SetCornerRadius(int radius);
//@备注 返回左上角圆角半径 (当四角不一致时返回左上角的值).
//@别名  取圆角()
	int  GetCornerRadius() const;

//@备注 分别设置四个角的圆角半径. 顺序按 CSS border-radius 标准:
//      左上 → 右上 → 右下 → 左下 (顺时针). 单位 = 元素逻辑像素.
//      传 0 表示该角不倒, 可与圆角混合实现"半圆角矩形" / 上圆下方 等效果.
//@参数 leftTop      左上角半径 (CSS 第 1 位)
//@参数 rightTop     右上角半径 (CSS 第 2 位)
//@参数 rightBottom  右下角半径 (CSS 第 3 位)
//@参数 leftBottom   左下角半径 (CSS 第 4 位)
//@别名  置圆角扩展()
	void SetCornerRadiusEx(int leftTop, int rightTop, int rightBottom, int leftBottom);

//@备注 读出四个角圆角半径.
//@别名  取圆角扩展()
	void GetCornerRadiusEx(int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom) const;

//@别名  置边框色()
	void SetBorderColor(COLORREF color);
//@别名  取边框色()
	COLORREF GetBorderColor() const;

//@别名  置边框宽()
	void SetBorderWidth(float w);
//@别名  取边框宽()
	float GetBorderWidth() const;

	//@隐藏{
private:
	// ===== 元素 / DPI =====
	bool   m_owned              = false;
	bool   m_attachToWindow     = false;
	HWINDOW m_attachedWnd       = NULL;
	bool   m_savedBkTransparent  = false;
	bool   m_hasSavedTransparent = false;
	float  m_dpiScale           = 1.0f;

	// ===== 模糊参数 (DWM 路径只控装饰层) =====
	std::atomic<COLORREF> m_tintColor    {0};
	std::atomic<int>      m_theme        {xblur_theme_custom};
	std::atomic<float>    m_noise        {0.06f};
	// 四角圆角半径: TL / TR / BR / BL (顺时针). 四值相等时走快路径
	// FillRoundedRectangle, 否则用 path geometry 拼 4 段弧.
	std::atomic<int>      m_cornerTL     {0};
	std::atomic<int>      m_cornerTR     {0};
	std::atomic<int>      m_cornerBR     {0};
	std::atomic<int>      m_cornerBL     {0};
	std::atomic<COLORREF> m_borderColor  {0};
	std::atomic<float>    m_borderWidth  {0.0f};

	// ===== host acrylic 状态 =====
	bool  m_acrylicApplied = false;   // host acrylic 是否成功启用
	HWND  m_hostHwnd       = NULL;    // host HWND (acrylic 生效的窗口)

public:
	// =================================================================
	// 内部回调入口 (公开仅为方便 C 风格事件回调调用; 用户代码请勿直接调用).
	// =================================================================
	int  OnPaintImpl   (HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int  OnSizeImpl    (HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled);
	int  OnDestroyImpl (HELE hEle, BOOL* pbHandled);
	int  OnWndSizeImpl (HWINDOW hWnd, UINT nFlags, SIZE* pSize, BOOL* pbHandled);
	int  OnWndSettingChangeImpl(HWINDOW hWnd, UINT uFlags, void* pStr, BOOL* pbHandled);

private:
	// =================================================================
	// 内部方法
	// =================================================================
	void AttachInternal(HELE hEle, bool owned);
	void DetachInternal();
	void HookEvents(HELE hEle);
	void UnhookEvents(HELE hEle);
	void RegisterWindowSizeHook(HWINDOW hWnd);
	void UnregisterWindowSizeHook();

	// 找到本元素所在的顶层 HWND.
	HWND FindHostHwnd() const;

	// 渲染分流: 只画 tint + noise + border + 圆角. host acrylic 背景由 DWM 提供.
	void OnPaintD2D(ID2D1RenderTarget* rt, HDRAW hDraw);
	void OnPaintGdi(HDC hdc, HDRAW hDraw);

	void RefreshDpiScale();
	void RedrawSelf();

	void ApplyThemePreset(int theme);
	static BOOL  IsSystemDarkMode();
	//@隐藏}
};
//@分组}

#endif // XCGUI_BLUR_H
