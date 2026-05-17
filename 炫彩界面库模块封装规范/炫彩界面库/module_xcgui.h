#ifndef  XCGUI_H
#define  XCGUI_H
//@模块名称  炫彩界面库
//@版本  4.0.0
//@日期  2026-03-31
//@作者  XCGUI
//@模块备注  炫彩界面库 API 接口
// 
// 2026-04-14
// SVG大升级, 支持裁剪, 蒙板, 滤镜, 渐变等,支持更多SVG格式, 所有SVG效果D2D和GDI都支持
// 新增接口:
// XSvg_EnableAlignPixel()
// XSvg_EnableAntialias()
//
// 2026-03-31
// 新增接口:
// XImage_GetWicBitMap()
// XImage_GetGdiplusBitmap()
// XImgSrc_GetWicBitMap()
// XImgSrc_GetGdiplusBitmap()
// XImage_LoadFromData()
// XImage_ModifyData()
// XImgSrc_LoadFromData()
// XImgSrc_ModifyData()
//
// 2026-03-28
// 公开一些内部使用未公开的绘图函数
//
// 2026-03-26
// 窗口通知消息其他未实现位置实现
//
// 2026-03-17
// XMenu_SetLeftWidth()
// 透明窗口编辑框插入法位置改变延迟问题
//
// 2026-03-15
// 1.CSS快捷键del删除项
// 2.背景编辑器: 打开自动添加默认状态组(常用的)
// 3.进度条CSS完善
// 4.滚动视图及滚动视图扩展的UI元素, 当进行滚动操作时自动检测并调整布局
// 新增组合状态 progressBar_state_flag_
// CSS功能完善升级, 完善XC_OBJECT_STYLE
//
// 2026-02-21
// XSBar_GetCurPos();
//
// 2026-02-09
// XC_LoadLayout_Create();
// XPropertyList_GetString();
// XPropertyList_GetSize();
//
// 2026-01-29
// XC_SetCallBack_LoadLayout();
//
// 2026-01-22
// XWnd_SetMouseHoverTime();
// XWnd_AdjustInScreen();
// XEle_SetToolTipDuration();
// CSS编辑器: 自动铺满功能, 同步修改圆角功能
// CSS编辑器: 工具提示窗口, 框架浮动窗口, 下拉组合框窗口
//
// 2026-01-13 
// CSS添加支持
// 1.TAB条左右切换按钮
// 2.TAB条-标签-关闭按钮
// 3.菜单弹出窗口
// 4.菜单项
//
// 2025-12-16 新增API: XDraw_ConvRect() XDraw_ConvXY()
// 2025-12-02 文本链接按钮 D2D模式下, 设置下划线颜色无效问题
// 2025-12-01 升级CSS功能
//
// 新增接口: 炫彩_启用窗口系统非客户区()
//@QQ  154460336
//@依赖  module_base.h 

#ifdef  XCGUI_STATIC_LIB  //使用 炫彩界面库 静态库
	#ifdef  _DEBUG
		#ifdef  _WIN64
		//@lib "x64\XCGUI.lib"
		//@复制文件 @当前模块路径 "x64\XCGUI.dll"
		#else
		//@lib "x86\XCGUI.lib"
		//@复制文件 @当前模块路径 "x86\XCGUI.dll"
		#endif
	#else
		#ifdef  _WIN64
		//@lib "x64\static-lib\static-XCGUI.lib"
		#else
		//@lib "x86\static-lib\static-XCGUI.lib"
		#endif
	#endif
#else
	#ifdef  _WIN64
	//@lib "x64\XCGUI.lib"
	//@复制文件 @当前模块路径 "x64\XCGUI.dll"
	#else
	//@lib "x86\XCGUI.lib"
	//@复制文件 @当前模块路径 "x86\XCGUI.dll"
	#endif
#endif

//@模块信息结束
//@解析头文件
#include "xcgui_event.h"
/**************************************************************************\
*
* Copyright (c)  炫彩界面库. All Rights Reserved.
*
* 炫彩界面库 - 官方网站 : http://www.xcgui.com
*
\**************************************************************************/
#pragma once


////////////输入输出///////////////////
#ifndef  __in
#define  __in
#endif

#ifndef  __out
#define  __out
#endif

#ifndef  __in_out
#define  __in_out
#endif


#define  in_buffer_  //标识为内存指针, 非变量指针
#define  out_buffer_

#define  XC_API extern "C"

#define  XC_NAME1   L"name1"
#define  XC_NAME2   L"name2"
#define  XC_NAME3   L"name3"
#define  XC_NAME4   L"name4"
#define  XC_NAME5   L"name5"
#define  XC_NAME6   L"name6"

#ifndef  GetAValue
#define  GetAValue(rgba)         (LOBYTE((rgba)>>24))
#endif

#ifndef  RGBA
#define  RGBA(r, g, b, a)        ((BYTE)(a)<<24 | (BYTE)(r) | (BYTE)(g)<<8 | (BYTE)(b)<<16)
#endif

//@隐藏{
#define  COLORREF_MAKE2(rgb,a)        ((BYTE)a<<24 | GetRValue(rgb) | GetGValue(rgb)<<8 | GetBValue(rgb)<<16)
#define  COLORREF_SET_RGB(color,rgb)  ((color & 0xFF000000) | (rgb & 0xFFFFFF))
#define  COLORREF_SET_A(color,a)      ((color & 0xFFFFFF) | ((BYTE)a<<24))
#define  COLORREF_GET_A(color)        ((BYTE)(color>>24))
//@隐藏}

#ifdef _WIN64
typedef  __int64  vint;
#else
typedef  int      vint;
#endif

//如果出现重复定义POINTF,请注释掉下面定义POINTF
#ifndef __IOleControlSite_INTERFACE_DEFINED__
struct POINTF
{
	float x;
	float y;
};
#endif

//@启用枚举前缀

/*@声明
#ifndef SW_HIDE

#define SW_HIDE             0   //@别名 窗口显示标识_隐藏
#define SW_SHOWNORMAL       1   //@别名 窗口显示标识_显示并激活还原
#define SW_SHOWMINIMIZED    2	//@别名 窗口显示标识_显示并最小化
#define SW_SHOWMAXIMIZED    3	//@别名 窗口显示标识_显示并最大化
#define SW_SHOWNOACTIVATE   4	//@别名 窗口显示标识_显示不激活
#define SW_SHOW             5	//@别名 窗口显示标识_显示
#define SW_MINIMIZE         6	//@别名 窗口显示标识_最小化
#define SW_SHOWMINNOACTIVE  7	//@别名 窗口显示标识_最小化不激活
#define SW_SHOWNA           8	//@别名 窗口显示标识_显示不激活2
#define SW_RESTORE          9	//@别名 窗口显示标识_还原
#define SW_SHOWDEFAULT      10	//@别名 窗口显示标识_启动初始值
#define SW_FORCEMINIMIZE    11	//@别名 窗口显示标识_强制最小化

#endif
*/
/////////////////////////////////////////////////////////////////////
/////////////////////句柄类型定义//////////////////////////////////////
/////////////////////////////////////////////////////////////////////
#ifndef DECLARE_HANDLEX
#define DECLARE_HANDLEX(name) struct name##__ { int unused; }; typedef struct name##__ *name
#endif

/// @defgroup groupHandle   句柄类型
/// @{

//@别名  炫彩句柄
typedef void* HXCGUI;       ///<资源句柄
//@别名  窗口句柄
DECLARE_HANDLEX(HWINDOW);     ///<窗口资源句柄
//@别名  元素句柄
DECLARE_HANDLEX(HELE);        ///<元素资源句柄
//@别名  菜单句柄
DECLARE_HANDLEX(HMENUX);      ///<菜单资源句柄
//@别名  绘图句柄
DECLARE_HANDLEX(HDRAW);       ///<图形绘制资源句柄
//@别名  图片句柄
DECLARE_HANDLEX(HIMAGE);      ///<图片资源句柄
//@别名  字体句柄
DECLARE_HANDLEX(HFONTX);      ///<炫彩字体句柄
//@别名  背景句柄
DECLARE_HANDLEX(HBKM);        ///<背景内容管理器句柄
//@别名  模板句柄
DECLARE_HANDLEX(HTEMP);       ///<模板句柄
//@别名  SVG句柄
DECLARE_HANDLEX(HSVG);        ///<SVG句柄
///@}

#ifndef _INC_SHELLAPI
DECLARE_HANDLEX(HDROP);
#endif

/////////////////////////////////////////////////////////////////////
/////////////////////炫彩对象类型及样式/////////////////////////////////
/////////////////////////////////////////////////////////////////////

//对象类型决定功能
//对象样式决定外观
/// @defgroup group_type_style_  对象类型及样式
/// @{

/// @defgroup group_ObjectType 对象句柄类型(XC_OBJECT_TYPE)
/// @{
enum XC_OBJECT_TYPE  //@别名  炫彩对象类型
{
	//@别名  错误
	XC_ERROR = -1,   ///<错误类型
	XC_NOTHING = 0,    ///<啥也不是
	//@别名  窗口
	XC_WINDOW = 1,    ///<窗口
	//@别名  模态窗口
	XC_MODALWINDOW = 2,    ///<模态窗口
	//@别名  框架窗口
	XC_FRAMEWND = 3,    ///<框架窗口
	//@别名  浮动窗口
	XC_FLOATWND = 4,    ///<浮动窗口
	//@别名  组合框下拉窗口
	XC_COMBOBOXWINDOW = 11,   ///<组合框弹出下拉列表窗口 comboBoxWindow_      
	//@别名  菜单主窗口
	XC_POPUPMENUWINDOW = 12,  ///<菜单主窗口 popupMenuWindow_   
	//@别名  菜单子窗口
	XC_POPUPMENUCHILDWINDOW = 13,   ///<菜单子窗口 popupMenuChildWindow_  
	//@别名  可视对象
	XC_OBJECT_UI = 19,   ///<可视对象
	//@别名  窗口组件
	XC_WIDGET_UI = 20,   ///<窗口组件
	//@别名  基础元素
	XC_ELE = 21,   ///<基础元素
	//@别名  按钮
	XC_BUTTON = 22,   ///<按钮

	XC_RICHEDIT = 23,   ///<富文本编辑框(已废弃), 请使用XC_EDIT
	//@别名  下拉组合框
	XC_COMBOBOX = 24,   ///<下拉组合框
	//@别名  滚动条
	XC_SCROLLBAR = 25,   ///<滚动条
	//@别名  滚动视图
	XC_SCROLLVIEW = 26,   ///<滚动视图
	//@别名  列表
	XC_LIST = 27,   ///<列表
	//@别名  列表框
	XC_LISTBOX = 28,   ///<列表框
	//@别名  列表视图
	XC_LISTVIEW = 29,   ///<列表视图,大图标
	//@别名  列表树
	XC_TREE = 30,   ///<列表树
	//@别名  菜单条
	XC_MENUBAR = 31,   ///<菜单条
	//@别名  滑动条
	XC_SLIDERBAR = 32,   ///<滑动条
	//@别名  进度条
	XC_PROGRESSBAR = 33,   ///<进度条
	//@别名  工具条
	XC_TOOLBAR = 34,   ///<工具条
	//@别名  月历卡片
	XC_MONTHCAL = 35,   ///<月历卡片
	//@别名  日期时间
	XC_DATETIME = 36,   ///<日期时间
	//@别名  属性网格
	XC_PROPERTYGRID = 37,   ///<属性网格
	//@别名  颜色选择框
	XC_EDIT_COLOR = 38,   ///<颜色选择框
	//@别名  设置编辑框
	XC_EDIT_SET = 39,   ///<设置编辑框
	//@别名  TAB条
	XC_TABBAR = 40,   ///<tab条
	//@别名  文本链接按钮
	XC_TEXTLINK = 41,   ///<文本链接按钮
	//@别名  窗格
	XC_PANE = 42,   ///<窗格
	//@别名  窗格分割条
	XC_PANE_SPLIT = 43,   ///<窗格拖动分割条
	//@别名  菜单条上按钮
	XC_MENUBAR_BUTTON = 44,   ///<菜单条上的按钮
	//	XC_TOOLBAR_BUTTON       =45,   ///<工具条上按钮
	//	XC_PROPERTYPAGE_LABEL   =46,   ///<属性页标签按钮
	//	XC_PIER                 =47,   ///<窗格停靠码头
	//	XC_BUTTON_MENU          =48,   ///<菜单按钮
	//	XC_VIRTUAL_ELE          =49,   ///<虚拟元素

		//@别名  编辑框
	XC_EDIT = 45,   ///<编辑框
	//@别名  代码编辑框
	XC_EDITOR = 46,   ///<代码编辑框

	XC_TABLE = 47,   ///<表格, 元素表格, 非形状表格

	//@别名  文件选择编辑框
	XC_EDIT_FILE = 50,   ///<EditFile 文件选择编辑框
	//@别名  文件夹选择编辑框
	XC_EDIT_FOLDER = 51,   ///<EditFolder  文件夹选择编辑框
	//@别名  列表头元素
	XC_LIST_HEADER = 52,   ///<列表头元素

	//@别名  布局元素
	XC_ELE_LAYOUT = 53,   ///<布局元素
	//@别名  布局框架
	XC_LAYOUT_FRAME = 54,   ///<布局框架,流式布局


	//@别名  形状对象
	XC_SHAPE = 61,    ///<形状对象
	//@别名  形状对象文本
	XC_SHAPE_TEXT = 62,    ///<形状对象-文本
	//@别名  形状对象图片
	XC_SHAPE_PICTURE = 63,    ///<形状对象-图片
	//@别名  形状对象矩形
	XC_SHAPE_RECT = 64,    ///<形状对象-矩形
	//@别名  形状对象圆形
	XC_SHAPE_ELLIPSE = 65,    ///<形状对象-圆形
	//@别名  形状对象直线
	XC_SHAPE_LINE = 66,    ///<形状对象-直线
	//@别名  形状对象组框
	XC_SHAPE_GROUPBOX = 67,    ///<形状对象-组框
	//@别名  形状对象GIF
	XC_SHAPE_GIF = 68,    ///<形状对象-GIF
	//@别名  形状对象表格
	XC_SHAPE_TABLE = 69,    ///<形状对象-表格

	//@别名  菜单
	XC_MENU = 81,   ///<菜单
	//@别名  图片源
	XC_IMAGE_TEXTURE = 82,   ///<图片纹理,图片源,图片素材
	//@别名  绘图
	XC_HDRAW = 83,   ///<绘图操作
	//@别名  字体
	XC_FONT = 84,   ///<炫彩字体
	//	XC_FLASH             =85,   ///<flash   
	//	XC_WEB               =87,   ///<...
		//@别名  图片帧
	XC_IMAGE_FRAME = 88,   ///<图片帧,指定图片的渲染属性
	//@别名  SVG
	XC_SVG = 89,   ///<SVG矢量图形

	XC_LAYOUT_OBJECT = 101, ///<布局对象LayoutObject, 已废弃
	//@别名  数据适配器
	XC_ADAPTER = 102, ///<数据适配器Adapter
	//@别名  数据适配器表
	XC_ADAPTER_TABLE = 103, ///<数据适配器AdapterTable
	//@别名  数据适配器树
	XC_ADAPTER_TREE = 104, ///<数据适配器AdapterTree
	//@别名  数据适配器列表视图
	XC_ADAPTER_LISTVIEW = 105, ///<数据适配器AdapterListView
	//@别名  数据适配器MAP
	XC_ADAPTER_MAP = 106, ///<数据适配器AdapterMap
	//@别名  背景管理器
	XC_BKINFOM = 116, ///<背景管理器

	//无实体对象,只是用来判断布局
	XC_LAYOUT_LISTVIEW = 111,  ///<内部使用
	XC_LAYOUT_LIST = 112,  ///<内部使用
	XC_LAYOUT_OBJECT_GROUP = 113,  ///<内部使用
	XC_LAYOUT_OBJECT_ITEM = 114,  ///<内部使用
	XC_LAYOUT_PANEL = 115,  ///<内部使用

	//无实体对象,只是用来判断类型
	//@别名  布局盒子
	XC_LAYOUT_BOX = 124,      ///<布局盒子,复合类型
	XC_EXTENSION       =125,      ///<扩展对象

	//@别名  动画序列
	XC_ANIMATION_SEQUENCE = 131,   ///<动画序列
	//@别名  动画组
	XC_ANIMATION_GROUP = 132,   ///<动画同步组
	//@别名  动画项
	XC_ANIMATION_ITEM = 133,   ///<动画项
};
///@}

//@禁用枚举前缀

/// @defgroup group_ObjectTypeEx   对象扩展类型(XC_OBJECT_TYPE_EX,功能扩展)
/// @{
enum XC_OBJECT_TYPE_EX  //@别名  炫彩对象扩展类型
{
	xc_ex_error = -1,        ///<错误类型
	//@别名 按钮类型_默认
	button_type_default = 0, ///<默认类型
	//@别名 按钮类型_单选
	button_type_radio,     ///<单选按钮
	//@别名 按钮类型_多选
	button_type_check,     ///<多选按钮
	//@别名 按钮类型_关闭
	button_type_close,     ///<窗口关闭按钮
	//@别名 按钮类型_最小化
	button_type_min,       ///<窗口最小化按钮
	//@别名 按钮类型_最大化
	button_type_max,       ///<窗口最大化还原按钮

	//@别名 布局元素
	element_type_layout,   ///<布局元素,启用布局功能的元素
};
///@}

//@启用枚举前缀

/// @defgroup group_ObjectStyle 对象样式(XC_OBJECT_STYLE,用于区分外观)
/// @{
enum  XC_OBJECT_STYLE    //@别名   炫彩对象样式
{
	//@别名  默认
	object_style_default = 0,

	//@别名  工具条_分割线
	object_style_element_toolBar_separator,    ///<工具条-分割线
	//@别名  框架窗口_停靠码头左
	object_style_element_frameWnd_dock_left,   ///<框架窗口-停靠码头左
	//@别名  框架窗口_停靠码头右
	object_style_element_frameWnd_dock_right,  ///<框架窗口-停靠码头右
	//@别名  框架窗口_停靠码头上
	object_style_element_frameWnd_dock_top,    ///<框架窗口-停靠码头上
	//@别名  框架窗口_停靠码头下
	object_style_element_frameWnd_dock_bottom, ///<框架窗口-停靠码头下
	//@别名  通知消息
	object_style_element_notifyMsg,            ///<通知消息
	
	//@别名  按钮样式_单选
	object_style_button_radio=100,             ///<单选按钮
	//@别名  按钮样式_多选
	object_style_button_check,                 ///<多选按钮
	//@别名  按钮样式_图标
	object_style_button_icon,                  ///<图标按钮
	//@别名  按钮样式_展开
	object_style_button_expand,                ///<展开按钮
	//@别名  按钮样式_关闭
	object_style_button_close,			       ///<关闭按钮
	//@别名  按钮样式_最大化
	object_style_button_max,			       ///<最大化按钮
	//@别名  按钮样式_最小化
	object_style_button_min,			       ///<最小化按钮

	object_style_button_arrow_left,            ///<按钮-箭头左
	object_style_button_arrow_right,           ///<按钮-箭头右
	object_style_button_arrow_top,             ///<按钮-箭头上
	object_style_button_arrow_bottom,          ///<按钮-箭头下

	object_style_button_toolBar,               ///<工具条-按钮
	object_style_button_tabBar,                ///<Tab条-按钮
	object_style_button_tabBar_close,          ///<Tab条-关闭按钮

	object_style_button_scrollbar_slider_h,    ///<水平滚动条-滑块
	object_style_button_scrollbar_slider_v,    ///<垂直滚动条-滑块
	object_style_button_slider,                ///<滑动条-滑块

	object_style_button_pane_close,            ///<窗格-关闭按钮
	object_style_button_pane_lock,             ///<窗格-锁定按钮
	object_style_button_pane_menu,             ///<窗格-菜单按钮

	object_style_button_pane_dock_left,        ///<窗格-码头按钮左
	object_style_button_pane_dock_right,       ///<窗格-码头按钮右
	object_style_button_pane_dock_top,         ///<窗格-码头按钮上
	object_style_button_pane_dock_bottom,      ///<窗格-码头按钮下

	object_style_monthCal_button_today,        ///<月历卡片-今天
	object_style_monthCal_button_last_year,    ///<月历卡片-上一年
	object_style_monthCal_button_next_year,    ///<月历卡片-下一年
	object_style_monthCal_button_last_month,   ///<月历卡片-上一月
	object_style_monthCal_button_next_month,   ///<月历卡片-下一月

	object_style_window_tooltip =200,          ///<工具提示窗口
	object_style_window_monthCal,              ///<月历卡片窗口
	object_style_window_messageBox,            ///<消息框窗口
};
///@}

/// @defgroup group_WindowStyle  窗口样式(window_style_)
/// @{
enum  window_style_ //@别名  炫彩窗口样式
{
	//@别名  空
	window_style_nothing = 0x0000,   ///<什么也没有
	//@别名  标题栏
	window_style_caption = 0x0001,   ///<标题栏
	//@别名  边框
	window_style_border = 0x0002,   ///<边框,如果没有指定,那么边框大小为0
	//@别名  居中
	window_style_center = 0x0004,   ///<窗口居中
	//@别名  拖动边框
	window_style_drag_border = 0x0008,   ///<拖动窗口边框
	//@别名  拖动窗口
	window_style_drag_window = 0x0010,   ///<拖动窗口
	//@别名  允许最大化
	window_style_allow_maxWindow = 0x0020,   ///<允许窗口最大化

	//@别名  图标
	window_style_icon = 0x0040,   ///<图标
	//@别名  标题
	window_style_title = 0x0080,   ///<标题
	//@别名  控制按钮最小化
	window_style_btn_min = 0x0100,   ///<控制按钮-最小化
	//@别名  控制按钮最大化
	window_style_btn_max = 0x0200,   ///<控制按钮-最大化
	//@别名  控制按钮关闭
	window_style_btn_close = 0x0400,   ///<控制按钮-关闭

	///窗口样式-控制按钮: 居中 图标, 标题, 关闭按钮, 最大化按钮, 最小化按钮
	//@别名  默认
	window_style_default = (window_style_caption | window_style_border | window_style_center | \
		window_style_drag_border | window_style_allow_maxWindow | window_style_icon | \
		window_style_title | window_style_btn_min | window_style_btn_max | window_style_btn_close),

	///窗口样式-简单: 居中
	//@别名  简单
	window_style_simple = (window_style_caption | window_style_border | window_style_center | window_style_drag_border | window_style_allow_maxWindow),

	///窗口样式-弹出窗口: 图标, 标题, 关闭按钮
	//@别名  弹出
	window_style_pop = (window_style_caption | window_style_border | window_style_center | \
		window_style_drag_border | window_style_allow_maxWindow | window_style_icon | \
		window_style_title | window_style_btn_close),

	///模态窗口样式-控制按钮: 居中, 图标, 标题, 关闭按钮
	//@别名  模态
	window_style_modal = (window_style_caption | window_style_border | window_style_center | \
		window_style_icon | window_style_title | window_style_btn_close),

	///模态窗口样式-简单: 居中
	//@别名  模态简单
	window_style_modal_simple = (window_style_caption | window_style_border | window_style_center),
};

///@}
///@}

/////////////////////////////////////////////////////////////////////
/////////////////////宏定义///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/// @defgroup group_macro_def  宏定义
/// @{

#ifdef _DEBUG
#define XTRACEA   _xtrace
#define XTRACE    _xtracew
#define XERRORA   XTRACEA 
#define XERROR    XTRACE
XC_API void WINAPI _xtrace(const char* pFormat, ...); //支持多线程
XC_API void WINAPI _xtracew(const wchar_t* pFormat, ...); //支持多线程
#else
#define XTRACEA
#define XTRACE
#define XERROR 
#define XERRORW
#endif

// #define  XC_GetObjectByUID_(T)  XC_GetObjectByUID(XRes_GetIDValue(L#T))
// #define  GetLayoutFile()        GetLayoutFile()
// #define  XC_LAYOUT_FILE(file)   const wchar_t*  GetLayoutFile(){ return file; };
// #define  XC_EVENT_DECLARE_MARK
// #define  XC_EVENT_REGISTER_MARK
// #define  XC_EVENT_IMPLEMENT_MARK(T)

//@隐藏{
#define  CLOUDUI_flag_openUrl       1
#define  CLOUDUI_flag_downloadFile  2
#define  CLOUDUI_flag_downloadFileComplete      3
#define  CLOUDUI_flag_complete      4
//@隐藏}

/// @name  特殊ID
/// @{
#define  XC_ID_ROOT            0   ///<根节点
#define  XC_ID_ERROR          -1   ///<ID错误
#define  XC_ID_FIRST          -2   ///<插入开始位置(当前层)
#define  XC_ID_LAST           -3   ///<插入末尾位置(当前层)
/// @}


//@隐藏{
///@name 菜单ID , 当前未使用
///@{
#define  IDM_CLIP          1000000000    ///<剪切
#define  IDM_COPY          1000000001    ///<复制
#define  IDM_PASTE         1000000002    ///<粘贴
#define  IDM_DELETE        1000000003    ///<删除
#define  IDM_SELECTALL     1000000004    ///<全选
#define  IDM_DELETEALL     1000000005    ///<清空
///@}

//窗格菜单 当前未使用
#define  IDM_LOCK          1000000006    ///<锁定
#define  IDM_DOCK          1000000007    ///<停靠
#define  IDM_FLOAT         1000000008    ///<浮动
#define  IDM_HIDE          1000000009    ///<隐藏
//@隐藏}

//#define  edit_style_no        0  ///<无效样式
#define  edit_style_default   1    ///<edit 默认样式

//@别名 文本共享缓冲区大小
#define   TEXT_BUFFER_SIZE     10240  ///<共享文本缓冲区大小
/// @}

/////////////////////////////////////////////////////////////////////
/////////////////////窗口事件/////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//@隐藏{  窗口事件

//窗口消息
/**
@addtogroup wndMSG

<hr>
<h2>Windows 标准消息</h2>
- @ref WM_PAINT 窗口绘制消息
- @ref WM_CLOSE 窗口关闭消息.
- @ref WM_DESTROY 窗口销毁消息.
- @ref WM_NCDESTROY 窗口非客户区销毁消息.
- @ref WM_LBUTTONDOWN 窗口鼠标左键按下消息
- @ref WM_LBUTTONUP 窗口鼠标左键弹起消息.
- @ref WM_RBUTTONDOWN 窗口鼠标右键按下消息.
- @ref WM_RBUTTONUP 窗口鼠标右键弹起消息.
- @ref WM_LBUTTONDBLCLK 窗口鼠标左键双击消息.
- @ref WM_RBUTTONDBLCLK 窗口鼠标右键双击消息.
- @ref WM_MOUSEMOVE 窗口鼠标移动消息.
- @ref WM_MOUSEHOVER 窗口鼠标悬停消息
- @ref WM_MOUSELEAVE 窗口鼠标离开消息.
- @ref WM_MOUSEWHEEL 窗口鼠标滚轮滚动消息.
- @ref WM_CAPTURECHANGED 窗口鼠标捕获改变消息.
- @ref WM_KEYDOWN 窗口键盘按键消息.
- @ref WM_KEYUP 窗口键盘弹起消息.
- @ref WM_CHAR       窗口字符消息.
- @ref WM_SIZE 窗口大小改变消息.
- @ref WM_EXITSIZEMOVE 窗口退出移动或调整大小模式循环改，详情参见MSDN.
- @ref WM_TIMER 窗口定时器消息.
- @ref WM_SETFOCUS 窗口获得焦点.
- @ref WM_KILLFOCUS 窗口失去焦点.
- @ref WM_SETCURSOR  窗口设置鼠标光标.
- @ref WM_DROPFILES  拖动文件到窗口.
- @ref other 其他Windows系统消息,包含你自定义的Windows消息.

<hr>
@anchor WM_PAINT WM_PAINT 窗口绘制消息
@code   int CALLBACK OnWndDrawWindow(HDRAW hDraw,BOOL *pbHandled) @endcode
@param  hDraw   图形绘制句柄.

<hr>
@anchor WM_CLOSE WM_CLOSE 窗口关闭消息.
@code   int CALLBACK OnWndClose(BOOL *pbHandled) @endcode

<hr>
@anchor WM_DESTROY WM_DESTROY 窗口销毁消息.
@code   int CALLBACK OnWndDestroy(BOOL *pbHandled)  @endcode

<hr>
@anchor WM_NCDESTROY WM_NCDESTROY 窗口非客户区销毁消息.
@code   int CALLBACK OnWndNCDestroy(BOOL *pbHandled) @endcode

<hr>
@anchor WM_LBUTTONDOWN WM_LBUTTONDOWN 窗口鼠标左键按下消息
@code   int CALLBACK OnWndLButtonDown(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
@param  flags   请参见MSDN WM_LBUTTONDOWN.
@param  pPt     鼠标坐标点.

<hr>
@anchor WM_LBUTTONUP WM_LBUTTONUP 窗口鼠标左键弹起消息.
@code   int CALLBACK OnWndLButtonUp(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
@param  flags   请参见MSDN WM_LBUTTONUP.
@param  pPt     鼠标坐标点.

<hr>
@anchor WM_RBUTTONDOWN WM_RBUTTONDOWN 窗口鼠标右键按下消息.
@code   int CALLBACK OnWndRButtonDown(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
@param  flags   请参见MSDN WM_RBUTTONDOWN.
@param  pPt     鼠标坐标点.

<hr>
@anchor WM_RBUTTONUP WM_RBUTTONUP 窗口鼠标右键弹起消息.
@code   int CALLBACK OnWndRButtonUp(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
@param  flags   请参见MSDN WM_RBUTTONUP.
@param  pPt     鼠标坐标点.

<hr>
@anchor WM_LBUTTONDBLCLK WM_LBUTTONDBLCLK 窗口鼠标左键双击消息.
@code   int CALLBACK OnWndLButtonDBClick(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
@param  flags   请参见MSDN WM_LBUTTONDBLCLK.
@param  pPt     鼠标坐标点.

<hr>
@anchor WM_RBUTTONDBLCLK WM_RBUTTONDBLCLK 窗口鼠标右键双击消息.
@code   int CALLBACK OnWndRButtonDBClick(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
@param  flags   请参见MSDN WM_RBUTTONDBLCLK.
@param  pPt     鼠标坐标点.

<hr>
@anchor WM_MOUSEMOVE WM_MOUSEMOVE 窗口鼠标移动消息.
@code   int CALLBACK OnWndMouseMove(UINT nFlags,POINT *pPt,BOOL *pbHandled)  @endcode
@param  flags   请参见MSDN WM_MOUSEMOVE wParam参数.
@param  pPt     鼠标坐标点.

<hr>
@anchor WM_MOUSEHOVER WM_MOUSEHOVER 窗口鼠标悬停消息.
@code   int CALLBACK OnWndMouseHover(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
@param  flags  请参见MSDN WM_MOUSEHOVER消息wParam参数.
@param  pPt    鼠标位置

<hr>
@anchor WM_MOUSELEAVE WM_MOUSELEAVE 窗口鼠标离开消息.
@code   int CALLBACK OnWndMouseLeave(BOOL *pbHandled) @endcode

<hr>
@anchor WM_MOUSEWHEEL WM_MOUSEWHEEL 窗口鼠标滚轮滚动消息.
@code   int CALLBACK OnWndMouseWheel(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
@param  flags   请参见MSDN WM_MOUSEWHEEL消息wParam参数.
@param  pPt     鼠标坐标点.

<hr>
@anchor WM_CAPTURECHANGED WM_CAPTURECHANGED 窗口鼠标捕获改变消息.
@code   int CALLBACK OnWndCaptureChanged(HWND hWnd,BOOL *pbHandled) @endcode
@param  hWnd    获得鼠标捕获的窗口句柄.

<hr>
@anchor WM_KEYDOWN WM_KEYDOWN 窗口键盘按下消息.
@code   int CALLBACK OnWndKeyDown(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
@note   wParam,lParam:请参见MSDN WM_KEYDOWN.
<hr>
@anchor WM_KEYUP WM_KEYUP 窗口键盘弹起消息.
@code   int CALLBACK OnWndKeyUp(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
@note   wParam,lParam:请参见MSDN WM_KEYUP.

<hr>
@anchor  WM_CHAR WM_CHAR 窗口字符输入消息.
@code    int CALLBACK OnWndChar(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
@note    wParam,lParam:请参见MSDN WM_CHAR.

<hr>
@anchor WM_SIZE WM_SIZE 窗口大小改变消息.
@code   int CALLBACK OnWndSize(UINT nFlags,SIZE *pSize,BOOL *pbHandled) @endcode
@param  flags   请参见MSDN WM_SIZE消息wParam参数.
@param  pSize   窗口大小.

<hr>
@anchor WM_EXITSIZEMOVE WM_EXITSIZEMOVE 窗口退出移动或调整大小模式循环，详情参见MSDN.
@code   int CALLBACK OnWndExitSizeMove(BOOL *pbHandled) @endcode

<hr>
@anchor WM_TIMER WM_TIMER 窗口定时器消息.
@code   int CALLBACK OnWndTimer(UINT nIDEvent,BOOL *pbHandled) @endcode
@param  nIDEnent 定时器标示符.
@param  uElapse  指定所经过的系统启动以来的毫秒数,这是由GetTickCount函数的返回值.

<hr>
@anchor WM_SETFOCUS WM_SETFOCUS 窗口获得焦点.
@code   int CALLBACK OnWndSetFocus(BOOL *pbHandled) @endcode

<hr>
@anchor WM_KILLFOCUS WM_KILLFOCUS 窗口失去焦点.
@code   int CALLBACK OnWndKillFocus(BOOL *pbHandled) @endcode

<hr>
@anchor  WM_SETCURSOR WM_SETCURSOR 窗口设置鼠标光标.
@code    int CALLBACK OnWndSetCursor(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
@note    wParam,lParam:请参见MSDN WM_SETCURSOR.

<hr>
@anchor  WM_DROPFILES WM_DROPFILES 拖动文件到窗口消息,需启用:XWnd_EnableDragFiles()
@code    int CALLBACK OnDropFiles(HDROP hDropInfo , BOOL *pbHandled) @endcode
@note    请参见MSDN WM_DROPFILES.

<hr>
@anchor other 其他Windows系统消息,包含你自定义的Windows消息.
@code   int CALLBACK OnWndOther(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
@param  wParam  消息参数.
@param  lParam  消息参数.
@note   参数信息请参见MSDN.
@{
*/

//窗口消息-包含系统非客户区消息
#define  XWM_EVENT_ALL        WM_APP+1000 //事件投递 -------不公开-------不需要注册

//wParam:left-top坐标组合; lParam:right-bottom坐标组合; 如果这2个参数为空,那么重绘整个窗口
//#define  XWM_REDRAW           WM_APP+1007  //窗口重绘延时 ----不公开-----内部自定义消息

//重绘元素,内部使用
#define  XWM_REDRAW_ELE       0x7000+1 //重绘元素 wParam:元素句柄, lParam:RECT*基于窗口坐标

///@brief 窗口消息过程
///@code  int CALLBACK OnWndProc(UINT message, WPARAM wParam, LPARAM lParam, BOOL *pbHandled)  @endcode
#define  XWM_WINDPROC         0x7000+2   
#define  XWM_DRAW_T           0x7000+3    //窗口绘制,内部使用, wParam:0, lParam:0

#define  XWM_TIMER_T          0x7000+4    //内部使用, wParam:hXCGUI, lParam:ID

/// @brief  炫彩定时器,非系统定时器,注册消息XWM_XC_TIMER接收
/// @code  int CALLBACK OnWndXCTimer(UINT nTimerID,BOOL *pbHandled)  @endcode
/// @param nTimerID   定时器ID
#define  XWM_XC_TIMER         0x7000+5 

#define  XWM_CLOUDUI_DOWNLOADFILE_COMPLETE   0x7000+6  //内部使用

#define  XWM_CLOUNDUI_OPENURL_WAIT    0x7000+7 //内部使用

#define  XWM_CALL_UI_THREAD   0x7000+8     //内部使用


/// @brief 指定元素获得焦点
/// @code  int CALLBACK OnWndSetFocusEle(HELE hEle,BOOL *pbHandled)  @endcode
#define  XWM_SETFOCUS_ELE         0x7000+9 

/// @brief  托盘图标事件
/// @code  int CALLBACK OnWndTrayIcon(WPARAM wParam, LPARAM lParam, BOOL *pbHandled)  @endcode
#define  XWM_TRAYICON         0x7000+10  

/// @brief 菜单弹出
///@code  int CALLBACK OnWndMenuPopup(HMENUX hMenu, BOOL *pbHandled) @endcode
#define  XWM_MENU_POPUP       0x7000+11 

/// @brief 菜单弹出窗口
///@code  int CALLBACK OnWndMenuPopupWnd(HMENUX hMenu,menu_popupWnd_ *pInfo,BOOL *pbHandled) @endcode
#define  XWM_MENU_POPUP_WND     0x7000+12   

/// @brief 菜单选择
///@code  int CALLBACK OnWndMenuSelect(int nID,BOOL *pbHandled) @endcode
#define  XWM_MENU_SELECT       0x7000+13 

/// @brief 菜单退出
///@code  int CALLBACK OnWndMenuExit(BOOL *pbHandled) @endcode
#define  XWM_MENU_EXIT         0x7000+14  

/// @brief 绘制菜单背景, 启用该功能需要调用XMenu_EnableDrawBackground().
///@code  int CALLBACK OnWndMenuDrawBackground(HDRAW hDraw,menu_drawBackground_ *pInfo,BOOL *pbHandled) @endcode
#define  XWM_MENU_DRAW_BACKGROUND   0x7000+15 

/// @brief 绘制菜单项事件, 启用该功能需要调用XMenu_EnableDrawItem().
/// @code  int CALLBACK OnMenuDrawItem(HDRAW hDraw,menu_drawItem_* pInfo,BOOL *pbHandled) @endcode
#define  XWM_MENU_DRAWITEM             0x7000+16  

#define  XWM_COMBOBOX_POPUP_DROPLIST   0x7000+17  //弹出下拉组框列表,内部使用

/// @brief 浮动窗格
///@code  int CALLBACK OnWndFloatPane(HWINDOW hFloatWnd, HELE hPane, BOOL *pbHandled) @endcode
#define  XWM_FLOAT_PANE               0x7000+18  //@别名 窗口事件_浮动窗格  //浮动窗格, 窗格从框架窗口中弹出,变成浮动窗格

/// @brief 窗口绘制完成消息
///@code   int CALLBACK OnWndDrawWindowEnd(HDRAW hDraw,BOOL *pbHandled) @endcode
#define  XWM_PAINT_END               0x7000+19  

///@brief 窗口绘制完成并且已经显示到屏幕
///@code   int CALLBACK OnWndDrawWindowDisplay(BOOL *pbHandled) @endcode
#define  XWM_PAINT_DISPLAY           0x7000+20   

/// @brief  框架窗口码头弹出窗格  当用户点击码头上的按钮时,显示对应的窗格,当失去焦点时自动隐藏窗格
///@param  hWindowDock 弹出窗格窗口句柄
///@param  hPane       窗格句柄
///@code   int CALLBACK OnWndDocPopup(HWINDOW hWindowDock,  HELE hPane, BOOL *pbHandled) @endcode
#define  XWM_DOCK_POPUP              0x7000+21    

/// @brief  浮动窗口拖动   用户拖动浮动窗口移动,显示停靠提示
///@param  hFloatWnd 拖动的浮动窗口句柄
///@param  hArray    HWINDOW array[6],窗格停靠提示窗口句柄数组,有6个成员,分别为:[0]中间十字, [1]左侧,[2]顶部,[3]右侧,[4]底部, [5]停靠位置预览 
///@code   int CALLBACK OnWndFloatWndDrag(HWINDOW hFloatWnd, HWINDOW* hArray, BOOL *pbHandled) @endcode
#define  XWM_FLOATWND_DRAG           0x7000+22   

/// @brief  窗格显示隐藏
///@code int CALLBACK OnWndPaneShow(HELE hPane, BOOL bShow, BOOL *pbHandled); @endcode
#define  XWM_PANE_SHOW              0x7000+23   

/// @brief  框架窗口主视图坐标改变, 如果主视图没有绑定元素, 那么当坐标改变时触发此事件
///@code int CALLBACK OnWndLayoutViewRect(int width, int height, BOOL *pbHandled); @endcode
#define  XWM_BODYVIEW_RECT          0x7000+24   
///@}
//@隐藏}

/////////////////////////////////////////////////////////////////////
/////////////////元素事件/////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//@隐藏{  元素事件
/// @addtogroup eleEvents
/// @{


/// @brief 元素处理过程事件.
/// @code  int CALLBACK OnEventProc(UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL *pbHandled) @endcode
#define  XE_ELEPROCE         1     

/// @brief 元素绘制事件
/// @code  int CALLBACK OnDraw(HDRAW hDraw,BOOL *pbHandled)  @endcode
#define  XE_PAINT            2  

/// @brief 该元素及子元素绘制完成事件.启用该功能需要调用XEle_EnableEvent_XE_PAINT_END()
/// @code  int CALLBACK OnPaintEnd(HDRAW hDraw,BOOL *pbHandled) @endcode
#define  XE_PAINT_END        3    

/// @brief 滚动视图绘制事件.
/// @code int CALLBACK OnDrawScrollView(HDRAW hDraw,BOOL *pbHandled) @endcode
#define  XE_PAINT_SCROLLVIEW   4   

/// @brief 元素鼠标移动事件.
/// @code  int CALLBACK OnMouseMove(UINT nFlags, POINT *pPt, BOOL *pbHandled)  @endcode
#define  XE_MOUSEMOVE        5  

/// @brief 元素鼠标进入事件.
/// @code  int CALLBACK OnMouseStay(BOOL *pbHandled) @endcode
#define  XE_MOUSESTAY        6    

/// @brief 元素鼠标悬停事件.
/// @code  int CALLBACK OnMouseHover(UINT nFlags, POINT *pPt, BOOL *pbHandled) @endcode
#define  XE_MOUSEHOVER       7   

/// @brief 元素鼠标离开事件.
/// @param  hEleStay 将获得鼠标停留状态的元素.
/// @code  int CALLBACK OnMouseLeave(HELE hEleStay,BOOL *pbHandled) @endcode
#define  XE_MOUSELEAVE       8   

/// @brief 元素鼠标滚轮滚动事件. 如果非滚动视图需要调用 XEle_EnableEvent_XE_MOUSEWHEEL()
/// @code  int CALLBACK OnMouseWheel(UINT nFlags,POINT *pPt,BOOL *pbHandled) @endcode
/// @param flags      见MSDN中WM_MOUSEWHEEL消息wParam参数说明.
#define  XE_MOUSEWHEEL         9  

/// @brief 鼠标左键按下事件.
/// @code  int CALLBACK OnLButtonDown(UINT nFlags, POINT *pPt,BOOL *pbHandled)  @endcode
#define  XE_LBUTTONDOWN        10  

/// @brief 鼠标左键弹起事件.
/// @code  int CALLBACK OnLButtonUp(UINT nFlags, POINT *pPt,BOOL *pbHandled) @endcode
#define  XE_LBUTTONUP          11  

/// @brief 鼠标右键按下事件.
/// @code  int CALLBACK OnRButtonDown(UINT nFlags, POINT *pPt,BOOL *pbHandled)  @endcode
#define  XE_RBUTTONDOWN        12  

/// @brief 鼠标右键弹起事件.
/// @code int CALLBACK OnRButtonUp(UINT nFlags, POINT *pPt,BOOL *pbHandled)  @endcode
#define  XE_RBUTTONUP          13   

/// @brief 鼠标左键双击事件.
/// @code   int CALLBACK OnLButtonDBClick(UINT nFlags, POINT *pPt,BOOL *pbHandled) @endcode
#define  XE_LBUTTONDBCLICK     14  


//#define  XE_RBUTTONDBCLICK     15


/// @brief 炫彩定时器,非系统定时器,定时器消息 XM_TIMER.
/// @code int CALLBACK OnEleXCTimer(UINT nTimerID,BOOL *pbHandled) @endcode
/// @param hEle        元素句柄.
/// @param nTimerID    定时器ID.
#define  XE_XC_TIMER             16 

/// @brief 调整布局事件. 暂停使用
/// @code int CALLBACK OnAdjustLayout(int nFlags, UINT nAdjustNo, BOOL *pbHandled) @endcode
#define  XE_ADJUSTLAYOUT         17

/// @brief 调整布局完成事件.
/// @code int CALLBACK OnAdjustLayoutEnd(int nFlags, UINT nAdjustNo, BOOL *pbHandled) @endcode
#define  XE_ADJUSTLAYOUT_END     18  

/// @brief 元素工具提示弹出事件.
/// @code int CALLBACK OnTooltipPopup(HWINDOW hWindowTooltip, const wchar_t* pText, BOOL *pbHandled) @endcode
#define  XE_TOOLTIP_POPUP        19   

// 计算表格坐标
/// @code int CALLBACK OnTableCalc(BOOL *pbHandled) @endcode
#define  XE_TABLE_CALC          20

//回车增加行
/// @code int CALLBACK OnTableEnter(int iRow, int iCol, BOOL *pbHandled) @endcode
#define  XE_TABLE_ENTER         21

/// @code int CALLBACK OnTableChar(wchar_t ch, BOOL *pbHandled) @endcode
//#define  XE_TABLE_CHAR          22

/// @brief 元素获得焦点事件.
/// @code int CALLBACK OnSetFocus(BOOL *pbHandled) @endcode
#define  XE_SETFOCUS           31   

/// @brief 元素失去焦点事件.
/// @code int CALLBACK OnKillFocus(BOOL *pbHandled) @endcode
#define  XE_KILLFOCUS          32   

/// @brief 元素即将销毁事件. 在销毁子对象之前触发
/// @code int CALLBACK OnDestroy(BOOL *pbHandled) @endcode
#define  XE_DESTROY            33   

/// @brief 元素销毁完成事件. 在销毁子对象之后触发
/// @code int CALLBACK OnDestroyeEnd(BOOL *pbHandled) @endcode
#define  XE_DESTROY_END        42   

/// @brief 按钮点击事件.
/// @code int CALLBACK OnBtnClick(BOOL *pbHandled) @endcode
#define  XE_BNCLICK            34  

/// @brief 按钮选中事件
/// @code int CALLBACK OnButtonCheck(BOOL bCheck,BOOL *pbHandled) @endcode
#define  XE_BUTTON_CHECK       35   

/// @brief 元素大小改变事件.
/// @code int CALLBACK OnSize(int nFlags, UINT nAdjustNo, BOOL *pbHandled) @endcode
/// @param  nFlags  adjustLayout_
#define  XE_SIZE               36   

/// @brief 元素显示隐藏事件.
/// @code  int CALLBACK OnShow(BOOL bShow,BOOL *pbHandled) @endcode
#define  XE_SHOW               37   

/// @brief 元素设置字体事件.
/// @code  int CALLBACK OnSetFont(BOOL *pbHandled) @endcode
#define  XE_SETFONT           38   

/// @brief 元素按键事件.
/// @code  int CALLBACK OnKeyDown(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
/// @note  wParam,lParam:请参见MSDN WM_KEYDOWN. 
#define  XE_KEYDOWN            39 

/// @brief 元素按键事件.
/// @code  int CALLBACK OnKeyUp(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
/// @note  wParam,lParam:请参见MSDN WM_KEYDOWN. 
#define  XE_KEYUP             40 

/// @brief 通过TranslateMessage函数翻译的字符事件.
/// @code  int CALLBACK OnChar(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
/// @note   wParam,lParam:请参见MSDN WM_KEYDOWN. 
#define  XE_CHAR               41 

//XE_DESTROY_END   42

#define  XE_SYSKEYDOWN     43

#define  XE_SYSKEYUP       44

/// @brief 元素设置鼠标捕获.
/// @code int CALLBACK OnSetCapture(BOOL *pbHandled) @endcode
#define  XE_SETCAPTURE      51   

/// @brief 元素失去鼠标捕获.
/// @code int CALLBACK OnKillCapture(BOOL *pbHandled) @endcode
#define  XE_KILLCAPTURE     52  

/// @brief 设置鼠标光标
/// @code int CALLBACK OnSetCursor(WPARAM wParam,LPARAM lParam,BOOL *pbHandled) @endcode
#define  XE_SETCURSOR       53   

///@brief 菜单弹出
///@code  int CALLBACK OnMenuPopup(HMENUX hMenu, BOOL *pbHandled) @endcode
#define  XE_MENU_POPUP       57   

///@brief 菜单弹出窗口
///@code  int CALLBACK OnMenuPopupWnd(HMENUX hMenu,menu_popupWnd_* pInfo,BOOL *pbHandled) @endcode
#define  XE_MENU_POPUP_WND     58  

/// @brief 菜单项选择事件.
/// @code  int CALLBACK OnMenuSelect(int nItem,BOOL *pbHandled) @endcode
/// @param nItem          菜单项id.
#define  XE_MENU_SELECT      59  

///@brief 绘制菜单背景, 启用该功能需要调用XMenu_EnableDrawBackground().
///@code  int CALLBACK OnMenuDrawBackground(HDRAW hDraw,menu_drawBackground_ *pInfo,BOOL *pbHandled) @endcode
#define  XE_MENU_DRAW_BACKGROUND   60 

/// @brief 绘制菜单项事件, 启用该功能需要调用XMenu_EnableDrawItem().
/// @code  int CALLBACK OnMenuDrawItem(HDRAW hDraw,menu_drawItem_* pInfo,BOOL *pbHandled) @endcode
#define  XE_MENU_DRAWITEM    61  

/// @brief 菜单退出事件.
/// @code  int CALLBACK OnMenuExit(BOOL *pbHandled) @endcode
#define  XE_MENU_EXIT        62 

/// @brief 滚动视图元素水平滚动事件,滚动视图触发.
/// @code  int CALLBACK OnScrollViewScrollH(int pos,BOOL *pbHandled) @endcode
/// @param pos         当前滚动点.
#define  XE_SCROLLVIEW_SCROLL_H    54   

/// @brief 滚动视图元素垂直滚动事件,滚动视图触发.
/// @code  int CALLBACK OnScrollViewScrollV(int pos,BOOL *pbHandled) @endcode
/// @param pos         当前滚动点.
#define  XE_SCROLLVIEW_SCROLL_V    55 

/// @brief 滚动条元素滚动事件,滚动条触发.
/// @code  int CALLBACK OnSBarScroll(int pos,BOOL *pbHandled) @endcode
/// @param pos   当前滚动点.
#define  XE_SBAR_SCROLL        56  

/// @brief 滑动条元素,滑块位置改变事件.
/// @code  int CALLBACK OnSliderBarChange(int pos,BOOL *pbHandled) @endcode
#define  XE_SLIDERBAR_CHANGE   63  

/// @brief 进度条元素,进度改变事件.
/// @code  int CALLBACK OnProgressBarChange(int pos,BOOL *pbHandled) @endcode
#define  XE_PROGRESSBAR_CHANGE  64  

/// @brief 组合框下拉列表项选择事件.
/// @code  int CALLBACK OnComboBoxSelect(int iItem,BOOL *pbHandled)  @endcode
#define  XE_COMBOBOX_SELECT       71   

/// @brief 组合框下拉列表项选择完成事件,编辑框内容已经改变.
/// @code  int CALLBACK OnComboBoxSelectEnd(int iItem,BOOL *pbHandled)  @endcode
#define  XE_COMBOBOX_SELECT_END   74  


/// @brief 组合框下拉列表即将弹出事件, 可拦截后续操作
/// @code  int CALLBACK OnComboBoxPopup(BOOL *pbHandled)  @endcode
#define  XE_COMBOBOX_POPUP        75  //新增

/// @brief 组合框下拉列表弹出事件.
/// @code  int CALLBACK OnComboBoxPopupList(HWINDOW hWindow,HELE hListBox,BOOL *pbHandled)  @endcode
#define  XE_COMBOBOX_POPUP_LIST   72  

/// @brief 组合框下拉列表退出事件.
/// @code  int CALLBACK OnComboBoxExitList(BOOL *pbHandled)  @endcode
#define  XE_COMBOBOX_EXIT_LIST    73  

/// @brief 列表框元素-项模板创建事件, 模板复用机制需先启用; 替换模板无效判断nFlag,因为内部会检查模板是否改变,不用担心重复
/// @code int CALLBACK OnListBoxTemplateCreate(listBox_item_* pItem, int nFlag, BOOL *pbHandled) @endcode
/// @param nFlag  0:状态改变; 1:新模板实例; 2:旧模板复用 
#define  XE_LISTBOX_TEMP_CREATE     81  

/// @brief 列表框元素-项模板创建完成事件,模板复用机制需先启用;不管是新建还是复用,都需要更新数据, 当为复用时不要注册事件以免重复注册
/// @code int CALLBACK OnListBoxTemplateCreateEnd(listBox_item_* pItem, int nFlag, BOOL *pbHandled) @endcode
/// @param nFlag  0:状态改变(复用); 1:新模板实例; 2:旧模板复用
#define  XE_LISTBOX_TEMP_CREATE_END     82  
#define  XE_LISTBOX_TEMP_UPDATE   XE_LISTBOX_TEMP_CREATE_END

/// @brief 列表框元素,项模板销毁.
/// @code int CALLBACK OnListBoxTemplateDestroy(listBox_item_* pItem, int nFlag, BOOL *pbHandled) @endcode
/// @param  nFlag   0:正常销毁;  1:移动到缓存(不会被销毁,临时缓存备用,当需要时被复用)
#define  XE_LISTBOX_TEMP_DESTROY    83  

/// @brief 列表框元素,项模板调整坐标. 已停用
/// @code  int CALLBACK OnListBoxTemplateAdjustCoordinate(listBox_item_* pItem, BOOL *pbHandled) @endcode
#define  XE_LISTBOX_TEMP_ADJUST_COORDINATE  84

/// @brief 列表框元素,项绘制事件.
/// @code  int CALLBACK OnListBoxDrawItem(HDRAW hDraw,listBox_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_LISTBOX_DRAWITEM     85 

/// @brief 列表框元素,项选择事件.
/// @code  int CALLBACK OnListBoxSelect(int iItem,BOOL *pbHandled)  @endcode
#define  XE_LISTBOX_SELECT       86   

/// @brief 列表元素-项模板创建事件,模板复用机制需先启用;替换模板无效判断nFlag,因为内部会检查模板是否改变,不用担心重复
/// @code  int CALLBACK OnListTemplateCreate(list_item_* pItem,int nFlag, BOOL *pbHandled) @endcode
/// @param nFlag  0:状态改变; 1:新模板实例; 2:旧模板复用 
#define  XE_LIST_TEMP_CREATE     101  

/// @brief 列表元素-项模板创建完成事件,模板复用机制需先启用;不管是新建还是复用,都需要更新数据, 当为复用时不要注册事件以免重复注册
/// @code  int CALLBACK OnListTemplateCreateEnd(list_item_* pItem, int nFlag, BOOL *pbHandled) @endcode
/// @param nFlag  0:状态改变(复用); 1:新模板实例; 2:旧模板复用
#define  XE_LIST_TEMP_CREATE_END     102  
#define  XE_LIST_TEMP_UPDATE  XE_LIST_TEMP_CREATE_END 

/// @brief 列表元素,项模板销毁.
/// @code int CALLBACK OnListTemplateDestroy(list_item_* pItem, int nFlag, BOOL *pbHandled) @endcode
/// @param  nFlag   0:正常销毁;  1:移动到缓存(不会被销毁,临时缓存备用,当需要时被复用)
#define  XE_LIST_TEMP_DESTROY    103  

/// @brief 列表元素,项模板调整坐标. 已停用
/// @code  typedef int CALLBACK OnListTemplateAdjustCoordinate(list_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_LIST_TEMP_ADJUST_COORDINATE  104

/// @brief 列表元素,绘制项.
/// @code  int CALLBACK OnListDrawItem(HDRAW hDraw,list_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_LIST_DRAWITEM                105  

/// @brief 列表元素,项选择事件.
/// @code  int CALLBACK OnListSelect(int iItem,BOOL *pbHandled) @endcode
#define  XE_LIST_SELECT                  106  

/// @brief 列表元素绘制列表头项.
/// @code  int CALLBACK OnListHeaderDrawItem(HDRAW hDraw, list_header_item_* pItem, BOOL *pbHandled) @endcode
#define  XE_LIST_HEADER_DRAWITEM         107  

/// @brief 列表元素,列表头项点击事件.
/// @code  int CALLBACK OnListHeaderClick(int iItem, BOOL *pbHandled) @endcode
#define  XE_LIST_HEADER_CLICK            108  

/// @brief 列表元素,列表头项宽度改变事件.
/// @code  int CALLBACK OnListHeaderItemWidthChange(int iItem, int nWidth BOOL *pbHandled) @endcode
#define  XE_LIST_HEADER_WIDTH_CHANGE     109  

/// @brief 列表元素,列表头项模板创建.
/// @code  int CALLBACK OnListHeaderTemplateCreate(list_header_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_LIST_HEADER_TEMP_CREATE          110  

/// @brief 列表元素,列表头项模板创建完成事件.
/// @code  int CALLBACK OnListHeaderTemplateCreateEnd(list_header_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_LIST_HEADER_TEMP_CREATE_END      111  

/// @brief 列表元素,列表头项模板销毁.
/// @code int CALLBACK OnListHeaderTemplateDestroy(list_header_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_LIST_HEADER_TEMP_DESTROY          112  

/// @brief 列表元素,列表头项模板调整坐标. 已停用
/// @code  typedef int CALLBACK OnListHeaderTemplateAdjustCoordinate(list_header_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_LIST_HEADER_TEMP_ADJUST_COORDINATE  113


/// @brief 列表树元素-项模板创建,模板复用机制需先启用; 替换模板无效判断nFlag,因为内部会检查模板是否改变,不用担心重复
/// @code  int CALLBACK OnTreeTemplateCreate(tree_item_* pItem,int nFlag, BOOL *pbHandled) @endcode
/// @param nFlag  0:状态改变; 1:新模板实例; 2:旧模板复用 
#define  XE_TREE_TEMP_CREATE             121  

/// @brief 列表树元素-项模板创建完成,模板复用机制需先启用; 不管是新建还是复用,都需要更新数据, 当为复用时不要注册事件以免重复注册
/// @code  int CALLBACK OnTreeTemplateCreateEnd(tree_item_* pItem, int nFlag, BOOL *pbHandled) @endcode
/// @param nFlag  0:状态改变(复用); 1:新模板实例; 2:旧模板复用
#define  XE_TREE_TEMP_CREATE_END         122  
#define  XE_TREE_TEMP_UPDATE   XE_TREE_TEMP_CREATE_END

/// @brief 列表树元素-项模板销毁,模板复用机制需先启用;
/// @code  int CALLBACK OnTreeTemplateDestroy(tree_item_* pItem, int nFlag, BOOL *pbHandled) @endcode
/// @param  nFlag   0:正常销毁;  1:移动到缓存(不会被销毁,临时缓存备用,当需要时被复用)
#define  XE_TREE_TEMP_DESTROY            123  

/// @brief 树元素,项模板,调整项坐标. 已停用
/// @code  int CALLBACK OnTreeTemplateAdjustCoordinate(tree_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_TREE_TEMP_ADJUST_COORDINATE  124  

/// @brief 树元素,绘制项.
/// @code  int CALLBACK OnTreeDrawItem(HDRAW hDraw,tree_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_TREE_DRAWITEM                125  

/// @brief 树元素,项选择事件.
/// @code  int CALLBACK OnTreeSelect(int nItemID,BOOL *pbHandled) @endcode
/// @param nItemID  项ID.
#define  XE_TREE_SELECT                 126  

/// @brief 树元素,项展开收缩事件.
/// @code int CALLBACK OnTreeExpand(int id,BOOL bExpand,BOOL *pbHandled) @endcode
#define  XE_TREE_EXPAND                 127   

/// @brief 树元素,用户正在拖动项, 可对参数值修改.
/// @code int CALLBACK OnTreeDragItemIng(tree_drag_item_* pInfo, BOOL *pbHandled) @endcode 
#define  XE_TREE_DRAG_ITEM_ING           128   

/// @brief 树元素,拖动项事件.
/// @code int CALLBACK OnTreeDragItem(tree_drag_item_* pInfo, BOOL *pbHandled) @endcode 
#define  XE_TREE_DRAG_ITEM               129  

/// @brief 列表视元素-项模板创建事件,模板复用机制需先启用; 替换模板无效判断nFlag,因为内部会检查模板是否改变,不用担心重复
/// @code  int CALLBACK OnListViewTemplateCreate(listView_item_* pItem,int nFlag, BOOL *pbHandled) @endcode
/// @param nFlag  0:状态改变(当前未使用); 1新模板实例; 2旧模板复用 
#define  XE_LISTVIEW_TEMP_CREATE           141   

/// @brief 列表视元素-项模板创建完成事件,模板复用机制需先启用; 不管是新建还是复用,都需要更新数据, 当为复用时不要注册事件以免重复注册
/// @code  int CALLBACK OnListViewTemplateCreateEnd(listView_item_* pItem,int nFlag, BOOL *pbHandled) @endcode
/// @param nFlag  0:状态改变(复用,当前未使用); 1:新模板实例; 2:旧模板复用 
#define  XE_LISTVIEW_TEMP_CREATE_END           142  
#define  XE_LISTVIEW_TEMP_UPDATE     XE_LISTVIEW_TEMP_CREATE_END

/// @brief 列表视元素-项模板销毁, 模板复用机制需先启用;
/// @code  int CALLBACK OnListViewTemplateDestroy(listView_item_* pItem, int nFlag, BOOL *pbHandled) @endcode
/// @param  nFlag   0:正常销毁;  1:移动到缓存列表(不会被销毁, 临时缓存备用, 当需要时被复用)
#define  XE_LISTVIEW_TEMP_DESTROY           143  

/// @brief 列表视元素,项模板调整坐标.已停用
/// @code  int CALLBACK OnListViewTemplateAdjustCoordinate(listView_item_* pItem,BOOL *pbHandled) @endcode
#define  XE_LISTVIEW_TEMP_ADJUST_COORDINATE   144

/// @brief 列表视元素,自绘项.
/// @code int CALLBACK OnListViewDrawItem(HDRAW hDraw,listView_item_* pItem,BOOL *pbHandled)  @endcode
#define  XE_LISTVIEW_DRAWITEM              145  

/// @brief 列表视元素,项选择事件.
/// @code int CALLBACK OnListViewSelect(int iGroup,int iItem,BOOL *pbHandled) @endcode
#define  XE_LISTVIEW_SELECT            146  

/// @brief 列表视元素,组展开收缩事件.
/// @code int CALLBACK OnListViewExpand(int iGroup,BOOL bExpand,BOOL *pbHandled) @endcode
#define  XE_LISTVIEW_EXPAND         147  

/// @brief 属性网格元素 项值改变事件
/// @code   int CALLBACK OnPGridValueChange(int nItemID,BOOL *pbHandled) @endcode
/// @param  nItemID  项ID.
#define  XE_PGRID_VALUE_CHANGE     151  

/// @code int CALLBACK OnPGridItemSet(int nItemID, BOOL *pbHandled)  @endcode
#define  XE_PGRID_ITEM_SET          152  

/// @code int CALLBACK OnPGridItemSelect(int nItemID, BOOL *pbHandled)  @endcode
#define  XE_PGRID_ITEM_SELECT         153   

/// @code int CALLBACK OnPGridItemAdjustCoordinate(propertyGrid_item_* pItem, BOOL *pbHandled)  @endcode
#define  XE_PGRID_ITEM_ADJUST_COORDINATE  154  

/// @code int CALLBACK OnPGridItemDestroy(int nItemID, BOOL *pbHandled)  @endcode
#define  XE_PGRID_ITEM_DESTROY   155  

/// @code int CALLBACK OnPGridItemExpand(int nItemID, BOOL bExpand, BOOL *pbHandled) @endcode
#define  XE_PGRID_ITEM_EXPAND    156  


/// @code int CALLBACK OnEditSet(BOOL *pbHandled)  @endcode
#define  XE_EDIT_SET           180  

/// @code int CALLBACK OnEditDrawRow(HDRAW hDraw, int iRow, BOOL *pbHandled)  @endcode
#define  XE_EDIT_DRAWROW    181   //暂未使用

/// @code int CALLBACK OnEditChanged(BOOL *pbHandled)  @endcode
#define  XE_EDIT_CHANGED   182   

/// @code int CALLBACK OnEditPosChanged(int iPos, BOOL *pbHandled)  @endcode
#define  XE_EDIT_POS_CHANGED    183  

/// @code int CALLBACK OnEditStyleChanged(int iStyle, BOOL *pbHandled)  @endcode
#define  XE_EDIT_STYLE_CHANGED  184  

/// @code int CALLBACK OnEditEnterGetTabAlign(BOOL *pbHandled)  @endcode
#define  XE_EDIT_ENTER_GET_TABALIGN    185  //回车TAB对齐,返回需要TAB数量

/// @code int CALLBACK OnEditSwapRow(int iRow, int bArrowUp, BOOL *pbHandled)  @endcode
#define  XE_EDIT_SWAPROW     186   

/// @brief 未公开, 光标位置改变, 格式代码检测
/// @code int CALLBACK OnEditorFormatCodeTest(int iRow, int iCol, BOOL *pbHandled)  @endcode
#define  XE_EDITOR_FORMATCODE_TEST    187

#define  XE_EDITOR_COMMENT_GEN_CODE    188  //根据注释生成代码
/// @brief 多行内容改变事件 例如:区块注释操作, 区块缩进操作, 代码格式化
/// @code int CALLBACK OnEditChangeRows(int iRow, int nRows, BOOL *pbHandled)  @endcode
/// @param iRow   开始行
/// @param nRows  改变行数量
#define  XE_EDITOR_MODIFY_ROWS         190  //多行内容修改事件

/// @code int CALLBACK OnEditorSetBreakpoint(int iRow, BOOL bCheck, BOOL *pbHandled)  @endcode
#define  XE_EDITOR_SETBREAKPOINT       191 //设置断点

/// @code int CALLBACK OnEditorRemoveBreakpoint(int iRow, BOOL *pbHandled)  @endcode
#define  XE_EDITOR_REMOVEBREAKPOINT    192 //移除断点

// iRow: 更改行开始位置索引,  if(nChangeRows>0) iEnd= iRow + nChangeRows
// nChangeRows: 改变行数, 正数添加行, 负数删除行
/// @code int CALLBACK OnEditorBreakpointChanged(int iRow, int nChangeRows, BOOL *pbHandled)  @endcode
#define  XE_EDIT_ROW_CHANGED         193 //可对断点位置修改

/// @code int CALLBACK OnEditorAutoMatchSelect(int iRow, int nRows, BOOL *pbHandled)  @endcode
#define  XE_EDITOR_AUTOMATCH_SELECT  194

// bSet: TRUE增加书签, FALSE移除书签
/// @code int CALLBACK OnEditorSetBookmark(int iRow, BOOL bSet, BOOL *pbHandled)
#define  XE_EDITOR_SETBOOKMARK       195 //设置书签


/// @brief  TabBar标签按钮选择改变事件
/// @code   int CALLBACK OnTabBarSelect(int iItem, BOOL *pbHandled) @endcode
/// @param  iItem  标签位置索引.
#define  XE_TABBAR_SELECT    221  

/// @brief  TabBar标签按钮删除事件
/// @code   int CALLBACK OnTabBarDelete(int iItem, BOOL *pbHandled) @endcode
/// @param  iItem  标签位置索引.
#define  XE_TABBAR_DELETE    222  

/// @brief  月历元素日期改变事件
/// @code   int CALLBACK OnCalendarChange(BOOL *pbHandled) @endcode
#define  XE_MONTHCAL_CHANGE   231 

/// @brief  日期时间元素,内容改变事件
/// @code   int CALLBACK OnDateTimeChange(BOOL *pbHandled) @endcode
#define  XE_DATETIME_CHANGE    241   

/// @brief  日期时间元素,弹出月历卡片事件
/// @code   int CALLBACK OnDateTimePopupMonthCal(HWINDOW hMonthCalWnd,HELE hMonthCal,BOOL *pbHandled) @endcode
#define  XE_DATETIME_POPUP_MONTHCAL     242   

/// @brief  日期时间元素,弹出的月历卡片退出事件
/// @code   int CALLBACK OnDateTimeExitMonthCal(HWINDOW hMonthCalWnd,HELE hMonthCal,BOOL *pbHandled) @endcode
#define  XE_DATETIME_EXIT_MONTHCAL      243   

/// @brief  文件拖放事件, 需先启用:XWnd_EnableDragFiles()
/// @code   int CALLBACK OnDropFiles(HDROP hDropInfo, BOOL *pbHandled) @endcode
#define  XE_DROPFILES                 250  

//#define  XE_LISTVIEW_DRAG_INSERT
//#define  XE_PANE_LOACK
//#define  XE_PANE_DOCK
//#define  XE_PANE_FLOAT

/// @code   int CALLBACK OnEditColorChange(COLORREF color, BOOL *pbHandled) @endcode
#define  XE_EDIT_COLOR_CHANGE        260
///@}
//@隐藏}

//////////////////////////////////////////////////////////////////
//////////////////枚举定义/////////////////////////////////////////
//////////////////////////////////////////////////////////////////

/// @defgroup  group_enum   枚举类型
/// @{

/// @defgroup groupWindow_position 窗口位置(window_position_)
/// @{
enum   window_position_
{
	//@别名  错误
	window_position_error = -1,  ///<错误
	//@别名  顶部
	window_position_top = 0,     ///<top
	//@别名  底部
	window_position_bottom,    ///<bottom
	//@别名  左边
	window_position_left,      ///<left
	//@别名  右边
	window_position_right,     ///<right
	//@别名  内容区
	window_position_body,      ///<body
	//@别名  整个窗口
	window_position_window,    ///<window 整个窗口 
};
///@}

/// @defgroup groupElement_position UI元素位置(element_position_)
/// @{
enum element_position_  //@别名  UI元素位置
{
	//@别名  无
	element_position_no = 0x00,     ///<无效
	//@别名  左边
	element_position_left = 0x01,     ///<左边
	//@别名  上边
	element_position_top = 0x02,		///<上边
	//@别名  右边
	element_position_right = 0x04,		///<右边
	//@别名  下边
	element_position_bottom = 0X08,		///<下边
};
///@}

/// @defgroup group_position 位置标识(element_position_)
/// @{
enum position_flag_  //@别名  位置标识
{
	//@别名  左
	position_flag_left,          ///<左
	//@别名  上
	position_flag_top,           ///<上
	//@别名  右
	position_flag_right,         ///<右
	//@别名  下
	position_flag_bottom,        ///<下
	//@别名  左上角
	position_flag_leftTop,       ///<左上角
	//@别名  左下角
	position_flag_leftBottom,    ///<左下角
	//@别名  右上角
	position_flag_rightTop,      ///<右上角
	//@别名  右下角
	position_flag_rightBottom,   ///<右下角
	//@别名  中心
	position_flag_center,        ///<中心
};
///@}

//透明窗口
/// @defgroup groupWindowTransparent 窗口透明标识(window_transparent_)
/// @{
enum  window_transparent_  //@别名  窗口透明标识
{
	//@别名  不透明
	window_transparent_false = 0,   ///<默认窗口,不透明
	//@别名  透明
	window_transparent_shaped,    ///<透明窗口,带透明通道,异型
	//@别名  阴影
	window_transparent_shadow,    ///<阴影窗口,带透明通道,边框阴影,窗口透明或半透明 
	window_transparent_simple,    ///<透明窗口,不带透明通道,指定半透明度,指定透明色 
	window_transparent_win7,      ///<WIN7玻璃窗口,需要WIN7开启特效,当前未启用,当前未启用.
};
///@}


/// @defgroup groupMenu 菜单(menu)
/// @{

///@name 菜单项标识(menu_item_flag_)
///@{
enum   menu_item_flag_  //@别名  菜单项标识
{
	//@别名  正常
	menu_item_flag_normal = 0x00,   ///<正常
	//@别名  选择
	menu_item_flag_select = 0x01,   ///<已废弃
	//@别名  停留
	menu_item_flag_stay = 0x01,    ///<已废弃
	//@别名  勾选
	menu_item_flag_check = 0x02,   ///<勾选
	//@别名  弹出
	menu_item_flag_popup = 0x04,   ///<已废弃
	//@别名  分隔栏
	menu_item_flag_separator = 0x08,   ///<分隔栏 ID号任意,ID号被忽略
	//@别名  禁用
	menu_item_flag_disable = 0x10,   ///<禁用
};
///@}

///@name 菜单弹出方向(menu_popup_position_)
///@{
enum  menu_popup_position_  //@别名  菜单弹出方向
{
	//@别名  左上角
	menu_popup_position_left_top = 0,      ///<左上角
	//@别名  左下角
	menu_popup_position_left_bottom,     ///<左下角
	//@别名  右上角
	menu_popup_position_right_top,       ///<右上角
	//@别名  右下角
	menu_popup_position_right_bottom,    ///<右下角
	//@别名  左居中
	menu_popup_position_center_left,     ///<左居中
	//@别名  上居中
	menu_popup_position_center_top,      ///<上居中
	//@别名  右居中
	menu_popup_position_center_right,    ///<右居中
	//@别名  下居中
	menu_popup_position_center_bottom,   ///<下居中
};
///@}
///@}

/// @defgroup groupImageDrawType 图片绘制类型(image_draw_type_)
/// @{
enum  image_draw_type_  //@别名  图片绘制类型
{
	//@别名  默认
	image_draw_type_default = 0,     ///<默认
	//@别名  拉伸
	image_draw_type_stretch,       ///<拉伸
	//@别名  九宫格
	image_draw_type_adaptive,      ///<自适应,九宫格
	//@别名  平铺
	image_draw_type_tile,          ///<平铺
	//@别名  固定比例
	image_draw_type_fixed_ratio,   ///<固定比例,当图片超出显示范围时,按照原始比例压缩显示图片
	//@别名  九宫格外围
	image_draw_type_adaptive_border,  ///<九宫格不绘制中间区域
};
///@}


//状态--------------------------------------
/// @defgroup groupCommonState3 普通三种状态(common_state3_)
/// @{
enum  common_state3_   //@别名  普通三种状态
{
	//@别名  离开
	common_state3_leave = 0,  ///<离开
	//@别名  停留
	common_state3_stay,     ///<停留
	//@别名  按下
	common_state3_down,     ///<按下
};

///@}

/// @defgroup groupButtonState 按钮状态(button_state_)
/// @{
enum  button_state_  //@别名  按钮状态
{
	//@别名  离开
	button_state_leave = 0,   ///<离开状态
	//@别名  停留
	button_state_stay,      ///<停留状态
	//@别名  按下
	button_state_down,      ///<按下状态
	//@别名  选中
	button_state_check,     ///<选中状态
	//@别名  禁用
	button_state_disable,   ///<禁用状态
};
///@}

/// @defgroup   groupComboBoxState 组合框状态(comboBox_state_)
/// @{
enum  comboBox_state_ //@别名  组合框状态
{
	//@别名  鼠标离开
	comboBox_state_leave = 0,   ///<鼠标离开状态
	//@别名  鼠标停留
	comboBox_state_stay = 1,    ///<鼠标停留状态
	//@别名  按下
	comboBox_state_down = 2,    ///<按下状态
};
///@}

/// @defgroup groupListItemState 列表项状态(list_item_state_)
/// 适用于(列表,列表框,列表视图)
/// @{
enum  list_item_state_   //@别名  列表项状态
{
	//@别名  鼠标离开
	list_item_state_leave = 0,   ///<项鼠标离开状态
	//@别名  鼠标停留
	list_item_state_stay = 1,    ///<项鼠标停留状态
	//@别名  项选择
	list_item_state_select = 2,  ///<项选择状态
	//@别名  缓存的项
	list_item_state_cache = 3,   ///<缓存的项
};
///@}


/// @defgroup groupTreeItemState  列表树项状态(tree_item_state_)
/// @{
enum  tree_item_state_  //@别名  列表树项状态
{
	//@别名  鼠标离开
	tree_item_state_leave = 0,   ///<项鼠标离开状态
	//@别名  鼠标停留
	tree_item_state_stay = 1,    ///<项鼠标停留状态
	//@别名  项选择
	tree_item_state_select = 2,  ///<项选择状态
};
///@}


//按钮图标对齐方式
/// @defgroup groupButtonIconAlign 按钮图标对齐方式(button_icon_align_)
/// @{
enum  button_icon_align_  //@别名  按钮图标对齐方式
{
	//@别名  左边
	button_icon_align_left = 0,  ///<图标在左边
	//@别名  顶部
	button_icon_align_top,     ///<图标在顶部
	//@别名  右边
	button_icon_align_right,   ///<图标在右边
	//@别名  底部
	button_icon_align_bottom,  ///<图标在底部
};
///@}

/// @defgroup  groupListDrawItemBkFlag  项背景绘制标识(List,ListBox,ListView,Tree)
/// @{
enum  list_drawItemBk_flag_  //@别名  项背景绘制标识
{
	//@别名  不绘制
	list_drawItemBk_flag_nothing = 0x000,     ///<不绘制
	//@别名  鼠标离开
	list_drawItemBk_flag_leave = 0x001,       ///<绘制鼠标离开状态项背景
	//@别名  鼠标停留
	list_drawItemBk_flag_stay = 0x002,        ///<绘制鼠标停留状态项背景
	//@别名  项选择
	list_drawItemBk_flag_select = 0x004,      ///<绘制鼠标选择状态项背景 
	//@别名  组鼠标离开
	list_drawItemBk_flag_group_leave = 0x008, ///<绘制鼠标离开状态组背景,当项为组时
	//@别名  组鼠标停留
	list_drawItemBk_flag_group_stay = 0x010,  ///<绘制鼠标停留状态组背景,当项为组时
	//@别名  水平分割线
	list_drawItemBk_flag_line = 0x020,        ///<列表绘制水平分割线
	//@别名  垂直分割线
	list_drawItemBk_flag_lineV = 0x040,        ///<列表绘制垂直分割线

};
/// @}


//弹出消息框类型
/// @defgroup groupMessageBox 弹出消息框(messageBox_flag_)
/// @{
enum  messageBox_flag_  //@别名  弹出消息框标识
{
	//@别名  其他
	messageBox_flag_other = 0x00,    ///<其他
	//@别名  确定按钮
	messageBox_flag_ok = 0x01,    ///<确定按钮
	//@别名  取消按钮
	messageBox_flag_cancel = 0x02,    ///<取消按钮
	//@别名  图标应用程序
	messageBox_flag_icon_appicon = 0x01000,  ///<图标 应用程序  IDI_APPLICATION
	//@别名  图标信息
	messageBox_flag_icon_info = 0x02000,  ///<图标 信息     IDI_ASTERISK
	//@别名  图标问询
	messageBox_flag_icon_qustion = 0x04000,  ///<图标 问询/帮助/提问   IDI_QUESTION
	//@别名  图标错误
	messageBox_flag_icon_error = 0x08000,  ///<图标 错误/拒绝/禁止  IDI_ERROR
	//@别名  图标警告
	messageBox_flag_icon_warning = 0x10000,  ///<图标 警告       IDI_WARNING
	//@别名  图标安全
	messageBox_flag_icon_shield = 0x20000,  ///<图标 盾牌/安全   IDI_SHIELD
};
///@}

///@defgroup GroupPropertyGrid_item_type    属性网格项类型(propertyGrid_item_type_)
///@{
enum  propertyGrid_item_type_  //@别名  属性网格项类型
{
	//@别名  文本
	propertyGrid_item_type_text = 0,      ///<默认,字符串类型
	//@别名  编辑框
	propertyGrid_item_type_edit,        ///<编辑框
	//@别名  颜色选择
	propertyGrid_item_type_edit_color,  ///<颜色选择元素
	//@别名  文件选择
	propertyGrid_item_type_edit_file,   ///<文件选择编辑框
	//@别名  设置
	propertyGrid_item_type_edit_set,    ///<设置
	//@别名  组合框
	propertyGrid_item_type_comboBox,    ///<组合框
	//@别名  组
	propertyGrid_item_type_group,       ///<组
	//@别名  面板
	propertyGrid_item_type_panel,       ///<面板
};
///@}

///@defgroup  GroupZOrder    Z序位置(zorder_)
///@{
enum zorder_  //@别名  Z序位置
{
	//@别名  最上面
	zorder_top,    ///<最上面
	//@别名  最下面
	zorder_bottom, ///<最下面
	//@别名  指定目标下面
	zorder_before, ///<指定目标下面
	//@别名  指定目标上面
	zorder_after,  ///<指定目标上面
};
///@}

///@defgroup Group_pane_align_  窗格对齐(pane_align_)
///@{
enum pane_align_  //@别名  窗格对齐
{
	//@别名  错误
	pane_align_error = -1,  ///<错误
	//@别名  左侧
	pane_align_left = 0,    ///<左侧
	//@别名  顶部
	pane_align_top,       ///<顶部
	//@别名  右侧
	pane_align_right,     ///<右侧
	//@别名  底部
	pane_align_bottom,    ///<底部
	//@别名  居中
	pane_align_center,    ///<居中
};
///@}

///@defgroup Group_layout_align_  布局对齐(layout_align_)
///@{
enum  layout_align_  //@别名  布局对齐
{
	//@别名  左侧
	layout_align_left = 0,		  ///<左侧
	//@别名  顶部
	layout_align_top,			  ///<顶部
	//@别名  右侧
	layout_align_right,			  ///<右侧
	//@别名  底部
	layout_align_bottom,		  ///<底部
	//@别名  居中
	layout_align_center,		  ///<居中
	//@别名  等距
	layout_align_equidistant,     ///<等距
};
///@}

/// @defgroup groupLayoutSize 布局大小类型(layout_size_)
/// @{
enum  layout_size_ //@别名  布局大小类型
{
	//@别名  固定
	layout_size_fixed = 0, ///<固定大小
	//@别名  填充父
	layout_size_fill,      ///<fill 填充父
	//@别名  自动
	layout_size_auto,      ///<auto 自动大小,根据内容计算大小
	//@别名  比例
	layout_size_weight,    ///<weight 比例,按照比例分配剩余空间
	//@别名  百分比
	layout_size_percent,   ///<百分比
	//@别名  禁用
	layout_size_disable,   ///<disable 不使用
};
///@}

/// @defgroup  groupLayoutAlignAxis 布局轴对齐(layout_align_axis_)
/// @{
enum layout_align_axis_  //@别名  布局轴对齐
{
	//@别名  无
	layout_align_axis_auto = 0,   ///<无
	//@别名  开始
	layout_align_axis_start,      ///<水平布局(顶部), 垂直布局(左侧)
	//@别名  居中
	layout_align_axis_center,     ///<居中
	//@别名  末尾
	layout_align_axis_end,        ///<水平布局(底部), 垂直布局(右侧)
};
///@}

///@defgroup Group_edit_textAlign_flag_  编辑框文本对齐(edit_textAlign_flag_)
///@{
enum edit_textAlign_flag_ //@别名  编辑框文本对齐
{
	//@别名  左侧
	edit_textAlign_flag_left = 0x0,   ///<左侧
	//@别名  右侧
	edit_textAlign_flag_right = 0x1,   ///<右侧
	//@别名  水平居中
	edit_textAlign_flag_center = 0x2,   ///<水平居中
	//@别名  顶部
	edit_textAlign_flag_top = 0x0,  ///<顶部
	//@别名  底部
	edit_textAlign_flag_bottom = 0x4,  ///<底部
	//@别名  垂直居中
	edit_textAlign_flag_center_v = 0x8,  ///<垂直居中
};
///@}

///@defgroup Group_pane_state_  窗格状态(pane_state_)
///@{
enum pane_state_  //@别名   窗格状态
{
	//@别名  错误
	pane_state_error = -1,
	//@别名  任意
	pane_state_any = 0,
	//@别名  锁定
	pane_state_lock,   ///<锁定
	//@别名  停靠码头
	pane_state_dock,   ///<停靠码头
	//@别名  浮动窗格
	pane_state_float,  ///<浮动窗格
};
///@}

///@defgroup Group_textFormatFlag_    文本对齐(textFormatFlag_)
///@{
enum  textFormatFlag_  //@别名   文本对齐
{
	//@别名  左对齐
	textAlignFlag_left = 0,      ///<左对齐
	//@别名  顶对齐
	textAlignFlag_top = 0,      ///<垂直顶对齐
	textAlignFlag_left_top = 0x4000, ///<内部保留
	//@别名  水平居中
	textAlignFlag_center = 0x1,    ///<水平居中
	//@别名  右对齐
	textAlignFlag_right = 0x2,    ///<右对齐.
	//@别名  垂直居中
	textAlignFlag_vcenter = 0x4,    ///<垂直居中
	//@别名  底对齐
	textAlignFlag_bottom = 0x8,    ///<垂直底对齐
	//@别名  从右向左
	textFormatFlag_DirectionRightToLeft = 0x10,   ///<从右向左顺序显示文本
	//@别名  禁止换行
	textFormatFlag_NoWrap = 0x20,   ///<禁止换行
	//@别名  垂直显示
	textFormatFlag_DirectionVertical = 0x40,   ///<垂直显示文本
	//@别名  向外延伸
	textFormatFlag_NoFitBlackBox = 0x80,   ///<允许部分字符延伸该字符串的布局矩形。默认情况下，将重新定位字符以避免任何延伸
	//@别名  显示控制字符
	textFormatFlag_DisplayFormatControl = 0x100,  ///<控制字符（如从左到右标记）随具有代表性的标志符号一起显示在输出中。
	//@别名  禁止回退字体
	textFormatFlag_NoFontFallback = 0x200,      ///<对于请求的字体中不支持的字符，禁用回退到可选字体。缺失的任何字符都用缺失标志符号的字体显示，通常是一个空的方块
	//@别名  测量包含空格
	textFormatFlag_MeasureTrailingSpaces = 0x400, ///<包括每一行结尾处的尾随空格。在默认情况下，MeasureString 方法返回的边框都将排除每一行结尾处的空格。设置此标记以便在测定时将空格包括进去
	//@别名  禁止超过行高
	textFormatFlag_LineLimit = 0x800,      ///<如果内容显示高度不够一行,那么不显示
	//@别名  禁止裁剪
	textFormatFlag_NoClip = 0x1000,     ///<允许显示标志符号的伸出部分和延伸到边框外的未换行文本。在默认情况下，延伸到边框外侧的所有文本和标志符号部分都被剪裁

	//textTrimming_None              = 0,       ///<不使用去尾
	//@别名  以字符为单位去尾
	textTrimming_Character = 0x40000,          ///<以字符为单位去尾
	//@别名  以单词为单位去尾
	textTrimming_Word = 0x80000,  ///<以单词为单位去尾
	//@别名  以字符为单位去尾加省略号
	textTrimming_EllipsisCharacter = 0x8000,   ///<以字符为单位去尾,省略部分使用且略号表示
	//@别名  以单词为单位去尾加省略号
	textTrimming_EllipsisWord = 0x10000,  ///<以单词为单位去尾,省略部分使用省略号表示
	//@别名  省略中间部分
	textTrimming_EllipsisPath = 0x20000,  ///<略去字符串中间部分，保证字符的首尾都能够显示  
};
///@}

///@defgroup Group_textFormatFlag_dwrite_    D2D文本渲染模式(XC_DWRITE_RENDERING_MODE)
///@{
enum XC_DWRITE_RENDERING_MODE  //@别名 D2D文本渲染模式
{
	//@别名  默认
	XC_DWRITE_RENDERING_MODE_DEFAULT = 0,                    ///<指定根据字体和大小自动确定呈现模式。
	//@别名  不抗锯齿
	XC_DWRITE_RENDERING_MODE_ALIASED,						 ///<指定不执行抗锯齿。 每个像素要么设置为文本的前景色，要么保留背景的颜色。
	//@别名  CLEARTYPE_GDI_CLASSIC
	XC_DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC,			 ///<使用与别名文本相同的度量指定 ClearType 呈现。 字形只能定位在整个像素的边界上。
	//@别名  CLEARTYPE_GDI_NATURAL
	XC_DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL,			 ///<使用使用 CLEARTYPE_NATURAL_QUALITY 创建的字体，使用与使用 GDI 的文本呈现相同的指标指定 ClearType 呈现。 与使用别名文本相比，字形度量更接近其理想值，但字形仍然位于整个像素的边界上。
	//@别名  CLEARTYPE_NATURAL
	XC_DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL,				 ///<仅在水平维度中指定具有抗锯齿功能的 ClearType 渲染。这通常用于中小字体大小（最多 16 ppem）。
	//@别名  CLEARTYPE_NATURAL_SYMMETRIC
	XC_DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC,	 ///<指定在水平和垂直维度上具有抗锯齿的 ClearType 渲染。这通常用于较大的尺寸，以使曲线和对角线看起来更平滑，但会牺牲一些柔和度。
	//@别名  OUTLINE
	XC_DWRITE_RENDERING_MODE_OUTLINE,                        ///<指定渲染应绕过光栅化器并直接使用轮廓。 这通常用于非常大的尺寸。
};
///@}

///@defgroup Group_listItemTemp_type_    列表项模板类型(listItemTemp_type_)
///@{
enum listItemTemp_type_  //@别名 列表项模板类型
{
	//@别名 列表树
	listItemTemp_type_tree = 0x01,					  ///<tree
	//@别名 列表框
	listItemTemp_type_listBox = 0x02,				  ///<listBox
	//@别名 列表头
	listItemTemp_type_list_head = 0x04,				  ///<list 列表头
	//@别名 列表项
	listItemTemp_type_list_item = 0x08,				  ///<list 列表项
	//@别名 列表视图组
	listItemTemp_type_listView_group = 0x10,		  ///<listView 列表视组
	//@别名 列表视图项
	listItemTemp_type_listView_item = 0x20,			  ///<listView 列表视项
	//@别名 列表头和列表项
	listItemTemp_type_list = listItemTemp_type_list_head | listItemTemp_type_list_item,  ///<list (列表头)与(列表项)组合
	//@别名 列表视图组和项
	listItemTemp_type_listView = listItemTemp_type_listView_group | listItemTemp_type_listView_item, ///<listView (列表视组)与(列表视项)组合
};
///@}

///@defgroup Group_adjustLayout    调整布局标识位(adjustLayout_)
///@{
enum adjustLayout_  //@别名 调整布局标识
{
	//@别名  不调整
	adjustLayout_no = 0x00,  ///<不调整布局
	//@别名  强制调整全部
	adjustLayout_all = 0x01,  ///<强制调整自身和子对象布局.
	//@别名  只调整自身
	adjustLayout_self = 0x02,  ///<只调整自身布局,不调整子对象布局.
	//xc_adjustLayout_free = 0x03   调整布局,非强制性, 只调整坐标改变的对象
};
///@}

/// @defgroup group_edit_macro 编辑框类型(edit_type_)
/// @{
enum edit_type_     //@别名 编辑框类型
{
	//@别名  普通
	edit_type_none = 0,   ///<普通编辑框,   每行的高度相同
	//@别名  代码
	edit_type_editor,     ///<代码编辑框,   每行的高度相同,  功能继承普通编辑框
	//@别名  富文本
	edit_type_richedit,   ///<富文本编辑框,  每行的高度可能不同
	//@别名  聊天气泡
	edit_type_chat,       ///<聊天气泡,     每行的高度可能不同, 功能继承富文本编辑框
	//@别名  代码表格
	edit_type_codeTable,  ///<代码表格,内部使用,  每行的高度相同, 未使用
};

enum edit_style_type_  //@别名 编辑框样式类型
{
	//@别名  字体
	edit_style_type_font_color = 1,  ///<字体
	//@别名  图片
	edit_style_type_image,           ///<图片
	//@别名  UI对象
	edit_style_type_obj,             ///<UI对象
};

///Edit 聊天气泡行标识
enum chat_flag_  //@别名  聊天气泡行标识
{
	//@别名  左侧
	chat_flag_left = 0x1,    ///<左侧
	//@别名  右侧
	chat_flag_right = 0x2,   ///<右侧
	//@别名  中间
	chat_flag_center = 0x4,  ///<中间
	//@别名  下一行显示气泡
	chat_flag_next_row_bubble = 0x8,     ///<下一行显示气泡
	//@别名  气泡行
	chat_flag_chat = 0x10000, ///<气泡(气泡行)
};
///@}

/// @defgroup group_table  形状表格标识(table_flag_)
/// @{

///@name  形状表格标识(table_flag_)
///@{
enum  table_flag_  //@别名  形状表格标识
{
	//@别名  正常
	table_flag_none = 0x0,       ///<正常最小单元格
	//@别名  铺满
	table_flag_full = 0x1,       ///<铺满组合单元格
	//@别名  编辑
	table_flag_editable = 0x2,   ///<支持编辑文本内容
	//@别名  自动匹配
	table_flag_automatch = 0x40, ///<支持自动匹配
};
///@}

///@name  形状表格线标识(table_line_flag_)
///@{
enum  table_line_flag_   //@别名  形状表格线标识
{
	table_line_flag_left = 0x1,   ///<待补充
	table_line_flag_top = 0x2,   ///<待补充
	table_line_flag_right = 0x4,	 ///<待补充
	table_line_flag_bottom = 0x8,   ///<待补充
	table_line_flag_left2 = 0x10,  ///<待补充
	table_line_flag_top2 = 0x20,	 ///<待补充
	table_line_flag_right2 = 0x40,	 ///<待补充
	table_line_flag_bottom2 = 0x80,	 ///<待补充
};
///@}
///@}

/// @defgroup group_monthCal_button_type_    月历元素上的按钮类型(monthCal_button_type_)
/// @{
enum monthCal_button_type_  //@别名 月历卡片按钮类型
{
	//@别名  今天
	monthCal_button_type_today = 0,  ///< 今天按钮
	//@别名  上一年
	monthCal_button_type_last_year,  ///< 上一年
	//@别名  下一年
	monthCal_button_type_next_year,  ///< 下一年
	//@别名  上一月
	monthCal_button_type_last_month, ///< 上一月
	//@别名  下一月
	monthCal_button_type_next_month, ///< 下一月
};
///@}

///@defgroup  group_fontStyle_  字体样式(fontStyle_)
///@{
enum fontStyle_  //@别名 字体样式
{
	//@别名  正常
	fontStyle_regular = 0,     ///<正常
	//@别名  粗体
	fontStyle_bold = 1,        ///<粗体
	//@别名  斜体
	fontStyle_italic = 2,      ///<斜体
	//@别名  粗斜体
	fontStyle_boldItalic = 3,  ///<粗斜体
	//@别名  下划线
	fontStyle_underline = 4,   ///<下划线
	//@别名  删除线
	fontStyle_strikeout = 8    ///<删除线
};
///@}

///@defgroup  group_adapter_date_type_    数据适配器数据类型(adapter_date_type_)
///@{
enum  adapter_date_type_  //@别名  数据适配器数据类型
{
	//@别名  错误
	adapter_date_type_error = -1,
	//@别名  整型
	adapter_date_type_int = 0,     ///<整型
	//@别名  浮点型
	adapter_date_type_float = 1,   ///<浮点型
	//@别名  字符串
	adapter_date_type_string = 2,  ///<字符串
	//@别名  图片
	adapter_date_type_image = 3,   ///<图片
};
///@}

/// @defgroup group_ease_type_  缓动类型(ease_type_)
/// @{
enum ease_type_ //@别名  缓动类型
{
	//@别名  从慢到快
	easeIn,      ///<从慢到快
	//@别名  从快到慢
	easeOut,     ///<从快到慢
	//@别名  从慢到快再到慢
	easeInOut,   ///<从慢到快再到慢
};
///@}

///@defgroup   group_ease_flag_  缓动标识(ease_flag_)
///@{
enum ease_flag_  //@别名  缓动标识
{
	//@别名  线性
	ease_flag_linear,			///<线性, 直线
	//@别名  二次方曲线
	ease_flag_quad,			    ///<二次方曲线
	//@别名  三次方曲线
	ease_flag_cubic,			///<三次方曲线, 圆弧
	//@别名  四次方曲线
	ease_flag_quart,			///<四次方曲线
	//@别名  五次方曲线
	ease_flag_quint,			///<五次方曲线
	//@别名  正弦
	ease_flag_sine,				///<正弦, 在末端变化
	//@别名  突击
	ease_flag_expo,			    ///<突击, 突然一下
	//@别名  圆环
	ease_flag_circ,		        ///<圆环, 好比绕过一个圆环
	//@别名  强力回弹
	ease_flag_elastic,		    ///<强力回弹
	//@别名  回弹
	ease_flag_back,				///<回弹, 比较缓慢
	//@别名  弹跳
	ease_flag_bounce,		    ///<弹跳, 模拟小球落地弹跳
	//@别名  从慢到快
	ease_flag_in = 0x010000, ///<从慢到快
	//@别名  从快到慢
	ease_flag_out = 0x020000, ///<从快到慢
	//@别名  从慢到快再到慢
	ease_flag_inOut = 0x030000, ///<从慢到快再到慢
};
///@}

///@defgroup   group_notifyMsg_skin_  通知消息外观(notifyMsg_skin_)
///@{
enum  notifyMsg_skin_  //@别名 通知消息外观
{
	//@别名  默认
	notifyMsg_skin_no,         ///<默认
	//@别名  成功
	notifyMsg_skin_success,    ///<成功
	//@别名  警告
	notifyMsg_skin_warning,	   ///<警告
	//@别名  消息
	notifyMsg_skin_message,	   ///<消息
	//@别名  错误
	notifyMsg_skin_error,	   ///<错误
};
///@}

///@defgroup group_animation_move_  动画移动标识(animation_move_)
///@{
enum animation_move_  //@别名 动画移动标识
{
	//@别名 X轴移动
	animation_move_x = 0x01,   ///<X轴移动
	//@别名 Y轴移动
	animation_move_y = 0x02,   ///<Y轴移动
};
///@}

///@defgroup group_bkInfo_align_flag_  背景对象对齐方式(bkObject_align_flag_)
///@{
enum bkObject_align_flag_  //@别名 背景对象对齐方式
{
	//@别名  无
	bkObject_align_flag_no = 0x000,    ///<无
	//@别名  左对齐
	bkObject_align_flag_left = 0x001,    ///<左对齐, 当设置此标识时,外间距(margin.left)代表左侧间距; 当right未设置时,那么外间距(margin.right)代表宽度;
	//@别名  顶对齐
	bkObject_align_flag_top = 0x002,    ///<顶对齐, 当设置此标识时,外间距(margin.top)代表顶部间距; 当bottom未设置时,那么外间距(margin.bottom)代表高度;
	//@别名  右对齐
	bkObject_align_flag_right = 0x004,    ///<右对齐, 当设置此标识时,外间距(margin.right)代表右侧间距; 当left未设置时,那么外间距(margin.left)代表宽度;
	//@别名  底对齐
	bkObject_align_flag_bottom = 0x008,    ///<底对齐, 当设置此标识时,外间距(margin.bottom)代表底部间距; 当top未设置时,那么外间距(margin.top)代表高度;
	//@别名  水平居中
	bkObject_align_flag_center = 0x010,    ///<水平居中, 当设置此标识时,外间距(margin.left)代表宽度;
	//@别名  垂直居中
	bkObject_align_flag_center_v = 0x020,  ///<垂直居中, 当设置此标识时,外间距(margin.top)代表高度; 
};
///@}


///@defgroup group_frameWnd_cell_type_  框架窗口单元格类型(frameWnd_cell_type_)
enum frameWnd_cell_type_  //@别名  框架窗口单元格类型
{
	//@别名  无
	frameWnd_cell_type_no = 0,   ///<无
	//@别名  窗格
	frameWnd_cell_type_pane = 1,   ///<窗格
	//@别名  窗格组
	frameWnd_cell_type_group = 2,   ///<窗格组
	//@别名  主视图区
	frameWnd_cell_type_bodyView = 3,   ///<主视图区
	//@别名  上下布局
	frameWnd_cell_type_top_bottom = 4,   ///<上下布局
	//@别名  左右布局
	frameWnd_cell_type_left_right = 5,   ///<左右布局
};

///@defgroup group_trayIcon_flag_  托盘图标标识(trayIcon_flag_)
enum  trayIcon_flag_   //@别名   托盘图标标识
{
	//@别名  无图标
	trayIcon_flag_icon_none = 0x0,      ///<无图标 NIIF_NONE
	//@别名  信息图标
	trayIcon_flag_icon_info = 0x1,     ///<信息图标 NIIF_INFO
	//@别名  警告图标
	trayIcon_flag_icon_warning = 0x2,   ///<警告图标 NIIF_WARNING
	//@别名  错误图标
	trayIcon_flag_icon_error = 0x3,     ///<错误图标 NIIF_ERROR
	//@别名  用户图标
	trayIcon_flag_icon_user = 0x4,      ///<用户指定的图标 NIIF_USER
	//@别名  禁止播放声音
	trayIcon_flag_nosound = 0x10,       ///<禁止播放气泡声音 NIIF_NOSOUND
};

///@}  //枚举类型

/////////////////////////////////////////////////////////////////////
//////////////炫彩组合状态/////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

//@分组{ 组合状态

/// @defgroup group_combo_StateFlag  组合状态
/// @{

///@defgroup group_window_state_flag_  窗口状态(window_state_flag_)
///@{
enum  window_state_flag_  //@别名  窗口状态
{
	//@别名  无
	window_state_flag_nothing = 0x0000,  ///<无
	//@别名  整个窗口
	window_state_flag_leave = 0x0001,  ///<整个窗口
	//@别名  内容区
	window_state_flag_body_leave = 0x0002,  ///<窗口-body
	//@别名  顶部
	window_state_flag_top_leave = 0x0004,  ///<窗口-top
	//@别名  底部
	window_state_flag_bottom_leave = 0x0008,  ///<窗口-bottom
	//@别名  左侧
	window_state_flag_left_leave = 0x0010,  ///<窗口-left
	//@别名  右侧
	window_state_flag_right_leave = 0x0020,  ///<窗口-right
	//@别名  布局内容区
	window_state_flag_layout_body = 0x20000000, ///<布局内容区
};
///@}

///@defgroup  group_element_state_flag_ 元素状态(element_state_flag_)
///@{
enum  element_state_flag_  //@别名  元素状态
{
	//@别名  无
	element_state_flag_nothing = window_state_flag_nothing,  ///<无
	//@别名  启用
	element_state_flag_enable = 0x0001,  ///<启用
	//@别名  禁用
	element_state_flag_disable = 0x0002,  ///<禁用
	//@别名  焦点
	element_state_flag_focus = 0x0004,  ///<焦点
	//@别名  无焦点
	element_state_flag_focus_no = 0x0008,  ///<无焦点
	//@别名  焦点扩展
	element_state_flag_focusEx = 0x40000000,  ///<该元素或该元素的子元素拥有焦点
	//@别名  无焦点扩展
	element_state_flag_focusEx_no = 0x80000000,  ///<无焦点Ex
	//@别名  布局内容区
	layout_state_flag_layout_body = window_state_flag_layout_body, ///<布局内容区
	//@别名  鼠标离开
	element_state_flag_leave = 0x0010,
	//@别名  鼠标停留
	element_state_flag_stay = 0x0020,
	//@别名  鼠标按下
	element_state_flag_down = 0x0040,
};
///@}

///@defgroup  group_button_state_flag_ 按钮状态标识(button_state_flag_)
///@{
enum  button_state_flag_  //@别名  按钮状态标识
{
	//@别名  鼠标离开
	button_state_flag_leave = element_state_flag_leave,  ///<鼠标离开
	//@别名  鼠标停留
	button_state_flag_stay = element_state_flag_stay,   ///<鼠标停留
	//@别名  鼠标按下
	button_state_flag_down = element_state_flag_down,   ///<鼠标按下
	//@别名  选中
	button_state_flag_check = 0x0080, ///<选中
	//@别名  未选中
	button_state_flag_check_no = 0x0100, ///<未选中
	//@别名  窗口还原
	button_state_flag_WindowRestore = 0x0200, ///<窗口还原
	//@别名  窗口最大化
	button_state_flag_WindowMaximize = 0x0400, ///<窗口最大化
};
///@}

///@defgroup   group_comboBox_state_flag_  组合框状态标识(comboBox_state_flag_)
///@{
enum comboBox_state_flag_   //@别名   组合框状态标识
{
	//@别名  鼠标离开
	comboBox_state_flag_leave = element_state_flag_leave, ///<鼠标离开
	//@别名  鼠标停留
	comboBox_state_flag_stay = element_state_flag_stay,  ///<鼠标停留
	//@别名  鼠标按下
	comboBox_state_flag_down = element_state_flag_down,  ///<鼠标按下
};
///@}

///@defgroup   group_listBox_state_flag_  列表框状态(listBox_state_flag_)
///@{
enum listBox_state_flag_  //@别名  列表框状态
{
	//@别名  项鼠标离开
	listBox_state_flag_item_leave = 0x0080, ///<项鼠标离开
	//@别名  项鼠标停留
	listBox_state_flag_item_stay = 0x0100, ///<项鼠标停留
	//@别名  项选择
	listBox_state_flag_item_select = 0x0200, ///<项选择
	//@别名  项未选择
	listBox_state_flag_item_select_no = 0x0400, ///<项未选择
};
///@}

///@defgroup   group_list_state_flag_  列表状态(list_state_flag_)
///@{
enum list_state_flag_  //@别名  列表状态
{
	//@别名  项鼠标离开
	list_state_flag_item_leave = 0x0080, ///<项鼠标离开
	//@别名  项鼠标停留
	list_state_flag_item_stay = 0x0100, ///<项鼠标停留
	//@别名  项选择
	list_state_flag_item_select = 0x0200, ///<项选择
	//@别名  项未选择
	list_state_flag_item_select_no = 0x0400, ///<项未选择
};
///@}

///@defgroup   group_listHeader_state_flag_  列表头状态(listHeader_state_flag_)
///@{
enum listHeader_state_flag_  //@别名  列表头状态
{
	//@别名  项鼠标离开
	listHeader_state_flag_item_leave = 0x0080, ///<项鼠标离开
	//@别名  项鼠标停留
	listHeader_state_flag_item_stay = 0x0100, ///<项鼠标停留
	//@别名  项鼠标按下
	listHeader_state_flag_item_down = 0x0200, ///<项鼠标按下
};
///@}

///@defgroup   group_listView_state_flag_ 列表视图状态(listView_state_flag_)
///@{
enum listView_state_flag_  //@别名  列表视图状态
{
	//@别名  项鼠标离开
	listView_state_flag_item_leave = 0x0080,  ///<项鼠标离开
	//@别名  项鼠标停留
	listView_state_flag_item_stay = 0x0100,  ///<项鼠标停留
	//@别名  项选择
	listView_state_flag_item_select = 0x0200,  ///<项选择
	//@别名  项未选择
	listView_state_flag_item_select_no = 0x0400,  ///<项未选择
	//@别名  组鼠标离开
	listView_state_flag_group_leave = 0x0800,  ///<组鼠标离开
	//@别名  组鼠标停留
	listView_state_flag_group_stay = 0x1000,  ///<组鼠标停留
	//@别名  组选择
	listView_state_flag_group_select = 0x2000,  ///<组选择
	//@别名  组未选择
	listView_state_flag_group_select_no = 0x4000,  ///<组未选择
};
///@}

///@defgroup   group_tree_state_flag_ 列表树状态(tree_state_flag_)
///@{
enum tree_state_flag_  //@别名  列表树状态
{
	//@别名  项鼠标离开
	tree_state_flag_item_leave = 0x0080,  ///<项鼠标离开
	//@别名  项鼠标停留
	tree_state_flag_item_stay = 0x0100,  ///<项鼠标停留,保留值, 暂未使用
	//@别名  项选择
	tree_state_flag_item_select = 0x0200,  ///<项选择
	//@别名  项未选择
	tree_state_flag_item_select_no = 0x0400,  ///<项未选择
	//@别名  项为组
	tree_state_flag_group = 0x0800,  ///<项为组
	//@别名  项不为组
	tree_state_flag_group_no = 0x1000,  ///<项不为组
};
///@}

///@defgroup   group_monthCal_state_flag_  月历卡片状态(monthCal_state_flag_)
///@{
enum monthCal_state_flag_     //@别名  月历卡片状态
{
	//@别名  鼠标离开
	monthCal_state_flag_leave = element_state_flag_leave,  ///<离开状态
	//@别名  项鼠标离开
	monthCal_state_flag_item_leave = 0x0080,     ///< 项-离开
	//@别名  项鼠标停留
	monthCal_state_flag_item_stay = 0x0100,     ///< 项-停留
	//@别名  项鼠标按下
	monthCal_state_flag_item_down = 0x0200,     ///< 项-按下
	//@别名  项选择
	monthCal_state_flag_item_select = 0x0400,     ///< 项-选择
	//@别名  项未选择
	monthCal_state_flag_item_select_no = 0x0800,     ///< 项-未选择
	//@别名  项今天
	monthCal_state_flag_item_today = 0x1000,     ///< 项-今天
	//@别名  项上月
	monthCal_state_flag_item_last_month = 0x2000,     ///< 项-上月
	//@别名  项当月
	monthCal_state_flag_item_cur_month = 0x4000,     ///< 项-当月
	//@别名  项下月
	monthCal_state_flag_item_next_month = 0x8000,     ///< 项-下月
};
///@}

///@defgroup   group_propertyGrid_state_flag_  属性网格状态(propertyGrid_state_flag_)
///@{
enum propertyGrid_state_flag_      //@别名   属性网格状态
{
	//@别名  项鼠标离开
	propertyGrid_state_flag_item_leave = 0x0080,  	 ///<离开
	//@别名  项鼠标停留
	propertyGrid_state_flag_item_stay = 0x0100,	 ///<停留
	//@别名  项选择
	propertyGrid_state_flag_item_select = 0x0200,	 ///<选择
	//@别名  项未选择
	propertyGrid_state_flag_item_select_no = 0x0400,	 ///<未选择
	//@别名  组鼠标离开
	propertyGrid_state_flag_group_leave = 0x0800,	 ///<组离开
	//@别名  组展开
	propertyGrid_state_flag_group_expand = 0x1000,	 ///<组展开
	//@别名  组未展开
	propertyGrid_state_flag_group_expand_no = 0x2000,	 ///<组未展开
};
///@}

///@defgroup   group_pane_state_flag_  窗格状态标识(pane_state_flag_)
///@{
enum pane_state_flag_      //@别名  窗格状态标识
{
	//@别名  鼠标离开
	pane_state_flag_leave = element_state_flag_leave,  ///<离开
	//@别名  鼠标停留
	pane_state_flag_stay = element_state_flag_stay,   ///<停留
	//@别名  标题栏
	pane_state_flag_caption = 0x0080,  ///<标题
	//@别名  内容区
	pane_state_flag_body = 0x0100,  ///<内容区
};
///@}

///@defgroup   group_layout_state_flag_  布局状态(layout_state_flag_)
///@{
enum layout_state_flag_  //@别名  布局状态
{
	//@别名  无
	layout_state_flag_nothing = window_state_flag_nothing,  ///<无
	//@别名  完整背景
	layout_state_flag_full = 0x0001,   ///<完整背景
	//@别名  内容区
	layout_state_flag_body = 0x0002,   ///<内容区域, 不包含边大小
};
///@}

///@defgroup   group_menu_item_state_flag_  弹出菜单项状态(menu_item_state_flag_)
///@{
enum   menu_item_state_flag_  //@别名  弹出菜单项状态
{
	//@别名  正常
	menu_item_state_flag_leave = 0x80,    ///<正常
	//@别名  停留
	menu_item_state_flag_stay = 0x01,      ///<鼠标停留
	//@别名  勾选
	menu_item_state_flag_check = 0x02,     ///<勾选
	//@别名  未勾选
	menu_item_state_flag_check_no = 0x20,  ///<未勾选
	//@别名  弹出
	menu_item_state_flag_popup = 0x04,     ///<弹出
	//@别名  非弹出
	menu_item_state_flag_popup_no = 0x40,  ///<非弹出

	//@别名  分隔栏
	menu_item_state_flag_separator = 0x08,  ///<分隔栏 ID号任意,ID号被忽略
	//@别名  禁用
	menu_item_state_flag_disable = 0x10,   ///<禁用
	//@别名  启用
	menu_item_state_flag_enable = 0x100,   ///<启用
};
///@}

///@defgroup   group_progressBar_state_flag_  进度条状态(progressBar_state_flag_)
///@{
enum progressBar_state_flag_  //@别名  进度条状态
{
	//@别名  进度背景
	progressBar_state_flag_background = 0x080,  ///<进度条进度背景
	//@别名  进度前景
	progressBar_state_flag_foreground = 0x100,  ///<进度条进度前景
};
///@}
///@}

//@分组} //组合状态

/////////////////////////////////////////////////////////////////////
//////////////定义结构体///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

///@defgroup  group_struct_  结构体定义
///@{

typedef   struct lua_State lua_State;

struct  RECTF
{
	float  left;
	float  top;
	float  right;
	float  bottom;
};

//@别名 边大小
struct borderSize_  //4条边的大小
{
	int  leftSize;   ///<左边大小
	int  topSize;    ///<上边大小
	int  rightSize;  ///<右边大小
	int  bottomSize; ///<下边大小
};
typedef  borderSize_  spaceSize_, paddingSize_, marginSize_;

///位置点
struct  position_
{
	int  iRow;    ///<行索引
	int  iColumn; ///<列索引
};

///ListBox 列表框项信息
//@别名  列表框项结构
struct listBox_item_
{
	//@别名  项索引
	int     index;      ///<项索引
	//@别名  用户数据
	vint    nUserData;  ///<用户绑定数据
	//@别名  项默认高度
	int     nHeight;    ///<项默认高度
	//@别名  项选中时高度
	int     nSelHeight; ///<项选中时高度
	//@别名  状态
	list_item_state_  nState;  ///<状态
	//@别名  项坐标
	RECT    rcItem;     ///<项坐标
	//@别名  布局元素
	HELE    hLayout;    ///<布局元素
	//@别名  项模板
	HTEMP   hTemp;      ///<列表项模板
};

///ListBox 列表框项信息
//@别名  列表框项信息结构
struct listBox_item_info_
{
	//@别名  用户数据
	vint    nUserData;  ///<用户绑定数据
	//@别名  项默认高度
	int     nHeight;    ///<项高度, -1使用默认高度
	//@别名  项选中时高度
	int     nSelHeight; ///<项选中时高度, -1使用默认高度
};

///ListView 列表视项ID
//@别名  列表视图项ID结构
struct  listView_item_id_
{
	//@别名  组索引
	int  iGroup;   ///<组索引
	//@别名  项索引
	int  iItem;    ///<项索引
};

///List 列表项信息
//@别名  列表项结构
struct list_item_
{
	//@别名  行索引
	int     index;             ///<行索引
	//@别名  子项索引
	int     iSubItem;          ///<子项索引(列索引)
	//@别名  用户数据
	vint    nUserData;         ///<用户数据
	//@别名  状态
	list_item_state_  nState;  ///<状态
	//@别名  项坐标
	RECT    rcItem;     ///<未使用
	//@别名  布局元素
	HELE    hLayout;    ///<布局元素
	//@别名  项模板
	HTEMP   hTemp;      ///<列表项模板
};

///List 列表头项信息
//@别名 列表头项结构
struct list_header_item_
{
	//@别名  项索引
	int     index;           ///<项索引
	//@别名  用户数据
	vint    nUserData;       ///<用户数据
	//@别名  是否排序
	BOOL    bSort;           ///<是否支持排序
	//@别名  排序方式
	int     nSortType;       ///<排序方式,0无效,1升序,2降序
	//@别名  数据适配器列索引
	int     iColumnAdapter;  ///<对应数据适配器中的列索引
	//@别名  状态
	common_state3_  nState;  ///<状态
	//@别名  项坐标
	RECT    rcItem;        ///<坐标
	//@别名  布局元素
	HELE    hLayout;       ///<布局元素
	//@别名  项模板
	HTEMP   hTemp;         ///<列表项模板
};

///Tree 树项信息
//@别名  列表树项结构
struct tree_item_
{
	//@别名  项ID
	int     nID;				 ///<项ID
	//@别名  深度
	int     nDepth;				 ///<深度
	//@别名  项高度
	int     nHeight;			 ///<项高度
	//@别名  项选中时高度
	int     nSelHeight;			 ///<项选中状态高度
	//@别名  用户数据
	vint    nUserData;			 ///<用户数据
	//@别名  展开
	BOOL    bExpand;			 ///<展开
	//@别名  状态
	tree_item_state_  nState;	 ///<状态
	//@别名  项坐标
	RECT    rcItem;              ///<坐标
	//@别名  布局元素
	HELE    hLayout;             ///<布局元素
	//@别名  项模板
	HTEMP   hTemp;               ///<列表项模板
};

///ListView 列表视项信息
//@别名  列表视图项结构
struct listView_item_
{
	//@别名  组索引
	int     iGroup;            ///<项所属组索引 -1没有组
	//@别名  项索引
	int     iItem;             ///<项在组中位置索引,如果此致为-1,那么为组
	//@别名  用户数据
	vint    nUserData;         ///<用户绑定数据
	//@别名  状态 
	list_item_state_  nState;  ///<状态  
	//@别名  项坐标
	RECT    rcItem;            ///<整个区域,包含边框
	//@别名  布局元素
	HELE    hLayout;           ///<布局元素
	//@别名  项模板 
	HTEMP   hTemp;             ///<列表项模板 
};

/// @defgroup group_menu_macro Menu菜单
/// @{

///菜单-弹出窗口信息

//@别名  菜单弹出窗口结构
struct  menu_popupWnd_
{
	//@别名  窗口句柄
	HWINDOW hWindow;    ///<窗口句柄
	//@别名  父项ID
	int     nParentID;  ///<父项ID
};

///菜单背景自绘结构
//@别名  菜单背景绘制结构
struct menu_drawBackground_
{
	//@别名  菜单句柄
	HMENUX  hMenu;      ///<菜单句柄
	//@别名 窗口句柄
	HWINDOW hWindow;    ///<当前菜单项的窗口句柄
	//@别名 父项ID
	int     nParentID;  ///<父项ID
};

///菜单项自绘结构
//@别名 菜单项绘制结构
struct  menu_drawItem_
{
	//@别名  菜单句柄
	HMENUX     hMenu;       ///<菜单句柄
	//@别名  窗口句柄
	HWINDOW    hWindow;     ///<当前菜单项的窗口句柄
	//@别名  项ID
	int        nID;         ///<ID
	//@别名  状态
	int        nState;	    ///<状态 @ref menu_item_flag_
	//@别名  右侧快捷键占位宽度
	int        nShortcutKeyWidth; ///<右侧快捷键占位宽度
	//@别名  项坐标
	RECT       rcItem;      ///<坐标
	//@别名  项图标
	HIMAGE     hIcon;       ///<菜单项图标
	//@别名  文本
	const wchar_t* pText;   ///<文本
};
///@}

///树UI元素拖动项
//@别名 列表树拖动项结构
struct tree_drag_item_
{
	//@别名  拖动项ID
	int  nDragItem;  ///< 拖动项ID
	//@别名  目标项ID
	int  nDestItem;  ///< 目标项ID
	//@别名  目标位置
	int  nType;      ///< 停放相对目标位置,0:(上)停放到目标的上面, 1:(下)停放到目标的下面, 3:(中)停放到目标的的子项, 
};

///字体信息
//@别名 字体信息结构
struct  font_info_
{
	//@别名  字体大小
	int   nSize;                 ///<字体大小,单位(pt,磅).
	//@别名  字体样式
	int   nStyle;                ///<字体样式 fontStyle_
	//@别名  字体名称
	wchar_t  name[LF_FACESIZE];  ///<字体名称
};

///PGrid 属性网格项信息
//@别名 属性网格项结构
struct propertyGrid_item_
{
	//@别名  类型
	propertyGrid_item_type_ nType; ///<类型
	//@别名  项ID
	int   nID;           ///<项ID
	//@别名  深度
	int   nDepth;        ///<深度
	//@别名  用户数据
	vint  nUserData;     ///<用户数据
	//@别名  名称列宽度
	int   nNameColWidth; ///<名称列宽度
	//@别名  项坐标
	RECT  rcItem;        ///<坐标
	//@别名  展开按钮坐标
	RECT  rcExpand;      ///<展开
	//@别名  是否展开
	BOOL  bExpand;       ///<是否展开
	//@别名  是否可见
	BOOL  bShow;         ///<是否可见
};

///Edit 样式信息
//@别名  编辑框样式结构
struct edit_style_info_
{
	//@别名  类型
	USHORT   type;              ///<样式类型
	//@别名  引用计数
	USHORT   nRef;              ///<引用计数
	//@别名  句柄
	HXCGUI   hFont_image_obj;   ///<句柄(字体,图片,UI对象)
	//@别名  颜色
	COLORREF color;             ///<颜色
	//@别名  是否使用颜色
	BOOL     bColor;            ///<是否使用颜色
};

///Edit 数据复制-样式
//@别名  编辑框数据复制样式结构
struct edit_data_copy_style_
{
	//@别名  句柄
	UINT     hFont_image_obj; ///<句柄(字体,图片,UI对象), 使用UINT目的当64位时可以节约4字节内存
	//@别名  颜色
	COLORREF color;           ///<颜色
	//@别名  是否使用颜色
	BOOL     bColor;          ///<是否使用颜色
};

///Edit 数据复制
//@别名  编辑框数据复制结构
struct edit_data_copy_
{
	//@别名  内容数量
	int  nCount;       ///<内容数量
	//@别名  样式数量
	int  nStyleCount;  ///<样式数量
	//@别名  样式数组
	edit_data_copy_style_* pStyle; ///<样式数组
	//@别名  数据
	UINT* pData;       ///<内容数组 高位2字节:样式索引, 低位2字节:值
};

///Editor 颜色信息
struct editor_color_
{
	BOOL       bAlignLineArrow;            ///<对齐线 - 是否显示箭头
	BOOL       bAlignLineBtnIndent;        ///<展开收缩按钮, 是否缩进
	BOOL       bTabFillColor;              ///<填充缩进TAB背景
	BOOL       bAlignLineColor7;           ///<是否使用彩虹线
	COLORREF   clrMargin1;                 ///<侧边栏 - 背景色1, 显示断点
	COLORREF   clrMargin2;                 ///<侧边栏 - 背景色2, 显示行号
	COLORREF   clrMargin_text;             ///<侧边栏 - 文本色 - 行号颜色
	COLORREF   clrMargin_breakpoint;       ///<侧边栏 - 断点色
	COLORREF   clrMargin_breakpointBorder; ///<侧边栏 - 断点描边色
	COLORREF   clrMargin_runRowArrow;      ///<侧边栏 - 调试位置箭头
	COLORREF   clrMargin_curRow;           ///<侧边栏 - 当前行指示色,光标所在行
	COLORREF   clrMargin_error;            ///<侧边栏 - 错误指示色

	COLORREF   clrCurRowFull;       ///<突出显示当前行颜色
	COLORREF   clrMatchSel;         ///<匹配选择文本背景色
	COLORREF   clrAlignLines[7];    ///<彩虹线

	COLORREF   clrAlignLineSel;     ///<对齐线 - 选择内容块
	COLORREF   clrFunSplitLine;     ///<函数分割线颜色 new

	COLORREF   clrIndentTab;        ///<缩进TAB
	COLORREF   clrIndentSpace;      ///<缩进遇到空格

	COLORREF   clrTipsDlg;          ///<弹出提示窗口背景色
	COLORREF   clrTipsDlgBorder;    ///<弹出提示窗口描边色
	COLORREF   clrCallFunArgName;   ///<调用函数显示参数名称的颜色

	int        funSplitLineMode;    ///<函数分割线-填充模式: 0:无, 1:线, 2:填充
	int        codeIndent;          ///<代码缩进模式(TAB)  自由缩进  固定缩进

	//选择文本背景 通过API设置
	//插入符颜色   通过API设置

	int       styleSys;             ///<系统关键字  return, break, for
	int       styleFunction;        ///<函数
	int       styleVar;             ///<变量
	int       styleDataType;        ///<基础数据类型  int, byte, char
	int       styleClass;           ///<类  class
	int       styleMacro;           ///<宏
	int       styleEnum;            ///<枚举   new
	int       styleNumber;          ///<数字
	int       styleString;          ///<字符串
	int       styleComment;         ///<注释
	int       StylePunctuation;     ///<标点符号  new
};

/// 月历元素项数据
struct monthCal_item_ //@别名  月历卡片项结构
{
	//@别名  日期
	int  nDay;     ///< 日期
	//@别名  类型
	int  nType;    ///< 1上月,2当月,3下月
	//@别名  状态
	int  nState;   ///< 组合状态 monthCal_state_flag_
	//@别名  项坐标
	RECT rcItem;   ///< 项坐标
};

///@}


//@隐藏{

///@defgroup  group_func_  炫彩回调函数定义
///@{

typedef void (CALLBACK* funDebugError)(const char* pInfo);       //错误回调
typedef BOOL(CALLBACK* funLoadFile)(const wchar_t* pFileName);  //图片资源文件加载回调
typedef void (CALLBACK* funCloudEvent)(const wchar_t* pFileName, int nEvent, HXCGUI hXCGUI);  //云UI事件回调
typedef vint(CALLBACK* funCallUiThread)(vint data);
typedef void (CALLBACK* funIdle)();
typedef void (CALLBACK* funExit)();

///@brief 动画回调
///@param hAnimation 动画序列或动画组句柄
///@param flag       当前忽略
typedef  void(CALLBACK* funAnimation)(HXCGUI hAnimation, int flag);

///@brief 动画项回调
///@param hAnimation 动画项句柄
///@param pos        当前进度(0.0f-1.0f)
typedef  void(CALLBACK* funAnimationItem)(HXCGUI hAnimation, float pos);

///@brief  XC_LoadLayout() 所需要的回调函数格式
typedef  HXCGUI (CALLBACK* funLoadLayout)(const wchar_t* pFileName, const wchar_t* pUIName, vint data, vint propertylist, HXCGUI hParent);

///@}

//@隐藏}

///////////////////////////////////////////////////////////////////////////////////
/////////////事件注册//////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//@分组{  事件注册
//@隐藏{

class bind_event_cpp
{
public:
	virtual BOOL  Equal(bind_event_cpp* p) = 0;
	virtual void* GetThis() = 0;
	virtual void* GetMFun(int& nSize) = 0;
	virtual int HandleEvent(HELE hEle, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled) = 0;
	virtual int HandleEventWnd(HWINDOW hWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled) = 0;
};

template<class R, class C, class F>
class bind_event_cpp_ : public bind_event_cpp
{
public:
	bind_event_cpp_(C* c, F f) :_c(c), _f(f) { }
	virtual BOOL Equal(bind_event_cpp* p) {
		if (_c != p->GetThis())
			return FALSE;
		int size = 0;
		void* f = p->GetMFun(size);
		if (sizeof(F) == size)
		{
			if (0 == memcmp(f, &_f, size))
				return TRUE;
		}
		return FALSE;
	}
	virtual void* GetThis() { return (void*)_c; }
	virtual void* GetMFun(int& nSize) { nSize = sizeof(F); return &_f; }

	template<class H, class A1>
	int CallT(int (C::* f)(A1), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)(pbHandled);
	}
	template<class H, class A1, class A2>
	int CallT(int (C::* f)(A1, A2), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)((A1)wParam, pbHandled);
	}
	template<class H, class A1, class A2, class A3>
	int CallT(int (C::* f)(A1, A2, A3), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)((A1)wParam, (A2)lParam, pbHandled);
	}
	template<class H, class A1, class A2, class A3, class A4>
	int CallT(int (C::* f)(A1, A2, A3, A4), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)((A1)nEvent, (A2)wParam, (A3)lParam, pbHandled);
	}
	template<class H, class A1, class A2, class A3, class A4, class A5>
	int CallT(int (C::* f)(A1, A2, A3, A4, A5), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)((A1)hEleOrWindow, (A2)nEvent, (A3)wParam, (A4)lParam, pbHandled);
	}
	virtual int HandleEvent(HELE hEle, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return CallT(_f, hEle, bHnadle, nEvent, wParam, lParam, pbHandled);
	}
	virtual int HandleEventWnd(HWINDOW hWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return CallT(_f, hWindow, bHnadle, nEvent, wParam, lParam, pbHandled);
	}
	C* _c;
	F   _f;
};

template<class R, class C, class F>
class bind_event_cpp2_ : public bind_event_cpp
{
public:
	bind_event_cpp2_(C* c, F f) :_c(c), _f(f) { }
	virtual BOOL Equal(bind_event_cpp* p) {
		if (_c != p->GetThis())
			return FALSE;
		int size = 0;
		void* f = p->GetMFun(size);
		if (sizeof(F) == size)
		{
			if (0 == memcmp(f, &_f, size))
				return TRUE;
		}
		return FALSE;
	}
	virtual void* GetThis() { return (void*)_c; }
	virtual void* GetMFun(int& nSize) { nSize = sizeof(F); return &_f; }

	template<class H, class A1, class A2>
	int CallT(int (C::* f)(A1, A2), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)((A1)hEleOrWindow, pbHandled);
	}
	template<class H, class A1, class A2, class A3>
	int CallT(int (C::* f)(A1, A2, A3), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)((A1)hEleOrWindow, (A2)wParam, pbHandled);
	}
	template<class H, class A1, class A2, class A3, class A4>
	int CallT(int (C::* f)(A1, A2, A3, A4), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)((A1)hEleOrWindow, (A2)wParam, (A3)lParam, pbHandled);
	}
	template<class H, class A1, class A2, class A3, class A4, class A5>
	int CallT(int (C::* f)(A1, A2, A3, A4, A5), H hEleOrWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return (_c->*f)((A1)hEleOrWindow, (A2)nEvent, (A3)wParam, (A4)lParam, pbHandled);
	}
	virtual int HandleEvent(HELE hEle, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return CallT(_f, hEle, bHnadle, nEvent, wParam, lParam, pbHandled);
	}
	virtual int HandleEventWnd(HWINDOW hWindow, BOOL bHnadle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		return CallT(_f, hWindow, bHnadle, nEvent, wParam, lParam, pbHandled);
	}
	C* _c;
	F  _f;
};

struct bind_event_c
{
	enum MyEnum
	{
		event_c_ex,
		event_lua_ex,
		event_js_ex,
	} type;
	virtual ~bind_event_c() {}
	virtual BOOL Equal(bind_event_c* p) { return FALSE; };
	virtual void* GetFunc() { return NULL; };
	virtual int HandleEvent(HELE hEle, BOOL bHandle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled) { return 0; };
	virtual int HandleEventWnd(HWINDOW hWindow, BOOL bHandle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled) { return 0; };
};

struct xc_event
{
	xc_event(int regType, const char* pFunName) {
		m_pBind = 0;
		m_pFunC = 0;
		m_name[0] = 0;
		m_regType = regType;
		if (pFunName) strcpy_s(m_name, pFunName);
	}
	int            m_regType;     //注册类型, 0:CPP,1:CPP1,2:CPP2
	bind_event_cpp* m_pBind;     //C++事件回调
	bind_event_c* m_pFunC;     //C事件回调
	char           m_name[MAX_PATH]; //函数名

	BOOL Equal(xc_event* pEvent) {
		if (m_pBind && pEvent->m_pBind)
			return (m_pBind->Equal(pEvent->m_pBind));
		if (m_pFunC && pEvent->m_pFunC)
			return (m_pFunC->Equal(pEvent->m_pFunC));
		return FALSE;
	}
	BOOL Equal(void* pFunc) {
		if (m_pFunC) {
			if (m_pFunC->GetFunc() == pFunc)
				return TRUE;
		}
		return FALSE;
	}
	int HandleEvent(HELE hEle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		if (m_pBind)
			return m_pBind->HandleEvent(hEle, m_regType, nEvent, wParam, lParam, pbHandled);
		if (m_pFunC)
			return m_pFunC->HandleEvent(hEle, m_regType, nEvent, wParam, lParam, pbHandled);
		MessageBoxA(NULL, "HandleEvent()", "error", 0);
		return 0;
	}
	int HandleEventWnd(HWINDOW hWindow, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL* pbHandled)
	{
		if (m_pBind)
			return m_pBind->HandleEventWnd(hWindow, m_regType, nEvent, wParam, lParam, pbHandled);
		if (m_pFunC)
			return m_pFunC->HandleEventWnd(hWindow, m_regType, nEvent, wParam, lParam, pbHandled);
		MessageBoxA(NULL, "HandleEventWnd()", "error", 0);
		return 0;
	}
	char* GetFunName() { return m_name; }
	virtual void Release() {
		if (m_pBind) delete m_pBind;
		if (m_pFunC) delete m_pFunC;
		delete this;
	}
};

XC_API BOOL WINAPI _XWnd_RegEvent(HWINDOW hWindow, UINT nEvent, xc_event* pEvent);
XC_API BOOL WINAPI _XWnd_RemoveEvent(HWINDOW hWindow, UINT nEvent, xc_event* pEvent);
XC_API BOOL WINAPI _XEle_RegEvent(HELE hEle, UINT nEvent, xc_event* pEvent);
XC_API BOOL WINAPI _XEle_RemoveEvent(HELE hEle, UINT nEvent, xc_event* pEvent);


static BOOL XEle_RegEventCPP_tt2(HXCGUI hXCGUI, UINT nEvent, int retType, BOOL bEle, xc_event* pEvent) {
	BOOL bResult = TRUE;
	if (-1 == retType) {
		if (bEle)
			bResult = _XEle_RemoveEvent((HELE)hXCGUI, nEvent, pEvent);
		else
			bResult = _XWnd_RemoveEvent((HWINDOW)hXCGUI, nEvent, pEvent);
		pEvent->Release();
	} else if (bEle) {
		if (FALSE == _XEle_RegEvent((HELE)hXCGUI, nEvent, pEvent))
		{
			pEvent->Release();
			bResult = FALSE;
		}
	} else if (FALSE == _XWnd_RegEvent((HWINDOW)hXCGUI, nEvent, pEvent)) {
		pEvent->Release();
		bResult = FALSE;
	}
	return bResult;
}

template<class R, class C, class F>
BOOL XEle_RegEventCPP_tt(HXCGUI hXCGUI, UINT nEvent, int retType, BOOL bEle, C* c, F f, const char* pFunName) {
	xc_event* pEvent = new xc_event(retType, pFunName);
	pEvent->m_pBind = new bind_event_cpp_<R, C, F>(c, f);
	return XEle_RegEventCPP_tt2(hXCGUI, nEvent, retType, bEle, pEvent);
}

template<class R, class C, class F>
BOOL XEle_RegEventCPP1_tt(HXCGUI hXCGUI, UINT nEvent, int retType, BOOL bEle, C* c, F f, const char* pFunName) {
	xc_event* pEvent = new xc_event(retType, pFunName);
	pEvent->m_pBind = new bind_event_cpp2_<R, C, F>(c, f);
	return XEle_RegEventCPP_tt2(hXCGUI, nEvent, retType, bEle, pEvent);
}

template<class R, class C, class CT, class A1>
BOOL XCGUI_RegEventCPP_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1), const char* pFunName) {
	return XEle_RegEventCPP_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);}

template<class R, class C, class CT, class A1, class A2>
BOOL XCGUI_RegEventCPP_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1, A2), const char* pFunName) {
	return XEle_RegEventCPP_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);}

template<class R, class C, class CT, class A1, class A2, class A3>
BOOL XCGUI_RegEventCPP_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1, A2, A3), const char* pFunName) {
	return XEle_RegEventCPP_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);}

template<class R, class C, class CT, class A1, class A2, class A3, class A4>
BOOL XCGUI_RegEventCPP_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1, A2, A3, A4), const char* pFunName) {
	return XEle_RegEventCPP_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);}

template<class R, class C, class CT, class A1, class A2, class A3, class A4, class A5>
BOOL XCGUI_RegEventCPP_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1, A2, A3, A4, A5), const char* pFunName) {
	return XEle_RegEventCPP_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);}

//---------------
template<class R, class C, class CT, class A1>
BOOL XCGUI_RegEventCPP1_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1), const char* pFunName) {
	return XEle_RegEventCPP1_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);
}

template<class R, class C, class CT, class A1, class A2>
BOOL XCGUI_RegEventCPP1_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1, A2), const char* pFunName) {
	return XEle_RegEventCPP1_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);
}

template<class R, class C, class CT, class A1, class A2, class A3>
BOOL XCGUI_RegEventCPP1_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1, A2, A3), const char* pFunName) {
	return XEle_RegEventCPP1_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);
}

template<class R, class C, class CT, class A1, class A2, class A3, class A4>
BOOL XCGUI_RegEventCPP1_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1, A2, A3, A4), const char* pFunName) {
	return XEle_RegEventCPP1_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);
}

template<class R, class C, class CT, class A1, class A2, class A3, class A4, class A5>
BOOL XCGUI_RegEventCPP1_t(HXCGUI hXCGUI, UINT nEvent, int regType, BOOL bEle, C* c, R(CT::* f)(A1, A2, A3, A4, A5), const char* pFunName) {
	return XEle_RegEventCPP1_tt<R>(hXCGUI, nEvent, regType, bEle, (CT*)c, f, pFunName);
}


#define XEle_RegEventCPP(hEle,Event,mFun)    XCGUI_RegEventCPP_t(hEle,Event,0,TRUE,this,mFun,#mFun)
#define XEle_RegEventCPP1(hEle,Event,mFun)   XCGUI_RegEventCPP1_t(hEle,Event,1,TRUE,this,mFun,#mFun)
#define XWnd_RegEventCPP(hWindow,Event,mFun)    XCGUI_RegEventCPP_t(hWindow,Event,0,FALSE,this,mFun,#mFun)
#define XWnd_RegEventCPP1(hWindow,Event,mFun)   XCGUI_RegEventCPP1_t(hWindow,Event,1,FALSE,this,mFun,#mFun)

//@隐藏}

//@备注 取消已注册的元素事件函数
//     例如: 元素_移除事件CPP(元素句柄, 元素事件_按钮点击, &我的主窗口::按钮点击响应函数)
//@别名 元素_移除事件CPP(元素句柄, 事件类型, 事件函数)
#define XEle_RemoveEventCPP(hEle,Event,mFun)  XCGUI_RegEventCPP_t(hEle,Event,-1,TRUE,this,mFun,#mFun)

//@备注 取消已注册的窗口事件函数
//     例如: XWnd_RemoveEventCPP(窗口句柄, 窗口事件_鼠标左键按下, &我的主窗口::窗口鼠标左键按下按下响应函数)
//@别名 窗口_移除事件CPP(窗口句柄, 事件类型, 事件函数)
#define XWnd_RemoveEventCPP(hWindow,Event,mFun) XCGUI_RegEventCPP_t(hWindow,Event,-1,FALSE,this,mFun,#mFun)


/*@声明

//@备注 注册元素事件, 将类成员函数作为事件响应函数, 函数省略元素自身句柄"元素句柄"; 事件格式见文档或对应事件说明
//  例如: 元素_注册事件CPP(元素句柄, 元素事件_按钮点击, &我的主窗口::按钮点击响应函数)
//@参数  元素句柄
//@参数  事件类型
//@参数  事件响应函数, 类的成员函数
//@别名  元素_注册事件CPP(元素句柄, 事件类型, 事件响应函数)
void XEle_RegEventCPP(HELE hEle, int event, void* callback);

//@备注 注册元素事件, 将类成员函数作为事件响应函数, 函数不省略参数"元素句柄"; 事件格式见文档或对应事件说明
//  例如: 元素_注册事件CPP1(元素句柄, 元素事件_按钮点击, &我的主窗口::按钮点击响应函数)
//@参数  元素句柄
//@参数  事件类型
//@参数  事件响应函数, 类的成员函数
//@别名  元素_注册事件CPP1(元素句柄, 事件类型, 事件响应函数)
void XEle_RegEventCPP1(HELE hEle, int event, void* callback);

//@备注 注册窗口事件,将类成员函数作为事件响应函数, 函数省略参数窗口自身句柄"窗口句柄"; 事件格式见文档或对应事件说明
//  例如: 窗口_注册事件CPP(窗口句柄, 窗口事件_鼠标左键按下, &我的主窗口::窗口鼠标左键按下按下响应函数)
//@参数  元素句柄
//@参数  事件类型
//@参数  事件响应函数, 类的成员函数
//@别名  窗口_注册事件CPP(窗口句柄, 事件类型, 事件响应函数)
void XWnd_RegEventCPP(HWINDOW hWindow, int event, void* callback);

//@备注 注册窗口事件,将类成员函数作为事件响应函数, 函数不省略参数"窗口句柄"; 事件格式见文档或对应事件说明
//  例如: 窗口_注册事件CPP1(窗口句柄, 窗口事件_鼠标左键按下, &我的主窗口::窗口鼠标左键按下按下响应函数)
//@参数  元素句柄
//@参数  事件类型
//@参数  事件响应函数, 类的成员函数
//@别名  窗口_注册事件CPP1(窗口句柄, 事件类型, 事件响应函数)
void XWnd_RegEventCPP1(HWINDOW hWindow, int event, void* callback);

*/
//@分组}

//@分组{  系统窗口样式
/*@声明

#define WS_OVERLAPPED       0x00000000L	  //@备注  窗口是重叠的窗口。 重叠的窗口带有标题栏和边框。 与 WS_TILED 样式相同。
#define WS_POPUP            0x80000000L	  //@备注  窗口是弹出窗口。 此样式不能与 WS_CHILD 样式一起使用。
#define WS_CHILD            0x40000000L	  //@备注  窗口是子窗口。 具有此样式的窗口不能有菜单栏。 此样式不能与 WS_POPUP 样式一起使用。
#define WS_MINIMIZE         0x20000000L	  //@备注  窗口最初最小化。 与 WS_ICONIC 样式相同。
#define WS_VISIBLE          0x10000000L	  //@备注  该窗口最初是可见的。可以使用 ShowWindow 或 SetWindowPos 函数打开和关闭此样式。
#define WS_DISABLED         0x08000000L	  //@备注  窗口最初处于禁用状态。 已禁用的窗口无法接收用户的输入。 若要在创建窗口后更改此设置，请使用 EnableWindow 函数。
#define WS_CLIPSIBLINGS     0x04000000L	  //@备注  相对于彼此剪裁子窗口;也就是说，当特定子窗口收到 WM_PAINT 消息时， WS_CLIPSIBLINGS 样式会将所有其他重叠的子窗口剪裁到子窗口的区域之外进行更新。 如果未指定 WS_CLIPSIBLINGS 且子窗口重叠，则当在子窗口的工作区内绘图时，可以在相邻子窗口的工作区内绘制。
#define WS_CLIPCHILDREN     0x02000000L	  //@备注  排除在父窗口内进行绘制时子窗口占用的区域。 创建父窗口时使用此样式。
#define WS_MAXIMIZE         0x01000000L	  //@备注  窗口最初是最大化的。
#define WS_CAPTION          0x00C00000L   //@备注  窗口具有标题栏, (包含 WS_BORDER 样式)
#define WS_BORDER           0x00800000L   //@备注  窗口具有细线边框
#define WS_DLGFRAME         0x00400000L	  //@备注  窗口具有通常与对话框一起使用的样式的边框。 具有此样式的窗口不能具有标题栏。
#define WS_VSCROLL          0x00200000L	  //@备注  窗口具有垂直滚动条。
#define WS_HSCROLL          0x00100000L	  //@备注  窗口具有水平滚动条。
#define WS_SYSMENU          0x00080000L	  //@备注  窗口的标题栏上有一个窗口菜单。 还必须指定 WS_CAPTION 样式。
#define WS_THICKFRAME       0x00040000L	  //@备注  窗口具有调整大小边框。 与 WS_SIZEBOX 样式相同。
#define WS_GROUP            0x00020000L	  //@备注  窗口是一组控件的第一个控件。 组由第一个控件及其后定义的所有控件组成，以及具有 WS_GROUP 样式的下一个控件。 每个组中的第一个控件通常具有 WS_TABSTOP 样式，以便用户可以在组之间移动。 用户随后可以使用方向键将键盘焦点从组中的一个控件更改为组中的下一个控件。
//可以打开和关闭此样式以更改对话框导航。 若要在创建窗口后更改此样式，请使用 SetWindowLong 函数。

#define WS_TABSTOP          0x00010000L	  //@备注  窗口是一个控件，当用户按下 TAB 键时，该控件可以接收键盘焦点。 按 Tab 键将键盘焦点更改为具有 WS_TABSTOP 样式的下一个控件。
//可以打开和关闭此样式以更改对话框导航。 若要在创建窗口后更改此样式，请使用 SetWindowLong 函数。 若要使用户创建的窗口和无模式对话框使用制表位，请更改消息循环以调用 IsDialogMessage 函数。

#define WS_MINIMIZEBOX      0x00020000L	  //@备注  窗口有一个最小化按钮。 不能与 WS_EX_CONTEXTHELP 样式组合使用。 还必须指定 WS_SYSMENU 样式。
#define WS_MAXIMIZEBOX      0x00010000L	  //@备注  窗口有一个“最大化”按钮。 不能与 WS_EX_CONTEXTHELP 样式组合使用。 还必须指定 WS_SYSMENU 样式。

#define WS_TILED            WS_OVERLAPPED        //@备注  该窗口是一个重叠的窗口。 重叠的窗口带有标题栏和边框。 与 WS_OVERLAPPED 样式相同。
#define WS_ICONIC           WS_MINIMIZE			 //@备注  窗口最初最小化。 与 WS_MINIMIZE 样式相同。
#define WS_SIZEBOX          WS_THICKFRAME		 //@备注  窗口具有大小调整边框。 与 WS_THICKFRAME 样式相同。
#define WS_TILEDWINDOW      WS_OVERLAPPEDWINDOW	 //@备注  该窗口是一个重叠的窗口。 与 WS_OVERLAPPEDWINDOW 样式相同。

*/
//@分组}


/*@声明
//@分组{ 回调函数格式

//@备注  动画回调
//@参数  hAnimation 动画序列或动画组句柄
//@参数  flag       当前忽略
//@别名  动画回调(动画序列或动画组句柄, 当前忽略)
typedef  void(CALLBACK* funAnimation)(HXCGUI hAnimation, int flag);

//@备注  动画项回调
//@参数  hAnimation 动画项句柄
//@参数  pos        当前进度(0.0f-1.0f)
//@别名  动画项回调(动画项句柄, 当前进度)
typedef  void(CALLBACK* funAnimationItem)(HXCGUI hAnimation, float pos);

//@备注  XC_CallUiThread() 所需要的回调函数格式
//@参数  data  用户数据
//@别名  UI线程回调(用户数据)
typedef vint(CALLBACK* funCallUiThread)(vint data);

//@备注  XC_LoadLayout() 所需要的回调函数格式
//@参数  pFileName    布局文件名
//@参数  pUIName      UI对象名称  
//@参数  pArrayName   属性名列表
//@参数  pArrayValue  属性值列表
//@参数  nArraySize   属性列表大小
//@参数  hParent      父对象句柄
//@别名  加载布局文件回调()
typedef  HXCGUI (CALLBACK* funLoadLayout)(const wchar_t* pFileName, const wchar_t* pUIName, const wchar_t** pArrayName, const wchar_t** pArrayValue, int nArraySize, HXCGUI hParent);

//@分组}
*/


//////////////////////////////////////////////////////////////////////
/////////////////////API//////////////////////////////////////////////
//@分组{  炫彩基类

//@备注 获取对象最终类型.  
//@参数 hXCGUI 对象句柄
//@返回 返回对象类型.
//@别名 炫彩对象_取类型()
XC_API XC_OBJECT_TYPE WINAPI XObj_GetType(HXCGUI hXCGUI);
//@备注 获取对象的基础类型,  
//XC_ERROR, XC_WINDOW, XC_ELE, XC_SHAPE, XC_ADAPTER
//@参数 hXCGUI 对象句柄
//@返回 返回对象类型, 以下类型之一:
//@别名 炫彩对象_取基础类型()
XC_API XC_OBJECT_TYPE WINAPI XObj_GetTypeBase(HXCGUI hXCGUI);
//@备注 获取对象扩展类型.  
//@参数 hXCGUI 对象句柄.
//@返回 返回对象扩展类型.
//@别名 炫彩对象_取类型扩展()
XC_API XC_OBJECT_TYPE_EX WINAPI XObj_GetTypeEx(HXCGUI hXCGUI);
//@备注 如果是按钮, 请使用按钮的增强接口 XBtn_SetTypeEx()
//@参数 hXCGUI 对象句柄.
//@参数 nType 扩展类型,参见宏定义.
//@别名 炫彩对象_置类型扩展()
XC_API void WINAPI XObj_SetTypeEx(HXCGUI hXCGUI, XC_OBJECT_TYPE_EX nType);
//@分组}
//@分组{  可视对象

//@备注 设置UI对象样式
//@参数 hXCGUI 对象句柄.
//@参数 nStyle 样式值.
//@别名 可视对象_置样式()
XC_API void WINAPI XUI_SetStyle(HXCGUI hXCGUI, XC_OBJECT_STYLE nStyle);
//@备注 获取UI对象样式
//@参数 hXCGUI 对象句柄.
//@返回 返回UI对象样式.
//@别名 可视对象_取样式()
XC_API XC_OBJECT_STYLE WINAPI XUI_GetStyle(HXCGUI hXCGUI);
//@备注 启用或禁用样式, 并且覆盖内嵌子元素属性,  例如:滚动视图上的滚动条, 滚动条上的按钮
//@参数 hXCGUI 对象句柄.
//@参数 bEnable 是否启用.
//@别名 可视对象_启用CSS()
XC_API void WINAPI XUI_EnableCSS(HXCGUI hXCGUI, BOOL bEnable);
//@备注 启用或禁用样式, 仅设置自身属性, 不包含内嵌子元素属性,  例如:滚动视图上的滚动条, 滚动条上的按钮
//@参数 hXCGUI 对象句柄.
//@参数 bEnable 是否启用.
//@别名 可视对象_启用CSS扩展()
XC_API void WINAPI XUI_EnableCssEx(HXCGUI hXCGUI, BOOL bEnable);
//@备注 设置CSS[套用样式]名称
//@参数 hXCGUI 对象句柄.
//@参数 pName 套用样式名称.
//@别名 可视对象_置CSS名称()
XC_API void WINAPI XUI_SetCssName(HXCGUI hXCGUI, const wchar_t* pName);
//@备注 获取CSS样式名称
//@参数 hXCGUI 对象句柄.
//@返回 返回CSS样式名称.
//@别名 可视对象_取CSS名称()
XC_API const wchar_t* WINAPI XUI_GetCssName(HXCGUI hXCGUI);
//@分组}
//@分组{  窗口组件

//@备注 判断UI对象是否显示
//@参数 hXCGUI 对象句柄.
//@返回 显示返回TRUE,否则返回FALSE.
//@别名 窗口组件_是否显示()
XC_API BOOL WINAPI XWidget_IsShow(HXCGUI hXCGUI);
//@参数 hXCGUI 对象句柄
//@参数 bShow 是否显示
//@别名 窗口组件_显示()
XC_API void WINAPI XWidget_Show(HXCGUI hXCGUI, BOOL bShow);
//@备注 该对象是否受布局控制
//@参数 hXCGUI 对象句柄
//@参数 bEnable 是否启用
//@别名 窗口组件_启用布局控制()
XC_API void WINAPI XWidget_EnableLayoutControl(HXCGUI hXCGUI, BOOL bEnable);
//@参数 hXCGUI 
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 窗口组件_是否布局控制()
XC_API BOOL WINAPI XWidget_IsLayoutControl(HXCGUI hXCGUI);
//@参数 hXCGUI 对象句柄.
//@返回 元素句柄.
//@别名 窗口组件_取父元素()
XC_API HELE WINAPI XWidget_GetParentEle(HXCGUI hXCGUI);
//@备注 获取父对象,父可能是元素或窗口,通过此函数可以检查是否有父
//@参数 hXCGUI 对象句柄.
//@返回 对象句柄.
//@别名 窗口组件_取父句柄()
XC_API HXCGUI WINAPI XWidget_GetParent(HXCGUI hXCGUI);
//@备注 获取HWND句柄
//@参数 hXCGUI 对象句柄.
//@返回 HWND句柄.
//@别名 窗口组件_取窗口句柄系统()
XC_API HWND WINAPI XWidget_GetHWND(HXCGUI hXCGUI);
//@备注 获取HWINDOW句柄
//@参数 hXCGUI 对象句柄.
//@返回 HWINDOW句柄.
//@别名 窗口组件_取窗口句柄()
XC_API HWINDOW WINAPI XWidget_GetHWINDOW(HXCGUI hXCGUI);
//@备注 设置元素ID
//@参数 hXCGUI 对象句柄
//@参数 nID ID值.
//@别名 窗口组件_置ID()
XC_API void WINAPI XWidget_SetID(HXCGUI hXCGUI, int nID);
//@备注 获取元素ID
//@参数 hXCGUI 对象句柄
//@返回 返回元素ID.
//@别名 窗口组件_取ID()
XC_API int WINAPI XWidget_GetID(HXCGUI hXCGUI);
//@备注 设置元素UID, 全局唯一标识符
//@参数 hXCGUI 对象句柄
//@参数 nUID UID值.
//@别名 窗口组件_置UID()
XC_API void WINAPI XWidget_SetUID(HXCGUI hXCGUI, int nUID);
//@备注 获取元素UID,全局唯一标识符
//@参数 hXCGUI 对象句柄
//@返回 返回元素UID.
//@别名 窗口组件_取UID()
XC_API int WINAPI XWidget_GetUID(HXCGUI hXCGUI);
//@备注 设置元素name
//@参数 hXCGUI 对象句柄
//@参数 pName name值,字符串指针.
//@别名 窗口组件_置名称()
XC_API void WINAPI XWidget_SetName(HXCGUI hXCGUI, const wchar_t* pName);
//@备注 获取元素name
//@参数 hXCGUI 对象句柄
//@返回 返回name.
//@别名 窗口组件_取名称()
XC_API const wchar_t* WINAPI XWidget_GetName(HXCGUI hXCGUI);
//@备注 强制换行
//@参数 hXCGUI UI对象句柄
//@参数 bWrap 是否换行
//@别名 窗口组件_布局项_启用换行()
XC_API void WINAPI XWidget_LayoutItem_EnableWrap(HXCGUI hXCGUI, BOOL bWrap);
//@备注 根据水平垂直布局变换, 交换属性(宽度,高度,最小宽度,最小高度)
//@参数 hXCGUI UI对象句柄
//@参数 bEnable 是否启用
//@别名 窗口组件_布局项_启用交换()
XC_API void WINAPI XWidget_LayoutItem_EnableSwap(HXCGUI hXCGUI, BOOL bEnable);
//@备注 向反方向对齐
//@参数 hXCGUI UI对象句柄
//@参数 bFloat 是否浮动
//@别名 窗口组件_布局项_启用浮动()
XC_API void WINAPI XWidget_LayoutItem_EnableFloat(HXCGUI hXCGUI, BOOL bFloat);
//@参数 hXCGUI UI对象句柄
//@参数 nType 类型
//@参数 nWidth 宽度
//@别名 窗口组件_布局项_置宽度()
XC_API void WINAPI XWidget_LayoutItem_SetWidth(HXCGUI hXCGUI, layout_size_ nType, int nWidth);
//@参数 hXCGUI UI对象句柄
//@参数 nType 类型
//@参数 nHeight 高度
//@别名 窗口组件_布局项_置高度()
XC_API void WINAPI XWidget_LayoutItem_SetHeight(HXCGUI hXCGUI, layout_size_ nType, int nHeight);
//@参数 hXCGUI UI对象句柄
//@参数 pType 类型
//@参数 pWidth 返回值
//@别名 窗口组件_布局项_取宽度()
XC_API void WINAPI XWidget_LayoutItem_GetWidth(HXCGUI hXCGUI, layout_size_* pType, int* pWidth);
//@参数 hXCGUI UI对象句柄
//@参数 pType 类型
//@参数 pHeight 返回值
//@别名 窗口组件_布局项_取高度()
XC_API void WINAPI XWidget_LayoutItem_GetHeight(HXCGUI hXCGUI, layout_size_* pType, int* pHeight);
//@备注 根据水平垂直轴变化对齐
//@参数 hXCGUI UI对象句柄
//@参数 nAlign 对齐方式
//@别名 窗口组件_布局项_置对齐()
XC_API void WINAPI XWidget_LayoutItem_SetAlign(HXCGUI hXCGUI, layout_align_axis_ nAlign);
//@参数 hXCGUI UI对象句柄
//@参数 left 左边大小
//@参数 top 上边大小
//@参数 right 右边大小
//@参数 bottom 下边大小
//@别名 窗口组件_布局项_置外间距()
XC_API void WINAPI XWidget_LayoutItem_SetMargin(HXCGUI hXCGUI, int left, int top, int right, int bottom);
//@参数 hXCGUI UI对象句柄
//@参数 pMargin 接收返回
//@别名 窗口组件_布局项_取外间距()
XC_API void WINAPI XWidget_LayoutItem_GetMargin(HXCGUI hXCGUI, marginSize_* pMargin);
//@备注 限制大小仅针对缩放有效(自动, 填充父, 比例, 百分比)
//@参数 hXCGUI UI对象句柄
//@参数 width 最小宽度
//@参数 height 最小高度
//@别名 窗口组件_布局项_置最小大小()
XC_API void WINAPI XWidget_LayoutItem_SetMinSize(HXCGUI hXCGUI, int width, int height);
//@备注 相对位置, 值大于等于0有效
//@参数 hXCGUI UI对象句柄
//@参数 left 左边距离
//@参数 top 上边距离
//@参数 right 右边距离
//@参数 bottom 下边距离
//@别名 窗口组件_布局项_置位置()
XC_API void WINAPI XWidget_LayoutItem_SetPosition(HXCGUI hXCGUI, int left, int top, int right, int bottom);
//@分组}
//@分组{  窗口

//@备注 创建窗口  
//@参数 x 窗口左上角x坐标.
//@参数 y 窗口左上角y坐标.
//@参数 cx 窗口宽度.
//@参数 cy 窗口高度.
//@参数 pTitle 窗口标题.
//@参数 hWndParent 父窗口.
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_.
//@返回 GUI库窗口资源句柄.
//@别名 窗口_创建()
XC_API HWINDOW WINAPI XWnd_Create(int x, int y, int cx, int cy, const wchar_t* pTitle, HWND hWndParent=NULL, int XCStyle=window_style_default);
//@备注 创建窗口,增强功能.   
//@参数 dwExStyle 窗口扩展样式.
//@参数 dwStyle 窗口样式
//@参数 lpClassName 窗口类名.
//@参数 x 窗口左上角x坐标.
//@参数 y 窗口左上角y坐标.
//@参数 cx 窗口宽度.
//@参数 cy 窗口高度.
//@参数 pTitle 窗口名.
//@参数 hWndParent 父窗口.
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_.
//@返回 GUI库窗口资源句柄.
//@别名 窗口_创建扩展()
XC_API HWINDOW WINAPI XWnd_CreateEx(DWORD dwExStyle, DWORD dwStyle, const wchar_t* lpClassName, int x, int y, int cx, int cy, const wchar_t* pTitle, HWND hWndParent=NULL, int XCStyle=window_style_default);
//@参数 hWnd 要附加的外部窗口句柄
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_
//@返回 返回窗口句柄
//@别名 窗口_附加窗口()
XC_API HWINDOW WINAPI XWnd_Attach(HWND hWnd, int XCStyle);
//@备注 注册事件函数C方式,事件函数省略自身HWINDOW句柄. 
//@参数 hWindow 窗口句柄.
//@参数 nEvent 事件类型.
//@参数 pFun 事件函数.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 窗口_注册事件C()
XC_API BOOL WINAPI XWnd_RegEventC(HWINDOW hWindow, int nEvent, void* pFun);
//@备注 注册事件函数C方式,事件函数不省略自身HWINDOW句柄.   
//@参数 hWindow 窗口句柄.
//@参数 nEvent 事件类型.
//@参数 pFun 事件函数.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 窗口_注册事件C1()
XC_API BOOL WINAPI XWnd_RegEventC1(HWINDOW hWindow, int nEvent, void* pFun);
//@备注 移除事件函数.   
//@参数 hWindow 窗口句柄.
//@参数 nEvent 事件类型.
//@参数 pFun 事件函数.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 窗口_移除事件C()
XC_API BOOL WINAPI XWnd_RemoveEventC(HWINDOW hWindow, int nEvent, void* pFun);
//@备注 添加子对象到窗口   
//@参数 hWindow 窗口句柄
//@参数 hChild 要添加的对象句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 窗口_添加子对象()
XC_API BOOL WINAPI XWnd_AddChild(HWINDOW hWindow, HXCGUI hChild);
//@备注 插入子对象到指定位置.  
//@参数 hWindow 窗口句柄.
//@参数 hChild 要插入的对象句柄.
//@参数 index 插入位置索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 窗口_插入子对象()
XC_API BOOL WINAPI XWnd_InsertChild(HWINDOW hWindow, HXCGUI hChild, int index);
//@备注 重绘窗口. 
//@参数 hWindow 窗口资源句柄.
//@参数 bUpdate 是否立即重绘,默认为否.
//@别名 窗口_重绘()
XC_API void WINAPI XWnd_Redraw(HWINDOW hWindow, BOOL bUpdate=FALSE);
//@备注 重绘窗口指定区域. 
//@参数 hWindow 窗口资源句柄.
//@参数 pRect 需要重绘的区域坐标.
//@参数 bUpdate TRUE立即重绘,FALSE放入消息队列延迟重绘.
//@别名 窗口_重绘指定区域()
XC_API void WINAPI XWnd_RedrawRect(HWINDOW hWindow, RECT* pRect, BOOL bUpdate=FALSE);
//@备注 设置焦点元素. 
//@参数 hWindow 窗口资源句柄.
//@参数 hFocusEle 将获得焦点的元素.
//@别名 窗口_置焦点()
XC_API void WINAPI XWnd_SetFocusEle(HWINDOW hWindow, HELE hFocusEle);
//@备注 获得拥有输入焦点的元素.  
//@参数 hWindow 窗口资源句柄.
//@返回 元素句柄.
//@别名 窗口_取焦点()
XC_API HELE WINAPI XWnd_GetFocusEle(HWINDOW hWindow);
//@备注 获取当前鼠标所停留元素.  
//@参数 hWindow 窗口资源句柄.
//@返回 返回鼠标停留元素句柄.
//@别名 窗口_取鼠标停留元素()
XC_API HELE WINAPI XWnd_GetStayEle(HWINDOW hWindow);
//@备注 在自绘事件函数中,用户手动调用绘制窗口,以便控制绘制顺序.  
//@参数 hWindow 窗口资源句柄.
//@参数 hDraw 图形绘制句柄.
//@别名 窗口_绘制()
XC_API void WINAPI XWnd_DrawWindow(HWINDOW hWindow, HDRAW hDraw);
//@备注 居中窗口. 
//@参数 hWindow 窗口资源句柄.
//@别名 窗口_居中()
XC_API void WINAPI XWnd_Center(HWINDOW hWindow);
//@备注 居中窗口. 
//@参数 hWindow 窗口资源句柄.
//@参数 width 窗口宽度
//@参数 height 窗口高度
//@别名 窗口_居中扩展()
XC_API void WINAPI XWnd_CenterEx(HWINDOW hWindow, int width, int height);
//@备注 设置窗口鼠标光标. 
//@参数 hWindow 窗口句柄.
//@参数 hCursor 鼠标光标句柄.
//@别名 窗口_置光标()
XC_API void WINAPI XWnd_SetCursor(HWINDOW hWindow, HCURSOR hCursor);
//@备注 获取窗口的鼠标光标. 
//@参数 hWindow 窗口句柄.
//@返回 鼠标光标句柄.
//@别名 窗口_取光标()
XC_API HCURSOR WINAPI XWnd_GetCursor(HWINDOW hWindow);
//@备注 获取HWND句柄. 
//@参数 hWindow 窗口句柄.
//@返回 HWND句柄.
//@别名 窗口_取HWND()
XC_API HWND WINAPI XWnd_GetHWND(HWINDOW hWindow);
//@备注 启用拖动窗口边框. 
//@参数 hWindow 窗口句柄.
//@参数 bEnable 是否启用.
//@别名 窗口_启用拖动边框()
XC_API void WINAPI XWnd_EnableDragBorder(HWINDOW hWindow, BOOL bEnable);
//@备注 启用拖动窗口. 
//@参数 hWindow 窗口句柄.
//@参数 bEnable 是否启用.
//@别名 窗口_启用拖动窗口()
XC_API void WINAPI XWnd_EnableDragWindow(HWINDOW hWindow, BOOL bEnable);
//@备注 启用拖动窗口标题栏. 
//@参数 hWindow 窗口句柄.
//@参数 bEnable 是否启用.
//@别名 窗口_启用拖动标题栏()
XC_API void WINAPI XWnd_EnableDragCaption(HWINDOW hWindow, BOOL bEnable);
//@备注 是否绘制窗口背景.  
//@参数 hWindow 窗口句柄.
//@参数 bEnable 是否启用.
//@别名 窗口_启用绘制背景()
XC_API void WINAPI XWnd_EnableDrawBk(HWINDOW hWindow, BOOL bEnable);
//@备注 当鼠标左键按下是否获得焦点. 
//@参数 hWindow 窗口句柄.
//@参数 bEnable 是否启用.
//@别名 窗口_启用自动焦点()
XC_API void WINAPI XWnd_EnableAutoFocus(HWINDOW hWindow, BOOL bEnable);
//@备注 允许窗口最大化.  
//@参数 hWindow 窗口句柄.
//@参数 bEnable 是否启用.
//@别名 窗口_启用允许最大化()
XC_API void WINAPI XWnd_EnableMaxWindow(HWINDOW hWindow, BOOL bEnable);
//@备注 限制窗口最小和最大尺寸
//@参数 hWindow 窗口句柄
//@参数 bEnable 是否启用
//@别名 窗口_启用限制窗口大小()
XC_API void WINAPI XWnd_EnableLimitWindowSize(HWINDOW hWindow, BOOL bEnable);
//@参数 hWindow 窗口句柄
//@参数 bEnable 是否启用
//@别名 窗口_启用拖放文件()
XC_API void WINAPI XWnd_EnableDragFiles(HWINDOW hWindow, BOOL bEnable);
//@备注 启用布局功能.   
//@参数 hWindow 窗口句柄.
//@参数 bEnable 是否启用.
//@别名 窗口_启用布局()
XC_API void WINAPI XWnd_EnableLayout(HWINDOW hWindow, BOOL bEnable);
//@参数 hWindow 窗口句柄
//@参数 bEnable 是否启用
//@别名 窗口_启用布局覆盖边框()
XC_API void WINAPI XWnd_EnableLayoutOverlayBorder(HWINDOW hWindow, BOOL bEnable);
//@备注 显示布局边界.  
//@参数 hWindow 窗口句柄.
//@参数 bEnable 是否启用.
//@别名 窗口_显示布局边界()
XC_API void WINAPI XWnd_ShowLayoutFrame(HWINDOW hWindow, BOOL bEnable);
//@备注 判断是否启用布局. 
//@参数 hWindow 窗口句柄
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 窗口_是否启用布局()
XC_API BOOL WINAPI XWnd_IsEnableLayout(HWINDOW hWindow);
//@参数 hWindow 窗口句柄
//@返回 如果窗口最大化返回TRUE,否则返回FALSE
//@别名 窗口_是否最大化()
XC_API BOOL WINAPI XWnd_IsMaxWindow(HWINDOW hWindow);
//@参数 hWindow 窗口句柄
//@返回 窗口边框可拖动返回TRUE,否则返回FALSE
//@别名 窗口_是否可拖动边框()
XC_API BOOL WINAPI XWnd_IsDragBorder(HWINDOW hWindow);
//@参数 hWindow 窗口句柄
//@返回 窗口内容区可拖动返回TRUE,否则返回FALSE
//@别名 窗口_是否可拖动窗口()
XC_API BOOL WINAPI XWnd_IsDragWindow(HWINDOW hWindow);
//@参数 hWindow 窗口句柄
//@返回 窗口标题可拖动返回TRUE,否则返回FALSE
//@别名 窗口_是否可拖动标题栏()
XC_API BOOL WINAPI XWnd_IsDragCaption(HWINDOW hWindow);
//@备注 设置鼠标捕获元素. 
//@参数 hWindow 窗口句柄.
//@参数 hEle 元素句柄.
//@别名 窗口_置鼠标捕获元素()
XC_API void WINAPI XWnd_SetCaptureEle(HWINDOW hWindow, HELE hEle);
//@备注 获取当前鼠标捕获元素. 
//@参数 hWindow 窗口句柄.
//@返回 元素句柄.
//@别名 窗口_取鼠标捕获元素()
XC_API HELE WINAPI XWnd_GetCaptureEle(HWINDOW hWindow);
//@备注 获取重绘区域. 
//@参数 hWindow 窗口句柄.
//@参数 pRcPaint 重绘区域坐标.
//@别名 窗口_取绘制矩形()
XC_API void WINAPI XWnd_GetDrawRect(HWINDOW hWindow, RECT* pRcPaint);
//@备注 显示隐藏窗口
//@参数 hWindow 窗口句柄.
//@参数 bShow 是否显示
//@别名 窗口_显示()
XC_API void WINAPI XWnd_Show(HWINDOW hWindow, BOOL bShow);
//@备注 显示隐藏及控制窗口最大化还原等
//@参数 hWindow 窗口句柄.
//@参数 nCmdShow 参见MSDN.
//@返回 参见MSDN.
//@别名 窗口_显示扩展()
XC_API BOOL WINAPI XWnd_ShowWindow(HWINDOW hWindow, int nCmdShow);
//@备注 系统函数,设置窗口类光标句柄. 
//@参数 hWindow 窗口句柄.
//@参数 hCursor 光标句柄.
//@返回 返回先前的光标句柄.
//@别名 窗口_置系统光标()
XC_API HCURSOR WINAPI XWnd_SetCursorSys(HWINDOW hWindow, HCURSOR hCursor);
//@备注 设置窗口字体.  
//@参数 hWindow 窗口句柄.
//@参数 hFontx 炫彩字体句柄.
//@别名 窗口_置字体()
XC_API void WINAPI XWnd_SetFont(HWINDOW hWindow, HFONTX hFontx);
//@备注 设置文本颜色. 
//@参数 hWindow 窗口句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 窗口_置文本颜色()
XC_API void WINAPI XWnd_SetTextColor(HWINDOW hWindow, COLORREF color);
//@备注 获取文本颜色. 
//@参数 hWindow 窗口句柄.
//@返回 文本颜色值.
//@别名 窗口_取文件颜色()
XC_API COLORREF WINAPI XWnd_GetTextColor(HWINDOW hWindow);
//@备注 获取文本颜色,优先从资源中获取.  
//@参数 hWindow 窗口句柄.
//@返回 文本颜色值.
//@别名 窗口_取文本颜色扩展()
XC_API COLORREF WINAPI XWnd_GetTextColorEx(HWINDOW hWindow);
//@备注 设置窗口ID. 
//@参数 hWindow 窗口句柄.
//@参数 nID ID值.
//@别名 窗口_置ID()
XC_API void WINAPI XWnd_SetID(HWINDOW hWindow, int nID);
//@备注 获取窗口ID. 
//@参数 hWindow 窗口句柄.
//@返回 返回窗口ID值.
//@别名 窗口_取ID()
XC_API int WINAPI XWnd_GetID(HWINDOW hWindow);
//@备注 设置窗口name 
//@参数 hWindow 窗口句柄
//@参数 pName name值,字符串指针.
//@别名 窗口_置名称()
XC_API void WINAPI XWnd_SetName(HWINDOW hWindow, const wchar_t* pName);
//@备注 获取窗口name 
//@参数 hWindow 窗口句柄
//@返回 返回name
//@别名 窗口_取名称()
XC_API const wchar_t* WINAPI XWnd_GetName(HWINDOW hWindow);
//@备注 设置边大小. 
//@参数 hWindow 窗口句柄.
//@参数 left 窗口左边大小.
//@参数 top 窗口上边大小.
//@参数 right 窗口右边大小.
//@参数 bottom 窗口底部大小.
//@别名 窗口_置边大小()
XC_API void WINAPI XWnd_SetBorderSize(HWINDOW hWindow, int left, int top, int right, int bottom);
//@备注 获取边大小. 
//@参数 hWindow 窗口句柄.
//@参数 pBorder 接收返回边大小.
//@别名 窗口_取边大小()
XC_API void WINAPI XWnd_GetBorderSize(HWINDOW hWindow, borderSize_* pBorder);
//@参数 hWindow 窗口句柄.
//@参数 left 左边大小.
//@参数 top 上边大小.
//@参数 right 右边大小.
//@参数 bottom 下边大小.
//@别名 窗口_置内填充大小()
XC_API void WINAPI XWnd_SetPadding(HWINDOW hWindow, int left, int top, int right, int bottom);
//@参数 hWindow 窗口句柄.
//@参数 pPadding 接收返回内填充大小.
//@别名 窗口_取内填充大小()
XC_API void WINAPI XWnd_GetPadding(HWINDOW hWindow, paddingSize_* pPadding);
//@备注 设置窗口拖动边框大小.  
//@参数 hWindow 窗口句柄.
//@参数 left 窗口左边大小.
//@参数 top 窗口上边大小.
//@参数 right 窗口右边大小.
//@参数 bottom 窗口底边大小.
//@别名 窗口_置拖动边框大小()
XC_API void WINAPI XWnd_SetDragBorderSize(HWINDOW hWindow, int left, int top, int right, int bottom);
//@备注 获取窗口拖动边框大小.  
//@参数 hWindow 窗口句柄.
//@参数 pSize 拖动边框大小.
//@别名 窗口_取拖动边框大小()
XC_API void WINAPI XWnd_GetDragBorderSize(HWINDOW hWindow, borderSize_* pSize);
//@备注 设置标题内容(图标,标题,控制按钮)外间距
//@参数 hWindow 窗口句柄
//@参数 left 左边间距
//@参数 top 上边间距
//@参数 right 右边间距
//@参数 bottom 下边间距
//@别名 窗口_置标题外间距()
XC_API void WINAPI XWnd_SetCaptionMargin(HWINDOW hWindow, int left, int top, int right, int bottom);
//@备注 设置窗口的最小宽度和高度.  
//@参数 hWindow 窗口句柄.
//@参数 width 最小宽度.
//@参数 height 最小高度.
//@别名 窗口_置最小大小()
XC_API void WINAPI XWnd_SetMinimumSize(HWINDOW hWindow, int width, int height);
//@备注 检测所在元素.  
//@参数 hWindow 窗口句柄.
//@参数 pPt 左边点.
//@返回 元素句柄.
//@别名 窗口_测试点击子元素()
XC_API HELE WINAPI XWnd_HitChildEle(HWINDOW hWindow, POINT* pPt);
//@备注 获取当前层子对象数量,不包含子元素的子元素.  
//@参数 hWindow 窗口句柄.
//@返回 子元素数量.
//@别名 窗口_取子对象数量()
XC_API int WINAPI XWnd_GetChildCount(HWINDOW hWindow);
//@备注 获取当前层子元素通过索引.  
//@参数 hWindow 窗口句柄.
//@参数 index 元素索引.
//@返回 元素句柄.
//@别名 窗口_取子对象从索引()
XC_API HXCGUI WINAPI XWnd_GetChildByIndex(HWINDOW hWindow, int index);
//@备注 获取当前层子对象通过对象ID.  
//@参数 hWindow 窗口句柄.
//@参数 nID 元素ID,ID必须大于0.
//@返回 对象句柄.
//@别名 窗口_取子对象从ID()
XC_API HXCGUI WINAPI XWnd_GetChildByID(HWINDOW hWindow, int nID);
//@备注 获取子对象通过对象ID,如果对象不在该窗口上无效.  
//@参数 hWindow 窗口句柄.
//@参数 nID 对象ID,ID必须大于0.
//@返回 元素句柄.
//@别名 窗口_取子对象()
XC_API HXCGUI WINAPI XWnd_GetChild(HWINDOW hWindow, int nID);
//@备注 设置当前窗口DPI, 默认DPI为96
//@参数 hWindow 窗口句柄
//@参数 nDPI DPI值
//@别名 窗口_置DPI()
XC_API void WINAPI XWnd_SetDPI(HWINDOW hWindow, int nDPI);
//@备注 获取当前窗口所在显示器DPI
//@参数 hWindow 窗口句柄
//@返回 返回窗口DPI
//@别名 窗口_取DPI()
XC_API int WINAPI XWnd_GetDPI(HWINDOW hWindow);
//@备注 炫彩窗口上的图标, 非系统图标
//@参数 hWindow 窗口句柄
//@参数 hImage 图标句柄
//@别名 窗口_置图标()
XC_API void WINAPI XWnd_SetIcon(HWINDOW hWindow, HIMAGE hImage);
//@备注 炫彩窗口标题栏文本, 非系统窗口标题
//@参数 hWindow 窗口句柄
//@参数 pTitle 标题文本
//@别名 窗口_置标题()
XC_API void WINAPI XWnd_SetTitle(HWINDOW hWindow, const wchar_t* pTitle);
//@参数 hWindow 窗口句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 窗口_置标题颜色()
XC_API void WINAPI XWnd_SetTitleColor(HWINDOW hWindow, COLORREF color);
//@参数 hWindow 窗口句柄
//@参数 nFlag 可用值: @ref window_style_btn_min , @ref window_style_btn_max , @ref window_style_btn_close
//@返回 返回按钮句柄
//@别名 窗口_取控制按钮()
XC_API HELE WINAPI XWnd_GetButton(HWINDOW hWindow, int nFlag);
//@参数 hWindow 窗口句柄
//@返回 返回图标句柄
//@别名 窗口_取图标()
XC_API HIMAGE WINAPI XWnd_GetIcon(HWINDOW hWindow);
//@参数 hWindow 窗口句柄
//@返回 返回标题文本
//@别名 窗口_取标题()
XC_API const wchar_t* WINAPI XWnd_GetTitle(HWINDOW hWindow);
//@参数 hWindow 窗口句柄
//@返回 返回颜色值
//@别名 窗口_取标题颜色()
XC_API COLORREF WINAPI XWnd_GetTitleColor(HWINDOW hWindow);
//@备注 关闭窗口. 发送WM_CLOSE消息关闭窗口, 若拦截WM_CLOSE消息可阻止关闭窗口  
//@参数 hWindow 窗口句柄.
//@别名 窗口_关闭()
XC_API void WINAPI XWnd_CloseWindow(HWINDOW hWindow);
//@备注 立即销毁窗口, 当函数返回时窗口已销毁
//@参数 hWindow 窗口句柄
//@别名 窗口_销毁()
XC_API void WINAPI XWnd_DestroyWindow(HWINDOW hWindow);
//@备注 调整窗口布局, 当窗口显示状态才有效.  
//@参数 hWindow 窗口句柄.
//@别名 窗口_调整布局()
XC_API void WINAPI XWnd_AdjustLayout(HWINDOW hWindow);
//@备注 调整窗口布局, 当窗口显示状态才有效.   
//@参数 hWindow 窗口句柄.
//@参数 nFlags 调整标识, @ref adjustLayout_
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 窗口_调整布局扩展()
XC_API void WINAPI XWnd_AdjustLayoutEx(HWINDOW hWindow, int nFlags=adjustLayout_self);
//@备注 创建插入符,基于元素坐标.  
//@参数 hWindow 窗口句柄.
//@参数 hEle 元素句柄.
//@参数 x x坐标.
//@参数 y y坐标.
//@参数 width 宽度.
//@参数 height 高度.
//@别名 窗口_创建插入符()
XC_API void WINAPI XWnd_CreateCaret(HWINDOW hWindow, HELE hEle, int x, int y, int width, int height);
//@备注 设置插入符位置. 
//@参数 hWindow 窗口句柄.
//@参数 x x坐标.
//@参数 y y坐标.
//@参数 width 宽度.
//@参数 height 高度.
//@参数 bUpdate 是否立即更新UI
//@别名 窗口_置插入符位置()
XC_API void WINAPI XWnd_SetCaretPos(HWINDOW hWindow, int x, int y, int width, int height, BOOL bUpdate=FALSE);
//@备注 设置插入符颜色.  
//@参数 hWindow 窗口句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 窗口_置插入符颜色()
XC_API void WINAPI XWnd_SetCaretColor(HWINDOW hWindow, COLORREF color);
//@备注 显示插入符. 
//@参数 hWindow 窗口句柄.
//@参数 bShow 是否显示.
//@别名 窗口_显示插入符()
XC_API void WINAPI XWnd_ShowCaret(HWINDOW hWindow, BOOL bShow);
//@参数 hWindow 窗口句柄
//@返回 返回插入符元素
//@别名 窗口_取插入符元素()
XC_API HELE WINAPI XWnd_GetCaretHELE(HWINDOW hWindow);
//@备注 获取插入符信息
//@参数 hWindow 窗口句柄
//@参数 pX 接收返回x坐标
//@参数 pY 接收返回y坐标
//@参数 pWidth 接收返回宽度
//@参数 pHeight 接收返回高度
//@返回 插入符元素句柄
//@别名 窗口_取插入符信息()
XC_API HELE WINAPI XWnd_GetCaretInfo(HWINDOW hWindow, int* pX, int* pY, int* pWidth, int* pHeight);
//@备注 销毁插入符.  
//@参数 hWindow 窗口句柄.
//@别名 窗口_销毁插入符()
XC_API void WINAPI XWnd_DestroyCaret(HWINDOW hWindow);
//@备注 获取窗口客户区坐标;如果是阴影窗口,那么不包含阴影部分.  
//@参数 hWindow 窗口句柄.
//@参数 pRect 坐标.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 窗口_取客户区坐标()
XC_API BOOL WINAPI XWnd_GetClientRect(HWINDOW hWindow, RECT* pRect);
//@备注 获取body坐标.  
//@参数 hWindow 窗口句柄.
//@参数 pRect 坐标.
//@别名 窗口_取Body坐标()
XC_API void WINAPI XWnd_GetBodyRect(HWINDOW hWindow, RECT* pRect);
//@参数 hWindow 窗口句柄
//@参数 pRect 接收返回坐标
//@别名 窗口_取布局坐标()
XC_API void WINAPI XWnd_GetLayoutRect(HWINDOW hWindow, RECT* pRect);
//@备注 移动窗口
//@参数 hWindow 窗口句柄
//@参数 x X坐标
//@参数 y Y坐标
//@别名 窗口_置位置()
XC_API void WINAPI XWnd_SetPosition(HWINDOW hWindow, int x, int y);
//@备注 封装的系统API: GetCursorPos(), 内部做了DPI适配
//@参数 hWindow 窗口句柄
//@参数 pPt 接收返回坐标点
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 窗口_取光标位置()
XC_API BOOL WINAPI XWnd_GetCursorPos(HWINDOW hWindow, POINT* pPt);
//@备注 封装的系统API: ClientToScreen(), 内部做了DPI适配
//@参数 hWindow 窗口句柄
//@参数 pPt 接收返回坐标点
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 窗口_客户区坐标点到屏幕()
XC_API BOOL WINAPI XWnd_ClientToScreen(HWINDOW hWindow, POINT* pPt);
//@备注 封装的系统API: ScreenToClient(), 内部做了DPI适配
//@参数 hWindow 窗口句柄
//@参数 pPt 接收返回坐标点
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 窗口_屏幕坐标点到客户区()
XC_API BOOL WINAPI XWnd_ScreenToClient(HWINDOW hWindow, POINT* pPt);
//@备注 窗口客户区坐标转换到缩放后DPI坐标
//@参数 hWindow 窗口句柄
//@参数 pRect 接收返回坐标
//@别名 窗口_坐标转换DPI()
XC_API void WINAPI XWnd_RectToDPI(HWINDOW hWindow, RECT* pRect);
//@备注 窗口客户区坐标点转换到缩放后
//@参数 hWindow 窗口句柄
//@参数 pPt 接收返回坐标点
//@别名 窗口_坐标点转换DPI()
XC_API void WINAPI XWnd_PointToDPI(HWINDOW hWindow, POINT* pPt);
//@参数 hWindow 窗口句柄
//@参数 pRect 坐标
//@别名 窗口_取坐标()
XC_API void WINAPI XWnd_GetRect(HWINDOW hWindow, RECT* pRect);
//@参数 hWindow 窗口句柄
//@参数 pRect 坐标
//@别名 窗口_置坐标()
XC_API void WINAPI XWnd_SetRect(HWINDOW hWindow, RECT* pRect);
//@参数 hWindow 窗口句柄
//@参数 bTop 是否置顶, TRUE置顶, FALSE取消置顶
//@别名 窗口_置顶()
XC_API void WINAPI XWnd_SetTop(HWINDOW hWindow, BOOL bTop=TRUE);
//@参数 hWindow 窗口句柄
//@参数 bMaximize 是否最大化
//@别名 窗口_最大化()
XC_API void WINAPI XWnd_MaxWindow(HWINDOW hWindow, BOOL bMaximize);
//@备注 封装系统API SetWindowPos(), 内部做了DPI适配
//@参数 hWindow 窗口句柄
//@参数 hWndInsertAfter 前面窗口
//@参数 X X坐标
//@参数 Y Y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 uFlags 窗口大小调整和定位标志, 详情见MSDN: https://learn.microsoft.com/zh-cn/windows/win32/api/winuser/nf-winuser-setwindowpos
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 窗口_置窗口位置()
XC_API BOOL WINAPI XWnd_SetWindowPos(HWINDOW hWindow, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);

//@备注  设置鼠标悬停触发时间间隔, 当鼠标移到目标UI元素上停留多久触发鼠标悬停事件
//@参数  hWindow  窗口句柄
//@参数  time     鼠标悬停触发时间间隔,毫秒为单位
//@别名  窗口_置鼠标悬停触发时间()
XC_API void WINAPI XWnd_SetMouseHoverTime(HWINDOW hWindow, int time);

//@备注  将窗口位置调整到屏幕内
//@参数  hWindow   窗口句柄
//@参数  nBorderSpace  窗口边框外围间隔大小
//@参数  bCoverTaskBar 窗口是否覆盖任务栏
//@返回  如果窗口位置改变返回真, 否则相反
//@别名  窗口_调整到屏幕内()
XC_API BOOL WINAPI XWnd_AdjustInScreen(HWINDOW hWindow, int nBorderSpace = 0, BOOL bCoverTaskBar = FALSE);

//@备注 设置窗口定时器.  
//@参数 hWindow 窗口句柄.
//@参数 nIDEvent 定时器ID.
//@参数 uElapse 间隔值,单位毫秒.
//@返回 参见MSDN.
//@别名 窗口_置定时器()
XC_API UINT WINAPI XWnd_SetTimer(HWINDOW hWindow, UINT nIDEvent, UINT uElapse);
//@备注 关闭定时器.  
//@参数 hWindow 窗口句柄.
//@参数 nIDEvent 定时器ID.
//@返回 参见MSDN.
//@别名 窗口_关闭定时器()
XC_API BOOL WINAPI XWnd_KillTimer(HWINDOW hWindow, UINT nIDEvent);
//@备注 设置炫彩窗口定时器.  
//@参数 hWindow 窗口句柄.
//@参数 nIDEvent 定时器ID.
//@参数 uElapse 间隔值,单位毫秒.
//@返回 成功返回TURE,否则返回FALSE.
//@别名 窗口_置炫彩定时器()
XC_API BOOL WINAPI XWnd_SetXCTimer(HWINDOW hWindow, UINT nIDEvent, UINT uElapse);
//@备注 关闭炫彩窗口定时器.  
//@参数 hWindow 窗口句柄.
//@参数 nIDEvent 定时器ID.
//@返回 成功返回TURE,否则返回FALSE.
//@别名 窗口_关闭炫彩定时器()
XC_API BOOL WINAPI XWnd_KillXCTimer(HWINDOW hWindow, UINT nIDEvent);
//@备注 添加背景内容边框
//@参数 hWindow 窗口句柄
//@参数 nState 组合状态 参加文档: API接口->组合状态
//@参数 color 颜色.
//@参数 width 线宽.
//@别名 窗口_添加背景边框()
XC_API void WINAPI XWnd_AddBkBorder(HWINDOW hWindow, int nState, COLORREF color, int width);
//@备注 添加背景内容填充
//@参数 hWindow 窗口句柄
//@参数 nState 组合状态 参加文档: API接口->组合状态
//@参数 color 颜色.
//@别名 窗口_添加背景填充()
XC_API void WINAPI XWnd_AddBkFill(HWINDOW hWindow, int nState, COLORREF color);
//@备注 添加背景内容图片
//@参数 hWindow 窗口句柄
//@参数 nState 组合状态 参加文档: API接口->组合状态
//@参数 hImage 图片句柄.
//@别名 窗口_添加背景图片()
XC_API void WINAPI XWnd_AddBkImage(HWINDOW hWindow, int nState, HIMAGE hImage);
//@参数 hWindow 窗口句柄
//@参数 pText 背景内容字符串
//@返回 返回设置的背景对象数量
//@别名 窗口_置背景()
XC_API int WINAPI XWnd_SetBkInfo(HWINDOW hWindow, const wchar_t* pText);
//@备注 获取背景内容数量
//@参数 hWindow 窗口句柄
//@返回 返回背景内容数量.
//@别名 窗口_取背景对象数量()
XC_API int WINAPI XWnd_GetBkInfoCount(HWINDOW hWindow);
//@备注 清空背景内容; 如果背景没有内容,将使用系统默认内容,以便保证背景正确
//@参数 hWindow 窗口句柄
//@别名 窗口_清空背景对象()
XC_API void WINAPI XWnd_ClearBkInfo(HWINDOW hWindow);
//@备注 获取背景管理器.  
//@参数 hWindow 窗口句柄.
//@返回 背景管理器.
//@别名 窗口_取背景管理器()
XC_API HBKM WINAPI XWnd_GetBkManager(HWINDOW hWindow);
//@备注 获取背景管理器,优先从资源中获取.  
//@参数 hWindow 窗口句柄
//@返回 背景管理器.
//@别名 窗口_取背景管理器扩展()
XC_API HBKM WINAPI XWnd_GetBkManagerEx(HWINDOW hWindow);
//@参数 hWindow 窗口句柄
//@参数 hBkInfoM 背景管理器
//@别名 窗口_置背景管理器()
XC_API void WINAPI XWnd_SetBkMagager(HWINDOW hWindow, HBKM hBkInfoM);
//@备注 设置透明窗口,同时可以通过该函数关闭透明窗口;  
//而调整布局的函数应该放在 XWnd_SetTransparentType() 之后调用.
//@参数 hWindow 窗口句柄.
//@参数 nType 窗口透明类型.
//@别名 窗口_置透明类型()
XC_API void WINAPI XWnd_SetTransparentType(HWINDOW hWindow, window_transparent_ nType);
//@备注 设置透明窗口的透明度,设置后调用重绘窗口API来更新. 
//@参数 hWindow 窗口句柄.
//@参数 alpha 窗口透明度,范围0-255之间,0透明,255不透明.
//@别名 窗口_置透明度()
XC_API void WINAPI XWnd_SetTransparentAlpha(HWINDOW hWindow, BYTE alpha);
//@备注 设置透明窗口的透明色.  
//@参数 hWindow 窗口句柄.
//@参数 color 窗口透明色.
//@别名 窗口_置透明色()
XC_API void WINAPI XWnd_SetTransparentColor(HWINDOW hWindow, COLORREF color);
//@备注 设置窗口阴影信息.   
//@参数 hWindow 窗口句柄.
//@参数 nSize 阴影大小
//@参数 nDepth 阴影深度,0-255.
//@参数 nAngeleSize 圆角阴影内收大小.
//@参数 bRightAngle 是否强制直角.
//@参数 color 阴影颜色. 
//@别名 窗口_置阴影信息()
XC_API void WINAPI XWnd_SetShadowInfo(HWINDOW hWindow, int nSize, int nDepth, int nAngeleSize, BOOL bRightAngle, COLORREF color);
//@备注 获取窗口阴影信息.  
//@参数 hWindow 窗口句柄.
//@参数 pnSize 阴影大小.
//@参数 pnDepth 阴影深度(0-255), 颜色的深浅
//@参数 pnAngeleSize 圆角阴影内收大小, 阴影圆角大小
//@参数 pbRightAngle 是否强制直角, 圆角或直角
//@参数 pColor 阴影颜色.
//@别名 窗口_取阴影信息()
XC_API void WINAPI XWnd_GetShadowInfo(HWINDOW hWindow, int* pnSize, int* pnDepth, int* pnAngeleSize, BOOL* pbRightAngle, COLORREF* pColor);
//@备注 获取窗口透明类型  
//@参数 hWindow 窗口句柄.
//@返回 返回窗口透明类型.
//@别名 窗口_取透明类型()
XC_API window_transparent_ WINAPI XWnd_GetTransparentType(HWINDOW hWindow);
//@分组}
//@分组{  模态窗口

//@备注 创建模态窗口;当模态窗口关闭时,会自动销毁模态窗口资源句柄.  
//@参数 nWidth 宽度.
//@参数 nHeight 高度.
//@参数 pTitle 窗口标题内容.
//@参数 hWndParent 父窗口句柄.
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_.
//@返回 模态窗口句柄.
//@别名 模态窗口_创建()
XC_API HWINDOW WINAPI XModalWnd_Create(int nWidth, int nHeight, const wchar_t* pTitle, HWND hWndParent, int XCStyle=window_style_modal);
//@备注 创建模态窗口,增强功能.  
//@参数 dwExStyle 窗口扩展样式.
//@参数 dwStyle 窗口样式.
//@参数 lpClassName 窗口类名.
//@参数 x 窗口左上角x坐标.
//@参数 y 窗口左上角y坐标.
//@参数 cx 窗口宽度.
//@参数 cy 窗口高度.
//@参数 pTitle 窗口名.
//@参数 hWndParent 父窗口.
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_.
//@返回 GUI库窗口资源句柄.
//@别名 模态窗口_创建扩展()
XC_API HWINDOW WINAPI XModalWnd_CreateEx(DWORD dwExStyle, DWORD dwStyle, const wchar_t* lpClassName, int x, int y, int cx, int cy, const wchar_t* pTitle, HWND hWndParent, int XCStyle=window_style_modal);
//@参数 hWnd 要附加的外部窗口句柄
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_
//@返回 返回窗口句柄
//@别名 模态窗口_附加窗口()
XC_API HWINDOW WINAPI XModalWnd_Attach(HWND hWnd, int XCStyle);
//@备注 是否自动关闭窗口,当窗口失去焦点时.  
//@参数 hWindow 模态窗口句柄.
//@参数 bEnable 开启开关.
//@别名 模态窗口_启用自动关闭()
XC_API void WINAPI XModalWnd_EnableAutoClose(HWINDOW hWindow, BOOL bEnable);
//@备注 当用户按ESC键时自动关闭模态窗口.  
//@参数 hWindow 模态窗口句柄.
//@参数 bEnable 是否启用
//@别名 模态窗口_启用ESC关闭()
XC_API void WINAPI XModalWnd_EnableEscClose(HWINDOW hWindow, BOOL bEnable);
//@备注 启动显示模态窗口,当窗口关闭时返回.  
//@参数 hWindow 模态窗口句柄.
//@返回 messageBox_flag_ok:点击确定按钮退出.messageBox_flag_cancel:点击取消按钮退出.messageBox_flag_other:其他方式退出.
//@别名 模态窗口_启动()
XC_API int WINAPI XModalWnd_DoModal(HWINDOW hWindow);
//@备注 结束模态窗口.  
//@参数 hWindow 窗口句柄.
//@参数 nResult XModalWnd_DoModal() 返回值.
//@别名 模态窗口_结束()
XC_API void WINAPI XModalWnd_EndModal(HWINDOW hWindow, int nResult);
//@分组}
//@分组{  框架窗口

//@备注 创建框架窗口  
//@参数 x 窗口左上角x坐标.
//@参数 y 窗口左上角y坐标.
//@参数 cx 窗口宽度.
//@参数 cy 窗口高度.
//@参数 pTitle 窗口标题.
//@参数 hWndParent 父窗口.
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_.
//@返回 GUI库窗口资源句柄.
//@别名 框架窗口_创建()
XC_API HWINDOW WINAPI XFrameWnd_Create(int x, int y, int cx, int cy, const wchar_t* pTitle, HWND hWndParent, int XCStyle);
//@备注 创建框架窗口,增强功能.  
//@参数 dwExStyle 窗口扩展样式.
//@参数 dwStyle 窗口样式
//@参数 lpClassName 窗口类名.
//@参数 x 窗口左上角x坐标.
//@参数 y 窗口左上角y坐标.
//@参数 cx 窗口宽度.
//@参数 cy 窗口高度.
//@参数 pTitle 窗口名.
//@参数 hWndParent 父窗口.
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_.
//@返回 GUI库窗口资源句柄.
//@别名 框架窗口_创建扩展()
XC_API HWINDOW WINAPI XFrameWnd_CreateEx(DWORD dwExStyle, DWORD dwStyle, const wchar_t* lpClassName, int x, int y, int cx, int cy, const wchar_t* pTitle, HWND hWndParent, int XCStyle);
//@参数 hWnd 要附加的外部窗口句柄
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_
//@返回 返回窗口句柄
//@别名 框架窗口_附加窗口()
XC_API HWINDOW WINAPI XFrameWnd_Attach(HWND hWnd, int XCStyle);
//@备注 用来布局窗格的区域坐标,不包含码头.  
//@参数 hWindow 窗口句柄.
//@参数 pRect 返回坐标.
//@别名 框架窗口_取布局区域坐标()
XC_API void WINAPI XFrameWnd_GetLayoutAreaRect(HWINDOW hWindow, RECT* pRect);
//@备注 获取框架窗口主视图区域坐标.  
//@参数 hWindow 窗口句柄.
//@参数 pRect 返回坐标.
//@别名 框架窗口_取主视图坐标()
XC_API void WINAPI XFrameWnd_GetViewRect(HWINDOW hWindow, RECT* pRect);
//@备注 设置主视图元素.  
//@参数 hWindow 窗口句柄.
//@参数 hEle 元素句柄.
//@别名 框架窗口_置视图()
XC_API void WINAPI XFrameWnd_SetView(HWINDOW hWindow, HELE hEle);
//@备注 设置窗格分隔条颜色.  
//@参数 hWindow 窗口句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 框架窗口_置窗格分隔条颜色()
XC_API void WINAPI XFrameWnd_SetPaneSplitBarColor(HWINDOW hWindow, COLORREF color);
//@参数 hWindow 窗口句柄
//@参数 nWidth 宽度
//@别名 框架窗口_置窗格分隔条宽度()
XC_API void WINAPI XFrameWnd_SetPaneSplitBarWidth(HWINDOW hWindow, int nWidth);
//@参数 hWindow 窗口句柄
//@返回 返回格分隔条宽度
//@别名 框架窗口_取窗格分隔条宽度()
XC_API int WINAPI XFrameWnd_GetPaneSplitBarWidth(HWINDOW hWindow);
//@备注 设置窗格组TabBar高度
//@参数 hWindow 窗口句柄
//@参数 nHeight 高度
//@别名 框架窗口_置TabBar条高度()
XC_API void WINAPI XFrameWnd_SetTabBarHeight(HWINDOW hWindow, int nHeight);
//@参数 hWindow 窗口句柄
//@返回 返回 拖动窗格 所处框架窗口单元格标识
//@别名 框架窗口_取拖动浮动窗格停留位置标识()
XC_API frameWnd_cell_type_ WINAPI XFrameWnd_GetDragFloatWndTopFlag(HWINDOW hWindow);
//@备注 设置框架窗口 窗格区域布局的外间距
//@参数 hWindow 窗口句柄
//@参数 left 左边间隔
//@参数 top 右边间隔
//@参数 right 右边间隔
//@参数 bottom 底边间隔
//@别名 框架窗口_置布局外间距()
XC_API void WINAPI XFrameWnd_SetLayoutMargin(HWINDOW hWindow, int left, int top, int right, int bottom);
//@备注 获取框架窗口停靠码头元素, 若是没有返回空
//@参数 hWindow 窗口句柄
//@参数 number 码头编号, 1:左侧码头, 2:顶部码头, 3:右侧码头, 4:底部码头
//@返回 返回码头句柄, 类型为基础元素
//@别名 框架窗口_取码头()
XC_API HELE WINAPI XFrameWnd_GetDock(HWINDOW hWindow, int number);
//@备注 保存布局信息到文件.  
//@参数 hWindow 窗口句柄.
//@参数 pFileName 文件名，如果文件名为空，将使用默认文件名frameWnd_layout.xml.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 框架窗口_保存布局到文件()
XC_API BOOL WINAPI XFrameWnd_SaveLayoutToFile(HWINDOW hWindow, const wchar_t* pFileName);
//@备注 加载布局信息文件, 若是失败, 需要手动将窗格添加到框架窗口中.  
//@参数 hWindow 窗口句柄.
//@参数 aPaneList 窗格句柄数组.
//@参数 nPaneCount 窗格数量.
//@参数 pFileName 文件名，如果文件名为空，将使用默认文件名frameWnd_layout.xml.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 框架窗口_加载布局信息文件()
XC_API BOOL WINAPI XFrameWnd_LoadLayoutFile(HWINDOW hWindow, HELE* aPaneList, int nEleCount, const wchar_t* pFileName);
//@备注 添加窗格到框架窗口.  
//@参数 hWindow 窗口句柄.
//@参数 hPaneDest 目标窗格.
//@参数 hPaneNew 当前窗格.
//@参数 align 对齐方式.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 框架窗口_添加窗格()
XC_API BOOL WINAPI XFrameWnd_AddPane(HWINDOW hWindow, HELE hPaneDest, HELE hPaneNew, pane_align_ align);
//@备注 合并窗格.  
//@参数 hWindow 窗口句柄.
//@参数 hPaneDest 目标窗格.
//@参数 hPaneNew 当前窗格.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 框架窗口_合并窗格()
XC_API BOOL WINAPI XFrameWnd_MergePane(HWINDOW hWindow, HELE hPaneDest, HELE hPaneNew);
//@分组}
//@分组{  弹出菜单

//@备注 创建菜单.默认弹出菜单窗口关闭后自动销毁.  
//@返回 菜单句柄.
//@别名 菜单_创建()
XC_API HMENUX WINAPI XMenu_Create();
//@备注 添加菜单项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@参数 pText 文本内容.
//@参数 nParentID 父项ID.
//@参数 nFlags 标识参见宏定义 @ref menu_item_flag_.
//@别名 菜单_添加项()
XC_API void WINAPI XMenu_AddItem(HMENUX hMenu, int nID, const wchar_t* pText, int parentId=XC_ID_ROOT, int nFlags=0);
//@备注 添加菜单项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@参数 pText 文本内容.
//@参数 nParentID 父项ID.
//@参数 hIcon 菜单项图标句柄.
//@参数 nFlags 标识参见宏定义 @ref menu_item_flag_.
//@别名 菜单_添加项图标()
XC_API void WINAPI XMenu_AddItemIcon(HMENUX hMenu, int nID, const wchar_t* pText, int nParentID, HIMAGE hImage, int nFlags=0);
//@备注 插入菜单项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@参数 pText 文本内容.
//@参数 nFlags 标识参见宏定义 @ref menu_item_flag_.
//@参数 insertID 插入位置ID.
//@别名 菜单_插入项()
XC_API void WINAPI XMenu_InsertItem(HMENUX hMenu, int nID, const wchar_t* pText, int nFlags, int insertID);
//@备注 插入菜单项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@参数 pText 文本内容.
//@参数 hIcon 菜单项图标句柄.
//@参数 nFlags 标识参见宏定义 @ref menu_item_flag_.
//@参数 insertID 插入位置ID.
//@别名 菜单_插入项图标()
XC_API void WINAPI XMenu_InsertItemIcon(HMENUX hMenu, int nID, const wchar_t* pText, HIMAGE hIcon, int nFlags, int insertID);
//@备注 获取第一个子项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@返回 返回项ID.
//@别名 菜单_取第一个子项()
XC_API int WINAPI XMenu_GetFirstChildItem(HMENUX hMenu, int nID);
//@备注 获取末尾子项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@返回 返回项ID.
//@别名 菜单_取末尾子项()
XC_API int WINAPI XMenu_GetEndChildItem(HMENUX hMenu, int nID);
//@备注 获取上一个兄弟项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@返回 返回项ID.
//@别名 菜单_取上一个兄弟项()
XC_API int WINAPI XMenu_GetPrevSiblingItem(HMENUX hMenu, int nID);
//@备注 获取下一个兄弟项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@返回 返回项ID.
//@别名 菜单_取下一个兄弟项()
XC_API int WINAPI XMenu_GetNextSiblingItem(HMENUX hMenu, int nID);
//@备注 获取父项.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@返回 返回项ID.
//@别名 菜单_取父项()
XC_API int WINAPI XMenu_GetParentItem(HMENUX hMenu, int nID);
//@备注 获取菜单所属菜单条.  
//@参数 hMenu 菜单句柄
//@返回 返回菜单条句柄
//@别名 菜单_取菜单条()
XC_API HELE WINAPI XMenu_GetMenuBar(HMENUX hMenu);
//@备注 设置是否自动销毁菜单.  
//@参数 hMenu 菜单句柄.
//@参数 bAuto 是否自动销毁.
//@别名 菜单_置自动销毁()
XC_API void WINAPI XMenu_SetAutoDestroy(HMENUX hMenu, BOOL bAuto);
//@备注 是否有用户绘制菜单背景,如果启用XWM_MENU_DRAW_BACKGROUND和XE_MENU_DRAW_BACKGROUND事件有效.  
//@参数 hMenu 菜单句柄.
//@参数 bEnable 是否启用.
//@别名 菜单_启用用户绘制背景()
XC_API void WINAPI XMenu_EnableDrawBackground(HMENUX hMenu, BOOL bEnable);
//@备注 是否有用户绘制菜单项,如果启用XWM_MENU_DRAWITEM和XE_MENU_DRAWITEM事件有效.  
//@参数 hMenu 菜单句柄.
//@参数 bEnable 是否启用.
//@别名 菜单_启用用户绘制项()
XC_API void WINAPI XMenu_EnableDrawItem(HMENUX hMenu, BOOL bEnable);
//@备注 弹出菜单.  
//否则将由hParentWnd窗口接收菜单的消息事件
//@参数 hMenu 菜单句柄.
//@参数 hParentWnd 父窗口句柄.
//@参数 x x坐标.
//@参数 y y坐标.
//@参数 hParentEle 父元素句柄,如果该值不为NULL,hParentEle元素将接收菜单消息事件,
//@参数 nPosition 弹出位置,参见宏定义.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 菜单_弹出()
XC_API BOOL WINAPI XMenu_Popup(HMENUX hMenu, HWND hParentWnd, int x, int y, HELE hParentEle=NULL, menu_popup_position_ nPosition=menu_popup_position_left_top);
//@备注 销毁菜单.  
//@参数 hMenu 菜单句柄.
//@别名 菜单_销毁()
XC_API void WINAPI XMenu_DestroyMenu(HMENUX hMenu);
//@备注 关闭菜单.  
//@参数 hMenu 菜单句柄.
//@别名 菜单_关闭()
XC_API void WINAPI XMenu_CloseMenu(HMENUX hMenu);
//@备注 设置菜单背景图片.  
//@参数 hMenu 菜单句柄.
//@参数 hImage 图片句柄.
//@别名 菜单_置背景图片()
XC_API void WINAPI XMenu_SetBkImage(HMENUX hMenu, HIMAGE hImage);
//@备注 设置项文本.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@参数 pText 文本内容.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 菜单_置项文本()
XC_API BOOL WINAPI XMenu_SetItemText(HMENUX hMenu, int nID, const wchar_t* pText);
//@备注 获取项文本.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@返回 返回文本内容
//@别名 菜单_取项文本()
XC_API const wchar_t* WINAPI XMenu_GetItemText(HMENUX hMenu, int nID);
//@备注 获取项文本长度,不包含字符串空终止符.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@返回 长度,字符为单位.
//@别名 菜单_取项文本长度()
XC_API int WINAPI XMenu_GetItemTextLength(HMENUX hMenu, int nID);
//@备注 设置菜单项图标.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@参数 hIcon 菜单项图标句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 菜单_置项图标()
XC_API BOOL WINAPI XMenu_SetItemIcon(HMENUX hMenu, int nID, HIMAGE hIcon);
//@备注 设置项标识.  
//@参数 hMenu 菜单句柄.
//@参数 nID 项ID.
//@参数 uFlags 标识参见宏定义 @ref menu_item_flag_.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 菜单_置项标志()
XC_API BOOL WINAPI XMenu_SetItemFlags(HMENUX hMenu, int nID, int uFlags);
//@备注 设置项高度.  
//@参数 hMenu 菜单句柄.
//@参数 height 高度.
//@别名 菜单_置项高度()
XC_API void WINAPI XMenu_SetItemHeight(HMENUX hMenu, int height);
//@备注 获取项高度.  
//@参数 hMenu 菜单句柄.
//@返回 返回项高度.
//@别名 菜单_取项高度()
XC_API int WINAPI XMenu_GetItemHeight(HMENUX hMenu);
//@备注 此宽度为文本显示区域宽度, 不包含侧边条和与文本间隔
//@参数 hMenu 菜单句柄
//@参数 nID 项ID
//@参数 nWidth 指定文本区域宽度
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 菜单_置项宽度()
XC_API BOOL WINAPI XMenu_SetItemWidth(HMENUX hMenu, int nID, int nWidth);
//@备注 设置菜单边框颜色.  
//@参数 hMenu 菜单句柄.
//@参数 crColor 颜色值, 请使用宏: RGBA()
//@别名 菜单_置边框颜色()
XC_API void WINAPI XMenu_SetBorderColor(HMENUX hMenu, COLORREF crColor);
//@备注 设置弹出菜单窗口边框大小.  
//@参数 hMenu 菜单句柄.
//@参数 nLeft 边大小.
//@参数 nTop 边大小.
//@参数 nRight 边大小.
//@参数 nBottom 边大小.
//@别名 菜单_置边框大小()
XC_API void WINAPI XMenu_SetBorderSize(HMENUX hMenu, int nLeft, int nTop, int nRight, int nBottom);
//@备注 获取左侧区域宽度.  
//@参数 hMenu 菜单句柄.
//@返回 返回左侧区域宽度.
//@别名 菜单_取左侧宽度()
XC_API int WINAPI XMenu_GetLeftWidth(HMENUX hMenu);

//@备注  设置左侧区域宽度
//@参数  hMenu 菜单句柄
//@参数  nWidth 宽度
//@别名  菜单_置左侧宽度()
XC_API void WINAPI XMenu_SetLeftWidth(HMENUX hMenu, int nWidth);

//@备注 获取菜单项文本左间隔.  
//@参数 hMenu 菜单句柄.
//@返回 返回菜单项文件左间隔大小.
//@别名 菜单_取左侧文本间隔()
XC_API int WINAPI XMenu_GetLeftSpaceText(HMENUX hMenu);
//@备注 获取菜单项数量,包含子菜单项.  
//@参数 hMenu 菜单句柄.
//@返回 菜单项数量.
//@别名 菜单_取项数量()
XC_API int WINAPI XMenu_GetItemCount(HMENUX hMenu);
//@备注 设置菜单项勾选状态.  
//@参数 hMenu 菜单句柄.
//@参数 nID 菜单项ID
//@参数 bCheck 勾选TRUE,非勾选FALSE
//@返回 如果勾选返回TRUE,否则返回FALSE.
//@别名 菜单_置项勾选()
XC_API BOOL WINAPI XMenu_SetItemCheck(HMENUX hMenu, int nID, BOOL bCheck);
//@备注 判断菜单项是否勾选.  
//@参数 hMenu 菜单句柄.
//@参数 nID 菜单项ID
//@返回 如果勾选返回TRUE,否则返回FALSE.
//@别名 菜单_是否项勾选()
XC_API BOOL WINAPI XMenu_IsItemCheck(HMENUX hMenu, int nID);

//@备注  启用或禁用样式
//@参数  hMenu 菜单句柄
//@参数  bEnable 是否启用
//@别名  菜单_启用CSS()
XC_API void WINAPI XMenu_EnableCSS(HMENUX hMenu, BOOL bEnable);
//@备注  设置CSS[套用样式]名称
//@参数  hMenu 菜单句柄
//@参数  type : 0菜单项, 1主菜单窗口背景, 2子菜单窗口背景
//@参数  pName 套用样式名称
//@别名  菜单_置CSS名称()
XC_API void WINAPI XMenu_SetCssName(HMENUX hMenu, int type, const wchar_t* pName);
//@备注  获取CSS样式名称
//@参数  hMenu 菜单句柄
//@参数  type : 0菜单项, 1主菜单窗口背景, 2子菜单窗口背景
//@返回  返回CSS样式名称
//@别名  菜单_取CSS名称()
XC_API const wchar_t* WINAPI XMenu_GetCssName(HMENUX hMenu, int type);

//@备注  置背景管理器
//@参数  hMenu 菜单句柄
//@参数  type : 0菜单项, 1主菜单窗口背景, 2子菜单窗口背景
//@参数  hBkInfoM  背景管理器
//@别名  菜单_置背景管理器()
XC_API void WINAPI XMenu_SetBkManager(HMENUX hMenu, int type, HBKM hBkInfoM);
//@备注 取背景管理器
//@参数 hMenu 菜单句柄
//@参数 type : 0菜单项, 1主菜单窗口背景, 2子菜单窗口背景
//@返回 背景管理器
//@别名 菜单_取背景管理器()
XC_API HBKM WINAPI XMenu_GetBkManager(HMENUX hMenu, int type);
//@备注 获取元素背景管理器,优先从资源中获取. 
//@参数 hMenu 菜单句柄
//@参数 type : 0菜单项, 1主菜单窗口背景, 2子菜单窗口背景
//@返回 背景管理器
//@别名 菜单_取背景管理器扩展()
XC_API HBKM WINAPI XMenu_GetBkManagerEx(HMENUX hMenu, int type);

//@分组}
//@分组{  布局盒子

//@参数 hLayoutBox 窗口或布局元素或布局框架句柄
//@参数 bEnable 是否启用
//@别名 布局盒子_启用水平()
XC_API void WINAPI XLayoutBox_EnableHorizon(HXCGUI hLayoutBox, BOOL bEnable);
//@参数 hLayoutBox 窗口或布局元素或布局框架句柄
//@参数 bEnable 是否启用
//@别名 布局盒子_启用自动换行()
XC_API void WINAPI XLayoutBox_EnableAutoWrap(HXCGUI hLayoutBox, BOOL bEnable);
//@参数 hLayoutBox 窗口或布局元素或布局框架句柄
//@参数 bEnable 是否启用
//@别名 布局盒子_启用溢出隐藏()
XC_API void WINAPI XLayoutBox_EnableOverflowHide(HXCGUI hLayoutBox, BOOL bEnable);
//@参数 hLayoutBox 窗口或布局元素或布局框架句柄
//@参数 nAlign 对齐方式
//@别名 布局盒子_置水平对齐()
XC_API void WINAPI XLayoutBox_SetAlignH(HXCGUI hLayoutBox, layout_align_ nAlign);
//@参数 hLayoutBox 窗口或布局元素或布局框架句柄
//@参数 nAlign 对齐方式
//@别名 布局盒子_置垂直对齐()
XC_API void WINAPI XLayoutBox_SetAlignV(HXCGUI hLayoutBox, layout_align_ nAlign);
//@参数 hLayoutBox 窗口或布局元素或布局框架句柄
//@参数 nAlign 对齐方式
//@别名 布局盒子_置对齐基线()
XC_API void WINAPI XLayoutBox_SetAlignBaseline(HXCGUI hLayoutBox, layout_align_axis_ nAlign);
//@参数 hLayoutBox 窗口或布局元素或布局框架句柄
//@参数 nSpace 项间距大小
//@别名 布局盒子_置间距()
XC_API void WINAPI XLayoutBox_SetSpace(HXCGUI hLayoutBox, int nSpace);
//@参数 hLayoutBox 窗口或布局元素或布局框架句柄
//@参数 nSpace 行间距大小
//@别名 布局盒子_置行距()
XC_API void WINAPI XLayoutBox_SetSpaceRow(HXCGUI hLayoutBox, int nSpace);
//@分组}
//@分组{  基础元素

//@备注 创建基础元素.  
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄.
//@别名 元素_创建()
XC_API HELE WINAPI XEle_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 注册事件C方式,事件函数省略自身句柄.  
//@参数 hEle 元素句柄.
//@参数 nEvent 事件类型.
//@参数 pFun 事件函数指针.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_注册事件C()
XC_API BOOL WINAPI XEle_RegEventC(HELE hEle, int nEvent, void* pFun);
//@备注 注册事件C1方式,事件函数包含自身句柄.  
//@参数 hEle 元素句柄.
//@参数 nEvent 事件类型.
//@参数 pFun 事件函数指针.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_注册事件C1()
XC_API BOOL WINAPI XEle_RegEventC1(HELE hEle, int nEvent, void* pFun);
//@备注 移除事件函数C方式.  
//@参数 hEle 元素句柄.
//@参数 nEvent 事件类型.
//@参数 pFun 事件函数指针.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_移除事件C()
XC_API BOOL WINAPI XEle_RemoveEventC(HELE hEle, int nEvent, void* pFun);
//@备注 发送事件.  
//@参数 hEle 目标元素句柄.
//@参数 nEvent 事件类型.
//@参数 wParam 参数.
//@参数 lParam 参数.
//@返回 事件返回值.
//@别名 元素_发送事件()
XC_API int WINAPI XEle_SendEvent(HELE hEle, int nEvent, WPARAM wParam, LPARAM lParam);
//@备注 POST事件.  
//@参数 hEle 元素句柄.
//@参数 nEvent 事件类型.
//@参数 wParam 参数.
//@参数 lParam 参数.
//@返回 事件返回值.
//@别名 元素_投递事件()
XC_API BOOL WINAPI XEle_PostEvent(HELE hEle, int nEvent, WPARAM wParam, LPARAM lParam);
//@备注 获取元素坐标.  
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@别名 元素_取坐标()
XC_API void WINAPI XEle_GetRect(HELE hEle, RECT* pRect);
//@备注 获取元素坐标,逻辑坐标,包含滚动视图偏移.  
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@别名 元素_取逻辑坐标()
XC_API void WINAPI XEle_GetRectLogic(HELE hEle, RECT* pRect);
//@备注 获取元素客户区坐标. 
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@别名 元素_取客户区坐标()
XC_API void WINAPI XEle_GetClientRect(HELE hEle, RECT* pRect);
//@备注 设置宽度  
//@参数 hEle 元素句柄
//@参数 nWidth 宽度
//@别名 元素_置宽度()
XC_API void WINAPI XEle_SetWidth(HELE hEle, int nWidth);
//@备注 设置高度   
//@参数 hEle 元素句柄
//@参数 nHeight 高度
//@别名 元素_置高度()
XC_API void WINAPI XEle_SetHeight(HELE hEle, int nHeight);
//@备注 获取元素宽度.  
//@参数 hEle 元素句柄.
//@返回 宽度.
//@别名 元素_取宽度()
XC_API int WINAPI XEle_GetWidth(HELE hEle);
//@备注 获取元素高度.  
//@参数 hEle 元素句柄.
//@返回 高度.
//@别名 元素_取高度()
XC_API int WINAPI XEle_GetHeight(HELE hEle);
//@备注 窗口客户区坐标转换到元素客户区坐标.  
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@别名 元素_窗口客户区坐标到元素客户区()
XC_API void WINAPI XEle_RectWndClientToEleClient(HELE hEle, RECT* pRect);
//@备注 窗口客户区坐标转换到元素客户区坐标.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标.
//@别名 元素_窗口客户区点到元素客户区()
XC_API void WINAPI XEle_PointWndClientToEleClient(HELE hEle, POINT* pPt);
//@备注 元素客户区坐标转换到窗口客户区坐标.  
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@别名 元素_客户区坐标到窗口客户区()
XC_API void WINAPI XEle_RectClientToWndClient(HELE hEle, RECT* pRect);
//@备注 缩放后坐标
//@参数 hEle 元素句柄
//@参数 pRect 接收返回坐标
//@别名 元素_客户区坐标到窗口客户区DPI()
XC_API void WINAPI XEle_RectClientToWndClientDPI(HELE hEle, RECT* pRect);
//@备注 元素客户区坐标转换到窗口客户区坐标.  
//@参数 hEle 元素句柄.
//@参数 pPt 接收返回坐标点.
//@别名 元素_客户区点到窗口客户区()
XC_API void WINAPI XEle_PointClientToWndClient(HELE hEle, POINT* pPt);
//@备注 缩放后的坐标
//@参数 hEle 元素句柄
//@参数 pPt 接收返回坐标点
//@别名 元素_客户区点到窗口客户区DPI()
XC_API void WINAPI XEle_PointClientToWndClientDPI(HELE hEle, POINT* pPt);
//@备注 元素取窗口客户区坐标, 100%比例坐标
//@参数 hEle 元素句柄
//@参数 pRect 接收坐标
//@别名 元素_取窗口客户区坐标()
XC_API void WINAPI XEle_GetWndClientRect(HELE hEle, RECT* pRect);
//@备注 基于DPI缩放后的坐标
//@参数 hEle 元素句柄
//@参数 pRect 接收坐标
//@别名 元素_取窗口客户区坐标DPI()
XC_API void WINAPI XEle_GetWndClientRectDPI(HELE hEle, RECT* pRect);
//@备注 获取元素鼠标光标.  
//@参数 hEle 元素句柄.
//@返回 返回光标句柄.
//@别名 元素_取光标()
XC_API HCURSOR WINAPI XEle_GetCursor(HELE hEle);
//@备注 设置元素鼠标光标.  
//@参数 hEle 元素句柄.
//@参数 hCursor 光标句柄.
//@别名 元素_置光标()
XC_API void WINAPI XEle_SetCursor(HELE hEle, HCURSOR hCursor);
//@备注 添加子对象. 
//@参数 hEle 元素句柄.
//@参数 hChild 要添加的子元素句柄或形状对象句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 元素_添加子对象()
XC_API BOOL WINAPI XEle_AddChild(HELE hEle, HXCGUI hChild);
//@备注 插入子对象到指定位置.  
//@参数 hEle 元素句柄.
//@参数 hChild 要插入的元素句柄或形状对象句柄.
//@参数 index 插入位置索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_插入子对象()
XC_API BOOL WINAPI XEle_InsertChild(HELE hEle, HXCGUI hChild, int index);
//@备注 设置元素坐标.  
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@参数 bRedraw 是否重绘.
//@参数 nFlags 调整布局标识位, @ref adjustLayout_
//@参数 nAdjustNo 调整布局流水号
//@返回 如果返回0坐标没有改变,如果大小改变返回2(触发XE_SIZE), 否则返回1(仅改变left,top,没有改变大小).
//@别名 元素_置坐标()
XC_API int WINAPI XEle_SetRect(HELE hEle, RECT* pRect, BOOL bRedraw=FALSE, int nFlags=adjustLayout_all, UINT nAdjustNo=0);
//@备注 设置元素坐标.  
//@参数 hEle 元素句柄.
//@参数 x X坐标.
//@参数 y Y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 bRedraw 是否重绘.
//@参数 nFlags 调整布局标识位, @ref adjustLayout_
//@参数 nAdjustNo 调整布局流水号
//@返回 如果坐标未改变返回0,如果大小改变返回2(触发XE_SIZE), 否则返回1.
//@别名 元素_置坐标扩展()
XC_API int WINAPI XEle_SetRectEx(HELE hEle, int x, int y, int cx, int cy, BOOL bRedraw=FALSE, int nFlags=adjustLayout_all, UINT nAdjustNo=0);
//@备注 设置元素坐标,逻辑坐标,包含滚动视图偏移.  
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@参数 bRedraw 是否重绘.
//@参数 nFlags 参数将被带入 XE_SIZE ,XE_ADJUSTLAYOUT 事件回调.@ref adjustLayout_
//@参数 nAdjustNo 调整布局流水号
//@返回 如果坐标未改变返回0,如果大小改变返回2(触发XE_SIZE), 否则返回1.
//@别名 元素_置逻辑坐标()
XC_API int WINAPI XEle_SetRectLogic(HELE hEle, RECT* pRect, BOOL bRedraw=FALSE, int nFlags=adjustLayout_all, UINT nAdjustNo=0);
//@备注 移动元素坐标
//@参数 hEle 元素句柄.
//@参数 x X坐标.
//@参数 y Y坐标.
//@参数 bRedraw 是否重绘.
//@参数 nFlags 调整布局标识位, @ref adjustLayout_
//@参数 nAdjustNo 调整布局流水号
//@返回 如果坐标未改变返回0,如果大小改变返回2(触发XE_SIZE), 否则返回1.
//@别名 元素_置位置()
XC_API int WINAPI XEle_SetPosition(HELE hEle, int x, int y, BOOL bRedraw=FALSE, int nFlags=adjustLayout_all, UINT nAdjustNo=0);
//@备注 移动元素坐标;  逻辑坐标,包含滚动视图偏移
//@参数 hEle 元素句柄.
//@参数 x X坐标.
//@参数 y Y坐标.
//@参数 bRedraw 是否重绘.
//@参数 nFlags 调整布局标识位, @ref adjustLayout_
//@参数 nAdjustNo 调整布局流水号
//@返回 如果坐标未改变返回0,如果大小改变返回2(触发XE_SIZE), 否则返回1.
//@别名 元素_置位置逻辑()
XC_API int WINAPI XEle_SetPositionLogic(HELE hEle, int x, int y, BOOL bRedraw=FALSE, int nFlags=adjustLayout_all, UINT nAdjustNo=0);
//@参数 hEle 元素句柄
//@参数 pOutX 返回X坐标
//@参数 pOutY 返回Y坐标
//@别名 元素_取位置()
XC_API void WINAPI XEle_GetPosition(HELE hEle, int* pOutX, int* pOutY);
//@参数 hEle 元素句柄
//@参数 nWidth 宽度
//@参数 nHeight 高度
//@参数 bRedraw 是否重绘
//@参数 nFlags 调整布局标识位, @ref adjustLayout_
//@参数 nAdjustNo 调整布局流水号
//@别名 元素_置大小()
XC_API int WINAPI XEle_SetSize(HELE hEle, int nWidth, int nHeight, BOOL bRedraw=FALSE, int nFlags=adjustLayout_all, UINT nAdjustNo=0);
//@参数 hEle 元素句柄
//@参数 pOutWidth 返回宽度
//@参数 pOutHeight 返回高度
//@别名 元素_取大小()
XC_API void WINAPI XEle_GetSize(HELE hEle, int* pOutWidth, int* pOutHeight);
//@备注 元素是否绘制焦点.  
//@参数 hEle 元素句柄.
//@返回 如果绘制焦点返回TRUE否则返回FALSE.
//@别名 元素_是否绘制焦点()
XC_API BOOL WINAPI XEle_IsDrawFocus(HELE hEle);
//@备注 元素是否为启用状态.  
//@参数 hEle 元素句柄.
//@返回 如果启用状态返回TRUE否则返回FALSE.
//@别名 元素_是否启用()
XC_API BOOL WINAPI XEle_IsEnable(HELE hEle);
//@备注 元素是否启用焦点.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_是否启用焦点()
XC_API BOOL WINAPI XEle_IsEnableFocus(HELE hEle);
//@备注 元素是否启用鼠标穿透.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_是否鼠标穿透()
XC_API BOOL WINAPI XEle_IsMouseThrough(HELE hEle);
//@备注 检测坐标点所在元素,包含子元素的子元素.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@返回 成功返回元素句柄,否则返回NULL.
//@别名 元素_测试点击元素()
XC_API HELE WINAPI XEle_HitChildEle(HELE hEle, POINT* pPt);
//@备注 是否背景透明. 
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_是否背景透明()
XC_API BOOL WINAPI XEle_IsBkTransparent(HELE hEle);
//@备注 是否启XE_PAINT_END用事件.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_是否启用事件_XE_PAINT_END()
XC_API BOOL WINAPI XEle_IsEnableEvent_XE_PAINT_END(HELE hEle);
//@备注 是否接受Tab键输入; 例如: XRichEdit, XEdit  
//@参数 hEle 元素句柄
//@返回 是返回TRUE否则返回FALSE
//@别名 元素_是否接受TAB()
XC_API BOOL WINAPI XEle_IsKeyTab(HELE hEle);
//@备注 是否接受通过键盘切换焦点(方向键,TAB键).  
//@参数 hEle 元素句柄.
//@返回 是返回TRUE否则返回FALSE.
//@别名 元素_是否接受切换焦点()
XC_API BOOL WINAPI XEle_IsSwitchFocus(HELE hEle);
//@备注 判断是否启用鼠标滚动事件,如果禁用那么事件会发送给他的父元素.  
//@参数 hEle 元素句柄.
//@返回 是返回TRUE否则返回FALSE.
//@别名 元素_是否启用_XE_MOUSEWHEEL()
XC_API BOOL WINAPI XEle_IsEnable_XE_MOUSEWHEEL(HELE hEle);
//@备注 判断hChildEle是否为hEle的子元素,   
//@参数 hEle 元素句柄.
//@参数 hChildEle 子元素句柄
//@返回 是返回TRUE否则返回FALSE.
//@别名 元素_是否为子元素()
XC_API BOOL WINAPI XEle_IsChildEle(HELE hEle, HELE hChildEle);
//@备注 判断是否启用画布,  
//@参数 hEle 元素句柄.
//@返回 是返回TRUE否则返回FALSE.
//@别名 元素_是否启用画布()
XC_API BOOL WINAPI XEle_IsEnableCanvas(HELE hEle);
//@备注 判断是否拥有焦点.  
//@参数 hEle 元素句柄.
//@返回 如果拥有焦点返回TRUE,否则返回FALSE.
//@别名 元素_是否焦点()
XC_API BOOL WINAPI XEle_IsFocus(HELE hEle);
//@备注 判断该元素或该元素的子元素是否拥有焦点  
//@参数 hEle 元素句柄.
//@返回 如果拥有焦点返回TRUE,否则返回FALSE.
//@别名 元素_是否焦点扩展()
XC_API BOOL WINAPI XEle_IsFocusEx(HELE hEle);
//@备注 启用或禁用元素.  
//@参数 hEle 元素句柄.
//@参数 bEnable 启用或禁用.
//@别名 元素_启用()
XC_API void WINAPI XEle_Enable(HELE hEle, BOOL bEnable);
//@备注 启用焦点.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用焦点()
XC_API void WINAPI XEle_EnableFocus(HELE hEle, BOOL bEnable);
//@备注 启用绘制焦点.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用绘制焦点()
XC_API void WINAPI XEle_EnableDrawFocus(HELE hEle, BOOL bEnable);
//@备注 启用或禁用绘制默认边框.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用绘制边框()
XC_API void WINAPI XEle_EnableDrawBorder(HELE hEle, BOOL bEnable);
//@备注 启用或禁用背景画布;如果禁用那么将绘制在父的画布之上,也就是说他没有自己的画布.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用画布()
XC_API void WINAPI XEle_EnableCanvas(HELE hEle, BOOL bEnable);
//@备注 启用XE_PAINT_END事件.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用事件_XE_PAINT_END()
XC_API void WINAPI XEle_EnableEvent_XE_PAINT_END(HELE hEle, BOOL bEnable);
//@备注 启用背景透明.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用背景透明()
XC_API void WINAPI XEle_EnableBkTransparent(HELE hEle, BOOL bEnable);
//@备注 启用鼠标穿透, 如果启用,那么该元素不能接收到鼠标事件,但是他的子元素不受影响,任然可以接收鼠标事件.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用鼠标穿透()
XC_API void WINAPI XEle_EnableMouseThrough(HELE hEle, BOOL bEnable);
//@备注 启用接收Tab输入.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用接收TAB()
XC_API void WINAPI XEle_EnableKeyTab(HELE hEle, BOOL bEnable);
//@备注 启用接受通过键盘切换焦点.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用切换焦点()
XC_API void WINAPI XEle_EnableSwitchFocus(HELE hEle, BOOL bEnable);
//@备注 启用接收鼠标滚动事件,如果禁用那么事件会传递给父元素.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 元素_启用事件_XE_MOUSEWHEEL()
XC_API void WINAPI XEle_EnableEvent_XE_MOUSEWHEEL(HELE hEle, BOOL bEnable);
//@备注 移除元素,但不销毁.  
//@参数 hEle 元素句柄.
//@别名 元素_移除()
XC_API void WINAPI XEle_Remove(HELE hEle);
//@备注 设置元素Z序.  
//@参数 hEle 元素句柄.
//@参数 index 位置索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_置Z序()
XC_API BOOL WINAPI XEle_SetZOrder(HELE hEle, int index);
//@备注 设置元素Z序.  
//@参数 hEle 元素句柄.
//@参数 hDestEle 目标元素.
//@参数 nType 类型.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_置Z序扩展()
XC_API BOOL WINAPI XEle_SetZOrderEx(HELE hEle, HELE hDestEle, zorder_ nType);
//@备注 获取元素Z序索引, 位置索引.  
//@参数 hEle 元素句柄.
//@返回 成功返回索引值,否则返回 XC_ID_ERROR.
//@别名 元素_取Z序()
XC_API int WINAPI XEle_GetZOrder(HELE hEle);
//@备注 设置元素置顶.  
//@参数 hEle 元素句柄.
//@参数 bTopmost 是否置顶显示
//@返回 成功返回TRUE否则返回FALSE.
//@别名 元素_启用置顶()
XC_API BOOL WINAPI XEle_EnableTopmost(HELE hEle, BOOL bTopmost);
//@备注 重绘元素.  
//@参数 hEle 元素句柄.
//@参数 bImmediate 是否立即重绘,默认为否.
//@别名 元素_重绘()
XC_API void WINAPI XEle_Redraw(HELE hEle, BOOL bImmediate=FALSE);
//@备注 重绘元素指定区域.  
//@参数 hEle 元素句柄.
//@参数 pRect 相对于元素客户区坐标.
//@参数 bImmediate 是否立即重绘,默认为否.
//@别名 元素_重绘指定区域()
XC_API void WINAPI XEle_RedrawRect(HELE hEle, RECT* pRect, BOOL bImmediate=FALSE);
//@备注 获取子对象(UI元素和形状对象)数量,只检测当前层子对象.  
//@参数 hEle 元素句柄.
//@返回 子元素数量.
//@别名 元素_取子对象数量()
XC_API int WINAPI XEle_GetChildCount(HELE hEle);
//@备注 获取子对象通过索引,只检测当前层子对象.  
//@参数 hEle 元素句柄.
//@参数 index 索引.
//@返回 对象句柄.
//@别名 元素_取子对象从索引()
XC_API HXCGUI WINAPI XEle_GetChildByIndex(HELE hEle, int index);
//@备注 获取子对象通过ID,只检测当前层子对象.  
//@参数 hEle 元素句柄.
//@参数 nID 元素ID.
//@返回 对象句柄.
//@别名 元素_取子对象从ID()
XC_API HXCGUI WINAPI XEle_GetChildByID(HELE hEle, int nID);
//@备注 设置边框大小.  
//@参数 hEle 元素句柄.
//@参数 left 左边大小.
//@参数 top 上边大小.
//@参数 right 右边大小.
//@参数 bottom 下边大小.
//@别名 元素_置边框大小()
XC_API void WINAPI XEle_SetBorderSize(HELE hEle, int left, int top, int right, int bottom);
//@备注 获取边框大小.  
//@参数 hEle 元素句柄.
//@参数 pBorder 大小.
//@别名 元素_取边框大小()
XC_API void WINAPI XEle_GetBorderSize(HELE hEle, borderSize_* pBorder);
//@备注 设置被填充大小.  
//@参数 hEle 元素句柄.
//@参数 left 左边大小.
//@参数 top 上边大小.
//@参数 right 右边大小.
//@参数 bottom 下边大小.
//@别名 元素_置内填充大小()
XC_API void WINAPI XEle_SetPadding(HELE hEle, int left, int top, int right, int bottom);
//@备注 获取内填充大小.  
//@参数 hEle 元素句柄.
//@参数 pPadding 大小.
//@别名 元素_取内填充大小()
XC_API void WINAPI XEle_GetPadding(HELE hEle, paddingSize_* pPadding);
//@备注 设置拖动边框.  
//@参数 hEle 元素句柄.
//@参数 nFlags 边框位置组合. element_position_
//@别名 元素_置拖动边框()
XC_API void WINAPI XEle_SetDragBorder(HELE hEle, int nFlags);
//@备注 设置拖动边框绑定元素, 调用 @ref XEle_SetDragBorder() 启用对应的边, 当拖动边框时, 自动调整绑定元素的大小, 每条边可绑定一个元素.  
//@参数 hEle 元素句柄.
//@参数 nFlags 边框位置标识,不可组合.  element_position_
//@参数 hBindEle 绑定元素.
//@参数 nSpace 元素间隔大小
//@别名 元素_置拖动边框绑定元素()
XC_API void WINAPI XEle_SetDragBorderBindEle(HELE hEle, int nFlags, HELE hBindEle, int nSpace);
//@备注 设置元素最小大小. 当用户鼠标拖动元素边框时限制大小, 不同于布局项的最小大小  
//@参数 hEle 元素句柄.
//@参数 nWidth 最小宽度
//@参数 nHeight 最小高度.
//@别名 元素_置最小大小()
XC_API void WINAPI XEle_SetMinSize(HELE hEle, int nWidth, int nHeight);
//@备注 设置元素最大大小. 当用户鼠标拖动元素边框时限制大小 
//@参数 hEle 元素句柄.
//@参数 nWidth 最大宽度.
//@参数 nHeight 最大高度.
//@别名 元素_置最大大小()
XC_API void WINAPI XEle_SetMaxSize(HELE hEle, int nWidth, int nHeight);
//@备注 设置锁定元素, 使其在滚动视图中禁止跟随滚动, 如果设置TRUE将不跟随滚动.  
//@参数 hEle 元素句柄.
//@参数 bHorizon 是否锁定水平滚动.
//@参数 bVertical 是否锁定垂直滚动.
//@别名 元素_置锁定滚动()
XC_API void WINAPI XEle_SetLockScroll(HELE hEle, BOOL bHorizon, BOOL bVertical);
//@备注 设置文本颜色.  
//@参数 hEle 元素句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 元素_置文本颜色()
XC_API void WINAPI XEle_SetTextColor(HELE hEle, COLORREF color);
//@备注 获取文本颜色.  
//@参数 hEle 元素句柄.
//@返回 文本颜色值.
//@别名 元素_取文本颜色()
XC_API COLORREF WINAPI XEle_GetTextColor(HELE hEle);
//@备注 获取文本颜色,优先从资源中获取.  
//@参数 hEle 元素句柄.
//@返回 文本颜色值.
//@别名 元素_取文本颜色扩展()
XC_API COLORREF WINAPI XEle_GetTextColorEx(HELE hEle);
//@备注 设置焦点边框颜色.  
//@参数 hEle 元素句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 元素_置焦点边框颜色()
XC_API void WINAPI XEle_SetFocusBorderColor(HELE hEle, COLORREF color);
//@备注 获取焦点边框颜色.  
//@参数 hEle 元素句柄.
//@返回 返回颜色值
//@别名 元素_取焦点边框颜色()
XC_API COLORREF WINAPI XEle_GetFocusBorderColor(HELE hEle);
//@备注 设置元素字体.  
//@参数 hEle 元素句柄.
//@参数 hFontx 炫彩字体.
//@别名 元素_置字体()
XC_API void WINAPI XEle_SetFont(HELE hEle, HFONTX hFontx);
//@备注 获取元素字体.  
//@参数 hEle 元素句柄.
//@返回 返回炫彩字体句柄.
//@别名 元素_取字体()
XC_API HFONTX WINAPI XEle_GetFont(HELE hEle);
//@备注 获取元素字体,优先从资源中获取.  
//@参数 hEle 元素句柄.
//@返回 返回炫彩字体句柄.
//@别名 元素_取字体扩展()
XC_API HFONTX WINAPI XEle_GetFontEx(HELE hEle);
//@备注 设置元素透明度.  
//@参数 hEle 元素句柄.
//@参数 alpha 透明度.
//@别名 元素_置透明度()
XC_API void WINAPI XEle_SetAlpha(HELE hEle, BYTE alpha);
//@参数 hEle 元素句柄
//@返回 返回透明度
//@别名 元素_取透明度()
XC_API BYTE WINAPI XEle_GetAlpha(HELE hEle);
//@备注 销毁元素.  
//@参数 hEle 元素句柄.
//@别名 元素_销毁()
XC_API void WINAPI XEle_Destroy(HELE hEle);
//@备注 添加背景内容边框.  
//@参数 hEle 元素句柄.
//@参数 nState 组合状态 参加文档: API接口->组合状态
//@参数 color 颜色.
//@参数 width 线宽.
//@别名 元素_添加背景边框()
XC_API void WINAPI XEle_AddBkBorder(HELE hEle, int nState, COLORREF color, int width);
//@备注 添加背景内容填充.  
//@参数 hEle 元素句柄.
//@参数 nState 组合状态 参加文档: API接口->组合状态
//@参数 color 颜色.
//@别名 元素_添加背景填充()
XC_API void WINAPI XEle_AddBkFill(HELE hEle, int nState, COLORREF color);
//@备注 添加背景内容图片.  
//@参数 hEle 元素句柄.
//@参数 nState 组合状态 参加文档: API接口->组合状态
//@参数 hImage 图片句柄.
//@别名 元素_添加背景图片()
XC_API void WINAPI XEle_AddBkImage(HELE hEle, int nState, HIMAGE hImage);
//@备注 设置背景内容
//@参数 hEle 元素句柄
//@参数 pText 背景内容字符串
//@返回 返回设置的背景对象数量
//@别名 元素_置背景()
XC_API int WINAPI XEle_SetBkInfo(HELE hEle, const wchar_t* pText);
//@备注 获取背景内容数量.  
//@参数 hEle 元素句柄.
//@返回 返回背景内容数量.
//@别名 元素_取背景对象数量()
XC_API int WINAPI XEle_GetBkInfoCount(HELE hEle);
//@备注 清空背景内容; 如果背景没有内容,将使用系统默认内容,以便保证背景正确.  
//@参数 hEle 元素句柄.
//@别名 元素_清空背景对象()
XC_API void WINAPI XEle_ClearBkInfo(HELE hEle);
//@备注 获取元素背景管理器.  
//@参数 hEle 元素句柄.
//@返回 背景管理器.
//@别名 元素_取背景管理器()
XC_API HBKM WINAPI XEle_GetBkManager(HELE hEle);
//@备注 获取元素背景管理器,优先从资源中获取.  
//@参数 hEle 元素句柄.
//@返回 背景管理器.
//@别名 元素_取背景管理器扩展()
XC_API HBKM WINAPI XEle_GetBkManagerEx(HELE hEle);
//@参数 hEle 元素句柄
//@参数 hBkInfoM 背景管理器
//@别名 元素_置背景管理器()
XC_API void WINAPI XEle_SetBkManager(HELE hEle, HBKM hBkInfoM);
//@备注 获取组合状态.  
//@参数 hEle 元素句柄.
//@返回 返回元素组合状态  @ref element_state_flag_
//@别名 元素_取状态()
XC_API int WINAPI XEle_GetStateFlags(HELE hEle);
//@备注 绘制元素焦点.  
//@参数 hEle 元素句柄.
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 区域坐标.
//@返回 绘制成功返回TRUE,如果不需要绘制焦点返回FALSE.
//@别名 元素_绘制焦点()
XC_API BOOL WINAPI XEle_DrawFocus(HELE hEle, HDRAW hDraw, RECT* pRect);
//@备注 在自绘事件函数中,用户手动调用绘制元素,以便控制绘制顺序.  
//@参数 hEle 元素句柄.
//@参数 hDraw 图形绘制句柄.
//@别名 元素_绘制()
XC_API void WINAPI XEle_DrawEle(HELE hEle, HDRAW hDraw);
//@备注 设置用户数据.  
//@参数 hEle 元素句柄.
//@参数 nData 用户数据.
//@别名 元素_置用户数据()
XC_API void WINAPI XEle_SetUserData(HELE hEle, vint nData);
//@备注 获取用户数据.  
//@参数 hEle 元素句柄.
//@返回 用户数据.
//@别名 元素_取用户数据()
XC_API vint WINAPI XEle_GetUserData(HELE hEle);
//@备注 获取内容大小.  
//@参数 hEle 元素句柄.
//@参数 bHorizon 水平或垂直,  布局属性交换依赖
//@参数 cx 宽度
//@参数 cy 高度
//@参数 pSize 返回大小.
//@别名 元素_取内容大小()
XC_API void WINAPI XEle_GetContentSize(HELE hEle, BOOL bHorizon, int cx, int cy, SIZE* pSize);
//@备注 设置鼠标捕获.  
//@参数 hEle 元素句柄.
//@参数 b TRUE设置,FALSE取消.
//@别名 元素_置鼠标捕获()
XC_API void WINAPI XEle_SetCapture(HELE hEle, BOOL b);
//@备注 启用或关闭元素透明通道,如果启用,将强制设置元素背景不透明,默认为启用,此功能是为了兼容GDI不支持透明通道问题.  
//@参数 hEle 元素句柄.
//@参数 bEnable 启用或关闭.
//@别名 元素_启用透明通道()
XC_API void WINAPI XEle_EnableTransparentChannel(HELE hEle, BOOL bEnable);
//@备注 设置元素定时器.  
//@参数 hEle 元素句柄.
//@参数 nIDEvent 事件ID.
//@参数 uElapse 延时毫秒.
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 元素_置炫彩定时器()
XC_API BOOL WINAPI XEle_SetXCTimer(HELE hEle, UINT nIDEvent, UINT uElapse);
//@备注 关闭元素定时器.  
//@参数 hEle 元素句柄.
//@参数 nIDEvent 事件ID.
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 元素_关闭炫彩定时器()
XC_API BOOL WINAPI XEle_KillXCTimer(HELE hEle, UINT nIDEvent);
//@备注 设置工具提示内容.  
//@参数 hEle 元素句柄.
//@参数 pText 工具提示内容.
//@别名 元素_置工具提示()
XC_API void WINAPI XEle_SetToolTip(HELE hEle, const wchar_t* pText);
//@备注 设置工具提示内容.  
//@参数 hEle 元素句柄.
//@参数 pText 工具提示内容.
//@参数 nTextAlign 文本对齐方式 @ref textFormatFlag_
//@别名 元素_置工具提示扩展()
XC_API void WINAPI XEle_SetToolTipEx(HELE hEle, const wchar_t* pText, int nTextAlign);

//@备注  设置工具提示弹出显示多久消失, 需要先设置工具提示,然后再设置持续时间
//@参数  hEle 元素句柄
//@参数  nDuration 持续时间
//@返回  成功返回TRUE,否则相反
//@别名  元素_置工具提示持续时间()
XC_API BOOL WINAPI XEle_SetToolTipDuration(HELE hEle, int nDuration);

//@备注 获取工具提示内容.  
//@参数 hEle 元素句柄.
//@返回 返回文本内容
//@别名 元素_取工具提示()
XC_API const wchar_t* WINAPI XEle_GetToolTip(HELE hEle);
//@备注 弹出工具提示.  
//@参数 hEle 元素句柄.
//@参数 x X坐标.
//@参数 y Y坐标.
//@别名 元素_弹出工具提示()
XC_API void WINAPI XEle_PopupToolTip(HELE hEle, int x, int y);
//@备注 调整布局.  
//@参数 hEle 元素句柄.
//@参数 nAdjustNo 调整布局流水号
//@别名 元素_调整布局()
XC_API void WINAPI XEle_AdjustLayout(HELE hEle, UINT nAdjustNo=0);
//@备注 调整布局.    
//@参数 hEle 元素句柄.
//@参数 nFlags 调整标识,  adjustLayout_
//@参数 nAdjustNo 调整布局流水号
//@别名 元素_调整布局扩展()
XC_API void WINAPI XEle_AdjustLayoutEx(HELE hEle, int nFlags=adjustLayout_self, UINT nAdjustNo=0);
//@分组}
//@分组{  布局

//@备注 创建布局元素.  
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄.
//@别名 布局_创建()
XC_API HELE WINAPI XLayout_Create(int x, int y, int cx, int cy, HXCGUI hParent);
//@备注 创建布局元素.  
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄.
//@别名 布局_创建扩展()
XC_API HELE WINAPI XLayout_CreateEx(HXCGUI hParent);
//@备注 启用布局功能.  
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用.
//@别名 布局_启用()
XC_API void WINAPI XLayout_EnableLayout(HELE hEle, BOOL bEnable);
//@备注 是否已经启用布局功能  
//@参数 hEle 元素句柄
//@返回 如果启用布局返回TRUE,否则返回FALSE
//@别名 布局_是否启用()
XC_API BOOL WINAPI XLayout_IsEnableLayout(HELE hEle);
//@备注 显示布局边界.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否显示
//@别名 布局_显示布局边界()
XC_API void WINAPI XLayout_ShowLayoutFrame(HELE hEle, BOOL bEnable);
//@备注 获取宽度,不包含内边距大小  
//@参数 hEle 元素句柄
//@返回 返回宽度
//@别名 布局_取内宽度()
XC_API int WINAPI XLayout_GetWidthIn(HELE hEle);
//@备注 获取高度,不包含内边距大小  
//@参数 hEle 元素句柄
//@返回 返回高度
//@别名 布局_取内高度()
XC_API int WINAPI XLayout_GetHeightIn(HELE hEle);
//@分组}
//@分组{  布局框架

//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄
//@别名 布局框架_创建()
XC_API HELE WINAPI XLayoutFrame_Create(int x, int y, int cx, int cy, HXCGUI hParent);
//@参数 hParent 父为窗口句柄或元素句柄
//@返回 元素句柄.
//@别名 布局框架_创建扩展()
XC_API HELE WINAPI XLayoutFrame_CreateEx(HXCGUI hParent);
//@备注 启用布局功能
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 布局框架_启用()
XC_API void WINAPI XLayoutFrame_EnableLayout(HELE hEle, BOOL bEnable);
//@备注 是否已经启用布局功能
//@参数 hEle 元素句柄
//@返回 如果启用布局返回TRUE,否则返回FALSE
//@别名 布局框架_是否启用()
XC_API BOOL WINAPI XLayoutFrame_IsEnableLayout(HELE hEle);
//@参数 hEle 元素句柄
//@参数 bEnable 是否显示
//@别名 布局框架_显示布局边界()
XC_API void WINAPI XLayoutFrame_ShowLayoutFrame(HELE hEle, BOOL bEnable);
//@备注 获取宽度,不包含内边距大小
//@参数 hEle 元素句柄
//@返回 返回宽度
//@别名 布局框架_取内宽度()
XC_API int WINAPI XLayoutFrame_GetWidthIn(HELE hEle);
//@备注 获取高度,不包含内边距大小
//@参数 hEle 元素句柄
//@返回 返回高度
//@别名 布局框架_取内高度()
XC_API int WINAPI XLayoutFrame_GetHeightIn(HELE hEle);
//@分组}
//@分组{  滚动视图

//@备注 创建滚动视图元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 滚动视_创建()
XC_API HELE WINAPI XSView_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 设置内容大小.  
//@参数 hEle 元素句柄.
//@参数 cx 宽度, 此宽度不包含边框和内填充
//@参数 cy 高度, 此高度不包含边框和内填充
//@返回 如果内容改变返回TRUE否则返回FALSE.
//@别名 滚动视_置视图大小()
XC_API BOOL WINAPI XSView_SetTotalSize(HELE hEle, int cx, int cy);
//@备注 获取内容总大小.  
//@参数 hEle 元素句柄.
//@参数 pSize 大小, 不包含边框和内填充
//@别名 滚动视_取视图大小()
XC_API void WINAPI XSView_GetTotalSize(HELE hEle, SIZE* pSize);
//@备注 设置滚动单位大小.  
//@参数 hEle 元素句柄.
//@参数 nWidth 宽度.
//@参数 nHeight 高度.
//@返回 如果内容改变返回TRUE否则返回FALSE.
//@别名 滚动视_置滚动单位大小()
XC_API BOOL WINAPI XSView_SetLineSize(HELE hEle, int nWidth, int nHeight);
//@备注 获取滚动单位大小.  
//@参数 hEle 元素句柄.
//@参数 pSize 返回大小.
//@别名 滚动视_取滚动单位大小()
XC_API void WINAPI XSView_GetLineSize(HELE hEle, SIZE* pSize);
//@备注 设置滚动条大小.  
//@参数 hEle 元素句柄.
//@参数 size 滚动条大小.
//@别名 滚动视_置滚动条大小()
XC_API void WINAPI XSView_SetScrollBarSize(HELE hEle, int size);
//@备注 获取视口原点X坐标.  
//@参数 hEle 元素句柄.
//@返回 视口原点X坐标.
//@别名 滚动视_取视口原点X()
XC_API int WINAPI XSView_GetViewPosH(HELE hEle);
//@备注 获取视口原点Y坐标.  
//@参数 hEle 元素句柄.
//@返回 视口原点Y坐标.
//@别名 滚动视_取视口原点Y()
XC_API int WINAPI XSView_GetViewPosV(HELE hEle);
//@备注 获取视口宽度.  
//@参数 hEle 元素句柄.
//@返回 返回视口宽度.
//@别名 滚动视_取视口宽度()
XC_API int WINAPI XSView_GetViewWidth(HELE hEle);
//@备注 获取视口高度.  
//@参数 hEle 元素句柄.
//@返回 返回视口高度.
//@别名 滚动视_取视口高度()
XC_API int WINAPI XSView_GetViewHeight(HELE hEle);
//@备注 获取视口坐标.  
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@别名 滚动视_取视口坐标()
XC_API void WINAPI XSView_GetViewRect(HELE hEle, RECT* pRect);
//@备注 获取水平滚动条.  
//@参数 hEle 元素句柄.
//@返回 滚动条句柄.
//@别名 滚动视_取水平滚动条()
XC_API HELE WINAPI XSView_GetScrollBarH(HELE hEle);
//@备注 获取垂直滚动条.  
//@参数 hEle 元素句柄.
//@返回 垂直滚动条句柄.
//@别名 滚动视_取垂直滚动条()
XC_API HELE WINAPI XSView_GetScrollBarV(HELE hEle);
//@备注 水平滚动条,滚动到指定位置点.  
//@参数 hEle 元素句柄.
//@参数 pos 位置点.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动视_水平滚动()
XC_API BOOL WINAPI XSView_ScrollPosH(HELE hEle, int pos);
//@备注 垂直滚动条,滚动到指定位置点.  
//@参数 hEle 元素句柄.
//@参数 pos 位置点.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动视_垂直滚动()
XC_API BOOL WINAPI XSView_ScrollPosV(HELE hEle, int pos);
//@备注 水平滚动条,滚动到指定坐标.  
//@参数 hEle 元素句柄.
//@参数 posX X坐标.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动视_水平滚动到X()
XC_API BOOL WINAPI XSView_ScrollPosXH(HELE hEle, int posX);
//@备注 垂直滚动条,滚动到指定坐标.  
//@参数 hEle 元素句柄.
//@参数 posY Y坐标.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动视_垂直滚动到Y()
XC_API BOOL WINAPI XSView_ScrollPosYV(HELE hEle, int posY);
//@备注 显示水平滚动条.  
//@参数 hEle 元素句柄.
//@参数 bShow 是否显示.
//@别名 滚动视_显示水平滚动条()
XC_API void WINAPI XSView_ShowSBarH(HELE hEle, BOOL bShow);
//@备注 显示垂直滚动条.  
//@参数 hEle 元素句柄.
//@参数 bShow 是否显示.
//@别名 滚动视_显示垂直滚动条()
XC_API void WINAPI XSView_ShowSBarV(HELE hEle, BOOL bShow);
//@备注 启用自动显示滚动条.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 滚动视_启用自动显示滚动条()
XC_API void WINAPI XSView_EnableAutoShowScrollBar(HELE hEle, BOOL bEnable);
//@备注 向左滚动.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 滚动视_向左滚动()
XC_API BOOL WINAPI XSView_ScrollLeftLine(HELE hEle);
//@备注 向右滚动.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 滚动视_向右滚动()
XC_API BOOL WINAPI XSView_ScrollRightLine(HELE hEle);
//@备注 向上滚动.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 滚动视_向上滚动()
XC_API BOOL WINAPI XSView_ScrollTopLine(HELE hEle);
//@备注 向下滚动.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 滚动视_向下滚动()
XC_API BOOL WINAPI XSView_ScrollBottomLine(HELE hEle);
//@备注 水平滚动到左侧.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 滚动视_滚动到左侧()
XC_API BOOL WINAPI XSView_ScrollLeft(HELE hEle);
//@备注 水平滚动到右侧.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 滚动视_滚动到右侧()
XC_API BOOL WINAPI XSView_ScrollRight(HELE hEle);
//@备注 垂直滚动到顶部.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 滚动视_滚动到顶部()
XC_API BOOL WINAPI XSView_ScrollTop(HELE hEle);
//@备注 垂直滚动到底部.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 滚动视_滚动到底部()
XC_API BOOL WINAPI XSView_ScrollBottom(HELE hEle);
//@分组}
//@分组{  按钮

//@备注 创建按钮元素  
//@参数 x 按钮x坐标
//@参数 y 按钮y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 pName 标题
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 按钮元素句柄.
//@别名 按钮_创建()
XC_API HELE WINAPI XBtn_Create(int x, int y, int cx, int cy, const wchar_t* pName, HXCGUI hParent=NULL);
//@备注 对当前文本内容处理, 将&符号后面的一个字符加上下划线
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 按钮_启用热键前缀()
XC_API void WINAPI XBtn_EnableHotkeyPrefix(HELE hEle, BOOL bEnable);
//@备注 是否选中状态.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 按钮_是否选中()
XC_API BOOL WINAPI XBtn_IsCheck(HELE hEle);
//@备注 设置选中状态.  
//@参数 hEle 元素句柄.
//@参数 bCheck 是否选中.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 按钮_置选中()
XC_API BOOL WINAPI XBtn_SetCheck(HELE hEle, BOOL bCheck);
//@备注 设置按钮状态.  
//@参数 hEle 元素句柄.
//@参数 nState 按钮状态见宏定义.
//@别名 按钮_置状态()
XC_API void WINAPI XBtn_SetState(HELE hEle, common_state3_ nState);
//@备注 获取按钮状态
//@参数 hEle 元素句柄.
//@返回 返回按钮状态.
//@别名 按钮_取三态()
XC_API common_state3_ WINAPI XBtn_GetState(HELE hEle);
//@备注 获取按钮状态
//@参数 hEle 元素句柄.
//@返回 返回按钮状态.
//@别名 按钮_取按钮状态()
XC_API button_state_ WINAPI XBtn_GetStateEx(HELE hEle);
//@备注 设置按钮类型并自动修改样式和文本对齐方式.  
//@参数 hEle 元素句柄.
//@参数 nType 按钮类型,参见宏定义.
//@别名 按钮_置类型扩展()
XC_API void WINAPI XBtn_SetTypeEx(HELE hEle, XC_OBJECT_TYPE_EX nType);
//@备注 设置组ID.  
//@参数 hEle 元素句柄.
//@参数 nID 组ID.
//@别名 按钮_置组ID()
XC_API void WINAPI XBtn_SetGroupID(HELE hEle, int nID);
//@备注 获取组ID.  
//@参数 hEle 元素句柄.
//@返回 组ID.
//@别名 按钮_取组ID()
XC_API int WINAPI XBtn_GetGroupID(HELE hEle);
//@备注 设置绑定元素.  
//@参数 hEle 元素句柄.
//@参数 hBindEle 将要绑定的元素.
//@别名 按钮_置绑定元素()
XC_API void WINAPI XBtn_SetBindEle(HELE hEle, HELE hBindEle);
//@备注 获取绑定的元素.  
//@参数 hEle 元素句柄.
//@返回 绑定的元素句柄.
//@别名 按钮_取绑定元素()
XC_API HELE WINAPI XBtn_GetBindEle(HELE hEle);
//@备注 设置文本对齐方式.  
//@参数 hEle 元素句柄.
//@参数 nFlags 对齐方式  @ref textFormatFlag_ .
//@别名 按钮_置文本对齐()
XC_API void WINAPI XBtn_SetTextAlign(HELE hEle, int nFlags);
//@备注 获取文本对齐方式.  
//@参数 hEle 元素句柄.
//@返回 文本对齐方式 @ref textFormatFlag_ .
//@别名 按钮_取文本对齐()
XC_API int WINAPI XBtn_GetTextAlign(HELE hEle);
//@备注 设置图标对齐.  
//@参数 hEle 元素句柄.
//@参数 align 对齐方式.
//@别名 按钮_置图标对齐()
XC_API void WINAPI XBtn_SetIconAlign(HELE hEle, button_icon_align_ align);
//@备注 设置按钮文本坐标偏移量.  
//@参数 hEle 元素句柄.
//@参数 x 偏移量.
//@参数 y 偏移量.
//@别名 按钮_置偏移()
XC_API void WINAPI XBtn_SetOffset(HELE hEle, int x, int y);
//@备注 设置按钮图标坐标偏移量.  
//@参数 hEle 元素句柄.
//@参数 x 偏移量.
//@参数 y 偏移量.
//@别名 按钮_置图标偏移()
XC_API void WINAPI XBtn_SetOffsetIcon(HELE hEle, int x, int y);
//@备注 设置图标与文本间隔大小.  
//@参数 hEle 元素句柄.
//@参数 size 间隔大小.
//@别名 按钮_置图标间隔()
XC_API void WINAPI XBtn_SetIconSpace(HELE hEle, int size);
//@备注 设置文本内容.  
//@参数 hEle 元素句柄.
//@参数 pName 文本内容.
//@别名 按钮_置文本()
XC_API void WINAPI XBtn_SetText(HELE hEle, const wchar_t* pName);
//@备注 获取文本内容.  
//@参数 hEle 元素句柄.
//@返回 返回文本
//@别名 按钮_取文本()
XC_API const wchar_t* WINAPI XBtn_GetText(HELE hEle);
//@备注 设置图标.  
//@参数 hEle 元素句柄.
//@参数 hImage 图片句柄.
//@别名 按钮_置图标()
XC_API void WINAPI XBtn_SetIcon(HELE hEle, HIMAGE hImage);
//@备注 设置图标禁用状态.  
//@参数 hEle 元素句柄.
//@参数 hImage 图片句柄.
//@别名 按钮_置禁用图标()
XC_API void WINAPI XBtn_SetIconDisable(HELE hEle, HIMAGE hImage);
//@备注 获取用户设置的图标.  
//@参数 hEle 元素句柄.
//@参数 nType 图标类型, 0:默认图标,1:禁用状态图标.
//@返回 返回图标句柄.
//@别名 按钮_取图标()
XC_API HIMAGE WINAPI XBtn_GetIcon(HELE hEle, int nType);
//@备注 添加动画帧.  
//@参数 hEle 元素句柄.
//@参数 hImage 图片句柄
//@参数 uElapse 图片帧延时,单位毫秒.
//@别名 按钮_添加动画帧()
XC_API void WINAPI XBtn_AddAnimationFrame(HELE hEle, HIMAGE hImage, UINT uElapse);
//@备注 开始或关闭图片动画的播放.  
//@参数 hEle 元素句柄.
//@参数 bEnable 开始播放动画TRUE,关闭播放动画FALSE.
//@参数 bLoopPlay 是否循环播放.
//@别名 按钮_启用动画()
XC_API void WINAPI XBtn_EnableAnimation(HELE hEle, BOOL bEnable, BOOL bLoopPlay=FALSE);
//@参数 hEle 元素句柄
//@别名 按钮_清空动画()
XC_API void WINAPI XBtn_ClearAnimation(HELE hEle);
//@分组}
//@分组{  编辑框

//@备注 创建
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄
//@返回 元素句柄
//@别名 编辑框_创建()
XC_API HELE WINAPI XEdit_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 创建
//@参数 x 元素x坐标
//@参数 y 元素y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 type 类型
//@参数 hParent 父为窗口句柄或元素句柄
//@返回 元素句柄
//@别名 编辑框_创建扩展()
XC_API HELE WINAPI XEdit_CreateEx(int x, int y, int cx, int cy, edit_type_ type, HXCGUI hParent=NULL);
//@备注 启用自动换行  
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 编辑框_启用自动换行()
XC_API void WINAPI XEdit_EnableAutoWrap(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 编辑框_启用只读()
XC_API void WINAPI XEdit_EnableReadOnly(HELE hEle, BOOL bEnable);
//@参数 hEle 
//@参数 bEnable 
//@别名 编辑框_启用多行()
XC_API void WINAPI XEdit_EnableMultiLine(HELE hEle, BOOL bEnable);

//@备注  自动为URL加上下划线
//@参数  hEle 元素句柄
//@参数  bEnable 是否启用
//@别名  编辑框_启用URL下划线()
XC_API void WINAPI XEdit_EnableUrlUnderline(HELE hEle, BOOL bEnable);

//@备注 启用密码模式(只支持默认类型编辑框
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 编辑框_启用密码()
XC_API void WINAPI XEdit_EnablePassword(HELE hEle, BOOL bEnable);
//@备注 当获得焦点时,自动选择所有内容
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 编辑框_启用自动选择()
XC_API void WINAPI XEdit_EnableAutoSelAll(HELE hEle, BOOL bEnable);
//@备注 当失去焦点时自动取消选择
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 编辑框_启用自动取消选择()
XC_API void WINAPI XEdit_EnableAutoCancelSel(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_是否只读()
XC_API BOOL WINAPI XEdit_IsReadOnly(HELE hEle);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_是否多行()
XC_API BOOL WINAPI XEdit_IsMultiLine(HELE hEle);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_是否密码()
XC_API BOOL WINAPI XEdit_IsPassword(HELE hEle);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_是否自动换行()
XC_API BOOL WINAPI XEdit_IsAutoWrap(HELE hEle);
//@参数 hEle 元素句柄
//@返回 如果为空返回TRUE否则返回FALSE.
//@别名 编辑框_是否为空()
XC_API BOOL WINAPI XEdit_IsEmpty(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@返回 如果在选择区域内返回TRUE,否则返回FALSE
//@别名 编辑框_是否在选择区域()
XC_API BOOL WINAPI XEdit_IsInSelect(HELE hEle, int iRow, int iCol);
//@参数 hEle 元素句柄
//@返回 返回总行数
//@别名 编辑框_取总行数()
XC_API int WINAPI XEdit_GetRowCount(HELE hEle);
//@备注 包含自动换行数量
//@参数 hEle 元素句柄
//@返回 返回总行数
//@别名 编辑框_取总行数扩展()
XC_API int WINAPI XEdit_GetRowCountEx(HELE hEle);
//@备注 包含文本或非文本内容
//@参数 hEle 元素句柄
//@返回 返回数据结构
//@别名 编辑框_取数据()
XC_API edit_data_copy_* WINAPI XEdit_GetData(HELE hEle);
//@参数 hEle 元素句柄
//@参数 pData 数据结构
//@参数 styleTable 样式表
//@参数 nStyleCount 样式数量
//@别名 编辑框_添加数据()
XC_API void WINAPI XEdit_AddData(HELE hEle, const edit_data_copy_* pData, USHORT* styleTable, int nStyleCount);
//@参数 pData 数据结构
//@别名 编辑框_释放数据()
XC_API void WINAPI XEdit_FreeData(edit_data_copy_* pData);
//@备注 当内容为空时,显示默认文本
//@参数 hEle 元素句柄
//@参数 pString 文本内容
//@别名 编辑框_置默认文本()
XC_API void WINAPI XEdit_SetDefaultText(HELE hEle, const wchar_t* pString);
//@参数 hEle 元素句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 编辑框_置默认文本颜色()
XC_API void WINAPI XEdit_SetDefaultTextColor(HELE hEle, COLORREF color);
//@参数 hEle 元素句柄
//@参数 ch 字符
//@别名 编辑框_置密码字符()
XC_API void WINAPI XEdit_SetPasswordCharacter(HELE hEle, wchar_t ch);
//@备注 单行模式下有效
//@参数 hEle 元素句柄
//@参数 align 对齐方式 @ref edit_textAlign_flag_
//@别名 编辑框_置文本对齐()
XC_API void WINAPI XEdit_SetTextAlign(HELE hEle, int align);
//@备注 置中文字体;  如果已设置, 当遇到中文字符时使用后备字体, 解决不支持中文的字体问题
//@参数 hEle 元素句柄
//@参数 hFont 字体
//@别名 编辑框_置后备字体()
XC_API void WINAPI XEdit_SetBackFont(HELE hEle, HFONTX hFont);
//@备注 TAB占空格数量,  TAB显示大小 = 空格大小 * TAB占空格数量
//@参数 hEle 元素句柄
//@参数 nSpace 空格数量
//@别名 编辑框_置TAB空格()
XC_API void WINAPI XEdit_SetTabSpace(HELE hEle, int nSpace);
//@备注 TAB显示大小 = 空格大小 * TAB占空格数量
//@参数 hEle 元素句柄
//@参数 size 空格大小
//@别名 编辑框_置空格大小()
XC_API void WINAPI XEdit_SetSpaceSize(HELE hEle, int size);
//@参数 hEle 元素句柄
//@参数 size 英文字符间距大小
//@参数 sizeZh 中文字符间距大小
//@别名 编辑框_置字符间距()
XC_API void WINAPI XEdit_SetCharSpaceSize(HELE hEle, int size, int sizeZh);
//@参数 hEle 元素句柄
//@参数 pString 字符串
//@别名 编辑框_置文本()
XC_API void WINAPI XEdit_SetText(HELE hEle, const wchar_t* pString);
//@参数 hEle 元素句柄
//@参数 nValue 整数值
//@别名 编辑框_置文本整数()
XC_API void WINAPI XEdit_SetTextInt(HELE hEle, int nValue);
//@备注 不包含非文本内容
//@参数 hEle 元素句柄
//@参数 pOut 接收文本内存指针
//@参数 nOutlen 内存大小, 字符为单位
//@返回 返回实际接收文本长度
//@别名 编辑框_取文本()
XC_API int WINAPI XEdit_GetText(HELE hEle, wchar_t* pOut, int nOutlen);
//@备注 不包含非文本内容
//@参数 hEle 元素句柄
//@返回 返回临时文本指针, 临时缓存区大小 @ref TEXT_BUFFER_SIZE
//@别名 编辑框_取文本_临时()
XC_API const wchar_t* WINAPI XEdit_GetText_Temp(HELE hEle);
//@备注 获取指定行文本内容
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 pOut 接收文本内存指针
//@参数 nOutlen 接收文本内存块长度,字符为单位
//@返回 返回实际接收文本长度
//@别名 编辑框_取文本行()
XC_API int WINAPI XEdit_GetTextRow(HELE hEle, int iRow, wchar_t* pOut, int nOutlen);
//@备注 包含非文本内容
//@参数 hEle 元素句柄
//@返回 返回内容长度
//@别名 编辑框_取内容长度()
XC_API int WINAPI XEdit_GetLength(HELE hEle);
//@备注 包含非文本内容
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 返回内容长度
//@别名 编辑框_取内容长度行()
XC_API int WINAPI XEdit_GetLengthRow(HELE hEle, int iRow);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@返回 返回指定位置字符
//@别名 编辑框_取字符()
XC_API wchar_t WINAPI XEdit_GetAt(HELE hEle, int iRow, int iCol);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 pString 字符串
//@别名 编辑框_插入文本()
XC_API void WINAPI XEdit_InsertText(HELE hEle, int iRow, int iCol, const wchar_t* pString);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 pString 字符串
//@参数 iStyle 样式
//@别名 编辑框_插入文本扩展()
XC_API void WINAPI XEdit_InsertTextEx(HELE hEle, int iRow, int iCol, const wchar_t* pString, int iStyle);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 hObj 对象句柄
//@别名 编辑框_插入对象()
XC_API void WINAPI XEdit_InsertObject(HELE hEle, int iRow, int iCol, HXCGUI hObj);
//@备注 在当前插入位置添加文本, 所以不一定就添加在末尾, 可以通过 XEdit_MoveEnd() 移动插入位置到末尾
//@参数 hEle 元素句柄
//@参数 pString 字符串
//@别名 编辑框_添加文本()
XC_API void WINAPI XEdit_AddText(HELE hEle, const wchar_t* pString);
//@备注 自动刷新UI, 支持撤销/恢复; 从当前插入符位置操作, 所以不一定就在末尾, 可以通过 XEdit_MoveEnd() 移动插入位置到末尾
//@参数 hEle 元素句柄
//@参数 pString 字符串
//@别名 编辑框_添加文本模拟用户操作()
XC_API void WINAPI XEdit_AddTextUser(HELE hEle, const wchar_t* pString);
//@备注 在当前插入位置添加文本, 所以不一定就添加在末尾, 可以通过 XEdit_MoveEnd() 移动插入位置到末尾
//@参数 hEle 元素句柄
//@参数 pString 字符串
//@参数 iStyle 样式索引
//@别名 编辑框_添加文本扩展()
XC_API void WINAPI XEdit_AddTextEx(HELE hEle, const wchar_t* pString, int iStyle);
//@备注 例如: 字体, 图片, UI对象
//@参数 hEle 元素句柄
//@参数 hObj 对象句柄
//@返回 返回样式索引
//@别名 编辑框_添加对象()
XC_API int WINAPI XEdit_AddObject(HELE hEle, HXCGUI hObj);
//@备注 当样式为图片时有效
//@参数 hEle 元素句柄
//@参数 iStyle 样式索引
//@别名 编辑框_添加对象从样式()
XC_API void WINAPI XEdit_AddByStyle(HELE hEle, int iStyle);
//@参数 hEle 元素句柄
//@参数 hFont_image_Obj 字体,图片或UI对象
//@参数 color 颜色
//@参数 bColor 是否使用颜色
//@返回 返回样式索引
//@别名 编辑框_添加样式()
XC_API int WINAPI XEdit_AddStyle(HELE hEle, HXCGUI hFont_image_Obj, COLORREF color, BOOL bColor);
//@参数 hEle 元素句柄
//@参数 fontName 字体名称
//@参数 fontSize 字体大小
//@参数 fontStyle 字体样式 @ref fontStyle_  此接口支持指定 下划线, 删除线, 因为内部做了处理
//@参数 color 颜色
//@参数 bColor 是否使用颜色
//@返回 返回样式索引
//@别名 编辑框_添加样式扩展()
XC_API int WINAPI XEdit_AddStyleEx(HELE hEle, const wchar_t* fontName, int fontSize, int fontStyle, COLORREF color, BOOL bColor);
//@参数 hEle 元素句柄
//@参数 iStyle 样式索引
//@参数 hFont 字体句柄
//@参数 color 颜色
//@参数 bColor 是否使用颜色
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 编辑框_修改样式()
XC_API BOOL WINAPI XEdit_ModifyStyle(HELE hEle, int iStyle, HFONTX hFont, COLORREF color, BOOL bColor);
//@参数 hEle 元素句柄
//@参数 iStyle 样式
//@返回 如果成功返回TRUE,否则返回FALSE 
//@别名 编辑框_释放样式()
XC_API BOOL WINAPI XEdit_ReleaseStyle(HELE hEle, int iStyle);
//@参数 hEle 元素句柄
//@参数 iStyle 样式索引
//@参数 info 返回样式信息
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_取样式信息()
XC_API BOOL WINAPI XEdit_GetStyleInfo(HELE hEle, int iStyle, edit_style_info_* info);
//@参数 hEle 元素句柄
//@参数 iStyle 样式索引
//@别名 编辑框_置当前样式()
XC_API void WINAPI XEdit_SetCurStyle(HELE hEle, int iStyle);
//@参数 hEle 元素句柄
//@参数 iStyle 样式索引
//@别名 编辑框_置选择文本样式()
XC_API void WINAPI XEdit_SetSelectTextStyle(HELE hEle, int iStyle);
//@参数 hEle 元素句柄
//@参数 color 颜色
//@别名 编辑框_置插入符颜色()
XC_API void WINAPI XEdit_SetCaretColor(HELE hEle, COLORREF color);
//@参数 hEle 元素句柄
//@参数 nWidth 宽度
//@别名 编辑框_置插入符宽度()
XC_API void WINAPI XEdit_SetCaretWidth(HELE hEle, int nWidth);
//@参数 hEle 元素句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 编辑框_置选择背景颜色()
XC_API void WINAPI XEdit_SetSelectBkColor(HELE hEle, COLORREF color);
//@参数 hEle 元素句柄
//@参数 nHeight 行高
//@别名 编辑框_置默认行高()
XC_API void WINAPI XEdit_SetRowHeight(HELE hEle, int nHeight);
//@备注 当类型为 edit_type_richedit 支持指定不同行高
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 nHeight 高度
//@别名 编辑框_置指定行高度()
XC_API void WINAPI XEdit_SetRowHeightEx(HELE hEle, int iRow, int nHeight);
//@备注 设置行间隔大小,多行模式有效
//@参数 hEle 元素句柄
//@参数 nSpace 行间隔大小
//@别名 编辑框_置行间隔()
XC_API void WINAPI XEdit_SetRowSpace(HELE hEle, int nSpace);
//@参数 hEle 元素句柄
//@参数 pos 位置
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 编辑框_置当前位置()
XC_API BOOL WINAPI XEdit_SetCurPos(HELE hEle, int pos);
//@参数 hEle 元素句柄
//@返回 返回位置
//@别名 编辑框_取当前位置()
XC_API int WINAPI XEdit_GetCurPos(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@别名 编辑框_置当前位置扩展()
XC_API void WINAPI XEdit_SetCurPosEx(HELE hEle, int iRow, int iCol);
//@参数 hEle 元素句柄
//@参数 iRow 返回行索引
//@参数 iCol 返回列索引
//@别名 编辑框_取当前位置扩展()
XC_API void WINAPI XEdit_GetCurPosEx(HELE hEle, int* iRow, int* iCol);
//@参数 hEle 元素句柄
//@返回 返回行索引
//@别名 编辑框_取当前行()
XC_API int WINAPI XEdit_GetCurRow(HELE hEle);
//@参数 hEle 元素句柄
//@返回 返回列索引
//@别名 编辑框_取当前列()
XC_API int WINAPI XEdit_GetCurCol(HELE hEle);
//@备注 讲插入符移到到末尾
//@参数 hEle 元素句柄
//@别名 编辑框_移动到末尾()
XC_API void WINAPI XEdit_MoveEnd(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 pOut 接收返回坐标点
//@别名 编辑框_取坐标点()
XC_API void WINAPI XEdit_GetPoint(HELE hEle, int iRow, int iCol, POINT* pOut);
//@备注 视图自动滚动到当前插入符位置
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_自动滚动()
XC_API BOOL WINAPI XEdit_AutoScroll(HELE hEle);
//@备注 视图自动滚动到指定位置
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_自动滚动扩展()
XC_API BOOL WINAPI XEdit_AutoScrollEx(HELE hEle, int iRow, int iCol);
//@备注 转换位置点到行列
//@参数 hEle 元素句柄
//@参数 iPos 位置点
//@参数 pInfo 行列
//@别名 编辑框_位置到行列()
XC_API void WINAPI XEdit_PosToRowCol(HELE hEle, int iPos, position_* pInfo);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@返回 位置点
//@别名 编辑框_行列到位置()
XC_API int WINAPI XEdit_RowColToPos(HELE hEle, int iRow, int iCol);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_选择全部()
XC_API BOOL WINAPI XEdit_SelectAll(HELE hEle);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_取消选择()
XC_API BOOL WINAPI XEdit_CancelSelect(HELE hEle);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_删除选择内容()
XC_API BOOL WINAPI XEdit_DeleteSelect(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iStartRow 起始行索引
//@参数 iStartCol 起始行列索引
//@参数 iEndRow 结束行索引
//@参数 iEndCol 结束行列索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_置选择()
XC_API BOOL WINAPI XEdit_SetSelect(HELE hEle, int iStartRow, int iStartCol, int iEndRow, int iEndCol);
//@备注 不包括非文本内容
//@参数 hEle 元素句柄
//@参数 pOut 接收返回文本内容
//@参数 nOutLen 接收内存大小,字符为单位
//@返回 返回接收文本内容实际长度
//@别名 编辑框_取选择文本()
XC_API int WINAPI XEdit_GetSelectText(HELE hEle, wchar_t* pOut, int nOutLen);
//@备注 不包括非文本内容
//@参数 hEle 元素句柄
//@返回 返回文本内容长度
//@别名 编辑框_取选择文本长度()
XC_API int WINAPI XEdit_GetSelectTextLength(HELE hEle);
//@参数 hEle 元素句柄
//@参数 pBegin 起始位置,可为空,当为空时如果有选择内容返回TRUE
//@参数 pEnd 结束位置,可为空,当为空时如果有选择内容返回TRUE
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_取选择内容范围()
XC_API BOOL WINAPI XEdit_GetSelectRange(HELE hEle, position_* pBegin, position_* pEnd);
//@参数 hEle 元素句柄
//@参数 piStart 起始行索引
//@参数 piEnd 结束行索引
//@别名 编辑框_取可视行范围()
XC_API void WINAPI XEdit_GetVisibleRowRange(HELE hEle, int* piStart, int* piEnd);
//@备注 删除指定范围内容; 删除全部请使用 XEdit_SetText(hEdit,L"")
//@参数 hEle 元素句柄
//@参数 iStartRow 起始行索引
//@参数 iStartCol 起始行列索引
//@参数 iEndRow 结束行索引
//@参数 iEndCol 结束行列索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_删除()
XC_API BOOL WINAPI XEdit_Delete(HELE hEle, int iStartRow, int iStartCol, int iEndRow, int iEndCol);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_删除行()
XC_API BOOL WINAPI XEdit_DeleteRow(HELE hEle, int iRow);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_剪贴板剪切()
XC_API BOOL WINAPI XEdit_ClipboardCut(HELE hEle);
//@备注 复制选择内容
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_剪贴板复制选择()
XC_API BOOL WINAPI XEdit_ClipboardCopy(HELE hEle);
//@备注 复制全部内容
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_剪贴板复制()
XC_API BOOL WINAPI XEdit_ClipboardCopyAll(HELE hEle);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_剪贴板粘贴()
XC_API BOOL WINAPI XEdit_ClipboardPaste(HELE hEle);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_撤销()
XC_API BOOL WINAPI XEdit_Undo(HELE hEle);
//@备注 恢复/重做
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 编辑框_恢复()
XC_API BOOL WINAPI XEdit_Redo(HELE hEle);
//@备注 当前行开始; 从当前插入符位置操作, 所以不一定就在末尾, 可以通过 XEdit_MoveEnd() 移动插入位置到末尾
//@参数 hEle 元素句柄
//@参数 hImageAvatar 头像
//@参数 hImageBubble 气泡背景
//@参数 nFlag 标志 @ref chat_flag_
//@别名 编辑框_插入气泡开始()
XC_API void WINAPI XEdit_InsertChatBegin(HELE hEle, HIMAGE hImageAvatar, HIMAGE hImageBubble, int nFlag);
//@备注 当前行开始; 末尾操作
//@参数 hEle 元素句柄
//@参数 hImageAvatar 头像
//@参数 hImageBubble 气泡背景
//@参数 nFlag 标志 @ref chat_flag_
//@别名 编辑框_添加气泡开始()
XC_API void WINAPI XEdit_AddChatBegin(HELE hEle, HIMAGE hImageAvatar, HIMAGE hImageBubble, int nFlag);
//@备注 当前行结束; 从当前插入符位置操作, 所以不一定就在末尾, 可以通过 XEdit_MoveEnd() 移动插入位置到末尾
//@参数 hEle 元素句柄
//@别名 编辑框_添加气泡结束()
XC_API void WINAPI XEdit_AddChatEnd(HELE hEle);
//@备注 设置聊天气泡内容缩进
//@参数 hEle 元素句柄
//@参数 nIndentation 缩进值
//@别名 编辑框_置气泡缩进()
XC_API void WINAPI XEdit_SetChatIndentation(HELE hEle, int nIndentation);
//@备注 当值为0时代表不限制宽度
//@参数 hEle 元素句柄
//@参数 nWidth 最大宽度
//@别名 编辑框_置气泡最大宽度()
XC_API void WINAPI XEdit_SetChatMaxWidth(HELE hEle, int nWidth);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 返回行标识 @ref chat_flag_
//@别名 编辑框_取指定行气泡标识()
XC_API int WINAPI XEdit_GetChatFlags(HELE hEle, int iRow);
//@分组}
//@分组{  组合框

//@备注 创建下拉组合框元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 组合框_创建()
XC_API HELE WINAPI XComboBox_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 设置选择项.  
//@参数 hEle 元素句柄.
//@参数 iIndex 项索引.
//@返回 成功返回否则返回FALSE.
//@别名 组合框_置选择项()
XC_API BOOL WINAPI XComboBox_SetSelItem(HELE hEle, int iIndex);
//@备注 创建数据适配器, 并且从绑定的项模板中扫描字段名, 初始化数据适配器的列(字段名);
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//若是没有设置项模板, 那么使用内置项模板;
//@参数 hEle 元素句柄.
//@返回 返回数据适配器句柄
//@别名 组合框_创建数据适配器()
XC_API HXCGUI WINAPI XComboBox_CreateAdapter(HELE hEle);
//@备注 绑定数据适配器.  
//@参数 hEle 元素句柄.
//@参数 hAdapter 适配器句柄.
//@别名 组合框_绑定数据适配器()
XC_API void WINAPI XComboBox_BindAdapter(HELE hEle, HXCGUI hAdapter);
//@备注 获取绑定的数据适配器.  
//@参数 hEle 元素句柄.
//@返回 返回数据适配器.
//@别名 组合框_取数据适配器()
XC_API HXCGUI WINAPI XComboBox_GetAdapter(HELE hEle);
//@备注 设置绑定数据适配器字段名.  
//@参数 hEle 元素句柄.
//@参数 pName 字段名
//@别名 组合框_置绑定名称()
XC_API void WINAPI XComboBox_SetBindName(HELE hEle, const wchar_t* pName);
//@备注 获取下拉按钮坐标.  
//@参数 hEle 元素句柄.
//@参数 pRect 坐标.
//@别名 组合框_取下拉按钮坐标()
XC_API void WINAPI XComboBox_GetButtonRect(HELE hEle, RECT* pRect);
//@备注 设置下拉按钮大小.  
//@参数 hEle 元素句柄.
//@参数 size 大小.
//@别名 组合框_置下拉按钮大小()
XC_API void WINAPI XComboBox_SetButtonSize(HELE hEle, int size);
//@备注 设置下拉列表允许的最大高度, @ref XComboBox_EnableDropHeightFixed , 
//@参数 hEle 元素句柄.
//@参数 height 高度, -1自动计算高度
//@别名 组合框_置下拉列表高度()
XC_API void WINAPI XComboBox_SetDropHeight(HELE hEle, int height);
//@备注 获取下拉列表高度.  
//@参数 hEle 元素句柄.
//@返回 下拉列表高度.
//@别名 组合框_取下拉列表高度()
XC_API int WINAPI XComboBox_GetDropHeight(HELE hEle);
//@备注 设置下拉列表项模板文件
//@参数 hEle 元素句柄.
//@参数 pXmlFile 项模板文件.
//@别名 组合框_置项模板文件()
XC_API void WINAPI XComboBox_SetItemTemplateXML(HELE hEle, const wchar_t* pXmlFile);
//@备注 设置下拉列表项模板.  
//@参数 hEle 元素句柄.
//@参数 pStringXML 字符串指针.
//@别名 组合框_置项模板从字符串()
XC_API void WINAPI XComboBox_SetItemTemplateXMLFromString(HELE hEle, const char* pStringXML);
//@参数 hEle 元素句柄
//@参数 data 内存地址
//@参数 length 内存大小, 字节为单位
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 组合框_置项模板从内存()
XC_API BOOL WINAPI XComboBox_SetItemTemplateXMLFromMem(HELE hEle, const void* data, int length);
//@备注 RC资源类型必须为:"RT_RCDATA"
//@参数 hEle 元素句柄
//@参数 id RC资源ID
//@参数 pFileName 文件名
//@参数 pPassword zip密码
//@参数 hModule 模块句柄
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 组合框_置项模板从资源ZIP()
XC_API BOOL WINAPI XComboBox_SetItemTemplateXMLFromZipRes(HELE hEle, int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@参数 hEle 元素句柄
//@参数 hTemp 项模板句柄
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 组合框_置项模板()
XC_API BOOL WINAPI XComboBox_SetItemTemplate(HELE hEle, HTEMP hTemp);
//@参数 hEle 元素句柄
//@返回 返回项模板句柄
//@别名 组合框_取项模板()
XC_API HTEMP WINAPI XComboBox_GetItemTemplate(HELE hEle);
//@备注 是否绘制下拉按钮.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否绘制.
//@别名 组合框_启用绘制下拉按钮()
XC_API void WINAPI XComboBox_EnableDrawButton(HELE hEle, BOOL bEnable);
//@备注 启用可编辑显示的文本内容.  
//@参数 hEle 元素句柄.
//@参数 bEdit TRUE可编辑,否则相反.
//@别名 组合框_启用编辑()
XC_API void WINAPI XComboBox_EnableEdit(HELE hEle, BOOL bEdit);
//@备注 启用/关闭下拉列表高度固定大小.  
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 组合框_启用下拉列表高度固定大小()
XC_API void WINAPI XComboBox_EnableDropHeightFixed(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 组合框_弹出下拉列表()
XC_API void WINAPI XComboBox_PopupDropList(HELE hEle);
//@备注 获取组合框下拉列表中选择项索引.  
//@参数 hEle 元素句柄.
//@返回 返回项索引.
//@别名 组合框_取选择项()
XC_API int WINAPI XComboBox_GetSelItem(HELE hEle);
//@备注 获取状态.  
//@参数 hEle 元素句柄.
//@返回 状态.
//@别名 组合框_取状态()
XC_API comboBox_state_ WINAPI XComboBox_GetState(HELE hEle);
//@备注 首次添加数据时, 需要先创建数据适配器  XComboBox_CreateAdapter() \ 组合框_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 pText 
//@返回 返回项索引
//@别名 组合框_添加项文本()
XC_API int WINAPI XComboBox_AddItemText(HELE hEle, const wchar_t* pText);
//@备注 首次添加数据时, 需要先创建数据适配器  XComboBox_CreateAdapter() \ 组合框_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 pName 字段名
//@参数 pText 文本
//@返回 返回项索引
//@别名 组合框_添加项文本扩展()
XC_API int WINAPI XComboBox_AddItemTextEx(HELE hEle, const wchar_t* pName, const wchar_t* pText);
//@备注 首次添加数据时, 需要先创建数据适配器  XComboBox_CreateAdapter() \ 组合框_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 hImage 图片句柄
//@返回 返回项索引
//@别名 组合框_添加项图片()
XC_API int WINAPI XComboBox_AddItemImage(HELE hEle, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XComboBox_CreateAdapter() \ 组合框_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 pName 字段名
//@参数 hImage 图片句柄
//@返回 返回项索引
//@别名 组合框_添加项图片扩展()
XC_API int WINAPI XComboBox_AddItemImageEx(HELE hEle, const wchar_t* pName, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XComboBox_CreateAdapter() \ 组合框_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pText 文本
//@返回 返回项索引
//@别名 组合框_插入项文本()
XC_API int WINAPI XComboBox_InsertItemText(HELE hEle, int iItem, const wchar_t* pValue);
//@备注 首次添加数据时, 需要先创建数据适配器  XComboBox_CreateAdapter() \ 组合框_创建数据适配器()
//@参数 hEle 元素句柄 
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 pText 文本
//@返回 返回项索引
//@别名 组合框_插入项文本扩展()
XC_API int WINAPI XComboBox_InsertItemTextEx(HELE hEle, int iItem, const wchar_t* pName, const wchar_t* pValue);
//@备注 首次添加数据时, 需要先创建数据适配器  XComboBox_CreateAdapter() \ 组合框_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 hImage 图片句柄
//@返回 返回项索引
//@别名 组合框_插入项图片()
XC_API int WINAPI XComboBox_InsertItemImage(HELE hEle, int iItem, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XComboBox_CreateAdapter() \ 组合框_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 hImage 图片句柄
//@返回 返回项索引
//@别名 组合框_插入项图片扩展()
XC_API int WINAPI XComboBox_InsertItemImageEx(HELE hEle, int iItem, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@参数 pText 文本
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_置项文本()
XC_API BOOL WINAPI XComboBox_SetItemText(HELE hEle, int iItem, int iColumn, const wchar_t* pText);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 pText 文本
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_置项文本扩展()
XC_API BOOL WINAPI XComboBox_SetItemTextEx(HELE hEle, int iItem, const wchar_t* pName, const wchar_t* pText);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@参数 hImage 图片句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_置项图片()
XC_API BOOL WINAPI XComboBox_SetItemImage(HELE hEle, int iItem, int iColumn, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 hImage 图片句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_置项图片扩展()
XC_API BOOL WINAPI XComboBox_SetItemImageEx(HELE hEle, int iItem, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@参数 nValue 整数值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_置项整数值()
XC_API BOOL WINAPI XComboBox_SetItemInt(HELE hEle, int iItem, int iColumn, int nValue);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 nValue 整数值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_置项整型值扩展()
XC_API BOOL WINAPI XComboBox_SetItemIntEx(HELE hEle, int iItem, const wchar_t* pName, int nValue);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@参数 fFloat 浮点数
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_置项浮点值()
XC_API BOOL WINAPI XComboBox_SetItemFloat(HELE hEle, int iItem, int iColumn, float fFloat);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 fFloat 浮点数
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_置项浮点值扩展()
XC_API BOOL WINAPI XComboBox_SetItemFloatEx(HELE hEle, int iItem, const wchar_t* pName, float fFloat);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@返回 返回文本内容
//@别名 组合框_取项文本()
XC_API const wchar_t* WINAPI XComboBox_GetItemText(HELE hEle, int iItem, int iColumn);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@返回 返回文本内容
//@别名 组合框_取项文本扩展()
XC_API const wchar_t* WINAPI XComboBox_GetItemTextEx(HELE hEle, int iItem, const wchar_t* pName);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@返回 返回图片句柄
//@别名 组合框_取项图片()
XC_API HIMAGE WINAPI XComboBox_GetItemImage(HELE hEle, int iItem, int iColumn);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@返回 返回图片句柄
//@别名 组合框_取项图片扩展()
XC_API HIMAGE WINAPI XComboBox_GetItemImageEx(HELE hEle, int iItem, const wchar_t* pName);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@参数 pOutValue 接收返回整数值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_取项整数值()
XC_API BOOL WINAPI XComboBox_GetItemInt(HELE hEle, int iItem, int iColumn, int* pOutValue);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 pOutValue 接收返回整数值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_取项整数值扩展()
XC_API BOOL WINAPI XComboBox_GetItemIntEx(HELE hEle, int iItem, const wchar_t* pName, int* pOutValue);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@参数 pOutValue 接收返回浮点值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_取项浮点值()
XC_API BOOL WINAPI XComboBox_GetItemFloat(HELE hEle, int iItem, int iColumn, float* pOutValue);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 pOutValue 接收返回浮点值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_取项浮点值扩展()
XC_API BOOL WINAPI XComboBox_GetItemFloatEx(HELE hEle, int iItem, const wchar_t* pName, float* pOutValue);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_删除项()
XC_API BOOL WINAPI XComboBox_DeleteItem(HELE hEle, int iItem);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 nCount 删除数量
//@返回 成功返回TRUE,否则返回FALSE
//@别名 组合框_删除项扩展()
XC_API BOOL WINAPI XComboBox_DeleteItemEx(HELE hEle, int iItem, int nCount);
//@参数 hEle 元素句柄
//@别名 组合框_删除项全部()
XC_API void WINAPI XComboBox_DeleteItemAll(HELE hEle);
//@参数 hEle 元素句柄
//@别名 组合框_删除列全部()
XC_API void WINAPI XComboBox_DeleteColumnAll(HELE hEle);
//@参数 hEle 
//@返回 返回项数量
//@别名 组合框_取项数量()
XC_API int WINAPI XComboBox_GetCount(HELE hEle);
//@参数 hEle 元素句柄
//@返回 返回列数量
//@别名 组合框_取列数量()
XC_API int WINAPI XComboBox_GetCountColumn(HELE hEle);
//@分组}
//@分组{  列表框

//@备注 创建列表框元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 列表框_创建()
XC_API HELE WINAPI XListBox_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 创建列表框元素; 使用内置项模板, 自动创建数据适配器 
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 列表框_创建扩展()
XC_API HELE WINAPI XListBox_CreateEx(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 注意:为了提高性能,默认使用列表框的全局项高度
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表框_启用固定行高()
XC_API void WINAPI XListBox_EnableFixedRowHeight(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表框_启用模板复用()
XC_API void WINAPI XListBox_EnableTemplateReuse(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表框_启用虚表()
XC_API void WINAPI XListBox_EnableVirtualTable(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 nRowCount 行数
//@别名 列表框_置虚表行数()
XC_API void WINAPI XListBox_SetVirtualRowCount(HELE hEle, int nRowCount);
//@备注 设置是否绘制指定状态下项的背景.  
//@参数 hEle 元素句柄.
//@参数 nFlags 标志位 @ref list_drawItemBk_flag_.
//@别名 列表框_置项背景绘制标志()
XC_API void WINAPI XListBox_SetDrawItemBkFlags(HELE hEle, int nFlags);
//@参数 hEle 元素句柄
//@参数 color 颜色值
//@别名 列表框_置分割线颜色()
XC_API void WINAPI XListBox_SetSplitLineColor(HELE hEle, COLORREF color);
//@备注 设置项用户数据.  
//@参数 hEle 元素句柄.
//@参数 iItem 想索引.
//@参数 nUserData 用户数据.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_置项数据()
XC_API BOOL WINAPI XListBox_SetItemData(HELE hEle, int iItem, vint nUserData);
//@备注 获取项用户数据.  
//@参数 hEle 元素句柄.
//@参数 iItem 项索引.
//@返回 用户数据.
//@别名 列表框_取项数据()
XC_API vint WINAPI XListBox_GetItemData(HELE hEle, int iItem);
//@备注 设置项信息.(已过时接口不推荐使用)  
//@参数 hEle 元素句柄.
//@参数 iItem 项索引.
//@参数 pItem 项信息.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_置项信息()
XC_API BOOL WINAPI XListBox_SetItemInfo(HELE hEle, int iItem, listBox_item_info_* pItem);
//@备注 获取项信息.(已过时接口不推荐使用)  
//@参数 hEle 元素句柄.
//@参数 iItem 项索引.
//@参数 pItem 项信息.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_取项信息()
XC_API BOOL WINAPI XListBox_GetItemInfo(HELE hEle, int iItem, listBox_item_info_* pItem);
//@备注 注意:为了提高性能,默认使用列表框的全局项高度, 如果需要指定项单独高度, 需要关闭固定行高 @ref XListBox_EnableFixedRowHeight
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 nHeight 项高度
//@参数 nSelHeight 项选中时高度
//@别名 列表框_置项高度()
XC_API void WINAPI XListBox_SetItemHeight(HELE hEle, int iItem, int nHeight, int nSelHeight);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@参数 pHeight 项高度
//@参数 pSelHeight 项选中时高度
//@别名 列表框_取项高度()
XC_API void WINAPI XListBox_GetItemHeight(HELE hEle, int iItem, int* pHeight, int* pSelHeight);
//@备注 设置选择选.  
//@参数 hEle 元素句柄.
//@参数 iItem 项索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_置选择项()
XC_API BOOL WINAPI XListBox_SetSelectItem(HELE hEle, int iItem);
//@备注 获取选择项.  
//@参数 hEle 元素句柄.
//@返回 项索引.
//@别名 列表框_取选择项()
XC_API int WINAPI XListBox_GetSelectItem(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iItem 项索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_添加选择项()
XC_API BOOL WINAPI XListBox_AddSelectItem(HELE hEle, int iItem);
//@备注 取消选择指定项.  
//@参数 hEle 元素句柄.
//@参数 iItem 项索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_取消选择项()
XC_API BOOL WINAPI XListBox_CancelSelectItem(HELE hEle, int iItem);
//@备注 取消所有选中的项.  
//@参数 hEle 元素句柄.
//@返回 如果之前有选择状态的项返回TRUE,此时可以更新UI,否则返回FALSE.
//@别名 列表框_取消选择全部()
XC_API BOOL WINAPI XListBox_CancelSelectAll(HELE hEle);
//@备注 获取所有选择项.  
//@参数 hEle 元素句柄.
//@参数 pArray 数组缓冲区.
//@参数 nArraySize 数组大小(数组成员数).
//@返回 返回接收数量.
//@别名 列表框_取全部选择()
XC_API int WINAPI XListBox_GetSelectAll(HELE hEle, int* pArray, int nArraySize);
//@备注 获取选择项数量.  
//@参数 hEle 元素句柄.
//@返回 返回数量.
//@别名 列表框_取选择项数量()
XC_API int WINAPI XListBox_GetSelectCount(HELE hEle);
//@备注 获取鼠标停留项.  
//@参数 hEle 元素句柄.
//@返回 返回鼠标所在项.
//@别名 列表框_取鼠标停留项()
XC_API int WINAPI XListBox_GetItemMouseStay(HELE hEle);
//@备注 选择所有项.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_选择全部项()
XC_API BOOL WINAPI XListBox_SelectAll(HELE hEle);
//@备注 滚动视图让指定项可见  
//@参数 hEle 元素句柄.
//@参数 iItem 项索引.
//@别名 列表框_显示指定项()
XC_API void WINAPI XListBox_VisibleItem(HELE hEle, int iItem);
//@备注 获取当前可见行范围.  
//@参数 hEle 元素句柄
//@参数 piStart 开始行索引
//@参数 piEnd 结束行索引
//@别名 列表框_取可视行范围()
XC_API void WINAPI XListBox_GetVisibleRowRange(HELE hEle, int* piStart, int* piEnd);
//@备注 设置项默认高度.  
//@参数 hEle 元素句柄.
//@参数 nHeight 项高度.
//@参数 nSelHeight 选中项高度.
//@别名 列表框_置项默认高度()
XC_API void WINAPI XListBox_SetItemHeightDefault(HELE hEle, int nHeight, int nSelHeight);
//@备注 获取项默认高度.  
//@参数 hEle 元素句柄.
//@参数 pHeight 高度.
//@参数 pSelHeight 选中时高度.
//@别名 列表框_取项默认高度()
XC_API void WINAPI XListBox_GetItemHeightDefault(HELE hEle, int* pHeight, int* pSelHeight);
//@备注 获取当前对象所在模板实例,属于列表中哪一个项(行).  
//@参数 hEle 元素句柄.
//@参数 hXCGUI 对象句柄, UI元素句柄或形状对象句柄.
//@返回 成功返回项索引, 否则返回@ref XC_ID_ERROR.
//@别名 列表框_取对象所在行()
XC_API int WINAPI XListBox_GetItemIndexFromHXCGUI(HELE hEle, HXCGUI hXCGUI);
//@备注 设置行间距.  
//@参数 hEle 元素句柄.
//@参数 nSpace 间距大小.
//@别名 列表框_置行间距()
XC_API void WINAPI XListBox_SetRowSpace(HELE hEle, int nSpace);
//@备注 获取行间距大小.  
//@参数 hEle 元素句柄.
//@返回 返回行间距大小.
//@别名 列表框_取行间距()
XC_API int WINAPI XListBox_GetRowSpace(HELE hEle);
//@备注 检测坐标点所在项.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@返回 返回项索引.
//@别名 列表框_测试点击项()
XC_API int WINAPI XListBox_HitTest(HELE hEle, POINT* pPt);
//@备注 检测坐标点所在项,自动添加滚动视图偏移量.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@返回 项索引.
//@别名 列表框_测试点击项扩展()
XC_API int WINAPI XListBox_HitTestOffset(HELE hEle, POINT* pPt);
//@备注 设置列表项模板文件.  
//@参数 hEle 元素句柄.
//@参数 pXmlFile 文件名.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_置项模板文件()
XC_API BOOL WINAPI XListBox_SetItemTemplateXML(HELE hEle, const wchar_t* pXmlFile);
//@备注 设置列表项模板.  
//@参数 hEle 元素句柄
//@参数 hTemp 模板句柄
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 列表框_置项模板()
XC_API BOOL WINAPI XListBox_SetItemTemplate(HELE hEle, HTEMP hTemp);
//@备注 获取列表项模板
//@参数 hEle 元素句柄
//@返回 返回项模板句柄
//@别名 列表框_取项模板()
XC_API HTEMP WINAPI XListBox_GetItemTemplate(HELE hEle);
//@备注 设置项模板文件.  
//@参数 hEle 元素句柄.
//@参数 pStringXML 字符串指针.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_置项模板从字符串()
XC_API BOOL WINAPI XListBox_SetItemTemplateXMLFromString(HELE hEle, const char* pStringXML);
//@备注 设置项模板文件
//@参数 hEle 元素句柄.
//@参数 data 内存地址.
//@参数 length 内存大小,字节为单位.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_置项模板从内存()
XC_API BOOL WINAPI XListBox_SetItemTemplateXMLFromMem(HELE hEle, void* data, int length);
//@备注 设置项模板文件, RC资源类型必须为:"RT_RCDATA"
//@参数 hEle 元素句柄.
//@参数 id RC资源ID
//@参数 pFileName 项模板文件名
//@参数 pPassword zip密码
//@参数 hModule 模块句柄
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表框_置项模板从资源ZIP()
XC_API BOOL WINAPI XListBox_SetItemTemplateXMLFromZipRes(HELE hEle, int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@备注 通过模板项ID,获取实例化模板项ID对应的对象句柄.  
//@参数 hEle 元素句柄.
//@参数 iItem 项索引.
//@参数 nTempItemID 模板项ID.
//@返回 成功返回对象句柄,否则返回NULL.
//@别名 列表框_取模板对象()
XC_API HXCGUI WINAPI XListBox_GetTemplateObject(HELE hEle, int iItem, int nTempItemID);
//@备注 是否启用多行选择功能.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 列表框_启用多选()
XC_API void WINAPI XListBox_EnableMultiSel(HELE hEle, BOOL bEnable);
//@备注 创建数据适配器, 并且从绑定的项模板中扫描字段名, 初始化数据适配器的列(字段名);
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//若是没有设置项模板, 那么使用内置项模板;
//@参数 hEle 元素句柄.
//@返回 返回适配器句柄.
//@别名 列表框_创建数据适配器()
XC_API HXCGUI WINAPI XListBox_CreateAdapter(HELE hEle);
//@备注 绑定数据适配器.  
//@参数 hEle 元素句柄.
//@参数 hAdapter 数据适配器句柄 XAdTable.
//@别名 列表框_绑定数据适配器()
XC_API void WINAPI XListBox_BindAdapter(HELE hEle, HXCGUI hAdapter);
//@备注 获取绑定的数据适配器.  
//@参数 hEle 元素句柄.
//@返回 返回数据适配器句柄.
//@别名 列表框_取数据适配器()
XC_API HXCGUI WINAPI XListBox_GetAdapter(HELE hEle);
//@参数 hEle 元素句柄
//@参数 color 颜色值 请使用宏: RGBA()
//@参数 width 线宽度
//@别名 列表框_置拖动矩形颜色()
XC_API void WINAPI XListBox_SetDragRectColor(HELE hEle, COLORREF color, int width);
//@备注 排序.  
//@参数 hEle 元素句柄.
//@参数 iColumnAdapter 需要排序的数据在数据适配器中所属列索引.
//@参数 bAscending 升序(TRUE)或降序(FALSE).
//@别名 列表框_排序()
XC_API void WINAPI XListBox_Sort(HELE hEle, int iColumnAdapter, BOOL bAscending);
//@备注 修改数据后,刷新所有项模板,以便更新数据到模板(如果项可见).  
//@参数 hEle 元素句柄.
//@别名 列表框_刷新数据()
XC_API void WINAPI XListBox_RefreshData(HELE hEle);
//@备注 修改数据后,刷新指定项模板,以便更新数据到模板(如果当前项可见).  
//@参数 hEle 元素句柄.
//@参数 iItem 项索引.
//@别名 列表框_刷新指定项()
XC_API void WINAPI XListBox_RefreshItem(HELE hEle, int iItem);
//@备注 首次添加数据时, 需要先创建数据适配器, XListBox_CreateAdapter() \ 列表框_创建数据适配器()
//@参数 hEle 
//@参数 pText 
//@返回 返回项索引
//@别名 列表框_添加项文本()
XC_API int WINAPI XListBox_AddItemText(HELE hEle, const wchar_t* pText);
//@备注 首次添加数据时, 需要先创建数据适配器  XListBox_CreateAdapter() \ 列表框_创建数据适配器()
//@参数 hEle 
//@参数 pName 
//@参数 pText 
//@别名 列表框_添加项文本扩展()
XC_API int WINAPI XListBox_AddItemTextEx(HELE hEle, const wchar_t* pName, const wchar_t* pText);
//@备注 首次添加数据时, 需要先创建数据适配器  XListBox_CreateAdapter() \ 列表框_创建数据适配器()
//@参数 hEle 
//@参数 hImage 
//@别名 列表框_添加项图片()
XC_API int WINAPI XListBox_AddItemImage(HELE hEle, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XListBox_CreateAdapter() \ 列表框_创建数据适配器()
//@参数 hEle 
//@参数 pName 
//@参数 hImage 
//@别名 列表框_添加项图片扩展()
XC_API int WINAPI XListBox_AddItemImageEx(HELE hEle, const wchar_t* pName, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XListBox_CreateAdapter() \ 列表框_创建数据适配器()
//@参数 hEle 
//@参数 iItem 
//@参数 pValue 
//@别名 列表框_插入项文本()
XC_API int WINAPI XListBox_InsertItemText(HELE hEle, int iItem, const wchar_t* pValue);
//@备注 首次添加数据时, 需要先创建数据适配器  XListBox_CreateAdapter() \ 列表框_创建数据适配器()
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@参数 pValue 
//@别名 列表框_插入项文本扩展()
XC_API int WINAPI XListBox_InsertItemTextEx(HELE hEle, int iItem, const wchar_t* pName, const wchar_t* pValue);
//@备注 首次添加数据时, 需要先创建数据适配器  XListBox_CreateAdapter() \ 列表框_创建数据适配器()
//@参数 hEle 
//@参数 iItem 
//@参数 hImage 
//@别名 列表框_插入项图片()
XC_API int WINAPI XListBox_InsertItemImage(HELE hEle, int iItem, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XListBox_CreateAdapter() \ 列表框_创建数据适配器()
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@参数 hImage 
//@别名 列表框_插入项图片扩展()
XC_API int WINAPI XListBox_InsertItemImageEx(HELE hEle, int iItem, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 
//@参数 iItem 
//@参数 iColumn 
//@参数 pText 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_置项文本()
XC_API BOOL WINAPI XListBox_SetItemText(HELE hEle, int iItem, int iColumn, const wchar_t* pText);
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@参数 pText 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_置项文本扩展()
XC_API BOOL WINAPI XListBox_SetItemTextEx(HELE hEle, int iItem, const wchar_t* pName, const wchar_t* pText);
//@参数 hEle 
//@参数 iItem 
//@参数 iColumn 
//@参数 hImage 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_置项图片()
XC_API BOOL WINAPI XListBox_SetItemImage(HELE hEle, int iItem, int iColumn, HIMAGE hImage);
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@参数 hImage 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_置项图片扩展()
XC_API BOOL WINAPI XListBox_SetItemImageEx(HELE hEle, int iItem, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 
//@参数 iItem 
//@参数 iColumn 
//@参数 nValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_置项整数值()
XC_API BOOL WINAPI XListBox_SetItemInt(HELE hEle, int iItem, int iColumn, int nValue);
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@参数 nValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_置项整数值扩展()
XC_API BOOL WINAPI XListBox_SetItemIntEx(HELE hEle, int iItem, const wchar_t* pName, int nValue);
//@参数 hEle 
//@参数 iItem 
//@参数 iColumn 
//@参数 fFloat 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_置项浮点值()
XC_API BOOL WINAPI XListBox_SetItemFloat(HELE hEle, int iItem, int iColumn, float fFloat);
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@参数 fFloat 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_置项浮点值扩展()
XC_API BOOL WINAPI XListBox_SetItemFloatEx(HELE hEle, int iItem, const wchar_t* pName, float fFloat);
//@参数 hEle 
//@参数 iItem 
//@参数 iColumn 
//@返回 返回文本内容
//@别名 列表框_取项文本()
XC_API const wchar_t* WINAPI XListBox_GetItemText(HELE hEle, int iItem, int iColumn);
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@返回 返回文本内容
//@别名 列表框_取项文本扩展()
XC_API const wchar_t* WINAPI XListBox_GetItemTextEx(HELE hEle, int iItem, const wchar_t* pName);
//@参数 hEle 
//@参数 iItem 
//@参数 iColumn 
//@别名 列表框_取项图片()
XC_API HIMAGE WINAPI XListBox_GetItemImage(HELE hEle, int iItem, int iColumn);
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@别名 列表框_取项图片扩展()
XC_API HIMAGE WINAPI XListBox_GetItemImageEx(HELE hEle, int iItem, const wchar_t* pName);
//@参数 hEle 
//@参数 iItem 
//@参数 iColumn 
//@参数 pOutValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_取项整数值()
XC_API BOOL WINAPI XListBox_GetItemInt(HELE hEle, int iItem, int iColumn, int* pOutValue);
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@参数 pOutValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_取项整数值扩展()
XC_API BOOL WINAPI XListBox_GetItemIntEx(HELE hEle, int iItem, const wchar_t* pName, int* pOutValue);
//@参数 hEle 
//@参数 iItem 
//@参数 iColumn 
//@参数 pOutValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_取项浮点值()
XC_API BOOL WINAPI XListBox_GetItemFloat(HELE hEle, int iItem, int iColumn, float* pOutValue);
//@参数 hEle 
//@参数 iItem 
//@参数 pName 
//@参数 pOutValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_取项浮点值扩展()
XC_API BOOL WINAPI XListBox_GetItemFloatEx(HELE hEle, int iItem, const wchar_t* pName, float* pOutValue);
//@参数 hEle 
//@参数 iItem 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_删除项()
XC_API BOOL WINAPI XListBox_DeleteItem(HELE hEle, int iItem);
//@参数 hEle 
//@参数 iItem 
//@参数 nCount 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表框_删除项扩展()
XC_API BOOL WINAPI XListBox_DeleteItemEx(HELE hEle, int iItem, int nCount);
//@参数 hEle 
//@别名 列表框_删除项全部()
XC_API void WINAPI XListBox_DeleteItemAll(HELE hEle);
//@参数 hEle 
//@别名 列表框_删除列全部()
XC_API void WINAPI XListBox_DeleteColumnAll(HELE hEle);
//@参数 hEle 
//@别名 列表框_取项数量AD()
XC_API int WINAPI XListBox_GetCount_AD(HELE hEle);
//@参数 hEle 
//@别名 列表框_取列数量AD()
XC_API int WINAPI XListBox_GetCountColumn_AD(HELE hEle);
//@分组}
//@分组{  列表

//@备注 创建列表元素.  
//项模板实例化规则:
//列表头: 在实例化项模板时, 根据列表头列数量, 限制实例化模板最大列数, 如果设置了列延伸, 延伸后的总列数也不能超过此限制; 例如当前列表为5列, 实例化后的模板最多为5列;
//列表行: 在实例化项模板时, 根据列表头列数量, 限制实例化模板最大列数, 如果设置了列延伸, 延伸后的总列数也不能超过此限制; 例如当前列表为5列, 实例化后的模板最多为5列;
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 列表_创建()
XC_API HELE WINAPI XList_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 创建列表元素, 使用内置项模板, 自动创建数据适配器  
//项模板实例化规则:
//列表头: 在实例化项模板时, 根据列表头列数量, 限制实例化模板最大列数, 如果设置了列延伸, 延伸后的总列数也不能超过此限制; 例如当前列表为5列, 实例化后的模板最多为5列;
//列表行: 在实例化项模板时, 根据列表头列数量, 限制实例化模板最大列数, 如果设置了列延伸, 延伸后的总列数也不能超过此限制; 例如当前列表为5列, 实例化后的模板最多为5列;
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@参数 col_extend_count 列数量; 例如:内置模板是1列, 如果数据有5列, 那么此参数填5
//@返回 元素句柄.
//@别名 列表_创建扩展()
XC_API HELE WINAPI XList_CreateEx(int x, int y, int cx, int cy, HXCGUI hParent, int col_extend_count);
//@备注 增加列.  
//@参数 hEle 元素句柄.
//@参数 width 列宽度.
//@返回 返回位置索引.
//@别名 列表_增加列()
XC_API int WINAPI XList_AddColumn(HELE hEle, int width);
//@备注 插入列.  
//@参数 hEle 元素句柄.
//@参数 width 列宽度.
//@参数 iCol 插入位置索引.
//@返回 返回插入位置索引.
//@别名 列表_插入列()
XC_API int WINAPI XList_InsertColumn(HELE hEle, int width, int iCol);
//@备注 启用或关闭多选功能.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 列表_启用多选()
XC_API void WINAPI XList_EnableMultiSel(HELE hEle, BOOL bEnable);
//@备注 启用拖动改变列宽度.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 列表_启用拖动更改列宽()
XC_API void WINAPI XList_EnableDragChangeColumnWidth(HELE hEle, BOOL bEnable);
//@备注 启用垂直滚动条顶部对齐.  
//@参数 hEle 元素句柄.
//@参数 bTop 是否启用.
//@别名 列表_启用垂直滚动条顶部对齐()
XC_API void WINAPI XList_EnableVScrollBarTop(HELE hEle, BOOL bTop);
//@备注 启用行背景铺满模式.  
//@参数 hEle 元素句柄.
//@参数 bFull 是否启用铺满.
//@别名 列表_启用行背景铺满()
XC_API void WINAPI XList_EnableRowBkFull(HELE hEle, BOOL bFull);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表_启用固定行高()
XC_API void WINAPI XList_EnableFixedRowHeight(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表_启用模板复用()
XC_API void WINAPI XList_EnableTemplateReuse(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表_启用虚表()
XC_API void WINAPI XList_EnableVirtualTable(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 nRowCount 行数
//@别名 列表_置虚表行数()
XC_API void WINAPI XList_SetVirtualRowCount(HELE hEle, int nRowCount);
//@备注 设置排序属性.  
//@参数 hEle 元素句柄.
//@参数 iColumn 列索引.
//@参数 iColumnAdapter 需要排序的数据在数据适配器中的列索引.
//@参数 bEnable 是否启用排序功能.
//@别名 列表_置排序()
XC_API void WINAPI XList_SetSort(HELE hEle, int iColumn, int iColumnAdapter, BOOL bEnable);
//@备注 设置是否绘制指定状态下行的背景.  
//@参数 hEle 元素句柄.
//@参数 nFlags 标志位 @ref list_drawItemBk_flag_.
//@别名 列表_置行背景绘制标志()
XC_API void WINAPI XList_SetDrawRowBkFlags(HELE hEle, int style);
//@参数 hEle 元素句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 列表_置分割线颜色()
XC_API void WINAPI XList_SetSplitLineColor(HELE hEle, COLORREF color);
//@备注 设置列宽度.  
//@参数 hEle 元素句柄.
//@参数 iRow 列索引.
//@参数 width 宽度.
//@别名 列表_置列宽()
XC_API void WINAPI XList_SetColumnWidth(HELE hEle, int iRow, int width);
//@备注 设置列最小宽度.  
//@参数 hEle 元素句柄.
//@参数 iRow 列索引.
//@参数 width 宽度.
//@别名 列表_置列最小宽度()
XC_API void WINAPI XList_SetColumnMinWidth(HELE hEle, int iRow, int width);
//@备注 设置指定列宽度固定.  
//@参数 hEle 元素句柄.
//@参数 iColumn 列索引.
//@参数 bFixed 是否固定宽度.
//@别名 列表_置列宽度固定()
XC_API void WINAPI XList_SetColumnWidthFixed(HELE hEle, int iColumn, BOOL bFixed);
//@备注 获取指定列宽度.  
//@参数 hEle 元素句柄.
//@参数 iColumn 列索引.
//@返回 返回指定列宽度.
//@别名 列表_取列宽度()
XC_API int WINAPI XList_GetColumnWidth(HELE hEle, int iColumn);
//@备注 获取列数量.  
//@参数 hEle 元素句柄.
//@返回 返回列数量.
//@别名 列表_取列数量()
XC_API int WINAPI XList_GetColumnCount(HELE hEle);
//@备注 设置项用户数据.  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@参数 data 用户数据.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表_置项数据()
XC_API BOOL WINAPI XList_SetItemData(HELE hEle, int iRow, int iColumn, vint data);
//@备注 获取项用户数据.  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@返回 返回用户数据.
//@别名 列表_取项数据()
XC_API vint WINAPI XList_GetItemData(HELE hEle, int iRow, int iColumn);
//@备注 设置选择行.  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表_置选择行()
XC_API BOOL WINAPI XList_SetSelectRow(HELE hEle, int iRow);
//@备注 获取选择行.  
//@参数 hEle 元素句柄.
//@返回 行索引.
//@别名 列表_取选择行()
XC_API int WINAPI XList_GetSelectRow(HELE hEle);
//@备注 获取选择行数量.  
//@参数 hEle 元素句柄.
//@返回 返回选择行数量.
//@别名 列表_取选择行数量()
XC_API int WINAPI XList_GetSelectRowCount(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_添加选择行()
XC_API BOOL WINAPI XList_AddSelectRow(HELE hEle, int iRow);
//@备注 选择全部行.  
//@参数 hEle 元素句柄.
//@别名 列表_置选择全部()
XC_API void WINAPI XList_SetSelectAll(HELE hEle);
//@备注 获取全部选择的行.  
//@参数 hEle 元素句柄.
//@参数 pArray 接收行索引数组.
//@参数 nArraySize 数组大小(数组成员数).
//@返回 返回行数量.
//@别名 列表_取全部选择()
XC_API int WINAPI XList_GetSelectAll(HELE hEle, int* pArray, int nArraySize);
//@备注 滚动视图让指定行可见  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引.
//@别名 列表_显示指定行()
XC_API void WINAPI XList_VisibleRow(HELE hEle, int iRow);
//@备注 取消选择指定行(这里的项可以理解为行).  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表_取消选择行()
XC_API BOOL WINAPI XList_CancelSelectRow(HELE hEle, int iRow);
//@备注 取消选择所有项(这里的项可以理解为行).  
//@参数 hEle 元素句柄.
//@别名 列表_取消全部选择行()
XC_API void WINAPI XList_CancelSelectAll(HELE hEle);
//@备注 获取列表头元素.  
//@参数 hEle 元素句柄.
//@返回 返回列表头元素句柄.
//@别名 列表_取列表头()
XC_API HELE WINAPI XList_GetHeaderHELE(HELE hEle);
//@备注 绑定数据适配器.  
//@参数 hEle 元素句柄.
//@参数 hAdapter 数据适配器句柄 XAdTable.
//@别名 列表_绑定数据适配器()
XC_API void WINAPI XList_BindAdapter(HELE hEle, HXCGUI hAdapter);
//@备注 列表头绑定数据适配器.  
//@参数 hEle 元素句柄.
//@参数 hAdapter 数据适配器句柄 XAdMap.
//@别名 列表_列表头绑定数据适配器()
XC_API void WINAPI XList_BindAdapterHeader(HELE hEle, HXCGUI hAdapter);
//@备注 创建数据适配器, 并且从绑定的项模板中扫描字段名, 以及根据列延伸设置, 生成延伸列的字段名, 初始化数据适配器的列(字段名);
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//若是没有设置项模板, 那么使用内置项模板, 默认为1列, 并且启用了列延伸;
//-------------------------
//参数的值作用: 若是没有使用列延伸, 忽略此参数, 在生成延伸列字段名时, 限制字段名的范围不超过参数指定的列数量; 例如填5, 那么认为列表最多有5列, 列延伸生成的字段名在5列范围内;
//--------------------
//例如默认模板是1列, 但是数据有5列,那么列延伸填5
//@参数 hEle 元素句柄.
//@参数 colExtend_count 列延伸-预计列表总列数, 当使用列延伸功能时此属性有效; 延伸后列表最大总列数, 避免超出范围, 生成不必要的字段名;  
//@返回 返回适配器句柄.
//@别名 列表_创建数据适配器()
XC_API HXCGUI WINAPI XList_CreateAdapter(HELE hEle, int col_extend_count=3);
//@备注 数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//@参数 hEle 元素句柄.
//@返回 返回适配器句柄.
//@别名 列表_列表头创建数据适配器()
XC_API HXCGUI WINAPI XList_CreateAdapterHeader(HELE hEle);
//@备注 创建数据适配器, 并且从绑定的项模板中扫描字段名, 以及根据列延伸设置, 生成延伸列的字段名, 初始化数据适配器的列(字段名);
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//若是没有设置项模板, 那么使用内置项模板, 默认为1列, 并且启用了列延伸;
//-------------------------
//参数的值作用: 若是没有使用列延伸, 忽略此参数, 在生成延伸列字段名时, 限制字段名的范围不超过参数指定的列数量; 例如填5, 那么认为列表最多有5列, 列延伸生成的字段名在5列范围内;
//--------------------
//此接口是简化接口, 合并了 XList_CreateAdapter() 和 XList_CreateAdapterHeader() 接口;
//默认内置模板支持列延伸, 为1列, 通过延伸自己需要的列数;
//例如默认模板是1列, 但是数据有5列,那么列延伸填5
//@参数 hEle 元素句柄.
//@参数 colExtend_count 列延伸-预计列表总列数, 当使用列延伸功能时此属性有效; 延伸后列表最大总列数, 避免超出范围, 增加不必要的字段;  
//@返回 如果成功返回TRUE, 否则返回FALSE.
//@别名 列表_创建数据适配器2()
XC_API BOOL WINAPI XList_CreateAdapters(HELE hEle, int col_extend_count=3);
//@备注 获取数据适配器.  
//@参数 hEle 元素句柄.
//@返回 数据适配器句柄.
//@别名 列表_取数据适配器()
XC_API HXCGUI WINAPI XList_GetAdapter(HELE hEle);
//@备注 获取列表头数据适配器.  
//@参数 hEle 元素句柄.
//@返回 返回数据适配器句柄.
//@别名 列表_列表头获取数据适配器()
XC_API HXCGUI WINAPI XList_GetAdapterHeader(HELE hEle);
//@备注 设置项布局模板文件.  
//@参数 hEle 元素句柄.
//@参数 pXmlFile 文件名.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表_置项模板文件()
XC_API BOOL WINAPI XList_SetItemTemplateXML(HELE hEle, const wchar_t* pXmlFile);
//@参数 hEle 元素句柄
//@参数 data 内地地址
//@参数 length 内存大小, 字节为单位
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 列表_置项模板从内存()
XC_API BOOL WINAPI XList_SetItemTemplateXMLFromMem(HELE hEle, const void* data, int length);
//@备注 从RC资源ZIP加载, RC资源类型必须为:"RT_RCDATA"
//@参数 hEle 元素句柄
//@参数 id RC资源ID
//@参数 pFileName 项模板文件名
//@参数 pPassword zip密码
//@参数 hModule 模块句柄
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 列表_置项模板从资源ZIP()
XC_API BOOL WINAPI XList_SetItemTemplateXMLFromZipRes(HELE hEle, int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@备注 设置项布局模板文件.  
//@参数 hEle 元素句柄.
//@参数 pStringXML 字符串指针.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表_置项模板从字符串()
XC_API BOOL WINAPI XList_SetItemTemplateXMLFromString(HELE hEle, const char* pStringXML);
//@备注 设置列表项模板.  
//@参数 hEle 元素句柄.
//@参数 hTemp 模板句柄, 支持列表头或列表项模板句柄(分别设置一次, 即可以把头和项都设置进去), 内部会自动识别模板类型
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 列表_置项模板()
XC_API BOOL WINAPI XList_SetItemTemplate(HELE hEle, HTEMP hTemp);
//@参数 hEle 元素句柄
//@返回 返回列表项模板句柄
//@别名 列表_取项模板()
XC_API HTEMP WINAPI XList_GetItemTemplate(HELE hEle);
//@参数 hEle 元素句柄
//@返回 返回列表头项模板句柄
//@别名 列表_取项模板列表头()
XC_API HTEMP WINAPI XList_GetItemTemplateHeader(HELE hEle);
//@备注 通过模板项ID,获取实例化模板项ID对应的对象句柄.  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@参数 nTempItemID 模板项itemID.
//@返回 成功返回对象句柄,否则返回NULL.
//@别名 列表_取模板对象()
XC_API HXCGUI WINAPI XList_GetTemplateObject(HELE hEle, int iRow, int iColumn, int nTempItemID);
//@备注 获取当前对象所在模板实例,属于列表中哪一行.  
//@参数 hEle 元素句柄.
//@参数 hXCGUI 对象句柄, UI元素句柄或形状对象句柄.
//@返回 成功返回行索引, 否则返回@ref XC_ID_ERROR.
//@别名 列表_取对象所在行()
XC_API int WINAPI XList_GetRowIndexFromHXCGUI(HELE hEle, HXCGUI hXCGUI);
//@备注 列表头,通过模板项ID,获取实例化模板项ID对应的对象句柄.  
//@参数 hEle 元素句柄.
//@参数 iColumn 列表头 列索引.
//@参数 nTempItemID 模板项ID.
//@返回 成功返回对象句柄,否则返回NULL.
//@别名 列表_取列表头模板对象()
XC_API HXCGUI WINAPI XList_GetHeaderTemplateObject(HELE hEle, int iColumn, int nTempItemID);
//@备注 列表头,获取当前对象所在模板实例,属于列表头中哪一个列.  
//@参数 hEle 元素句柄.
//@参数 hXCGUI 对象句柄.
//@返回 成功返回列索引, 否则返回@ref XC_ID_ERROR.
//@别名 列表_取列表头对象所在列()
XC_API int WINAPI XList_GetHeaderColumnIndexFromHXCGUI(HELE hEle, HXCGUI hXCGUI);
//@备注 设置列表头高度.  
//@参数 hEle 元素句柄.
//@参数 height 高度.
//@别名 列表_置列表头高度()
XC_API void WINAPI XList_SetHeaderHeight(HELE hEle, int height);
//@备注 获取列表头高度.  
//@参数 hEle 元素句柄.
//@返回 返回列表头高度.
//@别名 列表_取列表头高度()
XC_API int WINAPI XList_GetHeaderHeight(HELE hEle);
//@备注 获取当前可见行范围.  
//@参数 hEle 元素句柄.
//@参数 piStart 开始行索引.
//@参数 piEnd 结束行索引.
//@别名 列表_取可视行范围()
XC_API void WINAPI XList_GetVisibleRowRange(HELE hEle, int* piStart, int* piEnd);
//@备注 设置行默认高度.  
//@参数 hEle 元素句柄.
//@参数 nHeight 高度.
//@参数 nSelHeight 选中时高度.
//@别名 列表_置行默认高度()
XC_API void WINAPI XList_SetRowHeightDefault(HELE hEle, int nHeight, int nSelHeight);
//@备注 获取行默认高度.  
//@参数 hEle 元素句柄.
//@参数 pHeight 高度.
//@参数 pSelHeight 选中时高度.
//@别名 列表_取行默认高度()
XC_API void WINAPI XList_GetRowHeightDefault(HELE hEle, int* pHeight, int* pSelHeight);
//@备注 设置行高度.  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引
//@参数 nHeight 高度.
//@参数 nSelHeight 选中时高度.
//@别名 列表_置行高度()
XC_API void WINAPI XList_SetRowHeight(HELE hEle, int iRow, int nHeight, int nSelHeight);
//@备注 获取行高度.  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引
//@参数 pHeight 高度.
//@参数 pSelHeight 选中时高度.
//@别名 列表_取行高度()
XC_API void WINAPI XList_GetRowHeight(HELE hEle, int iRow, int* pHeight, int* pSelHeight);
//@备注 设置行间距.  
//@参数 hEle 元素句柄.
//@参数 nSpace 行间距大小.
//@别名 列表_置行间距()
XC_API void WINAPI XList_SetRowSpace(HELE hEle, int nSpace);
//@备注 获取行间距大小.  
//@参数 hEle 元素句柄.
//@返回 返回行间距大小.
//@别名 列表_取行间距()
XC_API int WINAPI XList_GetRowSpace(HELE hEle);
//@备注 锁定列,设置左侧锁定列分界列索引.  
//@参数 hEle 元素句柄.
//@参数 iColumn 列索引,-1代表不锁定.
//@别名 列表_置锁定列左侧()
XC_API void WINAPI XList_SetLockColumnLeft(HELE hEle, int iColumn);
//@备注 锁定列,设置右侧锁定列分界列索引  
//@参数 hEle 元素句柄.
//@参数 iColumn 列索引, -1代表不锁定. 暂时只支持锁定末尾列 
//@别名 列表_置锁定列右侧()
XC_API void WINAPI XList_SetLockColumnRight(HELE hEle, int iColumn);
//@备注 设置是否锁定末尾行.  
//@参数 hEle 元素句柄.
//@参数 bLock 是否锁定.
//@别名 列表_置锁定行底部()
XC_API void WINAPI XList_SetLockRowBottom(HELE hEle, BOOL bLock);
//@参数 hEle 元素句柄
//@参数 bOverlap 是否重叠
//@别名 列表_置锁定行底部重叠()
XC_API void WINAPI XList_SetLockRowBottomOverlap(HELE hEle, BOOL bOverlap);
//@参数 hEle 元素句柄
//@参数 color 颜色值 请使用宏: RGBA()
//@参数 width 线宽度
//@别名 列表_置拖动矩形颜色()
XC_API void WINAPI XList_SetDragRectColor(HELE hEle, COLORREF color, int width);
//@备注 检测坐标点所在项.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@参数 piRow 行索引.
//@参数 piColumn 列索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表_测试点击项()
XC_API BOOL WINAPI XList_HitTest(HELE hEle, POINT* pPt, int* piRow, int* piColumn);
//@备注 检查坐标点所在项,自动添加滚动视图偏移量.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@参数 piRow 行索引.
//@参数 piColumn 列索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表_测试点击项扩展()
XC_API BOOL WINAPI XList_HitTestOffset(HELE hEle, POINT* pPt, int* piRow, int* piColumn);
//@备注 修改数据后,刷新所有项模板,以便更新数据到模板(如果项可见).  
//@参数 hEle 元素句柄.
//@别名 列表_刷新数据()
XC_API void WINAPI XList_RefreshData(HELE hEle);
//@备注 修改数据后,刷新指定行模板,以便更新数据到模板(如果当前项可见).  
//@参数 hEle 元素句柄.
//@参数 iRow 行索引.
//@别名 列表_刷新指定行()
XC_API void WINAPI XList_RefreshRow(HELE hEle, int iRow);
//@返回 hEle  元素句柄
//@别名 列表_刷新数据列表头()
XC_API void WINAPI XList_RefreshDataHeader(HELE hEle);
//@备注 删除列.  
//列表头-删除指定列, 列表项保持不变, 数据适配器保持不变, 仅针对列表头项
//@参数 hEle 元素句柄.
//@参数 iColumn 列索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表_删除列()
XC_API BOOL WINAPI XList_DeleteColumn(HELE hEle, int iColumn);
//@备注 列表头-删除所有的列, 列表项保持不变, 数据适配器保持不变, 仅针对列表头项
//@参数 hEle 元素句柄.
//@别名 列表_删除列全部()
XC_API void WINAPI XList_DeleteColumnAll(HELE hEle);
//@备注 简化接口, 自动生成字段名, 规则为: "name1, name2, name3, ..."   
//首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 nWidth 列宽度
//@参数 pText 标题文本
//@返回 返回列索引
//@别名 列表_添加列文本2()
XC_API int WINAPI XList_AddColumnText2(HELE hEle, int nWidth, const wchar_t* pText);
//@备注 简化接口  
//首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 nWidth 列宽度
//@参数 hImage 图片句柄
//@返回 返回列索引
//@别名 列表_添加列图片2()
XC_API int WINAPI XList_AddColumnImage2(HELE hEle, int nWidth, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 nWidth 
//@参数 pName 
//@参数 pText 
//@返回 列索引
//@别名 列表_添加列文本()
XC_API int WINAPI XList_AddColumnText(HELE hEle, int nWidth, const wchar_t* pName, const wchar_t* pText);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 nWidth 
//@参数 pName 
//@参数 hImage 
//@返回 列索引
//@别名 列表_添加列图片()
XC_API int WINAPI XList_AddColumnImage(HELE hEle, int nWidth, const wchar_t* pName, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 pText 
//@返回 行索引
//@别名 列表_添加行文本()
XC_API int WINAPI XList_AddRowText(HELE hEle, const wchar_t* pText);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 pName 
//@参数 pText 
//@返回 行索引
//@别名 列表_添加行文本扩展()
XC_API int WINAPI XList_AddRowTextEx(HELE hEle, const wchar_t* pName, const wchar_t* pText);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 hImage 
//@返回 行索引
//@别名 列表_添加行图片()
XC_API int WINAPI XList_AddRowImage(HELE hEle, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 pName 
//@参数 hImage 
//@返回 行索引
//@别名 列表_添加行图片扩展()
XC_API int WINAPI XList_AddRowImageEx(HELE hEle, const wchar_t* pName, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pValue 
//@返回 行索引
//@别名 列表_插入行文本()
XC_API int WINAPI XList_InsertRowText(HELE hEle, int iRow, const wchar_t* pValue);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@参数 pValue 
//@返回 行索引
//@别名 列表_插入行文本扩展()
XC_API int WINAPI XList_InsertRowTextEx(HELE hEle, int iRow, const wchar_t* pName, const wchar_t* pValue);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 hImage 
//@返回 行索引
//@别名 列表_插入行图片()
XC_API int WINAPI XList_InsertRowImage(HELE hEle, int iRow, HIMAGE hImage);
//@备注 首次添加数据时, 需要先创建数据适配器  XList_CreateAdapters() \ 列表_创建数据适配器2()
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@参数 hImage 
//@返回 行索引
//@别名 列表_插入行图片扩展()
XC_API int WINAPI XList_InsertRowImageEx(HELE hEle, int iRow, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 iColumn 在数据适配器中的列索引
//@参数 pText 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_置项文本()
XC_API BOOL WINAPI XList_SetItemText(HELE hEle, int iRow, int iColumn, const wchar_t* pText);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@参数 pText 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_置项文本扩展()
XC_API BOOL WINAPI XList_SetItemTextEx(HELE hEle, int iRow, const wchar_t* pName, const wchar_t* pText);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 iColumn 
//@参数 hImage 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_置项图片()
XC_API BOOL WINAPI XList_SetItemImage(HELE hEle, int iRow, int iColumn, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@参数 hImage 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_置项图片扩展()
XC_API BOOL WINAPI XList_SetItemImageEx(HELE hEle, int iRow, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 iColumn 
//@参数 nValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_置项整型值()
XC_API BOOL WINAPI XList_SetItemInt(HELE hEle, int iRow, int iColumn, int nValue);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@参数 nValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_置项整数值扩展()
XC_API BOOL WINAPI XList_SetItemIntEx(HELE hEle, int iRow, const wchar_t* pName, int nValue);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 iColumn 
//@参数 fFloat 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_置项浮点值()
XC_API BOOL WINAPI XList_SetItemFloat(HELE hEle, int iRow, int iColumn, float fFloat);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@参数 fFloat 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_置项浮点值扩展()
XC_API BOOL WINAPI XList_SetItemFloatEx(HELE hEle, int iRow, const wchar_t* pName, float fFloat);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 iColumn 
//@返回 返回文本内容
//@别名 列表_取项文本()
XC_API const wchar_t* WINAPI XList_GetItemText(HELE hEle, int iRow, int iColumn);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@返回 返回文本内容
//@别名 列表_取项文本扩展()
XC_API const wchar_t* WINAPI XList_GetItemTextEx(HELE hEle, int iRow, const wchar_t* pName);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 iColumn 
//@别名 列表_取项图片()
XC_API HIMAGE WINAPI XList_GetItemImage(HELE hEle, int iRow, int iColumn);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@别名 列表_取项图片扩展()
XC_API HIMAGE WINAPI XList_GetItemImageEx(HELE hEle, int iRow, const wchar_t* pName);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 iColumn 
//@参数 pOutValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_取项整数值()
XC_API BOOL WINAPI XList_GetItemInt(HELE hEle, int iRow, int iColumn, int* pOutValue);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@参数 pOutValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_取项整数值扩展()
XC_API BOOL WINAPI XList_GetItemIntEx(HELE hEle, int iRow, const wchar_t* pName, int* pOutValue);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 iColumn 
//@参数 pOutValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_取项浮点值()
XC_API BOOL WINAPI XList_GetItemFloat(HELE hEle, int iRow, int iColumn, float* pOutValue);
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 pName 
//@参数 pOutValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_取项浮点值扩展()
XC_API BOOL WINAPI XList_GetItemFloatEx(HELE hEle, int iRow, const wchar_t* pName, float* pOutValue);
//@备注 数据适配器-删除指定项 和 列表删除指定项, 列表头保持不变, 仅针对列表项
//@参数 hEle 元素句柄
//@参数 iRow 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_删除行()
XC_API BOOL WINAPI XList_DeleteRow(HELE hEle, int iRow);
//@备注 数据适配器-删除指定数量的行 和 列表删除指定数量项, 列表头保持不变, 仅针对列表项
//@参数 hEle 元素句柄
//@参数 iRow 
//@参数 nCount 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表_删除行扩展()
XC_API BOOL WINAPI XList_DeleteRowEx(HELE hEle, int iRow, int nCount);
//@备注 数据适配器-删除全部项 和 列表删除全部项, 列表头保持不变, 仅针对列表项
//@参数 hEle 元素句柄
//@别名 列表_删除行全部()
XC_API void WINAPI XList_DeleteRowAll(HELE hEle);
//@备注 数据适配器-清空数据适配器 和 列表删除全部项, 列表头保持不变, 仅针对列表项
//@参数 hEle 元素句柄
//@别名 列表_删除列全部AD()
XC_API void WINAPI XList_DeleteColumnAll_AD(HELE hEle);
//@备注 数据适配器-获取行数量
//@参数 hEle 元素句柄
//@返回 返回行数量
//@别名 列表_取行数量AD()
XC_API int WINAPI XList_GetCount_AD(HELE hEle);
//@备注 数据适配器-获取列数量
//@参数 hEle 元素句柄
//@返回 返回列数量
//@别名 列表_取列数量AD()
XC_API int WINAPI XList_GetCountColumn_AD(HELE hEle);
//@分组}
//@分组{  列表视图

//@备注 创建列表视图元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或U I元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 列表视_创建()
XC_API HELE WINAPI XListView_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 创建列表视图元素. 使用内置项模板, 自动创建数据适配器  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或U I元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 列表视_创建扩展()
XC_API HELE WINAPI XListView_CreateEx(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 创建数据适配器, 并且从绑定的项模板中扫描字段名, 初始化数据适配器的列(字段名);
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//若是没有设置项模板, 那么使用内置项模板; 
//@参数 hEle 元素句柄.
//@返回 返回适配器句柄.
//@别名 列表视_创建数据适配器()
XC_API HXCGUI WINAPI XListView_CreateAdapter(HELE hEle);
//@备注 绑定数据适配器.  
//@参数 hEle 元素句柄.
//@参数 hAdapter 数据适配器 XAdListView.
//@别名 列表视_绑定数据适配器()
XC_API void WINAPI XListView_BindAdapter(HELE hEle, HXCGUI hAdapter);
//@备注 获取数据适配器.  
//@参数 hEle 元素句柄.
//@返回 返回数据适配器.
//@别名 列表视_取数据适配器()
XC_API HXCGUI WINAPI XListView_GetAdapter(HELE hEle);
//@备注 置项模板文件.  
//@参数 hEle 元素句柄.
//@参数 pXmlFile 文件名.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表视_置项模板文件()
XC_API BOOL WINAPI XListView_SetItemTemplateXML(HELE hEle, const wchar_t* pXmlFile);
//@备注 置项布局模板.  
//@参数 hEle 元素句柄.
//@参数 pStringXML 字符串指针.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表视_置项模板从字符串()
XC_API BOOL WINAPI XListView_SetItemTemplateXMLFromString(HELE hEle, const char* pStringXML);
//@参数 hEle 元素句柄
//@参数 data 内存地址
//@参数 length 内存大小, 字节为单位
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 列表视_置项模板从内存()
XC_API BOOL WINAPI XListView_SetItemTemplateXMLFromMem(HELE hEle, const void* data, int length);
//@备注 RC资源类型必须为:"RT_RCDATA"
//@参数 hEle 元素句柄
//@参数 id RC资源ID
//@参数 pFileName 文件名
//@参数 pPassword zip密码
//@参数 hModule 模块句柄
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 列表视_置项模板从资源ZIP()
XC_API BOOL WINAPI XListView_SetItemTemplateXMLFromZipRes(HELE hEle, int id, const wchar_t* pFileName, const wchar_t* pPassword, HMODULE hModule);
//@备注 置列表项模板.  
//@参数 hEle 元素句柄.
//@参数 hTemp 模板句柄, 支持组或项模板句柄(分别设置一次, 即可以把组和项都设置进去), 内部会自动识别模板类型
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 列表视_置项模板()
XC_API BOOL WINAPI XListView_SetItemTemplate(HELE hEle, HTEMP hTemp);
//@参数 hEle 元素句柄
//@返回 返回项模板句柄
//@别名 列表视_取项模板()
XC_API HTEMP WINAPI XListView_GetItemTemplate(HELE hEle);
//@参数 hEle 元素句柄
//@返回 返回项模板组句柄
//@别名 列表视_取项模板组()
XC_API HTEMP WINAPI XListView_GetItemTemplateGroup(HELE hEle);
//@备注 通过模板项ID,获取实例化模板项ID对应的对象句柄.  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 nTempItemID 模板项ID.
//@返回 成功返回对象句柄,否则返回NULL.
//@别名 列表视_取模板对象()
XC_API HXCGUI WINAPI XListView_GetTemplateObject(HELE hEle, int iGroup, int iItem, int nTempItemID);
//@备注 通过模板项ID,获取实例化模板项ID对应的对象句柄.  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 nTempItemID 模板项ID.
//@返回 成功返回对象句柄,否则返回NULL.
//@别名 列表视_取模板对象组()
XC_API HXCGUI WINAPI XListView_GetTemplateObjectGroup(HELE hEle, int iGroup, int nTempItemID);
//@备注 获取当前对象所在模板实例,属于列表视中哪一个项.  
//@参数 hEle 元素句柄.
//@参数 hXCGUI 对象句柄, UI元素句柄或形状对象句柄.
//@参数 piGroup 接收组索引.
//@参数 piItem 接收项索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表视_取对象所在项()
XC_API BOOL WINAPI XListView_GetItemIDFromHXCGUI(HELE hEle, HXCGUI hXCGUI, int* piGroup, int* piItem);
//@备注 检查坐标点所在项.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@参数 pOutGroup 接收组索引.
//@参数 pOutItem 接收项索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表视_测试点击项()
XC_API BOOL WINAPI XListView_HitTest(HELE hEle, POINT* pPt, int* pOutGroup, int* pOutItem);
//@备注 检查坐标点所在项,自动添加滚动视图偏移量.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@参数 pOutGroup 接收做索引.
//@参数 pOutItem 接收项索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表视_测试点击项扩展()
XC_API BOOL WINAPI XListView_HitTestOffset(HELE hEle, POINT* pPt, int* pOutGroup, int* pOutItem);
//@备注 启用多选.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 列表视_启用多选()
XC_API void WINAPI XListView_EnableMultiSel(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表视_启用模板复用()
XC_API void WINAPI XListView_EnableTemplateReuse(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表视_启用虚表()
XC_API void WINAPI XListView_EnableVirtualTable(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 nCount 项数量
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_置虚表项数量()
XC_API BOOL WINAPI XListView_SetVirtualItemCount(HELE hEle, int iGroup, int nCount);
//@备注 置是否绘制指定状态下项的背景.  
//@参数 hEle 元素句柄.
//@参数 nFlags 标志位  @ref list_drawItemBk_flag_.
//@别名 列表视_置项背景绘制标志()
XC_API void WINAPI XListView_SetDrawItemBkFlags(HELE hEle, int nFlags);
//@备注 置选择项.  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引, 如果选择组, 此值填-1
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表视_置选择项()
XC_API BOOL WINAPI XListView_SetSelectItem(HELE hEle, int iGroup, int iItem);
//@备注 获取选择项.  
//@参数 hEle 元素句柄.
//@参数 piGroup 接收组索引.
//@参数 piItem 接收项索引, 如果为组, 那么返回-1.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表视_取选择项()
XC_API BOOL WINAPI XListView_GetSelectItem(HELE hEle, int* piGroup, int* piItem);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_添加选择项()
XC_API BOOL WINAPI XListView_AddSelectItem(HELE hEle, int iGroup, int iItem);
//@备注 滚动视图让指定项可见  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@别名 列表视_显示指定项()
XC_API void WINAPI XListView_VisibleItem(HELE hEle, int iGroup, int iItem);
//@备注 获取当前可见项范围.  
//@参数 hEle 元素句柄
//@参数 piGroup1 可视开始组, 组可见
//@参数 piGroup2 可视结束组, 组可见
//@参数 piStartGroup 可视开始组
//@参数 piStartItem 可视开始项
//@参数 piEndGroup 可视结束组
//@参数 piEndItem 可视结束项
//@别名 列表视_取可视项范围()
XC_API void WINAPI XListView_GetVisibleItemRange(HELE hEle, int* piGroup1, int* piGroup2, int* piStartGroup, int* piStartItem, int* piEndGroup, int* piEndItem);
//@备注 获取选择项数量.  
//@参数 hEle 元素句柄.
//@返回 返回选择项数量.
//@别名 列表视_取选择项数量()
XC_API int WINAPI XListView_GetSelectItemCount(HELE hEle);
//@备注 获取选择的项ID.  
//@参数 hEle 元素句柄.
//@参数 pArray 数组,用来接收选择项ID.
//@参数 nArraySize 数组大小(数组成员数).
//@返回 返回接收项数量.
//@别名 列表视_取选择项全部()
XC_API int WINAPI XListView_GetSelectAll(HELE hEle, listView_item_id_* pArray, int nArraySize);
//@备注 选择所有的项.  
//@参数 hEle 元素句柄.
//@别名 列表视_置选择项全部()
XC_API void WINAPI XListView_SetSelectAll(HELE hEle);
//@备注 取消选择所有项.  
//@参数 hEle 元素句柄.
//@别名 列表视_取消选择项全部()
XC_API void WINAPI XListView_CancelSelectAll(HELE hEle);
//@备注 置列间隔大小.  
//@参数 hEle 元素句柄.
//@参数 space 间隔大小.
//@别名 列表视_置列间隔()
XC_API void WINAPI XListView_SetColumnSpace(HELE hEle, int space);
//@备注 置行间隔大小.  
//@参数 hEle 元素句柄.
//@参数 space 间隔大小.
//@别名 列表视_置行间隔()
XC_API void WINAPI XListView_SetRowSpace(HELE hEle, int space);
//@备注 置项大小.  
//@参数 hEle 元素句柄.
//@参数 width 宽度.
//@参数 height 高度.
//@别名 列表视_置项大小()
XC_API void WINAPI XListView_SetItemSize(HELE hEle, int width, int height);
//@备注 获取项大小.  
//@参数 hEle 元素句柄.
//@参数 pSize 接收返回大小.
//@别名 列表视_取项大小()
XC_API void WINAPI XListView_GetItemSize(HELE hEle, SIZE* pSize);
//@备注 置组高度.  
//@参数 hEle 元素句柄.
//@参数 height 高度.
//@别名 列表视_置组高度()
XC_API void WINAPI XListView_SetGroupHeight(HELE hEle, int height);
//@备注 获取组高度.  
//@参数 hEle 元素句柄.
//@返回 返回组高度.
//@别名 列表视_取组高度()
XC_API int WINAPI XListView_GetGroupHeight(HELE hEle);
//@备注 置组用户数据.  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 nData 数据.
//@别名 列表视_置组用户数据()
XC_API void WINAPI XListView_SetGroupUserData(HELE hEle, int iGroup, vint nData);
//@备注 设置项用户数据.  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 nData 数据.
//@别名 列表视_置项用户数据()
XC_API void WINAPI XListView_SetItemUserData(HELE hEle, int iGroup, int iItem, vint nData);
//@备注 获取组用户数据.  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@返回 返回用户数据.
//@别名 列表视_取组用户数据()
XC_API vint WINAPI XListView_GetGroupUserData(HELE hEle, int iGroup);
//@备注 获取项用户数据.  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@返回 返回用户数据.
//@别名 列表视_取项用户数据()
XC_API vint WINAPI XListView_GetItemUserData(HELE hEle, int iGroup, int iItem);
//@参数 hEle 元素句柄
//@参数 color 颜色值 请使用宏: RGBA()
//@参数 width 线宽度
//@别名 列表视_置拖动矩形颜色()
XC_API void WINAPI XListView_SetDragRectColor(HELE hEle, COLORREF color, int width);
//@备注 修改数据后,刷新所有项模板,以便更新数据到模板(如果项可见).  
//@参数 hEle 元素句柄.
//@别名 列表视_刷新数据()
XC_API void WINAPI XListView_RefreshData(HELE hEle);
//@备注 修改数据后,刷新指定项模板,以便更新数据到模板(如果当前项可见).  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引, 如果为-1,代表为组.
//@别名 列表视_刷新指定项()
XC_API void WINAPI XListView_RefreshItem(HELE hEle, int iGroup, int iItem);
//@备注 展开组.  
//@参数 hEle 元素句柄.
//@参数 iGroup 组索引.
//@参数 bExpand 是否展开.
//@返回 成功返回TRUE否则返回FALSE,如果状态没有改变返回FALSE.
//@别名 列表视_展开组()
XC_API BOOL WINAPI XListView_ExpandGroup(HELE hEle, int iGroup, BOOL bExpand);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 pName 字段称
//@返回 返回列索引
//@别名 列表视_组添加列()
XC_API int WINAPI XListView_Group_AddColumn(HELE hEle, const wchar_t* pName);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 pValue 值
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回组索引
//@别名 列表视_组添加项文本()
XC_API int WINAPI XListView_Group_AddItemText(HELE hEle, const wchar_t* pValue, int iPos);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 pName 字段称
//@参数 pValue 值
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回组索引
//@别名 列表视_组添加项文本扩展()
XC_API int WINAPI XListView_Group_AddItemTextEx(HELE hEle, const wchar_t* pName, const wchar_t* pValue, int iPos);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 hImage 图片句柄
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回组索引
//@别名 列表视_组添加项图片()
XC_API int WINAPI XListView_Group_AddItemImage(HELE hEle, HIMAGE hImage, int iPos);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 pName 字段称
//@参数 hImage 图片句柄
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回组索引
//@别名 列表视_组添加项图片扩展()
XC_API int WINAPI XListView_Group_AddItemImageEx(HELE hEle, const wchar_t* pName, HIMAGE hImage, int iPos);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iColumn 列索引
//@参数 pValue 值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_组置文本()
XC_API BOOL WINAPI XListView_Group_SetText(HELE hEle, int iGroup, int iColumn, const wchar_t* pValue);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 pName 字段名
//@参数 pValue 值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_组置文本扩展()
XC_API BOOL WINAPI XListView_Group_SetTextEx(HELE hEle, int iGroup, const wchar_t* pName, const wchar_t* pValue);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iColumn 列索引
//@参数 hImage 图片句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_组置图片()
XC_API BOOL WINAPI XListView_Group_SetImage(HELE hEle, int iGroup, int iColumn, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 pName 字段名
//@参数 hImage 图片句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_组置图片扩展()
XC_API BOOL WINAPI XListView_Group_SetImageEx(HELE hEle, int iGroup, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iColumn 列索引
//@返回 返回文本内容
//@别名 列表视_组取文本()
XC_API const wchar_t* WINAPI XListView_Group_GetText(HELE hEle, int iGroup, int iColumn);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 pName 字段名称
//@返回 返回文本内容
//@别名 列表视_组取文本扩展()
XC_API const wchar_t* XListView_Group_GetTextEx(HELE hEle, int iGroup, const wchar_t* pName);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iColumn 列索引
//@返回 返回图片句柄
//@别名 列表视_组取图片()
XC_API HIMAGE WINAPI XListView_Group_GetImage(HELE hEle, int iGroup, int iColumn);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 pName 字段名称
//@返回 返回图片句柄
//@别名 列表视_组取图片扩展()
XC_API HIMAGE WINAPI XListView_Group_GetImageEx(HELE hEle, int iGroup, const wchar_t* pName);
//@参数 hEle 元素句柄
//@返回 返回组数量
//@别名 列表视_组获取数量()
XC_API int WINAPI XListView_Group_GetCount(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@返回 成功返回项数量,否则返回 @ref XC_ID_ERROR.
//@别名 列表视_项取数量()
XC_API int WINAPI XListView_Item_GetCount(HELE hEle, int iGroup);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 pName 字段名
//@返回 返回列索引
//@别名 列表视_项添加列()
XC_API int WINAPI XListView_Item_AddColumn(HELE hEle, const wchar_t* pName);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 pValue 值
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回项索引
//@别名 列表视_项添加文本()
XC_API int WINAPI XListView_Item_AddItemText(HELE hEle, int iGroup, const wchar_t* pValue, int iPos);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 pName 字段名
//@参数 pValue 值
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回项索引
//@别名 列表视_项添加文本扩展()
XC_API int WINAPI XListView_Item_AddItemTextEx(HELE hEle, int iGroup, const wchar_t* pName, const wchar_t* pValue, int iPos);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 hImage 图片句柄
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回项索引
//@别名 列表视_项添加图片()
XC_API int WINAPI XListView_Item_AddItemImage(HELE hEle, int iGroup, HIMAGE hImage, int iPos);
//@备注 首次添加数据时, 需要先创建数据适配器 XListView_CreateAdapter() \ 列表视_创建数据适配器()
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 pName 字段名
//@参数 hImage 图片句柄
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回项索引
//@别名 列表视_项添加图片扩展()
XC_API int WINAPI XListView_Item_AddItemImageEx(HELE hEle, int iGroup, const wchar_t* pName, HIMAGE hImage, int iPos);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@参数 pValue 值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_项置文本()
XC_API BOOL WINAPI XListView_Item_SetText(HELE hEle, int iGroup, int iItem, int iColumn, const wchar_t* pValue);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@参数 pName 字段名
//@参数 pValue 值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_项置文本扩展()
XC_API BOOL WINAPI XListView_Item_SetTextEx(HELE hEle, int iGroup, int iItem, const wchar_t* pName, const wchar_t* pValue);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@参数 hImage 图片句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_项置图片()
XC_API BOOL WINAPI XListView_Item_SetImage(HELE hEle, int iGroup, int iItem, int iColumn, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@参数 pName 列名称
//@参数 hImage 图片句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_项置图片扩展()
XC_API BOOL WINAPI XListView_Item_SetImageEx(HELE hEle, int iGroup, int iItem, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_组删除()
XC_API BOOL WINAPI XListView_Group_DeleteItem(HELE hEle, int iGroup);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@别名 列表视_组删除全部子项()
XC_API void WINAPI XListView_Group_DeleteAllChildItem(HELE hEle, int iGroup);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表视_项删除()
XC_API BOOL WINAPI XListView_Item_DeleteItem(HELE hEle, int iGroup, int iItem);
//@参数 hEle 元素句柄
//@别名 列表视_删除全部()
XC_API void WINAPI XListView_DeleteAll(HELE hEle);
//@参数 hEle 元素句柄
//@别名 列表视_删除全部组()
XC_API void WINAPI XListView_DeleteAllGroup(HELE hEle);
//@参数 hEle 元素句柄
//@别名 列表视_删除全部项()
XC_API void WINAPI XListView_DeleteAllItem(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iColumn 列索引
//@别名 列表视_组删除列()
XC_API void WINAPI XListView_DeleteColumnGroup(HELE hEle, int iColumn);
//@参数 hEle 元素句柄
//@参数 iColumn 列索引
//@别名 列表视_项删除列()
XC_API void WINAPI XListView_DeleteColumnItem(HELE hEle, int iColumn);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@返回 返回文本内容
//@别名 列表视_项取文本()
XC_API const wchar_t* WINAPI XListView_Item_GetText(HELE hEle, int iGroup, int iItem, int iColumn);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@参数 pName 字段称
//@返回 返回文本内容
//@别名 列表视_项取文本扩展()
XC_API const wchar_t* WINAPI XListView_Item_GetTextEx(HELE hEle, int iGroup, int iItem, const wchar_t* pName);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@参数 iColumn 列索引
//@返回 返回图片句柄
//@别名 列表视_项取图片()
XC_API HIMAGE WINAPI XListView_Item_GetImage(HELE hEle, int iGroup, int iItem, int iColumn);
//@参数 hEle 元素句柄
//@参数 iGroup 组索引
//@参数 iItem 项索引
//@参数 pName 字段称
//@返回 返回图片句柄
//@别名 列表视_项取图片扩展()
XC_API HIMAGE WINAPI XListView_Item_GetImageEx(HELE hEle, int iGroup, int iItem, const wchar_t* pName);
//@分组}
//@分组{  菜单条

//@备注 创建菜单条元素;如果指定了父为窗口,默认调用XWnd_AddMenuBar()函数,将菜单条添加到窗口非客户区.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或U I元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 菜单条_创建()
XC_API HELE WINAPI XMenuBar_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 添加弹出菜单按钮.  
//@参数 hEle 元素句柄.
//@参数 pText 文本内容.
//@返回 返回菜单按钮索引.
//@别名 菜单条_添加按钮()
XC_API int WINAPI XMenuBar_AddButton(HELE hEle, const wchar_t* pText);
//@备注 设置菜单按钮高度.(已废弃)请使用内填充限制高度  
//@参数 hEle 元素句柄.
//@参数 height 高度.
//@别名 菜单条_置按钮高度()
XC_API void WINAPI XMenuBar_SetButtonHeight(HELE hEle, int height);
//@备注 获取菜单.  
//@参数 hEle 元素句柄.
//@参数 nIndex 菜单条上菜单按钮的索引.
//@返回 返回菜单句柄.
//@别名 菜单条_取菜单()
XC_API HMENUX WINAPI XMenuBar_GetMenu(HELE hEle, int nIndex);
//@参数 hEle 元素句柄
//@参数 nIndex 菜单条上菜单按钮的索引
//@返回 返回按钮句柄
//@别名 菜单条_取菜单按钮()
XC_API HELE WINAPI XMenuBar_GetButton(HELE hEle, int nIndex);
//@备注 菜单条当前选择项, 当前弹出菜单的按钮索引
//@参数 hEle 元素句柄
//@返回 返回选择项索引
//@别名 菜单条_取选择项()
XC_API int WINAPI XMenuBar_GetSelect(HELE hEle);
//@备注 删除菜单条上的菜单按钮,同时该按钮下的弹出菜单也被销毁.  
//@参数 hEle 元素句柄.
//@参数 nIndex 菜单条按钮索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 菜单条_删除按钮()
XC_API BOOL WINAPI XMenuBar_DeleteButton(HELE hEle, int nIndex);
//@备注 根据内容自动调整宽度
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 菜单条_启用自动宽度()
XC_API void WINAPI XMenuBar_EnableAutoWidth(HELE hEle, BOOL bEnable);
//@分组}
//@分组{  窗格

//@备注 创建窗格元素.  
//@参数 pName 窗格标题.
//@参数 nWidth 宽度.
//@参数 nHeight 高度.
//@参数 hFrameWnd 框架窗口.
//@返回 元素句柄.
//@别名 窗格_创建()
XC_API HELE WINAPI XPane_Create(const wchar_t* pName, int nWidth, int nHeight, HWINDOW hFrameWnd=NULL);
//@备注 设置窗格视图元素.  
//@参数 hEle 元素句柄.
//@参数 hView 绑定视图元素.
//@别名 窗格_置视图()
XC_API void WINAPI XPane_SetView(HELE hEle, HELE hView);
//@备注 设置标题文本.  
//@参数 hEle 元素句柄.
//@参数 pTitle 文本内容.
//@别名 窗格_置标题()
XC_API void WINAPI XPane_SetTitle(HELE hEle, wchar_t* pTitle);
//@备注 获取标题文本.  
//@参数 hEle 元素句柄.
//@返回 返回文本内容
//@别名 窗格_取标题()
XC_API const wchar_t* WINAPI XPane_GetTitle(HELE hEle);
//@备注 设置标题栏高度.  
//@参数 hEle 元素句柄.
//@参数 nHeight 高度.
//@别名 窗格_置标题栏高度()
XC_API void WINAPI XPane_SetCaptionHeight(HELE hEle, int nHeight);
//@备注 获取标题栏高度.  
//@参数 hEle 元素句柄.
//@返回 返回标题栏高度.
//@别名 窗格_取标题栏高度()
XC_API int WINAPI XPane_GetCaptionHeight(HELE hEle);
//@备注 判断窗格是否显示.  
//@参数 hEle 元素句柄.
//@返回 如果显示返回TRUE否则返回FALSE.
//@别名 窗格_是否显示()
XC_API BOOL WINAPI XPane_IsShowPane(HELE hEle);
//@备注 判断窗格是否激活, 当为组成员时有效
//@参数 hEle 元素句柄
//@返回 如果为窗格组显示项返回TRUE,否则返回FALSE
//@别名 窗格_是否激活()
XC_API BOOL WINAPI XPane_IsGroupActivate(HELE hEle);
//@备注 设置窗格大小.  
//@参数 hEle 元素句柄.
//@参数 nWidth 宽度.
//@参数 nHeight 高度.
//@别名 窗格_置大小()
XC_API void WINAPI XPane_SetSize(HELE hEle, int nWidth, int nHeight);
//@备注 获取窗格停靠状态.  
//@参数 hEle 元素句柄.
//@返回 返回状态标识 @ref pane_state_.
//@别名 窗格_取状态()
XC_API pane_state_ WINAPI XPane_GetState(HELE hEle);
//@备注 获取窗格视图坐标.  
//@参数 hEle 元素句柄.
//@参数 pRect 接收返回的坐标值.
//@别名 窗格_取视图坐标()
XC_API void WINAPI XPane_GetViewRect(HELE hEle, RECT* pRect);
//@备注 隐藏窗格.  
//@参数 hEle 元素句柄.
//@参数 bGroupDelay 当为窗格组成员时, 延迟处理窗格组成员激活的切换
//@别名 窗格_隐藏()
XC_API void WINAPI XPane_HidePane(HELE hEle, BOOL bGroupDelay=FALSE);
//@备注 显示窗格.  
//@参数 hEle 元素句柄.
//@参数 bGroupActivate 如果是窗格组成员,那么窗格组切换当前窗格为显示状态
//@别名 窗格_显示()
XC_API void WINAPI XPane_ShowPane(HELE hEle, BOOL bGroupActivate);
//@备注 窗格停靠到码头.  
//@参数 hEle 元素句柄.
//@别名 窗格_停靠()
XC_API void WINAPI XPane_DockPane(HELE hEle);
//@备注 锁定窗格.  
//@参数 hEle 元素句柄.
//@别名 窗格_锁定()
XC_API void WINAPI XPane_LockPane(HELE hEle);
//@备注 浮动窗格.  
//@参数 hEle 元素句柄.
//@别名 窗格_浮动()
XC_API void WINAPI XPane_FloatPane(HELE hEle);
//@备注 手动调用该函数绘制窗格, 以便控制绘制顺序.  
//@参数 hEle 元素句柄.
//@参数 hDraw 图形绘制句柄.
//@别名 窗格_绘制()
XC_API void WINAPI XPane_DrawPane(HELE hEle, HDRAW hDraw);
//@备注 如果窗格是组成员,设置选中当前窗格可见
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 窗格_置选中()
XC_API BOOL WINAPI XPane_SetSelect(HELE hEle);
//@备注 获取窗格所属的tabBar
//@参数 hEle 元素句柄
//@返回 若是有返回tabBar句柄, 否则返回空
//@别名 窗格_取TabBar()
XC_API HELE WINAPI XPane_GetTabBar(HELE hEle);
//@备注 获取窗格分割条, 分割条类型为基础元素
//@参数 hEle 元素句柄
//@返回 若是有返回分割条句柄, 否则返回空
//@别名 窗格_取分割条()
XC_API HELE WINAPI XPane_GetSplitBar(HELE hEle);
//@备注 获取窗格上的控制按钮
//@参数 hEle 元素句柄
//@参数 number 控制按钮编号,  1: 菜单按钮, 2:停靠码头按钮, 3:关闭按钮
//@返回 返回按钮句柄
//@别名 窗格_取控制按钮()
XC_API HELE WINAPI XPane_GetButton(HELE hEle, int number);
//@备注 显示隐藏窗格上的控制按钮
//@参数 hEle 元素句柄
//@别名 窗格_显示控制按钮()
XC_API void WINAPI XPane_ShowButton(HELE hPane, BOOL bShow);
//@分组}
//@分组{  进度条

//@备注 创建进度条元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 进度条_创建()
XC_API HELE WINAPI XProgBar_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 显示进度值文本
//@参数 hEle 元句柄
//@参数 bShow 是否启用
//@别名 进度条_启用进度文本()
XC_API void WINAPI XProgBar_EnableShowText(HELE hEle, BOOL bShow);
//@备注 缩放进度贴图为当前进度区域(当前进度所显示区域),否则为整体100%进度区域
//@参数 hEle 元句柄
//@参数 bStretch 缩放
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 进度条_启用缩放()
XC_API void WINAPI XProgBar_EnableStretch(HELE hEle, BOOL bStretch);
//@备注 设置范围.  
//@参数 hEle 元句柄.
//@参数 range 范围.
//@别名 进度条_置范围()
XC_API void WINAPI XProgBar_SetRange(HELE hEle, int range);
//@备注 获取范围.  
//@参数 hEle 元句柄.
//@返回 返回范围.
//@别名 进度条_取范围()
XC_API int WINAPI XProgBar_GetRange(HELE hEle);
//@备注 设置进度贴图.  
//@参数 hEle 元句柄.
//@参数 hImage 图片句柄.
//@别名 进度条_置进度图片()
XC_API void WINAPI XProgBar_SetImageLoad(HELE hEle, HIMAGE hImage);
//@备注 设置进度颜色
//@参数 hEle 元句柄
//@参数 color 颜色值 @ref RGBA()
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 进度条_置进度颜色()
XC_API void WINAPI XProgBar_SetColorLoad(HELE hEle, COLORREF color);
//@备注 设置位置点.  
//@参数 hEle 元句柄.
//@参数 pos 位置点.
//@别名 进度条_置进度()
XC_API void WINAPI XProgBar_SetPos(HELE hEle, int pos);
//@备注 获取当前位置点.  
//@参数 hEle 元句柄.
//@返回 返回当前位置点.
//@别名 进度条_取进度()
XC_API int WINAPI XProgBar_GetPos(HELE hEle);
//@备注 设置水平或垂直.  
//@参数 hEle 元句柄.
//@参数 bHorizon 水平或垂直.
//@别名 进度条_启用水平()
XC_API void WINAPI XProgBar_EnableHorizon(HELE hEle, BOOL bHorizon);
//@分组}
//@分组{  滚动条

//@备注 创建滚动条元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 滚动条_创建()
XC_API HELE WINAPI XSBar_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 设置滚动范围.  
//@参数 hEle 元素句柄.
//@参数 range 范围.
//@别名 滚动条_置范围()
XC_API void WINAPI XSBar_SetRange(HELE hEle, int range);
//@备注 获取滚动范围.  
//@参数 hEle 元素句柄.
//@返回 滚动范围.
//@别名 滚动条_取范围()
XC_API int WINAPI XSBar_GetRange(HELE hEle);

//@备注 获取当前滚动位置
//@参数 hEle 元素句柄
//@返回 返回当前位置
//@别名 滚动条_取当前位置()
XC_API int  WINAPI XSBar_GetCurPos(HELE hEle);
//@备注 显示隐藏滚动条上下按钮.  
//@参数 hEle 元素句柄.
//@参数 bShow 是否显示.
//@别名 滚动条_显示上下按钮()
XC_API void WINAPI XSBar_ShowButton(HELE hEle, BOOL bShow);
//@备注 设置滑块长度.  
//@参数 hEle 元素句柄.
//@参数 length 长度.
//@别名 滚动条_置滑块长度()
XC_API void WINAPI XSBar_SetSliderLength(HELE hEle, int length);
//@备注 设置滑块最小长度.  
//@参数 hEle 元素句柄.
//@参数 minLength 长度.
//@别名 滚动条_置滑块最小长度()
XC_API void WINAPI XSBar_SetSliderMinLength(HELE hEle, int minLength);
//@备注 设置滑块两端的间隔大小.  
//@参数 hEle 元素句柄.
//@参数 nPadding 间隔大小.
//@别名 滚动条_置滑块两端间隔()
XC_API void WINAPI XSBar_SetSliderPadding(HELE hEle, int nPadding);
//@备注 设置水平或者垂直.  
//@参数 hEle 元素句柄.
//@参数 bEnable 水平或垂直.
//@返回 如果改变返回TRUE否则返回FALSE.
//@别名 滚动条_启用水平()
XC_API BOOL WINAPI XSBar_EnableHorizon(HELE hEle, BOOL bHorizon);
//@备注 获取滑块最大长度.  
//@参数 hEle 元素句柄.
//@返回 长度.
//@别名 滚动条_取滑块最大长度()
XC_API int WINAPI XSBar_GetSliderMaxLength(HELE hEle);
//@备注 向上滚动.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动条_向上滚动()
XC_API BOOL WINAPI XSBar_ScrollUp(HELE hEle);
//@备注 向下滚动.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动条_向下滚动()
XC_API BOOL WINAPI XSBar_ScrollDown(HELE hEle);
//@备注 滚动到顶部.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动条_滚动到顶部()
XC_API BOOL WINAPI XSBar_ScrollTop(HELE hEle);
//@备注 滚动到底部.  
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动条_滚动到底部()
XC_API BOOL WINAPI XSBar_ScrollBottom(HELE hEle);
//@备注 滚动到指定位置点 ,触发事件 @ref XE_SBAR_SCROLL ,  
//@参数 hEle 元素句柄.
//@参数 pos 位置点.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 滚动条_滚动到指定位置()
XC_API BOOL WINAPI XSBar_ScrollPos(HELE hEle, int pos);
//@备注 获取上按钮.  
//@参数 hEle 元素句柄.
//@返回 返回按钮句柄.
//@别名 滚动条_取上按钮()
XC_API HELE WINAPI XSBar_GetButtonUp(HELE hEle);
//@备注 获取下按钮.  
//@参数 hEle 元素句柄.
//@返回 返回按钮句柄.
//@别名 滚动条_取下按钮()
XC_API HELE WINAPI XSBar_GetButtonDown(HELE hEle);
//@备注 获取滑动按钮.  
//@参数 hEle 元素句柄.
//@返回 返回按钮句柄.
//@别名 滚动条_取滑块()
XC_API HELE WINAPI XSBar_GetButtonSlider(HELE hEle);
//@分组}
//@分组{  滑动条

//@备注 创建滑动条元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 滑动条_创建()
XC_API HELE WINAPI XSliderBar_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 设置滑动范围.  
//@参数 hEle 元素句柄.
//@参数 range 范围.
//@别名 滑动条_置范围()
XC_API void WINAPI XSliderBar_SetRange(HELE hEle, int range);
//@备注 获取滚动范围.  
//@参数 hEle 元素句柄.
//@返回 返回滚动范围.
//@别名 滑动条_取范围()
XC_API int WINAPI XSliderBar_GetRange(HELE hEle);
//@备注 设置进度贴图.  
//@参数 hEle 元素句柄.
//@参数 hImage 图片句柄.
//@别名 滑动条_置进度图片()
XC_API void WINAPI XSliderBar_SetImageLoad(HELE hEle, HIMAGE hImage);
//@备注 设置滑块按钮宽度.  
//@参数 hEle 元素句柄.
//@参数 width 宽度.
//@别名 滑动条_置滑块宽度()
XC_API void WINAPI XSliderBar_SetButtonWidth(HELE hEle, int width);
//@备注 设置滑块按钮高度.  
//@参数 hEle 元素句柄.
//@参数 height 高度.
//@别名 滑动条_置滑块高度()
XC_API void WINAPI XSliderBar_SetButtonHeight(HELE hEle, int height);
//@备注 设置当前进度点.  
//@参数 hEle 元素句柄.
//@参数 pos 进度点.
//@别名 滑动条_置当前位置()
XC_API void WINAPI XSliderBar_SetPos(HELE hEle, int pos);
//@备注 获取当前进度点.  
//@参数 hEle 元素句柄.
//@返回 返回当前进度点.
//@别名 滑动条_取当前位置()
XC_API int WINAPI XSliderBar_GetPos(HELE hEle);
//@备注 获取滑块按钮.  
//@参数 hEle 元素句柄.
//@返回 按钮句柄.
//@别名 滑动条_取滑块()
XC_API HELE WINAPI XSliderBar_GetButton(HELE hEle);
//@备注 设置水平或垂直.  
//@参数 hEle 元素句柄.
//@参数 bHorizon 水平或垂直.
//@别名 滑动条_启用水平()
XC_API void WINAPI XSliderBar_EnableHorizon(HELE hEle, BOOL bHorizon);
//@分组}
//@分组{  Tab条

//@备注 创建tabBar元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 TAB条_创建()
XC_API HELE WINAPI XTabBar_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 添加一个标签.  
//@参数 hEle 元素句柄
//@参数 pName 标签文本内容.
//@返回 标签索引.
//@别名 TAB条_添加标签()
XC_API int WINAPI XTabBar_AddLabel(HELE hEle, const wchar_t* pName);
//@备注 插入一个标签.  
//@参数 hEle 元素句柄.
//@参数 index 插入位置.
//@参数 pName 标签文本内容.
//@返回 标签索引.
//@别名 TAB条_插入标签()
XC_API int WINAPI XTabBar_InsertLabel(HELE hEle, int index, const wchar_t* pName);
//@参数 hEle 元素句柄.
//@参数 iSrc 源位置索引
//@参数 iDest 目标位置索引
//@返回 成功返回TRUE否则FALSE.
//@别名 TAB条_移动标签()
XC_API BOOL WINAPI XTabBar_MoveLabel(HELE hEle, int iSrc, int iDest);
//@备注 删除一个标签.  
//@参数 hEle 元素句柄.
//@参数 index 位置索引.
//@返回 成功返回TRUE否则FALSE.
//@别名 TAB条_删除标签()
XC_API BOOL WINAPI XTabBar_DeleteLabel(HELE hEle, int index);
//@备注 删除所有标签.  
//@参数 hEle 元素句柄.
//@别名 TAB条_删除全部()
XC_API void WINAPI XTabBar_DeleteLabelAll(HELE hEle);
//@备注 获取标签按钮Button.  
//@参数 hEle 元素句柄.
//@参数 index 位置索引.
//@返回 按钮句柄.
//@别名 TAB条_取标签()
XC_API HELE WINAPI XTabBar_GetLabel(HELE hEle, int index);
//@备注 获取标签上关闭按钮.  
//@参数 hEle 元素句柄.
//@参数 index 位置索引.
//@返回 按钮句柄.
//@别名 TAB条_取标签上的关闭按钮()
XC_API HELE WINAPI XTabBar_GetLabelClose(HELE hEle, int index);
//@备注 获取左滚动按钮.  
//@参数 hEle 元素句柄.
//@返回 返回按钮句柄.
//@别名 TAB条_取左滚动按钮()
XC_API HELE WINAPI XTabBar_GetButtonLeft(HELE hEle);
//@备注 获取右滚动按钮.  
//@参数 hEle 元素句柄.
//@返回 返回按钮句柄.
//@别名 TAB条_取右滚动按钮()
XC_API HELE WINAPI XTabBar_GetButtonRight(HELE hEle);
//@参数 hEle 元素句柄.
//@返回 返回按钮句柄.
//@别名 TAB条_取下拉菜单按钮句柄()
XC_API HELE WINAPI XTabBar_GetButtonDropMenu(HELE hEle);
//@备注 获取选择的标签索引.  
//@参数 hEle 元素句柄.
//@返回 标签位置索引.
//@别名 TAB条_取当前选择()
XC_API int WINAPI XTabBar_GetSelect(HELE hEle);
//@备注 获取标签间距, 0没有间距.  
//@参数 hEle 元素句柄.
//@返回 标签间隔大小.
//@别名 TAB条_取间隔()
XC_API int WINAPI XTabBar_GetLabelSpacing(HELE hEle);
//@备注 获取标签项数量.  
//@参数 hEle 元素句柄.
//@返回 标签项数量.
//@别名 TAB条_取标签数量()
XC_API int WINAPI XTabBar_GetLabelCount(HELE hEle);
//@备注 获取标签按钮位置索引.  
//@参数 hEle 元素句柄.
//@参数 hLabel 标签按钮句柄.
//@返回 成功返回索引值,否则返回 @ref XC_ID_ERROR.
//@别名 TAB条_取标签位置索引()
XC_API int WINAPI XTabBar_GetindexByEle(HELE hEle, HELE hLabel);
//@备注 设置标签间距, 0没有间距.  
//@参数 hEle 元素句柄.
//@参数 spacing 标签间隔大小.
//@别名 TAB条_置间隔()
XC_API void WINAPI XTabBar_SetLabelSpacing(HELE hEle, int spacing);
//@备注 设置选择标签.  
//@参数 hEle 元素句柄.
//@参数 index 标签位置索引.
//@别名 TAB条_置选择()
XC_API void WINAPI XTabBar_SetSelect(HELE hEle, int index);
//@备注 左按钮滚动.  
//@参数 hEle 元素句柄.
//@别名 TAB条_左滚动()
XC_API void WINAPI XTabBar_SetUp(HELE hEle);
//@备注 右按钮滚动.  
//@参数 hEle 元素句柄.
//@别名 TAB条_右滚动()
XC_API void WINAPI XTabBar_SetDown(HELE hEle);
//@备注 平铺标签,每个标签显示相同大小.  
//@参数 hEle 元素句柄.
//@参数 bTile 是否启用.
//@别名 TAB条_启用平铺()
XC_API void WINAPI XTabBar_EnableTile(HELE hEle, BOOL bTile);
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 TAB条_启用下拉菜单按钮()
XC_API void WINAPI XTabBar_EnableDropMenu(HELE hEle, BOOL bEnable);
//@备注 启用关闭标签功能.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 TAB条_启用标签带关闭按钮()
XC_API void WINAPI XTabBar_EnableClose(HELE hEle, BOOL bEnable);
//@备注 设置关闭按钮大小.  
//@参数 hEle 元素句柄.
//@参数 pSize 大小值, 宽度和高度可以为-1,-1代表默认值.
//@别名 TAB条_置关闭按钮大小()
XC_API void WINAPI XTabBar_SetCloseSize(HELE hEle, SIZE* pSize);
//@备注 设置翻滚按钮大小.  
//@参数 hEle 元素句柄.
//@参数 pSize 大小值, 宽度和高度可以为-1,-1代表默认值.
//@别名 TAB条_置滚动按钮大小()
XC_API void WINAPI XTabBar_SetTurnButtonSize(HELE hEle, SIZE* pSize);
//@备注 设置指定标签为固定宽度.  
//@参数 hEle 元素句柄.
//@参数 index 索引.
//@参数 nWidth 宽度, 如果值为-1,那么自动计算宽度.
//@别名 TAB条_置指定标签固定宽度()
XC_API void WINAPI XTabBar_SetLabelWidth(HELE hEle, int index, int nWidth);
//@备注 显示或隐藏指定标签.  
//@参数 hEle 元素句柄.
//@参数 index 标签索引.
//@参数 bShow 是否显示.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 TAB条_显示标签()
XC_API BOOL WINAPI XTabBar_ShowLabel(HELE hEle, int index, BOOL bShow);
//@分组}
//@分组{  文本链接

//@备注 创建静态文本连接元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 pName 文本内容.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 文本链接_创建()
XC_API HELE WINAPI XTextLink_Create(int x, int y, int cx, int cy, const wchar_t* pName, HXCGUI hParent=NULL);
//@备注 启用下划线,鼠标离开状态.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 文本链接_启用离开状态下划线()
XC_API void WINAPI XTextLink_EnableUnderlineLeave(HELE hEle, BOOL bEnable);
//@备注 启用下划线,鼠标停留状态.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 文本链接_停留状态下划线()
XC_API void WINAPI XTextLink_EnableUnderlineStay(HELE hEle, BOOL bEnable);
//@备注 设置文本颜色,鼠标停留状态.  
//@参数 hEle 元素句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 文本链接_置停留状态文本颜色()
XC_API void WINAPI XTextLink_SetTextColorStay(HELE hEle, COLORREF color);
//@备注 设置下划线颜色,鼠标离开状态.  
//@参数 hEle 元素句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 文本链接_置离开状态下划线颜色()
XC_API void WINAPI XTextLink_SetUnderlineColorLeave(HELE hEle, COLORREF color);
//@备注 设置下划线颜色,鼠标停留状态.  
//@参数 hEle 元素句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 文本链接_置停留状态下划线颜色()
XC_API void WINAPI XTextLink_SetUnderlineColorStay(HELE hEle, COLORREF color);
//@分组}
//@分组{  工具条

//@备注 创建工具条元素;如果指定了父为窗口,默认调用XWnd_AddToolBar()函数,将工具条添加到窗口非客户区.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 工具条_创建()
XC_API HELE WINAPI XToolBar_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 插入元素到工具条.  
//@参数 hEle 元素句柄.
//@参数 hNewEle 将要插入的元素.
//@参数 index 插入位置索引, (-1)插入末尾..
//@返回 返回插入位置索引.
//@别名 工具条_插入元素()
XC_API int WINAPI XToolBar_InsertEle(HELE hEle, HELE hNewEle, int index=-1);
//@备注 插入分隔符到工具条.  
//@参数 hEle 元素句柄.
//@参数 index 插入位置索引, (-1)插入末尾.
//@参数 color 颜色值, 请使用宏: RGBA()
//@返回 返回插入位置索引.
//@别名 工具条_插入分割栏()
XC_API int WINAPI XToolBar_InsertSeparator(HELE hEle, int index=-1, COLORREF color=RGBA(128,128,128,255));
//@备注 启用下拉菜单,显示隐藏的项.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 工具条_启用下拉菜单()
XC_API void WINAPI XToolBar_EnableButtonMenu(HELE hEle, BOOL bEnable);
//@备注 获取工具条上指定元素.  
//@参数 hEle 元素句柄.
//@参数 index 索引值.
//@返回 返回元素句柄.
//@别名 工具条_取元素()
XC_API HELE WINAPI XToolBar_GetEle(HELE hEle, int index);
//@备注 获取左滚动按钮.  
//@参数 hEle 元素句柄.
//@返回 返回按钮句柄.
//@别名 工具条_取左滚动按钮()
XC_API HELE WINAPI XToolBar_GetButtonLeft(HELE hEle);
//@备注 获取右滚动按钮.  
//@参数 hEle 元素句柄.
//@返回 返回按钮句柄.
//@别名 工具条_取右滚动按钮()
XC_API HELE WINAPI XToolBar_GetButtonRight(HELE hEle);
//@备注 获取菜单按钮.  
//@参数 hEle 元素句柄.
//@返回 返回菜单按钮句柄.
//@别名 工具条_取菜单按钮()
XC_API HELE WINAPI XToolBar_GetButtonMenu(HELE hEle);
//@备注 设置对象之间的间距.  
//@参数 hEle 元素句柄.
//@参数 nSize 间距大小.
//@别名 工具条_置间距()
XC_API void WINAPI XToolBar_SetSpace(HELE hEle, int nSize);
//@备注 删除元素,并且销毁.  
//@参数 hEle 元素句柄.
//@参数 index 索引值.
//@别名 工具条_删除元素()
XC_API void WINAPI XToolBar_DeleteEle(HELE hEle, int index);
//@备注 删除所有元素,并且销毁.  
//@参数 hEle 元素句柄.
//@别名 工具条_删除全部()
XC_API void WINAPI XToolBar_DeleteAllEle(HELE hEle);
//@分组}
//@分组{  列表树

//@备注 创建树元素.  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 列表树_创建()
XC_API HELE WINAPI XTree_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 创建树元素. 使用内置项模板, 自动创建数据适配器  
//如果是元素资源句柄将被添加到元素.
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父是窗口资源句柄或UI元素资源句柄.如果是窗口资源句柄将被添加到窗口,
//@返回 元素句柄.
//@别名 列表树_创建扩展()
XC_API HELE WINAPI XTree_CreateEx(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 启用拖动项功能.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 列表树_启用拖动项()
XC_API void WINAPI XTree_EnableDragItem(HELE hEle, BOOL bEnable);
//@备注 启用或禁用显示项的连接线.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@参数 bSolid 实线或虚线; TRUE:实线, FALSE:虚线.
//@别名 列表树_启用连接线()
XC_API void WINAPI XTree_EnableConnectLine(HELE hEle, BOOL bEnable, BOOL bSolid);
//@备注 启动或关闭默认展开功能,如果开启新插入的项将自动展开.  
//@参数 hEle 元素句柄.
//@参数 bEnable 是否启用.
//@别名 列表树_启用展开()
XC_API void WINAPI XTree_EnableExpand(HELE hEle, BOOL bEnable);
//@参数 hEle 元素句柄
//@参数 bEnable 是否启用
//@别名 列表树_启用模板复用()
XC_API void WINAPI XTree_EnableTemplateReuse(HELE hEle, BOOL bEnable);
//@备注 设置项连接线颜色.  
//@参数 hEle 元素句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 列表树_置连接线颜色()
XC_API void WINAPI XTree_SetConnectLineColor(HELE hEle, COLORREF color);
//@备注 设置展开按钮占用空间大小.  
//@参数 hEle 元素句柄.
//@参数 nWidth 宽度.
//@参数 nHeight 高度.
//@别名 列表树_置展开按钮大小()
XC_API void WINAPI XTree_SetExpandButtonSize(HELE hEle, int nWidth, int nHeight);
//@备注 设置连线绘制长度,展开按钮与项内容之间的连线.  
//@参数 hEle 元素句柄.
//@参数 nLength 连线绘制长度.
//@别名 列表树_置连接线长度()
XC_API void WINAPI XTree_SetConnectLineLength(HELE hEle, int nLength);
//@备注 设置拖动项插入位置颜色提示.  
//@参数 hEle 元素句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 列表树_置拖动项插入位置颜色()
XC_API void WINAPI XTree_SetDragInsertPositionColor(HELE hEle, COLORREF color);
//@备注 设置项模板文件.  
//@参数 hEle 元素句柄.
//@参数 pXmlFile 文件名.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表树_置项模板文件()
XC_API BOOL WINAPI XTree_SetItemTemplateXML(HELE hEle, const wchar_t* pXmlFile);
//@备注 设置项模板文件,项选中状态.  
//@参数 hEle 元素句柄.
//@参数 pXmlFile 文件名.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表树_置选择项模板文件()
XC_API BOOL WINAPI XTree_SetItemTemplateXMLSel(HELE hEle, const wchar_t* pXmlFile);
//@备注 设置列表项模板.  
//@参数 hEle 元素句柄.
//@参数 hTemp 模板句柄.
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 列表树_置项模板()
XC_API BOOL WINAPI XTree_SetItemTemplate(HELE hEle, HTEMP hTemp);
//@备注 设置列表项模板,项选中状态.  
//@参数 hEle 元素句柄.
//@参数 hTemp 模板句柄.
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 列表树_置选择项模板()
XC_API BOOL WINAPI XTree_SetItemTemplateSel(HELE hEle, HTEMP hTemp);
//@备注 设置项模板文件.  
//@参数 hEle 元素句柄.
//@参数 pStringXML 字符串指针.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表树_置项模板从字符串()
XC_API BOOL WINAPI XTree_SetItemTemplateXMLFromString(HELE hEle, const char* pStringXML);
//@备注 设置项模板文件,项选中状态.  
//@参数 hEle 元素句柄.
//@参数 pStringXML 字符串指针.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表树_置选择项模板从字符串()
XC_API BOOL WINAPI XTree_SetItemTemplateXMLSelFromString(HELE hEle, const char* pStringXML);
//@参数 hEle 元素句柄
//@参数 data 内存地址
//@参数 length 内存大小, 字节为单位
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 列表树_置项模板从内存()
XC_API BOOL WINAPI XTree_SetItemTemplateXMLFromMem(HELE hEle, const void* data, int length);
//@备注 RC资源类型必须为:"RT_RCDATA"
//@参数 hEle 元素句柄
//@参数 id RC资源ID
//@参数 pFileName 文件名
//@参数 pPassword zip密码
//@参数 hModule 模块句柄
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 列表树_置项模板从资源ZIP()
XC_API BOOL WINAPI XTree_SetItemTemplateXMLFromZipRes(HELE hEle, int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@参数 hEle 元素句柄
//@返回 返回项模板句柄
//@别名 列表树_取项模板()
XC_API HTEMP WINAPI XTree_GetItemTemplate(HELE hEle);
//@备注 设置是否绘制指定状态下项的背景.  
//@参数 hEle 元素句柄.
//@参数 nFlags 标志位 @ref list_drawItemBk_flag_.
//@别名 列表树_置项背景绘制标志()
XC_API void WINAPI XTree_SetDrawItemBkFlags(HELE hEle, int nFlags);
//@参数 hEle 元素句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 列表树_置分割线颜色()
XC_API void WINAPI XTree_SetSplitLineColor(HELE hEle, COLORREF color);
//@备注 设置项用户数据.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@参数 nUserData 用户数据.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表树_置项数据()
XC_API BOOL WINAPI XTree_SetItemData(HELE hEle, int nID, vint nUserData);
//@备注 获取项用户数据.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@返回 项用户数据.
//@别名 列表树_取项数据()
XC_API vint WINAPI XTree_GetItemData(HELE hEle, int nID);
//@备注 设置选择项.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表树_置选择项()
XC_API BOOL WINAPI XTree_SetSelectItem(HELE hEle, int nID);
//@备注 获取选择项.  
//@参数 hEle 元素句柄.
//@返回 项ID.
//@别名 列表树_取选择项()
XC_API int WINAPI XTree_GetSelectItem(HELE hEle);
//@备注 滚动视图让指定项可见  
//@参数 hEle 元素句柄.
//@参数 nID 项索引.
//@别名 列表树_可视指定项()
XC_API void WINAPI XTree_VisibleItem(HELE hEle, int nID);
//@备注 判断项是否展开.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@返回 如果展开返回TRUE否则返回FALSE.
//@别名 列表树_是否展开()
XC_API BOOL WINAPI XTree_IsExpand(HELE hEle, int nID);
//@备注 展开项.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@参数 bExpand 是否展开.
//@返回 成功返回TRUE,如果项已经展开或失败返回FALSE.
//@别名 列表树_展开项()
XC_API BOOL WINAPI XTree_ExpandItem(HELE hEle, int nID, BOOL bExpand);
//@备注 展开所有的子项.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@参数 bExpand 是否展开.
//@返回 成功返回TRUE,失败返回FALSE.
//@别名 列表树_展开全部子项()
XC_API BOOL WINAPI XTree_ExpandAllChildItem(HELE hEle, int nID, BOOL bExpand);
//@备注 检测坐标点所在项.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@返回 项ID.
//@别名 列表树_测试点击项()
XC_API int WINAPI XTree_HitTest(HELE hEle, POINT* pPt);
//@备注 检测坐标点所在项,自动添加滚动视图偏移坐标.  
//@参数 hEle 元素句柄.
//@参数 pPt 坐标点.
//@返回 项ID.
//@别名 列表树_测试点击项扩展()
XC_API int WINAPI XTree_HitTestOffset(HELE hEle, POINT* pPt);
//@备注 获取第一个子项.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@返回 返回项ID,失败返回XC_ID_ERROR.
//@别名 列表树_取第一个子项()
XC_API int WINAPI XTree_GetFirstChildItem(HELE hEle, int nID);
//@备注 获取末尾子项.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@返回 返回末尾子项ID,失败返回XC_ID_ERROR.
//@别名 列表树_取末尾子项()
XC_API int WINAPI XTree_GetEndChildItem(HELE hEle, int nID);
//@备注 获取上一个兄弟项.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@返回 返回上一个兄弟项ID, 失败返回 XC_ID_ERROR.
//@别名 列表树_取上一个兄弟项()
XC_API int WINAPI XTree_GetPrevSiblingItem(HELE hEle, int nID);
//@备注 获取下一个兄弟项.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@返回 返回下一个兄弟项ID.
//@别名 列表树_取下一个兄弟项()
XC_API int WINAPI XTree_GetNextSiblingItem(HELE hEle, int nID);
//@备注 获取父项.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@返回 返回父项ID,错误返回-1.
//@别名 列表树_取父项()
XC_API int WINAPI XTree_GetParentItem(HELE hEle, int nID);
//@备注 创建数据适配器, 并且从绑定的项模板中扫描字段名, 初始化数据适配器的列(字段名);
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//若是没有设置项模板, 那么使用内置项模板;
//@参数 hEle 元素句柄.
//@返回 返回适配器句柄.
//@别名 列表树_创建数据适配器()
XC_API HXCGUI WINAPI XTree_CreateAdapter(HELE hEle);
//@备注 绑定数据适配器.  
//@参数 hEle 元素句柄.
//@参数 hAdapter 数据适配器句柄, XAdTree.
//@别名 列表树_绑定数据适配器()
XC_API void WINAPI XTree_BindAdapter(HELE hEle, HXCGUI hAdapter);
//@备注 获取数据适配器.  
//@参数 hEle 元素句柄.
//@返回 返回数据适配器句柄.
//@别名 列表树_取数据适配器()
XC_API HXCGUI WINAPI XTree_GetAdapter(HELE hEle);
//@备注 修改数据后,刷新所有项模板,以便更新数据到模板(如果项可见).  
//@参数 hEle 元素句柄.
//@别名 列表树_刷新数据()
XC_API void WINAPI XTree_RefreshData(HELE hEle);
//@备注 修改数据后,刷新指定项模板,以便更新数据到模板(如果当前项可见).  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@别名 列表树_刷新指定项()
XC_API void WINAPI XTree_RefreshItem(HELE hEle, int nID);
//@备注 设置缩进大小.  
//@参数 hEle 元素句柄.
//@参数 nWidth 缩进宽度.
//@别名 列表树_置缩进()
XC_API void WINAPI XTree_SetIndentation(HELE hEle, int nWidth);
//@备注 获取缩进值.  
//@参数 hEle 元素句柄.
//@返回 返回缩进值大小.
//@别名 列表树_取缩进()
XC_API int WINAPI XTree_GetIndentation(HELE hEle);
//@备注 设置项默认高度.  
//@参数 hEle 元素句柄.
//@参数 nHeight 高度.
//@参数 nSelHeight 选中时高度.
//@别名 列表树_置项默认高度()
XC_API void WINAPI XTree_SetItemHeightDefault(HELE hEle, int nHeight, int nSelHeight);
//@备注 获取项默认高度.  
//@参数 hEle 元素句柄.
//@参数 pHeight 接收返回高度.
//@参数 pSelHeight 接收返回值,当项选中时的高度.
//@别名 列表树_取项默认高度()
XC_API void WINAPI XTree_GetItemHeightDefault(HELE hEle, int* pHeight, int* pSelHeight);
//@备注 设置指定项高度.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@参数 nHeight 高度.
//@参数 nSelHeight 选中时高度.
//@别名 列表树_置项高度()
XC_API void WINAPI XTree_SetItemHeight(HELE hEle, int nID, int nHeight, int nSelHeight);
//@备注 获取指定项高度.  
//@参数 hEle 元素句柄.
//@参数 nID 项ID.
//@参数 pHeight 接收返回高度.
//@参数 pSelHeight 接收返回值,当项选中时的高度.
//@别名 列表树_取项高度()
XC_API void WINAPI XTree_GetItemHeight(HELE hEle, int nID, int* pHeight, int* pSelHeight);
//@备注 设置行间隔大小.  
//@参数 hEle 元素句柄.
//@参数 nSpace 行间隔大小.
//@别名 列表树_置行间距()
XC_API void WINAPI XTree_SetRowSpace(HELE hEle, int nSpace);
//@备注 获取行间距大小.  
//@参数 hEle 元素句柄.
//@返回 返回行间距大小.
//@别名 列表树_取行间距()
XC_API int WINAPI XTree_GetRowSpace(HELE hEle);
//@备注 移动项的位置.  
//@参数 hEle 元素句柄.
//@参数 nMoveItem 要移动的项ID.
//@参数 nDestItem 目标项ID, 参照位置.
//@参数 nFlag 0:目标前面, 1:目标后面, 2:目标子项首, 3:目标子项尾
//@返回 成功返回TRUE否则返回FALSE.
//@别名 列表树_移动项()
XC_API BOOL WINAPI XTree_MoveItem(HELE hEle, int nMoveItem, int nDestItem, int nFlag);
//@备注 通过模板项ID,获取实例化模板项ID对应的对象句柄.  
//@参数 hEle 元素句柄.
//@参数 nID 树项ID.
//@参数 nTempItemID 模板项ID.
//@返回 成功返回对象句柄,否则返回NULL.
//@别名 列表树_取模板对象()
XC_API HXCGUI WINAPI XTree_GetTemplateObject(HELE hEle, int nID, int nTempItemID);
//@备注 获取当前对象所在模板实例,属于列表树中哪一个项.  
//@参数 hEle 元素句柄.
//@参数 hXCGUI 对象句柄, UI元素句柄或形状对象句柄..
//@返回 成功返回项ID, 否则返回@ref XC_ID_ERROR.
//@别名 列表树_取对象所在项()
XC_API int WINAPI XTree_GetItemIDFromHXCGUI(HELE hEle, HXCGUI hXCGUI);
//@备注 首次添加数据时, 需要先创建数据适配器 XTree_CreateAdapter() \ 列表树_创建数据适配器()
//@参数 hEle 
//@参数 pValue 
//@参数 nParentID 父ID; 如果插入到目标位置, 此参数忽略(可填任意值)
//@参数 insertID 
//@别名 列表树_插入项文本()
XC_API int WINAPI XTree_InsertItemText(HELE hEle, const wchar_t* pValue, int nParentID=XC_ID_ROOT, int insertID=XC_ID_LAST);
//@备注 首次添加数据时, 需要先创建数据适配器 XTree_CreateAdapter() \ 列表树_创建数据适配器()
//@参数 hEle 
//@参数 pName 
//@参数 pValue 
//@参数 nParentID 父ID; 如果插入到目标位置, 此参数忽略(可填任意值)
//@参数 insertID 
//@别名 列表树_插入项文本扩展()
XC_API int WINAPI XTree_InsertItemTextEx(HELE hEle, const wchar_t* pName, const wchar_t* pValue, int nParentID=XC_ID_ROOT, int insertID=XC_ID_LAST);
//@备注 首次添加数据时, 需要先创建数据适配器 XTree_CreateAdapter() \ 列表树_创建数据适配器()
//@参数 hEle 
//@参数 hImage 
//@参数 nParentID 父ID; 如果插入到目标位置, 此参数忽略(可填任意值)
//@参数 insertID 
//@别名 列表树_插入项图片()
XC_API int WINAPI XTree_InsertItemImage(HELE hEle, HIMAGE hImage, int nParentID=XC_ID_ROOT, int insertID=XC_ID_LAST);
//@备注 首次添加数据时, 需要先创建数据适配器 XTree_CreateAdapter() \ 列表树_创建数据适配器()
//@参数 hEle 
//@参数 pName 
//@参数 hImage 
//@参数 nParentID 父ID; 如果插入到目标位置, 此参数忽略(可填任意值)
//@参数 insertID 
//@别名 列表树_插入项图片扩展()
XC_API int WINAPI XTree_InsertItemImageEx(HELE hEle, const wchar_t* pName, HIMAGE hImage, int nParentID=XC_ID_ROOT, int insertID=XC_ID_LAST);
//@参数 hEle 
//@别名 列表树_取项数量()
XC_API int WINAPI XTree_GetCount(HELE hEle);
//@参数 hEle 
//@别名 列表树_取列数量()
XC_API int WINAPI XTree_GetCountColumn(HELE hEle);
//@参数 hEle 
//@参数 nID 
//@参数 iColumn 
//@参数 pValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表树_置项文本()
XC_API BOOL WINAPI XTree_SetItemText(HELE hEle, int nID, int iColumn, const wchar_t* pValue);
//@参数 hEle 
//@参数 nID 
//@参数 pName 
//@参数 pValue 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表树_置项文本扩展()
XC_API BOOL WINAPI XTree_SetItemTextEx(HELE hEle, int nID, const wchar_t* pName, const wchar_t* pValue);
//@参数 hEle 
//@参数 nID 
//@参数 iColumn 
//@参数 hImage 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表树_置项图片()
XC_API BOOL WINAPI XTree_SetItemImage(HELE hEle, int nID, int iColumn, HIMAGE hImage);
//@参数 hEle 
//@参数 nID 
//@参数 pName 
//@参数 hImage 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表树_置项图片扩展()
XC_API BOOL WINAPI XTree_SetItemImageEx(HELE hEle, int nID, const wchar_t* pName, HIMAGE hImage);
//@参数 hEle 
//@参数 nID 
//@参数 iColumn 
//@返回 返回文本内容
//@别名 列表树_取项文本()
XC_API const wchar_t* WINAPI XTree_GetItemText(HELE hEle, int nID, int iColumn);
//@参数 hEle 
//@参数 nID 
//@参数 pName 
//@返回 返回文本内容
//@别名 列表树_取项文本扩展()
XC_API const wchar_t* WINAPI XTree_GetItemTextEx(HELE hEle, int nID, const wchar_t* pName);
//@参数 hEle 
//@参数 nID 
//@参数 iColumn 
//@别名 列表树_取项图片()
XC_API HIMAGE WINAPI XTree_GetItemImage(HELE hEle, int nID, int iColumn);
//@参数 hEle 
//@参数 nID 
//@参数 pName 
//@别名 列表树_取项图片扩展()
XC_API HIMAGE WINAPI XTree_GetItemImageEx(HELE hEle, int nID, const wchar_t* pName);
//@备注 XAdTree_DeleteItem  
//@参数 hEle 
//@参数 nID 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 列表树_删除项()
XC_API BOOL WINAPI XTree_DeleteItem(HELE hEle, int nID);
//@备注 XAdTree_DeleteItemAll  
//@参数 hEle 
//@别名 列表树_删除全部项()
XC_API void WINAPI XTree_DeleteItemAll(HELE hEle);
//@备注 XAdTree_DeleteColumnAll  
//@参数 hEle 
//@别名 列表树_删除列全部()
XC_API void WINAPI XTree_DeleteColumnAll(HELE hEle);
//@分组}
//@分组{  日期时间

//@备注 创建日期时间元素  
//@参数 x x坐标
//@参数 y y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄.
//@别名 日期_创建()
XC_API HELE WINAPI XDateTime_Create(int x, int y, int cx, int cy, HXCGUI hParent);
//@备注 设置样式.  
//@参数 hEle 元素句柄.
//@参数 nStyle 样式, 0为日期元素,1为时间元素.
//@别名 日期_置样式()
XC_API void WINAPI XDateTime_SetStyle(HELE hEle, int nStyle);
//@备注 获取样式.  
//@参数 hEle 元素句柄.
//@返回 元素样式.
//@别名 日期_取样式()
XC_API int WINAPI XDateTime_GetStyle(HELE hEle);
//@备注 切换分割栏为:斜线或横线.  
//@参数 hEle 元素句柄.
//@参数 bSlash TRUE:斜线, FALSE:横线.
//@别名 日期_启用分割栏为斜线()
XC_API void WINAPI XDateTime_EnableSplitSlash(HELE hEle, BOOL bSlash);
//@备注 获取内部按钮元素.  
//@参数 hEle 元素句柄.
//@参数 nType 按钮类型, 0:日历下拉按钮, 1:上箭头按钮, 2:下箭头按钮
//@返回 元素样式.
//@别名 日期_取内部按钮()
XC_API HELE WINAPI XDateTime_GetButton(HELE hEle, int nType);
//@备注 获取被选择文字的背景颜色.  
//@参数 hEle 元素句柄.
//@返回 元素样式.
//@别名 日期_取选择日期背景颜色()
XC_API COLORREF WINAPI XDateTime_GetSelBkColor(HELE hEle);
//@备注 设置被选择文字的背景颜色.  
//@参数 hEle 元素句柄.
//@参数 crSelectBk 文字被选中背景色, 颜色值, 请使用宏: RGBA()
//@别名 日期_置选择日期背景颜色()
XC_API void WINAPI XDateTime_SetSelBkColor(HELE hEle, COLORREF crSelectBk);
//@备注 获取当前日期.  
//@参数 hEle 元素句柄.
//@参数 pnYear 年.[OUT]
//@参数 pnMonth 月.[OUT]
//@参数 pnDay 日.[OUT]
//@别名 日期_取当前日期()
XC_API void WINAPI XDateTime_GetDate(HELE hEle, int* pnYear, int* pnMonth, int* pnDay);
//@备注 设置当前日期.  
//@参数 hEle 元素句柄.
//@参数 nYear 年.
//@参数 nMonth 月.
//@参数 nDay 日.
//@别名 日期_置当前日期()
XC_API void WINAPI XDateTime_SetDate(HELE hEle, int nYear, int nMonth, int nDay);
//@备注 获取当前时间.  
//@参数 hEle 元素句柄.
//@参数 pnHour 时.[OUT]
//@参数 pnMinute 分.[OUT]
//@参数 pnSecond 秒.[OUT]
//@别名 日期_取当前时间()
XC_API void WINAPI XDateTime_GetTime(HELE hEle, int* pnHour, int* pnMinute, int* pnSecond);
//@备注 设置当前时分秒.  
//@参数 hEle 元素句柄.
//@参数 nHour 时.
//@参数 nMinute 分.
//@参数 nSecond 秒.
//@别名 日期_置当前时间()
XC_API void WINAPI XDateTime_SetTime(HELE hEle, int nHour, int nMinute, int nSecond);
//@备注 弹出月历卡片
//@参数 hEle 元素句柄
//@别名 日期_弹出()
XC_API void WINAPI XDateTime_Popup(HELE hEle);
//@分组}
//@分组{  月历卡片

//@备注 创建日期时间元素  
//@参数 x x坐标
//@参数 y y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄.
//@别名 月历_创建()
XC_API HELE WINAPI XMonthCal_Create(int x, int y, int cx, int cy, HXCGUI hParent);
//@备注 获取内部按钮元素.  
//@参数 hEle 元素句柄.
//@参数 nType 按钮类型.
//@返回 元素样式.
//@别名 月历_取内部按钮()
XC_API HELE WINAPI XMonthCal_GetButton(HELE hEle, monthCal_button_type_ nType);
//@备注 设置月历当前年月日.  
//@参数 hEle 元素句柄.
//@参数 nYear 年.
//@参数 nMonth 月.
//@参数 nDay 日.
//@别名 月历_置当前日期()
XC_API void WINAPI XMonthCal_SetToday(HELE hEle, int nYear, int nMonth, int nDay);
//@备注 获取月历当前年月日.  
//@参数 hEle 元素句柄.
//@参数 pnYear 年.[INT,OUT]
//@参数 pnMonth 月.[INT,OUT]
//@参数 pnDay 日.[INT,OUT]
//@别名 月历_取当前日期()
XC_API void WINAPI XMonthCal_GetToday(HELE hEle, int* pnYear, int* pnMonth, int* pnDay);
//@备注 获取月历选中的年月日.  
//@参数 hEle 元素句柄.
//@参数 pnYear 年.[INT,OUT]
//@参数 pnMonth 月.[INT,OUT]
//@参数 pnDay 日.[INT,OUT]
//@别名 月历_取选择日期()
XC_API void WINAPI XMonthCal_GetSelDate(HELE hEle, int* pnYear, int* pnMonth, int* pnDay);
//@参数 hEle 元素句柄
//@参数 nFlag 1:周六,周日文字颜色, 2:日期文字的颜色;  其它周文字颜色, 使用元素自身颜色
//@参数 color 颜色值
//@别名 月历_置文本颜色()
XC_API void WINAPI XMonthCal_SetTextColor(HELE hEle, int nFlag, COLORREF color);
//@分组}
//@分组{  形状对象

//@备注 从父UI元素或窗口,和父布局对象中移除.  
//@参数 hShape 形状对象句柄.
//@别名 形状_移除()
XC_API void WINAPI XShape_RemoveShape(HXCGUI hShape);
//@备注 获取形状对象Z序.  
//@参数 hShape 形状对象句柄.
//@返回 成功返回索引值,否则返回 XC_ID_ERROR.
//@别名 形状_取Z序()
XC_API int WINAPI XShape_GetZOrder(HXCGUI hShape);
//@备注 重绘形状对象.  
//@参数 hShape 形状对象句柄.
//@别名 形状_重绘()
XC_API void WINAPI XShape_Redraw(HXCGUI hShape);
//@备注 获取内容宽度.  
//@参数 hShape 形状对象句柄.
//@返回 返回内容宽度.
//@别名 形状_取宽度()
XC_API int WINAPI XShape_GetWidth(HXCGUI hShape);
//@备注 获取内容高度.  
//@参数 hShape 形状对象句柄.
//@返回 返回内容高度.
//@别名 形状_取高度()
XC_API int WINAPI XShape_GetHeight(HXCGUI hShape);
//@备注 移动位置
//@参数 hShape 形状对象句柄
//@参数 x x坐标
//@参数 y y坐标
//@别名 形状_置位置()
XC_API void WINAPI XShape_SetPosition(HXCGUI hShape, int x, int y);
//@参数 hShape 形状对象句柄
//@参数 pOutX 返回X坐标
//@参数 pOutY 返回Y坐标
//@别名 形状_取位置()
XC_API void WINAPI XShape_GetPosition(HXCGUI hShape, int* pOutX, int* pOutY);
//@参数 hShape 形状对象句柄
//@参数 nWidth 宽度
//@参数 nHeight 高度
//@别名 形状_置大小()
XC_API void WINAPI XShape_SetSize(HXCGUI hShape, int nWidth, int nHeight);
//@参数 hShape 形状对象句柄
//@参数 pOutWidth 返回宽度
//@参数 pOutHeight 返回高度
//@别名 形状_取大小()
XC_API void WINAPI XShape_GetSize(HXCGUI hShape, int* pOutWidth, int* pOutHeight);
//@参数 hShape 形状对象句柄
//@参数 alpha 透明度
//@别名 形状_置透明度()
XC_API void WINAPI XShape_SetAlpha(HXCGUI hShape, BYTE alpha);
//@参数 hShape 形状对象句柄
//@返回 返回透明度
//@别名 形状_取透明度()
XC_API BYTE WINAPI XShape_GetAlpha(HXCGUI hShape);
//@备注 获取坐标.  
//@参数 hShape 形状对象句柄.
//@参数 pRect 接收返回坐标.
//@别名 形状_取坐标()
XC_API void WINAPI XShape_GetRect(HXCGUI hShape, RECT* pRect);
//@备注 设置坐标.  
//@参数 hShape 形状对象句柄.
//@参数 pRect 坐标.
//@别名 形状_置坐标()
XC_API void WINAPI XShape_SetRect(HXCGUI hShape, RECT* pRect);
//@备注 设置元素坐标,逻辑坐标,包含滚动视图偏移.  
//@参数 hShape 形状对象句柄.
//@参数 pRect 坐标.
//@参数 bRedraw 是否重绘.
//@返回 如果成功返回TRUE, 否则返回FALSE.
//@别名 形状_置逻辑坐标()
XC_API BOOL WINAPI XShape_SetRectLogic(HXCGUI hShape, RECT* pRect, BOOL bRedraw);
//@备注 获取元素坐标,逻辑坐标,包含滚动视图偏移.  
//@参数 hShape 形状对象句柄.
//@参数 pRect 坐标.
//@别名 形状_取逻辑坐标()
XC_API void WINAPI XShape_GetRectLogic(HXCGUI hShape, RECT* pRect);
//@备注 基于窗口客户区坐标.  
//@参数 hShape 形状对象句柄.
//@参数 pRect 坐标.
//@别名 形状_取基于窗口客户区坐标()
XC_API void WINAPI XShape_GetWndClientRect(HXCGUI hShape, RECT* pRect);
//@备注 仅计算有效内容, 填充父, 权重依赖父级所以无法计算.  
//@参数 hShape 形状对象句柄.
//@参数 pSize 接收返回内容大小值.
//@别名 形状_取内容大小()
XC_API void WINAPI XShape_GetContentSize(HXCGUI hShape, SIZE* pSize);
//@备注 是否显示布局边界.  
//@参数 hShape 形状对象句柄.
//@参数 bShow 是否显示.
//@别名 形状_显示布局边界()
XC_API void WINAPI XShape_ShowLayout(HXCGUI hShape, BOOL bShow);
//@备注 调整布局.  
//@参数 hShape 形状对象句柄.
//@别名 形状_调整布局()
XC_API void WINAPI XShape_AdjustLayout(HXCGUI hShape);
//@备注 销毁形状对象.  
//@参数 hShape 形状对象句柄.
//@别名 形状_销毁()
XC_API void WINAPI XShape_Destroy(HXCGUI hShape);
//@分组}
//@分组{  形状文本

//@备注 创建形状对象文本.  
//@参数 x X坐标.
//@参数 y Y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 pName 文本内容.
//@参数 hParent 父对象句柄.
//@返回 返回文本块句柄.
//@别名 形状文本_创建()
XC_API HXCGUI WINAPI XShapeText_Create(int x, int y, int cx, int cy, const wchar_t* pName, HXCGUI hParent=NULL);
//@备注 设置文本内容.  
//@参数 hTextBlock 形状对象文本句柄.
//@参数 pName 文本内容.
//@别名 形状文本_置文本()
XC_API void WINAPI XShapeText_SetText(HXCGUI hTextBlock, const wchar_t* pName);
//@备注 获取文本内容.  
//@参数 hTextBlock 形状对象文本句柄.
//@别名 形状文本_取文本()
XC_API const wchar_t* WINAPI XShapeText_GetText(HXCGUI hTextBlock);
//@备注 获取文本长度.  
//@参数 hTextBlock 形状对象文本句柄.
//@返回 文本长度.
//@别名 形状文本_取文本长度()
XC_API int WINAPI XShapeText_GetTextLength(HXCGUI hTextBlock);
//@备注 设置字体.  
//@参数 hTextBlock 形状对象文本句柄.
//@参数 hFontx 字体句柄.
//@别名 形状文本_置字体()
XC_API void WINAPI XShapeText_SetFont(HXCGUI hTextBlock, HFONTX hFontx);
//@备注 获取字体.  
//@参数 hTextBlock 形状对象文本句柄.
//@返回 返回字体句柄.
//@别名 形状文本_取字体()
XC_API HFONTX WINAPI XShapeText_GetFont(HXCGUI hTextBlock);
//@备注 设置文本颜色.  
//@参数 hTextBlock 形状对象文本句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 形状文本_置文本颜色()
XC_API void WINAPI XShapeText_SetTextColor(HXCGUI hTextBlock, COLORREF color);
//@备注 获取文本颜色.  
//@参数 hTextBlock 形状对象文本句柄.
//@返回 颜色值.
//@别名 形状文本_取文本颜色()
XC_API COLORREF WINAPI XShapeText_GetTextColor(HXCGUI hTextBlock);
//@备注 设置文本对齐方式.  
//@参数 hTextBlock 形状对象文本句柄.
//@参数 align 文本对齐方式; @ref textFormatFlag_ .
//@别名 形状文本_置文本对齐()
XC_API void WINAPI XShapeText_SetTextAlign(HXCGUI hTextBlock, int align);
//@备注 设置内容偏移.  
//@参数 hTextBlock 形状对象文本句柄.
//@参数 x X坐标.
//@参数 y Y坐标.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 形状文本_置偏移()
XC_API void WINAPI XShapeText_SetOffset(HXCGUI hTextBlock, int x, int y);
//@分组}
//@分组{  形状图片

//@备注 创建形状对象-图片.  
//@参数 x x坐标.
//@参数 y y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父对象句柄.
//@返回 成功返回图片对象句柄,否则返回NULL.
//@别名 形状图片_创建()
XC_API HXCGUI WINAPI XShapePic_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 设置图片.  
//@参数 hShape 形状对象句柄.
//@参数 hImage 图片句柄.
//@别名 形状图片_置图片()
XC_API void WINAPI XShapePic_SetImage(HXCGUI hShape, HIMAGE hImage);
//@备注 获取图片句柄  
//@参数 hShape 形状对象句柄
//@返回 返回图片句柄.
//@别名 形状图片_取图片()
XC_API HIMAGE WINAPI XShapePic_GetImage(HXCGUI hShape);
//@分组}
//@分组{  形状GIF

//@备注 创建形状对象GIF.  
//@参数 x X坐标.
//@参数 y Y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父对象句柄.
//@返回 成功返回形状对象GIF句柄,否则返回NULL.
//@别名 形状GIF_创建()
XC_API HXCGUI WINAPI XShapeGif_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 设置GIF图片.  
//@参数 hShape 形状对象句柄.
//@参数 hImage 图片句柄.
//@别名 形状GIF_置图片()
XC_API void WINAPI XShapeGif_SetImage(HXCGUI hShape, HIMAGE hImage);
//@备注 获取图片句柄.  
//@参数 hShape 形状对象句柄
//@返回 返回图片句柄.
//@别名 形状GIF_取图片()
XC_API HIMAGE WINAPI XShapeGif_GetImage(HXCGUI hShape);
//@分组}
//@分组{  形状矩形

//@备注 创建矩形形状对象.  
//@参数 x X坐标.
//@参数 y Y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父对象句柄.
//@返回 返回句柄.
//@别名 形状矩形_创建()
XC_API HXCGUI WINAPI XShapeRect_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 设置边框颜色.  
//@参数 hShape 形状对象句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 形状矩形_置边框色()
XC_API void WINAPI XShapeRect_SetBorderColor(HXCGUI hShape, COLORREF color);
//@备注 设置填充颜色.  
//@参数 hShape 形状对象句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 形状矩形_置填充色()
XC_API void WINAPI XShapeRect_SetFillColor(HXCGUI hShape, COLORREF color);
//@备注 设置圆角大小.  
//@参数 hShape 形状对象句柄.
//@参数 nWidth 圆角宽度.
//@参数 nHeight 圆角高度.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 形状矩形_置圆角大小()
XC_API void WINAPI XShapeRect_SetRoundAngle(HXCGUI hShape, int nWidth, int nHeight);
//@备注 获取圆角大小.  
//@参数 hShape 形状对象句柄.
//@参数 pWidth 圆角宽度.
//@参数 pHeight 圆角高度.
//@别名 形状矩形_取圆角大小()
XC_API void WINAPI XShapeRect_GetRoundAngle(HXCGUI hShape, int* pWidth, int* pHeight);
//@备注 启用绘制矩形边框.  
//@参数 hShape 形状对象句柄.
//@参数 bEnable 是否启用.
//@别名 形状矩形_启用边框()
XC_API void WINAPI XShapeRect_EnableBorder(HXCGUI hShape, BOOL bEnable);
//@备注 启用填充矩形.  
//@参数 hShape 形状对象句柄.
//@参数 bEnable 是否启用.
//@别名 形状矩形_启用填充()
XC_API void WINAPI XShapeRect_EnableFill(HXCGUI hShape, BOOL bEnable);
//@备注 启用圆角.  
//@参数 hShape 形状对象句柄.
//@参数 bEnable 是否启用.
//@别名 形状矩形_启用圆角()
XC_API void WINAPI XShapeRect_EnableRoundAngle(HXCGUI hShape, BOOL bEnable);
//@分组}
//@分组{  形状圆

//@备注 创建圆形形状对象.  
//@参数 x X坐标.
//@参数 y Y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父对象句柄.
//@返回 返回句柄.
//@别名 形状圆_创建()
XC_API HXCGUI WINAPI XShapeEllipse_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 设置边框颜色.  
//@参数 hShape 形状对象句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 形状圆_置边框色()
XC_API void WINAPI XShapeEllipse_SetBorderColor(HXCGUI hShape, COLORREF color);
//@备注 设置填充颜色.  
//@参数 hShape 形状对象句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 形状圆_置填充色()
XC_API void WINAPI XShapeEllipse_SetFillColor(HXCGUI hShape, COLORREF color);
//@备注 启用绘制圆边框.  
//@参数 hShape 形状对象句柄.
//@参数 bEnable 是否启用.
//@别名 形状圆_启用边框()
XC_API void WINAPI XShapeEllipse_EnableBorder(HXCGUI hShape, BOOL bEnable);
//@备注 启用填充圆.  
//@参数 hShape 形状对象句柄.
//@参数 bEnable 是否启用.
//@别名 形状圆_启用填充()
XC_API void WINAPI XShapeEllipse_EnableFill(HXCGUI hShape, BOOL bEnable);
//@分组}
//@分组{  形状组框

//@备注 创建组框形状对象.  
//@参数 x X坐标.
//@参数 y Y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 pName 名称.
//@参数 hParent 父对象句柄.
//@返回 返回句柄.
//@别名 形状组框_创建()
XC_API HXCGUI WINAPI XShapeGroupBox_Create(int x, int y, int cx, int cy, const wchar_t* pName, HXCGUI hParent=NULL);
//@备注 设置边框颜色.  
//@参数 hShape 形状对象句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 形状组框_置边框颜色()
XC_API void WINAPI XShapeGroupBox_SetBorderColor(HXCGUI hShape, COLORREF color);
//@备注 设置文本颜色.  
//@参数 hShape 形状对象句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 形状组框_置文本颜色()
XC_API void WINAPI XShapeGroupBox_SetTextColor(HXCGUI hShape, COLORREF color);
//@备注 设置字体.  
//@参数 hShape 形状对象句柄.
//@参数 hFontX 炫彩字体.
//@别名 形状组框_置字体()
XC_API void WINAPI XShapeGroupBox_SetFontX(HXCGUI hShape, HFONTX hFontX);
//@备注 设置文本偏移量.  
//@参数 hShape 形状对象句柄.
//@参数 offsetX 水平偏移.
//@参数 offsetY 垂直偏移.
//@别名 形状组框_置文本偏移()
XC_API void WINAPI XShapeGroupBox_SetTextOffset(HXCGUI hShape, int offsetX, int offsetY);
//@备注 设置圆角大小.  
//@参数 hShape 形状对象句柄.
//@参数 nWidth 圆角宽度.
//@参数 nHeight 圆角高度.
//@别名 形状组框_置圆角大小()
XC_API void WINAPI XShapeGroupBox_SetRoundAngle(HXCGUI hShape, int nWidth, int nHeight);
//@备注 设置文本内容.  
//@参数 hShape 形状对象句柄.
//@参数 pText 文本内容.
//@别名 形状组框_置文本()
XC_API void WINAPI XShapeGroupBox_SetText(HXCGUI hShape, const wchar_t* pText);
//@备注 获取文本内容.  
//@参数 hShape 形状对象句柄.
//@返回 返回文本内容.
//@别名 形状组框_取文本()
XC_API const wchar_t* WINAPI XShapeGroupBox_GetText(HXCGUI hShape);
//@备注 获取文本偏移量.  
//@参数 hShape 形状对象句柄.
//@参数 pOffsetX X坐标偏移量.
//@参数 pOffsetY Y坐标偏移量.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 形状组框_取文本偏移()
XC_API void WINAPI XShapeGroupBox_GetTextOffset(HXCGUI hShape, int* pOffsetX, int* pOffsetY);
//@备注 获取圆角大小.  
//@参数 hShape 形状对象句柄.
//@参数 pWidth 返回圆角宽度.
//@参数 pHeight 返回圆角高度.
//@别名 形状组框_取圆角大小()
XC_API void WINAPI XShapeGroupBox_GetRoundAngle(HXCGUI hShape, int* pWidth, int* pHeight);
//@备注 启用圆角.  
//@参数 hShape 形状对象句柄.
//@参数 bEnable 是否启用.
//@别名 形状组框_启用圆角()
XC_API void WINAPI XShapeGroupBox_EnableRoundAngle(HXCGUI hShape, BOOL bEnable);
//@分组}
//@分组{  表格

//@参数 x 按钮x坐标
//@参数 y 按钮y坐标
//@参数 cx 宽度
//@参数 cy 高度
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 表格句柄.
//@别名 表格_创建()
XC_API HXCGUI WINAPI XTable_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@参数 hShape 形状对象句柄
//@参数 nRow 行数量
//@参数 nCol 列数量
//@别名 表格_重置()
XC_API void WINAPI XTable_Reset(HXCGUI hShape, int nRow, int nCol);
//@备注 清空表格
//@参数 hShape 形状对象句柄
//@别名 表格_清空()
XC_API void WINAPI XTable_Clear(HXCGUI hShape);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 count 数量
//@别名 表格_组合行()
XC_API void WINAPI XTable_ComboRow(HXCGUI hShape, int iRow, int iCol, int count);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 count 数量
//@别名 表格_组合列()
XC_API void WINAPI XTable_ComboCol(HXCGUI hShape, int iRow, int iCol, int count);
//@参数 hShape 形状对象句柄
//@参数 iCol 列索引
//@参数 width 宽度
//@别名 表格_置列宽()
XC_API void WINAPI XTable_SetColWidth(HXCGUI hShape, int iCol, int width);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 height 高度
//@别名 表格_置行高()
XC_API void WINAPI XTable_SetRowHeight(HXCGUI hShape, int iRow, int height);
//@参数 hShape 形状对象句柄
//@参数 color 颜色
//@别名 表格_置边颜色()
XC_API void WINAPI XTable_SetBorderColor(HXCGUI hShape, COLORREF color);
//@参数 hShape 形状对象句柄
//@参数 color 颜色
//@别名 表格_置文本颜色()
XC_API void WINAPI XTable_SetTextColor(HXCGUI hShape, COLORREF color);
//@参数 hShape 形状对象句柄
//@参数 hFont 字体句柄
//@别名 表格_置字体()
XC_API void WINAPI XTable_SetFont(HXCGUI hShape, HFONTX hFont);
//@参数 hShape 形状对象句柄
//@参数 leftSize 内填充大小
//@参数 topSize 内填充大小
//@参数 rightSize 内填充大小
//@参数 bottomSize 内填充大小
//@别名 表格_置项内填充()
XC_API void WINAPI XTable_SetItemPadding(HXCGUI hShape, int leftSize, int topSize, int rightSize, int bottomSize);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 pText 文本
//@别名 表格_置项文本()
XC_API void WINAPI XTable_SetItemText(HXCGUI hShape, int iRow, int iCol, const wchar_t* pText);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 pText 文本
//@参数 textColor 文本颜色
//@参数 bkColor 背景颜色
//@参数 bTextColor 是否使用文本颜色
//@参数 bBkColor 是否使用背景颜色
//@参数 hFont 字体
//@别名 表格_置项文本扩展()
XC_API void WINAPI XTable_SetItemTextEx(HXCGUI hShape, int iRow, int iCol, const wchar_t* pText, COLORREF textColor, COLORREF bkColor, BOOL bTextColor=TRUE, BOOL bBkColor=TRUE, HFONTX hFont=NULL);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 hFont 字体句柄
//@别名 表格_置项字体()
XC_API void WINAPI XTable_SetItemFont(HXCGUI hShape, int iRow, int iCol, HFONTX hFont);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 nAlign 对齐方式  @ref textFormatFlag_
//@别名 表格_置项文本对齐()
XC_API void WINAPI XTable_SetItemTextAlign(HXCGUI hShape, int iRow, int iCol, int nAlign);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 color 颜色
//@参数 bColor 是否使用
//@别名 表格_置项文本色()
XC_API void WINAPI XTable_SetItemTextColor(HXCGUI hShape, int iRow, int iCol, COLORREF color, BOOL bColor);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 color 颜色
//@参数 bColor 是否使用
//@别名 表格_置项背景色()
XC_API void WINAPI XTable_SetItemBkColor(HXCGUI hShape, int iRow, int iCol, COLORREF color, BOOL bColor);
//@参数 hShape 形状对象句柄
//@参数 iRow1 行索引1
//@参数 iCol1 列索引1
//@参数 iRow2 行索引2
//@参数 iCol2 列索引2
//@参数 nFlag 标识  @ref  table_line_flag_
//@参数 color 颜色
//@别名 表格_置项线()
XC_API void WINAPI XTable_SetItemLine(HXCGUI hShape, int iRow1, int iCol1, int iRow2, int iCol2, int nFlag, COLORREF color);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 flag 标识   @ref table_flag_
//@别名 表格_置项标识()
XC_API void WINAPI XTable_SetItemFlag(HXCGUI hShape, int iRow, int iCol, int flag);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@返回 返回字符串指针
//@别名 表格_取项文本()
XC_API const wchar_t* XTable_GetItemText(HXCGUI hShape, int iRow, int iCol);
//@参数 hShape 形状对象句柄
//@参数 iRow 行索引
//@参数 iCol 列索引
//@参数 pRect 接收返回坐标
//@返回 成功返回TRUE,否则返回FALSE
//@别名 表格_取项坐标()
XC_API BOOL WINAPI XTable_GetItemRect(HXCGUI hShape, int iRow, int iCol, RECT* pRect);
//@分组}
//@分组{  数据适配器

//@参数 hAdapter 数据适配器句柄
//@返回 返回当前引用计数
//@别名 数据适配器_增加引用计数()
XC_API int WINAPI XAd_AddRef(HXCGUI hAdapter);
//@参数 hAdapter 数据适配器句柄
//@返回 返回当前引用计数
//@别名 数据适配器_释放引用计数()
XC_API int WINAPI XAd_Release(HXCGUI hAdapter);
//@参数 hAdapter 数据适配器句柄
//@返回 返回当前引用计数
//@别名 数据适配器_取引用计数()
XC_API int WINAPI XAd_GetRefCount(HXCGUI hAdapter);
//@备注 数据适配器销毁 
//@参数 hAdapter 数据适配器句柄
//@别名 数据适配器_销毁()
XC_API void WINAPI XAd_Destroy(HXCGUI hAdapter);
//@参数 hAdapter 数据适配器句柄
//@参数 bEnable 是否启用
//@别名 数据适配器_启用自动销毁()
XC_API void WINAPI XAd_EnableAutoDestroy(HXCGUI hAdapter, BOOL bEnable);
//@分组}
//@分组{  数据适配器列表视图

//@备注 创建列表视元素数据适配器.  
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//@返回 返回数据适配器句柄.
//@别名 数据适配器列表视_创建()
XC_API HXCGUI WINAPI XAdListView_Create();
//@备注 组操作,添加数据列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@返回 返回列索引.
//@别名 数据适配器列表视_组添加列()
XC_API int WINAPI XAdListView_Group_AddColumn(HXCGUI hAdapter, const wchar_t* pName);
//@备注 组操作,添加组,数据默认填充到第一列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pValue 值.
//@参数 iPos 插入位置索引, -1添加到末尾
//@返回 返回组索引.
//@别名 数据适配器列表视_添加组文本()
XC_API int WINAPI XAdListView_Group_AddItemText(HXCGUI hAdapter, const wchar_t* pValue, int iPos=-1);
//@备注 组操作,添加组,数据填充指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@参数 pValue 值.
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回组索引.
//@别名 数据适配器列表视_添加组文本扩展()
XC_API int WINAPI XAdListView_Group_AddItemTextEx(HXCGUI hAdapter, const wchar_t* pName, const wchar_t* pValue, int iPos=-1);
//@备注 组操作,添加组,数据默认填充第一列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 hImage 图片句柄.
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回组索引.
//@别名 数据适配器列表视_添加组图片()
XC_API int WINAPI XAdListView_Group_AddItemImage(HXCGUI hAdapter, HIMAGE hImage, int iPos=-1);
//@备注 组操作,添加组,数据填充指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@参数 hImage 图片句柄.
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回组索引.
//@别名 数据适配器列表视_添加组图片扩展()
XC_API int WINAPI XAdListView_Group_AddItemImageEx(HXCGUI hAdapter, const wchar_t* pName, HIMAGE hImage, int iPos=-1);
//@备注 组操作,设置指定项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iColumn 列索引.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_组设置文本()
XC_API BOOL WINAPI XAdListView_Group_SetText(HXCGUI hAdapter, int iGroup, int iColumn, const wchar_t* pValue);
//@备注 组操作,设置指定项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 pName 字段名.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_组设置文本扩展()
XC_API BOOL WINAPI XAdListView_Group_SetTextEx(HXCGUI hAdapter, int iGroup, const wchar_t* pName, const wchar_t* pValue);
//@备注 组操作,设置指定项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iColumn 列索引.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_组设置图片()
XC_API BOOL WINAPI XAdListView_Group_SetImage(HXCGUI hAdapter, int iGroup, int iColumn, HIMAGE hImage);
//@备注 组操作,设置指定项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 pName 字段名.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_组设置图片扩展()
XC_API BOOL WINAPI XAdListView_Group_SetImageEx(HXCGUI hAdapter, int iGroup, const wchar_t* pName, HIMAGE hImage);
//@参数 hAdapter 数据适配器句柄
//@参数 iGroup 组索引
//@参数 iColumn 列索引
//@返回 返回文本内容
//@别名 数据适配器列表视_组取文本()
XC_API const wchar_t* WINAPI XAdListView_Group_GetText(HXCGUI hAdapter, int iGroup, int iColumn);
//@参数 hAdapter 数据适配器句柄
//@参数 iGroup 组索引
//@参数 pName 字段名称
//@返回 返回文本内容
//@别名 数据适配器列表视_组取文本扩展()
XC_API const wchar_t* XAdListView_Group_GetTextEx(HXCGUI hAdapter, int iGroup, const wchar_t* pName);
//@参数 hAdapter 数据适配器句柄
//@参数 iGroup 组索引
//@参数 iColumn 列索引
//@返回 返回图片句柄
//@别名 数据适配器列表视_组取图片()
XC_API HIMAGE WINAPI XAdListView_Group_GetImage(HXCGUI hAdapter, int iGroup, int iColumn);
//@参数 hAdapter 数据适配器句柄
//@参数 iGroup 组索引
//@参数 pName 字段名称
//@返回 返回图片句柄
//@别名 数据适配器列表视_组取图片扩展()
XC_API HIMAGE WINAPI XAdListView_Group_GetImageEx(HXCGUI hAdapter, int iGroup, const wchar_t* pName);
//@备注 项操作,添加列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@返回 返回列索引.
//@别名 数据适配器列表视_项添加列()
XC_API int WINAPI XAdListView_Item_AddColumn(HXCGUI hAdapter, const wchar_t* pName);
//@备注 组操作,获取组数量.  
//@参数 hAdapter 数据适配器句柄.
//@返回 返回组数量.
//@别名 数据适配器列表视_取组数量()
XC_API int WINAPI XAdListView_Group_GetCount(HXCGUI hAdapter);
//@备注 项操作,获取指定组中项数量.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@返回 成功返回项数量,否则返回 @ref XC_ID_ERROR.
//@别名 数据适配器列表视_取组中项数量()
XC_API int WINAPI XAdListView_Item_GetCount(HXCGUI hAdapter, int iGroup);
//@备注 项操作,添加项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 pValue 值.
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回项索引.
//@别名 数据适配器列表视_添加项文本()
XC_API int WINAPI XAdListView_Item_AddItemText(HXCGUI hAdapter, int iGroup, const wchar_t* pValue, int iPos=-1);
//@备注 项操作,添加项,数据填充指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 pName 字段名称.
//@参数 pValue 值.
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回项索引.
//@别名 数据适配器列表视_添加项文本扩展()
XC_API int WINAPI XAdListView_Item_AddItemTextEx(HXCGUI hAdapter, int iGroup, const wchar_t* pName, const wchar_t* pValue, int iPos=-1);
//@备注 项操作,添加项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 hImage 图片句柄.
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回项索引.
//@别名 数据适配器列表视_添加项图片()
XC_API int WINAPI XAdListView_Item_AddItemImage(HXCGUI hAdapter, int iGroup, HIMAGE hImage, int iPos=-1);
//@备注 项操作,添加项,填充指定列数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 pName 字段名称.
//@参数 hImage 图片句柄.
//@参数 iPos 插入位置, -1添加到末尾
//@返回 返回项索引.
//@别名 数据适配器列表视_添加项图片扩展()
XC_API int WINAPI XAdListView_Item_AddItemImageEx(HXCGUI hAdapter, int iGroup, const wchar_t* pName, HIMAGE hImage, int iPos=-1);
//@备注 删除组,自动删除子项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_组删除()
XC_API BOOL WINAPI XAdListView_Group_DeleteItem(HXCGUI hAdapter, int iGroup);
//@备注 删除指定组的所有子项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@别名 数据适配器列表视_删除全部子项()
XC_API void WINAPI XAdListView_Group_DeleteAllChildItem(HXCGUI hAdapter, int iGroup);
//@备注 删除项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_删除项()
XC_API BOOL WINAPI XAdListView_Item_DeleteItem(HXCGUI hAdapter, int iGroup, int iItem);
//@备注 删除所有(组,项,列).  
//@参数 hAdapter 数据适配器句柄.
//@别名 数据适配器列表视_删除全部()
XC_API void WINAPI XAdListView_DeleteAll(HXCGUI hAdapter);
//@备注 删除所有的组.  
//@参数 hAdapter 数据适配器句柄.
//@别名 数据适配器列表视_删除全部组()
XC_API void WINAPI XAdListView_DeleteAllGroup(HXCGUI hAdapter);
//@备注 删除所有的项.  
//@参数 hAdapter 数据适配器句柄.
//@别名 数据适配器列表视_删除全部项()
XC_API void WINAPI XAdListView_DeleteAllItem(HXCGUI hAdapter);
//@备注 删除组的列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iColumn 列索引.
//@别名 数据适配器列表视_组删除列()
XC_API void WINAPI XAdListView_DeleteColumnGroup(HXCGUI hAdapter, int iColumn);
//@备注 删除项的列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iColumn 列索引.
//@别名 数据适配器列表视_项删除列()
XC_API void WINAPI XAdListView_DeleteColumnItem(HXCGUI hAdapter, int iColumn);
//@备注 项操作,获取项文本内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 iColumn 列索引.
//@返回 返回文本内容
//@别名 数据适配器列表视_项取文本()
XC_API const wchar_t* WINAPI XAdListView_Item_GetText(HXCGUI hAdapter, int iGroup, int iItem, int iColumn);
//@备注 项操作,获取项文本内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 pName 字段名称.
//@返回 返回文本内容
//@别名 数据适配器列表视_项取文本扩展()
XC_API const wchar_t* WINAPI XAdListView_Item_GetTextEx(HXCGUI hAdapter, int iGroup, int iItem, const wchar_t* pName);
//@备注 项操作,获取项图片句柄.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 iColumn 列索引.
//@返回 返回图片句柄.
//@别名 数据适配器列表视_项取图片()
XC_API HIMAGE WINAPI XAdListView_Item_GetImage(HXCGUI hAdapter, int iGroup, int iItem, int iColumn);
//@备注 项操作,获取项图片句柄.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 pName 字段称.
//@返回 返回图片句柄.
//@别名 数据适配器列表视_项取图片扩展()
XC_API HIMAGE WINAPI XAdListView_Item_GetImageEx(HXCGUI hAdapter, int iGroup, int iItem, const wchar_t* pName);
//@备注 项操作,数据填充指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 iColumn 列索引.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_项置文本()
XC_API BOOL WINAPI XAdListView_Item_SetText(HXCGUI hAdapter, int iGroup, int iItem, int iColumn, const wchar_t* pValue);
//@备注 项操作,数据填充指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 pName 字段称.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_项置文本扩展()
XC_API BOOL WINAPI XAdListView_Item_SetTextEx(HXCGUI hAdapter, int iGroup, int iItem, const wchar_t* pName, const wchar_t* pValue);
//@备注 项操作,数据填充指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 iColumn 列索引.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_项置图片()
XC_API BOOL WINAPI XAdListView_Item_SetImage(HXCGUI hAdapter, int iGroup, int iItem, int iColumn, HIMAGE hImage);
//@备注 项操作,数据填充指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iGroup 组索引.
//@参数 iItem 项索引.
//@参数 pName 字段称.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器列表视_项置图片扩展()
XC_API BOOL WINAPI XAdListView_Item_SetImageEx(HXCGUI hAdapter, int iGroup, int iItem, const wchar_t* pName, HIMAGE hImage);
//@分组}
//@分组{  数据适配器表格

//@备注 创建列表框元素数据适配器.  
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//@返回 返回数据适配器句柄.
//@别名 数据适配器表_创建()
XC_API HXCGUI WINAPI XAdTable_Create();
//@备注 对内容进行排序.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iColumn 要排序的列索引。
//@参数 bAscending 是否按照升序方式排序.
//@别名 数据适配器表_排序()
XC_API void WINAPI XAdTable_Sort(HXCGUI hAdapter, int iColumn, BOOL bAscending);
//@备注 获取项数据类型.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@返回 返还项数据类型 @ref adapter_date_type_ .
//@别名 数据适配器表_取项数据类型()
XC_API adapter_date_type_ WINAPI XAdTable_GetItemDataType(HXCGUI hAdapter, int iRow, int iColumn);
//@备注 获取项数据类型.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@返回 返还项数据类型 @ref adapter_date_type_ .
//@别名 数据适配器表_取项数据类型扩展()
XC_API adapter_date_type_ WINAPI XAdTable_GetItemDataTypeEx(HXCGUI hAdapter, int iRow, const wchar_t* pName);
//@备注 添加数据列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段称.
//@返回 返回列索引, 失败返回-1.
//@别名 数据适配器表_添加列()
XC_API int WINAPI XAdTable_AddColumn(HXCGUI hAdapter, const wchar_t* pName);
//@备注 设置列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pColName 列名,多个列名用逗号分开.
//@返回 返回列数量.
//@别名 数据适配器表_置列()
XC_API int WINAPI XAdTable_SetColumn(HXCGUI hAdapter, const wchar_t* pColName);
//@备注 添加数据项,默认值放到第一列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pValue 值.
//@返回 返回行索引值.
//@别名 数据适配器表_添加行文本()
XC_API int WINAPI XAdTable_AddRowText(HXCGUI hAdapter, const wchar_t* pValue);
//@备注 添加数据项,填充指定列数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段称.
//@参数 pValue 值.
//@返回 返回行索引.
//@别名 数据适配器表_添加行文本扩展()
XC_API int WINAPI XAdTable_AddRowTextEx(HXCGUI hAdapter, const wchar_t* pName, const wchar_t* pValue);
//@备注 添加数据项,默认值放到第一列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 hImage 图片句柄.
//@返回 返回行索引值.
//@别名 数据适配器表_添加行图片()
XC_API int WINAPI XAdTable_AddRowImage(HXCGUI hAdapter, HIMAGE hImage);
//@备注 添加数据项,并填充指定列数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段称.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_添加行图片扩展()
XC_API int WINAPI XAdTable_AddRowImageEx(HXCGUI hAdapter, const wchar_t* pName, HIMAGE hImage);
//@备注 插入数据项,填充第一列数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 插入位置行索引.
//@参数 pValue 值.
//@返回 返回行索引.
//@别名 数据适配器表_插入行文本()
XC_API int WINAPI XAdTable_InsertRowText(HXCGUI hAdapter, int iRow, const wchar_t* pValue);
//@备注 插入数据项,并填充指定列数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 插入位置行索引.
//@参数 pName 字段称.
//@参数 pValue 值.
//@返回 返回行索引.
//@别名 数据适配器表_插入行文本扩展()
XC_API int WINAPI XAdTable_InsertRowTextEx(HXCGUI hAdapter, int iRow, const wchar_t* pName, const wchar_t* pValue);
//@备注 插入数据项,填充第一列数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 插入位置行索引.
//@参数 hImage 图片句柄.
//@返回 返回行索引.
//@别名 数据适配器表_插入行图片()
XC_API int WINAPI XAdTable_InsertRowImage(HXCGUI hAdapter, int iRow, HIMAGE hImage);
//@备注 插入数据项,并填充指定列数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 插入位置行索引.
//@参数 pName 字段称.
//@参数 hImage 图片句柄.
//@返回 返回行索引.
//@别名 数据适配器表_插入行图片扩展()
XC_API int WINAPI XAdTable_InsertRowImageEx(HXCGUI hAdapter, int iRow, const wchar_t* pName, HIMAGE hImage);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_置项文本()
XC_API BOOL WINAPI XAdTable_SetItemText(HXCGUI hAdapter, int iRow, int iColumn, const wchar_t* pValue);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_置项文本扩展()
XC_API BOOL WINAPI XAdTable_SetItemTextEx(HXCGUI hAdapter, int iRow, const wchar_t* pName, const wchar_t* pValue);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@参数 nValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_置项整数值()
XC_API BOOL WINAPI XAdTable_SetItemInt(HXCGUI hAdapter, int iRow, int iColumn, int nValue);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@参数 nValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_置项整数值扩展()
XC_API BOOL WINAPI XAdTable_SetItemIntEx(HXCGUI hAdapter, int iRow, const wchar_t* pName, int nValue);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@参数 fValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_置项浮点值()
XC_API BOOL WINAPI XAdTable_SetItemFloat(HXCGUI hAdapter, int iRow, int iColumn, float nValue);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@参数 fValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_置项浮点值扩展()
XC_API BOOL WINAPI XAdTable_SetItemFloatEx(HXCGUI hAdapter, int iRow, const wchar_t* pName, float nValue);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_置项图片()
XC_API BOOL WINAPI XAdTable_SetItemImage(HXCGUI hAdapter, int iRow, int iColumn, HIMAGE hImage);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_置项图片扩展()
XC_API BOOL WINAPI XAdTable_SetItemImageEx(HXCGUI hAdapter, int iRow, const wchar_t* pName, HIMAGE hImage);
//@备注 删除行.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_删除行()
XC_API BOOL WINAPI XAdTable_DeleteRow(HXCGUI hAdapter, int iRow);
//@备注 删除多个项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行起始索引.
//@参数 nCount 删除行数量.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_删除行扩展()
XC_API BOOL WINAPI XAdTable_DeleteRowEx(HXCGUI hAdapter, int iRow, int nCount);
//@备注 删除所有项.  
//@参数 hAdapter 数据适配器句柄.
//@别名 数据适配器表_删除行全部()
XC_API void WINAPI XAdTable_DeleteRowAll(HXCGUI hAdapter);
//@备注 删除所有列,并且清空所有数据.  
//@参数 hAdapter 数据适配器句柄.
//@别名 数据适配器表_删除列全部()
XC_API void WINAPI XAdTable_DeleteColumnAll(HXCGUI hAdapter);
//@备注 获取行数量.  
//@参数 hAdapter 数据适配器句柄.
//@返回 返回行数量.
//@别名 数据适配器表_取行数量()
XC_API int WINAPI XAdTable_GetCountRow(HXCGUI hAdapter);
//@备注 获取列数量.  
//@参数 hAdapter 数据适配器句柄.
//@返回 返回列数量.
//@别名 数据适配器表_取列数量()
XC_API int WINAPI XAdTable_GetCountColumn(HXCGUI hAdapter);
//@备注 获取项文本内容.  
//@参数 hAdapter 数据适配器句柄
//@参数 iRow 行索引
//@参数 iColumn 列索引
//@返回 返回文本内容
//@别名 数据适配器表_取项文本()
XC_API const wchar_t* WINAPI XAdTable_GetItemText(HXCGUI hAdapter, int iRow, int iColumn);
//@备注 获取项文本内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@返回 返回文本内容
//@别名 数据适配器表_取项文本扩展()
XC_API const wchar_t* WINAPI XAdTable_GetItemTextEx(HXCGUI hAdapter, int iRow, const wchar_t* pName);
//@备注 获取项图片句柄.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@返回 返回图片句柄.
//@别名 数据适配器表_取项图片()
XC_API HIMAGE WINAPI XAdTable_GetItemImage(HXCGUI hAdapter, int iRow, int iColumn);
//@备注 获取项图片句柄.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@返回 返回图片句柄.
//@别名 数据适配器表_取项图片扩展()
XC_API HIMAGE WINAPI XAdTable_GetItemImageEx(HXCGUI hAdapter, int iRow, const wchar_t* pName);
//@备注 获取项值.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@参数 pOutValue 接收返还值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_取项整数值()
XC_API BOOL WINAPI XAdTable_GetItemInt(HXCGUI hAdapter, int iRow, int iColumn, int* pOutValue);
//@备注 获取项值.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@参数 pOutValue 接收返还值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_取项整数值扩展()
XC_API BOOL WINAPI XAdTable_GetItemIntEx(HXCGUI hAdapter, int iRow, const wchar_t* pName, int* pOutValue);
//@备注 获取项值.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 iColumn 列索引.
//@参数 pOutValue 接收返还值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_取项浮点值()
XC_API BOOL WINAPI XAdTable_GetItemFloat(HXCGUI hAdapter, int iRow, int iColumn, float* pOutValue);
//@备注 获取项值.  
//@参数 hAdapter 数据适配器句柄.
//@参数 iRow 行索引.
//@参数 pName 字段称.
//@参数 pOutValue 接收返还值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器表_取项浮点值扩展()
XC_API BOOL WINAPI XAdTable_GetItemFloatEx(HXCGUI hAdapter, int iRow, const wchar_t* pName, float* pOutValue);
//@分组}
//@分组{  数据适配器MAP

//@备注 创建数据适配器,单列数据.  
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//@返回 返回数据适配器句柄.
//@别名 数据适配器MAP_创建()
XC_API HXCGUI WINAPI XAdMap_Create();
//@备注 增加数据项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器MAP_添加项文本()
XC_API BOOL WINAPI XAdMap_AddItemText(HXCGUI hAdapter, const wchar_t* pName, const wchar_t* pValue);
//@备注 增加数据项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器MAP_添加项图片()
XC_API BOOL WINAPI XAdMap_AddItemImage(HXCGUI hAdapter, const wchar_t* pName, HIMAGE hImage);
//@备注 删除数据项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器MAP_删除项()
XC_API BOOL WINAPI XAdMap_DeleteItem(HXCGUI hAdapter, const wchar_t* pName);
//@备注 获取项数量.  
//@参数 hAdapter 数据适配器句柄.
//@返回 返回项数量.
//@别名 数据适配器MAP_取项数量()
XC_API int WINAPI XAdMap_GetCount(HXCGUI hAdapter);
//@备注 获取项内容,如果内容为文本.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@返回 返回文本内容.
//@别名 数据适配器MAP_取项文本()
XC_API const wchar_t* WINAPI XAdMap_GetItemText(HXCGUI hAdapter, const wchar_t* pName);
//@备注 获取项内容,如果内容为图片句柄.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@返回 返回图片句柄.
//@别名 数据适配器MAP_取项图片()
XC_API HIMAGE WINAPI XAdMap_GetItemImage(HXCGUI hAdapter, const wchar_t* pName);
//@备注 设置项内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器MAP_置项文本()
XC_API BOOL WINAPI XAdMap_SetItemText(HXCGUI hAdapter, const wchar_t* pName, const wchar_t* pValue);
//@备注 设置项内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@参数 hImage 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器MAP_置项图片()
XC_API BOOL WINAPI XAdMap_SetItemImage(HXCGUI hAdapter, const wchar_t* pName, HIMAGE hImage);
//@分组}
//@分组{  数据适配器树

//@备注 创建树元素数据适配器.  
//数据适配器存储数据, UI对象根据绑定的字段名显示数据适配器中对应的数据;
//@返回 返回数据适配器句柄.
//@别名 数据适配器树_创建()
XC_API HXCGUI WINAPI XAdTree_Create();
//@备注 添加列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@返回 返回索引值.
//@别名 数据适配器树_添加列()
XC_API int WINAPI XAdTree_AddColumn(HXCGUI hAdapter, const wchar_t* pName);
//@备注 设置列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pColName 列名,多个列名用逗号分开.
//@返回 返回列数量.
//@别名 数据适配器树_置列()
XC_API int WINAPI XAdTree_SetColumn(HXCGUI hAdapter, const wchar_t* pColName);
//@备注 插入项,数据填充到第一列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pValue 值.
//@参数 nParentID 父ID; 如果插入到目标位置, 此参数忽略(可填任意值)
//@参数 insertID 插入位置ID.
//@返回 返回项ID值.
//@别名 数据适配器树_插入项文本()
XC_API int WINAPI XAdTree_InsertItemText(HXCGUI hAdapter, const wchar_t* pValue, int nParentID=XC_ID_ROOT, int insertID=XC_ID_LAST);
//@备注 插入项,数据填充到指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@参数 pValue 值.
//@参数 nParentID 父ID; 如果插入到目标位置, 此参数忽略(可填任意值)
//@参数 insertID 插入位置ID.
//@返回 返回项ID值.
//@别名 数据适配器树_插入项文本扩展()
XC_API int WINAPI XAdTree_InsertItemTextEx(HXCGUI hAdapter, const wchar_t* pName, const wchar_t* pValue, int nParentID=XC_ID_ROOT, int insertID=XC_ID_LAST);
//@备注 插入项,数据填充到第一列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 hImage 图片句柄.
//@参数 nParentID 父ID; 如果插入到目标位置, 此参数忽略(可填任意值)
//@参数 insertID 插入位置ID.
//@返回 返回项ID值.
//@别名 数据适配器树_插入项图片()
XC_API int WINAPI XAdTree_InsertItemImage(HXCGUI hAdapter, HIMAGE hImage, int nParentID=XC_ID_ROOT, int insertID=XC_ID_LAST);
//@备注 插入项,数据填充到指定列.  
//@参数 hAdapter 数据适配器句柄.
//@参数 pName 字段名称.
//@参数 hImage 图片句柄.
//@参数 nParentID 父ID; 如果插入到目标位置, 此参数忽略(可填任意值)
//@参数 insertID 插入位置ID.
//@返回 返回项ID值.
//@别名 数据适配器树_插入项图片扩展()
XC_API int WINAPI XAdTree_InsertItemImageEx(HXCGUI hAdapter, const wchar_t* pName, HIMAGE hImage, int nParentID=XC_ID_ROOT, int insertID=XC_ID_LAST);
//@备注 获取项数量.  
//@参数 hAdapter 数据适配器句柄.
//@返回 返回数量.
//@别名 数据适配器树_取项数量()
XC_API int WINAPI XAdTree_GetCount(HXCGUI hAdapter);
//@备注 获取列数量.  
//@参数 hAdapter 数据适配器句柄.
//@返回 返回列数量.
//@别名 数据适配器树_取列数量()
XC_API int WINAPI XAdTree_GetCountColumn(HXCGUI hAdapter);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@参数 iColumn 列索引.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器树_置项文本()
XC_API BOOL WINAPI XAdTree_SetItemText(HXCGUI hAdapter, int nID, int iColumn, const wchar_t* pValue);
//@备注 设置项文件内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@参数 pName 字段名称.
//@参数 pValue 值.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器树_置项文本扩展()
XC_API BOOL WINAPI XAdTree_SetItemTextEx(HXCGUI hAdapter, int nID, const wchar_t* pName, const wchar_t* pValue);
//@备注 设置项数据.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@参数 iColumn 列索引.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器树_置项图片()
XC_API BOOL WINAPI XAdTree_SetItemImage(HXCGUI hAdapter, int nID, int iColumn, HIMAGE hImage);
//@备注 设置项内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@参数 pName 字段名称.
//@参数 hImage 图片句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器树_置项图片扩展()
XC_API BOOL WINAPI XAdTree_SetItemImageEx(HXCGUI hAdapter, int nID, const wchar_t* pName, HIMAGE hImage);
//@备注 获取项文本内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@参数 iColumn 列索引.
//@返回 返回文本内容
//@别名 数据适配器树_取项文本()
XC_API const wchar_t* WINAPI XAdTree_GetItemText(HXCGUI hAdapter, int nID, int iColumn);
//@备注 获取项文本内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@参数 pName 字段名称.
//@返回 返回文本内容
//@别名 数据适配器树_取项文本扩展()
XC_API const wchar_t* WINAPI XAdTree_GetItemTextEx(HXCGUI hAdapter, int nID, const wchar_t* pName);
//@备注 获取项内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@参数 iColumn 列索引.
//@返回 返回图片句柄.
//@别名 数据适配器树_取项图片()
XC_API HIMAGE WINAPI XAdTree_GetItemImage(HXCGUI hAdapter, int nID, int iColumn);
//@备注 获取项内容.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@参数 pName 字段名称.
//@返回 返回图片句柄.
//@别名 数据适配器树_取项图片扩展()
XC_API HIMAGE WINAPI XAdTree_GetItemImageEx(HXCGUI hAdapter, int nID, const wchar_t* pName);
//@备注 删除项.  
//@参数 hAdapter 数据适配器句柄.
//@参数 nID 项ID.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器树_删除项()
XC_API BOOL WINAPI XAdTree_DeleteItem(HXCGUI hAdapter, int nID);
//@备注 删除所有项.  
//@参数 hAdapter 数据适配器句柄.
//@别名 数据适配器树_删除项全部()
XC_API void WINAPI XAdTree_DeleteItemAll(HXCGUI hAdapter);
//@备注 删除所有列,并且清空数据.  
//@参数 hAdapter 数据适配器句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 数据适配器树_删除列全部()
XC_API void WINAPI XAdTree_DeleteColumnAll(HXCGUI hAdapter);
//@分组}
//@分组{  背景管理器

//@备注 创建背景管理器.  
//@返回 背景管理器句柄.
//@别名 背景_创建()
XC_API HBKM WINAPI XBkM_Create();
//@备注 销毁.  
//@参数 hBkInfoM 背景管理器句柄.
//@别名 背景_销毁()
XC_API void WINAPI XBkM_Destroy(HBKM hBkInfoM);
//@备注 废弃函数, 保留为了兼容旧版; 请使用 @ref XBkM_SetInfo()
//@参数 hBkInfoM 背景管理器句柄.
//@参数 pText 背景内容字符串.
//@返回 返回设置的背景对象数量.
//@别名 背景_置内容old()
XC_API int WINAPI XBkM_SetBkInfo(HBKM hBkInfoM, const wchar_t* pText);
//@备注 设置背景内容.  
//@参数 hBkInfoM 背景管理器句柄.
//@参数 pText 背景内容字符串.
//@返回 返回设置的背景对象数量.
//@别名 背景_置内容()
XC_API int WINAPI XBkM_SetInfo(HBKM hBkInfoM, const wchar_t* pText);
//@备注 添加背景内容.  
//@参数 hBkInfoM 背景管理器句柄.
//@参数 pText 背景内容字符串.
//@返回 返回添加的背景对象数量.
//@别名 背景_添加内容()
XC_API int WINAPI XBkM_AddInfo(HBKM hBkInfoM, const wchar_t* pText);
//@备注 添加背景内容边框.  
//@参数 hBkInfoM 背景管理器句柄.
//@参数 nState 组合状态  参加文档: API接口->组合状态
//@参数 color 颜色.
//@参数 width 线宽.
//@参数 id 背景对象ID, 可忽略(0)
//@别名 背景_添加边框()
XC_API void WINAPI XBkM_AddBorder(HBKM hBkInfoM, int nState, COLORREF color, int width, int id=0);
//@备注 添加背景内容填充.  
//@参数 hBkInfoM 背景管理器句柄.
//@参数 nState 组合状态  参加文档: API接口->组合状态
//@参数 color 颜色值, 请使用宏: RGBA()
//@参数 id 背景对象ID, 可忽略(0)
//@别名 背景_添加填充()
XC_API void WINAPI XBkM_AddFill(HBKM hBkInfoM, int nState, COLORREF color, int id=0);
//@备注 添加背景内容图片.  
//@参数 hBkInfoM 背景管理器句柄.
//@参数 nState 组合状态  参加文档: API接口->组合状态
//@参数 hImage 图片句柄.
//@参数 id 背景对象ID, 可忽略(0)
//@别名 背景_添加图片()
XC_API void WINAPI XBkM_AddImage(HBKM hBkInfoM, int nState, HIMAGE hImage, int id=0);
//@备注 获取背景内容数量.  
//@参数 hBkInfoM 背景管理器句柄.
//@返回 背景内容数量.
//@别名 背景_取数量()
XC_API int WINAPI XBkM_GetCount(HBKM hBkInfoM);
//@备注 清空背景内容.  
//@参数 hBkInfoM 背景管理器句柄.
//@别名 背景_清空()
XC_API void WINAPI XBkM_Clear(HBKM hBkInfoM);
//@备注 绘制背景内容.  
//@参数 hBkInfoM 背景管理器句柄.
//@参数 nState 组合状态  参加文档: API接口->组合状态
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 区域坐标.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 背景_绘制()
XC_API BOOL WINAPI XBkM_Draw(HBKM hBkInfoM, int nState, HDRAW hDraw, RECT* pRect);
//@备注 绘制背景内容, 设置条件.  
//@参数 hBkInfoM 背景管理器句柄.
//@参数 nState 组合状态  参加文档: API接口->组合状态
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 区域坐标.
//@参数 nStateEx 当(nState)中包含(nStateEx)中的一个或多个状态时有效.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 背景_绘制扩展()
XC_API BOOL WINAPI XBkM_DrawEx(HBKM hBkInfoM, int nState, HDRAW hDraw, RECT* pRect, int nStateEx);
//@备注 是否自动销毁
//@参数 hBkInfoM 背景管理器句柄
//@参数 bEnable 是否启用
//@别名 背景_启用自动销毁()
XC_API void WINAPI XBkM_EnableAutoDestroy(HBKM hBkInfoM, BOOL bEnable);
//@参数 hBkInfoM 背景管理器句柄
//@别名 背景_增加引用计数()
XC_API void WINAPI XBkM_AddRef(HBKM hBkInfoM);
//@备注 释放引用计数,当引用计数为0时,自动销毁
//@参数 hBkInfoM 背景管理器句柄
//@别名 背景_释放引用计数()
XC_API void WINAPI XBkM_Release(HBKM hBkInfoM);
//@备注 获取引用计数
//@参数 hBkInfoM 背景管理器句柄
//@返回 返回引用计数.
//@别名 背景_取引用计数()
XC_API int WINAPI XBkM_GetRefCount(HBKM hBkInfoM);
//@参数 hBkInfoM 背景管理器句柄
//@参数 nState 状态
//@参数 color 接收返回颜色
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 背景_取指定状态文本颜色()
XC_API BOOL WINAPI XBkM_GetStateTextColor(HBKM hBkInfoM, int nState, COLORREF* color);
//@参数 hBkInfoM 背景管理器句柄
//@参数 id 背景对象ID
//@返回 返回对象句柄
//@别名 背景_取背景对象()
XC_API vint WINAPI XBkM_GetObject(HBKM hBkInfoM, int id);
//@分组}
//@分组{  绘图

//@备注 创建图形绘制模块实例.  
//@参数 hWindow 窗口句柄
//@返回 图形绘制模块实例句柄.
//@别名 绘制_创建()
XC_API HDRAW WINAPI XDraw_Create(HWINDOW hWindow);
//@备注 创建图形绘制模块实例
//@参数 hWindow 窗口句柄
//@参数 hdc hdc句柄
//@返回 图形绘制模块实例句柄.
//@别名 绘制_创建GDI()
XC_API HDRAW WINAPI XDraw_CreateGDI(HWINDOW hWindow, HDC hdc);
//@备注 销毁图形绘制模块实例句柄.  
//@参数 hDraw 图形绘制句柄.
//@别名 绘制_销毁()
XC_API void WINAPI XDraw_Destroy(HDRAW hDraw);
//@备注 设置坐标偏移量,X向左偏移为负数,向右偏移为正数.  
//@参数 hDraw 图形绘制句柄.
//@参数 x X轴偏移量.
//@参数 y Y轴偏移量.
//@别名 绘制_置偏移()
XC_API void WINAPI XDraw_SetOffset(HDRAW hDraw, int x, int y);
//@备注 获取坐标偏移量,X向左偏移为负数,向右偏移为正数.  
//@参数 hDraw 图形绘制句柄.
//@参数 pX 接收X轴偏移量.
//@参数 pY 接收Y轴偏移量.
//@别名 绘制_取偏移()
XC_API void WINAPI XDraw_GetOffset(HDRAW hDraw, int* pX, int* pY);
//@备注 还原状态,释放用户绑定的GDI对象,例如画刷,画笔
//@参数 hDraw 图形绘制句柄. 
//@别名 绘制_GDI_还原状态()
XC_API void WINAPI XDraw_GDI_RestoreGDIOBJ(HDRAW hDraw);
//@备注 SetBkMode() 参见MSDN
//@参数 hDraw 图形绘制句柄.
//@参数 bTransparent 参见MSDN.
//@返回 参见MSDN.
//@别名 绘制_GDI_置背景模式()
XC_API int WINAPI XDraw_GDI_SetBkMode(HDRAW hDraw, BOOL bTransparent);
//@备注 选择一个区域作为当前裁剪区域,注意:该函数只对GDI有效
//NULLREGION Region is empty.\n
//SIMPLEREGION Region is a single rectangle.\n
//COMPLEXREGION Region is more than one rectangle.\n
//ERROR An error occurred. (The previous clipping region is unaffected).
//@参数 hDraw 图形绘制句柄.
//@参数 hRgn 区域句柄.
//@返回 返回值指定地区的复杂性，可以是下列值之一.\n
//@别名 绘制_GDI_选择裁剪区域()
XC_API int WINAPI XDraw_GDI_SelectClipRgn(HDRAW hDraw, HRGN hRgn);
//@备注 GDI创建具有指定的纯色逻辑刷
//@参数 hDraw 图形绘制句柄.
//@参数 crColor 画刷颜色.
//@返回 如果函数成功,返回值标识一个逻辑刷,如果函数失败,返回值是NULL.
//@别名 绘制_GDI_创建实心画刷()
XC_API HBRUSH WINAPI XDraw_GDI_CreateSolidBrush(HDRAW hDraw, COLORREF crColor);
//@备注 GDI创建一个逻辑笔,指定的样式,宽度和颜色,随后的笔可以选择到设备上下文,用于绘制线条和曲线
//@参数 hDraw 图形绘制句柄
//@参数 fnPenStyle 画笔样式, PS_SOLID:实线 PS_DASH:段线 PS_DOT:点线  PS_DASHDOT:段线_点线 PS_DASHDOTDOT:段线_点_点 PS_NULL:空  PS_INSIDEFRAME:实线_笔宽是向里扩展
//@参数 width 画笔宽度
//@参数 crColor 颜色
//@返回 如果函数成功,返回值是一个句柄,标识一个逻辑笔,如果函数失败,返回值是NULL.
//@别名 绘制_GDI_创建画笔()
XC_API HPEN WINAPI XDraw_GDI_CreatePen(HDRAW hDraw, int fnPenStyle, int width, COLORREF crColor);
//@备注 GDI创建矩形区域
//@参数 hDraw 图形绘制句柄.
//@参数 nLeftRect 左上角X坐标.
//@参数 nTopRect 左上角Y坐标.
//@参数 nRightRect 右下角X坐标.
//@参数 nBottomRect 右下角Y坐标.
//@返回 成功返回区域句柄,失败返回NULL.
//@别名 绘制_GDI_创建矩形区域()
XC_API HRGN WINAPI XDraw_GDI_CreateRectRgn(HDRAW hDraw, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
//@备注 GDI创建一个圆角的矩形区域
//@参数 hDraw 图形绘制句柄.
//@参数 nLeftRect X-坐标的左上角.
//@参数 nTopRect Y-坐标左上角坐标
//@参数 nRightRect X-坐标右下角
//@参数 nBottomRect Y-坐标右下角
//@参数 nWidthEllipse 椭圆的宽度.
//@参数 nHeightEllipse 椭圆的高度.
//@返回 如果函数成功,返回值是该区域的句柄,如果函数失败,返回值是NULL.
//@别名 绘制_GDI_创建圆角矩形区域()
XC_API HRGN WINAPI XDraw_GDI_CreateRoundRectRgn(HDRAW hDraw, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect, int nWidthEllipse, int nHeightEllipse);
//@备注 GDI创建一个多边形区域
//ALTERNATE Selects alternate mode (fills area between odd-numbered and even-numbered polygon sides on each scan line).\n
//WINDING Selects winding mode (fills any region with a nonzero winding value).
//@参数 hDraw 图形绘制句柄.
//@参数 pPt POINT数组.
//@参数 cPoints 数组大小(数组成员数).
//@参数 fnPolyFillMode 多边形填充模式,指定用于确定在该地区的像素填充模式,这个参数可以是下列值之一.\n
//@返回 如果函数成功,返回值是该区域的句柄,如果函数失败,返回值是NULL.
//@别名 绘制_GDI_创建多边形区域()
XC_API HRGN WINAPI XDraw_GDI_CreatePolygonRgn(HDRAW hDraw, const POINT* pPt, int cPoints, int fnPolyFillMode);
//@备注 绘制矩形,使用当前的画刷和画笔
//@参数 hDraw 图形绘制句柄.
//@参数 nLeftRect 左上角X坐标.
//@参数 nTopRect 左上角Y坐标.
//@参数 nRightRect 右下角X坐标.
//@参数 nBottomRect 右下角Y坐标.
//@返回 如果函数成功,返回非零值,如果函数失败,返回值是零.
//@别名 绘制_GDI_矩形()
XC_API BOOL WINAPI XDraw_GDI_Rectangle(HDRAW hDraw, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
//@备注 通过使用指定的画刷填充一个区域
//@参数 hDraw 图形绘制句柄.
//@参数 hrgn 区域句柄.
//@参数 hbr 画刷句柄.
//@返回 如果函数成功,返回非零值,如果函数失败,返回值是零.
//@别名 绘制_GID_填充区域()
XC_API BOOL WINAPI XDraw_GDI_FillRgn(HDRAW hDraw, HRGN hrgn, HBRUSH hbr);
//@参数 hDraw 图形绘制句柄
//@参数 pRect 矩形区域
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 绘制_GDI_椭圆()
XC_API BOOL WINAPI XDraw_GDI_Ellipse(HDRAW hDraw, const RECT* pRect);
//@备注 绘制边框,使用指定的画刷绘制指定的区域的边框
//@参数 hDraw 图形绘制句柄.
//@参数 hrgn 区域句柄.
//@参数 hbr 画刷句柄.
//@参数 width 边框宽度,垂直边.
//@参数 height 边框高度,水平边.
//@返回 如果函数成功,返回非零值,如果函数失败,返回值是零.
//@别名 绘制_GDI_边框区域()
XC_API BOOL WINAPI XDraw_GDI_FrameRgn(HDRAW hDraw, HRGN hrgn, HBRUSH hbr, int width, int nHeight);
//@备注 更新当前位置到指定点，并返回以前的位置
//@参数 hDraw 图形绘制句柄.
//@参数 X 坐标.
//@参数 Y 坐标.
//@参数 pPoint 接收以前的当前位置到一个POINT结构的指针,如果这个参数是NULL指针,没有返回原来的位置.
//@返回 如果函数成功,返回非零值,如果函数失败,返回值是零.
//@别名 绘制_GDI_移动到起点()
XC_API BOOL WINAPI XDraw_GDI_MoveToEx(HDRAW hDraw, int X, int Y, const POINT* pPoint=NULL);
//@备注 函数绘制一条线从当前位置到,但不包括指定点
//@参数 hDraw 图形绘制句柄.
//@参数 nXEnd X坐标,线结束点.
//@参数 nYEnd Y坐标,线结束点.
//@返回 如果函数成功,返回非零值,如果函数失败,返回值是零.
//@别名 绘制_GDI_线终点()
XC_API BOOL WINAPI XDraw_GDI_LineTo(HDRAW hDraw, int nXEnd, int nYEnd);
//@备注 Polyline() 参见MSDN
//@参数 hDraw 图形绘制句柄.
//@参数 pArrayPt 参见MSDN.
//@参数 arrayPtSize 参见MSDN.
//@返回 参见MSDN.
//@别名 绘制_GDI_折线()
XC_API BOOL WINAPI XDraw_GDI_Polyline(HDRAW hDraw, const POINT* pArrayPt, int arrayPtSize);
//@备注 绘制图标,DrawIconEx()参见MSDN
//DI_COMPAT：使用与 16 位 Windows 相同的绘制方式
//DI_DEFAULTSIZE：使用图标的默认大小
//DI_IMAGE：仅绘制图像
//DI_MASK：仅绘制掩码
//DI_NORMAL：绘制图像和掩码（默认）
//@参数 hDraw 图形绘制句柄
//@参数 xLeft 图标左上角的 x 坐标
//@参数 yTop 图标左上角的 y 坐标
//@参数 hIcon 要绘制的图标的句柄
//@参数 cxWidth 图标的宽度
//@参数 cyWidth 图标的高度
//@参数 istepIfAniCur 动画光标的帧索引，如果是静态图标则设置为 0
//@参数 hbrFlickerFreeDraw 背景闪烁自由的画刷，通常设置为 NULL
//@参数 diFlags 绘制标志, 
//@返回 .
//@别名 绘制_GDI_图标扩展()
XC_API BOOL WINAPI XDraw_GDI_DrawIconEx(HDRAW hDraw, int xLeft, int yTop, HICON hIcon, int cxWidth, int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);
//@备注 BitBlt() 参见MSDN
//@参数 hDrawDest XX.
//@参数 nXDest XX.
//@参数 nYDest XX.
//@参数 width XX.
//@参数 height XX.
//@参数 hdcSrc XX.
//@参数 nXSrc XX.
//@参数 nYSrc XX.
//@参数 dwRop XX.
//@返回 .
//@别名 绘制_GDI_复制()
XC_API BOOL WINAPI XDraw_GDI_BitBlt(HDRAW hDrawDest, int nXDest, int nYDest, int width, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
//@备注 BitBlt() 参见MSDN
//@参数 hDrawDest XX.
//@参数 nXDest XX.
//@参数 nYDest XX.
//@参数 width XX.
//@参数 height XX.
//@参数 hDrawSrc XX.
//@参数 nXSrc XX.
//@参数 nYSrc XX.
//@参数 dwRop XX.
//@返回 .
//@别名 绘制_GDI_复制2()
XC_API BOOL WINAPI XDraw_GDI_BitBlt2(HDRAW hDrawDest, int nXDest, int nYDest, int width, int nHeight, HDRAW hDrawSrc, int nXSrc, int nYSrc, DWORD dwRop);
//@备注 AlphaBlend() 参见MSDN
//@参数 hDraw XX.
//@参数 nXOriginDest XX.
//@参数 nYOriginDest XX.
//@参数 nWidthDest XX.
//@参数 nHeightDest XX.
//@参数 hdcSrc XX.
//@参数 nXOriginSrc XX.
//@参数 nYOriginSrc XX.
//@参数 nWidthSrc XX.
//@参数 nHeightSrc XX.
//@参数 alpha XX.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 绘制_GDI_带透明复制()
XC_API BOOL WINAPI XDraw_GDI_AlphaBlend(HDRAW hDraw, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, int alpha);
//@备注 函数设置在指定的坐标到指定的颜色的像素
//@参数 hDraw 图形绘制句柄.
//@参数 X 坐标
//@参数 Y 坐标
//@参数 crColor 颜色值, 请使用宏: RGBA()
//@返回 如果函数成功返回RGB值,如果失败返回-1.
//@别名 绘制_GDI_置像素颜色()
XC_API COLORREF WINAPI XDraw_GDI_SetPixel(HDRAW hDraw, int X, int Y, COLORREF crColor);
//@备注 获取绑定的设备上下文HDC.  
//@参数 hDraw 图形绘制句柄.
//@返回 返回HDC句柄. 
//@别名 绘制_取HDC()
XC_API HDC WINAPI XDraw_GetHDC(HDRAW hDraw);

//@备注  从当前UI对象客户区坐标转换到画布对齐坐标, 自动适应DPI, 便于用户使用第三方绘图接口
//@参数  hDraw 图形绘制句柄
//@参数  pRect  矩形坐标
//@别名  绘制_转换坐标()
XC_API void WINAPI XDraw_ConvRect(HDRAW hDraw, const RECT* pRect);

//@备注  从当前UI对象客户区坐标转换到画布对齐坐标, 自动适应DPI, 便于用户使用第三方绘图接口
//@参数  hDraw 图形绘制句柄
//@参数  x  X坐标
//@参数  y  Y坐标
//@别名  绘制_转换坐标XY()
XC_API void WINAPI XDraw_ConvXY(HDRAW hDraw, int x, int y);

//@参数 hDraw 图形绘制句柄
//@返回 返回 ID2D1Factory*,参数可空
//@别名 绘制_取D2D工厂()
XC_API vint WINAPI XDraw_GetD2dFactory(HDRAW hDraw);
//@参数 hDraw 图形绘制句柄
//@返回 返回 IDWriteFactory*,参数可空
//@别名 绘制_取D2DWrite工厂()
XC_API vint WINAPI XDraw_GetD2dWriteFactory(HDRAW hDraw);
//@参数 hDraw 图形绘制句柄
//@返回 返回 IWICImagingFactory*,参数可空
//@别名 绘制_取D2DWIC工厂()
XC_API vint WINAPI XDraw_GetD2dWICFactory(HDRAW hDraw);
//@参数 hDraw 图形绘制句柄
//@返回 返回 ID2D1RenderTarget*
//@别名 绘制_取D2D渲染目标()
XC_API vint WINAPI XDraw_GetD2dRenderTarget(HDRAW hDraw);
//@参数 hDraw 图形绘制句柄
//@返回 返回字体句柄
//@别名 绘制_取字体()
XC_API HFONTX WINAPI XDraw_GetFont(HDRAW hDraw);
//@参数 hDraw 图形绘制句柄
//@参数 hImageFrame 图片句柄
//@返回 返回 ID2D1Bitmap*
//@别名 绘制_取D2DBitmap()
XC_API vint WINAPI XDraw_GetD2dBitmap(HDRAW hDraw, HIMAGE hImageFrame);
//@参数 hDraw 图形绘制句柄
//@参数 mode 渲染模式  @ref XC_DWRITE_RENDERING_MODE
//@别名 绘制_置D2D文本渲染模式()
XC_API void WINAPI XDraw_SetD2dTextRenderingMode(HDRAW hDraw, XC_DWRITE_RENDERING_MODE mode);
//@备注 设置文本渲染质量GDI+
//@参数 hDraw 图形绘制句柄
//@参数 nType 参见GDI+ TextRenderingHint 定义.
//@别名 绘制_置文本炫彩质量()
XC_API void WINAPI XDraw_SetTextRenderingHint(HDRAW hDraw, int nType);
//@备注 使用指定颜色清理画布
//@参数 hDraw 图形绘制句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 绘制_D2D_清理()
XC_API void WINAPI XDraw_D2D_Clear(HDRAW hDraw, COLORREF color);
//@备注 设置画刷颜色.  
//@参数 hDraw 图形绘制句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 绘制_置画刷颜色()
XC_API void WINAPI XDraw_SetBrushColor(HDRAW hDraw, COLORREF color);
//@备注 设置文本垂直显示.  
//@参数 hDraw 图形绘制句柄.
//@参数 bVertical 是否垂直显示文本.
//@别名 绘制_置文本垂直()
XC_API void WINAPI XDraw_SetTextVertical(HDRAW hDraw, BOOL bVertical);
//@备注 设置文本对齐.  
//@参数 hDraw 图形绘制句柄.
//@参数 nFlags 对齐标识 @ref textFormatFlag_ .
//@别名 绘制_置文本对齐()
XC_API void WINAPI XDraw_SetTextAlign(HDRAW hDraw, int nFlag);
//@备注 设置字体.  
//@参数 hDraw 图形绘制句柄.
//@参数 hFontx 炫彩字体.
//@别名 绘制_置字体()
XC_API void WINAPI XDraw_SetFont(HDRAW hDraw, HFONTX hFontx);
//@备注 设置线宽.  
//@参数 hDraw 图形绘制句柄.
//@参数 width 宽度.
//@别名 绘制_置线宽()
XC_API void WINAPI XDraw_SetLineWidth(HDRAW hDraw, int width);
//@备注 设置线宽
//@参数 hDraw 图形绘制句柄.
//@参数 width 宽度.
//@别名 绘制_置线宽F()
XC_API void WINAPI XDraw_SetLineWidthF(HDRAW hDraw, float width);
//@备注 设置裁剪区域.  
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 区域坐标.
//@别名 绘制_置裁剪区域()
XC_API void WINAPI XDraw_SetClipRect(HDRAW hDraw, const RECT* pRect);
//@备注 清除裁剪区域.  
//@参数 hDraw 图形绘制句柄.
//@别名 绘制_清除裁剪区域()
XC_API void WINAPI XDraw_ClearClip(HDRAW hDraw);
//@备注 启用平滑模式.  
//@参数 hDraw 图形绘制句柄.
//@参数 bEnable 是否启用.
//@别名 绘制_启用平滑模式()
XC_API void WINAPI XDraw_EnableSmoothingMode(HDRAW hDraw, BOOL bEnable);
//@备注 当启用之后,调用GDI+函数时, 如果参数alpha=255,将自动修改为254, 应对GDI+的bug, 否则透明通道异常
//@参数 hDraw 图形绘制句柄
//@参数 bTransparent 是否启用
//@别名 绘制_启用窗口透明判断()
XC_API void WINAPI XDraw_EnableWndTransparent(HDRAW hDraw, BOOL bTransparent);
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形区域.
//@别名 绘制_填充矩形()
XC_API void WINAPI XDraw_FillRect(HDRAW hDraw, const RECT* pRect);
//@参数 hDraw 图形绘制句柄
//@参数 pRect 矩形区域
//@别名 绘制_填充矩形F()
XC_API void WINAPI XDraw_FillRectF(HDRAW hDraw, const RECTF* pRect);
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形区域.
//@参数 color 颜色.
//@别名 绘制_填充矩形指定颜色()
XC_API void WINAPI XDraw_FillRectColor(HDRAW hDraw, const RECT* pRect, COLORREF color);
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形区域.
//@参数 color 颜色.
//@别名 绘制_填充矩形指定颜色F()
XC_API void WINAPI XDraw_FillRectColorF(HDRAW hDraw, const RECTF* pRect, COLORREF color);
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形区域
//@别名 绘制_填充椭圆()
XC_API void WINAPI XDraw_FillEllipse(HDRAW hDraw, const RECT* pRect);
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形区域
//@别名 绘制_填充椭圆F()
XC_API void WINAPI XDraw_FillEllipseF(HDRAW hDraw, const RECTF* pRect);
//@备注 绘制椭圆边框
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形区域.
//@别名 绘制_椭圆()
XC_API void WINAPI XDraw_DrawEllipse(HDRAW hDraw, const RECT* pRect);
//@备注 绘制椭圆边框
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形区域.
//@别名 绘制_椭圆F()
XC_API void WINAPI XDraw_DrawEllipseF(HDRAW hDraw, const RECTF* pRect);
//@备注 填充圆角矩形
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@参数 width 圆角宽度.
//@参数 height 圆角高度.
//@别名 绘制_填充圆角矩形()
XC_API void WINAPI XDraw_FillRoundRect(HDRAW hDraw, const RECT* pRect, int width, int nHeight);
//@备注 填充圆角矩形
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@参数 width 圆角宽度.
//@参数 height 圆角高度.
//@别名 绘制_填充圆角矩形F()
XC_API void WINAPI XDraw_FillRoundRectF(HDRAW hDraw, const RECTF* pRect, float width, float height);
//@备注 绘制圆角矩形边框.  
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@参数 width 圆角宽度.
//@参数 height 圆角高度.
//@别名 绘制_圆角矩形()
XC_API void WINAPI XDraw_DrawRoundRect(HDRAW hDraw, const RECT* pRect, int width, int nHeight);
//@备注 绘制圆角矩形边框
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@参数 width 圆角宽度.
//@参数 height 圆角高度.
//@别名 绘制_圆角矩形F()
XC_API void WINAPI XDraw_DrawRoundRectF(HDRAW hDraw, const RECTF* pRect, float width, float height);
//@备注 填充圆角矩形.  
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 坐标.
//@参数 leftTop 圆角大小.
//@参数 rightTop 圆角大小.
//@参数 rightBottom 圆角大小.
//@参数 leftBottom 圆角大小.
//@别名 绘制_填充圆角矩形扩展()
XC_API void WINAPI XDraw_FillRoundRectEx(HDRAW hDraw, const RECT* pRect, int leftTop, int rightTop, int rightBottom, int leftBottom);
//@备注 填充圆角矩形
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 坐标.
//@参数 leftTop 圆角大小.
//@参数 rightTop 圆角大小.
//@参数 rightBottom 圆角大小.
//@参数 leftBottom 圆角大小.
//@别名 绘制_填充圆角矩形扩展F()
XC_API void WINAPI XDraw_FillRoundRectExF(HDRAW hDraw, const RECTF* pRect, float leftTop, float rightTop, float rightBottom, float leftBottom);
//@备注 绘制圆角矩形边框  
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 坐标.
//@参数 leftTop 圆角大小.
//@参数 rightTop 圆角大小.
//@参数 rightBottom 圆角大小.
//@参数 leftBottom 圆角大小.
//@别名 绘制_圆角矩形扩展()
XC_API void WINAPI XDraw_DrawRoundRectEx(HDRAW hDraw, const RECT* pRect, int leftTop, int rightTop, int rightBottom, int leftBottom);
//@备注 绘制圆角矩形边框
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 坐标.
//@参数 leftTop 圆角大小.
//@参数 rightTop 圆角大小.
//@参数 rightBottom 圆角大小.
//@参数 leftBottom 圆角大小.
//@别名 绘制_圆角矩形扩展F()
XC_API void WINAPI XDraw_DrawRoundRectExF(HDRAW hDraw, const RECTF* pRect, float leftTop, float rightTop, float rightBottom, float leftBottom);
//@备注 渐变填充,从一种颜色过渡到另一种颜色.  
//GRADIENT_FILL_RECT_H 水平填充 .
//GRADIENT_FILL_RECT_V 垂直填充.
//GRADIENT_FILL_TRIANGLE 三角形.
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@参数 color1 开始颜色.
//@参数 color2 结束颜色.
//@参数 mode 模式.
//@别名 绘制_渐变填充2()
XC_API void WINAPI XDraw_GradientFill2(HDRAW hDraw, const RECT* pRect, COLORREF color1, COLORREF color2, int mode);
//@备注 渐变填充,从一种颜色过渡到另一种颜色
//GRADIENT_FILL_RECT_H 水平填充 .
//GRADIENT_FILL_RECT_V 垂直填充.
//GRADIENT_FILL_TRIANGLE 三角形.
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@参数 color1 开始颜色.
//@参数 color2 结束颜色.
//@参数 mode 模式.
//@别名 绘制_渐变填充2F()
XC_API void WINAPI XDraw_GradientFill2F(HDRAW hDraw, const RECTF* pRect, COLORREF color1, COLORREF color2, int mode);
//@备注 渐变填充,从一种颜色过渡到另一种颜色.  
//GRADIENT_FILL_RECT_H 水平填充. 
//GRADIENT_FILL_RECT_V 垂直填充.
//GRADIENT_FILL_TRIANGLE 三角形.
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@参数 color1 开始颜色.
//@参数 color2 结束颜色,中间.
//@参数 color3 开始颜色,中间.
//@参数 color4 结束颜色.
//@参数 mode 模式.
//@别名 绘制_渐变填充4()
XC_API void WINAPI XDraw_GradientFill4(HDRAW hDraw, const RECT* pRect, COLORREF color1, COLORREF color2, COLORREF color3, COLORREF color4, int mode);
//@备注 渐变填充,从一种颜色过渡到另一种颜色
//GRADIENT_FILL_RECT_H 水平填充. 
//GRADIENT_FILL_RECT_V 垂直填充.
//GRADIENT_FILL_TRIANGLE 三角形.
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@参数 color1 开始颜色.
//@参数 color2 结束颜色,中间.
//@参数 color3 开始颜色,中间.
//@参数 color4 结束颜色.
//@参数 mode 模式.
//@别名 绘制_渐变填充4F()
XC_API void WINAPI XDraw_GradientFill4F(HDRAW hDraw, const RECTF* pRect, COLORREF color1, COLORREF color2, COLORREF color3, COLORREF color4, int mode);
//@备注 绘制矩形边框  
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标
//@别名 绘制_矩形()
XC_API void WINAPI XDraw_DrawRect(HDRAW hDraw, const RECT* pRect);
//@备注 绘制矩形边框
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标
//@别名 绘制_矩形F()
XC_API void WINAPI XDraw_DrawRectF(HDRAW hDraw, const RECTF* pRect);
//@备注 绘制线条.  
//@参数 hDraw 图形绘制句柄.
//@参数 x1 坐标.
//@参数 y1 坐标.
//@参数 x2 坐标.
//@参数 y2 坐标.
//@别名 绘制_线条()
XC_API void WINAPI XDraw_DrawLine(HDRAW hDraw, int x1, int y1, int x2, int y2);
//@备注 绘制线条
//@参数 hDraw 图形绘制句柄.
//@参数 x1 坐标.
//@参数 y1 坐标.
//@参数 x2 坐标.
//@参数 y2 坐标.
//@别名 绘制_线条F()
XC_API void WINAPI XDraw_DrawLineF(HDRAW hDraw, float x1, float y1, float x2, float y2);
//@备注 D2D暂时留空
//@参数 hDraw 图形绘制句柄.
//@参数 points 坐标点数组
//@参数 count 数组大小(数组成员数)
//@参数 tension 大于或等于0.0F的值，指定曲线的张力。D2D 忽略此参数
//@别名 绘制_曲线()
XC_API void WINAPI XDraw_DrawCurve(HDRAW hDraw, const POINT* points, int count, float tension);
//@备注 D2D暂时留空
//@参数 hDraw 图形绘制句柄.
//@参数 points 坐标点数组
//@参数 count 数组大小(数组成员数)
//@参数 tension 大于或等于0.0F的值，指定曲线的张力。D2D 忽略此参数
//@别名 绘制_曲线F()
XC_API void WINAPI XDraw_DrawCurveF(HDRAW hDraw, const POINTF* points, int count, float tension);
//@参数 hDraw 图形绘制句柄
//@参数 x 坐标
//@参数 y 坐标
//@参数 width 宽度
//@参数 height 高度
//@参数 startAngle 起始角度
//@参数 sweepAngle 绘制角度, 从起始角度开始计算
//@别名 绘制_圆弧()
XC_API void WINAPI XDraw_DrawArc(HDRAW hDraw, int x, int y, int width, int nHeight, float startAngle, float sweepAngle);
//@参数 hDraw 图形绘制句柄
//@参数 x 坐标
//@参数 y 坐标
//@参数 width 宽度
//@参数 height 高度
//@参数 startAngle 起始角度
//@参数 sweepAngle 绘制角度, 从起始角度开始计算
//@别名 绘制_圆弧F()
XC_API void WINAPI XDraw_DrawArcF(HDRAW hDraw, float x, float y, float width, float height, float startAngle, float sweepAngle);
//@备注 绘制焦点矩形.  
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@别名 绘制_焦点矩形()
XC_API void WINAPI XDraw_FocusRect(HDRAW hDraw, const RECT* pRect);
//@备注 绘制焦点矩形
//@参数 hDraw 图形绘制句柄.
//@参数 pRect 矩形坐标.
//@别名 绘制_焦点矩形F()
XC_API void WINAPI XDraw_FocusRectF(HDRAW hDraw, const RECTF* pRect);
//@备注 绘制水平或垂直虚线.  
//@参数 hDraw 图形绘制句柄.
//@参数 x1 起点x坐标.
//@参数 y1 起点y坐标.
//@参数 x2 结束点x坐标.
//@参数 y2 结束点y坐标.
//@别名 绘制_虚线()
XC_API void WINAPI XDraw_Dottedline(HDRAW hDraw, int x1, int y1, int x2, int y2);
//@备注 绘制水平或垂直虚线
//@参数 hDraw 图形绘制句柄.
//@参数 x1 起点x坐标.
//@参数 y1 起点y坐标.
//@参数 x2 结束点x坐标.
//@参数 y2 结束点y坐标.
//@别名 绘制_虚线F()
XC_API void WINAPI XDraw_DottedlineF(HDRAW hDraw, float x1, float y1, float x2, float y2);
//@备注 绘制多边形.  
//@参数 hDraw 图形绘制句柄.
//@参数 points 顶点坐标数组.
//@参数 nCount 顶点数量.
//@别名 绘制_多边形()
XC_API void WINAPI XDraw_DrawPolygon(HDRAW hDraw, const POINT* points, int nCount);
//@备注 绘制多边形
//@参数 hDraw 图形绘制句柄.
//@参数 points 顶点坐标数组.
//@参数 nCount 顶点数量.
//@别名 绘制_多边形F()
XC_API void WINAPI XDraw_DrawPolygonF(HDRAW hDraw, const POINTF* points, int nCount);
//@备注 填充多边形.  
//@参数 hDraw 图形绘制句柄.
//@参数 points 顶点坐标数组.
//@参数 nCount 顶点数量.
//@别名 绘制_填充多边形()
XC_API void WINAPI XDraw_FillPolygon(HDRAW hDraw, const POINT* points, int nCount);
//@备注 填充多边形
//@参数 hDraw 图形绘制句柄.
//@参数 points 顶点坐标数组.
//@参数 nCount 顶点数量.
//@别名 绘制_填充多边形F()
XC_API void WINAPI XDraw_FillPolygonF(HDRAW hDraw, const POINTF* points, int nCount);
//@备注 绘制图片.  
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 x x坐标.
//@参数 y y坐标.
//@别名 绘制_图片()
XC_API void WINAPI XDraw_Image(HDRAW hDraw, HIMAGE hImageFrame, int x, int y);
//@备注 绘制图片
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 x x坐标.
//@参数 y y坐标.
//@别名 绘制_图片F()
XC_API void WINAPI XDraw_ImageF(HDRAW hDraw, HIMAGE hImageFrame, float x, float y);
//@备注 绘制图片
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 x x坐标.
//@参数 y y坐标.
//@参数 width 宽度.
//@参数 height 高度.
//@别名 绘制_图片扩展()
XC_API void WINAPI XDraw_ImageEx(HDRAW hDraw, HIMAGE hImageFrame, int x, int y, int width, int height);
//@备注 绘制图片
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 x x坐标.
//@参数 y y坐标.
//@参数 width 宽度.
//@参数 height 高度.
//@别名 绘制_图片扩展F()
XC_API void WINAPI XDraw_ImageExF(HDRAW hDraw, HIMAGE hImageFrame, float x, float y, float width, float height);
//@备注 绘制图片.  
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 pRect 坐标.
//@参数 bOnlyBorder 是否只绘制边缘区域.
//@别名 绘制_图片自适应()
XC_API void WINAPI XDraw_ImageAdaptive(HDRAW hDraw, HIMAGE hImageFrame, const RECT* pRect, BOOL bOnlyBorder=FALSE);
//@备注 绘制图片
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 pRect 坐标.
//@参数 bOnlyBorder 是否只绘制边缘区域.
//@别名 绘制_图片自适应F()
XC_API void WINAPI XDraw_ImageAdaptiveF(HDRAW hDraw, HIMAGE hImageFrame, const RECTF* pRect, BOOL bOnlyBorder=FALSE);
//@备注 绘制图片.  
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 pRect 坐标.
//@参数 flag 标识, 0:从左上角开始平铺,1:从左下角开始平铺.
//@别名 绘制_图片平铺()
XC_API void WINAPI XDraw_ImageTile(HDRAW hDraw, HIMAGE hImageFrame, const RECT* pRect, int flag=0);
//@备注 绘制图片
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 pRect 坐标.
//@参数 flag 标识, 0:从左上角开始平铺,1:从左下角开始平铺.
//@别名 绘制_图片平铺F()
XC_API void WINAPI XDraw_ImageTileF(HDRAW hDraw, HIMAGE hImageFrame, const RECTF* pRect, int flag=0);
//@备注 绘制图片.  
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 pRect 坐标.
//@参数 bClip 是否裁剪区域.
//@别名 绘制_图片增强()
XC_API void WINAPI XDraw_ImageSuper(HDRAW hDraw, HIMAGE hImageFrame, const RECT* pRect, BOOL bClip=FALSE);
//@备注 绘制图片
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 pRect 坐标.
//@参数 bClip 是否裁剪区域.
//@别名 绘制_图片增强F()
XC_API void WINAPI XDraw_ImageSuperF(HDRAW hDraw, HIMAGE hImageFrame, const RECTF* pRect, BOOL bClip=FALSE);
//@备注 绘制图片.  
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 pRcDest 目标坐标.
//@参数 pRcSrc 源坐标.
//@别名 绘制_图片增强2()
XC_API void WINAPI XDraw_ImageSuperEx(HDRAW hDraw, HIMAGE hImageFrame, const RECT* pRcDest, const RECT* pRcSrc);
//@备注 绘制图片
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 pRcDest 目标坐标.
//@参数 pRcSrc 源坐标.
//@别名 绘制_图片增强扩展F()
XC_API void WINAPI XDraw_ImageSuperExF(HDRAW hDraw, HIMAGE hImageFrame, const RECTF* pRcDest, const RECTF* pRcSrc);
//@备注 绘制带遮盖的图片. D2D留空  
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 hImageFrameMask 图片句柄,遮盖.
//@参数 pRect 坐标.
//@参数 pRectMask 坐标,遮盖.
//@参数 bClip 是否裁剪区域.
//@别名 绘制_图片增强遮盖()
XC_API void WINAPI XDraw_ImageSuperMask(HDRAW hDraw, HIMAGE hImageFrame, HIMAGE hImageFrameMask, const RECT* pRect, const RECT* pRectMask, BOOL bClip=FALSE);
//@备注 绘制带遮盖的图片.D2D留空  
//@参数 hDraw 图形绘制句柄.
//@参数 hImageFrame 图片句柄.
//@参数 hImageFrameMask 图片句柄,遮盖.
//@参数 pRect 坐标.
//@参数 x 目标X坐标.
//@参数 y 目标Y坐标.
//@别名 绘制_图片遮盖()
XC_API void WINAPI XDraw_ImageMask(HDRAW hDraw, HIMAGE hImageFrame, HIMAGE hImageFrameMask, const RECT* pRect, int x, int y);
//@备注 使用矩形作为遮罩
//@参数 hDraw 图形绘制句柄
//@参数 hImageFrame 图片句柄
//@参数 pRect 矩形坐标
//@参数 pRcMask 遮罩坐标
//@参数 pRcRoundAngle 遮罩圆角
//@别名 绘制_图片遮盖矩形()
XC_API void WINAPI XDraw_ImageMaskRect(HDRAW hDraw, HIMAGE hImageFrame, const RECT* pRect, const RECT* pRcMask, const RECT* pRcRoundAngle);
//@备注 使用圆形作为遮罩
//@参数 hDraw 图形绘制句柄
//@参数 hImageFrame 图片句柄
//@参数 pRect 矩形坐标
//@参数 pRcMask 遮罩坐标
//@别名 绘制_图片遮盖圆型()
XC_API void WINAPI XDraw_ImageMaskEllipse(HDRAW hDraw, HIMAGE hImageFrame, const RECT* pRect, const RECT* pRcMask);
//@参数 hDraw 图形绘制句柄.
//@参数 pString 字符串.
//@参数 nCount 字符串长度.如果填-1自动取长度
//@参数 pRect 坐标.
//@别名 绘制_文本指定矩形()
XC_API void WINAPI XDraw_DrawText(HDRAW hDraw, const wchar_t* pString, int nCount, const RECT* pRect);
//@参数 hDraw 图形绘制句柄.
//@参数 pString 字符串.
//@参数 nCount 字符串长度.如果填-1自动取长度
//@参数 pRect 坐标.
//@别名 绘制_文本指定矩形F()
XC_API void WINAPI XDraw_DrawTextF(HDRAW hDraw, const wchar_t* pString, int nCount, const RECTF* pRect);
//@备注 参见MSDN.  
//@参数 hDraw 图形绘制句柄.
//@参数 pString 字符串.
//@参数 nCount 字符串长度.如果填-1自动取长度
//@参数 pRect 坐标.
//@参数 colorLine 下划线颜色.
//@别名 绘制_文本下划线()
XC_API void WINAPI XDraw_DrawTextUnderline(HDRAW hDraw, const wchar_t* pString, int nCount, const RECT* pRect, COLORREF colorLine);
//@参数 hDraw 图形绘制句柄.
//@参数 pString 字符串.
//@参数 nCount 字符串长度.如果填-1自动取长度
//@参数 pRect 坐标.
//@参数 colorLine 下划线颜色.
//@别名 绘制_文本下划线F()
XC_API void WINAPI XDraw_DrawTextUnderlineF(HDRAW hDraw, const wchar_t* pString, int nCount, const RECTF* pRect, COLORREF colorLine);
//@参数 hDraw 图形绘制句柄.
//@参数 xStart XX.
//@参数 yStart XX.
//@参数 pString XX.
//@参数 cbString XX. 如果填-1自动取长度
//@别名 绘制_文本()
XC_API void WINAPI XDraw_TextOut(HDRAW hDraw, int xStart, int yStart, const wchar_t* pString, int cbString);
//@参数 hDraw 图形绘制句柄.
//@参数 xStart XX.
//@参数 yStart XX.
//@参数 pString XX.
//@参数 cbString XX.
//@别名 绘制_文本F()
XC_API void WINAPI XDraw_TextOutF(HDRAW hDraw, float xStart, float yStart, const wchar_t* pString, int cbString);
//@参数 hDraw 图形绘制句柄.
//@参数 xStart XX.
//@参数 yStart XX.
//@参数 pString XX.
//@别名 绘制_文本扩展()
XC_API void WINAPI XDraw_TextOutEx(HDRAW hDraw, int xStart, int yStart, const wchar_t* pString);
//@参数 hDraw 图形绘制句柄.
//@参数 xStart XX.
//@参数 yStart XX.
//@参数 pString XX.
//@别名 绘制_文本扩展F()
XC_API void WINAPI XDraw_TextOutExF(HDRAW hDraw, float xStart, float yStart, const wchar_t* pString);
//@参数 hDraw 图形绘制句柄.
//@参数 xStart XX.
//@参数 yStart XX.
//@参数 pString XX.
//@别名 绘制_文本A()
XC_API void WINAPI XDraw_TextOutA(HDRAW hDraw, int xStart, int yStart, const char* pString);
//@参数 hDraw 图形绘制句柄.
//@参数 xStart XX.
//@参数 yStart XX.
//@参数 pString XX.
//@别名 绘制_文本AF()
XC_API void WINAPI XDraw_TextOutAF(HDRAW hDraw, float xStart, float yStart, const char* pString);
//@参数 hDraw 图形绘制句柄
//@参数 hSvg SVG句柄
//@别名 绘制_SVG源()
XC_API void WINAPI XDraw_DrawSvgSrc(HDRAW hDraw, HSVG hSvg);
//@参数 hDraw 图形绘制句柄
//@参数 hSvg SVG句柄
//@参数 x x坐标
//@参数 y y坐标
//@别名 绘制_SVG()
XC_API void WINAPI XDraw_DrawSvg(HDRAW hDraw, HSVG hSvg, int x, int y);
//@参数 hDraw 图形绘制句柄
//@参数 hSvg SVG句柄
//@参数 x x坐标
//@参数 y y坐标
//@参数 nWidth 宽度
//@参数 nHeight 高度
//@别名 绘制_SVG扩展()
XC_API void WINAPI XDraw_DrawSvgEx(HDRAW hDraw, HSVG hSvg, int x, int y, int nWidth, int nHeight);
//@参数 hDraw 图形绘制句柄
//@参数 hSvg SVG句柄
//@参数 nWidth 宽度
//@参数 nHeight 高度
//@别名 绘制_SVG大小()
XC_API void WINAPI XDraw_DrawSvgSize(HDRAW hDraw, HSVG hSvg, int nWidth, int nHeight);

struct  gradient_point_  //渐变位置点信息
{
	COLORREF    color;  //颜色
	int         nPos;   //位置  0-100
};
struct  gradient_info_  //渐变信息
{
	gradient_info_() { pArray = NULL; nCount = 0; fAngle = 0; }
	~gradient_info_() { if (pArray) delete[]pArray; }
	gradient_point_* pArray;  //渐变点数组
	int    nCount;    //渐变点数量
	float  fAngle;    //渐变角度
};

//@备注  渐变填充多点颜色
//@参数  hDraw 图形绘制句柄.
//@参数  pPoints 渐变点数组.
//@参数  nCount 渐变点数量.
//@参数  pRect 填充区域.
//@别名  绘制_填充渐变()
XC_API void WINAPI XDraw_GradientFill(HDRAW hDraw, const gradient_point_* pPoints, int nCount, const RECT* pRect);

//@备注  多边形区域渐变填充
//@参数  hDraw 图形绘制句柄.
//@参数  pPolygonPts 多边形顶点数组.
//@参数  nPolygonCount 顶点数量.
//@参数  pPoints 渐变点数组.
//@参数  nCount 渐变点数量.
//@参数  fAngle 渐变角度.
//@别名  绘制_填充渐变多边形()
XC_API void WINAPI XDraw_GradientFillPolygon(HDRAW hDraw, const POINTF* pPolygonPts, int nPolygonCount, const gradient_point_* pPoints, int nCount, float  fAngle);

//@备注  多边形边框渐变描边
//@参数  hDraw 图形绘制句柄.
//@参数  pPolygonPts 多边形顶点数组.
//@参数  nPolygonCount 顶点数量.
//@参数  pPoints 渐变点数组.
//@参数  nCount 渐变点数量.
//@参数  width 线宽.
//@参数  fAngle 渐变角度.
//@别名  绘制_渐变多边形()
XC_API void WINAPI XDraw_GradientDrawPolygon(HDRAW hDraw, const POINTF* pPolygonPts, int nPolygonCount, const gradient_point_* pPoints, int nCount, int width, float  fAngle);

//@备注  填充旋转后的圆角矩形
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 矩形区域.
//@参数  pRoundAngle 圆角参数.
//@参数  fRotationAngle 旋转角度.
//@别名  绘制_填充圆角矩形旋转()
XC_API void WINAPI XDraw_FillRoundRectRotate(HDRAW hDraw, const RECT* pRect, const RECT* pRoundAngle, float fRotationAngle);

//@备    绘制旋转后的圆角矩形边框
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 矩形区域.
//@参数  pRoundAngle 圆角参数.
//@参数  fRotationAngle 旋转角度.
//@参数  width 线宽.
//@别名  绘制_圆角矩形旋转()
XC_API void WINAPI XDraw_DrawRoundRectRotate(HDRAW hDraw, const RECT* pRect, const RECT* pRoundAngle, float fRotationAngle, int width);

//@备注  渐变填充旋转后的圆角矩形
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 矩形区域.
//@参数  pGradient 渐变信息.
//@参数  pRoundAngle 圆角参数.
//@参数  fRotationAngle 旋转角度.
//@别名  绘制_填充渐变圆角矩形旋转()
XC_API void WINAPI XDraw_FillRoundRectGradientRotate(HDRAW hDraw, const RECT* pRect, const gradient_info_* pGradient, const RECT* pRoundAngle, float fRotationAngle);

//@备注  渐变描边旋转后的圆角矩形
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 矩形区域.
//@参数  pGradient 渐变信息.
//@参数  pRoundAngle 圆角参数.
//@参数  fRotationAngle 旋转角度.
//@参数  width 线宽.
//@别名  绘制_渐变圆角矩形旋转()
XC_API void WINAPI XDraw_DrawRoundRectGradientRotate2(HDRAW hDraw, const RECT* pRect, const gradient_info_* pGradient, const RECT* pRoundAngle, float fRotationAngle, int width);

//@备注  渐变填充椭圆
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 椭圆区域.
//@参数  pPoints 渐变点数组.
//@参数  nCount 渐变点数量.
//@参数  fAngleRotate 椭圆旋转角度.
//@参数  fAngleGradient 渐变角度.
//@别名  绘制_填充渐变椭圆()
XC_API void WINAPI XDraw_FillGradientEllipse(HDRAW hDraw, const RECT* pRect, const gradient_point_* pPoints, int nCount, float fAngle, float fAngleGradient);

//@备注  渐变描边椭圆
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 椭圆区域.
//@参数  pPoints 渐变点数组.
//@参数  nCount 渐变点数量.
//@参数  width 线宽.
//@参数  fAngle 椭圆旋转角度.
//@参数  fAngleGradient 渐变角度.
//@别名  绘制_渐变椭圆()
XC_API void WINAPI XDraw_DrawGradientEllipse(HDRAW hDraw, const RECT* pRect, const gradient_point_* pPoints, int nCount, int width, float fAngle, float fAngleGradient);

//@备注  填充旋转后的椭圆
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 椭圆区域.
//@参数  fRotationAngle 旋转角度.
//@别名  绘制_填充椭圆旋转()
XC_API void WINAPI XDraw_FillEllipseRotate(HDRAW hDraw, const RECT* pRect, float fRotationAngle);

//@备注  绘制旋转后的椭圆边框
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 椭圆区域.
//@参数  width 线宽.
//@参数  fRotationAngle 旋转角度.
//@别名  绘制_椭圆旋转()
XC_API void WINAPI XDraw_DrawEllipseRotate(HDRAW hDraw, const RECT* pRect, int width, float fRotationAngle);

//@备注  绘制旋转后的多边形
//@参数  hDraw 图形绘制句柄.
//@参数  points 顶点数组.
//@参数  nCount 顶点数量.
//@参数  fRotationAngle 旋转角度.
//@别名  绘制_多边形旋转()
XC_API void WINAPI XDraw_DrawPolygonRotate(HDRAW hDraw, const POINTF* points, int nCount, int fRotationAngle);

//@备注  填充旋转后的多边形
//@参数  hDraw 图形绘制句柄.
//@参数  points 顶点数组.
//@参数  nCount 顶点数量.
//@参数  fRotationAngle 旋转角度.
//@别名  绘制_填充多边形旋转()
XC_API void WINAPI XDraw_FillPolygonRotate(HDRAW hDraw, const POINTF* points, int nCount, int fRotationAngle);

//@备注  绘制渐变的旋转文本
//@参数  hDraw 图形绘制句柄.
//@参数  pString 字符串.
//@参数  nCount 字符串长度.
//@参数  pRect 区域.
//@参数  pGradient 渐变信息.
//@别名  绘制_渐变文本旋转()
XC_API void WINAPI XDraw_DrawTextGradientRotate(HDRAW hDraw, const wchar_t* pString, int nCount, const RECT* pRect, const gradient_info_* pGradient);

//@备注  绘制三角形箭头
//@参数  hDraw 图形绘制句柄.
//@参数  x X坐标.
//@参数  y Y坐标.
//@参数  width 宽度.
//@参数  height 高度.
//@参数  align 对齐方式.
//@别名  绘制_三角箭头()
XC_API void WINAPI XDraw_TriangularArrow(HDRAW hDraw, int x, int y, int width, int height, int align);

//@备注  绘制带标题的矩形分组框
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 区域.
//@参数  pString 标题字符串.
//@参数  nLength 标题长度.
//@参数  textColor 标题颜色.
//@参数  pOffset 标题偏移.
//@别名  绘制_分组框矩形()
XC_API void WINAPI XDraw_DrawGroupBox_Rect(HDRAW hDraw, const RECT* pRect, const wchar_t* pString, int nLength, COLORREF textColor, const POINT* pOffset);

//@备注  绘制带标题的圆角分组框
//@参数  hDraw 图形绘制句柄.
//@参数  pRect 区域.
//@参数  ppStringName 标题字符串.
//@参数  nLength 标题长度.
//@参数  textColor 标题颜色.
//@参数  pOffset 标题偏移.
//@参数  width 圆角宽度.
//@参数  nHeight 圆角高度.
//@别名  绘制_分组框圆角矩形()
XC_API void WINAPI XDraw_DrawGroupBox_RoundRect(HDRAW hDraw, const RECT* pRect, const wchar_t* pString, int nLength, COLORREF textColor, const POINT* pOffset, int width, int height);

//@备注  绘制GIF图片指定帧
//@参数  hDraw 图形绘制句柄.
//@参数  hImageFrame 图片句柄.
//@参数  pRect 区域.
//@参数  iFrame 帧索引.
//@别名  绘制_GIF()
XC_API void WINAPI XDraw_Gif(HDRAW hDraw, HIMAGE hImageFrame, const RECT* pRect, UINT iFrame);

//@备注  绘制带透明度的图片
//@参数  hDraw 图形绘制句柄.
//@参数  hImageFrame 图片句柄.
//@参数  x X坐标.
//@参数  y Y坐标.
//@参数  width 宽度.
//@参数  height 高度.
//@参数  alpha 透明度.
//@别名  绘制_图片扩展Alpha()
XC_API void WINAPI XDraw_DrawImageExAlpha(HDRAW hDraw, HIMAGE hImageFrame, int x, int y, int width, int height, BYTE alpha);


//@分组}
//@分组{  字体

//@备注 创建炫彩字体,当字体句柄与元素关联后,会自动释放.  
//@参数 size 字体大小,字体大小,单位(pt,磅).
//@返回 字体句柄.
//@别名 字体_创建()
XC_API HFONTX WINAPI XFont_Create(int size);
//@备注 创建炫彩字体
//@参数 pName 字体名称.
//@参数 size 字体大小,单位(pt,磅).
//@参数 style 字体样式 @ref fontStyle_ ;  因为edit不支持下划线字体, 下划线,删除线,需要单独设置,见本模块接口
//@返回 字体句柄.
//@别名 字体_创建扩展()
XC_API HFONTX WINAPI XFont_CreateEx(const wchar_t* pName=L"宋体", int size=12, int style=fontStyle_regular);
//@备注 创建炫彩字体 D2D不支持此接口
//@参数 pFontInfo 字体信息.
//@返回 字体句柄.
//@别名 字体_创建从LOGFONT()
XC_API HFONTX WINAPI XFont_CreateFromLOGFONTW(LOGFONTW* pFontInfo);
//@备注 创建炫彩字体从现有HFONT字体,  D2D不支持此接口  
//@参数 hFont 字体句柄.
//@返回 返回炫彩字体.
//@别名 字体_创建从HFONT()
XC_API HFONTX WINAPI XFont_CreateFromHFONT(HFONT hFont);
//@备注 创建炫彩字体从GDI+字体(Font), D2D不支持此接口  
//@参数 pFont GDI+字体指针(Font*).
//@返回 返回炫彩字体句柄.
//@别名 字体_创建从Font()
XC_API HFONTX WINAPI XFont_CreateFromFont(void* pFont);
//@备注 创建字体从文件.  
//@参数 pFontFile 字体文件名.
//@参数 size 字体大小.
//@参数 style 样式, @ref fontStyle_ ;  因为edit不支持下划线字体, 下划线,删除线,需要单独设置,见本模块接口
//@返回 返回炫彩字体句柄.
//@别名 字体_创建从文件()
XC_API HFONTX WINAPI XFont_CreateFromFile(const wchar_t* pFontFile, int size=12, int style=fontStyle_regular);
//@参数 data 内存地址
//@参数 length 长度
//@参数 fontSize 字体大小
//@参数 style 字体样式,  @ref fontStyle_ ;  因为edit不支持下划线字体, 下划线,删除线,需要单独设置,见本模块接口
//@返回 返回炫彩字体句柄.
//@别名 字体_创建从内存()
XC_API HFONTX WINAPI XFont_CreateFromMem(const void* data, UINT length, int fontSize=12, int style=fontStyle_regular);
//@参数 id 资源ID
//@参数 pType 资源类型. 在rc资源文件中,资源的类型,例如:xcgui.rc,用记事本打开可以看见资源类型; 例如:BITMAP, PNG, FONT; 参见MSDN
//@参数 fontSize 字体大小
//@参数 style 字体样式,  @ref fontStyle_ ;  因为edit不支持下划线字体, 下划线,删除线,需要单独设置,见本模块接口
//@参数 hModule 从指定模块加载, 例如:DLL, EXE; 如果为空, 从当前EXE加载
//@返回 返回炫彩字体句柄.
//@别名 字体_创建从资源()
XC_API HFONTX WINAPI XFont_CreateFromRes(int id, const wchar_t* pType, int fontSize, int style, HMODULE hModule=NULL);
//@参数 pZipFileName zip文件名
//@参数 pFileName 字体文件名
//@参数 pPassword zip密码
//@参数 fontSize 字体大小
//@参数 style 字体样式,  @ref fontStyle_ ;  因为edit不支持下划线字体, 下划线,删除线,需要单独设置,见本模块接口
//@返回 返回炫彩字体句柄
//@别名 字体_创建从ZIP()
XC_API HFONTX WINAPI XFont_CreateFromZip(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword, int fontSize, int style);
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 字体文件名
//@参数 pPassword zip密码
//@参数 fontSize 字体大小
//@参数 style 字体样式,  @ref fontStyle_ ;  因为edit不支持下划线字体, 下划线,删除线,需要单独设置,见本模块接口
//@返回 返回炫彩字体句柄
//@别名 字体_创建从内存ZIP()
XC_API HFONTX WINAPI XFont_CreateFromZipMem(const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword, int fontSize, int style);
//@备注 是否自动销毁.  
//@参数 hFontX 字体句柄.
//@参数 bEnable 是否启用.
//@别名 字体_启用自动销毁()
XC_API void WINAPI XFont_EnableAutoDestroy(HFONTX hFontX, BOOL bEnable);
//@备注 仅供edit字体使用, 因为edit不支持下划线字体, 所以需要单独设置
//@参数 hFontX 字体句柄
//@参数 bUnderline 是否启用下划线
//@参数 bStrikeout 是否启用删除线
//@别名 字体_置下划线()
XC_API void WINAPI XFont_SetUnderlineEdit(HFONTX hFontX, BOOL bUnderline, BOOL bStrikeout);
//@备注 仅供edit字体使用, 因为edit不支持下划线字体, 所以需要单独设置
//@参数 hFontX 字体句柄
//@参数 bUnderline 接收返回值, 是否启用下划线
//@参数 bStrikeout 接收返回值, 是否启用删除线
//@别名 字体_取下划线()
XC_API void WINAPI XFont_GetUnderlineEdit(HFONTX hFontX, BOOL* bUnderline, BOOL* bStrikeout);
//@备注 获取字体,返回GDI+ Font指针.  
//@参数 hFontX 字体句柄.
//@返回 返回GDI+ Font指针.
//@别名 字体_取Font()
XC_API void* WINAPI XFont_GetFont(HFONTX hFontX);
//@备注 获取字体信息.  
//@参数 hFontX 字体句柄.
//@参数 pInfo 接收返回的字体信息.
//@别名 字体_取信息()
XC_API void WINAPI XFont_GetFontInfo(HFONTX hFontX, font_info_* pInfo);
//@备注 获取字体LOGFONTW  
//@参数 hFontX 字体句柄
//@参数 hdc hdc句柄
//@参数 pOut 接收返回信息
//@返回 成功返回TRUE,否则返回FALSE
//@别名 字体_取LOGFONTW()
XC_API BOOL WINAPI XFont_GetLOGFONTW(HFONTX hFontX, HDC hdc, LOGFONTW* pOut);
//@备注 强制销毁炫彩字体,谨慎使用, 建议使用 @ref XFont_Release() 释放.  
//@参数 hFontX 字体句柄.
//@别名 字体_销毁()
XC_API void WINAPI XFont_Destroy(HFONTX hFontX);
//@备注 增加引用计数.  
//@参数 hFontX 字体句柄.
//@别名 字体_增加引用计数()
XC_API void WINAPI XFont_AddRef(HFONTX hFontX);
//@备注 获取引用计数.  
//@参数 hFontX 字体句柄.
//@返回 返回引用计数.
//@别名 字体_取引用计数()
XC_API int WINAPI XFont_GetRefCount(HFONTX hFontX);
//@备注 释放引用计数,当引用计数为0时,自动销毁.  
//@参数 hFontX 字体句柄.
//@别名 字体_释放引用计数()
XC_API void WINAPI XFont_Release(HFONTX hFontX);
//@分组}
//@分组{  图片源

//@备注 加载图片从文件.  
//@参数 pFileName 图片文件.
//@返回 图片句柄.
//@别名 图片源_加载从文件()
XC_API HIMAGE WINAPI XImgSrc_LoadFile(const wchar_t* pFileName);
//@备注 加载图片,指定区位置及大小.  
//@参数 pFileName 图片文件.
//@参数 x 坐标.
//@参数 y 坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@返回 图片句柄.
//@别名 图片源_加载从文件指定区域()
XC_API HIMAGE WINAPI XImgSrc_LoadFileRect(const wchar_t* pFileName, int x, int y, int cx, int cy);
//@备注 加载图片从资源.  
//@参数 id 资源ID.
//@参数 pType 资源类型, 在rc资源文件中,资源的类型, 例如:xcgui.rc, 用记事本打开可以看见资源类型; 例如:BITMAP, PNG; 参见MSDN
//@参数 hModule 从指定模块加载, 如果为空从当前EXE加载
//@返回 图片句柄.
//@别名 图片源_加载从资源()
XC_API HIMAGE WINAPI XImgSrc_LoadRes(int id, const wchar_t* pType, HMODULE hModule);
//@备注 加载图片从ZIP压缩包.  
//@参数 pZipFileName ZIP压缩包文件名.
//@参数 pFileName 图片文件名.
//@参数 pPassword ZIP压缩包密码.
//@返回 图片句柄.
//@别名 图片源_加载从ZIP()
XC_API HIMAGE WINAPI XImgSrc_LoadZip(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 加载ZIP图片,指定区位置及大小.  
//@参数 pZipFileName ZIP文件.
//@参数 pFileName 图片名称
//@参数 pPassword 密码
//@参数 x 坐标.
//@参数 y 坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@返回 图片句柄.
//@别名 图片源_加载从ZIP指定区域()
XC_API HIMAGE WINAPI XImgSrc_LoadZipRect(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword, int x, int y, int cx, int cy);
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 图片名称
//@参数 pPassword zip压缩包密码
//@返回 图片句柄
//@别名 图片源_加载从内存ZIP()
XC_API HIMAGE WINAPI XImgSrc_LoadZipMem(const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 加载流图片,指定区位置及大小.  
//@参数 pBuffer 图片缓冲区
//@参数 nSize 图片缓冲区大小
//@返回 图片句柄.
//@别名 图片源_加载从内存()
XC_API HIMAGE WINAPI XImgSrc_LoadMemory(const void* pBuffer, int nSize);
//@备注 加载流图片,指定区位置及大小.  
//@参数 pBuffer 图片缓冲区
//@参数 nSize 图片缓冲区大小
//@参数 x 坐标.
//@参数 y 坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@返回 图片句柄.
//@别名 图片源_加载从内存指定区域()
XC_API HIMAGE WINAPI XImgSrc_LoadMemoryRect(const void* pBuffer, int nSize, int x, int y, int cx, int cy);
//@备注 加载图片从GDI+的Image对象.  
//@参数 pImage GDI图片对象指针 Bitmap*.
//@返回 成功返回炫彩图片句柄,失败返回FALSE.
//@别名 图片源_加载从Image()
XC_API HIMAGE WINAPI XImgSrc_LoadFromImage(const void* pImage);
//@备注 加载文件图标,从一个EXE文件或DLL文件或图标文件;例如:*.exe文件的图标.  
//@参数 pFileName 文件名.
//@返回 成功返回炫彩图片句柄,失败返回FALSE.
//@别名 图片源_加载从模块()
XC_API HIMAGE WINAPI XImgSrc_LoadFromExtractIcon(const wchar_t* pFileName);
//@备注 创建一个炫彩图片句柄,从一个现有的图标句柄HICON.  
//@参数 hIcon 图标句柄,如果你不使用可以释放 DestroyIcon().
//@返回 成功返回炫彩图片句柄,失败返回FALSE.
//@别名 图片源_加载从HICON()
XC_API HIMAGE WINAPI XImgSrc_LoadFromHICON(HICON hIcon);
//@备注 创建一个炫彩图片句柄,从一个现有的位图句柄HBITMAP.  
//@参数 hBitmap 位图句柄,如果你不使用可以释放 DeleteObject().
//@返回 成功返回炫彩图片句柄,失败返回FALSE.
//@别名 图片源_加载从HBITMAP()
XC_API HIMAGE WINAPI XImgSrc_LoadFromHBITMAP(HBITMAP hBitmap);

//@备注  从内存加载原始像素数据(BGRA32位格式), 1个像素占4个字节,分别是(B,G,R,A)
//@参数  data  数据地址
//@参数  width 图片宽度
//@参数  width 图片高度
//@返回 成功返回炫彩图片句柄,失败返回FALSE.
//@别名 图片源_加载从数据()
XC_API HIMAGE WINAPI XImgSrc_LoadFromData(const void* data, int width, int height);
//@备注  修改图片内存数据(BGRA32位格式), 1个像素占4个字节,分别是(B,G,R,A)
//@参数  data  数据地址
//@参数  width 图片宽度
//@参数  width 图片高度
//@返回 成功返回TRUE,失败返回FALSE.
//@别名 图片源_修改数据()
XC_API BOOL  WINAPI XImgSrc_ModifyData(HIMAGE hImage, const void* data, int width, int height);

//@备注  获取WIC位图指针 IWICBitmapSource*
//@参数  hImage  图片句柄
//@返回  返回 IWICBitmapSource*
//@别名  图片源_取WicBitmap()
XC_API vint WINAPI XImgSrc_GetWicBitMap(HIMAGE hImage);
//@备注  获取GDI+位图指针 Gdiplus::Bitmap*
//@参数  hImage  图片句柄
//@返回  返回 Gdiplus::Bitmap*
//@别名  图片源_取GdiplusBitmap()
XC_API vint WINAPI XImgSrc_GetGdiplusBitmap(HIMAGE hImage);

//@备注 启用或关闭自动销毁,当与UI元素关联时有效  
//@参数 hImage 图片句柄.
//@参数 bEnable 启用自动销毁TRUE,关闭自动销毁FALSE.
//@别名 图片源_启用自动销毁()
XC_API void WINAPI XImgSrc_EnableAutoDestroy(HIMAGE hImage, BOOL bEnable);
//@备注 获取图片宽度.  
//@参数 hImage 图片句柄.
//@返回 图片宽度.
//@别名 图片源_取宽度()
XC_API int WINAPI XImgSrc_GetWidth(HIMAGE hImage);
//@备注 获取图片高度.  
//@参数 hImage 图片句柄.
//@返回 图片高度.
//@别名 图片源_取高度()
XC_API int WINAPI XImgSrc_GetHeight(HIMAGE hImage);
//@备注 获取图片文件名  
//@参数 hImage 图片句柄
//@返回 返回文件名
//@别名 图片源_取文件名()
XC_API const wchar_t* WINAPI XImgSrc_GetFile(HIMAGE hImage);
//@备注 增加引用计数.  
//@参数 hImage 图片句柄.
//@别名 图片源_增加引用计数()
XC_API void WINAPI XImgSrc_AddRef(HIMAGE hImage);
//@备注 释放引用计数,当引用计数为0时,自动销毁.  
//@参数 hImage 图片句柄.
//@别名 图片源_释放引用计数()
XC_API void WINAPI XImgSrc_Release(HIMAGE hImage);
//@备注 获取引用计数.  
//@参数 hImage 图片句柄.
//@返回 返回引用计数.
//@别名 图片源_取引用计数()
XC_API int WINAPI XImgSrc_GetRefCount(HIMAGE hImage);
//@备注 强制销毁图片, 谨慎使用, 建议使用 @ref XImgSrc_Release() 释放.  
//@参数 hImage 图片句柄.
//@别名 图片源_销毁()
XC_API void WINAPI XImgSrc_Destroy(HIMAGE hImage);
//@分组}
//@分组{  图片

//@参数 hImageSrc 图片源句柄
//@返回 返回图片句柄
//@别名 图片_加载从图片源()
XC_API HIMAGE WINAPI XImage_LoadSrc(HIMAGE hImageSrc);
//@备注 加载图片从文件; 如果是SVG文件, 自动加载SVG文件返回图片句柄
//@参数 pFileName 图片文件.
//@返回 图片句柄.
//@别名 图片_加载从文件()
XC_API HIMAGE WINAPI XImage_LoadFile(const wchar_t* pFileName);
//@备注 加载图片从文件,自适应图片.  
//@参数 pFileName 图片文件.
//@参数 leftSize 坐标.
//@参数 topSize 坐标.
//@参数 rightSize 坐标.
//@参数 bottomSize 坐标.
//@返回 图片句柄.
//@别名 图片_加载从文件自适应()
XC_API HIMAGE WINAPI XImage_LoadFileAdaptive(const wchar_t* pFileName, int leftSize, int topSize, int rightSize, int bottomSize);
//@备注 加载图片,指定区位置及大小.  
//@参数 pFileName 图片文件.
//@参数 x 坐标.
//@参数 y 坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@返回 图片句柄.
//@别名 图片_加载从文件指定区域()
XC_API HIMAGE WINAPI XImage_LoadFileRect(const wchar_t* pFileName, int x, int y, int cx, int cy);
//@备注 加载图片从资源,自适应图片.  
//@参数 id 资源ID.
//@参数 pType 资源类型. 在rc资源文件中,资源的类型, 例如:xcgui.rc, 用记事本打开可以看见资源类型; 例如:BITMAP, PNG; 参见MSDN
//@参数 leftSize 坐标.
//@参数 topSize 坐标.
//@参数 rightSize 坐标.
//@参数 bottomSize 坐标.
//@参数 hModule 从指定模块加载, 例如:DLL, EXE; 如果为空, 从当前EXE加载
//@返回 图片句柄.
//@别名 图片_加载从资源自适应()
XC_API HIMAGE WINAPI XImage_LoadResAdaptive(int id, const wchar_t* pType, int leftSize, int topSize, int rightSize, int bottomSize, HMODULE hModule=NULL);
//@备注 加载图片从资源. RC资源类型必须为:"RT_RCDATA"  
//@参数 id 资源ID.
//@参数 pType 资源类型. 在rc资源文件中,资源的类型, 例如:xcgui.rc, 用记事本打开可以看见资源类型; 例如:RT_BITMAP, PNG; 注意若是默认的"BITMAP"需要改资源类型名称, 否则会失败; 参见MSDN
//@参数 hModule 从指定模块加载, 例如:DLL, EXE; 如果为空, 从当前EXE加载
//@返回 图片句柄.
//@别名 图片_加载从资源()
XC_API HIMAGE WINAPI XImage_LoadRes(int id, const wchar_t* pType, HMODULE hModule=NULL);
//@备注 加载图片从ZIP压缩包.  
//@参数 pZipFileName ZIP压缩包文件名.
//@参数 pFileName 图片文件名.
//@参数 pPassword ZIP压缩包密码.
//@返回 图片句柄.
//@别名 图片_加载从ZIP()
XC_API HIMAGE WINAPI XImage_LoadZip(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 加载图片从ZIP压缩包,自适应图片.  
//@参数 pZipFileName ZIP压缩包文件名.
//@参数 pFileName 图片文件名.
//@参数 pPassword ZIP压缩包密码,如果没有填NULL.
//@参数 x1 坐标.
//@参数 x2 坐标.
//@参数 y1 坐标.
//@参数 y2 坐标.
//@返回 图片句柄.
//@别名 图片_加载从ZIP自适应()
XC_API HIMAGE WINAPI XImage_LoadZipAdaptive(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword, int x1, int x2, int y1, int y2);
//@备注 加载ZIP图片,指定区位置及大小.  
//@参数 pZipFileName ZIP文件.
//@参数 pFileName 图片名称
//@参数 pPassword 密码
//@参数 x 坐标.
//@参数 y 坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@返回 图片句柄.
//@别名 图片_加载从ZIP指定区域()
XC_API HIMAGE WINAPI XImage_LoadZipRect(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword, int x, int y, int cx, int cy);
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 图片名称
//@参数 pPassword zip压缩包密码
//@返回 图片句柄
//@别名 图片_加载从内存ZIP()
XC_API HIMAGE WINAPI XImage_LoadZipMem(const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 RC资源类型必须为:"RT_RCDATA"
//@参数 id RC资源ID
//@参数 pFileName 图片名称
//@参数 pPassword zip压缩包密码
//@参数 hModule 模块句柄
//@返回 图片句柄
//@别名 图片_加载从资源ZIP()
XC_API HIMAGE WINAPI XImage_LoadZipRes(int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@备注 加载流图片,指定区位置及大小.  
//@参数 pBuffer 图片缓冲区
//@参数 nSize 图片缓冲区大小
//@返回 图片句柄.
//@别名 图片_加载从内存()
XC_API HIMAGE WINAPI XImage_LoadMemory(const void* pBuffer, int nSize);
//@备注 加载流图片,指定区位置及大小.  
//@参数 pBuffer 图片缓冲区
//@参数 nSize 图片缓冲区大小
//@参数 x 坐标.
//@参数 y 坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@返回 图片句柄.
//@别名 图片_加载从内存指定区域()
XC_API HIMAGE WINAPI XImage_LoadMemoryRect(const void* pBuffer, int nSize, int x, int y, int cx, int cy);
//@备注 加载流图片压缩包,自适应图片(九宫格).  
//@参数 pBuffer 图片缓冲区
//@参数 nSize 图片缓冲区大小
//@参数 leftSize 坐标.
//@参数 topSize 坐标.
//@参数 rightSize 坐标.
//@参数 bottomSize 坐标.
//@返回 图片句柄
//@别名 图片_加载从内存自适应()
XC_API HIMAGE WINAPI XImage_LoadMemoryAdaptive(const void* pBuffer, int nSize, int leftSize, int topSize, int rightSize, int bottomSize);
//@备注 加载图片从GDI+的Image对象.  
//@参数 pImage GDI图片对象指针 Image*.
//@返回 图片句柄
//@别名 图片_加载从Image()
XC_API HIMAGE WINAPI XImage_LoadFromImage(const void* pImage);
//@备注 加载文件图标,从一个EXE文件或DLL文件或图标文件;例如:*.exe文件的图标.  
//@参数 pFileName 文件名.
//@返回 图片句柄
//@别名 图片_加载文件图标()
XC_API HIMAGE WINAPI XImage_LoadFromExtractIcon(const wchar_t* pFileName);
//@备注 创建一个炫彩图片句柄,从一个现有的图标句柄HICON.  
//@参数 hIcon 图标句柄,如果你不使用可以释放 DestroyIcon().
//@返回 图片句柄
//@别名 图片_加载从HICON()
XC_API HIMAGE WINAPI XImage_LoadFromHICON(HICON hIcon);
//@备注 创建一个炫彩图片句柄,从一个现有的位图句柄HBITMAP.  
//@参数 hBitmap 位图句柄,如果你不使用可以释放 DeleteObject().
//@返回 图片句柄
//@别名 图片_加载从HBITMAP()
XC_API HIMAGE WINAPI XImage_LoadFromHBITMAP(HBITMAP hBitmap);

//@备注  从内存加载原始像素数据(BGRA32位格式), 1个像素占4个字节,分别是(R,G,B,A)
//@参数  data  数据地址
//@参数  width 图片宽度
//@参数  width 图片高度
//@返回 成功返回炫彩图片句柄,失败返回FALSE.
//@别名  图片_加载从数据()
XC_API HIMAGE WINAPI XImage_LoadFromData(const void* data, int width, int height);
//@备注  修改图片内存数据(BGRA32位格式), 1个像素占4个字节,分别是(R,G,B,A)
//@参数  data  数据地址
//@参数  width 图片宽度
//@参数  width 图片高度
//@返回 成功返回TRUE,失败返回FALSE.
//@别名  图片_修改数据()
XC_API BOOL   WINAPI XImage_ModifyData(HIMAGE hImage, const void* data, int width, int height);

//@备注  获取WIC位图指针 IWICBitmapSource*
//@参数  hImage  图片句柄
//@返回 返回 IWICBitmapSource*
//@别名 图片_取WicBitmap()
XC_API vint WINAPI XImage_GetWicBitMap(HIMAGE hImage);

//@备注 获取GDI+位图指针 Gdiplus::Bitmap*
//@参数  hImage  图片句柄
//@返回 返回 Gdiplus::Bitmap*
//@别名 图片_取GdiplusBitmap()
XC_API vint WINAPI XImage_GetGdiplusBitmap(HIMAGE hImage);

//@备注 是否为拉伸图片句柄  
//@参数 hImage 图片句柄.
//@返回 是否缩放.
//@别名 图片_是否缩放()
XC_API BOOL WINAPI XImage_IsStretch(HIMAGE hImage);
//@备注 是否为自适应图片句柄  
//@参数 hImage 图片句柄.
//@返回 是否自适应
//@别名 图片_是否自适应()
XC_API BOOL WINAPI XImage_IsAdaptive(HIMAGE hImage);
//@备注 是否为平铺图片  
//@参数 hImage 图片句柄.
//@返回 是否平铺
//@别名 图片_是否平铺()
XC_API BOOL WINAPI XImage_IsTile(HIMAGE hImage);
//@参数 hSvg SVG句柄
//@返回 图片句柄
//@别名 图片_加载从SVG()
XC_API HIMAGE WINAPI XImage_LoadSvg(HSVG hSvg);
//@备注 UTF8文件
//@参数 pFileName 文件名
//@返回 图片句柄
//@别名 图片_加载从SVG文件()
XC_API HIMAGE WINAPI XImage_LoadSvgFile(const wchar_t* pFileName);
//@备注 多字节字符串ANSI
//@参数 pString 字符串指针
//@返回 图片句柄
//@别名 图片_加载从SVG字符串()
XC_API HIMAGE WINAPI XImage_LoadSvgString(const char* pString);
//@备注 UNICODE字符串
//@参数 pString 字符串指针
//@返回 图片句柄
//@别名 图片_加载从SVG字符串W()
XC_API HIMAGE WINAPI XImage_LoadSvgStringW(const wchar_t* pString);
//@备注 UTF8字符串
//@参数 pString 字符串指针
//@返回 图片句柄
//@别名 图片_加载从SVG字符串UTF8()
XC_API HIMAGE WINAPI XImage_LoadSvgStringUtf8(const char* pString);
//@参数 hImage 图片句柄
//@返回 SVG句柄
//@别名 图片_取SVG()
XC_API HSVG WINAPI XImage_GetSvg(HIMAGE hImage);
//@备注 设置图片绘制类型  
//@参数 hImage 图片句柄.
//@参数 nType 图片绘制类型.
//@返回 如果成功返回TRUE,否则相反.
//@别名 图片_置绘制类型()
XC_API BOOL WINAPI XImage_SetDrawType(HIMAGE hImage, image_draw_type_ nType);
//@备注 设置图片自适应(九宫格)  
//@参数 hImage 图片句柄.
//@参数 leftSize 坐标.
//@参数 topSize 坐标.
//@参数 rightSize 坐标.
//@参数 bottomSize 坐标.
//@返回 如果成功返回TRUE,否则相反.
//@别名 图片_置绘制类型自适应()
XC_API BOOL WINAPI XImage_SetDrawTypeAdaptive(HIMAGE hImage, int leftSize, int topSize, int rightSize, int bottomSize);
//@备注 指定图片透明颜色. 仅支持GDI+模式 
//@参数 hImage 图片句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 图片_置透明色()
XC_API void WINAPI XImage_SetTranColor(HIMAGE hImage, COLORREF color);
//@备注 指定图片透明颜色及透明度.仅支持GDI+模式  
//@参数 hImage 图片句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@参数 tranColor 透明色的透明度.
//@别名 图片_置透明色扩展()
XC_API void WINAPI XImage_SetTranColorEx(HIMAGE hImage, COLORREF color, BYTE tranColor);
//@备注 设置旋转角度.  
//@参数 hImage 图片句柄.
//@参数 fAngle 选择角度.
//@返回 返回先前角度.
//@别名 图片_置旋转角度()
XC_API float WINAPI XImage_SetRotateAngle(HIMAGE hImage, float fAngle);
//@参数 hImage 图片句柄
//@参数 nCount 等分数量
//@参数 iIndex 索引
//@别名 图片_置等分()
XC_API void WINAPI XImage_SetSplitEqual(HIMAGE hImage, int nCount, int iIndex);
//@备注 启用缩放属性后有效,值大于0有效, @ref XImage_SetDrawType() 例如: 拉伸, 九宫格 
//@参数 hImage 图片句柄
//@参数 width 宽度
//@参数 height 高度
//@别名 图片_置缩放大小()
XC_API void WINAPI XImage_SetScaleSize(HIMAGE hImage, int width, int height);
//@备注 启用或关闭图片透明色.仅支持GDI+模式  
//@参数 hImage 图片句柄.
//@参数 bEnable 启用TRUE,关闭FALSE.
//@别名 图片_启用透明色()
XC_API void WINAPI XImage_EnableTranColor(HIMAGE hImage, BOOL bEnable);
//@备注 启用或关闭自动销毁,当与UI元素关联时有效  
//@参数 hImage 图片句柄.
//@参数 bEnable 启用自动销毁TRUE,关闭自动销毁FALSE.
//@别名 图片_启用自动销毁()
XC_API void WINAPI XImage_EnableAutoDestroy(HIMAGE hImage, BOOL bEnable);
//@备注 启用或关闭图片居中显示，默认属性图片有效。  
//@参数 hImage 图片句柄.
//@参数 bCenter 是否居中显示.
//@别名 图片_启用居中()
XC_API void WINAPI XImage_EnableCenter(HIMAGE hImage, BOOL bCenter);
//@备注 判断图片是否居中显示  
//@参数 hImage 图片句柄.
//@返回 如果居中显示返回TRUE，否则相反.
//@别名 图片_是否居中()
XC_API BOOL WINAPI XImage_IsCenter(HIMAGE hImage);
//@备注 获取图片绘制类型  
//@参数 hImage 图片句柄.
//@返回 图片绘制类型.
//@别名 图片_取绘制类型()
XC_API image_draw_type_ WINAPI XImage_GetDrawType(HIMAGE hImage);
//@备注 获取图片宽度.  
//@参数 hImage 图片句柄.
//@返回 图片宽度.
//@别名 图片_取宽度()
XC_API int WINAPI XImage_GetWidth(HIMAGE hImage);
//@备注 获取图片高度.  
//@参数 hImage 图片句柄.
//@返回 图片高度.
//@别名 图片_取高度()
XC_API int WINAPI XImage_GetHeight(HIMAGE hImage);
//@参数 hImage 图片句柄
//@返回 返回图片源句柄
//@别名 图片_取图片源()
XC_API HIMAGE WINAPI XImage_GetImageSrc(HIMAGE hImage);
//@备注 增加引用计数.  
//@参数 hImage 图片句柄.
//@别名 图片_增加引用计数()
XC_API void WINAPI XImage_AddRef(HIMAGE hImage);
//@备注 释放引用计数,当引用计数为0时,自动销毁.  
//@参数 hImage 图片句柄.
//@别名 图片_释放引用计数()
XC_API void WINAPI XImage_Release(HIMAGE hImage);
//@备注 获取引用计数.  
//@参数 hImage 图片句柄.
//@返回 返回引用计数.
//@别名 图片_取引用计数()
XC_API int WINAPI XImage_GetRefCount(HIMAGE hImage);
//@备注 强制销毁图片, 谨慎使用, 建议使用 @ref XImage_Release() 释放.  
//@参数 hImage 图片句柄.
//@别名 图片_销毁()
XC_API void WINAPI XImage_Destroy(HIMAGE hImage);
//@备注 加图片从字节集 
//@参数 字节集, 图片内存数据
//@返回 图片句柄
//@别名 图片_加载从字节集(字节集)
static HIMAGE XImage_LoadBytes(CXBytes& bytes){
	return XImage_LoadMemory((void*)bytes.getPtr(), (int)bytes.getSize());
}
//@分组}
//@分组{  动画特效

//@备注 将动画加入动画系统中运行
//@参数 hAnimationEx 动画序列或动画组句柄
//@参数 hRedrawObjectUI 当更新UI时重绘的UI层;  UI对象句柄: 窗口句柄,元素句柄,形状句柄,SVG句柄
//@别名 动画_运行()
XC_API void WINAPI XAnima_Run(HXCGUI hAnimationEx, HXCGUI hRedrawObjectUI);
//@备注 将动画从动画系统中移除,并且自动销毁(如果启用自动销毁)
//@参数 hAnimationEx 动画序列或动画组句柄
//@参数 bEnd 是否立即执行到终点
//@别名 动画_释放()
XC_API BOOL WINAPI XAnima_Release(HXCGUI hAnimationEx, BOOL bEnd=TRUE);
//@备注 从动画系统中移除与指定UI对象关联的所有动画, 并且自动销毁(如果启用自动销毁)
//@参数 hObjectUI 指定UI对象句柄
//@参数 bEnd 是否立即执行到终点
//@返回 返回释放动画数量
//@别名 动画_释放扩展()
XC_API int WINAPI XAnima_ReleaseEx(HXCGUI hObjectUI, BOOL bEnd);
//@备注 按顺序执行的动画列表
//@参数 hObjectUI 绑定的UI对象,  UI对象句柄: 窗口句柄,元素句柄,形状句柄,SVG句柄
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@返回 返回句柄
//@别名 动画_创建动画序列()
XC_API HXCGUI WINAPI XAnima_Create(HXCGUI hObjectUI, int nLoopCount=1);
//@备注 移动到目标位置, 默认以UI对象中心点为操作方式,避免出现坐标错位
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 x 终点位置X(对象左上角坐标)
//@参数 y 终点位置Y(对象左上角坐标)
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识 @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_移动()
XC_API HXCGUI WINAPI XAnima_Move(HXCGUI hSequence, UINT duration, float x, float y, int nLoopCount=1, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 从指定位置移动到目标位置, 默认以UI对象中心点为操作方式,避免出现坐标错位
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 from_x 起点位置X(对象左上角坐标)
//@参数 from_y 起点位置Y(对象左上角坐标)
//@参数 to_x 终点位置X(对象左上角坐标)
//@参数 to_y 终点位置Y(对象左上角坐标)
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识 @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_移动扩展()
XC_API HXCGUI WINAPI XAnima_MoveEx(HXCGUI hSequence, UINT duration, float from_x, float from_y, float to_x, float to_y, int nLoopCount=1, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 旋转角度支持负数值, 因为负数可以控制反向旋转
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 angle 角度
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识 @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_旋转()
XC_API HXCGUI WINAPI XAnima_Rotate(HXCGUI hSequence, UINT duration, float angle, int nLoopCount=1, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 指定起点和终点
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 from 起点角度
//@参数 to 终点角度
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识 @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_旋转扩展()
XC_API HXCGUI WINAPI XAnima_RotateEx(HXCGUI hSequence, UINT duration, float from, float to, int nLoopCount=1, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 缩放对象   默认以自身为中心缩放
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 scaleX X轴缩放比例
//@参数 scaleY Y轴缩放比例
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识  @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_缩放()
XC_API HXCGUI WINAPI XAnima_Scale(HXCGUI hSequence, UINT duration, float scaleX, float scaleY, int nLoopCount=0, int ease_flag=0, BOOL bGoBack=TRUE);
//@备注 修改UI对象大小,默认向右延伸
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 width 宽度
//@参数 height 高度
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识 @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_缩放大小()
XC_API HXCGUI WINAPI XAnima_ScaleSize(HXCGUI hSequence, UINT duration, float width, float height, int nLoopCount=1, int ease_flag=0, BOOL bGoBack=FALSE);
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 alpha 透明度
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识 @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_透明度()
XC_API HXCGUI WINAPI XAnima_Alpha(HXCGUI hSequence, UINT duration, BYTE alpha, int nLoopCount=0, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 从指定透明度到目标透明度
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 from_alpha 起始透明度
//@参数 to_alpha 终止透明度
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识  @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_透明度扩展()
XC_API HXCGUI WINAPI XAnima_AlphaEx(HXCGUI hSequence, UINT duration, BYTE from_alpha, BYTE to_alpha, int nLoopCount=0, int ease_flag=0, BOOL bGoBack=FALSE);
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 color 颜色
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识 @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_颜色()
XC_API HXCGUI WINAPI XAnima_Color(HXCGUI hSequence, UINT duration, COLORREF color, int nLoopCount=0, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 从指定颜色到目标颜色
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 from 起点颜色
//@参数 to 终点颜色
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识 @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_颜色扩展()
XC_API HXCGUI WINAPI XAnima_ColorEx(HXCGUI hSequence, UINT duration, COLORREF from, COLORREF to, int nLoopCount=0, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 修改布局宽度属性
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 nType 布局宽度类型
//@参数 width 布局宽度
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识  @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_布局宽度()
XC_API HXCGUI WINAPI XAnima_LayoutWidth(HXCGUI hSequence, UINT duration, layout_size_ nType, float width, int nLoopCount=1, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 修改布局高度属性
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 nType 布局高度类型
//@参数 height 布局高度
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识  @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_布局高度()
XC_API HXCGUI WINAPI XAnima_LayoutHeight(HXCGUI hSequence, UINT duration, layout_size_ nType, float height, int nLoopCount=1, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 修改布局宽度和高度
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 nWidthType 布局宽度类型, @ref layout_size_disable : 禁用宽度动画
//@参数 width 布局宽度
//@参数 nHeightType 布局高度类型, @ref layout_size_disable : 禁用高度动画
//@参数 height 布局高度
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识  @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_布局大小()
XC_API HXCGUI WINAPI XAnima_LayoutSize(HXCGUI hSequence, UINT duration, layout_size_ nWidthType, float width, layout_size_ nHeightType, float height, int nLoopCount=0, int ease_flag=0, BOOL bGoBack=TRUE);
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@返回 返回动画项句柄
//@别名 动画_延迟()
XC_API HXCGUI WINAPI XAnima_Delay(HXCGUI hSequence, float duration);
//@备注 可以作为一个空动画, 然后在回调里处理自己的算法
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@参数 ease_flag 缓动标识  @ref ease_flag_
//@参数 bGoBack 是否返回; 当启用后:往返到起点,  起点->终点->起点
//@返回 返回动画项句柄
//@别名 动画_延迟扩展()
XC_API HXCGUI WINAPI XAnima_DelayEx(HXCGUI hSequence, float duration, int nLoopCount=1, int ease_flag=0, BOOL bGoBack=FALSE);
//@备注 显示或隐藏UI对象
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@参数 bShow 显示或隐藏
//@返回 返回动画项句柄
//@别名 动画_显示()
XC_API HXCGUI WINAPI XAnima_Show(HXCGUI hSequence, float duration, BOOL bShow);
//@参数 hSequence 动画序列句柄
//@参数 duration 持续时间
//@返回 返回动画项句柄
//@别名 动画_销毁UI对象()
XC_API HXCGUI WINAPI XAnima_DestroyObjectUI(HXCGUI hSequence, float duration);
//@备注 TRUE:自动销毁 FALSE:手动销毁
//@参数 hAnimationEx 动画序列或动画组句柄
//@参数 bEnable 是否启用
//@别名 动画_启用自动销毁()
XC_API void WINAPI XAnima_EnableAutoDestroy(HXCGUI hAnimationEx, BOOL bEnable);
//@备注 获取动画关联的UI对象
//@参数 hAnimationEx 动画序列或动画组或动画项句柄
//@返回 返回UI对象句柄
//@别名 动画_取UI对象()
XC_API HXCGUI WINAPI XAnima_GetObjectUI(HXCGUI hAnimationEx);
//@参数 hAnimationEx 动画序列或动画组句柄
//@参数 callback 回调函数
//@别名 动画_置回调()
XC_API void WINAPI XAnima_SetCallback(HXCGUI hAnimationEx, funAnimation callback);
//@参数 hAnimationEx 动画序列或动画组句柄
//@参数 nUserData 用数据
//@别名 动画_置用户数据()
XC_API void WINAPI XAnima_SetUserData(HXCGUI hAnimationEx, vint nUserData);
//@参数 hAnimationEx 动画序列或动画组句柄
//@返回 返回用户数据
//@别名 动画_取用户数据()
XC_API vint WINAPI XAnima_GetUserData(HXCGUI hAnimationEx);
//@参数 hAnimationEx 动画序列或动画组句柄
//@别名 动画_停止()
XC_API void WINAPI XAnima_Stop(HXCGUI hAnimationEx);
//@参数 hAnimationEx 动画序列或动画组句柄
//@别名 动画_开始()
XC_API void WINAPI XAnima_Start(HXCGUI hAnimationEx);
//@备注 未实现功能
//@参数 hAnimationEx 动画序列或动画组句柄
//@别名 动画_暂停()
XC_API void WINAPI XAnima_Pause(HXCGUI hAnimationEx);
//@分组}
//@分组{  动画特效

//@备注 动画同步组, 当组中动画序列全部完成后, 重新开始; 当遇到无限循环项时,直至其他序列完成后终止循环,避免出现无法到达终点,无法返回头部进行同步
//@参数 nLoopCount 动画循环次数, 0:无限循环
//@返回 返回动画组句柄
//@别名 动画组_创建()
XC_API HXCGUI WINAPI XAnimaGroup_Create(int nLoopCount=1);
//@备注 将动画序列添加到组中
//@参数 hGroup 动画组句柄
//@参数 hSequence 动画序列句柄
//@别名 动画组_添加项()
XC_API void WINAPI XAnimaGroup_AddItem(HXCGUI hGroup, HXCGUI hSequence);
//@分组}
//@分组{  动画特效

//@参数 hAnimationItem 动画项句柄
//@参数 bEnable 是否启用
//@别名 动画项_启用自动销毁()
XC_API void WINAPI XAnimaItem_EnableAutoDestroy(HXCGUI hAnimationItem, BOOL bEnable);
//@备注 当动画项完成后自动释放
//例如对多个动画序列进行渐近式延迟, 在动画序列头标添加延时项(时间差), 当延时项完成时自动释放, 后续动画循环就形成一种时间差(因为对齐的时间差销毁了,他们永远无法对齐时间)
//@参数 hAnimationItem 动画项句柄
//@参数 bEnable 是否启用
//@别名 动画项_启用完成释放()
XC_API void WINAPI XAnimaItem_EnableCompleteRelease(HXCGUI hAnimationItem, BOOL bEnable);
//@参数 hAnimationItem 动画项句柄
//@参数 callback 回调函数
//@别名 动画项_置回调()
XC_API void WINAPI XAnimaItem_SetCallback(HXCGUI hAnimationItem, funAnimationItem callback);
//@参数 hAnimationItem 动画项句柄
//@参数 nUserData 用户数据
//@别名 动画项_置用户数据()
XC_API void WINAPI XAnimaItem_SetUserData(HXCGUI hAnimationItem, vint nUserData);
//@参数 hAnimationItem 动画项句柄
//@返回 返回用户数据
//@别名 动画项_取用户数据()
XC_API vint WINAPI XAnimaItem_GetUserData(HXCGUI hAnimationItem);
//@分组}
//@分组{  动画特效

//@备注 设置旋转中心点坐标
//@参数 hAnimationRotate 动画旋转项句柄
//@参数 x 坐标X
//@参数 y 坐标Y
//@参数 bOffset TRUE:相对于自身中心点偏移, FALSE:绝对坐标
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 动画旋转_置中心()
XC_API void WINAPI XAnimaRotate_SetCenter(HXCGUI hAnimationRotate, float x, float y, BOOL bOffset=FALSE);
//@分组}
//@分组{  动画特效

//@备注 设置缩放起点, 确定延伸方向
//@参数 hAnimationScale 动画缩放项句柄
//@参数 position 位置
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 动画缩放_置延伸位置()
XC_API void WINAPI XAnimaScale_SetPosition(HXCGUI hAnimationScale, position_flag_ position);
//@分组}
//@分组{  动画特效

//@备注 此接口可独立设置x轴移动或y轴移动
//@参数 hAnimationMove 动画移动项句柄
//@参数 flags @ref animation_move_x : X轴移动, @ref animation_move_y : Y轴移动, 可组合使用, @ref animation_move_
//@别名 动画移动_置标识()
XC_API void WINAPI XAnimaMove_SetFlag(HXCGUI hAnimationMove, int flags);
//@分组}
//@分组{  托盘图标

//@备注 重置数据,  当未添加到系统托盘状态才可调用 
//@别名 托盘图标_重置()
XC_API void WINAPI XTrayIcon_Reset();
//@备注 将图标添加到系统托盘
//@参数 hWindow 关联窗口句柄
//@参数 id 托盘图标唯一标识符
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 托盘图标_添加()
XC_API BOOL WINAPI XTrayIcon_Add(HWINDOW hWindow, int id);
//@备注 从系统托盘删除
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 托盘图标_删除()
XC_API BOOL WINAPI XTrayIcon_Del();
//@备注 修改托盘图标
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 托盘图标_修改()
XC_API BOOL WINAPI XTrayIcon_Modify();
//@备注 设置图标
//@参数 hIcon 图标句柄
//@别名 托盘图标_置图标()
XC_API void WINAPI XTrayIcon_SetIcon(HICON hIcon);
//@备注 将焦点设置回系统托盘
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 托盘图标_置焦点()
XC_API BOOL WINAPI XTrayIcon_SetFocus();
//@备注 设置工具提示内容
//@参数 pTips 提示文本内容, 长度不能超过127个字符
//@别名 托盘图标_置提示文本()
XC_API void WINAPI XTrayIcon_SetTips(const wchar_t* pTips);
//@备注 设置弹出气泡信息
//@参数 pTitle 弹出气泡标题
//@参数 pText 弹出气泡内容
//@参数 hBalloonIcon 气泡图标
//@参数 flags 标识, 可设置默认图标类型, 禁用声音,  @ref trayIcon_flag_
//@别名 托盘图标_置弹出气泡()
XC_API void WINAPI XTrayIcon_SetPopupBalloon(const wchar_t* pTitle, const wchar_t* pText, HICON hBalloonIcon=NULL, int flags=0);
//@备注 设置用户自定义的回调消息类型, 触发事件后, 系统会发送到此消息
//@参数 user_message 用户自定义消息, 界面库默认定义消息为 @ref XWM_TRAYICON
//@别名 托盘图标_置回调消息()
XC_API void WINAPI XTrayIcon_SetCallbackMessage(UINT user_message);
//@分组}
//@分组{  全局API

//@备注 Unicode转换Ansi编码, 
//@参数 pIn 指向待转换的Unicode字符串指针.
//@参数 inLen pIn字符数量.
//@参数 pOut 指向接收转换后的Ansi字符串缓冲区指针.
//@参数 outLen pOut缓冲区大小,字节单位.
//@返回 如果成功,返回写入接收缓冲区字节数量.
//@别名 炫彩_U2A()
XC_API int WINAPI XC_UnicodeToAnsi(const wchar_t* pIn, int inLen, char* pOut, int outLen);
//@备注 Ansi转换Unicode编码, 
//@参数 pIn 指向待转换的Ansi字符串指针.
//@参数 inLen pIn字符数量.
//@参数 pOut 指向接收转换后的Unicode字符串缓冲区指针.
//@参数 outLen pOut缓冲区大小,字符wchar_t单位.
//@返回 如果成功,返回写入接收缓冲区字符wchar_t数量.
//@别名 炫彩_A2U()
XC_API int WINAPI XC_AnsiToUnicode(const char* pIn, int inLen, wchar_t* pOut, int outLen);
//@参数 hWindow 窗口句柄
//@参数 msg 
//@参数 wParam 
//@参数 lParam 
//@返回 成功返回TRUE,否则返回FALSE
//@别名 炫彩_发送窗口消息()
XC_API LRESULT WINAPI XC_SendMessage(HWINDOW hWindow, UINT msg, WPARAM wParam, LPARAM lParam);
//@参数 hWindow 窗口句柄
//@参数 msg 
//@参数 wParam 
//@参数 lParam 
//@别名 炫彩_投递窗口消息()
XC_API BOOL WINAPI XC_PostMessage(HWINDOW hWindow, UINT msg, WPARAM wParam, LPARAM lParam);
//@备注 调用UI线程, 设置回调函数,在回调函数里操作UI,通过SendMessage()实现,阻塞模式
//@参数 pCall 回调函数 funCallUiThread ; vint CALLBACK funCallUiThread(vint data)
//@参数 data 用户自定义数据
//@返回 回调函数返回值
//@别名 炫彩_调用界面线程()
XC_API vint WINAPI XC_CallUiThread(funCallUiThread pCall, vint data);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240.  
//@参数 nValue 参数.
//@返回 返回结果.
//@别名 炫彩_整数到文本A()
XC_API const char* WINAPI XC_itoa(int nValue);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放.  
//@参数 nValue 参数.
//@返回 返回结果.
//@别名 炫彩_整数到文本W()
XC_API const wchar_t* WINAPI XC_itow(int nValue);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放. 
//@参数 fValue 参数.
//@返回 返回结果.
//@别名 炫彩_浮点数到文本A()
XC_API const char* WINAPI XC_ftoa(float fValue);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放.  
//@参数 fValue 参数.
//@返回 返回结果.
//@别名 炫彩_浮点数到文本W()
XC_API const wchar_t* WINAPI XC_ftow(float fValue);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放.  
//@参数 pValue 参数.
//@返回 返回结果.
//@别名 炫彩_A2W()
XC_API const wchar_t* WINAPI XC_atow(const char* pValue);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放.  
//@参数 pValue 参数.
//@返回 返回结果.
//@别名 炫彩_W2A()
XC_API const char* WINAPI XC_wtoa(const wchar_t* pValue);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放.  
//@参数 pUtf8 参数.
//@返回 返回结果.
//@别名 炫彩_UTF8到文本W()
XC_API const wchar_t* WINAPI XC_utf8tow(const char* pUtf8);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放.  
//@参数 pUtf8 utf8字符串指针.
//@参数 length utf8字符串长度.
//@返回 返回结果.
//@别名 炫彩_UTF8到文本W扩展()
XC_API const wchar_t* WINAPI XC_utf8towEx(const char* pUtf8, int length);
//@备注 转换函数, 返回结果最大长度10240, 使用共享缓存,不需要释放.  
//@参数 pUtf8 utf8字符串指针.
//@返回 返回结果.
//@别名 炫彩_UTF8到文本A()
XC_API const char* WINAPI XC_utf8toa(const char* pUtf8);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放.  
//@参数 pValue 参数.
//@返回 返回结果.
//@别名 炫彩_文本A到UTF8()
XC_API const char* WINAPI XC_atoutf8(const char* pValue);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240,, 使用共享缓存,不需要释放. 
//@参数 pValue 字符串指针
//@返回 返回结果
//@别名 炫彩_文本W到UTF8()
XC_API const char* WINAPI XC_wtoutf8(const wchar_t* pValue);
//@备注 转换函数, 返回结果最大长度 TEXT_BUFFER_SIZE = 10240, 使用共享缓存,不需要释放.   
//@参数 pValue 字符串指针
//@参数 length 字符串长度
//@返回 返回结果
//@别名 炫彩_文本W到UTF8扩展()
XC_API const char* WINAPI XC_wtoutf8Ex(const wchar_t* pValue, int length);
//@备注 打印调试信息到文件xcgui_debug.txt.  
//@参数 pInfo 
//@别名 炫彩_输出调试信息到文件()
XC_API void WINAPI XC_DebugToFileInfo(const char* pInfo);
//@备注 判断是否为元素句柄. 
//@参数 hEle 元素句柄.
//@返回 成功返回TRUE,否则相反.
//@别名 炫彩_是否元素()
XC_API BOOL WINAPI XC_IsHELE(HXCGUI hEle);
//@备注 判断是否为窗口句柄. 
//@参数 hWindow 窗口句柄.
//@返回 成功返回TRUE,否则相反.
//@别名 炫彩_是否窗口()
XC_API BOOL WINAPI XC_IsHWINDOW(HXCGUI hWindow);
//@备注 判断是否为形状对象. 
//@参数 hShape 形状对象句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 炫彩_是否形状对象()
XC_API BOOL WINAPI XC_IsShape(HXCGUI hShape);
//@备注 判断句柄是否拥有该类型.  
//@参数 hXCGUI 炫彩句柄.
//@参数 nType 句柄类型.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 炫彩_是否句柄包含类型()
XC_API BOOL WINAPI XC_IsHXCGUI(HXCGUI hXCGUI, XC_OBJECT_TYPE nType);
//@备注 通过窗口HWND句柄获取HWINDOW句柄.  
//@参数 hWnd 窗口HWND句柄.
//@返回 返回HWINDOW句柄.
//@别名 炫彩_转换HWND到HWINDOW()
XC_API HWINDOW WINAPI XC_hWindowFromHWnd(HWND hWnd);
//@备注 激活当前进程最上层窗口  
//@返回 成功返回TRUE,否则返回FALSE
//@别名 炫彩_激活窗口()
XC_API BOOL WINAPI XC_SetActivateTopWindow();
//@备注 设置对象属性.  
//@参数 hXCGUI 对象句柄.
//@参数 pName 属性名.
//@参数 pValue 属性值, 如果是资源, 需要加上"@"符号, 例如: "@资源名"
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 炫彩_置属性()
XC_API BOOL WINAPI XC_SetProperty(HXCGUI hXCGUI, const wchar_t* pName, const wchar_t* pValue);
//@备注 获取对象属性.  
//@参数 hXCGUI 对象句柄. 
//@参数 pName 属性名.
//@返回 返回属性值, 如果没有返回空.
//@别名 炫彩_取属性()
XC_API const wchar_t* WINAPI XC_GetProperty(HXCGUI hXCGUI, const wchar_t* pName);
//@备注 注册窗口类名
//当炫彩退出时, 会自动注销类名; 假如类名没有注销, DLL卸载后, 类名所指向的窗口过程地址失效, 再次使用此类名, 造成程序崩溃
//@参数 pClassName 类名. 
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 炫彩_注册窗口类名()
XC_API BOOL WINAPI XC_RegisterWindowClassName(const wchar_t* pClassName);
//@备注 判断元素是否从滚动视图元素扩展的新元素,包含滚动视图元素.  
//@参数 hEle 元素句柄.
//@返回 如果成功返回TRUE,否则相反.
//@别名 炫彩_是否滚动视图扩展元素()
XC_API BOOL WINAPI XC_IsSViewExtend(HELE hEle);
//@备注 获取句柄类型.   
//@参数 hXCGUI 炫彩对象句柄.
//@返回 返回句柄类型.
//@别名 炫彩_取对象类型()
XC_API XC_OBJECT_TYPE WINAPI XC_GetObjectType(HXCGUI hXCGUI);
//@备注 通过ID获取对象句柄,不包括窗口对象.   
//@参数 hWindow 所属窗口句柄,如果不属于任何窗口填NULL.
//@参数 nID ID值.
//@返回 成功返回句柄,否则返回NULL.
//@别名 炫彩_取对象从ID()
XC_API HXCGUI WINAPI XC_GetObjectByID(HWINDOW hWindow, int nID);
//@备注 通过ID名称获取对象句柄.   
//@参数 hWindow 所属窗口句柄,如果不属于任何窗口填NULL.
//@参数 pName ID名称.
//@返回 成功返回句柄,否则返回NULL.
//@别名 炫彩_取对象从ID名称()
XC_API HXCGUI WINAPI XC_GetObjectByIDName(HWINDOW hWindow, const wchar_t* pName);
//@备注 通过UID获取对象句柄,不包括窗口对象.  
//@参数 nUID UID值.
//@返回 成功返回句柄,否则返回NULL.
//@别名 炫彩_取对象从UID()
XC_API HXCGUI WINAPI XC_GetObjectByUID(int nUID);
//@备注 通过UID名称获取对象句柄.  
//@参数 pName UID名称.
//@返回 成功返回句柄,否则返回NULL.
//@别名 炫彩_取对象从UID名称()
XC_API HXCGUI WINAPI XC_GetObjectByUIDName(const wchar_t* pName);
//@备注 通过name获取对象句柄.  
//@参数 pName name名称.
//@返回 成功返回句柄,否则返回NULL.
//@别名 炫彩_取对象从名称()
XC_API HXCGUI WINAPI XC_GetObjectByName(const wchar_t* pName);
//@备注 获取当前所使用的句柄总数量
//@返回 返回当前所使用的句柄总数量
//@别名 炫彩_取句柄总数()
XC_API int WINAPI XC_GetHandleCount();
//@备注 设置UI的最小重绘频率.  
//@参数 nMilliseconds 重绘最小时间间隔,单位毫秒.
//@别名 炫彩_置绘制频率()
XC_API void WINAPI XC_SetPaintFrequency(UINT nMilliseconds);
//@备注 设置文本渲染质量GDI+.  
//@参数 nType 参见GDI+ TextRenderingHint 定义.
//@别名 炫彩_置文本炫彩质量()
XC_API void WINAPI XC_SetTextRenderingHint(int nType);
//@参数 mode 渲染模式  @ref XC_DWRITE_RENDERING_MODE
//@别名 炫彩_置D2D文本渲染模式()
XC_API void WINAPI XC_SetD2dTextRenderingMode(XC_DWRITE_RENDERING_MODE mode);
//@备注 当背景半透明时, 需要使用灰度抗锯齿或不抗锯齿, 因为ClearType模式 文字透明通道混合有问题
//@参数 mode 0:使用系统默认的抗锯齿模式, 1:使用 ClearType 抗锯齿技术, 2:使用灰度抗锯齿, 3:不使用抗锯齿
//@别名 炫彩_置D2D文本抗锯齿模式()
XC_API void WINAPI XC_SetD2dTextAntialiasMode(int mode);
//@备注 将影响到以下函数: XDraw_TextOut XDraw_TextOutEx XDraw_TextOutA
//@参数 bEnable 是否启用
//@别名 炫彩_启用GDI绘制文本()
XC_API void WINAPI XC_EnableGdiDrawText(BOOL bEnable);
//@备注 当修改UI后将自动调用重绘函数更新UI, 例如按钮: XBtn_SetText(), 将自动调用 XEle_Redraw() 更新UI
//@参数 bEnable 是否启用
//@别名 炫彩_启用自动重绘UI()
XC_API void WINAPI XC_EnableAutoRedrawUI(BOOL bEnable);
//@备注 判断两个矩形是否相交及重叠. 
//@参数 pRect1 矩形1.
//@参数 pRect2 矩形2.
//@返回 如果两个矩形相交返回TRUE,否则相反.
//@别名 炫彩_是否矩形相交()
XC_API BOOL WINAPI XC_RectInRect(RECT* pRect1, RECT* pRect2);
//@备注 组合两个矩形区域.  
//@参数 pDest 新的矩形区域.
//@参数 pSrc1 源矩形1.
//@参数 pSrc2 源矩形2.
//@别名 炫彩_组合矩形()
XC_API void WINAPI XC_CombineRect(RECT* pDest, RECT* pSrc1, RECT* pSrc2);
//@备注 显示布局对象边界.  
//@参数 bShow 是否显示.
//@别名 炫彩_显示布局边界()
XC_API void WINAPI XC_ShowLayoutFrame(BOOL bShow);
//@备注 辅助功能
//@参数 bShow 是否显示
//@别名 炫彩_显示边界()
XC_API void WINAPI XC_ShowSvgFrame(BOOL bShow);
//@备注 启用debug文件.  
//@参数 bEnable 是否启用.
//@别名 炫彩_启用debug文件()
XC_API void WINAPI XC_EnableDebugFile(BOOL bEnable);
//@备注 启用资源监视器.  
//@参数 bEnable 是否启用.
//@别名 炫彩_启用资源监视器()
XC_API void WINAPI XC_EnableResMonitor(BOOL bEnable);
//@备注 设置布局边框颜色.  
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 炫彩_置布局边界颜色()
XC_API void WINAPI XC_SetLayoutFrameColor(COLORREF color);
//@备注 启用错误弹出,通过该接口可以设置遇到严重错误时不弹出消息提示框.  
//@参数 bEnable 是否启用.
//@别名 炫彩_启用错误弹窗()
XC_API void WINAPI XC_EnableErrorMessageBox(BOOL bEnable);
//@备注 启用DPI几种方式: 1.在项目属性中启用DPI, 2.使用清单文件, 3.调用此函数, 4.自己调用DPI函数,
//参考MSDN: https://learn.microsoft.com/zh-cn/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process
//@参数 bEnable 是否启用.
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 炫彩_启用DPI()
XC_API BOOL WINAPI XC_EnableDPI(BOOL bEnable);
//@备注 默认禁用; 当启用后, 创建窗口时自动检测DPI调整UI缩放, 处理DPI改变消息; 禁用后,当DPI改变,需要手动设置窗口DPI
//@参数 bEnable 是否启用
//@别名 炫彩_启用自动DPI()
XC_API void WINAPI XC_EnableAutoDPI(BOOL bEnable);
//@备注 启动或禁用自动退出程序,当检测到所有用户创建的窗口都关闭时,自动退出程序; 
//可调用 XC_PostQuitMessage() 手动退出程序
//@参数 bEnable 是否启用.
//@别名 炫彩_启用自动退出程序()
XC_API void WINAPI XC_EnableAutoExitApp(BOOL bEnable);

//@备注  当启用后拥有系统非客户区样式, 替代模拟非客户区
//@参数  bEnable 是否启用.
//@别名  炫彩_启用窗口系统非客户区()
XC_API void WINAPI XC_EnableWindowSysNc(BOOL bEnable);

//@备注 判断是否启用了D2D
//@返回 如果启用D2D返回TRUE,否则相反
//@别名 炫彩_是否启用D2D()
XC_API BOOL WINAPI XC_IsEnableD2D();
//@备注 获取文本绘制大小.  
//@参数 pString 字符串.
//@参数 length 字符串长度
//@参数 hFontX 字体.
//@参数 pOutSize 接收返回大小.
//@别名 炫彩_取文本绘制大小()
XC_API void WINAPI XC_GetTextSize(const wchar_t* pString, int length, HFONTX hFontX, SIZE* pOutSize);
//@备注 获取文本显示大小.  
//@参数 pString 字符串.
//@参数 length 字符串长度
//@参数 hFontX 字体.
//@参数 pOutSize 接收返回大小.
//@别名 炫彩_取文本显示大小()
XC_API void WINAPI XC_GetTextShowSize(const wchar_t* pString, int length, HFONTX hFontX, SIZE* pOutSize);
//@参数 pString 字符串.
//@参数 length 字符串长度
//@参数 hFontX 字体.
//@参数 nTextAlign 文本对齐方式 @ref textFormatFlag_
//@参数 pOutSize 接收返回大小.
//@别名 炫彩_取文本显示大小扩展()
XC_API void WINAPI XC_GetTextShowSizeEx(const wchar_t* pString, int length, HFONTX hFontX, int nTextAlign, SIZE* pOutSize);
//@参数 pString 字符串.
//@参数 length 字符串长度
//@参数 hFontX 字体.
//@参数 nTextAlign 文本对齐  @ref textFormatFlag_
//@参数 width 最大宽度
//@参数 pOutSize 接收返回大小.
//@别名 炫彩_取文本显示矩形()
XC_API void WINAPI XC_GetTextShowRect(const wchar_t* pString, int length, HFONTX hFontX, int nTextAlign, int width, SIZE* pOutSize);
//@备注 获取默认字体.  
//@返回 返回默认字体句柄.
//@别名 炫彩_取默认字体()
XC_API HFONTX WINAPI XC_GetDefaultFont();
//@备注 设置默认字体. 
//@参数 hFontX 炫彩字体句柄.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 炫彩_置默认字体()
XC_API void WINAPI XC_SetDefaultFont(HFONTX hFontX);
//@备注 添加文件搜索路径, 默认路为exe目录和程序当前运行目录.  
//@参数 pPath 文件夹, 目录, 路径.
//@别名 炫彩_添加搜索路径()
XC_API void WINAPI XC_AddFileSearchPath(const wchar_t* pPath);
//@备注 初始化LOGFONTW结构体.  
//@参数 pFont LOGFONTW结构体指针.
//@参数 pName 字体名称.
//@参数 size 字体大小.
//@参数 bBold 是否为粗体.
//@参数 bItalic 是否为斜体.
//@参数 bUnderline 是否有下划线.
//@参数 bStrikeOut 是否有删除线.
//@别名 炫彩_初始化字体()
XC_API void WINAPI XC_InitFont(LOGFONTW* pFont, wchar_t* pName, int size, BOOL bBold=FALSE, BOOL bItalic=FALSE, BOOL bUnderline=FALSE, BOOL bStrikeOut=FALSE);
//@备注 在UI库中申请内存.  
//@参数 size 大小,字节为单位.
//@返回 内存首地址.
//@别名 炫彩_分配内存()
XC_API void* WINAPI XC_Malloc(int size);
//@备注 在UI库中释放内存.  
//@参数 p 内存首地址.
//@别名 炫彩_释放内存()
XC_API void WINAPI XC_Free(void* p);
//@备注 弹出提示框.  
//@参数 pTitle 提示框标题
//@参数 pText 提示内容
//@别名 炫彩_弹框()
XC_API void WINAPI XC_Alert(const wchar_t* pTitle, const wchar_t* pText);
//@备注 参见系统API ShellExecute()  
//@参数 hwnd 
//@参数 lpOperation 
//@参数 lpFile 
//@参数 lpParameters 
//@参数 lpDirectory 
//@参数 nShowCmd 
//@返回 执行成功会返回应用程序句柄
//@别名 炫彩_系统_ShellExecute()
XC_API HINSTANCE WINAPI XC_Sys_ShellExecute(HWND hwnd, const wchar_t* lpOperation, const wchar_t* lpFile, const wchar_t* lpParameters, const wchar_t* lpDirectory, int nShowCmd);
//@备注 系统API LoadLibrary   
//@参数 lpFileName 文件名
//@返回 返回动态库模块句柄
//@别名 炫彩_载入动态库()
XC_API HMODULE WINAPI XC_LoadLibrary(const wchar_t* lpFileName);
//@备注 系统API GetProcAddress  
//@参数 hModule 动态库模块句柄
//@参数 lpProcName 函数名
//@返回 返回函数地址
//@别名 炫彩_取动态库中函数地址()
XC_API FARPROC WINAPI XC_GetProcAddress(HMODULE hModule, const char* lpProcName);
//@备注 系统API FreeLibrary   
//@参数 hModule 动态库模块句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 炫彩_释放动态库()
XC_API BOOL WINAPI XC_FreeLibrary(HMODULE hModule);
//@备注 加载指定DLL,并且调用DLL中函数LoadDll(),  DLL中导出函数格式: int WINAPI LoadDll()     
//@参数 pDllFileName DLL文件名
//@返回 返回DLL模块句柄
//@别名 炫彩_加载DLL()
XC_API HMODULE WINAPI XC_LoadDll(const wchar_t* pDllFileName);

//@备注  为加载布局文件设置回调函数, 当调用"XC_LoadLayout()"时遇到扩展对象, 回调用户设置的回调函数, 让用户在回调函数中创建扩展对象 
//@参数  fun  回调函数
//@别名  炫彩_置回调_加载布局文件()
XC_API void WINAPI XC_SetCallBack_LoadLayout(funLoadLayout fun);

//@备注 当调用"XC_LoadLayout()"时遇到扩展对象,用户可在回调函数中创建内置UI对象
//@参数 data     内部使用,包含布局文件相关信息
//@参数 propertylist  属性列表
//@参数 uiType   UI类型
//@参数 hParent  父句柄
//@返回 返回创建的UI对象句柄
//@别名 炫彩_加载布局文件_创建()
XC_API HXCGUI WINAPI XC_LoadLayout_Create(vint data, vint propertylist, XC_OBJECT_TYPE uiType, HXCGUI hParent);

//@备注 从属性列表中, 获取指定属性名称的值
//@参数 propertylist   属性列表
//@参数 pName     属性名
//@返回 返回属性值
//@别名 属性列表_取文本()
XC_API const wchar_t* WINAPI XPropertyList_GetString(vint propertylist, const wchar_t* pName);

//@备注 获取属性列表成员数量
//@参数 propertylist   属性列表
//@返回 返回成员数量
//@别名 属性列表_取大小()
XC_API int            WINAPI XPropertyList_GetSize(vint propertylist);

//@备注 加载布局文件.  
//@参数 pFileName 布局文件名.
//@参数 hParent 父对象句柄,窗口句柄或UI元素句柄.
//@参数 hAttachWnd 附加到指定的窗口HWND,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件()
XC_API HXCGUI WINAPI XC_LoadLayout(const wchar_t* pFileName, HXCGUI hParent=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件从zip压缩包中.  
//@参数 pZipFileName zip文件名.
//@参数 pFileName 布局文件名.
//@参数 pPassword zip密码.
//@参数 hParent 父对象句柄,窗口句柄或UI元素句柄.
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件ZIP()
XC_API HXCGUI WINAPI XC_LoadLayoutZip(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HXCGUI hParent=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件从zip压缩包中.  
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 布局文件名
//@参数 pPassword zip密码
//@参数 hParent 父对象句柄,窗口句柄或UI元素句柄.
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件内存ZIP()
XC_API HXCGUI WINAPI XC_LoadLayoutZipMem(const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HXCGUI hParent=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件从内存字符串.  
//@参数 pStringXML 字符串指针.
//@参数 hParent 父对象,窗口句柄或UI元素句柄.
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件从字符串()
XC_API HXCGUI WINAPI XC_LoadLayoutFromString(const char* pStringXML, HXCGUI hParent=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件从内存字符串.  
//@参数 pStringXML 字符串指针.
//@参数 hParent 父对象,窗口句柄或UI元素句柄.
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件从字符串UTF8()
XC_API HXCGUI WINAPI XC_LoadLayoutFromStringUtf8(const char* pStringXML, HXCGUI hParent=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件
//@参数 pFileName 布局文件名
//@参数 pPrefixName 名称(name)前缀, 可选参数; 给当前布局文件中所有name属性增加前缀, 那么name属性值为: 前缀 + name;
//@参数 hParent 父对象句柄,窗口句柄或UI元素句柄
//@参数 hParentWnd 父窗口句柄HWND, 提供给第三方窗口使用
//@参数 hAttachWnd 附加到指定的窗口HWND,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件扩展()
XC_API HXCGUI WINAPI XC_LoadLayoutEx(const wchar_t* pFileName, const wchar_t* pPrefixName=NULL, HXCGUI hParent=NULL, HWND hParentWnd=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件从zip压缩包中
//@参数 pZipFileName zip文件名
//@参数 pFileName 布局文件名
//@参数 pPassword zip密码
//@参数 pPrefixName 名称(name)前缀, 可选参数; 给当前布局文件中所有name属性增加前缀, 那么name属性值为: 前缀 + name;
//@参数 hParent 父对象句柄,窗口句柄或UI元素句柄.
//@参数 hParentWnd 父窗口句柄HWND, 提供给第三方窗口使用
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件ZIP扩展()
XC_API HXCGUI WINAPI XC_LoadLayoutZipEx(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword=NULL, const wchar_t* pPrefixName=NULL, HXCGUI hParent=NULL, HWND hParentWnd=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件从zip压缩包中
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 布局文件名
//@参数 pPassword zip密码
//@参数 pPrefixName 名称(name)前缀, 可选参数; 给当前布局文件中所有name属性增加前缀, 那么name属性值为: 前缀 + name;
//@参数 hParent 父对象句柄,窗口句柄或UI元素句柄
//@参数 hParentWnd 父窗口句柄HWND, 提供给第三方窗口使用
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件内存ZIP扩展()
XC_API HXCGUI WINAPI XC_LoadLayoutZipMemEx(const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword=NULL, const wchar_t* pPrefixName=NULL, HXCGUI hParent=NULL, HWND hParentWnd=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件从RC资源zip压缩包中, RC资源类型必须为:"RT_RCDATA"
//@参数 id RC资源ID
//@参数 pFileName 布局文件名
//@参数 pPassword zip密码
//@参数 pPrefixName 名称(name)前缀, 可选参数; 给当前布局文件中所有name属性增加前缀, 那么name属性值为: 前缀 + name;
//@参数 hParent 父对象句柄,窗口句柄或UI元素句柄
//@参数 hParentWnd 父窗口句柄HWND, 提供给第三方窗口使用
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@参数 hModule 模块句柄
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件资源ZIP扩展()
XC_API HXCGUI WINAPI XC_LoadLayoutZipResEx(int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, const wchar_t* pPrefixName=NULL, HXCGUI hParent=NULL, HWND hParentWnd=NULL, HWND hAttachWnd=NULL, HMODULE hModule=NULL);
//@备注 加载布局文件从内存字符串.
//@参数 pStringXML 字符串指针
//@参数 pPrefixName 名称(name)前缀, 可选参数; 给当前布局文件中所有name属性增加前缀, 那么name属性值为: 前缀 + name;
//@参数 hParent 父对象,窗口句柄或UI元素句柄
//@参数 hParentWnd 父窗口句柄HWND, 提供给第三方窗口使用
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件从字符串扩展()
XC_API HXCGUI WINAPI XC_LoadLayoutFromStringEx(const char* pStringXML, const wchar_t* pPrefixName=NULL, HXCGUI hParent=NULL, HWND hParentWnd=NULL, HWND hAttachWnd=NULL);
//@备注 加载布局文件从内存字符串
//@参数 pStringXML 字符串指针
//@参数 pPrefixName 名称(name)前缀, 可选参数; 给当前布局文件中所有name属性增加前缀, 那么name属性值为: 前缀 + name;
//@参数 hParent 父对象,窗口句柄或UI元素句柄
//@参数 hParentWnd 父窗口句柄HWND, 提供给第三方窗口使用
//@参数 hAttachWnd 附加窗口句柄, 附加到指定的窗口,如果未指定忽略
//@返回 返回窗口句柄或元素句柄
//@别名 炫彩_加载布局文件从字符串UTF8扩展()
XC_API HXCGUI WINAPI XC_LoadLayoutFromStringUtf8Ex(const char* pStringXML, const wchar_t* pPrefixName=NULL, HXCGUI hParent=NULL, HWND hParentWnd=NULL, HWND hAttachWnd=NULL);
//@备注 加载样式文件.  
//@参数 pFileName 样式文件名称.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 炫彩_加载样式文件()
XC_API BOOL WINAPI XC_LoadStyle(const wchar_t* pFileName);
//@参数 pZipFile ZIP文件名
//@参数 pFileName 文件名
//@参数 pPassword 密码
//@返回 成功返回TRUE,否则返回FALSE
//@别名 炫彩_加载样式文件ZIP()
XC_API BOOL WINAPI XC_LoadStyleZip(const wchar_t* pZipFile, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 文件名
//@参数 pPassword 密码
//@返回 成功返回TRUE,否则返回FALSE
//@别名 炫彩_加载样式文件从内存ZIP()
XC_API BOOL WINAPI XC_LoadStyleZipMem(const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 从RC资源中的ZIP包中, 加载样式文件, RC资源类型必须为:"RT_RCDATA"
//@参数 id RC资源ID
//@参数 pFileName 文件名
//@参数 pPassword 密码
//@参数 hModule 模块句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 炫彩_加载样式文件从资源ZIP()
XC_API BOOL WINAPI XC_LoadStyleZipRes(int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@参数 pString 字符串
//@参数 pFileName 样式文件名, 用于打印错误文件和定位关联资源文件位置
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 炫彩_加载样式文件从字符串()
XC_API BOOL WINAPI XC_LoadStyleFromString(const char* pString, const wchar_t* pFileName);
//@参数 pString 字符串
//@参数 pFileName 样式文件名, 用于打印错误文件和定位关联资源文件位置
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 炫彩_加载样式文件从字符串UTF8()
XC_API BOOL WINAPI XC_LoadStyleFromStringUtf8(const char* pString, const wchar_t* pFileName);
//@参数 pFileName 样式文件名, 用于打印错误文件和定位关联资源文件位置
//@参数 pString 字符串
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 炫彩_加载样式文件从字符串W()
XC_API BOOL WINAPI XC_LoadStyleFromStringW(const wchar_t* pString, const wchar_t* pFileName);
//@备注 加载资源文件.  
//@参数 pFileName 资源文件名.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 炫彩_加载资源文件()
XC_API BOOL WINAPI XC_LoadResource(const wchar_t* pFileName);
//@备注 加载资源文件从zip压缩包中.  
//@参数 pZipFileName zip文件名.
//@参数 pFileName 资源文件名.
//@参数 pPassword zip压缩包密码.
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 炫彩_加载资源文件ZIP()
XC_API BOOL WINAPI XC_LoadResourceZip(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 加载资源文件从内存zip压缩包中
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 资源文件名
//@参数 pPassword zip压缩包密码
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 炫彩_加载资源文件内存ZIP()
XC_API BOOL WINAPI XC_LoadResourceZipMem(const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@参数 id RC资源ID
//@参数 pFileName 资源文件名
//@参数 pPassword zip压缩包密码
//@参数 hModule 模块句柄
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 炫彩_加载资源文件资源ZIP()
XC_API BOOL WINAPI XC_LoadResourceZipRes(int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@备注 加载资源文件从内存字符串.  
//@参数 pStringXML 字符串指针.
//@参数 pFileName 资源文件名,例如:'resource1'; 用于区分不同的资源文件,如果名称重复, 那么替换先前的资源文件.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 炫彩_加载资源文件从字符串()
XC_API BOOL WINAPI XC_LoadResourceFromString(const char* pStringXML, const wchar_t* pFileName);
//@备注 加载资源文件从内存字符串.  
//@参数 pStringXML 字符串指针.
//@参数 pFileName 资源文件名,例如:'resource1'; 用于区分不同的资源文件,如果名称重复, 那么替换先前的资源文件.
//@返回 成功返回TRUE否则返回FALSE.
//@别名 炫彩_加载资源文件从字符串UTF8()
XC_API BOOL WINAPI XC_LoadResourceFromStringUtf8(const char* pStringXML, const wchar_t* pFileName);
//@返回 返回 ID2D1Factory*
XC_API vint WINAPI XC_GetD2dFactory();
//@备注 开启D2D有效
//@返回 返回 IWICImagingFactory*
//@别名 炫彩_取WIC工厂()
XC_API vint WINAPI XC_GetWicFactory();
//@备注 开启D2D有效
//@返回 返回 IDWriteFactory*
//@别名 炫彩_取DWrite工厂()
XC_API vint WINAPI XC_GetDWriteFactory();
//@备注 炫彩扩展接口
//@参数 hXCGUI 炫彩对象句柄
//@参数 nType 类型
//@别名 炫彩_置类型()
XC_API void WINAPI _XC_SetType(HXCGUI hXCGUI, XC_OBJECT_TYPE nType);
//@备注 炫彩扩展开发接口
//@参数 hXCGUI 炫彩对象句柄
//@参数 nType 类型
//@别名 炫彩_添加类型()
XC_API void WINAPI _XC_AddType(HXCGUI hXCGUI, XC_OBJECT_TYPE nType);
//@备注 炫彩扩展开发接口
//@参数 hXCGUI 炫彩对象句柄
//@参数 data 绑定数据
//@别名 炫彩_绑定数据()
XC_API void WINAPI _XC_BindData(HXCGUI hXCGUI, vint data);
//@备注 炫彩扩展开发接口
//@参数 hXCGUI 炫彩对象句柄
//@返回 返回绑定数据
//@别名 炫彩_取绑定数据()
XC_API vint WINAPI _XC_GetBindData(HXCGUI hXCGUI);
//@备注 初始化界面库.  
//@参数 bD2D 是否启用D2D, 如果启用将使用D2D绘制图形,否则使用GDI+绘制图形
//@返回 成功返回TRUE否则返回FALSE.
//@别名 炫彩_初始化()
XC_API BOOL WINAPI XInitXCGUI(BOOL bD2D);
//@备注 运行消息循环,当炫彩窗口数量为0时退出.  
//@返回 无返回
//@别名 炫彩_运行()
XC_API void WINAPI XRunXCGUI();
//@备注 退出界面库释放资源.  
//不可在dllmain()中卸载, 否则会死锁
//@返回 无返回
//@别名 炫彩_退出()
XC_API void WINAPI XExitXCGUI();
//@备注 判断界面库是否已初始化   
//@返回 如果界面库已初始化返回TRUE, 否则返回FALSE
//@别名 炫彩_是否初始化()
XC_API BOOL WINAPI XC_IsInit();
//@备注 发送WM_QUIT消息退出消息循环,详情参见系统API PostQuitMessage().   
//@参数 nExitCode 退出码.
//@别名 炫彩_PostQuitMessage()
XC_API void WINAPI XC_PostQuitMessage(int nExitCode);
//@备注 全局窗口图标, 所有未设置图标的窗口,都将使用此默认图标; 此图标为炫彩窗口图标, 非系统图标
//@参数 hImage 图标句柄
//@别名 炫彩_置窗口图标()
XC_API void WINAPI XC_SetWindowIcon(HIMAGE hImage);
//@备注 外间距与对齐标识互相依赖 @ref bkObject_align_flag_
//当水平居中时left代表宽度;
//当垂直居中时top代表高度; 
//当对齐left时right代表宽度;
//当对齐right时left代表宽度;
//当对齐top时bottom代表宽度;
//当对齐bottom时top代表宽度;
//@参数 hObj 背景对象句柄
//@参数 left 左边间距
//@参数 top 顶部间距
//@参数 right 右边间距
//@参数 bottom 底部间距
//@别名 背景对象_置外间距()
XC_API void WINAPI XBkObj_SetMargin(vint hObj, int left, int top, int right, int bottom);
//@参数 hObj 背景对象句柄
//@参数 nFlags 对齐方式 @ref bkObject_align_flag_
//@别名 背景对象_置对齐()
XC_API void WINAPI XBkObj_SetAlign(vint hObj, int nFlags);
//@参数 hObj 背景对象句柄
//@参数 hImage 图片句柄
//@别名 背景对象_置图片()
XC_API void WINAPI XBkObj_SetImage(vint hObj, HIMAGE hImage);
//@参数 hObj 背景对象句柄
//@参数 angle 旋转角度
//@别名 背景对象_置旋转()
XC_API void WINAPI XBkObj_SetRotate(vint hObj, float angle);
//@参数 hObj 背景对象句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 背景对象_置填充颜色()
XC_API void WINAPI XBkObj_SetFillColor(vint hObj, COLORREF color);
//@参数 hObj 背景对象句柄
//@参数 width 宽度
//@别名 背景对象_置边框宽度()
XC_API void WINAPI XBkObj_SetBorderWidth(vint hObj, int width);
//@参数 hObj 背景对象句柄
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 背景对象_置边框颜色()
XC_API void WINAPI XBkObj_SetBorderColor(vint hObj, COLORREF color);
//@参数 hObj 背景对象句柄
//@参数 leftTop 左上角
//@参数 leftBottom 左下角
//@参数 rightTop 右上角
//@参数 rightBottom 右下角
//@别名 背景对象_置矩形圆角()
XC_API void WINAPI XBkObj_SetRectRoundAngle(vint hObj, int leftTop, int leftBottom, int rightTop, int rightBottom);
//@备注 启用绘制填充
//@参数 hObj 背景对象句柄
//@参数 bEnable 是否启用
//@别名 背景对象_启用填充()
XC_API void WINAPI XBkObj_EnableFill(vint hObj, BOOL bEnable);
//@备注 启用绘制边框
//@参数 hObj 背景对象句柄
//@参数 bEnable 是否启用
//@别名 背景对象_启用边框()
XC_API void WINAPI XBkObj_EnableBorder(vint hObj, BOOL bEnable);
//@参数 hObj 背景对象句柄
//@参数 pText 文本字符串
//@别名 背景对象_置文本()
XC_API void WINAPI XBkObj_SetText(vint hObj, const wchar_t* pText);
//@参数 hObj 背景对象句柄
//@参数 hFont 字体句柄
//@别名 背景对象_置字体()
XC_API void WINAPI XBkObj_SetFont(vint hObj, HFONTX hFont);
//@参数 hObj 背景对象句柄
//@参数 nAlign 文本对齐方式  @ref textFormatFlag_
//@别名 背景对象_置文本对齐()
XC_API void WINAPI XBkObj_SetTextAlign(vint hObj, int nAlign);
//@备注 当水平居中时left代表宽度;
//当垂直居中时top代表高度; 
//当对齐left时right代表宽度;
//当对齐right时left代表宽度;
//当对齐top时bottom代表宽度;
//当对齐bottom时top代表宽度;
//@参数 hObj 背景对象句柄
//@参数 pMargin 接收返回外间距
//@别名 背景对象_取外间距()
XC_API void WINAPI XBkObj_GetMargin(vint hObj, marginSize_* pMargin);
//@参数 hObj 背景对象句柄
//@返回 返回对齐标识 @ref bkObject_align_flag_
//@别名 背景对象_取对齐()
XC_API int WINAPI XBkObj_GetAlign(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 返回图片句柄
//@别名 背景对象_取图片()
XC_API HIMAGE WINAPI XBkObj_GetImage(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 返回旋转角度
//@别名 背景对象_取旋转角度()
XC_API int WINAPI XBkObj_GetRotate(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 返回填充色
//@别名 背景对象_取填充色()
XC_API COLORREF WINAPI XBkObj_GetFillColor(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 返回边框色
//@别名 背景对象_取边框色()
XC_API COLORREF WINAPI XBkObj_GetBorderColor(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 返回边框宽度
//@别名 背景对象_取边框宽度()
XC_API int WINAPI XBkObj_GetBorderWidth(vint hObj);
//@参数 hObj 背景对象句柄
//@参数 pRect 接收返回圆角大小
//@别名 背景对象_取矩形圆角()
XC_API void WINAPI XBkObj_GetRectRoundAngle(vint hObj, RECT* pRect);
//@参数 hObj 背景对象句柄
//@返回 填充返回TRUE,否则返回FALSE
//@别名 背景对象_是否填充()
XC_API BOOL WINAPI XBkObj_IsFill(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 绘制边框返回TRUE,否则返回FALSE
//@别名 背景对象_是否边框()
XC_API BOOL WINAPI XBkObj_IsBorder(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 返回文本
//@别名 背景对象_取文本()
XC_API const wchar_t* WINAPI XBkObj_GetText(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 返回字体
//@别名 背景对象_取字体()
XC_API HFONTX WINAPI XBkObj_GetFont(vint hObj);
//@参数 hObj 背景对象句柄
//@返回 返回文本对齐方式 @ref textFormatFlag_
//@别名 背景对象_取文本对齐()
XC_API int WINAPI XBkObj_GetTextAlign(vint hObj);
//@备注 线性
//@参数 pos 位置, 0.0f - 1.0f
//@返回 返回计算结果
//@别名 缓动_Linear()
XC_API float WINAPI XEase_Linear(float pos);
//@备注 二次方曲线
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Quad()
XC_API float WINAPI XEase_Quad(float pos, ease_type_ flag);
//@备注 三次方曲线  圆弧
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Cubic()
XC_API float WINAPI XEase_Cubic(float pos, ease_type_ flag);
//@备注 四方曲线
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Quart()
XC_API float WINAPI XEase_Quart(float pos, ease_type_ flag);
//@备注 五次方曲线
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Quint()
XC_API float WINAPI XEase_Quint(float pos, ease_type_ flag);
//@备注 正弦曲线, 在末端变化
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Sine()
XC_API float WINAPI XEase_Sine(float pos, ease_type_ flag);
//@备注 突击曲线, 突然一下
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Expo()
XC_API float WINAPI XEase_Expo(float pos, ease_type_ flag);
//@备注 圆环, 好比绕过一个圆环
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Circ()
XC_API float WINAPI XEase_Circ(float pos, ease_type_ flag);
//@备注 强力回弹
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Elastic()
XC_API float WINAPI XEase_Elastic(float pos, ease_type_ flag);
//@备注 回弹, 比较缓慢
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Back()
XC_API float WINAPI XEase_Back(float pos, ease_type_ flag);
//@备注 弹跳  模拟小球落地弹跳
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式
//@返回 返回计算结果
//@别名 缓动_Bounce()
XC_API float WINAPI XEase_Bounce(float pos, ease_type_ flag);
//@备注 全部缓动类型
//@参数 pos 位置, 0.0f - 1.0f
//@参数 flag 缓动方式 @ref ease_flag_
//@返回 返回计算结果
//@别名 缓动_扩展()
XC_API float WINAPI XEase_Ex(float pos, int flag);
//@参数 x 元素x坐标.
//@参数 y 元素y坐标.
//@参数 cx 宽度.
//@参数 cy 高度.
//@参数 hParent 父为窗口句柄或元素句柄.
//@返回 元素句柄.
//@别名 代码编辑框_创建()
XC_API HELE WINAPI XEditor_Create(int x, int y, int cx, int cy, HXCGUI hParent=NULL);
//@备注 判断指定行是否为空, 可能包含空格和TAB
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 代码编辑框_是否空行()
XC_API BOOL WINAPI XEditor_IsEmptyRow(HELE hEle, int iRow);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 代码编辑框_是否断点()
XC_API BOOL WINAPI XEditor_IsBreakpoint(HELE hEle, int iRow);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 bActivate 是否激活
//@返回 成功返回TRUE,否则返回FALSE
//@别名 代码编辑框_置断点()
XC_API BOOL WINAPI XEditor_SetBreakpoint(HELE hEle, int iRow, BOOL bActivate=TRUE);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 代码编辑框_移除断点()
XC_API BOOL WINAPI XEditor_RemoveBreakpoint(HELE hEle, int iRow);
//@参数 hEle 元素句柄
//@返回 成功返回TRUE,否则返回FALSE
//@别名 代码编辑框_清空断点()
XC_API void WINAPI XEditor_ClearBreakpoint(HELE hEle);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 代码编辑框_置当前运行()
XC_API BOOL WINAPI XEditor_SetRunRow(HELE hEle, int iRow);
//@参数 hEle 元素句柄
//@参数 pInfo 颜色信息结构体指针
//@别名 代码编辑框_取颜色信息()
XC_API void WINAPI XEditor_GetColor(HELE hEle, editor_color_* pInfo);
//@参数 hEle 元素句柄
//@参数 pInfo 颜色信息结构体指针
//@别名 代码编辑框_置颜色()
XC_API void WINAPI XEditor_SetColor(HELE hEle, editor_color_* pInfo);
//@参数 hEle 元素句柄
//@返回 返回断点数量
//@别名 代码编辑框_取断点数量()
XC_API int WINAPI XEditor_GetBreakpointCount(HELE hEle);
//@参数 hEle 元素句柄
//@参数 nDelay 延迟值毫秒
//@别名 代码编辑框_置提示信息延迟()
XC_API void WINAPI XEditor_SetTipsDelay(HELE hEle, int nDelay);
//@参数 hEle 元素句柄
//@参数 mode 0:回车选择, 1:空格选择, 3:tab键选择
//@别名 代码编辑框_置自动匹配选择模式()
XC_API void WINAPI XEditor_SetAutoMatchSelectMode(HELE hEle, int mode);
//@参数 hEle 元素句柄
//@参数 mode 0:中英文(上屏中文), 1:中英文(上屏英文), 2:中文(上屏中文), 3:英文(上屏英文)
//@别名 代码编辑框_置自动匹配结果显示模式()
XC_API void WINAPI XEditor_SetAutoMatchMode(HELE hEle, int mode);
//@参数 hEle 元素句柄
//@参数 aPoints 接收断点数组
//@参数 nCount 数组大小(数组成员数)
//@返回 返回实际获取断点数量
//@别名 代码编辑框_取全部断点()
XC_API int WINAPI XEditor_GetBreakpoints(HELE hEle, int* aPoints, int nCount);
//@备注 跳过收缩行
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@别名 代码编辑框_设置当前行()
XC_API void WINAPI XEditor_SetCurRow(HELE hEle, int iRow);
//@备注 获取代码对齐线缩进深度
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 代码编辑框_取深度()
XC_API int WINAPI XEditor_GetDepth(HELE hEle, int iRow);
//@备注 若是自由缩进模式, 获取实际TAB缩进, 否则返回代码对齐线缩进深度
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 成功返回TRUE,否则返回FALSE
//@别名 代码编辑框_取深度扩展()
XC_API int WINAPI XEditor_GetDepthEx(HELE hEle, int iRow);
//@备注 跳过收缩行
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@返回 返回展开行索引
//@别名 代码编辑框_转换到展开行()
XC_API int WINAPI XEditor_ToExpandRow(HELE hEle, int iRow);
//@备注 展开指定行, 让被折叠隐藏的行展开, 自身的展开状态保持不变, 例如: 行包含在折叠内容中, 将其展开
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@别名 代码编辑框_展开指定行扩展()
XC_API void WINAPI XEditor_ExpandEx(HELE hEle, int iRow);
//@参数 hEle 元素句柄
//@参数 bExpand 是否展开
//@别名 代码编辑框_展开全部()
XC_API void WINAPI XEditor_ExpandAll(HELE hEle, BOOL bExpand);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@参数 bExpand 是否展开
//@别名 代码编辑框_展开指定行()
XC_API void WINAPI XEditor_Expand(HELE hEle, int iRow, BOOL bExpand);
//@参数 hEle 元素句柄
//@参数 iRow 行索引
//@别名 代码编辑框_展开收缩指定行()
XC_API void WINAPI XEditor_ExpandSwitch(HELE hEle, int iRow);
//@备注 如果已存在,那么仅更新样式
//@参数 hEle 元素句柄
//@参数 pKey 字符串
//@参数 iStyle 样式
//@别名 代码编辑框_添加关键字()
XC_API void WINAPI XEditor_AddKeyword(HELE hEle, const wchar_t* pKey, int iStyle);
//@参数 hEle 元素句柄
//@参数 pKey 字符串
//@别名 代码编辑框_添加自动匹配字符串()
XC_API void WINAPI XEditor_AddConst(HELE hEle, const wchar_t* pKey);
//@参数 hEle 元素句柄
//@参数 pKey 字符串
//@别名 代码编辑框_添加自动匹配函数()
XC_API void WINAPI XEditor_AddFunction(HELE hEle, const wchar_t* pKey);
//@备注 排除定义变量的关键字, 用于排除定义变量, 因为定义变量禁用自动匹配;
//此关键字不加入自动匹配,仅用于排除定义变量
//@参数 hEle 元素句柄
//@参数 pKeyword 字符串
//@别名 代码编辑框_添加排除定义变量关键字()
XC_API void WINAPI XEditor_AddExcludeDefVarKeyword(HELE hEle, const wchar_t* pKeyword);
//@备注 列表项模板文件载入
//@参数 nType 模板类型,支持类型: \n
//@参数 pFileName 文件名.
//@返回 返回项模板句柄.
//@别名 项模板_加载从文件()
XC_API HTEMP WINAPI XTemp_Load(listItemTemp_type_ nType, const wchar_t* pFileName);
//@备注 加载列表项模板从zip压缩包中
//@参数 nType 模板类型,支持类型: \n
//@参数 pZipFile zip文件
//@参数 pFileName 文件名
//@参数 pPassword zip密码
//@返回 返回模板句柄.
//@别名 项模板_加载从ZIP()
XC_API HTEMP WINAPI XTemp_LoadZip(listItemTemp_type_ nType, const wchar_t* pZipFile, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 加载列表项模板从内存zip压缩包中
//@参数 nType 模板类型,支持类型, 只可选一个: \n
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 文件名
//@参数 pPassword zip密码
//@返回 返回项模板句柄.
//@别名 项模板_加载从内存ZIP()
XC_API HTEMP WINAPI XTemp_LoadZipMem(listItemTemp_type_ nType, const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 加载列表项模板从文件
//@参数 nType 模板类型, 支持类型, 只可选一个: \n 
//@参数 pFileName 文件名
//@参数 pOutTemp1 返回模板句柄1, 项模板
//@参数 pOutTemp2 返回模板句柄2, 列表头模板或列表视组模板
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 项模板_加载从文件扩展()
XC_API BOOL WINAPI XTemp_LoadEx(listItemTemp_type_ nType, const wchar_t* pFileName, HTEMP* pOutTemp1, HTEMP* pOutTemp2);
//@备注 加载列表项模板从zip压缩包中
//@参数 nType 模板类型, 支持类型: \n 
//@参数 pZipFile zip文件
//@参数 pFileName 文件名
//@参数 pPassword zip密码
//@参数 pOutTemp1 返回模板句柄1, 项模板
//@参数 pOutTemp2 返回模板句柄2, 列表头模板或列表视组模板
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 项模板_加载从ZIP扩展()
XC_API BOOL WINAPI XTemp_LoadZipEx(listItemTemp_type_ nType, const wchar_t* pZipFile, const wchar_t* pFileName, const wchar_t* pPassword, HTEMP* pOutTemp1, HTEMP* pOutTemp2);
//@备注 加载列表项模板从内存zip压缩包中
//@参数 nType 模板类型, 支持类型: \n 
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName 文件名
//@参数 pPassword zip密码
//@参数 pOutTemp1 返回模板句柄1, 项模板
//@参数 pOutTemp2 返回模板句柄2, 列表头模板或列表视组模板
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 项模板_加载从内存ZIP扩展()
XC_API BOOL WINAPI XTemp_LoadZipMemEx(listItemTemp_type_ nType, const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword, HTEMP* pOutTemp1, HTEMP* pOutTemp2);
//@备注 加载列表项模板文件从RC资源ZIP, RC资源类型必须为:"RT_RCDATA"
//@参数 nType 模板类型,支持类型: \n
//@参数 id RC资源ID
//@参数 pFileName 模板文件名
//@参数 pPassword ZIP密码
//@参数 hModule 模块句柄
//@返回 返回项模板句柄.
//@别名 项模板_加载从资源ZIP()
XC_API HTEMP WINAPI XTemp_LoadZipRes(listItemTemp_type_ nType, int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@备注 加载列表项模板从RC资源ZIP, RC资源类型必须为:"RT_RCDATA"
//@参数 nType 模板类型, 支持类型: \n 
//@参数 id RC资源ID
//@参数 pFileName 文件名
//@参数 pPassword zip密码
//@参数 pOutTemp1 返回模板句柄1, 项模板
//@参数 pOutTemp2 返回模板句柄2, 列表头模板或列表视组模板
//@参数 hModule 模块句柄
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 项模板_加载从资源ZIP扩展()
XC_API BOOL WINAPI XTemp_LoadZipResEx(listItemTemp_type_ nType, int id, const wchar_t* pFileName, const wchar_t* pPassword, HTEMP* pOutTemp1, HTEMP* pOutTemp2, HMODULE hModule=NULL);
//@备注 加载列表项模板文件从内存字符串
//@参数 nType 模板类型,支持类型: \n
//@参数 pStringXML 字符串指针.
//@返回 返回项模板句柄.
//@别名 项模板_加载从字符串()
XC_API HTEMP WINAPI XTemp_LoadFromString(listItemTemp_type_ nType, const char* pStringXML);
//@备注 加载列表项模板从字符串
//@参数 nType 模板类型, 支持类型: \n 
//@参数 pStringXML 字符串内容
//@参数 pOutTemp1 返回模板句柄1, 项模板
//@参数 pOutTemp2 返回模板句柄2, 列表头模板或列表视组模板
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 项模板_加载从字符串扩展()
XC_API BOOL WINAPI XTemp_LoadFromStringEx(listItemTemp_type_ nType, const char* pStringXML, HTEMP* pOutTemp1, HTEMP* pOutTemp2);
//@备注 加载列表项模板文件从内存
//@参数 nType 模板类型,支持类型: \n
//@参数 data 内存地址
//@参数 length 内存大小, 字节为单位
//@返回 返回项模板句柄.
//@别名 项模板_加载从内存()
XC_API HTEMP WINAPI XTemp_LoadFromMem(listItemTemp_type_ nType, const void* data, int length);
//@备注 加载列表项模板从内存
//@参数 nType 模板类型, 支持类型: \n 
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pOutTemp1 返回模板句柄1, 项模板
//@参数 pOutTemp2 返回模板句柄2, 列表头模板或列表视组模板
//@返回 如果成功返回TRUE,否则返回FALSE.
//@别名 项模板_加载从内存扩展()
XC_API BOOL WINAPI XTemp_LoadFromMemEx(listItemTemp_type_ nType, const void* data, int length, HTEMP* pOutTemp1, HTEMP* pOutTemp2);
//@备注 获取内置项模板,非用户设置的项模板
//@参数 nType 模板类型,支持类型: \n
//@参数 pFileName 文件名.
//@返回 返回项模板句柄.
//@别名 项模板_取内置项模板()
XC_API HTEMP WINAPI XTemp_Get(listItemTemp_type_ nType);
//@备注 获取列表项模板类型
//@参数 hTemp 列表项模板句柄.
//@返回 返回模板类型.
//@别名 项模板_取类型()
XC_API listItemTemp_type_ WINAPI XTemp_GetType(HTEMP hTemp);
//@备注 项模板销毁
//@参数 hTemp 项模板句柄.
//@别名 项模板_销毁()
XC_API BOOL WINAPI XTemp_Destroy(HTEMP hTemp);
//@备注 复制一份新的项模板
//@参数 hTemp 列表项模板句柄
//@返回 返回项模板句柄
//@别名 项模板_克隆()
XC_API HTEMP WINAPI XTemp_Clone(HTEMP hTemp);
//@备注 创建项模板
//@参数 nType 模板类型
//@返回 返回模板句柄.
//@别名 项模板_创建()
XC_API HTEMP WINAPI XTemp_Create(listItemTemp_type_ nType);
//@备注 添加根节点
//@参数 hTemp 项模板句柄.
//@参数 pNode 节点指针.
//@返回 成功返回TRUE,否则返回FALSE.
//@别名 项模板_添加根节点()
XC_API BOOL WINAPI XTemp_AddNodeRoot(HTEMP hTemp, void* pNode);
//@参数 hTemp 列表项模板句柄
//@参数 index 插入位置索引
//@参数 pNode 节点指针
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 项模板_列表_插入节点()
XC_API BOOL WINAPI XTemp_List_InsertNode(HTEMP hTemp, int index, void* pNode);
//@参数 hTemp 列表项模板句柄
//@参数 index 插入位置索引
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 项模板_列表_删除节点()
XC_API BOOL WINAPI XTemp_List_DeleteNode(HTEMP hTemp, int index);
//@备注 取子节点数量, 只当前层子节点
//@参数 hTemp 列表项模板句柄
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 项模板_列表_取数量()
XC_API int WINAPI XTemp_List_GetCount(HTEMP hTemp);
//@备注 添加子节点
//@参数 pParentNode 父节点指针.
//@参数 pNode 节点指针.
//@返回 成功返回TRUE,否则返回FALSE.
//@别名 项模板_添加子节点()
XC_API BOOL WINAPI XTemp_AddNode(void* pParentNode, void* pNode);
//@备注 创建节点
//@参数 nType 对象类型.
//@返回 成功返回节点指针,否则返回NULL.
//@别名 项模板_创建节点()
XC_API void* WINAPI XTemp_CreateNode(XC_OBJECT_TYPE nType);
//@备注 设置属性
//@参数 pNode 节点指针.
//@参数 pName 属性名.
//@参数 pAttr 属性值.
//@返回 成功返回TRUE,否则返回FALSE.
//@别名 项模板_置节点属性()
XC_API BOOL WINAPI XTemp_SetNodeAttribute(void* pNode, const wchar_t* pName, const wchar_t* pAttr);
//@备注 设置节点属性
//@参数 pNode 节点指针
//@参数 itemID 模板项ID
//@参数 pName 属性名
//@参数 pAttr 属性值
//@返回 成功返回TRUE,否则返回FALSE
//@别名 项模板_置节点属性扩展()
XC_API BOOL WINAPI XTemp_SetNodeAttributeEx(void* pNode, int itemID, const wchar_t* pName, const wchar_t* pAttr);
//@备注 获取列表中的节点
//@参数 hTemp 模板句柄
//@参数 index 节点位置索引
//@返回 返回返回节点指针
//@别名 项模板_取列表中的节点()
XC_API void* WINAPI XTemp_List_GetNode(HTEMP hTemp, int index);
//@备注 将指定列移动到目标位置
//@参数 hTemp 列表项模板句柄
//@参数 iColSrc 源列索引
//@参数 iColDest 目标列索引
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 项模板_列表_移动列()
XC_API BOOL WINAPI XTemp_List_MoveColumn(HTEMP hTemp, int iColSrc, int iColDest);
//@备注 获取节点, 根据itemID
//@参数 pNode 节点指针
//@参数 itemID ID.
//@返回 返回itemID对应的节点指针
//@别名 项模板_取节点()
XC_API void* WINAPI XTemp_GetNode(void* pNode, int itemID);
//@备注 克隆一个节点
//@参数 pNode 节点指针
//@返回 返回克隆的节点
//@别名 项模板_克隆节点()
XC_API void* WINAPI XTemp_CloneNode(void* pNode);
//@备注 此窗口是一个模态窗口
//@参数 pTitle 标题, 修改颜色通过 设置标题颜色
//@参数 pText 内容文本, 修改颜色通过 设置窗口颜色
//@参数 nFlags 标识 @ref messageBox_flag_
//@参数 hWndParent 父窗口句柄
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_
//@返回 messageBox_flag_ok:点击确定按钮退出, messageBox_flag_cancel:点击取消按钮退出, messageBox_flag_other:其他方式退出
//@别名 炫彩_消息框()
XC_API int WINAPI XC_MessageBox(const wchar_t* pTitle, const wchar_t* pText, int nFlags=messageBox_flag_ok | messageBox_flag_icon_info, HWND hWndParent=0, int XCStyle=window_style_modal);
//@备注 弹出窗口调用 @ref XModalWnd_DoModal() , 此窗口是一个模态窗口, 
//@参数 pTitle 标题, 修改颜色通过 设置标题颜色
//@参数 pText 内容文本, 修改颜色通过 设置窗口颜色
//@参数 nFlags 标识 @ref messageBox_flag_
//@参数 hWndParent 父窗口句柄
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_
//@返回 返回窗口句柄, 根据 @ref XModalWnd_DoModal() 返回值可判断用户点了哪个按钮退出
//@别名 炫彩消息框_创建()
XC_API HWINDOW WINAPI XMsg_Create(const wchar_t* pTitle, const wchar_t* pText, int nFlags=messageBox_flag_ok | messageBox_flag_icon_info, HWND hWndParent=0, int XCStyle=window_style_modal);
//@备注 此窗口是一个模态窗口
//@参数 dwExStyle 窗口扩展样式
//@参数 dwStyle 窗口样式
//@参数 lpClassName 窗口类名
//@参数 pTitle 标题, 修改颜色通过 设置标题颜色
//@参数 pText 内容文本, 修改颜色通过 设置窗口颜色
//@参数 nFlags 标识 @ref messageBox_flag_
//@参数 hWndParent 父窗口句柄
//@参数 XCStyle GUI库窗口样式,样式请参见宏定义 @ref window_style_
//@返回 返回窗口句柄,  然后调用 @ref XModalWnd_DoModal() 显示, 根据 @ref XModalWnd_DoModal() 返回值可判断用户点了哪个按钮退出
//@别名 炫彩消息框_创建扩展()
XC_API HWINDOW WINAPI XMsg_CreateEx(DWORD dwExStyle, DWORD dwStyle, const wchar_t* lpClassName, const wchar_t* pTitle, const wchar_t* pText, int nFlags=messageBox_flag_ok | messageBox_flag_icon_info, HWND hWndParent=0, int XCStyle=window_style_modal);
//@备注 启用后,将创建标题(形状文本)和关闭按钮
//@参数 hWindow 窗口句柄
//@参数 bEnable 
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 浮动窗口_启用标题栏内容()
XC_API BOOL WINAPI XFloatWnd_EnableCaptionContent(HWINDOW hWindow, BOOL bEnable);
//@备注 启用标题栏内容有效
//@参数 hWindow 窗口句柄
//@返回 返回形状文本句柄
//@别名 浮动窗口_取标题形状文本()
XC_API HXCGUI WINAPI XFloatWnd_GetCaptionShapeText(HWINDOW hWindow);
//@备注 启用标题栏内容有效
//@参数 hWindow 窗口句柄
//@返回 返回关闭按钮句柄
//@别名 浮动窗口_取标题关闭按钮()
XC_API HELE WINAPI XFloatWnd_GetCaptionButtonClose(HWINDOW hWindow);
//@备注 启用标题栏内容有效
//@参数 hWindow 窗口句柄
//@参数 pTitle 标题文本
//@别名 浮动窗口_置标题()
XC_API void WINAPI XFloatWnd_SetTitle(HWINDOW hWindow, const wchar_t* pTitle);
//@备注 启用标题栏内容有效
//@参数 hWindow 窗口句柄
//@返回 标题文本
//@别名 浮动窗口_取标题()
XC_API const wchar_t* WINAPI XFloatWnd_GetTitle(HWINDOW hWindow);
//@备注 启用延迟加载;图片文件,列表项模板文件.  
//@参数 bEnable 是否启用.
//@别名 资源_启用延迟加载()
XC_API void WINAPI XRes_EnableDelayLoad(BOOL bEnable);
//@备注 设置文件加载回调函数.  
//@参数 pFun 回调函数.
//@别名 资源_置文件加载回调()
XC_API void WINAPI XRes_SetLoadFileCallback(funLoadFile pFun);
//@备注 获取资源ID整型值.  
//@参数 pName 资源ID名称.
//@返回 返回整型值.
//@别名 资源_取ID值()
XC_API int WINAPI XRes_GetIDValue(const wchar_t* pName);
//@备注 查找资源图片.  
//@参数 pName 资源名称.
//@返回 返回图片句柄.
//@别名 资源_取图片()
XC_API HIMAGE WINAPI XRes_GetImage(const wchar_t* pName);
//@备注 从指定的资源文件中查找图片.  
//@参数 pFileName 资源文件名.
//@参数 pName 资源名称.
//@返回 返回图片句柄.
//@别名 资源_取图片扩展()
XC_API HIMAGE WINAPI XRes_GetImageEx(const wchar_t* pFileName, const wchar_t* pName);
//@备注 从资源中查找颜色.  
//@参数 pName 资源名称.
//@返回 返回颜色值.
//@别名 资源_取颜色()
XC_API COLORREF WINAPI XRes_GetColor(const wchar_t* pName);
//@备注 从资源中查找字体.  
//@参数 pName 资源名称.
//@返回 返回字体句柄.
//@别名 资源_取字体()
XC_API HFONTX WINAPI XRes_GetFont(const wchar_t* pName);
//@备注 从资源中查找背景  
//@参数 pName 资源名称.
//@返回 返回背景管理器句柄.
//@别名 资源_取背景管理器()
XC_API HBKM WINAPI XRes_GetBkM(const wchar_t* pName);
//@备注 创建矩形形状对象.  
//@参数 x1 坐标.
//@参数 y1 坐标.
//@参数 x2 坐标.
//@参数 y2 坐标.
//@参数 hParent 父对象句柄.
//@返回 返回句柄.
//@别名 形状线_创建()
XC_API HXCGUI WINAPI XShapeLine_Create(int x1, int y1, int x2, int y2, HXCGUI hParent=NULL);
//@备注 设置位置.  
//@参数 hShape 形状对象句柄.
//@参数 x1 坐标.
//@参数 y1 坐标.
//@参数 x2 坐标.
//@参数 y2 坐标.
//@别名 形状线_置位置()
XC_API void WINAPI XShapeLine_SetPosition(HXCGUI hShape, int x1, int y1, int x2, int y2);
//@备注 设置直线颜色.  
//@参数 hShape 形状对象句柄.
//@参数 color 颜色值, 请使用宏: RGBA()
//@别名 形状线_置颜色()
XC_API void WINAPI XShapeLine_SetColor(HXCGUI hShape, COLORREF color);
//@备注 UTF8文件
//@参数 pFileName 文件名
//@返回 SVG句柄
//@别名 SVG_加载从文件()
XC_API HSVG WINAPI XSvg_LoadFile(const wchar_t* pFileName);
//@备注 多字节字符串ANSI
//@参数 pString 字符串指针
//@返回 SVG句柄
//@别名 SVG_加载从字符串()
XC_API HSVG WINAPI XSvg_LoadString(const char* pString);
//@备注 UNICODE字符串
//@参数 pString 字符串指针
//@返回 SVG句柄
//@别名 SVG_加载从字符串W()
XC_API HSVG WINAPI XSvg_LoadStringW(const wchar_t* pString);
//@备注 UTF8字符串
//@参数 pString 字符串指针
//@返回 SVG句柄
//@别名 SVG_加载从字符串UTF8()
XC_API HSVG WINAPI XSvg_LoadStringUtf8(const char* pString);
//@参数 pZipFileName zip文件名
//@参数 pFileName svg文件名
//@参数 pPassword zip密码
//@返回 SVG句柄
//@别名 SVG_加载从ZIP()
XC_API HSVG WINAPI XSvg_LoadZip(const wchar_t* pZipFileName, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@参数 data 内存块指针
//@参数 length 内存块大小,字节为单位
//@参数 pFileName svg文件名
//@参数 pPassword zip密码
//@返回 SVG句柄
//@别名 SVG_加载从内存ZIP()
XC_API HSVG WINAPI XSvg_LoadZipMem(const void* data, int length, const wchar_t* pFileName, const wchar_t* pPassword=NULL);
//@备注 RC资源类型必须为:"RT_RCDATA"
//@参数 id RC资源ID
//@参数 pFileName svg文件名
//@参数 pPassword zip密码
//@参数 hModule 模块句柄
//@返回 SVG句柄
//@别名 SVG_加载从资源ZIP()
XC_API HSVG WINAPI XSvg_LoadZipRes(int id, const wchar_t* pFileName, const wchar_t* pPassword=NULL, HMODULE hModule=NULL);
//@参数 id 资源ID
//@参数 pType 资源类型. 在rc资源文件中,资源的类型, 例如:xcgui.rc, 用记事本打开可以看见资源类型; 例如:BITMAP, PNG; 参见MSDN
//@参数 hModule 从指定模块加载, 例如:DLL, EXE; 如果为空, 从当前EXE加载
//@返回 SVG句柄
//@别名 SVG_加载从资源()
XC_API HSVG WINAPI XSvg_LoadRes(int id, const wchar_t* pType, HMODULE hModule=NULL);

//@备注  启用像素对齐, 默认关闭, 如果SVG出现模糊, 可以启用像素对齐功能
//@参数  hSvg    SVG句柄
//@参数  bEnable 是否启用
//@别名  SVG_启用像素对齐()
XC_API void WINAPI XSvg_EnableAlignPixel(HSVG hSvg, BOOL bEnable);

//@备注  启用抗锯齿, 默认启用, 特殊情况为了让SVG更清晰, 可能需要关闭抗锯齿
//@参数  hSvg    SVG句柄
//@参数  bEnable  是否启用
//@别名  SVG_启用抗锯齿()
XC_API void WINAPI XSvg_EnableAntialias(HSVG hSvg, BOOL bEnable);

//@参数 hSvg SVG句柄
//@参数 nWidth 宽度, 如果输入0,那么还原初始宽度
//@参数 nHeight 高度, 如果输入0,那么还原初始高度
//@别名 SVG_置大小()
XC_API void WINAPI XSvg_SetSize(HSVG hSvg, int nWidth, int nHeight);
//@参数 hSvg SVG句柄
//@参数 pWidth 接收返回宽度
//@参数 pHeight 接收返回高度
//@别名 SVG_取大小()
XC_API void WINAPI XSvg_GetSize(HSVG hSvg, int* pWidth, int* pHeight);
//@参数 hSvg SVG句柄
//@返回 返回宽度
//@别名 SVG_取宽度()
XC_API int WINAPI XSvg_GetWidth(HSVG hSvg);
//@参数 hSvg SVG句柄
//@返回 返回高度
//@别名 SVG_取高度()
XC_API int WINAPI XSvg_GetHeight(HSVG hSvg);
//@参数 hSvg SVG句柄
//@参数 x x轴偏移
//@参数 y y轴偏移
//@别名 SVG_置偏移()
XC_API void WINAPI XSvg_SetPosition(HSVG hSvg, int x, int y);
//@参数 hSvg SVG句柄
//@参数 pX x轴偏移
//@参数 pY y轴偏移
//@别名 SVG_取偏移()
XC_API void WINAPI XSvg_GetPosition(HSVG hSvg, int* pX, int* pY);
//@参数 hSvg SVG句柄
//@参数 pViewBox 接收返回视图框
//@别名 SVG_取视图框()
XC_API void WINAPI XSvg_GetViewBox(HSVG hSvg, RECT* pViewBox);
//@参数 hSvg SVG句柄
//@参数 alpha 透明度
//@别名 SVG_置透明度()
XC_API void WINAPI XSvg_SetAlpha(HSVG hSvg, BYTE alpha);
//@参数 hSvg SVG句柄
//@返回 返回透明度
//@别名 SVG_取透明度()
XC_API BYTE WINAPI XSvg_GetAlpha(HSVG hSvg);
//@备注 用户颜色将覆盖默认样式
//@参数 hSvg SVG句柄
//@参数 color 颜色
//@参数 bEnable 是否有效
//@别名 SVG_置用户填充颜色()
XC_API void WINAPI XSvg_SetUserFillColor(HSVG hSvg, COLORREF color, BOOL bEnable);
//@备注 用户颜色将覆盖默认样式
//@参数 hSvg SVG句柄
//@参数 color 颜色
//@参数 strokeWidth 笔触宽度
//@参数 bEnable 是否有效
//@别名 SVG_置用户笔触颜色()
XC_API void WINAPI XSvg_SetUserStrokeColor(HSVG hSvg, COLORREF color, float strokeWidth, BOOL bEnable);
//@参数 hSvg SVG句柄
//@参数 pColor 返回颜色值
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 SVG_取用户填充颜色()
XC_API BOOL WINAPI XSvg_GetUserFillColor(HSVG hSvg, COLORREF* pColor);
//@参数 hSvg SVG句柄
//@参数 pColor 返回颜色值
//@参数 pStrokeWidth 
//@返回 如果成功返回TRUE,否则返回FALSE
//@别名 SVG_取用户笔触颜色()
XC_API BOOL WINAPI XSvg_GetUserStrokeColor(HSVG hSvg, COLORREF* pColor, float* pStrokeWidth);
//@备注 默认以自身中心点旋转, 当旋转角度为0时无效
//@参数 hSvg SVG句柄
//@参数 angle 转角度
//@别名 SVG_置旋转角度()
XC_API void WINAPI XSvg_SetRotateAngle(HSVG hSvg, float angle);
//@备注 默认以自身中心点旋转
//@参数 hSvg SVG句柄
//@返回 返回旋转角度
//@别名 SVG_取旋转角度()
XC_API float WINAPI XSvg_GetRotateAngle(HSVG hSvg);
//@备注 当旋转角度为0时无效
//@参数 hSvg SVG句柄
//@参数 angle 角度
//@参数 x 旋转中心点X偏移
//@参数 y 旋转中心点Y偏移
//@参数 bCenter TRUE:旋转中心点相对于自身中心偏移, FALSE:使用绝对坐标
//@别名 SVG_置旋转()
XC_API void WINAPI XSvg_SetRotate(HSVG hSvg, float angle, float x, float y, BOOL bCenter=TRUE);
//@参数 hSvg SVG句柄
//@参数 pAngle 返回 角度
//@参数 pX 返回 旋转中心点X偏移
//@参数 pY 返回 旋转中心点Y偏移
//@参数 pbOffset 返回 TRUE:旋转中心点相对于自身中心偏移, FALSE:使用绝对坐标
//@别名 SVG_取旋转()
XC_API void WINAPI XSvg_GetRotate(HSVG hSvg, float* pAngle, float* pX, float* pY, BOOL* pbCenter);
//@备注 显示或隐藏
//@参数 hSvg SVG句柄
//@参数 bShow 是否显示
//@别名 SVG_显示()
XC_API void WINAPI XSvg_Show(HSVG hSvg, BOOL bShow);
//@参数 hSvg SVG句柄
//@参数 bEnable 是否自动销毁
//@别名 SVG_启用自动销毁()
XC_API void WINAPI XSvg_EnableAutoDestroy(HSVG hSvg, BOOL bEnable);
//@参数 hSvg SVG句柄
//@别名 SVG_增加引用计数()
XC_API void WINAPI XSvg_AddRef(HSVG hSvg);
//@参数 hSvg SVG句柄
//@别名 SVG_释放引用计数()
XC_API void WINAPI XSvg_Release(HSVG hSvg);
//@参数 hSvg SVG句柄
//@返回 返回引用计数
//@别名 SVG_取引用计数()
XC_API int WINAPI XSvg_GetRefCount(HSVG hSvg);
//@备注 强制销毁
//@参数 hSvg SVG句柄
//@别名 SVG_销毁()
XC_API void WINAPI XSvg_Destroy(HSVG hSvg);
//@备注 使用基础元素作为面板,弹出一个通知消息,返回面板句柄, 通过此句柄可对其操作
//@参数 hWindow 窗口句柄
//@参数 position 位置; @ref position_flag_top : 顶部, @ref position_flag_right : 右侧,
//@参数 pTitle 标题
//@参数 pText 内容
//@参数 hIcon 图标
//@参数 skin 外观类型
//@返回 返回元素句柄
//@别名 通知消息_窗口中弹出()
XC_API HELE WINAPI XNotifyMsg_WindowPopup(HWINDOW hWindow, position_flag_ position, const wchar_t* pTitle, const wchar_t* pText, HIMAGE hIcon=NULL, notifyMsg_skin_ skin=notifyMsg_skin_no);
//@备注 使用基础元素作为面板,弹出一个通知消息,返回面板句柄, 通过此句柄可对其操作; 可指定自定义宽度和高度
//@参数 hWindow 窗口句柄
//@参数 position 位置;  position_flag_top:顶部, position_flag_right:右侧,
//@参数 pTitle 标题
//@参数 pText 内容
//@参数 hIcon 图标
//@参数 skin 外观类型
//@参数 bBtnClose 是否启用关闭按钮
//@参数 bAutoClose 是否自动关闭
//@参数 nWidth 自定义宽度, -1(使用默认值)
//@参数 nHeight 自定义高度, -1(使用默认值)
//@返回 返回元素句柄
//@别名 通知消息_窗口中弹出扩展()
XC_API HELE WINAPI XNotifyMsg_WindowPopupEx(HWINDOW hWindow, position_flag_ position, const wchar_t* pTitle, const wchar_t* pText, HIMAGE hIcon=NULL, notifyMsg_skin_ skin=notifyMsg_skin_no, BOOL bBtnClose=TRUE, BOOL bAutoClose=TRUE, int nWidth=-1, int nHeight=-1);
//@备注 未实现,预留接口
//@参数 position 
//@参数 pTitle 
//@参数 pText 
//@参数 hIcon 
//@参数 skin 
//@返回 返回窗口句柄
//@别名 通知消息_弹出()
XC_API HWINDOW WINAPI XNotifyMsg_Popup(position_flag_ position, const wchar_t* pTitle, const wchar_t* pText, HIMAGE hIcon, notifyMsg_skin_ skin=notifyMsg_skin_no);
//@备注 未实现,预留接口
//@参数 position 
//@参数 pTitle 
//@参数 pText 
//@参数 hIcon 
//@参数 skin 
//@参数 bBtnClose 
//@参数 bAutoClose 
//@参数 nWidth 
//@参数 nHeight 
//@返回 返回窗口句柄
//@别名 通知消息_弹出扩展()
XC_API HWINDOW WINAPI XNotifyMsg_PopupEx(position_flag_ position, const wchar_t* pTitle, const wchar_t* pText, HIMAGE hIcon=NULL, notifyMsg_skin_ skin=notifyMsg_skin_no, BOOL bBtnClose=TRUE, BOOL bAutoClose=TRUE, int nWidth=-1, int nHeight=-1);
//@参数 hWindow 通知消息所属窗口句柄, 如果未指定那么认为是桌面通知消息
//@参数 duration 持续时间
//@别名 通知消息_置持续时间()
XC_API void WINAPI XNotifyMsg_SetDuration(HWINDOW hWindow, UINT duration);
//@参数 hWindow 通知消息所属窗口句柄, 如果未指定那么认为是桌面通知消息
//@参数 nHeight 高度
//@别名 通知消息_置标题高度()
XC_API void WINAPI XNotifyMsg_SetCaptionHeight(HWINDOW hWindow, int nHeight);
//@备注 设置默认宽度
//@参数 hWindow 通知消息所属窗口句柄, 如果未指定那么认为是桌面通知消息
//@参数 nWidth 宽度
//@别名 通知消息_置宽度()
XC_API void WINAPI XNotifyMsg_SetWidth(HWINDOW hWindow, int nWidth);
//@参数 hWindow 通知消息所属窗口句柄, 如果未指定那么认为是桌面通知消息
//@参数 nSpace 间隔大小
//@别名 通知消息_置间隔()
XC_API void WINAPI XNotifyMsg_SetSpace(HWINDOW hWindow, int nSpace);
//@备注 设置通知消息面板边大小
//@参数 hWindow 通知消息所属窗口句柄, 如果未指定那么认为是桌面通知消息
//@参数 left 坐标
//@参数 top 顶边
//@参数 right 右边
//@参数 bottom 底边
//@别名 通知消息_置边大小()
XC_API void WINAPI XNotifyMsg_SetBorderSize(HWINDOW hWindow, int left, int top, int right, int bottom);
//@备注 设置通知消息与父对象的四边间隔
//@参数 hWindow 通知消息所属窗口句柄, 如果未指定那么认为是桌面通知消息
//@参数 left 左侧间隔, 未实现预留功能
//@参数 top 顶部间隔
//@参数 right 右侧间隔
//@参数 bottom 底部间隔, 未实现预留功能
//@别名 通知消息_置父边距()
XC_API void WINAPI XNotifyMsg_SetParentMargin(HWINDOW hWindow, int left, int top, int right, int bottom);
//@分组}
#endif
