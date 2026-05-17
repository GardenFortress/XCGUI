#include "module_base.h"

void xcl_mkValue(CXText& strText, const std::wstring& value_) {
	strText += value_.c_str();
}
void xcl_mkValue(CXText& strText, const std::string& value_) {
	strText += A2W(value_.c_str(), (int)value_.size());
}
void xcl_mkValue(CXText& strText, const CXText& value_){
	strText += value_;
}
void xcl_mkValue(CXText& strText, const CXTextA& value_){
	strText += A2W(value_.get(), (int)value_.size());
}
void xcl_mkValue(CXText& strText, const CXBytes& value_){
	size_t length_old = value_.size();
	size_t length = length_old;
	if (length > 128) length = 128;
	CXText  text = xcl_dataToHexText(value_.get(), length, TRUE, TRUE);
	strText += L"大小:";
	strText += xcl_toText(length_old);
	strText += L", 十六进制字节{";
	strText += text;
	if (length_old > 128)
		strText += L" ...";
	strText += L"}";
}
void xcl_mkValue(CXText &strText, void* value_){
#ifdef _WIN64
	wchar_t make_buf[20] = { 0 };
	swprintf_s(make_buf, L"0x%016I64X", (INT64)value_);
#else
	wchar_t make_buf[12] = { 0 };
	swprintf_s(make_buf, L"0x%08X", (unsigned int)value_);
#endif // _WIN64
	strText += make_buf;
}
void xcl_mkValue(CXText& strText, const wchar_t* value_){
	if (value_) strText += value_;
}
void xcl_mkValue(CXText& strText, wchar_t* value_) {
	if (value_) strText += value_;
}
void xcl_mkValue(CXText& strText, const char* value_){
	if (value_)	strText += A2W(value_);
}
void xcl_mkValue(CXText& strText, char* value_){
	if (value_)
		strText += A2W(value_);
}
void xcl_mkValue(CXText& strText, const wchar_t value_){
	strText += value_;
}
void xcl_mkValue(CXText& strText, const char value_){
	char  chs[2] = { value_,0 };
	strText += A2W((char*)chs);
}

//--------------
void xcl_mkValue(CXText& strText, const bool value_){
	if(value_)
		strText += L"true";
	else
		strText += L"false";
}
void xcl_mkValue(CXText &strText, const int value_){
	wchar_t make_buf[12] = { 0 };
	_itow_s(value_, make_buf, 10);
	strText += make_buf;
}
void xcl_mkValue(CXText &strText, const UINT value_){
	wchar_t make_buf[12] = { 0 };
	_ultow_s(value_, make_buf, 10);
	strText += make_buf;
}
void xcl_mkValue(CXText &strText, const INT64 value_){
	wchar_t make_buf[24] = { 0 };
	_i64tow_s(value_, make_buf, 24, 10);
	strText += make_buf;
}
void xcl_mkValue(CXText &strText, const UINT64 value_){
	wchar_t make_buf[24] = { 0 };
	_ui64tow_s(value_, make_buf, 24, 10);
	strText += make_buf;
}
void xcl_mkValue(CXText &strText, const long value_){
	wchar_t make_buf[12] = { 0 };
	_itow_s(value_, make_buf, 10);
	strText += make_buf;
}
void xcl_mkValue(CXText& strText, const unsigned long value_){
	wchar_t make_buf[12] = { 0 };
	_itow_s(value_, make_buf, 10);
	strText += make_buf;
}
void xcl_mkValue(CXText &strText, const USHORT value_){
	xcl_mkValue(strText, (int)value_);
}
void xcl_mkValue(CXText &strText, const SHORT value_){
	xcl_mkValue(strText, (int)value_);
}
void xcl_mkValue(CXText &strText, const BYTE value_){
	xcl_mkValue(strText, (int)value_);
}
void xcl_mkValue(CXText &strText, const float value_){
	wchar_t make_buf[32] = { 0 };
	swprintf_s(make_buf, L"%.8g", value_);   //"%.8g":和字母值一样; "%.9g": 和VS显示值一样
	strText += make_buf;
}
void xcl_mkValue(CXText &strText, const double value_){
	wchar_t make_buf[32] = { 0 };
	swprintf_s(make_buf, L"%.15g", value_);   //"%.15g": 和字母值一样; "%.17g": 和VS显示值一样
	strText += make_buf;
}

