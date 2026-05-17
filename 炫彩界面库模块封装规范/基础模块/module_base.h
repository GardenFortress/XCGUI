#ifndef  XCL_BASE_H
//@隐藏{
#define  XCL_BASE_H

#define WIN32_LEAN_AND_MEAN    // 从 Windows 头文件中排除极少使用的内容

//系统库vector文件非UTF8编码, 遇到中文变量名 会出现代码页包不支出的字符
//warning C4819: 该文件包含不能在当前代码页(0)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失
#pragma warning(disable:4819)
#pragma warning(disable:4091)
#pragma warning(disable:4834) //[[nodiscard]] 是 C++17 引入的属性，用于修饰函数，表明该函数的返回值不应该被忽略（通常因为返回值包含重要信息，如错误码、状态标识等）

#define  NOMINMAX

#include <stdio.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <time.h>
#include <direct.h>
#include <atlbase.h>
#include <comdef.h>
#include <io.h>
#include <algorithm>
#include <functional>
#include <windows.h>
#include <shellapi.h>

//@隐藏}

//@模块名称  基础模块
//@版本  1.0
//@日期  2026-03-30
//@作者  XCGUI
//@QQ  154460336
//@模块备注 基础功能模块, 其他模块依赖此核心模块
// 
// 2026-03-30
// #define  NOMINMAX
// 
// 2026-03-04
// 动态数组, 字典, 字节集, 文本型, 增加接口获取包装的数据容器原始类型std::xxx指针
// 
// 2026-03-01
// 动态数组, 字典, 字节集, 文本型, 长度单位从整型改为正变整型
// 
// 2026-02-26
// std::vector 添加成员 push_back(),resize(),pop_back(),empty(),back(),front()
// 
// 2026-02-24
// std::vector 添加成员 size(), data(), operator[]
// 字节集添加构造std::vector<BYTE>
// 
// 2025-11-23
// 增加 std::vector 兼容
// 新增类型 time_t  FILE
// 
// 2025-11-03
// 字节集增加接口 字节集.交换()
// 字节集.替换() 增加参数, 可指定替换旧内容长度, 不影响旧代码
// 
// 2025-10-22
// 文本型 文本型A 字节集 动态数组 字典 添加 C++封装模块时 STD容器兼容接口
// 文本型 文本型A 字节集 动态数组 字典 增加重载操作符 operator bool()  判断是否为空
// 
// 2025-10-19:
// 增加 动态数组 与 std::vector 兼容接口
// 
// 2025-09-27:
// W2A,A2W, W2U, U2W, A2U, U2A 参数兼容 文本型, 文本型A
// 
// 2025-09-01:
// 关键字 "空" 改为 宏定义 移到基础模块中
// 
// 2025-08-17:
// 文本型 文本型A 动态数组 字典 进行优化, 接口不变
// 去除文本型隐式转换, 因为不安全
// 
// 2025-04-02:
// 增加 取最大值() 取最小值()
// 
// 2025-02-13:
// 完善备注和示例
// 
// 2025-02-10:
// 完善备注和示例, 优化 日期时间 接口
// 
// 2025-02-09:
// 完善备注和示例
// 
// 2025-02-06:
// 完善备注和示例, 增加 调试输出格式化(), 调试输出格式化A()
// 
// 2025-02-05:
// 完善备注和示例, 增加 std::wstring, std::string 兼容
// 
// 2024-01-22:
// 新增: 文件读取(),文件写入(),文本取重复(), 取屏幕宽度(), 取屏幕高度();
// 优化: 文本分割() 处理结果通过参数返回 改为 返回值返回, 降低使用难度, 另外几个函数类似;
// 
// 	<基础模块>命名规范:
// 
// 1. 小驼峰命名法: 第一个单词的首字母小写, 从第二个单词开始，每个单词的首字母都大写;
// 	例如: xcl_log(), text_getLeft(), text_getRight(), text_skipSpaceWrapRight();
// 	与大驼峰相比, 输入时减少了大小写切换;
// 
// 2. 前缀分类命名法: 全局函数加上前缀"xcl_": 代表炫彩编程语言, "XC_": 代表炫彩界面库; 在别名中不需要下滑线;
// 	例如: xcl_log(), xcl_doMessage();
// 	相同单词,尽量向前排列,以获得更好的效果; 例如: text_skipSpace(), text_skipSpaceWrap(), text_skipSpaceWrapRight();
// 	对于自身已经有前缀的, 就不需要再加前缀;
// 	优点:
// 		1.更好的自动匹配效果;
// 		2.更好的分类管理与显示, 便于查找和查看;
// 		3.加上下划线获得更清晰阅读效果;
// 		4.避免一些命名与其他库冲突;
// 
// 3. 系统API: 部分系统API没有二次封装, 所以我们需要做的是, 中文别名遵守规范, 英文名称不变;
// 	例如: printf_s():控制台格式输出A   OpenClipboard():剪贴板打开  CloseClipboard():剪贴板关闭()
// 
// 4. 特殊简化函数: 这些函数简化后使得更加易用; 例如: W2A(), A2W();
// 
// 5. 类名: 格式为: "CXAaaBbbCcc", "C" 代表类, "X",代表特殊前缀,避免与其他模块冲突, "Aaa"代表名称;
// 	例如: "CXText", "CXBytes";
// 
// 6. 类成员函数: 同样遵循小驼峰命名法, 例如: CXAaaa::getTextPtr(); 私有函数以下划线开头, 例如: CXAaaa::_getPtr();
// 
// 7. 类成员变量: 同样遵循小驼峰命名法, 例如: CXAaaa::m_aaaBbbCcc; 私有变量以下划线开头, 例如: CXAaaa::_length;
// 
// 8. 结构体: 参考下面格式
// 	//@别名  列表框项结构
// 	struct  listBox_item_   //名称小写字母, 下划线分割单词, 末尾再加上下划线;
// 	{
// 		int  width;
// 		int  height;
// 		int  textLength;    //成员采用小驼峰命名法;
// 	};
// 
// 9. 枚举: 参考下面格式
// 	//@启用枚举前缀
// 	//@别名  列表框项标识
// 	enum  listBox_item_flag_   //名称小写字母, 下划线分割单词, 末尾再加上下划线;
// 	{
// 		listBox_item_flag_length,        //@别名 长度       //在IDE中显示名称: 列表框项标识_长度
// 		listBox_item_flag_textLength,    //@别名 文本长度   //在IDE中显示名称: 列表框项标识_文本长度
// 		listBox_item_flag_maxWidth,      //成员采用小驼峰命名法;
// 	};
// 
// 10. 函数重载: 目前不支持函数重载, 多个同名函数命名格式: 函数名1(),函数名2(),函数名3(); 参数少的往前排列; 例如:
// 	text_lower()                      //@别名 文本到小写()
// 	text_lower2(int pos)              //@别名 文本到小写2()
// 	text_lower3(int pos, int length)  //@别名 文本到小写3()
// 
// 11. 函数参数为指针时: 参数名第一个字母用"p", 代表指针, 例如: text_lower(const wchar_t* pText);
// 
// 12. 函数参数出参入参: 当参数是数据传入或者传出不明确时, 需要明确指出入参:"__out", 或出参:"__in", 或出入参:"__in_out";
// 
// 13. 函数参数为字符串时: 使用字符型指针(const wchar_t*), 更好的适应性和性能; 例如: text_getLeft(const wchar_t* pText, int length);
// 
// 14. 函数返回字符串时: 如果需要释放内存, 那么使用文本型返回, 避免内存泄露; 例如: CXText text_getLeft();
// 
// 15. 函数返回内存数据时: 如果需要释放内存, 那么使用字节集返回, 避免内存泄露; 例如: CXBytes rc_findFileToBytes();
// 
// 16. 函数参数传对象或结构体时: 尽可能的传对象引用, 这样避免了指针的使用和内存的复制; 例如: test_fun(RECT &rc);
// 	但在传递字符串时, 尽可能使用字符型指针(const wchar_t*), 见(13);
// 
// 17. 逻辑型: 逻辑型统一使用BOOL;
// 
// 18. 宏定义, 以大写字母作为前缀, 以便区分是宏; 例如: XCL_min, XCL_max, XCL_INT, XCL_PI;
// 
// 19. 常用单词中英文翻译: 见文档



//@src "module_base.cpp"

#pragma comment(lib, "Advapi32.lib")

//@模块信息结束

//@隐藏{
#ifdef _WIN64
	typedef  __int64             vint;  //变整型
	typedef  unsigned __int64   uvint;  //正变整型
#else
	typedef  int             vint;  //变整型
	typedef  unsigned int   uvint;  //正变整型
#endif

#ifndef __out
	#define  __out
#endif

#ifndef __in
	#define  __in
#endif

#ifndef __in_out
	#define  __in_out
#endif

#define  cx_iterator  auto
#define  事件定义

#ifdef A2W
	#undef A2W
	#undef W2A
#endif

BOOL _dataToHexStrA(__out BYTE* pbDest, size_t destSize, __in const BYTE* pbSrc, size_t nLen, BOOL bUpper, BOOL bSplit);
BOOL _dataToHexStrW(__out wchar_t* pbDest, size_t destSize, __in const BYTE* pbSrc, size_t nLen, BOOL bUpper, BOOL bSplit);

//@隐藏}

/*@声明
namespace  std
{
	//@备注  此类用于兼容模块中的 C++ std::wstring, 炫语言中建议使用 "文本型"
	//@示例  wstring str = 模块中函数()
	//文本型  文本1 = str
	//调试输出(文本1, str)
	class wstring
	{
	public:
		const wchar_t* data();
		size_t length();
	};

	//@备注  此类用于兼容模块中的 C++ std::string, 炫语言中建议使用 "文本型A"
	//@示例  string str = 模块中函数()
	//文本型A  文本1 = str
	//调试输出(文本1, str)
	class string
	{
	public:
		const wchar_t* data();
		size_t length();
	};

	//@备注  此类用于兼容模块中的 C++ std::vector, 炫语言中建议使用 "动态数组"
	template<class T>
	class vector
	{
	public:
		T& operator[](UINT index);
		void push_back(const T& value);
		void push_back(T&& value);
		void resize(size_t new_size);
		void resize(size_t new_size, const T& value);
		void pop_back();
		size_t size();
		bool empty(); 
		T& back();
		T& front();
		T* data();
	};
}
*/

//@备注  字节集, 文本型, 动态数组, 字典, 队列
//@分组{ 数据容器

//----------CXBytes------------------------------------------
class CXText;
class CXTextA;

//@备注 字节集用于存储任意类型的二进制数据, 可以看作是内存块; 
// 例如文件内容, 网络数据包, 图像数据, 加密数据等;
// 可以使用字节集来进行数据的读取、写入、修改和传输;
// 字节集末尾隐藏2个字节(00终止符), 以防止字符串越界, size()获取长度不包含终止符;
// 若是添加数据, 调用 添加XX() 接口, 最后一个参数指定插入位置;
//@doc "doc\bytes_class.txt"
//@别名 字节集
class CXBytes
{
public:
	//@隐藏{
	CXBytes();
	CXBytes(const CXBytes& _that);
	CXBytes(CXBytes&& _that);
	CXBytes(const CXText& _that);
	CXBytes(const CXTextA& _that);
	CXBytes(const wchar_t* pText);
	CXBytes(const char* pText);
	CXBytes(const int value_);
	CXBytes(const __int64 value_);
	CXBytes(const BYTE value_);
	CXBytes(const BYTE* data, size_t length);
	CXBytes(const char value_);

	CXBytes(const std::initializer_list<BYTE> list);
	CXBytes(const std::vector<BYTE>& list);
	~CXBytes();
	void Init();

	// 自动转换不安全, 存在难以发现的意外BUG, 所以 自动转换交给炫彩IDE做
	//explicit operator const BYTE*() const { return _data; } //当作为函数参数传递时, 自动转换为[const BYTE*]
	
	bool operator==(const CXBytes& _that);
	bool operator!=(const CXBytes& _that);

	template<size_t n>
	bool operator==(const char (&_that)[n]) {
		if (_length != n) return FALSE;
		return 0 == memcmp(_data, _that, n);
	}
	template<size_t n>
	bool operator==(const BYTE (&_that)[n]) {
		if (_length != n) return FALSE;
		return 0 == memcmp(_data, _that, n);
	}

	template<size_t n>
	bool operator!=(const char(&_that)[n]) {
		if (_length != n) return TRUE;
		return 0 != memcmp(_data, _that, n);
	}
	template<size_t n>
	bool operator!=(const BYTE(&_that)[n]) {
		if (_length != n) return TRUE;
		return 0 != memcmp(_data, _that, n);
	}

	void operator=(const CXBytes& _that);
	void operator=(CXBytes&& _that);
	void operator=(const CXText& _that);
	void operator=(const CXTextA& _that);
	void operator=(const wchar_t* pText);
	void operator=(const char* pText);
	void operator=(const int value_);
	void operator=(const __int64 value_);
	explicit operator bool() const { return !empty(); }

	const BYTE& operator[](size_t index) const;

	//@隐藏}
	BYTE& operator[](size_t index);

	CXBytes operator+(const CXBytes& _that);
	void operator+=(const CXBytes& _that);
	void operator+=(const std::initializer_list<BYTE> list);

	//@备注 分配指定大小的内存空间, 不保留原有数据; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 数据.置大小(100) //分配大小100字节空间
	//@参数 分配内存大小,字节为单位
	//@返回 内存地址
	//@别名 置大小(内存大小)
	BYTE* alloc(size_t length);

	//@备注 调整大小, 保留原有数据, 如果空间不足将截断
	//@示例 字节集  数据
	//  数据.调整大小(200) //调整大小为200字节空间
	//@参数 重置内存大小, 字节为单位
	//@返回 内存地址
	//@别名 调整大小(内存大小)
	BYTE* resize(size_t length);

	//@备注 追加指定大小的内存到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 数据.添加大小(100) //在原有基础上增加100字节空间
	//@参数 内存大小, 字节为单位
	//@别名 添加大小(内存大小)
	void addSize(size_t length);

	//@备注 替换指定位置数据
	//@示例  字节集  数据
	// 数据.替换数据("abc", 文本取长度("abc")*2, 3) //将偏移位置3开始的数据替换为"abc", 长度(3*2)字节 
	//@参数 新的数据
	//@参数 新的数据长度, 字节为单位
	//@参数 替换位置偏移, 本字节集中的偏移位置, 必须大于等于0且小于等于本字节集的长度.
	//@参数 旧数据长度(可空), 将被替换的旧数据长度, 如果为-1, 那么替换相同长度的数据
	//@别名 替换数据(新数据地址, 新数据长度, 替换开始位置, 旧数据长度)
	void replace(const void* data, size_t length, size_t off, size_t old_length = -1);

	//@备注  交换数据, 交换两个字节集变量的数据
	//@示例  字节集  数据1 = {1,2,3}
	// 字节集  数据2 = {4,5,6}
	// 数据1.交换(数据2)
	//@参数  待交换的字节集
	//@别名  交换(字节集)
	void swap(CXBytes& right);

	//@隐藏{
	void insert(const void* data, size_t length, size_t off = -1);
	void addEnd(const void* data, size_t length);
	//@隐藏}

	//@备注  添加数据到指定位置, 如果参数 "插入位置" 等于 -1, 那么添加到末尾, 否则插入到指定的位置
	//@示例 字节集  数据
	// 数据.添加数据("abc", 文本取长度("abc")*2, 0) //添加数据"abc", 并指定数据长度(因为一个字符占2字节所以*2), 偏移位置0
	// 整型 变量1 = 10
	// 数据.添加数据(&变量1, 4, 2) //添加变量"变量1"的值, 长度4字节(因为整型变量为4字节), 偏移位置2
	//@参数 被添加的数据地址
	//@参数 被添加的数据长度(字节为单位)
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加数据(数据地址, 数据长度, 插入位置)
	void add(const void* data, size_t length, size_t off = -1);

	//@备注 添加文本到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例  字节集  数据
	// 数据.添加文本("abc")  //添加文本"abc"到末尾, 如果需要添加终止符参数2填真
	//@参数 要添加的文本
	//@参数 是否添加字符串末尾的终止符到字节集
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加文本(字符串, 是否添加终止符, 插入位置)
	void addText(const wchar_t* pText, BOOL bTerminator = FALSE, size_t off = -1);

	//@备注 添加文本A到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 数据.添加文本A(A"abc")  //添加文本A"abc"到末尾, 如果需要添加终止符参数2填真
	//@参数 要添加的文本A
	//@参数 是否添加字符串末尾的终止符到字节集
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加文本A(字符串, 是否添加终止符, 插入位置)
	void addTextA(const char* pText, BOOL bTerminator = FALSE, size_t off = -1);

	//@备注 添加整型到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 整型 变量1=10
	// 数据.添加整型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的整型数据
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加整型(整型值, 插入位置)
	void addInt(int value_, size_t off = -1);
	
	//@备注 添加多个整型值到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加整型连续( {整型值1, 整型值2, 整型值3, ...} )
	//@示例 字节集  数据
	// 数据.添加整型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个整型值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加整型连续(数据列表, 插入位置)
	void addInts(std::initializer_list<int> list, size_t off = -1);

	//@备注 添加正整型数据到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 正整型 变量1=10
	// 数据.添加正整型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的正整型数据
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加正整型(正整型值, 插入位置)
	void addUInt(UINT value_, size_t off = -1);

	//@备注 添加多个正整型值到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加整型连续( {正整型值1, 正整型值2, 正整型值3, ...} )
	//@示例 字节集  数据
	// 数据.添加正整型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个正整型值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加正整型连续(数据列表, 插入位置)
	void addUInts(std::initializer_list<UINT> list, size_t off = -1);

	//@备注 添加 "64位整型数据" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 长整型 变量1=10
	// 数据.添加长整型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"长整型数据", 也就是有符号64位整型
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加长整型(长整型值, 插入位置)
	void addInt64(__int64 value_, size_t off = -1);

	//@备注 添加多个长整型值到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加长整型连续( {长整型值1, 长整型值2, 长整型值3, ...} )
	//@示例 字节集  数据
	// 数据.添加长整型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个长整型值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加长整型连续(数据列表, 插入位置)
	void addInt64s(std::initializer_list<__int64> list, size_t off = -1);

	//@备注 添加 "正长整型值" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 正长整型 变量1=10
	// 数据.添加正长整型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"正长整型值", 也就是无符号64位整型
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加正长整型(正长整型值, 插入位置)
	void addUInt64(UINT64 value_, size_t off = -1);

	//@备注 添加多个正长整型值到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加正长整型连续( {正长整型值1, 正长整型值2, 正长整型值3, ...} )
	//@示例 字节集  数据
	// 数据.添加正长整型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个正长整型值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加正长整型连续(数据列表, 插入位置)
	void addUInt64s(std::initializer_list<UINT64> list, size_t off = -1);

	//@备注 添加 "短整型数据" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	//  短整型 变量1=10
	// 数据.添加短整型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"短整型数据"
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加短整型(短整型数据, 插入位置)
	void addShort(short value_, size_t off = -1);

	//@备注 添加多个短整型值到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加短整型连续( {短整型值1, 短整型值2, 短整型值3, ...} )
	//@示例 字节集  数据
	// 数据.添加短整型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个短整型值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加短整型连续(数据列表, 插入位置)
	void addShorts(std::initializer_list<short> list, size_t off = -1);

	//@备注 添加 "正短整型值" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 正短整型 变量1=10
	// 数据.添加正短整型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"正短整型值", 也就是无符号短整型
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加正短整型(正短整型值, 插入位置)
	void addUShort(unsigned short value_, size_t off = -1);

	//@备注 添加多个 "正短整型值" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加正短整型连续( {正短整型值1, 正短整型值2, 正短整型值3, ...} )
	//@示例 字节集  数据
	// 数据.添加正短整型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个正短整型值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加正短整型连续(数据列表, 插入位置)
	void addUShorts(std::initializer_list<unsigned short> list, size_t off = -1);

	//@备注 添加 "浮点型值" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 浮点型 变量1=10
	// 数据.添加浮点型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"浮点型值"
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加浮点型(浮点型值, 插入位置)
	void addFloat(float value_, size_t off = -1);

	//@备注 添加多个 "浮点型值" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加浮点型连续( {浮点型值1, 浮点型值2, 浮点型值3, ...} )
	//@示例 字节集  数据
	// 数据.添加浮点型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个浮点型值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加浮点型连续(数据列表, 插入位置)
	void addFloats(std::initializer_list<float> list, size_t off = -1);

	//@备注 添加 "双浮点型值" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	// @示例 字节集  数据
	// 双浮点型 变量1=10
	// 数据.添加双浮点型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"双浮点型值"
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加双浮点型(双浮点型值, 插入位置)
	void addDouble(double value_, size_t off = -1);

	//@备注 添加多个 "双浮点型值" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加双浮点型连续( {双浮点型值1, 双浮点型值2, 双浮点型值3, ...} )
	//@示例  字节集  数据
	// 数据.添加双浮点型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个双浮点型值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加双浮点型连续(数据列表, 插入位置)
	void addDoubles(std::initializer_list<double> list, size_t off = -1);

	//@备注 添加 "宽字符" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 字符型 变量1=10
	// 数据.添加字符型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"宽字符"
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加字符型(宽字符, 插入位置)
	void addWchar(wchar_t value_, size_t off = -1);
	
	//@备注 添加多个 "宽字符" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加字符型连续( {宽字符值1, 宽字符值2, 宽字符值3, ...} )
	//@示例 字节集  数据
	// 数据.添加字符连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个宽字符值}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加字符连续(数据列表, 插入位置)
	void addWchars(std::initializer_list<wchar_t> list, size_t off = -1);

	//@备注 添加多个 "字节数据" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加字节型连续( {字节数据值1, 字节数据值2, 字节数据值3, ...} )
	//@示例  字节集  数据
	// 数据.添加字节型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个字节型}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加字节型连续(数据列表, 插入位置)
	void addBytes(std::initializer_list<BYTE> list, size_t off = -1);

	//@备注 添加 "字节数据" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 字节型 变量1=10
	// 数据.添加字节型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"字节数据"
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名  添加字节型(字节数据, 插入位置)
	void addByte(BYTE value_, size_t off = -1);

	//@备注 添加 "十六进制字符串" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	// 支持 0-9, a-z, A-Z 字符;
	//@示例 字节集  数据
	//  数据.添加十六进制("616263")  //"616263"十六进制文本对应"abc",  将十六进制文本转换为为"abc"后写入字节集
	//@参数 要添加的"十六进制字符串"
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加十六进制(字符串, 插入位置)
	void addHex(const wchar_t* pHex, size_t off = -1);
	
////////////////增加 变整数
	//@备注 添加 "变整型" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//@示例 字节集  数据
	// 变整型 变量1=10
	// 数据.添加变整型(变量1) //添加"变量1"的值到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 要添加的"变整型数据"
	//@别名 添加变整型(变整型值, 插入位置)
	void  addVint(vint value_, size_t off = -1);

	//@备注 添加多个 "变整型" 到末尾; 如果旧数据是引用的数据, 那么将断开旧数据的引用, 作为新的实例
	//调用格式: 添加变整型连续( {变整型值1, 变整型值2, 变整型值3, ...} )
	//@示例  字节集  数据
	// 数据.添加变整型连续({1,2,3}) //添加"{1,2,3}"到末尾, 如果需要插入到指定位置, 那么指定参数2的值
	//@参数 {连续多个变整型}
	//@参数 插入位置偏移, 若是0插入开始位置, 若是-1插入到末尾;
	//@别名 添加变整型连续(数据列表, 插入位置)
	void  addVints(const std::initializer_list<vint> list, size_t off = -1);
	
//@隐藏{
	//@备注 与 getPtr() 功能相同
	const BYTE* get()const { return _data; }
//@隐藏}

	//@备注 获取数据地址
	//@示例 字节集  数据
	// 常量 字节型* 数据地址= 数据.取地址() //获取字节集中存放数据的内存地址
	//@返回 返回数据地址
	//@别名 取地址()
	const BYTE* getPtr()const { return _data; }

	//@备注 获取数据文本指针, 可指定位置偏移, 返回文本指针, 偏移位置以字节为单位; 若数据不是文本数据, 可能出现异常
	//@示例 字节集  数据
	// 常量 字符型* 数据地址= 数据.取文本指针(2) //获取字节集中存放数据的内存地址 + 位置偏移
	//@参数 可空,位置偏移(字节为单位)
	//@返回 字符串指针
	//@别名 取文本指针(位置偏移)
	const wchar_t* getTextPtr(size_t off =0) const { return (const wchar_t*)(_data+ off); }
	
	//@备注 获取数据文本指针, 可指定位置偏移, 返回文本指针A, 偏移位置以字节为单位; 若数据不是文本A数据, 可能出现异常
	//@示例  字节集  数据
	// 常量 字符型A* 数据地址= 数据.取文本指针(2) //获取字节集中存放数据的内存地址 + 位置偏移
	//@参数 可空,位置偏移(字节为单位)
	//@返回 字符串指针A
	//@别名 取文本指针A(位置偏移)
	const char* getTextPtrA(size_t off =0) const { return (const char*)(_data+ off); }

	//@备注 从指定位置获取指定长度文本
	//@示例 字节集  数据
	// 文本型 变量1 = 数据.取文本(10)  //将指定偏移位置的数据按 "文本" 的形式返回, 要确保是文本型的数据
	//@参数 位置偏移(字节为单位)
	//@参数 要获取的文本长度, 字符为单位
	//@返回 结果文本
	//@别名 取文本(位置偏移, 字符长度)
	CXText  getText(size_t off, size_t wcharLength);

	//@备注 从指定位置获取指定长度文本
	//@示例 字节集  数据
	// 文本型A 变量1 = 数据.取文本A(10)  //将指定偏移位置的数据按 "文本" 的形式返回, 要确保是文本型A的数据
	//@参数 位置偏移(字节为单位)
	//@参数 要获取的文本长度, 字符为单位
	//@返回 结果文本A
	//@别名 取文本A(位置偏移, 字符长度)
	CXTextA  getTextA(size_t off, size_t charLength);

	//@备注  从指定位置取 "整型"数据
	//@示例  字节集  数据
	// 整型 变量1 = 数据.取整型(10)  //将指定偏移位置的数据按 "整型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  整型数据
	//@别名  取整型(位置偏移)
	int getInt(size_t off =0);

	//@备注  从指定位置取 "无符号整型" 数据
	//@示例  字节集  数据
	// 正整型 变量1 = 数据.取正整型(10)  //将指定偏移位置的数据按 "正整型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  无符号整型数据
	//@别名  取正整型(位置偏移)
	UINT getUInt(size_t off = 0);

	//@备注  从指定位置取 "64位整型" 数据
	//@示例  字节集  数据
	// 长整型 变量1 = 数据.取长整型(10)  //将指定偏移位置的数据按 "长整型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  64位整型数据
	//@别名  取长整型(位置偏移)
	__int64 getInt64(size_t off = 0);

	//@备注  从指定位置取 "64位无符号整型" 数据
	//@示例  字节集  数据
	// 正长整型 变量1 = 数据.取正长整型(10)  //将指定偏移位置的数据按 "正长整型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  64位无符号整型数据
	//@别名  取正长整型(位置偏移)
	UINT64 getUInt64(size_t off = 0);

	//@备注  从指定位置取 "短整型" 数据
	//@示例  字节集  数据
	// 短整型 变量1 = 数据.取短整型(10)  //将指定偏移位置的数据按 "短整型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  短整型数据
	//@别名  取短整型(位置偏移)
	short getShort(size_t off = 0);

	//@备注  从指定位置取 "无符号短整型" 数据
	//@示例  字节集  数据
	// 正短整型 变量1 = 数据.取正短整型(10)  //将指定偏移位置的数据按 "正短整型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  无符号短整型数据
	//@别名  取正短整型(位置偏移)
	unsigned short getUShort(size_t off = 0);

	//@备注  从指定位置取 "浮点型" 数据
	//@示例  字节集  数据
	// 浮点型 变量1 = 数据.取浮点型(10)  //将指定偏移位置的数据按 "浮点型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  浮点型数据
	//@别名  取浮点型(位置偏移)
	float getFloat(size_t off = 0);

	//@备注  从指定位置取 "双浮点型" 数据
	//@示例  字节集  数据
	// 双浮点型 变量1 = 数据.取双浮点型(10)  //将指定偏移位置的数据按 "双浮点型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  双浮点型数据
	//@别名  取双浮点型(位置偏移)
	double getDouble(size_t off = 0);

	//@备注  从指定位置取 "字符型" 数据
	//@示例  字节集  数据
	// 字符型 变量1 = 数据.取字符型(10)  //将指定偏移位置的数据按 "字符型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  字符型数据
	//@别名  取字符型(位置偏移)
	wchar_t getWchar(size_t off = 0);

	//@备注  从指定位置取 "字节型" 数据
	//@示例  字节集  数据
	// 字节型 变量1 = 数据.取字节型(10)  //将指定偏移位置的数据按 "字节型" 的形式返回
	//@参数  位置偏移(字节为单位)
	//@返回  字节型数据
	//@别名  取字节型(位置偏移)
	BYTE getByte(size_t off = 0);

	//@备注  清空数据; 如果旧数据是引用的数据, 那么将断开旧数据的引用,作为新的实例
	//@别名  清空()
	void  clear();

//@隐藏{
	//@备注  与 getSize() 功能相同
	size_t  size() const { return _length; }
//@隐藏}

	//@备注  获取数据长度, 字节为单位
	//@示例  字节集  数据
	// 整型 变量1 = 数据.取大小()
	//@返回  数据长度
	//@别名  取大小()
	size_t  getSize() const { return _length; }

	//@备注  判断数据是否为空
	//@示例  字节集  数据
	// 逻辑型 变量1 = 数据.是否为空()
	//@返回  如果数据为空返回真, 否则返回假
	//@别名  是否为空()
	BOOL empty() const { return (0 == _length); }

	//@备注  重置数据, 旧的数据将被清空, 将新的数据填入内存; 如果旧数据是引用的数据, 那么将断开旧数据的引用,作为新的实例
	//@示例  字节集  数据
	// 整型 变量1=10;  数据.置数据(&变量1, 4) //清空旧数据, 将指定地址的数据, 并指定长度, 写入到字节集
	//@参数  新的数据地址
	//@参数  数据长度,字节为单位
	//@别名  置数据(数据地址, 数据长度)
	void set(const void* data, size_t length);

	//@备注  重置文本数据, 旧的数据将被清空;  如果旧数据是引用的数据, 那么将断开旧数据的引用,作为新的实例
	//@示例  字节集  数据
	//  数据.置文本("abc", 文本取长度("abc")) //清空旧数据, 将"abc"写入到字节集
	//@参数  字符串指针, 将要置入的文本
	//@别名  置文本(字符串)
	void setText(const wchar_t *pText);

	//@备注  重置文本A数据, 旧的数据将被清空;  如果旧数据是引用的数据, 那么将断开旧数据的引用,作为新的实例
	//@示例  字节集  数据
	// 数据.置文本A(A"abc")
	//@参数  字符串指针A, 将要置入的文本A
	//@别名  置文本A(字符串A)
	void setTextA(const char* pText);
	
	//@备注  重置十六进制文本, 旧的数据将被清空;  如果旧数据是引用的数据, 那么将断开旧数据的引用,作为新的实例
	// 支持 0-9, a-z, A-Z 字符;
	//@示例  字节集  数据
	// 数据.置十六进制文本("616263") //"616263"十六进制文本对应"abc",  将十六进制文本转换为为"abc"后写入字节集
	//@参数  字符串指针, 将要置入的十六进制文本
	//@别名  置十六进制文本(十六进制字符串)
	void setHexText(const wchar_t* pHex);

	//@备注  取指定长度的数据, 存储在字节集中返回, 从左边计算
	//@示例  字节集  数据
	// 字节集 变量1 = 数据.取左边(6) //从字节集中取左边6字节数据
	//@参数  数据长度, 字节为单位
	//@返回  返回的字节集数据
	//@别名  取左边(长度)
	CXBytes getLeft(size_t length);

	//@备注  取指定长度的数据, 存储在字节集中返回, 从右边计算
	//@示例  字节集  数据
	// 字节集 变量1 = 数据.取右边(6) //从字节集取末尾6字节数据
	//@参数  数据长度, 字节为单位
	//@返回  返回的字节集数据
	//@别名  取右边(长度)
	CXBytes getRight(size_t length);

	//@备注  取指定长度的数据, 存储在字节集中返回, 从指定位置计算
	//@示例  字节集  数据
	// 字节集 变量1 = 数据.取中间(3, 6) //从字节集中指定位置(3) 取长度(6) 字节数据
	//@参数  数据长度, 字节为单位
	//@返回  返回的字节集数据
	//@别名  取中间(开始位置索引, 长度)
	CXBytes getMid(size_t iStart, size_t length);

	//@备注  将数据转换为十六进制文本, 存储在字节集中返回
	//@示例  字节集  数据=A"abc"
	// 字节集 变量1 = 数据.到十六进制字节集() //将字节集中的数据转换为十六进制文本, 然后返回结果字节集, 原字节集不变
	// //A"abc" 对应值为 "{61 62 63}", 字符'a' = 0x61, 字符'b' = 0x62, 字符'c' = 0x63
	// //打印结果值为 "{36 00 31 00 36 00 32 00 36 00 33 00}", 字符'6' = 0x36, 字符'1' = 0x31, ... , 因为是宽字符, 所以占2字节
	//@参数  是否为大写
	//@参数  是否分割, 使用空格分割
	//@参数  位置偏移(字节为单位), 默认值0
	//@参数  获取内容长度, 字节为单位, 默认值0全部
	//@返回  返回的字节集数据
	//@别名  到十六进制字节集(是否为大写, 是否分隔)
	CXBytes toHexBytes(BOOL bUpper = TRUE, BOOL bSplit = FALSE, size_t off = 0, size_t length=0);

	//@备注  将数据转换为十六进制文本A, 存储在字节集中返回
	//@示例  字节集  数据 = A"abc"
	// 字节集 变量1 = 数据.到十六进制字节集A() //将字节集中的数据转换为十六进制文本, 然后返回新的字节集, 原字节集不变
	// //A"abc" 对应值为 "{61 62 63}", 字符'a' = 0x61, 字符'b' = 0x62, 字符'c' = 0x63
	// //打印结果值为 "{36 31 36 32 36 33}", 字符'6' = 0x36, 字符'1' = 0x31, ...
	//@参数  是否为大写
	//@参数  是否分割, 使用空格分割
	//@参数  位置偏移(字节为单位), 默认值0
	//@参数  获取内容长度, 字节为单位, 默认值0全部
	//@返回  返回的字节集数据
	//@别名  到十六进制字节集A(是否为大写, 是否分隔)
	CXBytes toHexBytesA(BOOL bUpper = TRUE, BOOL bSplit = FALSE, size_t off = 0, size_t length = 0);

	//@备注  转换为十六进制字符串W
	//@示例  字节集  数据 = A"abc"
	// 文本型 变量1 = 数据.到十六进制文本() //将字节集中的数据转换为十六进制文本然后返回, 原字节集不变
	// //A"abc" 对应值为 "{61 62 63}", 字符'a' = 0x61, 字符'b' = 0x62, 字符'c' = 0x63
	// //打印结果值为 "616263
	//@参数  是否为大写
	//@参数  是否分割, 使用空格分割
	//@参数  位置偏移(字节为单位), 默认值0
	//@参数  获取内容长度, 字节为单位, 默认值0全部
	//@返回  返回文本
	//@别名  到十六进制文本(是否为大写,是否分隔)
	CXText  toHex(BOOL bUpper = TRUE, BOOL bSplit = FALSE, size_t off = 0, size_t length = 0);

	//@备注  转换为十六进制字符串A
	//@示例  字节集  数据 = A"abc"
	// 文本型A 变量1 = 数据.到十六进制文本A() //将字节集中的数据转换为十六进制文本然后返回, 原字节集不变
	// //A"abc" 对应值为 "{61 62 63}", 字符'a' = 0x61, 字符'b' = 0x62, 字符'c' = 0x63
	// //打印结果值为 "616263
	//@参数  是否为大写
	//@参数  是否分割, 使用空格分割
	//@参数  位置偏移(字节为单位), 默认值0
	//@参数  获取内容长度, 字节为单位, 默认值0全部
	//@返回  返回文本A
	//@别名  到十六进制文本A(是否为大写,是否分隔)
	CXTextA toHexA(BOOL bUpper = TRUE, BOOL bSplit=FALSE, size_t off = 0, size_t length = 0);

