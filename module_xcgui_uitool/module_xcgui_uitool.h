#ifndef  XCGUI_UITOOL_H
#define  XCGUI_UITOOL_H
//@模块名称  炫彩界面库-UI工具集
//@版本  1.0.0
//@日期  2026-05-22
//@作者  未闻花名
//@QQ    936599025
//@依赖  module_xcgui_class.h
//@模块备注  通用 UI 辅助工具集合, 模块文件名 *uitool* 仅作工程标记, 后续会持续加入
//          多个 *顶层* 工具类 (Toast / Popover / HUD / Menu ...) — 每个独立 class,
//          XCGUI 别名工具会逐个识别. 不再用嵌套 class 形式 (扫描器不识别).
//          首发: CXTooltip (鼠标悬停气泡提示) —
//            - 全局静态注册表 + 共享气泡窗口, 同一时刻至多 1 个气泡显示.
//            - 支持 普通 / 成功 / 信息 / 警告 / 错误 5 种语义 (后 4 个用内置 SVG 图标).
//            - 支持 深色 / 浅色 / 自定义 / 跟随系统 4 套主题.
//            - 显示/隐藏带 alpha 渐显渐隐过渡, 鼠标停留延迟 + 自动关闭时间均可配置.
//            - 弹出窗口 鼠标穿透, 不抢焦点, 不干扰用户操作.
//            - 自动检测鼠标从源元素哪条边进入, 绘制对应方向的三角指针 (指向源).
//@模块信息结束

// =================================================================
// 头文件依赖拓扑顺序 (照规范, 不要改顺序)
// =================================================================
#include <d2d1.h>

// 标准库
#include <string>
#include <unordered_map>

// XCGUI 自身按拓扑顺序
#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"

//@src "module_xcgui_uitool.cpp"

//@lib "User32.lib"
//@lib "Gdi32.lib"
//@lib "Advapi32.lib"
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Advapi32.lib")

//@隐藏{
class CXTooltip;
//@隐藏}

///<Tooltip 类型: 决定左侧图标 (default = 无图标, 1~4 = 对应 SVG)
//@别名 提示类型
enum xtooltip_type_
{
	//@别名 提示类型_默认
	xtooltip_type_default  = 0,   ///<仅文本, 无图标
	//@别名 提示类型_成功
	xtooltip_type_success  = 1,   ///<绿色对勾 (1.svg)
	//@别名 提示类型_信息
	xtooltip_type_info     = 2,   ///<蓝色感叹 (2.svg)
	//@别名 提示类型_警告
	xtooltip_type_warning  = 3,   ///<橙色三角 (3.svg)
	//@别名 提示类型_错误
	xtooltip_type_error    = 4,   ///<红色叉 (4.svg)
};

///<Tooltip 颜色主题
//@别名 提示主题
enum xtooltip_theme_
{
	//@别名 提示主题_深色
	xtooltip_theme_dark    = 0,   ///<文本 #F5F5F5 / 背景 #171717 (默认)
	//@别名 提示主题_浅色
	xtooltip_theme_light   = 1,   ///<文本 #171717 / 背景 #FFFFFF
	//@别名 提示主题_自定义
	xtooltip_theme_custom  = 2,   ///<使用 SetTextColor / SetBkColor 提供的颜色
	//@别名 提示主题_自动
	xtooltip_theme_auto    = 3,   ///<根据系统 light/dark 模式自动选择
};

///<水平对齐方式 (单行默认 center, 多行默认 left)
//@别名 提示水平对齐
enum xtooltip_align_h_
{
	//@别名 提示水平对齐_左
	xtooltip_align_h_left   = 0,
	//@别名 提示水平对齐_中
	xtooltip_align_h_center = 1,
	//@别名 提示水平对齐_右
	xtooltip_align_h_right  = 2,
};

///<垂直对齐方式 (单行默认 center, 多行默认 top)
//@别名 提示垂直对齐
enum xtooltip_align_v_
{
	//@别名 提示垂直对齐_上
	xtooltip_align_v_top    = 0,
	//@别名 提示垂直对齐_中
	xtooltip_align_v_center = 1,
	//@别名 提示垂直对齐_下
	xtooltip_align_v_bottom = 2,
};

//@分组{ 提示气泡
//@备注  全局气泡提示工具类, 全静态方法, 不需实例化. 注册的 HELE 在销毁时会被
//       自动清理. 共享 1 个气泡窗口, 同一时刻至多显示 1 个气泡.
//       调用形如: CXTooltip::AddEleTip(hEle, L"提示文本");
//@别名  炫彩气泡提示类
class CXTooltip
{
public:

