#ifndef XCL_LOGA_H
#define XCL_LOGA_H

//----------------------------------------------
// template<class T>
// void xcl_mkValue(CXTextA &strText, T t){
// 	strText += "?不支持类型?";
// }
template<class T>
void xcl_mkValue(CXTextA& strText, T* t){
	xcl_mkValue(strText, (void*)t);
}

template<class T, typename std::enable_if<!std::is_enum<T>::value, int>::type = 0>
void xcl_mkValue(CXTextA& strText, T t) {
	strText += "?不支持类型?";
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value>::type
xcl_mkValue(CXTextA& strText, T value_)
{
	// 将枚举类型转换为其底层整数类型
	xcl_mkValue(strText, static_cast<typename std::underlying_type<T>::type>(value_));
}

void xcl_mkValue(CXTextA& strText, const std::wstring& value_);
void xcl_mkValue(CXTextA& strText, const std::string& value_);

void xcl_mkValue(CXTextA& strText, const CXText& value_);
void xcl_mkValue(CXTextA& strText, const CXTextA& value_);
void xcl_mkValue(CXTextA& strText, const CXBytes& value_);
void xcl_mkValue(CXTextA& strText, void* value_);
void xcl_mkValue(CXTextA& strText, const wchar_t* value_);
void xcl_mkValue(CXTextA& strText, wchar_t* value_);
void xcl_mkValue(CXTextA& strText, const char* value_);
void xcl_mkValue(CXTextA& strText, char* value_);
void xcl_mkValue(CXTextA& strText, const wchar_t value_);
void xcl_mkValue(CXTextA& strText, const char value_);
//---------------
void xcl_mkValue(CXTextA& strText, bool value_);
void xcl_mkValue(CXTextA& strText, int value_);
void xcl_mkValue(CXTextA& strText, UINT value_);
void xcl_mkValue(CXTextA& strText, INT64 value_);
void xcl_mkValue(CXTextA& strText, UINT64 value_);
void xcl_mkValue(CXTextA& strText, const long value_);
void xcl_mkValue(CXTextA& strText, const unsigned long value_);
void xcl_mkValue(CXTextA& strText, const USHORT value_);
void xcl_mkValue(CXTextA& strText, const SHORT value_);
void xcl_mkValue(CXTextA& strText, const BYTE value_);
void xcl_mkValue(CXTextA& strText, const float value_);
void xcl_mkValue(CXTextA& strText, const double value_);

//----------xcl_mkStr-------------------------
template<class T>
CXTextA xcl_mkStrA(T t)
{
	CXTextA strText;
	xcl_mkValue(strText, t);
	return strText;
}

template<class T1, class T2>
CXTextA  xcl_mkStrA(T1 t1, T2 t2)
{
	CXTextA strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	return strText;
}

template<class T1, class T2, class T3>
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3)
{
	CXTextA strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	return strText;
}

template<class T1, class T2, class T3, class T4>
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4)
{
	CXTextA strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5>
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
{
	CXTextA strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6>
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
{
	CXTextA strText;
	xcl_mkValue(strText, t1);
	xcl_mkValue(strText, t2);
	xcl_mkValue(strText, t3);
	xcl_mkValue(strText, t4);
	xcl_mkValue(strText, t5);
	xcl_mkValue(strText, t6);
	return strText;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7>
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
{
	CXTextA strText;
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
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
{
	CXTextA strText;
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
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9)
{
	CXTextA strText;
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
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10)
{
	CXTextA strText;
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
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11)
{
	CXTextA strText;
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
CXTextA  xcl_mkStrA(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12)
{
	CXTextA strText;
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

//----------xcl_mkStr2A-------------------------
template<typename T>
void _xcl_mkStr2AT(CXTextA& mkString, const T value_)
{
	xcl_mkValue(mkString, value_);
}

template<typename T, typename ... Args>
void _xcl_mkStr2AT(CXTextA& mkString, const T t, const Args ... args)
{
	_xcl_mkStr2AT(mkString, t);
	_xcl_mkStr2AT(mkString, args...);
}

template<typename ... Args>
CXTextA xcl_mkStr2A(const Args ... args)
{
	CXTextA  mkString;
	_xcl_mkStr2AT(mkString, args...);
	return mkString;
}

#endif  //XC_LOGA_H