	//@备注 查找指定字节集, 返回字节集所在位置索引
	//@示例  字节集  数据
	// 字节集 要查找的内容 = {1,2,3}; 整型 位置索引 = 数据.查找字节集(要查找的内容, 0)
	//@参数 要查找的字节集
	//@参数 开始位置索引, 从该位置开始查找
	//@返回 所在位置索引, 如果没有找到返回-1
	//@别名 查找字节集(字节集, 开始位置索引)
	size_t  findBytes(CXBytes& dest, size_t off=0);

private:
	void  move() { _data = NULL; _length = 0; } //移除数据
	BYTE*   _data;
	size_t  _length;   //总长度 不包含隐藏的终止字符 
	BYTE hexCharToByte(wchar_t c);
};

//@隐藏{
//COM变体型赋值到文本型
extern void _xcl_comVarToText(CXText* text_, const class CXComVariant* pComVar);
//@隐藏}

//@备注 文本操作类, 字符串操作包装类, Unicode编码, 每个字符占2个字节
//@示例 文本型  文本1 = "Unicode字符串" 
// 文本1 += "abc"
// 调试输出(文本1)
//@别名 文本型
class CXText
{
public:
	//@隐藏{
	CXText() = default;
	CXText(const CXText& _that) { _text = _that._text; }
	CXText(const CXTextA& _that);
	CXText(CXText&& _that) { _text = std::move(_that._text); }
	CXText(const CXBytes &_that);
	CXText(const std::initializer_list<wchar_t> list) { _text = list; }
	CXText(const std::wstring& wstring_) { _text = wstring_; }
	CXText(const std::string& string_);
	CXText(std::wstring&& wstring_) { _text = std::move(wstring_); }
	CXText(const wchar_t* pText, size_t length=-1);
	CXText(const char* pText, size_t length=-1);
	CXText(const int value_);
	CXText(const INT64 value_);
	CXText(const double value_);
	CXText(const float  value_);
	CXText(const char value_);
	CXText(const wchar_t value_);
	
	void operator=(const wchar_t* pText) { _text = pText ? pText : L""; }
	void operator=(const char* pText);
	void operator=(const wchar_t ch) { _text = ch; }
	void operator=(const std::wstring& wstring_) { _text = wstring_; }
	void operator=(const std::string& string_);
	void operator=(const CXText &_that) {_text = _that._text;}
	void operator=(const CXBytes &_that);
	void operator=(const CXTextA& _that);
	void operator=(CXText&& _that) {
		_text = std::move(_that._text);
	}
	CXText& operator+=(const CXText& _that){
		_text += _that.get();
		return *this;
	}

 	void operator=(const class CXComVariant& _that) { _xcl_comVarToText(this, &_that); }
	bool operator>(const CXText& right) const { return (_text.compare(right._text) > 0); }
	
	bool operator<(const CXText& right) const { return (_text.compare(right._text) < 0); }
	bool operator!=(const CXText& right) const { return (_text.compare(right._text) != 0); }
	bool operator!=(const wchar_t* right) const { return (_text.compare(right) != 0); }

	bool operator==(const CXText& right) const { return _text == right._text; }
	bool operator==(const wchar_t* right) const { return _text == right; }
	friend bool operator==(const wchar_t* left, const CXText& right) { return right.equal(left); }
	friend bool operator!=(const wchar_t* left, const CXText& right) { return (right != left); }
	friend CXText operator+(const wchar_t* left, const CXText& right)  { CXText  text = left; text += right;	return text; }
	friend std::wostream& operator<<(std::wostream& os, const CXText& myStr) { os << myStr.get();return os; }

	// 自动转换不安全, 存在难以发现的意外BUG, 所以 自动转换交给炫彩IDE做
	//explicit operator const wchar_t*() const { return (_text.c_str());}//当作为函数参数传递时, 自动转换为[const char*]
	explicit operator bool() const { return !_text.empty(); }

	bool equal(const CXText &right) const { return  _text == right._text; }
	const wchar_t& operator[](size_t index)const { return _text[index]; }
	const std::wstring* getString() const { return &_text; }
//@隐藏}
	std::wstring*  getString(){ return &_text; }
	wchar_t& operator[](size_t index) { return _text[index]; }
	
	CXText operator+(const wchar_t ch) const {
		CXText  ret = *this;
		ret += ch;
		return ret;
	}
	CXText operator+(const CXText& right) const {
		CXText  ret = *this; 
		ret += right;
		return ret; }

	CXText operator+(const wchar_t* pText) const {
		CXText  ret = *this;
		ret += pText;
		return ret;
	}
	CXText operator+(wchar_t* pText) const  //若是不重载, 特殊情况会报错, 例如:  文本型 + (wchar_t*)"123"
	{
		CXText  ret = *this;
		ret += pText;
		return ret;
	}
	CXText operator+(const int value_) const;

	void operator+=(const wchar_t ch) { _text += ch; }
	void operator+=(const wchar_t* pText) { _text += pText; }
//@隐藏{	
	//@备注  与 getPtr() 功能相同
	const wchar_t* get() const { return _text.c_str(); }
//@隐藏}

	//@备注  获取文本指针, 因为文本型是类, 不是基础类型, 所以要取内容地址, 需要调用该接口
	//@示例 文本型  文本1
	// 字符型* 字符串指针 =  文本1.取地址() //获取字符串在内存中的地址
	//@返回  字符串指针
	//@别名  取地址()
	const wchar_t* getPtr() const { return _text.c_str(); }
//@隐藏{
	//@备注 与 getSize() 功能相同
	size_t  size() const { return _text.size(); }

	const wchar_t* c_str() const { return _text.c_str(); }

	auto begin() { return _text.begin(); }
	auto begin() const { return _text.begin(); }
	auto end() { return _text.end(); }
	auto end() const { return _text.end(); }

	auto rbegin() { _text.rbegin(); }
	auto rbegin() const { _text.rbegin(); }

	auto rend() { _text.rend(); }
	auto rend() const { _text.rend(); }

	void push_back(const wchar_t _ch) { _text.push_back(_ch); }
	void pop_back() { _text.pop_back(); }

//@隐藏}

	//@备注  获取文本长度
	//@示例  文本型  文本1
	//  整型 文本长度 = 文本1.取大小() //获取内容长度(字符数量),
	//@返回  文本长度, 以字符为单位
	//@别名  取大小()
	size_t  getSize() const { return _text.size(); }

	//@备注  重置大小, 原有内容不变, 超出的部分会被截断
	//@示例  文本型  文本1
	// 文本1.置大小(10) //设置长度为10个字符大小空间, 相当置空文本(填10个0)
	//@参数  内容长度, 字符为单位
	//@参数  填充字符
	//@别名  置大小(长度)
	void resize(size_t length, const wchar_t ch_=0) { _text.resize(length, ch_); }

	//@备注  为字符串分配指定大小的内存, 预留一定的内存空间，以避免在后续添加字符时频繁进行内存重新分配，从而提高性能
	//@示例  文本型  文本1
	// 文本1.预置大小(256)  //提前分配大于256字符的空间,方便后面使用
	//@参数  分配空间大小, 字符为单位
	//@别名  预置大小(长度)
	void reserve(size_t length) { _text.reserve(length); }
	
	//@备注  判断内容是否为空
	//@示例  文本型  文本1
	// 逻辑型 内容是否为空 = 文本1.是否为空() //判断内容是否为空
	//@返回  如果内容为空返回真,否则返回假
	//@别名  是否为空()
	BOOL empty() const { return _text.empty();return TRUE;}

	//@备注  添加文本到末尾
	//@示例  文本型  文本1
	// 文本1.添加("abc") //在末尾追加文本
	//@参数  字符串指针
	//@别名  添加(文本)
	void append(const wchar_t* pText) { _text.append(pText); }

	//@备注  添加指定长度的文本到末尾
	//@示例  文本型  文本1
	// 文本1.添加2("abc", 2) //在末尾追加文本, 并且指定只添加2个字符"ab"进去
	//@参数  字符串指针
	//@参数  字符串长度, 字符为单位
	//@别名  添加2(文本, 长度)
	void append2(const wchar_t* pText, size_t length) { _text.append(pText, length); }

	//@备注  插入文本到指定位置
	//@示例  文本型  文本1
	// 文本1.插入2(2, "abc") //在偏移位置2插入字符串"abc"
	//@参数  位置偏移, 字符为单位, 插入位置
	//@参数  字符串指针
	//@别名  插入(插入位置, 文本)
	void insert(size_t off, const wchar_t* pText) { _text.insert(off, pText); }
	
	//@备注  将指定范围内的文本替换为新的文本
	//@示例 文本型  文本1 = "123456"
	// 文本1.替换2(2, 10, "abc") //从偏移位置 1 - 5 的内容 替换为 "abc"
	// //结果为: "1abc6"
	//@参数  开始位置, 位置偏移, 字符为单位
	//@参数  结束位置, 位置偏移, 字符为单位
	//@参数  将被填入的新文本
	//@别名  替换(开始位置, 结束位置, 文本)
	void replace(size_t off1, size_t off2, const wchar_t* pText) {
		_text.replace(_text.begin() + off1, _text.begin() + off2, pText);
	}

	//@备注  将文本中指定的子字符串替换
	//@示例  文本型  文本1 = "123456"
	// 文本1.替换子文本("34", "abc") //将 "34" 替换为 "abc"
	// //结果为: "12abc56"
	//@参数  要被替换的子字符串
	//@参数  替换后的字符串
	//@别名  替换子文本(要被替换的子字符串,  替换后的字符串)
	void replaceSub(__in const wchar_t* pSubStr, __in const wchar_t* pReplaceStr);

	//@备注  将指定数量的文本替换为新的文本
	//@示例  文本型  文本1 = "123456"
	// 文本1.替换指定数量( 0, 5, "abc") //从偏移位置 0 - 5 的内容替换为 "abc"
	// //结果为: "abc6"
	//@参数  起始位置偏移, 字符为单位
	//@参数  替换文本数量, 从起始位置开始计算, 字符为单位
	//@参数  将被填入的新文本
	//@别名  替换指定数量(开始位置, 数量, 文本)
	void replaceCount(size_t off, size_t count, const wchar_t* pText) {
		_text.replace(off, count, pText);
	}

	//@备注  查指定字符串, 返回字符串所在位置索引
	//@示例  文本型  文本1 = "123456"
	// 整型 位置 = 文本1.查找("56") //返回"56"的偏移位置4
	//@参数  查找的字符串
	//@返回  文本所在位置索引, 字符为单位, 失败返回-1
	//@别名  查找(文本)
	size_t find(const wchar_t* pText) const {return _text.find(pText, 0);}

	//@备注  从指定位置开始查找字符串, 返回字符串所在位置索引
	//@示例  文本型  文本1 = "123456123456"
	// 整型 位置 = 文本1.查找2("56",6) //从偏移位置6开始查找,返回10
	//@参数  查找的字符串
	//@参数  开始查找位置偏移, 字符为单位
	//@返回  文本所在位置索引, 失败返回-1
	//@别名  查找2(文本, 开始位置)
	size_t find2(const wchar_t* pText, size_t off) const { return _text.find(pText, off);}
	
	//@备注  反向查指定字符串, 返回字符串所在位置索引
	//@示例  文本型  文本1 = "123456123456"
	// 整型 位置 = 文本1.反向查找("56") //从后向前查找, 返回10
	//@参数  查找的字符串
	//@返回  文本所在位置索引, 字符为单位, 失败返回-1
	//@别名  反向查找(文本)
	size_t rfind(const wchar_t* pText) const { return _text.rfind(pText); }

	//@备注  从指定位置开始反向查找字符串, 返回字符串所在位置索引
	//@示例  文本型  文本1 = "123456123456"
	// 整型 位置 = 文本1.反向查找2("56", 12) //从偏移位置12向前查找, 返回10
	//@参数  查找的字符串
	//@参数  开始查找位置偏移, 字符为单位
	//@返回  文本所在位置索引, 失败返回-1
	//@别名  反向查找2(文本, 开始位置)
	size_t rfind2(const wchar_t* pText, size_t off = -1) const { return _text.rfind(pText, off); }

	//@备注  获取指定位置范围的字符串
	//@示例 文本型  文本1 = "123456"
	// 文本型 结果 = 文本1.取子文本(3, 2) //从偏移位置3开始取内容长度为2(2个字符), 返回结果为: "45"
	//@参数  开始位置偏移, 字符为单位
	//@参数  从开始位置计算长度, 字符为单位
	//@返回  获取的文本
	//@别名  取子文本(开始位置 ,长度)
	CXText substr(size_t off, size_t count) const;

	//@备注  取末尾字符
	//@示例  文本型  文本1 = "123456"
	// 字符型 结果 = 文本1.取尾字符() //获取末尾字符, 返回结果为: '6'
	//@返回  末尾字符
	//@别名  取尾字符()
	wchar_t back() const { return _text.back(); }
	
	//@备注  取首字符
	//@示例 文本型  文本1 = "123456"
	// 字符型 结果 = 文本1.取首字符() //获取末尾字符, 返回结果为: '1'
	//@返回  首字符
	//@别名  取首字符()
	wchar_t front() const {	return _text.front(); }

	//@备注  交换两个文本对象内容
	//@示例 文本型  文本1 = "123456"
	// 文本型  文本2 = "abc"; 文本1.交换(文本2) //文本1 与 文本2 的内容交换 
	//@参数  将被替换的文本内容
	//@别名  交换(文本)
	void swap(CXText& right) { _text.swap( *right.getString() ); }

	//@备注  删除指定位置范围的字符串
	//@示例 文本型  文本1 = "123456"
	// 文本1.删除(3, 2) //从偏移位置3开始长度2字符, 结果为: "1236"
	//@参数  开始位置偏移, 字符为单位
	//@参数  从开始位置计算长度, 字符为单位
	//@别名  删除(开始位置, 长度)
	void erase(size_t off, size_t count=1) { _text.erase(off, count); }

	//------add---------------------------------------------
	//@备注 返回指定位置的字符, 与下标访问功能相同
	//@示例 文本型  文本1 = "123456"
	//  字符型  结果 = 文本1.取字符(3) //从偏移位置3取一个字符返回, 结果为: "4"
	//@参数 位置偏移, 字符为单位
	//@返回 返回指定位置的字符
	//@别名 取字符(位置索引)
	const wchar_t at(size_t index) const { return _text[index]; }
private:
	std::wstring  _text;
};


//@备注 文本操作类A, 字符串操作包装类, 多字节编码, 每英文字符占1个字节, 中文字符占多个字节
//@示例 文本型A  文本1 = A"多字节编码字符串"   //"A"标识为多字节编码字符串 
// 文本型A  文本2 = U"utf8字符串"  //"U"标识UTF8字符串
// 调试输出(文本1, 文本2)
//@别名 文本型A
class CXTextA
{
public:
	//@隐藏{
	CXTextA() = default;
	CXTextA(const CXTextA& _that) { _text = _that._text; }
	CXTextA(const CXText& _that);
	CXTextA(CXTextA&& _that) { _text = std::move(_that._text); }
	CXTextA(const CXBytes &_that);
	CXTextA(const std::wstring& wstring_);
	CXTextA(const std::string& string_) {_text = string_; }
	CXTextA(std::string&& string_) { _text = std::move(string_); }
	CXTextA(const char* pText, size_t length = -1);
	CXTextA(const wchar_t* pText, size_t length = -1);
	CXTextA(const int value_);
	CXTextA(const INT64 value_);
	CXTextA(const double value_);
	CXTextA(const float  value_);
	CXTextA(const char value_);
	CXTextA(const wchar_t value_);
	CXTextA(const std::initializer_list<char> list) { _text = list; }

	void operator=(const char* pText) { _text = pText ? pText : ""; }
	void operator=(const wchar_t* pText);
	void operator=(const char ch) { _text = ch; }
	void operator=(const std::wstring& wstring_);
	void operator=(const std::string& string_) { _text = string_; }
	void operator=(const CXTextA &_that) { _text = _that._text; }
	void operator=(const CXText &_that);
	void operator=(const CXBytes &_that);
	void operator=(CXTextA&& _that) { _text = std::move(_that._text);}
	CXTextA& operator+=(const CXTextA& _that) {
		_text += _that.get();
		return *this;
	}

	bool operator>(const CXTextA& right) const { return _text.compare(right._text) > 0; }
	
	bool operator<(const CXTextA& right) const { return _text.compare(right._text) < 0; }
	bool operator!=(const CXTextA& right) const { return _text.compare(right._text) != 0; }
	bool operator!=(const char* right) const { return  _text.compare(right) != 0; }

	bool operator==(const CXTextA& right) const { return _text == right._text; }
	bool operator==(const char* right) const { return  _text == right; }
	friend bool operator==(const char* left, const CXTextA& right) { return right.equal(left); }
	friend bool operator!=(const char* left, const CXTextA& right) { return (right != left); }
	friend CXTextA operator+(const char* left, const CXTextA& right) { CXTextA  text = left; text += right;	return text; }

	// 自动转换不安全, 存在难以发现的意外BUG, 所以 自动转换交给炫彩IDE做
	//explicit operator const char*() const { return _text.c_str(); } //当作为函数参数传递时, 自动转换为[const char*]
	
	explicit operator bool() const { return !_text.empty(); }

	bool equal(const CXTextA &_right) const { return _text == _right._text; }
	char& operator[](size_t index) { return _text[index]; }
	const std::string* getString() const { return &_text; }
//@隐藏}
	std::string* getString() { return &_text; }
	const char& operator[](size_t index) const { return _text[index]; }

	CXTextA operator+(const char ch) const {
		CXTextA  ret = *this;
		ret += ch;
		return ret;
	}
	CXTextA operator+(const char* pText) const {
		CXTextA  ret = *this;
		ret += pText;
		return ret;
	}
	CXTextA operator+(char* pText) const  //若是不重载, 特殊情况会报错, 例如:  文本型 + (char*)"123"
	{
		CXTextA  ret = *this;
		ret += pText;
		return ret;
	}
	CXTextA operator+(const CXTextA& right) const {
		CXTextA  ret = *this;
		ret += right;
		return ret;
	}
   	void operator+=(const char ch) { _text += ch; }
	void operator+=(const char* pText) { _text += pText; }
//@隐藏{
	//与 getPtr() 功能相同
	const char* get() const { return _text.c_str(); }

	const char* c_str() const { return _text.c_str(); }

	auto begin() { return _text.begin(); }
	auto begin() const { return _text.begin(); }
	auto end() { return _text.end(); }
	auto end() const { return _text.end(); }

	auto rbegin() { _text.rbegin(); }
	auto rbegin() const { _text.rbegin(); }

	auto rend() { _text.rend(); }
	auto rend() const { _text.rend(); }

	void push_back(const char _ch) { _text.push_back(_ch); }
	void pop_back() { _text.pop_back(); }
//@隐藏}

	//@备注  获取文本指针
	//@示例 文本型A  文本1
	// 字符型A* 字符串指针 = 文本1.取地址() //获取字符串在内存中的地址
	//@返回  字符串指针
	//@别名  取地址()
	const char* getPtr() const { return _text.c_str(); }
//@隐藏{
	//@备注  与 getSize() 功能相同
	size_t  size() const { return _text.size(); }
//@隐藏}

	//@备注  获取文本长度
	//@示例  文本型A  文本1
	// 整型 文本长度 = 文本1.取大小() //获取内容长度(字符数量),
	//@返回  文本长度, 以字符为单位
	//@别名  取大小()
	size_t getSize() const { return _text.size(); }

	//@备注  重置大小
	//@示例  文本型A  文本1
	//文本1.置大小(10) //设置长度为10个字符大小空间, 相当置空文本(填10个0)
	//@参数  内容长度, 字符为单位
	//@参数  填充字符
	//@别名  置大小(长度, 填充字符)
	void resize(size_t length, const char ch_=0) { _text.resize(length, ch_); }
	
	//@备注  预分配指定大小空间
	//@示例  文本型A  文本1
	//文本1.预置大小(256)  //提前分配大于256字符的空间,方便后面使用
	//@参数  分配空间大小, 字符为单位
	//@别名  预置大小(长度)
	void reserve(size_t length) { _text.reserve(length); }

	//@备注  判断内容是否为空
	//@示例  文本型A  文本1
	//逻辑型 内容是否为空 = 文本1.是否为空() //判断内容是否为空
	//@返回  如果内容为空返回真,否则返回假
	//@别名  是否为空()
	BOOL empty() const { return _text.empty(); }
	
	//@备注  添加文本到末尾
	//@示例  文本型A  文本1
	//文本1.添加(A"abc") //在末尾追加文本
	//@参数  字符串指针
	//@别名  添加(文本)
	void append(const char* pText) { _text.append(pText); }

	//@备注  添加指定长度的文本到末尾
	//@示例  文本型A  文本1
	//文本1.添加2(A"abc", 2) //在末尾追加文本, 并且指定只添加2个字符"ab"进去
	//@参数  字符串指针
	//@参数  字符串长度, 字符为单位
	//@别名  添加2(文本, 长度)
	void append2(const char* pText, size_t length) { _text.append(pText, length); }

	//@备注  插入文本到指定位置
	//@示例  文本型A  文本1
	//文本1.插入2(2, A"abc") //在偏移位置2插入字符串 A"abc"
	//@参数  位置偏移, 字符为单位, 插入位置
	//@参数  字符串指针
	//@别名  插入(插入位置, 文本)
	void insert(size_t off, const char* pText) { _text.insert(off, pText); }

	//@备注  将指定范围内的文本替换为新的文本
	//@示例  文本型A  文本1 = A"123456"
	//文本1.替换2(2, 10, A"abc") //从偏移位置 1 - 5 的内容 替换为 A"abc"
	////结果为: A"1abc6"
	//@参数  开始位置, 位置偏移, 字符为单位
	//@参数  结束位置, 位置偏移, 字符为单位
	//@参数  将被填入的新文本
	//@别名  替换(开始位置, 结束位置, 文本)
	void replace(size_t off1, size_t off2, const char* pText) {
		_text.replace(_text.begin() + off1, _text.begin() + off2, pText);
	}

	//@备注  将文本中指定的子字符串替换
	//@示例  文本型A  文本1 = A"123456"
	//文本1.替换子文本(A"34", A"abc") //将 A"34" 替换为 A"abc"
	//结果为: A"12abc56"
	//@参数  要被替换的子字符串
	//@参数  替换后的字符串
	//@别名  替换子文本(要被替换的子字符串, 替换后的字符串)
	void replaceSub(__in const char* pSubStr, __in const char* pReplaceStr);

	//@备注  将指定数量的文本替换为新的文本
	//@示例  文本型A  文本1 = A"123456"
	//文本1.替换指定数量(0, 5, A"abc") //从偏移位置 0 - 5 的内容替换为 A"abc"
	//结果为: A"abc6"
	//@参数  起始位置偏移, 字符为单位
	//@参数  替换文本数量, 从起始位置开始计算, 字符为单位
	//@参数  将被填入的新文本
	//@别名  替换指定数量(开始位置, 数量, 文本)
	void replaceCount(size_t off, int count, const char* pText) {
		_text.replace(off, count, pText);
	}

	//@备注  查指定字符串, 返回字符串所在位置索引
	//@示例  文本型A  文本1 = A"123456"
	//整型 位置 = 文本1.查找(A"56") //返回A"56"的偏移位置4
	//@参数  查找的字符串
	//@返回  文本所在位置索引, 字符为单位, 失败返回-1
	//@别名  查找(文本)
	int find(const char* pText) const { return (int)_text.find(pText, 0); }

	//@备注  从指定位置开始查找字符串, 返回字符串所在位置索引
	//@示例  文本型A  文本1 = A"123456123456"
	//整型 位置 = 文本1.查找2(A"56", 6) //从偏移位置6开始查找,返回10
	//@参数  查找的字符串
	//@参数  开始查找位置偏移, 字符为单位
	//@返回  文本所在位置索引, 失败返回-1
	//@别名  查找2(文本, 开始位置)
	int find2(const char* pText, size_t off) const { return (int)_text.find(pText, off); }

	//@备注  反向查指定字符串, 返回字符串所在位置索引
	//@示例  文本型A  文本1 = A"123456123456"
	//整型 位置 = 文本1.反向查找(A"56") //从后向前查找, 返回10
	//@参数  查找的字符串
	//@返回  文本所在位置索引, 字符为单位, 失败返回-1
	//@别名  反向查找(文本)
	int rfind(const char* pText) const { return (int)_text.rfind(pText); }

	//@备注  从指定位置开始反向查找字符串, 返回字符串所在位置索引
	//@示例  文本型A  文本1 = A"123456123456"
	//整型 位置 = 文本1.反向查找2(A"56", 12) //从偏移位置12向前查找, 返回10
	//@参数  查找的字符串
	//@参数  开始查找位置偏移, 字符为单位
	//@返回  文本所在位置索引, 失败返回-1
	//@别名  反向查找2(文本, 开始位置)
	int rfind2(const char* pText, size_t off = -1) const { return (int)_text.rfind(pText, off); }

	//@备注  获取指定位置范围的字符串
	//@示例  文本型A  文本1 = A"123456"
	//文本型A 结果 = 文本1.取子文本(3, 2) //从偏移位置3开始取内容长度为2(2个字符), 返回结果为: A"45"
	//@参数  开始位置偏移, 字符为单位
	//@参数  从开始位置计算长度, 字符为单位
	//@返回  获取的文本
	//@别名  取子文本(开始位置 ,长度)
	CXTextA substr(size_t off, size_t count) const;

	//@备注  取末尾字符
	//@示例  文本型A  文本1 = A"123456"
	//字符型A 结果 = 文本1.取尾字符() //获取末尾字符, 返回结果为: A'6'
	//@返回  末尾字符
	//@别名  取尾字符()
	char back() const { return _text.back(); }

	//@备注  取首字符
	//@示例  文本型A  文本1 = A"123456"
	//字符型A 结果 = 文本1.取首字符() //获取末尾字符, 返回结果为: A'1'
	//@返回  首字符
	//@别名  取首字符()
	char front() const { return _text.front(); }

	//@备注  交换两个文本对象内容
	//@示例  文本型A  文本1 = A"123456"
	//文本型A  文本2 = A"abc"; 文本1.交换(文本2) //文本1 与 文本2 的内容交换 
	//@参数  将被替换的文本内容
	//@别名  交换(文本)
	void swap(CXTextA& right) { _text.swap(*right.getString()); }

	//@备注  删除指定位置范围的字符串
	//@示例  文本型A  文本1 = A"123456"
	//文本1.删除(3, 2) //从偏移位置3开始长度2字符, 结果为: A"1236"
	//@参数  开始位置偏移, 字符为单位
	//@参数  从开始位置计算长度, 字符为单位
	//@别名  删除(开始位置, 长度)
	void erase(size_t off, int count = 1) { _text.erase(off, count); }

	//@备注  返回指定位置的字符, 与下标访问功能相同
	//@示例  文本型A  文本1 = A"123456"
	//字符型A  结果 = 文本1.取字符(3) //从偏移位置3取一个字符返回, 结果为: A'4'
	//@参数  位置偏移, 字符为单位
	//@返回  返回指定位置的字符
	//@别名  取字符(位置索引)
	const char at(size_t index) const { return _text[index]; }
private:
	std::string  _text;
};

//@备注  动态数组操作类, 非静态数组, 支持 添加 删除 修改等操作
//@doc "doc\vector_class.txt"
//@别名 动态数组
template<class T>
class CXVector
{
public:
//@隐藏{
	CXVector() = default;
	CXVector(CXVector&& _that) { _list = std::move(_that._list); }
	CXVector(const CXVector& _that) { _list = _that._list; }
	CXVector(const std::initializer_list<T> list) { _list = list; }
	CXVector(size_t size_) { _list.resize(size_); }

	CXVector(const std::vector<T>& list) { _list = list; }
	CXVector(const std::vector<T>&& list) { _list = std::move(list); }

	void operator=(const CXVector& _that) { _list = _that._list; }
	void operator=(const std::vector<T>& list) { _list = list; }
	void operator=(const std::vector<T>&& list) { _list = std::move(list); }

	void operator+=(const CXVector& _that) {_list.insert(_list.begin() + _list.size(), _that._list.begin(), _that._list.end());}
	bool operator==(const CXVector<T>& right) { return _list == right._list; }
	bool operator!=(const CXVector<T>& right) { return _list != right._list; }

	CXVector operator+(const CXVector& _that) {
		CXVector<T> ret = *this;
		ret += _that;
		return ret;
	}

	template<int n>
	bool operator==(const T(&_that)[n]) {
		if (_list.size() != n) return FALSE;
		return std::equal(_list.begin(), _list.end(), _that);
	}
	template<int n>
	bool operator!=(const T(&_that)[n]) {
		if (_list.size() != n) return TRUE;
		return !std::equal(_list.begin(), _list.end(), _that);
	}
	explicit operator bool() const { return !_list.empty(); }
	const T& operator[](size_t index) const { return _list[index]; }
	const std::vector<T>* getVector() const { return &_list; }
//@隐藏}
 	T& operator[](size_t index) { return _list[index]; }
	std::vector<T>* getVector() { return &_list; }

	//@备注  取第一个成员
	//@示例  动态数组<整型>  数组1 = {1,2,3,4,5,6}; 整型 结果 = 数组1.取首()  //取第一个成员, 返回结果: 1
	//@返回  第一个成员
	//@别名  取首()
	T& front() { return  _list.front(); }

	//@备注  取末尾成员
	//@示例  动态数组<整型>  数组1 = {1,2,3,4,5,6}; 整型 结果 = 数组1.取尾()  //取取末尾成员, 返回结果: 6
	//@别名  取尾()
	T& back() { return  _list.back(); }

	//@备注  置成员数量, 原有数据不变, 超出部分被截断
	//@示例  动态数组<整型>  数组1; 数组1.置大小(100) //调整数组大小为100个成员
	//@参数  成员数量
	//@别名  置大小(数量)
	void resize(int count) { _list.resize(count); }
//@隐藏{
	//@备注  与 getSize() 功能相同
	int size() const { return (int)_list.size(); }

	T& at(const size_t index) { return _list.at(index); }
	const T& at(const size_t index) const { return _list.at(index); }

	auto begin() { return _list.begin(); }
	auto begin() const { return _list.begin(); }
	auto end() { return _list.end(); }
	auto end() const { return _list.end(); }

	auto rbegin() { _list.rbegin(); }
	auto rbegin() const { _list.rbegin(); }

	auto rend() { _list.rend(); }
	auto rend() const { _list.rend(); }

	void push_back(const T& _Val) { _list.push_back(_Val); }
	void pop_back() { _list.pop_back(); }
	void reserve(const size_t _Newcapacity) { _list.reserve(_Newcapacity); }
	void assign(const size_t _Newsize, const T& _Val) { _list.assign(_Newsize, _Val); }

//@隐藏}

	//@备注  取成员数量
	//@示例  动态数组<整型>  数组1
	// 整型  数组大小 = 数组1.取大小() //获取数组成员数量 
	//@返回  成员数量
	//@别名  取大小()
	size_t getSize() const { return _list.size(); }

	//@备注  添加成员
	//@示例  动态数组<整型>  数组1
	// 数组1.添加(10)
	//@参数  将要添加的成员
	//@别名  添加(值)
	void add(const T& value) { _list.push_back(value);}
	//@隐藏{
	void add(const T&& value) { _list.push_back(value); }
	//@隐藏}
	
	//@备注  插入成员到指定位置
	//@示例  动态数组<整型>  数组1 = {1,2,3,4,5,6}
	// 数组1.插入(3, 100) //在下标3位置插入100, 结果为 : {1,2,3,100,4,5,6}
	//@参数  插入位置索引
	//@参数  插入内容
	//@别名  插入(插入位置索引, 插入值)
	void insert(size_t off, T& value) { _list.insert(_list.begin() + off, value); }
	//@隐藏{
	void insert(size_t off, T&& value) { _list.insert(_list.begin() + off, value); }
	//@隐藏}

	//@备注 弹出末尾成员
	//@示例 动态数组<整型>  数组1 = {1,2,3,4,5,6}
	// 数组1.弹出() //结果为: {1,2,3,4,5}
	//@别名 弹出()
	void popBack() {_list.pop_back();}

	//@备注  获取数据地址,  与 取地址() 功能相同
	//@示例  动态数组<整型>  数组1
	// 整型  *内存指针 = 数组1.取数据() //因为成员为 整型, 所以这里是整型指针
	//@返回  数据地址
	//@别名  取数据()
	const T* data() const { return _list.data(); }

	//@备注  获取数据地址, 与 取数据() 功能相同
	//@示例  动态数组<整型>  数组1
	// 常量  字节型*  内存指针 = 数组1.取地址()
	//@返回  数据地址
	//@别名  取地址()
	const BYTE* getPtr() const { return (const BYTE*)_list.data(); }

	//@备注  返回索引对应成员值
	//@示例  动态数组<整型>  数组1 = {1,2,3,4,5,6}
	// 整型  结果 = 数组1.取成员(3) //结果为:4
	//@参数  位置索引
	//@返回  成员值
	//@别名  取成员(位置索引)
	T& get(size_t index) {return _list[index];}
	//@隐藏{
	const T& get(size_t index) const { return _list[index]; }
	//@隐藏}
	
	//@备注  设置索引对应成员值
	//@示例  动态数组<整型>  数组1 = {1,2,3,4,5,6}
	// 数组1.置成员(3, 100) //结果为:{1,2,3,100,5,6}
	//@参数  位置索引
	//@参数  填充值
	//@别名  置成员(位置索引, 值)
	void set(size_t off, T& value) { _list[off] = value; }
	//@隐藏{
	void set(size_t off, T&& value) { _list[off] = value; }
	//@隐藏}

	//@备注  交互两个动态数组数据
	//@示例  动态数组<整型>  数组1
	// 动态数组<整型>  数组2
	// 数组1.交换(数组2) //数组1 与 数组2 内容交换
	//@参数  将被替换动态数组对象
	//@别名  交换(动态数组)
	void swap(CXVector<T>& right) { _list.swap(*right.getVector()); }

	//@备注  判断是否为空, 当数组没有成员时为空
	//@示例  动态数组<整型>  数组1
	// 整型  是否为空 = 数组1.是否为空()
	//@返回  如果为空返回真, 否则返回假
	//@别名  是否为空()
	BOOL empty() const { return _list.empty(); }

	//@备注  删除指定位置成员, 可指定删除数量
	//@示例  动态数组<整型>  数组1 ={1,2,3,4,5,6}
	// 数组1.删除(3) //删除索引为3的成员, 结果为:{1,2,3,5,6}
	// 动态数组<整型>  数组2 ={1,2,3,4,5,6}
	// 数组2.删除(3, 2) //从索引3开始 删除2个成员, 结果为:{1,2,3,6}
	//@参数  成员位置索引
	//@参数  要删除的成员数量, 默认值为1删除参数1指定位置的成员
	//@别名  删除(位置索引, 删除数量)
	void erase(size_t off, size_t count = 1) {
		if (1 == count) _list.erase(_list.begin() + off);
		else  _list.erase(_list.begin() + off, _list.begin() + off + count);
	}

	//@备注  删除成员范围, 从起始索引到结束索引
	//@示例  动态数组<整型>  数组1 ={1,2,3,4,5,6}
	// 数组1.删除范围(3, 5) //删除索引范围(3-5)内容, 结果为:{1,2,3,6}
	//@参数  起始位置索引
	//@参数  结束位置索引
	//@别名  删除范围(起始索引, 结束索引)
	void erase2(size_t first_, size_t last_) { _list.erase(_list.begin() + first_, _list.begin() + last_); }

	//@备注  清空内容
	//@示例  动态数组<整型>  数组1 = {1,2,3,4,5,6}
	// 数组1.清空()  //清空成员, 结果成员数为0
	//@别名  清空()
	void clear() { _list.clear(); }
private:
	std::vector<T>  _list;
};


//-------------map--------------------------------
/*@声明
//@备注 用于字典map的迭代器
//@doc "doc\map_class.txt"
//@别名 迭代器
class cx_iterator
{
	//@备注 字典成员对应的键
	void first;   //@别名  键
	//@备注 字典成员对应的值
	void second;  //@别名  值
};
*/

//@备注 字典-键值表; 字典<键,值>, 每个键都是唯一, 每个键对应一个值, 用于键值存储, 快速查找 
//@doc "doc\map_class.txt"
//@别名 字典
template<class K, class V>
class CXMap
{
public:
	//@隐藏{
	typedef typename  std::map<K, V>::iterator  iterator;
	CXMap() = default;
	CXMap(CXMap&& _that) { _map = std::move(_that._map); }
	CXMap(CXMap& _that) { _map = _that._map; }
	CXMap(std::initializer_list<typename std::map<K, V>::value_type> list) { _map = list; }

	void operator=(const CXMap& _that) { _map = _that._map; }
	CXMap operator+(const CXMap& _that) {
		CXMap<K, V> ret = *this;
		ret += _that;
		return ret;
	}
	void operator+=(const CXMap& _that) {
		for (auto var : _that._map)
			_map[var.first] = var.second;
	}
	bool operator==(const  CXMap& _that) {
		return _map == _that._map;
	}
	bool operator!=(const  CXMap& _that) {
		return _map != _that._map;
	}
	explicit operator bool() const { return !_map.empty(); }

