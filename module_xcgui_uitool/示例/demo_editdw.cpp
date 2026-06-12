// CXEditDW 完整接口使用示例.
// 覆盖 module_xcgui_uitool.h 中 CXEditDW 全部 public 方法.
//
// 编译依赖: @依赖 module_xcgui_uitool.h + @src module_xcgui_uitool.cpp
//
// 用法: 在窗口创建后调用 BuildEditDWDemo(hWnd).

#include "module_xcgui_uitool.h"

static CXEditDW* g_edit = NULL;

// =============================================================================
// 1. 创建 + 字体 / 颜色 / 边框 / 模式
// =============================================================================

static void SetupEditStyle(CXEditDW& edit)
{
	edit.SetFontName(L"Segoe UI");
	edit.SetFontSize(14.0f);
	(void)edit.GetFontSize();
	edit.SetLineSpacing(22.0f);
	(void)edit.GetLineSpacing();

	edit.SetTextColor(RGBA(0x20, 0x21, 0x24, 0xFF));
	edit.SetSelectBkColor(RGBA(0xCC, 0xE5, 0xFF, 0xFF));
	edit.SetCaretColor(RGBA(0x00, 0x00, 0x00, 0xFF));
	edit.SetCaretWidth(2);
	(void)edit.GetCaretWidth();
	edit.SetBkColor(RGBA(0xFF, 0xFF, 0xFF, 0xFF));
	edit.SetBorderColor(RGBA(0xD1, 0xD5, 0xDB, 0xFF));
	edit.SetFocusBorderColor(RGBA(0x3B, 0x82, 0xF6, 0xFF));
	edit.SetHintColor(RGBA(0x9C, 0xA3, 0xAF, 0xFF));
	edit.SetBorderWidth(1);
	(void)edit.GetBorderWidth();
	edit.SetBorderSize(8, 6, 8, 6);
	edit.EnableDrawBorderEx(TRUE);
	(void)edit.IsDrawBorderEx();

	edit.SetHintText(L"请输入内容...");
	edit.SetDefaultText(L"默认文本");
	edit.SetDefaultTextColor(RGBA(0x9C, 0xA3, 0xAF, 0xFF));

	edit.EnableMultiLine(TRUE);
	(void)edit.IsMultiLine();
	edit.EnableAutoWrap(TRUE);
	(void)edit.IsAutoWrap();
	edit.EnableReadOnly(FALSE);
	(void)edit.IsReadOnly();
	edit.SetTextAlign(edit_textAlign_flag_left | edit_textAlign_flag_top);
	(void)edit.GetTextAlign();
	edit.EnableAutoSelAll(FALSE);
	edit.EnableAutoCancelSel(TRUE);

	edit.SetImageThumbMaxSize(200, 150);
	edit.SetMaxTextLength(5 * 1024 * 1024);
	(void)edit.GetMaxTextLength();

	CXEditDW::SetImagePersistPath(L"D:\\editdw\\images");
	(void)CXEditDW::GetImagePersistPath();
}

// =============================================================================
// 2. 文本 + 样式 + 对象
// =============================================================================

static void DemoEditTextAndStyle(CXEditDW& edit)
{
	edit.SetText(L"Hello DirectWrite \U0001F600");
	(void)edit.GetText();
	(void)edit.GetTextTemp();
	(void)edit.GetLength();
	(void)edit.IsEmpty();

	edit.AddText(L"\n追加一行");
	edit.InsertText(L" (插入)");

	int iRed = edit.AddStyleEx(L"Microsoft YaHei", 16, fontStyle_bold, RGBA(0xE5, 0x3E, 0x3E, 0xFF), TRUE);
	int iBlue = edit.AddStyle(NULL, RGBA(0x25, 0x63, 0xEB, 0xFF), TRUE);
	edit.SetCurStyle(iRed);
	edit.AddTextEx(L" 红色粗体", iRed);
	edit.InsertTextEx(0, L"[前缀] ", iBlue);
	(void)edit.GetCurStyle();

	editdw_style_info_ info;
	edit.GetStyleInfo(iRed, &info);

	HFONTX hFont = XFont_Create(L"Arial", 12, fontStyle_regular);
	edit.ModifyStyle(iBlue, hFont, RGBA(0x00, 0x00, 0xFF, 0xFF), TRUE);

	HELE hBtn = XBtn_Create(0, 0, 48, 24, L"OK", edit);
	edit.AddObject(hBtn);
	edit.InsertObject(0, XBtn_Create(0, 0, 48, 24, L"Go", edit));
	edit.AddByStyle(iRed);

	// edit.DeleteStyle(iBlue);  // 引用计数 > 0 时失败
	(void)edit.InsertImageThumb(L"D:\\res\\sample.png");
	(void)edit.ClipboardPasteImage();
}

// =============================================================================
// 3. 内容提取 + 序列化 + CopyFrom
// =============================================================================

static void DemoEditPersistence(CXEditDW& edit, HXCGUI hWnd)
{
	CXVector<editdw_content_item_> contents;
	edit.GetContents(contents);

	CXBytes mem;
	edit.SaveToMem(mem);
	edit.LoadFromMem(mem.getPtr(), mem.size());

	edit.SaveToFile(L"D:\\editdw\\content.xdw");
	edit.LoadFromFile(L"D:\\editdw\\content.xdw");

	static CXEditDW mirror;
	mirror.Create(0, 200, 400, 120, hWnd);
	mirror.CopyFrom(edit);
}

// =============================================================================
// 4. 选择 / 光标 / 剪贴板 / 撤销
// =============================================================================

static void DemoEditSelection(CXEditDW& edit)
{
	edit.SetCurPos(0);
	(void)edit.GetCurPos();
	edit.SelectAll();
	(void)edit.HasSelection();
	(void)edit.GetSelStart();
	(void)edit.GetSelEnd();
	(void)edit.GetSelText();
	edit.CancelSelect();
	edit.MoveEnd();
	edit.AutoScroll();
	edit.RelayoutNow();

	(void)edit.ClipboardCopy();
	(void)edit.ClipboardCut();
	(void)edit.ClipboardPaste();
	(void)edit.Undo();
	(void)edit.Redo();
	edit.DeleteSelect();
	edit.Clear();
	edit.SetTextInt(42);
}

// =============================================================================
// 5. 入口
// =============================================================================

void BuildEditDWDemo(HXCGUI hWnd)
{
	static CXEditDW edit;
	edit.Create(20, 20, 460, 160, hWnd);
	g_edit = &edit;

	// 也支持构造时一次性创建:
	// CXEditDW edit2(20, 200, 460, 80, hWnd);

	SetupEditStyle(edit);
	DemoEditTextAndStyle(edit);
	DemoEditPersistence(edit, hWnd);
	DemoEditSelection(edit);
}
