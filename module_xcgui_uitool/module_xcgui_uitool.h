#ifndef  XCGUI_UITOOL_H
#define  XCGUI_UITOOL_H
//@模块名称  炫彩界面库UI工具集
//@版本  1.0.0
//@日期  2026-05-22
//@作者  未闻花名
//@QQ    936599025
//@依赖  module_xcgui_class.h
//@模块备注  通用 UI 辅助工具集合, 模块文件名 *uitool* 仅作工程标记, 后续会持续加入
//          多个 *顶层* 工具类 (Toast / Popover / HUD / Menu ...) — 每个独立 class,
//          XCGUI 别名工具会逐个识别. 不再用嵌套 class 形式 (扫描器不识别).
//
//          已实现的顶层类:
//          模块内主题: CXTooltip / CXLoading / CXCalendarCard 共用 xuitool_theme_ 与
//            _XUITool 主题层 (深/浅/自定义/跟随系统), 预设色与系统检测只维护一份.
//          [1] CXTooltip — 鼠标悬停气泡提示
//            - 全局静态注册表 + 共享气泡窗口, 同一时刻至多 1 个气泡显示.
//            - 支持 普通 / 成功 / 信息 / 警告 / 错误 5 种语义 (后 4 个用内置 SVG 图标).
//            - 主题使用 xuitool_theme_ (深/浅/自定义/跟随系统).
//            - 显示/隐藏带 alpha 渐显渐隐过渡, 鼠标停留延迟 + 自动关闭时间均可配置.
//            - 弹出窗口 鼠标穿透, 不抢焦点, 不干扰用户操作.
//            - 自动检测鼠标从源元素哪条边进入, 绘制对应方向的三角指针 (指向源).
//
//          [2] CXLoading — 加载动画 (loading 指示器)
//            - 3 种宿主形态: 元素附加 / 窗口附加 / 自建元素.
//            - 5 种动画风格 (spinner 圆环 / dots 跳动点 / spokes 时钟辐条 /
//              pulse 脉冲圈 / bars 频谱条).
//            - 主题使用 xuitool_theme_; 强调色默认: 深色=#FFFFFF / 浅色=#171717.
//            - 居中显示, 默认 40x40, 可配, 可设单行文本 (用户等待小贴士).
//            - 背景圆角支持统一/4 角分别 (扩展版本, CSS 顺序).
//            - 多种 ease (cubic-in-out / cubic-out / sine) 衍生自 CSS 标准.
//            - 元素/窗口销毁时自动释放, 共享 timer 心跳.
//
//          [3] CXCalendarCard — 日期 / 日期范围选择卡片
//            - PopupSingle 单个月历选择日期; PopupDouble 双个月历选择范围.
//            - 主题使用 xuitool_theme_ (推荐 dark / light / auto).
//            - 默认限制最大可选日期为今天, 也可指定最大日期或关闭限制.
//            - 双月范围支持今天 / 近7天 / 近15天 / 近30天快捷选择.
//            - 弹窗使用 window_transparent_shaped, 自绘圆角背景与柔和阴影.
//          [4] CXShadow — Win11 风格窗口外阴影 + 圆角描边 (原 shadow 模块)
//          [5] CXEditDW — DirectWrite 彩色 emoji 编辑框 (原 editdw 模块)
//          [6] CXBlur — DWM 亚克力 / 磨砂玻璃虚化 (原 blur 模块)
//          [7] CXChatBubbleBox — IM 聊天气泡富文本对话框 (原 chat 模块)
//          [8] CXAccordion — 折叠面板 (FAQ / 设置分组 / 引导清单)
//@模块信息结束
// =================================================================
// 头文件依赖拓扑顺序说明:
//   - @依赖 是 IDE 解析器(智能感知/别名/语法着色)用的, 不会自动注入 #include 给 cl.exe;
//     所以下面要再用真实的 #include 链一次.
//   - d2d1.h 必须先于 module_xcgui.h 完成.
// =================================================================
#include <d2d1.h>
#include <d2d1helper.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <dwrite.h>
#include <gdiplus.h>
#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

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
#pragma comment(lib, "D2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Dxguid.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "D3d11.lib")
#pragma comment(lib, "Dxgi.lib")
#pragma comment(lib, "DComp.lib")
#pragma comment(lib, "WindowsApp.lib")
#pragma comment(lib, "CoreMessaging.lib")

//@隐藏{
class CXTooltip;
class CXLoading;
class CXCalendarCard;
class CXShadow;
class CXEditDW;
class CXBlur;
class CXChatBubbleBox;
class CXAccordion;
//@隐藏}

///<UI 工具集颜色主题 (CXTooltip / CXLoading / CXCalendarCard 共用)
//@别名 UI工具主题
enum xuitool_theme_
{
	//@别名 UI工具主题_深色
	xuitool_theme_dark   = 0,   ///<文本 #F5F5F5 / 背景 #171717 / 强调 #FFFFFF (默认)
	//@别名 UI工具主题_浅色
	xuitool_theme_light  = 1,   ///<文本 #171717 / 背景 #FFFFFF / 强调 #171717
	//@别名 UI工具主题_自定义
	xuitool_theme_custom = 2,   ///<使用 SetTextColor / SetBkColor / SetAccentColor 等自定义颜色
	//@别名 UI工具主题_自动
	xuitool_theme_auto   = 3,   ///<跟随系统 light/dark 模式自动选择
};

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

///<气泡三角箭头方向 — 决定气泡相对源元素出现在哪一侧, 以及三角顶点的指向.
///默认 auto: 按鼠标进入元素时离哪条边最近, 自动选反向 (鼠标从左进 → 气泡在源右侧).
///4 个固定方向用于强制锚定 (例如锚到工具栏按钮总是在下方弹出).
//@别名 提示箭头方向
enum xtooltip_arrow_side_
{
	//@别名 提示箭头方向_自动
	xtooltip_arrow_side_auto   = 0,   ///<默认: 跟随鼠标进入边自动判断
	//@别名 提示箭头方向_左
	xtooltip_arrow_side_left   = 1,   ///<箭头在气泡左侧 → 气泡在源元素*右*方
	//@别名 提示箭头方向_右
	xtooltip_arrow_side_right  = 2,   ///<箭头在气泡右侧 → 气泡在源元素*左*方
	//@别名 提示箭头方向_上
	xtooltip_arrow_side_top    = 3,   ///<箭头在气泡上侧 → 气泡在源元素*下*方
	//@别名 提示箭头方向_下
	xtooltip_arrow_side_bottom = 4,   ///<箭头在气泡下侧 → 气泡在源元素*上*方
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

//@备注 移除元素的气泡提示, 同步反挂 5 个源事件 (XEle_RemoveEventC 对 C/C1 通用).
//      如果该元素正显示气泡, 会先立刻隐藏. 元素 destroy 时 XCGUI 自动清, 提前
//      调本接口可在元素生命周期内动态启停 tooltip.
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

	// ===== 箭头方向 (强制锚定 - 默认 auto 跟随鼠标进入边) =====

//@备注 设置气泡三角箭头方向. 默认 auto = 按鼠标进入元素的边自动判断 (现状行为).
//      传 left/right/top/bottom 强制锚定: 气泡永远出现在源元素的反方向那一侧.
//      典型场景: 工具栏按钮固定下方弹气泡, 输入框始终在右侧提示等.
//@参数 hEle 元素句柄
//@参数 side 箭头方向枚举
//@返回 成功 TRUE
//@别名  置箭头方向()
	static BOOL SetArrowSide(HELE hEle, xtooltip_arrow_side_ side);

//@备注 取箭头方向. 未设置返 xtooltip_arrow_side_auto.
//@参数 hEle 元素句柄
//@返回 箭头方向枚举
//@别名  取箭头方向()
	static xtooltip_arrow_side_ GetArrowSide(HELE hEle);

//@备注 设置是否显示三角箭头. 默认 TRUE = 显示. 关闭后气泡仍按 ArrowSide 决定的
//      方位出现, 仅不绘制三角且不为三角预留外侧空间.
//@参数 hEle 元素句柄
//@参数 bShow 是否显示三角
//@返回 成功 TRUE
//@别名  置显示箭头()
	static BOOL SetShowArrow(HELE hEle, BOOL bShow);

//@备注 取是否显示三角箭头.
//@参数 hEle 元素句柄
//@返回 显示 TRUE
//@别名  取显示箭头()
	static BOOL GetShowArrow(HELE hEle);

	// ===== 主题 / 颜色 =====

//@备注 设置主题. xuitool_theme_dark / light / custom / auto(跟随系统).
//@参数 hEle 元素句柄
//@参数 theme UI工具主题枚举
//@返回 成功 TRUE
//@别名  置主题()
	static BOOL SetTheme(HELE hEle, xuitool_theme_ theme);

//@备注 取主题.
//@参数 hEle 元素句柄
//@返回 UI工具主题枚举
//@别名  取主题()
	static xuitool_theme_ GetTheme(HELE hEle);

//@备注 设置自定义文本颜色. 仅在 theme = xuitool_theme_custom 时生效, 同时会把 theme 切到 custom.
//      COLORREF 走 XCGUI 的 RGBA(ARGB) 格式, alpha 必须 255 否则会被 GDI 拒识.
//@参数 hEle 元素句柄
//@参数 color 文本颜色
//@返回 成功 TRUE
//@别名  置文本颜色()
	static BOOL SetTextColor(HELE hEle, COLORREF color);

//@备注 取文本颜色 (xuitool_theme_custom 下) 或主题对应的预设值.
//@参数 hEle 元素句柄
//@返回 COLORREF (ARGB)
//@别名  取文本颜色()
	static COLORREF GetTextColor(HELE hEle);

//@备注 设置自定义背景颜色. 仅在 theme = xuitool_theme_custom 时生效, 同时会把 theme 切到 custom.
//@参数 hEle 元素句柄
//@参数 color 背景颜色
//@返回 成功 TRUE
//@别名  置背景颜色()
	static BOOL SetBkColor(HELE hEle, COLORREF color);

//@备注 取背景颜色 (xuitool_theme_custom 下) 或主题对应的预设值.
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

//@备注 设置鼠标停留多少毫秒后才显示气泡 (防止快速划过抖动). 默认 0 = 立即显示.
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


// =====================================================================
// CXLoading — 加载动画指示器
// =====================================================================

///<Loading 动画风格 (5 种, 设计师手挑)
//@别名 加载样式
enum xloading_style_
{
	//@别名 加载样式_圆环
	xloading_style_spinner  = 0,   ///<无缝 indeterminate 圆环 (grow/shrink snake, 1.5s 单周期), 默认
	//@别名 加载样式_跳点
	xloading_style_dots     = 1,   ///<3 个上下跳动的点, 正弦缓动
	//@别名 加载样式_辐条
	xloading_style_spokes   = 2,   ///<12 条辐条循环淡出 (传统 Mac Beachball 风)
	//@别名 加载样式_脉冲
	xloading_style_pulse    = 3,   ///<2 圈同心扩散 + 透明度衰减, ease-out
	//@别名 加载样式_频谱
	xloading_style_bars     = 4,   ///<5 根竖条上下变高 (音频电平计风格)
};

//@分组{ 加载动画
//@备注  全局加载动画工具类, 全静态方法. 支持以 3 种方式宿主:
//         1) AttachEle  — 在已有元素上以 XE_PAINT 接管绘制 (loading 期间元素原内容
//            被 loading 覆盖; Stop 后恢复)
//         2) AttachWnd  — 同上, 但宿主是窗口 (整窗 loading 蒙层)
//         3) Create     — 新建独立 loading 元素, 由用户布局
//       元素/窗口销毁时自动释放. 共享 16ms 心跳 timer 驱动所有动画.
//@别名  炫彩加载动画类
class CXLoading
{
public:

	// ===== 创建 / 附加 / 解除 =====

//@备注 自建独立加载动画元素 (新建 HELE, 由调用方布局). 默认 启动 = TRUE.
//@参数 x 元素 x 坐标 (相对父容器)
//@参数 y 元素 y 坐标
//@参数 cx 元素宽度
//@参数 cy 元素高度
//@参数 hParent 父容器 (HXCGUI: HELE 或 HWINDOW)
//@返回 新建的元素句柄, 失败返回 NULL
//@别名  创建()
	static HELE Create(int x, int y, int cx, int cy, HXCGUI hParent);

//@备注 附加到已有元素. 内部启用该元素 XE_PAINT 接管 — loading 运行期间
//      元素内容被 loading 覆盖; Stop 或 Detach 后让出 paint, 恢复元素原渲染.
//@参数 hEle 宿主元素句柄
//@返回 成功 TRUE
//@别名  附加元素()
	static BOOL AttachEle(HELE hEle);

//@备注 附加到窗口 (整窗 loading 蒙层). 同上.
//@参数 hWnd 宿主窗口句柄
//@返回 成功 TRUE
//@别名  附加窗口()
	static BOOL AttachWnd(HWINDOW hWnd);

//@备注 解除附加 / 销毁注册项. 已挂的事件保持挂载 (XCGUI 暂无 C1 版 Remove),
//      回调里若注册表查不到自动 *pbHandled = FALSE 让出渲染.
//@参数 hHost 宿主 (HELE 或 HWINDOW)
//@返回 成功 TRUE
//@别名  解除()
	static BOOL Detach(HXCGUI hHost);

//@备注 是否已注册 loading.
//@参数 hHost 宿主
//@返回 已注册 TRUE
//@别名  是否已附加()
	static BOOL HasAttached(HXCGUI hHost);

	// ===== 启动 / 停止 =====

//@备注 启动动画 (开心跳 timer + 立即重绘).
//@参数 hHost 宿主
//@返回 成功 TRUE
//@别名  启动()
	static BOOL Start(HXCGUI hHost);

//@备注 停止动画 (停心跳 + 让出 paint, 元素恢复原渲染).
//@参数 hHost 宿主
//@返回 成功 TRUE
//@别名  停止()
	static BOOL Stop(HXCGUI hHost);

//@备注 是否正在运行.
//@参数 hHost 宿主
//@返回 运行中 TRUE
//@别名  是否运行中()
	static BOOL IsRunning(HXCGUI hHost);

	// ===== 风格 =====

//@备注 设置动画风格. 默认 spinner.
//@参数 hHost 宿主
//@参数 style 风格枚举
//@返回 成功 TRUE
//@别名  置样式()
	static BOOL SetStyle(HXCGUI hHost, xloading_style_ style);

//@备注 取风格.
//@参数 hHost 宿主
//@返回 风格枚举
//@别名  取样式()
	static xloading_style_ GetStyle(HXCGUI hHost);

	// ===== 尺寸 (动画绘制区, 居中) =====

//@备注 设置动画绘制尺寸 (在宿主中央). 默认 40x40.
//@参数 hHost 宿主
//@参数 cx 宽度 (逻辑像素, >=8)
//@参数 cy 高度 (逻辑像素, >=8)
//@返回 成功 TRUE
//@别名  置尺寸()
	static BOOL SetSize(HXCGUI hHost, int cx, int cy);

//@备注 取动画绘制尺寸.
//@参数 hHost 宿主
//@参数 pcx 接收宽 (可 NULL)
//@参数 pcy 接收高 (可 NULL)
//@别名  取尺寸()
	static void GetSize(HXCGUI hHost, int* pcx, int* pcy);

	// ===== 文本 (单行小贴士) =====

//@备注 设置 loading 期间显示的单行文本 (在动画下方居中). 实时可改, 用于
//      "正在等待 / 小贴士" 类提示. 默认空 (不显示文本).
//@参数 hHost 宿主
//@参数 pText 文本, NULL/空 = 不显示文本
//@返回 成功 TRUE
//@别名  置文本()
	static BOOL SetText(HXCGUI hHost, const wchar_t* pText);

//@备注 取文本.
//@参数 hHost 宿主
//@返回 文本指针, 未注册返 NULL
//@别名  取文本()
	static const wchar_t* GetText(HXCGUI hHost);

//@备注 设置文本字号 (磅, pt). 默认 9. 范围 [6, 72]. 改字号会重建该宿主的 HFONTX.
//@参数 hHost 宿主
//@参数 pt 字号 (磅)
//@返回 成功 TRUE
//@别名  置字号()
	static BOOL SetFontSize(HXCGUI hHost, int pt);

//@备注 取文本字号 (磅).
//@参数 hHost 宿主
//@返回 字号
//@别名  取字号()
	static int GetFontSize(HXCGUI hHost);

	// ===== 主题 / 颜色 =====

//@备注 设置主题. xuitool_theme_dark / light / custom / auto(跟随系统). 默认 dark.
//@参数 hHost 宿主
//@参数 theme UI工具主题枚举
//@返回 成功 TRUE
//@别名  置主题()
	static BOOL SetTheme(HXCGUI hHost, xuitool_theme_ theme);

//@备注 取主题.
//@参数 hHost 宿主
//@返回 UI工具主题枚举
//@别名  取主题()
	static xuitool_theme_ GetTheme(HXCGUI hHost);

//@备注 自定义文本颜色 (会切到 xuitool_theme_custom).
//@参数 hHost 宿主
//@参数 color XCGUI ARGB
//@返回 成功 TRUE
//@别名  置文本颜色()
	static BOOL SetTextColor(HXCGUI hHost, COLORREF color);

//@备注 取文本颜色 (按当前主题).
//@参数 hHost 宿主
//@返回 ARGB
//@别名  取文本颜色()
	static COLORREF GetTextColor(HXCGUI hHost);

//@备注 自定义背景色 (会切到 xuitool_theme_custom).
//@参数 hHost 宿主
//@参数 color XCGUI ARGB
//@返回 成功 TRUE
//@别名  置背景颜色()
	static BOOL SetBkColor(HXCGUI hHost, COLORREF color);

//@备注 取背景色 (按当前主题).
//@参数 hHost 宿主
//@返回 ARGB
//@别名  取背景颜色()
	static COLORREF GetBkColor(HXCGUI hHost);

//@备注 自定义动画强调色 (会切到 xuitool_theme_custom). 该色会被用于动画主体笔画;
//      非强调色 (淡色) 由本类内部按 RGB 通道一致原则自动派生 (alpha 衰减).
//@参数 hHost 宿主
//@参数 color XCGUI ARGB
//@返回 成功 TRUE
//@别名  置强调色()
	static BOOL SetAccentColor(HXCGUI hHost, COLORREF color);

//@备注 取强调色 (按当前主题).
//@参数 hHost 宿主
//@返回 ARGB
//@别名  取强调色()
	static COLORREF GetAccentColor(HXCGUI hHost);

	// ===== 圆角 (背景) =====

//@备注 统一设置 4 个圆角. 单位 = 逻辑像素. 默认 0 (直角).
//@参数 hHost 宿主
//@参数 radius 圆角半径
//@返回 成功 TRUE
//@别名  置圆角()
	static BOOL SetCornerRadius(HXCGUI hHost, int radius);

//@备注 取圆角 (返回 4 角中左上的值; 4 角不一致时其它角值丢失, 用 GetCornerRadiusEx).
//@参数 hHost 宿主
//@返回 圆角半径
//@别名  取圆角()
	static int GetCornerRadius(HXCGUI hHost);

//@备注 分别设置 4 个圆角 (CSS 顺序: 左上 → 右上 → 右下 → 左下, 顺时针).
//@参数 hHost 宿主
//@参数 leftTop 左上 (CSS 第 1)
//@参数 rightTop 右上 (CSS 第 2)
//@参数 rightBottom 右下 (CSS 第 3)
//@参数 leftBottom 左下 (CSS 第 4)
//@返回 成功 TRUE
//@别名  置圆角扩展()
	static BOOL SetCornerRadiusEx(HXCGUI hHost, int leftTop, int rightTop, int rightBottom, int leftBottom);

//@备注 取 4 个圆角 (CSS 顺序). 任意 p 参数为 NULL 时忽略.
//@参数 hHost 宿主
//@参数 pLeftTop 接收左上
//@参数 pRightTop 接收右上
//@参数 pRightBottom 接收右下
//@参数 pLeftBottom 接收左下
//@别名  取圆角扩展()
	static void GetCornerRadiusEx(HXCGUI hHost, int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom);

	// ===== 速度 =====

//@备注 设置动画速度倍率. 1.0 = 默认, 2.0 = 双速, 0.5 = 半速. 范围 [0.1, 5.0].
//@参数 hHost 宿主
//@参数 speed 倍率
//@返回 成功 TRUE
//@别名  置速度()
	static BOOL SetSpeed(HXCGUI hHost, float speed);

//@备注 取速度倍率.
//@参数 hHost 宿主
//@返回 倍率
//@别名  取速度()
	static float GetSpeed(HXCGUI hHost);

	// ===== 全局清理 =====

//@备注 全局清理: 停所有 timer, 清空注册表.
//@别名  清理()
	static void Cleanup();
};
//@分组}

// =====================================================================
// CXCalendarCard — 日期 / 日期范围选择卡片
// =====================================================================

///<月历卡片日期时间结构
//@别名 月历日期时间
struct xcalendar_datetime_
{
	//@别名 年
	int year;    ///<年
	//@别名 月
	int month;   ///<月, 1-12
	//@别名 日
	int day;     ///<日, 1-31
	//@别名 时
	int hour;    ///<时, 0-23
	//@别名 分
	int minute;  ///<分, 0-59
	//@别名 秒
	int second;  ///<秒, 0-59
};

//@分组{ 月历卡片
//@备注  日期 / 日期范围选择卡片工具类, 全静态方法. PopupSingle 弹出单个月历,
//       PopupDouble 弹出双个月历范围选择; 默认选中今天, 可限制最大可选日期
//       (默认限制到今天), 双月范围支持今天 / 近7天 / 近15天 / 近30天快捷选择,
//       并可通过输入框和上下调节按钮修改年月日时分秒. 弹窗带确认和取消按钮,
//       主题使用 xuitool_theme_ (月历不支持 custom, 传入时按深色处理).
//@别名  炫彩月历卡片类
class CXCalendarCard
{
public:

	// ===== 日期工具 =====

//@备注 获取本机当前日期时间.
//@返回 当前日期时间
//@别名  取今天()
	static xcalendar_datetime_ GetToday();

//@备注 格式化为完整日期时间: YYYY-MM-DD HH:MM:SS.
//@参数 date 日期时间
//@返回 格式化文本
//@别名  格式化日期时间()
	static CXText FormatDateTime(xcalendar_datetime_ date);

//@备注 格式化为短日期: YY/MM/DD, 适合单月历选择结果.
//@参数 date 日期时间
//@返回 格式化文本
//@别名  格式化短日期()
	static CXText FormatShortDate(xcalendar_datetime_ date);

//@备注 格式化为短日期: YY/MM/DD, 返回临时 const wchar_t* 指针; 下次调用会覆盖.
//@参数 date 日期时间
//@返回 临时文本指针
//@别名  格式化短日期指针()
	static const wchar_t* FormatShortDatePtr(xcalendar_datetime_ date);

//@备注 格式化为完整日期时间: YYYY-MM-DD HH:MM:SS, 返回临时 const wchar_t* 指针; 下次调用会覆盖.
//@参数 date 日期时间
//@返回 临时文本指针
//@别名  格式化日期时间指针()
	static const wchar_t* FormatDateTimePtr(xcalendar_datetime_ date);

