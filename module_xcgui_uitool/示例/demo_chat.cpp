// CXChatBubbleBox 完整接口使用示例.
// 覆盖 module_xcgui_uitool.h 中 CXChatBubbleBox 所有 public 方法 + 全部事件回调 +
// 数据持久化 + 标签/名称/头像/颜色/圆角/字号/三态背景等所有可调样式.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建处实例化一个 CXChatBubbleBox, 把 g_chat 指针赋值即可.
//       主流程在 BuildChatDemo() 里.

#include "module_xcgui_uitool.h"

static CXChatBubbleBox* g_chat = NULL;
static CXEditDW*        g_inputEdit = NULL;

// =============================================================================
// 1. 事件回调
// =============================================================================

int WINAPI OnChatLClick(HELE hChat, int iItem, int insertType, int part,
                        int iTag, HXCGUI hSender, BOOL* pbHandled)
{
	switch (part){
	case chat_click_part_avatar:     break;
	case chat_click_part_name:       break;
	case chat_click_part_tag:        break;
	case chat_click_part_bubble:     break;
	case chat_click_part_message:    break;
	case chat_click_part_object:     break;
	case chat_click_part_newMessage: break;
	}
	(void)hChat; (void)iItem; (void)insertType; (void)iTag; (void)hSender; (void)pbHandled;
	return 0;
}

int WINAPI OnChatRClick(HELE, int, int, int, int, HXCGUI, BOOL*)
{
	return 0;
}

void WINAPI OnChatObjectLoaded(HELE hChat, int iItem, int iContent,
                               int objType, HXCGUI hObject, const wchar_t* pKey)
{
	if (objType == XC_BUTTON && pKey && wcscmp(pKey, L"confirm") == 0){
		// 例如给 "确认" 按钮重新挂事件
	}
	(void)hChat; (void)iItem; (void)iContent; (void)hObject;
}

// =============================================================================
// 2. 创建 + 全部样式 / 布局参数
// =============================================================================

