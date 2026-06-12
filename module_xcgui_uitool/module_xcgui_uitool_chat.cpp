//============================================================================
// module_xcgui_uitool_chat.cpp — CXChatBubbleBox split (原 module_xcgui_chat.cpp 纯搬运)
// 仅由 module_xcgui_uitool.cpp #include; 勿单独编译.
// 依赖同 TU 内的 CXEditDW (uitool_editdw split).
//============================================================================

#ifndef _XCGUI_UITOOL_AGGREGATED_
#error "module_xcgui_uitool_chat.cpp must be included from module_xcgui_uitool.cpp only"
#endif

#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstring>

struct _xcgui_chat_hit_
{
	HELE hEle;
	_xcgui_chat_node_* pNode;
	int part;
	int iTag;
};

struct _xcgui_chat_node_
{
	xcgui_chat_item_ data;
	HELE hRow;
	HELE hAvatar;
	HELE hMeta;     // 水平布局容器 (XLayout horiz), 装名称按钮 + 标签按钮 (auto width)
	HELE hName;
	HELE hEdit;
	HXCGUI hCenterText;
	CXEditDW* pEditDW;
	std::vector<HELE> hTags;
	std::vector<HIMAGE> hImages;
	RECT rcBubble;
	int index;
	int rowHeight;
};

// 气泡 "无包裹" 判定: 所有内容原子均为 image / UI 对象 (无文本 / 文件 / 语音 / 视频
// 等会被渲染成文本的类型) → 不画气泡背景, 不留内边距, 让图片 / 控件直接占位. 用户场景:
// 单图 / 单 UI 元素消息 (类似 IM 单独发图).
static bool _xcchat_is_borderless_bubble(const _xcgui_chat_node_* pNode)
{
	if (!pNode) return false;
	const CXVector<xcgui_chat_content_>& contents = pNode->data.contents;
	if (contents.size() == 0) return false;
	for (int i = 0; i < (int)contents.size(); ++i){
		int t = contents[i].type;
		if (t != chat_content_type_image && t != chat_content_type_object) return false;
	}
	return true;
}

static int _xcchat_max(int a, int b){ return a > b ? a : b; }
static int _xcchat_min(int a, int b){ return a < b ? a : b; }
static int _xcchat_clamp(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }
static void _xcchat_prepare_ele(HELE hEle)
{
	if (!hEle) return;
	XEle_EnableBkTransparent(hEle, TRUE);
	XEle_EnableDrawBorder(hEle, FALSE);
	// 所有走本助手的非 Shape 元素 (hRow / hAvatar / hName / hTag / hEdit / 可点击消息按钮 /
	// 新消息按钮) 都不画焦点矩形, 否则点击 / Tab 切焦点会出现虚线框, 与气泡 UI 风格不符.
	XEle_EnableDrawFocus(hEle, FALSE);
}

// 给可点击元素设手型光标. 用 IDC_HAND 系统光标 (链接 / 按钮悬停常见). 一次性 LoadCursor,
// 静态缓存 - 系统光标无需 Destroy, 直接缓存到进程结束.
// 取元素所在窗口当前 DPI 百分比 (XCGUI 约定: 100=100% / 150=150% / 200=200%).
// 100% 时返回 100, 150% 时返回 150, 失败兜底 100.
static int _xcchat_get_dpi_percent(HELE hEle)
{
	if (!hEle) return 100;
	HWINDOW xWnd = XWidget_GetHWINDOW(hEle);
	if (!xWnd) return 100;
	int p = XWnd_GetDPI(xWnd);
	return p > 0 ? p : 100;
}

static void _xcchat_set_hand_cursor(HELE hEle)
{
	if (!hEle) return;
	static HCURSOR sHand = NULL;
	// IDC_HAND 在某些 SDK 下展开为 LPSTR 与 LoadCursorW 签名不兼容. IDC_HAND 数值固定 = 32649,
	// 显式 MAKEINTRESOURCEW 转 LPCWSTR 即可兼容所有编译配置.
	if (!sHand) sHand = LoadCursorW(NULL, MAKEINTRESOURCEW(32649));
	if (sHand) XEle_SetCursor(hEle, sHand);
}

// 应用 离开/停留/按下 三态圆角填充 + 三态文本颜色 BkInfo 字符串.
// 该字符串遵循 XCGUI 内部序列化格式:
//   99 = 版本号, 98 = 状态→对象索引映射 (16→0 即离开走 obj0, 32→1 停留, 64→2 按下),
//   5  = 圆角填充对象 (22:颜色 23:alpha 9:四角圆角 40:可选阴影/平滑标志),
//   8  = 状态文本颜色绑定 (1:状态值 5:文本颜色).
// 通过此格式可以让 XCGUI 自己负责状态切换重绘, 无需我们再监听鼠标事件手动改色.
static void _xcchat_apply_state_bk(HELE hEle,
                                   COLORREF leaveFill, COLORREF stayFill, COLORREF downFill,
                                   COLORREF leaveText, COLORREF stayText, COLORREF downText,
                                   int round)
{
	if (!hEle) return;
	// XCGUI BkInfo 把 RGB 与 alpha 分两个键: 22(RGB-with-alpha-bits-FF) + 23(alpha 0~255).
	// COLORREF 用 RGBA(r,g,b,a) 把 alpha 编进高 8 位 (0xAA RR GG BB), 这里要拆开:
	//   22 字段: 把 alpha 位强制成 0xFF (与参考串中的 4294967295 / 0xFFFFFFFF 一致), 否则
	//            再被 23 字段乘一次会导致颜色看起来更透.
	//   23 字段: 取原 COLORREF 的 alpha 通道.
	// 文本色 (8:5) 单值, XCGUI 使用其 alpha 直接渲染, 无需拆分.
	auto rgb = [](COLORREF c)->unsigned{ return (unsigned)(c | 0xFF000000); };
	auto a   = [](COLORREF c)->unsigned{ return (unsigned)((c >> 24) & 0xFF); };
	wchar_t buf[1024] = {0};
	swprintf_s(buf, 1024,
		L"{99:1.9.9;"
		L"98:16(0)32(1)64(2);"
		L"5:2(15)20(1)21(3)26(1)22(%u)23(%u)9(%d,%d,%d,%d);"
		L"5:2(15)20(1)21(3)26(1)22(%u)23(%u)9(%d,%d,%d,%d);"
		L"5:2(15)20(1)21(3)26(1)22(%u)23(%u)40(1)9(%d,%d,%d,%d);"
		L"8:1(16)5(%u);"
		L"8:1(32)5(%u);"
		L"8:1(64)5(%u);}",
		rgb(leaveFill), a(leaveFill), round, round, round, round,
		rgb(stayFill),  a(stayFill),  round, round, round, round,
		rgb(downFill),  a(downFill),  round, round, round, round,
		(unsigned)leaveText, (unsigned)stayText, (unsigned)downText);
	XEle_SetBkInfo(hEle, buf);
}

CXChatBubbleBox::CXChatBubbleBox()
{
	m_currentNode = NULL;
	m_hNewMsgButton = NULL;
	m_enableNewMessageLocate = TRUE;
	m_forceScrollOnNextEnd = FALSE;
	m_inSizeImpl = FALSE;
	m_newMessageCount = 0;
	m_insertType = chat_insert_type_receiver;
	m_bubbleMaxWidth = 0;
	m_bubbleIndentation = 10;
	m_avatarSize = 36;
	m_messageSpace = 16;
	m_rowPadding = 12;
	// 滚动条 ↔ 内容之间的右内边距, 避免气泡贴到垂直滚动条上. 全局生效, 通过
	// Set/GetContentRightPadding 调整.
	m_contentRightPadding = 8;
	m_metaHeight = 24;
	m_lineHeight = 26;
	m_senderBubbleColor = RGBA(149, 236, 105, 255);
	m_receiverBubbleColor = RGBA(245, 245, 245, 255);
	m_senderTextColor = RGBA(0, 0, 0, 255);
	m_receiverTextColor = RGBA(0, 0, 0, 255);
	// 0 = 沿用 CXEditDW 默认 (14pt). 通过 SetBubbleFontSize(sender, receiver) 修改.
	m_senderBubbleFontSize = 0;
	m_receiverBubbleFontSize = 0;
	m_messageTextColor = RGBA(130, 130, 130, 255);
	m_clickableTextColor = RGBA(150, 150, 150, 255);
	// 发送方/接收方各自一组 4 角圆角, 默认对称 8. 旧接口 SetBubbleRound* 同时写两套.
	m_senderBubbleRoundLT = m_senderBubbleRoundRT = m_senderBubbleRoundRB = m_senderBubbleRoundLB = 8;
	m_receiverBubbleRoundLT = m_receiverBubbleRoundRT = m_receiverBubbleRoundRB = m_receiverBubbleRoundLB = 8;
	// 头像默认圆角 18, 配合默认 m_avatarSize=36 呈现圆形.
	m_avatarRound = 18;
	// 名称 / 标签 文本色 + 字号 (sender / receiver 各一份, 0=用 XCGUI 默认字体).
	m_senderNameColor = RGBA(110, 110, 110, 255);
	m_receiverNameColor = RGBA(110, 110, 110, 255);
	m_senderTagColor = RGBA(45, 120, 210, 255);
	m_receiverTagColor = RGBA(45, 120, 210, 255);
	m_senderNameFontSize = 0;
	m_receiverNameFontSize = 0;
	m_senderTagFontSize = 0;
	m_receiverTagFontSize = 0;
	m_senderNameFont = NULL;
	m_receiverNameFont = NULL;
	m_senderTagFont = NULL;
	m_receiverTagFont = NULL;
	m_messageFontSize = 0;
	m_clickableFontSize = 0;
	m_messageFont = NULL;
	m_clickableFont = NULL;
	// 默认参考 image\聊天气泡参考.png 的 VIP 标签样式: 浅蓝填充, 鼠标 hover/down 时略加深.
	m_tagFillLeave = RGBA(230, 242, 255, 255);
	m_tagFillStay  = RGBA(210, 232, 255, 255);
	m_tagFillDown  = RGBA(190, 222, 255, 255);
	m_tagRound = 10;
	// 可点击 (时间/系统提示) 的灰底圆角胶囊样式.
	m_clickableFillLeave = RGBA(230, 230, 230, 180);
	m_clickableFillStay  = RGBA(215, 215, 215, 210);
	m_clickableFillDown  = RGBA(195, 195, 195, 230);
	m_clickableRound = 12;
	m_leftClickEvent = NULL;
	m_rightClickEvent = NULL;
	m_objectLoadedEvent = NULL;
}

CXChatBubbleBox::~CXChatBubbleBox()
{
	ReleaseNodes(FALSE);
	// 释放本类创建的字体: SetXxxFontSize 内部走 XFont_Create, 默认带引用计数, XFont_Release
	// 减引用为 0 时销毁. 我们没有 EnableAutoDestroy, 必须显式 Release.
	if (m_senderNameFont) { XFont_Release(m_senderNameFont); m_senderNameFont = NULL; }
	if (m_receiverNameFont) { XFont_Release(m_receiverNameFont); m_receiverNameFont = NULL; }
	if (m_senderTagFont) { XFont_Release(m_senderTagFont); m_senderTagFont = NULL; }
	if (m_receiverTagFont) { XFont_Release(m_receiverTagFont); m_receiverTagFont = NULL; }
	if (m_messageFont) { XFont_Release(m_messageFont); m_messageFont = NULL; }
	if (m_clickableFont) { XFont_Release(m_clickableFont); m_clickableFont = NULL; }
}

HELE CXChatBubbleBox::Create(int x, int y, int cx, int cy, HXCGUI hParent)
{
	m_hEle = XLayoutFrame_Create(x, y, cx, cy, hParent);
	if (!m_hEle) return NULL;
	XLayoutBox_SetAlignV(m_hEle, layout_align_top);
	XLayoutBox_SetAlignH(m_hEle, layout_align_left);
	// 行间距 + 上下留白交给 XLayoutBox/Padding, 不再用 RelayoutNode 里的手算 y. 之前在
	// LayoutBox 自动布局开启的情况下又用 SetRectEx 写 y, 两者抢位 (LayoutBox 异步重排
	// 会覆盖我们的 y) → 出现 "顶部一大块空白 / 行被推到下半区, 90% 概率重现" 的随机现象.
	XLayoutBox_SetSpace(m_hEle, m_messageSpace);
	XSView_SetLineSize(m_hEle, 0, 48);
	XSView_EnableAutoShowScrollBar(m_hEle, TRUE);
	XSView_ShowSBarH(m_hEle, FALSE);
	XEle_SetPadding(m_hEle, 0, m_messageSpace, 0, m_messageSpace);
	_xcchat_prepare_ele(m_hEle);
	InstallFrameEvents();

	m_hNewMsgButton = XBtn_Create(0, 0, 116, 34, L"新消息 0", m_hEle);
	if (m_hNewMsgButton){
		_xcchat_prepare_ele(m_hNewMsgButton);
		XEle_SetLockScroll(m_hNewMsgButton, TRUE, TRUE);
		XEle_EnableTopmost(m_hNewMsgButton, TRUE);
		XEle_EnableDrawBorder(m_hNewMsgButton, FALSE);
		_xcchat_set_hand_cursor(m_hNewMsgButton);
		BindHit(m_hNewMsgButton, NULL, chat_click_part_newMessage, -1);
		XEle_RegEventCPP1(m_hNewMsgButton, XE_LBUTTONUP, &CXChatBubbleBox::OnLButtonUpImpl);
		XWidget_Show(m_hNewMsgButton, FALSE);
	}
	return m_hEle;
}

void CXChatBubbleBox::InstallFrameEvents()
{
	if (!m_hEle) return;
	XEle_RegEventCPP1(m_hEle, XE_SIZE, &CXChatBubbleBox::OnSizeImpl);
	// 父窗口最大化/还原时, 子元素 XE_SIZE 触发顺序可能早于 XLayoutFrame 真正完成布局
	// (GetViewClientWidth 在该时刻读到的还是旧 viewport), 单 XE_SIZE 重排算出的 maxBubble
	// 用的是旧宽度 -> 气泡停留在旧尺寸, 用户必须手动拖一下窗口才"恢复". 这里追加监听
	// XE_ADJUSTLAYOUT_END (布局调整完成事件), 它在窗口/父布局算完后再触发一次, 此时
	// GetViewClientWidth 取到的是新值, 再 RelayoutAll 一次即可对齐.
	XEle_RegEventCPP1(m_hEle, XE_ADJUSTLAYOUT_END, &CXChatBubbleBox::OnSizeImpl);
	XEle_RegEventCPP1(m_hEle, XE_DESTROY_END, &CXChatBubbleBox::OnDestroyEndImpl);
	XEle_RegEventCPP1(m_hEle, XE_MOUSEWHEEL, &CXChatBubbleBox::OnMouseWheelImpl);
}

void CXChatBubbleBox::ClearMessages()
{
	ReleaseNodes(TRUE);
	m_currentNode = NULL;
	m_newMessageCount = 0;
	UpdateTotalSize(1);
	ShowNewMessageButton(FALSE);
	if (m_hEle) XEle_Redraw(m_hEle, FALSE);
}