	// ===== 弹出选择 =====

//@备注 设置下一次月历弹出的绑定元素位置. PopupSingle/PopupDouble 会以元素左下角为基准弹出,
//       并叠加 offsetX/offsetY; 超出屏幕时自动调整到屏幕内.
//@参数 hEle 绑定元素句柄
//@参数 offsetX 横向偏移
//@参数 offsetY 纵向偏移
//@别名  置绑定元素()
	static void SetBindEle(HELE hEle, int offsetX = 0, int offsetY = 0);

//@备注 设置下一次月历弹出的屏幕坐标位置. PopupSingle/PopupDouble 会直接按该 POINT 弹出,
//       超出屏幕时自动调整到屏幕内.
//@参数 pt 屏幕坐标
//@别名  置弹出位置()
	static void SetPopupPosition(POINT pt);

//@备注 弹出单个月历日期选择卡片. pDate 为 NULL 或 year<=0 时默认选中今天; 取消返回 FALSE.
//       bLimitMaxDate=TRUE 时禁止选择 pMaxDate 之后的日期; pMaxDate=NULL 表示限制到今天.
//@参数 hParent 父窗口句柄, 可为 NULL
//@参数 pDate 输入初始日期并接收选择结果
//@参数 bLimitMaxDate 是否启用最大日期限制
//@参数 theme UI工具主题枚举 (推荐 dark / light / auto)
//@参数 pMaxDate 最大可选日期, NULL 表示今天
//@参数 nCornerRadius 弹窗圆角大小
//@返回 确认 TRUE, 取消 FALSE
//@别名  弹出单月历()
	static BOOL PopupSingle(HWINDOW hParent, xcalendar_datetime_* pDate,
		BOOL bLimitMaxDate = FALSE, xuitool_theme_ theme = xuitool_theme_auto,
		const xcalendar_datetime_* pMaxDate = NULL, int nCornerRadius = 8);

//@备注 弹出双个月历日期范围选择卡片. pStart/pEnd 为 NULL 或 year<=0 时默认今天
//       00:00:00 到 23:59:59; 取消返回 FALSE. bLimitMaxDate=TRUE 时禁止选择
//       pMaxDate 之后的日期; pMaxDate=NULL 表示限制到今天.
//@参数 hParent 父窗口句柄, 可为 NULL
//@参数 pStart 输入起始时间并接收结果
//@参数 pEnd 输入结束时间并接收结果
//@参数 bLimitMaxDate 是否启用最大日期限制
//@参数 theme UI工具主题枚举 (推荐 dark / light / auto)
//@参数 pMaxDate 最大可选日期, NULL 表示今天
//@参数 nCornerRadius 弹窗圆角大小
//@返回 确认 TRUE, 取消 FALSE
//@别名  弹出双月历()
	static BOOL PopupDouble(HWINDOW hParent, xcalendar_datetime_* pStart, xcalendar_datetime_* pEnd,
		BOOL bLimitMaxDate = TRUE, xuitool_theme_ theme = xuitool_theme_auto,
		const xcalendar_datetime_* pMaxDate = NULL, int nCornerRadius = 8);
};
//@分组}

// =================================================================
// CXShadow / CXEditDW / CXBlur — 自原独立模块并入 (P0-4)
// =================================================================

//@隐藏{
class CXShadow;
//@隐藏}

///<阴影主题预设 (CXShadow::SetTheme)
//@别名 阴影主题
enum xshadow_theme_
{
    //@别名 阴影主题_自定义
    xshadow_theme_custom    = 0,
    //@别名 阴影主题_浅色
    xshadow_theme_light     = 1,
    //@别名 阴影主题_深色
    xshadow_theme_dark      = 2,
    //@别名 阴影主题_跟随系统
    xshadow_theme_auto      = 3,
};

//@分组{ 窗口阴影
//@备注  Win11 风格的窗口外阴影 + 圆角 AA 描边 + 圆角内圈背景. 通过 AttachToWnd
//       附加到一个 XCGUI 窗口接管其 paint, 不继承 CXEle, 也不创建额外 HWND.
//@别名  炫彩窗口阴影类
class CXShadow
{
public:
    //@隐藏{
    CXShadow();
    virtual ~CXShadow();
    //@隐藏}

//@备注 堆上创建实例. 创建后须再调 附加窗口(). 勿在 CXShadow(HWINDOW) 构造里附加
//      (已移除该构造). 炫彩自定义类勿用 CXShadow 值成员.
//@别名  创建()
    static CXShadow* Create() { return new CXShadow(); }

//@备注 销毁 Create() 返回的实例. 会先 Detach.
//@参数 p Create() 返回值; 允许 NULL.
//@别名  销毁()
    static void Destroy(CXShadow* p) { delete p; }

//@备注 把阴影附加到一个炫彩窗口. 不创建额外 HWND, 而是接管主窗的透明属性
//      + padding + WM_PAINT / NCHITTEST / SIZE / DPICHANGED 事件.
//      主窗原状态 (transparent type/alpha, padding, layout) 被保存,
//      Detach 时还原. 与 XCGUI EnableDragBorder / EnableMaxWindow / EnableDragCaption 兼容.
//
//      *与 CXBlur (DWM acrylic) 共存说明*:
//      本类强制把宿主切到 window_transparent_shaped (halo alpha 通道需要).
//      若 attach 之后调用方再 XWnd_SetTransparentType 把宿主切回 false / shadow /
//      simple (例如想让 CXBlur 的 acrylic 生效), 本类自动 *降级* 为 "无 halo, 无
//      描边, 仅填内圈 bg" — 因为 DWM 架构限制 alpha 通道在 shaped 与 acrylic 路径
//      间只能二选一. 切回 shaped 下一帧自动恢复完整阴影. 用户通过切换 transparent
//      type 自由选择 "此刻要 acrylic blur 还是要 shadow halo".
//@参数 hWnd 目标窗口.
//@返回 TRUE 成功, FALSE 句柄非法或事件注册失败.
//@别名  附加窗口()
    BOOL AttachToWnd(HWINDOW hWnd);

//@备注 解除附加. 还原主窗 transparent / padding / layout 状态,
//      反注册事件钩子. 不影响主窗本身的子元素.
//@别名  解除绑定()
    void Detach();

//@备注 当前是否已附加到窗口.
//@别名  是否已附加()
    BOOL IsAttached() const;

//@备注 取被附加的窗口句柄.
//@别名  取附加窗口()
    HWINDOW GetAttachedWnd() const;

    // ===== 圆角 =====
//@备注 设置圆角半径. 单位 = 逻辑像素 @ 96 DPI, 内部按 DPI/96 缩放. Win11 默认 8.
//      传 0 = 直角 (无圆角, 仅外阴影). 大于内圈短边一半时自动 clamp.
//@参数 radius 圆角半径 (逻辑像素).
//@别名  置圆角()
    void SetCornerRadius(int radius);

//@备注 取圆角半径 (逻辑像素).
//@别名  取圆角()
    int GetCornerRadius() const;

    // ===== 阴影 =====
//@备注 设置阴影模糊半径. Gaussian blur 标准差近似. 单位 = 逻辑像素.
//      值越大阴影越柔越远. Win11 默认 16. 传 0 = 不模糊 (硬边阴影).
//@参数 radius 模糊半径 (逻辑像素).
//@别名  置阴影模糊半径()
    void SetShadowRadius(int radius);

//@备注 取阴影模糊半径 (逻辑像素).
//@别名  取阴影模糊半径()
    int GetShadowRadius() const;

//@备注 设置阴影扩散. 类似 CSS box-shadow 的第三个长度参数. 阴影源矩形向外膨胀
//      此值后再应用 blur. 默认 0. 单位 = 逻辑像素.
//@参数 spread 扩散 (逻辑像素).
//@别名  置阴影扩散()
    void SetShadowSpread(int spread);

//@备注 取阴影扩散 (逻辑像素).
//@别名  取阴影扩散()
    int GetShadowSpread() const;

//@备注 设置阴影偏移. (dx, dy) 单位 = 逻辑像素. Win11 默认 (0, 4) (轻微下沉).
//      正 dx 阴影向右; 正 dy 阴影向下.
//@参数 dx X 偏移 (逻辑像素).
//@参数 dy Y 偏移 (逻辑像素).
//@别名  置阴影偏移()
    void SetShadowOffset(int dx, int dy);

//@备注 取阴影偏移 (逻辑像素). pdx / pdy 可为 NULL.
//@参数 pdx 接收 X 偏移.
//@参数 pdy 接收 Y 偏移.
//@别名  取阴影偏移()
    void GetShadowOffset(int* pdx, int* pdy) const;

//@备注 设置阴影颜色. RGBA, alpha 高字节, XCGUI 标准编码 0xAABBGGRR.
//      默认 0x40000000 (黑 25% alpha).
//@参数 color 阴影色.
//@别名  置阴影色()
    void SetShadowColor(COLORREF color);

//@备注 取阴影色.
//@别名  取阴影色()
    COLORREF GetShadowColor() const;

//@备注 设置主窗口失活时的阴影色. 主窗口失去焦点 (WM_ACTIVATE = WA_INACTIVE)
//      自动切换到此色, 重新激活恢复. 默认 0x20000000 (黑 12% alpha).
//      若不希望区分激活态, 设为与 SetShadowColor 相同值.
//@参数 color 阴影色 (失活态).
//@别名  置失活阴影色()
    void SetInactiveShadowColor(COLORREF color);

//@备注 取失活阴影色.
//@别名  取失活阴影色()
    COLORREF GetInactiveShadowColor() const;

    // ===== 圆角描边 (Win11 风格 stroke) =====
//@备注 设置圆角描边色. RGBA, 默认 0x33000000 (黑 20% alpha, Win11 风格).
//      此描边画在阴影 bitmap 内圈圆角上, AA, 用于强化主窗口边缘的 Win11 视感.
//      圆角半径 = 0 时仅画矩形描边.
//@参数 color 描边色.
//@别名  置描边色()
    void SetBorderColor(COLORREF color);

//@备注 取描边色.
//@别名  取描边色()
    COLORREF GetBorderColor() const;

//@备注 设置描边宽度. 单位 = 逻辑像素 (浮点, 支持 0.5 这种亚像素描边).
//      默认 1.0. 传 0 关闭描边.
//@参数 w 描边宽度 (逻辑像素).
//@别名  置描边宽()
    void SetBorderWidth(float w);

//@备注 取描边宽度 (逻辑像素).
//@别名  取描边宽()
    float GetBorderWidth() const;

    // ===== 圆角内扣修正 =====
//@备注 设置 *圆角内扣* 像素数 (覆盖主窗硬直角的修正量). 单位 = 物理像素.
//      默认 1. 增大可在大圆角 + 大模糊场景下消除主窗口边缘锯齿露出, 但
//      会有 1~2px 边缘色与主窗口内容混色 (取决于阴影色 alpha). 0 = 不修正.
//@参数 px 内扣像素 (物理像素, 不随 DPI 缩放, 与位图本身锯齿对应).
//@别名  置内扣修正()
    void SetInsetCorrection(int px);

//@备注 取内扣修正像素数.
//@别名  取内扣修正()
    int GetInsetCorrection() const;

    // ===== 主题 =====
//@备注 应用主题预设. light = 浅色背景下的暖灰阴影; dark = 深色背景下的强阴影;
//      auto = 根据系统亮/暗模式自动选. 主题会同时调整 shadow color / border color /
//      inactive shadow color, 不改变 radius / spread / offset / corner.
//@参数 theme 见 xshadow_theme_*.
//@别名  置主题()
    void SetTheme(int theme);

//@备注 取当前主题.
//@别名  取主题()
    int GetTheme() const;

//@备注 全局主题: 设置后会同步到当前所有 CXShadow 实例, 之后新创建
//      的实例默认也使用此主题. 适用于 "整个应用统一阴影风格" 场景.
//      个别窗口仍可通过 SetTheme(...) 单独 override.
//@别名  置全局主题()
    static void SetGlobalTheme(int theme);
//@别名  取全局主题()
    static int  GetGlobalTheme();

    // ===== 内圈背景填充 =====
//@备注 设置内圈背景色. 本类接管了主窗 WM_PAINT, 默认按主题 (light/dark/auto) 填一个
//      不透明背景, 调本函数可覆盖. 颜色 = 0xAABBGGRR XCGUI 标准;
//      alpha = 0 等价 ClearInnerBgColor (仅画阴影+描边, 内圈透明).
//      推荐: light 主题 RGBA(252,252,252,255), dark 主题 RGBA(32,32,32,255).
//      最大化状态下全矩面填该色 (此时 padding 已去, 主窗 = 全屏内圈).
//@参数 color 0xAABBGGRR 背景色.
//@别名  置内圈背景色()
    void SetInnerBgColor(COLORREF color);

//@备注 取消用户自定义背景, 退回主题默认. 调 SetTheme 后会同步重置为主题默认色.
//@别名  清除内圈背景色()
    void ClearInnerBgColor();

//@备注 取当前生效的内圈背景色 (用户自定义 或 主题默认).
//@别名  取内圈背景色()
    COLORREF GetInnerBgColor() const;

    // ===== 控制 =====
//@备注 当前主窗口是否处于最大化 (SIZE_MAXIMIZED) 状态. 此状态下阴影自动隐藏.
//@别名  是否最大化()
    BOOL IsMaximized() const;

//@备注 立即重绘阴影 bitmap (尺寸 / 位置不变). 改了任何视觉参数后内部已自动调用,
//      用户一般无需手动调.
//@别名  立即刷新()
    void Invalidate();

    // ===== Snap / 最大化 控制 =====
//@备注 启用 / 禁用本窗的 *Aero Snap 阻止* 功能. **默认 TRUE — snap 被阻止**.
//      *(注意: 与 CXBlur::EnableSnap 语义相反, 本类默认就阻止 snap. 因为 snap
//       状态会让 CXShadow 必须 ClearPadding 收起阴影, 视觉打断, 多数用户不希
//       望出现这种状态.)*
//
//      参数语义:
//        * bEnable = TRUE  → **阻止 snap** (默认). 字面禁 snap.
//        * bEnable = FALSE → 允许 snap (系统行为).
//
//      *bEnable=TRUE 是 "字面禁 snap"*:
//        - strip WS_MAXIMIZEBOX → 消除拖窗到屏幕边时浮出的 snap preview UI
//                                   (半透蒙层 / Snap Layouts 飞出框).
//                                   *副作用*: 标题栏最大化按钮变灰 *不可点*.
//        - WM_WINDOWPOSCHANGING 几何过滤 → 子类 proc 检测目标矩形是否匹配
//                                          snap layout (full / half / quarter),
//                                          是则设 SWP_NOMOVE | SWP_NOSIZE 阻止
//                                          落位. 兜底, 即便 preview 漏出也拦.
//        - *不* 吞 SC_MAXIMIZE — 用户仍可通过键盘 Win+↑ / 程序化 ShowWindow
//                                  (SW_MAXIMIZE) / SetWindowPlacement /
//                                  WS_MAXIMIZE 创建属性 来最大化.
//
//        几何过滤里用 IsZoomed(hwnd) 区分 "真最大化" 和 "snap 全屏": Win32
//        在派发 WINDOWPOSCHANGING 前已更新 WINDOWPLACEMENT.showCmd, 真最大
//        化时 IsZoomed=true → 跳过过滤. 这覆盖所有真最大化路径.
//
//      *与 EnableMaximize 的关系*:
//        - 本接口 (EnableSnap(TRUE)) 让按钮变灰但保留 Win+↑ / API 通路.
//        - EnableMaximize(FALSE) 在此基础上额外吞 SC_MAXIMIZE → 拦键盘 Win+↑
//          + 双击标题栏 + 系统菜单 "最大化". (API 路径 ShowWindow 仍能用.)
//        - 二者共享 WS_MAXIMIZEBOX strip 状态: 只要任一为禁用就 strip, 二者
//          都启用才还原.
//
//      *bEnable=TRUE 的额外性能收益*: 既然 snap 不可能发生, SyncWindowState
//      会跳过 IsWindowSnapped() 计算 (m_isSnapped 强制为 false), 省去每次
//      WM_SIZE / WM_WINDOWPOSCHANGED 的 GetWindowRect / MonitorFromWindow /
//      GetMonitorInfo 调用.
//
//      *副作用 / 限制*:
//        * snap 几何检测有 2 px 容差, 用户手动恰好 resize 到 1/2 屏 / 1/4
//          屏尺寸会被误拦. 概率极低 (要求 4 边都对齐 work area).
//        * 触摸板三指手势 / 屏幕投递的 snap 不走以上路径, 拦不住.
//
//      *bEnable=FALSE (从 TRUE 切回)*: 还原 WS_MAXIMIZEBOX (除非 EnableMaximize
//      (FALSE) 仍然要求 strip), 几何过滤 / IsWindowSnapped 计算恢复.
//
//      *attach 时序*:
//        * AttachToWnd 之前调本接口 → 仅记忆设置, AttachToWnd 时按值 strip.
//        * AttachToWnd 之后调本接口 → 立即生效.
//
//      返回 void (设置不会失败).
//@参数 bEnable TRUE 阻止 snap (默认, 按钮灰, 保留 Win+↑/API), FALSE 允许 snap.
//@别名  启用Snap阻止()
    void EnableSnap(BOOL bEnable);

//@备注 取当前 snap 阻止启用状态. 默认 TRUE (阻止). 返 EnableSnap 最近一次
//      参数; 与 EnableSnap 参数语义保持一致 (TRUE = 阻止 snap).
//@别名  是否阻止Snap()
    BOOL IsSnapEnabled() const;

//@备注 启用 / 禁用本窗的最大化能力. **默认 TRUE — 允许最大化** (与
//      XWnd_EnableMaxWindow 接口语义一致, 跟随窗口 WS_MAXIMIZEBOX 原始状态).
//
//      参数语义:
//        * bEnable = TRUE  → 允许最大化 (默认).
//        * bEnable = FALSE → 禁最大化: strip WS_MAXIMIZEBOX (按钮变灰, Snap
//                              Layouts 飞出框消失) + 子类 proc 吞 SC_MAXIMIZE
//                              (拦键盘 Win+Up + 双击标题栏 + 系统菜单 "最大化"
//                              + 程序化 ShowWindow(SW_MAXIMIZE)).
//
//      *与 EnableSnap 完全独立*. 见 EnableSnap 文档 "解耦" 章节.
//
//      *Detach 行为*: Detach 时本类自动还原 attach 前的 WS_MAXIMIZEBOX 状态.
//
//      *attach 时序*:
//        * AttachToWnd 之前调本接口 → 仅记忆设置, AttachToWnd 时按值 strip.
//        * AttachToWnd 之后调本接口 → 立即生效.
//
//      返回 void (设置不会失败).
//@参数 bEnable TRUE 允许最大化 (默认), FALSE 禁用最大化.
//@别名  启用最大化()
    void EnableMaximize(BOOL bEnable);

//@备注 取当前最大化启用状态. 默认 TRUE (允许). 返 EnableMaximize 最近一次
//      参数 (TRUE = 允许最大化).
//@别名  是否允许最大化()
    BOOL IsMaximizeEnabled() const;

    //@隐藏{
private:
    // ===== 绑定状态 =====
    HWINDOW m_hAttachedWnd  = NULL;       // 主窗口 (XCGUI 句柄)
    HWND    m_hMainHwnd     = NULL;       // 主窗口的 Win32 HWND
    int     m_dpi           = 96;
    float   m_dpiScale      = 1.0f;

    // ===== 状态机 =====
    bool    m_isMaximized   = false;
    bool    m_isMinimized   = false;
    bool    m_isSnapped     = false;      // aero snap (任意一边贴 monitor work area) → 阴影隐藏
    bool    m_isActive      = true;
    bool    m_dirty         = true;       // 保留字段; 重绘由 Invalidate → XWnd_Redraw 触发
    bool    m_eventsHooked  = false;
    bool    m_subclassInstalled = false;  // Win32 SetWindowSubclass 状态
    bool    m_firstPaintDone    = false;  // 第一次 WM_PAINT 完成 → 触发 ForceSubclassToTop 第二轮 bump
    bool    m_inSizeMove    = false;      // WM_ENTERSIZEMOVE..WM_EXITSIZEMOVE (snap 时序)

    // ===== Snap / 最大化 控制 =====
    //
    // m_snapDisabled (EnableSnap / IsSnapEnabled):
    //   true  (默认) → *字面禁 snap*. UpdateMaxBoxState 触发 strip WS_MAXIMIZEBOX
    //                  (消除拖边 snap preview UI), 子类 proc 几何过滤兜底,
    //                  SyncWindowState 跳过 IsWindowSnapped 计算 (m_isSnapped
    //                  强制 false). *不* 吞 SC_MAXIMIZE — Win+↑ / API 仍可
    //                  最大化.
    //   false           → snap 系统默认行为.
    //
    //   *与 CXBlur 语义反转*: CXBlur 的 m_snapEnabled=true 表示 snap *允许*,
    //   本类的 m_snapDisabled=true 表示 snap *被阻止*. 默认值都是 true 但
    //   含义相反 — 本类默认阻止 snap, CXBlur 默认允许. 详见 EnableSnap 文档.
    //
    // m_maxDisabled (EnableMaximize / IsMaximizeEnabled):
    //   false (默认) → 允许最大化 (与 XWnd_EnableMaxWindow 一致).
    //   true             → 禁最大化: UpdateMaxBoxState 触发 strip WS_MAXIMIZEBOX
    //                       (与 m_snapDisabled 共享 strip 状态) + 子类 proc 吞
    //                       SC_MAXIMIZE.
    //
    // *WS_MAXIMIZEBOX strip 共享状态机*: m_snapDisabled || m_maxDisabled 任一
    // 为 true 都 strip, 二者都 false 才还原. 由 UpdateMaxBoxState 集中维护,
    // EnableSnap / EnableMaximize / AttachToWnd 三入口都调它.
    //
    // *区分 "用户最大化" vs "snap 全屏" (二者 WINDOWPOSCHANGING 几何相同 = 全
    // 工作区)*: 子类 proc 直接调 IsZoomed(hwnd). Win32 在派发 WINDOWPOSCHANGING
    // 之前已更新 WINDOWPLACEMENT.showCmd, IsZoomed=true 覆盖所有真最大化路径
    // (键盘 Win+Up / 鼠标按钮 / API ShowWindow / SetWindowPlacement /
    // WS_MAXIMIZE 启动 / 拖到顶 snap-to-max). 不需要额外的暂态 flag.
    //
    // atomic 因为 EnableSnap / EnableMaximize 可能被 UI 线程外的线程调 (与
    // SetTheme 一致策略), 子类 proc 在 Win32 消息派发 (UI) 线程读, 共享访问
    // 需 atomic 防 tearing.
    std::atomic<bool> m_snapDisabled        {true};
    std::atomic<bool> m_maxDisabled         {false};
    bool              m_maxBoxSaved         = false;   // 是否已 strip WS_MAXIMIZEBOX
    bool              m_maxBoxOriginallySet = false;   // strip 前 WS_MAXIMIZEBOX 是否本来置位

