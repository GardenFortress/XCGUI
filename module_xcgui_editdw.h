#ifndef  XCGUI_EDITDW_H
#define  XCGUI_EDITDW_H
//@模块名称  炫彩界面库-DirectWrite编辑框
//@版本  1.0.0
//@日期  2026-05-10
//@作者  未闻花名(QQ936599025)
//@模块备注  基于 Direct2D + DirectWrite 实现的彩色 emoji 编辑框. 继承 CXScrollView,
//          支持 SMP 代理对 (例如 U+1F4B0 💰 / U+1F600 😀) 真实码位渲染、
//          彩色字体 (Segoe UI Emoji) 自动回退、文本 1:1 复制粘贴、
//          鼠标拖选、双击选词、Shift+方向键扩选、撤销重做、自绘闪烁光标。

//@依赖  module_xcgui_class.h

// 头文件依赖说明:
//   - @依赖 是给 IDE 解析器(智能感知/别名/语法着色)用的, 它不会自动注入 #include
//     到 C++ 预处理阶段; 因此下面还要再用真实的 #include 链一次给 cl.exe.
//   - d2d1.h / dwrite.h 不是炫彩模块, 必须用 #include 引入, 且要先于
//     module_xcgui.h 完成, 让 d2d1 引入的 POINTF 抢占名字 (xcgui 内部用
//     __IOleControlSite_INTERFACE_DEFINED__ 保护宏跳过重复定义).
//   - module_base.h / module_xcgui.h / module_xcgui_class.h 自身只用 @依赖
//     声明上游, 不带 #include 链, 所以这里必须按拓扑顺序逐个手动 #include,
//     才能让 CXScrollView / HELE / HDRAW / RGBA() 等符号在类声明处可见.
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <vector>

#include "module_base.h"
#include "module_xcgui.h"
#include "module_xcgui_class.h"

//@src "module_xcgui_editdw.cpp"

#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "user32.lib")

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
//@备注 置字体名称. 默认 "Segoe UI", 渲染 emoji 时自动回退到 "Segoe UI Emoji".
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
	void SetCharStyle(int pos, int len, int iStyle);
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

#endif // XCGUI_EDITDW_H