	//与getSize()功能相同
	size_t size() const { return _map.size(); }

	auto rbegin() { _map.rbegin(); }
	auto rbegin() const { _map.rbegin(); }

	auto rend() { _map.rend(); }
	auto rend() const { _map.rend(); }

	auto find(const K& key_) const { return _map.find(key_); }
	auto begin() const { return _map.begin(); }
	auto end() const { return _map.end(); }

	void swap(CXMap& _Right) { _map.swap(_Right); }

	V& at(const K& _Keyval) { _map.swap(_Keyval); }
	const V& at(const K& _Keyval)const { _map.swap(_Keyval); }
	const std::map<K, V>* getMap() const { return &_map; }
//@隐藏}
	std::map<K, V>* getMap() { return &_map; }
	V& operator[](const K& key_){ return _map[key_];}

	//@备注  获取成员数量, https://jmp6bo524n.k.topthink.com/@xcdoc/shujurongqi.html
	//@示例  字典<整型,整型>  字典1
	//      整型  成员数量 = 字典1.取大小()
	//@返回  成员数量
	//@别名  取大小()
	size_t getSize() const { return _map.size(); }

	//@备注  查找指定键对应的值
	//@doc   "doc/map_find.txt"
	//@参数  查找的键
	//@返回  查找结果迭代器
	//@别名  查找(键)
	cx_iterator find(const K& key_) { return _map.find(key_); }

	//@备注  查找指定键对应的值
	//@示例 	字典<文本型, 整型>  字典1
	// 字典1["aaa"] = 1
	// 整型 结果 = 字典1.查找值("aaa", 0) //参数2指定默认值, 如果没有查找到返回默认值
	//@参数  查找的键
	//@参数  如果没有查找到返回的默认值
	//@返回  返回值
	//@别名  查找值(键, 默认值)
	V findValue(const K&  key_, const V& defaultValue) const {
		auto  pIter = _map.find(key_);
		if (pIter != _map.end())
			return  pIter->second;
		return defaultValue;
	}

	//@备注  查找指定键对应的值
	//@示例 字典<文本型, 整型>  字典1
	// 字典1["aaa"] = 1
	// 整型  结果 = 0
	// 逻辑型 是否成功 = 字典1.查找值2("aaa", 结果) //通过参数返回结果, 通过返回值判断是否成功
	//@参数  查找的键
	//@参数  接收返回值
	//@返回  如果存在返回真, 否则返回假
	//@别名  查找值2(键, 接收返回值)
	BOOL findValue2(const K&  key_, V &outValue) const { 
		auto  pIter = _map.find(key_);
		if (pIter != _map.end())
		{
			outValue = pIter->second;
			return TRUE;
		}
		return FALSE;
	}

	//@备注  判断指定键是否存在
	//@示例  字典<文本型, 整型>  字典1
	// 字典1["aaa"] = 1
	// 逻辑型 是否存在 = 字典1.是否存在("aaa")
	//@参数  查找的键
	//@返回  如果存在返回真,否则返回假
	//@别名  是否存在(键)
	BOOL isExist(const K& key_) const { return (_map.find(key_) != _map.end()); }
	//BOOL isExist(K key_) const { return (_map.find(key_) != _map.end()); }

	//@备注  获取第一成员迭代器
	//@示例  字典<文本型, 整型>  字典1
	// 字典1["aaa"] = 1
	// 字典1["bbb"] = 2
	// 迭代器  查找 = 字典1.第一个()
	// 调试输出( 查找.键, " | ", 查找.值)
	//@返回  结果迭代器
	//@别名  第一个()
	cx_iterator begin() { return _map.begin(); }

	//@备注  获取末尾的下一个,空值
	//@示例  字典<文本型, 整型>  字典1
	// 字典1["aaa"] = 1
	// 如果 (字典1.空值() = 字典1.查找("aaa"))
	// -- 调试输出("未找到")
	// 否则
	// -- 调试输出("找到")
	//@返回  结果迭代器
	//@别名  空值()
	cx_iterator end() { return _map.end(); }

	//@备注  判断内容是否为空
	//@示例  字典<文本型, 整型>  字典1
	// 逻辑型 是否为空 = 字典1.是否为空()
	//@返回  如果内容为空返回真, 否则返回假
	//@别名  是否为空()
	BOOL empty() const { return _map.empty(); }

	//@备注  删除指定成员
	//@示例  字典<文本型, 整型>  字典1
	// 字典1["aaa"] = 1
	// 字典1.删除("aaa")
	//@参数  要删除成员对应的键
	//@返回  下一个成员对应迭代器
	//@别名  删除(键)
	cx_iterator erase(const K& key_) { return _map.erase(key_); }

	//@备注  清空内容
	//@示例 字典<文本型, 整型>  字典1
	// 字典1["aaa"] = 1
	// 字典1.清空()
	//@别名  清空()
	void clear() { _map.clear(); }
private:
	std::map<K, V>  _map;
};

//@备注 先进先出队列, 一端进入,另一端弹出(末端进入, 前端弹出)
//自动加锁操作, 支持多线程操作
//@doc "doc\queue_class.txt"
//@别名 队列类
template<class T>
class CXQueue
{
public:
//@隐藏{
	std::queue<T>     m_task;  //任务队列
	CRITICAL_SECTION  m_task_cs;
	BOOL  _isInit;
	CXQueue(){
		_isInit = FALSE;
	}
	~CXQueue(){
		if (_isInit)
		{
			_isInit = FALSE;
			DeleteCriticalSection(&m_task_cs);
		}
	}
	size_t size() { m_task.size(); }
//@隐藏}
	std::queue<T>* getQueue() { return m_task; }

	//@备注  使用前需要先初始化, 以便支持多线程访问保护
	//@示例  队列类<整型>  队列1
	// 队列1.初始化()
	//@别名  初始化()
	void init() {
		if (!_isInit)
		{
			_isInit = TRUE;
			InitializeCriticalSection(&m_task_cs);
		}
	}
	//@备注  释放操作,  如果之后再使用需要再初始化
	//@别名  释放()
	void release() {
		if (_isInit)
		{
			_isInit = FALSE;
			DeleteCriticalSection(&m_task_cs);
		}
	}
	//@备注 加入一个成员到队列末尾
	//@示例 队列类<整型>  队列1
	// 队列1.加入(100)
	//@参数 要加入的成员
	//@别名 加入()
	void push(T pTask)
	{
		if (_isInit)
		{
			EnterCriticalSection(&m_task_cs);
			m_task.push(pTask);
			LeaveCriticalSection(&m_task_cs);
		}
	}
	//@备注 弹出队列的第一个成员
	//@示例 队列类<整型>  队列1
	// 队列1.加入(100)
	// 队列1.加入(200)
	// 整型 结果 = 队列1.弹出()
	//@返回 返回弹出的成员
	//@别名 弹出()
	T pop()
	{
		T pTask = {0};
		if (_isInit)
		{
			EnterCriticalSection(&m_task_cs);
			if (!m_task.empty())
			{
				pTask = m_task.front();
				m_task.pop();
			}
			LeaveCriticalSection(&m_task_cs);
		}
		return pTask;
	}
	//@备注  判断队列是否为空
	// @示例 队列类<整型>  队列1
	// 逻辑型  是否为空 = 队列1.是否为空()
	//@返回  如果为空返回真, 否则返回假
	//@别名  是否为空()
	BOOL empty()
	{
		BOOL bEmpty = TRUE;
		if (_isInit)
		{
			EnterCriticalSection(&m_task_cs);
			bEmpty = m_task.empty();
			LeaveCriticalSection(&m_task_cs);
		}
		return bEmpty;
	}
	//@备注 清空队列成员
	//@别名 清空()
	void clear(){
		if (_isInit)
		{
			EnterCriticalSection(&m_task_cs);
			while (!m_task.empty())
				m_task.pop();
			LeaveCriticalSection(&m_task_cs);
		} else
		{
			while (!m_task.empty())
				m_task.pop();
		}
	}
	//@备注 取成员数量
	// @示例 队列类<整型>  队列1
	// 整型 成员数量 = 队列1.取大小()
	//@别名 取大小()
	size_t getSize()
	{
		size_t size_ = 0;
		if (_isInit)
		{
			EnterCriticalSection(&m_task_cs);
			size_ = m_task.size();
			LeaveCriticalSection(&m_task_cs);
		}
		return size_;
	}
	//@备注 交换数据
	// @示例 队列类<整型>  队列1, 队列2
	// 队列1.交换(队列2)
	//@别名 交换()
	void swap(CXQueue& right)
	{
		if (_isInit)
		{
			EnterCriticalSection(&m_task_cs);
			m_task.swap(right.m_task);
			LeaveCriticalSection(&m_task_cs);
		}
	}
};

//@备注 对象构造中添加了初始化, 不需要手动调用 初始化() 函数
//@doc  "doc\queueEx_class.txt"
//@别名 队列扩展类
template<class T>
class CXQueueEx : public CXQueue<T>
{
public:
	CXQueueEx(){
		CXQueue::init();
	}
};

//@分组}   核心组


//-------------------------------------------

//@分组{  外部基础类型
/*@声明

DECLARE_HANDLEX(HINSTANCE); //@备注 表示应用程序实例的句柄，用于标识正在运行的应用程序实例
DECLARE_HANDLEX(HCURSOR);   //@备注 用于表示鼠标光标的句柄，可用于设置和操作鼠标指针的外观和行为
DECLARE_HANDLEX(HFONT);     //@备注 表示字体的句柄，用于在绘制文本时指定字体样式、大小等属性
DECLARE_HANDLEX(HDC);       //@备注 设备上下文句柄，提供了一个绘制设备的接口，如屏幕、打印机等，用于进行图形绘制操作
DECLARE_HANDLEX(HBRUSH);    //@备注 画刷句柄，用于填充图形区域，可指定颜色、图案等填充效果
DECLARE_HANDLEX(HPEN);      //@备注 画笔句柄，用于绘制线条和轮廓，可设置线条的颜色、宽度和样式等
DECLARE_HANDLEX(HRGN);      //@备注 区域句柄，用于表示一个图形区域，可用于裁剪、命中测试等操作
DECLARE_HANDLEX(HWND);      //@备注 系统窗口句柄(HWND), 非炫彩窗口句柄(HWINDOW)，用于唯一标识系统中的窗口，通过该句柄可以对窗口进行操作，如移动、显示、隐藏等
DECLARE_HANDLEX(HMODULE);   //@备注 模块句柄，通常表示可执行文件或动态链接库（DLL）的模块，可用于加载、卸载和访问模块中的资源
DECLARE_HANDLEX(FARPROC);   //@备注 远程过程句柄，通常用于调用动态链接库（DLL）中的函数，存储函数的地址
DECLARE_HANDLEX(HICON);     //@备注 图标句柄，用于表示图标资源，可用于显示应用程序、窗口或对象的图标

//@备注 通用句柄，可用于表示各种对象，具体对象取决于上下文，例如文件句柄、进程句柄、线程句柄等。
//它是一种不明确的句柄类型，根据使用情况可能代表不同的系统资源或对象，需要根据相应的 API 调用和程序逻辑来确定其具体指代的对象。
//@别名  句柄型
DECLARE_HANDLEX(HANDLE);

//兼容C++类型

//@备注 空类型指针; LPVOID(void*) 是一个指针类型，通常表示指向未知类型的指针。
//常用于 Windows 编程中，在不同的上下文中可以被强制转换为指向具体数据类型的指针，方便处理各种数据类型的指针，例如传递给函数或存储在变量中。
//@示例 LPVOID  data= NULL //LPVOID 等于 void* 等于 空类型*
//整型*  数据地址 = (整型*)data //转换到实际类型指针, 例如整型指针
typedef LPVOID LPVOID;

//@备注 空类型; VOID(void) 表示无类型，通常用于表示函数的返回值为空，或者作为函数参数，表示该参数不接收任何数据，在某些情况下也可以作为一种通用的数据类型，但不能存储具体的数据。
//@示例 VOID*  data = NULL //VOID 等于 void 等于 空类型
//整型*  数据地址 = (整型*)data //转换到实际类型指针, 例如整型指针
typedef VOID   VOID;
typedef DWORD  DWORD;  //@备注 DWORD 是一个无符号的 32 位整数类型，在 Windows 编程中广泛使用，用于存储各种计数、大小、标志等信息，它可以表示较大范围的正整数。
typedef WORD   WORD;   //@备注 WORD 是一个无符号的 16 位整数类型，适用于存储较小范围的正整数，例如字符编码、位图的宽度或高度等信息，其范围比 DWORD 小。
typedef LONG   LONG;   //@备注 LONG 是一个有符号的 32 位整数类型，可用于存储正负整数，常用于表示长度、偏移量等信息，其范围比 WORD 更宽，能处理更广泛的数值。
typedef long   long;   //@备注 long 是 C/C++ 中的标准有符号整数类型，通常是 32 位，具体取决于编译器和目标平台，用于存储整数，可处理较大范围的正负数值。
typedef size_t  size_t; //@备注 size_t 是 C/C++ 标准库中定义的一种无符号整数类型, 32位程序中4字节, 64位程序中8字节

typedef  FILE    FILE;    //@备注  文件指针类型, 例如: FILE*  file = NULL
typedef  time_t  time_t;  //@备注  32对应 long; 64位对应 __int64
*/
//@分组}

//@分组{  数据结构
/*@声明

//@备注  结构体包含宽度和高度, 可以在定义变量时直接初始化值;
//@示例  大小结构 大小 = {20,30}
// 大小.宽度 = 100
// 大小.高度=200;
//@别名 大小结构
struct SIZE
{
	int cx; //@别名 宽度
	int cy; //@别名 高度
};

//@备注 指定对象的位置(X,Y), 可以在定义变量时直接初始化值;
//@示例 点结构  位置 = {20,30}
// 位置.x = 100
// 位置.y=200;
//@别名 点结构
struct POINT
{
	int x;   //X坐标
	int y;   //Y坐标
};

//@备注 指定对象的矩形坐标, 可以在定义变量时直接初始化值;
//@示例 矩形结构  位置 = {10,10, 100,100}
// 位置.左边 = 20
// 位置.上边=20;
//@别名 矩形结构
struct RECT
{
	int left;    //@别名 左边
	int top;     //@别名 上边
	int right;   //@别名 右边
	int bottom;  //@别名 下边
};

//@备注  用于表示全局唯一标识符（Globally Unique Identifier，简称 GUID）的结构体
//GUID 是一种用于在计算机系统中唯一标识对象或信息的标识符，它在不同的系统和应用程序中被广泛使用，以确保唯一性
//在 Windows 系统中，GUID 常用于各种对象的标识，例如 COM（Component Object Model）组件、注册表项、文件标识符等。通过使用这个结构体，可以创建、存储和操作 GUID，以确保在不同系统和环境下，对象的标识具有唯一性，避免了标识符冲突的问题
//通常会使用相应的 Windows API 函数来生成、操作和比较 GUID，例如 CoCreateGuid 函数可以创建一个新的 GUID;
//@示例 GUID myGuid = {0x12345678, 0x9ABC, 0xDEF0, {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}};
struct GUID
{
	unsigned long  Data1;     //@备注 存储 GUID 的第一部分，是一个 32 位无符号长整数，用于表示 GUID 的一部分信息。
	unsigned short Data2;     //@备注 存储 GUID 的第二部分，是一个 16 位无符号短整数，作为 GUID 的一部分。
	unsigned short Data3;     //@备注 存储 GUID 的第三部分，也是一个 16 位无符号短整数，构成 GUID 的组成部分
	unsigned char  Data4[8];  //@备注 存储 GUID 的最后一部分，是一个 8 字节的无符号字符数组，包含了 GUID 的剩余信息。
};

//@备注  在 Windows 操作系统的进程管理中经常使用。它通常与 CreateProcess 函数一起使用，CreateProcess 函数用于创建一个新的进程，并将新进程和其主线程的相关信息存储在 PROCESS_INFORMATION 结构体中;
//@示例  PROCESS_INFORMATION pi
// STARTUPINFO si = { sizeof(STARTUPINFO) }
// CreateProcess(NULL, "notepad.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)
//@别名  进程信息
struct PROCESS_INFORMATION
{
	HANDLE hProcess;   //@备注 该成员存储新创建进程的句柄。通过这个句柄，可以对进程进行各种操作，如等待进程结束（WaitForSingleObject）、终止进程（TerminateProcess）等。它是一个 HANDLE 类型，是 Windows 操作系统中用于标识进程的抽象句柄，程序可以使用它与进程进行交互。
	HANDLE hThread;    //@备注 存储新创建进程的主线程的句柄。可以使用这个句柄来操作进程的主线程，例如等待线程结束（WaitForSingleObject）或挂起/恢复线程（SuspendThread、ResumeThread）。HANDLE 类型，用于标识线程。
	DWORD dwProcessId; //@备注 存储新创建进程的唯一标识符（PID）。PID 是操作系统分配给进程的唯一数字标识，可用于进程的管理和监控，例如在任务管理器中查看进程的 PID，通过 PID 可以向进程发送信号或进行进程间通信。
	DWORD dwThreadId;  //@备注 存储新创建进程的主线程的唯一标识符（TID）。类似于进程 ID，用于唯一标识线程，方便对线程进行操作和管理，例如在多线程程序中，根据 TID 可以对线程进行调度和同步。
};

//@备注  在 Windows 操作系统中用于指定进程启动时的一些信息，通常与 CreateProcess 函数一起使用，以控制新创建进程的初始状态和环境。
//@示例  PROCESS_INFORMATION pi
// STARTUPINFO si = { sizeof(STARTUPINFO) }
// CreateProcess(NULL, "notepad.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)
struct STARTUPINFOW {
	DWORD   cb; //@备注 该结构体的大小，使用时通常初始化为 sizeof(STARTUPINFOW)，以便 Windows API 函数进行参数验证和确保版本兼容性。
	LPWSTR  lpReserved; //@备注 保留字段，通常设置为 NULL，用于将来可能的扩展，当前不使用。
	LPWSTR  lpDesktop; //@备注 新进程使用的桌面名称。若为 NULL，则使用父进程的桌面。可用于在 Windows 系统中，让新进程运行在不同的桌面环境下，如在多用户环境或虚拟桌面场景中使用。
	LPWSTR  lpTitle; //@备注 为新进程的控制台窗口设置标题，仅当新进程是控制台进程且创建新控制台窗口时有效，一般设为 NULL。对于 GUI 进程通常不使用此参数。
	DWORD   dwX; //@备注 新进程窗口的初始水平位置，以像素为单位。若设置为 CW_USEDEFAULT，则使用默认位置。可精确控制新进程窗口在屏幕上的水平起始位置。
	DWORD   dwY; //@备注 新进程窗口的初始垂直位置，以像素为单位。若设置为 CW_USEDEFAULT，则使用默认位置。可精确控制新进程窗口在屏幕上的垂直起始位置。
	DWORD   dwXSize; //@备注 新进程窗口的初始宽度，以像素为单位。若设置为 CW_USEDEFAULT，则使用默认大小。可精确设置新进程窗口的初始宽度。
	DWORD   dwYSize; //@备注 新进程窗口的初始高度，以像素为单位。若设置为 CW_USEDEFAULT，则使用默认大小。可精确设置新进程窗口的初始高度。
	DWORD   dwXCountChars; //@备注 控制台窗口的屏幕缓冲区宽度，以字符为单位，仅对控制台进程有效。当创建新控制台窗口时使用，一般为 0 表示使用默认值。
	DWORD   dwYCountChars; //@备注 控制台窗口的屏幕缓冲区高度，以字符为单位，仅对控制台进程有效。当创建新控制台窗口时使用，一般为 0 表示使用默认值。
	DWORD   dwFillAttribute; //@备注 控制台窗口的初始文本和背景颜色，仅对控制台进程有效。可使用预定义的控制台颜色属性组合，如 FOREGROUND_RED | BACKGROUND_BLUE 等。
	DWORD   dwFlags; //@备注 标志字段，用于指定哪些 STARTUPINFOW 成员是有效的。例如，STARTF_USESIZE 表示使用 dwXSize 和 dwYSize，STARTF_USEPOSITION 表示使用 dwX 和 dwY 等。可通过设置不同的标志组合启用相应成员的设置。
	WORD    wShowWindow; //@备注 新进程窗口的显示状态。例如，SW_SHOW 表示显示窗口，SW_HIDE 表示隐藏窗口，可控制新进程窗口的显示和隐藏状态。
	WORD    cbReserved2; //@备注 保留字段，通常设为 0，为未来的扩展预留。
	LPBYTE  lpReserved2; //@备注 保留字段，通常设为 NULL，为未来的扩展预留。
	HANDLE  hStdInput; //@备注 新进程的标准输入句柄。可将其重定向，如将其设置为文件句柄或管道句柄，以改变新进程的输入源。
	HANDLE  hStdOutput; //@备注 新进程的标准输出句柄。可将其重定向，如将其设置为文件句柄或管道句柄，以改变新进程的输出目标。
	HANDLE  hStdError; //@备注 新进程的标准错误句柄。可将其重定向，如将其设置为文件句柄或管道句柄，以改变新进程的错误输出目标。
};

*/
//@分组}


//@分组{ 数学计算

/*@声明

//@备注  比较参数1与参数2, 返回最大的一个
//@别名 取最大值()
template<class T> T xcl_getMax(T a, T b);

//@备注  比较参数1与参数2, 返回最小的一个
//@别名 取最小值()
template<class T> T xcl_getMin(T a, T b);

*/

//@隐藏{
#define xcl_getMax(a, b)   (((a) > (b)) ? (a) : (b))
#define xcl_getMin(a, b)   (((a) < (b)) ? (a) : (b))
//@隐藏}

//@备注  为随机数生成器初始化一个种子值，不同的种子值将导致“取随机数”命令返回不同的随机数系列
//@doc "doc\xcl_setSrand.txt"
//@别名  置随机数种子(种子数值)
void xcl_setSrand(int seed);

//@备注  返回一个指定范围内的随机数值。在使用本命令取一系列的随机数之前，应该先使用“置随机数种子”命令为随机数生成器初始化一个种子值
//@doc "doc\xcl_setSrand.txt"
//@别名  取随机数(最小值, 最大值)
int  xcl_getRand(int minv, int maxv);

//以下为C++标准库的cmath库的直接定义
//参见 https://learn.microsoft.com/zh-cn/cpp/standard-library/cmath?view=msvc-170

/*@声明
//@备注  计算输入值的反余弦值。反余弦函数是余弦函数的反函数, 它可以根据一个余弦值计算出对应的角度（以弧度为单位）
//@doc  "doc\acos.txt"
//@参数  余弦值(double, float), 表示一个余弦值，该值的有效范围必须在 [-1, 1] 区间内;
//这是因为余弦函数的值域是 [-1, 1]，反余弦函数作为其反函数，定义域就是 [-1, 1];
//@返回  返回一个 T 类型的值，代表输入余弦值对应的反余弦值，单位是弧度;
//返回值的范围是 [0, π] 弧度（约等于 [0, 3.14159]），这是反余弦函数的值域;
//@别名   取反余弦(余弦值)
template<class T> T acos(T x);

//@备注  计算给定参数的反正弦值
//@doc  "doc\asin.txt"
//@参数  表示正弦值(double, float)，其取值范围必须在 [-1, 1] 之间
//@返回  返回一个 T 类型的数值，表示 x 的反正弦值，单位为弧度，返回值的范围是 [-π/2, π/2]
//@别名  取反正弦(正弦值)
template<class T> T asin(T value);

//@备注  计算参数 x 的反正切值。反正切函数是正切函数的反函数 用于根据正切值来求出对应的角度
//若需要将结果转换为角度值，可使用公式：角度值 = 弧度值 * (180 / π)。
//@doc "doc\atan.txt"
//@参数 表示正切值(double, float)，该参数可以是任意实数
//@返回  返回一个 T 类型的数值，表示 x 的反正切值，单位为弧度;
//返回值的范围是 (-π/2, π/2) ，即约 (-1.5708, 1.5708)
//@别名   取反正切(正切值)
template<class T> T atan(T x);

//@备注  计算二维平面上点 (x, y) 相对于原点的极角（即从正 x 轴逆时针旋转到该点所形成的角度）
//此函数能够根据坐标的正负精确确定角度所在的象限，是对 atan 函数的扩展
//@doc  "doc\atan2.txt"
//@参数  表示点的纵坐标(double, float)。
//@参数  表示点的横坐标(double, float)。
//@返回  返回一个 T 类型的数值，表示点 (x, y) 相对于原点的极角，单位为弧度。
//返回值的范围是 (-π, π]，即约 (-3.14159, 3.14159]。
//@别名   取反正切2()
template<class T> T atan2(T y, T x);

//@备注  计算给定角度的余弦值。在数学中，余弦函数描述了直角三角形中邻边与斜边的比值关系,
//或者在单位圆上，给定角度对应的横坐标值
//@doc  "doc\cos.txt"
//@参数  表示角度(double, float)，单位为弧度。该参数可以是任意实数，因为余弦函数的定义域是全体实数
//@返回  返回一个 T 类型的数值，表示输入角度 x 的余弦值。返回值的范围是 [-1, 1],
//这是因为在单位圆上，任何角度对应的横坐标值都在 -1 到 1 之间
//@别名   取余弦(角度)
template<class T> T cos(T x);

//@备注  计算给定角度的正弦值。在数学里，正弦函数体现了直角三角形中对边与斜边的比例关系,
//或者在单位圆上，代表给定角度所对应的纵坐标值
//@doc   "doc\sin.txt"
//@参数  代表角度(double, float)，单位为弧度。由于正弦函数的定义域是全体实数, 所以该参数可以取任意实数值
//@返回  返回一个 T 类型的数值，表示输入角度 x 的正弦值。返回值的范围是 [-1, 1],
//这是因为在单位圆上，任意角度对应的纵坐标值都处于 -1 到 1 这个区间内
//@别名  取正弦(角度)
template<class T> T sin(T x);

//@备注  计算给定角度的正切值。在数学中，正切函数定义为直角三角形中对边与邻边的比值
//在单位圆的情境下，它描述了特定角度对应的正切线的长度
//@doc  "doc\tan.txt"
//@参数  表示角度(double, float)，单位为弧度。该参数的取值范围是除了 (2n + 1) * π/2（n 为整数）之外的所有实数,
//因为在这些点上正切函数无定义（此时邻边长度为 0）
//@返回  返回一个 T 类型的数值，表示输入角度 x 的正切值。返回值的范围是负无穷到正无穷，即 (-∞, +∞)
//@别名   取正切(弧度值)
template<class T> T tan(T x);

//@备注  计算双曲反余弦值。双曲反余弦函数是双曲余弦函数的反函数，用于根据双曲余弦函数的结果，反向求出对应的输入值
//@示例  双浮点型 input = 2.0
//	双浮点型 result = 取双曲反余弦(input)
//	调试输出格式化("acosh(%f) 的结果是 %f\n", input, result)
//@参数  双曲余弦值(double, float), 该参数必须满足 x >= 1，因为双曲余弦函数 cosh(t) = (exp(t) + exp(-t)) / 2 的值域是 [1, +∞)，
//所以只有当输入值在此范围内时，才能得到有意义的反双曲余弦值。
//@返回  返回值是一个 T 类型的数值，表示对应双曲余弦值 x 所对应的双曲反余弦值。
//返回值的范围是 [0, +∞)，这是因为双曲余弦函数在 t >= 0 时单调递增且一一对应。
//@别名   取双曲反余弦(双曲余弦值)
template<class T> T acosh(T x);

//@备注  计算双曲反正弦值。双曲反正弦函数是双曲正弦函数的反函数，双曲正弦函数 sinh(x) = (exp(x) - exp(-x)) / 2，
//该函数用于根据双曲正弦函数的结果反向求出对应的输入值
//@示例   //计算不同双曲正弦值的双曲反正弦值
//调试输出(取双曲反正弦(-1.0))
//调试输出(取双曲反正弦(0.0))
//调试输出(取双曲反正弦(1.0))
//@参数  双曲正弦值(double, float)。双曲正弦函数的定义域是全体实数，所以该参数可以取任意实数值
//@返回  返回一个 T 类型的数值，表示对应双曲正弦值 x 所对应的双曲反正弦值。返回值的范围是 (-∞, +∞)，这与双曲正弦函数的值域一致
//@别名   取双曲反正弦(双曲正弦值)
template<class T> T asinh(T x);

//@备注  计算双曲反正切值。双曲反正切函数是双曲正切函数的反函数，双曲正切函数 tanh(x) = (exp(x) - exp(-x)) / (exp(x) + exp(-x))，
//该函数用于根据双曲正切函数的结果反向求出对应的输入值。
//@示例  调试输出(取双曲反正切(0.5))
//@参数  双曲正切值(double, float), 双曲正切函数的值域是 (-1, 1)，所以该参数的取值范围必须是 -1 < x < 1。
//@返回  返回一个 T 类型的数值，表示对应双曲正切值 x 所对应的双曲反正切值。返回值的范围是 (-∞, +∞)
//@别名  取双曲反正切()
template<class T> T atanh(T x);

//@备注  计算双曲余弦值。双曲余弦函数是双曲三角函数中的一种，在数学和工程领域有广泛应用，比如在悬链线问题、波动方程求解等场景
//@示例  调试输出(取双曲余弦(2.0))
//@参数  表示输入的自变量(double, float)，通常是一个实数。双曲余弦函数的定义域为全体实数，即 x 可以取任意实数值
//@返回  返回一个 T 类型的数值，表示输入值 x 的双曲余弦值。双曲余弦函数的值域是 [1, +∞)，所以返回值一定大于或等于 1。
//@别名  取双曲余弦()
template<class T> T cosh(T x);

//@备注  计算输入值 x 的双曲正弦值。双曲正弦函数是双曲三角函数的一种，在数学建模、物理问题求解（如悬链线、波动问题等）中有着重要应用
//@示例  调试输出(取双曲正弦(2.0))
//@参数  表示双曲正弦函数的自变量(double, float), 该函数的定义域为全体实数，即 x 可以取任意实数值。
//@返回  返回一个 T 类型的数值，代表输入值 x 的双曲正弦值。双曲正弦函数的值域是 (-∞, +∞)，即返回值可以是任意实数。
//@别名  取双曲正弦()
template<class T> T sinh(T x);

//@备注  计算输入值 x 的双曲正切值。双曲正切函数是双曲三角函数中的一员，在神经网络（如激活函数）、物理学等领域有广泛应用
//@示例  调试输出(取双曲正切(2.0))
//@参数  表示双曲正切函数的自变量(double, float), 双曲正切函数的定义域为全体实数，即 x 可以取任意实数值。
//@返回  返回一个 T 类型的数值，代表输入值 x 的双曲正切值。双曲正切函数的值域是 (-1, 1)，也就是说返回值会处于 -1 和 1 之间，但不会取到 -1 和 1 这两个边界值。
//@别名  取双曲正切()
template<class T> T tanh(T x);

//@备注  计算以自然常数 e（约为 2.71828）为底，x 为指数的指数函数值，即计算 e 的 x 次幂（e^x）。
//指数函数在数学、物理、工程、经济等众多领域都有广泛应用，例如在描述放射性衰变、人口增长、复利计算等问题中经常会用到。
//@示例  调试输出(取E的N次方(10))
//@参数  表示指数(double, float)。该函数的定义域为全体实数，即 x 可以取任意实数值。
//@返回  返回一个 T 类型的数值，代表 e 的 x 次幂的结果。返回值的范围是 (0, +∞)，因为对于任意实数 x，e^x 的值总是大于 0 的。
//@别名  取E的N次方()
template<class T> T exp(T x);

//@备注  计算以 2 为底，x 为指数的指数函数值，也就是计算 2 的 x 次幂（2^x）。
//在计算机科学领域，特别是涉及到二进制、数据存储容量计算（如字节、位等概念）以及算法复杂度分析中，该函数非常常用。
//@示例  调试输出(取2的N次方(10))
//@参数  表示指数(double, float), 该函数的定义域为全体实数，即 x 可以取任意实数值。
//@返回  返回一个 T 类型的数值，代表 2 的 x 次幂的结果。返回值的范围是 (0, +∞)，因为对于任意实数 x，2^x 的值总是大于 0 的。
//@别名  取2的N次方()
template<class T> T exp2(T x);

//@备注  计算并返回一个数的绝对值。绝对值是指一个数在数轴上所对应点到原点的距离，所以绝对值一定是非负的
//@示例  调试输出(取绝对值(-10))
//@参数  表示要计算绝对值的数(double, float, int)。
//@返回  返回一个 T 类型的整数，为输入参数的绝对值。
//如果输入参数本身是非负的，那么返回值就是该参数本身；
//如果输入参数是负数，返回值则是该参数去掉负号后的数值。
//@别名  取绝对值(x)
template<class T> T abs(T x);

//@备注  计算 x 的 y 次幂，即 x^y。在数学和工程计算中，该函数常用于处理各种幂运算问题，例如计算面积、体积、指数增长等场景
//@示例  调试输出(取次方(2,3))
//@参数  表示底数(double, float)，可以是任意实数。不过在某些特殊情况下，如 y 为非整数时，x 不能为负数，否则会导致结果无意义或产生错误。
//@参数  表示指数(double, float)，可以是任意实数。不同的指数值会使函数有不同的行为和结果。
//@返回  返回一个 double 类型的数值，表示 x 的 y 次幂的计算结果。
//@别名  取次方()
template<class T> T pow(T x, T y);

//@备注  计算一个非负实数的算术平方根。算术平方根是指一个非负数的正的平方根，
//在数学、物理、工程等众多领域都有广泛应用，比如计算距离、求解几何问题等
//@示例  调试输出(取平方根(9))
//@参数  表示要计算平方根的数(double, float)。该参数必须是非负的，即 x >= 0，因为在实数范围内，负数没有算术平方根。
//@返回  返回一个 T 类型的数值，表示输入参数 x 的算术平方根。如果输入 x 为 0，返回值为 0；对于正数 x，返回值是一个大于 0 的实数。
//@别名  取平方根(x)
template<class T> T sqrt(T x);
*/

//@分组}  //数学计算


//-----------------------------------------------
//@分组{   编码转换

//@备注 将Unicode转成Ansi
//@示例 文本型A  ansi字符串 = W2A("Unicode字符串")
//@参数 pString Unicode字符串
//@参数 length  可空, 输入字符串长度(字符为单位)
//@别名 W2A(Unicode文本指针,长度)
CXTextA W2A(const wchar_t* pString, int length = -1);

//@备注 将Ansi转成Unicode
//@示例 文本型  unicode字符串 = A2W(A"Ansi字符串")
//@参数 pString Ansi字符串
//@参数 length  可空, 输入字符串长度(字节为单位)
//@别名 A2W(Ansi文本,长度)
CXText  A2W(const char* pString, int length = -1);

//@备注 Unicode到UTF8
//@示例 文本型A utf8字符串 = W2U("Unicode字符串")
//@参数 pString Unicode字符串
//@参数 length  可空, 输入字符串长度(字符为单位)
CXTextA W2U(const wchar_t* pString, int length = -1);

//@备注 UTF8到Unicode
//@示例 文本型  unicode字符串 = U2W(A"utf8字符串")
//@参数 pString utf8字符串
//@参数 length  可空, 输入字符串长度(字节为单位)
CXText U2W(const char* utf8, int length = -1);

//@备注  Ansi文本转换到UTF8文本
//@示例  文本型A  utf8字符串 = A2U(A"Ansi字符串")
//@参数  pString Ansi字符串
//@参数  length  可空, 输入字符串长度(字节为单位)
//@返回  返回UTF8文本
CXTextA A2U(const char* pString, int length = -1);

//@备注  UTF8文本转换到Ansi
//@示例  文本型A  ansi字符串 = U2A(A"utf8字符串")
//@参数  utf8    utf8字符串
//@参数  length  可空, 输入字符串长度(字节为单位)
//@返回  返回Ansi文本
CXTextA U2A(const char* utf8, int length = -1);

//@隐藏{
CXTextA W2A(const CXText& text, int length = -1);
CXText  A2W(const CXTextA& text, int length = -1);
CXTextA W2U(const CXText& text, int length = -1);
CXText  U2W(const CXTextA& utf8, int length = -1);
CXTextA A2U(const CXTextA& text, int length = -1);
CXTextA U2A(const CXTextA& utf8, int length = -1);

//@隐藏}