    // ===== 主窗原状态备份 (Detach 还原用) =====
    bool    m_saved             = false;      // 是否已抓取过原状态
    int     m_savedTransType    = 0;          // window_transparent_* 原值
    BYTE    m_savedTransAlpha   = 255;        // 原 alpha
    int     m_savedPadL         = 0;          // 原 padding (Attach 前)
    int     m_savedPadT         = 0;
    int     m_savedPadR         = 0;
    int     m_savedPadB         = 0;
    BOOL    m_savedLayout       = TRUE;       // 原 EnableLayout

    // ===== 当前几何 / padding =====
    //
    // 区分两组值, 不能混用 (历史 bug 来源):
    //   m_marginL/T/R/B  : *阴影几何 margin* (= _XUITool::kShadowMargin 四边等宽).
    //                       用于: 内圈 bg / 描边 RECT, WM_NCHITTEST halo 边界.
    //   m_curPadL/T/R/B  : *OS padding* (= margin + borderWidth_phys),
    //                       通过 XWnd_SetPadding 写到 XCGUI. 比 margin 多 borderWidth
    //                       是为让子元素 layout 起点不压在 1px 描边上.
    //
    // ApplyPadding 会同步写两组值; ClearPadding 把两组都清零.
    int     m_marginL = 0, m_marginT = 0, m_marginR = 0, m_marginB = 0;
    int     m_curPadL = 0, m_curPadT = 0, m_curPadR = 0, m_curPadB = 0;


    // ===== 视觉参数 (逻辑像素 @ 96 DPI) =====
    //
    // halo 由 DrawDropShadow 绘制 (calendar 风格); 下列 setter 保留 API 兼容,
    // radius/spread/offset 不再影响 margin (固定 kShadowMargin).
    std::atomic<int>      m_cornerRadius  {8};
    std::atomic<int>      m_shadowRadius  {24};        // = key.blur (logical px)
    std::atomic<int>      m_shadowSpread  {0};
    std::atomic<int>      m_shadowDx      {0};
    std::atomic<int>      m_shadowDy      {6};         // = key.dy
    std::atomic<COLORREF> m_shadowColor   {0x1A000000u};   // 10% 黑 (light active key)
    std::atomic<COLORREF> m_inactiveShadow{0x0E000000u};   //  5.5% 黑 (light inactive)
    std::atomic<COLORREF> m_borderColor   {0x0F000000u};   //  6% 黑 (light stroke)
    std::atomic<float>    m_borderWidth   {1.0f};
    std::atomic<int>      m_inset         {0};
    std::atomic<int>      m_theme         {xshadow_theme_auto};   // 默认跟随系统

    // 内圈背景. m_innerBgUserSet=true → 用 m_innerBgColor;
    // false → ApplyThemePreset 根据当前 theme 算一个默认值写进 m_innerBgColor.
    std::atomic<COLORREF> m_innerBgColor  {0xFFFCFCFCu};   // light 主题默认 (FCFCFC 不透明)
    std::atomic<bool>     m_innerBgUserSet{false};

public:
    // =================================================================
    // 内部回调入口 (公开仅为方便 C 风格回调调用; 用户代码请勿直接调用)
    // =================================================================
    int OnWndPaintImpl(HWINDOW hWnd, HDRAW hDraw, BOOL* pbHandled);
    int OnWndProcImpl(HWINDOW hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL* pbHandled);

    // 公开是因为 Win32 SetWindowSubclass 的子类 proc 在匿名 namespace 里, 不属于 CXShadow.
    // 用户代码请勿直接调用 — 仅用于 click-through 命中测试.
    LRESULT ComputeNcHitTest(int screenX, int screenY) const;

    // 当前 padding (逻辑像素, 与 XWnd_SetPadding 单位一致). 供子类 proc 的
    // WM_GETMINMAXINFO 用, 将 ptMinTrackSize 加上 padding × dpiScale (物理),
    // 让用户配置的 minSize 不被阴影吃掉.
    // snap / max 状态 padding=0, 返 0 — 不影响系统对那两种状态的 sizing.
    int   GetCurPadX()  const { return m_curPadL + m_curPadR; }
    int   GetCurPadY()  const { return m_curPadT + m_curPadB; }
    float GetDpiScale() const { return m_dpiScale; }

private:
    // ===== 生命周期 =====
    void HookEvents();
    void UnhookEvents();
    // 强制把 Win32 子类放到 chain 顶部 (Remove + Add). XCGUI 后续装它自己的子类
    // 时, 会把我们压到下层, NCHITTEST → HTTRANSPARENT 失效. 在 HookEvents +
    // 第一次 WM_PAINT 各调一次防御性 bump.
    void ForceSubclassToTop();

    // 主窗属性接管/还原 (不动 EnableDrawBk — 靠 OnWndPaint 设 pbHandled=TRUE 跳默认背景填充)
    void CaptureMainStyles();    // Attach 第一次调: 抓 transparent / padding / layout 原值
    void RestoreMainStyles();    // Detach: 还原全部

    // 最大化禁用: WS_MAXIMIZEBOX strip / restore. 调用方在主线程, *不持任何锁*,
    // 所以可以直接 SetWindowPos(SWP_FRAMECHANGED) — 与 CXBlur 的死锁回避不同,
    // 这里没有共享状态需要锁.
    void StripMaxBox();          // 记录原态 + 关 WS_MAXIMIZEBOX (n-op 若已 strip)
    void RestoreMaxBox();        // 还原 WS_MAXIMIZEBOX 到 strip 前 (n-op 若没 strip)
    // WS_MAXIMIZEBOX 联合状态机: snap 阻止 / 最大化禁用 任一为 true 都 strip.
    // EnableSnap / EnableMaximize / AttachToWnd 三个入口都调本函数, 不直接
    // 用 StripMaxBox/RestoreMaxBox.
    void UpdateMaxBoxState();

    // ===== 同步 / 渲染 =====
    void RefreshDpi();                              // 读 XWnd_GetDPI(主窗), 更新 m_dpi / m_dpiScale

    // ===== padding 控制 =====
    void ComputeShadowMargin(int* pL, int* pT, int* pR, int* pB) const;  // 按 DPI 算 4 边 margin
    void ApplyPadding();         // = ComputeMargin + XWnd_SetPadding + EnableLayout(TRUE) + 缓存
    void ClearPadding();         // SetPadding(0,0,0,0) (最大化态用)

    // aero snap 检测: 任意一边贴 monitor work area 即视为 snap. 阴影应隐藏 (SyncWindowState)
    // (issue #4: 窗口拖到屏幕边缘触发系统缩放后, 阴影占太多空间)
    bool IsWindowSnapped() const;
    // 重新计算 m_isMaximized/m_isSnapped/m_isMinimized 并 ClearPadding 或 ApplyPadding.
    void SyncWindowState();

    // ===== Paint 实际绘制 =====
    void DrawNormalPaint(HDRAW hDraw, int clientW, int clientH);     // 普通态: shadow halo + 内圈 bg + 描边
    void DrawMaximizedPaint(HDRAW hDraw, int clientW, int clientH);  // 最大化态: 全矩内圈 bg

    // ComputeNcHitTest 已上移到 public 区 (subclass proc 在 namespace 里调用).

    void ApplyThemePreset(int theme);   // 同时计算 inner bg 默认色 (若用户未自定义)
    static BOOL IsSystemDarkMode();

    //@隐藏}
};
//@分组}

//@隐藏{
// CXEditDW 内部撤销 / 重做栈条目. 放在类外是为了避开部分预编译头解析器
// 对类内嵌 struct + 紧随的 std::vector<内嵌类型> 的查找顺序问题.
struct _XEditDW_UndoState{
	std::wstring text;
	std::vector<int> charStyle;   // 与 text 等长, 每个元素是样式表索引(-1=默认)
	int caret;
	int anchor;
};

// 内部样式表条目. 与公开的 editdw_style_info_ 一一对应, 但 imagePath 用 std::wstring
// (内部 hot path 不引入 CXText 包装开销). GetStyleInfo 公开 API 会做一次字段拷贝.
// type: 0=文本(字体) / 1=图片 / 2=UI对象. nRef = 当前多少个字符使用该样式.
// hFontImageObj: 根据 type 为 HFONTX / HIMAGE / HXCGUI; image/object 下是该对象本享句柄.
// color/bColor: 仅对 type=0 有意义 (文本颜色).
struct _XEditDW_Style{
	USHORT   type;            // 0=text(font), 1=image, 2=object
	USHORT   nRef;            // 引用计数
	HXCGUI   hFontImageObj;   // 句柄(字体/图片/UI对象)
	COLORREF color;
	BOOL     bColor;
	// 仅 type=1 (图片) 通过 InsertImageThumb / ClipboardPasteImage / 拖入图片路径时填充.
	// 编辑框内只显示 *缩略图*, 原始路径保留在此处供未来序列化保存 / 反序列化重载原图.
	// 其它 type 此字段为空.
	std::wstring imagePath;
};

// CXEditDW 文本分段单元 (分段 layout 架构).
// 单 IDWriteTextLayout 全文 shape 在 200K+ 字符量级 cost ~1秒 / 改字, 体验灾难. 改用 *按段
// 分 layout*: 每个 \n 之间各自一个 IDWriteTextLayout, 改一段只 reshape 那一段 (~1-5ms).
//  - textStart : 该段在 m_text 中的起始 wchar 偏移 (绝对位置).
//  - textLen   : 段长度 wchar 数, *包含* 末尾的 \n (除最后一段外); 不再分行内换行.
//  - pLayout   : 该段的 IDWriteTextLayout. NULL = 尚未构建 (lazily 在 EnsureParagraphLayout 建).
//  - yOffset   : 累加得到的 *物理像素* 顶端坐标 (相对 layout 区原点). RecomputeParaYOffsets 算.
//  - height    : 该段 layout GetMetrics().height (物理像素).
//  - width     : 该段 widthIncludingTrailingWhitespace (物理像素).
struct _XEditDW_Para{
	int                 textStart;
	int                 textLen;
	IDWriteTextLayout*  pLayout;
	float               yOffset;
	float               height;
	float               width;
};

// HFONTX → IDWriteFontFace + 系统字体集合. 与 XCGUI GDI 字体面一致, 避免 DirectWrite
// 按族名命中系统错误版本 (Win7 外挂 emoji 字体场景).
struct _EditDW_FontBinding{
	HFONTX                 hFontX;
	IDWriteFontFace*       pFace;
	IDWriteFontCollection* pCollection;
	LOGFONTW               logFont;
	bool                   valid;
};
//@隐藏}

///DirectWrite编辑框 样式类型 (editdw_style_info_::type 字段取值)
//@别名 DW编辑框样式类型
enum editdw_style_type_
{
	//@别名 DW编辑框样式类型_字体
	editdw_style_type_font   = 0,    ///<字体 (含颜色)
	//@别名 DW编辑框样式类型_图片
	editdw_style_type_image  = 1,    ///<图片
	//@别名 DW编辑框样式类型_UI对象
	editdw_style_type_object = 2,    ///<UI对象 (HELE / HXC_SHAPE 等)
};

///DirectWrite编辑框 内容原子类型 (editdw_content_item_::type 字段取值)
//@别名 DW编辑框内容类型
enum editdw_content_type_
{
	//@别名 DW编辑框内容类型_文本
	editdw_content_type_text   = 0,    ///<文本段
	//@别名 DW编辑框内容类型_图片
	editdw_content_type_image  = 1,    ///<内嵌图片
	//@别名 DW编辑框内容类型_UI对象
	editdw_content_type_object = 2,    ///<内嵌 UI 对象
};

///DirectWrite编辑框 样式信息 (CXEditDW::GetStyleInfo 输出)
//@别名 DW编辑框样式信息结构
struct editdw_style_info_
{
	//@别名  类型
	USHORT   type;              ///<样式类型, 见 editdw_style_type_
	//@别名  引用计数
	USHORT   nRef;              ///<引用计数
	//@别名  句柄
	HXCGUI   hFont_image_obj;   ///<句柄(字体,图片,UI对象)
	//@别名  颜色
	COLORREF color;             ///<颜色 (仅 type=字体 有效)
	//@别名  是否使用颜色
	BOOL     bColor;            ///<是否使用颜色 (仅 type=字体 有效)
	//@别名  图片路径
	CXText   imagePath;         ///<原始图片全路径 (仅 type=图片 有效, 其它为空)
};

///DirectWrite编辑框 内容原子 (CXEditDW::GetContents 输出元素)
///规则: 一次 GetContents 输出一组按 *出现顺序* 排列的原子.
///  text      : type=文本时有效, 其余为空文本.
///  imagePath : type=图片时有效, 其余为空文本.
///  hObject   : type=UI对象时有效, 其余为 NULL.
///\n 切割: 每个 \n 都触发新 item, 空行保留为 type=文本、text=空 的占位 item,
///保证调用方能根据 item 顺序还原行结构. \uFFFC 周围的空文本不发 (image/object 自身已是分隔).
//@别名 DW编辑框内容原子结构
struct editdw_content_item_
{
	//@别名  类型
	int      type;              ///<原子类型, 见 editdw_content_type_
	//@别名  文本
	CXText   text;              ///<type=文本时: 文本内容 (UTF-16, 不含 \uFFFC, 不含 \n)
	//@别名  图片路径
	CXText   imagePath;         ///<type=图片时: 原始图片全路径 (= InsertImageThumb 时记下)
	//@别名  对象句柄
	HXCGUI   hObject;           ///<type=UI对象时: 对象句柄 (借用, 生命周期归编辑框, 不要释放)
};

//@分组{ DirectWrite编辑框
//@备注  继承: CXScrollView, CXEle, CXWidgetUI, CXObjectUI, CXBase
//@别名  炫彩DW编辑框类
class CXEditDW : public CXScrollView
{
public:
	//@隐藏{
	CXEditDW(){}
	virtual ~CXEditDW();
	//@隐藏}

//@备注 创建 DirectWrite 编辑框元素. 内部调用 XSView_Create 后挂上自绘绘制 / 鼠标 /
//      键盘 / 焦点 / 定时器事件, 默认接收键盘焦点并设置 I 形光标。
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
//@返回 元素句柄
//@别名  创建()
	HELE Create(int x, int y, int cx, int cy, HXCGUI hParent = NULL);

//@备注 创建 DirectWrite 编辑框元素 (构造函数版本).
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
	CXEditDW(int x, int y, int cx, int cy, HXCGUI hParent = NULL){ Create(x, y, cx, cy, hParent); }

	// ===== 文本 =====
//@备注 设置全部文本. 1:1 写入, 完整保留原 wchar_t 序列 (含代理对).
//@参数 pString 字符串
//@别名  置文本()
	void SetText(const wchar_t* pString);

//@备注 取文本. 完整保留原 wchar_t 序列 (含代理对).
//@返回 返回文本字符串拷贝
//@别名  取文本()
	CXText GetText() const;

//@备注 取临时文本指针, 指向内部缓冲, 调用方不得长期持有, 不得修改.
//@返回 文本只读指针
//@别名  取文本临时()
	const wchar_t* GetTextTemp() const;

//@备注 取公开文本长度 (与 GetText / GetTextTemp 同源, 不含 inline 对象占位字符 \uFFFC).
//想取含占位的内部 wchar 数, 见 XSView_GetTotalSize 高度 / m_paragraphs 总段数等内部度量.
//@返回 wchar_t 数量 (代理对计为 2, 不含 \uFFFC)
//@别名  取文本长度()
	int GetLength() const;

//@返回 如果为空返回TRUE,否则返回FALSE
//@别名  是否为空()
	BOOL IsEmpty() const;

//@备注 清空所有文本.
//@别名  清空()
	void Clear(){ SetText(L""); }

//@备注 追加文本到末尾, 光标跟随到末尾.
//@参数 pString 字符串
//@别名  追加文本()
	void AddText(const wchar_t* pString);

//@备注 在光标处插入文本 (替换选区). 只读时无效.
//@参数 pString 字符串
//@别名  插入文本()
	void InsertText(const wchar_t* pString);

	// ===== 样式表 / 对象插入 (XEdit 兼容, 单一 wchar_t 偏移寻址) =====
//@备注 添加一条样式。hFontImageObj 可传 HFONTX(字体) / HIMAGE(图片) / HXCGUI(UI对象),
//内部通过 XC_GetObjectType 识别. type=0(字体) 时 color/bColor 生效; image/object 时忽略.
//返回的样式索引可用于 AddTextEx / InsertTextEx / AddByStyle / SetCurStyle / ModifyStyle.
//@参数 hFontImageObj 字体/图片/UI对象句柄 (可为 NULL, 仅作颜色样式)
//@参数 color 颜色, RGBA()
//@参数 bColor 是否使用颜色
//@返回 样式索引 (从 0 开始), 失败返回 -1
//@别名  添加样式()
	int AddStyle(HXCGUI hFontImageObj, COLORREF color, BOOL bColor);

//@备注 从字体名/字号/字型/颜色添加样式. 内部调 XFont_Create + AddStyle, 返回样式索引.
//@参数 fontName 字体名称
//@参数 fontSize 字体大小 (pt)
//@参数 fontStyle 字体样式, 参考 font_style_ 宏
//@参数 color 颜色, RGBA()
//@参数 bColor 是否使用颜色
//@返回 样式索引
//@别名  添加样式扩展()
	int AddStyleEx(const wchar_t* fontName, int fontSize, int fontStyle, COLORREF color, BOOL bColor);

//@备注 修改样式. 仅对 type=0(字体) 生效 - hFont 换为新字体句柄, color/bColor 同时更新.
//修改后使用该样式的所有字符会重新绘制.
//@参数 iStyle 样式索引
//@参数 hFont 新字体句柄 (传 NULL 保留原字体)
//@参数 color 新颜色
//@参数 bColor 是否使用颜色
//@返回 成功返回TRUE,否则返回FALSE
//@别名  修改样式()
	BOOL ModifyStyle(int iStyle, HFONTX hFont, COLORREF color, BOOL bColor);

//@备注 删除样式. 仅当引用计数为 0 (没有字符使用该样式) 时才能删除.
//删除后该索引可被后续 AddStyle 复用.
//@参数 iStyle 样式索引
//@返回 成功返回TRUE
//@别名  删除样式()
	BOOL DeleteStyle(int iStyle);

//@备注 取样式信息.
//@参数 iStyle 样式索引
//@参数 info 输出信息结构
//@返回 成功返回TRUE
//@别名  取样式信息()
	BOOL GetStyleInfo(int iStyle, editdw_style_info_* info) const;

//@备注 置当前插入样式。设置后 InsertText / AddText / OnChar 输入的新字符默认使用该样式.
//传 -1 表示默认 (使用 m_fontName / m_fontSize / m_textColor).
//@参数 iStyle 样式索引
//@别名  置当前样式()
	void SetCurStyle(int iStyle);

//@返回 当前样式索引
//@别名  取当前样式()
	int  GetCurStyle() const;

//@备注 追加文本到末尾 并使用指定样式.
//@参数 pString 字符串
//@参数 iStyle 样式索引
//@别名  添加文本扩展()
	void AddTextEx(const wchar_t* pString, int iStyle);

//@备注 在指定 wchar_t 偏移位置插入文本 并使用指定样式.
//@参数 pos wchar_t 偏移 (超出范围自动夹到 [0, len])
//@参数 pString 字符串
//@参数 iStyle 样式索引
//@别名  插入文本扩展()
	void InsertTextEx(int pos, const wchar_t* pString, int iStyle);

//@备注 追加 UI 对象到末尾 (在文本中占 1 个字符位, 字符为 U+FFFC).
//内部会为对象创建一条样式, 后续可用返回的样式索引另调 ModifyStyle.
//hObj 需事先创建为 CXEditDW 本身的子元素 (例如 ele.Create(... , editdw._hEle)).
//@参数 hObj UI 对象句柄 (HELE 或其他 HXCGUI)
//@返回 所创建的样式索引
//@别名  添加对象()
	int AddObject(HXCGUI hObj);

//@备注 在指定 wchar_t 偏移位置插入 UI 对象.
//@参数 pos wchar_t 偏移
//@参数 hObj UI 对象句柄
//@别名  插入对象()
	void InsertObject(int pos, HXCGUI hObj);

//@备注 将样式代表的 image/object 追加到光标处 (样式类型必须是 image 或 object).
//@参数 iStyle 样式索引
//@别名  添加对象从样式()
	void AddByStyle(int iStyle);

//@备注 设置图片缩略图尺寸阈值. 影响后续 InsertImageThumb / ClipboardPasteImage / 拖入
//图片. 已插入的图片不变. 规则: 正方形图片边长 > maxSquare 时按比例缩到 maxSquare; 非
//正方形长边 > maxLong 时按比例缩到 maxLong; 已小于阈值的图片保留原始尺寸不放大.
//@参数 maxLong 非正方形图片长边最大值 (默认 200 px)
//@参数 maxSquare 正方形图片边长最大值 (默认 150 px)
//@别名  置缩略图尺寸()
	void SetImageThumbMaxSize(int maxLong, int maxSquare);

//@备注 设置文本内容最大长度上限 (wchar 数). 任何 *新增* 文本会被截断到剩余配额; 已有
//内容不受影响. inline 对象占 1 wchar (\uFFFC). 推荐取值: 5M (≈ 5MB UTF-16 文本); 文本量
//更大时 DirectWrite layout cost 急剧上升, 建议拆分编辑器实例或分页加载.
//@参数 maxLen 上限 wchar 数. 传 <=0 视为无效, 不修改.
//@别名  置最大长度()
	void SetMaxTextLength(int maxLen);

//@备注 取当前文本长度上限设置. 默认 5*1024*1024.
//@返回 当前上限 wchar 数
//@别名  取最大长度()
	int  GetMaxTextLength() const;

//@备注 设置全局图片持久化目录 (静态, 进程级). 设置后, *任何* 路径下的图片在被插入编辑框
// (拖入 / CF_HDROP 粘贴 / CF_DIB|CF_BITMAP 剪贴图像 / 直接调 InsertImageThumb / CopyFrom 跨编辑器复制)
// 时, 会先把原文件 *复制* 到该目录, 内部 imagePath 改记新路径. 这样:
//   - 序列化 (SaveToFile / SaveToMem) 出来的 imagePath 字段是稳定可控的;
//   - 临时图 (Win+Shift+S / QQ 粘贴 等走 %TEMP%) 不再因系统清理而丢图源;
//   - 多个编辑器实例可共享同一图片仓库.
//规则: 原路径已位于该目录下 (前缀匹配, 不区分大小写) → 视为已持久化, 直接复用原路径不再
//复制. 复制目标文件名 = "img_<srcPath 的 FNV-1a 64-bit hash>.<原扩展名>"; 同源图重复粘贴
//只生成一份目标 (hash 同 → 复用文件名, 覆盖写入相同内容).
//传 NULL / 空串 → 关闭转存 (默认状态, 每条 imagePath 保留传入原值, 同旧行为).
//@参数 pPath 目录全路径 (不含尾部反斜杠也可, 内部会规范化)
//@别名 置图片转存路径()
	static void SetImagePersistPath(const wchar_t* pPath);

//@备注 取当前全局图片持久化目录. 未设置时返空字符串 (但永远非 NULL).
//@返回 目录全路径 (内部缓存指针, 调用方不要 free)
//@别名 取图片转存路径()
	static const wchar_t* GetImagePersistPath();

//@备注 从文件路径加载图片, 按缩略图尺寸缩放后作为 inline 对象插入到光标处. 原路径会
//保留在内部样式槽, 供未来序列化场景按路径回写原始文件. 失败 (路径无效 / 不是图片) 返
//FALSE. 若已通过 SetImagePersistPath 配置全局转存目录, 图片会先被复制到该目录, 内部
//imagePath 改记新路径; 原路径已在该目录下时不重复复制 (直接复用).
//@参数 pPath 图片文件全路径
//@返回 成功返回TRUE, 否则返回FALSE
//@别名  插入图片缩略图()
	BOOL InsertImageThumb(const wchar_t* pPath);

//@备注 检查系统剪贴板, 若有图像数据 (CF_DIB / CF_BITMAP) 则写入到 %TEMP% 下的临时 BMP
//文件, 再以缩略图尺寸插入到光标处. 没有图像数据 (或读取失败) 返 FALSE - 上层可继续走
//ClipboardPaste 的文本路径. ClipboardPaste 已自动先调用本函数, 一般无需直接调.
//@返回 成功返回TRUE, 否则返回FALSE
//@别名  剪贴板粘贴图片()
	BOOL ClipboardPasteImage();

//@备注 把 src 的可视内容 1:1 复制到本编辑器. 适用于 "聊天记录还原" / "两编辑器并排镜像"
// 等场景. 复制范围: 全部文本 (含代理对) + 字符级样式 (字体 / 字号 / 字型 / 颜色) +
// inline 对象 (内置类型 XC_SHAPE_TEXT / XC_SHAPE_PICTURE / XC_BUTTON / XC_TEXTLINK /
// XC_ELE / XC_SLIDERBAR / XC_PROGRESSBAR 走克隆; 其它 HELE 因事件无法迁移, 暂留空槽).
// *不* 复制: 自定义事件回调 / UserData / 撤销栈 / 字体名 / 字号 / 多行 / 只读等编辑器
// 自身配置 (这些在调用方 Create 后已配好, 不该被 src 覆盖).
// dst 调用后撤销栈清空 (CopyFrom 视作起点状态), 光标停在文末.
//@参数 src 源编辑框
//@返回 成功返 TRUE, 失败 (dst 未 Create) 返 FALSE
//@别名  从源复制()
	BOOL CopyFrom(const CXEditDW& src);

//@备注 提取编辑框内容为 *仅含内容描述* 的原子序列, 用于在外部按内容重建 XCGUI 元素 /
//序列化保存等. 与 CopyFrom 不同: 不复制样式 (字体/字号/颜色), 不复制资源 (图片走原始路径,
//UI 对象走句柄借用). 输出原子按 m_text 出现顺序排列, 类型见 editdw_content_type_.
//\n 切割: 每个 \n 都触发新 item, 空行保留为 text=空 的占位 TEXT, 调用方据此还原行结构.
//空文档返回空数组. 调用前会清空 out.
//@参数 out 输出原子列表 (调用前会被清空)
//@返回 成功返 TRUE
//@别名  取内容()
	BOOL GetContents(CXVector<editdw_content_item_>& out) const;

