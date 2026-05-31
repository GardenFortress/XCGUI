#ifndef XCGUI_CHAT_H
#define XCGUI_CHAT_H
//@模块名称  炫彩界面库-聊天气泡框
//@版本  1.0.0
//@日期  2026-05-24
//@作者  未闻花名(QQ936599025)
//@模块备注  基于 CXLayoutFrame 封装的聊天气泡富文本对话框。支持发送者/接收者消息、头像、名称、标签、气泡内容、系统消息、可点击消息、聊天数据提取与恢复、新消息提醒以及头像/名称/标签/气泡左右键点击回调。

//@依赖  module_xcgui_class.h
//@依赖  module_xcgui_editdw.h

#include <string>
#include <vector>

#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"

//@src "module_xcgui_chat.cpp"

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

class CXEditDW;

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

#endif