//@备注  Base64 是一种基于 64 个可打印字符来表示二进制数据的编码方式。它的主要用途是将二进制数据转换为文本格式，
// 以便在只支持文本传输的环境（如电子邮件、URL 等）中安全地传输二进制数据。
// 这 64 个可打印字符通常是 A - Z、a - z、0 - 9 以及 + 和 /，
// 在 URL 安全的 Base64 编码中，+ 会被替换为 -，/ 会被替换为 _，并且使用 = 作为填充字符。
//@示例  base64编码A(A"www.xcgui.com/doc")  //结果为: A"d3d3LnhjZ3VpLmNvbS9kb2M="
//@参数  输入要编码的数据
//@参数  数据长度(字节为单位)
//@返回  结果文本A
//@别名  base64编码A()
CXTextA xcl_base64EncodeA(const char* bytes_to_encode_, unsigned int in_len = -1);

//@备注  Base64 是一种基于 64 个可打印字符来表示二进制数据的编码方式。它的主要用途是将二进制数据转换为文本格式，
// 以便在只支持文本传输的环境（如电子邮件、URL 等）中安全地传输二进制数据。
// 这 64 个可打印字符通常是 A - Z、a - z、0 - 9 以及 + 和 /，
// 在 URL 安全的 Base64 编码中，+ 会被替换为 -，/ 会被替换为 _，并且使用 = 作为填充字符。
//@示例  base64解码A(aa, aa.取大小())  //结果为: A"www.xcgui.com/doc"
//@参数  输入要解码的数据
//@参数  数据长度(字节为单位)
//@返回  结果文本A
//@别名  base64解码A()
CXTextA xcl_base64EecodeA(const char* encoded_string_, unsigned int in_len = -1);

//@备注  输入UTF8字符串 进行 URL编码 然后返回
//@doc   "doc\xcl_urlEncodeUtf8.txt"
//@返回  返回的字符串进行了编码, 不包含中文字符, 所以不用区分UTF8
//@别名  URL编码UTF8(UTF8字符串)
CXTextA xcl_urlEncodeUtf8(const char* pStringUtf8);

//@备注  输入UTF8字符串 进行 URL解码 然后返回
//@doc   "doc\xcl_urlEncodeUtf8.txt"
//@返回  返回UTF8字符串
//@别名  URL解码UTF8(UTF8字符串)
CXTextA xcl_urlDecodeUtf8(const char* pStringUtf8);

//@备注  输入字符串 进行 URL编码 然后返回
//@doc   "doc\xcl_urlEncode.txt"
//@别名  URL编码(字符串)
CXText  xcl_urlEncode(const wchar_t* pString);

//@备注  输入字符串 进行 URL编码 然后返回
//@doc   "doc\xcl_urlEncodeA.txt"
//@别名  URL编码A(字符串A)
CXTextA xcl_urlEncodeA(const char* pString);

//@备注  输入字符串 进行 URL解码 然后返回
//@doc   "doc\xcl_urlEncode.txt"
//@别名  URL解码(字符串)
CXText  xcl_urlDecode(const wchar_t* pString);

//@备注  输入字符串 进行 URL解码 然后返回
//@doc   "doc\xcl_urlEncodeA.txt"
//@别名  URL解码A(字符串A)
CXTextA xcl_urlDecodeA(const char* pString);

//@分组}  //编码转换

//--------------------------------------------
//@分组{   数值转换

//@备注  将传入参数转换为文本型; 文本组合() 可以将多个参数转为文本
//@示例  文本型  结果1 = 到文本(A"abc")
//文本型  结果2 = 到文本(100)
//文本型  结果3 = 到文本(88.88)
//@别名  到文本(通用类型)
template<class T>
CXText  xcl_toText(T value_) {
	CXText retText;
	xcl_mkValue(retText, value_);
	return retText;
}

//@备注  将传入参数转换为文本型; 文本组合() 可以将多个参数转为文本
//@示例  文本型A  结果1 = 到文本A("abc")
//文本型A  结果2 = 到文本A(100)
//文本型A  结果3 = 到文本A(88.88)
//@别名  到文本A(通用类型)
template<class T>
CXTextA  xcl_toTextA(T value_) {
	CXTextA retText;
	xcl_mkValue(retText, value_);
	return retText;
}

//@隐藏{
CXText  xcl_toText(const CXBytes& value_);
CXTextA xcl_toTextA(const CXBytes& value_);
//@隐藏}

//@备注  整型转换到十六进制文本, 例如: "0x000000FF", "0xFF", "0xff"; 也可以使用 swprintf_s 自定义格式文本
//@示例  文本型  结果 = 整型到十六进制文本(255) //结果为: "0x000000FF"
// 文本型  结果 = 整型到十六进制文本(255, TRUE, FALSE) //结果为: "0xFF"
//@参数  输入整型值
//@参数  是否大写
//@参数  是否自动填充0补齐到8位对齐, TRUE为补齐, FALSE为不补齐
//@返回  结果文本
//@别名  整型到十六进制文本(输入值, 是否大写, 是否自动填充0)
CXText xcl_intToHexText(int value_, BOOL bUpper = TRUE, BOOL bFillZero = TRUE);

//@备注  长整型转换到十六进制文本, 例如: 长整型到十六进制文本(255) = "0x00000000000000FF", "0xFF", "0xff"; 也可以使用 swprintf_s 自定义格式文本
//@示例  文本型  结果 = 长整型到十六进制文本(255) //结果为: "0x00000000000000FF"
// 文本型  结果 = 长整型到十六进制文本(255, TRUE, FALSE) //结果为: "0xFF"
//@参数  输入长整型值
//@参数  是否大写
//@参数  是否自动填充0补齐到8位对齐, TRUE为补齐, FALSE为不补齐
//@返回  结果文本
//@别名  长整型到十六进制文本()
CXText xcl_int64ToHexText(__int64 value_, BOOL bUpper = TRUE, BOOL bFillZero = TRUE);

//@备注  整型转换到二进制文本
//@示例  文本型  结果1 = 整型到二进制文本(7)   //结果为: "111"
//@参数  输入整型值
//@返回  结果文本
//@别名  整型到二进制文本()
CXText xcl_intToBinText(int value_);

//@备注  长整型转换到二进制文本
//@示例  文本型  结果1 = 长整型到二进制文本(7)  //结果为: "111"
//@参数  输入长整型值
//@返回  结果文本
//@别名  长整型到二进制文本()
CXText xcl_int64ToBinText(__int64 value_);

//@备注  字符串转换到浮点型
//@示例  浮点型  结果 = 文本到浮点型("1.5")
//@参数  输入字符串
//@返回  结果浮点型
//@别名  文本到浮点型()
float xcl_wtof(const wchar_t* pText);

//@备注  转换数据到十六进制文本
//@示例  数据到十六进制文本("abc", 3) //结果为: "61 62 63"
//@参数  数据地址
//@参数  数据长度, 字节为单位
//@参数  是否大写
//@参数  是否空格分割
//@返回  结果文本
//@别名  数据到十六进制文本(数据地址, 数据长度, 是否大写, 是否分割)
template<typename T>
CXText  xcl_dataToHexText(__in const T* data, size_t nLen, BOOL bUpper = TRUE, BOOL bSplit = TRUE)
{
	CXText  text;
	if (nLen > 0)
	{
		size_t length = 0;
		if (bSplit)
			length = nLen * 3 - 1;
		else
			length = nLen * 2;
		text.resize(length);
		_dataToHexStrW((wchar_t*)text.get(), length + 1, (const BYTE*)data, nLen, bUpper, bSplit);
	}
	return text;
}

//@备注  转换数据到十六进制文本A
//@示例  数据到十六进制文本A("abc", 3) //结果为: A"61 62 63"
//@参数  数据地址
//@参数  数据长度, 字节为单位
//@参数  是否大写
//@参数  是否空格分割
//@返回  结果文本A
//@别名  数据到十六进制文本A(数据地址, 数据长度, 是否大写, 是否分割)
template<typename T>
CXTextA xcl_dataToHexTextA(__in const T* data, size_t nLen, BOOL bUpper = TRUE, BOOL bSplit = TRUE)
{
	CXTextA  text;
	if (nLen > 0)
	{
		size_t length = 0;
		if (bSplit)
			length = nLen * 3 - 1;
		else
			length = nLen * 2;
		text.resize(length);
		_dataToHexStrA((BYTE*)text.get(), length + 1, (const BYTE*)data, nLen, bUpper, bSplit);
	}
	return text;
}
//@备注  转换数据到十进制文本
//@示例  数据到十进制文本("abc", 3) //结果为: "97 98 99"
//@参数  数据地址
//@参数  数据长度, 字节为单位
//@参数  是否大写
//@参数  是否空格分割
//@返回  结果文本
//@别名  数据到十进制文本(数据地址, 数据长度, 是否大写, 是否分割)
template<typename T>
CXText xcl_dataToDecText(__in const T* data, int nLen, BOOL bSplit = TRUE)
{
	const BYTE* _data = (const BYTE*)data;
	CXText  text;
	wchar_t  buf[4] = { 0 };
	for (int i = 0; i < nLen; i++)
	{
		if (0 != i && bSplit)
			text += ' ';
		_data[i];
		_itow_s(_data[i], buf, 10);
		text += buf;
	}
	return text;
}

//@备注  转换数据到十进制文本A
//@示例  数据到十进制文本("abc", 3) //结果为: A"97 98 99"
//@参数  数据地址
//@参数  数据长度, 字节为单位
//@参数  是否大写
//@参数  是否空格分割
//@返回  结果文本A
//@别名  数据到十进制文本A(数据地址, 数据长度, 是否大写, 是否分割)
template<typename T>
CXTextA xcl_dataToDecTextA(__in const T* data, int nLen, BOOL bSplit = TRUE)
{
	const BYTE* _data = (const BYTE*)data;
	CXTextA  text;
	char  buf[4] = { 0 };
	for (int i = 0; i < nLen; i++)
	{
		if (0 != i && bSplit)
			text += ' ';
		_data[i];
		_itoa_s(_data[i], buf, 10);
		text += buf;
	}
	return text;
}


//@备注  将输入的二进制数据转换为可读的二进制字符串表示，支持字节对齐和分隔符。
//@示例  整型 var = 0x01020304
// 文本型A 结果 = 数据到二进制文本A(&var, 4) //结果为: A"00000001 00000010 00000011 00000100"
//@参数  输入的二进制数据指针，指向需要转换的内存数据。
//@参数  输入数据的长度，单位为字节。如果为 0 或负数，返回空字符串。
//@参数  指定按几字节对齐输出，默认为 1，如果小于等于 0，返回空字符串。
//@参数  是否在字节之间添加空格分隔符, 字节组之间额外添加逗号分割, TRUE 为添加，FALSE 为不添加。
//@参数  是否将每个字节的二进制表示自动填充到 8 位，TRUE 为补齐，FALSE 为不补齐。
//@返回  返回表示输入数据的二进制字符串A，按大端序输出。
//@别名  数据到二进制文本A(数据地址, 数据长度, 对齐字节, 是否空格分割, 填充0对齐)
CXTextA xcl_dataToBinTextA(const void* data, int nLen, int nAlignByte = 1, BOOL bSplit = TRUE, BOOL bFillZero = TRUE);

//@备注  将输入的二进制数据转换为可读的二进制字符串表示，支持字节对齐和分隔符。
//@示例  整型 var = 0x01020304
// 文本型  结果 = 数据到二进制文本(&var, 4) //结果为: "00000001 00000010 00000011 00000100"
//@参数  输入的二进制数据指针，指向需要转换的内存数据。
//@参数  输入数据的长度，单位为字节。如果为 0 或负数，返回空字符串。
//@参数  指定按几字节对齐输出，默认为 1，如果小于等于 0，返回空字符串。
//@参数  是否在字节之间添加空格分隔符, 字节组之间额外添加逗号分割, TRUE 为添加，FALSE 为不添加。
//@参数  是否将每个字节的二进制表示自动填充到 8 位，TRUE 为补齐，FALSE 为不补齐。
//@返回  返回表示输入数据的二进制字符串，按大端序输出。
//@别名  数据到二进制文本(数据地址, 数据长度, 对齐字节, 是否空格分割, 填充0对齐)
CXText xcl_dataToBinText(const void* data, int nLen, int nAlignByte = 1, BOOL bSplit = TRUE, BOOL bFillZero = TRUE);

//@备注  将输入的二进制数据转换为可读的二进制字符串表示，支持字节对齐和分隔符。
//@示例  整型 var = 0x01020304
// 文本型  结果 = xcl_dataTypeToBinText(&var, TRUE, TRUE) 
// //结果为: "00000001 00000010 00000011 00000100"
//@参数  输入的二进制数据指针，指向需要转换的内存数据。
//@参数  是否在字节或字节组之间添加空格分隔符，TRUE 为添加，FALSE 为不添加。
//@参数  是否将每个字节的二进制表示自动填充到 8 位，TRUE 为补齐，FALSE 为不补齐。
//@返回  返回表示输入数据的二进制字符串，按大端序输出。
//@别名  数据类型到二进制文本(数据地址, 是否空格分割, 填充0对齐)
template<typename T>
CXText xcl_dataTypeToBinText(__in const T data, BOOL bSplit = TRUE, BOOL bFillZero = TRUE)
{
	int typeSize = sizeof(T);
	return xcl_dataToBinText(&data, typeSize, typeSize, bSplit, bFillZero);
}

//@备注  将输入的十六进制文本转换为整型值返回
//@示例  整型 结果1 = 十六进制文本到整型("0xFF")  //结果为: 255
// 整型 结果2 = 十六进制文本到整型("FF") //结果为: 255
//@参数  十六进制文本
//@返回  返回转换后的整型值
//@别名  十六进制文本到整型()
int  xcl_hexTextToInt(const wchar_t* pHexText);

//@备注  将输入的十六进制文本转换为长整型值返回
//@示例  长整型 结果1 = 十六进制文本到长整型("0xFF") //结果为: 255
// 长整型 结果2 = 十六进制文本到长整型("FF") //结果为: 255
//@参数  十六进制文本
//@返回  返回转换后的长整型值
//@别名  十六进制文本到长整型()
__int64  xcl_hexTextToInt64(const wchar_t* pHexText);

//@备注  将输入的二进制文本转换为整型值返回
//@示例  整型 结果 = 二进制文本到整型("111") //结果为: 7
//@参数  二进制文本
//@返回  返回转换后的整型值
//@别名  二进制文本到整型()
int  xcl_binTextToInt(const wchar_t* pHexText);

//@备注  将输入的二进制文本转换为长整型值返回
//@示例  长整型 结果 = 二进制文本到长整型("111")  //结果为: 7
//@参数  二进制文本
//@返回  返回转换后的长整型值
//@别名  二进制文本到长整型()
__int64  xcl_binTextToInt64(const wchar_t* pHexText);


/*@声明

//@备注 字符串转换到整型
//@示例 整型 结果 = 文本到整型("123") //结果为: 123
//@参数 输入字符串
//@返回 结果整型
//@别名 文本到整型()
int _wtoi(const wchar_t* pText);

//@备注  文本转换到长整型
//@示例  长整型 结果 = 文本到长整型("123") //结果为: 123
//@参数  输入字符串
//@返回  结果整型64
//@别名  文本到长整型()
__int64 _wtoi64(const wchar_t* pText);

//@备注  文本转换到双浮点型
//@示例  双浮点型 结果 = 文本到双精度浮点型("123.5") //结果为: 123.5
//@参数  输入字符串
//@返回  结果双浮点型
//@别名  文本到双精度浮点型()
double _wtof(const wchar_t* pText);

//@备注  文本A转换到整型
//@示例  整型 结果 = 文本到整型A(A"123")  //结果为: 123
//@参数  输入字符串A
//@返回  结果整型
//@别名  文本到整型A()
int  atoi(const char* _String);       

//@备注  文本A转换到整型64
//@示例  整型64 结果 = 文本到整型64A(A"123")  //结果为: 123
//@参数  输入字符串A
//@返回  结果整型64
//@别名  文本到整型64A()
int  _atoi64(const char* _String);

*/
//@分组}


//--------------------------------------------
//@分组{ 内存操作
/*@声明

//@备注 从原(src)所指的内存地址的起始位置开始拷贝n个字节到目标(dest)所指的内存地址的起始位置中
//@示例	字符型  数组1[10] = {0}
//	文本型  字符串 = "abc"
//	内存复制(数组1, 字符串, 字符串.取大小() * 2)
//	调试输出(数组1)
//@参数 --dest 指向用于存储复制内容的目标数组
//@参数 --src  指向要复制的数据源
//@参数 --count 要被复制的字节数
//@返回 该函数返回一个指向目标存储区(dest)的指针。
//@别名 内存复制(存储数据, 要复制的数据, 复制的字节数)
void *memcpy(void *dest,const void *src,size_t count);

//@备注 从原(src)所指的内存地址的起始位置开始拷贝n个字节到目标(dest)所指的内存地址的起始位置中
//@示例	字符型  数组1[10] = {0}
//	文本型  字符串 = "abc"
//	内存复制W(数组1, 字符串, 字符串.取大小())
//	调试输出(数组1)
//@参数 --dest 指向用于存储复制内容的目标数组
//@参数 --src  指向要复制的数据源
//@参数 --count 要被复制的字符数(每个字符2字节)
//@返回 该函数返回一个指向目标存储区(dest)的指针。
//@别名 内存复制W(存储数据, 要复制的数据, 复制的字符数)
wchar_t *wmemcpy(wchar_t *dest,const wchar_t *src,size_t count);

//@备注 比较 内存块1 和 内存块2 的前指定数量字节 是否相同
//@示例 	字符型A  数组1[10] = A"abc"
//	如果 0 == 内存比较A(数组1, A"abc", 文本取长度A(A"abc"))
//	-- 调试输出("内存相同")
//	否则
//	-- 调试输出("内存不同")
//@参数 --buf1 指向内存块的指针
//@参数 --buf2 指向内存块的指针
//@参数 --count 要被比较的字节数
//@返回 如果返回值 = 0，则表示 内存块1 等于 内存块2; 如果返回值 < 0，则表示 内存块1 小于 内存块2; 如果返回值 > 0，则表示 内存块2 小于 内存块1;
//@别名 内存比较(内存块1, 内存块2, 字节数)
int memcmp(const void *buf1, const void *buf2,size_t count);

//@备注 比较 内存块1 和 内存块2 的前指定数量字符 是否相同
//@示例 	字符型  数组1[10] = "abc"
//	如果 0 == 内存比较(数组1, "abc", 文本取长度("abc"))
//	-- 调试输出("内存相同")
//	否则
//	-- 调试输出("内存不同")
//@参数 --buf1 指向内存块的指针
//@参数 --buf2 指向内存块的指针
//@参数 --count 要被比较的字符数
//@返回 如果返回值 = 0，则表示 内存块1 等于 内存块2; 如果返回值 < 0，则表示 内存块1 小于 内存块2; 如果返回值 > 0，则表示 内存块2 小于 内存块1;
//@别名 内存比较W(内存块1, 内存块2, 字节数)
int wmemcmp(const wchar_t * buf1,const wchar_t * buf2, size_t count);

//@备注  它是为了替代传统的 memcpy 函数而设计的，旨在增强程序的安全性，避免因缓冲区溢出等问题导致的潜在安全风险
//@示例  字符型  数组1[10] = {0}
// 内存复制2(数组1, 10 * 2, "abc", 文本取长度("abc") * 2)
//@参数  目标地址
//@参数  目标内存大写(字节为单位)
//@参数  原地址
//@参数  要复制的内容长度(字节为单位)
//@返回  成功返回0
//@别名 内存复制2(目标地址, 目标大小, 原地址, 复制字节数)
errno_t memcpy_s(void *dest,  size_t numberOfElements,  const void *src,  size_t count );

//@备注  它是为了替代传统的 wmemcpy 函数而设计的，旨在增强程序的安全性，避免因缓冲区溢出等问题导致的潜在安全风险
//@示例  字符型  数组1[10] = {0}
// 内存复制2W(数组1, 10 , "abc", 文本取长度("abc"))
//@参数  目标地址
//@参数  目标内存大写(字节为单位)
//@参数  原地址
//@参数  要复制的内容长度(字符为单位), 每个字符2字节
//@返回  成功返回0
//@别名 内存复制2W(目标地址, 目标大小, 原地址, 字符数量)
errno_t wmemcpy_s(wchar_t *dest,  size_t numberOfElements,  const wchar_t *src,  size_t count);

//@备注  将内存中N个字节用指定的值替换, 初始值范围(0-255)
//@示例  字符型  数组1[10]
// 内存初始化(数组1, 0, 10*2)
//@参数  目标地址
//@参数  替换的值
//@参数  内存长度(字节为单位)
//@别名  内存初始化(内存地址, 初始值, 内存大小)
void* memset(void* dest, int val, size_t size);

//@备注 将内存中N个字符用指定的字符替换
//@示例  字符型  数组1[10]
// 内存初始化W(数组1, 0, 10)
//@参数  目标地址
//@参数  替换的值
//@参数  内存长度(字符为单位), 每个字符2字节
//@别名 内存初始化W(内存地址, 初始值, 字符数量)
wchar_t* wmemset(wchar_t* dest, wchar_t ch, size_t count);

//@备注 分配所需的内存空间，并返回一个指向它的指针。当不使用后需要释放内存, 否则会造成内存泄露
//@示例 字节型* 地址 = 内存分配(100) //分配100字节内存空间
//内存释放(地址) //释放分配的内存
//@参数 内存块大小(字节为单位)
//@返回 返回分配的内存地址,如果失败返回空
//@别名 内存分配(大小)
void *malloc(size_t size);

//@备注 该内存块之前是通过调用 malloc、calloc 或 realloc 进行分配内存的。如果传递的参数是一个空指针，则不会执行任何动作
//@示例  字节型* 地址 = 内存分配(100) //分配100字节内存空间
//内存释放(地址) //释放分配的内存
//@参数 内存块指针
//@别名 内存释放(内存块指针)
void free(void *memblock);

*/

//@分组}

//----------------------------------------------
//@分组{  文本操作
/*@声明

//@备注 最多12个参数, 将不同的类型组合成字符串返回
//@参数 欲组合的文本 类型为:字符型, 文本型, 数字型等
//@示例 	文本型 结果 = 文本组合("abc", 123, 6.5f) //结果为: "abc1236.5"
//@返回 组合后的文本
//@别名 文本组合()
CXText  xcl_mkStr(...);

//@备注 最多12个参数, 将不同的类型组合成字符串返回
//@参数 欲组合的文本 类型为:字符型A, 文本型A, 数字型等
//@示例 	文本型A 结果 = 文本组合A(A"abc", 123, 6.5f) //结果为: A"abc1236.5"
//@返回 组合后的文本
//@别名 文本组合A()
CXTextA xcl_mkStrA(...);


//@备注 不限参数, 将不同的类型组合成字符串返回
//@参数 欲组合的文本 类型为:字符型, 文本型, 数字型等
//@示例 	文本型 结果 = 文本组合2("abc", 123, 6.5f) //结果为: "abc1236.5"
//@返回 组合后的文本
//@别名 文本组合2()
CXText  xcl_mkStr2(...);

//@备注 不限参数, 将不同的类型组合成字符串返回
//@参数 欲组合的文本 类型为:字符型A, 文本型A, 数字型等
//@示例 	文本型A 结果 = 文本组合2A(A"abc", 123, 6.5f) //结果为: A"abc1236.5"
//@返回 组合后的文本
//@别名 文本组合2A()
CXTextA xcl_mkStr2A(...);

//@备注 将字符串从原位置复制到目标位置, 增强安全版本
//@示例 字符型A  数组1[10] = {0}
// 文本复制A(数组1, 10, A"abc")
//@参数 目标字符串地址
//@参数 目标内存大小(字节为单位)
//@参数 原字符串地址
//@返回 如果成功返回0
//@别名 文本复制A(目标地址, 目标内存大小, 源文本)
errno_t strcpy_s(char *strDestination,size_t numberOfElements,const char *strSource);

//@备注 将字符串从原位置复制到目标位置, 增强安全版本
//@示例 字符型  数组1[10] = {0}
// 文本复制(数组1, 10, "abc")
//@参数 目标字符串地址
//@参数 目标内存大小(字节为单位)
//@参数 原字符串地址
//@返回 如果成功返回0
//@别名 文本复制(目标地址, 目标内存大小, 原地址)
errno_t wcscpy_s(wchar_t *strDestination,size_t numberOfElements,const wchar_t *strSource);

//@备注 将一个字符串追加到另一个字符串的末尾; 安全版本
//@示例 字符型A  数组1[10] = A"abc"
// 文本拼接A(数组1, 10, A"123") //结果为: A"abc123"
//@参数 目标字符串地址
//@参数 目标内存大小(字节为单位)
//@参数 原字符串地址, 要添加的字符串
//@返回 如果成功返回0
//@别名 文本拼接A(目标缓冲区, 目标大小, 添加的字符串)
errno_t strcat_s(char *strDestination,size_t numberOfElements,const char *strSource);

//@备注 将一个字符串追加到另一个字符串的末尾; 安全版本
//@示例 字符型  数组1[10] = "abc"
// 文本拼接(数组1, 10, "123") //结果为: "abc123"
//@参数 目标字符串地址
//@参数 目标内存大小(字符为单位), 每个字符2字节
//@参数 原字符串地址, 要添加的字符串
//@返回 如果成功返回0
//@别名 文本拼接(目标缓冲区, 目标大小, 添加的字符串)
errno_t wcscat_s(wchar_t *strDestination,size_t numberOfElements,const wchar_t *strSource);

//@备注 获取字符串长度
//@示例 整型 长度 = 文本取长度A(A"abc")
//@参数 字符串A
//@返回 字符串的长度(字节为单位), 不包含终止符
//@别名 文本取长度A(文本数据)
UINT strlen(const char *str);

//@备注 获取字符串长度
//@示例 整型 长度 = 文本取长度("abc")
//@参数 字符串
//@返回 字符串的长度(字符为单位), 每个字符2字节, 不包含终止符
//@别名 文本取长度(文本数据)
UINT wcslen(const wchar_t *str);

//@备注 比较两个字符串是否相同
//@示例	字符型A  数组1[10] = A"abc"
//	如果 (0 == 文本比较A(数组1, A"abc"))
//	-- 调试输出("相同")
//	否则
//	-- 调试输出("不相同")
//@参数 待比较文本一
//@参数 待比较文本二
//@返回 如果返回值小于0，表示文本一小于文本二；如果等于0，表示文本一等于文本二；如果大于0，表示文本一大于文本二
//@别名 文本比较A(待比较文本一,待比较文本二)
int strcmp(const char *string1, const char *string2);

//@备注 比较两个字符串是否相同
//@示例	字符型  数组1[10] = "abc"
//	如果 (0 == 文本比较(数组1, "abc"))
//	-- 调试输出("相同")
//	否则
//	-- 调试输出("不相同")
//@参数 待比较文本一
//@参数 待比较文本二
//@返回 如果返回值小于0，表示文本一小于文本二；如果等于0，表示文本一等于文本二；如果大于0，表示文本一大于文本二
//@别名 文本比较(待比较文本一,待比较文本二)
int wcscmp( const wchar_t *string1, const wchar_t *string2);

//@备注 格式化内容到目标缓冲区
//文本格式化(接收缓冲区, 缓冲区大小, 格式字符串, 内容...)
//@示例	字符型  数组1[100] = {0}
//文本格式化(数组1, 100, "%s-%d-%f", "abc", 100, 6.5f)
//调试输出(数组1)  //结果为: "abc-100-6.500000"
//@返回  如果成功，返回写入 缓冲区 的字符数(不包括终止符), 如果发生错误，返回一个负值
//@别名 文本格式化()
int swprintf_s(...);

//@备注 格式化内容到目标缓冲区
//文本格式化A(接收缓冲区, 缓冲区大小, 格式字符串, 内容...)
//@示例	字符型A  数组1[100] = {0}
//文本格式化A(数组1, 100, A"%s-%d-%f", A"abc", 100, 6.5f)
//调试输出(数组1)  //结果为: A"abc-100-6.500000"
//@返回  如果成功，返回写入 缓冲区 的字符数(不包括终止符), 如果发生错误，返回一个负值
//@别名 文本格式化A()
int sprintf_s(...);

//@备注  将小写字母转换为大写字母，如果传入的字符不是小写字母，则返回字符本身
//@示例  字符型  大写字符 = 字符到大写('a') //结果为: 'A'
//@参数  要转换的字符，以整数形式传入（代表字符的 ASCII 值）
//@返回  如果 参数1 是小写字母，返回对应的大写字母的 ASCII 值；否则返回 参数1 本身
//@别名  字符到大写(字符)
int toupper(int c);

//@备注  将大写字母转换为对应的小写字母, 如果传入的字符不是大写字母，则直接返回该字符本身
//@示例  字符型  小写字符 = 字符到小写('A') //结果为: 'a'
//@参数  要进行转换检查的字符，以整数形式传入（实际代表字符的 ASCII 值）
//@返回  若 参数1 是大写字母，返回对应的小写字母的 ASCII 值；若不是，返回 参数1 本身
//@别名  字符到小写(字符)
int tolower(int c);

//@备注 在主字符串中查找子字符串首次出现的位置(字符串指针)
//@示例 常量 字符型A *查找结果 = 文本查找A(A"abcdefg", A"ef") //结果为 A"efg"
// //返回的指针是指向参数1字符串内存, 所以需要加上常量限制, 避免误修改内容导致内存错误
//@参数 被搜索的字符串
//@参数 要搜索的字符串
//@返回 若成功则返回一个指参数2者第一次出现位置的字符串指针，否则返回NULL
//@别名 文本查找A(被搜索的字符串, 要搜索的字符串)
const char *strstr(const char *str,const char *strSearch);

//@备注 在主字符串中查找子字符串首次出现的位置(字符串指针)
//@示例 常量 字符型 *查找结果 = 文本查找("abcdefg", "ef") //结果为 "efg"
// //返回的指针是指向参数1字符串内存, 所以需要加上常量限制, 避免误修改内容导致内存错误
//@参数 被搜索的宽字符串
//@参数 要搜索的宽字符串
//@返回 若成功则返回一个指向参数2第一次出现位置的字符串指针，否则返回NULL
//@别名 文本查找(被搜索的宽字符串,要搜索的宽字符串)
const wchar_t *wcsstr(const wchar_t *str,const wchar_t *strSearch);

*/

//@备注 从后往前查找指定文本
//@示例  常量 字符型 *查找结果 = 文本查找反向("123456-123456", "123") //结果为: "123456"
// //返回的指针是指向参数1字符串内存, 所以需要加上常量限制, 避免误修改内容导致内存错误
//@参数 被搜索的宽字符串
//@参数 要搜索的宽字符串
//@返回 若成功则返回一个指向参数2第一次出现位置的字符串指针，否则返回NULL
//@别名 文本查找反向(被搜索的宽字符串, 要搜索的宽字符串)
const wchar_t* text_findRight(const wchar_t* str, const wchar_t* strSearch);

//@备注 从后往前查找指定文本
//@示例  常量 字符型A *查找结果 = 文本查找反向A(A"123456-123456", A"123") //结果为: A"123456"
// //返回的指针是指向参数1字符串内存, 所以需要加上常量限制, 避免误修改内容导致内存错误
//@参数 被搜索的宽字符串
//@参数 要搜索的宽字符串
//@返回 若成功则返回一个指向参数2第一次出现位置的字符串指针，否则返回NULL
//@别名 文本查找反向A(被搜索的字符串, 要搜索的字符串)
const char* text_findRightA(const char* str, const char* strSearch);

//@备注  在目标字符串指定位置插入子字符串, 返回结果文本
//@示例  文本型  结果 = 文本插入("123456", 3, "abc") //结果为: "123abc456"
//@参数  目标字符串
//@参数  插入位置索引(字符为单位)
//@参数  要插入的字符串
//@返回  结果文本
//@别名  文本插入(目标字符串, 插入位置, 插入内容)
CXText  text_insert(const wchar_t* pText, size_t off, const wchar_t* pSubText);

//@备注  在目标字符串指定位置插入子字符串, 返回结果文本
//@示例  文本型A  结果 = 文本插入A(A"123456", 3, A"abc") //结果为: A"123abc456"
//@参数  目标字符串
//@参数  插入位置偏移(字符为单位)
//@参数  要插入的字符串
//@返回  返回结果文本
//@别名  文本插入A(目标字符串, 插入位置, 插入内容)
CXTextA text_insertA(const char* pText, size_t off, const char* pSubText);

//@备注  替换目标字符串中指定内容
//@示例  文本型  结果 = 文本替换("123456", "34", "abc") //结果为: "12abc56"
//@参数  目标字符串
//@参数  查找字符串
//@参数  替换字符串
//@返回  结果文本
//@别名  文本替换(目标字符串, 查找内容, 替换内容)
CXText  text_replace(const wchar_t* pText, const wchar_t* pSrc, const wchar_t* pDest);

//@备注  替换目标字符串中指定内容
//@示例  文本型A  结果 = 文本替换A(A"123456", A"34", A"abc") //结果为: A"12abc56"
//@参数  目标字符串
//@参数  查找字符串
//@参数  替换字符串
//@返回  结果文本
//@别名  文本替换A(目标字符串, 查找内容, 替换内容)
CXTextA text_replaceA(const char* pText, const char* pSrc, const char* pDest);

//@备注  获取左边(起始)指定长度文本
//@示例  文本型  结果 = 文本取左边("123456", 3) //结果为: "123"
//@参数  目标字符串
//@参数  获取长度(字符为单位)
//@返回  结果文本
//@别名  文本取左边(目标字符串, 长度)
CXText  text_getLeft(const wchar_t* pText, UINT length);

//@备注  获取左边指定长度文本
//@示例  文本型A  结果 = 文本取左边A(A"123456", 3) //结果为: A"123"
//@参数  目标字符串
//@参数  获取长度(字节为单位)
//@返回  结果文本
//@别名  文本取左边A(目标字符串, 长度)
CXTextA  text_getLeftA(const char* pText, UINT length);

//@备注  获取右侧(末尾)指定长度文本
//@示例  文本型  结果 = 文本取右边("123456", 3) //结果为 "456"
//@参数  目标字符串
//@参数  获取长度(字符为单位)
//@返回  结果文本
//@别名  文本取右边(目标字符串, 长度)
CXText  text_getRight(const wchar_t* pText, UINT length);

//@备注  获取右侧(末尾)指定长度文本
//@示例  文本型A  结果 = 文本取右边A(A"123456", 3) //结果为 A"456"
//@参数  目标字符串
//@参数  获取长度(字节为单位)
//@返回  结果文本
//@别名  文本取右边A(目标字符串, 长度)
CXTextA  text_getRightA(const char* pText, UINT length);

//@备注  获取指定范围内字符串
//@示例  文本型  结果 = 文本取中间("123456", 2, 5) //结果为: "345"
//@参数  目标字符串
//@参数  开始位置索引(字符为单位)
//@参数  结束位置索引(字符为单位)
//@返回  结果文本
//@别名  文本取中间(目标字符串, 开始位置, 结束位置)
CXText  text_getSub(const wchar_t* pText, UINT off1, UINT off2);

//@备注  获取指定范围内字符串
//@示例  文本型A  结果 = 文本取中间A(A"123456", 2, 5) //结果为: A"345"
//@参数  目标字符串
//@参数  开始位置索引(字节为单位)
//@参数  结束位置索引(字节为单位)
//@返回  结果文本
//@别名  文本取中间A(目标字符串, 开始位置, 结束位置)
CXTextA text_getSubA(const char* pText, UINT off1, UINT off2);

//@备注  从指定位置获取指定数量字符串
//@示例  文本型  结果 = 文本取中间2("123456", 2, 3) //结果为: "345"
//@参数  目标字符串
//@参数  开始位置索引(字符为单位)
//@参数  获取数量(字符为单位)
//@返回  结果文本
//@别名  文本取中间2(目标字符串, 开始位置, 数量)
CXText  text_getSub2(const wchar_t* pText, UINT off, UINT count);

//@备注  从指定位置获取指定数量字符串
//@示例  文本型A  结果 = 文本取中间2A(A"123456", 2, 3) //结果为: A"345"
//@参数  目标字符串
//@参数  开始位置索引(字节为单位)
//@参数  获取数量(字节为单位)
//@返回  结果文本
//@别名  文本取中间2A(目标字符串, 开始位置, 数量)
CXTextA text_getSub2A(const char* pText, UINT off, UINT count);

//@备注  获取指定范围内字符串
//@示例  文本型  结果 = 文本取中间3("123456", "12", "56")  //结果为: "34"
//@参数  目标字符串
//@参数  开始位置字符串
//@参数  结束位置字符串
//@返回  结果文本
//@别名  文本取中间3(目标字符串, 开始文本, 结束文本)
CXText  text_getSub3(const wchar_t* pText, const wchar_t* pBegin, const wchar_t* pEnd);