void CXChatBubbleBox::DestroyChat()
{
	ReleaseNodes(TRUE);
	if (m_hNewMsgButton){
		XEle_Destroy(m_hNewMsgButton);
		m_hNewMsgButton = NULL;
	}
	if (m_hEle){
		XEle_Destroy(m_hEle);
		m_hEle = NULL;
	}
}

void CXChatBubbleBox::ReleaseNodes(BOOL bDestroyRows)
{
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p) continue;
		if (bDestroyRows && p->hRow){
			XEle_Destroy(p->hRow);
		}
		for (size_t k = 0; k < p->hImages.size(); ++k){
			if (p->hImages[k]) XImage_Release(p->hImages[k]);
		}
		if (p->pEditDW){
			delete p->pEditDW;
			p->pEditDW = NULL;
		}
		delete p;
	}
	m_nodes.clear();
	if (bDestroyRows && m_hNewMsgButton){
		std::vector<_xcgui_chat_hit_*> keepHits;
		for (size_t i = 0; i < m_hits.size(); ++i){
			if (m_hits[i] && m_hits[i]->hEle == m_hNewMsgButton){
				keepHits.push_back(m_hits[i]);
			}
			else{
				delete m_hits[i];
			}
		}
		m_hits.swap(keepHits);
	}
	else{
		for (size_t i = 0; i < m_hits.size(); ++i){
			delete m_hits[i];
		}
		m_hits.clear();
	}
	m_currentNode = NULL;
}

void CXChatBubbleBox::EnableNewMessageLocate(BOOL bEnable)
{
	m_enableNewMessageLocate = bEnable ? TRUE : FALSE;
	UpdateNewMessageState();
}

BOOL CXChatBubbleBox::IsEnableNewMessageLocate() const
{
	return m_enableNewMessageLocate;
}

void CXChatBubbleBox::SetInsertType(int nType)
{
	m_insertType = (nType == chat_insert_type_sender) ? chat_insert_type_sender : chat_insert_type_receiver;
}

int CXChatBubbleBox::GetInsertType() const
{
	return m_insertType;
}

CXChatBubbleBox::_xcchat_user_profile_* CXChatBubbleBox::_findUserProfile(const wchar_t* pSenderId, BOOL bCreate)
{
	// pSenderId == NULL 时回退到当前方向的 active senderId. 空字符串 ("" / 未设置) 也是合法
	// 的 key, 视为 "匿名发送者", 所有未指定 id 的调用都共享同一桶.
	CXText id;
	if (pSenderId) id = pSenderId;
	else id = (m_insertType == chat_insert_type_sender) ? m_activeSenderIdSender : m_activeSenderIdReceiver;
	for (size_t i = 0; i < m_userProfiles.size(); ++i){
		if (m_userProfiles[i].senderId == id) return &m_userProfiles[i];
	}
	if (!bCreate) return NULL;
	_xcchat_user_profile_ p;
	p.senderId = id;
	m_userProfiles.push_back(p);
	return &m_userProfiles.back();
}

void CXChatBubbleBox::SetInsertUserInfo(const wchar_t* pSenderId, const wchar_t* pSenderName, const wchar_t* pAvatarPath)
{
	// 按 senderId 维度更新用户资料, 同时把该 id 设为当前方向的 active 发送者. 不再按 sender/
	// receiver 方向分桶 — 同一个 id 在两个方向都用同一份资料 (实际场景: 一个用户不会同时既
	// 是发送者又是接收者, 切换方向只是切换 active id). 不会清空 tags, 多次设置只覆盖名称/
	// 头像两个字段.
	CXText id = pSenderId ? pSenderId : L"";
	// 兼容 "先 AddInsertTag 后 SetInsertUserInfo" 的常见调用顺序: 这种写法下旧的 active 还是
	// 默认值 "", AddInsertTag(NULL,...) 把 tags 灌进了 "" profile. 这里把 "" profile 上的 tags
	// 整体迁移到新 id 上 (前提: 新 id != "" 且新 id profile 还没自己的 tags), 之后清空 "" 桶,
	// 避免下一个用户继续吸收上一个的残留. 不动 name / avatar — 那两个字段以本次入参为准.
	CXText oldActive = (m_insertType == chat_insert_type_sender) ? m_activeSenderIdSender : m_activeSenderIdReceiver;
	if (!id.empty() && oldActive.empty()){
		_xcchat_user_profile_* scratch = _findUserProfile(L"", FALSE);
		if (scratch && scratch->tags.size() > 0){
			// 把 scratch 上的 tags 先值拷贝出来再去查 / 创建 dst: _findUserProfile 内部
			// push_back 会让 scratch 这个 vector 元素指针失效, 之后 scratch->tags 是 UAF.
			CXVector<CXText> migrated = scratch->tags;
			BOOL bConsumed = scratch->bConsumed;
			scratch = NULL;   // 防止后面误用
			_xcchat_user_profile_* dst = _findUserProfile(id.getPtr(), TRUE);
			if (dst && dst->tags.size() == 0){
				dst->tags = migrated;
				dst->bConsumed = bConsumed;
				// 迁移完, 重新查回 "" 清掉 (此时 vector 可能已扩容, 必须重新查)
				_xcchat_user_profile_* scratch2 = _findUserProfile(L"", FALSE);
				if (scratch2){
					scratch2->tags.resize(0);
					scratch2->bConsumed = FALSE;
				}
			}
		}
	}
	_xcchat_user_profile_* p = _findUserProfile(id.getPtr(), TRUE);
	p->senderName = pSenderName ? pSenderName : L"";
	p->avatarPath = pAvatarPath ? pAvatarPath : L"";
	if (m_insertType == chat_insert_type_sender) m_activeSenderIdSender = id;
	else                                          m_activeSenderIdReceiver = id;
}

void CXChatBubbleBox::ClearInsertTags(const wchar_t* pSenderId)
{
	_xcchat_user_profile_* p = _findUserProfile(pSenderId, FALSE);
	if (p){
		p->tags.resize(0);
		p->bConsumed = FALSE;
	}
}

void CXChatBubbleBox::AddInsertTag(const wchar_t* pSenderId, const wchar_t* pTag)
{
	if (!pTag || !pTag[0]) return;
	_xcchat_user_profile_* p = _findUserProfile(pSenderId, TRUE);
	// 上一条气泡已经把 tags "消费" 走了, 调用者再次进入 AddInsertTag 视为"开启新一轮标签
	// 配置", 清空旧值再追加; 这样循环里 AddInsertTag*M + InsertBubble 不会无限累积.
	// 若调用方根本不再调 AddInsertTag, tags 维持原样, 后续 InsertBubble 共享同一份标签.
	if (p->bConsumed){
		p->tags.resize(0);
		p->bConsumed = FALSE;
	}
	p->tags.add(CXText(pTag));
}

void CXChatBubbleBox::AddInsertTag(const wchar_t* pTag)
{
	// 旧版接口: 走当前方向的 active senderId. 等价于 AddInsertTag(NULL, pTag).
	AddInsertTag((const wchar_t*)NULL, pTag);
}

BOOL CXChatBubbleBox::InsertBubbleBegin()
{
	if (!m_hEle || m_currentNode) return FALSE;
	_xcgui_chat_node_* p = new _xcgui_chat_node_;
	p->data.itemType = chat_item_type_bubble;
	p->data.insertType = m_insertType;
	{
		// 按当前方向的 active senderId 在 m_userProfiles 里取一份完整资料 (name/avatar/tags).
		// 若该 id 还没注册过 (用户没调过 SetInsertUserInfo 也没 AddInsertTag), profile 为 NULL,
		// 此时使用空 name + 空 avatar + 空 tags, 行为与旧实现一致.
		_xcchat_user_profile_* up = _findUserProfile(NULL, FALSE);
		p->data.senderId   = (m_insertType == chat_insert_type_sender) ? m_activeSenderIdSender : m_activeSenderIdReceiver;
		p->data.senderName = up ? up->senderName : CXText(L"");
		p->data.avatarPath = up ? up->avatarPath : CXText(L"");
		if (up){
			p->data.tags = up->tags;
			// 标记 tags 已消费: 下次 AddInsertTag 会清空并重新开始, 防止循环模式累积.
			up->bConsumed = TRUE;
		}
		else p->data.tags.resize(0);
	}
	p->index = (int)m_nodes.size();
	p->rowHeight = 0;
	p->hRow = p->hAvatar = p->hMeta = p->hName = p->hEdit = NULL;
	p->hCenterText = NULL;
	p->pEditDW = NULL;
	p->rcBubble.left = p->rcBubble.top = p->rcBubble.right = p->rcBubble.bottom = 0;
	m_nodes.push_back(p);
	m_currentNode = p;
	BuildNode(p);
	return TRUE;
}

BOOL CXChatBubbleBox::InsertBubbleBeginEx(const wchar_t* pSenderName, const wchar_t* pAvatarPath)
{
	// 临时覆盖当前方向 active 用户的 name + avatar, 仅作用于本次气泡; 调用结束后恢复
	// 原值, 不影响后续 InsertBubbleBegin / 其他用户的资料 / 该用户的 tags.
	_xcchat_user_profile_* up = _findUserProfile(NULL, TRUE);
	if (!up) return InsertBubbleBegin();
	CXText oldName   = up->senderName;
	CXText oldAvatar = up->avatarPath;
	up->senderName = pSenderName ? pSenderName : L"";
	up->avatarPath = pAvatarPath ? pAvatarPath : L"";
	BOOL ret = InsertBubbleBegin();
	// _findUserProfile 内部可能已 push_back, 这里用 senderId 重新定位 (push_back 后旧指针失效).
	_xcchat_user_profile_* up2 = _findUserProfile(NULL, FALSE);
	if (up2){
		up2->senderName = oldName;
		up2->avatarPath = oldAvatar;
	}
	return ret;
}

BOOL CXChatBubbleBox::InsertBubbleEnd()
{
	if (!m_currentNode) return FALSE;
	if (m_currentNode->pEditDW){
		// 只读 + 可获取焦点: CXEditDW 只读模式下仍允许选中 / Ctrl+C 复制 / 鼠标拖选, 仅禁掉
		// 编辑/剪切/粘贴/撤销. 这里之前调了 XEle_EnableFocus(FALSE) 会让鼠标按下被框架丢弃,
		// 用户无法在气泡里拖选文本 → 移除. 焦点矩形仍关掉避免视觉干扰.
		m_currentNode->pEditDW->EnableReadOnly(TRUE);
		if (m_currentNode->hEdit){
			XEle_EnableDrawFocus(m_currentNode->hEdit, FALSE);
		}
	}
	m_currentNode = NULL;
	// nearBottom 必须 RelayoutAll 前抓拍: SetTotalSize 会让 posY=0, 之后再判就永远不贴底了.
	const BOOL bNearBottom = CaptureNearBottom();
	if (m_hEle) XEle_AdjustLayout(m_hEle);
	RelayoutAll(TRUE);
	FinishAppendScrollPolicy(bNearBottom);
	return TRUE;
}

BOOL CXChatBubbleBox::InsertText(const wchar_t* pText)
{
	if (!m_currentNode || !pText) return FALSE;
	xcgui_chat_content_ c;
	c.type = chat_content_type_text;
	c.text = pText;
	m_currentNode->data.contents.add(c);
	AppendContentToEdit(m_currentNode, c);
	return TRUE;
}

BOOL CXChatBubbleBox::InsertImage(const wchar_t* pImagePath)
{
	return InsertPathContent(chat_content_type_image, pImagePath);
}

BOOL CXChatBubbleBox::InsertFile(const wchar_t* pFilePath)
{
	return InsertPathContent(chat_content_type_file, pFilePath);
}

BOOL CXChatBubbleBox::InsertVoice(const wchar_t* pVoicePath)
{
	return InsertPathContent(chat_content_type_voice, pVoicePath);
}

BOOL CXChatBubbleBox::InsertVideo(const wchar_t* pVideoPath)
{
	return InsertPathContent(chat_content_type_video, pVideoPath);
}

BOOL CXChatBubbleBox::InsertObject(HXCGUI hObject)
{
	return InsertObjectEx(hObject, NULL);
}

BOOL CXChatBubbleBox::InsertObjectEx(HXCGUI hObject, const wchar_t* pKey)
{
	if (!m_currentNode || !hObject) return FALSE;
	// CXEditDW 的 inline 对象布局假设对象是 *该 edit 元素的子元素* (PositionInlineObjects
	// 走 XEle_SetPosition 用 edit 局部坐标). 调用方常常用 chat 的 m_hEle 当父建对象, 不
	// reparent 就会落在 chat 区左上角 / 任意位置. 这里强制 AddChild 到 pNode->hEdit, 让
	// XCGUI 自动从旧父摘出再挂到 edit 下, 保证 inline 对象跟随气泡布局.
	if (m_currentNode->hEdit && XC_IsHELE(hObject)){
		// XEle_AddChild 不会自动从旧父摘除, 旧父非空时直接 AddChild 会报 "重复添加对象,
		// 该对象已经有父" — 这里先 Remove (无父时为 no-op).
		XEle_Remove((HELE)hObject);
		XEle_AddChild(m_currentNode->hEdit, hObject);
		// 事件统一接到聊天框的左/右键回调, 走 part = chat_click_part_object. 这样 UI 对象
		// 的点击不需要使用方再手动 RegEventCPP, 与 tag/avatar/name 一致.
		BindHit((HELE)hObject, m_currentNode, chat_click_part_object, -1);
		XEle_RegEventCPP1((HELE)hObject, XE_LBUTTONUP, &CXChatBubbleBox::OnLButtonUpImpl);
		XEle_RegEventCPP1((HELE)hObject, XE_RBUTTONUP, &CXChatBubbleBox::OnRButtonUpImpl);
	}
	xcgui_chat_content_ c;
	c.type    = chat_content_type_object;
	c.hObject = hObject;
	// 复用 text 字段保存调用方提供的标识键: object 类型的渲染路径不读 text (走 AddObject),
	// 所以 text 自由作为 key 持久化, 加载时原样回到 content.text 供回调使用.
	if (pKey) c.text = pKey;
	m_currentNode->data.contents.add(c);
	AppendContentToEdit(m_currentNode, c);
	return TRUE;
}