	// ===== 序列化 / 反序列化 =====
//@备注 保存编辑框内容到文件 (自定义二进制格式 v1, magic "XDW1").
// *保存*: 全文本 (wchar 序列, 含 \uFFFC) + 每字符样式索引 + 样式表 (字体: 名/字号/字型/颜色;
// 图片: 原始全路径; 内置 UI 对象: 类型 + 宽高 + 文本颜色 + 文本/范围/进度等基础属性).
// *不保存*: HBKM (背景管理器) / 按钮图标 HIMAGE / 自定义事件回调 / UserData / 撤销栈 /
// 编辑器自身配置 (字体名/字号/多行/只读 - 这些由 Create 后调用方自行配).
// 内置 UI 对象支持类型 (与 CopyFrom 一致): XC_SHAPE_TEXT / XC_SHAPE_PICTURE (走图片路径) /
// XC_BUTTON / XC_TEXTLINK / XC_ELE / XC_SLIDERBAR / XC_PROGRESSBAR. 其它 HELE 类型 (XEdit/
// XListBox 等) 因状态过于复杂, *跳过* - 加载后这些位置为空占位字符 \uFFFC.
//@参数 pPath 文件全路径
//@返回 成功返 TRUE
//@别名  保存到文件()
	BOOL SaveToFile(const wchar_t* pPath) const;

//@备注 从文件加载编辑框内容 (与 SaveToFile 配对). 加载前会全清当前内容 + 撤销栈.
//光标置于文末. magic / version 不匹配会返 FALSE 且不修改现有内容.
//@参数 pPath 文件全路径
//@返回 成功返 TRUE
//@别名  从文件加载()
	BOOL LoadFromFile(const wchar_t* pPath);

//@备注 保存编辑框内容到字节集 (二进制格式同 SaveToFile). 适合内存中传递 / 数据库存储.
//@参数 out 输出字节集 (调用前会被清空)
//@返回 成功返 TRUE
//@别名  保存到内存()
	BOOL SaveToMem(CXBytes& out) const;

//@备注 从内存加载编辑框内容 (与 SaveToMem 配对). 加载前会全清当前内容 + 撤销栈.
//@参数 pData 数据地址
//@参数 size 数据长度 (字节)
//@返回 成功返 TRUE
//@别名  从内存加载()
	BOOL LoadFromMem(const void* pData, size_t size);

	// ===== 字体 =====
//@备注 绑定真实字体句柄 (HFONTX). 支持 XFont_CreateFromFile 等私有加载字体.
//与 SetFontName 互斥: 本接口按句柄解析字体面; SetFontName 仅按系统已安装族名匹配.
//Create() 后首次排版会自动尝试绑定 XC_GetDefaultFont() 设置的全局默认字体.
//@参数 hFont 字体句柄 (NULL 表示清除句柄绑定)
//@别名  置字体()
	void SetFont(HFONTX hFont);

//@备注 置字体名称. 默认 "Segoe UI". 与 SetFont 互斥, 仅按系统字体集合族名匹配.
//@参数 pName 字体族名
//@别名  置字体名称()
	void SetFontName(const wchar_t* pName);

//@备注 置字号 (pt, 默认 14).
//@参数 pt 字号点数
//@别名  置字号()
	void SetFontSize(float pt);

//@返回 当前字号 (pt)
//@别名  取字号()
	float GetFontSize() const;

//@备注 置统一行高 (逻辑像素). 0 = DirectWrite 默认行高 (含 font leading); >0 走
//DWRITE_LINE_SPACING_METHOD_UNIFORM, 每行恰好高 nPixels, baseline 取 80%. 用于做
//"紧凑" 单/多行文本 (例如聊天气泡), 避免默认 line metric 把 leading 累加到行底.
//@参数 nPixels 行高 (逻辑像素, 0 表示恢复默认行为)
//@别名  置行间距()
	void SetLineSpacing(float nPixels);

//@返回 当前统一行高设置 (0=默认)
//@别名  取行间距()
	float GetLineSpacing() const;

	// ===== 颜色 =====
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名  置文本颜色()
	void SetTextColor(COLORREF color);
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名  置选区背景色()
	void SetSelectBkColor(COLORREF color);
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名  置插入符颜色()
	void SetCaretColor(COLORREF color);
//@备注 置插入符宽度 (逻辑像素, 默认 1; 实际画出笔画为 nWidth * dpiScale).
//@参数 nWidth 宽度
//@别名  置插入符宽度()
	void SetCaretWidth(int nWidth);
//@返回 当前插入符宽度 (逻辑像素).
//@别名  取插入符宽度()
	int  GetCaretWidth() const;
//@备注 置边框笔画宽度 (BkInfo 边框, 逻辑像素 × dpiScale). 与 SetBorderSize 不同:
//SetBorderSize 是 *文本内容收缩距离* (不画线); 本接口是 *边框实际笔画粗细*.
//@参数 nWidth 宽度
//@别名  置边框宽度()
	void SetBorderWidth(int nWidth);
//@返回 当前边框笔画宽度 (逻辑像素).
//@别名  取边框宽度()
	int  GetBorderWidth() const;
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名  置背景颜色()
	void SetBkColor(COLORREF color);
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名  置边框颜色()
	void SetBorderColor(COLORREF color);
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名  置焦点边框颜色()
	void SetFocusBorderColor(COLORREF color);
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名  置提示颜色()
	void SetHintColor(COLORREF color);

	// ===== 边框 =====
//@备注 置边框大小 - *仅作为文本可显示区向内收缩距离*. 不会改变 BkInfo
//边框颜色 / 笔画宽, 也不会改变 XEle_GetClientRect 的返回值. 控件需要可见边框颜色
//请另调 SetBorderColor / SetFocusBorderColor / XEle_AddBkBorder. 默认 (6,4,6,4) 让文本距边有呼吸.
//@参数 left 左边大小
//@参数 top 上边大小
//@参数 right 右边大小
//@参数 bottom 下边大小
//@别名  置边框大小()
	void SetBorderSize(int left, int top, int right, int bottom);

//@备注 启用或禁用绘制边框 (本类专属扩展). 基类 EnableDrawBorder 只关 XCGUI 默认隐含
//边框, 关不掉本类 BkInfo border 项, 也关不掉 XCGUI 内置焦点框 (#58B1FC 默认蓝). 本扩展
//接口三者一起处理: 重建 BkInfo (跳过 AddBkBorder) + 透传 XEle_EnableDrawBorder + 把
//XCGUI 内置焦点边框色覆盖为透明 (启用时从缓存恢复). 用本接口管理可一步到位.
//@参数 bEnable 是否启用 (默认 TRUE)
//@别名  启用绘制边框扩展()
	void EnableDrawBorderEx(BOOL bEnable);

//@返回 启用返 TRUE, 禁用返 FALSE.
//@别名  是否绘制边框扩展()
	BOOL IsDrawBorderEx() const;

	// ===== 默认文本 (与 XEdit 接口一致, 与 SetHintText / SetHintColor 途同) =====
//@备注 置默认文本 (内容为空时显示, 等同 SetHintText).
//@参数 pString 文本内容
//@别名  置默认文本()
	void SetDefaultText(const wchar_t* pString);
//@备注 置默认文本颜色 (等同 SetHintColor).
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名  置默认文本颜色()
	void SetDefaultTextColor(COLORREF color);

	// ===== 模式 =====
//@备注 启用 / 关闭多行模式 (默认单行).
//@参数 bEnable 是否启用
//@别名  启用多行()
	void EnableMultiLine(BOOL bEnable);

//@返回 多行模式返回TRUE,否则返回FALSE
//@别名  是否多行()
	BOOL IsMultiLine() const;

//@备注 启用 / 关闭自动换行 (默认启用).
//@参数 bEnable 是否启用
//@别名  启用自动换行()
	void EnableAutoWrap(BOOL bEnable);

//@返回 自动换行返回TRUE,否则返回FALSE
//@别名  是否自动换行()
	BOOL IsAutoWrap() const;

//@备注 强制立刻同步重建内部布局. 通常 SetRect / SetBorderSize 等改尺寸的接口只把布局
//标脏, 等下一帧 OnPaint 触发 EnsureLayout. 调用本接口后可立即通过 XSView_GetTotalSize
//读到最新内容尺寸, 用于自适应排版 (如气泡按内容高度收缩).
//@别名  强制刷新布局()
	void RelayoutNow();

//@备注 启用 / 关闭只读 (只读时禁止编辑、剪切、粘贴、撤销).
//@参数 bEnable 是否启用
//@别名  启用只读()
	void EnableReadOnly(BOOL bEnable);

//@返回 只读返回TRUE,否则返回FALSE
//@别名  是否只读()
	BOOL IsReadOnly() const;

//@备注 置文本对齐方式. *仅单行模式生效* (多行模式永远顶部左对齐). 水平 / 垂直标志可
//组合, 如 edit_textAlign_flag_center | edit_textAlign_flag_center_v = 水平垂直双居中.
//默认为 edit_textAlign_flag_left | edit_textAlign_flag_top.
//@参数 align 对齐方式, 参见 edit_textAlign_flag_.
//@别名  置文本对齐()
	void SetTextAlign(int align);

//@返回 当前对齐方式 (edit_textAlign_flag_ 位组合).
//@别名  取文本对齐()
	int  GetTextAlign() const;

//@备注 获得焦点时自动选中所有文本.
//@参数 bEnable 是否启用
//@别名  启用自动选择()
	void EnableAutoSelAll(BOOL bEnable);

//@备注 失去焦点时自动取消选区.
//@参数 bEnable 是否启用
//@别名  启用自动取消选择()
	void EnableAutoCancelSel(BOOL bEnable);

//@备注 将插入符移动到末尾.
//@别名  移动到末尾()
	void MoveEnd();

//@备注 视图自动滚动到当前插入符位置.
//@返回 成功返回TRUE
//@别名  自动滚动()
	BOOL AutoScroll();

//@备注 以整数设置文本 (内部调用 swprintf %d).
//@参数 nValue 整数值
//@别名  置文本整数()
	void SetTextInt(int nValue);

//@备注 置占位提示文本 (空文本时显示).
//@参数 pString 字符串
//@别名  置提示文本()
	void SetHintText(const wchar_t* pString);

	// ===== 选择 / 光标 =====
//@返回 当前光标 wchar_t 偏移
//@别名  取光标位置()
	int GetCurPos() const;

//@备注 置光标位置 (按 wchar_t 偏移; 自动跳过代理对中间).
//@参数 pos 偏移
//@返回 成功返回TRUE,否则返回FALSE
//@别名  置光标位置()
	BOOL SetCurPos(int pos);

//@备注 全选所有文本.
//@返回 成功返回TRUE,否则返回FALSE
//@别名  全选()
	BOOL SelectAll();

//@备注 取消选择, 锚点拉到当前光标.
//@返回 原本有选区返回TRUE,否则返回FALSE
//@别名  取消选择()
	BOOL CancelSelect();

//@备注 删除选择内容. 只读 / 无选区返回 FALSE.
//@返回 成功返回TRUE,否则返回FALSE
//@别名  删除选择()
	BOOL DeleteSelect();

//@返回 有选区返回TRUE,否则返回FALSE
//@别名  是否有选区()
	BOOL HasSelection() const;

//@返回 选区起点 wchar_t 偏移
//@别名  取选区起点()
	int GetSelStart() const;

//@返回 选区终点 wchar_t 偏移 (开区间)
//@别名  取选区终点()
	int GetSelEnd() const;

//@备注 取选中的文本拷贝.
//@返回 选中文本字符串
//@别名  取选中文本()
	CXText GetSelText() const;

	// ===== 剪贴板 =====
//@备注 复制选区文本到剪贴板 (CF_UNICODETEXT, 保留代理对).
//@返回 成功返回TRUE,否则返回FALSE
//@别名  复制()
	BOOL ClipboardCopy();

//@备注 剪切选区到剪贴板 (复制后删除).
//@返回 成功返回TRUE,否则返回FALSE
//@别名  剪切()
	BOOL ClipboardCut();

//@备注 从剪贴板粘贴文本到光标处. 单行模式自动剔除换行.
//@返回 成功返回TRUE,否则返回FALSE
//@别名  粘贴()
	BOOL ClipboardPaste();

	// ===== 撤销 / 重做 =====
//@备注 撤销上一步编辑 (最多 256 步).
//@返回 成功返回TRUE,否则返回FALSE
//@别名  撤销()
	BOOL Undo();

//@备注 重做上一步被撤销的编辑.
//@返回 成功返回TRUE,否则返回FALSE
//@别名  重做()
	BOOL Redo();

//@隐藏{
	// ===== 事件回调 (注册到 XEle_RegEventCPP1) =====
	int OnPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int OnLButtonDownImpl(HELE hEle, UINT flags, POINT* pt, BOOL* pbHandled);
	int OnLButtonUpImpl(HELE hEle, UINT flags, POINT* pt, BOOL* pbHandled);
	int OnMouseMoveImpl(HELE hEle, UINT flags, POINT* pt, BOOL* pbHandled);
	int OnLButtonDBClickImpl(HELE hEle, UINT flags, POINT* pt, BOOL* pbHandled);
	int OnKeyDownImpl(HELE hEle, WPARAM wParam, LPARAM lParam, BOOL* pbHandled);
	int OnCharImpl(HELE hEle, WPARAM wParam, LPARAM lParam, BOOL* pbHandled);
	int OnSetFocusImpl(HELE hEle, BOOL* pbHandled);
	int OnKillFocusImpl(HELE hEle, BOOL* pbHandled);
	// XE_KILLCAPTURE: 失去鼠标捕获 (Esc / 切窗口 / 程序内强制 SetCapture(FALSE) 等). 用作
	// 拖选自动滚动的 *安全网*: OnLButtonUpImpl 是常规终止路径, 但若 capture 被外部夺走 (例如
	// 用户拖到其他窗口或 Alt+Tab), 我们收不到 mouseUp, m_mouseDown 卡 true + timer 还在转,
	// 后续误把 caret 一直拉到滚动尾. 这里清空 m_mouseDown + Kill timer, 与 mouseUp 同一处理.
	int OnKillCaptureImpl(HELE hEle, BOOL* pbHandled);
	int OnTimerImpl(HELE hEle, UINT timerId, BOOL* pbHandled);
	int OnDestroyEndImpl(HELE hEle, BOOL* pbHandled);
	// XE_SIZE: 父布局填充 / SetRect 改尺寸后触发. 重新计算文本布局 (新 maxW), 否则保留
	// Create 时刻的 layout 在父布局完成后是 *陈旧* 的 - 表现是文本不按当前宽度自动换行,
	// 用户必须随便编辑一下才把布局刷掉 (CopyFrom 大量短文本看似 \n 失效就是这个原因).
	int OnSizeImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled);

	// XE_DROPFILES: 用户从资源管理器 / 桌面拖文件进编辑框. 需 *父窗口* 先调
	// XWnd_EnableDragFiles 启用. 流程: 遍历 HDROP - 每文件先嗅探图片魔数 → InsertImageThumb;
	// 否则尝试文本编码 (UTF-8 BOM / UTF-16 LE/BE BOM / 无 BOM UTF-8 校验 / 系统 ANSI) →
	// AddText. 完成后 MoveEnd 自动滚到底.
	int OnDropFilesImpl(HELE hEle, HDROP hDropInfo, BOOL* pbHandled);

private:
	// ===== 内部数据 =====
	std::wstring m_text;
	std::wstring m_hint;
	std::wstring m_fontName = L"Segoe UI";
	float m_fontSize = 14.0f;
	HFONTX m_hFontX = NULL;
	bool   m_useFontHandle = false;
	bool   m_fontNameOnly = false;
	_EditDW_FontBinding m_fontBinding = {};
	std::vector<_EditDW_FontBinding> m_styleFontBindings;
	// 0 = 默认 (DirectWrite 自己按 font metrics 决定每行高); >0 = 强制 UNIFORM 行高.
	float m_lineSpacing = 0.0f;

	int m_caret = 0;
	int m_anchor = 0;

	bool m_multiLine = false;
	bool m_wrap = true;
	bool m_readOnly = false;
	// 文本对齐 (edit_textAlign_flag_ 位组合). 仅单行模式下 EnsureParagraphLayout 与 paint
	// 路径会消费: 水平分量用 IDWriteTextLayout::SetTextAlignment, 垂直分量在绘制/命中/光标/
	// inline 对象 6 处用 ContentVerticalOffsetPhys() 统一偏移. 多行模式永远忽略 (顶左对齐).
	int  m_textAlign = 0;
	bool m_focused = false;
	bool m_mouseDown = false;
	bool m_caretVisible = true;
	wchar_t m_pendingHigh = 0;
	static const UINT kCaretTimerId = 0xC121;
	// 拖选自动滚动: 鼠标按住 (capture) 拖出元素本地边界时, 启动周期 timer 持续滚动 + 把
	// 选择端拉到 *鼠标投影到边界* 的位置. m_lastDragPt 是最近一次 MouseMove 的 *元素本地
	// 逻辑像素* (XCGUI POINT 单位), timer 触发时直接复用; m_autoScrollOn 表示 timer 已起,
	// 用于幂等开关 (避免重复 SetTimer).
	static const UINT kAutoScrollTimerId = 0xC122;
	POINT m_lastDragPt = { 0, 0 };
	bool  m_autoScrollOn = false;

	// Undo / Redo 栈, 元素类型 _XEditDW_UndoState 见类外声明.
	std::vector<_XEditDW_UndoState> m_undoStack;
	std::vector<_XEditDW_UndoState> m_redoStack;
	static const size_t kMaxUndoDepth = 256;
	// PushUndo 节流: 长文本下每次 push 都 deep copy m_text + m_charStyle (~600KB 文本 ~5ms),
	// 连续打字累积到秒级卡顿. 节流: 与上次 push 时间差 < kUndoMergeMs && |caret - lastCaret| <= 1
	// 视为同一编辑会话, 不再 push (栈顶 state 仍是合并后的 *会话起点*, Undo 一次回到起点).
	// kUndoMergeMs 800ms = Notepad 默认; 光标距离 1 = 单字符插/删 (大段删除 / 粘贴 distance > 1
	// 自然不合并, 边界正确). 用户按方向键 / 鼠标点击移动光标后, 距离自然 > 1 不会被合并.
	DWORD m_lastUndoTick  = 0;
	int   m_lastUndoCaret = -1;
	static const DWORD kUndoMergeMs = 800;

	// 内部剪贴板副本 (含样式索引). 系统剪贴板里放的是 *过滤掉 \uFFFC 的纯文本*, 外部程序
	// 粘到 Notepad 不会出现 "￼" 占位符. 粘回自己时: 比较 *m_clipText 过滤版本* 与系统文本,
	// 若一致 && 序列号未变, 用 m_clipStyles + m_clipText (含 \uFFFC) 完整还原 inline 对象.
	std::wstring     m_clipText;
	std::vector<int> m_clipStyles;
	DWORD            m_clipSeq = 0;

	// GetTextTemp 过滤 \uFFFC 后需要稳定指针 - 用 mutable 缓存承载; 每次调用重生成.
	mutable std::wstring m_textNoOrcCache;

	// ===== 样式与插入对象 (XEdit 模型) =====
	// m_styleTable: 样式表. 插槽可被复用 (DeleteStyle 后 type=0xFFFF 标记为无效、下次 AddStyle 会优先复用).
	std::vector<_XEditDW_Style> m_styleTable;

	// m_charStyle: 每个 wchar_t 一个样式索引. 与 m_text 同长. -1 表示使用默认 字体/颜色.
	// m_text[i] == U+FFFC (对象替换字符) 时 m_charStyle[i] 必为有效的 image/object 样式.
	std::vector<int> m_charStyle;

	// 当前插入样式. -1 = 默认 (输入的新字符用默认字体/颜色).
	int m_curStyle = -1;

	// U+FFFC = OBJECT REPLACEMENT CHARACTER, 为插入 UI 对象/图片 在文本中占 1 格.
	static const wchar_t kObjectReplacementChar = L'\xFFFC';