//@备注  获取指定范围内字符串
//@示例  文本型A  结果 = 文本取中间3A(A"123456", A"12", A"56")  //结果为: A"34"
//@参数  目标字符串
//@参数  开始位置字符串
//@参数  结束位置字符串
//@返回  结果文本
//@别名  文本取中间3A(目标字符串, 开始文本, 结束文本)
CXTextA  text_getSub3A(const char* pText, const char* pBegin, const char* pEnd);

//@备注  获取子串位置
//@示例  文本型  结果 = 文本取位置("123456", "56")  //结果为: 4
//@参数  目标字符串
//@参数  子字符串
//@返回  子串位置索引, 失败返回-1, EOF(宏) = 0xFFFFFFFF
//@别名  文本取位置(目标字符串, 子字符串)
int  text_getSubPos(const wchar_t* pText, const wchar_t* pSubText);

//@备注  获取子串位置
//@示例  文本型A  结果 = 文本取位置A(A"123456", A"56")  //结果为: 4 
//@参数  目标字符串
//@参数  子字符串
//@返回  子串位置索引, 失败返回-1, EOF(宏) = 0xFFFFFFFF
//@别名  文本取位置A(目标字符串, 子字符串)
int  text_getSubPosA(const char* pText, const char* pSubText);

//@备注  反向查找子字符串出现位置
//@示例  文本型  结果 = 文本取位置反向("123456-123456", "56") //结果为: 11
//@参数  目标字符串
//@参数  子字符串
//@返回  返回子串位置偏移, 如果未找到返回-1
//@别名  文本取位置反向(目标字符串, 子字符串)
int  text_getSubPosRight(const wchar_t* pText, const wchar_t* pSubText);

//@备注  反向查找子字符串出现位置
//@示例  文本型A  结果 = 文本取位置反向A(A"123456-123456", A"56") //结果为: 11
//@参数  目标字符串
//@参数  子字符串
//@返回  返回子串位置偏移, 如果未找到返回-1
//@别名  文本取位置反向A(目标字符串, 子字符串)
int  text_getSubPosRightA(const char* pText, const char* pSubText);

//@备注  分割字符串保存到列表中返回
//@示例  动态数组<文本型> 结果数组 = 文本分割("123-456-789", "-")
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  目标字符串
//@参数  分割符
//@返回  返回结果列表
//@别名  文本分割(目标字符串, 分割字符串)
CXVector<CXText> text_split(const wchar_t* pText, const wchar_t* pSplit);

//@备注  分割字符串保存到列表中
//@示例  动态数组<文本型A> 结果数组 = 文本分割A(A"123-456-789", A"-")
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  目标字符串
//@参数  分割字符串
//@返回  返回结果列表
//@别名  文本分割A(目标字符串, 分割字符串)
CXVector<CXTextA> text_splitA(const char* pText, const char* pSplit);

//@备注  分割字符串保存到列表中, 并且删除两端空格
//@示例  动态数组<文本型> 结果数组 = 文本分割删除空格("123- 456 - 789 ", "-")
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  目标字符串
//@参数  分割字符串
//@返回  返回结果列表
//@别名  文本分割删除空格(目标字符串, 分割字符串)
CXVector<CXText> text_split2(const wchar_t* pText, const wchar_t* pSplit);

//@备注  分割字符串保存到列表中, 并且删除两端空格
//@示例  动态数组<文本型A> 结果数组 = 文本分割删除空格A(A"123- 456 - 789 ", A"-")
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  目标字符串
//@参数  分割字符串
//@返回  返回结果列表
//@别名  文本分割删除空格A(目标字符串, 分割字符串)
CXVector<CXTextA> text_split2A(const char* pText, const char* pSplit);

//@备注  分割字符串保存到列表中
//@示例  动态数组<文本型> 结果数组 = 文本分割字符("123-456-789", '-')
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  目标字符串
//@参数  分割字符
//@返回  返回结果列表
//@别名  文本分割字符(目标字符串, 分割字符)
CXVector<CXText> text_splitChar(const wchar_t* pText, const wchar_t splitChar);

//@备注  分割字符串保存到列表中
//@示例  动态数组<文本型A> 结果数组 = 文本分割字符A(A"123-456-789", A'-')
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  目标字符串
//@参数  分割字符
//@返回  返回结果列表
//@别名  文本分割字符A(目标字符串, 分割字符)
CXVector<CXTextA> text_splitCharA(const char* pText, const char splitChar);

//@备注  分割字符串保存到列表中, 并且删除两端空格
//@示例  动态数组<文本型> 结果数组 = 文本分割字符删除空格("123- 456 - 789 ", '-')
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  目标字符串
//@参数  分割字符
//@返回  返回结果列表
//@别名  文本分割字符删除空格(目标字符串, 分割字符)
CXVector<CXText> text_splitChar2(const wchar_t* pText, const wchar_t splitChar);

//@备注  分割字符串保存到列表中, 并且删除两端空格
//@示例  动态数组<文本型A> 结果数组 = 文本分割字符删除空格A(A"123- 456 - 789 ", A'-')
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  目标字符串
//@参数  分割字符
//@返回  返回结果列表
//@别名  文本分割字符删除空格A(目标字符串, 分割字符)
CXVector<CXTextA>  text_splitChar2A(const char* pText, const char splitChar);

//@备注  文本按行分割, 结果保存在列表中返回
//@示例  动态数组<文本型> 结果数组 = 文本分割行("123\n456\n789")
// 列表循环(整型 i = 0; 结果数组)
// 调试输出(结果数组[i])
//@参数  字符串
//@返回  返回结果列表
//@别名  文本分割行(字符串)
CXVector<CXText> text_splitRow(const wchar_t* pText);

//@备注  文本按行分割, 结果保存在列表中返回
//@示例  动态数组<文本型A> 结果数组 = 文本分割行A(A"123\n456\n789")
// 列表循环(整型 i = 0; 结果数组)
// 调试输出(结果数组[i])
//@参数  字符串
//@返回  返回结果列表
//@别名  文本分割行A(字符串)
CXVector<CXTextA> text_splitRowA(const char* pText);

//@备注  文本按行分割, 结果保存在列表中返回, 并且删除两端空格
//@示例  动态数组<文本型> 结果数组 = 文本分割行删除空格("123 \n 456 \n 789")
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  字符串
//@返回  返回结果列表
//@别名  文本分割行删除空格(字符串)
CXVector<CXText> text_splitRow2(const wchar_t* pText);

//@备注  文本按行分割, 结果保存在列表中返回, 并且删除两端空格
//@示例  动态数组<文本型A> 结果数组 = 文本分割行删除空格A(A"123 \n 456 \n 789")
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@参数  字符串
//@返回  返回结果列表 
//@别名  文本分割行删除空格A(字符串)
CXVector<CXTextA> text_splitRow2A(const char* pText);

//@备注  文本删除两端空格
//@示例  文本型 结果 = 文本删除两端空格(" 123 ")  //结果为: "123"
//@参数  字符串
//@返回  结果文本
//@别名  文本删除两端空格(字符串)
CXText text_trim(const wchar_t* pText);

//@备注  文本删除两端空格
//@示例  文本型A 结果 = 文本删除两端空格A(A" 123 ")  //结果为: A"123"
//@参数  字符串
//@返回  结果文本A
//@别名  文本删除两端空格A(字符串)
CXTextA text_trimA(const char* pText);

//@备注  此方法在原有字符串基础上修改
//@示例  字符型  数组1[10] = " abc "
// 常量 字符型* 结果 = 文本删除两端空格2(数组1) //结果为: "abc"
//@参数  字符串
//@返回  结果字符串
//@别名  文本删除两端空格2(字符串)
const wchar_t* text_trim2(const wchar_t* pText);

//@备注  此方法在原有字符串基础上修改
//@示例  字符型A  数组1[10] = A" abc "
// 常量 字符型A* 结果 = 文本删除两端空格2A(数组1) //结果为: A"abc"
//@参数  字符串
//@返回  结果字符串A
//@别名  文本删除两端空格2A(字符串)
const char* text_trim2A(const char* pText);

//@备注  文本删除左侧空格
//@示例  文本型 结果 = 文本删除左侧空格(" abc ") //结果为: "abc "
//@参数  字符串
//@返回  结果文本
//@别名  文本删除左侧空格(字符串)
CXText text_trimLeft(const wchar_t* pText);

//@备注  文本删除左侧空格
//@示例  文本型A 结果 = 文本删除左侧空格A(A" abc ") //结果为: A"abc "
//@参数  字符串
//@返回  结果文本
//@别名  文本删除左侧空格A(字符串)
CXTextA text_trimLeftA(const char* pText);

//@备注  文本删除右侧(末尾)空格
//@示例  文本型 结果 = 文本删除末尾空格(" abc ") //结果为: " abc"
//@参数  字符串
//@返回  结果文本
//@别名  文本删除末尾空格(字符串)
CXText text_trimRight(const wchar_t* pText);

//@备注  文本删除右侧(末尾)空格
//@示例  文本型A 结果 = 文本删除末尾空格A(A" abc ") //结果为: A" abc"
//@参数  字符串
//@返回  结果文本A
//@别名  文本删除末尾空格A(字符串)
CXTextA text_trimRightA(const char* pText);

//@备注  此方法在原有字符串基础上修改
//@示例  字符型  数组1[10] = " abc "
// 文本删除末尾空格2(数组1) //结果为: " abc"
//@参数  字符串
//@别名  文本删除末尾空格2(字符串)
void text_trimRight2(const wchar_t* pText);

//@备注  此方法在原有字符串基础上修改
//@示例  字符型A  数组1[10] = A" abc "
// 文本删除末尾空格2A(数组1) //结果为: A" abc"
//@参数  字符串
//@别名  文本删除末尾空格2A(字符串)
void text_trimRight2A(const char* pText);

//@备注  删除文本中的空行
//@示例  文本型  结果 = 文本删除空行("123\n\n\n456") //结果为: "123\n456"
//@别名  文本删除空行(字符串)
CXText text_trimEmptyRow(const wchar_t* pText);

//@备注  删除文本中的空行
//@示例  文本型  结果 = 文本删除空行A(A"123\n\n\n456") //结果为: A"123\n456"
//@别名  文本删除空行A(字符串)
CXTextA text_trimEmptyRowA(const char* pText);

//@备注  转换为大写字符串, 在原有字符串上修改
//@示例  字符型  数组1[10] = "abc"
// 文本到大写(数组1) //结果为: "ABC"
//@参数  字符串
//@别名  文本到大写(字符串)
void text_upper(__in_out wchar_t* pText);

//@备注  转换为大写字符串, 在原有字符串上修改
//@示例  字符型A  数组1[10] = A"abc"
// 文本到大写A(数组1) //结果为: A"ABC"
//@参数  字符串
//@别名  文本到大写A(字符串)
void text_upperA(__in_out char* pText);

//@备注  文本到大写, 返回转换后的文本
//@示例  文本型 结果 = 文本到大写2("abc") //结果为: "ABC"
//@别名  文本到大写2(字符串)
CXText text_upper2(__in const wchar_t* pText);

//@备注  文本到大写, 返回转换后的文本
//@示例  文本型A 结果 = 文本到大写2A(A"abc") //结果为: A"ABC"
//@别名  文本到大写2A(字符串)
CXTextA text_upper2A(__in const char* pText);

//@备注  转换为小写字符串, 在原有字符串上修改
//@示例  字符型  数组1[10] = "ABC"
// 文本到小写(数组1) //结果为: "abc"
//@参数  字符串
//@别名  文本到小写(字符串)
void text_lower(__in_out wchar_t* pText);

//@备注  转换为小写字符串, 在原有字符串上修改
//@示例  字符型A  数组1[10] = A"ABC"
// 文本到小写A(数组1) //结果为: A"abc"
//@参数  字符串
//@别名  文本到小写A(字符串)
void text_lowerA(__in_out char* pText);

//@备注  文本到小写, 返回转换后的文本
//@示例  文本型 结果 = 文本到小写2("ABC")
// 调试输出(结果)  //结果为: "abc"
//@别名  文本到小写2(字符串)
CXText text_lower2(__in const wchar_t* pText);

//@备注  文本到小写, 返回转换后的文本
//@示例  文本型A 结果 = 文本到小写2A(A"ABC")
// 调试输出(结果)  //结果为: A"abc"
//@别名  文本到小写2A(字符串)
CXTextA text_lower2A(__in const char* pText);

//@备注  A-Z、a-z、0-9 中取出随机指定数量的字符
//@示例  文本型 结果 = 文本取随机字符(10) //结果为: "FRaqbC8wSA"
//@参数  随机字符数量
//@返回  结果文本
//@别名  文本取随机字符(字符数量)
CXText text_getRandChar(int length);

//@备注  A-Z、a-z、0-9 中取出随机指定数量的字符
//@示例  文本型A 结果 = 文本取随机字符A(10) //结果为: A"vpukBY1Ynv"
//@参数  随机字符数量
//@返回  结果文本
//@别名  文本取随机字符A(字符数量)
CXTextA text_getRandCharA(int length);

//@备注  返回指定数量数字字符: 0-9
//@示例  文本型 结果 = 文本取随机数字(10)  //结果为: "1740948824"
//@参数  随机字符数量
//@返回  结果文本
//@别名  文本取随机数字(字符数量)
CXText text_getRandNumber(int length);

//@备注  返回指定数量数字字符: 0-9
//@示例  文本型A 结果 = 文本取随机数字A(10)  //结果为: A"7891819361"
//@参数  随机字符数量
//@返回  结果文本
//@别名  文本取随机数字A(字符数量)
CXTextA text_getRandNumberA(int length);

//@备注  返回一个文本，其中包含指定次数的文本重复结果。
//@示例  文本型 结果 = 文本取重复("abc", 3) //结果为: "abcabcabc"
//@参数  该文本将用于建立返回的文本。如果为空，将返回一个空文本。
//@参数  重复次数
//@返回  返回结果文本
//@别名  文本取重复(需要重复的文本, 重复次数)
CXText text_getReText(const wchar_t* pText, int count);

//@备注  返回一个文本，其中包含指定次数的文本重复结果。
//@示例  文本型A 结果 = 文本取重复A(A"abc", 3) //结果为: A"abcabcabc"
//@参数  该文本将用于建立返回的文本。如果为空，将返回一个空文本。
//@参数  重复次数
//@返回  返回结果文本
//@别名  文本取重复A(需要重复的文本, 重复次数)
CXTextA text_getReTextA(const char* pText, int count);

//@备注  跳过左边空格和制表符TAB,单行模式
//@示例  常量 字符型 *结果 = 文本跳过空格("  abc 123")  //结果为: "abc 123"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本跳过空格(字符串)
const wchar_t* text_skipSpace(const wchar_t* pText);

//@备注  跳过左边空格和制表符TAB,单行模式
//@示例  常量 字符型A *结果 = 文本跳过空格A(A"  abc 123")  //结果为: A"abc 123"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本跳过空格A(字符串)
const char* text_skipSpaceA(const char* pText);

//@备注  跳过左边空格和制表符TAB,单行模式
//@示例  文本型  字符串 = " 123 456"
//常量 字符型* 地址 = 字符串.取地址()
//常量 字符型 * 结果 = 文本跳过空格2(地址, 地址 + 字符串.取大小()) //结果为: A"123 456"
//@参数  开始位置,字符串指针
//@参数  结束位置,字符串指针
//@返回  字符串指针
//@别名  文本跳过空格2(字符串, 结束地址)
const wchar_t* text_skipSpace2(const wchar_t* pText, const wchar_t* pEnd);

//@备注  跳过左边空格和制表符TAB,单行模式
//@示例  文本型A  字符串 = A" 123 456"
//常量 字符型A * 地址 = 字符串.取地址()
//常量 字符型A * 结果 = 文本跳过空格2A(地址, 地址 + 字符串.取大小()) //结果为: A"123 456"
//@参数  开始位置,字符串指针
//@参数  结束位置,字符串指针
//@返回  字符串指针A
//@别名  文本跳过空格2A(目标字符串, 结束地址)
const char* text_skipSpace2A(const char* pText, const char* pEnd);

//@备注  跳过左边空格和制表符TAB和换行符
//@示例 	 字符型  数组1[] = "  \n\n123"
// 常量 字符型* 结果 = 文本跳过空格和换行符(数组1) //结果为: "123"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本跳过空格和换行符(字符串)
const wchar_t* text_skipSpaceWrap(const wchar_t* pText);

//@备注  跳过左边空格和制表符TAB和换行符
//@示例 	 字符型A  数组1[] = A"  \n\n123"
// 常量 字符型* 结果 = 文本跳过空格和换行符A(数组1) //结果为: A"123"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本跳过空格和换行符A(文本地址)
const char* text_skipSpaceWrapA(const char* pText);

//@备注  跳过尾部空格和制表符TAB和换行符
//@示例  字符型  数组1[] = "123 \t\n"
// 常量 字符型* 结果 = 文本跳过尾部空格和换行符(数组1) //结果为: " \t\n"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本跳过尾部空格和换行符(文本地址)
const wchar_t* text_skipSpaceWrapRight(const wchar_t* pText);

//@备注  跳过尾部空格和制表符TAB和换行符
//@示例  字符型A  数组1[] = A"123 \t\n"
// 常量 字符型A* 结果 = 文本跳过尾部空格和换行符A(数组1) //结果为: A" \t\n  "
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本跳过尾部空格和换行符A(文本地址)
const char* text_skipSpaceWrapRightA(const char* pText);

//@备注  跳过尾部空格和制表符TAB和换行符
//@示例	常量 字符型 *字符串 = "123\t\n  "
// 常量 字符型* 结果 = 文本跳过尾部空格和换行符2(字符串, 字符串 + 文本取长度(字符串))
// //结果为:  " \t\n"
//@参数  开始位置, 字符串指针
//@参数  结束位置, 字符串指针
//@返回  字符串指针
//@别名  文本跳过尾部空格和换行符2(开始地址, 结束地址)
const wchar_t* text_skipSpaceWrapRight2(const wchar_t* pText, const wchar_t* pEnd);

//@备注  跳过尾部空格和制表符TAB和换行符
//@示例	常量 字符型A *字符串 = A"123\t\n  "
// 常量 字符型* 结果 = 文本跳过尾部空格和换行符2A(字符串, 字符串 + 文本取长度A(字符串))
// //结果为:  A" \t\n"
//@参数  开始位置, 字符串指针
//@参数  结束位置, 字符串指针
//@返回  字符串指针
//@别名  文本跳过尾部空格和换行符2A(文本地址, 结束地址)
const char* text_skipSpaceWrapRight2A(const char* pText, const char* pEnd);

//@备注  跳到行尾
//@示例  常量 字符型 *字符串 = "123\n456"
// 常量 字符型* 结果 = 文本跳过到行末尾(字符串) //结果为: "\n456"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本跳过到行末尾(字符串)
const wchar_t* text_skipEnd(const wchar_t* pText);

//@备注  跳到行尾
//@示例  常量 字符型A *字符串 = A"123\n456"
// 常量 字符型A* 结果 = 文本跳过到行末尾A(字符串) //结果为: A"\n456"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本跳过到行末尾A(字符串)
const char* text_skipEndA(const char* pText);

//@备注  跳到下一行
//@示例  常量 字符型 *字符串 = "123\n456"
// 常量 字符型* 结果 = 文本下一行(字符串)  //结果为: "456"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本下一行(字符串)
const wchar_t* text_skipNext(const wchar_t* pText);

//@备注  跳到下一行
//@示例  常量 字符型A *字符串 = A"123\n456"
// 常量 字符型A* 结果 = 文本下一行A(字符串)  //结果为: A"456"
//@参数  字符串指针
//@返回  字符串指针
//@别名  文本下一行A(字符串)
const char* text_skipNextA(const char* pText);

//@分组}  文本操作

//@分组{ 文件操作
//@备注 文件读写操作; 支持多种读写模式, 追加模块, 覆盖模式, 只读模式等
//  每次读写操作都是从当前读写位置开始, 操作完成后内部自动更新读写位置: [读写位置] = [当前读写位置] + [读写字节长度];
//  可手动移动文件读写位置;
//@doc  "doc/file_class.txt"
//@别名 文件类
class CXFile
{
	//@备注  文件指针
	FILE*  m_pFile;
	//@备注  文件长度
	size_t m_length;
public:
	CXFile();
	~CXFile();

	//@备注  打开以进行读取或追加写入。追加操作包括在新数据写入文件之前移除 EOF 标记。 
	//写入完成后，EOF 标记不会还原。
	//如果文件不存在，则创建文件。
	//@doc  "doc/file_class.txt"
	//@参数  文件名, 完整路径 或 相对路径
	//@返回  如果成功返回真, 否则返回假
	//@别名  打开(文件名)
	BOOL open(const wchar_t* pFileName);      //读写追加模式

	//@备注  打开以便读取。如果文件不存在或找不到，调用将失败
	//@示例  文件类  文件
	// 文件.打开只读("123.txt") //只读, 不支持写入
	//@参数  文件名, 完整路径 或 相对路径
	//@返回  如果成功返回真, 否则返回假
	//@别名  打开只读(文件名)
	BOOL openRead(const wchar_t* pFileName);  //只读模式

	//@备注  打开用于读取和写入的空文件。 如果文件存在，则其内容会被销毁
	//@示例  文件类  文件
	// 文件.打开写入("123.txt")
	//@参数  文件名, 完整路径 或 相对路径
	//@返回  如果成功返回真, 否则返回假
	//@别名  打开写入(文件名)
	BOOL openWrite(const wchar_t* pFileName);   //打开空文件写入

	//@备注  模式说明参见MSDN
	//@示例  文件类  文件
	// 文件.打开指定模式("123.txt", "wb+")  //读取和写入, 二进制方式
	//@参数  文件名, 完整路径 或 相对路径
	//@参数  打开模式, 例如: "ab+" , "wb+", "rb";
	// "r"  打开以便读取。 如果文件不存在或找不到，fopen_s 调用将失败。
	// "w"	打开用于写入的空文件。 如果给定文件存在，则其内容会被销毁
	// "a"	在文件末尾打开以进行写入（追加），在新数据写入到文件之前不移除文件末尾 (EOF) 标记。 如果文件不存在，则创建文件。
	// "r+"	打开以便读取和写入。 文件必须存在。
	// "w+"	打开用于读取和写入的空文件。 如果文件存在，则其内容会被销毁。
	// "a+"	打开以进行读取和追加。 追加操作包括在新数据写入文件之前移除 EOF 标记。 写入完成后，EOF 标记不会还原。 如果文件不存在，则创建文件。
	// "t"	在文本（转换）模式下打开。 输入时，回车换行 (CR-LF) 组合将转换为单一的换行 (LF)；输出时，LF 字符将转换为 CR-LF 组合。 CTRL+Z 将在输入时解释为文件尾字符。
	// "b"  在二进制（未转换）模式下打开；禁止涉及回车和换行字符的转换。
	//@返回  如果成功返回真, 否则返回假
	//@别名  打开指定模式(文件名, 模式字符串)
	BOOL openEx(const wchar_t* pFileName, const wchar_t* pMode); //扩展
	
	//@备注  关闭文件
	//@别名  关闭()
	void close();

	//@备注 取成员变量 文件指针(FILE* m_pFile)
	//@返回 返回 文件指针(FILE* m_pFile)
	//@别名 取文件指针()
	FILE* getFilePtr();

	//@备注  获取文件当前长度, 字节为单位
	//@示例  文件类  文件
	// 文件.打开("123.txt")
	// 整型 文件大小 = 文件.取长度()
	// 文件.关闭()
	//@返回  文件长度
	//@别名  取长度()
	size_t getLength();

	//@备注  先前getLength()获取的文件长度保存变量(m_length)的值; 如果修改文件后, 请使用getLength()重新获取长度
	//@返回  文件长度, 字节为单位
	//@别名  取长度_缓存()
	size_t getLength_temp();

	//@备注  获取当前读写位置
	//@返回  当前位置
	//@别名  取当前位置()
	size_t getCurPos();

	//@备注  读写位置: 移动到文件首
	//@返回  如果成功返回真, 否则返回假
	//@别名  移动到首()
	BOOL seekBegin();

	//@备注  读写位置: 移动到文件尾
	//@返回  如果成功返回真, 否则返回假
	//@别名  移动到尾()
	BOOL seekEnd();

	//@备注  移动读写位置
	//@doc   "doc\file_seekPos.txt"
	//@参数  偏移量, 从0开始
	//@参数  SEEK_CUR(1):文件指针的当前位置,  SEEK_END(2):文件结尾, SEEK_SET(0):文件开头,
	//@返回  如果成功返回真, 否则返回假
	//@别名  移动读写位置(偏移量, 起始位置)
	BOOL seekPos(size_t offset, int origin); //移动读写位置

	//@备注  读取文件内容, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 读取字节长度
	//@示例  文件类  文件
	// 文件.打开只读("123.txt")
	// 字符型A  数组1[10] = { 0 }
	// 文件.读取数据(数组1, 10)
	// 调试输出(数组1)
	// 文件.关闭()
	//@参数  内存块指针, 接收读取的内容
	//@参数  内存块大小(字节为单位)
	//@返回  返回读取长度
	//@别名  读取数据(内存块指针, 内存块大小)
	size_t readData(__out void* buf, __in size_t nBufSize);

	//@备注  读取一行内容, 从当前读写位置开始, 操作完成后并更新读写位置到下一行
	//@示例  文件类  文件
	// 文件.打开只读("123.txt")
	// 字符型A  数组1[10] = { 0 }
	// 文件.读取数据行(数组1, 10)
	// 调试输出(数组1)
	// 文件.关闭()
	//@参数  内存块指针, 接收读取的内容
	//@参数  内存块大小(字节为单位)
	//@返回  如果成功返回真, 否则返回假
	//@别名  读取数据行(内存块指针, 内存块大小)
	BOOL readDataRow(__out char* buf, __in int nBufSize);

	//@备注  读取内容保存到字节集, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 读取字节长度
	//@参数  读取长度(字节为单位), -1读取全部
	//@返回  结果字节集
	//@别名  读取到字节集(读取长度)
	CXBytes readDataBytes(size_t nLength=-1);
	
	//@备注  读取文本内容, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 读取字节长度
	//@参数  nLength 读取长度(字节为单位), -1读取全部
	//@返回  返回读取的文本内容
	//@别名  读取文本A(读取长度)
	CXTextA readTextA(size_t nLength = -1);

	//@备注  读取一个字符A, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 读取字节长度
	//@返回  返回读取的字符A或返回 EOF 以指示错误或文件尾
	//@别名  读取字符A()
	char readChar();

	//@备注  读取一个宽字符, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 读取字节长度
	//@返回  返回读取的字符W或返回 EOF 以指示错误或文件尾
	//@别名  读取字符()
	wchar_t readWChar();

	//@备注  写入内容到文件, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 写入字节长度
	//@参数  数据指针
	//@参数  数据长度(字节为单位)
	//@返回  返回写入长度, 字节为单位
	//@别名  写入数据(数据指针, 写入长度)
	size_t writeData(const void* data, size_t nLength);

	//@备注  写入文本到文件, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 写入字节长度
	//@参数  要写入的字符串
	//@参数  数据长度(字符为单位), -1写入全部,
	//@返回  返回写入长度(字符为单位)
	//@别名  写入文本(字符串, 写入长度)
	size_t writeText(const wchar_t* data, size_t nLength = -1);

	//@备注  写入文本到文件, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 写入字节长度
	//@参数  要写入的字符串
	//@参数  写入长度(字符为单位), -1写入全部
	//@返回  返回写入长度, 字节为单位
	//@别名  写入文本A(字符串, 写入长度)
	size_t writeTextA(const char* data, size_t nLength = -1);
	
	//@备注  写入文本到文件, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 写入字节长度
	//@参数  要写入的文本
	//@参数  写入长度(字符为单位), -1写入全部
	//@返回  返回写入长度(字符为单位)
	//@别名  写入文本2(文本,写入长度)
	size_t writeText2(const CXText& text, size_t nLength = -1);
	
	//@备注  写入文本到文件, 从当前读写位置开始, 操作完成后并更新读写位置: [读写位置] = [当前读写位置] + 写入字节长度
	//@参数  要写入的文本
	//@参数  写入长度(字符为单位), -1写入全部
	//@返回  返回写入长度, 字节为单位
	//@别名  写入文本2A(文本,写入长度)
	size_t writeText2A(const CXTextA& text, size_t nLength = -1);

	//@备注 判断是否读取到文件末尾, 例如:文件长度6, 读取完6个字符即返回真
	//@返回 如果到末尾返回真, 否则返回假
	//@别名 是否到文件尾()
	BOOL isEnd();

	//@备注 判断读取是否超过文件末尾,例如:文件长度6, 当读取第7个字符时才返回真
	//@返回 如果超过末尾返回真, 否则返回假
	//@别名 是否EOF()
	BOOL isEof();
};

//@备注  将数据写入到文件中, 如果文件不存在将自动创建, 如果存在将清空先前内容
//@示例  文件写入("D:\\test\\123.txt", A"炫语言", 文本取长度A(A"炫语言")) //完整路径
// 文件写入("doc\\123.txt", A"炫语言", 文本取长度A(A"炫语言")) //相对路径
//@参数  文件名, 完整路径或相对当前目录路径
//@参数  要写入的数据
//@参数  数据长度(字节为单位)
//@返回  返回写入长度, 字节为单位
//@别名  文件写入(文件名, 写入数据, 数据长度)
UINT xcl_writeFile(const wchar_t* pFileName, const void* data, int length);

//@备注  从文件中读取内容
//@示例  字节集 结果1 = 文件读取("d:\\test\\abc.txt") //完整路径
// 字节集 结果2 = 文件读取("doc\\abc.txt")  //相对路径
//@参数  文件名, 完整路径或相对当前目录路径
//@参数  读取长度(字节为单位), -1 读取全部
//@返回  返回读取的数据
//@别名  文件读取(文件名, 读取长度)
CXBytes xcl_readFile(const wchar_t* pFileName, int length = -1);

//@备注  从文件中读取文本内容
//@示例  文本型A 结果1 = 文件读取文本A("d:\\test\\abc.txt")
// 文本型A 结果2 = 文件读取文本A("abc.txt")
//@参数  文件名, 完整路径 或 相对当前目录路径
//@参数  读取长度(字节为单位), -1 读取全部
//@返回  返回读取的文本内容
//@别名  文件读取文本A(文件名, 读取长度)
CXTextA xcl_readFileTextA(const wchar_t* pFileName, int length = -1);

//@备注  创建文件夹, 父文件夹必须存在
//@示例  文件夹创建("D:\\Test\\abc") //完整路径
// 文件夹创建("doc\\abc2")   //相对路径
//@参数  文件夹路径,  完整路径或相对当前目录路径
//@返回  成功返回0, 失败返回-1, 文件已存在返回-2
//@别名  文件夹创建(路径)
int xcl_createFolder(const wchar_t* pPath);

//@备注  创建文件夹, 父文件夹必须存在
//@示例  文件夹创建A(A"D:\\Test\\abc") //完整路径
// 文件夹创建A(A"doc\\abc2")   //相对路径
//@参数  文件夹完整路径
//@返回  如果成功返回0, 失败返回-1, 文件已存在返回-2
//@别名  文件夹创建A(路径)
int xcl_createFolderA(const char* pPath);

//@备注 创建文件夹及子文件夹,如果路径不存在则创建
//@示例 文件夹创建2("D:\\Test\\abc") //完整路径
//@参数 路径(完整路径)
//@返回 成功返回真, 否则返回假
//@别名 文件夹创建2(路径)
BOOL xcl_createFolder2(const wchar_t*  pPath);

//@备注  删除指定文件夹及子文件
//@示例  文件夹删除("D:\\Test\\doc")  //删除文件夹及文件夹中内容
//@参数  文件夹路径(完整路径)
//@返回  如果成功返回真, 否则返回假
//@别名  文件夹删除(文件夹路径)
BOOL xcl_deleteFolder(const wchar_t* path);

//@备注  判断文件夹是否存在, 内部调用API: GetFileAttributes()
//@示例  如果 (文件夹是否存在("Test\\doc"))  //完整路径 或 相对路径
// -- 调试输出("存在")
// 否则
// -- 调试输出("不存在")
//@参数  目录名, 完整路径或相对当前目录路径
//@返回  如果存在返回真, 否则相反
//@别名  文件夹是否存在(目录名)
BOOL xcl_isFolderExist(const wchar_t* pDir);

//@备注 复制文件到目标路径, 如果路径不存在将自动创建路径
//@示例 文件复制("D:\\Test\\doc\\index.txt", E:\\Test\\doc\\index.txt")
//@参数 目标文件(完整路径), 该参数指定了复制后的新文件的完整路径和文件名, 如果目标路径不存在将自动创建
//@参数 源文件(完整路径), 该参数指定了要复制的现有文件的完整路径和文件名
//@返回 如果成功返回真, 否则返回假
//@别名 文件复制(目标文件, 源文件)
BOOL xcl_copyFile(const wchar_t* dest, const wchar_t*  src);

//@备注 判断文件是否存在, 内部调用API: GetFileAttributes()
//@示例  如果 (文件是否存在("doc\\index.txt")) //完整路径 或 相对路径
// -- 调试输出("存在")
// 否则
// -- 调试输出("不存在")
//@参数 文件名, 完整路径或相对当前目录路径
//@返回 如果文件存在返回真, 否则相反
//@别名 文件是否存在(文件名)
BOOL xcl_isFileExist(const wchar_t* pFileName);

//@备注 判断文件是否存在, 内部调用API:_waccess_s(); 有用户反馈: 在内存中没保存的文件也判断为存在, 可以尝试使用 文件是否存在()
//@示例  如果 (文件是否存在2("doc\\index.txt")) //完整路径 或 相对路径
// -- 调试输出("存在")
// 否则
// -- 调试输出("不存在")
//@参数 文件名, 完整路径或相对当前目录路径
//@返回 如果文件存在返回真, 否则相反
//@别名 文件是否存在2(文件或目录路径)
BOOL xcl_isFileExist2(const wchar_t* pFileName);

//@备注  判断文件或文件夹是否存在, 内部调用API: GetFileAttributes()
//@示例  如果 (文件或文件夹是否存在("doc\\index.txt")) //文件 或 文件夹, 完整路径 或 相对路径
// -- 调试输出("存在")
// 否则
// -- 调试输出("不存在")
//@参数  路径, 完整路径或相对当前目录路径
//@返回  如果存在返回真, 否则相反
//@别名  文件或文件夹是否存在(路径)
BOOL xcl_isPathExist(const wchar_t* pPath);

/*@声明
//@备注  删除指定文件
//@参数  文件名, 完整路径 或 相对路径
//@返回  如果该函数成功，则返回值为非零值, 如果函数失败，则返回值为零(0). 要获得更多的错误信息，请调用 GetLastError
//@别名  文件删除(文件路径)
BOOL WINAPI DeleteFile(LPCWSTR lpFileName);

//@备注  当使用文件时，确定指定的文件是否存在并且可以根据 mode值访问。 在使用目录时，_access_s 只确定指定的目录是否存在。 在 Windows 2000 和更高版本的操作系统上，所有目录有读取和写入权限。
//@示例  如果 (0 == 文件判断权限A(A"test\\111.txt", 0))  //完整路径 或 相对路径
// -- 调试输出("存在")
// 否则
// -- 调试输出("不存在")
//@参数  文件或目录路径, 完整路径或相对路径
//@参数  权限设置; 0: 仅检查是否存在, 2:写入权限, 4:读取权限, 6:读取和写入权限
//@返回  如果该文件具有给定的模式，则函数将返回 0。 如果文件不存在或无法以指定模式进行访问，则该函数将返回错误代码。 在这种情况下，函数集合返回代码错误并将 errno 设置为相同的值
//  EACCES(13): 访问被拒绝。 文件权限的设置不允许指定的访问权限
//  ENOENT(2): 未找到文件名或路径
//  EINVAL(22): 无效参数
//@别名  文件判断权限A(文件或目录路径, 权限设置)
int _access_s(const char* path, int mode);

//@备注  当使用文件时，函数来确定指定的文件是否存在并且可以根据 mode值访问。 在使用目录时，_waccess_s 只确定指定的目录是否存在。 在 Windows 2000 和更高版本的操作系统上，所有目录有读取和写入权限。
//@示例  如果 (0 == 文件判断权限("test\\111.txt", 0))  //完整路径 或 相对路径
// -- 调试输出("存在")
// 否则
// -- 调试输出("不存在")
//@参数  文件或目录路径, 完整路径或相对路径
//@参数  权限设置  0:仅检查是否存在 2:写入权限 4:读权限 6:读取和写入权限
//@返回  如果该文件具有给定的模式，则函数将返回 0。 如果文件不存在或无法以指定模式进行访问，则该函数将返回错误代码。 在这种情况下，函数集合返回代码错误并将 errno 设置为相同的值
//  EACCES(13): 访问被拒绝。 文件权限的设置不允许指定的访问权限
//  ENOENT(2): 未找到文件名或路径
//  EINVAL(22): 无效参数
//@别名  文件判断权限(文件或目录路径, 权限设置)
int _waccess_s(const wchar_t* path, int mode);

//@备注  未别名的属性可忽略
//@示例  //注意 此结构体名称比较特殊, 他即是结构体也是函数名, 所以需要通过嵌入代码方式 定义变量, 显示的指定他为结构体
//#嵌入代码
//struct _stat32  st;
//#嵌入代码结束
//@别名  文件信息结构
struct _stat32
{
	UINT           st_dev; //@备注  与st_rdev相同
	unsigned short st_ino;
	unsigned short st_mode;
	short          st_nlink;
	short          st_uid;
	short          st_gid;
	UINT           st_rdev;//@别名  驱动器号
	long           st_size;//@别名  大小
	long     st_atime;     //@别名  访问时间
	long     st_mtime;     //@别名  修改时间
	long     st_ctime;     //@别名  创建时间
};

//@备注  获取文件信息
//@参数  文件名
//@参数  接收返回结果
//@返回  成功返回0; 失败返回-1, 错误码 errno 以下值:
//  EACCES(13): 访问被拒绝。 文件权限的设置不允许指定的访问权限
//  ENOENT(2): 未找到文件名或路径
//  EINVAL(22): 无效参数
//@别名  文件取信息(文件名, 返回信息)
int _wstat32(__in const wchar_t* _FileName, __out _stat32* _Stat);

*/