void SetupChatStyle(CXChatBubbleBox& chat)
{
	chat.SetBubbleMaxWidth(0);
	chat.SetBubbleIndentation(10);
	chat.SetAvatarSize(36);
	chat.SetMessageSpace(16);
	chat.SetContentRightPadding(8);
	chat.SetAvatarRound(18);

	chat.SetBubbleColor(RGB(0xC8, 0xE6, 0xC9), RGB(0xF5, 0xF5, 0xF5));
	chat.SetBubbleTextColor(RGB(0x10, 0x20, 0x30), RGB(0x10, 0x20, 0x30));
	chat.SetBubbleFontSize(14, 14);

	chat.SetBubbleRound(8);
	chat.SetBubbleRoundEx(8, 2, 8, 8);
	chat.SetSenderBubbleRoundEx(8, 2, 8, 8);
	chat.SetReceiverBubbleRoundEx(8, 8, 8, 2);
	int lt = 0, rt = 0, rb = 0, lb = 0;
	chat.GetBubbleRoundEx(&lt, &rt, &rb, &lb);
	chat.GetSenderBubbleRoundEx(&lt, &rt, &rb, &lb);
	chat.GetReceiverBubbleRoundEx(&lt, &rt, &rb, &lb);

	chat.SetNameTextColor(RGB(0x4C, 0xAF, 0x50), RGB(0x21, 0x96, 0xF3));
	chat.SetTagTextColor (RGB(0x21, 0x96, 0xF3), RGB(0x21, 0x96, 0xF3));
	chat.SetNameFontSize(12, 12);
	chat.SetTagFontSize(11, 11);

	chat.SetTagBkStyle(RGB(0xE3, 0xF2, 0xFD),
	                   RGB(0xBB, 0xDE, 0xFB),
	                   RGB(0x90, 0xCA, 0xF9), 8);

	chat.SetHintTextColor(RGB(0x99, 0x99, 0x99), RGB(0x21, 0x96, 0xF3));
	chat.SetMessageTextColor(RGB(0x99, 0x99, 0x99));
	chat.SetClickableTextColor(RGB(0x21, 0x96, 0xF3));
	chat.SetMessageFontSize(11);
	chat.SetClickableFontSize(11);
	chat.SetClickableBkStyle(RGB(0xEE, 0xEE, 0xEE),
	                         RGB(0xE0, 0xE0, 0xE0),
	                         RGB(0xBD, 0xBD, 0xBD), 8);

	chat.EnableNewMessageLocate(TRUE);
	(void)chat.GetNewMessageButton();

	chat.SetLButtonClickEvent(OnChatLClick);
	chat.SetRButtonClickEvent(OnChatRClick);
	chat.SetObjectLoadedEvent(OnChatObjectLoaded);

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
	(void)chat.GetSenderBubbleRound();
	(void)chat.GetReceiverBubbleRound();
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
	chat.InsertMessage(L"------ 2026-05-26 23:00 ------");
	chat.InsertClickableMessage(L"点击查看历史消息");

	chat.SetInsertType(chat_insert_type_receiver);
	chat.SetInsertUserInfo(L"u_alice", L"Alice", L"D:\\avatar\\alice.png");
	chat.AddInsertTag(L"u_alice", L"VIP");
	chat.AddInsertTag(L"u_alice", L"已认证");

	chat.InsertBubbleBegin();
	chat.InsertText(L"你好, 这是一条普通文本消息.");
	chat.InsertBubbleEnd();

	chat.InsertBubbleBegin();
	chat.InsertText(L"看下这张图: ");
	chat.InsertImage(L"D:\\res\\sample.png");
	chat.InsertText(L"\n附带一个文件:");
	chat.InsertFile(L"D:\\res\\report.pdf");
	chat.InsertText(L"\n语音 / 视频:");
	chat.InsertVoice(L"D:\\res\\v.mp3");
	chat.InsertVideo(L"D:\\res\\v.mp4");
	HELE hConfirm = XBtn_Create(0, 0, 60, 24, L"确认", NULL);
	HELE hCancel  = XBtn_Create(0, 0, 60, 24, L"取消", NULL);
	chat.InsertObjectEx(hConfirm, L"confirm");
	chat.InsertObjectEx(hCancel,  L"cancel");
	chat.InsertObject(XBtn_Create(0, 0, 60, 24, L"详情", NULL));
	chat.InsertBubbleEnd();

	chat.InsertBubbleBeginEx(L"Alice (临时身份)", L"D:\\avatar\\alice2.png");
	chat.InsertText(L"这条用 InsertBubbleBeginEx 临时改名.");
	chat.InsertBubbleEnd();

	chat.SetInsertType(chat_insert_type_sender);
	chat.SetInsertUserInfo(L"u_me", L"我", L"D:\\avatar\\me.png");
	chat.AddInsertTag(L"u_me", L"管理员");
	chat.AddInsertTag(L"管理员-旧版API");

	chat.InsertBubbleBegin();
	chat.InsertText(L"我也回一条.");
	chat.InsertBubbleEnd();

	chat.ClearInsertTags(L"u_me");
	chat.InsertBubbleBegin();
	chat.InsertText(L"上一条之后清了标签, 这条没标签.");
	chat.InsertBubbleEnd();

	if (g_inputEdit){
		chat.SetInsertType(chat_insert_type_sender);
		chat.SetInsertUserInfo(L"u_me", L"我", L"D:\\avatar\\me.png");
		chat.InsertBubbleBegin();
		chat.InsertFromEditDW(g_inputEdit);
		chat.InsertBubbleEnd();
	}
}

// =============================================================================
// 4. 数据持久化
// =============================================================================

void DemoPersistence(CXChatBubbleBox& chat)
{
	CXVector<xcgui_chat_item_> data;
	chat.GetChatData(data);
	chat.SetChatData(data);

	CXBytes mem;
	chat.SaveToMem(mem);
	chat.LoadFromMem(mem.getPtr(), mem.size());

	chat.SaveToFile(L"D:\\chat\\session.bin");
	chat.LoadFromFile(L"D:\\chat\\session.bin");
}

// =============================================================================
// 5. 编辑框访问 + 滚动 / 计数 / 清空
// =============================================================================

void DemoUtilities(CXChatBubbleBox& chat)
{
	(void)chat.GetCurrentEdit();
	(void)chat.GetItemEdit(3);

	int n = chat.GetNewMessageCount();
	chat.ClearNewMessageCount();
	chat.LocateNewMessage();
	(void)n;

	chat.ClearMessages();
	// chat.DestroyChat();
}

// =============================================================================
// 6. 入口
// =============================================================================

void BuildChatDemo(HXCGUI hWnd)
{
	static CXEditDW inputEdit;
	inputEdit.Create(0, 610, 480, 80, hWnd);
	inputEdit.SetText(L"来自输入框的消息 \U0001F600");
	g_inputEdit = &inputEdit;

	static CXChatBubbleBox chat;
	chat.Create(0, 0, 480, 600, hWnd);
	g_chat = &chat;

	SetupChatStyle(chat);
	DemoInsertMessages(chat);
	DemoPersistence(chat);
	DemoUtilities(chat);
}
