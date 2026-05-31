// CXChatBubbleBox 完整接口使用示例.
// 覆盖头文件 module_xcgui_chat.h 中所有 public 方法 + 全部事件回调 + 数据持久化 +
// 标签 / 名称 / 头像 / 颜色 / 圆角 / 字号 / 三态背景等所有可调样式.
//
// 编译依赖: 跟主模块同套, 需链接 xcgui + 编辑框模块.
//
// 用法: 在窗口创建处实例化一个 CXChatBubbleBox, 把 g_chat 指针赋值即可.
//       主流程在 BuildChatDemo() 里.

#include "module_xcgui_chat.h"
#include "module_xcgui_editdw.h"

// 持有指针, 事件回调里要用
static CXChatBubbleBox* g_chat = NULL;
static CXEditDW*        g_inputEdit = NULL;

// =============================================================================
// 1. 事件回调
// =============================================================================

// 左键点击: 头像 / 名称 / 标签 / 气泡 / 内嵌对象 / 新消息按钮 都会触发.
int WINAPI OnChatLClick(HELE hChat, int iItem, int insertType, int part,
                        int iTag, HXCGUI hSender, BOOL* pbHandled)
{
	switch (part){
	case chat_click_part_avatar:     /* 头像 */              break;
	case chat_click_part_name:       /* 名称 */              break;
	case chat_click_part_tag:        /* 第 iTag 个标签 */    break;
	case chat_click_part_bubble:     /* 气泡空白处 */        break;
	case chat_click_part_message:    /* 居中消息 / 可点击 */ break;
	case chat_click_part_object:     /* 富文本内嵌对象 */    break;
	case chat_click_part_newMessage: /* 新消息提醒按钮 */    break;
	}
	return 0;
}

// 右键点击, 形参与左键一致.
int WINAPI OnChatRClick(HELE, int, int, int, int, HXCGUI, BOOL*)
{
	return 0;
}

// 加载阶段重建 UI 对象后回调, 可在此挂业务事件 / 按 pKey 区分同类型多对象.
void WINAPI OnChatObjectLoaded(HELE hChat, int iItem, int iContent,
                               int objType, HXCGUI hObject, const wchar_t* pKey)
{
	if (objType == XC_BUTTON && pKey && wcscmp(pKey, L"confirm") == 0){
		// 例如给 "确认" 按钮重新挂事件
	}
}

// =============================================================================
// 2. 创建 + 全部样式 / 布局参数
// =============================================================================