//@备注 显示一个文件夹选择对话框, 选择一个文件夹, 返回文件夹路径和名称
//@示例 文本型 结果 = 打开文件夹对话框("D:\\桌面文件", "标题")
//@参数 默认目录
//@参数 对话框标题
//@参数 父窗口句柄
//@返回 选择的文件夹全路径名称
//@别名 打开文件夹对话框(默认目录, 标题, 父窗口句柄)
CXText xcl_openFloderDlg(const wchar_t* pDefaultDir, const wchar_t* pTitle = NULL, HWND hWnd = NULL);

//@备注 显示一个单文件打开对话框，允许用户选择或输入一个所需要打开的已存在文件，返回用户所选择或输入后的结果文本
//@示例 文本型 结果 = 打开文件对话框("D:\\桌面文件", "标题", "所有文件(*.*)|*.*|文本文件(*.txt)|*.txt")
//@参数 默认目录
//@参数 对话框标题, 可为空
//@参数 过滤器, 例如："文本文件(*.txt)|*.txt|所有文件(*.*)|*.*", 如果被省略，则默认没有过滤器
//@参数 是否改变目录, 在对话框关闭后是否改变当前文件目录
//@参数 父窗口句柄
//@返回 返回用户选择的文件路径和名称
//@别名 打开文件对话框(默认目录, 标题, 过滤器, 是否改变目录, 父窗口句柄)
CXText xcl_openFileDlg(const wchar_t* pDefaultDir, const wchar_t* pTitle , const wchar_t* pFilter = L"所有文件(*.*)|*.*|文本文件(*.txt)|*.txt", BOOL bChangeDir = FALSE, HWND hWnd = NULL);

//@备注 显示一个多文件打开对话框，允许用户选择或输入多个所需要打开的已存在文件，返回用户所选择或输入后的结果文本数组
//@示例 动态数组<文本型> 结果数组 = 打开多文件对话框("D:\\桌面文件", "标题", "所有文件(*.*)|*.*|文本文件(*.txt)|*.txt")
//@参数 默认目录
//@参数 对话框标题, 可为空
//@参数 过滤器, 例如："文本文件(*.txt)|*.txt|所有文件(*.*)|*.*", 如果被省略，则默认没有过滤器
//@参数 是否改变目录, 在对话框关闭后是否改变当前文件目录
//@参数 父窗口句柄
//@返回 动态数组, 保存用户选择的文件 
//@别名 打开多文件对话框(默认目录, 标题, 过滤器, 是否改变目录, 父窗口句柄)
CXVector<CXText> xcl_openFilesDlg(const wchar_t* pDefaultDir, const wchar_t* pTitle, const wchar_t* pFilter = L"所有文件(*.*)|*.*|文本文件(*.txt)|*.txt", BOOL bChangeDir = FALSE, HWND hWnd = NULL);

//@备注 显示一个文件保存对话框，允许用户选择或输入一个所需要保存的文件名，返回用户所选择或输入后的结果文本
// 例如: 文本型 结果 = 保存文件对话框("D:\\桌面文件", "标题", "所有文件(*.*)|*.*|文本文件(*.txt)|*.txt")
//@参数 --pDefaultDir 默认目录
//@参数 --pTitle  对话框标题, 可为空
//@参数 --pFilter 过滤器, 例如："文本文件(*.txt)|*.txt|所有文件(*.*)|*.*", 如果被省略，则默认没有过滤器
//@参数 --pDefExt 默认文件扩展名, 如果用户没有输入扩展名,将使用默认扩展名, 例如: ".txt"
//@参数 --bChangeDir 是否改变目录, 在对话框关闭后是否改变当前文件目录
//@参数 --hWnd 父窗口句柄
//@返回 返回用户选择的或输入的文件名
//@别名 保存文件对话框(默认目录, 标题, 过滤器, 是否改变目录, 父窗口句柄)
CXText xcl_saveFileDlg(const wchar_t* pDefaultDir, const wchar_t* pTitle, const wchar_t* pFilter = L"所有文件(*.*)|*.*|文本文件(*.txt)|*.txt", const wchar_t* pDefExt= L".txt", BOOL bChangeDir = FALSE, HWND hWnd = NULL);

//@分组}

//@分组{  文件查找
/*@声明

//@别名 文件查找结构
struct WIN32_FIND_DATAW {
	DWORD    dwFileAttributes;   //@别名 文件属性
	FILETIME ftCreationTime;     //@别名 创建日期
	FILETIME ftLastAccessTime;   //@别名 最后存取日期
	FILETIME ftLastWriteTime;    //@别名 最后修改日期
	DWORD    nFileSizeHigh;      //@别名 长度高32位
	DWORD    nFileSizeLow;       //@别名 长度低32位
	DWORD    dwReserved0;        //@别名 内部使用
	DWORD    dwReserved1;        //@别名 内部使用
	WCHAR    cFileName[MAX_PATH];     //@别名 文件名
	WCHAR    cAlternateFileName[14];  //@别名 短文件名
};

//@备注  用于在指定的目录中搜索与指定文件名匹配的第一个文件或目录。它会返回一个搜索句柄，
//后续可以使用该句柄结合 FindNextFile 函数来继续搜索其他匹配的文件或目录。
//@doc  "doc\FindFirstFile.txt"
//@参数 搜索的文件名, 参数可如下：c:\Windows\*.*，详见MSDN
//@参数 文件查找信息，这个结构用于装载与找到的文件有关的信息。该结构可用于后续的搜索
//@返回 如执行成功，返回一个搜索句柄。如果出错，返回一个INVALID_HANDLE_VALUE常数，一旦不再需要，应该用FindClose函数关闭这个句柄
//@别名 文件查找第一个(搜索的文件名, 文件查找信息)
HANDLE WINAPI FindFirstFile(const wchar_t* lpFileName, __out WIN32_FIND_DATAW* lpFindFileData);

//@备注 它通常与 FindFirstFile 配合使用。FindFirstFile 用于在指定目录中查找与指定文件名匹配的第一个文件或目录，
//而 FindNextFile 则用于在同一搜索操作中继续查找下一个匹配的文件或目录。
//@doc  "doc\FindFirstFile.txt"
//@参数 文件句柄, 由 FindFirstFile 函数返回的搜索句柄，用于标识当前的搜索操作。这个句柄在整个搜索过程中保持不变，直到调用 FindClose 函数关闭它
//@参数 文件查找信息, 该结构体用于存储找到的下一个文件或目录的相关信息，包括文件名、文件大小、文件属性、创建时间、访问时间和修改时间等
//@返回 如执行成功，返回TRUE。否则为FALSE。
//@别名 文件查找下一个(文件句柄, 文件信息)
BOOL WINAPI FindNextFile(HANDLE hFindFile, __out WIN32_FIND_DATAW* lpFindFileData);

//@备注  用于关闭由 FindFirstFile 或 FindFirstFileEx 函数创建的文件搜索句柄。
//当你使用这些函数进行文件或目录搜索操作时，系统会分配资源来维护搜索状态，
//搜索结束后，必须调用 FindClose 函数来释放这些资源，避免资源泄漏
//@doc  "doc\FindFirstFile.txt"
//@别名  文件查找关闭()
BOOL  WINAPI FindClose(__in_out HANDLE hFindFile);

@别名  文件属性_只读文件
#define FILE_ATTRIBUTE_READONLY             0x00000001   ///<只读文件
@别名  文件属性_隐藏文件
#define FILE_ATTRIBUTE_HIDDEN               0x00000002   ///<隐藏文件
@别名  文件属性_系统文件
#define FILE_ATTRIBUTE_SYSTEM               0x00000004   ///<系统文件
@别名  文件属性_文件目录
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010   ///<文件目录
@别名  文件属性_存档文件
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020   ///<存档文件
#define FILE_ATTRIBUTE_DEVICE               0x00000040   ///<此值保留供系统使用
@别名  文件属性_无属性
#define FILE_ATTRIBUTE_NORMAL               0x00000080   ///<未设置其他属性的文件。 此属性仅在单独使用时才有效。
@别名  文件属性_临时文件
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100   ///<用于临时存储的文件。 如果有足够的缓存内存可用，文件系统会避免将数据写回到大容量存储，因为通常情况下，应用程序在句柄关闭后会删除临时文件。 在这种情况下，系统可以完全避免写入数据。 否则，在句柄关闭后写入数据。
@别名  文件属性_稀疏文件
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200   ///<是稀疏文件的文件 
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400   ///<具有关联的重新分析点的文件或目录，或作为符号链接的文件。
@别名  文件属性_压缩文件
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800   ///<压缩的文件或目录
#define FILE_ATTRIBUTE_OFFLINE              0x00001000   ///<文件的数据不会立即可用。 此属性指示文件数据以物理方式移动到脱机存储。 此属性由远程存储（分层存储管理软件）使用。 应用程序不应随意更改此属性。
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000   ///<内容索引服务不会为文件或目录编制索引。
@别名  文件属性_加密文件
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000   ///<加密的文件或目录。 对于文件，文件中的所有数据流都已加密。 对于目录，加密是新创建的文件和子目录的默认加密。
#define FILE_ATTRIBUTE_VIRTUAL              0x00010000   ///<此值保留供系统使用。

//@备注   句柄本质上是一个用于标识系统资源（像文件、窗口、进程、线程等）的唯一整数或指针。
//当一个函数尝试返回一个句柄，但由于某种原因（例如资源分配失败、文件不存在等）无法返回有效的句柄时，就会返回 INVALID_HANDLE_VALUE。
//它作为一个特殊的标记，用来指示当前得到的句柄不指向任何有效的系统资源。
//@示例  HANDLE hFile = CreateFile(...)
// 如果 (hFile == INVALID_HANDLE_VALUE)
// -- 调试输出("无效句柄")
//@别名   无效句柄值
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)  ///<表示无效的句柄

//@备注  主要在文件操作相关的函数里使用。在获取文件大小时，若函数无法正确获取文件的大小（比如文件句柄无效、文件损坏等情况）,
//就会返回 INVALID_FILE_SIZE。它起到一个错误标识的作用，提示调用者文件大小获取操作失败
//@示例  HANDLE hFile = CreateFile(...)
// DWORD fileSize = GetFileSize(hFile, NULL)
// 如果 (fileSize == INVALID_FILE_SIZE)
// -- 调试输出("无效文件大小")
//@别名  无效文件大小
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)        ///<表示无效的文件大小

//@备注  用于文件指针操作。在使用 SetFilePointer 等函数来移动文件指针时，
//如果操作失败（例如传入的参数无效、文件句柄无效等），
//函数会返回 INVALID_SET_FILE_POINTER。它表明文件指针的设置操作没有成功完成。
//@示例  HANDLE hFile = CreateFile(...)
//DWORD newPosition = SetFilePointer(hFile, 1000, NULL, FILE_BEGIN);
// 如果 (newPosition == INVALID_SET_FILE_POINTER)
// -- 调试输出("无效的文件指针设置")
//@别名  无效的文件指针设置
#define INVALID_SET_FILE_POINTER ((DWORD)-1)         ///<无效的文件指针位置

//@备注  用于文件属性操作。当调用 GetFileAttributes 等函数来获取文件的属性时，
//如果无法获取到有效的文件属性（比如文件不存在、路径错误等），函数就会返回 INVALID_FILE_ATTRIBUTES。
//它作为一个错误信号，告知调用者文件属性获取操作出现了问题。
//@示例  DWORD fileAttributes = GetFileAttributes("nonexistent_file.txt");
// 如果 (fileAttributes == INVALID_FILE_ATTRIBUTES)
// -- 调试输出("获取文件属性失败")
//@别名  无效文件属性
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)          ///<无效的文件属性
*/

//@分组}


//----------------------
//@分组{  日期时间

/*@声明
//@备注 用于表示 64 位整数。在 Windows 编程里，许多涉及到大范围数值计算、高精度计时或者处理大文件大小的场景，
//32 位整数往往无法满足需求，这时就会用到 LARGE_INTEGER 结构体。
//@doc "doc\QueryPerformanceFrequency.txt"
//@别名 大整数结构
struct LARGE_INTEGER {
	LONGLONG QuadPart;
};

//@备注 此函数检索高精度性能计数器的频率
//@doc "doc\QueryPerformanceFrequency.txt"
//@返回 成功返回真,否则返回假
//@别名 取高精度计数器频率(接收返回)
BOOL WINAPI QueryPerformanceFrequency(__out LARGE_INTEGER *lpFrequency);

//@备注 此函数检索高精度性能计数器的当前值
//@doc "doc\QueryPerformanceFrequency.txt"
//@返回 成功返回真,否则返回假
//@别名 取高精度计数器当前值(接收返回)
BOOL WINAPI QueryPerformanceCounter(__out LARGE_INTEGER *lpPerformanceCount);


//@备注 结构化时间是一种用于表示年、月、日、时、分、秒等时间元素的数据结构。在C语言中，结构化时间通常以struct tm类型表示。这种方式更容易理解和处理
//时间戳INT64为: 函数根据系统时钟返回自协调世界时 (UTC) 1970 年 1 月 1 日午夜 (00:00:00) 以来经过的秒数
//@doc "doc\xcl_getTimeTM.txt"
//@别名 日期时间TM结构
struct tm
{
	//@备注 秒 – 取值区间为[0,59]
	//@别名 秒数
	int tm_sec;   // seconds after the minute - [0, 60] including leap second

	//@备注 分 - 取值区间为[0,59]
	//@别名 分钟
	int tm_min;   // minutes after the hour - [0, 59]

	//@备注 时 - 取值区间为[0,23]
	//@别名 小时
	int tm_hour;  // hours since midnight - [0, 23]

	//@备注 一个月中的日期 - 取值区间为[1,31]
	//@别名 日
	int tm_mday;  // day of the month - [1, 31]

	//@备注 月份（从一月开始，0代表一月） - 取值区间为[0,11]
	//@别名 月
	int tm_mon;   // months since January - [0, 11]

	//@备注 年份，其值等于实际年份减去1900
	//@别名 年
	int tm_year;  // years since 1900

	//@备注 星期 – 取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推
	//@别名 星期
	int tm_wday;  // days since Sunday - [0, 6]

	//@备注 从每年的1月1日开始的天数 – 取值区间为[0,365]，其中0代表1月1日，1代表1月2日，以此类推
	//@别名 年日
	int tm_yday;  // days since January 1 - [0, 365]

	//@备注 对于 gmtime ，始终为0。夏令时标识符，实行夏令时的时候，tm_isdst为正。不实行夏令时的时候，tm_isdst为0；不了解情况时，tm_isdst()为负
	//@别名 夏令时
	int tm_isdst; // daylight savings time flag
};
*/

//@备注  对TM结构操作包装类
//@doc "doc\xcl_getTimeTM.txt"
//@别名 日期时间TM类
class  CDateTimeTM
{
public:
	//@隐藏{
	CDateTimeTM();
	CDateTimeTM(struct tm& tm_);
	CDateTimeTM(INT64 time_);
	//@隐藏}
	tm m_tm;
	int getYear();      //@别名 取年()
	int getMonth();     //@别名 取月()
	int getDay();       //@别名 取日()
	int getHour();      //@别名 取时()
	int getMinute();    //@别名 取分()
	int getSecond();    //@别名 取秒()
	int getDayOfWeek(); //@别名 取周()
};

/*@声明

//@备注 检索自系统启动以来经过的毫秒数，最长为49.7天, 旧名称 "取系统时钟()"
//@示例 DWORD  结构 = 取系统运行时间()
//@返回 时间毫秒数
//@别名 取系统运行时间()
DWORD GetTickCount();

//@备注 检索自系统启动以来经过的毫秒数，最长为49.7天, 旧名称 "取系统时钟64()"
//@示例 UINT64  结构 = 取系统运行时间64()
//@返回 时间毫秒数
//@别名 取系统运行时间64()
UINT64 GetTickCount64();

*/
//@备注  获取本地时间TM结构
//@doc "doc\xcl_getTimeTM.txt"
//@别名 取系统时间TM()
struct tm xcl_getTimeTM();

//@备注 格式化TM时间到文本型, 格式字符串参见MSDN: https://learn.microsoft.com/zh-cn/cpp/c-runtime-library/reference/strftime-wcsftime-strftime-l-wcsftime-l?view=msvc-170
//@doc "doc\xcl_getTimeTM.txt"
//@参数 格式字符串, 可空; 若参数为空输出格式为:2020-10-29 12:05:03, 否则根据用户指定的格式字符串输出
// 例如: "%Y-%m-%d %X" -> 年-月-日 小时:分:秒;  
// 例如: "%Y-%m-%d %H:%M:%S" -> 年-月-日 小时:分:秒;
//@参数 时间结构TM
//@返回 文本型时间
//@别名 TM到文本格式化(格式字符串, 时间结构TM)
CXText xcl_formatTimeTM(__in const wchar_t* pFormat, __in const tm* tm_);

//@备注 获取系统时间转换到文本返回, 格式字符串参见MSDN: https://learn.microsoft.com/zh-cn/cpp/c-runtime-library/reference/strftime-wcsftime-strftime-l-wcsftime-l?view=msvc-170
//@参数 格式字符串, 可空;若参数为空输出格式为:2020-10-29 12:05:03, 否则根据用户指定的格式字符串输出
// 例如: "%Y-%m-%d %X" -> 年-月-日 小时:分:秒;  
// 例如: "%Y-%m-%d %H:%M:%S" -> 年-月-日 小时:分:秒;
//@参数 时间戳, 时间表示为自 1970 年 1 月 1 日午夜 (00:00:00)，协调世界时 (UTC) 以来所经过的秒数
//@示例 调试输出(取系统时间到文本()) //结果为: 2025-02-10 10:33:27
//调试输出(取系统时间到文本("%Y-%m-%d")) //结果为: 2025-02-10
//@返回 结果文本
//@别名 取系统时间到文本(格式字符串)
CXText xcl_getTimeToText(const wchar_t* pFormat= NULL);

//@备注 获取当前系统日期, 返回固定格式文本
//@示例 调试输出(取系统日期到文本())  //结果为: 2025-02-10
//@返回 结果文本
//@别名 取系统日期到文本()
CXText xcl_getDate();

/*@声明

//@备注  用于表示一个具体的日期和时间。它将日期和时间拆分为年、月、日、时、分、秒、毫秒等各个部分，
//方便开发者对时间进行读取、修改和操作。在 Windows 编程中，SYSTEMTIME 结构体常用于获取系统当前时间、
//设置系统时间、格式化时间显示等场景
//@doc  "doc\GetSystemTime.txt"
//@别名  系统时间结构
struct SYSTEMTIME
{
	//@别名  年
	WORD wYear; //@备注  表示年份，取值范围是从 1601 到 30827。
	//@别名  月
	WORD wMonth;  //@备注  表示月份，取值范围是从 1（一月）到 12（十二月）
	//@别名 周
	WORD wDayOfWeek;  //@备注  表示星期几，取值范围是从 0（星期日）到 6（星期六）
	//@别名  日
	WORD wDay;  //@备注  表示一个月中的日期，取值范围是从 1 到 31
	//@别名 小时
	WORD wHour;  //@备注  表示小时，取值范围是从 0 到 23
	//@别名 分钟
	WORD wMinute;  //@备注  表示分钟，取值范围是从 0 到 59
	//@别名 秒
	WORD wSecond;  //@备注  表示秒，取值范围是从 0 到 59
	//@别名 毫秒
	WORD wMilliseconds;  //@备注  表示毫秒，取值范围是从 0 到 999
};

//@doc  "doc\GetSystemTime.txt"
//@别名  取系统时间结构(系统时间结构)
VOID WINAPI GetSystemTime(__out SYSTEMTIME* lpSystemTime);
*/

//@备注 获取某月天数
//@示例 调试输出(取某月天数(2025, 2))  //结果为: 28
//@参数 年
//@参数 月
//@返回 当前月天数
//@别名 取某月天数(年, 月)
int xcl_getDayInMonth(int y, int m);

/*@声明
//@备注 获取两个时间值间隔,以秒为单位
//提示: 在大多数情况下，两个时间戳直接相减 它们的值是相同的，但 difftime 函数在可移植性和类型兼容性方面更具优势
//@示例 	长整型  开始时间 = 时间戳从系统(NULL)
// 延时(1000)
// 长整型  结束时间 = 时间戳从系统(NULL)
// 调试输出(取时间间隔(结束时间, 开始时间))
// 调试输出(结束时间 - 开始时间)
//@参数 结束时间戳, 用于表示自 1970 年 1 月 1 日午夜（UTC）以来经过的秒数。
//@参数 开始时间戳, 用于表示自 1970 年 1 月 1 日午夜（UTC）以来经过的秒数。
//@返回 返回从 timeStart 到 timeEnd 流逝的时间（以秒为单位）。 返回值为双精度浮点数字。 返回值可能为 0，表示错误。
//@别名 取时间间隔(结束时间戳, 开始时间戳)
double difftime(INT64 timeEnd, INT64 timeStart);
*/

/*@声明

//@备注  FILETIME 是 Windows 操作系统中用于表示文件相关时间戳的结构体，也可用于系统中的其他时间表示。
//它以一种高精度的方式记录时间，其时间值基于自 1601 年 1 月 1 日午夜（UTC）以来的 100 纳秒间隔数。
//在 Windows API 中，许多文件操作函数（如 CreateFile、GetFileAttributesEx 等）都会涉及到 FILETIME 结构体，
//用于获取或设置文件的创建时间、最后访问时间和最后修改时间等信息
//@doc  "doc\FILETIME.txt"
//@别名 文件时间结构
struct FILETIME {
	DWORD  dwLowDateTime;  //@别名 低32位的时间值
	DWORD  dwHighDateTime; //@别名 高32位的时间值
};

//@备注 获取文件的创建时间、最后访问时间和最后修改时间，并将这些时间信息存储在 FILETIME 结构体中
//@doc  "doc\FILETIME.txt"
//@别名 取文件时间(文件句柄, 创建时间, 最后访问时间, 最后写入时间)
BOOL WINAPI GetFileTime(__in  HANDLE hFile,	__out_opt FILETIME* lpCreationTime,__out_opt FILETIME* lpLastAccessTime, __out_opt FILETIME* lpLastWriteTime);

//@doc  "doc\FILETIME.txt"
//@别名  文件时间结构到系统时间(文件时间, 系统时间)
BOOL WINAPI FileTimeToSystemTime(__in const FILETIME *lpFileTime, __out SYSTEMTIME* lpSystemTime);
*/

/*@声明

//@备注  返回自1970年1月1日午夜以来经过的秒数，如果出现错误，则返回-1
//@doc  "doc\time.txt"
//@参数  接收返回值, 可为空 NULL
//@返回  获取结果时间
//@别名  时间戳从系统(参数等于返回值_填空即可)
INT64 time(INT64* timer);

//@备注  将tm结构时间值转换为时间戳
//@doc  "doc\time.txt"
//@返回  时间戳, 返回自1970年1月1日午夜以来经过的秒数，如果出现错误，则返回-1
//@别名  时间戳从TM(输入时间TM)
INT64 mktime(__in struct tm* tm_);

*/

//@备注  从系统时间结构转换到时间戳
//@示例  系统时间结构  st
// 取系统时间结构(&st)
// 长整型  时间戳 = 时间戳从系统时间结构(&st)
// 调试输出(时间戳到文本格式化("[%Y-%m-%d %X]", 时间戳)) //结果为: [2025-02-10 11:10:51]
//@返回 时间戳
//@别名 时间戳从系统时间结构(系统时间)
INT64 xcl_systemTimeToTime(__in SYSTEMTIME& systemTime);

//@备注  从文件时间结构转换到时间戳
//@示例  文件时间结构  ft
// 时间戳到文件时间结构(时间戳从系统(NULL), &ft)
// 长整型  时间戳 = 时间戳从文件时间结构(&ft)
// 调试输出(时间戳到文本格式化("[%Y-%m-%d %X]", 时间戳)) //结果为: [2025-02-10 11:10:51]
//@返回 时间戳
//@别名 时间戳从文件时间结构(文件时间)
INT64 xcl_fileTimeToTime(__in FILETIME& fileTime);

//@示例 文件时间结构  ft
// 时间戳到文件时间结构(时间戳从系统(NULL), &ft)
//@别名 时间戳到文件时间结构(输入时间戳, 接收返回结构)
void xcl_timeToFileTime(__in INT64 time_, __out FILETIME& fileTime);

//@示例 系统时间结构  st
// 时间戳到系统时间结构(时间戳从系统(NULL), &st)
//@别名 时间戳到系统时间结构(输入时间戳, 接收返回结构)
void xcl_timeToSystemTime(__in INT64 time_, __out SYSTEMTIME& systemTime);

//@备注  将时间戳转换为 tm 结构, 基于协调世界时（UTC）
//@doc  "doc\time.txt"
//@参数  时间戳, 时间表示为自 1970 年 1 月 1 日午夜 (00:00:00)，协调世界时 (UTC) 以来所经过的秒数
//@参数  接收返回TM结构
//@返回  如果成功，则返回 0。 如果失败，则返回值为错误代码
//@别名  时间戳到TM(输入时间戳, 接收返回TM)
int xcl_timeToTM(__in INT64 time_, __out tm* tm_);

//@备注  将时间戳转换为 tm 结构, 本地时间, 例如 北京时间（UTC+8)
//@doc   "doc\time.txt"  
//@参数  时间戳, 时间表示为自 1970 年 1 月 1 日午夜 (00:00:00)，协调世界时 (UTC) 以来所经过的秒数
//@参数  接收返回TM结构
//@返回  如果成功，则返回 0。 如果失败，则返回值为错误代码
//@别名  时间戳到本地时间TM(输入时间戳, 接收返回TM)
int xcl_timeToLocalTM(__in INT64 time_, __out tm* tm_);

//@备注 格式化时间到文本型, 格式字符串参见MSDN: https://learn.microsoft.com/zh-cn/cpp/c-runtime-library/reference/strftime-wcsftime-strftime-l-wcsftime-l?view=msvc-170
//@示例 调试输出(时间戳到文本格式化("%Y-%m-%d %X", 时间戳从系统(NULL)))
// 结果为: 2025-02-10 11:10:51
//@参数 格式字符串, 可空;若参数为空输出格式为:2020-10-29 12:05:03, 否则根据用户指定的格式字符串输出
// 例如: "%Y-%m-%d %X" -> 年-月-日 小时:分:秒;  
// 例如: "%Y-%m-%d %H:%M:%S" -> 年-月-日 小时:分:秒;
//@参数 时间戳, 时间表示为自 1970 年 1 月 1 日午夜 (00:00:00)，协调世界时 (UTC) 以来所经过的秒数
//@返回 文本型时间
//@别名 时间戳到文本格式化(格式字符串, 时间戳)
CXText xcl_formatTime(__in const wchar_t* pFormat, __in const INT64 time_);

//@分组}

//--------------------------------------------------
//@分组{  控制台操作

//@隐藏{
#ifndef LC_ALL

//@备注  所有类别的地域信息
#define  LC_ALL  0
#endif
//@隐藏}

/*@声明

#define  LC_ALL  0

//@备注 将字符串写入流
//@doc  "doc\fputs.txt"
//@参数 str 输出字符串
//@参数 stream 指向文件指针, 例如控制台: stdout(输出流), stdin(输入流), stderr(错误流), 以及通过 fopen 函数打开的文件流
//@返回 如果成功返回一个非负值。 出现错误-1
//@别名 控制台标准输出()
int fputws(const wchar_t *str,FILE *stream);

//@备注 将字符串写入到指定的文件流中
//@doc  "doc\fputs.txt"
//@参数 str 输出字符串
//@参数 stream 指向文件指针, 例如控制台: stdout(输出流), stdin(输入流), stderr(错误流), 以及通过 fopen 函数打开的文件流
//@返回 如果成功返回一个非负值。 出现错误-1
//@别名 控制台标准输出A()
int fputs(const char *str, FILE *stream);

//@备注  从 stdin 流中获取一行
//@示例  字符型  数组1[100] = {0}
// 字符型 *结果 = 控制台标准输入(数组1, 100)
// 如果 结果
// -- 调试输出(数组1) //输出 控制台键盘输入内容
//@参数  内存块指针
//@参数  内存块大小(字符为单位)
//@返回  如果成功返回内存块指针,失败返回NULL
//@别名  控制台标准输入(内存块指针, 内存块大小)
wchar_t *_getws_s(wchar_t *buffer,size_t sizeInCharacters);

//@备注  从标准输入(stdin)（通常是键盘）读取一行字符串到指定的字符数组中
//@示例	字符型A  数组1[100] = {0}
// 字符型A *结果 = 控制台标准输入A(数组1, 100)
// 如果 结果
// -- 调试输出(数组1) //输出 控制台键盘输入内容
//@参数  内存块指针
//@参数  内存块大小(字符为单位)
//@返回  如果成功返回内存块指针,失败返回NULL
//@别名  控制台标准输入A(内存块指针, 内存块大小)
char *gets_s(char *buffer,size_t sizeInCharacters);

//@备注  格式输出字符串到控制台
//@示例  控制台置中文编码()
// 控制台格式输出("%s, %d\n", "abc", 100) //结果为: "abc100"
// 控制台格式输出("整型 = %d\n", 100)
// 控制台格式输出("整型64 = %I64d\n", 100)
// 控制台格式输出("浮点型 = %7.2f\n", 8.88)
// 控制台格式输出("字符串 = %s\n", "abc")
// 控制台格式输出("十六进制 = %X\n", "0x0123")
// 控制台格式输出("单个字符 = %C\n", 'A')
//@参数 格式字符串
//@参数 无限参数...
//@返回 返回输出的字符数或负值
//@别名 控制台格式输出(格式字符串,参数)
int wprintf_s(const wchar_t *format,...);

//@备注  格式输出字符串到控制台
//@示例  控制台置中文编码()
// 控制台格式输出A(A"%s, %d\n", "abc", 100) //结果为: A"abc100"
// 控制台格式输出A(A"整型 = %d\n", 100)
// 控制台格式输出A(A"整型64 = %I64d\n", 100)
// 控制台格式输出A(A"浮点型 = %7.2f\n", 8.88)
// 控制台格式输出A(A"字符串 = %s\n", A"abc")
// 控制台格式输出A(A"十六进制 = %X\n", 123456)
// 控制台格式输出A(A"单个字符 = %C\n", 'A')
//@参数 格式字符串A
//@参数 无限参数...
//@返回 返回输出的字符数或负值
//@别名 控制台格式输出A(格式字符串,参数)
int printf_s(const char *format,...);

//@备注  用于设置程序的当前地域（locale）信息。地域信息会影响程序中与文化、语言相关的操作，例如日期和时间的格式化、货币符号的显示、字符分类等。
//运行时区域设置 参见MSDN: https://learn.microsoft.com/zh-cn/cpp/c-runtime-library/reference/setlocale-wsetlocale?view=msvc-170
//@示例 控制台置编码(LC_ALL, "zh_CN.UTF-8")  //设置地域为中文（中国），使用 UTF-8 编码
//@参数 受区域设置影响的分类
//@参数 区域设置说明符
//@返回 如果设置成功，返回一个指向以空字符结尾的宽字符串的指针，该字符串表示当前的地域设置, 如果设置失败，返回 NULL
//@别名 控制台置编码(区域分类, 区域设置名称)
wchar_t* _wsetlocale(int category, const wchar_t* locale);
*/

//@备注 设置中文编码环境
//@示例 控制台置中文编码()
//控制台格式输出("炫语言-中文编程IDE %s\n", "www.xcgui.com")
//控制台格式输出A(A"炫彩界面库 %s\n", A"www.xcgui.com")
//控制台暂停()
//@别名 控制台置中文编码()
inline wchar_t* console_setlocale() { return _wsetlocale(LC_ALL, L"chs");
//	return _wsetlocale(LC_ALL, L"zh_CN.UTF-8");
}

//@备注 让控制台暂停,否则程序会继续往下执行
//@别名 控制台暂停()
inline void console_pause() { system("pause"); }

/*@声明
#define  stdout   //@别名  控制台_输出流
#define  stdin    //@别名  控制台_输入流
#define  stderr   //@别名  控制台_错误流
*/

//@备注 运行指定的控制台程序(不会打开控制台窗口), 返回是否运行成功
//@示例 	文本型  标准输出内容存放变量
//文本型  错误输出内容存放变量
//DWORD  返回值存放变量=0
//运行控制台程序("notepad.exe", &标准输出内容存放变量, &错误输出内容存放变量, &返回值存放变量)
//@参数 提供欲运行控制台程序的命令行文本。
//@参数 控制台程序在运行期间输出到标准输出设备的所有内容将被存放到此变量中
//@参数 控制台程序在运行期间输出到标准错误设备的所有内容将被存放到此变量中
//@参数 控制台程序退出时所提供的返回值将被存放到此变量中
//@别名 运行控制台程序(欲运行的命令行, 标准输出内容存放变量, 错误输出内容存放变量, 返回值存放变量)
BOOL xcl_runConsoleProgram(const wchar_t* command, __out CXText* stdOutput = NULL, __out CXText* stdError = NULL, __out DWORD* exitCode=NULL);

//@分组}

//--------------------------------------------------
//@分组{  环境存取

/*@声明

//@备注 获取当前进程的伪句柄
//@返回 返回值是当前进程的伪句柄
//@别名 取当前进程句柄()
HANDLE WINAPI GetCurrentProcess();

//@备注 设置当前进程的环境变量的值。如果指定的环境变量不存在，该函数会创建这个环境变量；如果环境变量已经存在，则会更新其值
//@示例 置环境变量("环境变量名称", "环境变量值")
// 调试输出(取环境变量("环境变量名称"))  //输出设置的环境变量值
//@参数 设置的环境变量的名称。如具有这个名称的一个环境变量尚不存在，则函数会自动创建它
//@参数 为变量设置的新值
//@别名 置环境变量(名称, 值)
//@返回 返回值：TRUE（非零）表示成功，否则返回零
BOOL WINAPI SetEnvironmentVariable(const wchar_t* pName, const wchar_t* pValue);
*/

//@备注 检索当前进程的环境变量的值
//@示例 调试输出(取环境变量("PATH"))
//@参数 环境变量名称
//@参数 环境变量对应的值
//@别名 取环境变量(名称)
CXText xcl_getEnvironmentVariable(const wchar_t* pName);

//@备注 获取系统全部环境变量保存到字典中返回
//@示例 	字典<文本型, 文本型> 字典1
// 如果 取环境变量到字典(字典1)
// -- 列表循环(迭代器 迭代器项; 字典1)
// -- -- 调试输出(迭代器项.键, 迭代器项.值)
//@参数 字典<名称, 值>, 接收环境变量
//@别名 取环境变量到字典(接收环境变量)
BOOL xcl_getEnvironmentStringsToMap(__out CXMap<CXText, CXText>& map_out);

//@备注 获取当前进程的命令行字符串
//@示例 动态数组<文本型> 结果数组 = 取命令行到列表()
// 列表循环(整型 i = 0; 结果数组)
// -- 调试输出(结果数组[i])
//@返回 返回命令行字符串列表
//@别名 取命令行到列表()
CXVector<CXText> xcl_getCommandLine();

//@备注 取当前程序exe或DLL目录
//@示例 调试输出(取运行目录())
//@别名 取运行目录()
CXText xcl_getRunDir();

//@备注 取程序当前目录,此目录不是固定的路径,可能被修改 GetCurrentDirectory()
//@示例 调试输出(取当前目录())
//@别名 取当前目录()
CXText xcl_getCurrentDir();