	// ===== 注册 / 注销 =====

//@备注 为元素添加 (或更新) 气泡提示. 内部使用静态注册表 -> 同一 hEle 重复
//      调用相当于修改文本; 首次调用会挂上鼠标进入 / 移动 / 离开 / 销毁事件.
//@参数 hEle 元素句柄 (必须 XC_IsHELE 验证通过)
//@参数 pText 提示文本, NULL 视同空串
//@返回 成功 TRUE, 元素无效或内部失败 FALSE
//@别名  添加元素提示()
	static BOOL AddEleTip(HELE hEle, const wchar_t* pText);

//@备注 移除元素的气泡提示. 已挂的元素事件保持挂载 (XCGUI 暂无 C1 版 RemoveEvent),
//      回调内部会先 find registry, 找不到直接返回 — 安全无副作用. 元素 destroy
//      时 XCGUI 自动清.
//@参数 hEle 元素句柄
//@返回 移除成功 TRUE, 未注册或元素无效 FALSE
//@别名  移除元素提示()
	static BOOL DelEleTip(HELE hEle);

//@备注 判断元素是否已注册气泡提示.
//@参数 hEle 元素句柄
//@返回 已注册 TRUE
//@别名  是否存在提示()
	static BOOL HasTip(HELE hEle);

	// ===== 文本 =====

//@备注 单独修改提示文本 (等同 AddEleTip).
//@参数 hEle 元素句柄
//@参数 pText 文本
//@返回 成功 TRUE
//@别名  置文本()
	static BOOL SetText(HELE hEle, const wchar_t* pText);

//@备注 获取提示文本. 返回的指针在 hEle 注销或文本更新前有效, 跨调用不要长缓存.
//@参数 hEle 元素句柄
//@返回 文本指针, 未注册返 NULL
//@别名  取文本()
	static const wchar_t* GetText(HELE hEle);

	// ===== 类型 (default / success / info / warning / error) =====

//@备注 设置提示类型, 决定左侧图标. 默认 xtooltip_type_default (无图标).
//@参数 hEle 元素句柄
//@参数 type 类型
//@返回 成功 TRUE
//@别名  置类型()
	static BOOL SetType(HELE hEle, xtooltip_type_ type);

//@备注 取提示类型.
//@参数 hEle 元素句柄
//@返回 类型枚举, 未注册返 xtooltip_type_default
//@别名  取类型()
	static xtooltip_type_ GetType(HELE hEle);

	// ===== 多行 / 单行 =====

//@备注 设置多行模式. TRUE: 文本自动换行, 默认上对齐+左对齐;
//      FALSE: 单行, 默认水平垂直居中.
//@参数 hEle 元素句柄
//@参数 bMultiline 是否多行
//@返回 成功 TRUE
//@别名  置多行()
	static BOOL SetMultiline(HELE hEle, BOOL bMultiline);

//@备注 是否多行模式.
//@参数 hEle 元素句柄
//@返回 多行 TRUE
//@别名  是否多行()
	static BOOL IsMultiline(HELE hEle);

	// ===== 对齐 =====

//@备注 设置水平对齐. 默认 单行 center / 多行 left.
//@参数 hEle 元素句柄
//@参数 align 水平对齐枚举
//@返回 成功 TRUE
//@别名  置水平对齐()
	static BOOL SetAlignH(HELE hEle, xtooltip_align_h_ align);

//@备注 取水平对齐.
//@参数 hEle 元素句柄
//@返回 水平对齐枚举
//@别名  取水平对齐()
	static xtooltip_align_h_ GetAlignH(HELE hEle);

//@备注 设置垂直对齐. 默认 单行 center / 多行 top.
//@参数 hEle 元素句柄
//@参数 align 垂直对齐枚举
//@返回 成功 TRUE
//@别名  置垂直对齐()
	static BOOL SetAlignV(HELE hEle, xtooltip_align_v_ align);

//@备注 取垂直对齐.
//@参数 hEle 元素句柄
//@返回 垂直对齐枚举
//@别名  取垂直对齐()
	static xtooltip_align_v_ GetAlignV(HELE hEle);