BOOL CXChatBubbleBox::InsertFromEditDW(CXEditDW* pSrc)
{
	if (!m_currentNode || !pSrc) return FALSE;
	// 语义: "把用户在编辑器里编辑好的一条消息发送进来", 期望必发必滚到底, 哪怕用户当前
	// 滚动条不在底部. 由 InsertBubbleEnd → FinishAppendScrollPolicy 消费此标志.
	m_forceScrollOnNextEnd = TRUE;
	// 用公开 API GetContents 取按 *出现顺序* 排列的原子序列 (text / image / object), \n
	// 已被切到独立 text 原子. \uFFFC 周围空文本被 editdw 主动抑制, 所以原子序列里不会出
	// 现 image/object 紧邻的空文本占位.
	CXVector<editdw_content_item_> items;
	if (!pSrc->GetContents(items)) return FALSE;
	const int n = items.size();
	// 关键: chat 的 InsertText / InsertImage / InsertObjectEx 各自走 AppendContentToEdit, 该
	// 函数为了表达 "新的一条 content = 新一行" 会在已有内容前自动 AddText("\n"). 如果对每个
	// editdw 原子都调一次 Insert*, "123/456/789" 三段会被插出 6 个多余 \n. 这里改成: 把连续
	// 的 text 原子合并成 *一段* 内嵌 \n 的文本, 用 *一次* InsertText 写入; image / object 各
	// 单独成一段 (天然就该独占一行, 让 AppendContentToEdit 的自动 \n 起作用是想要的).
	CXText pending;
	BOOL bHasPending = FALSE;
	auto flushPending = [&](){
		if (bHasPending){
			InsertText(pending.getPtr());
			pending = L"";
			bHasPending = FALSE;
		}
	};
	int prevType = -1;
	for (int i = 0; i < n; ++i){
		const editdw_content_item_& it = items[i];
		if (it.type == editdw_content_type_text){
			// 相邻两个 text 原子之间补回换行 (editdw 把 \n 切成独立原子, 空原子 = 空行).
			if (prevType == editdw_content_type_text){
				pending += L"\n";
				bHasPending = TRUE;
			}
			if (!it.text.empty()){
				pending += it.text;
				bHasPending = TRUE;
			}
			prevType = editdw_content_type_text;
		}
		else if (it.type == editdw_content_type_image){
			flushPending();
			// 图片用源记录的原始路径重新 InsertImage; 经过 SetImagePersistDir 的图都是稳
			// 定路径, 直接复用即可, 与序列化策略一致.
			if (!it.imagePath.empty()){
				InsertImage(it.imagePath.getPtr());
			}
			prevType = editdw_content_type_image;
		}
		else if (it.type == editdw_content_type_object){
			flushPending();
			// hObject 现属于源 edit, InsertObjectEx 内部 XEle_Remove + XEle_AddChild 把它
			// reparent 到本气泡的 edit, 同时绑定统一点击事件.
			if (it.hObject){
				InsertObjectEx(it.hObject, NULL);
			}
			prevType = editdw_content_type_object;
		}
	}
	flushPending();
	return TRUE;
}

BOOL CXChatBubbleBox::InsertPathContent(int nType, const wchar_t* pPath)
{
	if (!m_currentNode || !pPath || !pPath[0]) return FALSE;
	xcgui_chat_content_ c;
	c.type = nType;
	c.path = pPath;
	c.text = PathFileName(pPath);
	if (c.text.empty()) c.text = pPath;
	m_currentNode->data.contents.add(c);
	AppendContentToEdit(m_currentNode, c);
	return TRUE;
}

BOOL CXChatBubbleBox::InsertMessage(const wchar_t* pText)
{
	if (!m_hEle) return FALSE;
	_xcgui_chat_node_* p = new _xcgui_chat_node_;
	p->data.itemType = chat_item_type_system;
	p->data.displayText = pText ? pText : L"";
	p->index = (int)m_nodes.size();
	p->rowHeight = 0;
	p->hRow = p->hAvatar = p->hMeta = p->hName = p->hEdit = NULL;
	p->hCenterText = NULL;
	p->pEditDW = NULL;
	const BOOL bNearBottom = CaptureNearBottom();
	m_nodes.push_back(p);
	BuildNode(p);
	RelayoutAll(TRUE);
	FinishAppendScrollPolicy(bNearBottom);
	return TRUE;
}

BOOL CXChatBubbleBox::InsertClickableMessage(const wchar_t* pText)
{
	if (!m_hEle) return FALSE;
	_xcgui_chat_node_* p = new _xcgui_chat_node_;
	p->data.itemType = chat_item_type_clickable;
	p->data.displayText = pText ? pText : L"";
	p->index = (int)m_nodes.size();
	p->rowHeight = 0;
	p->hRow = p->hAvatar = p->hMeta = p->hName = p->hEdit = NULL;
	p->hCenterText = NULL;
	p->pEditDW = NULL;
	const BOOL bNearBottom = CaptureNearBottom();
	m_nodes.push_back(p);
	BuildNode(p);
	RelayoutAll(TRUE);
	FinishAppendScrollPolicy(bNearBottom);
	return TRUE;
}

void CXChatBubbleBox::BuildNode(_xcgui_chat_node_* pNode)
{
	if (!m_hEle || !pNode) return;
	pNode->hRow = XEle_Create(0, 0, GetViewClientWidth(), 40, m_hEle);
	_xcchat_prepare_ele(pNode->hRow);
	XEle_EnableDrawBorder(pNode->hRow, FALSE);
	// 每条消息独占一行: 在水平文档流中给每个 hRow 标记 EnableWrap=TRUE, 让 LayoutBox
	// 在该项之前强制换行 → 自上而下文档流的视觉效果, 同时保留 LayoutBox 自动测算位置
	// 与总滚动高度的能力 (我们不再手算 y / 不再 SetTotalSize).
	XWidget_LayoutItem_EnableWrap(pNode->hRow, TRUE);

	if (pNode->data.itemType == chat_item_type_bubble){
		pNode->hAvatar = XBtn_Create(0, 0, m_avatarSize, m_avatarSize, L"", pNode->hRow);
		if (pNode->hAvatar){
			_xcchat_prepare_ele(pNode->hAvatar);
			// 头像背景信息: 走 XCGUI 实际 BkInfo 序列化字符串.
			//   5:  填充对象 (ID=2), 9(R,R,R,R) 为 4 角圆角
			//   3:  图片对象 (ID=1), 我们随后通过 XBkObj_SetImage 注入头像 HIMAGE
			// 这样圆角裁剪由 XCGUI 内部处理, 不需要额外 XDraw_FillRoundRect 遮罩, 也不依赖
			// 发送/接收方气泡颜色.
			// XEle_SetBkInfo 中 9(...) 圆角值是物理像素 (XCGUI 不会按 DPI 自动缩放该字段),
			// 而 m_avatarSize 走 XEle_SetRectEx 是逻辑像素, 由 XCGUI 自动 *DPI/96 渲染. 100% DPI
			// 下 round=18 / size=36 → 物理 18px 圆角覆盖物理 36px 头像 = 圆; 150% DPI 下 size 渲染
			// 为物理 54px 但圆角仍是物理 18px → 圆角矩形. 这里把 round 也按 DPI 缩放.
			int scaledRound = m_avatarRound * _xcchat_get_dpi_percent(pNode->hAvatar) / 100;
			wchar_t bkInfo[256] = {0};
			swprintf_s(bkInfo, 256,
				L"{99:1.9.9;98:16(1,0)32(1,0)64(1,0);5:41(2)2(15)20(1)21(3)26(1)22(4294967295)23(255)40(1)9(%d,%d,%d,%d);3:41(1)2(15)4();}",
				scaledRound, scaledRound, scaledRound, scaledRound);
			XEle_SetBkInfo(pNode->hAvatar, bkInfo);
			XEle_EnableDrawBorder(pNode->hAvatar, FALSE);
			if (!pNode->data.avatarPath.empty()){
				HIMAGE hAvatar = XImage_LoadFile(pNode->data.avatarPath.getPtr());
				if (hAvatar){
					// XImage_LoadFile 默认开启自动销毁; XBkObj/元素销毁时会随之释放, 我们
					// 自己再走 hImages 列表 XImage_Release 会触发二次释放崩溃. 这里关掉自动
					// 销毁, 改由 ReleaseNodes 里 XImage_Release 统一释放, 生命周期与节点绑定.
					XImage_EnableAutoDestroy(hAvatar, FALSE);
					// 让 BkObj 内部按等比缩放绘制头像图片.
					XImage_SetDrawType(hAvatar, image_draw_type_fixed_ratio);
					HBKM hBkM = XEle_GetBkManager(pNode->hAvatar);
					if (hBkM){
						vint hImgObj = XBkM_GetObject(hBkM, 1);
						if (hImgObj) XBkObj_SetImage(hImgObj, hAvatar);
					}
					pNode->hImages.push_back(hAvatar);
				}
			}
			_xcchat_set_hand_cursor(pNode->hAvatar);
			BindHit(pNode->hAvatar, pNode, chat_click_part_avatar, -1);
			XEle_RegEventCPP1(pNode->hAvatar, XE_LBUTTONUP, &CXChatBubbleBox::OnLButtonUpImpl);
			XEle_RegEventCPP1(pNode->hAvatar, XE_RBUTTONUP, &CXChatBubbleBox::OnRButtonUpImpl);
		}

		// 名称 + 标签的水平布局容器: XLayout (横向 + auto-size 子项) 让 XCGUI 自己按文本内容
		// 量出按钮宽度, 不再手动估算 TextVisualWidth (字符宽度估算与 DirectWrite/GDI 真实
		// 字体度量差异大, 之前导致按钮宽度过宽 / 与标签间距 hard-to-tune). hMeta 自身在
		// RelayoutNode 中 SetRectEx 定位, 不影响头像/气泡布局.
		pNode->hMeta = XLayout_Create(0, 0, 100, m_metaHeight, pNode->hRow);
		if (pNode->hMeta){
			_xcchat_prepare_ele(pNode->hMeta);
			XLayoutBox_EnableHorizon(pNode->hMeta, TRUE);
			XLayoutBox_SetAlignV(pNode->hMeta, layout_align_center);
			// 名称按钮 ↔ 标签按钮 / 标签 ↔ 标签 之间留 4px 视觉间隙, 之前都贴在一起.
			XLayoutBox_SetSpace(pNode->hMeta, 4);
			// 发送方 meta 整体右对齐 (添加顺序 标签... → 名称, 视觉 [... | 标签 | 名称]);
			// 接收方 meta 整体左对齐 (添加顺序 名称 → 标签..., 视觉 [名称 | 标签 | ...]).
			BOOL bSenderBuild = pNode->data.insertType == chat_insert_type_sender;
			XLayoutBox_SetAlignH(pNode->hMeta, bSenderBuild ? layout_align_right : layout_align_left);

			COLORREF nameColor = bSenderBuild ? m_senderNameColor : m_receiverNameColor;
			COLORREF tagColor = bSenderBuild ? m_senderTagColor : m_receiverTagColor;
			HFONTX hNameFont = bSenderBuild ? m_senderNameFont : m_receiverNameFont;
			HFONTX hTagFont = bSenderBuild ? m_senderTagFont : m_receiverTagFont;

			auto buildName = [&]{
				pNode->hName = XBtn_Create(0, 0, 80, m_metaHeight, pNode->data.senderName.getPtr(), pNode->hMeta);
				if (!pNode->hName) return;
				_xcchat_prepare_ele(pNode->hName);
				XBtn_SetTextAlign(pNode->hName, textAlignFlag_center | textAlignFlag_vcenter);
				XEle_ClearBkInfo(pNode->hName);
				XEle_EnableDrawBorder(pNode->hName, FALSE);
				XEle_SetTextColor(pNode->hName, nameColor);
				if (hNameFont) XEle_SetFont(pNode->hName, hNameFont);
				// 名称为可点击元素 (chat_click_part_name), 设手型光标提示.
				_xcchat_set_hand_cursor(pNode->hName);
				// layout.width=auto / height=fill 与参考 XML 一致: 宽按文本真实宽自动量,
				// 高填满 meta 容器 (m_metaHeight).
				XWidget_LayoutItem_SetWidth(pNode->hName, layout_size_auto, 0);
				XWidget_LayoutItem_SetHeight(pNode->hName, layout_size_fill, 0);
				BindHit(pNode->hName, pNode, chat_click_part_name, -1);
				XEle_RegEventCPP1(pNode->hName, XE_LBUTTONUP, &CXChatBubbleBox::OnLButtonUpImpl);
				XEle_RegEventCPP1(pNode->hName, XE_RBUTTONUP, &CXChatBubbleBox::OnRButtonUpImpl);
			};
			auto buildTags = [&]{
				for (int i = 0; i < (int)pNode->data.tags.size(); ++i){
					HELE hTag = XBtn_Create(0, 0, 60, m_metaHeight, pNode->data.tags[i].getPtr(), pNode->hMeta);
					if (!hTag) continue;
					_xcchat_prepare_ele(hTag);
					XBtn_SetTextAlign(hTag, textAlignFlag_center | textAlignFlag_vcenter);
					XEle_SetTextColor(hTag, tagColor);
					if (hTagFont) XEle_SetFont(hTag, hTagFont);
					// 文本色三态共用 tagColor (sender/receiver 区分通过外层 SetTagColors 控制).
					_xcchat_apply_state_bk(hTag,
						m_tagFillLeave, m_tagFillStay, m_tagFillDown,
						tagColor, tagColor, tagColor,
						m_tagRound);
					XEle_EnableDrawBorder(hTag, FALSE);
					_xcchat_set_hand_cursor(hTag);
					XWidget_LayoutItem_SetWidth(hTag, layout_size_auto, 0);
					XWidget_LayoutItem_SetHeight(hTag, layout_size_fill, 0);
					BindHit(hTag, pNode, chat_click_part_tag, i);
					XEle_RegEventCPP1(hTag, XE_LBUTTONUP, &CXChatBubbleBox::OnLButtonUpImpl);
					XEle_RegEventCPP1(hTag, XE_RBUTTONUP, &CXChatBubbleBox::OnRButtonUpImpl);
					pNode->hTags.push_back(hTag);
				}
			};
			if (bSenderBuild){
				// 发送方: 标签先, 名称后 → 整体右对齐后视觉上 [...| 标签 | 名称 | 头像]
				buildTags();
				buildName();
			}
			else{
				// 接收方: 名称先, 标签后 → 整体左对齐后视觉上 [头像 | 名称 | 标签 |...]
				buildName();
				buildTags();
			}
		}

		int initBubbleW = _xcchat_max(200, GetEffectiveBubbleMaxWidth(GetViewClientWidth()));
		pNode->pEditDW = new CXEditDW;
		pNode->hEdit = pNode->pEditDW->Create(0, 0, initBubbleW, 40, pNode->hRow);
		if (pNode->hEdit){
			pNode->pEditDW->EnableMultiLine(TRUE);
			pNode->pEditDW->EnableAutoWrap(TRUE);
			pNode->pEditDW->SetBorderSize(m_bubbleIndentation, m_bubbleIndentation, m_bubbleIndentation, m_bubbleIndentation);
			pNode->pEditDW->SetBkColor(RGBA(0, 0, 0, 0));
			// 应用气泡内默认文本颜色: 发送/接收方各自一份, 调用者可后续通过 SetBubbleTextColor
			// 修改; 富文本内部用 SetCurStyle 显式设置过的字符仍按各自颜色渲染.
			pNode->pEditDW->SetTextColor(pNode->data.insertType == chat_insert_type_sender ? m_senderTextColor : m_receiverTextColor);
			// 字号: 0 时不调用 (沿用 CXEditDW 默认 14pt).
			int bubbleFontPt = (pNode->data.insertType == chat_insert_type_sender)
				? m_senderBubbleFontSize : m_receiverBubbleFontSize;
			if (bubbleFontPt > 0) pNode->pEditDW->SetFontSize((float)bubbleFontPt);
			// 紧凑行高: DirectWrite 默认 line metric 会在每行底部累加 leading, 14pt 下视觉
			// 多出 ~5px 空白. 用 fontSize * 1.5 做 UNIFORM 行高, 14pt 得 21px (字符墨迹 +
			// 少量上下间隙), 与气泡视觉紧贴; 多行场景按同样比例缩, 末行不再塞 leading.
			pNode->pEditDW->SetLineSpacing(pNode->pEditDW->GetFontSize() * 1.5f);
			pNode->pEditDW->EnableDrawBorderEx(FALSE);
			_xcchat_prepare_ele(pNode->hEdit);
			XEle_EnableDrawFocus(pNode->hEdit, FALSE);
			XEle_EnableEvent_XE_MOUSEWHEEL(pNode->hEdit, FALSE);
			XSView_EnableAutoShowScrollBar(pNode->hEdit, FALSE);
			XSView_ShowSBarV(pNode->hEdit, FALSE);
			XSView_ShowSBarH(pNode->hEdit, FALSE);
			BindHit(pNode->hEdit, pNode, chat_click_part_bubble, -1);
			XEle_RegEventCPP1(pNode->hEdit, XE_PAINT, &CXChatBubbleBox::OnBubblePaintImpl);
			XEle_RegEventCPP1(pNode->hEdit, XE_LBUTTONUP, &CXChatBubbleBox::OnLButtonUpImpl);
			XEle_RegEventCPP1(pNode->hEdit, XE_RBUTTONUP, &CXChatBubbleBox::OnRButtonUpImpl);
		}
		else{
			delete pNode->pEditDW;
			pNode->pEditDW = NULL;
		}
	}
	else if (pNode->data.itemType == chat_item_type_system){
		XEle_EnableDrawBorder(pNode->hRow, FALSE);
		pNode->hCenterText = XShapeText_Create(0, 0, 240, 26, pNode->data.displayText.getPtr(), pNode->hRow);
		if (pNode->hCenterText){
			XShapeText_SetTextAlign(pNode->hCenterText, textAlignFlag_center | textAlignFlag_vcenter);
			XShapeText_SetTextColor(pNode->hCenterText, m_messageTextColor);
			if (m_messageFont) XShapeText_SetFont(pNode->hCenterText, m_messageFont);
		}
	}
	else{
		pNode->hCenterText = XBtn_Create(0, 0, 240, 26, pNode->data.displayText.getPtr(), pNode->hRow);
		if (pNode->hCenterText){
			HELE hCenterEle = (HELE)pNode->hCenterText;
			_xcchat_prepare_ele(hCenterEle);
			XBtn_SetTextAlign(hCenterEle, textAlignFlag_center | textAlignFlag_vcenter);
			if (m_clickableFont) XEle_SetFont(hCenterEle, m_clickableFont);
			_xcchat_apply_state_bk(hCenterEle,
				m_clickableFillLeave, m_clickableFillStay, m_clickableFillDown,
				m_clickableTextColor, m_clickableTextColor, m_clickableTextColor,
				m_clickableRound);
			XEle_EnableDrawBorder(hCenterEle, FALSE);
			_xcchat_set_hand_cursor(hCenterEle);
			BindHit(hCenterEle, pNode, chat_click_part_message, -1);
			XEle_RegEventCPP1(hCenterEle, XE_LBUTTONUP, &CXChatBubbleBox::OnLButtonUpImpl);
			XEle_RegEventCPP1(hCenterEle, XE_RBUTTONUP, &CXChatBubbleBox::OnRButtonUpImpl);
		}
	}
}