	// 颜色 (XCGUI RGBA 字节序: R | G<<8 | B<<16 | A<<24, 用 RGBA(r,g,b,a) 宏构造)
	// 文本/选区/光标/提示 这四种颜色由本类自绘 (在 XE_PAINT_END 中叠加于 XCGUI 默认背景之上).
	// 背景/边框/焦点边框 三种颜色仅作为 SetBkColor / SetBorderColor / SetFocusBorderColor 的
	// 数据缓存; 实际绘制走 XCGUI 标准 BkInfo 系统 (XEle_AddBkFill / XEle_AddBkBorder),
	// 因此用户也可直接调用 SetBkInfo / EnableDrawBorder / EnableDrawFocus / AddBkFill / AddBkBorder
	// 等基类 API, 与本类设置器同源, 互不冲突.
	COLORREF m_textColor        = RGBA(0x20, 0x21, 0x24, 0xFF);   // 深灰
	COLORREF m_selBgColor       = RGBA(0xCC, 0xE5, 0xFF, 0xFF);   // 浅蓝
	COLORREF m_caretColor       = RGBA(0x00, 0x00, 0x00, 0xFF);   // 黑
	COLORREF m_bkColor          = RGBA(0xFF, 0xFF, 0xFF, 0xFF);   // 白
	COLORREF m_borderColor      = RGBA(0xD1, 0xD5, 0xDB, 0xFF);   // 浅灰
	COLORREF m_focusBorderColor = RGBA(0x3B, 0x82, 0xF6, 0xFF);   // 蓝
	COLORREF m_hintColor        = RGBA(0x9C, 0xA3, 0xAF, 0xFF);   // 中灰

	// BkInfo 边框笔画宽 (XEle_AddBkBorder 的 width 入参). 与 SetBorderSize *无关*:
	// SetBorderSize 是布局 / 内容区收缩 概念, 不控画笔宽. 默认 1px (逻辑 × dpiScale).
	int m_borderWidth = 1;
	// EnableDrawBorderEx 的状态缓存. RebuildBkInfo 在此为 FALSE 时跳过 AddBkBorder,
	// 让 BkInfo 中只剩 BkFill - 用户视觉上看不见边框. 默认 true 保持原默认渲染行为.
	bool m_drawBorder = true;
	// XCGUI 内置焦点边框 (XEle_SetFocusBorderColor 系统) 与 BkInfo / XEle_EnableDrawBorder
	// 是独立路径 - 我们 ClearBkInfo + EnableDrawBorder(FALSE) 后这条 *仍画*, 颜色 #58B1FC
	// 大概率是 XCGUI 默认主题硬编码. EnableDrawBorderEx(FALSE) 时把它 *改成透明 RGBA(0,0,0,0)*
	// 视觉关闭; EnableDrawBorderEx(TRUE) 时从下面缓存恢复原色.
	COLORREF m_savedXcguiFocusBorderColor = 0;
	bool     m_focusBorderSaved = false;

	// 插入符 (光标) 画笔宽度, 逻辑像素; 实际出笔 = m_caretWidth * m_dpiScale (不少于 1px).
	int m_caretWidth = 1;

	// 焦点联动: 获得焦点自动全选 / 失去焦点自动取消选区 (与 XEdit 同名接口).
	bool m_autoSelAll = false;
	bool m_autoCancelSel = false;

	// DirectWrite 资源
	// m_pDWFactory: D2D 模式下从 XC_GetDWriteFactory 借 (XCGUI 自己持有, 不 Release);
	//               GDI+ 模式下 XC_GetDWriteFactory 返 NULL, 我们自己 DWriteCreateFactory
	//               建一份并由 m_dwFactoryOwned 标识所有权, 析构时 Release.
	// DirectWrite 与 D2D 解耦, dwrite.dll 在 Win7 SP1+ 全系统自带, 不需要 GPU - GDI+ 模式
	// 同样能用 DirectWrite 排版 + IDWriteBitmapRenderTarget 输出 GDI 渲染.
	IDWriteFactory* m_pDWFactory = NULL;
	bool m_dwFactoryOwned = false;
	IDWriteTextFormat* m_pTextFormat = NULL;   // 拥有
	// 分段 layout 架构: 替代之前的单 IDWriteTextLayout. 每段独立 IDWriteTextLayout, 详见
	// 文件顶部 _XEditDW_Para 结构定义和注释. 文本编辑只改受影响那一段, 其他段 layout 缓存
	// 跨编辑保留, 大文本 (>200K wchar) 单键入 cost 从 ~1s 降到 ~1-5ms.
	std::vector<_XEditDW_Para> m_paragraphs;
	float m_paraTotalHeight = 0.0f;   // 物理像素, 所有段 height 之和
	float m_paraMaxWidth    = 0.0f;   // 物理像素, max(段 width)
	// 整体 layout 是否需要复算 yOffsets / 总尺寸. 段结构本身由 ParaOnText* 增量维护
	// 始终与 m_text 同步; 此标志只表示 (a) 某段 pLayout 还没建 或 (b) yOffsets 累加待刷新.
	bool m_layoutDirty = true;
	// 上次 CreateTextLayout 时 m_text.size() 的快照. 给 GetCaretPointStale 用: 在键入热路径
	// 里 layout 比 m_text 旧, HitTestTextPosition 越界会失败, 这里 clamp 到旧长度即可拿到
	// 一个 "差不多" 的位置, 后续 EnsureLayout 重建后再精确补一次滚动 (m_scrollToCaretPending).
	int  m_lastBuiltTextLen = 0;
	// 上次 EnsureLayout 走完时的 *视口宽 物理像素*. OnSizeImpl 用它判断 contentW 是否真变化:
	//   - 变化: maxW 旧, 段 layout 内部换行结果错误, 必须 InvalidateLayout 全量重建.
	//   - 未变 (用户只拖高度 / 只滚动等): 段 layout 全部仍有效, 跳过 Invalidate, 滚动条
	//     只刷 totalSize, 60fps resize 60 帧不至于每帧都 600KB 文本重 reshape 几百 ms.
	float m_lastContentW = 0.0f;
	// 文本变更路径里设 true; EnsureLayout 末尾重建完一次, 检测此 flag 即调 EnsureCaretVisible
	// 精确滚动. 取代 v1 "InsertTextAtCursor 直接 EnsureCaretVisible 强制重建 layout" 的卡顿路径.
	bool m_scrollToCaretPending = false;

	// GDI 渲染回退路径 (Win7 无 GPU 的 VM 等场景, XDraw_GetD2dRenderTarget 返 NULL).
	// 我们仍用 DirectWrite 排版 (跟 D2D 无依赖), 只换渲染管线:
	//   m_pBmpRT  : IDWriteBitmapRenderTarget, 由 IDWriteGdiInterop 创建, 内部带 HDC + DIB,
	//               所有 glyph run 通过 BitmapRenderTarget::DrawGlyphRun 输出到该 DIB.
	//   m_pParams : IDWriteRenderingParams, ClearType / 灰度抗锯齿参数. 全局默认就行.
	// 流程: 每帧 OnPaintImpl 把目标 HDC 当前像素 BitBlt 进 DIB → 在 DIB 上画选区/文本/光标
	//       → 整块 BitBlt 回目标 HDC. 不预读会把 XCGUI BkInfo 画的背景擦掉.
	// 缓存: BitmapRT 与元素客户区一样大, 元素 resize 后才 Resize 重建.
	// emoji: BitmapRenderTarget 不支持 COLR/CPAL, emoji 显示单色 (灰度), 接受这个降级.
	IDWriteBitmapRenderTarget* m_pBmpRT = NULL;
	IDWriteRenderingParams*    m_pParams = NULL;
	int m_bmpRTW = 0;
	int m_bmpRTH = 0;

	// DPI 缩放比 (96=1.0, 144=1.5, 192=2.0). 启用 XC_EnableDPI 后, XDraw_GetD2dRenderTarget
	// 拿到的 D2D 渲染目标在物理像素上工作 (face-value). 内边距 / 边框 / 字号 仍是
	// 100% 逻辑值, 布局 / 绘制 / 命中测试都乘上 m_dpiScale 后才与 D2D RT 同坐标系.
	// 鼠标事件 POINT* 是元素本地 *逻辑* 坐标, 也要乘 m_dpiScale 才能落到 layout 物理坐标系.
	float m_dpiScale = 1.0f;

	// DrawSelection 复用 buffer, 避免每帧 vector 分配 (P1-2).
	mutable std::vector<DWRITE_HIT_TEST_METRICS> m_selHitMetrics;

	// 图片缩略图尺寸阈值, 逻辑像素. ComputeThumbSize 用. 通过 SetImageThumbMaxSize 改.
	// 默认: 长边 200 (非正方形); 边长 150 (正方形). 原图小于阈值时按原尺寸插入不放大.
	int m_imageThumbMaxLong   = 200;
	int m_imageThumbMaxSquare = 150;

	// 文本内容长度上限 (wchar 数, 含 \uFFFC 占位). 在 InsertCharsRaw 入口卡住; 超出则 *截断*
	// 新增部分 (而不是整段拒绝, 因为粘贴大文本时部分插入比丢弃更友好). 默认 5M wchars ≈ 10MB
	// 存储 - 与 DirectWrite layout cost 体感拐点接近, 再大就需要切换分段 layout 策略.
	int m_maxTextLen = 5 * 1024 * 1024;

	// ===== 内部辅助 =====
	static bool IsHighSurrogate(wchar_t c){ return c >= 0xD800 && c <= 0xDBFF; }
	static bool IsLowSurrogate(wchar_t c){ return c >= 0xDC00 && c <= 0xDFFF; }
	int ClampPos(int p) const;
	int NextCodepoint(int p) const;
	int PrevCodepoint(int p) const;
	bool HasSelectionInner() const;
	void GetSelectionRangeInner(int& start, int& end) const;
	static D2D1::ColorF RgbaToD2D(COLORREF rgba);
	void RedrawSelf();
	void ReleaseLayoutOnly();
	void ReleaseTextFormat();
	void ReleaseDWriteResources();
	void InvalidateLayout();
	void InstallEvents();
	void EnsureFactory();
	void InvalidateFontBinding();
	void InvalidateStyleFontBindings();
	bool BuildFontBinding(HFONTX hFont, _EditDW_FontBinding& out, float ptSize = -1.0f);
	void EnsureFontBinding();
	void EnsureStyleFontBinding(int styleId);
	HFONTX ResolveBindFont() const;
	IDWriteFontFace* GetBoundFontFace();
	IDWriteFontFace* GetStyleFontFace(int styleId);
	void EnsureTextFormat();
	void EnsureLayout();
	// 读 XWnd_GetDPI 刷新 m_dpiScale; 如果发生变化则释放 m_pTextFormat 和 m_paragraphs
	// 各段 layout (字号 = m_fontSize * m_dpiScale 创建, 故 DPI 改变时必须重建) 并同步滚动行高.
	void RefreshDpiScale();
	// XEle_ClearBkInfo + XEle_AddBkFill + XEle_AddBkBorder, 让 SetBkColor 等设置器和
	// XEle_SetBkInfo / EnableDrawBorder / EnableDrawFocus 共用同一条 XCGUI 标准绘制流水线.
	void RebuildBkInfo();
	float GetContentWidth();
	float GetContentHeight();
	// 文本对齐辅助. 仅单行模式有意义; 多行模式返回 0 / LEADING.
	// ContentVerticalOffsetPhys: 视口高 - 内容总高 > 0 时按 m_textAlign 的垂直位给整体段块
	// 一个向下偏移量 (物理像素). 0 = top / avail/2 = center_v / avail = bottom. 供 paint /
	// HitTestPoint / GetCaretPoint / PositionInlineObjects 统一消费.
	float ContentVerticalOffsetPhys() const;
	void DrawSelection(ID2D1RenderTarget* rt, float originX, float originY, int selStart, int selEnd);
	// GDI 选区: 用 ::FillRect 在 BitmapRT 的 DIB 上画蓝色矩形, 与 D2D 路径同语义
	// (跳过 \uFFFC 占位字符段, 保留 inline 子元素背景).
	void DrawSelectionGdi(HDC hdc, float originX, float originY, int selStart, int selEnd);
	// 取 / 建 / 改尺寸 BitmapRT. 失败返 NULL.
	IDWriteBitmapRenderTarget* EnsureBmpRT(int wantW, int wantH);
	// 释放 GDI 渲染资源 (BitmapRT + Params). 跟 ReleaseDWriteResources 分开,
	// 因为 BitmapRT 跟尺寸绑定, DPI / resize 都要重建; DWriteFactory / TextFormat 不动.
	void ReleaseGdiResources();
	// OnPaint 在拿不到 D2D RT 时走的分支. 与 D2D 分支同结构: 占位字 + 选区 + 文本 + 光标.
	// 传 hDraw 进来是为了 XDraw_GetOffset - GDI 自绘路径 HDC 坐标系是 *元素本地* (与 D2D RT
	// 的窗口客户区坐标不同, 见 XCGUI 文档 "画布偏移问题"), 必须加上 offset 才对位.
	void OnPaintGdi(HDC hdcTarget, HDRAW hDraw);
	int HitTestPoint(float xLocal, float yLocal);
	bool GetCaretPoint(int textPos, float* outX, float* outY, float* outH);
	// 同 GetCaretPoint, 但 *不* 调 EnsureLayout - 直接拿现有 m_paragraphs 段 layouts 做 HitTest.
	// 当某段 pLayout 还没建 / 段比 m_text 段结构滞后时, 段查不到 / 越界则返 false; 调用方
	// (EnsureCaretVisible) 拿不到精确点会跳过本帧滚动, 等 EnsureLayout 后续帧建好再补.
	bool GetCaretPointStale(int textPos, float* outX, float* outY, float* outH);
	// 样式表里是否存在 *被字符引用* 且 *启用了自定义颜色* 的文本样式. 用作 paint 路径分流:
	//   - 有 → 走自定义 IDWriteTextRenderer (per-segment SetDrawingEffect, 保证 D2D 1.1 bug
	//     下颜色仍生效).
	//   - 无 → 走 rt->DrawTextLayout 快路径 (跳过 O(n) charStyle 扫描 + 每 GlyphRun COM 回调,
	//     大文本性能差几个数量级).
	// O(|styleTable|) 不依赖 m_text 长度, 每帧调用代价可忽略.
	bool HasAnyColoredText() const;
	void UpdateCaret();
	void EnsureCaretVisible();
	// 拖选自动滚动. 由 OnMouseMoveImpl (用户移动鼠标到边外) 与 OnTimerImpl (鼠标停在边外
	// 不动) 共用: 读 m_lastDragPt → 算越界量 → ScrollPosXH/YV 单步 → 把鼠标坐标投影到边界
	// HitTestPoint 得到新 m_caret → RedrawSelf + UpdateCaret. 是否在边外由内部判定, 调用方
	// 无需提前检查. 入口 *仅在 m_mouseDown* 下生效, 防止 timer 在非拖选时也滚动.
	void UpdateAutoScroll();
	void PtToLayout(const POINT* pt, float* outX, float* outY);
	void InsertTextAtCursor(const wchar_t* p, int len);
	void PushUndo();
	void AfterCursorMove();
	void MoveCaretByLine(int dir, bool extendSel);
	void MoveCaretLineEdge(bool toEnd, bool ctrl, bool extendSel);
	bool CopyToClipboardImpl(const std::wstring& s);
	bool GetClipboardTextImpl(std::wstring& out);

	// ===== 图片缩略图 + 拖放分发 =====
	// 输入 *逻辑像素* 的原图尺寸, 输出 *逻辑像素* 的缩略图尺寸. 规则见 SetImageThumbMaxSize.
	void ComputeThumbSize(int srcW, int srcH, int& outW, int& outH) const;
	// 处理一个拖入文件: 先嗅探图片魔数 → InsertImageThumb; 否则尝试文本编码 → AddText.
	// 不支持的二进制文件返 FALSE (整个流程仍继续处理后续文件).
	bool TryInsertDroppedFile(const wchar_t* path);
	// 读取文件前 maxBytes 字节. 失败返 FALSE; out 在成功时含读到的字节.
	static bool ReadFileBytes(const wchar_t* path, std::vector<unsigned char>& out, size_t maxBytes);
	// 嗅探图片魔数: PNG / JPEG / GIF / BMP / WebP. 用前 16 字节判定.
	static bool SniffIsImage(const unsigned char* d, size_t n);
	// 判定字节流 *可能是文本*: NUL 字节 / 大量控制字符占比超 5% → 二进制 (返 FALSE).
	// 否则当文本 (返 TRUE).
	static bool ProbablyText(const unsigned char* d, size_t n);
	// 校验是否合法 UTF-8 (BOM 与续字节模式都对).
	static bool IsValidUtf8(const unsigned char* d, size_t n);
	// 文本字节 → wstring: BOM 优先 (UTF-8/16 LE/BE), 无 BOM 时先 UTF-8 校验失败再退到系统 ACP.
	static bool BytesToWStr(const unsigned char* d, size_t n, std::wstring& out);
	// 把当前剪贴板的 CF_DIB 写为 BMP 文件 (拼 BITMAPFILEHEADER + 原 DIB), 路径写入 outPath.
	// 文件在 %TEMP%\xcgui_paste_<pid>_<tick>.bmp.
	bool ClipboardImageToTempBmp(std::wstring& outPath);
	// 处理剪贴板里 CF_HDROP 文件列表 (用户从资源管理器 Ctrl+C 文件后, Ctrl+V 粘贴的场景).
	// 与 OnDropFilesImpl 共用 TryInsertDroppedFile, 但 *光标位置插入* (paste 语义), 不动
	// 选区/视图. 没文件 / 全部处理失败 返 FALSE - 调用方可继续 fallback 到文本路径.
	bool ClipboardPasteHDropFiles();

	// ===== 样式 / 对象 辅助 =====
	// 样式索引是否有效 (范围内 且 type 不是被标记的 0xFFFF “已删除”).
	bool IsStyleIdValid(int iStyle) const;
	// 引用计数调节 - 只处理有效索引.
	void StyleIncRef(int iStyle);
	void StyleDecRef(int iStyle);
	// 释放单条样式槽里的句柄 (字体 Release / 图片 Release / UI 对象 Destroy),
	// 并把槽位标记为 0xFFFF 供 AddStyle 复用. 不动 nRef (调用方决定).
	void ReleaseStyleSlot(int iStyle);
	// 克隆 inline 对象句柄: 给 Copy+Paste 多次粘贴产生独立实例使用. 内置 XC_SHAPE_TEXT /
	// XC_SHAPE_PICTURE + 五种 HELE (按钮 / 文本链接 / 通用元素 / 滑动条 / 进度条);
	// 其他类型返 NULL, 调用方退化为 *共享原句柄* (粘多次只显示最后一次).
	HXCGUI CloneInlineHandle(HXCGUI hSrc);
	// HELE 共通属性 (背景管理器 HBKM, 文本颜色) 的克隆转抄. 由 CloneInlineHandle 内每个
	// HELE 分支调用, 避免五份重复代码. HBKM 走 XBkM_AddRef 引用计数共享, 多个元素同时引
	// 用同一背景定义是 XCGUI 常规模式 (按钮主题共享) - 安全.
	void CopyCommonEleProps(HELE hSrc, HELE hDst);
	// 硬重置: 清 text + charStyle + 撤销/重做栈, 然后释放样式表里 *所有* 句柄并清空表.
	// SetText 与元素销毁 (析构 / XE_DESTROY) 都用它. 调用后 m_styleTable 为空, m_curStyle = -1.
	void ReleaseAllStyleResources();
	// 给 [pos, pos+len) 范围的字符赋样式, 自动维护并调整引用计数.
	void SetCharStyle(int pos, int len, int iStyle, bool invalidateLayout = true);
	// 在 m_text + m_charStyle 同时插入 / 删除. styleId 传入后会为每个新字符增引用计数.
	// 调用方 *不需要* 手动 InvalidateLayout - 这两个里会通过 ParaOnTextInserted / ParaOnTextErased
	// 增量维护 m_paragraphs.
	void InsertCharsRaw(int pos, const wchar_t* p, int len, int styleId);
	void EraseCharsRaw(int pos, int len);

	// ===== 分段 layout 维护 =====
	// 释放所有段的 IDWriteTextLayout (m_paragraphs 结构本身保留, pLayout 置 NULL). 用于
	// 字号 / DPI 改变后需要 *全量重建 layout 但段边界不变* 的场景.
	void ParaReleaseAllLayouts();
	// 清空 m_paragraphs (含 layouts 释放). 用于 SetText / 析构.
	void ParaClear();
	// 从当前 m_text 完整重建段结构 (按 \n 切, 释放所有旧 layout). SetText / 加载时调.
	void ParaRebuildFromText();
	// 文本插入 hook. 调用前提: m_text 已经包含了新字符. 仅 *增量更新* m_paragraphs:
	// 找到 pos 所在段, 释放其 layout, 按 \n 重新切, 后续段 textStart 偏移 +len.
	void ParaOnTextInserted(int pos, int len);
	// 文本删除 hook. 调用前提: m_text 已经少了 len 个字符. 仅 *增量更新* m_paragraphs:
	// 找到删除范围跨越的段, 合并并按 \n 重新切, 后续段 textStart 偏移 -len.
	void ParaOnTextErased(int pos, int len);
	void ParaInvalidateLayoutsForRange(int pos, int len);
	// 确保段 idx 的 IDWriteTextLayout 已构建 (含 ApplyStylesToParagraph + GetMetrics 度量).
	// 已建则跳过. NULL pLayout = 之前被释放过 (新段, 或该段被编辑 invalidate).
	void EnsureParagraphLayout(int paraIdx);
	// 对段 idx 的 layout 应用文本样式 (字体/字号/字型/下划线/inline/颜色). 必须在该段 layout
	// 已 CreateTextLayout 之后调. 跟 EnsureParagraphLayout 配对.
	void ApplyStylesToParagraph(int paraIdx);
	// 累加各段 height 重算 yOffset + 全局 totalHeight / maxWidth. 段 layouts 必须已建.
	void RecomputeParaYOffsets();
	// 用绝对文本位置查找段索引 (二分). 返回 [0, m_paragraphs.size()-1] 或 -1 (空文档).
	// textPos 越界时 clamp 到最后一段 / 第一段.
	int  FindParagraphByTextPos(int textPos) const;
	// 用 layout-local y (物理像素) 查找段索引 (二分). 段高度可能为 0 (空段), 这种情况返
	// 最接近的非空段.
	int  FindParagraphByY(float yPhys) const;
	// 将嵌入的 UI 子元素按 layout 计算出的坐标设位. 在 OnPaintImpl 里 紧接 EnsureLayout 之后调.
	void PositionInlineObjects(float originX, float originY);
	// 字符串中某位是否是 inline 对象占位字符.
	bool IsInlinePos(int i) const;
//@隐藏}
};
//@分组}

//@隐藏{
DWORD XBlur_GetOsBuild();  // 进程内缓存 OS build, dcomp 模块共用
//@隐藏}

