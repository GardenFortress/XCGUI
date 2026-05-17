/**************************************************************************\
*
* Copyright (c)  炫彩界面库. All Rights Reserved.
*
* 炫彩界面库 - 官方网站 : http://www.xcgui.com
*
\**************************************************************************/
#pragma once


//@分组{  窗口事件
/*@声明

//@备注  窗口-绘制事件，当窗口发生重绘(调整大小、显示、调整布局、改变位置)时，触发该事件；可在本事下拦截默认窗口绘制(炫彩内部默认绘制)，进行自定义绘制。
//事件响应函数格式：
//	整型 WINAPI 窗口_绘制(窗口句柄 窗口, 绘图句柄 绘图, 逻辑型 *是否拦截)
//	int WINAPI OnWndDrawWindow(HWINDOW hWindow, HDRAW hDraw,BOOL *pbHandled) 
//参数<1> 窗口：窗口句柄
//参数<2> 绘图：绘图句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_绘制
#define WM_PAINT

//@备注  窗口-关闭事件，当窗口即将关闭时(还未关闭)，触发该事件；可在本事下拦截本次事件 *是否拦截 = TRUE则会取消本次关闭事件。
//事件响应函数格式：
//	整型 WINAPI 窗口_即将关闭(窗口句柄 窗口, 逻辑型 *是否拦截)
//	int WINAPI OnWndClose(HWINDOW hWindow, BOOL *pbHandled) 
//参数<1> 窗口：窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_即将关闭
#define WM_CLOSE

//@备注  窗口-销毁事件，当窗口即将销毁时(还未销毁)，触发该事件；可在本事下拦截本次事件 *是否拦截 = TRUE则会取消本次关闭事件。
//事件响应函数格式：
//	整型 WINAPI 窗口_即将销毁(窗口句柄 窗口, 逻辑型 *是否拦截)
//	int WINAPI OnWndDestroy(HWINDOW hWindow, BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_即将销毁
#define WM_DESTROY

//@备注  窗口-销毁非客户区事件，当窗口非客户区(窗口边框、阴影.)销毁时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_非客户区销毁(窗口句柄 窗口, 逻辑型 *是否拦截)//
//	int WINAPI OnWndNCDestroy(HWINDOW hWindow,BOOL *pbHandled) 
//参数<1> 窗口：窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_非客户区销毁
#define WM_NCDESTROY

//@备注  窗口-鼠标左键按下事件，当鼠标左键在窗口空白区域(不包含其他元素的区域，或包含区域的元素是禁用、鼠标穿透时)按下时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标左键按下(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndLButtonDown(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 标识：请参见MSDN WM_LBUTTONDOWN.
//参数<3> 鼠标位置：鼠标左键在窗口上按下的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标左键按下
#define WM_LBUTTONDOWN

//@备注  窗口-鼠标左键弹起事件，当鼠标左键在窗口放开时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标左键弹起(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndLButtonUp(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 标识：请参见MSDN WM_LBUTTONUP.
//参数<3> 鼠标位置：鼠标左键在窗口上按下的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标左键弹起
#define WM_LBUTTONUP

//@备注  窗口-鼠标右键按下事件，当鼠标右键在窗口空白区域(不包含其他元素的区域，或包含区域的元素是禁用、鼠标穿透时)按下时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标右键按下(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndRButtonDown(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 标识：请参见MSDN WM_RBUTTONDOWN.
//参数<3> 鼠标位置：鼠标右键在窗口上按下的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标右键按下
#define WM_RBUTTONDOWN

//@备注  窗口-鼠标右键弹起事件，当鼠标右键在窗口放开时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标右键弹起(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndRButtonUp(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 标识：请参见MSDN WM_RBUTTONUP.
//参数<2> 鼠标位置：鼠标右键在窗口上弹起的点(点->x，点->y)
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标右键弹起
#define WM_RBUTTONUP

//@备注  窗口-鼠标左键双击事件，当鼠标左键在窗口空白区域(不包含其他元素的区域，或包含区域的元素是禁用、鼠标穿透时)双击时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标左键双击(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndLButtonDBClick(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled) 
//参数<1> 窗口：窗口句柄
//参数<2> 标识：请参见MSDN WM_LBUTTONDBLCLK.
//参数<3> 鼠标位置：鼠标左键在窗口上双击的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标左键双击
#define WM_LBUTTONDBLCLK

//@备注  窗口-鼠标右键双击事件，当鼠标右键在窗口空白区域(不包含其他元素的区域，或包含区域的元素是禁用、鼠标穿透时)双击时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标右键双击(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndRButtonDBClick(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 标识：请参见MSDN WM_RBUTTONDBLCLK.
//参数<3> 鼠标位置：鼠标在窗口上右键双击的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标右键双击
#define WM_RBUTTONDBLCLK

//@备注  窗口-鼠标移动事件，当鼠标在窗口中移动时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标移动(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndMouseMove(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 标识：请参见MSDN WM_MOUSEMOVE wParam参数.
//参数<3> 鼠标位置：鼠标在窗口上移动的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标移动
#define WM_MOUSEMOVE

//@备注  窗口-鼠标悬停事件，当鼠标在窗口上停留(鼠标在窗口中，但未移动鼠标)时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标悬停(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndMouseHover(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 标识：请参见MSDN WM_MOUSEMOVE wParam参数.
//参数<3> 鼠标位置：鼠标在窗口上停留的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标悬停
#define WM_MOUSEHOVER

//@备注  窗口-鼠标离开事件，当鼠标进入窗口后又离开窗口时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标离开(窗口句柄 窗口, 逻辑型 *是否拦截)
//	int WINAPI OnWndMouseLeave(HWINDOW hWindow, BOOL *pbHandled) 
//参数<1> 窗口：窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标离开
#define  WM_MOUSELEAVE


//@备注  窗口-鼠标滚轮滚动事件，当鼠标在窗口中滚动鼠标滚轮时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标滚动(窗口句柄 窗口, 正整数 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnWndMouseWheel(HWINDOW hWindow, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 标识：请参见MSDN WM_MOUSEWHEEL消息wParam参数
//参数<3> 鼠标位置：鼠标滚动时滚动的点的距离(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标滚动
#define WM_MOUSEWHEEL

//@备注  窗口-鼠标捕获改变事件，当窗口捕获到鼠标时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_鼠标捕获改变(HWND hWnd, 逻辑型 *是否拦截)
//	int WINAPI OnWndCaptureChanged(HWINDOW hWindow, HWND hWnd,BOOL *pbHandled) 
//参数<1> 窗口：窗口句柄
//参数<2> hWnd：系统-窗口句柄,通过XWnd_GetHWND()获取
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_鼠标捕获改变
#define WM_CAPTURECHANGED

//@备注  窗口-键盘按下事件，当窗口激活(获得焦点)并且键盘按下键盘时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_键按下(窗口句柄 窗口, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnWndKeyDown(HWINDOW hWindow, WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 参数1：请参见MSDN WM_KEYDOWN. wParam
//参数<3> 参数2：请参见MSDN WM_KEYDOWN. lParam
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_键按下
#define WM_KEYDOWN

//@备注  窗口-键盘弹起事件，当窗口键盘放开时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_键弹起(窗口句柄 窗口, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnWndKeyUp(HWINDOW hWindow, WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 参数1：请参见MSDN WM_KEYUP. wParam
//参数<3> 参数2：请参见MSDN WM_KEYUP. lParam
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_键弹起
#define WM_KEYUP

//@备注  窗口-字符输入事件，当窗口激活(获得焦点)时，点击或按住键盘按键时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_字符输入(窗口句柄 窗口, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnWndChar(HWINDOW hWindow, WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 参数1：请参见MSDN WM_CHAR. wParam
//参数<3> 参数2：请参见MSDN WM_CHAR. lParam
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_字符输入
#define WM_CHAR

//@备注  窗口-尺寸改变事件，当窗口尺寸发生改变(最小化、最大化、还原大小、调整窗口大小、调整窗口可见性)时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_尺寸改变(窗口句柄 窗口, 正整型 标识, 大小结构 *大小, 逻辑型 *是否拦截)
//	int WINAPI OnWndSize(HWINDOW hWindow, UINT nFlags,SIZE *pSize,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 标识：请参见MSDN WWM_SIZE. wParam
//参数<3> 大小：改变后的大小尺寸结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_大小改变
#define  WM_SIZE

//@备注  窗口-退出改变大小或移动事件，当窗口退出移动或调整大小模式循环后，向窗口发送一次。当用户单击窗口的标题栏或大小调整边框时时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_退出改变大小或移动(窗口句柄 窗口, 逻辑型 *是否拦截)
//	int WINAPI OnWndExitSizeMove(HWINDOW hWindow, BOOL *pbHandled) 
//参数<1> 窗口： 窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_退出改变大小或移动
#define WM_EXITSIZEMOVE

//@备注  窗口-定时器事件(系统)，当窗口注册定时器事件，并启用(通过 窗口_置定时器()启用,通过 窗口_关闭定时器() 关闭)定时器(时钟)后，定时器间隔多久后触发一次本事件
//事件响应函数格式：
//	整型 WINAPI 窗口_定时器(窗口句柄 窗口, 正整型 事件ID, 逻辑型 *是否拦截)
//	int WINAPI OnWndTimer(HWINDOW hWindow, UINT nIDEvent,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 事件ID：定义定时器时，填写的ID
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_定时器
#define WM_TIMER

//@备注  窗口-获得焦点事件，当窗口获得焦点(通过鼠标点击、任务视图选择、WinAPI激活)时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_获得焦点(窗口句柄 窗口, 逻辑型 *是否拦截)
//	int WINAPI OnWndSetFocus(HWINDOW hWindow,BOOL *pbHandled) 
//参数<1> 窗口： 窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_获得焦点
#define WM_SETFOCUS

//@备注  窗口-失去焦点事件，当窗口失去焦点时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_失去焦点(窗口句柄 窗口, 逻辑型 *是否拦截)
//	int WINAPI OnWndKillFocus(HWINDOW hWindow, BOOL *pbHandled)  
//参数<1> 窗口： 窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_失去焦点
#define WM_KILLFOCUS

//@备注  窗口-设置鼠标光标事件，当鼠标光标在窗口上移动、鼠标左键\中键\右键\小键\点击时，触发该事件；该消息可以改光标(使用 LoadCursorW\LoadCursorA 函数)
//事件响应函数格式：
//	整型 WINAPI 窗口_置鼠标光标(窗口句柄 窗口, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnWndSetCursor(HWINDOW hWindow,WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 参数1：请参见MSDN WM_SETCURSOR. wParam
//参数<3> 参数2：请参见MSDN WM_SETCURSOR. lParam
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_置光标
#define WM_SETCURSOR

//@备注 窗口-文件拖入事件，需先启用:XWnd_EnableDragFiles()事件；当鼠标拖入文件到窗口时，触发该事件；本事件参数<hDropInfo>表示拖入的文件信息，该信息包含了，详情请参考:https://www.cnblogs.com/chechen/p/4062149.html.注意该地址内的wParam和本事件下的hDropInfo相同。
//事件响应函数格式：
//	整型 WINAPI 窗口_文件拖入(窗口句柄 窗口, HDROP hDropInfo, 逻辑型 *是否拦截)
//	int WINAPI OnDropFiles(HWINDOW hWindow, HDROP hDropInfo , BOOL *pbHandled) 
//参数<1> 窗口： 窗口句柄
//参数<2> hDropInfo：拖入文件信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_文件拖入
#define WM_DROPFILES

//@备注  窗口-其他事件，当窗口触发其他事件(炫彩未定义的 'WM_' 开头的其他事件，一级你自定义的Windows消息事件)时，触发该事件；
//事件响应函数格式：
//	整型 WINAPI 窗口_其他(窗口句柄 窗口, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnWndOther(HWINDOW hWindow, WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 参数1：请参见MSDN wParam
//参数<3> 参数2：请参见MSDN lParam
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_其他事件模板
#define WM_OTHER


//窗口消息-包含系统非客户区消息
//#define  XWM_EVENT_ALL        WM_APP+1000 //事件投递 -------不公开-------不需要注册

//wParam:left-top坐标组合; lParam:right-bottom坐标组合; 如果这2个参数为空,那么重绘整个窗口
//#define  XWM_REDRAW           WM_APP+1007  //窗口重绘延时 ----不公开-----内部自定义消息

//重绘元素,内部使用
//#define  XWM_REDRAW_ELE       0x7000+1 //重绘元素 wParam:元素句柄, lParam:RECT*基于窗口坐标

//@备注  窗口-消息过程事件，当窗口触发任意事件时，触发该事件；
//事件响应函数格式：
//	整型 WINAPI 窗口_过程(窗口句柄 窗口, 正整型 消息, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnWndProc(HWINDOW hWindow, UINT message, WPARAM wParam, LPARAM lParam, BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 消息：请参见MSDN,参考资料:https://www.cnblogs.com/carekee/articles/2409833.html
//参数<3> 参数1：请参见MSDN wParam
//参数<4> 参数2：请参见MSDN lParam
//参数<5> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 窗口事件_事件过滤
#define  XWM_WINDPROC         0x7000+2

//#define  XWM_DRAW_T           0x7000+3    //窗口绘制,内部使用, wParam:0, lParam:0

//#define  XWM_TIMER_T          0x7000+4    //内部使用, wParam:hXCGUI, lParam:ID

//@备注  窗口-炫彩定时器(非系统定时器)，需注册 'XWM_XC_TIMER' 窗口事件以接收该事件触发，并启用(通过 窗口_置炫彩定时器()启用,通过 窗口_关闭炫彩定时器() 关闭)定时器(时钟)了后，定时器间隔多久后触发一次本事件 (wParam:定时器ID, lParam:0)
//事件响应函数格式：
//	整型 WINAPI 窗口_炫彩定时器(窗口句柄 窗口, 正整型 定时器ID, 逻辑型 *是否拦截)
//	int WINAPI OnWndXCTimer(HWINDOW hWindow,UINT nTimerID,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 窗口ID：窗口句柄
//参数<3> 定时器ID：定义定时器时，填写的ID
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名   窗口事件_炫彩定时器
#define  XWM_XC_TIMER         0x7000+5    

//#define  XWM_CLOUDUI_DOWNLOADFILE_COMPLETE   0x7000+6  //内部使用

//#define  XWM_CLOUNDUI_OPENURL_WAIT    0x7000+7 //内部使用

//#define  XWM_CALL_UI_THREAD   0x7000+8     //内部使用



//@备注  窗口-置焦点事件，当使用 窗口_置焦点() 功能时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_获得焦点(窗口句柄 窗口, 元素句柄 元素, 逻辑型 *是否拦截)
//	int WINAPI OnWndSetFocusEle(HWINDOW hWindow, HELE hEle,BOOL *pbHandled)
//参数<1> 窗口： 窗口句柄
//参数<2> 元素：置焦点后，获得焦点的元素的句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_置焦点元素
#define  XWM_SETFOCUS_ELE         0x7000+9

//@备注  窗口-托盘图标事件，当鼠标在托盘图标操作本窗口托盘图标时，触发该事件；使用 托盘图标_添加() 设置托盘图标(更多功能以 '托盘图标_' 开头)
//事件响应函数格式：
//	整型 WINAPI 窗口_托盘图标(窗口句柄 窗口, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnWndTrayIcon(HWINDOW hWindow, WPARAM wParam, LPARAM lParam, BOOL *pbHandled)
//参数<1> 窗口:  窗口句柄
//参数<2> 参数1：托盘事件ID,在调用 '托盘图标_添加()'时，填写的第二个参数
//参数<3> 参数2：鼠标操作类型(左键\中键\右键\按下\弹起等.)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_托盘图标
#define  XWM_TRAYICON         0x7000+10


//@备注 窗口-菜单弹出事件，当窗口菜单弹出时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_菜单弹出(窗口句柄 窗口, 菜单句柄 菜单, 逻辑型 *是否拦截)
//	int WINAPI OnWndMenuPopup(HWINDOW hWindow, HMENUX hMenu, BOOL *pbHandled)
//参数<1> 窗口: 窗口句柄
//参数<2> 菜单：菜单句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_菜单弹出
#define  XWM_MENU_POPUP       0x7000+11

//@备注 窗口-菜单弹出窗口事件，当菜单弹出后显示一个窗口，可以在本事件下设置窗口的样式(位置、坐标、窗口类型、阴影等.)
//事件响应函数格式：
//	整型 WINAPI 窗口_菜单弹出窗口(窗口句柄 窗口, 菜单句柄 菜单, 菜单弹出窗口结构* 信息, 逻辑型 *是否拦截)
//	int WINAPI OnWndMenuPopupWnd(HWINDOW hWindow, HMENUX hMenu,menu_popupWnd_ *pInfo,BOOL *pbHandled)
//参数<1> 窗口: 窗口句柄
//参数<2> 菜单：菜单句柄
//参数<3> 信息：菜单弹出窗口结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_菜单弹出窗口
#define  XWM_MENU_POPUP_WND     0x7000+12

//@备注 窗口-菜单项选择事件，当菜单弹出后，鼠标点选某一项菜单项时，触发该事件(菜单选择 wParam:菜单项ID, lParam:0)
//事件响应函数格式：
//	整型 WINAPI 窗口_菜单项选择(窗口句柄 窗口, 整型 菜单项id, 逻辑型 *是否拦截)
//  int CALLBACK OnWndMenuSelect(HWINDOW hWindow, int nID,BOOL *pbHandled)
//参数<1> 窗口: 窗口句柄
//参数<2> 菜单项id：菜单项id
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_菜单项选择
#define  XWM_MENU_SELECT       0x7000+13

//@备注 窗口-菜单退出事件，当菜单关闭后，触发该事件(窗口句柄 窗口, 菜单退出 wParam:0, lParam:0)
//事件响应函数格式：
//	整型 WINAPI 窗口_菜单退出(窗口句柄 窗口, 逻辑型 *是否拦截)
//	int WINAPI OnWndMenuSelect(HWINDOW hWindow, BOOL *pbHandled)
//参数<1> 窗口: 窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 窗口事件_菜单退出
#define  XWM_MENU_EXIT         0x7000+14

//@备注 窗口-绘制菜单背景, 启用该功能需要调用XMenu_EnableDrawBackground()
//事件响应函数格式：
//	整型 WINAPI 窗口_菜单背景绘制(窗口句柄 窗口, 绘图句柄 绘图, 菜单项绘制结构 *绘制信息, 逻辑型 *是否拦截)
//	int CALLBACK OnWndMenuDrawBackground(HWINDOW hWindow, HDRAW hDraw,menu_drawBackground_ *pInfo,BOOL *pbHandled)
//参数<1> 窗口: 窗口句柄
//参数<2> 绘图: 绘图句柄
//参数<3> 绘制信息：菜单项绘制结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 窗口事件_菜单背景绘制
#define  XWM_MENU_DRAW_BACKGROUND   0x7000+15

//@备注 窗口-绘制菜单项事件，启用该功能需要调用XMenu_EnableDrawItem()
//事件响应函数格式：
//	整型 WINAPI 窗口_菜单项绘制(窗口句柄 窗口, 绘图句柄 绘图, 菜单项绘制结构* 信息, 逻辑型 *是否拦截)
//	int WINAPI OnMenuDrawItem(HWINDOW hWindow, HDRAW hDraw,menu_drawItem_* pInfo,BOOL *pbHandled)
//参数<1> 窗口: 窗口句柄
//参数<2> 绘图句柄：绘图句柄
//参数<3> 信息：菜单项绘制结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 窗口事件_菜单项绘制
#define  XWM_MENU_DRAWITEM             0x7000+16

//#define  XWM_COMBOBOX_POPUP_DROPLIST   0x7000+17  //弹出下拉组框列表,内部使用

//@备注 窗口-浮动窗格事件，当窗格从框架窗口中弹出时，变成浮动窗格(未固定到特定区域)后，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_菜单项绘制(窗口句柄 窗口, 窗口句柄 浮动窗口, 元素句柄 窗格句柄, 逻辑型 *是否拦截)
//	int WINAPI OnWndFloatPane(HWINDOW hWindow, HWINDOW hFloatWnd, HELE hPane, BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 浮动窗口：浮动窗口句柄
//参数<3> 窗格句柄：浮动的窗格句柄
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 窗口事件_浮动窗格
#define  XWM_FLOAT_PANE               0x7000+18

//@备注 窗口-绘制完成事件，当窗口完成自身绘制时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_绘制完成(窗口句柄 窗口, 绘图句柄 绘图, 逻辑型 *是否拦截)
//	int WINAPI OnWndDrawWindowEnd(HWINDOW hWindow, HDRAW hDraw,BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 绘图：绘图句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 窗口事件_绘制完成
#define  XWM_PAINT_END               0x7000+19

//@备注 窗口-绘制完成并显示事件，当窗口完成自身绘制后，并且窗口已经显示到屏幕时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_绘制完成显示(窗口句柄 窗口, 逻辑型 *是否拦截)
//	int WINAPI OnWndDrawWindowDisplay(HWINDOW hWindow, BOOL *pbHandled)
//参数<1> 窗口：窗口句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 窗口事件_绘制完成显示
#define  XWM_PAINT_DISPLAY           0x7000+20

//@备注 框架窗口-码头弹出窗格事件，当点击码头(拖动窗格时显示的可停靠区域)上的按钮时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_框架窗口码头弹出窗格(窗口句柄 框架窗口, 窗口句柄 弹出的窗格句柄, 元素句柄 窗格句柄, 逻辑型 *是否拦截)
//	int WINAPI OnWndDocPopup(HWINDOW hFrameWindow, HWINDOW hWindowDock,  HELE hPane, BOOL *pbHandled) 
//参数<1> 框架窗口：框架窗口句柄
//参数<2> 弹出的窗格句柄：弹出的窗格窗口句柄
//参数<3> 窗格句柄：浮动的窗格的元素句柄
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_框架窗口码头弹出窗格
#define  XWM_DOCK_POPUP              0x7000+21    

//@备注 框架窗口-浮动窗口拖动事件，当拖动浮动窗口移动时，显示的停靠提示，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_浮动窗口拖动(窗口句柄 框架窗口, 窗口句柄 拖动的浮动窗口, 窗口句柄* 窗格停靠窗口句柄数组, 逻辑型 *是否拦截)
//	int WINAPI OnWndFloatWndDrag(HWINDOW hFrameWindow, HWINDOW hFloatWnd, HWINDOW* hArray, BOOL *pbHandled) 
//参数<1> 框架窗口：框架窗口句柄
//参数<2> 拖动的浮动窗口：拖动的浮动窗口句柄
//参数<3> 窗格停靠窗口句柄数组：HWINDOW array[6],窗格停靠提示窗口句柄数组,有6个成员,分别为:[0]中间十字, [1]左侧,[2]顶部,[3]右侧,[4]底部, [5]停靠位置预览 
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_浮动窗口拖动
#define  XWM_FLOATWND_DRAG           0x7000+22

//@备注 框架窗口-窗格显示隐藏事件，当窗格显示或隐藏时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_窗格显示(窗口句柄  框架窗口, 元素句柄 窗格句柄, BOOL 是否显示, 逻辑型 *是否拦截)
//	int WINAPI OnWndPaneShow(HWINDOW hFrameWindow, HELE hPane, BOOL bShow, BOOL *pbHandled)
//参数<1> 框架窗口：框架窗口句柄
//参数<2> 窗格句柄：窗格元素的句柄
//参数<3> 是否显示：该窗格是否显示
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  窗口事件_窗格显示
#define  XWM_PANE_SHOW              0x7000+23

//@备注 框架窗口-主视图坐标改变事件，当框架窗口未绑定主视图(通过 XFrameWnd_SetView() 创建的主视图元素)时，且框架窗口尺寸发生改变时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 窗口_窗格显示(窗口句柄 框架窗口 整型 宽度, 整型 高度, 逻辑型 *是否拦截)
//	int WINAPI OnWndLayoutViewRect(HWINDOW hFrameWindow, int width, int height, BOOL *pbHandled)
//参数<1> 框架窗口：框架窗口句柄
//参数<2> 宽度：主视图区域宽度
//参数<3> 高度：主视图区域高度
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 窗口事件_主视图坐标改变
#define  XWM_BODYVIEW_RECT          0x7000+24
///@}
//@分组}


/////////////////////////////////////////////////////////////////////
/////////////////元素事件/////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//@分组{  元素事件
/// @addtogroup eleEvents
/// @{
//@备注 元素-过滤事件，除界面库提供的 'XE_' 开头的所有事件以外的事件亦可过滤到，通过判断 "wParam" 进行事件区分
//事件响应函数格式：
//	整型 WINAPI 元素_事件过滤(元素句柄 来源句柄, 正整型 事件类型, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnEventProc(HELE hEle, UINT nEvent, WPARAM wParam, LPARAM lParam, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 事件类型：元素触发的事件类型
//参数<3> WPARAM：附加参数1
//参数<4> LPARAM：附加参数2
//参数<5> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_事件过滤
#define  XE_ELEPROCE         1

///@备注 元素-绘制事件，当元素改变状态时(触发鼠标事件、调用重绘时)，触发该事件进行重绘(元素)自身。
//事件响应函数格式：
//	整型 WINAPI 元素_重绘(元素句柄 来源句柄, 绘图句柄 绘图, 逻辑型 *是否拦截)
//	int WINAPI OnDraw(HELE hEle, HDRAW hDraw,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图：绘图句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理 0-
//@别名  元素事件_绘制
#define  XE_PAINT            2

///@备注 元素-绘制完成事件，当元素或子元素完成自身绘制时，触发该事件，该事件需要元素启用 XEle_EnableEvent_XE_PAINT_END() 功能才能生效。
//事件响应函数格式：
//	整型 WINAPI 元素_绘制完成(元素句柄 来源句柄, 绘图句柄 绘图, 逻辑型 *是否拦截)
//	int WINAPI OnPaintEnd(HELE hEle, HDRAW hDraw,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图：绘图句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_绘制完成
#define  XE_PAINT_END        3

//@备注 元素-滚动视图绘制事件，当元素发生滚动(鼠标滑轮、拖动滚动条、改变滚动条位置)时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_绘制滚动视图(元素句柄 来源句柄, 绘图句柄 绘图, 逻辑型 *是否拦截)
//	int WINAPI OnDrawScrollView(HELE hEle, HDRAW hDraw,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图：绘图句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_绘制滚动视图
#define  XE_PAINT_SCROLLVIEW   4

//@备注 元素-鼠标移动事件，当鼠标在元素上移动时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标移动(元素句柄 来源句柄, 整型 标识, 点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnMouseMove(HELE hEle, UINT nFlags, POINT *pPt, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标识：标识
//参数<3> 鼠标位置：鼠标在元素上移动的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标移动
#define  XE_MOUSEMOVE        5

//@备注 元素-鼠标进入事件，当鼠标进入鼠标时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标进入(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnMouseStay(HELE hEle, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标进入
#define  XE_MOUSESTAY        6

//@备注 元素-鼠标悬停事件，当鼠标在元素上停留时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标悬停(元素句柄 来源句柄, 整型 标识,点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnMouseHover(HELE hEle, UINT nFlags, POINT *pPt, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标识：标识
//参数<3> 鼠标位置：鼠标在元素上停留的点(点->x，点->y)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标悬停
#define  XE_MOUSEHOVER       7

//@备注 元素-鼠标离开事件，当鼠标移出元素时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标离开(元素句柄 来源句柄, 元素句柄 停留元素句柄, 逻辑型 *是否拦截)
//	int WINAPI OnMouseLeave(HELE hEle, HELE hEleStay,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 鼠标停留元素句柄：鼠标停留元素句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标离开
#define  XE_MOUSELEAVE       8

//@备注 元素-鼠标滚轮滚动事件，当鼠标发生鼠标滚轮事件时，触发该事件，如果元素不是具有滚动视图(滚动条)时，需要元素启用XEle_EnableEvent_XE_MOUSEWHEEL()功能才能生效。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标滚动(元素句柄 来源句柄, 整型 标识, 点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnMouseWheel(HELE hEle, UINT nFlags,POINT *pPt,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标识：标识
//参数<3> 鼠标位置：点结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标滚动
#define  XE_MOUSEWHEEL         9

//@备注 元素-鼠标左键按下事件，当鼠标左键按下时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标左键按下(元素句柄 来源句柄, 正整型 标志, 点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnLButtonDown(HELE hEle, UINT nFlags, POINT *pPt,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标志：标志信息
//参数<3> 鼠标位置：点结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标左键按下
#define  XE_LBUTTONDOWN        10

//@备注 元素-鼠标左键弹起事件，当鼠标左键弹起时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标左键弹起(元素句柄 来源句柄, 正整型 标志, 点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnLButtonUp(HELE hEle, UINT nFlags, POINT *pPt,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标志：标志信息
//参数<3> 鼠标位置：点结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标左键弹起
#define  XE_LBUTTONUP          11

//@备注 元素-鼠标右键按下事件，当鼠标右键按下时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标右键按下(元素句柄 来源句柄, 正整型 标志, 点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnRButtonDown(HELE hEle, UINT nFlags, POINT *pPt,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标志：标志信息
//参数<3> 鼠标位置：点结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标右键按下
#define  XE_RBUTTONDOWN        12

//@备注 元素-鼠标右键弹起事件，当鼠标右键弹起时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标右键弹起(元素句柄 来源句柄, 正整型 标志, 点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnRButtonUp(HELE hEle, UINT nFlags, POINT *pPt,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标志：标志信息
//参数<3> 鼠标位置：点结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标右键弹起
#define  XE_RBUTTONUP          13

//@备注 元素-鼠标左键双击事件，当鼠标左键双击时，触发该事件。
//事件响应函数格式：
//	整型 WINAPI 元素_鼠标左键双击(元素句柄 来源句柄, 正整型 标志, 点结构 *鼠标位置, 逻辑型 *是否拦截)
//	int WINAPI OnLButtonDBClick(HELE hEle, UINT nFlags, POINT *pPt,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标志：标志信息
//参数<3> 鼠标位置：点结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_鼠标左键双击
#define  XE_LBUTTONDBCLICK     14


//#define  XE_RBUTTONDBCLICK     15


//@备注 元素-炫彩定时器（时钟）,非系统定时器,定时器消息
//事件响应函数格式：
//	整型 WINAPI 元素_炫彩定时器(元素句柄 来源句柄, 正整型 定时器ID, 逻辑型 *是否拦截)
//	int WINAPI OnEleXCTimer(HELE hEle,UINT nTimerID,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 定时器ID：定时器ID
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_炫彩定时器
#define  XE_XC_TIMER             16

//@备注 元素-调整布局事件, 暂停使用
//事件响应函数格式：
//	整型 WINAPI 元素_调整布局(元素句柄 来源句柄, 整型 标志, 正整型 调整编号, 逻辑型 *是否拦截)
//	int WINAPI OnAdjustLayout(HELE hEle,int nFlags, UINT nAdjustNo, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标志：标志信息
//参数<3> 调整编号：调整编号
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_调整布局
#define  XE_ADJUSTLAYOUT         17

//@备注 元素-调整布局完成事件
//事件响应函数格式：
//	整型 WINAPI 元素_调整布局完成(元素句柄 来源句柄, 整型 标志, 正整型 调整编号, 逻辑型 *是否拦截)
//	int WINAPI OnAdjustLayoutEnd(HELE hEle,int nFlags, UINT nAdjustNo, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 标志：标志信息
//参数<3> 调整编号：调整编号
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_调整布局完成
#define  XE_ADJUSTLAYOUT_END     18

//@备注 元素-工具提示弹出事件
//事件响应函数格式：
//	整型 WINAPI 元素_工具提示弹出(元素句柄 来源句柄, 窗口句柄 工具提示窗口, 常量 字符型* 文本, 逻辑型 *是否拦截)
//	int WINAPI OnTooltipPopup(HELE hEle,HWINDOW hWindowTooltip, const wchar_t* pText, BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 工具提示窗口：工具提示窗口句柄
//参数<3> 文本：工具提示文本
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_工具提示弹出
#define  XE_TOOLTIP_POPUP        19

//@备注 元素-获得焦点事件，当元素获得焦点后，触发该事件(前提是元素非鼠标穿透，启用焦点)
//事件响应函数格式：
//	整型 WINAPI 元素_获得焦点(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnSetFocus(HELE hEle,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_获得焦点
#define  XE_SETFOCUS           31

//@备注 元素-失去焦点事件，当元素获得焦点后，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_失去焦点(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnKillFocus(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_失去焦点
#define  XE_KILLFOCUS          32

//@备注 元素-即将销毁事件，在销毁元素之前触发，表示该元素正在进行销毁(还未销毁)
//事件响应函数格式：
//	整型 WINAPI 元素_即将销毁(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnDestroy(HELE hEle,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_即将销毁
#define  XE_DESTROY            33

//@备注 元素-销毁完成事件，在销毁元素之后触发(元素已经销毁)
//事件响应函数格式：
//	整型 WINAPI 元素_销毁完成(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnDestroyeEnd(HELE hEle,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_销毁完成
#define  XE_DESTROY_END        42

//@备注 元素-大小改变事件，当元素大小发生改变时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_大小改变(元素句柄 来源句柄, 整型 调整布局, 正整型 调整编号, 逻辑型 *是否拦截)
//	int WINAPI OnSize(HELE hEle,int nFlags, UINT nAdjustNo, BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 调整布局：调整布局标志
//参数<3> 调整编号：调整编号
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_大小改变
#define  XE_SIZE               36

//@备注 元素-显示隐藏事件，当元素显示或隐藏时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_显示(元素句柄 来源句柄, BOOL 显示, 逻辑型 *是否拦截)
//	int WINAPI OnShow(HELE hEle,BOOL bShow,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 显示：显示或隐藏元素，TRUE 或 FALSE
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_显示
#define  XE_SHOW               37

//@备注 元素-设置字体事件，当元素设置字体时，触发该事件(可在此环节替换元素字体句柄,并拦截此次操作，以达到动态修改效果)
//事件响应函数格式：
//	整型 WINAPI 元素_设置字体(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnSetFont(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_设置字体
#define  XE_SETFONT           38

//@备注 元素-按键事件，当元素获得焦点时键盘按下按键后，触发该事件(前提是元素非鼠标穿透，启用焦点才能触发该事件)
//事件响应函数格式：
//	整型 WINAPI 元素_键盘按下(元素句柄 来源句柄, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnKeyDown(HELE hEle,WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 参数1：按键信息，请参见MSDN WM_KEYDOWN
//参数<3> 参数2：请参见MSDN WM_KEYDOWN
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_键按下
#define  XE_KEYDOWN            39

//@备注 元素-按键事件，当元素获得焦点时键盘按弹起键后，触发该事件(前提是元素非鼠标穿透，启用焦点才能触发该事件)
//事件响应函数格式：
//	整型 WINAPI 元素_键盘弹起(元素句柄 来源句柄, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnKeyUp(HELE hEle,WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 参数1：按键信息，请参见MSDN WM_KEYUP
//参数<3> 参数2：请参见MSDN WM_KEYUP
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_键弹起
#define  XE_KEYUP             40

//@备注 元素-字符输入事件，当元素具有可输入状态(编辑框)时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_字符输入(元素句柄 来源句柄, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnChar(HELE hEle,WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 参数1：请参见MSDN WM_KEYDOWN
//参数<3> 参数2：请参见MSDN WM_KEYDOWN
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_字符输入
#define  XE_CHAR               41

//@备注 元素-系统键按下事件，当元素有(元素非鼠标穿透并启用焦点)焦点并按下系统键(Win键)时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_系统键按下(元素句柄 来源句柄, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnSysKeyDown(HELE hEle,WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 参数1：请参见MSDN WM_SYSKEYDOWN 
//参数<3> 参数2：请参见MSDN WM_SYSKEYDOWN 
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_系统键按下
#define  XE_SYSKEYDOWN     42

//@备注 元素-系统键弹起事件，当元素放开系统键(Win键)时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_系统键弹起(元素句柄 来源句柄, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnSysKeyUp(HELE hEle,WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 参数1：请参见MSDN WM_SYSKEYUP 
//参数<3> 参数2：请参见MSDN WM_SYSKEYUP 
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_系统键弹起
#define  XE_SYSKEYUP       43

//@备注 元素-设置鼠标捕获事件，当元素捕获到鼠标时，触发该事件(捕获后，元素所在窗口整个区域将会响应元素的事件，可再次调用XEle_SetCapture()开启或关闭)
//事件响应函数格式：
//	整型 WINAPI 元素_设置鼠标捕获(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnSetCapture(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_获得鼠标捕获
#define  XE_SETCAPTURE      51

//@备注 元素-失去鼠标捕获事件，当元素失去捕获时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_失去鼠标捕获(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnKillCapture(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_失去鼠标捕获
#define  XE_KILLCAPTURE     52

//@备注 元素-设置鼠标光标事件
//事件响应函数格式：
//	整型 WINAPI 元素_设置鼠标光标(元素句柄 来源句柄, 附加参数1 参数1, 附加参数2 参数2, 逻辑型 *是否拦截)
//	int WINAPI OnSetCursor(HELE hEle,WPARAM wParam,LPARAM lParam,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 参数1：请参见MSDN WM_SETCURSOR
//参数<3> 参数2：请参见MSDN WM_SETCURSOR
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_置光标
#define  XE_SETCURSOR            53

//@备注 元素-菜单弹出事件，当菜单弹出时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_菜单弹出(元素句柄 来源句柄, 菜单句柄 菜单, 逻辑型 *是否拦截)
//	int WINAPI OnMenuPopup(HELE hEle,HMENUX hMenu, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 菜单：菜单句柄
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_菜单弹出
#define  XE_MENU_POPUP       57

//@备注 元素-菜单弹出窗口事件，当菜单弹出后显示一个窗口，可以在本事件下设置窗口的样式(位置、坐标、窗口类型、阴影等.)
//事件响应函数格式：
//	整型 WINAPI 元素_菜单弹出窗口(元素句柄 来源句柄, 菜单句柄 菜单, 菜单弹出窗口结构* 信息, 逻辑型 *是否拦截)
//	int WINAPI OnMenuPopupWnd(HELE hEle,HMENUX hMenu,menu_popupWnd_* pInfo,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 菜单：菜单句柄
//参数<3> 信息：菜单弹出窗口结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_菜单弹出窗口
#define  XE_MENU_POPUP_WND     58

//@备注 元素-菜单项选择事件，当菜单弹出后，鼠标点选某一项菜单项时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_菜单项选择(元素句柄 来源句柄, 整型 菜单项id, 逻辑型 *是否拦截)
//	int WINAPI OnMenuSelect(HELE hEle,int nItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 菜单项id：菜单项id
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_菜单项选择
#define  XE_MENU_SELECT      59

//@备注 元素-绘制菜单背景, 启用该功能需要调用XMenu_EnableDrawBackground()
//事件响应函数格式：
//	整型 WINAPI 元素_菜单背景绘制(元素句柄 来源句柄, 绘图句柄 绘图, 菜单项绘制结构 *绘制信息, 逻辑型 *是否拦截)
//	int WINAPI OnMenuDrawBackground(HELE hEle,HDRAW hDraw,menu_drawBackground_ *pInfo,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图：绘图句柄(HDC)
//参数<3> 绘制信息：菜单项绘制结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_菜单背景绘制
#define  XE_MENU_DRAW_BACKGROUND   60

//@备注 元素-绘制菜单项事件，启用该功能需要调用XMenu_EnableDrawItem()
//事件响应函数格式：
//	整型 WINAPI 元素_菜单项绘制(元素句柄 来源句柄, 绘图句柄 绘图, 菜单项绘制结构* 信息, 逻辑型 *是否拦截)
//	int WINAPI OnMenuDrawItem(HELE hEle,HDRAW hDraw,menu_drawItem_* pInfo,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图句柄：绘图句柄(HDC)
//参数<3> 信息：菜单项绘制结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_菜单项绘制
#define  XE_MENU_DRAWITEM    61

//@备注 元素-菜单退出事件，当菜单关闭后，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_菜单退出(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnMenuExit(HELE hEle,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_菜单退出
#define  XE_MENU_EXIT        62

//@备注 按钮点击事件，当鼠标点击按钮时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 元素_按钮点击(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnBtnClick(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_按钮点击
#define  XE_BNCLICK            34

//@备注 元素-按钮选中事件，当按钮被选中时，触发该事件(按钮类型必须为 单选 或 多选 时)
//事件响应函数格式：
//	整型 WINAPI 元素_按钮选中(元素句柄 来源句柄, BOOL 是否选中, 逻辑型 *是否拦截)
//	int WINAPI OnButtonCheck(HELE hEle,BOOL bCheck,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否选中：按钮是否被选中
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_按钮选中
#define  XE_BUTTON_CHECK       35

//@备注 元素-滚动视图元素水平滚动事件，滚动视图触发(滚动视图触发,表明滚动视图已滚动完成)
//事件响应函数格式：
//	整型 WINAPI 滚动视图_水平滚动(元素句柄 来源句柄, 整型 滚动点, 逻辑型 *是否拦截)
//	int WINAPI OnScrollViewScrollH(HELE hEle,int pos,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 滚动点：当前滚动点
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_滚动视图水平滚动
#define  XE_SCROLLVIEW_SCROLL_H    54

//@备注 元素-滚动视图元素垂直滚动事件，滚动视图触发(滚动视图触发,表明滚动视图已滚动完成)
//事件响应函数格式：
//	整型 WINAPI 滚动视图_垂直滚动(元素句柄 来源句柄, 整型 滚动点, 逻辑型 *是否拦截)
//	int WINAPI OnScrollViewScrollV(HELE hEle,int pos,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 滚动点：当前滚动点
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_滚动视图垂直滚动
#define  XE_SCROLLVIEW_SCROLL_V    55

//@备注 元素-滚动条元素滚动事件，滚动条触发滚动时(还未滚动)，触发该事件
//事件响应函数格式：
//	整型 WINAPI 滚动条_滚动(元素句柄 来源句柄, 整型 滚动点, 逻辑型 *是否拦截)
//	int WINAPI OnSBarScroll(HELE hEle,int pos,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 滚动点：当前滚动点
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_滚动条滚动
#define  XE_SBAR_SCROLL        56

//@备注 元素-滑动条元素，滑块位置改变事件
//事件响应函数格式：
//	整型 WINAPI 滑动条_改变(元素句柄 来源句柄, 整型 位置, 逻辑型 *是否拦截)
//	int WINAPI OnSliderBarChange(HELE hEle,int pos,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 位置：滑块的新位置
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_滑动条改变
#define  XE_SLIDERBAR_CHANGE   63

//@备注 元素-进度条元素，进度改变事件
//事件响应函数格式：
//	整型 WINAPI 进度条_改变(元素句柄 来源句柄, 整型 进度, 逻辑型 *是否拦截)
//	int WINAPI OnProgressBarChange(HELE hEle,int pos,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 进度：新的进度值
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_进度条改变
#define  XE_PROGRESSBAR_CHANGE  64

//@备注 元素-元素-组合框下拉列表项选择事件
//事件响应函数格式：
//	整型 WINAPI 组合框_项选择(元素句柄 来源句柄, 整型 项索引, 逻辑型 *是否拦截)
//	int WINAPI OnComboBoxSelect(HELE hEle,int iItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项索引：被选择的项的索引
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_组合框项选择
#define  XE_COMBOBOX_SELECT       71

//@备注 元素-组合框下拉列表项选择完成事件，(组合框包含了普通的编辑框，此事件下 编辑框内容已经改变为选择的项目文本)
//事件响应函数格式：
//	整型 WINAPI 组合框_项选择完成(元素句柄 来源句柄, 整型 项索引, 逻辑型 *是否拦截)
//	int WINAPI OnComboBoxSelectEnd(HELE hEle,int iItem,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项索引：被选择的项的索引
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_组合框项选择完成
#define  XE_COMBOBOX_SELECT_END   74

//@备注 元素-组合框下拉列表弹出事件，当组合框被点击后弹出下拉列表后，触发该事件；本事件下可获得弹出的组合框下拉窗口句柄、下拉窗口内包含的列表框句柄，可通过该句柄进行设置窗口的样式(位置、坐标、窗口类型、阴影等.)，设置列表框的属性(加载列表框模板文件XML、设置列表框的样式、所继承的滚动视图包含的滚动条样式等.)
//事件响应函数格式：
//	整型 WINAPI 组合框_弹出列表(元素句柄 来源句柄, 窗口句柄 窗口, 元素句柄 列表框, 逻辑型 *是否拦截)
//	int WINAPI OnComboBoxPopupList(HELE hEle,HWINDOW hWindow,HELE hListBox,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 窗口：窗口句柄
//参数<3> 列表框：列表框元素句柄
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_组合框弹出列表
#define  XE_COMBOBOX_POPUP_LIST   72

//@备注 元素-组合框下拉列表退出事件，当组合框列表被销毁后，触发该事件
//事件响应函数格式：
//	整型 WINAPI 组合框_退出列表(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnComboBoxExitList(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_组合框退出列表
#define  XE_COMBOBOX_EXIT_LIST    73

//@备注 元素-列表框-模板创建事件，当列表框模板正在创建时(还未创建)，触发该事件；模板复用机制需先启用; 替换模板无效判断参数<标识>,因为内部会检查模板是否改变,不用担心重复该事件下可以处理一些模板替换(例如：XListBox_SetItemTemplate() \ 列表框_置项模板())
//事件响应函数格式：
//	整型 WINAPI 列表框_模板创建(元素句柄 来源句柄, 列表框项结构 * 项信息, 整型 标识, 逻辑型 *是否拦截)
//	int WINAPI OnListBoxTemplateCreate(HELE hEle,listBox_item_* pItem, int nFlag, BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表项结构信息
//参数<3> 标识：0:状态改变; 1:新模板实例; 2:旧模板复用
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表框模板创建
#define  XE_LISTBOX_TEMP_CREATE     81

//@备注 元素-列表框-项模板创建完成事件，当列表框模板创建完毕时(已创建)，触发该事件；模板复用机制需先启用；不管是新建还是复用，都需要更新数据，当为复用时不要注册事件以免重复注册
//事件响应函数格式：
//	整型 WINAPI 列表框_模板创建完成(元素句柄 来源句柄, 列表框项结构* 项信息, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnListBoxTemplateCreateEnd(HELE hEle,listBox_item_* pItem, int nFlag, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表框项结构信息
//参数<3> 标志：0:状态改变(复用)；1:新模板实例；2:旧模板复用
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表框模板创建完成
#define  XE_LISTBOX_TEMP_CREATE_END     82
//#define  XE_LISTBOX_TEMP_UPDATE   XE_LISTBOX_TEMP_CREATE_END

//@备注 元素-列表框-项模板销毁事件，当列表框滚动到项不可见(隐藏)区域时(启用模板复用不会销毁)，会自动销毁并触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表框_模板销毁(元素句柄 来源句柄, 列表框项结构* 项信息, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnListBoxTemplateDestroy(HELE hEle,listBox_item_* pItem, int nFlag, BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表框项结构信息
//参数<3> 标志：0:正常销毁；1:移动到缓存(不会被销毁，临时缓存备用，当需要时被复用)
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表框模板销毁
#define  XE_LISTBOX_TEMP_DESTROY    83

//备注 元素-列表框项模板调整坐标，已停用
//事件响应函数格式：
//	整型 WINAPI 列表框_模板调整坐标(元素句柄 来源句柄, 列表框项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnListBoxTemplateAdjustCoordinate(HELE hEle,listBox_item_* pItem, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表框项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//#define  XE_LISTBOX_TEMP_ADJUST_COORDINATE  84

//@备注 元素-列表框-项绘制事件，当列表框的项发生改变状态时(创建后显示时，调整大小)，触发该事件；进行重绘，如需自定义效果，可注册本事件并进行代码绘制(自定义绘制需要设置<是否拦截>为TRUE -> *是否拦截 = TRUE，表示不进行炫彩内部默认的绘制)。
//事件响应函数格式：
//	整型 WINAPI 列表框_项绘制(元素句柄 来源句柄, 绘图句柄 绘图句柄, 列表框项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnListBoxDrawItem(HELE hEle,HDRAW hDraw,listBox_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图句柄：用于标识要进行绘制的窗口或控件
//参数<3> 项信息：列表框项结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表框项绘制
#define  XE_LISTBOX_DRAWITEM     85

//@备注 元素-列表框-项选择事件，当列表框的项被选择时，触发该事件；可通过列表框 XListBox_SetSelectItem() \ 列表框_置选择项() 进行设置当前选中项，取当前选中可通过 XListBox_GetSelectItem() \ 列表框_取选择项() 获取当前选择的项；列表框默认是多选模式，可通过 XListBox_EnableMultiSel() \ 列表框_启用多选() 进行多选是否启用。
//事件响应函数格式：
//	整型 WINAPI 列表框_项选择(元素句柄 来源句柄, 整型 项索引, 逻辑型 *是否拦截)
//	int WINAPI OnListBoxSelect(HELE hEle,int iItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项索引：被选择的项的索引
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表框项选择
#define  XE_LISTBOX_SELECT       86

//@备注 元素-列表项模板创建事件，当列表模板正在创建时(还未创建)，触发该事件；模板复用机制需先启用; 替换模板无效判断参数<标识>,因为内部会检查模板是否改变,不用担心重复该事件下可以处理一些模板替换(例如：XList_SetItemTemplate() \ 列表_置项模板())；模板复用机制需先启用；替换模板无效判断标志，因为内部会检查模板是否改变，不用担心重复
//事件响应函数格式：
//	整型 WINAPI 列表_模板创建(元素句柄 来源句柄, 列表项结构* 项信息, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnListTemplateCreate(HELE hEle,list_item_* pItem,int nFlag, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表项结构信息
//参数<3> 标志：0:状态改变；1:新模板实例；2:旧模板复用
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表模板创建
#define  XE_LIST_TEMP_CREATE     101

//@备注 元素-列表-项模板创建完成事件，当列表模板创建完毕时(已创建)，触发该事件；模板复用机制需先启用；不管是新建还是复用，都需要更新数据，当为复用时不要注册事件以免重复注册
//事件响应函数格式：
//	整型 WINAPI 列表_模板创建完成(元素句柄 来源句柄, 列表项结构* 项, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnListTemplateCreateEnd(HELE hEle,list_item_* pItem, int nFlag, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表项结构信息
//参数<3> 标志：0:状态改变(复用)；1:新模板实例；2:旧模板复用
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表模板创建完成
#define  XE_LIST_TEMP_CREATE_END     102

//@备注 元素-列表-项模板销毁事件，当列表滚动到项不可见(隐藏)区域时(启用模板复用不会销毁)，会自动销毁并触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表_模板销毁(列表项结构* 项信息, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnListTemplateDestroy(HELE hEle,list_item_* pItem, int nFlag, BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表项结构信息
//参数<3> 标志：0-正常销毁；1-移动到缓存(元素句柄 来源句柄, 不会被销毁，临时缓存备用，当需要时被复用)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表模板销毁
#define  XE_LIST_TEMP_DESTROY    103

//备注 元素-列表-项模板调标事件，已停用
//事件响应函数格式：
//	整型 WINAPI 列表_模板调整坐标(元素句柄 来源句柄, 列表项结构* 项信息, 逻辑型 *是否拦截)
//	typedef int WINAPI OnListTemplateAdjustCoordinate(HELE hEle,list_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//#define  XE_LIST_TEMP_ADJUST_COORDINATE  104

//@备注 元素-列表绘制项，当列表的项发生改变状态时(创建后显示时，调整大小)，触发该事件进行重绘，如需自定义效果，可注册本事件并进行代码绘制(自定义绘制需要设置<是否拦截>为TRUE -> *是否拦截 = TRUE，表示不进行炫彩内部默认的绘制)。
//事件响应函数格式：
//	整型 WINAPI 列表_项绘制(元素句柄 来源句柄, 绘图句柄 绘图句柄, 列表项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnListDrawItem(HELE hEle,HDRAW hDraw,list_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图句柄：用于标识要进行绘制的窗口或控件
//参数<3> 项信息：列表项结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表项绘制
#define  XE_LIST_DRAWITEM                105

//@备注 元素-列表-项被选择事件，当列表的项被选择时，触发该事件；可通过列表框 XList_SetSelectItem() \ 列表_置选择项() 进行设置当前选中项，取当前选中可通过 XList_GetSelectItem() \ 列表_取选择项() 获取当前选择的项；列表默认是多选模式，可通过 XList_EnableMultiSel() \ 列表_启用多选() 进行多选是否启用。
//事件响应函数格式：
//	整型 WINAPI 列表_项选择(元素句柄 来源句柄, 整型 项索引, 逻辑型 *是否拦截)
//	int WINAPI OnListSelect(HELE hEle,int iItem,BOOL *pbHandled)
//参数<1> 项索引：被选择的项的索引
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表项选择
#define  XE_LIST_SELECT                  106

//@备注 元素-列表-绘制列表头项事件，当列表表头绘制时，触发该事件进行重绘；，如需自定义效果，可注册本事件并进行代码绘制(自定义绘制需要设置<是否拦截>为TRUE -> *是否拦截 = TRUE，表示不进行炫彩内部默认的绘制)。
//事件响应函数格式：
//	整型 WINAPI 列表_头项绘制(绘图句柄 绘图, 列表头项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnListHeaderDrawItem(HELE hEle,HDRAW hDraw, list_header_item_* pItem, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图：绘图句柄
//参数<3> 项信息：列表头项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表头项绘制
#define  XE_LIST_HEADER_DRAWITEM         107

//@备注 元素-列表-列表头点击事件，当列表头被点击时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表_头项点击(元素句柄 来源句柄, 整型 项索引, 逻辑型 *是否拦截)
//	int WINAPI OnListHeaderClick(HELE hEle,int iItem, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项索引：被点击的头项的索引
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表头项点击
#define  XE_LIST_HEADER_CLICK            108

//@备注 元素-列表-列表头宽度改变事件，当鼠标拖动列表头改变列宽度时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表_头项宽度改变(元素句柄 来源句柄, 整型 项索引, 整型 宽度, 逻辑型 *是否拦截)
//	int WINAPI OnListHeaderItemWidthChange(HELE hEle,int iItem, int nWidth BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项索引：宽度改变的头项的索引
//参数<3> 宽度：改变后的宽度
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表头项宽度改变
#define  XE_LIST_HEADER_WIDTH_CHANGE     109

//@备注 列表-列表头模板创建，当列表头模板正在创建时(还未创建)，触发该事件；
//事件响应函数格式：
//	整型 WINAPI 列表_头项模板创建(元素句柄 来源句柄, 列表头项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnListHeaderTemplateCreate(HELE hEle,list_header_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表头项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表头模板创建
#define  XE_LIST_HEADER_TEMP_CREATE          110

///@备注 元素-列表-列表头模板创建完成事件，当列表头模板创建完毕时(已创建)，触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表_头项模板创建完成(元素句柄 来源句柄, 列表头项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnListHeaderTemplateCreateEnd(HELE hEle,list_header_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表头项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表头模板创建完成
#define  XE_LIST_HEADER_TEMP_CREATE_END      111

//@备注 元素-列表-列表头模板销毁事件，当列表头销毁时，触发该事件；
//事件响应函数格式：
//	整型 WINAPI 列表_头项模板销毁(元素句柄 来源句柄, 列表头项结构* 项, 逻辑型 *是否拦截)
//	int WINAPI OnListHeaderTemplateDestroy(HELE hEle,list_header_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表头项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表头模板销毁
#define  XE_LIST_HEADER_TEMP_DESTROY          112

//备注 元素-列表-列表头项模板调整坐标事件，已停用
//事件响应函数格式：
//	整型 WINAPI 列表_头项模板调整坐标(元素句柄 来源句柄, 列表头项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnListHeaderTemplateDestroy(HELE hEle,list_header_item_* pItem,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表头项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//#define  XE_LIST_HEADER_TEMP_ADJUST_COORDINATE  113

//@备注 元素-列表树-模板创建事件，当列表树模板正在创建时(还未创建)，触发该事件；模板复用机制需先启用; 替换模板无效判断参数<标识>,因为内部会检查模板是否改变,不用担心重复该事件下可以处理一些模板替换(例如：XTree_SetItemTemplate() \ 列表树_置项模板())
//事件响应函数格式：
//	整型 WINAPI 列表树_头项模板调整坐标(元素句柄 来源句柄, 列表树项结构* 项信息, 整型 标识, 逻辑型 *是否拦截)
//	int WINAPI OnTreeTemplateCreate(HELE hEle,tree_item_* pItem,int nFlag, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表树项结构信息
//参数<3> 标识：0:状态改变; 1:新模板实例; 2:旧模板复用
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表树模板创建
#define  XE_TREE_TEMP_CREATE             121

//@备注 元素-列表树模板创建完成事件，当列表树模板创建完毕时(已创建)，触发该事件；模板复用机制需先启用；不管是新建还是复用，都需要更新数据，当为复用时不要注册事件以免重复注册
//事件响应函数格式：
//	整型 WINAPI 列表树_模板创建完成(元素句柄 来源句柄, 列表树项结构* 项信息, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnTreeTemplateCreateEnd(HELE hEle,tree_item_* pItem, int nFlag, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表树项结构信息
//参数<3> 标志：0:状态改变(复用)；1:新模板实例；2:旧模板复用
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表树模板创建完成
#define  XE_TREE_TEMP_CREATE_END         122
//#define  XE_TREE_TEMP_UPDATE   XE_TREE_TEMP_CREATE_END

//@备注 元素-列表树-模板销毁事件，当列表树滚动到项不可见(隐藏)区域时(启用模板复用不会销毁)，会自动销毁并触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表树_模板销毁(元素句柄 来源句柄, 列表树项结构* 项信息, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnTreeTemplateDestroy(HELE hEle,tree_item_* pItem, int nFlag, BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表树项结构信息
//参数<3> 标志：0:正常销毁；1:移动到缓存(不会被销毁，临时缓存备用，当需要时被复用)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表树模板销毁
#define  XE_TREE_TEMP_DESTROY            123

//备注 元素-列表树-项模板调整坐标，已停用
//事件响应函数格式：
//	整型 WINAPI 列表框_模板调整坐标(元素句柄 来源句柄, 列表树项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnTreeTemplateAdjustCoordinate(HELE hEle,tree_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表树项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//#define  XE_TREE_TEMP_ADJUST_COORDINATE  124

//@备注 元素-列表树-项绘制事件，当列表树的项发生改变状态时(创建后显示时，调整大小)，触发该事件；进行重绘，如需自定义效果，可注册本事件并进行代码绘制(自定义绘制需要设置<是否拦截>为TRUE -> *是否拦截 = TRUE，表示不进行炫彩内部默认的绘制)。
//事件响应函数格式：
//	整型 WINAPI 列表树_项绘制(元素句柄 来源句柄, 绘图句柄 绘图, 列表树项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnTreeDrawItem(HELE hEle,HDRAW hDraw,tree_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图：绘图句柄
//参数<3> 项信息：列表树项结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表树项绘制
#define  XE_TREE_DRAWITEM                125

//@备注 元素-列表树-项选择事件，当列表树的项被选择时，触发该事件；可通过列表框 XTree_SetSelectItem() \ 列表树_置选择项() 进行设置当前选中项，取当前选中可通过 XTree_GetSelectItem() \ 列表树_取选择项() 获取当前选择的项；列表框默认是多选模式，可通过 XTree_EnableMultiSel() \ 列表树_启用多选() 进行多选是否启用。
//事件响应函数格式：
//	整型 WINAPI 列表框_项选择(元素句柄 来源句柄, 整型 项索引, 逻辑型 *是否拦截)
//	int WINAPI OnTreeSelect(HELE hEle,int nItemID,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项索引：被选择的项的索引
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表树项选择
#define  XE_TREE_SELECT                 126

//@备注 元素-列表树-项展开收缩事件，当列表树的项被展开或收缩时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表框_项选择(元素句柄 来源句柄, 整型 id,BOOL 是否展开, 逻辑型 *是否拦截)
//	int WINAPI OnTreeExpand(HELE hEle,int id,BOOL bExpand,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> id：设置文本\图片时，填写的ID
//参数<3> 是否展开：是否已经展开该组
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表树项展开
#define  XE_TREE_EXPAND                 127

//@备注 元素-列表树-项正在拖动，当列表树的项被拖动时(正在拖动)，触发该事件；在本事件下，可修改参数<项信息>以达到动态修改的作用。可通过XTree_EnableDragItem() \ 列表树_启用拖动项() 进行启用或关闭；
//事件响应函数格式：
//	整型 WINAPI 列表框_项选择(元素句柄 来源句柄, 列表树拖动项结构 *项信息, 逻辑型 *是否拦截)
//	int WINAPI OnTreeDragItemIng(HELE hEle,tree_drag_item_* pInfo, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表树拖动项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名   元素事件_列表树项拖动中
#define  XE_TREE_DRAG_ITEM_ING           128

//@备注 元素-列表树-项拖动完成，当列表树的项被拖动时(拖动完成)，触发该事件；当拖动完成后，通过参数<项信息>判断是从哪个id拖动到另一个id。可通过XTree_EnableDragItem() \ 列表树_启用拖动项() 进行启用或关闭；
//事件响应函数格式：
//	整型 WINAPI 列表框_项选择(元素句柄 来源句柄, 列表树拖动项结构 *项信息, 逻辑型 *是否拦截)
//	int WINAPI OnTreeDragItem(HELE hEle,tree_drag_item_* pInfo, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表树拖动项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名   元素事件_列表树项拖动
#define  XE_TREE_DRAG_ITEM               129

//@备注 元素-列表视-项模板创建事件，当列表树模板正在创建时(还未创建)，触发该事件；模板复用机制需先启用; 替换模板无效判断参数<标识>,因为内部会检查模板是否改变,不用担心重复该事件下可以处理一些模板替换(例如：XTree_SetItemTemplate() \ 列表树_置项模板())
//事件响应函数格式：
//	整型 WINAPI 元素事件_列表视图模板创建完成(元素句柄 来源句柄, 列表视图项结构* 项信息, 整型 标识, 逻辑型 *是否拦截)
//	int WINAPI OnListViewTemplateCreate(HELE hEle,listView_item_* pItem,int nFlag, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表视图项结构信息
//参数<3> 标识：0:状态改变; 1:新模板实例; 2:旧模板复用
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表视图模板创建
#define  XE_LISTVIEW_TEMP_CREATE           141

//@备注 元素-列表视-项模板创建完成事件，当列表树模板创建完毕时(已创建)，触发该事件；模板复用机制需先启用；不管是新建还是复用，都需要更新数据，当为复用时不要注册事件以免重复注册
//事件响应函数格式：
//	整型 WINAPI 列表视_项模板创建完成(元素句柄 来源句柄, 列表视图项结构* 项信息, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnListViewTemplateCreateEnd(HELE hEle,listView_item_* pItem,int nFlag, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表视图项结构信息
//参数<3> 标志：0:状态改变(复用)；1:新模板实例；2:旧模板复用
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表视图模板创建完成
#define  XE_LISTVIEW_TEMP_CREATE_END           142
//#define  XE_LISTVIEW_TEMP_UPDATE     XE_LISTVIEW_TEMP_CREATE_END

//@备注 元素-列表视-项模板销毁事件，当列表树滚动到项不可见(隐藏)区域时(启用模板复用不会销毁)，会自动销毁并触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表视_模板销毁(元素句柄 来源句柄, 列表视图项结构* 项信息, 整型 标志, 逻辑型 *是否拦截)
//	int WINAPI OnListViewTemplateDestroy(HELE hEle,listView_item_* pItem, int nFlag, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表视图项结构信息
//参数<3> 标志：0:正常销毁；1:移动到缓存(不会被销毁，临时缓存备用，当需要时被复用)
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表视图模板销毁
#define  XE_LISTVIEW_TEMP_DESTROY           143

//备注 元素-列表视-项模板调整坐标，已停用
//事件响应函数格式：
//	整型 WINAPI 列表视_模板调整坐标(元素句柄 来源句柄, 列表视图项结构* 项信息, 逻辑型 *是否拦截)
//	
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：列表视图项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//#define  XE_LISTVIEW_TEMP_ADJUST_COORDINATE   144

//@备注 列表视-项绘制事件，当列表视的项发生改变状态时(创建后显示时，调整大小)，触发该事件；进行重绘，如需自定义效果，可注册本事件并进行代码绘制(自定义绘制需要设置<是否拦截>为TRUE -> *是否拦截 = TRUE，表示不进行炫彩内部默认的绘制)。
//事件响应函数格式：
//	整型 WINAPI 列表树_项绘制(元素句柄 来源句柄, 绘图句柄 绘图, 列表视图项结构* 项信息, 逻辑型 *是否拦截)
//	int WINAPI OnListViewDrawItem(HELE hEle,HDRAW hDraw,listView_item_* pItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 绘图：绘图句柄
//参数<3> 项信息：列表视图项结构信息
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表视图项绘制
#define  XE_LISTVIEW_DRAWITEM              145

//@备注 元素-列表视-项选择事件，当列表视的项被选择时，触发该事件；可通过列表框 XListView_SetSelectItem() \ 列表视_置选择项() 进行设置当前选中项，取当前选中可通过 XTree_GetSelectItem() \ 列表视_取选择项() 获取当前选择的项；列表框默认是多选模式，可通过 XListView_EnableMultiSel() \ 列表视_启用多选() 进行多选是否启用。
//事件响应函数格式：
//	整型 WINAPI 列表框_项选择(元素句柄 来源句柄, 整型 项索引, 逻辑型 *是否拦截)
//	int WINAPI OnListViewSelect(HELE hEle,int iGroup,int iItem,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项索引：被选择的项的索引
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表视图项选择
#define  XE_LISTVIEW_SELECT            146

//@备注 元素-列表视-项展开收缩事件，当列表视的项被展开或收缩时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 列表视_项选择(元素句柄 来源句柄, 整型 组ID,BOOL 是否展开, 逻辑型 *是否拦截)
//	int WINAPI OnListViewExpand(HELE hEle,int iGroup,BOOL bExpand,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 组ID：选择的组的ID
//参数<3> 是否展开：是否已经展开该组
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_列表视图组展开
#define  XE_LISTVIEW_EXPAND         147

//@备注 元素-属性网格-项值被改变事件，当属性网格项的值被改变时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 属性网格_项值改变(元素句柄 来源句柄, 整型 项ID, 逻辑型 *是否拦截)
//	int WINAPI OnPGridValueChange(HELE hEle,int nItemID,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> ID：被改变的项ID
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_属性网格值改变
#define  XE_PGRID_VALUE_CHANGE     151

//@备注 元素-属性网格-属性网格项设置事件，当属性网格项设置改变后，触发该事件
//事件响应函数格式：
//	整型 WINAPI 属性网格_项设置(元素句柄 来源句柄, 整型 项ID, 逻辑型 *是否拦截)
//	int WINAPI OnPGridItemSet(HELE hEle,int nItemID, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> ID：改变的项ID
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_属性网格项设置
#define  XE_PGRID_ITEM_SET          152

//@备注 元素-属性网格-项选择事件，当属性网格项被选择时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 属性网格_项选择(元素句柄 来源句柄, 整型 项ID, 逻辑型 *是否拦截)
//	int WINAPI OnPGridItemSelect(HELE hEle,int nItemID, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> ID：改变的项ID
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_属性网格项选择
#define  XE_PGRID_ITEM_SELECT         153

//@备注 属性网格-项调整坐标事件，当属性网格项尺寸调整(改变)时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 属性网格_项调整(元素句柄 来源句柄, 属性网格项结构 *项信息, 逻辑型 *是否拦截)
//	int WINAPI OnPGridItemAdjustCoordinate(HELE hEle,propertyGrid_item_* pItem, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项信息：属性网格项结构信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_属性网格项调整坐标
#define  XE_PGRID_ITEM_ADJUST_COORDINATE  154

//@备注 元素-属性网格-项销毁事件，当属性网格销毁时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 属性网格_项销毁(元素句柄 来源句柄, 整型 项ID, 逻辑型 *是否拦截)
//	int WINAPI OnPGridItemDestroy(HELE hEle,int nItemID, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> ID：改变的项ID
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_属性网格项销毁
#define  XE_PGRID_ITEM_DESTROY   155

//@备注 元素-属性网格-项展开事件，当属性网格项被展开时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 属性网格_项展开(元素句柄 来源句柄, 整型 项ID, BOOL 是否展开, 逻辑型 *是否拦截)
//	int WINAPI OnPGridItemExpand(HELE hEle,int nItemID, BOOL bExpand, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> ID：改变的项ID
//参数<3> 是否展开：是否已经展开该项
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_属性网格项展开
#define  XE_PGRID_ITEM_EXPAND    156

//@备注  元素-编辑框-设置事件, 当点击编辑框右侧设置按钮触发
// 整型 WINAPI 编辑框_设置(元素句柄 来源句柄, 逻辑型 *是否拦截)
// int WINAPI OnEditSet(元素句柄 来源句柄, 逻辑型 *是否拦截)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名  元素事件_编辑框设置
#define  XE_EDIT_SET           180  

/// @code int WINAPI OnEditDrawRow(HDRAW hDraw, int iRow, 逻辑型 *pbHandled)  @endcode
//#define  XE_EDIT_DRAWROW    181   //暂未使用

//@备注 元素-编辑框-内容改变事件，当编辑框内容发生改变(已改变)后，触发该事件
//事件响应函数格式：
//	整型 WINAPI 编辑框_内容改变(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnEditChanged(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_编辑框内容改变
#define  XE_EDIT_CHANGED   182

//@备注 元素-编辑框-位置改变事件，当编辑框获得输入焦点时，光标改变时所在的字符位置，触发该事件；从0开始计算位置，以字符为单位
//事件响应函数格式：
//	整型 WINAPI 编辑框_位置改变(元素句柄 来源句柄, 整型 位置, 逻辑型 *是否拦截)
//	int WINAPI OnEditPosChanged(HELE hEle,int iPos, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 位置：当前鼠标所在的位置
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_编辑框位置改变
#define  XE_EDIT_POS_CHANGED    183

//@备注 元素-编辑框-样式改变事件，当编辑框的样式发生改变时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 编辑框_样式改变(元素句柄 来源句柄, 整型 样式, 逻辑型 *是否拦截)
//	int WINAPI OnEditStyleChanged(HELE hEle,int iStyle, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 样式：改变的样式
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_样式改变
#define  XE_EDIT_STYLE_CHANGED  184

//@备注 编辑框-回车TAB对齐事件，返回需要TAB数量
//事件响应函数格式：
//	整型 WINAPI 编辑框_回车TAB对齐(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnEditEnterGetTabAlign(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 编辑框_回车TAB对齐,返回需要TAB数量
#define  XE_EDIT_ENTER_GET_TABALIGN    185


/// @code int WINAPI OnEditSwapRow(元素句柄 来源句柄, int iRow, int bArrowUp, 逻辑型 *pbHandled)  @endcode
///@别名  元素事件_交换行
//#define  XE_EDIT_SWAPROW     186   


//@备注 元素-多行编辑框-内容改变事件，当编辑框为多行模式且内容发生改变时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 多行编辑框_内容修改(元素句柄 来源句柄, 整型 开始行, 整型 改变行数量, 逻辑型 *是否拦截)
//	int WINAPI OnEditChangeRows(HELE hEle,int iRow, int nRows, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 开始行：开始的行
//参数<3> 改变行数量：改变行数量
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_多行内容修改事件
#define  XE_EDITOR_MODIFY_ROWS         190

/// @code int WINAPI OnEditorSetBreakpoint(int iRow, BOOL bCheck, 逻辑型 *pbHandled)  @endcode
//#define  XE_EDITOR_SETBREAKPOINT       191 //设置断点

/// @code int WINAPI OnEditorRemoveBreakpoint(int iRow, 逻辑型 *pbHandled)  @endcode
//#define  XE_EDITOR_REMOVEBREAKPOINT    192 //移除断点

// iRow: 更改行开始位置索引,  if(nChangeRows>0) iEnd= iRow + nChangeRows
// nChangeRows: 改变行数, 正数添加行, 负数删除行
/// @code int WINAPI OnEditorBreakpointChanged(int iRow, int nChangeRows, 逻辑型 *pbHandled)  @endcode
//#define  XE_EDIT_ROW_CHANGED  193 //可对断点位置修改

/// @code int WINAPI OnEditorAutoMatchSelect(int iRow, int nRows, 逻辑型 *pbHandled)  @endcode
//#define  XE_EDITOR_AUTOMATCH_SELECT  194

/// @brief 未公开, 光标位置改变, 格式代码检测
/// @code int WINAPI OnEditorFormatCodeTest(int iRow, int iCol, 逻辑型 *pbHandled)  @endcode
//#define  XE_EDITOR_FORMATCODE_TEST    187

//@备注 元素-TAB条-项选择事件，当TAB条的项被选择时，触发该事件
//事件响应函数格式：
//	整型 WINAPI TAB条_项选择(元素句柄 来源句柄, 整型 项, 逻辑型 *是否拦截)
//	int WINAPI OnTabBarSelect(HELE hEle,int iItem, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项：被选择的项
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_TAB条项选择
#define  XE_TABBAR_SELECT    221

//@备注 元素-TAB条-项删除事件，当TAB条的项被删除时，触发该事件
//事件响应函数格式：
//	整型 WINAPI TAB条_项删除(元素句柄 来源句柄, 整型 项, 逻辑型 *是否拦截)
//	int WINAPI OnTabBarDelete(HELE hEle,int iItem, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 项：被删除的项
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_TAB条项删除
#define  XE_TABBAR_DELETE    222

//@备注 元素-月历日期改变事件，当月历日期发生改变时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 月历改变(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnCalendarChange(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_月历改变
#define  XE_MONTHCAL_CHANGE   231

//@备注 元素-日期时间-内容改变事件，当日期时间的内容发生改变时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 日期时间_日期时间改变(元素句柄 来源句柄, 逻辑型 *是否拦截)
//	int WINAPI OnDateTimeChange(HELE hEle,BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_日期时间改变
#define  XE_DATETIME_CHANGE    241

//@备注 元素-日期时间-弹出月历卡片事件，当日期时间弹出月历卡片时，触发该事件；本事件下可获得弹出月历卡片的窗口句柄、可通过该句柄进行设置窗口的样式(位置、坐标、窗口类型、阴影等.) 设置月历的属性(月历当月、上下月、上下年、今天背景及文本颜色信息等.)
//事件响应函数格式：
//	整型 WINAPI 日期时间_日期时间弹出月历(元素句柄 来源句柄, 窗口句柄 月历卡片窗口, 元素句柄 月历句柄, 逻辑型 *是否拦截)
//	int WINAPI OnDateTimePopupMonthCal(HELE hEle,HWINDOW hMonthCalWnd,HELE hMonthCal,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 月历卡片窗口：弹出的月历卡片的窗口句柄
//参数<3> 月历句柄：弹出窗口的月历的句柄
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_日期时间弹出月历
#define  XE_DATETIME_POPUP_MONTHCAL     242

//@备注 元素-日期时间-退出月历卡片事件，当日期时间关闭月历卡片时，触发该事件
//事件响应函数格式：
//	整型 WINAPI 日期时间_日期时间退出月历(元素句柄 来源句柄, 窗口句柄 月历卡片窗口, 元素句柄 月历句柄, 逻辑型 *是否拦截)
//	int WINAPI OnDateTimeExitMonthCal(HELE hEle,HWINDOW hMonthCalWnd,HELE hMonthCal,BOOL *pbHandled) 
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> 月历卡片窗口：弹出的月历卡片的窗口句柄
//参数<3> 月历句柄：弹出窗口的月历的句柄
//参数<4> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_日期时间退出月历
#define  XE_DATETIME_EXIT_MONTHCAL      243

//@备注 元素-元素-文件拖入事件，需先启用:XWnd_EnableDragFiles()事件；当鼠标拖入文件到该元素(注册该事件的元素)时，触发该事件；本事件参数<hDropInfo>表示拖入的文件信息，该信息包含了，详情请参考:https://www.cnblogs.com/chechen/p/4062149.html.注意该地址内的wParam和本事件下的hDropInfo相同。
//事件响应函数格式：
//	整型 WINAPI 元素_文件拖入(元素句柄 来源句柄, HDROP hDropInfo, 逻辑型 *是否拦截)
//	int WINAPI OnDropFiles(HELE hEle,HDROP hDropInfo, BOOL *pbHandled)
//参数<1> 来源句柄：触发该事件的元素句柄
//参数<2> hDropInfo：拖入文件信息
//参数<3> 是否拦截：用于决定是否拦截此事件的默认处理
//@别名 元素事件_文件拖入
#define  XE_DROPFILES                 250

//#define  XE_LISTVIEW_DRAG_INSERT
//#define  XE_PANE_LOACK
//#define  XE_PANE_DOCK
//#define  XE_PANE_FLOAT

/// @code   int WINAPI OnEditColorChange(COLORREF color, 逻辑型 *pbHandled) @endcode
//#define  XE_EDIT_COLOR_CHANGE        260

///@}
//@分组}
*/
