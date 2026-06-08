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
//
//          已实现的顶层类:
//          [1] CXTooltip — 鼠标悬停气泡提示
//            - 全局静态注册表 + 共享气泡窗口, 同一时刻至多 1 个气泡显示.
//            - 支持 普通 / 成功 / 信息 / 警告 / 错误 5 种语义 (后 4 个用内置 SVG 图标).
//            - 支持 深色 / 浅色 / 自定义 / 跟随系统 4 套主题.
//            - 显示/隐藏带 alpha 渐显渐隐过渡, 鼠标停留延迟 + 自动关闭时间均可配置.
//            - 弹出窗口 鼠标穿透, 不抢焦点, 不干扰用户操作.
//            - 自动检测鼠标从源元素哪条边进入, 绘制对应方向的三角指针 (指向源).
//
//          [2] CXLoading — 加载动画 (loading 指示器)
//            - 3 种宿主形态: 元素附加 / 窗口附加 / 自建元素.
//            - 5 种动画风格 (spinner 圆环 / dots 跳动点 / spokes 时钟辐条 /
//              pulse 脉冲圈 / bars 频谱条).
//            - 4 套主题 (深色 / 浅色 / 自定义 / 跟随系统), 文本/背景色与 CXTooltip
//              语义一致但状态独立.
//            - 强调色 (动画颜色) 默认: 深色=#FFFFFF / 浅色=#171717.
//            - 居中显示, 默认 40x40, 可配, 可设单行文本 (用户等待小贴士).
//            - 背景圆角支持统一/4 角分别 (扩展版本, CSS 顺序).
//            - 多种 ease (cubic-in-out / cubic-out / sine) 衍生自 CSS 标准.
//            - 元素/窗口销毁时自动释放, 共享 timer 心跳.
//
//          [3] CXCalendarCard — 日期 / 日期范围选择卡片
//            - PopupSingle 单个月历选择日期; PopupDouble 双个月历选择范围.
//            - 默认限制最大可选日期为今天, 也可指定最大日期或关闭限制.
//            - 双月范围支持今天 / 近7天 / 近15天 / 近30天快捷选择.
//            - 弹窗使用 window_transparent_shaped, 自绘圆角背景与柔和阴影.
//@模块信息结束
// =================================================================
// 头文件依赖拓扑顺序说明:
//   - @依赖 是 IDE 解析器(智能感知/别名/语法着色)用的, 不会自动注入 #include 给 cl.exe;
//     所以下面要再用真实的 #include 链一次.
//   - d2d1.h 必须先于 module_xcgui.h 完成.
// =================================================================
#include <d2d1.h>

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
class CXLoading;
class CXCalendarCard;
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
	xloading_style_spinner  = 0,   ///<旋转圆环 + 弧长缓动 (Material Snake), 默认
	//@别名 加载样式_跳点
	xloading_style_dots     = 1,   ///<3 个上下跳动的点, 正弦缓动
	//@别名 加载样式_辐条
	xloading_style_spokes   = 2,   ///<12 条辐条循环淡出 (传统 Mac Beachball 风)
	//@别名 加载样式_脉冲
	xloading_style_pulse    = 3,   ///<2 圈同心扩散 + 透明度衰减, ease-out
	//@别名 加载样式_频谱
	xloading_style_bars     = 4,   ///<5 根竖条上下变高 (音频电平计风格)
};

///<Loading 颜色主题
//@别名 加载主题
enum xloading_theme_
{
	//@别名 加载主题_深色
	xloading_theme_dark     = 0,   ///<文本 #F5F5F5 / 背景 #171717 / 强调 #FFFFFF (默认)
	//@别名 加载主题_浅色
	xloading_theme_light    = 1,   ///<文本 #171717 / 背景 #FFFFFF / 强调 #171717
	//@别名 加载主题_自定义
	xloading_theme_custom   = 2,   ///<使用 SetTextColor / SetBkColor / SetAccentColor 提供的颜色
	//@别名 加载主题_自动
	xloading_theme_auto     = 3,   ///<跟随系统 light/dark
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

//@备注 设置主题. dark / light / custom / auto(跟随系统). 默认 dark.
//@参数 hHost 宿主
//@参数 theme 主题枚举
//@返回 成功 TRUE
//@别名  置主题()
	static BOOL SetTheme(HXCGUI hHost, xloading_theme_ theme);

//@备注 取主题.
//@参数 hHost 宿主
//@返回 主题枚举
//@别名  取主题()
	static xloading_theme_ GetTheme(HXCGUI hHost);

//@备注 自定义文本颜色 (会切到 custom 主题).
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

//@备注 自定义背景色 (会切到 custom 主题).
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

//@备注 自定义动画强调色 (会切到 custom 主题). 该色会被用于动画主体笔画;
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

///<月历卡片颜色主题
//@别名 月历卡片主题
enum xcalendar_theme_
{
	//@别名 月历卡片主题_深色
	xcalendar_theme_dark  = 0,   ///<深色主题
	//@别名 月历卡片主题_浅色
	xcalendar_theme_light = 1,   ///<浅色主题
	//@别名 月历卡片主题_自动
	xcalendar_theme_auto  = 2,   ///<跟随系统 light/dark
};

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
//       支持深色 / 浅色 / 跟随系统主题.
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
//@参数 theme 主题
//@参数 pMaxDate 最大可选日期, NULL 表示今天
//@参数 nCornerRadius 弹窗圆角大小
//@返回 确认 TRUE, 取消 FALSE
//@别名  弹出单月历()
	static BOOL PopupSingle(HWINDOW hParent, xcalendar_datetime_* pDate,
		BOOL bLimitMaxDate = FALSE, xcalendar_theme_ theme = xcalendar_theme_auto,
		const xcalendar_datetime_* pMaxDate = NULL, int nCornerRadius = 8);

//@备注 弹出双个月历日期范围选择卡片. pStart/pEnd 为 NULL 或 year<=0 时默认今天
//       00:00:00 到 23:59:59; 取消返回 FALSE. bLimitMaxDate=TRUE 时禁止选择
//       pMaxDate 之后的日期; pMaxDate=NULL 表示限制到今天.
//@参数 hParent 父窗口句柄, 可为 NULL
//@参数 pStart 输入起始时间并接收结果
//@参数 pEnd 输入结束时间并接收结果
//@参数 bLimitMaxDate 是否启用最大日期限制
//@参数 theme 主题
//@参数 pMaxDate 最大可选日期, NULL 表示今天
//@参数 nCornerRadius 弹窗圆角大小
//@返回 确认 TRUE, 取消 FALSE
//@别名  弹出双月历()
	static BOOL PopupDouble(HWINDOW hParent, xcalendar_datetime_* pStart, xcalendar_datetime_* pEnd,
		BOOL bLimitMaxDate = TRUE, xcalendar_theme_ theme = xcalendar_theme_auto,
		const xcalendar_datetime_* pMaxDate = NULL, int nCornerRadius = 8);
};
//@分组}

#endif // XCGUI_UITOOL_H