//----------------------------------------
void xcl_mkValue(CXText& strText, const unsigned int* value_) {
	xcl_mkValue(strText, (void*)value_);
	wchar_t make_buf[32] = { 0 };
	if (value_)
	{
		strText += L"{";
		for (int i = 0; i < 16; i++)
		{
			if (0 == i)
				swprintf_s(make_buf, L"%d", value_[i]);
			else
				swprintf_s(make_buf, L",%d", value_[i]);
			strText += make_buf;
		}
		strText += L",...}";
	}
}
void xcl_mkValue(CXText& strText, int* value_) {
	xcl_mkValue(strText, (const unsigned int*)value_);
}
void xcl_mkValue(CXText& strText, unsigned int* value_) {
	xcl_mkValue(strText, (const unsigned int*)value_);
}

//----------unsigned char----------------
void xcl_mkValue(CXText& strText, const unsigned char* value_) {
	xcl_mkValue(strText, (void*)value_);
	if (value_)
	{
		wchar_t make_buf[32] = { 0 };
		strText += L"{";
		for (int i = 0; i < 16; i++)
		{
			if (0 == i)
				swprintf_s(make_buf, L"%d", value_[i]);
			else
				swprintf_s(make_buf, L",%d", value_[i]);
			strText += make_buf;
		}
		strText += L",...}";
	}
}
void xcl_mkValue(CXText& strText, unsigned char* value_) {
	xcl_mkValue(strText, (const unsigned char*)value_);
}

//---------short------------------------
void xcl_mkValue(CXText& strText, const unsigned short* value_) {
	xcl_mkValue(strText, (void*)value_);
	if (value_)
	{
		wchar_t make_buf[32] = { 0 };
		strText += L"{";
		for (int i = 0; i < 16; i++)
		{
			if (0 == i)
				swprintf_s(make_buf, L"%d", value_[i]);
			else
				swprintf_s(make_buf, L",%d", value_[i]);
			strText += make_buf;
		}
		strText += L",...}";
	}
}
void xcl_mkValue(CXText& strText, short* value_) {
	xcl_mkValue(strText, (const unsigned short*)value_);
}
void xcl_mkValue(CXText& strText, const short* value_) {
	xcl_mkValue(strText, (const unsigned short*)value_);
}
void xcl_mkValue(CXText& strText, unsigned short* value_) {
	xcl_mkValue(strText, (const unsigned short*)value_);
}



/////////////////////////////////////////////////////////////
//----CXTextA------------------------------------------------
/////////////////////////////////////////////////////////////
void xcl_mkValue(CXTextA& strText, const std::wstring& value_) {
	strText += W2A(value_.c_str(), (int)value_.size());
}
void xcl_mkValue(CXTextA& strText, const std::string& value_) {
	strText += value_.c_str();
}