//@别名 取系统版本()
static inline int GetCurrentVersion() {
    DWORD build = XBlur_GetOsBuild();
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

///AttachToWndEx 路径选择. auto = dcomp > dwm 自动按 OS / 能力降级.
//@别名 模糊路径
enum xblur_path_
{
	//@别名 模糊路径_自动
	xblur_path_auto    = 0,
	//@别名 模糊路径_DCOMP合成
	xblur_path_dcomp   = 1,
	//@别名 模糊路径_DWM亚克力
	xblur_path_dwm     = 2,
};

///DWM 原生圆角预设 (CXBlur::EnableNativeRoundedCorner 参数).
///这些是 Windows 11 21H2+ DwmSetWindowAttribute(DWMWA_WINDOW_CORNER_PREFERENCE)
///的 4 个取值. 老系统 (Win10 / Win7) 调用静默失败, 本枚举值与 DWM
///原生 DWM_WINDOW_CORNER_PREFERENCE 枚举二进制一致 (0/1/2/3).
//@别名 原生圆角预设
enum xblur_corner_
{
	//@别名 原生圆角_默认
	xblur_corner_default     = 0,   // DWMWCP_DEFAULT     由 DWM 决定 (顶层窗 ≈ round)
	//@别名 原生圆角_不圆角
	xblur_corner_donotround  = 1,   // DWMWCP_DONOTROUND  强制直角
	//@别名 原生圆角_圆角
	xblur_corner_round       = 2,   // DWMWCP_ROUND       8 px (Win11 窗体默认)
	//@别名 原生圆角_小圆角
	xblur_corner_roundsmall  = 3,   // DWMWCP_ROUNDSMALL  4 px (菜单 / popup)
};

///主题预设默认参数 (CXBlur::SetThemeDefault / GetThemeDefault)
///v3.0 DWM acrylic 路径下只保留 tint + noise (两个应用可控参数).
///blur 强度/饱和/亮度/对比度 都由 DWM 系统决定, 应用层不控。
//@别名 模糊主题默认参数
struct CXBlurThemeDefaults
{
	//@别名 叠加色 ()
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

//@备注 一键挂载 dcomp acrylic 主导架构 (acrylic owner 子窗承载模糊, XCGUI 透明 owned 上层).
//      内部完整封装 PoC 流程: 进 layered 透明 → 创建 NOREDIRECTIONBITMAP acrylic 子窗 →
//      Apply effect chain → owner-owned + WS_EX_APPWINDOW → 装两 subclass 同步 →
//      Show acrylic + 系统圆角. 调用方仅一行调用即可拿到 Win11 Start Menu 同款 acrylic.
//
//      参数预设按当前 SetTheme/SetTintColor/SetNoise/SetUniformBrightness/SetBlurOpacity
//      的值用 (没显式 set 走主题默认: light=243,243,243 / blurOpacity=0.5 / sat=1.3 /
//      noise=1% ; dark=32,32,32 / blurOpacity=0.15 / sat=1.2 / noise=3% , uniformBright=TRUE).
//      要先 SetTintColor 等再调本接口, 否则用主题默认.
//
//      销毁: XCGUI 主窗销毁时自动清理 acrylic + dcomp 资源, 调用方不需手工.
//@参数 hWnd 目标 XCGUI 主窗.
//@参数 path xblur_path_auto / dcomp / dwm. auto 默认 = dcomp > dwm 按 OS 降级.
//@返回 TRUE 成功, FALSE 句柄非法 / 路径不支持.
//@别名  附加窗口扩展()
	BOOL AttachToWndEx(HWINDOW hWnd, int path = xblur_path_auto);

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

//@备注 启用 / 关闭"亮度锁定" (LuminosityBlend). 仅 *dcomp 路径* (Win10 1803+ 走
//      Windows.UI.Composition 直接合成时) 生效, 老路径 (ACCENT / DWM) 调用本接口
//      被静默忽略.
//
//      启用时 (默认 TRUE): 不论桌面背景深浅, 窗口都保持 tint 自身亮度,
//        只透出色相变化. 这是 Win11 Start Menu / Settings 标准观感.
//      关闭时 (FALSE): blur 亮度跟桌面同步起伏 (Win10 Aero / 老 acrylic 风),
//        视觉更"通透"但深背景下窗会变暗.
//
//      改值后立即重建 effect chain (跟 SetTintColor 一样需要 reapply).
//@参数 bEnable TRUE 锁亮度 (默认), FALSE 跟桌面起伏
//@别名  置亮度锁定()
	void SetUniformBrightness(BOOL bEnable);
//@别名  取亮度锁定()
	BOOL GetUniformBrightness() const;

//@备注 设置"模糊层通透感" — 控制 blur 通道相对 tint 的显示比例.
//      *仅 dcomp 路径生效* (Win10 1803+ 走 Windows.UI.Composition 自合成路径时).
//      老路径 (ACCENT_ACRYLIC / DWM_TRANSIENT / BLURBEHIND / DECORATIVE) 上调用
//      被静默忽略 (那些路径用 SetTintColor 的 alpha 控制等价行为).
//
//      取值范围 0.0 ~ 1.0:
//        0.0  完全不通透 — 仅 tint 颜色, 看不到背景 blur
//        0.15 PoC dark 默认 — tint 主导 85%, 背景隐约透出 15% (Win11 Start Menu 深色)
//        0.5  PoC light 默认 — tint / blur 各半 (Win11 Start Menu 浅色)
//        1.0  完全通透 — 仅 blur, 看不到 tint (有点像玻璃)
//
//      传 *负值* (例如 -1) → 还原为"按主题默认 + tintA 反算" (默认状态).
//
//      改值后立即重建 effect chain (跟 SetTintColor 一样需要 reapply).
//@参数 fOpacity blur 可见度 0.0~1.0; 负数 = 还原默认
//@别名  置模糊通透度()
	void SetBlurOpacity(float fOpacity);
//@别名  取模糊通透度()
	float GetBlurOpacity() const;

//@备注 当前运行环境是否支持系统级 acrylic / blur (Vista+ 任意一档即视为支持).
//      不支持时 (Win8/8.1) CXBlur 仅画 tint+border+圆角.
//@别名  是否支持系统亚克力()
	static BOOL IsSystemAcrylicSupported();

//@备注 当前 CXBlur 实例的系统 acrylic / blur 是否成功启用.
//@别名  是否已启用系统亚克力()
	BOOL IsSystemAcrylicEnabled() const;

//@备注 启用 / 关闭 DWM 原生窗外阴影 (DwmExtendFrameIntoClientArea).
//
//      *绝大多数 XCGUI 用户不需要本接口* ——————————————————
//      XCGUI 默认窗 (CXWindow / window_style_default) 自带 WS_THICKFRAME,
//      DWM 已自动给它画 drop shadow + 1 px 描边. 这种窗只要调用
//      EnableNativeRoundedCorner 设圆角就够了, 阴影自来.
//
//      本接口存在的唯一场景: *纯 WS_POPUP* (无 WS_THICKFRAME / WS_CAPTION)
//      的 borderless 窗 — DWM 不会自动给它画 shadow, 必须靠 frame extension
//      显式触发.
//
//      *代价 (DwmExtendFrameIntoClientArea 的固有副作用)*:
//        1. frame extension 区域 (1 px 边带) 在 resize 瞬间会闪出窗类
//           hbrBackground 默认色 (XCGUI 通常是白色). 深色主题上肉眼可见.
//           cloak 机制只能盖首帧, resize 期偶发白闪改不掉.
//        2. snap state 下 DWM frame 强制方角, EnableNativeRoundedCorner
//           的 corner pref 此时无效 — 表现为 "snap 中方角 + 阴影". 这是
//           Win11 DWM by design (frame extension 与 corner pref 在 snap
//           state 互斥).
//
//      *自动行为* (host 状态机):
//        * 真最大化 (IsZoomed): margins=0 屏蔽 shadow, 还原后恢复.
//        * 启动白闪缓解: 调用时若窗未 visible, 自动 DWMWA_CLOAK 150 ms.
//
//      本接口与 CXBlur 实例生命周期无关 —— 静态 HWND 级, *不* 在 Detach
//      时撤销, 调用方须显式传 FALSE 还原.
//
//      返回 TRUE = DWM 接受, FALSE = 句柄非法 / DWM 未启用 / 老 OS 不支持.
//@参数 hWnd     XCGUI 窗口句柄.
//@参数 bEnable  TRUE 启用, FALSE 还原.
//@别名  启用原生阴影()
	static BOOL EnableNativeShadow(HWINDOW hWnd, BOOL bEnable);

//@备注 启用 Win11 DWM 原生窗体圆角 (DwmSetWindowAttribute /
//      DWMWA_WINDOW_CORNER_PREFERENCE = 33). 仅 Win11 21H2+ (build >= 22000)
//      生效, 老系统调用返 E_INVALIDARG, 本函数透传 FALSE.
//
//      圆角由 DWM 在合成级别处理, 不动 SetWindowRgn / 位图, 与
//      ACCENT_ACRYLIC 兼容. 与 EnableNativeShadow 解耦 (二者状态机共享但
//      可独立 set).
//
//      *推荐用法 (默认 XCGUI 窗)*:
//        XCGUI 默认窗带 WS_THICKFRAME, DWM 会 *自动* 给它画 drop shadow,
//        所以一般 *只调本接口* 即可: 圆角 + 自动阴影 + 描边都有, 无白闪.
//        *不要* 再调 EnableNativeShadow (那是给纯 popup 窗用的, 调了会
//        引入白闪 + snap 方角, 详见其文档).
//
//      *自动行为* (host 状态机):
//        * 真最大化 (IsZoomed): 强制 DWMWCP_DONOTROUND, 还原后恢复 user pref.
//        * WM_SIZE / WM_WINDOWPOSCHANGED / WM_DPICHANGED: 重 set corner
//          pref, 保险 DWM 在状态切换时仍按 user pref 渲染.
//
//      *已知限制 (Win11 by design, 引 MS 官方文档)*:
//        原文: "By design, apps are not rounded when maximized, snapped,
//              running in a Virtual Machine (VM), running on a Windows
//              Virtual Desktop (WVD), or running as a Windows Defender
//              Application Guard (WDAG) window."
//        参考: https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-rounded-corners
//
//        即:
//          * 最大化 → 方角 + 无 shadow (Edge / Settings / Notepad 一致).
//          * snap (Aero Snap / Snap Layouts) → 方角 + 无 shadow + 无描边.
//            这是 Win11 让 snap 窗紧贴屏幕边 / 邻窗的设计选择.
//          * VM / 远程桌面 → DWM 不参与渲染, 圆角自然无.
//        本接口对上述状态的 set 调用 *会* 成功 (返 S_OK), 但 DWM 不渲染.
//        无任何接口 / DWM 私有 attribute 能 override 这个 by-design 行为.
//        若必须在 snap 也保留圆角/阴影, 唯一办法是放弃 DWM 路径全程自绘
//        (per-pixel alpha layering + SetWindowRgn), 代价见 MS 文档.
//
//      本接口同样是静态 HWND 级, *不* 在 Detach 时还原.
//
//      返回 TRUE = Win11+ 且 DWM 接受, FALSE = Win10- / DWM 未启用 / 句柄非法.
//@参数 hWnd          XCGUI 窗口句柄.
//@参数 cornerStyle   xblur_corner_* 枚举值.
//@别名  启用原生圆角()
	static BOOL EnableNativeRoundedCorner(HWINDOW hWnd, int cornerStyle);

//@备注 启用 / 禁用本窗的 Aero Snap. 默认 TRUE (启用, 与系统一致).
//
//      *动机*: Win11 snap 状态下 DWM by design 不画圆角 / 阴影 / 描边
//      (见 EnableNativeRoundedCorner 文档已知限制). 若你的 UI 不希望
//      切到 snap 状态破坏视觉, 可调本接口禁掉 snap.
//
//      *bEnable=FALSE 是 "字面禁 snap"*:
//        - strip WS_MAXIMIZEBOX → 消除拖窗到屏幕边时浮出的 snap preview
//                                   (半透蒙层 / Snap Layouts 飞出框).
//                                   *副作用*: 标题栏最大化按钮变灰 *不可点*.
//        - WM_WINDOWPOSCHANGING 几何过滤 → 检测目标矩形是否匹配 snap layout
//                                          (full / half / quarter), 是则设
//                                          SWP_NOMOVE | SWP_NOSIZE 阻止落位.
//                                          兜底, 即便 preview 漏出也拦.
//        - *不* 吞 SC_MAXIMIZE — 用户仍可通过键盘 Win+↑ / 程序化 ShowWindow
//                                  (SW_MAXIMIZE) / SetWindowPlacement /
//                                  WS_MAXIMIZE 创建属性 来最大化.
//
//        几何过滤里用 IsZoomed(hwnd) 区分 "真最大化" 和 "snap 全屏":
//        Win32 在派发 WINDOWPOSCHANGING 前已更新 WINDOWPLACEMENT.showCmd,
//        真最大化时 IsZoomed=true → 跳过过滤. 这覆盖所有真最大化路径.
//
//      *与 EnableMaximize 的关系*:
//        - 本接口 (EnableSnap(FALSE)) 让按钮变灰但保留 Win+↑ / API 通路.
//        - EnableMaximize(FALSE) 在此基础上额外吞 SC_MAXIMIZE → 拦键盘 Win+↑
//          + 双击标题栏 + 系统菜单 "最大化". (API 路径 ShowWindow 仍能用.)
//        - 二者共享 WS_MAXIMIZEBOX strip 状态: 只要任一为禁用就 strip, 二者
//          都启用才还原.
//
//      *副作用 / 限制*:
//        * snap 几何检测有 2 px 容差, 用户手动恰好 resize 到 1/2 屏 / 1/4
//          屏尺寸会被误拦. 概率极低 (要求 4 边都对齐 work area).
//        * 触摸板三指手势 / 屏幕投递的 snap 不走以上路径, 拦不住.
//          (Win 系统 hook, 接口层无法干预.)
//
//      本接口是静态 HWND 级, 与 EnableNativeShadow / EnableNativeRoundedCorner
//      / EnableMaximize 共用同一 hook + 状态机, *不* 在 Detach 时还原.
//      调用方须显式 EnableSnap(TRUE) 还原.
//
//      返回 TRUE = 设置成功, FALSE = 句柄非法.
//@参数 hWnd     XCGUI 窗口句柄.
//@参数 bEnable  TRUE 启用 (默认), FALSE 字面禁 snap (按钮灰, 保留 Win+↑/API).
//@别名  启用Snap()
	static BOOL EnableSnap(HWINDOW hWnd, BOOL bEnable);

//@备注 启用 / 禁用本窗的最大化能力. 默认 TRUE (启用, 跟随窗口 WS_MAXIMIZEBOX
//      原始状态; 与 XWnd_EnableMaxWindow 接口语义一致).
//
//      *bEnable=FALSE 实现*:
//        1. strip WS_MAXIMIZEBOX → 标题栏最大化按钮变灰 + Snap Layouts 悬
//           停飞出框消失.
//        2. WM_SYSCOMMAND 吞 SC_MAXIMIZE → 拦键盘 Win+Up + 双击标题栏 +
//           系统菜单 "最大化" + 程序化 ShowWindow(SW_MAXIMIZE).
//
//      *bEnable=TRUE (从 FALSE 切回)*: 还原 WS_MAXIMIZEBOX 到 strip 前状态
//      (若原本就没 MAXIMIZEBOX, 不改).
//
//      *与 EnableSnap 完全独立*. 见 EnableSnap 文档 "解耦" 章节.
//
//      本接口静态 HWND 级, *不* 在 Detach 时还原. 调用方须显式 EnableMaximize(TRUE)
//      还原.
//
//      返回 TRUE = 设置成功, FALSE = 句柄非法.
//@参数 hWnd     XCGUI 窗口句柄.
//@参数 bEnable  TRUE 启用最大化 (默认), FALSE 禁用最大化.
//@别名  启用最大化()
	static BOOL EnableMaximize(HWINDOW hWnd, BOOL bEnable);

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
	// AttachToWnd 临时改了宿主窗的 layout / overlayBorder, Detach 时还原.
	// 仅 attachToWindow 路径用; AttachToEle 路径不动这俩.
	bool   m_savedWndLayout      = TRUE;   // 原 XWnd_IsEnableLayout
	bool   m_savedWndOverlayBorder = FALSE; // 原 overlay border 状态 (XCGUI 无 getter, 假设默认 FALSE)
	bool   m_hasSavedWndLayout   = false;
	float  m_dpiScale           = 1.0f;

	// ===== 模糊参数 (DWM 路径只控装饰层) =====
	std::atomic<COLORREF> m_tintColor    {0};
	std::atomic<int>      m_theme        {xblur_theme_custom};
	std::atomic<float>    m_noise        {0.06f};
	// 仅 dcomp 路径生效: 是否锁亮度 (LuminosityBlend). TRUE=Win11 Start Menu 风,
	// 不论桌面深浅窗口稳定 tint 亮度; FALSE=Win10 Aero 风, 亮度跟桌面起伏.
	std::atomic<int>      m_uniformBrightness {1};   // BOOL 用 atomic<int> 避免对齐坑
	// 仅 dcomp 路径生效: blur 通道可见度 ("通透感"). 0..1.
	//   -1 = "未设, 按主题默认" (light=0.5, dark=0.15) 或按 tintA 反算
	//   0..1 = 用户显式覆盖
	// 用 atomic<int> 存 1000 倍整数 (毫单位) 避免 atomic<float> 对齐麻烦.
	std::atomic<int>      m_blurOpacityMilli {-1};   // -1 = unset
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
	// AttachToWndEx 绑定标志 — 走 dcomp acrylic owner 子窗路径. 跟老 ACCENT/element
	// 路径互斥. 用于 Setter 决定走哪个 reapply 路径.
	bool  m_attachedExDcomp = false;

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

	// AttachToWndEx 路径下 Setter 改了参数后重刷 dcomp effect chain.
	void ReapplyExEffects();
	void DetachExInternal();
	//@隐藏}
};
//@分组}

//@隐藏{
struct _xcgui_chat_node_;
struct _xcgui_chat_hit_;
//@隐藏}

///聊天气泡消息类型
//@别名 聊天气泡消息类型
enum xcgui_chat_item_type_
{
	//@别名 聊天气泡_气泡
	chat_item_type_bubble = 0,
	//@别名 聊天气泡_系统消息
	chat_item_type_system = 1,
	//@别名 聊天气泡_可点击消息
	chat_item_type_clickable = 2,
};

///聊天气泡插入类型
//@别名 聊天气泡插入类型
enum xcgui_chat_insert_type_
{
	//@别名 聊天气泡_接收者
	chat_insert_type_receiver = 0,
	//@别名 聊天气泡_发送者
	chat_insert_type_sender = 1,
};

///聊天气泡内容类型
//@别名 聊天气泡内容类型
enum xcgui_chat_content_type_
{
	//@别名 聊天气泡_文本
	chat_content_type_text = 0,
	//@别名 聊天气泡_图片
	chat_content_type_image = 1,
	//@别名 聊天气泡_文件
	chat_content_type_file = 2,
	//@别名 聊天气泡_语音
	chat_content_type_voice = 3,
	//@别名 聊天气泡_视频
	chat_content_type_video = 4,
	//@别名 聊天气泡_UI对象
	chat_content_type_object = 5,
};

///聊天气泡点击位置
//@别名 聊天气泡点击位置
enum xcgui_chat_click_part_
{
	//@别名 聊天气泡_头像
	chat_click_part_avatar = 1,
	//@别名 聊天气泡_名称
	chat_click_part_name = 2,
	//@别名 聊天气泡_标签
	chat_click_part_tag = 3,
	//@别名 聊天气泡_气泡位置
	chat_click_part_bubble = 4,
	//@别名 聊天气泡_新消息提醒
	chat_click_part_newMessage = 5,
	//@别名 聊天气泡_消息
	chat_click_part_message = 6,
	//@别名 聊天气泡_内嵌对象
	chat_click_part_object = 7,
};

///聊天气泡内容原子
//@别名 聊天气泡内容结构
struct xcgui_chat_content_
{
	//@别名 聊天气泡_内容类型
	int type;
	//@别名 聊天气泡_文本
	CXText text;
	//@别名 聊天气泡_路径
	CXText path;
	//@别名 聊天气泡_UI对象
	HXCGUI hObject;

	//@隐藏{
	xcgui_chat_content_(){ type = chat_content_type_text; hObject = NULL; }
	//@隐藏}
};

///聊天气泡消息数据
//@别名 聊天气泡消息结构
struct xcgui_chat_item_
{
	//@别名 聊天气泡_消息类型
	int itemType;
	//@别名 聊天气泡_插入类型
	int insertType;
	//@别名 聊天气泡_发送者ID
	CXText senderId;
	//@别名 聊天气泡_发送者名称
	CXText senderName;
	//@别名 聊天气泡_头像路径
	CXText avatarPath;
	//@别名 聊天气泡_标签列表
	CXVector<CXText> tags;
	//@别名 聊天气泡_内容列表
	CXVector<xcgui_chat_content_> contents;
	//@别名 聊天气泡_显示文本
	CXText displayText;
	//@别名 聊天气泡_消息ID
	CXText messageId;
	//@别名 聊天气泡_时间戳
	__int64 timestamp;
	//@别名 聊天气泡_用户数据
	vint userData;

	//@隐藏{
	xcgui_chat_item_(){ itemType = chat_item_type_bubble; insertType = chat_insert_type_receiver; timestamp = 0; userData = 0; }
	//@隐藏}
};

///聊天气泡点击事件回调
//@参数 hChat 聊天框元素句柄
//@参数 iItem 消息索引
//@参数 insertType 插入类型, 见 xcgui_chat_insert_type_
//@参数 part 点击位置, 见 xcgui_chat_click_part_
//@参数 iTag 标签索引, 非标签为 -1
//@参数 hSender 触发点击的元素句柄
//@参数 pbHandled 是否拦截
//@返回 返回事件结果
//@别名 聊天气泡点击事件
typedef int (WINAPI *xcgui_chat_click_event)(HELE hChat, int iItem, int insertType, int part, int iTag, HXCGUI hSender, BOOL* pbHandled);

///聊天气泡 UI 对象加载回调
//@参数 hChat 聊天框元素句柄
//@参数 iItem 消息索引
//@参数 iContent 该消息内容数组中的下标
//@参数 objType XC_OBJECT_TYPE
//@参数 hObject 重建出来的 UI 对象句柄
//@参数 pKey  InsertObjectEx 提供的标识键 (可空, 用于同类型多对象区分)
//@别名 聊天气泡对象加载事件
typedef void (WINAPI *xcgui_chat_object_loaded_event)(HELE hChat, int iItem, int iContent, int objType, HXCGUI hObject, const wchar_t* pKey);

//@分组{ 炫彩聊天气泡框
//@备注  继承: CXLayoutFrame, CXScrollView, CXEle, CXWidgetUI, CXObjectUI, CXBase
//@别名  炫彩聊天气泡框类
class CXChatBubbleBox : public CXLayoutFrame
{
public:
	//@隐藏{
	CXChatBubbleBox();
	virtual ~CXChatBubbleBox();
	//@隐藏}

//@备注 创建聊天气泡框。内部使用 CXLayoutFrame 作为滚动容器, 每条消息独占一行子容器。
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
//@返回 元素句柄
//@别名 创建()
	HELE Create(int x, int y, int cx, int cy, HXCGUI hParent = NULL);

//@备注 构造并创建聊天气泡框。
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
	CXChatBubbleBox(int x, int y, int cx, int cy, HXCGUI hParent = NULL){ Create(x, y, cx, cy, hParent); }

//@备注 清空所有聊天消息和运行时子元素, 不销毁聊天框本身。
//@别名 清空消息()
	void ClearMessages();

//@备注 销毁聊天框及内部子元素。
//@别名 销毁扩展()
	void DestroyChat();

//@备注 启用新消息定位提醒。启用后, 新消息到达且垂直滚动条未接近底部时不自动滚动到底部, 而显示右下角新消息数量。
//@参数 bEnable 是否启用
//@别名 启用新消息定位()
	void EnableNewMessageLocate(BOOL bEnable);

//@返回 是否启用新消息定位提醒。
//@别名 是否启用新消息定位()
	BOOL IsEnableNewMessageLocate() const;

//@备注 设置后续插入气泡的类型。发送者靠右, 接收者靠左。
//@参数 nType 插入类型, 见 xcgui_chat_insert_type_
//@别名 置插入类型()
	void SetInsertType(int nType);

//@返回 当前插入类型。
//@别名 取插入类型()
	int GetInsertType() const;

//@备注 设置/更新某个发送者的用户信息(名称+头像), 并把该 senderId 设为 *当前 SetInsertType 指
// 向方向* 的 active 发送者. 后续 InsertBubbleBegin 会按 active senderId 查找对应名称/头像/标签.
// 多次调用同一个 pSenderId 只更新该用户的资料, 不影响其他用户的标签/资料.
//@参数 pSenderId 发送者ID, 不可为空(空字符串视为匿名发送者)
//@参数 pSenderName 发送者名称
//@参数 pAvatarPath 头像图片路径, 可为空
//@别名 置插入用户信息()
	void SetInsertUserInfo(const wchar_t* pSenderId, const wchar_t* pSenderName, const wchar_t* pAvatarPath = NULL);

//@备注 清空指定发送者的标签缓存。
//@参数 pSenderId 发送者ID; 传 NULL 表示当前方向的 active 发送者
//@别名 清空插入标签()
	void ClearInsertTags(const wchar_t* pSenderId = NULL);

//@备注 给指定发送者追加一条标签。tag 与用户资料一样按 senderId 维度存储, 与 SetInsertType 方向
// 解耦: 切换方向不会丢失任意用户的标签, 顺序也不影响最终结果.
//@参数 pSenderId 发送者ID; 传 NULL 表示当前方向的 active 发送者
//@参数 pTag 标签文本
//@别名 添加插入标签()
	void AddInsertTag(const wchar_t* pSenderId, const wchar_t* pTag);
//@备注 旧版本兼容: 给当前方向的 active 发送者追加标签 (等价于 AddInsertTag(NULL, pTag)).
	void AddInsertTag(const wchar_t* pTag);

//@备注 开始插入一条气泡消息。之后可调用插入文本/图片/文件/语音等接口写入内容, 最后调用插入气泡结束。
//@返回 成功返回TRUE
//@别名 插入气泡开始()
	BOOL InsertBubbleBegin();

//@备注 开始插入一条气泡消息并临时指定名称和头像。
//@参数 pSenderName 发送者名称
//@参数 pAvatarPath 头像图片路径, 可为空
//@返回 成功返回TRUE
//@别名 插入气泡开始扩展()
	BOOL InsertBubbleBeginEx(const wchar_t* pSenderName, const wchar_t* pAvatarPath = NULL);

//@备注 结束当前气泡插入。
//@返回 成功返回TRUE
//@别名 插入气泡结束()
	BOOL InsertBubbleEnd();

//@备注 向当前气泡插入文本。
//@参数 pText 文本内容
//@返回 成功返回TRUE
//@别名 插入文本()
	BOOL InsertText(const wchar_t* pText);

//@备注 向当前气泡插入图片路径。图片会作为富文本图片对象插入, 路径会写入聊天数据。
//@参数 pImagePath 图片文件路径
//@返回 成功返回TRUE
//@别名 插入图片()
	BOOL InsertImage(const wchar_t* pImagePath);

//@备注 向当前气泡插入文件记录。界面中显示文件名占位文本, 路径写入聊天数据。
//@参数 pFilePath 文件路径
//@返回 成功返回TRUE
//@别名 插入文件()
	BOOL InsertFile(const wchar_t* pFilePath);

//@备注 向当前气泡插入语音记录。界面中显示语音占位文本, 路径写入聊天数据。
//@参数 pVoicePath 语音文件路径
//@返回 成功返回TRUE
//@别名 插入语音()
	BOOL InsertVoice(const wchar_t* pVoicePath);

//@备注 向当前气泡插入视频记录。界面中显示视频占位文本, 路径写入聊天数据。
//@参数 pVideoPath 视频文件路径
//@返回 成功返回TRUE
//@别名 插入视频()
	BOOL InsertVideo(const wchar_t* pVideoPath);

//@备注 向当前气泡插入 UI 对象句柄。对象需由调用方创建并保证适合作为编辑框富文本对象使用。
//@参数 hObject UI对象句柄
//@返回 成功返回TRUE
//@别名 插入对象()
	BOOL InsertObject(HXCGUI hObject);

//@备注 插入UI对象, 同时附带一个标识键。键随消息一起持久化, 加载回调里可凭此区分
//      同类型多对象 (例如同一气泡里有 2 个 XBtn, 用 key 区分 "确认" / "取消").
//@参数 hObject UI对象句柄
//@参数 pKey 标识字符串 (可空)
//@返回 成功返回TRUE
//@别名 插入对象扩展()
	BOOL InsertObjectEx(HXCGUI hObject, const wchar_t* pKey);

//@备注 把一个 CXEditDW 的内容按出现顺序整体追加到当前气泡 (须在 InsertBubbleBegin /
//      InsertBubbleEnd 之间调用)。文本走 InsertText, 图片走 InsertImage (用源中记录的
//      原始路径), UI 对象走 InsertObjectEx (会把对象从源 edit 摘出 reparent 到本气泡
//      的 edit).
//      注意: 此调用会把源中的 UI 对象转移到本气泡, 源 edit 将不再持有这些对象, 调用方
//      若希望源仍可用, 应先克隆一份再传入.
//@参数 pSrc 源 DirectWrite 编辑框
//@返回 成功返回TRUE
//@别名 插入DW编辑框内容()
	BOOL InsertFromEditDW(CXEditDW* pSrc);

//@备注 插入居中独占一行的普通提示消息。内部使用 XShapeText, 不触发点击事件。
//@参数 pText 提示文本
//@返回 成功返回TRUE
//@别名 插入消息()
	BOOL InsertMessage(const wchar_t* pText);

//@备注 插入居中独占一行的可点击消息 (例如时间、撤回提示、点击查看详情等)。
//@参数 pText 显示文本
//@返回 成功返回TRUE
//@别名 插入可点击消息()
	BOOL InsertClickableMessage(const wchar_t* pText);

//@备注 设置气泡最大宽度。传0表示按聊天框宽度自动取 62%。
//@参数 nWidth 最大宽度
//@别名 置气泡最大宽度()
	void SetBubbleMaxWidth(int nWidth);

//@返回 当前气泡最大宽度设置。
//@别名 取气泡最大宽度()
	int GetBubbleMaxWidth() const;

//@备注 设置气泡内容缩进。
//@参数 nIndentation 缩进值
//@别名 置气泡缩进()
	void SetBubbleIndentation(int nIndentation);

//@返回 当前气泡内容缩进。
//@别名 取气泡缩进()
	int GetBubbleIndentation() const;

//@备注 设置头像尺寸。
//@参数 nSize 头像宽高
//@别名 置头像大小()
	void SetAvatarSize(int nSize);

//@备注 设置消息行间距 (相邻两条消息之间的垂直间隔, 默认 16)。
//@参数 nSpace 行间距
//@别名 置行间距()
	void SetMessageSpace(int nSpace);

//@返回 当前消息行间距。
//@别名 取行间距()
	int GetMessageSpace() const;

//@备注 设置内容区右内边距 (内容与垂直滚动条之间的留白, 默认 8)。
//@参数 nPadding 右内边距像素
//@别名 置内容右内边距()
	void SetContentRightPadding(int nPadding);

//@返回 当前内容区右内边距。
//@别名 取内容右内边距()
	int GetContentRightPadding() const;

//@备注 设置气泡颜色。
//@参数 senderColor 发送者气泡颜色
//@参数 receiverColor 接收者气泡颜色
//@别名 置气泡颜色()
	void SetBubbleColor(COLORREF senderColor, COLORREF receiverColor);

//@返回 发送者气泡颜色。
//@别名 取发送者气泡颜色()
	COLORREF GetSenderBubbleColor() const;

//@返回 接收者气泡颜色。
//@别名 取接收者气泡颜色()
	COLORREF GetReceiverBubbleColor() const;

//@备注 设置气泡内文本颜色。CXEditDW 富文本默认颜色, 富文本本身的局部颜色优先级更高。
//@参数 senderColor 发送者文本颜色
//@参数 receiverColor 接收者文本颜色
//@别名 置气泡文本颜色()
	void SetBubbleTextColor(COLORREF senderColor, COLORREF receiverColor);

//@返回 发送者气泡文本颜色。
//@别名 取发送者气泡文本颜色()
	COLORREF GetSenderBubbleTextColor() const;

//@返回 接收者气泡文本颜色。
//@别名 取接收者气泡文本颜色()
	COLORREF GetReceiverBubbleTextColor() const;

//@备注 设置气泡内文本字号 (pt). 0 表示沿用 CXEditDW 默认 (14pt)。
//@参数 senderSize 发送者气泡字号
//@参数 receiverSize 接收者气泡字号
//@别名 置气泡字号()
	void SetBubbleFontSize(int senderSize, int receiverSize);

//@返回 发送者气泡字号 (0=默认)。
//@别名 取发送者气泡字号()
	int GetSenderBubbleFontSize() const;

//@返回 接收者气泡字号 (0=默认)。
//@别名 取接收者气泡字号()
	int GetReceiverBubbleFontSize() const;

//@备注 设置发送方+接收方气泡圆角(四角统一, 两边相同)。
//@参数 nRound 圆角像素值
//@别名 置气泡圆角()
	void SetBubbleRound(int nRound);

//@返回 发送方气泡圆角(取左上角)。
//@别名 取气泡圆角()
	int GetBubbleRound() const;

//@备注 设置发送方+接收方气泡圆角扩展(两边相同)。顺序 leftTop / rightTop / rightBottom / leftBottom。
//@参数 leftTop 左上圆角
//@参数 rightTop 右上圆角
//@参数 rightBottom 右下圆角
//@参数 leftBottom 左下圆角
//@别名 置气泡圆角扩展()
	void SetBubbleRoundEx(int leftTop, int rightTop, int rightBottom, int leftBottom);

//@备注 取发送方气泡圆角扩展(自定义四角)。任一参数为NULL表示忽略。
//@参数 pLeftTop 输出左上
//@参数 pRightTop 输出右上
//@参数 pRightBottom 输出右下
//@参数 pLeftBottom 输出左下
//@别名 取气泡圆角扩展()
	void GetBubbleRoundEx(int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom) const;

//@备注 单独设置发送方气泡圆角(四角统一)。
//@参数 nRound 圆角像素值
//@别名 置发送者气泡圆角()
	void SetSenderBubbleRound(int nRound);

//@备注 单独设置发送方气泡圆角扩展。
//@别名 置发送者气泡圆角扩展()
	void SetSenderBubbleRoundEx(int leftTop, int rightTop, int rightBottom, int leftBottom);

//@返回 发送方气泡圆角(取左上)。
//@别名 取发送者气泡圆角()
	int GetSenderBubbleRound() const;

//@备注 取发送方气泡四角圆角。
//@别名 取发送者气泡圆角扩展()
	void GetSenderBubbleRoundEx(int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom) const;

//@备注 单独设置接收方气泡圆角(四角统一)。
//@参数 nRound 圆角像素值
//@别名 置接收者气泡圆角()
	void SetReceiverBubbleRound(int nRound);

//@备注 单独设置接收方气泡圆角扩展。
//@别名 置接收者气泡圆角扩展()
	void SetReceiverBubbleRoundEx(int leftTop, int rightTop, int rightBottom, int leftBottom);

//@返回 接收方气泡圆角(取左上)。
//@别名 取接收者气泡圆角()
	int GetReceiverBubbleRound() const;

//@备注 取接收方气泡四角圆角。
//@别名 取接收者气泡圆角扩展()
	void GetReceiverBubbleRoundEx(int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom) const;

//@备注 设置头像圆角. 默认 18 (与默认头像大小 36 配合呈现圆形)。
//@参数 nRound 圆角像素值
//@别名 置头像圆角()
	void SetAvatarRound(int nRound);

//@返回 头像圆角。
//@别名 取头像圆角()
	int GetAvatarRound() const;

//@备注 设置系统消息与可点击消息文本颜色。
//@参数 messageColor 系统消息颜色
//@参数 clickableColor 可点击消息颜色
//@别名 置提示颜色()
	void SetHintTextColor(COLORREF messageColor, COLORREF clickableColor);

//@备注 设置 InsertMessage 系统消息文本颜色。
//@参数 color 文本颜色
//@别名 置消息文本颜色()
	void SetMessageTextColor(COLORREF color);

//@返回 InsertMessage 系统消息文本颜色。
//@别名 取消息文本颜色()
	COLORREF GetMessageTextColor() const;

//@备注 设置 InsertClickableMessage 文本颜色。
//@参数 color 文本颜色
//@别名 置可点击消息文本颜色()
	void SetClickableTextColor(COLORREF color);

//@返回 InsertClickableMessage 文本颜色。
//@别名 取可点击消息文本颜色()
	COLORREF GetClickableTextColor() const;

//@备注 设置发送/接收方 名称按钮 文本颜色。
//@参数 senderColor 发送方名称颜色
//@参数 receiverColor 接收方名称颜色
//@别名 置名称文本颜色()
	void SetNameTextColor(COLORREF senderColor, COLORREF receiverColor);

//@返回 发送方名称文本颜色。
//@别名 取发送方名称文本颜色()
	COLORREF GetSenderNameTextColor() const;

//@返回 接收方名称文本颜色。
//@别名 取接收方名称文本颜色()
	COLORREF GetReceiverNameTextColor() const;

//@备注 设置发送/接收方 标签按钮 文本颜色。
//@参数 senderColor 发送方标签颜色
//@参数 receiverColor 接收方标签颜色
//@别名 置标签文本颜色()
	void SetTagTextColor(COLORREF senderColor, COLORREF receiverColor);

//@返回 发送方标签文本颜色。
//@别名 取发送方标签文本颜色()
	COLORREF GetSenderTagTextColor() const;

//@返回 接收方标签文本颜色。
//@别名 取接收方标签文本颜色()
	COLORREF GetReceiverTagTextColor() const;

//@备注 设置发送/接收方 名称按钮 字号 (pt). 传 0 表示使用 XCGUI 默认字体。
//@参数 senderSize 发送方名称字号
//@参数 receiverSize 接收方名称字号
//@别名 置名称字号()
	void SetNameFontSize(int senderSize, int receiverSize);

//@返回 发送方名称字号 (0=默认)。
//@别名 取发送方名称字号()
	int GetSenderNameFontSize() const;

//@返回 接收方名称字号 (0=默认)。
//@别名 取接收方名称字号()
	int GetReceiverNameFontSize() const;

//@备注 设置发送/接收方 标签按钮 字号 (pt). 传 0 表示使用 XCGUI 默认字体。
//@参数 senderSize 发送方标签字号
//@参数 receiverSize 接收方标签字号
//@别名 置标签字号()
	void SetTagFontSize(int senderSize, int receiverSize);

//@返回 发送方标签字号 (0=默认)。
//@别名 取发送方标签字号()
	int GetSenderTagFontSize() const;

//@返回 接收方标签字号 (0=默认)。
//@别名 取接收方标签字号()
	int GetReceiverTagFontSize() const;

//@备注 获取 "新消息" 提醒按钮元素句柄, 可用于自定义文本/背景/位置等.
//@返回 按钮元素句柄, 未创建时返回 NULL。
//@别名 取新消息按钮()
	HELE GetNewMessageButton() const;

//@备注 设置系统消息 (InsertMessage) 字号 (pt). 传 0 表示使用 XCGUI 默认字体。
//@参数 size 字号
//@别名 置系统消息字号()
	void SetMessageFontSize(int size);

//@返回 系统消息字号 (0=默认)。
//@别名 取系统消息字号()
	int GetMessageFontSize() const;

//@备注 设置可点击消息 (InsertClickableMessage) 字号 (pt). 传 0 表示使用 XCGUI 默认字体。
//@参数 size 字号
//@别名 置可点击消息字号()
	void SetClickableFontSize(int size);

//@返回 可点击消息字号 (0=默认)。
//@别名 取可点击消息字号()
	int GetClickableFontSize() const;

//@备注 设置标签按钮三态圆角填充 (BkInfo). 文本色仍由 SetTagColors 控制 (三态共用)。
//@参数 leaveFill 离开状态填充色
//@参数 stayFill 鼠标停留状态填充色
//@参数 downFill 鼠标按下状态填充色
//@参数 round 圆角半径
//@别名 置标签背景()
	void SetTagBkStyle(COLORREF leaveFill, COLORREF stayFill, COLORREF downFill, int round);

//@备注 取标签按钮三态填充与圆角.
//@参数 leaveFill [out] 离开
//@参数 stayFill  [out] 停留
//@参数 downFill  [out] 按下
//@参数 round     [out] 圆角
//@别名 取标签背景()
	void GetTagBkStyle(COLORREF& leaveFill, COLORREF& stayFill, COLORREF& downFill, int& round) const;

//@备注 设置可点击消息三态圆角填充 (BkInfo). 文本色仍由 SetClickableTextColor 控制。
//@别名 置可点击消息背景()
	void SetClickableBkStyle(COLORREF leaveFill, COLORREF stayFill, COLORREF downFill, int round);

//@备注 取可点击消息三态填充与圆角.
//@别名 取可点击消息背景()
	void GetClickableBkStyle(COLORREF& leaveFill, COLORREF& stayFill, COLORREF& downFill, int& round) const;

//@备注 取所有聊天数据, 包含文本/图片路径/文件/语音/视频/UI对象句柄等记录。
//@参数 out 输出聊天数据
//@返回 成功返回TRUE
//@别名 取聊天数据()
	BOOL GetChatData(CXVector<xcgui_chat_item_>& out) const;

//@备注 使用聊天数据恢复界面。加载前会清空当前消息。
//@参数 data 聊天数据
//@返回 成功返回TRUE
//@别名 置聊天数据()
	BOOL SetChatData(const CXVector<xcgui_chat_item_>& data);

//@备注 把当前聊天记录序列化到内存 (二进制, 仅数据, 不含样式 / UI 对象句柄;
//图片/视频/语音/文件原子只保存 path, 加载方可按 path 复用磁盘资源).
//@参数 out 输出字节集
//@返回 成功返回TRUE
//@别名 保存到内存()
	BOOL SaveToMem(CXBytes& out) const;

//@备注 从内存反序列化聊天记录, 加载前会清空当前消息。
//@参数 data 二进制数据起始
//@参数 size 二进制数据字节长度
//@返回 成功返回TRUE
//@别名 读取内存()
	BOOL LoadFromMem(const void* data, size_t size);

//@备注 把当前聊天记录保存到文件 (二进制).
//@参数 pPath 文件全路径
//@返回 成功返回TRUE
//@别名 保存到文件()
	BOOL SaveToFile(const wchar_t* pPath) const;

//@备注 从文件读取聊天记录, 加载前会清空当前消息。
//@参数 pPath 文件全路径
//@返回 成功返回TRUE
//@别名 读取文件()
	BOOL LoadFromFile(const wchar_t* pPath);

//@备注 取当前正在插入的气泡内部 CXEditDW 编辑框句柄。
//@返回 编辑框元素句柄, 没有当前气泡返回NULL
//@别名 取当前编辑框()
	HELE GetCurrentEdit();

//@备注 取指定消息的气泡内部 CXEditDW 编辑框句柄。
//@参数 iItem 消息索引
//@返回 编辑框元素句柄, 非气泡消息返回NULL
//@别名 取消息编辑框()
	HELE GetItemEdit(int iItem);

//@备注 取新消息提醒数量。
//@返回 新消息数量
//@别名 取新消息数量()
	int GetNewMessageCount() const;

//@备注 清空并隐藏新消息提醒。
//@别名 清空新消息提醒()
	void ClearNewMessageCount();

//@备注 滚动到底部并清空新消息提醒。
//@别名 定位到最新消息()
	void LocateNewMessage();

//@备注 设置鼠标左键点击头像/名称/标签/气泡事件回调。
//@参数 pFun 回调函数
//@别名 置左键点击事件()
	void SetLButtonClickEvent(xcgui_chat_click_event pFun);

//@备注 设置鼠标右键点击头像/名称/标签/气泡事件回调。
//@参数 pFun 回调函数
//@别名 置右键点击事件()
	void SetRButtonClickEvent(xcgui_chat_click_event pFun);

//@备注 设置 UI 对象加载回调。LoadFromMem / LoadFromFile / SetChatData 重建每个
//      UI 对象后, 在已 reparent 到气泡 edit / 已绑定默认点击事件之后触发一次, 调用
//      方可在此为对象补样式 / 注册业务事件 / 按 pKey 区分同类型多对象。
//@参数 pFun 回调函数
//@别名 置对象加载事件()
	void SetObjectLoadedEvent(xcgui_chat_object_loaded_event pFun);

private:
	int OnSizeImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled);
	int OnMouseWheelImpl(HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled);
	int OnDestroyEndImpl(HELE hEle, BOOL* pbHandled);
	int OnBubblePaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int OnLButtonUpImpl(HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled);
	int OnRButtonUpImpl(HELE hEle, UINT nFlags, POINT* pPt, BOOL* pbHandled);

	void InstallFrameEvents();
	void ReleaseNodes(BOOL bDestroyRows);
	void BuildNode(_xcgui_chat_node_* pNode);
	void AppendContentToEdit(_xcgui_chat_node_* pNode, const xcgui_chat_content_& content);
	void RelayoutAll(BOOL bRedraw = TRUE);
	void RelayoutNode(_xcgui_chat_node_* pNode, int y, int width, int* pOutHeight);
	void UpdateTotalSize(int totalHeight);
	void UpdateNewMessageState();
	void PositionNewMessageButton();
	void ShowNewMessageButton(BOOL bShow);
	void FinishAppendScrollPolicy(BOOL bWasNearBottom);
	BOOL CaptureNearBottom() const;   // 在 RelayoutAll 前抓拍 "滚动条是否贴底" — 之后 SetTotalSize 会让 posY 归零
	void BindHit(HELE hEle, _xcgui_chat_node_* pNode, int part, int iTag);
	_xcgui_chat_hit_* GetHit(HELE hEle);
	_xcgui_chat_node_* GetNode(int iItem) const;
	int GetViewClientWidth() const;
	int GetViewClientHeight() const;
	int GetEffectiveBubbleMaxWidth(int viewWidth) const;
	SIZE MeasureBubbleContent(const _xcgui_chat_node_* pNode, int maxContentWidth) const;
	int TextVisualWidth(const wchar_t* pText, int* pLineCount) const;
	int TagWidth(const wchar_t* pText) const;
	CXText PathFileName(const wchar_t* pPath) const;
	BOOL InsertPathContent(int nType, const wchar_t* pPath);

private:
	std::vector<_xcgui_chat_node_*> m_nodes;
	std::vector<_xcgui_chat_hit_*> m_hits;
	_xcgui_chat_node_* m_currentNode;
	HELE m_hNewMsgButton;
	BOOL m_enableNewMessageLocate;
	BOOL m_forceScrollOnNextEnd;   // InsertFromEditDW 用: 下一次 InsertBubbleEnd 不走 nearBottom 判断, 强制滚到底
	BOOL m_inSizeImpl;             // OnSizeImpl 重入哨兵: 内部 XEle_AdjustLayout 会再次触发 XE_ADJUSTLAYOUT_END, 防止递归爆栈
	int m_newMessageCount;
	int m_insertType;
	// 用户资料 + 标签按 senderId 维度存储, 不与 SetInsertType 方向耦合: 每个 senderId 独立持
	// 名称 / 头像 / 标签三件套, 多次切换 SetInsertType 不会丢失任意用户的资料 / 标签, 调用顺序
	// 也无关. 仅记录两个方向各自的 active senderId, InsertBubbleBegin 据此查表.
	struct _xcchat_user_profile_ {
		CXText senderId;
		CXText senderName;
		CXText avatarPath;
		CXVector<CXText> tags;
		// 标签 "已消费" 标志: InsertBubbleBegin 把 tags 复制到气泡数据后置 TRUE; 下次
		// AddInsertTag 检测到 TRUE 时把 tags 清空再追加, 并复位为 FALSE. 这样两种调用模式
		// 同时被支持:
		//   A. 设一次标签 + N 次 InsertBubble: 不再调 AddInsertTag, 标签保留;
		//   B. 循环 (AddInsertTag*M + InsertBubble) * N: 每轮首个 AddInsertTag 自动清空前次
		//      残留, 不会无限累积.
		// SetInsertUserInfo / ClearInsertTags / SetChatData 等显式接口不依赖这个标志.
		BOOL bConsumed;
		_xcchat_user_profile_() : bConsumed(FALSE) {}
	};
	std::vector<_xcchat_user_profile_> m_userProfiles;
	CXText m_activeSenderIdSender;     // SetInsertType=sender 时的 active senderId
	CXText m_activeSenderIdReceiver;   // SetInsertType=receiver 时的 active senderId
	// 内部辅助: 按 senderId 查找用户资料, 若 bCreate 为 TRUE 时找不到则新建并返回引用.
	_xcchat_user_profile_* _findUserProfile(const wchar_t* pSenderId, BOOL bCreate);
	int m_bubbleMaxWidth;
	int m_bubbleIndentation;
	int m_avatarSize;
	int m_messageSpace;
	int m_rowPadding;
	int m_contentRightPadding;
	int m_metaHeight;
	int m_lineHeight;
	COLORREF m_senderBubbleColor;
	COLORREF m_receiverBubbleColor;
	COLORREF m_senderTextColor;
	COLORREF m_receiverTextColor;
	int m_senderBubbleFontSize;
	int m_receiverBubbleFontSize;
	COLORREF m_messageTextColor;
	COLORREF m_clickableTextColor;
	// 发送方/接收方各自一组 4 角圆角. 旧接口 SetBubbleRound / SetBubbleRoundEx 同时写两套
	// 保持向后兼容; 新接口 SetSenderBubbleRound* / SetReceiverBubbleRound* 单独控制.
	int m_senderBubbleRoundLT;
	int m_senderBubbleRoundRT;
	int m_senderBubbleRoundRB;
	int m_senderBubbleRoundLB;
	int m_receiverBubbleRoundLT;
	int m_receiverBubbleRoundRT;
	int m_receiverBubbleRoundRB;
	int m_receiverBubbleRoundLB;
	int m_avatarRound;
	COLORREF m_senderNameColor;
	COLORREF m_receiverNameColor;
	COLORREF m_senderTagColor;
	COLORREF m_receiverTagColor;
	int m_senderNameFontSize;
	int m_receiverNameFontSize;
	int m_senderTagFontSize;
	int m_receiverTagFontSize;
	HFONTX m_senderNameFont;
	HFONTX m_receiverNameFont;
	HFONTX m_senderTagFont;
	HFONTX m_receiverTagFont;
	int m_messageFontSize;
	int m_clickableFontSize;
	HFONTX m_messageFont;
	HFONTX m_clickableFont;
	COLORREF m_tagFillLeave;
	COLORREF m_tagFillStay;
	COLORREF m_tagFillDown;
	int m_tagRound;
	COLORREF m_clickableFillLeave;
	COLORREF m_clickableFillStay;
	COLORREF m_clickableFillDown;
	int m_clickableRound;
	xcgui_chat_click_event m_leftClickEvent;
	xcgui_chat_click_event m_rightClickEvent;
	xcgui_chat_object_loaded_event m_objectLoadedEvent;
};
//@分组}