void SetupChatStyle(CXChatBubbleBox& chat)
{
	// --- 布局尺寸 ---
	chat.SetBubbleMaxWidth(0);            // 0 = 自动按聊天框 62%
	chat.SetBubbleIndentation(10);        // 气泡内文本缩进
	chat.SetAvatarSize(36);               // 头像宽高
	chat.SetMessageSpace(16);             // 相邻消息行间距
	chat.SetContentRightPadding(8);       // 内容区右内边距 (避免贴滚动条)
	chat.SetAvatarRound(18);              // 头像圆角 (18 + 36 = 圆形)

	// --- 气泡颜色 / 文本 / 字号 ---
	chat.SetBubbleColor(RGB(0xC8, 0xE6, 0xC9), RGB(0xF5, 0xF5, 0xF5));
	chat.SetBubbleTextColor(RGB(0x10, 0x20, 0x30), RGB(0x10, 0x20, 0x30));
	chat.SetBubbleFontSize(14, 14);

	// --- 气泡圆角 (统一 + 自定义二选一) ---
	chat.SetBubbleRound(8);
	chat.SetBubbleRoundEx(8, 2, 8, 8);    // 发送方右上削尖, 类似 IM 习惯
	int lt = 0, rt = 0, rb = 0, lb = 0;
	chat.GetBubbleRoundEx(&lt, &rt, &rb, &lb);

	// --- 名称 / 标签 颜色 + 字号 ---
	chat.SetNameTextColor(RGB(0x4C, 0xAF, 0x50), RGB(0x21, 0x96, 0xF3));
	chat.SetTagTextColor (RGB(0x21, 0x96, 0xF3), RGB(0x21, 0x96, 0xF3));
	chat.SetNameFontSize(12, 12);
	chat.SetTagFontSize(11, 11);

	// --- 标签三态背景 ---
	chat.SetTagBkStyle(RGB(0xE3, 0xF2, 0xFD),
	                   RGB(0xBB, 0xDE, 0xFB),
	                   RGB(0x90, 0xCA, 0xF9), 8);

	// --- 系统消息 / 可点击消息 颜色 + 字号 ---
	chat.SetHintTextColor(RGB(0x99, 0x99, 0x99), RGB(0x21, 0x96, 0xF3));   // 一次设两种
	chat.SetMessageTextColor(RGB(0x99, 0x99, 0x99));   // 单独覆写系统消息
	chat.SetClickableTextColor(RGB(0x21, 0x96, 0xF3)); // 单独覆写可点击
	chat.SetMessageFontSize(11);
	chat.SetClickableFontSize(11);
	chat.SetClickableBkStyle(RGB(0xEE, 0xEE, 0xEE),
	                         RGB(0xE0, 0xE0, 0xE0),
	                         RGB(0xBD, 0xBD, 0xBD), 8);

	// --- 新消息提醒 ---
	chat.EnableNewMessageLocate(TRUE);
	HELE hNewBtn = chat.GetNewMessageButton();   // 拿到按钮可继续个性化

	// --- 事件 ---
	chat.SetLButtonClickEvent(OnChatLClick);
	chat.SetRButtonClickEvent(OnChatRClick);
	chat.SetObjectLoadedEvent(OnChatObjectLoaded);

	// --- Get 系列 (示例性读取) ---
	(void)chat.GetBubbleMaxWidth();
	(void)chat.GetBubbleIndentation();
	(void)chat.GetMessageSpace();
	(void)chat.GetContentRightPadding();
	(void)chat.GetSenderBubbleColor();
	(void)chat.GetReceiverBubbleColor();
	(void)chat.GetSenderBubbleTextColor();
	(void)chat.GetReceiverBubbleTextColor();
	(void)chat.GetSenderBubbleFontSize();
	(void)chat.GetReceiverBubbleFontSize();
	(void)chat.GetBubbleRound();
	(void)chat.GetAvatarRound();
	(void)chat.GetSenderNameTextColor();
	(void)chat.GetReceiverNameTextColor();
	(void)chat.GetSenderTagTextColor();
	(void)chat.GetReceiverTagTextColor();
	(void)chat.GetSenderNameFontSize();
	(void)chat.GetReceiverNameFontSize();
	(void)chat.GetSenderTagFontSize();
	(void)chat.GetReceiverTagFontSize();
	(void)chat.GetMessageTextColor();
	(void)chat.GetClickableTextColor();
	(void)chat.GetMessageFontSize();
	(void)chat.GetClickableFontSize();
	(void)chat.IsEnableNewMessageLocate();
	(void)chat.GetInsertType();
	COLORREF lF, sF, dF; int round;
	chat.GetTagBkStyle(lF, sF, dF, round);
	chat.GetClickableBkStyle(lF, sF, dF, round);
}

// =============================================================================
// 3. 写消息 (覆盖全部 Insert* / Set Insert* 接口)
// =============================================================================