void CXChatBubbleBox::AppendContentToEdit(_xcgui_chat_node_* pNode, const xcgui_chat_content_& content)
{
	if (!pNode || !pNode->pEditDW) return;
	BOOL bReadOnly = pNode->pEditDW->IsReadOnly();
	if (bReadOnly) pNode->pEditDW->EnableReadOnly(FALSE);
	pNode->pEditDW->MoveEnd();
	if (!pNode->pEditDW->IsEmpty()){
		pNode->pEditDW->AddText(L"\n");
	}
	if (content.type == chat_content_type_text){
		pNode->pEditDW->AddText(content.text.getPtr());
	}
	else if (content.type == chat_content_type_image){
		if (!pNode->pEditDW->InsertImageThumb(content.path.getPtr())){
			pNode->pEditDW->AddText((CXText(L"[图片] ") + content.text).getPtr());
		}
	}
	else if (content.type == chat_content_type_file){
		pNode->pEditDW->AddText((CXText(L"[文件] ") + content.text).getPtr());
	}
	else if (content.type == chat_content_type_voice){
		pNode->pEditDW->AddText((CXText(L"[语音] ") + content.text).getPtr());
	}
	else if (content.type == chat_content_type_video){
		pNode->pEditDW->AddText((CXText(L"[视频] ") + content.text).getPtr());
	}
	else if (content.type == chat_content_type_object && content.hObject){
		pNode->pEditDW->AddObject(content.hObject);
	}
	if (bReadOnly) pNode->pEditDW->EnableReadOnly(TRUE);
}

void CXChatBubbleBox::SetBubbleMaxWidth(int nWidth)
{
	m_bubbleMaxWidth = nWidth < 0 ? 0 : nWidth;
	RelayoutAll(TRUE);
}

int CXChatBubbleBox::GetBubbleMaxWidth() const
{
	return m_bubbleMaxWidth;
}

void CXChatBubbleBox::SetBubbleIndentation(int nIndentation)
{
	m_bubbleIndentation = _xcchat_clamp(nIndentation, 0, 80);
	for (size_t i = 0; i < m_nodes.size(); ++i){
		if (m_nodes[i] && m_nodes[i]->pEditDW){
			m_nodes[i]->pEditDW->SetBorderSize(m_bubbleIndentation, m_bubbleIndentation, m_bubbleIndentation, m_bubbleIndentation);
		}
	}
	RelayoutAll(TRUE);
}

int CXChatBubbleBox::GetBubbleIndentation() const
{
	return m_bubbleIndentation;
}

void CXChatBubbleBox::SetAvatarSize(int nSize)
{
	m_avatarSize = _xcchat_clamp(nSize, 20, 96);
	RelayoutAll(TRUE);
}

void CXChatBubbleBox::SetMessageSpace(int nSpace)
{
	m_messageSpace = _xcchat_clamp(nSpace, 0, 80);
	RelayoutAll(TRUE);
}
int CXChatBubbleBox::GetMessageSpace() const { return m_messageSpace; }

void CXChatBubbleBox::SetContentRightPadding(int nPadding)
{
	m_contentRightPadding = _xcchat_clamp(nPadding, 0, 80);
	RelayoutAll(TRUE);
}
int CXChatBubbleBox::GetContentRightPadding() const { return m_contentRightPadding; }

void CXChatBubbleBox::SetBubbleColor(COLORREF senderColor, COLORREF receiverColor)
{
	m_senderBubbleColor = senderColor;
	m_receiverBubbleColor = receiverColor;
	if (m_hEle) XEle_Redraw(m_hEle, FALSE);
}

void CXChatBubbleBox::SetHintTextColor(COLORREF messageColor, COLORREF clickableColor)
{
	SetMessageTextColor(messageColor);
	SetClickableTextColor(clickableColor);
}

COLORREF CXChatBubbleBox::GetSenderBubbleColor() const { return m_senderBubbleColor; }
COLORREF CXChatBubbleBox::GetReceiverBubbleColor() const { return m_receiverBubbleColor; }

void CXChatBubbleBox::SetBubbleTextColor(COLORREF senderColor, COLORREF receiverColor)
{
	m_senderTextColor = senderColor;
	m_receiverTextColor = receiverColor;
	// 应用到所有已存在的气泡 CXEditDW 默认文本色; 富文本中已显式着色的字符不变.
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->pEditDW) continue;
		p->pEditDW->SetTextColor(p->data.insertType == chat_insert_type_sender ? m_senderTextColor : m_receiverTextColor);
		if (p->hEdit) XEle_Redraw(p->hEdit, FALSE);
	}
}

COLORREF CXChatBubbleBox::GetSenderBubbleTextColor() const { return m_senderTextColor; }
COLORREF CXChatBubbleBox::GetReceiverBubbleTextColor() const { return m_receiverTextColor; }

void CXChatBubbleBox::SetBubbleFontSize(int senderSize, int receiverSize)
{
	m_senderBubbleFontSize  = _xcchat_max(0, senderSize);
	m_receiverBubbleFontSize = _xcchat_max(0, receiverSize);
	// 应用到所有已存在的气泡 CXEditDW. 0 → 跳过 (保持原字号), 否则 SetFontSize 会重排文本.
	// 字号变化必然影响 wrap / 行数, 必须 RelayoutAll 重新量气泡.
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->pEditDW) continue;
		int pt = (p->data.insertType == chat_insert_type_sender)
			? m_senderBubbleFontSize : m_receiverBubbleFontSize;
		if (pt > 0) p->pEditDW->SetFontSize((float)pt);
		// 字号变了, 同步紧凑行高 (与 BuildNode 同公式 fontSize * 1.5).
		p->pEditDW->SetLineSpacing(p->pEditDW->GetFontSize() * 1.5f);
	}
	RelayoutAll(TRUE);
}
int CXChatBubbleBox::GetSenderBubbleFontSize() const { return m_senderBubbleFontSize; }
int CXChatBubbleBox::GetReceiverBubbleFontSize() const { return m_receiverBubbleFontSize; }

// 旧接口: 同时写发送方+接收方两套. 想分别控制改用 SetSenderBubbleRound* /
// SetReceiverBubbleRound*.
void CXChatBubbleBox::SetBubbleRound(int nRound)
{
	SetBubbleRoundEx(nRound, nRound, nRound, nRound);
}

void CXChatBubbleBox::SetBubbleRoundEx(int leftTop, int rightTop, int rightBottom, int leftBottom)
{
	SetSenderBubbleRoundEx(leftTop, rightTop, rightBottom, leftBottom);
	SetReceiverBubbleRoundEx(leftTop, rightTop, rightBottom, leftBottom);
}

int CXChatBubbleBox::GetBubbleRound() const { return m_senderBubbleRoundLT; }

void CXChatBubbleBox::GetBubbleRoundEx(int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom) const
{
	GetSenderBubbleRoundEx(pLeftTop, pRightTop, pRightBottom, pLeftBottom);
}

// 内部: 触发所有气泡重绘 (圆角变更后用).
static void _xcchat_redraw_bubbles(const std::vector<_xcgui_chat_node_*>& nodes)
{
	for (size_t i = 0; i < nodes.size(); ++i){
		_xcgui_chat_node_* p = nodes[i];
		if (p && p->hEdit) XEle_Redraw(p->hEdit, FALSE);
	}
}

void CXChatBubbleBox::SetSenderBubbleRound(int nRound)
{
	SetSenderBubbleRoundEx(nRound, nRound, nRound, nRound);
}

void CXChatBubbleBox::SetSenderBubbleRoundEx(int leftTop, int rightTop, int rightBottom, int leftBottom)
{
	m_senderBubbleRoundLT = _xcchat_max(0, leftTop);
	m_senderBubbleRoundRT = _xcchat_max(0, rightTop);
	m_senderBubbleRoundRB = _xcchat_max(0, rightBottom);
	m_senderBubbleRoundLB = _xcchat_max(0, leftBottom);
	_xcchat_redraw_bubbles(m_nodes);
}

int CXChatBubbleBox::GetSenderBubbleRound() const { return m_senderBubbleRoundLT; }

void CXChatBubbleBox::GetSenderBubbleRoundEx(int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom) const
{
	if (pLeftTop)     *pLeftTop     = m_senderBubbleRoundLT;
	if (pRightTop)    *pRightTop    = m_senderBubbleRoundRT;
	if (pRightBottom) *pRightBottom = m_senderBubbleRoundRB;
	if (pLeftBottom)  *pLeftBottom  = m_senderBubbleRoundLB;
}

void CXChatBubbleBox::SetReceiverBubbleRound(int nRound)
{
	SetReceiverBubbleRoundEx(nRound, nRound, nRound, nRound);
}

void CXChatBubbleBox::SetReceiverBubbleRoundEx(int leftTop, int rightTop, int rightBottom, int leftBottom)
{
	m_receiverBubbleRoundLT = _xcchat_max(0, leftTop);
	m_receiverBubbleRoundRT = _xcchat_max(0, rightTop);
	m_receiverBubbleRoundRB = _xcchat_max(0, rightBottom);
	m_receiverBubbleRoundLB = _xcchat_max(0, leftBottom);
	_xcchat_redraw_bubbles(m_nodes);
}

int CXChatBubbleBox::GetReceiverBubbleRound() const { return m_receiverBubbleRoundLT; }

void CXChatBubbleBox::GetReceiverBubbleRoundEx(int* pLeftTop, int* pRightTop, int* pRightBottom, int* pLeftBottom) const
{
	if (pLeftTop)     *pLeftTop     = m_receiverBubbleRoundLT;
	if (pRightTop)    *pRightTop    = m_receiverBubbleRoundRT;
	if (pRightBottom) *pRightBottom = m_receiverBubbleRoundRB;
	if (pLeftBottom)  *pLeftBottom  = m_receiverBubbleRoundLB;
}

void CXChatBubbleBox::SetAvatarRound(int nRound)
{
	m_avatarRound = _xcchat_max(0, nRound);
	// 同步刷新所有已存在头像 BkInfo 的 4 角圆角. XBkObj_SetRectRoundAngle 注意签名顺序为
	// (leftTop, leftBottom, rightTop, rightBottom), 与 XDraw_FillRoundRectEx 不同.
	// 与 BuildNode 一致, 这里也要按 DPI 把 round 缩放成物理像素.
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->hAvatar) continue;
		int scaledRound = m_avatarRound * _xcchat_get_dpi_percent(p->hAvatar) / 100;
		HBKM hBkM = XEle_GetBkManager(p->hAvatar);
		if (!hBkM) continue;
		vint hFillObj = XBkM_GetObject(hBkM, 2);
		if (hFillObj){
			XBkObj_SetRectRoundAngle(hFillObj, scaledRound, scaledRound, scaledRound, scaledRound);
		}
		XEle_Redraw(p->hAvatar, FALSE);
	}
}

int CXChatBubbleBox::GetAvatarRound() const { return m_avatarRound; }

void CXChatBubbleBox::SetMessageTextColor(COLORREF color)
{
	m_messageTextColor = color;
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->hCenterText) continue;
		if (p->data.itemType == chat_item_type_system){
			XShapeText_SetTextColor(p->hCenterText, m_messageTextColor);
		}
	}
}

COLORREF CXChatBubbleBox::GetMessageTextColor() const { return m_messageTextColor; }