//@备注 获取当前程序exe文件名称, 不包含路径
//@示例 调试输出(取执行文件名())
//@别名 取执行文件名()
CXText xcl_getRunFileName();

//@备注 获取当前程序exe文件全路径名称, 包含路径
//@示例 调试输出(取执行文件全路径())
//@别名 取执行文件全路径()
CXText xcl_getRunFileFullPath();

//@备注 如果当前程序是64位返回真,否则返回假
//@返回 如果为64为返回真, 否则返回假
//@别名 是否为64位()
BOOL xcl_is64();


//@备注  读取配置文件中指定项目的文本内容。
//@示例  文本型  结果 = 读配置项("D:\\test\\abc.ini", "Section1", "Key1", L"默认值");
//@参数  --pFileName  指定配置文件的名称，通常以.ini作为文件名后缀。
//@参数  --pNodeName  配置项所处节的名称。
//@参数  --pItemName  配置项在其节中的项名称。
//@参数  --pDefaultText  可空,如果指定配置项不存在，将返回此默认文本。如果指定配置项不存在且本参数被省略，将返回空文本。
//@别名  读配置项(配置文件名, 节名称, 配置项名称, 默认文本)
CXText  xcl_readConfigItem(const wchar_t* pFileName, const wchar_t* pNodeName, const wchar_t* pItemName, const wchar_t* pDefaultText = L"");

//@备注  将文本内容写入配置项中或者删除配置项或节，如果指定配置文件不存在，将会自动创建。成功返回真，失败返回假。
//@示例  写配置项("D:\\test\\abc.ini", "Section1", "name1", "123456");
//@参数  --pFileName  配置文件的名称，通常以.ini作为文件名后缀。
//@参数  --pNodeName  写入配置项所处节的名称。
//@参数  --pItemName  可空, 参数值指定写入配置项在其节中的名称。如果参数值被省略，则删除指定节及其下的所有配置项。
//@参数  --pValue  可空, 参数值指定写入到指定配置项中的文本。如果参数值被省略，则删除所指定配置项。
//@别名  写配置项(配置文件名, 节名称, 配置项名称)
BOOL xcl_writeConfigItem(const wchar_t* pFileName, const wchar_t* pNodeName, const wchar_t* pItemName = NULL, const wchar_t* pValue = NULL);

//@备注  删除配置文件中指定项
//@示例  删配置项("D:\\test\\abc.ini", "Section1", "Key1");
//@参数  --pFileName  指定配置文件的名称，通常以.ini作为文件名后缀。
//@参数  --pNodeName  配置项所处节的名称。
//@参数  --pItemName  配置项在其节中的项名称。
//@别名  删配置项(配置文件名, 节名称, 配置项名称)
BOOL xcl_deleteConfigItem(const wchar_t* pFileName, const wchar_t* pNodeName, const wchar_t* pItemName);

//@备注  获取配置文件中所有节名的文本数组。
//@示例  动态数组<文本型> 结果数组 = 取配置节名("D:\\test\\abc.ini");
//@参数  --pFileName  配置文件的名称，通常以.ini作为文件名后缀。
//@别名  取配置节名(配置文件名)
CXVector<CXText> xcl_getConfigSectionNames(const wchar_t* pFileName);

//@备注  删除配置文件中指定节点。
//@示例  删配置节名("D:\\test\\abc.ini", "Section1");
//@参数  --pFileName  配置文件的名称，通常以.ini作为文件名后缀。
//@参数  --pSectionName  要删除的节点名称
//@别名  删配置节名(配置文件名, 节点名)
BOOL xcl_deleteConfigSection(const wchar_t* pFileName, const wchar_t* pSectionName);

//@分组}  //环境存取


//@分组{ 颜色操作

#define COLORREF  DWORD   //@别名 颜色

//@备注 RGB颜色交换R和B的通道, A和G的通道不变, 实现 ABGR转ARGB, ARGB转ABGR功能;
// A(透明度), R(红色), G(绿色), B(蓝色)
// ABGR ->  ARGB;  A | R<<16| G| B>>16;
// ARGB ->  ABGR;  A | B<<16| G| R>>16;
//@示例  颜色  颜色1 = RGBA(255,0,0,255)  //红色255, 透明通道255
// 颜色  颜色2= RGB2RB(颜色1)  //对A和G通道对换
//@参数 颜色值
//@返回 返回交换结果
inline COLORREF RGB2RB(COLORREF color)
{
	return  (color & 0xFF000000) | ((color & 0xFF) << 16) | (color & 0xFF00) | (color >> 16 & 0xFF);
}

/*@声明
//@备注  返回一个整数，用来表示一个红(R)绿(G)蓝(B)颜色值, 不包含A通道
//A(透明度), R(红色), G(绿色), B(蓝色)
//@示例  颜色  颜色RGB = 取颜色值RGB(255,0,0) //红色255, 透明通道255
//@参数  数值范围从 0 到 255，表示颜色中的红色成份。
//@参数  数值范围从 0 到 255，表示颜色中的绿色成份。
//@参数  数值范围从 0 到 255，表示颜色中的蓝色成份。
//@别名  取颜色值RGB(红色, 绿色, 蓝色)
int RGB(int R, int G, int B);
*/

#ifndef    RGBA
//@备注 取参数(r,g,b,a)的组合颜色为ABGR值, GDI/GDI+所使用颜色的值, 内存中分布为:A|B|G|R, 与系统宏RGB()内存分布一样,只是多了A通道,RGB(r,g,b) = B|G|R;
//A(透明度), R(红色), G(绿色), B(蓝色)
//@示例 颜色  颜色RGBA = 取颜色值RGBA(255,0,0,255) //红色255, 透明通道255
//@别名 取颜色值RGBA(红色, 绿色, 蓝色, 透明度)
#define    RGBA(r,g,b,a)    ((BYTE)a<<24 | (BYTE)r | (BYTE)g<<8 | (BYTE)b<<16)
#endif


#ifndef  GetRValue

//@备注  取出rgb或rgba颜色的R值, 红色
//@示例  颜色  颜色RGBA = 取颜色值RGBA(255,0,0,255) //红色255, 透明通道255
//  字节型  R = 取颜色值R(颜色RGBA) //结果为: 255
//@别名 取颜色值R()
#define GetRValue(rgb)      (LOBYTE(rgb))

//@备注  取出rgb或rgba颜色的G值, 绿色
//@示例  颜色  颜色RGBA = 取颜色值RGBA(0,255,0,255) //绿色255, 透明通道255
//  字节型  G = 取颜色值G(颜色RGBA) //结果为: 255
//@别名 取颜色值G()
#define GetGValue(rgb)      (LOBYTE(((WORD)(rgb)) >> 8))

//@备注  取出rgb或rgba颜色的B值, 蓝色
//@示例  颜色  颜色RGBA = 取颜色值RGBA(0,0,255,255) //蓝色255, 透明通道255
//  字节型  B = 取颜色值B(颜色RGBA) //结果为: 255
//@别名 取颜色值B()
#define GetBValue(rgb)      (LOBYTE((rgb)>>16))
#endif

#ifndef GetAValue

//@备注  取出rgba颜色的A值, 透明度
//@示例  颜色  颜色RGBA = 取颜色值RGBA(0,0,0,255) //黑色, 透明通道255
//  字节型  A = 取颜色值A(颜色RGBA) //结果为: 255 
//@别名 取颜色值A()
#define GetAValue(rgba)   (LOBYTE((rgba)>>24))
#endif

//@分组} //颜色操作


//@分组{  特殊功能
/*@声明

//@备注  输出调试信息到IDE[调试输出]视图, 仅编译调试版有效(DEBUG); 自动分割参数: 参数1, 参数2, 参数3
//@示例  调试输出("abc", 123)  //结果为: abc, 123
//@别名  调试输出()
void xcl_log(...);

//@备注  输出调试信息到IDE[调试输出]视图, 仅编译调试版有效(DEBUG); 不分割参数
//@示例  调试输出("abc", 123)  //结果为:  abc123
//@别名  调试输出2()
void xcl_log2(...);

//@备注  输出调试信息到IDE[调试输出]视图, 支持格式化, 仅编译调试版有效(DEBUG)
//@示例  调试输出格式化("str = %s, x = %d, f=%f", "abc", 10, 5.6f) 
// //结果为: "str = abc, x = 10, f=5.600000"
//@别名  调试输出格式化(格式字符串)
void xcl_xtracew(const wchar_t* pFormat, ...);

//@备注  输出调试信息到IDE[调试输出]视图, 支持格式化, 仅编译调试版有效(DEBUG)
//@示例  调试输出格式化A(A"str = %s, x = %d, f=%f", A"abc", 10, 5.6f) 
// //结果为: A"str = abc, x = 10, f=5.600000"
//@别名  调试输出格式化A(格式字符串)
void xcl_xtrace(const char* pFormat, ...);
*/

//@备注  输出到[调试输出]视图
//@示例  调试输出十六进制(A"123456", 6)  //结果为: 31 32 33 34 35 36
// 整型  var = 123456
// 调试输出十六进制(&var, 4) //结果为: 40 E2 01 00
//@参数  数据地址
//@参数  数据长度, 字节为单位
//@参数  是否大写
//@参数  是否空格分割
//@别名  调试输出十六进制(数据地址, 数据长度, 是否大写, 是否分割)
template<class T>
void xcl_logDataHex(__in const T* data, int length, BOOL bUpper = TRUE, BOOL bSplit = TRUE)
{
#ifdef DEBUG
	const BYTE* _data = (const BYTE*)data;
	wchar_t  hexs_upper[17] = L"0123456789ABCDEF";
	wchar_t  hexs[17] = L"0123456789abcdef";
	std::wstring  text;
	BYTE index = 0;
	for (int i = 0; i < length; i++)
	{
		if (0 != i && bSplit)
			text += ' ';

		BYTE index = (_data[i] >> 4) & 0xf;
		if (bUpper)
			text += hexs_upper[index];
		else
			text += hexs[index];

		index = _data[i] & 0xf;
		if (bUpper)
			text += hexs_upper[index];
		else
			text += hexs[index];
	}
	text += '\n';
	OutputDebugString(text.c_str());
#endif
}

//@备注  输出到[调试输出]视图
//@示例  调试输出十进制(A"123456", 6) //结果为: 49 50 51 52 53 54
// 整型  var = 123456
// 调试输出十进制(&var, 4)  //结果为: 64 226 1 0
//@参数  数据地址
//@参数  数据长度, 字节为单位
//@参数  是否空格分割
//@别名  调试输出十进制(数据地址, 数据长度, 是否分割)
template<class T>
void xcl_logDataDec(__in const T* data, int length, BOOL bSplit = TRUE)
{
#ifdef DEBUG
	const BYTE* _data = (const BYTE*)data;
	wchar_t  buf[4] = { 0 };
	std::wstring  text;
	for (int i = 0; i < length; i++)
	{
		if (0 != i && bSplit)
			text += ' ';
		_itow_s(_data[i], buf, 10);
		text += buf;
	}
	text += '\n';
	OutputDebugString(text.c_str());
#endif
}

//@备注  输出到[调试输出]视图
//@示例  调试输出二进制(A"123456", 6) //结果为: 110001 110010 110011 110100 110101 110110
// 整型  var = 123456
// 调试输出二进制(&var, 4) //结果为: 1000000 11100010 1 0
//@参数  数据地址
//@参数  数据长度, 字节为单位
//@参数  是否空格分割
//@别名  调试输出二进制(数据地址, 数据长度, 是否分割)
template<class T>
void xcl_logDataBin(__in const T* data, int length, BOOL bSplit = TRUE)
{
#ifdef DEBUG
	const BYTE* _data = (const BYTE*)data;
	wchar_t  buf[9] = { 0 };
	std::wstring  text;
	for (int i = 0; i < length; i++)
	{
		if (0 != i && bSplit)
			text += ' ';
		_itow_s(_data[i], buf, 2);
		text += buf;
	}
	text += '\n';
	OutputDebugString(text.c_str());
#endif
}

//@备注 弹出消息框
//@示例 消息框("消息内容", "标题")
//@参数 内容,消息框的内容。
//@参数 标题,消息框的标题
//@别名 消息框(内容, 标题)
void xcl_msgBox(const wchar_t* pText, const wchar_t* pCaption = NULL);

/*@声明
//@备注 弹出系统消息框
//@示例 系统消息框(NULL, "消息内容", "标题", MB_OK)
//@参数 窗口句柄,此参数代表消息框拥有的窗口。如果为NULL，则消息框没有拥有窗口。
//@参数 内容,消息框的内容。
//@参数 标题,消息框的标题
//@参数 标志值, 例如: MB_OK, 详见MSDN
//@返回 返回值为:IDOK(1)等详见MSDN
//@别名 系统消息框(父窗口句柄, 内容, 标题, 类型)
int WINAPI MessageBox(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);

//@备注  结束当前程序运行
//@示例  结束(0) //退出程序
//@参数  退出码,可填0
//@别名  结束(退出码)
void exit(int exitCode);
*/

/*@声明
//@备注  打开或执行程序, MSDN: https://learn.microsoft.com/zh-cn/windows/win32/api/shellapi/nf-shellapi-shellexecutea
//@示例  HWND  hWnd = NULL
// 执行(hWnd, "open", "calc.exe", "", "", SW_SHOW ) //打开计算器
// 执行(hWnd, "open", "notpad.exe", "", "", SW_SHOW ) //打开记事本
// 执行(hWnd, "open", "c:\\abc.txt", "", "", SW_SHOW ) //打开文档
// 执行(hWnd, "open", "http://www.xcgui.com", NULL, NULL, SW_SHOW) //打开网站
// 执行(hWnd, "open", "mailto:123@qq.com", "", "", SW_SHOW ) //发送EMAIL
// 执行(hWnd, "print", "c:\\abc.txt", "", "", SW_SHOW ) //打印文档
// 执行(hWnd, "find", "c:\\abc.txt", "", "", SW_SHOW ) //查找文件
//@参数  --hWnd 用于显示 UI 或错误消息的父窗口的句柄。 如果操作未与窗口关联，则此值可以为 NULL 
//@参数  --lpOperation  操作类型
// edit: 启动编辑器并打开文档进行编辑。 如果 lpFile 不是文档文件，则函数将失败。
// explore: 浏览 lpFile 指定的文件夹
// find: 启动从 lpDirectory 指定的目录开始的搜索。
// open: 打开 lpFile 参数指定的项。 该项可以是文件或文件夹。
// print: 打印 lpFile 指定的文件。 如果 lpFile 不是文档文件，则函数将失败。
// runas: 以管理员身份启动应用程序。 用户帐户控制 (UAC) 将提示用户同意运行提升的应用程序，或者输入用于运行应用程序的管理员帐户的凭据。
//@参数  --lpFile 要执行的文件
//@参数  --lpParameters 该字符串指定要传递给应用程序的参数
//@参数  --lpDirectory 默认目录, 如果在 lpFile 中提供了相对路径，请不要对 lpDirectory 使用相对路径
//@参数  --nShowCmd 指定应用程序在打开时如何显示应用程序的标志, 它可以是在 ShowWindow 函数的 nCmdShow 参数中指定的任何值
//@别名  执行(窗口句柄HWND, 操作类型, 文件名, 要传递给应用程序的参数, 默认目录, 显示方式)
//@返回 如果函数成功，则返回大于 32 的值。 如果函数失败，它将返回一个错误值，该值指示失败的原因
HINSTANCE ShellExecute(HWND hWnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, __in INT nShowCmd);

//@备注  根据所提供参数的值，返回两部分中的其中一个。对应C++的三目运算: int ret = 1<2 ? 1 : 2
//@示例  整型  结果 = 选择(1 < 2, 1, 2)  //结果为:1
//@参数  如果该值为真，将返回第一个待选择项，否则将返回第二个待选择项。
//@参数  如果给定逻辑值为真，命令将返回本选择项。
//@参数  如果给定逻辑值为假，命令将返回本选择项。
template<class T>
T 选择(BOOL 用作选择的逻辑值, T 待选择项一,T 待选择项二);

//@备注 从第二个参数开始, 根据第一个参数索引, 选择其中一个参数作为返回值; 
//@示例 整型  返回值 = 多项选择(2, 0, 1, 2, 3, 4)  //结果为: 2
// 文本型  返回值 = 多项选择(2, "AAA", "BBB", "CCC", "DDD") //结果为: "CCC"
//@参数 索引, 索引所对应的参数作为返回值, 从第二个参数开始计算
//@返回 返回类型,根据传入的第二个参数类型决定, 第二个参数传什么类型就返回什么类型
//@别名 多项选择(索引, 任意类型参数列表)
template<typename T>
T xcl_multipleItemChoice(int index, ...);
*/

//@隐藏{
#define 选择(用作选择的逻辑值, 待选择项一, 待选择项二) (用作选择的逻辑值 ? 待选择项一 : 待选择项二)
//@隐藏}


//@隐藏{
template<typename T>
struct xcl_multipleItemChoice_
{
	T      _value;
	int    _index;
	int    _index_sel;
};

template<typename T>
BOOL xcl_multipleItemChoiceT(xcl_multipleItemChoice_<T>* st, const T value_)
{
	if (st->_index == st->_index_sel)
	{
		st->_value = value_;
		return TRUE;
	}
	st->_index++;
	return FALSE;
}

template<typename T, typename... Args>
void WINAPI xcl_multipleItemChoiceT(xcl_multipleItemChoice_<T>* st, const T t, const Args ... args)
{
	if (xcl_multipleItemChoiceT(st, t))
		return;
	xcl_multipleItemChoiceT(st, args...);
}

template<typename T, typename ... Args>
T WINAPI xcl_multipleItemChoice(int index, T args1, const Args ... args)
{
	xcl_multipleItemChoice_<T>  st;
	st._index_sel = index;
	st._value = 0;
	st._index = 0;
	if (xcl_multipleItemChoiceT(&st, args1))
		return st._value;
	xcl_multipleItemChoiceT(&st, args...);
	return st._value;
}
//@隐藏}

//@备注  处理消息队列中消息, 直到处理完全部消息才返回;
// 暂时转让控制权，以便让 Windows 操作系统有机会处理其它的如用户键盘或鼠标输入等事件。直到操作系统处理并发送完程序队列中的所有事件后，命令才会返回
//@别名  处理事件()
void xcl_doMessage();

/*@声明
//@备注  睡眠当前线程, 持续时间毫秒为单位
//@示例  延时(1000)
//@别名  延时(持续时间)
void WINAPI Sleep(DWORD dwMilliseconds);
*/

//@备注  延迟一段时间，然后继续执行后面的程序代码。与 "延时()" 命令不同的是，本命令在等待期间允许用户进行其它操作（如点击按钮等）
//@参数  单位为毫秒。提示：1000毫秒等于1秒。
//@别名  延迟(等待时间)
void xcl_delay(UINT delay);

//@备注 获取屏幕宽度, 不包含任务栏
//@别名 取屏幕宽度()
int xcl_getScreenWidth();

//@备注 获取屏幕高度, 不包含任务栏
//@别名 取屏幕高度()
int xcl_getScreenHeight();

//@备注  未来将废弃, 获取变量的地址, 将地址转换为: "空类型*"
//@示例  调试输出(取变量地址(变量名))
#define  取变量地址(var)   ((void*)&var)

//@备注  未来将废弃, 获取变量的地址, 将地址转换为: "变整型", 支持64位指针地址
//@示例  变整型 地址 = 取变量地址到变整型(变量名)
#define  取变量地址到变整型(var)   ((vint)&var)

//MessageBox() Flags
#ifndef MB_OK
	#define MB_OK                       0x00000000L
	#define MB_OKCANCEL                 0x00000001L
	#define MB_ABORTRETRYIGNORE         0x00000002L
	#define MB_YESNOCANCEL              0x00000003L
	#define MB_YESNO                    0x00000004L
	#define MB_RETRYCANCEL              0x00000005L
	#define MB_CANCELTRYCONTINUE        0x00000006L

	#define MB_ICONHAND                 0x00000010L
	#define MB_ICONQUESTION             0x00000020L
	#define MB_ICONEXCLAMATION          0x00000030L
	#define MB_ICONASTERISK             0x00000040L
	#define MB_USERICON                 0x00000080L
	#define MB_ICONWARNING              MB_ICONEXCLAMATION
	#define MB_ICONERROR                MB_ICONHAND

	#define MB_ICONINFORMATION          MB_ICONASTERISK
	#define MB_ICONSTOP                 MB_ICONHAND

	#define MB_DEFBUTTON1               0x00000000L
	#define MB_DEFBUTTON2               0x00000100L
	#define MB_DEFBUTTON3               0x00000200L
	#define MB_DEFBUTTON4               0x00000300L

	#define MB_APPLMODAL                0x00000000L
	#define MB_SYSTEMMODAL              0x00001000L
	#define MB_TASKMODAL                0x00002000L
	#define MB_HELP                     0x00004000L // Help Button

	#define MB_NOFOCUS                  0x00008000L
	#define MB_SETFOREGROUND            0x00010000L
	#define MB_DEFAULT_DESKTOP_ONLY     0x00020000L

	#define MB_TOPMOST                  0x00040000L
	#define MB_RIGHT                    0x00080000L
	#define MB_RTLREADING               0x00100000L

	#define MB_TYPEMASK                 0x0000000FL
	#define MB_ICONMASK                 0x000000F0L
	#define MB_DEFMASK                  0x00000F00L
	#define MB_MODEMASK                 0x00003000L
	#define MB_MISCMASK                 0x0000C000L
#endif

//@分组}  //特殊功能


//@分组{ 剪贴板

/*@声明

//@备注  打开剪贴板进行检查，并阻止其他应用程序修改剪贴板内容, 不使用后需要关闭剪贴板
//如果另一个窗口打开剪贴板，OpenClipboard 将失败。
//应用程序应在每次成功调用 OpenClipboard 后调用 CloseClipboard 函数。
//除非调用 EmptyClipboard 函数，否则由 hWndNewOwner 参数标识的窗口不会成为剪贴板所有者。
//如果应用程序调用将 hwnd 设置为 NULL 的 OpenClipboard，EmptyClipboard 会将剪贴板所有者设置为 NULL;这会导致 SetClipboardData 失败。
// MSDN: https://learn.microsoft.com/zh-cn/windows/win32/dataxchg/using-the-clipboard
//@返回  如果该函数成功，则返回值为非零值
//@别名  剪贴板打开(窗口句柄HWND)
BOOL WINAPI OpenClipboard(__in_opt HWND hWndNewOwner);

//@返回  如果该函数成功，则返回值为非零值。如果函数失败，则返回值为零
//@别名  剪贴板关闭()
BOOL WINAPI CloseClipboard();

//@备注  清空剪贴板并释放剪贴板中的数据句柄。 然后，该函数将剪贴板的所有权分配给当前打开剪贴板的窗口
//在调用 EmptyClipboard 之前，应用程序必须使用 OpenClipboard 函数打开剪贴板。 如果应用程序在打开剪贴板时指定 NULL 窗口句柄， 则 EmptyClipboard 成功，但将剪贴板所有者设置为 NULL。 请注意，这会导致 SetClipboardData 失败。
//@返回  如果该函数成功，则返回值为非零值。
//@别名  剪贴板清空()
BOOL WINAPI EmptyClipboard();

//@备注  以指定的剪贴板格式将数据放在剪贴板上。 窗口必须是当前剪贴板所有者，并且应用程序必须已调用 OpenClipboard 函数。 (响应WM_RENDERFORMAT消息时，剪贴板所有者在调用 SetClipboardData.) 之前不得调用 OpenClipboard
//@参数  剪贴板格式。 此参数可以是已注册的格式或任何标准剪贴板格式
//@参数  指定格式的数据句柄。 此参数可以为 NULL，指示窗口提供指定剪贴板格式中的数据， (在请求时呈现格式) ;这称为 延迟呈现。 如果窗口延迟呈现，则必须处理 WM_RENDERFORMAT 并 WM_RENDERALLFORMATS 消息。
//      如果 SetClipboardData 成功，则系统拥有 由 hMem 参数标识的对象。 一旦所有权传输到系统，应用程序可能不会写入或释放数据，但可以在调用 CloseClipboard 函数之前锁定和读取数据。 (必须在剪贴板关闭之前解锁内存。) 如果 hMem 参数标识内存对象，则必须使用具有 GMEM_MOVEABLE 标志的函数分配该对象。
//@别名  剪贴板置数据(剪贴板格式, 数据句柄)
HANDLE WINAPI SetClipboardData(__in UINT uFormat, __in_opt HANDLE hMem);

//@备注  以指定格式从剪贴板检索数据。剪贴板之前必须已打开
//@返回  如果函数成功，则返回值是指定格式的剪贴板对象的句柄,如果函数失败，则返回值为 NULL
//@别名  剪贴板取数据(剪贴板格式)
HANDLE WINAPI GetClipboardData(__in UINT uFormat);

//@备注  注册新的剪贴板格式。 然后，此格式可用作有效的剪贴板格式
//@返回  如果函数成功，则返回值标识已注册的剪贴板格式,如果函数失败，则返回值为零
//@别名  剪贴板注册格式(剪贴板格式)
UINT WINAPI RegisterClipboardFormat(__in LPCWSTR lpszFormat);

#define CF_TEXT             1   //@别名 剪贴板格式_文本
#define CF_BITMAP           2	//@别名 剪贴板格式_位图
#define CF_METAFILEPICT     3	//@别名 剪贴板格式_图元文件
#define CF_SYLK             4	//@别名 剪贴板格式_符号链接
#define CF_DIF              5	//@别名 剪贴板格式_DIF
#define CF_TIFF             6	//@别名 剪贴板格式_标记图像文件
#define CF_OEMTEXT          7	//@别名 剪贴板格式_OEM文本
#define CF_DIB              8	//@别名 剪贴板格式_DIB位图
#define CF_PALETTE          9	//@别名 剪贴板格式_调色板
#define CF_PENDATA          10	//@别名 剪贴板格式_笔数据
#define CF_RIFF             11	//@别名 剪贴板格式_RIFF
#define CF_WAVE             12	//@别名 剪贴板格式_WAVE
#define CF_UNICODETEXT      13	//@别名 剪贴板格式_文本W
#define CF_ENHMETAFILE      14	//@别名 剪贴板格式_增强型图元文件句柄
#define CF_HDROP            15  //@别名 剪贴板格式_HDROP
#define CF_LOCALE           16  //@别名 剪贴板格式_LOCALE

*/

//@备注  设置剪贴板文本
//@示例  HWND  hWnd = NULL;  //指定剪贴板所有者
// 剪贴板置文本(hWnd, "123456", 文本取长度("123456"))
//@参数  表示要成为剪贴板新所有者的窗口的句柄。如果此参数为 NULL，则打开的剪贴板没有所有者
//@参数  字符串指针
//@参数  字符串长度(字符为单位)
//@返回  如果成功返回真, 否则返回假
//@别名  剪贴板置文本(窗口句柄HWND,  字符串, 字符串长度)
BOOL xcl_setClipboardText(HWND hWnd, const wchar_t* pText, int wcharLen);

//@备注  从剪贴板中获取文本
//@示例  HWND  hWnd = NULL;  //指定剪贴板所有者
// 文本型  文本数据 = 剪贴板取文本(hWnd)
//@参数  表示要成为剪贴板新所有者的窗口的句柄。如果此参数为 NULL，则打开的剪贴板没有所有者
//@别名  剪贴板取文本(窗口句柄HWND)
CXText xcl_getClipboardText(HWND hWnd);

//@分组}

//@分组{ RC资源操作

//@备注  从RC资源中查找指定文件资源, 返回文件内存地址和大小
//@示例  正整型  数据大小 = 0
// 空类型  数据地址 = RC资源查找文件("IDI_PNG1", &数据大小, NULL, "PNG")
// HIMAGE hImage = 图片_加载从内存(数据地址, 数据大小)
//@参数  资源名称(字符串), 如果资源名称为数字, 请使用"RC资源查找文件从ID()"
//@参数  返回资源大小(字节为单位)
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@参数  资源类型, 在RC资源中指定的类型名称, 一般普通文件"RT_RCDATA"
//@返回  文件内存地址
//@别名  RC资源查找文件(资源名称, 返回资源大小, 模块句柄, 资源类型)
void* rc_findFile(const wchar_t* pName, __out UINT* pOutSize = NULL, HMODULE hModule = NULL, const wchar_t* pResType = L"RT_RCDATA");

//@备注  从RC资源中查找指定文件资源, 返回文件内存地址和大小
//@示例  正整型  数据大小 = 0
// 空类型  数据地址 = RC资源查找文件从ID(101, &数据大小, NULL, "PNG")
// HIMAGE hImage = 图片_加载从内存(数据地址, 数据大小)
//@参数  资源ID(数字型), 如果资源名称为字符串, 请使用"RC资源查找文件()"
//@参数  返回资源大小(字节为单位)
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@参数  资源类型, 在RC资源中指定的类型名称, 一般普通文件"RT_RCDATA"
//@返回  文件内存地址
//@别名  RC资源查找文件从ID(资源ID, 返回资源大小, 模块句柄, 资源类型)
void* rc_findFileByID(int id, __out UINT* pOutSize = NULL, HMODULE hModule = NULL, const wchar_t* pResType = L"RT_RCDATA");

//@备注  从RC资源中查找指定文件资源
//@示例  字节集  数据 = RC资源查找文件到字节集("IDI_PNG1",  NULL, "PNG")
// HIMAGE  hImage = 图片_加载从内存(数据.取地址(), 数据.取大小())
//@参数  资源名称(字符串), 如果资源名称为数字, 请使用"RC资源查找文件到字节集从ID"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@参数  资源类型, 在RC资源中指定的类型名称, 一般普通文件"RT_RCDATA"
//@返回  获取的内容存储到字节集中返回
//@别名  RC资源查找文件到字节集(资源名称, 模块句柄, 资源类型)
CXBytes rc_findFileToBytes(const wchar_t* pName, HMODULE hModule = NULL, const wchar_t* pResType = L"RT_RCDATA");

//@备注  从RC资源中查找指定文件资源
//@示例  字节集  数据 = RC资源查找文件到字节集从ID(101,  NULL, "PNG")
// HIMAGE  hImage = 图片_加载从内存(数据.取地址(), 数据.取大小())
//@参数  资源ID(数字型), 如果资源名称为字符串, 请使用"RC资源查找文件到字节集()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@参数  资源类型, 在RC资源中指定的类型名称, 一般普通文件"RT_RCDATA"
//@返回  获取的内容存储到字节集中返回
//@别名  RC资源查找文件到字节集从ID(资源ID, 模块句柄, 资源类型)
CXBytes rc_findFileToBytesByID(int id, HMODULE hModule = NULL, const wchar_t* pResType = L"RT_RCDATA");

//@备注  从RC资源中查找指定文件资源
//@示例  文本型  文本数据 = RC资源查找文件到文本("file1")
//@参数  资源名称(字符串), 如果资源名称为数字, 请使用"RC资源查找文件到文本从ID()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@参数  资源类型, 在RC资源中指定的类型名称, 一般普通文件"RT_RCDATA"
//@返回  获取的内容存储到文本型中返回
//@别名  RC资源查找文件到文本(资源名称, 模块句柄, 资源类型)
CXText  rc_findFileToText(const wchar_t* pName, HMODULE hModule = NULL, const wchar_t* pResType = L"RT_RCDATA");

//@备注  从RC资源中查找指定文件资源
//@示例  文本型  文本数据 = RC资源查找文件到文本从ID(101)
//@参数  资源ID(数字型), 如果资源名称为字符串, 请使用"RC资源查找文件到文本()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@参数  资源类型, 在RC资源中指定的类型名称, 一般普通文件"RT_RCDATA"
//@返回  获取的内容存储到文本型中返回
//@别名  RC资源查找文件到文本从ID(资源ID, 模块句柄, 资源类型)
CXText  rc_findFileToTextByID(int id, HMODULE hModule = NULL, const wchar_t* pResType = L"RT_RCDATA");

//@备注  从RC资源中查找指定文件资源
//@示例  文本型A  文本数据 = RC资源查找文件到文本A("file1")
//@参数  资源名称(字符串), 如果资源名称为数字, 请使用"RC资源查找文件到文本A从ID()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@参数  资源类型, 在RC资源中指定的类型名称, 一般普通文件"RT_RCDATA"
//@返回  获取的内容存储到文本型中返回
//@别名  RC资源查找文件到文本A(资源名称, 模块句柄, 资源类型)
CXTextA rc_findFileToTextA(const wchar_t* pName, HMODULE hModule = NULL, const wchar_t* pResType = L"RT_RCDATA");

//@备注  从RC资源中查找指定文件资源
//@示例  文本型A  文本数据 = RC资源查找文件到文本A从ID(101)
//@参数  资源ID(数字型), 如果资源名称为字符串, 请使用"RC资源查找文件到文本A()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@参数  资源类型, 在RC资源中指定的类型名称, 一般普通文件"RT_RCDATA"
//@返回  获取的内容存储到文本型中返回
//@别名  RC资源查找文件到文本A从ID(资源ID, 模块句柄, 资源类型)
CXTextA rc_findFileToTextAByID(int id, HMODULE hModule = NULL, const wchar_t* pResType = L"RT_RCDATA");

//@备注  从RC资源中查找指定图标资源
//@示例  HICON  hIcon1 = RC资源查找图标("IDC_ID1")
//@参数  资源名称(字符串), 如果资源名称为数字, 请使用"RC资源查找图标从ID()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的资源图标句柄
//@别名  RC资源查找图标(资源名称, 模块句柄)
HICON  rc_findIcon(const wchar_t* pName, HMODULE hModule = NULL);

//@备注  从RC资源中查找指定图标资源
//@示例  HICON  hIcon1 = RC资源查找图标从ID(101)
//@参数  资源ID(数字型), 如果资源名称为字符串, 请使用"RC资源查找图标()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的资源图标句柄
//@别名  RC资源查找图标从ID(资源ID, 模块句柄)
HICON  rc_findIconByID(int id, HMODULE hModule = NULL);

//@备注  从RC资源中查找指定图标资源, 并指定匹配图标的宽度和高度
//@示例  HICON  hIcon1 = RC资源查找图标2("IDC_ID1", 32, 32)
//@参数  资源名称(字符串), 如果资源名称为数字, 请使用"RC资源查找图标2从ID()"
//@参数  指定匹配图标的宽度, 如果为 0，则使用图像的原始宽度
//@参数  指定匹配图标的高度, 如果为 0，则使用图像的原始高度
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的资源图标句柄
//@别名  RC资源查找图标2(资源名称, 宽度, 高度, 模块句柄)
HICON  rc_findIcon2(const wchar_t* pName, int cx, int cy, HMODULE hModule = NULL);

//@备注  从RC资源中查找指定图标资源, 并指定匹配图标的宽度和高度
//@示例  HICON  hIcon1 = RC资源查找图标2从ID(101, 32, 32)
//@参数  资源ID(数字型), 如果资源名称为字符串, 请使用"RC资源查找图标2()"
//@参数  指定匹配图标的宽度, 如果为 0，则使用图像的原始宽度
//@参数  指定匹配图标的高度, 如果为 0，则使用图像的原始高度
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的资源图标句柄
//@别名  RC资源查找图标2从ID(资源ID, 宽度, 高度, 模块句柄)
HICON  rc_findIcon2ByID(int id, int cx, int cy, HMODULE hModule = NULL);

//@备注  从RC资源中查找指定光标资源
//@示例  HCURSOR hCursor = RC资源查找光标("IDC_ID1")
//@参数  资源名称(字符串), 如果资源名称为数字, 请使用"RC资源查找光标从ID()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的资源光标句柄
//@别名  RC资源查找光标(资源名称, 模块句柄)
HCURSOR rc_findCursor(const wchar_t* pName, HMODULE hModule = NULL);

//@备注  从RC资源中查找指定光标资源
//@示例  HCURSOR hCursor = RC资源查找光标从ID(101)
//@参数  资源ID(数字型), 如果资源名称为字符串, 请使用"RC资源查找光标()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的资源光标句柄
//@别名  RC资源查找光标从ID(资源ID, 模块句柄)
HCURSOR rc_findCursorByID(int id, HMODULE hModule = NULL);

//@备注  从RC资源中查找指定位图资源
//@示例  HBITMAP hBitmap = RC资源查找位图("IDC_ID1")
//@参数  资源名称(字符串), 如果资源名称为数字, 请使用"RC资源查找位图从ID()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的资源位图句柄
//@别名  RC资源查找位图(资源名称, 模块句柄)
HBITMAP rc_findBitmap(const wchar_t* pName, HMODULE hModule = NULL);