///<折叠面板展开模式
//@别名 折叠面板展开模式
enum xaccordion_expand_mode_
{
	//@别名 折叠面板展开模式_单选组内
	xaccordion_expand_mode_single        = 0, ///<同组仅一项展开 (默认); 标题 check, 展开时自动收起同组其它项
	//@别名 折叠面板展开模式_多项
	xaccordion_expand_mode_multiple      = 1, ///<同组可多项展开; 标题 check, 点击可展开/收起
	//@别名 折叠面板展开模式_全局单选
	xaccordion_expand_mode_single_global = 2, ///<全局仅一项展开; 标题 check, 展开时自动收起其它组项
};

///<折叠面板内容类型
//@别名 折叠面板内容类型
enum xaccordion_content_type_
{
	//@别名 折叠面板内容类型_无
	xaccordion_content_none    = 0,
	//@别名 折叠面板内容类型_文本
	xaccordion_content_text    = 1,
	//@别名 折叠面板内容类型_元素
	xaccordion_content_element = 2,
};

///<折叠面板图标类型
//@别名 折叠面板图标类型
enum xaccordion_icon_type_
{
	//@别名 折叠面板图标类型_无
	xaccordion_icon_none            = 0,
	//@别名 折叠面板图标类型_自定义
	xaccordion_icon_custom          = 1,
	//@别名 折叠面板图标类型_状态完成
	xaccordion_icon_status_done     = 2,
	//@别名 折叠面板图标类型_状态进行中
	xaccordion_icon_status_progress = 3,
	//@别名 折叠面板图标类型_状态未开始
	xaccordion_icon_status_todo     = 4,
};