void CXChatBubbleBox::SetClickableTextColor(COLORREF color)
{
	m_clickableTextColor = color;
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->hCenterText) continue;
		if (p->data.itemType == chat_item_type_clickable){
			XEle_SetTextColor((HELE)p->hCenterText, m_clickableTextColor);
		}
	}
}

COLORREF CXChatBubbleBox::GetClickableTextColor() const { return m_clickableTextColor; }

void CXChatBubbleBox::SetNameTextColor(COLORREF senderColor, COLORREF receiverColor)
{
	m_senderNameColor = senderColor;
	m_receiverNameColor = receiverColor;
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->hName) continue;
		XEle_SetTextColor(p->hName, p->data.insertType == chat_insert_type_sender ? m_senderNameColor : m_receiverNameColor);
		XEle_Redraw(p->hName, FALSE);
	}
}
COLORREF CXChatBubbleBox::GetSenderNameTextColor() const { return m_senderNameColor; }
COLORREF CXChatBubbleBox::GetReceiverNameTextColor() const { return m_receiverNameColor; }

void CXChatBubbleBox::SetTagTextColor(COLORREF senderColor, COLORREF receiverColor)
{
	m_senderTagColor = senderColor;
	m_receiverTagColor = receiverColor;
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p) continue;
		COLORREF c = p->data.insertType == chat_insert_type_sender ? m_senderTagColor : m_receiverTagColor;
		for (size_t k = 0; k < p->hTags.size(); ++k){
			if (!p->hTags[k]) continue;
			XEle_SetTextColor(p->hTags[k], c);
			XEle_Redraw(p->hTags[k], FALSE);
		}
	}
}
COLORREF CXChatBubbleBox::GetSenderTagTextColor() const { return m_senderTagColor; }
COLORREF CXChatBubbleBox::GetReceiverTagTextColor() const { return m_receiverTagColor; }

// 字体大小变更: 旧 HFONTX 显式 Release, 按新尺寸 XFont_Create 一份新的; size <= 0 视为
// 清回默认 (HFONTX = NULL, BuildNode 不会调 XEle_SetFont, 继承 XCGUI 默认字体).
static void _xcchat_update_font(HFONTX& slot, int& sizeSlot, int newSize)
{
	if (newSize < 0) newSize = 0;
	sizeSlot = newSize;
	if (slot){ XFont_Release(slot); slot = NULL; }
	if (newSize > 0){
		slot = XFont_Create(newSize);
		// XFont_Create 默认 EnableAutoDestroy=TRUE: XCGUI 关闭时会自己销毁所有未关掉自动
		// 销毁的字体. 而我们在 ~CXChatBubbleBox 里也调 XFont_Release, 等 XCGUI 已经销毁
		// 它后再 Release 就拿到无效句柄 -> 报错. 关掉自动销毁, 完全由本类管生命周期.
		if (slot) XFont_EnableAutoDestroy(slot, FALSE);
	}
}

void CXChatBubbleBox::SetNameFontSize(int senderSize, int receiverSize)
{
	_xcchat_update_font(m_senderNameFont, m_senderNameFontSize, senderSize);
	_xcchat_update_font(m_receiverNameFont, m_receiverNameFontSize, receiverSize);
	// 已存在的名称按钮同步换字体. 改完单个按钮文本宽变了, XLayout (auto width) 在下一次
	// 布局时会自动重排, 这里只对元素 SetFont + 触发重绘.
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->hName) continue;
		HFONTX f = p->data.insertType == chat_insert_type_sender ? m_senderNameFont : m_receiverNameFont;
		if (f) XEle_SetFont(p->hName, f);
		XEle_Redraw(p->hName, FALSE);
	}
	RelayoutAll(TRUE);
}
int CXChatBubbleBox::GetSenderNameFontSize() const { return m_senderNameFontSize; }
int CXChatBubbleBox::GetReceiverNameFontSize() const { return m_receiverNameFontSize; }

void CXChatBubbleBox::SetTagFontSize(int senderSize, int receiverSize)
{
	_xcchat_update_font(m_senderTagFont, m_senderTagFontSize, senderSize);
	_xcchat_update_font(m_receiverTagFont, m_receiverTagFontSize, receiverSize);
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p) continue;
		HFONTX f = p->data.insertType == chat_insert_type_sender ? m_senderTagFont : m_receiverTagFont;
		for (size_t k = 0; k < p->hTags.size(); ++k){
			if (!p->hTags[k]) continue;
			if (f) XEle_SetFont(p->hTags[k], f);
			XEle_Redraw(p->hTags[k], FALSE);
		}
	}
	RelayoutAll(TRUE);
}
int CXChatBubbleBox::GetSenderTagFontSize() const { return m_senderTagFontSize; }
int CXChatBubbleBox::GetReceiverTagFontSize() const { return m_receiverTagFontSize; }

HELE CXChatBubbleBox::GetNewMessageButton() const { return m_hNewMsgButton; }

void CXChatBubbleBox::SetMessageFontSize(int size)
{
	_xcchat_update_font(m_messageFont, m_messageFontSize, size);
	// 同步已存在的系统消息节点字体. XShapeText 使用 XShapeText_SetFont; 中心元素是 XBtn
	// (chat_item_type_clickable 路径) 不应受此影响, 故只处理 chat_item_type_system.
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->hCenterText) continue;
		if (p->data.itemType == chat_item_type_system){
			if (m_messageFont) XShapeText_SetFont(p->hCenterText, m_messageFont);
		}
	}
	RelayoutAll(TRUE);
}
int CXChatBubbleBox::GetMessageFontSize() const { return m_messageFontSize; }

void CXChatBubbleBox::SetClickableFontSize(int size)
{
	_xcchat_update_font(m_clickableFont, m_clickableFontSize, size);
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->hCenterText) continue;
		if (p->data.itemType == chat_item_type_clickable){
			if (m_clickableFont) XEle_SetFont((HELE)p->hCenterText, m_clickableFont);
			XEle_Redraw((HELE)p->hCenterText, FALSE);
		}
	}
	RelayoutAll(TRUE);
}
int CXChatBubbleBox::GetClickableFontSize() const { return m_clickableFontSize; }

void CXChatBubbleBox::SetTagBkStyle(COLORREF leaveFill, COLORREF stayFill, COLORREF downFill, int round)
{
	m_tagFillLeave = leaveFill;
	m_tagFillStay  = stayFill;
	m_tagFillDown  = downFill;
	m_tagRound = _xcchat_clamp(round, 0, 64);
	// 重新应用到现有标签按钮: 文本色仍走对应方向的 tagColor.
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p) continue;
		COLORREF tagColor = (p->data.insertType == chat_insert_type_sender)
			? m_senderTagColor : m_receiverTagColor;
		for (size_t k = 0; k < p->hTags.size(); ++k){
			if (!p->hTags[k]) continue;
			_xcchat_apply_state_bk(p->hTags[k],
				m_tagFillLeave, m_tagFillStay, m_tagFillDown,
				tagColor, tagColor, tagColor,
				m_tagRound);
			XEle_Redraw(p->hTags[k], FALSE);
		}
	}
}
void CXChatBubbleBox::GetTagBkStyle(COLORREF& leaveFill, COLORREF& stayFill, COLORREF& downFill, int& round) const
{
	leaveFill = m_tagFillLeave; stayFill = m_tagFillStay; downFill = m_tagFillDown; round = m_tagRound;
}

void CXChatBubbleBox::SetClickableBkStyle(COLORREF leaveFill, COLORREF stayFill, COLORREF downFill, int round)
{
	m_clickableFillLeave = leaveFill;
	m_clickableFillStay  = stayFill;
	m_clickableFillDown  = downFill;
	m_clickableRound = _xcchat_clamp(round, 0, 64);
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p || !p->hCenterText) continue;
		if (p->data.itemType != chat_item_type_clickable) continue;
		_xcchat_apply_state_bk((HELE)p->hCenterText,
			m_clickableFillLeave, m_clickableFillStay, m_clickableFillDown,
			m_clickableTextColor, m_clickableTextColor, m_clickableTextColor,
			m_clickableRound);
		XEle_Redraw((HELE)p->hCenterText, FALSE);
	}
}
void CXChatBubbleBox::GetClickableBkStyle(COLORREF& leaveFill, COLORREF& stayFill, COLORREF& downFill, int& round) const
{
	leaveFill = m_clickableFillLeave; stayFill = m_clickableFillStay; downFill = m_clickableFillDown; round = m_clickableRound;
}

BOOL CXChatBubbleBox::GetChatData(CXVector<xcgui_chat_item_>& out) const
{
	out.resize(0);
	for (size_t i = 0; i < m_nodes.size(); ++i){
		if (m_nodes[i]) out.add(m_nodes[i]->data);
	}
	return TRUE;
}

BOOL CXChatBubbleBox::SetChatData(const CXVector<xcgui_chat_item_>& data)
{
	if (!m_hEle) return FALSE;
	ClearMessages();
	for (int i = 0; i < (int)data.size(); ++i){
		const xcgui_chat_item_& item = data[i];
		if (item.itemType == chat_item_type_bubble){
			SetInsertType(item.insertType);
			SetInsertUserInfo(item.senderId.getPtr(), item.senderName.getPtr(), item.avatarPath.getPtr());
			// SetChatData 恢复路径: 把 item.tags 完整覆盖到该 senderId 的 profile.tags 上,
			// 之后 InsertBubbleBegin 按 active id 取出. 不影响其他用户的 tags.
			{
				_xcchat_user_profile_* up = _findUserProfile(item.senderId.getPtr(), TRUE);
				if (up) up->tags = item.tags;
			}
			if (!InsertBubbleBegin()) return FALSE;
			if (m_currentNode){
				m_currentNode->data.messageId = item.messageId;
				m_currentNode->data.timestamp = item.timestamp;
				m_currentNode->data.userData = item.userData;
			}
			for (int k = 0; k < (int)item.contents.size(); ++k){
				const xcgui_chat_content_& c = item.contents[k];
				if (m_currentNode){
					m_currentNode->data.contents.add(c);
					// object 类型: 复用 InsertObject 的 reparent + 事件绑定路径, 让加载/恢复
					// 出来的 UI 对象与运行时新建走同一行为. 这里直接 inline 同样的处理 (避免
					// InsertObject 再向 contents 二次 add).
					if (c.type == chat_content_type_object && c.hObject &&
					    m_currentNode->hEdit && XC_IsHELE(c.hObject)){
						// LoadFromMem 重建出来的对象父 = chat m_hEle, 直接 AddChild 会
						// 触发 "重复添加" 报错, 必须先 Remove 再 AddChild.
						XEle_Remove((HELE)c.hObject);
						XEle_AddChild(m_currentNode->hEdit, c.hObject);
						BindHit((HELE)c.hObject, m_currentNode, chat_click_part_object, -1);
						XEle_RegEventCPP1((HELE)c.hObject, XE_LBUTTONUP, &CXChatBubbleBox::OnLButtonUpImpl);
						XEle_RegEventCPP1((HELE)c.hObject, XE_RBUTTONUP, &CXChatBubbleBox::OnRButtonUpImpl);
					}
					AppendContentToEdit(m_currentNode, c);
					// 对象重建 + reparent + 默认事件挂好后回调一次, 让上层补样式 / 业务
					// 事件. text 字段持久化时携带 InsertObjectEx 的 pKey, 在此原样传出.
					if (m_objectLoadedEvent && c.type == chat_content_type_object && c.hObject){
						m_objectLoadedEvent(m_hEle, m_currentNode->index, k,
						                    (int)XC_GetObjectType(c.hObject),
						                    c.hObject,
						                    c.text.getPtr());
					}
				}
			}
			InsertBubbleEnd();
		}
		else if (item.itemType == chat_item_type_clickable){
			InsertClickableMessage(item.displayText.getPtr());
		}
		else{
			InsertMessage(item.displayText.getPtr());
		}
		if (!m_nodes.empty()){
			m_nodes.back()->data.messageId = item.messageId;
			m_nodes.back()->data.timestamp = item.timestamp;
			m_nodes.back()->data.userData = item.userData;
		}
	}
	// SetChatData 是 "全量回填": 用户期望加载完后默认停在最底部 (=最新一条) 视图.
	// 这里强制 AdjustLayout 后 ScrollBottom, 不走 FinishAppendScrollPolicy 的 nearBottom
	// 判断 (批量回填前 posY=0, 必定不算 nearBottom, 走 nearBottom 路径会变成显示 "新消息"
	// 提醒, 体验奇怪).
	XEle_AdjustLayout(m_hEle);
	RelayoutAll(TRUE);
	XSView_ScrollBottom(m_hEle);
	m_newMessageCount = 0;
	ShowNewMessageButton(FALSE);
	return TRUE;
}

// ============================================================================
//  序列化 / 反序列化
// ============================================================================
// 二进制格式 v1, little-endian:
//   Header (16 字节):
//     Magic       4B  "XCB1"
//     Version     2B  = 1
//     Flags       2B  = 0
//     ItemCount   4B  int32 - 消息数
//     Reserved    4B  = 0
//   Item[ItemCount] (变长):
//     itemType        4B  int32
//     insertType      4B  int32
//     timestamp       8B  int64
//     userData        8B  int64
//     senderId        wstr (uint32 wchar 数 + UTF-16)
//     senderName      wstr
//     avatarPath      wstr
//     messageId       wstr
//     displayText     wstr
//     tagCount        4B  int32
//     Tags[tagCount]: wstr
//     contentCount    4B  int32
//     Contents[contentCount]:
//       type          4B  int32  (xcgui_chat_content_type_)
//       text          wstr
//       path          wstr
//       -- v2 追加 (仅 type == chat_content_type_object 时) --
//       objType       4B  int32  (XC_OBJECT_TYPE)
//       width         4B  int32
//       height        4B  int32
//       textColor     4B  uint32
//       btnText       wstr (BUTTON / TEXTLINK / SHAPE_TEXT 的子文本; 其它类型空)
//       range         4B  int32  (SLIDERBAR / PROGRESSBAR 用; 其它 0)
//       pos           4B  int32
// 注意: 仅 v1 文件加载后 object 类内容 hObject = NULL. SHAPE_PICTURE 因 HIMAGE 无导出
// 接口, 重建为同尺寸空 shape (保住占位排版).

static const char     kXcbMagic[4] = {'X','C','B','1'};
// v1: 仅文本/路径数据; v2: 在 v1 基础上, 当 content.type == chat_content_type_object
// 时追加 UI 对象描述符 (objType + w/h + textColor + 子文本 + range/pos), 加载时按描述符
// 重建 XBtn / XTextLink / XEle / XSliderBar / XProgressBar / XShapeText / XShapePic.
// 仍支持读 v1 (加载后 hObject 为 NULL).
static const uint16_t kXcbVersion  = 2;
// 单消息字段数量极限 (防御坏数据 / 误传二进制): 标签 / 内容上限.
static const int      kXcbMaxTagCount     = 1024;
static const int      kXcbMaxContentCount = 65536;
static const uint32_t kXcbMaxWStrLen      = 16 * 1024 * 1024; // 16M wchar 上限