void xcl_mkValue(CXTextA& strText, const CXText& value_) {
	strText += W2A(value_.get(), (int)value_.size());
}
void xcl_mkValue(CXTextA& strText, const CXTextA& value_) {
	strText += value_;
}
void xcl_mkValue(CXTextA& strText, const CXBytes& value_) {
	size_t length = value_.size();
	if (length > 128) length = 128;
	CXTextA  text = xcl_dataToHexTextA(value_.get(), length, TRUE, TRUE);
	strText += "大小:";
	strText += xcl_toTextA(length);
	strText += ", 十六进制字节{";
	strText += text;
	if (value_.size() > 128)
		strText += " ...";
	strText += "}";
}
void xcl_mkValue(CXTextA& strText, void* value_) {
#ifdef _WIN64
	char make_buf[20] = { 0 };
	sprintf_s(make_buf, "0x%016I64X", (INT64)value_);
#else
	char make_buf[12] = { 0 };
	sprintf_s(make_buf, "0x%08X", (unsigned int)value_);
#endif // _WIN64
	strText += make_buf;
}
void xcl_mkValue(CXTextA& strText, const wchar_t* value_) {
	if (value_) strText += W2A(value_);
}
void xcl_mkValue(CXTextA& strText, wchar_t* value_) {
	if (value_) strText += W2A(value_);
}
void xcl_mkValue(CXTextA& strText, const char* value_) {
	if (value_) strText += value_;
}
void xcl_mkValue(CXTextA& strText, char* value_) {
	if (value_) strText += value_;
}
void xcl_mkValue(CXTextA& strText, const wchar_t value_) {
	wchar_t  chs[2] = { value_,0 };
	strText += W2A(chs);
}
void xcl_mkValue(CXTextA& strText, const char value_) {
	strText += value_;
}

//---------------
void xcl_mkValue(CXTextA& strText, bool value_) {
	if (value_)
		strText += "true";
	else
		strText += "false";
}
void xcl_mkValue(CXTextA& strText, int value_) {
	char make_buf[12] = { 0 };
	_itoa_s(value_, make_buf, 10);
	strText += make_buf;
}
void xcl_mkValue(CXTextA& strText, UINT value_) {
	char make_buf[12] = { 0 };
	_ultoa_s(value_, make_buf, 10);
	strText += make_buf;
}
void xcl_mkValue(CXTextA& strText, INT64 value_) {
	char make_buf[24] = { 0 };
	_i64toa_s(value_, make_buf, 24, 10);
	strText += make_buf;
}
void xcl_mkValue(CXTextA& strText, UINT64 value_) {
	char make_buf[24] = { 0 };
	_ui64toa_s(value_, make_buf, 24, 10);
	strText += make_buf;
}
void xcl_mkValue(CXTextA& strText, const long value_) {
	char make_buf[12] = { 0 };
	_itoa_s(value_, make_buf, 10);
	strText += make_buf;
}
void xcl_mkValue(CXTextA& strText, const unsigned long value_) {
	char make_buf[12] = { 0 };
	_itoa_s(value_, make_buf, 10);
	strText += make_buf;
}
void xcl_mkValue(CXTextA& strText, const USHORT value_) {
	xcl_mkValue(strText, (int)value_);
}
void xcl_mkValue(CXTextA& strText, const SHORT value_) {
	xcl_mkValue(strText, (int)value_);
}
void xcl_mkValue(CXTextA& strText, const BYTE value_) {
	xcl_mkValue(strText, (int)value_);
}
void xcl_mkValue(CXTextA& strText, const float value_) {
	char make_buf[32] = { 0 };
	sprintf_s(make_buf, "%.9g", value_);   //"%.8g":和字母值一样; "%.9g": 和VS显示值一样
	strText += make_buf;
}
void xcl_mkValue(CXTextA& strText, const double value_)
{
	char make_buf[32] = { 0 };
	sprintf_s(make_buf, "%.15g", value_);   //"%.15g": 和字母值一样; "%.17g": 和VS显示值一样
	strText += make_buf;
}

#ifdef DEBUG
void xcl_xtrace(const char* pFormat, ...)
{
	char  buf_message[1024] = { 0 };
	va_list pArgs;
	va_start(pArgs, pFormat);
	vsnprintf_s(buf_message, 1024, _TRUNCATE, pFormat, pArgs);
	va_end(pArgs);

	OutputDebugStringA(buf_message);
}
void xcl_xtracew(const wchar_t* pFormat, ...)
{
	wchar_t  buf_message[1024] = { 0 };
	va_list  pArgs;
	va_start(pArgs, pFormat);
	_vsnwprintf_s(buf_message, 1024, _TRUNCATE, pFormat, pArgs);
	va_end(pArgs);
	OutputDebugStringW(buf_message);
}
void xcl_log()
{
}
void xcl_log2()
{
}
#endif