///<折叠面板徽章语义
//@别名 折叠面板徽章类型
enum xaccordion_badge_kind_
{
	//@别名 折叠面板徽章类型_中性
	xaccordion_badge_neutral = 0,
	//@别名 折叠面板徽章类型_成功
	xaccordion_badge_success = 1,
	//@别名 折叠面板徽章类型_警告
	xaccordion_badge_warning = 2,
	//@别名 折叠面板徽章类型_信息
	xaccordion_badge_info    = 3,
	//@别名 折叠面板徽章类型_危险
	xaccordion_badge_danger  = 4,
};

///<折叠面板展开指示样式
//@别名 折叠面板指示样式
enum xaccordion_indicator_style_
{
	//@别名 折叠面板指示样式_文本
	xaccordion_indicator_text   = 0, ///<+ / - 文本 (默认)
	//@别名 折叠面板指示样式_箭头
	xaccordion_indicator_chevron = 1, ///<上下箭头字符占位
};

///<折叠面板分组标题对齐
//@别名 折叠面板分组标题对齐
enum xaccordion_group_title_align_
{
	//@别名 折叠面板分组标题对齐_左
	xaccordion_group_title_align_left   = 0, ///<左对齐 (默认)
	//@别名 折叠面板分组标题对齐_中
	xaccordion_group_title_align_center = 1, ///<水平居中
	//@别名 折叠面板分组标题对齐_右
	xaccordion_group_title_align_right  = 2, ///<右对齐
};

//@备注 项展开/收起/点击事件. pbHandled=TRUE 可阻止默认切换 (仅 OnItemClick).
typedef int (CALLBACK* xaccordion_item_event)(CXAccordion* pAccordion, int nItemId, BOOL* pbHandled);
//@备注 主题变更后通知
typedef int (CALLBACK* xaccordion_void_event)(CXAccordion* pAccordion);

struct _XAcc_ThemeColors;
struct _XAcc_ItemState;
struct _XAcc_GroupState;

//@分组{ 炫彩折叠面板
//@备注  继承: CXLayoutFrame, CXScrollView, CXEle, CXWidgetUI, CXObjectUI, CXBase
//       FAQ / 设置分组 / 引导清单用折叠面板容器. 支持分组、左图标、右徽章、文本或元素内容、
//       展开动画与 xuitool_theme_ 深/浅主题.
//@别名  炫彩折叠面板类
class CXAccordion : public CXLayoutFrame
{
public:
	//@隐藏{
	CXAccordion();
	virtual ~CXAccordion();
	//@隐藏}

//@备注 创建折叠面板根容器; 尺寸由 layout_size_weight 填满父容器, 勿传 rect.
//@参数 hParent 父容器 (HELE 或 HWINDOW)
//@返回 根元素句柄
//@别名  创建()
	HELE Create(HXCGUI hParent = NULL);

//@备注 主动销毁根布局框架; C++ 状态与字体在 XE_DESTROY / XE_DESTROY_END 中清理.
//@别名  销毁扩展()
	void DestroyAccordion();

//@返回 根元素是否有效.
//@别名  是否有效()
	BOOL IsValid() const;

//@返回 根元素句柄.
//@别名  取句柄()
	HELE GetHandle() const;

	// ===== 主题 =====

//@备注 设置主题 (深/浅/自定义/自动).
//@别名  置主题()
	void SetTheme(xuitool_theme_ theme);

//@返回 当前主题.
//@别名  取主题()
	xuitool_theme_ GetTheme() const;

//@备注 custom 模式文本色.
//@别名  置文本颜色()
	void SetTextColor(COLORREF c);

//@备注 custom 模式卡片背景色.
//@别名  置背景颜色()
	void SetBkColor(COLORREF c);

//@备注 custom 模式强调色.
//@别名  置强调颜色()
	void SetAccentColor(COLORREF c);

//@备注 卡片圆角, 默认 8.
//@别名  置圆角()
	void SetCornerRadius(int r);

	// ===== 行为 =====

//@备注 展开互斥策略 (标题均为 check; 单选模式由代码 CollapseOthers 实现).
//@别名  置展开模式()
	void SetExpandMode(xaccordion_expand_mode_ mode);

//@返回 展开模式.
//@别名  取展开模式()
	xaccordion_expand_mode_ GetExpandMode() const;

//@备注 是否允许多项同时展开. FALSE=组内单选 (默认), TRUE=组内多选.
//@别名  置允许多项展开()
	void SetAllowMultipleExpand(BOOL bAllow);

//@返回 是否允许多项同时展开.
//@别名  是否允许多项展开()
	BOOL IsAllowMultipleExpand() const;

//@备注 是否启用展开/收起动画.
//@别名  启用动画()
	void SetAnimEnabled(BOOL bEnable);

//@备注 动画时长 (毫秒), 默认 220.
//@别名  置动画时长()
	void SetAnimDuration(int ms);

//@备注 右侧展开指示样式 (+/- 或箭头).
//@别名  置指示样式()
	void SetIndicatorStyle(xaccordion_indicator_style_ style);

//@备注 内容超出时启用垂直滚动.
//@别名  启用滚动()
	void EnableScroll(BOOL bEnable);

//@备注 构建完分组/项后调用, 对最外层执行 XEle_AdjustLayout.
//@别名  调整布局()
	void AdjustLayout();

	// ===== 分组 =====

//@备注 添加分组; pTitle 可为空.
//@返回 分组 ID (>0)
//@别名  添加分组()
	int AddGroup(const wchar_t* pTitle = NULL);

//@备注 设置全部分组标题的水平对齐; 已创建的分组会立即更新.
//@别名  置分组标题对齐()
	void SetGroupTitleAlign(xaccordion_group_title_align_ align);

//@返回 当前分组标题对齐.
//@别名  取分组标题对齐()
	xaccordion_group_title_align_ GetGroupTitleAlign() const;

//@备注 设置组标题.
//@别名  置分组标题()
	BOOL SetGroupTitle(int groupId, const wchar_t* pTitle);

//@备注 禁用整组; 组内项不可展开, 已展开项自动收起.
//@别名  置分组启用()
	BOOL SetGroupEnabled(int groupId, BOOL bEnabled);

//@返回 分组是否启用.
//@别名  是否分组启用()
	BOOL IsGroupEnabled(int groupId) const;

//@备注 删除组及组内全部项.
//@别名  删除分组()
	BOOL RemoveGroup(int groupId);

//@备注 清空全部分组与项.
//@别名  清空分组()
	void ClearGroups();

//@返回 分组数量.
//@别名  取分组数量()
	int GetGroupCount() const;

	// ===== 项 =====

//@备注 添加项 (默认左侧 3.svg 图标).
//@返回 项 ID (>0)
//@别名  添加项()
	int AddItem(int groupId, const wchar_t* pTitle, xaccordion_content_type_ type = xaccordion_content_none);

//@备注 添加项. hIcon: NULL=无图标; 非空=自定义 HIMAGE.
//@返回 项 ID (>0)
//@别名  添加项带图标()
	int AddItem(int groupId, const wchar_t* pTitle, xaccordion_content_type_ type, HIMAGE hIcon);

//@备注 删除项.
//@别名  删除项()
	BOOL RemoveItem(int itemId);

//@备注 设置项标题.
//@别名  置项标题()
	BOOL SetItemTitle(int itemId, const wchar_t* pTitle);

//@备注 设置文本内容 (自动切换为 text 模式).
//@别名  置项正文()
	BOOL SetItemBodyText(int itemId, const wchar_t* pText);

//@备注 设置元素内容; 仅 Reparent 到项内容区, 不修改元素布局; 生命周期由调用方管理.
//@别名  置项内容元素()
	BOOL SetItemContentEle(int itemId, HELE hEle);

//@备注 清空项内容.
//@别名  清空项内容()
	BOOL ClearItemContent(int itemId);

//@备注 元素内容最小高度 (避免动画测量抖动).
//@别名  置项内容最小高度()
	BOOL SetItemContentMinHeight(int itemId, int h);

//@备注 设置左侧图标 (枚举兼容: none=无图标, 其它=默认 3.svg).
//@别名  置项图标()
	BOOL SetItemIcon(int itemId, xaccordion_icon_type_ type, HSVG hSvg = NULL);

//@备注 设置左侧图标图片. NULL=无图标; 非空=自定义 HIMAGE.
//@别名  置项图标图片()
	BOOL SetItemIconImage(int itemId, HIMAGE hIcon);

//@备注 设置右侧徽章; pText 为空则隐藏.
//@别名  置项徽章()
	BOOL SetItemBadge(int itemId, const wchar_t* pText, xaccordion_badge_kind_ kind = xaccordion_badge_neutral);

//@备注 禁用后不可展开 (若所属组已禁用同样不可操作).
//@别名  置项启用()
	BOOL SetItemEnabled(int itemId, BOOL bEnabled);

//@返回 项是否启用 (不含组启用状态).
//@别名  是否项启用()
	BOOL IsItemEnabled(int itemId) const;

//@返回 组内项数.
//@别名  取项数量()
	int GetItemCount(int groupId) const;

	// ===== 展开控制 =====

//@备注 展开项.
//@别名  展开项()
	BOOL ExpandItem(int itemId, BOOL bAnimate = TRUE);

//@备注 收起项.
//@别名  收起项()
	BOOL CollapseItem(int itemId, BOOL bAnimate = TRUE);

//@备注 切换展开状态.
//@别名  切换项()
	BOOL ToggleItem(int itemId, BOOL bAnimate = TRUE);

//@返回 是否已展开.
//@别名  是否项已展开()
	BOOL IsItemExpanded(int itemId) const;

//@备注 groupId=0 表示全部组.
//@别名  收起全部()
	BOOL CollapseAll(int groupId = 0);

//@备注 single 模式下获取组内当前展开项; 无则 -1.
//@别名  取已展开项()
	int GetExpandedItem(int groupId) const;

	// ===== 查询 =====

//@返回 标题行句柄 (高级自定义).
//@别名  取项标题元素()
	HELE GetItemHeaderEle(int itemId) const;

//@返回 内容宿主布局句柄.
//@别名  取项内容宿主()
	HELE GetItemContentHost(int itemId) const;

	// ===== 事件 =====

//@别名  置项展开事件()
	void SetOnItemExpand(xaccordion_item_event fn);
//@别名  置项收起事件()
	void SetOnItemCollapse(xaccordion_item_event fn);
//@别名  置项点击事件()
	void SetOnItemClick(xaccordion_item_event fn);
//@别名  置主题变更事件()
	void SetOnThemeChanged(xaccordion_void_event fn);

	//@隐藏{
	static void CALLBACK AnimaCb(HXCGUI hAnima, int flag);
	static void CALLBACK AnimaItemProgressCb(HXCGUI hAnimaItem, float pos);
	void ReleaseFonts();
	void DetachFonts();
	void EnsureFonts();
	void _xacc_ApplyScroll();
	void _xacc_StopAllAnima(BOOL bReleaseAnima = TRUE);
	void _xacc_NormalizeItemLayout(_XAcc_ItemState* item);
	void _xacc_ApplyCollapsedLayout(_XAcc_ItemState* item);
	void _xacc_ApplyExpandedLayout(_XAcc_ItemState* item);
	void _xacc_RefreshScrollExtent();
	void _xacc_UpdateScrollTotalSize();
	void _xacc_OnWindowClosing();
	void _xacc_BindOwnerWnd(HWINDOW hWnd);
	void _xacc_UnbindOwnerWnd();
	void _xacc_DetachFontRefsFromUi();
	void _xacc_ReleaseOwnedResources();
	void _xacc_DetachAllUserContent();
	void _xacc_DestroyDetachedUserContent();
	void _xacc_ClearState();
	void _xacc_OnRootDestroyed();
	void _xacc_LoadSvgAssets();
	void _xacc_ReleaseSvgAssets();
	void _xacc_ApplyItemIcon(_XAcc_ItemState* item, HIMAGE hIcon);
	void _xacc_ApplyDefaultItemIcon(_XAcc_ItemState* item);
	HIMAGE _xacc_ResolveItemIcon(const _XAcc_ItemState* item) const;
	BOOL _xacc_ItemHasIcon(const _XAcc_ItemState* item) const;
	void InstallEvents();
	_XAcc_GroupState* FindGroup(int groupId);
	_XAcc_ItemState* FindItem(int itemId);
	HELE CreateItemLayout(_XAcc_ItemState* item, HELE hCard);
	void _xacc_ApplyGroupCardMargin(_XAcc_GroupState* g);
	void _xacc_ApplyGroupItemGaps(_XAcc_GroupState* g);
	int _xacc_GroupTitleAlignFlags() const;
	void _xacc_ApplyGroupTitleAlign(_XAcc_GroupState* g);
	void _xacc_ApplyAllGroupTitleAlign();
	void _xacc_ApplyItemContentPadding(_XAcc_ItemState* item);
	void UpdateItemShell(_XAcc_ItemState* item);
	void UpdateGroupItemsShell(_XAcc_GroupState* g);
	void UpdateGroupVisual(_XAcc_GroupState* g);
	void UpdateItemBadge(_XAcc_ItemState* item);
	void UpdateItemVisual(_XAcc_ItemState* item);
	BOOL ClearItemContentEle(_XAcc_ItemState* item);
	int MeasureItemContentHeight(_XAcc_ItemState* item);
	void _xacc_ApplyExpandedHeight(_XAcc_ItemState* item);
	void ApplyItemContentHeight(_XAcc_ItemState* item, int h);
	void FinishItemAnim(_XAcc_ItemState* item);
	void RemeasureItem(_XAcc_ItemState* item);
	void StopItemAnima(_XAcc_ItemState* item, BOOL bReleaseAnima = TRUE);
	void StartItemAnim(_XAcc_ItemState* item, BOOL expanding, BOOL bInstant);
	void OnItemAnimaEnd(HXCGUI hAnima, int flag);
	void CollapseOthers(int itemId, int groupId);
	void RefreshTheme();
	void ApplyItemBtnSelectMode(_XAcc_ItemState* item);
	void ApplyAllItemsBtnSelectMode();
	void SyncItemBtnCheck(_XAcc_ItemState* item);
	BOOL IsItemOperable(_XAcc_ItemState* item) const;

	int OnHeaderCheckImpl(HELE hEle, BOOL bCheck, BOOL* pbHandled);
	int OnItemWrapPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int OnHeaderPaintImpl(HELE hEle, HDRAW hDraw, BOOL* pbHandled);
	int OnDestroyImpl(HELE hEle, BOOL* pbHandled);
	int OnDestroyEndImpl(HELE hEle, BOOL* pbHandled);
	int OnShowImpl(HELE hEle, BOOL bShow, BOOL* pbHandled);
	int OnSizeImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled);
	int OnAdjustLayoutEndImpl(HELE hEle, int nFlags, UINT nAdjustNo, BOOL* pbHandled);
	static int CALLBACK OnOwnerWndCloseImpl(HWINDOW hWnd, BOOL* pbHandled);

	xuitool_theme_ m_theme;
	COLORREF m_customText;
	COLORREF m_customBg;
	COLORREF m_customAccent;
	int m_cornerRadius;
	xaccordion_expand_mode_ m_expandMode;
	BOOL m_bAnimEnabled;
	int m_animDurationMs;
	xaccordion_indicator_style_ m_indicatorStyle;
	xaccordion_group_title_align_ m_groupTitleAlign;
	BOOL m_bScrollEnabled;
	int m_nextGroupId;
	int m_nextItemId;
	HFONTX m_hFontTitle;
	HFONTX m_hFontTitleBold;
	HFONTX m_hFontBody;
	HFONTX m_hFontBadge;
	HFONTX m_hFontIndicator;
	HFONTX m_hFontGroup;
	_XAcc_ThemeColors* m_pColors;
	xaccordion_item_event m_onItemExpand;
	xaccordion_item_event m_onItemCollapse;
	xaccordion_item_event m_onItemClick;
	xaccordion_void_event m_onThemeChanged;
	BOOL m_programmaticBtnCheck;
	BOOL m_bRootDestroyed;
	BOOL m_inAdjustLayoutEndImpl;
	BOOL m_inLayoutSync;
	HWINDOW m_hOwnerWnd;
	HSVG m_hSvgIndCollapsed;
	HSVG m_hSvgIndExpanded;
	HSVG m_hSvgDefaultIcon;
	HIMAGE m_hImgIndCollapsed;
	HIMAGE m_hImgIndExpanded;
	HIMAGE m_hImgDefaultIcon;
	std::unordered_map<int, _XAcc_GroupState*> m_groups;
	std::unordered_map<int, _XAcc_ItemState*> m_items;
	//@隐藏}
};
//@分组}


#endif // XCGUI_UITOOL_H