namespace {
struct CbSerCursor{
	std::vector<BYTE> buf;
	void writeBytes(const void* p, size_t n){
		if (n == 0) return;
		const BYTE* b = (const BYTE*)p;
		buf.insert(buf.end(), b, b + n);
	}
	void writeU16(uint16_t v){ writeBytes(&v, 2); }
	void writeU32(uint32_t v){ writeBytes(&v, 4); }
	void writeI32(int32_t  v){ writeBytes(&v, 4); }
	void writeI64(int64_t  v){ writeBytes(&v, 8); }
	void writeWStr(const wchar_t* p){
		uint32_t n = p ? (uint32_t)wcslen(p) : 0;
		writeU32(n);
		if (n) writeBytes(p, (size_t)n * sizeof(wchar_t));
	}
};

struct CbDeserCursor{
	const BYTE* p;
	size_t      remain;
	bool        ok;
	CbDeserCursor(const void* data, size_t size) : p((const BYTE*)data), remain(size), ok(true) {}
	bool readBytes(void* dst, size_t n){
		if (!ok || n > remain){ ok = false; return false; }
		memcpy(dst, p, n); p += n; remain -= n;
		return true;
	}
	uint16_t readU16(){ uint16_t v = 0; readBytes(&v, 2); return v; }
	uint32_t readU32(){ uint32_t v = 0; readBytes(&v, 4); return v; }
	int32_t  readI32(){ int32_t  v = 0; readBytes(&v, 4); return v; }
	int64_t  readI64(){ int64_t  v = 0; readBytes(&v, 8); return v; }
	void readWStr(CXText& s){
		s = L"";
		if (!ok) return;
		uint32_t n = readU32();
		if (!ok) return;
		if (n == 0) return;
		// 防御: 长度上限 + 不能超过剩余 buffer
		if (n > kXcbMaxWStrLen || (size_t)n * sizeof(wchar_t) > remain){ ok = false; return; }
		std::vector<wchar_t> tmp((size_t)n + 1, L'\0');
		readBytes(tmp.data(), (size_t)n * sizeof(wchar_t));
		if (!ok) return;
		tmp[n] = L'\0';
		s = tmp.data();
	}
};
// v2 UI 对象描述符: 提取 / 重建. 与 CXEditDW 的 kind=2 分支语义一致, 写在 chat 层避免
// 触碰 editdw 私有 API.
static void _WriteObjectDesc(CbSerCursor& w, HXCGUI h)
{
	XC_OBJECT_TYPE t = h ? XC_GetObjectType(h) : (XC_OBJECT_TYPE)0;
	const bool isEle   = h && !!XC_IsHELE(h);
	const bool isShape = h && !isEle && !!XC_IsShape(h);
	w.writeI32((int32_t)t);
	int wv = 0, hv = 0;
	if      (isEle  ){ wv = XEle_GetWidth ((HELE)h); hv = XEle_GetHeight((HELE)h); }
	else if (isShape){ wv = XShape_GetWidth(h);      hv = XShape_GetHeight(h);     }
	w.writeI32(wv);
	w.writeI32(hv);
	uint32_t tc = 0;
	if      (isEle)              tc = (uint32_t)XEle_GetTextColor((HELE)h);
	else if (t == XC_SHAPE_TEXT) tc = (uint32_t)XShapeText_GetTextColor(h);
	w.writeU32(tc);
	const wchar_t* pTxt = NULL;
	if      (t == XC_SHAPE_TEXT)                 pTxt = XShapeText_GetText(h);
	else if (t == XC_BUTTON || t == XC_TEXTLINK) pTxt = XBtn_GetText((HELE)h);
	w.writeWStr(pTxt);
	int32_t range = 0, pos = 0;
	if      (t == XC_SLIDERBAR  ){ range = XSliderBar_GetRange((HELE)h); pos = XSliderBar_GetPos((HELE)h); }
	else if (t == XC_PROGRESSBAR){ range = XProgBar_GetRange  ((HELE)h); pos = XProgBar_GetPos  ((HELE)h); }
	w.writeI32(range);
	w.writeI32(pos);
}

// 重建 UI 对象 (新建为 hParent 的子, chat 后续 InsertObject reparent 到 bubble edit).
// 不支持的类型 / 0 句柄 → 返回 NULL, 调用方按 hObject=NULL 处理 (content 仍保留 type=object,
// 渲染端 editdw AddObject(NULL) 时不入文本).
static HXCGUI _ReadObjectDesc(CbDeserCursor& r, HXCGUI hParent)
{
	int32_t  objType = r.readI32();
	int32_t  w_      = r.readI32();
	int32_t  h_      = r.readI32();
	uint32_t tc      = r.readU32();
	CXText   txt;    r.readWStr(txt);
	int32_t  range   = r.readI32();
	int32_t  pos     = r.readI32();
	if (!r.ok || !hParent) return NULL;
	if (w_ <= 0) w_ = 20;
	if (h_ <= 0) h_ = 20;
	const wchar_t* pTxt = txt.getPtr() ? txt.getPtr() : L"";
	HXCGUI hNew = NULL;
	HELE   hEle = NULL;
	switch ((XC_OBJECT_TYPE)objType){
	case XC_SHAPE_TEXT:
		hNew = XShapeText_Create(0, 0, w_, h_, pTxt, hParent);
		if (hNew) XShapeText_SetTextColor(hNew, (COLORREF)tc);
		break;
	case XC_SHAPE_PICTURE:
		// 无 HIMAGE round-trip, 重建为同尺寸空 shape 占位.
		hNew = XShapePic_Create(0, 0, w_, h_, hParent);
		break;
	case XC_BUTTON:      hEle = XBtn_Create     (0, 0, w_, h_, pTxt, hParent); break;
	case XC_TEXTLINK:    hEle = XTextLink_Create(0, 0, w_, h_, pTxt, hParent); break;
	case XC_ELE:         hEle = XEle_Create     (0, 0, w_, h_,       hParent); break;
	case XC_SLIDERBAR:
		hEle = XSliderBar_Create(0, 0, w_, h_, hParent);
		if (hEle){ XSliderBar_SetRange(hEle, range); XSliderBar_SetPos(hEle, pos); }
		break;
	case XC_PROGRESSBAR:
		hEle = XProgBar_Create(0, 0, w_, h_, hParent);
		if (hEle){ XProgBar_SetRange (hEle, range); XProgBar_SetPos (hEle, pos); }
		break;
	default: break;
	}
	if (hEle){
		XEle_SetTextColor(hEle, (COLORREF)tc);
		hNew = (HXCGUI)hEle;
	}
	return hNew;
}

} // namespace

BOOL CXChatBubbleBox::SaveToMem(CXBytes& out) const
{
	out.clear();
	CbSerCursor w;
	// Header
	w.writeBytes(kXcbMagic, 4);
	w.writeU16(kXcbVersion);
	w.writeU16(0);                                // flags
	const int itemCount = (int)m_nodes.size();
	w.writeI32((int32_t)itemCount);
	w.writeU32(0);                                // reserved
	// Items
	for (int i = 0; i < itemCount; ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p) continue;
		const xcgui_chat_item_& it = p->data;
		w.writeI32((int32_t)it.itemType);
		w.writeI32((int32_t)it.insertType);
		w.writeI64((int64_t)it.timestamp);
		w.writeI64((int64_t)(intptr_t)it.userData);
		w.writeWStr(it.senderId.getPtr());
		w.writeWStr(it.senderName.getPtr());
		w.writeWStr(it.avatarPath.getPtr());
		w.writeWStr(it.messageId.getPtr());
		w.writeWStr(it.displayText.getPtr());
		// tags
		const int tagCount = (int)it.tags.size();
		w.writeI32((int32_t)tagCount);
		for (int t = 0; t < tagCount; ++t){
			w.writeWStr(it.tags[t].getPtr());
		}
		// contents
		const int contentCount = (int)it.contents.size();
		w.writeI32((int32_t)contentCount);
		for (int c = 0; c < contentCount; ++c){
			const xcgui_chat_content_& cc = it.contents[c];
			w.writeI32((int32_t)cc.type);
			w.writeWStr(cc.text.getPtr());
			w.writeWStr(cc.path.getPtr());
			// v2: object 子段
			if (cc.type == chat_content_type_object){
				_WriteObjectDesc(w, cc.hObject);
			}
		}
	}
	out.set(w.buf.empty() ? NULL : w.buf.data(), w.buf.size());
	return TRUE;
}

BOOL CXChatBubbleBox::LoadFromMem(const void* data, size_t size)
{
	if (!m_hEle || !data || size < 16) return FALSE;
	CbDeserCursor r(data, size);
	// Header
	char magic[4] = {0};
	r.readBytes(magic, 4);
	if (!r.ok || memcmp(magic, kXcbMagic, 4) != 0) return FALSE;
	uint16_t ver = r.readU16();
	r.readU16();                                  // flags (skip)
	int32_t itemCount = r.readI32();
	r.readU32();                                  // reserved
	// v1 兼容: 不带 object 描述符, 加载后 hObject = NULL.
	if (!r.ok || (ver != 1 && ver != 2) || itemCount < 0) return FALSE;
	const bool bHasObjectDesc = (ver >= 2);
	// 解析所有 item 到临时 CXVector, 成功后再一把灌进 SetChatData (复用现有重建路径).
	CXVector<xcgui_chat_item_> items;
	for (int i = 0; i < itemCount; ++i){
		xcgui_chat_item_ it;
		it.itemType   = (int)r.readI32();
		it.insertType = (int)r.readI32();
		it.timestamp  = (__int64)r.readI64();
		it.userData   = (vint)(intptr_t)r.readI64();
		r.readWStr(it.senderId);
		r.readWStr(it.senderName);
		r.readWStr(it.avatarPath);
		r.readWStr(it.messageId);
		r.readWStr(it.displayText);
		int32_t tagCount = r.readI32();
		if (!r.ok || tagCount < 0 || tagCount > kXcbMaxTagCount) return FALSE;
		for (int t = 0; t < tagCount; ++t){
			CXText tag;
			r.readWStr(tag);
			if (!r.ok) return FALSE;
			it.tags.add(tag);
		}
		int32_t contentCount = r.readI32();
		if (!r.ok || contentCount < 0 || contentCount > kXcbMaxContentCount) return FALSE;
		for (int c = 0; c < contentCount; ++c){
			xcgui_chat_content_ cc;
			cc.type    = (int)r.readI32();
			r.readWStr(cc.text);
			r.readWStr(cc.path);
			cc.hObject = NULL;
			if (!r.ok) return FALSE;
			// v2: object 类型读出描述符并按 chat 元素当父重建. 后续 SetChatData →
			// InsertObject 会把对象 reparent 到具体气泡的 edit 子层级.
			if (bHasObjectDesc && cc.type == chat_content_type_object){
				cc.hObject = _ReadObjectDesc(r, (HXCGUI)m_hEle);
				if (!r.ok) return FALSE;
			}
			it.contents.add(cc);
		}
		if (!r.ok) return FALSE;
		items.add(it);
	}
	if (!r.ok) return FALSE;
	return SetChatData(items);
}