	// ===== 主题 / 颜色 =====

//@备注 设置主题. dark / light / custom / auto(跟随系统).
//@参数 hEle 元素句柄
//@参数 theme 主题枚举
//@返回 成功 TRUE
//@别名  置主题()
	static BOOL SetTheme(HELE hEle, xtooltip_theme_ theme);

//@备注 取主题.
//@参数 hEle 元素句柄
//@返回 主题枚举
//@别名  取主题()
	static xtooltip_theme_ GetTheme(HELE hEle);

//@备注 设置自定义文本颜色. 仅在 theme = custom 时生效, 同时会把 theme 切到 custom.
//      COLORREF 走 XCGUI 的 RGBA(ARGB) 格式, alpha 必须 255 否则会被 GDI 拒识.
//@参数 hEle 元素句柄
//@参数 color 文本颜色
//@返回 成功 TRUE
//@别名  置文本颜色()
	static BOOL SetTextColor(HELE hEle, COLORREF color);

//@备注 取文本颜色 (custom 主题下) 或主题对应的预设值.
//@参数 hEle 元素句柄
//@返回 COLORREF (ARGB)
//@别名  取文本颜色()
	static COLORREF GetTextColor(HELE hEle);

//@备注 设置自定义背景颜色. 仅在 theme = custom 时生效, 同时会把 theme 切到 custom.
//@参数 hEle 元素句柄
//@参数 color 背景颜色
//@返回 成功 TRUE
//@别名  置背景颜色()
	static BOOL SetBkColor(HELE hEle, COLORREF color);

//@备注 取背景颜色 (custom 主题下) 或主题对应的预设值.
//@参数 hEle 元素句柄
//@返回 COLORREF (ARGB)
//@别名  取背景颜色()
	static COLORREF GetBkColor(HELE hEle);

	// ===== 边距 (内填充) =====

//@备注 设置内填充边距 (文本到气泡边缘的距离). 默认 16/10/16/10.
//@参数 hEle 元素句柄
//@参数 l 左 (逻辑像素)
//@参数 t 上
//@参数 r 右
//@参数 b 下
//@返回 成功 TRUE
//@别名  置边距()
	static BOOL SetMargin(HELE hEle, int l, int t, int r, int b);

//@备注 取内填充边距. pl/pt/pr/pb 可为 NULL 忽略.
//@参数 hEle 元素句柄
//@参数 pl 接收 左
//@参数 pt 接收 上
//@参数 pr 接收 右
//@参数 pb 接收 下
//@别名  取边距()
	static void GetMargin(HELE hEle, int* pl, int* pt, int* pr, int* pb);

	// ===== 显示延迟 / 自动关闭 =====

//@备注 设置鼠标停留多少毫秒后才显示气泡 (防止快速划过抖动). 默认 500ms.
//@参数 hEle 元素句柄
//@参数 ms 延迟毫秒数 (>=0)
//@返回 成功 TRUE
//@别名  置显示延迟()
	static BOOL SetShowDelay(HELE hEle, int ms);

//@备注 取显示延迟.
//@参数 hEle 元素句柄
//@返回 毫秒数
//@别名  取显示延迟()
	static int GetShowDelay(HELE hEle);

//@备注 设置气泡显示后多少毫秒自动关闭 (用户鼠标仍停留时也会关). 0 = 不自动关闭,
//      由鼠标离开触发关闭. 默认 0.
//@参数 hEle 元素句柄
//@参数 ms 毫秒数 (>=0)
//@返回 成功 TRUE
//@别名  置自动关闭时间()
	static BOOL SetAutoCloseTime(HELE hEle, int ms);

//@备注 取自动关闭时间.
//@参数 hEle 元素句柄
//@返回 毫秒数, 0 = 不自动关闭
//@别名  取自动关闭时间()
	static int GetAutoCloseTime(HELE hEle);

//@备注 设置渐显渐隐动画持续时长 (单边). 默认 150ms. 0 = 关闭动画立即显示/隐藏.
//@参数 hEle 元素句柄
//@参数 ms 毫秒数
//@返回 成功 TRUE
//@别名  置渐变时长()
	static BOOL SetFadeDuration(HELE hEle, int ms);

//@备注 取渐变时长.
//@参数 hEle 元素句柄
//@返回 毫秒数
//@别名  取渐变时长()
	static int GetFadeDuration(HELE hEle);

	// ===== 全局 =====

//@备注 全局清理: 销毁共享气泡窗口 + 释放 SVG 句柄 + 清空注册表.
//      一般 XExitXCGUI() 之前 (或主窗口销毁后) 调一次. 进程退出时 OS 也会回收,
//      但显式调用更稳妥, 避免 XCGUI 卸载顺序异常弹错框.
//@别名  清理()
	static void Cleanup();
};
//@分组}

#endif // XCGUI_UITOOL_H