void DemoInsertMessages(CXChatBubbleBox& chat)
{
	// --- 系统提示 + 可点击提示 ---
	chat.InsertMessage(L"------ 2026-05-26 23:00 ------");
	chat.InsertClickableMessage(L"点击查看历史消息");

	// =========== 接收方 (左侧) ===========
	chat.SetInsertType(chat_insert_type_receiver);
	chat.SetInsertUserInfo(L"u_alice", L"Alice", L"D:\\avatar\\alice.png");
	// 标签按 senderId 维度持续, 一次配置可被多条气泡共享.
	chat.AddInsertTag(L"u_alice", L"VIP");
	chat.AddInsertTag(L"u_alice", L"已认证");

	// 简单文本气泡
	chat.InsertBubbleBegin();
	chat.InsertText(L"你好, 这是一条普通文本消息.");
	chat.InsertBubbleEnd();

	// 多内容混排气泡 (文本 + 图片 + 文件 + 语音 + 视频 + UI 对象)
	chat.InsertBubbleBegin();
	chat.InsertText(L"看下这张图: ");
	chat.InsertImage(L"D:\\res\\sample.png");
	chat.InsertText(L"\n附带一个文件:");
	chat.InsertFile(L"D:\\res\\report.pdf");
	chat.InsertText(L"\n语音 / 视频:");
	chat.InsertVoice(L"D:\\res\\v.mp3");
	chat.InsertVideo(L"D:\\res\\v.mp4");
	// 内嵌按钮: InsertObject 简化版 / InsertObjectEx 带 key, 加载回调里能区分
	HELE hConfirm = XBtn_Create(0, 0, 60, 24, L"确认", NULL);
	HELE hCancel  = XBtn_Create(0, 0, 60, 24, L"取消", NULL);
	chat.InsertObjectEx(hConfirm, L"confirm");
	chat.InsertObjectEx(hCancel,  L"cancel");
	chat.InsertObject(XBtn_Create(0, 0, 60, 24, L"详情", NULL));   // 不带 key
	chat.InsertBubbleEnd();

	// 临时覆盖名称 / 头像 (仅本气泡, 不影响 user profile)
	chat.InsertBubbleBeginEx(L"Alice (临时身份)", L"D:\\avatar\\alice2.png");
	chat.InsertText(L"这条用 InsertBubbleBeginEx 临时改名.");
	chat.InsertBubbleEnd();

	// =========== 发送方 (右侧) ===========
	chat.SetInsertType(chat_insert_type_sender);
	chat.SetInsertUserInfo(L"u_me", L"我", L"D:\\avatar\\me.png");
	chat.AddInsertTag(L"u_me", L"管理员");                // 等价 AddInsertTag(L"u_me", ...)
	chat.AddInsertTag(L"管理员-旧版API");                  // 老接口: 走当前方向 active

	chat.InsertBubbleBegin();
	chat.InsertText(L"我也回一条.");
	chat.InsertBubbleEnd();

	// 清掉某个发送者的标签
	chat.ClearInsertTags(L"u_me");
	chat.InsertBubbleBegin();
	chat.InsertText(L"上一条之后清了标签, 这条没标签.");
	chat.InsertBubbleEnd();

	// =========== 把外部 CXEditDW 内容整条搬进气泡 ===========
	if (g_inputEdit){
		chat.SetInsertType(chat_insert_type_sender);
		chat.SetInsertUserInfo(L"u_me", L"我", L"D:\\avatar\\me.png");
		chat.InsertBubbleBegin();
		chat.InsertFromEditDW(g_inputEdit);   // 文本/图片/UI 对象按出现顺序复制过来
		chat.InsertBubbleEnd();
	}
}

// =============================================================================
// 4. 数据持久化 (取/置 + 内存/文件 双通道)
// =============================================================================

void DemoPersistence(CXChatBubbleBox& chat)
{
	// 取整份聊天记录 (含路径 / UI 对象句柄等)
	CXVector<xcgui_chat_item_> data;
	chat.GetChatData(data);

	// 用相同结构覆写: 加载前会 ClearMessages
	chat.SetChatData(data);

	// 序列化到内存 (不带 UI 对象句柄, 仅二进制数据)
	CXBytes mem;
	chat.SaveToMem(mem);
	chat.LoadFromMem(mem.getPtr(), mem.size());

	// 落盘 / 读盘
	chat.SaveToFile(L"D:\\chat\\session.bin");
	chat.LoadFromFile(L"D:\\chat\\session.bin");
}

// =============================================================================
// 5. 编辑框访问 + 滚动 / 计数 / 清空 / 销毁
// =============================================================================

void DemoUtilities(CXChatBubbleBox& chat)
{
	// 当前正在插入的气泡内的编辑框 (须在 InsertBubbleBegin / End 之间调用)
	HELE hCurEdit = chat.GetCurrentEdit();

	// 任意一条消息的编辑框 (非气泡返回 NULL)
	HELE hItem3 = chat.GetItemEdit(3);

	// 新消息提醒计数 / 清空 / 滚动到底
	int n = chat.GetNewMessageCount();
	chat.ClearNewMessageCount();
	chat.LocateNewMessage();   // 滚动到底部并清空提醒

	// 清空消息但保留控件
	chat.ClearMessages();

	// 销毁聊天框 (走完析构)
	// chat.DestroyChat();
}

// =============================================================================
// 6. 入口: 创建窗口里的聊天框 + 跑一遍上面的演示
// =============================================================================

void BuildChatDemo(HXCGUI hWnd)
{
	static CXChatBubbleBox chat;
	chat.Create(0, 0, 480, 600, hWnd);
	g_chat = &chat;

	// 也支持构造时一次性创建:
	// CXChatBubbleBox chat2(0, 0, 480, 600, hWnd);

	SetupChatStyle(chat);
	DemoInsertMessages(chat);
	DemoPersistence(chat);
	DemoUtilities(chat);
}