BOOL CXChatBubbleBox::SaveToFile(const wchar_t* pPath) const
{
	if (!pPath || !*pPath) return FALSE;
	CXBytes bytes;
	if (!SaveToMem(bytes)) return FALSE;
	HANDLE h = ::CreateFileW(pPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
	                         FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return FALSE;
	DWORD written = 0;
	const BYTE* p = bytes.getPtr();
	DWORD n = (DWORD)bytes.getSize();
	BOOL  ok = TRUE;
	if (n > 0){
		if (!::WriteFile(h, p, n, &written, NULL) || written != n) ok = FALSE;
	}
	::CloseHandle(h);
	return ok;
}

BOOL CXChatBubbleBox::LoadFromFile(const wchar_t* pPath)
{
	if (!pPath || !*pPath || !m_hEle) return FALSE;
	HANDLE h = ::CreateFileW(pPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return FALSE;
	LARGE_INTEGER li; li.QuadPart = 0;
	// 上限 256MB, 防御坏数据 / 误传超大文件.
	if (!::GetFileSizeEx(h, &li) || li.QuadPart < 0 || li.QuadPart > (LONGLONG)(256ll * 1024 * 1024)){
		::CloseHandle(h);
		return FALSE;
	}
	std::vector<BYTE> buf((size_t)li.QuadPart);
	DWORD got = 0;
	BOOL  rd  = TRUE;
	if (!buf.empty()){
		rd = ::ReadFile(h, buf.data(), (DWORD)buf.size(), &got, NULL);
	}
	::CloseHandle(h);
	if (!rd || got != buf.size()) return FALSE;
	return LoadFromMem(buf.empty() ? NULL : buf.data(), buf.size());
}

HELE CXChatBubbleBox::GetCurrentEdit()
{
	return m_currentNode ? m_currentNode->hEdit : NULL;
}

HELE CXChatBubbleBox::GetItemEdit(int iItem)
{
	_xcgui_chat_node_* p = GetNode(iItem);
	return p ? p->hEdit : NULL;
}

int CXChatBubbleBox::GetNewMessageCount() const
{
	return m_newMessageCount;
}

void CXChatBubbleBox::ClearNewMessageCount()
{
	m_newMessageCount = 0;
	ShowNewMessageButton(FALSE);
}

void CXChatBubbleBox::LocateNewMessage()
{
	if (!m_hEle) return;
	XSView_ScrollBottom(m_hEle);
	ClearNewMessageCount();
}

void CXChatBubbleBox::SetLButtonClickEvent(xcgui_chat_click_event pFun)
{
	m_leftClickEvent = pFun;
}

void CXChatBubbleBox::SetRButtonClickEvent(xcgui_chat_click_event pFun)
{
	m_rightClickEvent = pFun;
}

void CXChatBubbleBox::SetObjectLoadedEvent(xcgui_chat_object_loaded_event pFun)
{
	m_objectLoadedEvent = pFun;
}

void CXChatBubbleBox::RelayoutAll(BOOL bRedraw)
{
	if (!m_hEle) return;
	// 在客户区宽里扣掉右内边距, 内容只在 [0, width - m_contentRightPadding) 区间布局, 留出
	// 与垂直滚动条的视觉间隔. m_contentRightPadding 默认 8, 通过 SetContentRightPadding 调整.
	int width = _xcchat_max(1, GetViewClientWidth() - m_contentRightPadding);
	int y = m_messageSpace;
	for (size_t i = 0; i < m_nodes.size(); ++i){
		_xcgui_chat_node_* p = m_nodes[i];
		if (!p) continue;
		p->index = (int)i;
		int h = 0;
		RelayoutNode(p, y, width, &h);
		y += h + m_messageSpace;
	}
	// 末尾再补一个 messageSpace 作底部留白, 防止 ScrollBottom 后最后一行紧贴视口底而被裁
	UpdateTotalSize(y + m_messageSpace);
	PositionNewMessageButton();
	if (bRedraw) XEle_Redraw(m_hEle, FALSE);
}

void CXChatBubbleBox::RelayoutNode(_xcgui_chat_node_* pNode, int y, int width, int* pOutHeight)
{
	if (!pNode || !pNode->hRow) return;
	if (width < 1) width = 1;
	if (pNode->data.itemType != chat_item_type_bubble){
		int textW = TextVisualWidth(pNode->data.displayText.getPtr(), NULL) + 28;
		textW = _xcchat_clamp(textW, 80, _xcchat_max(80, width - m_rowPadding * 2));
		int innerH = 26;
		int h = innerH + 8;
		int x = (width - textW) / 2;
		// 同 bubble 行: 只设 size, 不写 y, 让 XLayoutFrame 的 LayoutBox 自动堆栈.
		XEle_SetSize(pNode->hRow, width, h, FALSE);
		if (pNode->hCenterText){
			int innerY = (h - innerH) / 2;
			if (pNode->data.itemType == chat_item_type_system){
				RECT rcText = { x, innerY, x + textW, innerY + innerH };
				XShape_SetRect(pNode->hCenterText, &rcText);
			}
			else{
				XEle_SetRectEx((HELE)pNode->hCenterText, x, innerY, textW, innerH, FALSE);
			}
		}
		pNode->rowHeight = h;
		if (pOutHeight) *pOutHeight = h;
		return;
	}

	int maxBubble = GetEffectiveBubbleMaxWidth(width);
	int bubbleW = 0;
	int bubbleH = 0;
	// 无包裹气泡: 内边距清零, 让图片 / 控件直接贴边. 这里在 RelayoutNode 入口同步一次
	// SetBorderSize, 覆盖 BuildNode 时按 m_bubbleIndentation 设置的初值; 后续 SetBubble
	// Indentation 调整时也是经 RelayoutAll → RelayoutNode 走过来, 此分支会再次校正.
	const bool bBorderless = _xcchat_is_borderless_bubble(pNode);
	const int  indent      = bBorderless ? 0 : m_bubbleIndentation;
	if (pNode->pEditDW && bBorderless){
		pNode->pEditDW->SetBorderSize(0, 0, 0, 0);
	}
	else if (pNode->pEditDW){
		pNode->pEditDW->SetBorderSize(m_bubbleIndentation, m_bubbleIndentation, m_bubbleIndentation, m_bubbleIndentation);
	}
	if (pNode->pEditDW && pNode->hEdit){
		// 两段式测量:
		// (1) 先撑到一个非常大的宽度 (maxBubble * 4), CXEditDW 在该宽度上不换行,
		//     XSView_GetTotalSize 返回的就是富文本"自然宽"(最长视觉行的真实宽).
		//     直接用 maxBubble 测量会得到 widthIncludingTrailingWhitespace ≈ 视图宽,
		//     与实际可见文字不符, 气泡右侧出现明显空白.
		// (2) 自然宽 ≤ maxBubble - indent*2 → 直接用自然宽 (气泡紧贴最长行).
		// (3) 否则 → 设回 maxBubble 并 RelayoutNow, 此时 totalSize.cy 才是 wrap 后的总高,
		//     宽度采用 maxBubble (wrap 后视觉宽与约束宽接近, 误差忽略).
		int probeW = _xcchat_max(maxBubble * 4, 4000);
		XEle_SetRectEx(pNode->hEdit, 0, 0, probeW, _xcchat_max(34, m_lineHeight + indent * 2), FALSE);
		pNode->pEditDW->RelayoutNow();
		SIZE natural = {0, 0};
		XSView_GetTotalSize(pNode->hEdit, &natural);
		int maxContent = maxBubble - indent * 2;
		int contentW = 0;
		int contentH = 0;
		if (natural.cx <= maxContent){
			contentW = natural.cx;
			contentH = natural.cy;
		}
		else{
			// 自然宽超过 maxBubble → 必然换行. CXEditDW 内部 m_paraMaxWidth 取自 DirectWrite
			// 的 widthIncludingTrailingWhitespace, 在密集字符断行 (无空白可断点) 场景下经常
			// 返回约束宽本身, 而非真实最长行宽, 导致气泡贴到最大宽出现右侧大段空白.
			// 这里改用 "二分搜索最小可用宽度": 在保持 wrap 总高不变的前提下逐步收窄约束宽,
			// 收敛到的 hi 就是真实最长行宽 (再窄就会多产生一行 → 高度增加).
			XEle_SetRectEx(pNode->hEdit, 0, 0, maxBubble, _xcchat_max(34, natural.cy + indent * 2), FALSE);
			pNode->pEditDW->RelayoutNow();
			SIZE atMax = {0, 0};
			XSView_GetTotalSize(pNode->hEdit, &atMax);
			int targetH = atMax.cy;
			int lo = 20;            // 极小下界, 实际收敛后远大于这个值
			int hi = maxBubble;
			// 至多 ~log2(maxBubble) 步, maxBubble≈300 → 9 步 RelayoutNow; 单次 RelayoutNow
			// 在该编辑框 (单段短文本) 内 < 1ms, 总成本对气泡可接受.
			while (lo + 1 < hi){
				int mid = (lo + hi) / 2;
				int probeH = _xcchat_max(34, targetH + indent * 2);
				XEle_SetRectEx(pNode->hEdit, 0, 0, mid, probeH, FALSE);
				pNode->pEditDW->RelayoutNow();
				SIZE m = {0, 0};
				XSView_GetTotalSize(pNode->hEdit, &m);
				if (m.cy <= targetH) hi = mid; else lo = mid;
			}
			// 把编辑框 settle 到收敛宽 hi (取 hi 是因为 hi 保证 wrap 高 ≤ targetH; lo 会多一行).
			XEle_SetRectEx(pNode->hEdit, 0, 0, hi, _xcchat_max(34, targetH + indent * 2), FALSE);
			pNode->pEditDW->RelayoutNow();
			SIZE settled = {0, 0};
			XSView_GetTotalSize(pNode->hEdit, &settled);
			// settled.cx 现在等于真实最长行 widthIncludingTrailingWhitespace; 若仍贴到 hi, 说明
			// 它本身就是最长行宽 (而不是被约束撑大的伪值).
			contentW = _xcchat_min(_xcchat_max(settled.cx, hi - indent * 2), maxContent);
			contentH = settled.cy;
		}
		// + indent*2 才是编辑框元素自身需要的尺寸 (也是气泡显示尺寸).
		// 不再做行高量化 / leading 裁剪: 之前两种尝试都有副作用 — 量化到 m_lineHeight 倍
		// 会让单行底部多 5px, 强行减 leading 又会把多行末行下半截切掉 (DirectWrite 每行
		// 的 ascent/descent/leading 不能整体减一次).
		bubbleW = _xcchat_clamp(contentW + indent * 2, bBorderless ? 1 : 44, maxBubble);
		bubbleH = contentH + indent * 2;
	}
	else{
		SIZE bubbleSize = MeasureBubbleContent(pNode, maxBubble - indent * 2);
		bubbleW = _xcchat_clamp(bubbleSize.cx + indent * 2, bBorderless ? 1 : 44, maxBubble);
		bubbleH = _xcchat_max(m_lineHeight + indent * 2, bubbleSize.cy + indent * 2);
	}
	// meta 区宽度: 让 XCGUI 的 XLayout (auto-size 子项) 自己布置内部 name + tags;
	// 这里只决定 meta 容器的位置 / 总宽 / 内部水平对齐 (sender 右对齐 / receiver 左对齐).
	// 旧实现只用 bubbleW 当 metaW, 当气泡内容很短 (如 "你好") 时 bubbleW 仅 ~50px, 但
	// name+tags 自然宽 (例如 "群成员A ABC DBC") 远超过 50, hMeta LayoutBox 排不下 →
	// 后追加的 name (sender) / tags (receiver) 被裁掉, 视觉上随机丢字段. 这里改用
	// 'name + 所有 tag + spacing*(n+1)' 计算的自然宽与 bubbleW 取较大值, 保证 LayoutBox
	// 始终能容下全部子项.
	int contentW = bubbleW;
	int natMeta = 0;
	if (pNode->hMeta){
		const int kMetaSpace = 4;   // 与 BuildNode 中 XLayoutBox_SetSpace 一致.
		int n = (int)pNode->data.tags.size();
		if (!pNode->data.senderName.empty()){
			natMeta += TextVisualWidth(pNode->data.senderName.getPtr(), NULL) + 12;
		}
		for (int i = 0; i < n; ++i){
			natMeta += TagWidth(pNode->data.tags[i].getPtr());
		}
		// 间隙: name + n 个 tag 之间共 (childCount-1) 个 space, 留 1 个余量
		int childCount = (pNode->data.senderName.empty() ? 0 : 1) + n;
		if (childCount > 1) natMeta += kMetaSpace * (childCount - 1);
		// 留 4px 余量, 避免 LayoutBox 边界紧贴最后一个子项时被四舍五入截掉.
		natMeta += 4;
	}
	int maxMeta = _xcchat_max(80, width - m_avatarSize - m_rowPadding * 4);
	int metaW = _xcchat_min(_xcchat_max(_xcchat_max(contentW, natMeta), m_avatarSize * 2), maxMeta);
	// hMeta(名称+标签) 与 bubble 之间留 4px 视觉间隙, 之前 bubble 直接贴到 hMeta 底部, 视觉
	// 上文字与气泡边缘几乎相连; 与气泡 bottom 之间也留 4px (rowH 末尾的 + 4 已存在).
	const int kMetaBubbleGap = 4;
	// 每条气泡顶部额外留 12px 空白, 与 LayoutBox 自身 m_messageSpace 叠加, 让相邻气泡
	// 视觉上更宽松 (用户要求). 系统消息 / 可点击消息分支不加, 保持紧凑.
	const int kBubblePreGap = 12;
	int rowH = _xcchat_max(m_avatarSize, m_metaHeight + kMetaBubbleGap + bubbleH) + 4 + kBubblePreGap;
	int leftX = m_rowPadding;
	int rightX = width - m_rowPadding - m_avatarSize;
	BOOL bSender = pNode->data.insertType == chat_insert_type_sender;
	int avatarX = bSender ? rightX : leftX;
	int contentX = bSender ? avatarX - 8 - contentW : avatarX + m_avatarSize + 8;
	int bubbleX = bSender ? contentX + contentW - bubbleW : contentX;
	int metaX = bSender ? contentX + contentW - metaW : contentX;

	// 不写入 y: m_hEle 是 XLayoutFrame, 它启用了 auto-layout (XLayoutBox 垂直栈) 自己负责
	// 把每个 hRow 按顺序贴上去 + 用 XLayoutBox_SetSpace 间隔. 我们再写 y 会和 LayoutBox
	// 竞争, 在 LayoutBox 异步重排后被覆盖, 导致 "首条消息 y 偏移 / 顶部空白" 的随机现象.
	// 只设大小: LayoutBox 据此完成纵向堆叠. 调用方 (RelayoutAll) 累加 y 仅供 UpdateTotalSize
	// 计算总滚动高使用, 不再下发到子元素. 子元素 y 整体下移 kBubblePreGap, 把 12px 空白
	// 留在 hRow 顶部.
	XEle_SetSize(pNode->hRow, width, rowH, FALSE);
	if (pNode->hAvatar) XEle_SetRectEx(pNode->hAvatar, avatarX, kBubblePreGap, m_avatarSize, m_avatarSize, FALSE);
	if (pNode->hMeta) XEle_SetRectEx(pNode->hMeta, metaX, kBubblePreGap, metaW, m_metaHeight, FALSE);
	if (pNode->hEdit){
		int bubbleY = kBubblePreGap + m_metaHeight + kMetaBubbleGap;
		XEle_SetRectEx(pNode->hEdit, bubbleX, bubbleY, bubbleW, bubbleH, TRUE);
		pNode->rcBubble.left = bubbleX;
		pNode->rcBubble.top = bubbleY;
		pNode->rcBubble.right = bubbleX + bubbleW;
		pNode->rcBubble.bottom = bubbleY + bubbleH;
	}
	pNode->rowHeight = rowH;
	if (pOutHeight) *pOutHeight = rowH;
}

SIZE CXChatBubbleBox::MeasureBubbleContent(const _xcgui_chat_node_* pNode, int maxContentWidth) const
{
	SIZE size = { 20, m_lineHeight };
	if (!pNode) return size;
	maxContentWidth = _xcchat_max(40, maxContentWidth);
	int totalH = 0;
	int maxW = 20;
	for (int i = 0; i < (int)pNode->data.contents.size(); ++i){
		const xcgui_chat_content_& c = pNode->data.contents[i];
		if (c.type == chat_content_type_image){
			// 仅当图片真能加载时按缩略图尺寸估算; 加载失败 (路径无效 / 文件不存在 等)
			// 时渲染端会回退为 "[图片] xxx" 纯文本 (见 AppendContentToEdit 的
			// InsertImageThumb 失败分支), 此处也必须按文本路径估算, 否则气泡按 112px
			// 占位但实际只渲染 ~26px 文本, 出现 "气泡塌陷" (内部大块留白) 现象.
			BOOL imgLoaded = FALSE;
			int iw = 160;
			int ih = 112;
			if (!c.path.empty()){
				HIMAGE hImg = XImage_LoadFile(c.path.getPtr());
				if (hImg){
					int ow = XImage_GetWidth(hImg);
					int oh = XImage_GetHeight(hImg);
					if (ow > 0 && oh > 0){
						// 关键: 必须与 CXEditDW::ComputeThumbSize 完全一致, 否则估算尺寸
						// 与编辑框实际渲染的缩略图尺寸不一致, 行高 / 气泡高 不够, 图片越过
						// 气泡底界画到下一条消息上.
						// 规则: 正方形 → 边长 ≤ 150; 非正方形 → 长边 ≤ 200; 原图小于阈值不放大.
						// 阈值与 CXEditDW 默认 m_imageThumbMaxLong=200 / m_imageThumbMaxSquare=150
						// 同步; 调用方若改过 SetImageThumbMaxSize, 这里需相应调整.
						const int kThumbMaxLong   = 200;
						const int kThumbMaxSquare = 150;
						int cap = (ow == oh) ? kThumbMaxSquare : kThumbMaxLong;
						int longEdge = ow > oh ? ow : oh;
						if (longEdge <= cap){
							iw = ow;
							ih = oh;
						}
						else{
							double scale = (double)cap / (double)longEdge;
							iw = (int)((double)ow * scale + 0.5);
							ih = (int)((double)oh * scale + 0.5);
						}
						if (iw < 1) iw = 1;
						if (ih < 1) ih = 1;
						imgLoaded = TRUE;
					}
					XImage_Release(hImg);
				}
			}
			if (imgLoaded){
				maxW = _xcchat_max(maxW, iw);
				totalH += ih;
			}
			else{
				// 与 AppendContentToEdit 失败回退一致: 文本 = "[图片] " + displayText
				CXText text = CXText(L"[图片] ") + c.text;
				int lines = 1;
				int tw = TextVisualWidth(text.getPtr(), &lines);
				int wrapLines = _xcchat_max(1, (tw + maxContentWidth - 1) / maxContentWidth);
				lines = _xcchat_max(lines, wrapLines);
				maxW = _xcchat_max(maxW, _xcchat_min(tw, maxContentWidth));
				totalH += lines * m_lineHeight;
			}
		}
		else{
			CXText text;
			if (c.type == chat_content_type_text) text = c.text;
			else if (c.type == chat_content_type_file) text = CXText(L"[文件] ") + c.text;
			else if (c.type == chat_content_type_voice) text = CXText(L"[语音] ") + c.text;
			else if (c.type == chat_content_type_video) text = CXText(L"[视频] ") + c.text;
			else text = L"[对象]";
			int lines = 1;
			int tw = TextVisualWidth(text.getPtr(), &lines);
			int wrapLines = _xcchat_max(1, (tw + maxContentWidth - 1) / maxContentWidth);
			lines = _xcchat_max(lines, wrapLines);
			maxW = _xcchat_max(maxW, _xcchat_min(tw, maxContentWidth));
			totalH += lines * m_lineHeight;
		}
		if (i + 1 < (int)pNode->data.contents.size()) totalH += 4;
	}
	if (pNode->data.contents.size() == 0) totalH = m_lineHeight;
	size.cx = _xcchat_clamp(maxW, 20, maxContentWidth);
	size.cy = _xcchat_max(totalH, m_lineHeight);
	return size;
}

void CXChatBubbleBox::UpdateTotalSize(int /*totalHeight*/)
{
	if (!m_hEle) return;
	// XLayoutFrame (auto-layout 开启) 会根据子项 layout-item 大小 + LayoutBox padding/space
	// 自己计算并维护 XSView 的 totalSize, 我们再手动 XSView_SetTotalSize(我们自己累加的 y)
	// 会与 LayoutBox 的真实内容尺寸不一致 (我们算的 y 比 LayoutBox 实际堆栈结果偏大),
	// 滚动条按我们的总高显示, 但 LayoutBox 只把行布到自己算的位置 → 末尾出现一大块空白
	// + 即使内容能完整放下也出现滚动条. 把这里降级为 no-op, 总高完全交给 LayoutFrame.
	// 调用方 (RelayoutAll) 仍累加 y 但不会下发, 保留累加的目的是后续诊断 / 备用.
}

void CXChatBubbleBox::UpdateNewMessageState()
{
	if (!m_hEle) return;
	if (!m_enableNewMessageLocate || m_newMessageCount <= 0){
		ShowNewMessageButton(FALSE);
		return;
	}
	SIZE total = { 0, 0 };
	XSView_GetTotalSize(m_hEle, &total);
	int viewH = GetViewClientHeight();
	int posY = XSView_GetViewPosV(m_hEle);
	BOOL nearBottom = (posY + viewH >= total.cy - 300);
	ShowNewMessageButton(nearBottom ? FALSE : TRUE);
}

void CXChatBubbleBox::PositionNewMessageButton()
{
	if (!m_hNewMsgButton || !m_hEle) return;
	int w = GetViewClientWidth();
	int h = GetViewClientHeight();
	XEle_SetRectEx(m_hNewMsgButton, _xcchat_max(0, w - 132), _xcchat_max(0, h - 52), 116, 34, FALSE);
}

void CXChatBubbleBox::ShowNewMessageButton(BOOL bShow)
{
	if (!m_hNewMsgButton) return;
	if (bShow){
		CXText text = L"新消息 ";
		text += CXText(m_newMessageCount);
		XBtn_SetText(m_hNewMsgButton, text.getPtr());
		PositionNewMessageButton();
	}
	XWidget_Show(m_hNewMsgButton, bShow);
}

BOOL CXChatBubbleBox::CaptureNearBottom() const
{
	// 必须在 RelayoutAll 之前调用. 第一条消息时 total < viewH, posY=0, 这里也算 "贴底"
	// (true), 让首次插入触发自动滚到底; 否则用户初次发消息会看到 "新消息" 提示按钮而不是
	// 直接滚到底, 不符合 IM 习惯. 距底 30px 的容差用于鼠标拖滚动条快到底但没完全贴边的场景.
	if (!m_hEle) return TRUE;
	SIZE total = { 0, 0 };
	XSView_GetTotalSize(m_hEle, &total);
	int viewH = const_cast<CXChatBubbleBox*>(this)->GetViewClientHeight();
	int posY  = XSView_GetViewPosV(m_hEle);
	if (total.cy <= viewH) return TRUE;
	return (posY + viewH >= total.cy - 30) ? TRUE : FALSE;
}

void CXChatBubbleBox::FinishAppendScrollPolicy(BOOL bWasNearBottom)
{
	if (!m_hEle) return;
	// 关键: nearBottom 必须在 RelayoutAll → UpdateTotalSize → XSView_SetTotalSize *之前*
	// 抓拍 (XSView 在 totalSize 变化时会把 posY 重置到 0, 这里再读 posY 永远是 0, 纯文本
	// 触底新消息再也不会自动跟随到底). 所以 bWasNearBottom 由调用方在 RelayoutAll 之前
	// 算好传入, 这里只负责执行 "贴底滚动 vs 累加新消息计数" 两条策略分支.
	// 一次性 force-scroll: InsertFromEditDW 把"用户主动发送/插入" 这种语义化场景标记为
	// 必须滚到底, 不受 nearBottom 判定影响. 用完即清.
	const BOOL bForce = m_forceScrollOnNextEnd;
	m_forceScrollOnNextEnd = FALSE;
	if (bForce || !m_enableNewMessageLocate || bWasNearBottom){
		// 在 ScrollBottom 之前 flush 一次布局: LayoutFrame 在我们 RelayoutAll 改动 hRow 尺寸
		// 之后, 内部 totalSize 可能尚未提交; XSView_ScrollBottom 用的是 XSView 当前 totalSize,
		// 没 flush 就会按旧总高滚, 用户看到末尾只露第一行一小截被裁掉.
		XEle_AdjustLayout(m_hEle);
		XSView_ScrollBottom(m_hEle);
		m_newMessageCount = 0;
		ShowNewMessageButton(FALSE);
	}
	else{
		m_newMessageCount++;
		UpdateNewMessageState();
	}
}

int CXChatBubbleBox::OnSizeImpl(HELE, int, UINT, BOOL*)
{
	// 重入哨兵: 我们注册了 XE_ADJUSTLAYOUT_END 走到这里, 而 XEle_AdjustLayout 会再次
	// 触发 XE_ADJUSTLAYOUT_END → 直接 return 防止无限递归 (0xc00000fd 栈溢出).
	if (m_inSizeImpl) return 0;
	m_inSizeImpl = TRUE;
	// 经过实测: XE_SIZE / XE_ADJUSTLAYOUT_END 触发时 XSView_GetViewWidth/Height 读到的
	// 仍可能是 *上一帧* 的视口尺寸 (XSView 内部视口在父布局完成后才更新, 而事件就是
	// 在父布局完成的同一调度链上抛的, XEle_AdjustLayout(m_hEle) 此时是 no-op 救不回).
	// 为彻底摆脱这种时序依赖, 这里改用 *元素自身外部 rect* + *扣除垂直滚动条占位* 的
	// 算法在 GetViewClientWidth / GetViewClientHeight 内部直接计算, 不再读 XSView 缓存.
	// 因此这里也不再调 XEle_AdjustLayout, 减少多余 layout 抖动.
	// 保留滚动位置: 之前 RelayoutAll → UpdateTotalSize → XSView_SetTotalSize 会把当前
	// 滚动位置归零 (XSView 在 totalSize 变化时重置 posY 到 0), 用户调整窗口后聊天框
	// 会突然跳回最顶部. 这里在重排前记录 "底部锚定" 状态 + 旧 posY, 重排后按需恢复:
	//   - 之前贴底 → 滚到新底部 (内容增减后视觉上仍贴底)
	//   - 否则 → 还原原 posY (clamp 到合法范围)
	BOOL bWasBottom = FALSE;
	int  oldPosY   = 0;
	if (m_hEle){
		SIZE total = { 0, 0 };
		XSView_GetTotalSize(m_hEle, &total);
		int viewH = GetViewClientHeight();
		oldPosY   = XSView_GetViewPosV(m_hEle);
		bWasBottom = (oldPosY + viewH >= total.cy - 1);
	}
	RelayoutAll(FALSE);
	if (m_hEle){
		if (bWasBottom){
			XSView_ScrollBottom(m_hEle);
		}
		else if (oldPosY > 0){
			SIZE total = { 0, 0 };
			XSView_GetTotalSize(m_hEle, &total);
			int viewH = GetViewClientHeight();
			int maxY  = _xcchat_max(0, total.cy - viewH);
			int newY  = _xcchat_clamp(oldPosY, 0, maxY);
			XSView_ScrollPosV(m_hEle, newY);
		}
	}
	UpdateNewMessageState();
	m_inSizeImpl = FALSE;
	return 0;
}

int CXChatBubbleBox::OnMouseWheelImpl(HELE, UINT, POINT*, BOOL*)
{
	UpdateNewMessageState();
	return 0;
}

int CXChatBubbleBox::OnDestroyEndImpl(HELE, BOOL*)
{
	m_hNewMsgButton = NULL;
	ReleaseNodes(FALSE);
	m_hEle = NULL;
	return 0;
}

int CXChatBubbleBox::OnBubblePaintImpl(HELE hEle, HDRAW hDraw, BOOL*)
{
	_xcgui_chat_hit_* hit = GetHit(hEle);
	if (!hit || !hit->pNode || !hDraw) return 0;
	// 纯图片 / UI 对象气泡: 不画背景, 让 inline 内容 (图 / 控件) 直接占满 edit 区域.
	if (_xcchat_is_borderless_bubble(hit->pNode)) return 0;
	RECT rc;
	XEle_GetClientRect(hEle, &rc);
	BOOL bSender = hit->pNode->data.insertType == chat_insert_type_sender;
	XDraw_SetBrushColor(hDraw, bSender ? m_senderBubbleColor : m_receiverBubbleColor);
	XDraw_EnableSmoothingMode(hDraw, TRUE);
	int rLT = bSender ? m_senderBubbleRoundLT : m_receiverBubbleRoundLT;
	int rRT = bSender ? m_senderBubbleRoundRT : m_receiverBubbleRoundRT;
	int rRB = bSender ? m_senderBubbleRoundRB : m_receiverBubbleRoundRB;
	int rLB = bSender ? m_senderBubbleRoundLB : m_receiverBubbleRoundLB;
	XDraw_FillRoundRectEx(hDraw, &rc, rLT, rRT, rRB, rLB);
	return 0;
}

int CXChatBubbleBox::OnLButtonUpImpl(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	_xcgui_chat_hit_* hit = GetHit(hEle);
	if (!hit) return 0;
	if (hit->part == chat_click_part_newMessage){
		LocateNewMessage();
		if (m_leftClickEvent) return m_leftClickEvent(m_hEle, -1, -1, hit->part, -1, hEle, pbHandled);
		return 1;
	}
	if (m_leftClickEvent && hit->pNode){
		return m_leftClickEvent(m_hEle, hit->pNode->index, hit->pNode->data.insertType, hit->part, hit->iTag, hEle, pbHandled);
	}
	return 0;
}

int CXChatBubbleBox::OnRButtonUpImpl(HELE hEle, UINT, POINT*, BOOL* pbHandled)
{
	_xcgui_chat_hit_* hit = GetHit(hEle);
	if (!hit || !hit->pNode) return 0;
	if (m_rightClickEvent){
		return m_rightClickEvent(m_hEle, hit->pNode->index, hit->pNode->data.insertType, hit->part, hit->iTag, hEle, pbHandled);
	}
	return 0;
}

void CXChatBubbleBox::BindHit(HELE hEle, _xcgui_chat_node_* pNode, int part, int iTag)
{
	if (!hEle) return;
	_xcgui_chat_hit_* hit = new _xcgui_chat_hit_;
	hit->hEle = hEle;
	hit->pNode = pNode;
	hit->part = part;
	hit->iTag = iTag;
	m_hits.push_back(hit);
}

_xcgui_chat_hit_* CXChatBubbleBox::GetHit(HELE hEle)
{
	for (size_t i = 0; i < m_hits.size(); ++i){
		if (m_hits[i] && m_hits[i]->hEle == hEle) return m_hits[i];
	}
	return NULL;
}

_xcgui_chat_node_* CXChatBubbleBox::GetNode(int iItem) const
{
	if (iItem < 0 || iItem >= (int)m_nodes.size()) return NULL;
	return m_nodes[(size_t)iItem];
}

int CXChatBubbleBox::GetViewClientWidth() const
{
	if (!m_hEle) return 1;
	int w = XSView_GetViewWidth(m_hEle);
	if (w <= 0) w = XEle_GetWidth(m_hEle);
	return _xcchat_max(1, w);
}

int CXChatBubbleBox::GetViewClientHeight() const
{
	if (!m_hEle) return 1;
	int h = XSView_GetViewHeight(m_hEle);
	if (h <= 0) h = XEle_GetHeight(m_hEle);
	return _xcchat_max(1, h);
}

int CXChatBubbleBox::GetEffectiveBubbleMaxWidth(int viewWidth) const
{
	if (m_bubbleMaxWidth > 0) return _xcchat_min(m_bubbleMaxWidth, _xcchat_max(80, viewWidth - m_avatarSize - m_rowPadding * 4));
	return _xcchat_max(120, viewWidth * 62 / 100);
}

int CXChatBubbleBox::TextVisualWidth(const wchar_t* pText, int* pLineCount) const
{
	if (pLineCount) *pLineCount = 1;
	if (!pText || !pText[0]) return 8;
	int line = 1;
	for (const wchar_t* p = pText; *p; ++p){
		if (*p == L'\n') line++;
	}
	if (pLineCount) *pLineCount = line;
	int cur = 0;
	int maxw = 0;
	for (const wchar_t* p = pText; *p; ++p){
		if (*p == L'\n'){
			maxw = _xcchat_max(maxw, cur);
			cur = 0;
			continue;
		}
		cur += (*p > 0xFF) ? 18 : 9;
	}
	maxw = _xcchat_max(maxw, cur);
	return _xcchat_max(8, maxw);
}

int CXChatBubbleBox::TagWidth(const wchar_t* pText) const
{
	// 标签按钮宽 = 文本宽 + 12 (左右各 6px 内边距, 与 BkInfo 圆角填充配合). 取消之前的
	// min=36 / +18 内边距 - 那个值会让 "VIP" 等短标签内左右白边过大, 与名称之间视觉间距
	// 超出用户期望 (用户要求名称↔标签的间距 ≈ 12px). 配合 kMetaGap=6 时实际视觉间距 ≈
	// 6 (按钮边距) + 6 (tag 内左边距) = 12.
	return TextVisualWidth(pText, NULL) + 12;
}

CXText CXChatBubbleBox::PathFileName(const wchar_t* pPath) const
{
	if (!pPath) return CXText(L"");
	const wchar_t* p1 = wcsrchr(pPath, L'\\');
	const wchar_t* p2 = wcsrchr(pPath, L'/');
	const wchar_t* p = p1 > p2 ? p1 : p2;
	return CXText(p ? p + 1 : pPath);
}
