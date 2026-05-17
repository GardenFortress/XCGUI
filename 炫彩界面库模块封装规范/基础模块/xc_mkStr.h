#ifndef XC_LOG_H
#define XC_LOG_H

//----------------------------------------------
// template<class T>
// void xcl_mkValue(CXText &strText, T t){
// 	strText += L"?不支持类型?";
// }
template<class T>
void xcl_mkValue(CXText& strText, T* t){
	xcl_mkValue(strText, (void*)t);
}

template<class T, typename std::enable_if<!std::is_enum<T>::value, int>::type = 0>
void xcl_mkValue(CXText& strText, T t) {
	strText += L"?不支持类型?";
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value>::type
xcl_mkValue(CXText& strText, T value_)
{
	// 将枚举类型转换为其底层整数类型
	xcl_mkValue(strText, static_cast<typename std::underlying_type<T>::type>(value_));
}

void xcl_mkValue(CXText& strText, const std::wstring& value_);
void xcl_mkValue(CXText& strText, const std::string& value_);

void xcl_mkValue(CXText& strText, const CXText& value_);
void xcl_mkValue(CXText& strText, const CXTextA& value_);
void xcl_mkValue(CXText& strText, const CXBytes& value_);
void xcl_mkValue(CXText& strText, void* value_);
void xcl_mkValue(CXText& strText, const wchar_t* value_);
void xcl_mkValue(CXText& strText, wchar_t* value_);
void xcl_mkValue(CXText& strText, const char* value_);
void xcl_mkValue(CXText& strText, char* value_);
void xcl_mkValue(CXText& strText, const wchar_t value_);
void xcl_mkValue(CXText& strText, const char value_);
//---------------------
void xcl_mkValue(CXText& strText, const bool value_);
void xcl_mkValue(CXText& strText, const int value_);
void xcl_mkValue(CXText& strText, const UINT value_);
void xcl_mkValue(CXText& strText, const INT64 value_);
void xcl_mkValue(CXText& strText, const UINT64 value_);
void xcl_mkValue(CXText& strText, const long value_);
void xcl_mkValue(CXText& strText, const unsigned long value_);
void xcl_mkValue(CXText& strText, const USHORT value_);
void xcl_mkValue(CXText& strText, const SHORT value_);
void xcl_mkValue(CXText& strText, const BYTE value_);
void xcl_mkValue(CXText& strText, const float value_);
void xcl_mkValue(CXText& strText, const double value_);

//-------------------------------
void xcl_mkValue(CXText& strText, const unsigned int* value_);
void xcl_mkValue(CXText& strText, int* value_);
void xcl_mkValue(CXText& strText, unsigned int* value_);

//--------unsigned char-------------
void xcl_mkValue(CXText& strText, const unsigned char* value_);
void xcl_mkValue(CXText& strText, unsigned char* value_);
//--------short---------------------
void xcl_mkValue(CXText& strText, const unsigned short* value_);
void xcl_mkValue(CXText& strText, short* value_);
void xcl_mkValue(CXText& strText, const short* value_);
void xcl_mkValue(CXText& strText, unsigned short* value_);

//----------xcl_mkStr--------------------
template<class T>
CXText xcl_mkStr(T t)
{
	CXText strText;
	xcl_mkValue(strText, t);
	return strText;
}

template<class T1, class T2>
CXText  xcl_mkStr(T1 t1, T2 t2)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	return strText;
}

template<class T1, class T2, class T3>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	return strText;
}

template<class T1, class T2, class T3, class T4>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	xcl_mkValue(strText, t6);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	xcl_mkValue(strText, t6);
	xcl_mkValue(strText, t7);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	xcl_mkValue(strText, t6);
	xcl_mkValue(strText, t7);
	xcl_mkValue(strText, t8);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	xcl_mkValue(strText, t6);
	xcl_mkValue(strText, t7);
	xcl_mkValue(strText, t8);
	xcl_mkValue(strText, t9);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	xcl_mkValue(strText, t6);
	xcl_mkValue(strText, t7);
	xcl_mkValue(strText, t8);
	xcl_mkValue(strText, t9);
	xcl_mkValue(strText, t10);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	xcl_mkValue(strText, t6);
	xcl_mkValue(strText, t7);
	xcl_mkValue(strText, t8);
	xcl_mkValue(strText, t9);
	xcl_mkValue(strText, t10);
	xcl_mkValue(strText, t11);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
CXText  xcl_mkStr(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12)
{
	CXText strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	xcl_mkValue(strText, t6);
	xcl_mkValue(strText, t7);
	xcl_mkValue(strText, t8);
	xcl_mkValue(strText, t9);
	xcl_mkValue(strText, t10);
	xcl_mkValue(strText, t11);
	xcl_mkValue(strText, t12);
	return strText;
}

//----------xcl_mkStr2-------------------------
template<typename T>
void _xcl_mkStr2T(CXText& mkString, const T value_)
{
	xcl_mkValue(mkString, value_);
}

template<typename T, typename ... Args>
void _xcl_mkStr2T(CXText& mkString, const T t, const Args ... args)
{
	_xcl_mkStr2T(mkString, t);
	_xcl_mkStr2T(mkString, args...);
}

template<typename ... Args>
CXText xcl_mkStr2(const Args ... args)
{
	CXText  mkString;
	_xcl_mkStr2T(mkString, args...);
	return mkString;
}


//----------log-------------------------
#ifdef DEBUG
template<typename T>
void _xcl_logT(CXText& mkString, const T value_)
{
	xcl_mkValue(mkString, value_);
}

template<typename T, typename ... Args>
void _xcl_logT(CXText& mkString, const T t, const Args ... args)
{
	_xcl_logT(mkString, t); mkString += L", ";
	_xcl_logT(mkString, args...);
}

template<typename ... Args>
void xcl_log(const Args ... args)
{
	CXText  mkString;
	_xcl_logT(mkString, args...);
	mkString += L"\n";
	OutputDebugStringW(mkString.get());
}
void xcl_log();
//----------log2-------------------------
template<typename T>
void _xcl_logT2(CXText& mkString, const T value_)
{
	xcl_mkValue(mkString, value_);
}

template<typename T, typename ... Args>
void _xcl_logT2(CXText& mkString, const T t, const Args ... args)
{
	_xcl_logT2(mkString, t);
	_xcl_logT2(mkString, args...);
}

template<typename ... Args>
void xcl_log2(const Args ... args)
{
	CXText mkString;
	_xcl_logT2(mkString, args...);
	mkString += L"\n";
	OutputDebugStringW(mkString.get());
}
void xcl_log2();

void xcl_xtrace(const char* pFormat, ...);
void xcl_xtracew(const wchar_t* pFormat, ...);

#else

#define xcl_log(...)
#define xcl_log2(...)
#define xcl_xtrace(...)
#define xcl_xtracew(...)
#endif



#endif  //XC_LOG_H