//@备注  从RC资源中查找指定位图资源
//@示例  HBITMAP hBitmap = RC资源查找位图从ID(101)
//@参数  资源ID(数字型), 如果资源名称为字符串, 请使用"RC资源查找位图()"
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的资源位图句柄
//@别名  RC资源查找位图从ID(资源ID, 模块句柄)
HBITMAP rc_findBitmapByID(int id, HMODULE hModule = NULL);

//@备注  从RC资源中查找指定字符串资源, 资源名称必须为纯数字ID
//@示例  文本型  文本数据 = RC资源查找字符串(101)
//@参数  资源ID(数字型)
//@参数  RC资源所属模块, 如果指定NULL, 那么为当前EXE模块
//@返回  返回查找到的字符串
//@别名  RC资源查找字符串(资源ID, 模块句柄)
CXText  rc_findString(int id, HMODULE hModule = NULL);

//@分组}

//@分组{ 注册表操作
/*@声明

#define  HKEY_CLASSES_ROOT     //@别名  注册表键_根类
#define  HKEY_CURRENT_USER     //@别名  注册表键_当前用户
#define  HKEY_LOCAL_MACHINE    //@别名  注册表键_本地机器
#define  HKEY_USERS            //@别名  注册表键_所有用户
#define  HKEY_CURRENT_CONFIG   //@别名  注册表键_当前配置


DECLARE_HANDLE(HKEY);  ///<注册表-键句柄
@备注  未定义值类型。
@别名  注册表值类型_未定义
#define  REG_NONE      0

@备注  指定以空值终止的 Unicode 字符串。
@别名  注册表值类型_字符串
#define  REG_SZ         1

@备注  指定以 null 结尾的字符串，该字符串包含对环境变量的未展开引用，例如 。它是 Unicode 或 ANSI 字符串，具体取决于您使用的是 Unicode 函数还是 ANSI 函数。%PATH%
@别名  注册表值类型_字符串2
#define  REG_EXPAND_SZ   2

@备注  指定任何形式的二进制数据。
@别名  注册表值类型_二进制数据
#define  REG_BINARY      3

@备注  指定 32 位数字。
@别名  注册表值类型_整型
#define  REG_DWORD      4

@备注  指定 64 位数字。
@别名  注册表值类型_长整型
#define  REG_QWORD     11

@备注  指定以空值终止的字符串的数组，以两个空字符终止。
@别名  注册表值类型_字符串数组
#define  REG_MULTI_SZ     7

*/

//@备注 此函数打开指定的键, 当您不再需要返回的句柄时，请调用 RegCloseKey 函数将其关闭。
//https://docs.microsoft.com/en-us/previous-versions/ms813308(v=msdn.10)
//@示例 注册表打开键(注册表键_本地机器, "SOFTWARE\\7-Zip", hKey) //见 基础示例->注册表操作
//@参数  主键句柄,当前打开的键或以下任何预定义的保留句柄值的句柄:  注册表键_*
//@参数  子键名称
//@参数  接收返回的句柄, 请在使用完句柄后调用 RegCloseKey 函数
//@返回  ERROR_SUCCESS(0)表示成功
//@别名  注册表打开键(主键句柄, 子键名称, 返回打开的键)
LONG reg_openKey(HKEY hKey, LPCWSTR lpSubKey, HKEY& hResult);

// @备注  此函数枚举指定的打开的注册表项的值。此函数在每次调用密钥时复制一个索引值名称和数据块
// @示例  //见 基础示例->注册表操作
// @参数  --hKey     [in] 键句柄
// @参数  --dwIndex  [in] 要检索的值的索引。对于此函数的第一次调用，此参数设置为零，然后在后续调用中递增。
// @参数  --retNmae  [out] 接收名称。
// @参数  --retBytes [out] 接收返回值
// @参数  --retType  [out] 接收返回数据类型
// @返回  ERROR_SUCCESS(0)表示成功, 应用程序最初调用此函数，并将 dwIndex 设置为零。然后，应用程序递增 dwIndex 并调用此函数，直到没有更多值。也就是说，直到函数返回ERROR_NO_MORE_ITEMS
// @别名  注册表枚举值(键句柄, 要检索的值的索引, 接收名称, 接收值, 接受值缓冲区大小, 接收数据类型)
LONG reg_enumValue(HKEY hKey, DWORD dwIndex, CXText& retNmae, CXBytes& retBytes, UINT& retType);

// @备注  注册表设置指定项值
// @示例  //见 基础示例->注册表操作
// @参数  --hRoot    主键句柄
// @参数  --pPath    子键路径字符串
// @参数  --pName    名称
// @参数  --pValue   值
// @返回  如果成功返回TRUE,否则返回FALSE
// @别名  注册表置文本(主键句柄, 子键路径, 名称, 值)
BOOL reg_setValueText(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, const wchar_t* pValue);

// @示例  //见 基础示例->注册表操作
// @参数  主键句柄
// @参数  子键路径字符串
// @参数  名称
// @参数  值
// @返回  如果成功返回TRUE,否则返回FALSE
// @别名 注册表置整型(主键句柄, 子键路径, 名称, 值)
BOOL reg_setValueInt(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, DWORD dwValue);

// @示例  //见 基础示例->注册表操作
// @参数  主键句柄
// @参数  子键路径字符串
// @参数  名称
// @参数  值
// @返回  如果成功返回TRUE,否则返回FALSE
// @别名 注册表置长整型(主键句柄, 子键路径, 名称, 值)
BOOL reg_setValueInt64(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, INT64 dwValue);

// @示例  //见 基础示例->注册表操作
// @参数  主键句柄
// @参数  子键路径字符串
// @参数  名称
// @参数  数据地址
// @参数  数据大小(字节为单位)
// @返回  如果成功返回TRUE,否则返回FALSE
// @别名 注册表置数据(主键句柄, 子键路径, 名称, 数据, 数据大小)
BOOL reg_setValueData(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, BYTE* data, DWORD dataSize);

// @备注  检索与打开的注册表项关联的指定值名称的类型和数据
// @示例  //见 基础示例->注册表操作
// @参数  --hRoot  主键句柄
// @参数  --pPath  子键路径字符串
// @参数  --pName  注册表值的名称
// @参数  --retBytes  接收返回数据
// @参数  --pType 指向变量的指针，该变量接收指示以指定值存储的数据类型的代码。有关可能的类型代码的列表，请参阅注册表值类型。如果不需要类型代码，则 pType 参数可以为 NULL。
// @返回  ERROR_SUCCESS(0)表示成功
// @别名  注册表取数据(主键句柄, 子键路径, 名称, 接收数据, 接收数据类型)
LONG reg_queryValueData(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, CXBytes& retBytes, DWORD& retType);

// @备注  注册表查询值, 当值为文本型有效
// @示例  //见 基础示例->注册表操作
// @参数  --hRoot  主键句柄
// @参数  --pPath  子键路径字符串
// @参数  --pName  注册表值的名称
// @参数  --retValue 返回值文本
// @返回  如果成功返回TRUE,否则返回FALSE
// @别名  注册表取文本(主键句柄, 子键路径, 名称, 接收文本)
BOOL reg_queryValueText(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, CXText& retValue);

// @示例  //见 基础示例->注册表操作
// @参数  主键句柄
// @参数  子键路径字符串
// @参数  名称
// @参数  接收返回值
// @返回  如果成功返回TRUE,否则返回FALSE
// @别名  注册表取整型(主键句柄, 子键路径, 名称, 接收数据)
BOOL reg_queryValueInt(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, UINT& retValue);

// @示例  //见 基础示例->注册表操作
// @参数  主键句柄
// @参数  子键路径字符串
// @参数  名称
// @参数  接收返回值
// @返回  如果成功返回TRUE,否则返回FALSE
// @别名  注册表取长整型(主键句柄, 子键路径, 名称, 接收数据)
BOOL reg_queryValueInt64(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, INT64& retValue);

// @示例  //见 基础示例->注册表操作
// @参数  主键句柄
// @参数  子键路径字符串
// @参数  名称
// @返回  如果成功返回TRUE,否则返回FALSE
//@别名  注册表是否存在(主键句柄, 子键路径, 名称)
BOOL reg_isExist(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName);

// @示例  整型 ret = 注册表创建键(注册表键_当前用户, "xc")  //见 基础示例->注册表操作
// @参数  [in] 主键句柄
// @参数  [in] 子键路径字符串
// @返回  如果函数成功，则返回值ERROR_SUCCESS(0), 若返回 ERROR_ACCESS_DENIED(5) 权限不足, 尝试使用管理员权限运行程序
// @别名  注册表创建键(主键句柄, 子键路径字符串, 接收键句柄)
LONG reg_createKey(HKEY hRoot, LPCWSTR pSubKey);

// @示例  //见 基础示例->注册表操作
// @参数  主键句柄
// @参数  子键路径字符串
// @参数  名称
// @返回  ERROR_SUCCESS(0)表示成功
//@别名  注册表删除值(主键句柄, 子键路径字符串, 名称)
LONG reg_deleteVaule(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName);

// @示例  注册表删除键(注册表键_当前用户, "xc") //见 基础示例->注册表操作
// @参数  [in] 主键句柄
// @参数  [in] 子键路径字符串
//@返回  ERROR_SUCCESS(0)成功, ERROR_FILE_NOT_FOUND(2)没有找到
//@别名  注册表删除键(主键句柄, 子键路径字符串)
LONG reg_deleteKey(HKEY hKey, const wchar_t* pPath);

/*@声明

@备注  此函数释放指定键的句柄
@示例  注册表关闭键(hKey) //见 基础示例->注册表操作
@参数  要关闭的打开键的句柄
@返回  ERROR_SUCCESS(0)表示成功。非零值表示失败
@别名  注册表关闭键(键句柄)
LONG WINAPI RegCloseKey(HKEY hKey);
*/

//@分组}


//@分组{ 系统错误码
/*@声明

//C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include\WinError.h

//@备注  Windows API 中定义的一个常量，用于表示函数调用成功。
//在 Windows 编程里，很多系统函数在执行操作后会返回一个表示操作结果的状态码，
//当这个状态码为 ERROR_SUCCESS 时，就意味着函数执行的操作顺利完成，没有出现错误
#define  ERROR_SUCCESS         0         //@别名   错误码_成功

//@备注  Windows API 中定义的一个错误代码常量，用于表示在文件或目录操作过程中，指定的文件或目录未被找到。以下是关于它的详细介绍
#define  ERROR_FILE_NOT_FOUND  0x2       //@别名   错误码_文件找不到

//@备注  Windows API 里的一个错误代码常量，用于表明在执行文件、目录或者注册表相关操作时，指定的路径不存在
#define  ERROR_PATH_NOT_FOUND  0x3       //@别名   错误码_路径找不到

//@备注  Windows API 中定义的一个错误代码常量，它表示在尝试访问某个资源（如文件、目录、注册表项等）时，由于权限不足而被拒绝访问
#define  ERROR_ACCESS_DENIED   0x5       //@别名   错误码_拒绝

//@备注  Windows API 中用于表示枚举操作已完成，没有更多项可供枚举的错误代码常量
#define  ERROR_NO_MORE_ITEMS   0x259     //@别名   错误码_没有更多数据

*/
//@分组}


//@分组{  动态库-模块

//@备注 获取指定模块（通常是可执行文件或动态链接库）的完整路径和文件名
//@示例 文本型  结果 = 取模块文件路径(NULL) //取当前进程完整路径和文件名
//@参数 模块句柄,可以是一个DLL模块，或者是一个应用程序的实例句柄。如果该参数为NULL，该函数返回该应用程序全路径。
//@返回 返回完整路径和文件名
//@别名 取模块文件路径(模块句柄)
CXText xcl_GetModuleFileName(HMODULE hModule);

/*@声明
 
//@备注 只有欲获取的模块已映射到调用该函数的进程内，才会正确得到模块句柄。常用模块映射函数：LoadLibrary。
//@示例 HMODULE hModule = 取模块句柄()
//@参数 模块名称, 指向含有模块名称字符串的指针；
//@返回 如执行成功成功，则返回模块句柄。零表示失败
//@别名 取模块句柄(模块文件名)
HMODULE GetModuleHandle(LPCWSTR lpModuleName);

//@备注 只有欲获取的模块已映射到调用该函数的进程内，才会正确得到模块句柄。常用模块映射函数：LoadLibrary。
//@参数 标志
//@参数 模块名称, 指向含有模块名称字符串的指针；
//@参数 句柄
//@返回 如执行成功成功，则返回真。零表示假
//@别名 取模块句柄扩展(标志, 模块文件名, 返回模块句柄)
BOOL GetModuleHandleEx(DWORD dwFlags,LPCWSTR lpModuleName,HMODULE *phModule);

//@备注 载入指定的动态链接库，并将它映射到当前进程使用的地址空间。一旦载入，即可访问库内保存的资源
//@doc  "doc\LoadLibrary.txt"
//@参数 动态库名称, 指定要载入的动态链接库的名称
//@返回 非零表示成功，零表示失败。会设置GetLastError
//@别名 动态库载入(动态库名称)
HMODULE LoadLibrary(LPCWSTR lpLibFileName);

//@备注 载入指定的动态链接库，并将它映射到当前进程使用的地址空间。一旦载入，即可访问库内保存的资源
//@参数 动态库名称, 指定要载入的动态链接库的名称
//@参数 文件,未用, 设为零
//@参数 标志, 指定下述常数的一个或多个,详细MSDN
//@返回 非零表示成功，零表示失败。会设置GetLastError
//@别名 动态库载入扩展(动态库名称,文件,标志)
HMODULE LoadLibraryEx(LPCWSTR lpLibFileName,HANDLE hFile,DWORD dwFlags);

//@备注 释放指定的动态链接库，它们早先是用LoadLibrary API函数装载的
//@doc  "doc\LoadLibrary.txt"
//@参数 库文件句柄, 要释放的动态链接库句柄
//@返回 非零表示成功，零表示失败。会设置GetLastError
//@别名 动态库释放(库文件句柄)
BOOL FreeLibrary(HMODULE hLibModule);

//@备注 载入指定的动态链接库，并将它映射到当前进程使用的地址空间。一旦载入，即可访问库内保存的资源
//@doc  "doc\LoadLibrary.txt"
//@参数 模块句柄,包含此函数的DLL模块的句柄。LoadLibrary、AfxLoadLibrary 或者GetModuleHandle函数可以返回此句柄。
//@参数 函数名,包含函数名的以NULL结尾的字符串，或者指定函数的序数值。如果此参数是一个序数值，它必须在低字，高字必须为0。
//@返回 如果函数调用成功，返回值是DLL中的输出函数地址。失败，返回值是NULL
//@别名 取函数地址(模块句柄,函数名)
FARPROC GetProcAddress(HMODULE hModule,LPCSTR lpProcName);

//@备注 将目录添加到进程 DLL 搜索路径, 最低支持WIN7
//Windows API 中的一个函数，它主要用于向进程的动态链接库（DLL）搜索路径中添加指定的目录。
//在 Windows 系统里，当程序加载 DLL 时，会按照一定的搜索顺序去查找所需的 DLL 文件，
//而 AddDllDirectory 可以帮助开发者灵活地修改这个搜索路径，以满足特定的 DLL 加载需求
//@参数 要添加到搜索路径的目录的绝对路径
//@返回 如果函数成功，则返回值是一个不透明的指针，可传递给 RemoveDllDirectory 以从进程 DLL 搜索路径中删除 DLL;
//如果函数失败，则返回值为零
//@别名 添加DLL搜索路径()
DLL_DIRECTORY_COOKIE AddDllDirectory(PCWSTR NewDirectory);

//@备注 Windows API 中的一个函数，用于从进程的动态链接库（DLL）搜索路径中移除之前通过 AddDllDirectory 函数添加的特定目录
//@参数 将目录添加到搜索路径时, AddDllDirectory 返回的 Cookie
//@返回 如果函数调用成功, 返回TRUE, 否则返回FALSE
//@别名 移除DLL搜索路径()
BOOL RemoveDllDirectory(DLL_DIRECTORY_COOKIE Cookie);

//@分组}


//@分组{ COM相关

/*@声明

#define HRESULT long         //@别名 COM返回值
BOOL SUCCEEDED(HRESULT hr);  //@别名 COM是否成功()
BOOL FAILED(HRESULT hr);     //@别名 COM是否失败()

#define S_OK               ((HRESULT)0L)
#define S_FALSE            ((HRESULT)1L)

struct IUnknown
{
	//@备注 用于查询对象是否支持指定的接口，并返回该接口的指针
	//@参数 riid 接口的唯一标识符 (IID)
	//@参数 ppvObject 返回的接口指针
	//@返回 如果成功，返回 S_OK；如果接口不被支持，返回 E_NOINTERFACE
	//@别名 查询接口()
	virtual HRESULT __stdcall QueryInterface(__in  REFIID riid, __out void** ppvObject) = 0;

	//@备注 用于增加对象的引用计数，表示有新的客户端在使用该对象
	//@返回 对象的当前引用计数
	//@别名 增加引用计数()
	virtual ULONG __stdcall AddRef(void) = 0;

	//@备注 用于减少对象的引用计数，当引用计数为 0 时，对象会被销毁
	//@返回 对象的当前引用计数
	//@别名 减少引用计数()
	virtual ULONG __stdcall Release(void) = 0;

	//@备注 模板方法，用于简化接口查询，内部调用 QueryInterface
	//@参数 pp 返回的接口指针
	//@返回 如果成功，返回 S_OK；如果接口不被支持，返回 E_NOINTERFACE
	//@别名 查询接口模板()
	template<class Q>
	HRESULT __stdcall QueryInterface(Q** pp) {
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
};

*/

//@备注 用于自动封装COM模块 基类
class CXComObj
{
public:
//@隐藏{
	CComPtr<IUnknown>  m_ptr;
//@隐藏}
};

//@分组}


//@分组{  虚拟键码.字母键
// VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
#define  VK_A  0x41  //@别名  键A
#define  VK_B  0x42  //@别名  键B
#define  VK_C  0x43  //@别名  键C
#define  VK_D  0x44  //@别名  键D
#define  VK_E  0x45  //@别名  键E
#define  VK_F  0x46  //@别名  键F
#define  VK_G  0x47  //@别名  键G
#define  VK_H  0x48  //@别名  键H
#define  VK_I  0x49  //@别名  键I
#define  VK_J  0x4A  //@别名  键J
#define  VK_K  0x4B  //@别名  键K
#define  VK_L  0x4C  //@别名  键L
#define  VK_M  0x4D  //@别名  键M
#define  VK_N  0x4E  //@别名  键N
#define  VK_O  0x4F  //@别名  键O
#define  VK_P  0x50  //@别名  键P
#define  VK_Q  0x51  //@别名  键Q
#define  VK_R  0x52  //@别名  键R
#define  VK_S  0x53  //@别名  键S
#define  VK_T  0x54  //@别名  键T
#define  VK_U  0x55  //@别名  键U
#define  VK_V  0x56  //@别名  键V
#define  VK_W  0x57  //@别名  键W
#define  VK_X  0x58  //@别名  键X
#define  VK_Y  0x59  //@别名  键Y
#define  VK_Z  0x5A  //@别名  键Z
//@分组}

//@分组{ 虚拟键码.数字键
// VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
#define VK_NUM0    0x30    //@别名 键0
#define VK_NUM1    0x31    //@别名 键1
#define VK_NUM2    0x32    //@别名 键2
#define VK_NUM3    0x33    //@别名 键3
#define VK_NUM4    0x34    //@别名 键4
#define VK_NUM5    0x35    //@别名 键5
#define VK_NUM6    0x36    //@别名 键6
#define VK_NUM7    0x37    //@别名 键7
#define VK_NUM8    0x38    //@别名 键8
#define VK_NUM9    0x39    //@别名 键9

/*@声明
//@别名  数字键0
#define VK_NUMPAD0	0x60	///<数字键盘 0 键
//@别名  数字键1
#define VK_NUMPAD1	0x61	///<数字键盘 1 键
//@别名  数字键2
#define VK_NUMPAD2	0x62	///<数字键盘 2 键
//@别名  数字键3
#define VK_NUMPAD3	0x63	///<数字键盘 3 键
//@别名  数字键4
#define VK_NUMPAD4	0x64	///<数字键盘 4 键
//@别名  数字键5
#define VK_NUMPAD5	0x65	///<数字键盘 5 键
//@别名  数字键6
#define VK_NUMPAD6	0x66	///<数字键盘 6 键
//@别名  数字键7
#define VK_NUMPAD7	0x67	///<数字键盘 7 键
//@别名  数字键8
#define VK_NUMPAD8	0x68	///<数字键盘 8 键
//@别名  数字键9
#define VK_NUMPAD9	0x69	///<数字键盘 9 键
*/
//@分组}


//@分组{ 虚拟键码.F键
//@别名  键F1
#define VK_F1	0x70	///<F1 键
//@别名  键F2
#define VK_F2	0x71	///<F2 键
//@别名  键F3
#define VK_F3	0x72	///<F3 键
//@别名  键F4
#define VK_F4	0x73	///<F4 键
//@别名  键F5
#define VK_F5	0x74	///<F5 键
//@别名  键F6
#define VK_F6	0x75	///<F6 键
//@别名  键F7
#define VK_F7	0x76	///<F7 键
//@别名  键F8
#define VK_F8	0x77	///<F8 键
//@别名  键F9
#define VK_F9	0x78	///<F9 键
//@别名  键F10
#define VK_F10	0x79	///<F10 键
//@别名  键F11
#define VK_F11	0x7A	///<F11 键
//@别名  键F12
#define VK_F12	0x7B	///<F12 键
//@别名  键F13
#define VK_F13	0x7C	///<F13 键
//@别名  键F14
#define VK_F14	0x7D	///<F14 键
//@别名  键F15
#define VK_F15	0x7E	///<F15 键
//@别名  键F16
#define VK_F16	0x7F	///<F16 键
//@别名  键F17
#define VK_F17	0x80	///<F17 键
//@别名  键F18
#define VK_F18	0x81	///<F18 键
//@别名  键F19
#define VK_F19	0x82	///<F19 键
//@别名  键F20
#define VK_F20	0x83	///<F20 键
//@别名  键F21
#define VK_F21	0x84	///<F21 键
//@别名  键F22
#define VK_F22	0x85	///<F22 键
//@别名  键F23
#define VK_F23	0x86	///<F23 键
//@别名  键F24
#define VK_F24	0x87	///<F24 键
//@分组} //虚拟键码-F键


//@分组{ 虚拟键码.其他

/*@声明

//@别名  鼠标左键
#define VK_LBUTTON	0x01	///<鼠标左键
//@别名  鼠标右键
#define VK_RBUTTON	0x02	///<鼠标右键
#define VK_CANCEL	0x03	///<控制中断处理
//@别名  鼠标中键
#define VK_MBUTTON	0x04	///<鼠标中键

//@别名  鼠标按钮1
#define VK_XBUTTON1	0x05	///<X1 鼠标按钮
//@别名  鼠标按钮2
#define VK_XBUTTON2	0x06	///<X2 鼠标按钮
//@别名  退格键
#define VK_BACK		0x08	///<BACKSPACE 键
//@别名  Tab键
#define VK_TAB		0x09	///<Tab 键

#define VK_CLEAR	0x0C	///<CLEAR 键
//@别名  回车键
#define VK_RETURN	0x0D	///<Enter 键
//@别名  Shift键
#define VK_SHIFT	0x10	///<SHIFT 键
//@别名  Ctrl键
#define VK_CONTROL	0x11	///<CTRL 键
//@别名  Alt键
#define VK_MENU		0x12	///<Alt 键
//@别名  暂停键
#define VK_PAUSE	0x13	///<PAUSE 键

//@别名  大写锁定键
#define VK_CAPITAL	0x14	///<CAPS LOCK 键
#define VK_KANA		0x15	///<IME Kana 模式
#define VK_HANGUL	0x15	///<IME Hanguel 模式
#define VK_IME_ON	0x16	///<IME 打开
#define VK_JUNJA	0x17	///<IME Junja 模式
#define VK_FINAL	0x18	///<IME 最终模式
#define VK_HANJA	0x19	///<IME Hanja 模式
#define VK_KANJI	0x19	///<IME Kanji 模式
#define VK_IME_OFF	0x1A	///<IME 关闭

//@别名  退出键
#define VK_ESCAPE	0x1B	///<ESC 键
#define VK_CONVERT	0x1C	///<IME 转换
#define VK_NONCONVERT	0x1D	///<IME 不转换
#define VK_ACCEPT		0x1E	///<IME 接受
#define VK_MODECHANGE	0x1F	///<IME 模式更改请求

//@别名  空格键
#define VK_SPACE	0x20	///<空格键
//@别名  上翻页键
#define VK_PRIOR	0x21	///<PAGE UP 键
//@别名  下翻页键
#define VK_NEXT		0x22	///<PAGE DOWN 键
//@别名  End键
#define VK_END		0x23	///<END 键
//@别名  Home键
#define VK_HOME		0x24	///<HOME 键
//@别名  左光标键
#define VK_LEFT		0x25	///<LEFT ARROW 键
//@别名  上光标键
#define VK_UP		0x26	///<UP ARROW 键
//@别名  右光标键
#define VK_RIGHT	0x27	///<RIGHT ARROW 键
//@别名  下光标键
#define VK_DOWN		0x28	///<DOWN ARROW 键
//@别名  选择键
#define VK_SELECT	0x29	///<SELECT 键
//@别名  打印键
#define VK_PRINT	0x2A	///<PRINT 键

//@别名 执行键
#define VK_EXECUTE	0x2B	///<EXECUTE 键
// 抓屏键
#define VK_SNAPSHOT	0x2C	///<PRINT SCREEN 键
//@别名 插入键
#define VK_INSERT	0x2D	///<INS 键
//@别名 删除键
#define VK_DELETE	0x2E	///<DEL 键
//@别名 帮助键
#define VK_HELP		0x2F	///<HELP 键


// VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
// 0x40 : unassigned
// VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)

//@别名 左Win键
#define VK_LWIN	0x5B	///<左 Windows 键
//@别名 右Win键
#define VK_RWIN	0x5C	///<右侧 Windows 键

#define VK_APPS	0x5D	///<应用程序密钥
//@别名  休眠键
#define VK_SLEEP	0x5F	///<计算机休眠键

//@别名  乘号键
#define VK_MULTIPLY	0x6A	///<乘号键
//@别名  加号键
#define VK_ADD		0x6B	///<加号键
//@别名  分隔符键
#define VK_SEPARATOR	0x6C	///<分隔符键
//@别名  减号键
#define VK_SUBTRACT		0x6D	///<减号键
//@别名  句点键
#define VK_DECIMAL		0x6E	///<句点键
//@别名  除号键
#define VK_DIVIDE		0x6F	///<除号键

//@别名  数字锁定键
#define VK_NUMLOCK	0x90	///<NUM LOCK 键
//@别名  滚动锁定键
#define VK_SCROLL	0x91	///<SCROLL LOCK 键


//NEC PC-9800 kbd definitions
#define VK_OEM_NEC_EQUAL  0x92   // '=' key on numpad

//Fujitsu/OASYS kbd definitions
#define VK_OEM_FJ_JISHO   0x92   ///< 'Dictionary' key
#define VK_OEM_FJ_MASSHOU 0x93   ///< 'Unregister word' key
#define VK_OEM_FJ_TOUROKU 0x94   ///< 'Register word' key
#define VK_OEM_FJ_LOYA    0x95   ///< 'Left OYAYUBI' key
#define VK_OEM_FJ_ROYA    0x96   ///< 'Right OYAYUBI' key

//@别名  左Shift键
#define VK_LSHIFT	0xA0	///<左 SHIFT 键
//@别名  右Shift键
#define VK_RSHIFT	0xA1	///<右 SHIFT 键
//@别名  左Ctrl键
#define VK_LCONTROL	0xA2	///<左 Ctrl 键
//@别名  右Ctrl键
#define VK_RCONTROL	0xA3	///<右 Ctrl 键
//@别名  左Alt键
#define VK_LMENU	0xA4	///<左 ALT 键
//@别名  右Alt键
#define VK_RMENU	0xA5	///<右 ALT 键
//@别名  浏览器后退键
#define VK_BROWSER_BACK		0xA6	///<浏览器后退键
//@别名  浏览器前进键
#define VK_BROWSER_FORWARD	0xA7	///<浏览器前进键
//@别名  浏览器刷新键
#define VK_BROWSER_REFRESH	0xA8	///<浏览器刷新键
//@别名  浏览器停止键
#define VK_BROWSER_STOP		0xA9	///<浏览器停止键
//@别名  浏览器搜索键
#define VK_BROWSER_SEARCH	0xAA	///<浏览器搜索键
//@别名  浏览器收藏键
#define VK_BROWSER_FAVORITES	0xAB	///<浏览器收藏键
//@别名  浏览器主页键
#define VK_BROWSER_HOME	0xAC	///<浏览器“开始”和“主页”键
//@别名  静音键
#define VK_VOLUME_MUTE	0xAD	///<静音键
//@别名  音量减小键
#define VK_VOLUME_DOWN	0xAE	///<音量减小键
//@别名  音量增加键
#define VK_VOLUME_UP	0xAF	///<音量增加键
//@别名  下一曲目键
#define VK_MEDIA_NEXT_TRACK	0xB0	///<下一曲目键
//@别名  上一曲目键
#define VK_MEDIA_PREV_TRACK	0xB1	///<上一曲目键
//@别名  停止媒体键
#define VK_MEDIA_STOP		0xB2	///<停止媒体键
//@别名  暂停媒体键
#define VK_MEDIA_PLAY_PAUSE	0xB3	///<播放 / 暂停媒体键
//@别名  启动邮件键
#define VK_LAUNCH_MAIL		0xB4	///<启动邮件键
//@别名  选择媒体键
#define VK_LAUNCH_MEDIA_SELECT	0xB5	///<选择媒体键
//@别名  启动应用程序1键
#define VK_LAUNCH_APP1	0xB6	///<启动应用程序 1 键
//@别名  启动应用程序2键
#define VK_LAUNCH_APP2	0xB7	///<启动应用程序 2 键


#define VK_OEM_1		0xBA	///<用于杂项字符；它可能因键盘而异。 对于美国标准键盘，键; :
#define VK_OEM_PLUS		0xBB	///<对于任何国家 / 地区，键 +
#define VK_OEM_COMMA	0xBC	///<对于任何国家 / 地区，键,
#define VK_OEM_MINUS	0xBD	///<对于任何国家 / 地区，键 -
#define VK_OEM_PERIOD	0xBE	///<对于任何国家 / 地区，键.
#define VK_OEM_2		0xBF	///<用于杂项字符；它可能因键盘而异。 对于美国标准键盘，键 / ?
#define VK_OEM_3		0xC0	///<用于杂项字符；它可能因键盘而异。 对于美国标准键盘，键`~

#define VK_OEM_4	0xDB	///<用于杂项字符；它可能因键盘而异。 对于美国标准键盘，键[{
#define VK_OEM_5	0xDC	///<用于杂项字符；它可能因键盘而异。 对于美国标准键盘，键\\ |
#define VK_OEM_6	0xDD	///<用于杂项字符；它可能因键盘而异。 对于美国标准键盘，键]}
#define VK_OEM_7	0xDE	///<用于杂项字符；它可能因键盘而异。 对于美国标准键盘，键'"
#define VK_OEM_8	0xDF	///<用于杂项字符；它可能因键盘而异。


#define VK_OEM_AX		0xE1  ///<  'AX' key on Japanese AX kbd
#define VK_OEM_102		0xE2  ///< 美国标准键盘上的 <> 键，或非美国 102 键键盘上的 \\| 键
#define VK_ICO_HELP		0xE3  ///<  Help key on ICO
#define VK_ICO_00		0xE4  ///<  00 key on ICO

#define VK_PROCESSKEY	0xE5	///<IME PROCESS 键

#define VK_ICO_CLEAR      0xE6  ///<OEM 特有


#define VK_PACKET		0xE7	///<用于将 Unicode 字符当作键击传递。 VK_PACKET 键是用于非键盘输入法的 32 位虚拟键值的低位字。 有关更多信息，请参阅 KEYBDINPUT、SendInput、WM_KEYDOWN 和 WM_KEYUP 中的注释

#define VK_OEM_RESET      0xE9
#define VK_OEM_JUMP       0xEA
#define VK_OEM_PA1        0xEB
#define VK_OEM_PA2        0xEC
#define VK_OEM_PA3        0xED
#define VK_OEM_WSCTRL     0xEE
#define VK_OEM_CUSEL      0xEF
#define VK_OEM_ATTN       0xF0
#define VK_OEM_FINISH     0xF1
#define VK_OEM_COPY       0xF2
#define VK_OEM_AUTO       0xF3
#define VK_OEM_ENLW       0xF4
#define VK_OEM_BACKTAB    0xF5

#define VK_ATTN		0xF6	///<Attn 键
#define VK_CRSEL	0xF7	///<CrSel 键
#define VK_EXSEL	0xF8	///<ExSel 键
#define VK_EREOF	0xF9	///<Erase EOF 键

//@别名  播放键
#define VK_PLAY		0xFA	///<Play 键
//@别名  缩放键
#define VK_ZOOM		0xFB	///<Zoom 键
#define VK_NONAME	0xFC	///<预留
#define VK_PA1		0xFD	///<PA1 键
#define VK_OEM_CLEAR  0xFE  ///<Clear 键
*/

//@分组}  //虚拟键码.其他


//@分组{  其他
/*@声明
#define WPARAM   //@别名 附加参数1
#define LPARAM   //@别名 附加参数2

#define  DLL_PROCESS_ATTACH   1   //@别名 DLL进程附加
#define  DLL_THREAD_ATTACH    2   //@别名 DLL线程附加
#define  DLL_THREAD_DETACH    3   //@别名 DLL线程分离
#define  DLL_PROCESS_DETACH   0   //@别名 DLL进程分离

#define   NULL      0   //@别名   空
#define   nullptr       //@别名   空指针

*/

//@备注  获取当前函数名称
//@示例  调试输出(__函数名__)
//@别名  __函数名__
#define  XL_FUNNAME   U2W(__FUNCTION__).get()

//@备注  圆周率，是指圆的周长与直径的比值，即圆周率=圆周长÷直径，一般用希腊字母π表示，是一个在数学及物理学中普遍存在的数学常数
//@别名  圆周率
#define XCL_PI 3.14159265358979323846



//@分组}   //其他


#include "xc_mkStr.h"
#include "xc_mkStrA.h"
//@src  "xc_mkStr.cpp"


//@隐藏{

// 用于炫语言代码类成员绑定事件
struct xc_bind_event_
{
	void* pDestObj;
	std::wstring strDestObj;
};

// 事件管理类，用于 CPP 模块，类成员变量事件代码自动生成与绑定
template<class T>
class CXEventMgr
{
public:
	template<class T>
	struct xc_bind_event_item_
	{
		int  nEvent;
		T*   pObj;
		void (T::* memberFun)();
	};

	std::map<std::wstring, xc_bind_event_item_<T>> m_map;
	bool RegEvent(const wchar_t* pName, T* pObj, void (T::* memberFun)())
	{
		if (NULL== pName || 0==*pName || m_map.find(pName) != m_map.end())
			return FALSE;
		 
		xc_bind_event_item_<T>& slot = m_map[pName];
		slot.nEvent = 0;
		slot.pObj = pObj;
		slot.memberFun = memberFun;
		return true;
	}

	xc_bind_event_item_<T>* FindEvent(const wchar_t* pName)
	{
		auto pIter = m_map.find(pName);
		if (pIter != m_map.end())
			return &pIter->second;
		return nullptr;
	}
};

int  xcl_textToInt(const CXText& text);
int  xcl_textToInt(const CXTextA& text);
int  xcl_textToInt(const wchar_t* text);
int  xcl_textToInt(const char* text);

UINT  xcl_textToUint(const CXText& text);
UINT  xcl_textToUint(const CXTextA& text);
UINT  xcl_textToUint(const wchar_t* text);
UINT  xcl_textToUint(const char* text);

INT64  xcl_textToInt64(const CXText& text);
INT64  xcl_textToInt64(const CXTextA& text);
INT64  xcl_textToInt64(const wchar_t* text);
INT64  xcl_textToInt64(const char* text);

UINT64  xcl_textToUint64(const CXText& text);
UINT64  xcl_textToUint64(const CXTextA& text);
UINT64  xcl_textToUint64(const wchar_t* text);
UINT64  xcl_textToUint64(const char* text);

float  xcl_textToFloat(const CXText& text);
float  xcl_textToFloat(const CXTextA& text);
float  xcl_textToFloat(const wchar_t* text);
float  xcl_textToFloat(const char* text);

double  xcl_textToDouble(const CXText& text);
double  xcl_textToDouble(const CXTextA& text);
double  xcl_textToDouble(const wchar_t* text);
double  xcl_textToDouble(const char* text);

//@隐藏}

#endif //XC_BASE_H