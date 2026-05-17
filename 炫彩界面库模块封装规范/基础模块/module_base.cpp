#include "module_base.h"
#include <ShlObj.h>
#include <commdlg.h>
#pragma comment(lib, "Comdlg32.lib")

//-------------------------------
CXBytes::CXBytes()
{
	Init();
}

CXBytes::CXBytes(const CXBytes& _that)
{
	if (this == &_that) return;
	Init();

	alloc(_that._length);
	if(_that._length >0)
		memcpy_s(_data, _that._length, _that._data, _that._length);
}

CXBytes::CXBytes(CXBytes&& _that)
{
	if (this == &_that) return;
	_data = _that._data;
	_length = _that._length;
	_that.move();
}
CXBytes::CXBytes(const CXText& _that)
{
	Init();
	size_t size = _that.size() * 2;
	if(size>0)
		set((void*)_that.get(), size);
}
CXBytes::CXBytes(const CXTextA& _that)
{
	Init();
	size_t size = _that.size();
	if (size > 0)
		set((void*)_that.get(), size);
}
CXBytes::CXBytes(const wchar_t* pText)
{
	Init();
	if (pText && *pText)
	{
		size_t size = wcslen(pText) * 2;
		set((void*)pText, size);
	}
}

CXBytes::CXBytes(const char* pText)
{
	Init();
	if (pText && *pText)
	{
		size_t size = strlen(pText);
		set((void*)pText, size);
	}
}

CXBytes::CXBytes(const int value_)
{
	Init();
	addInt(value_);
}

CXBytes::CXBytes(const __int64 value_)
{
	Init();
	addInt64(value_);
}

CXBytes::CXBytes(const BYTE value_)
{
	Init();
	addByte(value_);
}
CXBytes::CXBytes(const BYTE* data, size_t length)
{
	Init();
	set(data, length);
}

CXBytes::CXBytes(const char value_)
{
	Init();
	addByte(value_);
}

CXBytes::CXBytes(const std::initializer_list<BYTE> list)
{
	Init();
	set((void*)list.begin(), list.size());
}
CXBytes::CXBytes(const std::vector<BYTE>& list)
{
	Init();
	set(list.data(), list.size());
}
///---------------------------------------------
void CXBytes::operator=(const CXText& _that)
{
	Init();
	size_t size = _that.size() * 2;
	if(size>0)
		set((void*)_that.get(), size);
}
void CXBytes::operator=(const CXTextA& _that)
{
	Init();

	size_t size = _that.size();
	if(size>0)
		set((void*)_that.get(), size);
}
void CXBytes::operator=(const wchar_t* pText)
{
	Init();
	if (NULL == pText) pText = L"";

	size_t size = wcslen(pText)*2;
	set((void*)pText, size);
}

void CXBytes::operator=(const char* pText)
{
	Init();
	if (NULL == pText) pText = "";

	size_t size = strlen(pText);
	set((void*)pText, size);
}
void CXBytes::operator=(const int value_)
{
	Init();
	addInt(value_);
}
void CXBytes::operator=(const __int64 value_)
{
	Init();
	addInt64(value_);
}

CXBytes::~CXBytes()
{
	if (_data) free(_data);
}

void CXBytes::Init()
{
	_data = NULL;
	_length = 0;
}

void CXBytes::operator=(const CXBytes &_that)
{
	if (_data){
		free(_data);
		_data = NULL;
	}
	alloc(_that._length);
	memcpy_s(_data, _that._length, _that._data, _that._length);
}

void CXBytes::operator=(CXBytes&& _that)
{
	if (_data){
		free(_data);
		_data = NULL;
	}
	_data = _that._data;
	_length = _that._length;
	_that.move();
}

bool CXBytes::operator==(const CXBytes& _that)
{
	if (_length != _that._length)
		return FALSE;

	return 0 == memcmp(_data, _that._data, _length);
}

bool CXBytes::operator!=(const CXBytes& _that)
{
	if (_length != _that._length)
		return TRUE;

	return 0 != memcmp(_data, _that._data, _length);
}

CXBytes CXBytes::operator+(const CXBytes& _that)
{
	CXBytes  bytes = *this;
	bytes.add(_that._data, _that._length, -1);
	return bytes;
}
void CXBytes::operator+=(const CXBytes& _that)
{
	add(_that._data, _that._length, -1);
}

void CXBytes::operator+=(const std::initializer_list<BYTE> list)
{
	add((void*)list.begin(), list.size(), -1);
}

BYTE& CXBytes::operator[](size_t index)
{
#ifdef _DEBUG
	assert(index >= 0 && index < _length);
#endif
	return  _data[index];
}
const BYTE& CXBytes::operator[](size_t index) const
{
#ifdef _DEBUG
	assert(index >= 0 && index < _length);
#endif
	return  _data[index];
}

BYTE* CXBytes::alloc(size_t length)
{
	if (_data) free(_data);
	if (length > 0)
	{
		_data = (BYTE*)malloc(length+2);
		if (_data)
		{
			memset(_data, 0, length);

			_data[length] = 0;
			_data[length + 1] = 0;
			_length = length;
			return _data;
		}
	}
	_length = 0;
	_data = NULL;
	return _data;
}

BYTE* CXBytes::resize(size_t length)
{
	if (_length == length) return _data;
	if (length > 0)
	{
		BYTE* addr = (BYTE*)realloc(_data, length + 2);
		if (NULL == addr) return _data;

		_data = addr;
		_data[length] = 0;
		_data[length + 1] = 0;

		if(length > _length)
			memset(_data + _length, 0, length - _length);
		_length = length;
	} else
	{
		if (_data) free(_data);
		_length = 0;
		_data = NULL;
	}
	return _data;
}

void CXBytes::addSize(size_t length)
{
	if (length <= 0) return ;

	size_t  total_length = _length + length;
	BYTE* data_new = (BYTE*)malloc(total_length+2);
	if (NULL == data_new) return;
	memset(data_new + _length, 0, length);

	if (_length > 0)
	{
		memcpy_s(data_new, total_length, _data, _length);
	}
	data_new[total_length] = 0;
	data_new[total_length + 1] = 0;

	if(_data) //释放旧数据
		free(_data);

	//_bOwner = TRUE;
	_data = data_new;
	_length = total_length;
}

void CXBytes::replace(const void* data, size_t length, size_t off, size_t old_length)
{
	if (NULL == data) return;
	if (off < 0 || off >= _length) return;
	if (length <= 0) return;

	if (-1 == old_length || length == old_length)
	{
		if ((off + length) > _length)  length = _length - off;
		memcpy_s(_data + off, _length - off, data, length);
	} else
	{
		if (old_length <= 0) return;
		size_t last_length = _length - off;
		if (old_length > last_length) old_length = last_length;

		size_t  new_length =_length + (length - old_length); //新的总长度
		BYTE*  new_data = (BYTE*)malloc(new_length);
		if (new_data)
		{
			//复制替换位置前面数据
			if (off > 0)
				memcpy_s(new_data, new_length, _data, off);

			//复制替换数据
			if (length > 0)
				memcpy_s(new_data + off, new_length - off, data, length);

			//复制替换位置后面数据
			memcpy_s(new_data + off + length, new_length - off - length, _data + off + old_length, _length - off - old_length);

			if (_data) free(_data);  //释放旧数据

			_data = new_data;
			_length = new_length;
		}
	}
}

void CXBytes::swap(CXBytes& right)
{
	BYTE* _data_right = right._data;
	size_t  _length_right = right._length;

	right._data = _data;
	right._length = _length;

	_data = _data_right;
	_length = _length_right;
}

void CXBytes::insert(const void* data, size_t length, size_t off)
{
	if (-1 == off) off = _length;
	if (NULL == data || length <= 0 || off < 0) return;
	if (off > _length) off = _length;

	size_t  total_length = _length + length;
	BYTE* data_new = (BYTE*)malloc(total_length + 2);
	if (NULL==data_new)	return;
	
	if (off > 0)
	{
		memcpy_s(data_new, total_length, _data, off);
	}
	memcpy_s(data_new+ off, total_length- off, data, length);
	if((_length - off)>0)
		memcpy_s(data_new + off + length, total_length - off - length, _data + off, _length - off);

	data_new[total_length] = 0;
	data_new[total_length + 1] = 0;

	if (_data) //释放旧数据
		free(_data);

	_data = data_new;
	_length = total_length;
}

void CXBytes::addEnd(const void* data, size_t length)
{
	if (NULL == data || length <= 0) return;
	size_t length_old = _length;
	if (data >= _data && data < _data + _length) //自身范围内复制
	{
		size_t offset = (size_t)((BYTE*)data - _data);
		resize(_length + length);
		memcpy_s(_data + length_old, length, _data + offset, length);
	} else
	{
		addSize(length);
		memcpy_s(_data + length_old, length, data, length);
	}
}

void CXBytes::add(const void* data, size_t length, size_t off)
{
	if (-1 == off || off >= _length) //添加到末尾
		addEnd(data, length);
	else
		insert(data, length, off);
}

void CXBytes::addText(const wchar_t* pText, BOOL bTerminator, size_t off)
{
	if (NULL == pText || 0==*pText) return;
	if (bTerminator)
		add(pText, wcslen(pText) * 2 + 2, off);

	add(pText, wcslen(pText) * 2, off);
}

void CXBytes::addTextA(const char* pText, BOOL bTerminator, size_t off)
{
	if (NULL == pText || 0 == *pText) return;
	if(bTerminator)
		add(pText, strlen(pText) + 1, off);
	else
		add(pText, strlen(pText), off);
}

void CXBytes::addInt(int value_, size_t off)
{
	add(&value_, 4, off);
}

void CXBytes::addInts(std::initializer_list<int> list, size_t off)
{
	add((void*)list.begin(), list.size() * 4, off);
}

void CXBytes::addUInt(UINT value_, size_t off)
{
	add(&value_, 4, off);
}

void CXBytes::addUInts(std::initializer_list<UINT> list, size_t off)
{
	add((void*)list.begin(), list.size()*4, off);
}

void CXBytes::addInt64(__int64 value_, size_t off)
{
	add(&value_, 8, off);
}
void CXBytes::addInt64s(std::initializer_list<__int64> list, size_t off)
{
	add((void*)list.begin(), list.size() * 8, off);
}

void CXBytes::addUInt64(UINT64 value_, size_t off)
{
	add(&value_, 8, off);
}
void CXBytes::addUInt64s(std::initializer_list<UINT64> list, size_t off)
{
	add((void*)list.begin(), list.size() * 8, off);
}

void CXBytes::addShort(short value_, size_t off)
{
	add(&value_, 2, off);
}
void CXBytes::addShorts(std::initializer_list<short> list, size_t off)
{
	add((void*)list.begin(), list.size() * 2, off);
}

void CXBytes::addUShort(unsigned short value_, size_t off)
{
	add(&value_, 2, off);
}
void CXBytes::addUShorts(std::initializer_list<unsigned short> list, size_t off)
{
	add((void*)list.begin(), list.size() * 2, off);
}

void CXBytes::addFloat(float value_, size_t off)
{
	add(&value_, 4, off);
}
void CXBytes::addFloats(std::initializer_list<float> list, size_t off)
{
	add((void*)list.begin(), list.size() * 4, off);
}

void CXBytes::addDouble(double value_, size_t off)
{
	add(&value_, 8, off);
}
void CXBytes::addDoubles(std::initializer_list<double> list, size_t off)
{
	add((void*)list.begin(), list.size() * 8, off);
}

void CXBytes::addWchar(wchar_t value_, size_t off)
{
	add(&value_, 2, off);
}

void CXBytes::addWchars(std::initializer_list<wchar_t> list, size_t off)
{
	add((void*)list.begin(), list.size() * 2, off);
}

void CXBytes::addByte(BYTE value_, size_t off)
{
	add(&value_, 1, off);
}

void CXBytes::addBytes(std::initializer_list<BYTE> list, size_t off)
{
	add((void*)list.begin(), list.size() * 1, off);
}

BYTE CXBytes::hexCharToByte(wchar_t c)
{
	if (c >= L'0' && c <= L'9') return (c - L'0');
	if (c >= L'A' && c <= L'F') return (c - L'A' + 10);
	if (c >= L'a' && c <= L'f') return (c - L'a' + 10);
	return 0;
}

void CXBytes::addHex(const wchar_t* pHex, size_t off)
{
	if (NULL == pHex) return;

	size_t length = wcslen(pHex);
	if (0 == length) return;

	size_t count = length / 2;
	BYTE* data = NULL;
	if (0 != length % 2)
		data = (BYTE*)malloc(count+1);
	else
		data = (BYTE*)malloc(count);
	if (NULL== data) return;

	size_t index = 0;
	BYTE  byte_ = 0;
	for (UINT i = 0; i < count; i++)
	{
		index = i * 2;
		byte_ = hexCharToByte(pHex[index]);
		byte_ = byte_ << 4;
		byte_ |= hexCharToByte(pHex[index + 1]);
		data[i] = byte_;
	}

	if (0 != length % 2)
	{
		index = count * 2;
		byte_ = hexCharToByte(pHex[index]);
		byte_ = byte_ << 4;

		data[count] = byte_;
		add(data, count + 1, off);
	} else
	{
		add(data, count, off);
	}
	free(data);
}

void CXBytes::addVint(vint value_, size_t off)
{
#ifdef _WIN64
	add(&value_, 8, off);
#else
	add(&value_, 4, off);
#endif
}

void CXBytes::addVints(const std::initializer_list<vint> list, size_t off)
{
#ifdef _WIN64
	add((void*)list.begin(), list.size() * 8, off);
#else
	add((void*)list.begin(), list.size() * 4, off);
#endif
}

CXText CXBytes::getText(size_t off, size_t wcharLength)
{
	CXText  text;
	size_t len = std::min(_length - off, wcharLength *2) / 2;
	if(len>0)
		text.append2((const wchar_t*)(_data+ off), len);
	return text;
}

CXTextA CXBytes::getTextA(size_t off, size_t charLength)
{
	CXTextA  text;
	size_t len = std::min(_length - off, charLength);
	if (len > 0)
		text.append2((const char*)(_data + off), len);
	return text;
}

int CXBytes::getInt(size_t off)
{
	if (_length < 4) return 0;

	if (off > (_length - 4))
		return 0;
	if (off < 0) off = 0;

	return *(int*)(_data+ off);
}

__int64 CXBytes::getInt64(size_t off)
{
	if (_length < 8) return 0;

	if (off > (_length - 8))
		return 0;
	if (off < 0) off = 0;

	return *(__int64*)(_data + off);
}

UINT CXBytes::getUInt(size_t off)
{
	if (_length < 4) return 0;

	if (off > (_length - 4))
		return 0;
	if (off < 0) off = 0;

	return *(UINT*)(_data + off);
}

UINT64 CXBytes::getUInt64(size_t off)
{
	if (_length < 8) return 0;

	if (off > (_length - 8))
		return 0;
	if (off < 0) off = 0;

	return *(UINT64*)(_data + off);
}

short CXBytes::getShort(size_t off)
{
	if (_length < 2) return 0;

	if (off > (_length - 2))
		return 0;
	if (off < 0) off = 0;

	return *(short*)(_data + off);
}

unsigned short CXBytes::getUShort(size_t off)
{
	if (_length < 2) return 0;

	if (off > (_length - 2))
		return 0;
	if (off < 0) off = 0;

	return *(unsigned short*)(_data + off);
}

float CXBytes::getFloat(size_t off)
{
	if (_length < 4) return 0;

	if (off > (_length - 4))
		return 0;
	if (off < 0) off = 0;

	return *(float*)(_data + off);
}

double CXBytes::getDouble(size_t off)
{
	if (_length < 8) return 0;

	if (off > (_length - 8))
		return 0;
	if (off < 0) off = 0;

	return *(double*)(_data + off);
}

wchar_t CXBytes::getWchar(size_t off)
{
	if (_length < 1) return 0;

	if (off > (_length - 1))
		return 0;
	if (off < 0) off = 0;

	return *(wchar_t*)(_data + off);
}

BYTE CXBytes::getByte(size_t off)
{
	if (_length < 1) return 0;

	if (off > (_length - 1))
		return 0;
	if (off < 0) off = 0;

	return *(BYTE*)(_data + off);
}

void CXBytes::clear()
{
	if (_data) free(_data);
	
	_length = 0;
	_data = NULL;
}

void CXBytes::set(const void* data, size_t length)
{
	alloc(length);
	if(length>0)
		memcpy_s(_data, _length, data, length);
}

void CXBytes::setText(const wchar_t* pText)
{
	if (NULL == pText) return ;

	size_t length = wcslen(pText)*2;
	alloc(length);
	if (length > 0)
		memcpy_s(_data, _length, pText, length);
}

void CXBytes::setTextA(const char* pText)
{
	if (NULL == pText) return;
	size_t length = strlen(pText);
	alloc(length);
	if (length > 0)
		memcpy_s(_data, _length, pText, length);
}

void CXBytes::setHexText(const wchar_t* pHex)
{
	clear();
	addHex(pHex);
}

CXBytes CXBytes::getLeft(size_t length)
{
	CXBytes bytes;
	if (length > 0)
	{
		size_t size = std::min(_length, length);
		if (size > 0)
			bytes.set(_data, size);
	}
	return bytes;
}

CXBytes CXBytes::getRight(size_t length)
{
	CXBytes bytes;
	if (length > 0)
	{
		size_t size = length;
		if (length > _length)
			size = _length;

		if (size > 0)
			bytes.set(_data + (_length - size), size);
	}
	return bytes;
}

CXBytes CXBytes::getMid(size_t iStart, size_t length)
{
	CXBytes bytes;
	if (iStart >= _length) return bytes;
	if (length > 0)
	{
		size_t size = length;
		if (size > (_length - iStart))
			size = _length - iStart;

		if (size > 0)
			bytes.set(_data + iStart, size);
	}
	return bytes;
}

CXBytes CXBytes::toHexBytes(BOOL bUpper, BOOL bSplit, size_t off, size_t length)
{
	CXBytes bytes;
	if (_length > 0)
	{
		if (0==length)  length = _length;
		size_t lengthDest = _length- off;
		size_t lengthSrc = std::min(lengthDest, length);

		if (bSplit)
			lengthDest = lengthSrc * 3 - 1;
		else
			lengthDest = lengthSrc * 2;

		bytes.alloc(lengthDest *2);
		_dataToHexStrW((wchar_t*)bytes.get(), lengthDest +1, (BYTE*)(_data+ off), lengthSrc, bUpper, bSplit);
	}
	return bytes;
}

CXBytes CXBytes::toHexBytesA(BOOL bUpper, BOOL bSplit, size_t off, size_t length)
{
	CXBytes bytes;
	if (_length > 0)
	{
		if (0 == length)  length = _length;
		size_t lengthDest = _length - off;
		size_t lengthSrc = std::min(lengthDest, length);

		if (bSplit)
			lengthDest = lengthSrc * 3 - 1;
		else
			lengthDest = lengthSrc * 2;
		bytes.alloc(lengthDest);
		_dataToHexStrA((BYTE*)bytes.get(), bytes.size() + 1, (BYTE*)(_data+ off), lengthSrc, bUpper, bSplit);
	}
	return bytes;
}

CXText CXBytes::toHex(BOOL bUpper, BOOL bSplit, size_t off, size_t length)
{
	CXText  text;
	if (_length > 0)
	{
		if (0 == length)  length = _length;
		size_t lengthDest = _length - off;
		size_t lengthSrc = std::min(lengthDest, length);

		if (bSplit)
			lengthDest = lengthSrc * 3 - 1;
		else
			lengthDest = lengthSrc * 2;

		text.resize(lengthDest);
		_dataToHexStrW((wchar_t*)text.get(), lengthDest + 1, (BYTE*)(_data+ off), lengthSrc, bUpper, bSplit);
	}
	return text;
}

CXTextA CXBytes::toHexA(BOOL bUpper, BOOL bSplit, size_t off, size_t length)
{
	CXTextA  text;
	if (_length > 0)
	{
		if (0 == length)  length = _length;
		size_t lengthDest = _length - off;
		size_t lengthSrc = std::min(lengthDest, length);

		if (bSplit)
			lengthDest = lengthSrc * 3 - 1;
		else
			lengthDest = lengthSrc * 2;
		text.resize(lengthDest);
		_dataToHexStrA((BYTE*)text.get(), lengthDest +1, (BYTE*)(_data+ off), lengthSrc, bUpper, bSplit);
	}
	return text;
}

size_t  CXBytes::findBytes(CXBytes& dest, size_t off)
{
	if (off < 0) return -1; // 如果开始位置索引小于0，则直接返回-1

	size_t srcSize = _length;
	size_t destSize = dest.size();

	// 如果目标字节集大小为0或者源字节集大小小于目标字节集大小，则直接返回-1
	if (destSize == 0 || srcSize < destSize) {
		return -1;
	}
	const BYTE* data_dest = dest.getPtr();
	// 遍历源字节集，寻找目标字节集
	size_t count = srcSize - destSize;
	for (size_t i = off; i <= count; ++i) {
		bool found = true;
		for (size_t j = 0; j < destSize; ++j) {
			if (_data[i + j] != data_dest[j]) {
				found = false;
				break;
			}
		}
		if (found) {
			return i; // 找到目标字节集，返回位置索引
		}
	}
	return -1; // 未找到目标字节集，返回-1
}

//----------------------------------------------------
CXText::CXText(const CXTextA& _that)
{
	xcl_mkValue(*this, _that);
}
CXText::CXText(const CXBytes &_that)
{
	size_t size = _that.size()/2;
	if (size > 0)
	{
		_text.resize(size);
		wmemcpy_s((wchar_t*)_text.data(), size, (wchar_t*)_that.get(), size);
	}
}
CXText::CXText(const std::string& string_)
{
	xcl_mkValue(*this, string_);
}
CXText::CXText(const wchar_t* pText, size_t length)
{
	if (pText)
	{
		if(-1== length)
			_text = pText;
		else
			_text = pText;
	}
}
CXText::CXText(const char* pText, size_t length)
{
	if (pText) *this = A2W(pText, (int)length);
}
CXText::CXText(const int value_)
{
	xcl_mkValue(*this, value_);
}
CXText::CXText(const INT64 value_)
{
	xcl_mkValue(*this, value_);
}
CXText::CXText(const double value_)
{
	xcl_mkValue(*this, value_);
}
CXText::CXText(const float value_)
{
	xcl_mkValue(*this, value_);
}
CXText::CXText(const char value_)
{
	xcl_mkValue(*this, value_);
}
CXText::CXText(const wchar_t value_)
{
	xcl_mkValue(*this, value_);
}

void CXText::operator=(const char* pText)
{
	_text.clear();
	xcl_mkValue(*this, pText);
}
void CXText::operator=(const std::string& string_)
{
	_text.clear();
	xcl_mkValue(*this, string_);
}
void CXText::operator=(const CXBytes &_that)
{
	size_t size = _that.size()/2;
	if (size > 0)
	{
		_text.resize(size);
		wmemcpy_s((wchar_t*)_text.data(), size, (wchar_t*)_that.get(), size);
	} else
	{
		_text.clear();
	}
}
void CXText::operator=(const CXTextA &_that)
{
	_text.clear();
	xcl_mkValue(*this, _that);
}

CXText CXText::operator+(const int value_) const 
{
	CXText ret;
	xcl_mkValue(ret, value_);
	return *this;
}
void CXText::replaceSub(__in const wchar_t* pSubStr, __in const wchar_t* pReplaceStr)
{
	size_t off = 0;
	size_t subStrLen = wcslen(pSubStr);
	size_t replaceStrLen = wcslen(pReplaceStr);

	while ((off = _text.find(pSubStr, off)) != std::wstring::npos)
	{
		_text.replace(off, subStrLen, pReplaceStr);
		off += replaceStrLen;
	}
}
CXText CXText::substr(size_t off, size_t count) const
{
	CXText retText;
	const wchar_t* p = _text.c_str();
	size_t length = _text.size();
	if ((off + count) > length)
		count = length - off;
	if (count > 0)
		retText.append2(p + off, count);
	return retText;
}

//-------------------------------------
CXTextA::CXTextA(const CXText& _that)
{
	xcl_mkValue(*this, _that);
}
CXTextA::CXTextA(const std::wstring& wstring_)
{
	xcl_mkValue(*this, wstring_);
}

CXTextA::CXTextA(const CXBytes &_that)
{
	size_t size = _that.size();
	if (size > 0)
	{
		_text.resize(size);
		memcpy_s((void*)_text.data(), size, _that.get(), size);
	}
}
CXTextA::CXTextA(const char* pText, size_t length)
{ 
	if (pText)
	{
		if (-1 == length)
			_text = pText;
		else
			_text.append(pText, length);
	}
}
CXTextA::CXTextA(const wchar_t* pText, size_t length)
{
	if(pText) *this = W2A(pText, (int)length);
}
CXTextA::CXTextA(const int value_)
{
	xcl_mkValue(*this, value_);
}
CXTextA::CXTextA(const INT64 value_)
{
	xcl_mkValue(*this, value_);
}
CXTextA::CXTextA(const double value_)
{
	xcl_mkValue(*this, value_);
}
CXTextA::CXTextA(const float value_)
{
	xcl_mkValue(*this, value_);
}
CXTextA::CXTextA(const char value_)
{
	xcl_mkValue(*this, value_);
}
CXTextA::CXTextA(const wchar_t value_)
{
	xcl_mkValue(*this, value_);
}

void CXTextA::operator=(const wchar_t* pText)
{
	_text.clear();
	xcl_mkValue(*this, pText);
}
void CXTextA::operator=(const std::wstring& wstring_)
{
	_text.clear();
	xcl_mkValue(*this, wstring_);
}
void CXTextA::operator=(const CXText& _that)
{
	_text.clear();
	xcl_mkValue(*this, _that);
}
void CXTextA::operator=(const CXBytes &_that)
{
	size_t size = _that.size();
	if (size > 0)
	{
		_text.resize(size);
		memcpy_s((void*)_text.data(), size, _that.get(), size);
	} else
	{
		_text.clear();
	}
}
//GBK中文是2节，次字节范围是 0x40~0xFE，可能与 ASCII 符号重叠，直接字节查找可能误判。需要按字符步进：
void CXTextA::replaceSub(__in const char* pSubStr, __in const char* pReplaceStr)
{
	size_t subStrLen = strlen(pSubStr);
	size_t replaceStrLen = strlen(pReplaceStr);
	size_t off = 0;
	while (off < _text.size())
	{
		if (_text.compare(off, subStrLen, pSubStr) == 0)
		{
			_text.replace(off, subStrLen, pReplaceStr);
			off += replaceStrLen;
		} else
		{
			// GBK双字节首字节则跳2，否则跳1
			unsigned char c = (unsigned char)_text[off];
			off += (c >= 0x81 && c <= 0xFE) ? 2 : 1;
		}
	}
}
CXTextA CXTextA::substr(size_t off, size_t count) const
{
	CXTextA retText;
	const char* p = _text.c_str();
	size_t length = _text.size();
	if ((off + count) > length)
		count = length - off;
	if (count > 0)
		retText.append2(p + off, count);
	return retText;
}

//--------------------------------------------------------------
void xcl_setSrand(int seed)
{
	srand(seed);
}

int  xcl_getRand(int minv, int maxv)
{
	int v = maxv - minv + 1;
	int r = rand() % v + minv;
	return r;
}

//--------------------------------------------------------------
CXTextA xcl_dataToBinTextA(const void* data, int nLen, int nAlignByte, BOOL bSplit, BOOL bFillZero)
{
	CXTextA text;  // 最终输出的二进制文本

	if (!data || nLen <= 0 || nAlignByte <= 0)
		return text;  // 错误检查，空数据或无效长度时返回空

	const BYTE* bytes = static_cast<const BYTE*>(data);  // 将数据转换为字节指针

	// 从高位字节开始遍历：即倒序遍历字节数据，确保大端序输出
	for (int i = 0; i < nLen; i += nAlignByte) {
		if (i > 0 && bSplit) {
			if (1 == nAlignByte)
				text += " ";
			else
				text += ", ";  // 在每个对齐字节组之间添加空格
		}

		// 从高位到低位，倒序处理每个字节
		for (int j = nAlignByte - 1; j >= 0; --j) {
			if (i + j < nLen) {
				BYTE currentByte = bytes[i + j];  // 获取当前处理的字节

				// 逐位处理当前字节的每一位
				int bit = 7;
				if (FALSE == bFillZero)
				{
					for (; bit >= 0; --bit)
					{
						if ((currentByte >> bit) & 1)
						{
							break;
						}
					}
				}
				if (bit >= 0)
				{
					for (; bit >= 0; --bit) {
						text += ((currentByte >> bit) & 1) ? '1' : '0';  // 输出每一位 (1或0)
					}
				} else
				{
					text += '0';
				}

				if (bSplit && j > 0) {
					text += ' ';  // 在每个字节之间添加空格
				}
			}
		}
	}
	return text;
}

CXText xcl_dataToBinText(const void* data, int nLen, int nAlignByte, BOOL bSplit, BOOL bFillZero)
{
	CXText text;  // 最终输出的二进制文本

	if (!data || nLen <= 0 || nAlignByte <= 0)
		return text;  // 错误检查，空数据或无效长度时返回空

	const BYTE* bytes = static_cast<const BYTE*>(data);  // 将数据转换为字节指针

	// 从高位字节开始遍历：即倒序遍历字节数据，确保大端序输出
	for (int i = 0; i < nLen; i += nAlignByte) {
		if (i > 0 && bSplit) {
			if(1== nAlignByte)
				text += L" ";
			else
				text += L", ";  // 在每个对齐字节组之间添加空格
		}

		// 从高位到低位，倒序处理每个字节
		for (int j = nAlignByte - 1; j >= 0; --j) {
			if (i + j < nLen) {
				BYTE currentByte = bytes[i + j];  // 获取当前处理的字节

				// 逐位处理当前字节的每一位
				int bit = 7;
				if (FALSE == bFillZero)
				{
					for (; bit >= 0; --bit)
					{
						if ((currentByte >> bit) & 1)
						{
							break;
						}
					}
				}
				if (bit >= 0)
				{
					for (; bit >= 0; --bit) {
						text += ((currentByte >> bit) & 1) ? L'1' : L'0';  // 输出每一位 (1或0)
					}
				} else
				{
					text += L'0';
				}

				if (bSplit && j > 0) {
					text += L' ';  // 在每个字节之间添加空格
				}
			}
		}
	}
	return text;
}

int  xcl_hexTextToInt(const wchar_t* pHexText)
{
	return std::wcstol(pHexText, nullptr, 16);
}
__int64  xcl_hexTextToInt64(const wchar_t* pHexText)
{
	return std::wcstoll(pHexText, nullptr, 16);
}

int  xcl_binTextToInt(const wchar_t* pHexText)
{
	return std::wcstol(pHexText, nullptr, 2);
}
__int64 xcl_binTextToInt64(const wchar_t* pHexText)
{
	return std::wcstoll(pHexText, nullptr, 2);
}

const wchar_t* text_findRight(const wchar_t* str, const wchar_t* strSearch)
{
	if (NULL == str || NULL == strSearch) return NULL;
	int len = (int)wcslen(strSearch);
	if (len <= 0) return NULL;

	const wchar_t* p = str;
	while (*p)
		p++;

	const wchar_t* pStart = NULL;
	while (p >= str)
	{
		if (*strSearch == *p)
		{
			if (0 == wmemcmp(p, strSearch, (size_t)len))
			{
				return p;
			}
		}
		p--;
	}
	return NULL;
}

const char* text_findRightA(const char* str, const char* strSearch)
{
	if (NULL == str || NULL == strSearch) return NULL;
	int len = (int)strlen(strSearch);
	if (len <= 0) return NULL;

	const char* p = str;
	while (*p)
		p++;

	const char* pStart = NULL;
	while (p >= str)
	{
		if (*strSearch == *p)
		{
			if (0 == memcmp(p, strSearch, (size_t)len))
			{
				return p;
			}
		}
		p--;
	}
	return NULL;
}

CXText text_insert(const wchar_t* pText, size_t off, const wchar_t* pSubText)
{
	CXText  retText = pText;
	retText.insert(off, pSubText);
	return retText;
}

CXTextA text_insertA(const char* pText, size_t off, const char* pSubText)
{
	if (NULL == pText || 0 == *pText) return "";
	if (NULL == pSubText || 0 == *pSubText) return "";

	CXTextA  retText = pText;
	retText.insert(off, pSubText);
	return retText;
}

CXText text_replace(const wchar_t* pText, const wchar_t* pSrc, const wchar_t* pDest)
{
	if (NULL == pText || 0 == *pText) return L"";
	if (NULL == pSrc || 0 == *pSrc) return L"";
	if (NULL == pDest) return L"";

	CXText  retText;
	std::wstring*  str = retText.getString();

	UINT len = (UINT)wcslen(pSrc);
	const wchar_t* p = pText;
	const wchar_t* pStart = p;
	while (*p)
	{
		p = wcsstr(p, pSrc);
		if (p)
		{
			str->append(pStart, p- pStart);
			str->append(pDest);
			p += len;
			pStart = p;
		} else
		{
			str->append(pStart);
			break;
		}
	}
	return retText;
}

CXTextA text_replaceA(const char* pText, const char* pSrc, const char* pDest)
{
	if (NULL == pText || 0 == *pText) return "";
	if (NULL == pSrc || 0 == *pSrc) return "";
	if (NULL == pDest) return "";

	CXTextA  retText;
	std::string* str = retText.getString();

	//因为strstr 是按字节查找, 有的字符是1个字节, 导致错位,所以不用strstr()
	UINT len = (UINT)strlen(pSrc);
	const char* p = pText;
	const char* pStart = p;
	while (*p)
	{
		// GBK边界安全匹配
		if (memcmp(p, pSrc, len) == 0)
		{
			str->append(pStart, p - pStart);
			str->append(pDest);
			p += len;
			pStart = p;
		} else
		{
			unsigned char c = (unsigned char)*p;
			p += (c >= 0x81 && c <= 0xFE) ? 2 : 1;
		}
	}
	// 追加剩余部分
	str->append(pStart);
	return retText;
}

CXText  text_getLeft(const wchar_t* pText, UINT length)
{
	if (NULL== pText || 0==*pText) return L"";

	CXText  retText;
	UINT len = 0;
	const wchar_t* p = pText;
	while (*p)
	{
		retText += *p;
		len++;
		if(len>=length)
			break;
		p++;
	}
	return retText;
}

CXTextA  text_getLeftA(const char* pText, UINT length)
{
	if (NULL == pText || 0 == *pText) return "";

	CXTextA  retText;
	UINT len = 0;
	const char* p = pText;
	while (*p)
	{
		retText += *p;
		len++;
		if (len >= length)
			break;
		p++;
	}
	return retText;
}

CXText  text_getRight(const wchar_t* pText, UINT length)
{
	if (NULL == pText || 0 == *pText) return L"";

	__int64 off = (__int64)wcslen(pText) - length;
	if (off < 0) off = 0;

	return (pText + off);
}

CXTextA  text_getRightA(const char* pText, UINT length)
{
	if (NULL == pText || 0 == *pText) return "";

	__int64 off = (__int64)strlen(pText) - length;
	if (off < 0) off = 0;

	return (pText + off);
}

CXText  text_getSub(const wchar_t* pText, UINT off1, UINT off2)
{
	if (NULL == pText || 0 == *pText) return L"";

	CXText  retText;

	UINT len = 0;
	const wchar_t* p = pText;
	while (*p)
	{
		len++;
		if(len>= off2)
			break;
		p++;
	}
	if (len >= off2)
		retText.append2(pText + off1, off2 - off1);
	return retText;
}

CXTextA  text_getSubA(const char* pText, UINT off1, UINT off2)
{
	if (NULL == pText || 0 == *pText) return "";

	CXTextA  retText;

	UINT len = 0;
	const char* p = pText;
	while (*p)
	{
		len++;
		if (len >= off2)
			break;
		p++;
	}
	if (len >= off2)
		retText.append2(pText + off1, off2 - off1);
	return retText;
}

CXText  text_getSub2(const wchar_t* pText, UINT off, UINT count)
{
	if (NULL == pText || 0 == *pText) return L"";
	CXText  retText;

	UINT len = 0;
	UINT off2 = off + count;
	const wchar_t* p = pText;
	while (*p)
	{
		len++;
		if (len >= off2)
			break;
		p++;
	}
	if (len >= off2)
		retText.append2(pText + off, count);
	return retText;
}

CXTextA  text_getSub2A(const char* pText, UINT off, UINT count)
{
	if (NULL == pText || 0 == *pText) return "";
	CXTextA  retText;

	UINT len = 0;
	UINT off2 = off + count;
	const char* p = pText;
	while (*p)
	{
		len++;
		if (len >= off2)
			break;
		p++;
	}
	if (len >= off2)
		retText.append2(pText + off, count);
	return retText;
}

CXText  text_getSub3(const wchar_t* pText, const wchar_t* pBegin, const wchar_t* pEnd)
{
	if (NULL == pText || 0 == *pText) return L"";
	if (NULL == pBegin || NULL == pEnd) return L"";
	//if(pBegin<pText || pBegin>pEnd) return L"";

	const wchar_t* p = wcsstr(pText, pBegin);
	if (p)
	{
		p = p + wcslen(pBegin);
		const wchar_t* p2 = wcsstr(p, pEnd);
		if (p2 && (p2 - p)>0)
		{
			CXText  retText;
			retText.append2(p, (UINT)(p2 - p));
			return retText;
		}
	}
	return L"";
}

CXTextA  text_getSub3A(const char* pText, const char* pBegin, const char* pEnd)
{
	if (NULL == pText || 0 == *pText) return "";
	if (NULL == pBegin || NULL == pEnd) return "";
	//if (pBegin<pText || pBegin>pEnd) return "";

	const char* p = strstr(pText, pBegin);
	if (p)
	{
		p = p + strlen(pBegin);
		const char* p2 = strstr(p, pEnd);
		if (p2 && (p2 - p) > 0)
		{
			CXTextA  retText;
			retText.append2(p, (UINT)(p2 - p));
			return retText;
		}
	}
	return "";
}

int  text_getSubPos(const wchar_t* pText, const wchar_t* pSubText)
{
	if (NULL == pText || 0 == *pText) return EOF;
	if (NULL == pSubText || 0 == *pSubText) return EOF;

	const wchar_t* p = wcsstr(pText, pSubText);
	if (p)
		return  (int)(p - pText);
	return  EOF;
}

int  text_getSubPosA(const char* pText, const char* pSubText)
{
	if (NULL == pText || 0 == *pText) return EOF;
	if (NULL == pSubText || 0 == *pSubText) return EOF;

	const char* p = strstr(pText, pSubText);
	if (p)
		return  (int)(p - pText);
	return  EOF;
}

int text_getSubPosRight(const wchar_t* pText, const wchar_t* pSubText)
{
	if (NULL == pText || NULL == pSubText) return NULL;
	int len = (int)wcslen(pSubText);
	if (len <= 0) return NULL;

	const wchar_t* p = pText;
	while (*p)
		p++;

	const wchar_t* pStart = NULL;
	while (p >= pText)
	{
		if (*pSubText == *p)
		{
			if (0 == wmemcmp(p, pSubText, (size_t)len))
			{
				return (int)(p - pText);
			}
		}
		p--;
	}
	return -1;
}

int text_getSubPosRightA(const char* pText, const char* pSubText)
{
	if (NULL == pText || NULL == pSubText) return NULL;
	int len = (int)strlen(pSubText);
	if (len <= 0) return NULL;

	const char* p = pText;
	while (*p)
		p++;

	const char* pStart = NULL;
	while (p >= pText)
	{
		if (*pSubText == *p)
		{
			if (0 == memcmp(p, pSubText, (size_t)len))
			{
				return (int)(p - pText);
			}
		}
		p--;
	}
	return -1;
}

CXVector<CXText> text_split(const wchar_t* pText, const wchar_t* pSplit)
{
	CXVector<CXText> list;
	if (NULL == pText || NULL == pSplit) return list;
	if (0 == *pText || 0 == *pSplit) return list;

	UINT len = (UINT)wcslen(pSplit);
	const wchar_t* p = pText;
	const wchar_t* pStart = p;
	while (*p)
	{
		p = wcsstr(p, pSplit);
		if (p)
		{
			if ((p - pStart) > 0)
			{
				list.add(L"");
				CXText& text = list.back();
				text.append2(pStart, (UINT)(p - pStart));
			}else if ((p - pStart) == 0)
			{
				list.add(L"");
			}
			p += len;
			pStart = p;
		} else
		{
			if (*pStart)
			{
				list.add(L"");
				CXText& text = list.back();
				text.append(pStart);
			}
			break;
		}
	}
	return list;
}

CXVector<CXTextA>  text_splitA(const char* pText, const char* pSplit)
{
	CXVector<CXTextA> list;
	if (NULL == pText || NULL == pSplit) return list;
	if (0 == *pText || 0 == *pSplit) return list;

	UINT len = (UINT)strlen(pSplit);
	const char* p = pText;
	const char* pStart = p;
	while (*p)
	{
		p = strstr(p, pSplit);
		if (p)
		{
			if ((p - pStart) > 0)
			{
				list.add("");
				CXTextA& text = list.back();

				text.append2(pStart, (UINT)(p - pStart));
			} else if ((p - pStart) == 0)
			{
				list.add(L"");
			}
			p += len;
			pStart = p;
		} else
		{
			if (*pStart)
			{
				list.add("");
				CXTextA& text = list.back();
				text.append(pStart);
			}
			break;
		}
	}
	return list;
}

CXVector<CXText> text_split2(const wchar_t* pText, const wchar_t* pSplit)
{
	CXVector<CXText> list;
	if (NULL == pText || NULL == pSplit) return list;
	if (0 == *pText || 0 == *pSplit) return list;

	UINT len = (UINT)wcslen(pSplit);
	const wchar_t* p = text_skipSpaceWrap(pText);
	const wchar_t* pStart = p;
	while (*p)
	{
		p = wcsstr(p, pSplit);
		if (p)
		{
			const wchar_t* pEnd = text_skipSpaceWrapRight2(pStart, p);
			if (pEnd > pStart)
			{
				list.add(L"");
				CXText& text = list.back();
				text.append2(pStart, (UINT)(pEnd - pStart));
			} else if (pEnd == pStart)
			{
				list.add("");
			}
			p += len;
			pStart = text_skipSpaceWrap(p);
		} else
		{
			const wchar_t* pEnd = text_skipSpaceWrapRight(pStart);
			if (pEnd > pStart)
			{
				list.add(L"");
				CXText& text = list.back();
				text.append2(pStart, (UINT)(pEnd - pStart));
			}
			break;
		}
	}
	return list;
}

CXVector<CXTextA> text_split2A(const char* pText, const char* pSplit)
{
	CXVector<CXTextA> list;
	if (NULL == pText || NULL == pSplit) return list;
	if (0 == *pText || 0 == *pSplit) return list;

	UINT len = (UINT)strlen(pSplit);
	const char* p = text_skipSpaceWrapA(pText);
	const char* pStart = p;
	while (*p)
	{
		p = strstr(p, pSplit);
		if (p)
		{
			const char* pEnd = text_skipSpaceWrapRight2A(pStart, p);
			if (pEnd > pStart)
			{
				list.add("");
				CXTextA& text = list.back();
				text.append2(pStart, (UINT)(pEnd - pStart));
			} else if (pEnd == pStart)
			{
				list.add("");
			}
			p += len;
			pStart = text_skipSpaceWrapA(p);
		} else
		{
			const char* pEnd = text_skipSpaceWrapRightA(pStart);
			if (pEnd > pStart)
			{
				list.add("");
				CXTextA& text = list.back();
				text.append2(pStart, (UINT)(pEnd - pStart));
			}
			break;
		}
	}
	return list;
}

CXVector<CXText> text_splitChar(const wchar_t* pText, const wchar_t splitChar)
{
	CXVector<CXText> list;
	if (NULL == pText || 0==*pText) return list;

	const wchar_t* p = pText;
	const wchar_t* pStart = p;
	while (*p)
	{
		if (splitChar == *p)
		{
			list.add(L"");
			CXText& text = list.back();

			text.append2(pStart, (UINT)(p - pStart));
			p++;
			pStart = p;
		} else
		{
			p++;
		}
	}
	if (p > pStart)
	{
		list.add(L"");
		CXText& text = list.back();
		text.append(pStart);
	}
	return list;
}

CXVector<CXTextA>  text_splitCharA(const char* pText, const char splitChar)
{
	CXVector<CXTextA> list;
	if (NULL == pText || 0==*pText) return list;

	const char* p = pText;
	const char* pStart = p;
	while (*p)
	{
		if (splitChar == *p)
		{
			list.add("");
			CXTextA& text = list.back();

			text.append2(pStart, (UINT)(p - pStart));
			p++;
			pStart = p;
		} else
		{
			p++;
		}
	}
	if (p > pStart)
	{
		list.add("");
		CXTextA& text = list.back();
		text.append(pStart);
	}
	return list;
}

CXVector<CXText> text_splitChar2(const wchar_t* pText, const wchar_t splitChar)
{
	CXVector<CXText> list;
	if (NULL == pText || 0==*pText) return list;

	const wchar_t* p = text_skipSpaceWrap(pText);
	const wchar_t* pStart = p;
	while (*p)
	{
		if (splitChar == *p)
		{
			const wchar_t* pEnd = text_skipSpaceWrapRight2(pStart, p);
			if (pEnd > pStart)
			{
				list.add(L"");
				CXText& text = list.back();
				text.append2(pStart, (UINT)(pEnd - pStart));
			}
			p++;
			pStart = text_skipSpaceWrap(p);
		} else
		{
			p++;
		}
	}
	if (p > pStart)
	{
		const wchar_t* pEnd = text_skipSpaceWrapRight(pStart);
		if (pEnd > pStart)
		{
			list.add(L"");
			CXText& text = list.back();
			text.append2(pStart, (UINT)(pEnd - pStart));
		}
	}
	return list;
}

CXVector<CXTextA> text_splitChar2A(const char* pText, const char splitChar)
{
	CXVector<CXTextA> list;
	if (NULL == pText || 0==*pText) return list;

	const char* p = text_skipSpaceWrapA(pText);
	const char* pStart = p;
	while (*p)
	{
		if (splitChar == *p)
		{
			const char* pEnd = text_skipSpaceWrapRight2A(pStart, p);
			if (pEnd > pStart)
			{
				list.add("");
				CXTextA& text = list.back();
				text.append2(pStart, (UINT)(pEnd - pStart));
			}
			p++;
			pStart = text_skipSpaceWrapA(p);
		} else
		{
			p++;
		}
	}
	if (p > pStart)
	{
		const char* pEnd = text_skipSpaceWrapRightA(pStart);
		if (pEnd > pStart)
		{
			list.add("");
			CXTextA& text = list.back();
			text.append2(pStart, (UINT)(pEnd - pStart));
		}
	}
	return list;
}

CXVector<CXText> text_splitRow(const wchar_t* pText)
{
	CXVector<CXText> list;
	if (NULL == pText || 0 == *pText) return list;

	const wchar_t* p = pText;
	const wchar_t* pStart = p;
	while (*p)
	{
		if (L'\r' == *p || L'\n' == *p)
		{
			list.add(L"");
			CXText& text = list.back();

			text.append2(pStart, (UINT)(p - pStart));
			if (L'\r' == *p) p++;
			if (L'\n' == *p) p++;
			pStart = p;
		} else
		{
			p++;
		}
	}
	if (p > pStart)
	{
		list.add(L"");
		CXText& text = list.back();
		text.append(pStart);
	}
	return list;
}

CXVector<CXTextA> text_splitRowA(const char* pText)
{
	CXVector<CXTextA> list;
	if (NULL == pText || 0 == *pText) return list;

	const char* p = pText;
	const char* pStart = p;
	while (*p)
	{
		if (L'\r' == *p || L'\n' == *p)
		{
			list.add("");
			CXTextA& text = list.back();

			text.append2(pStart, (UINT)(p - pStart));
			if (L'\r' == *p) p++;
			if (L'\n' == *p) p++;
			pStart = p;
		} else
		{
			p++;
		}
	}
	if (p > pStart)
	{
		list.add("");
		CXTextA& text = list.back();
		text.append(pStart);
	}
	return list;
}

CXVector<CXText> text_splitRow2(const wchar_t* pText)
{
	CXVector<CXText> list;
	if (NULL == pText || 0 == *pText) return list;

	const wchar_t* p = text_skipSpaceWrap(pText);
	const wchar_t* pStart = p;
	while (*p)
	{
		if (L'\r' == *p || L'\n' == *p)
		{
			const wchar_t* pEnd = text_skipSpaceWrapRight2(pStart, p);
			if (pEnd > pStart)
			{
				list.add(L"");
				CXText& text = list.back();
				text.append2(pStart, (UINT)(pEnd - pStart));
			}
			if (L'\r' == *p) p++;
			if (L'\n' == *p) p++;
			pStart = text_skipSpaceWrap(p);
		} else
		{
			p++;
		}
	}
	if (p > pStart)
	{
		const wchar_t* pEnd = text_skipSpaceWrapRight(pStart);
		if (pEnd > pStart)
		{
			list.add(L"");
			CXText& text = list.back();
			text.append2(pStart, (UINT)(pEnd - pStart));
		}
	}
	return list;
}

CXVector<CXTextA> text_splitRow2A(const char* pText)
{
	CXVector<CXTextA> list;
	if (NULL == pText || 0 == *pText) return list;
	const char* p = text_skipSpaceWrapA(pText);
	const char* pStart = p;
	while (*p)
	{
		if (L'\r' == *p || L'\n' == *p)
		{
			const char* pEnd = text_skipSpaceWrapRight2A(pStart, p);
			if (pEnd > pStart)
			{
				list.add("");
				CXTextA& text = list.back();
				text.append2(pStart, (UINT)(pEnd - pStart));
			}
			if (L'\r' == *p) p++;
			if (L'\n' == *p) p++;
			pStart = text_skipSpaceWrapA(p);
		} else
		{
			p++;
		}
	}
	if (p > pStart)
	{
		const char* pEnd = text_skipSpaceWrapRightA(pStart);
		if (pEnd > pStart)
		{
			list.add("");
			CXTextA& text = list.back();
			text.append2(pStart, (UINT)(pEnd - pStart));
		}
	}
	return list;
}

CXText text_trim(const wchar_t* pText)
{
	if (NULL == pText || 0 == *pText) return L"";

	CXText retText;
	const wchar_t* p = text_skipSpaceWrap(pText);
	wchar_t* pEnd = (wchar_t*)text_skipSpaceWrapRight(p);
	if (p && pEnd)
	{
		retText.append2(p, (UINT)(pEnd - p));
	}
	return retText;
}

CXTextA text_trimA(const char* pText)
{
	if (NULL == pText || 0 == *pText) return "";

	CXTextA retText;
	const char* p = text_skipSpaceWrapA(pText);
	char* pEnd = (char*)text_skipSpaceWrapRightA(p);
	if (p && pEnd)
	{
		retText.append2(p, (UINT)(pEnd - p));
	}
	return retText;
}

const wchar_t* text_trim2(const wchar_t* pText)
{
	if (NULL == pText || 0 == *pText) return L"";

	const wchar_t* p = text_skipSpaceWrap(pText);
	wchar_t* pEnd = (wchar_t*)text_skipSpaceWrapRight(p);
	if (pEnd)
		*pEnd = 0;
	return p;
}

const char* text_trim2A(const char* pText)
{
	if (NULL == pText || 0 == *pText) return "";

	const char* p = text_skipSpaceWrapA(pText);
	char* pEnd = (char*)text_skipSpaceWrapRightA(p);
	if (pEnd)
		*pEnd = 0;
	return p;
}

CXText text_trimLeft(const wchar_t* pText)
{
	return  text_skipSpaceWrap(pText);
}

CXTextA text_trimLeftA(const char* pText)
{
	return  text_skipSpaceWrapA(pText);
}

CXText text_trimRight(const wchar_t* pText)
{
	CXText retText;
	if (NULL == pText || 0==pText) return retText;

	const wchar_t* pEnd =text_skipSpaceWrapRight(pText);
	if (pEnd)
		retText.append2(pText, (UINT)(pEnd - pText));
	return retText;
}

CXTextA text_trimRightA(const char* pText)
{
	if (NULL == pText || 0 == *pText) return "";
	CXTextA retText;

	const char* pEnd = text_skipSpaceWrapRightA(pText);
	if (pEnd)
		retText.append2(pText, (UINT)(pEnd - pText));
	return retText;
}

void text_trimRight2(const wchar_t* pText)
{
	if (NULL == pText || 0 == *pText) return ;

	wchar_t* pEnd = (wchar_t*)text_skipSpaceWrapRight(pText);
	if (pEnd)
		*pEnd = 0;
}

void text_trimRight2A(const char* pText)
{
	if (NULL == pText || 0 == *pText) return;

	char* pEnd = (char*)text_skipSpaceWrapRightA(pText);
	if (pEnd)
		*pEnd = 0;
}

//@别名  文本删除空行(文本地址)
CXText text_trimEmptyRow(const wchar_t* pText)
{
	CXText retText;
	if (NULL == pText || 0 == *pText) return retText;

	const wchar_t* p = pText;
	const wchar_t* pStart = p;
	while (*p)
	{
		if ('\r' == *p || '\n' == *p)
		{
			retText.append2(pStart, (int)(p-pStart));
			while (*p)
			{
				if ('\r' == *p || '\n' == *p)
					p++;
				else
					break;
			}
			pStart = p;

			if(*pStart) //末尾还有内容, 加上换行符
				retText += L"\n";
		} else
		{
			p++;
		}
	}
	if (*pStart)
	{
		retText.append(pStart);
	}
	return retText;
}

CXTextA text_trimEmptyRowA(const char* pText)
{
	CXTextA retText;
	if (NULL == pText || 0 == *pText) return retText;

	const char* p = pText;
	const char* pStart = p;
	while (*p)
	{
		if ('\r' == *p || '\n' == *p)
		{
			retText.append2(pStart, (int)(p - pStart));
			while (*p)
			{
				if ('\r' == *p || '\n' == *p)
					p++;
				else
					break;
			}
			pStart = p;

			if (*pStart) //末尾还有内容, 加上换行符
				retText += "\n";
		} else
		{
			p++;
		}
	}
	if (*pStart)
	{
		retText.append(pStart);
	}
	return retText;
}

//文本到大写(文本地址)
template<class T> void text_upperT(T* pText)
{
	if (NULL == pText || 0 == *pText) return ;

	T* p = pText;
	while (*p)
	{
		if (*p >= L'a' && *p <= L'z')
			*p = (*p) - 32;
		p++;
	}
}

void text_upper(__in_out wchar_t* pText) {
	text_upperT(pText); 
}

void text_upperA(__in_out char* pText) {
	text_upperT(pText);
}

CXText text_upper2(__in const wchar_t* pText)
{
	CXText retText = pText ? pText : L"";
	text_upperT((wchar_t*)retText.get());
	return retText;
}
CXTextA text_upper2A(__in const char* pText)
{
	CXTextA retText = pText ? pText : "";
	text_upperT((char*)retText.get());
	return retText;
}

//文本到小写(文本地址)
template<class T> void text_lowerT(T* pText){
	if (NULL == pText || 0 == *pText) return ;

	T* p = pText;
	while (*p)
	{
		if (*p >= L'A' && *p <= L'Z')
			*p = (*p) + 32;
		p++;
	}
}

void text_lower(__in_out wchar_t* pText) {
	text_lowerT(pText);
}

void text_lowerA(__in_out char* pText) {
	text_lowerT(pText);
}

CXText text_lower2(__in const wchar_t* pText)
{
	CXText retText = pText ? pText : L"";
	text_lowerT((wchar_t*)retText.get());
	return retText;
}

CXTextA text_lower2A(__in const char* pText)
{
	CXTextA retText = pText ? pText : "";
	text_lowerT((char*)retText.get());
	return retText;
}

CXText text_getRandChar(int length)
{
	const wchar_t* arr = L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	srand((UINT)time(NULL));
	std::wstring  retText;
	for (int i = 0; i < length; i++)
	{
		int  index = xcl_getRand(0, 61);
		retText += arr[index];
	}
	return retText;
}

CXTextA text_getRandCharA(int length)
{
	const char* arr = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	srand((UINT)time(NULL));
	std::string  retText;
	for (int i = 0; i < length; i++)
	{
		int  index = xcl_getRand(0, 61);
		retText += arr[index];
	}
	return retText;
}

CXText text_getRandNumber(int length)
{
	const wchar_t* arr = L"0123456789";
	srand((UINT)time(NULL));
	std::wstring  retText;
	for (int i = 0; i < length; i++)
	{
		int  index = xcl_getRand(0, 9);
		retText += arr[index];
	}
	return retText;
}

CXTextA text_getRandNumberA(int length)
{
	const char* arr = "0123456789";
	srand((UINT)time(NULL));
	std::string  retText;
	for (int i = 0; i < length; i++)
	{
		int  index = xcl_getRand(0, 9);
		retText += arr[index];
	}
	return retText;
}

CXText text_getReText(const wchar_t* pText, int count)
{
	if (count <= 0 || NULL == pText || 0 == *pText)
		return L"";

	CXText retText;
	for (int i = 0; i < count; i++)
		retText += pText;
	return retText;
}
CXTextA text_getReTextA(const char* pText, int count)
{
	if (count <= 0 || NULL == pText || 0 == *pText)
		return "";

	CXTextA retText;
	for (int i = 0; i < count; i++)
		retText += pText;
	return retText;
}

//-------------------------------------
template<class T> const T* text_skipSpaceT(const T* pIn)
{
	if (NULL == pIn) return NULL;
	const T* p = pIn;
	while (*p)
	{
		if (L' ' == *p || L'\t' == *p)
			p++;
		else
			break;
	}
	return p;
}
const wchar_t* text_skipSpace(const wchar_t* pText) {
	return text_skipSpaceT(pText);
}

const char* text_skipSpaceA(const char* pText) {
	return text_skipSpaceT(pText);
}

template<class T> const T* text_skipSpace2T(const T* pIn, const T* pEnd)
{
	if (NULL == pIn) return NULL;
	const T* p = pIn;
	while (p < pEnd)
	{
		if (L' ' == *p || L'\t' == *p)
			p++;
		else
			break;
	}
	return p;
}

const wchar_t* text_skipSpace2(const wchar_t* pText, const wchar_t* pEnd) {
	return text_skipSpace2T(pText, pEnd);
}

const char* text_skipSpace2A(const char* pText, const char* pEnd) {
	return text_skipSpace2T(pText, pEnd);
}

template<class T> const T* text_skipSpaceWrapT(const T* pIn)
{
	if (NULL == pIn) return NULL;
	const T* p = pIn;
	while (*p)
	{
		if (L' ' == *p || L'\t' == *p || L'\r' == *p || L'\n' == *p)
			p++;
		else
			break;
	}
	return p;
}

const wchar_t* text_skipSpaceWrap(const wchar_t* pText) {
	return text_skipSpaceWrapT(pText);
}

const char* text_skipSpaceWrapA(const char* pText) {
	return text_skipSpaceWrapT(pText);
}

template<class T> const T* text_skipSpaceWrapRightT(const T* pIn)
{
	const T* p = pIn;
	const T* pBegin = pIn;

	while (*p)
		p++;

	while (p > pBegin)
	{
		p--;
		if (L' ' == *p || L'\t' == *p || L'\r' == *p || L'\n' == *p)
		{
		} else
		{
			p++;
			break;
		}
	}
	return p;
}

const wchar_t* text_skipSpaceWrapRight(const wchar_t* pText) {
	return text_skipSpaceWrapRightT(pText);
}

const char* text_skipSpaceWrapRightA(const char* pText) {
	return text_skipSpaceWrapRightT(pText);
}

template<class T> const T* text_skipSpaceWrapRight2T(const T* pBegin, const T* pEnd)
{
	const T* p = pEnd;
	while (p > pBegin)
	{
		p--;
		if (L' ' == *p || L'\t' == *p || L'\r' == *p || L'\n' == *p)
		{
		} else
		{
			p++;
			break;
		}
	}
	return p;
}

const wchar_t* text_skipSpaceWrapRight2(const wchar_t* pText, const wchar_t* pEnd) {
	return text_skipSpaceWrapRight2T(pText, pEnd);
}

const char* text_skipSpaceWrapRight2A(const char* pText, const char* pEnd) {
	return text_skipSpaceWrapRight2T(pText, pEnd);
}

template<class T> const T* text_skipEndT(const T* pText)
{
	if (NULL == pText) return NULL;
	const T* p = pText;
	while (*p)
	{
		if (L'\r' == *p || L'\n' == *p)
			break;
		p++;
	}
	return p;
}

const wchar_t* text_skipEnd(const wchar_t* pText) {
	return text_skipEndT(pText);
}

const char* text_skipEndA(const char* pText) {
	return text_skipEndT(pText);
}

template<class T> const T* text_skipNextT(const T* pText)
{
	if (NULL == pText) return NULL;
	const T* p = pText;
	while (*p)
	{
		if (L'\n' == *p)
		{
			p++;
			break;
		}
		p++;
	}
	return p;
}

const wchar_t* text_skipNext(const wchar_t* pText) {
	return text_skipNextT(pText);
}

const char* text_skipNextA(const char* pText) {
	return text_skipNextT(pText);
}

//--------------------------------------
CXFile::CXFile()
{
	m_pFile = NULL;
	m_length = 0;
}

CXFile::~CXFile()
{
	if (m_pFile) fclose(m_pFile);
}

BOOL CXFile::open(const wchar_t* pFileName)
{
	if (m_pFile) {
		xcl_log(L"请先关闭文件");
		return FALSE;
	}
	int err = _wfopen_s(&m_pFile, pFileName, L"ab+");
	if (0 == err)
	{
		m_length = getLength();
		return TRUE;
	}
	return FALSE;
}

//打开文件, 只读模式
BOOL CXFile::openRead(const wchar_t* pFileName)
{
	if (m_pFile) {
		xcl_log(L"请先关闭文件");
		return FALSE;
	}
	if (0 == _wfopen_s(&m_pFile, pFileName, L"rb"))
	{
		m_length = getLength();
		return TRUE;
	}
	return FALSE;
}

//打开一个空的文件进行读写。如果该文件存在，其内容将被销毁。
BOOL CXFile::openWrite(const wchar_t* pFileName)
{
	if (m_pFile) {
		xcl_log(L"请先关闭文件");
		return FALSE;
	}
	if (0 == _wfopen_s(&m_pFile, pFileName, L"wb+"))
	{
		m_length = getLength();
		return TRUE;
	}
	return FALSE;
}

//打开文件, 指定打开方式
BOOL CXFile::openEx(const wchar_t* pFileName, const wchar_t* pMode)
{
	if (m_pFile) {
		xcl_log(L"请先关闭文件");
		return FALSE;
	}
	if (0 == _wfopen_s(&m_pFile, pFileName, pMode))
	{
		m_length = getLength();
		return TRUE;
	}
	return FALSE;
}

void CXFile::close()
{
	if (m_pFile) {
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

FILE* CXFile::getFilePtr()
{
	return m_pFile;
}

size_t  CXFile::getLength()
{
	size_t length = 0;
	if (m_pFile)
	{
		size_t old = (size_t)_ftelli64(m_pFile);

		_fseeki64(m_pFile, 0, SEEK_END);
		length = (size_t)_ftelli64(m_pFile);
		_fseeki64(m_pFile, old, SEEK_SET);
	}
	return length;
}

size_t  CXFile::getLength_temp()
{
	return m_length;
}

size_t CXFile::getCurPos()
{
	if (m_pFile)
		return (size_t)_ftelli64(m_pFile);
	return 0;
}

BOOL CXFile::seekBegin()
{
	if (m_pFile && 0 == _fseeki64(m_pFile, 0, SEEK_SET))
		return TRUE;
	return FALSE;
}

BOOL CXFile::seekEnd()
{
	if (m_pFile && 0 == _fseeki64(m_pFile, 0, SEEK_END))
		return TRUE;
	return FALSE;
}

BOOL CXFile::seekPos(size_t offset, int origin)
{
	if (m_pFile && 0 == _fseeki64(m_pFile, offset, origin))
		return TRUE;
	return FALSE;
}

//返回实际读取长度
size_t CXFile::readData(__out void *buf, __in size_t nBufSize)
{
	if (m_pFile)
	{
		size_t size = getLength();
		if (nBufSize > size)
			nBufSize = size;
		return fread(buf, 1, nBufSize, m_pFile);
	}
	return 0;
}

BOOL CXFile::readDataRow(__out char* buf, __in int nBufSize)
{
	if (m_pFile && fgets(buf, nBufSize, m_pFile))
		return TRUE;
	return FALSE;
}

CXBytes CXFile::readDataBytes(size_t nLength)
{
	CXBytes  bytes;
	if (m_pFile)
	{
		if (-1 == nLength)
			nLength = getLength();
		if (nLength > 0)
		{
			const BYTE* data = bytes.alloc(nLength);
			size_t ret = fread((void*)data, 1, nLength, m_pFile);
		}
	}
	return bytes;
}

CXTextA CXFile::readTextA(size_t nLength)
{
	CXTextA  ret;
	if (m_pFile)
	{
		size_t size = getLength();
		if (size > 0)
		{
			ret.resize(size);
			fread((wchar_t*)ret.getPtr(), 1, size, m_pFile);
			return ret;
		}
	}
	return 0;
}

char CXFile::readChar()
{
	if(m_pFile)
		return fgetc(m_pFile);
	return 0;
}

wchar_t CXFile::readWChar()
{
	if(m_pFile)
		return fgetwc(m_pFile);
	return 0;
}

size_t CXFile::writeData(const void* buf, size_t nLength)
{
	if(m_pFile)
		return fwrite(buf, 1, nLength, m_pFile);
	return 0;
}

size_t CXFile::writeText(const wchar_t* data, size_t nLength)
{
	if (m_pFile)
	{
		if (-1 == nLength)  nLength = wcslen(data);
		return fwrite(data, 2, nLength, m_pFile);
	}
	return 0;
}

size_t CXFile::writeTextA(const char* data, size_t nLength)
{
	if (m_pFile)
	{
		if (-1 == nLength)  nLength = strlen(data);
		return fwrite(data, 1, nLength, m_pFile);
	}
	return 0;
}

size_t CXFile::writeText2(const CXText& text, size_t nLength)
{
	if (m_pFile && !text.empty())
	{
		if (-1 == nLength) nLength = text.size();
		return fwrite(text.get(), 2, nLength, m_pFile);
	}
	return 0;
}

size_t CXFile::writeText2A(const CXTextA& text, size_t nLength)
{
	if (m_pFile && !text.empty())
	{
		if (-1 == nLength) nLength = text.size();
		return fwrite(text.get(), 1, nLength, m_pFile);
	}
	return 0;
}

BOOL CXFile::isEnd()
{
	if (m_pFile)
	{
		getLength();
		if (ftell(m_pFile) == m_length)
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CXFile::isEof()
{
	if(m_pFile && feof(m_pFile))
		return  TRUE;
	return FALSE;
}

//------------------------------------------------
CXVector<CXText> xcl_getCommandLine()
{
	CXVector<CXText> list_out;
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv != NULL)
	{
		for (int i = 0; i < argc; i++)
		{
			list_out.add(argv[i]);
		}
		// 释放内存
		LocalFree(argv);
	}
	return list_out;
}

CXText xcl_getRunDir()
{
	wchar_t  buf[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buf, MAX_PATH);
	wchar_t*  p = wcsrchr(buf, L'\\');
	if(p)  *p = 0;
	return buf;
}

CXText xcl_getCurrentDir()
{
	wchar_t  buf[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, buf);
	return buf;
}

CXText xcl_getRunFileFullPath()
{
	wchar_t  buf[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buf, MAX_PATH);
	return buf;
}

CXText xcl_getRunFileName()
{
	wchar_t  buf[MAX_PATH] = { 0 };
	if (GetModuleFileName(NULL, buf, MAX_PATH))
	{
		return PathFindFileName(buf);
	}
	return L"";
}

BOOL xcl_runConsoleProgram(const wchar_t* command, CXText* stdOutput, CXText* stdError, DWORD* exitCode)
{
	if (NULL == command || 0 == *command) return FALSE;

	std::wstring  command_ = command;

	// 设置用于执行命令的安全属性
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	// 创建管道用于读取进程的输出
	HANDLE hStdOut_rd = NULL, hStdOut_wr = NULL;
	HANDLE hStdErr_rd = NULL, hStdErr_wr = NULL;

	if (!CreatePipe(&hStdOut_rd, &hStdOut_wr, &sa, 0) || !CreatePipe(&hStdErr_rd, &hStdErr_wr, &sa, 0)) {
		return FALSE;
	}

	// 设置句柄不继承属性
	if (!SetHandleInformation(hStdOut_rd, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(hStdErr_rd, HANDLE_FLAG_INHERIT, 0)) {
		CloseHandle(hStdOut_rd);
		CloseHandle(hStdErr_rd);
		CloseHandle(hStdOut_wr);
		CloseHandle(hStdErr_wr);
		return FALSE;
	}

	// 创建进程信息结构
	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);
	si.hStdError = hStdErr_wr;
	si.hStdOutput = hStdOut_wr;
	si.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	// 创建进程
	if (!CreateProcessW(NULL, (wchar_t*)command_.c_str(), NULL, NULL, TRUE,
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		CloseHandle(hStdOut_rd);
		CloseHandle(hStdErr_rd);
		CloseHandle(hStdOut_wr);
		CloseHandle(hStdErr_wr);
		return FALSE;
	}

	// 关闭写端句柄
	CloseHandle(hStdOut_wr);
	CloseHandle(hStdErr_wr);

	// 读取输出
	const int buffer_size = 4096;
	char buffer[buffer_size];
	DWORD bytesRead;

	// 读取标准输出
	if (stdOutput)
	{
		while (ReadFile(hStdOut_rd, buffer, buffer_size - 1, &bytesRead, NULL) && bytesRead != 0) {
			buffer[bytesRead] = '\0'; // 确保字符串结尾
			(*stdOutput) += A2W(buffer);
		}
	}

	// 读取标准错误
	if (stdError)
	{
		while (ReadFile(hStdErr_rd, buffer, buffer_size - 1, &bytesRead, NULL) && bytesRead != 0) {
			buffer[bytesRead] = '\0'; // 确保字符串结尾
			(*stdError) += A2W(buffer);
		}
	}

	// 等待进程结束
	WaitForSingleObject(pi.hProcess, INFINITE);

	// 获取退出代码
	if(exitCode)
		GetExitCodeProcess(pi.hProcess, exitCode);

	// 关闭句柄
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hStdOut_rd);
	CloseHandle(hStdErr_rd);

	return TRUE;
}

CXText xcl_getEnvironmentVariable(const wchar_t* pName)
{
	wchar_t  buf[4096] = { 0 };
	GetEnvironmentVariable(pName, buf, 4096);
	return buf;
}

BOOL xcl_getEnvironmentStringsToMap(__out CXMap<CXText, CXText>& map_out)
{
	const wchar_t* lpvEnv = GetEnvironmentStrings();
	if (lpvEnv == NULL)
		return FALSE;

	CXText varName;
	const wchar_t* lpszVariable = lpvEnv;
	while (*lpszVariable)
	{
		const wchar_t* p = wcschr(lpszVariable, '=');
		if (p && p != lpszVariable)
		{
			varName = L"";
			varName.append2(lpszVariable, (int)(p - lpszVariable));
			if('='==*p)
				p++;
			map_out[varName] = p;
		}
		lpszVariable += lstrlen(lpszVariable) + 1;
	}
	FreeEnvironmentStrings((wchar_t*)lpvEnv);
	if (map_out.empty())
		return FALSE;
	return TRUE;
}

BOOL xcl_is64()
{
	return (sizeof(vint) == (vint)8);
}

CXText xcl_readConfigItem(const wchar_t* pFileName, const wchar_t* pNodeName, const wchar_t* pItemName, const wchar_t* pDefaultText)
{
	wchar_t valueBuffer[1024] = {0};
	DWORD bytesRead = GetPrivateProfileStringW(pNodeName, pItemName, pDefaultText, valueBuffer, 1024, pFileName);
	return valueBuffer;
}

BOOL xcl_writeConfigItem(const wchar_t* pFileName, const wchar_t* pNodeName, const wchar_t* pItemName, const wchar_t* pValue)
{
	return WritePrivateProfileStringW(pNodeName, pItemName, pValue, pFileName);
}
BOOL xcl_deleteConfigItem(const wchar_t* pFileName, const wchar_t* pNodeName, const wchar_t* pItemName)
{
	return WritePrivateProfileString(pNodeName, pItemName, NULL, pFileName);// 向指定键写入空字符串实现删除
}
CXVector<CXText> xcl_getConfigSectionNames(const wchar_t* pFileName)
{
	CXVector<CXText> retArray;
	if (NULL == pFileName || 0 == *pFileName)
		return retArray;

	const int bufferSize = 32767;
	wchar_t sectionNames[bufferSize];
	DWORD bytesRead = GetPrivateProfileSectionNames(sectionNames, bufferSize, pFileName);
	if (bytesRead > 0)
	{
		// 解析 sectionNames 并存储到一个字符串数组
		const wchar_t* sectionNamePtr = sectionNames;
		while (*sectionNamePtr)
		{
			retArray.add(sectionNamePtr);
			sectionNamePtr += wcslen(sectionNamePtr) + 1;
		}
	} else
	{
		return {};
	}
	return retArray;
}

BOOL xcl_deleteConfigSection(const wchar_t* pFileName, const wchar_t* pSectionName)
{
	return WritePrivateProfileString(pSectionName, NULL, NULL, pFileName); // 向节中的NULL键写入空字符串实现删除整个节
}

//----------------------------------------------
void  xcl_msgBox(const wchar_t* pText, const wchar_t* pCaption)
{
	if(NULL==pCaption || 0==*pCaption)
		MessageBox(NULL, pText, L"提示", 0);
	else
		MessageBox(NULL, pText, pCaption,0);
}

void xcl_doMessage()
{
	MSG Msg;
	while (PeekMessage(&Msg, NULL, NULL, NULL, PM_NOREMOVE))
	{
		if (Msg.message == WM_QUIT)
			break;
		if (GetMessage(&Msg, NULL, 0, 0))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
}

void xcl_delay(UINT delay)
{
	UINT64  time_start = GetTickCount64();
	MSG Msg;
	while (1)
	{
		if (PeekMessage(&Msg, NULL, NULL, NULL, PM_NOREMOVE))
		{
			if (Msg.message == WM_QUIT)
				break;
			if (GetMessage(&Msg, NULL, 0, 0))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		} else
		{
			Sleep(10);
		}
		if ((GetTickCount() - time_start) >= delay)
			return;
	}
}

int xcl_getScreenWidth()// 取屏幕宽度()
{
	return GetSystemMetrics(SM_CXSCREEN);
}

int xcl_getScreenHeight()//取屏幕高度()
{
	return GetSystemMetrics(SM_CYSCREEN);
}

//------------------------------------------------
UINT xcl_writeFile(const wchar_t* pFileName, const void* data, int length)
{
	CXFile file_;
	if (file_.openWrite(pFileName))
	{
		size_t ret = file_.writeData((void*)data, (size_t)length);
		file_.close();
		return (UINT)ret;
	}
	return 0;
}

CXBytes xcl_readFile(const wchar_t* pFileName, int length)
{
	CXFile file_;
	CXBytes ret;
	if (file_.openRead(pFileName)) {
		ret = file_.readDataBytes(length);
		file_.close();
	}
	return ret;
}

CXTextA xcl_readFileTextA(const wchar_t* pFileName, int length)
{
	CXTextA ret;
	CXFile file_;
	if (file_.openRead(pFileName)) {
		ret = file_.readTextA(length);
		file_.close();
	}
	return ret;
}

int xcl_createFolder(const wchar_t* pPath)
{
	if (0 == _wmkdir(pPath))
		return 0;
	if (EEXIST == errno)
		return -2;
	return -1;
}

int xcl_createFolderA(const char* pPath)
{
	if (0 == _mkdir(pPath))
		return 0;
	if (EEXIST == errno)
		return -2;
	return -1;
}

BOOL xcl_createFolder2(const wchar_t*  pPath)
{
	if (NULL == pPath)return FALSE;
	wchar_t fullPath[MAX_PATH] = { 0 };
	wcscpy_s(fullPath, MAX_PATH, pPath);

	int len = (int)wcslen(fullPath);
	if (!(L'\\' == fullPath[len - 1] || L'/' == fullPath[len - 1]))
	{
		const wchar_t*   pEnd = wcsrchr(fullPath, L'\\');
		if (NULL == pEnd) pEnd = wcsrchr(fullPath, L'/');
		if (NULL == pEnd) return FALSE;

		pEnd = wcschr(pEnd, L'.');
		if (NULL == pEnd)  //末尾是文件夹
		{
			if ((len + 1) >= MAX_PATH) return FALSE;
			fullPath[len] = L'\\';
			fullPath[len + 1] = 0;
		}
	}

	const wchar_t* p = wcsstr(fullPath, L":\\");
	if (NULL == p) p = wcsstr(fullPath, L"://");

	if (NULL == p) return FALSE;
	if (p) p += 2;

	wchar_t buf[MAX_PATH] = { 0 };
	while (*p)
	{
		if (L'\\' == *p || L'/' == *p)
		{
			int len = (int)(p - fullPath);
			if (len < 1 || len >= MAX_PATH)
				return FALSE;
			wmemcpy_s(buf, MAX_PATH, fullPath, len);
			if (0 != _waccess_s(buf, 0))
			{
				if (-1 == _wmkdir(buf))
				{
					if (EEXIST != errno)
					{
						return FALSE;
					}
				}
			}
		}
		p++;
	}
	return TRUE;
}

// 判断文件夹是否存在
BOOL xcl_isFolderExist(const wchar_t* pDir)
{
	DWORD dwAttrib = GetFileAttributes(pDir);
	return INVALID_FILE_ATTRIBUTES != dwAttrib && 0 != (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL xcl_copyFile(const wchar_t* dest, const wchar_t*  src)
{
	wchar_t  dir[MAX_PATH] = { 0 };
	wmemcpy_s(dir, MAX_PATH, dest, wcslen(dest));
	//PathRemoveFileSpec(dir); 
	if (xcl_createFolder2(dir))
		return ::CopyFile(src, dest, TRUE);
	return FALSE;
}

BOOL xcl_isFileExist(const wchar_t* pFileName)
{
	DWORD attributes = GetFileAttributesW(pFileName);
	return INVALID_FILE_ATTRIBUTES != attributes && 0 == (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL xcl_isFileExist2(const wchar_t* pFileName)
{
	if (0 == _waccess_s(pFileName, 0))
		return TRUE;
	return FALSE;
}

// 判断文件或文件夹是否存在
BOOL xcl_isPathExist(const wchar_t* pPath)
{
	DWORD dwAttrib = GetFileAttributes(pPath);
	return INVALID_FILE_ATTRIBUTES != dwAttrib;
}

BOOL xcl_deleteFolder(const wchar_t* path)
{
	std::wstring dir(path);
	//在目录后面加上"\\*.*"进行第一次搜索
	std::wstring newDir = dir + L"\\*.*";
	//用于查找的句柄
	intptr_t handle;
	struct _wfinddata_t fileinfo;
	//第一次查找
	handle = _wfindfirst(newDir.c_str(), &fileinfo);
	if (handle == -1)
	{
		return FALSE;
	}
	do
	{
		if (fileinfo.attrib & _A_SUBDIR)
		{
			//如果为文件夹，加上文件夹路径，再次遍历
			if (wcscmp(fileinfo.name, L".") == 0 || wcscmp(fileinfo.name, L"..") == 0)
			{
				continue;
			}
			// 在目录后面加上"\\"和搜索到的目录名进行下一次搜索
			newDir = dir + L"\\" + fileinfo.name;
			//先遍历删除文件夹下的文件，再删除空的文件夹
			xcl_deleteFolder(newDir.c_str());
			//删除文件夹
			//_wrmdir(newDir.c_str());
		} else
		{
			std::wstring file_path = dir + L"\\" + fileinfo.name;
			//删除文件
			_wremove(file_path.c_str());
		}
	} while (!_wfindnext(handle, &fileinfo));
	_findclose(handle);
	if(0==_wrmdir(path))
		return TRUE;
	return FALSE;
}

//-------------------------------------------------
CXTextA W2A(const wchar_t* pString, int length)
{
	CXTextA  text;
	if (NULL == pString || 0 == *pString) return text;
	if (-1 == length) length = (int)wcslen(pString);
	if (length <= 0) return text;

	int nNeedLen = WideCharToMultiByte(CP_ACP, NULL, pString, length, NULL, 0, NULL, NULL);
	if (nNeedLen > 0)
	{	
		text.resize(nNeedLen);
		WideCharToMultiByte(CP_ACP, NULL, pString, nNeedLen, (char*)text.get(), nNeedLen, NULL, NULL);
	}
	return text;
}

// const char* W2A_temp(const wchar_t* pString, int length)
// {
// 	__g_convTemp[0] = 0;
// 	if (pString)
// 	{
// 		int len = WideCharToMultiByte(CP_ACP, NULL, pString, length, __g_convTemp, __MAX_TEXT_1024 - 1, NULL, NULL);
// 		if (len <= 0)
// 			return "";
// 	}
// 	return __g_convTemp;
// }

CXText  A2W(const char* pString, int length)
{
	CXText  text;
	if (NULL == pString || 0 == *pString) return text;
	if (-1 == length) length = (int)strlen(pString);
	if (length <= 0) return text;

	int nNeedLen = MultiByteToWideChar(CP_ACP, 0, pString, length, NULL,0 );
	if (nNeedLen > 0)
	{
		text.resize(nNeedLen);
		MultiByteToWideChar(CP_ACP, 0, pString, length, (wchar_t*)text.get(), nNeedLen);
	}
	return text;
}

// const wchar_t* A2W_temp(const char* pString, int length)
// {
// 	__g_convTempW[0] = 0;
// 	if (pString)
// 	{
// 		int len = MultiByteToWideChar(CP_ACP, 0, pString, length, __g_convTempW, __MAX_TEXT_1024 - 1);
// 		if (len <= 0)
// 			return L"";
// 	}
// 	return __g_convTempW;
// }

//UNICODE 到 UTF8 字节集
CXTextA W2U(const wchar_t* pString, int length)
{
	CXTextA  text;
	if (NULL == pString || 0 == *pString) return text;
	if (-1 == length) length = (int)wcslen(pString);
	if (length <= 0) return text;

	int nNeedLen = WideCharToMultiByte(CP_UTF8, NULL, pString, length, NULL, 0, NULL, NULL);
	if (nNeedLen > 0)
	{
		text.resize(nNeedLen);
		nNeedLen = WideCharToMultiByte(CP_UTF8, NULL, pString, length, (char*)text.get(), nNeedLen, NULL, NULL);
	}
	return text;
}

//UTF8 到 UNICODE
CXText U2W(const char* utf8, int length)
{
	CXText  text;
	if (NULL == utf8 || 0 == *utf8) return text;
	if (NULL == utf8) return text;
	if (-1 == length) length = (int)strlen(utf8);
	if (length <= 0) return text;

	int nNeedLen = MultiByteToWideChar(CP_UTF8, 0, utf8, length, NULL, 0);
	if (nNeedLen > 0)
	{
		text.resize(nNeedLen);
		nNeedLen = MultiByteToWideChar(CP_UTF8, 0, utf8, length, (wchar_t*)text.get(), nNeedLen);
	}
	return text;
}

CXTextA  A2U(const char* pString, int length)
{
	if (NULL == pString || 0 == *pString) return "";
	if (-1 == length) length = (int)strlen(pString);

	CXText text = A2W(pString, length);
	CXTextA textA = W2U(text.get(), (int)text.size());
	return textA;
}

CXTextA U2A(const char* utf8, int length)
{
	if (NULL == utf8 || 0==*utf8) return "";
	if (-1 == length) length = (int)strlen(utf8);

	CXText text = U2W(utf8, length);
	CXTextA textA = W2A(text.get(), (int)text.size());
	return textA;
}

CXTextA W2A(const CXText& text, int length)
{
	return W2A(text.get(), length);
}
CXText A2W(const CXTextA& text, int length)
{
	return A2W(text.get(), length);
}
CXTextA W2U(const CXText& text, int length)
{
	return W2U(text.get(), length);
}
CXText U2W(const CXTextA& utf8, int length)
{
	return U2W(utf8.get(), length);
}
CXTextA A2U(const CXTextA& text, int length)
{
	return A2U(text.get(), length);
}
CXTextA U2A(const CXTextA& utf8, int length)
{
	return U2A(utf8.get(), length);
}

CXTextA xcl_base64EncodeA(const char* bytes_to_encode_, unsigned int in_len)
{
	std::string ret;
	if (NULL == bytes_to_encode_) return ret;
	if (-1 == in_len) in_len = (UINT)strlen(bytes_to_encode_);

	const unsigned char* bytes_to_encode = (const unsigned char*)bytes_to_encode_;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}
	return ret;
}

inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
};

CXTextA xcl_base64EecodeA(const char* encoded_string_, unsigned int in_len)
{
	std::string ret;
	if (NULL == encoded_string_ ) return ret;
	if (-1 == in_len) in_len = (UINT)strlen(encoded_string_);

	const unsigned char* encoded_string = (const unsigned char*)encoded_string_;
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}
	return ret;
}

CXTextA xcl_urlEncodeUtf8(const char* pStringUtf8)
{
	const char* hex_ = "0123456789ABCDEF";
	CXTextA strTemp = "";
	size_t length = strlen(pStringUtf8);
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)pStringUtf8[i]) ||
			(pStringUtf8[i] == '-') ||
			(pStringUtf8[i] == '_') ||
			(pStringUtf8[i] == '.') ||
			(pStringUtf8[i] == '~')
			) {
			strTemp += pStringUtf8[i];
		} else if (pStringUtf8[i] == ' ') {
			strTemp += "+";
		} else {
			strTemp += '%';
			strTemp += hex_[(unsigned char)pStringUtf8[i] >> 4];
			strTemp += hex_[(unsigned char)pStringUtf8[i] % 16];
		}
	}
	return strTemp;
}

CXTextA xcl_urlDecodeUtf8(const char* pStringUtf8)
{
	CXTextA decoded;
	for (size_t i = 0; pStringUtf8[i] != '\0'; ++i) {
		if (pStringUtf8[i] == '%') {
			if (pStringUtf8[i + 1] && pStringUtf8[i + 2]) {
				// 提取十六进制值并转换为字符
				char hexStr[3] = { pStringUtf8[i + 1], pStringUtf8[i + 2], '\0' };
				int hexValue;
				sscanf_s(hexStr, "%x", &hexValue);
				decoded += static_cast<char>(hexValue);
				i += 2; // 跳过下两个字符，因为它们是百分比编码的一部分
			} else {
				// 如果剩余的字符不足以完成完整的编码，请将 '%' 视为常规字符
				decoded += pStringUtf8[i];
			}
		} else if (pStringUtf8[i] == '+') {
			// 将 '+' 转换为空格字符
			decoded += ' ';
		} else {
			// 将其他字符直接附加到解码后的字符串中
			decoded += pStringUtf8[i];
		}
	}
	return decoded;
}

CXText xcl_urlEncode(const wchar_t* pString)
{
	CXTextA str = W2U(pString);
	str = xcl_urlEncodeUtf8(str.get());
	return A2W(str.get(), (int)str.size());
}

CXTextA xcl_urlEncodeA(const char* pString)
{
	CXTextA str = A2U(pString);
	str = xcl_urlEncodeUtf8(str.get());
	return str;
}

CXTextA xcl_urlDecodeA(const char* pString)
{
	CXTextA str = xcl_urlDecodeUtf8(pString);
	return U2A(str.get());
}

CXText xcl_urlDecode(const wchar_t* pString)
{
	CXTextA str = W2A(pString);
	str = xcl_urlDecodeUtf8(str.get());
	return U2W(str.get());
}

CXText  xcl_toText(const CXBytes& value_)
{
	return value_.getTextPtr();
}

CXTextA  xcl_toTextA(const CXBytes& value_)
{
	return value_.getTextPtrA();
}

CXText xcl_intToHexText(int value_, BOOL bUpper, BOOL bFillZero)
{
	wchar_t buf[11] = { 0 };
	if (bFillZero)
	{
		if (bUpper)
			swprintf_s(buf, L"0x%08X", value_);
		else
			swprintf_s(buf, L"0x%08x", value_);
	} else
	{
		if (bUpper)
			swprintf_s(buf, L"0x%X", value_);
		else
			swprintf_s(buf, L"0x%x", value_);
	}
	return buf;
}

CXText xcl_intToBinText(int value_)
{
	wchar_t buf[33] = { 0 };
	_itow_s(value_, buf, 2);
	return buf;
}

CXText xcl_int64ToBinText(__int64 value_)
{
	wchar_t buf[65] = { 0 };
	_i64tow_s(value_, buf, 65, 2);
	return buf;
}

CXText xcl_int64ToHexText(__int64 value_, BOOL bUpper, BOOL bFillZero)
{
	wchar_t buf[20] = { 0 };
	if (bFillZero)
	{
		if (bUpper)
			swprintf_s(buf, L"0x%016I64X", value_);
		else
			swprintf_s(buf, L"0x%016I64x", value_);
	} else
	{
		if (bUpper)
			swprintf_s(buf, L"0x%I64X", value_);
		else
			swprintf_s(buf, L"0x%I64x", value_);
	}
	return buf;
}

float xcl_wtof(const wchar_t* pText) {
	wchar_t* stopstring;
	return wcstof(pText, &stopstring);
}

BOOL _dataToHexStrA(__out BYTE* pbDest, size_t destSize, __in const BYTE* pbSrc, size_t nLen, BOOL bUpper, BOOL bSplit)
{
	if (nLen < 1) return FALSE;
	size_t length = 0;
	if (bSplit)
		length = nLen * 3 - 1 + 1;
	else
		length = nLen * 2 + 1;

	if (destSize < length)
		return FALSE;

	char ddl, ddh;
	size_t i;
	for (i = 0; i < nLen; i++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;
		if (bUpper)
		{
			if (ddh > 57) ddh = ddh + 7;
			if (ddl > 57) ddl = ddl + 7;
		} else
		{
			if (ddh > 57) ddh = ddh + 39;
			if (ddl > 57) ddl = ddl + 39;
		}
		if (bSplit)
		{
			pbDest[i * 3] = ddh;
			pbDest[i * 3 + 1] = ddl;
			pbDest[i * 3 + 2] = ' ';
		} else
		{
			pbDest[i * 2] = ddh;
			pbDest[i * 2 + 1] = ddl;
		}
	}
	pbDest[length - 1] = '\0';
	return TRUE;
}

BOOL _dataToHexStrW(__out wchar_t* pbDest, size_t destSize, __in const BYTE* pbSrc, size_t nLen, BOOL bUpper, BOOL bSplit)
{
	if (nLen < 1) return FALSE;
	size_t length = 0;
	if (bSplit)
		length = nLen * 3 - 1 + 1;
	else
		length = nLen * 2 + 1;

	if (destSize < length)
		return FALSE;

	char ddl, ddh;
	size_t i;
	for (i = 0; i < nLen; i++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;
		if (bUpper)
		{
			if (ddh > 57) ddh = ddh + 7;
			if (ddl > 57) ddl = ddl + 7;
		} else
		{
			if (ddh > 57) ddh = ddh + 39;
			if (ddl > 57) ddl = ddl + 39;
		}
		if (bSplit)
		{
			pbDest[i * 3] = ddh;
			pbDest[i * 3 + 1] = ddl;
			pbDest[i * 3 + 2] = ' ';
		} else
		{
			pbDest[i * 2] = ddh;
			pbDest[i * 2 + 1] = ddl;
		}
	}
	pbDest[length - 1] = '\0';
	return TRUE;
}

int CALLBACK xcl_openFloderDlg_browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (BFFM_INITIALIZED == uMsg)
	{
		if(lpData)
			::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}
CXText xcl_openFloderDlg(const wchar_t* pDefaultDir, const wchar_t* pTitle, HWND hWnd)
{
	TCHAR strDirName[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	bi.hwndOwner = hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = strDirName;
	bi.lpszTitle = pTitle? pTitle : L"请选择一个文件夹";
	bi.ulFlags = BIF_BROWSEFORCOMPUTER | BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lpfn = xcl_openFloderDlg_browseCallbackProc;
	bi.lParam = (LPARAM)pDefaultDir;
	bi.iImage = 0;
	
	LPITEMIDLIST pItemIDList = ::SHBrowseForFolder(&bi);
	if (pItemIDList == NULL)
	{
		return L"";
	}
	if (::SHGetPathFromIDList(pItemIDList, strDirName)) //转换为文件夹路径
	{
		return strDirName; //文件夹路径
	}
	return L"";
}

CXText xcl_openFileDlg(const wchar_t* pDefaultDir, const wchar_t* pTitle, const wchar_t* pFilter, BOOL bChangeDir, HWND hWnd)
{
	OPENFILENAME ofn;
	wchar_t fileNames[1024] = { 0 }; // 用于存储选择的文件名
	ZeroMemory(&ofn, sizeof(ofn));

	//拷贝过滤器到缓冲区， 过滤器由|进行分割，需要替换为\0
	int filterLen = (int)wcslen(pFilter);
	wchar_t* filter = new wchar_t[filterLen + 2];
	//wmemset(filter, 0, filterLen + 2);
	wcscpy_s(filter, filterLen+2, pFilter);
	filter[filterLen] = 0;
	filter[filterLen + 1] = 0;

	for (int i = 0; i < filterLen; i++) {
		if (filter[i] == L'|') {
			filter[i] = L'\0';
		}
	}

	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileNames;
	ofn.nMaxFile = sizeof(fileNames);
	ofn.lpstrTitle = pTitle;
	ofn.nFilterIndex = 0;
	ofn.lpstrInitialDir = pDefaultDir;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (hWnd)
		ofn.hwndOwner = hWnd;
	else
		ofn.hwndOwner = GetActiveWindow();

	if (!bChangeDir) {
		ofn.Flags |= OFN_NOCHANGEDIR;
	}
	CXVector<CXText>  retArray;
	if (GetOpenFileNameW(&ofn)) {
		delete[]filter;
		return ofn.lpstrFile;
	} else {
		DWORD error = CommDlgExtendedError();
		if (error != 0) {
			xcl_log(L"打开文件对话框错误: ", error);
		}
		delete[]filter;
	}
	return L"";
}

CXVector<CXText> xcl_openFilesDlg(const wchar_t* pDefaultDir, const wchar_t* pTitle, const wchar_t* pFilter, BOOL bChangeDir, HWND hWnd)
{
	OPENFILENAME ofn;
	wchar_t fileNames[26000] = { 0 }; // 用于存储选择的文件名
	ZeroMemory(&ofn, sizeof(ofn));

	//拷贝过滤器到缓冲区， 过滤器由|进行分割，需要替换为\0
	int filterLen = (int)wcslen(pFilter);
	wchar_t* filter = new wchar_t[filterLen + 2];
	wcscpy_s(filter, filterLen+2, pFilter);
	filter[filterLen] = 0;
	filter[filterLen+1] = 0;

	for (int i = 0; i < filterLen; i++) {
		if (filter[i] == L'|') {
			filter[i] = L'\0';
		}
	}

	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileNames;
	ofn.nMaxFile = sizeof(fileNames);
	ofn.lpstrTitle = pTitle;
	ofn.nFilterIndex = 0;
	ofn.lpstrInitialDir = pDefaultDir;
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (hWnd)
		ofn.hwndOwner = hWnd;
	else
		ofn.hwndOwner = GetActiveWindow();

	if (bChangeDir) {
		ofn.Flags |= OFN_NOCHANGEDIR;
	}
	CXVector<CXText>  retArray;
	if (GetOpenFileNameW(&ofn)) {

		//获取文件目录
		std::wstring fileDir;
		if (ofn.nFileOffset != 0)
		{
			fileDir.append(ofn.lpstrFile, ofn.nFileOffset);
			fileDir[ofn.nFileOffset - 1] = '\\';
		}

		wchar_t* start = ofn.lpstrFile;
		if (ofn.nFileOffset != 0) //多选标志
		{
			start += ofn.nFileOffset;    // 移动到第一个文件名
		}

		// 遍历所有选中的文件
		while (*start) {
			std::wstring  filePath = fileDir;
			filePath += start;
			retArray.add(filePath);
			start += wcslen(start) + 1; // 跳转到下一个文件路径
		}
	} else {
		DWORD error = CommDlgExtendedError();
		if (error != 0) {
			xcl_log(L"打开文件对话框错误: ", error);
		}
	}
	delete[]filter;
	return retArray;
}

CXText xcl_saveFileDlg(const wchar_t* pDefaultDir, const wchar_t* pTitle, const wchar_t* pFilter, const wchar_t* pDefExt, BOOL bChangeDir, HWND hWnd)
{
	OPENFILENAME ofn;
	wchar_t fileNames[1024] = { 0 }; // 用于存储选择的文件名
	ZeroMemory(&ofn, sizeof(ofn));

	//拷贝过滤器到缓冲区， 过滤器由|进行分割，需要替换为\0
	int filterLen = (int)wcslen(pFilter);
	wchar_t* filter = new wchar_t[filterLen + 2];

	wcscpy_s(filter, filterLen + 2, pFilter);
	filter[filterLen] = 0;
	filter[filterLen + 1] = 0;

	for (int i = 0; i < filterLen; i++) {
		if (filter[i] == L'|') {
			filter[i] = L'\0';
		}
	}

	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileNames;
	ofn.nMaxFile = sizeof(fileNames);
	ofn.lpstrTitle = pTitle;
	ofn.nFilterIndex = 0;
	ofn.lpstrInitialDir = pDefaultDir;
	ofn.lpstrDefExt = pDefExt;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	if (hWnd)
		ofn.hwndOwner = hWnd;
	else
		ofn.hwndOwner = GetActiveWindow();

	if (!bChangeDir) {
		ofn.Flags |= OFN_NOCHANGEDIR;
	}
	CXVector<CXText>  retArray;
	if (GetSaveFileName(&ofn)) {
		delete[]filter;
		return ofn.lpstrFile;
	} else {
		DWORD error = CommDlgExtendedError();
		if (error != 0) {
			xcl_log(L"打开文件对话框错误: ", error);
		}
		delete[]filter;
	}
	return L"";
}


//-----------------------------------------
CDateTimeTM::CDateTimeTM()
{
	__time64_t time_;
	_time64(&time_);
	_localtime64_s(&m_tm, &time_);
}
CDateTimeTM::CDateTimeTM(struct tm& tm_)
{
	m_tm = tm_;
}
CDateTimeTM::CDateTimeTM(INT64 time_)
{
	_localtime64_s(&m_tm, &time_);
}

int CDateTimeTM::getYear()
{
	return m_tm.tm_year + 1900;
}

int CDateTimeTM::getMonth()
{
	return m_tm.tm_mon + 1;
}

int CDateTimeTM::getDay()
{
	return m_tm.tm_mday;
}

int CDateTimeTM::getHour()
{
	return m_tm.tm_hour;
}

int CDateTimeTM::getMinute()
{
	return m_tm.tm_min;
}

int CDateTimeTM::getSecond()
{
	return m_tm.tm_sec;
}

int CDateTimeTM::getDayOfWeek()
{
	return m_tm.tm_wday;
}

//------------------------------------------------
struct tm xcl_getTimeTM()
{
	__time64_t time_;
	_time64(&time_);

	tm tm_;
	_localtime64_s(&tm_, &time_);
	return tm_;
}

CXText xcl_getTimeToText(const wchar_t* pFormat)
{
	__time64_t time_;
	_time64(&time_);

	tm tm_;
	_localtime64_s(&tm_, &time_);
	wchar_t buf[32] = { 0 };
	if (pFormat && *pFormat)
		wcsftime(buf, 32, pFormat, &tm_);
	else
		wcsftime(buf, 32, L"%Y-%m-%d %H:%M:%S", &tm_);
	return buf;
}

CXText xcl_getDate()
{
	__time64_t time_;
	_time64(&time_);

	tm tm_;
	_localtime64_s(&tm_, &time_);
	
	wchar_t buf[32] = { 0 };
	wcsftime(buf, 32, L"%Y-%m-%d", &tm_);
	return buf;
}

CXText xcl_formatTimeTM(__in const wchar_t* pFormat, __in const tm* tm_)
{
	wchar_t buf[128] = { 0 }; //因为用户可能输入比较多的格式字符串, 所以加大BUF, 例如: xcl_formatTimeTM("[%Y-%m-%d %X] 123456....", );
	if(pFormat && *pFormat)
		wcsftime(buf, 128, pFormat, tm_);
	else
		wcsftime(buf, 128, L"%Y-%m-%d %H:%M:%S", tm_);
	return buf;
}

CXText xcl_formatTime(__in const wchar_t* pFormat, __in const INT64 time_)
{
 	tm tm_;
 	if(0==_localtime64_s(&tm_, &time_))
	{
		wchar_t buf[128] = { 0 }; //因为用户可能输入比较多的格式字符串, 所以加大BUF, 例如: xcl_formatTimeTM("[%Y-%m-%d %X] 123456....", );
		if(pFormat && *pFormat)
			wcsftime(buf, 128, pFormat, &tm_);
		else
			wcsftime(buf, 128, L"%Y-%m-%d %H:%M:%S", &tm_);
		return buf;
	}
	return L"";
}


int xcl_getDayInMonth(int y, int m)
{
	int d;
	int day[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (2 == m)
	{
		d = (((0 == y % 4) && (0 != y % 100) || (0 == y % 400)) ? 29 : 28);
	} else
	{
		d = day[m - 1];
	}
	return d;
}

int xcl_timeToTM(__in INT64 time_, __out tm* tm_)
{
	return gmtime_s(tm_, &time_);
}

int xcl_timeToLocalTM(__in INT64 time_, __out tm* tm_)
{
	return localtime_s(tm_, &time_);
}

void xcl_timeToSystemTime(__in INT64 time_, __out SYSTEMTIME& systemTime)
{
	struct tm timeStruct;
	localtime_s(&timeStruct , &time_);

	systemTime.wYear = timeStruct.tm_year + 1900;
	systemTime.wMonth = timeStruct.tm_mon + 1;
	systemTime.wDayOfWeek = timeStruct.tm_wday;
	systemTime.wDay = timeStruct.tm_mday;
	systemTime.wHour = timeStruct.tm_hour;
	systemTime.wMinute = timeStruct.tm_min;
	systemTime.wSecond = timeStruct.tm_sec;
	systemTime.wMilliseconds = 0;
}

INT64 xcl_systemTimeToTime(__in SYSTEMTIME& systemTime)
{
	struct tm tmTime;
	tmTime.tm_sec = systemTime.wSecond;     // 秒
	tmTime.tm_min = systemTime.wMinute;     // 分
	tmTime.tm_hour = systemTime.wHour;      // 时
	tmTime.tm_mday = systemTime.wDay;       // 日
	tmTime.tm_mon = systemTime.wMonth - 1;  // 月份从0开始，所以减去1
	tmTime.tm_year = systemTime.wYear - 1900;  // 年份需要减去1900
	tmTime.tm_isdst = -1;  // 时区信息，默认为-1表示使用系统配置的时区信息

	return mktime(&tmTime);  // 使用mktime函数将tm结构体转换为time_t类型
}

void xcl_timeToFileTime(__in INT64 time_, __out FILETIME& fileTime)
{
	// 将 time_t 转换为 FILETIME 结构
	ULARGE_INTEGER uli;

	// 转换为64位整数，单位是100纳秒
	uli.QuadPart = (static_cast<ULONGLONG>(time_) + 11644473600LL) * 10000000LL;

	fileTime.dwLowDateTime = uli.LowPart;
	fileTime.dwHighDateTime = uli.HighPart;
}

INT64 xcl_fileTimeToTime(__in FILETIME& fileTime)
{
	ULARGE_INTEGER uli;
	uli.LowPart = fileTime.dwLowDateTime;
	uli.HighPart = fileTime.dwHighDateTime;

	ULONGLONG ft64 = uli.QuadPart;

	// 将FILETIME转换为从1601年1月1日起的秒数
	ft64 -= 116444736000000000ULL;

	// 将纳秒转换为秒
	ft64 /= 10000000;

	return static_cast<time_t>(ft64);
}

//-----------------------------------------------
BOOL xcl_setClipboardText(HWND hWnd, const wchar_t* pText, int wcharLen)
{
	if (OpenClipboard(hWnd))
	{
		EmptyClipboard();
		wcharLen++;
		int lengByte = wcharLen * 2;
		HGLOBAL hClipText = GlobalAlloc(GMEM_MOVEABLE, lengByte);
		if (hClipText)
		{
			wchar_t* pBufText = (wchar_t*)GlobalLock(hClipText);
			wmemcpy(pBufText, pText, wcharLen);
			GlobalUnlock(hClipText);
			SetClipboardData(CF_UNICODETEXT, hClipText);
		}
		CloseClipboard();
		return TRUE;
	}
	return FALSE;
}

CXText xcl_getClipboardText(HWND hWnd)
{
	CXText  retText;
	if (NULL == OpenClipboard(hWnd))
		return retText;

	HANDLE hGlb = GetClipboardData(CF_UNICODETEXT);
	if (hGlb)
	{
		wchar_t* pText = (wchar_t*)GlobalLock(hGlb);
		if (pText)
		{
			retText = pText;
		}
		GlobalUnlock(hGlb);
	}
	CloseClipboard();
	return retText;
}

//-----------------------------------------------
void* rc_findFile(const wchar_t* pName, __out UINT* pOutSize, HMODULE hModule, const wchar_t* pResType)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);

	HRSRC hResHandle = FindResource(hModule, pName, pResType ? pResType : L"RT_RCDATA");
	if (hResHandle)
	{
		HGLOBAL hGlobal = LoadResource(hModule, hResHandle);
		if (hGlobal)
		{
			if (pOutSize)
				*pOutSize = SizeofResource(hModule, hResHandle);
			return LockResource(hGlobal);
		}
	}
	return NULL;
}
void* rc_findFileByID(int id, __out UINT* pOutSize, HMODULE hModule, const wchar_t* pResType)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);
	return rc_findFile(MAKEINTRESOURCE(id), pOutSize, hModule, pResType);
}

CXBytes rc_findFileToBytes(const wchar_t* pName, HMODULE hModule, const wchar_t* pResType)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);

	CXBytes ret;
	HRSRC hResHandle = FindResource(hModule, pName, pResType ? pResType : L"RT_RCDATA");
	if (hResHandle)
	{
		HGLOBAL hGlobal = LoadResource(hModule, hResHandle);
		if (hGlobal)
		{
			DWORD size = SizeofResource(hModule, hResHandle);
			if (size > 0)
			{
				void* data = LockResource(hGlobal);
				ret.set(data, size);
			}
		}
	}
	return ret ;
}
CXBytes rc_findFileToBytesByID(int id, HMODULE hModule, const wchar_t* pResType)
{
	return rc_findFileToBytes(MAKEINTRESOURCE(id), hModule, pResType);
}

CXText  rc_findFileToText(const wchar_t* pName, HMODULE hModule, const wchar_t* pResType)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);

	HRSRC hResHandle = FindResource(hModule, pName, pResType ? pResType : L"RT_RCDATA");
	if (hResHandle)
	{
		HGLOBAL hGlobal = LoadResource(hModule, hResHandle);
		if (hGlobal)
		{
			DWORD size = SizeofResource(hModule, hResHandle);
			if (size > 0)
			{
				void* data = LockResource(hGlobal);
				if (data)
					return A2W((const char*)data);
			}
		}
	}
	return "";
}
CXText  rc_findFileToTextByID(int id, HMODULE hModule, const wchar_t* pResType)
{
	return rc_findFileToText(MAKEINTRESOURCE(id), hModule, pResType);
}

CXTextA rc_findFileToTextA(const wchar_t* pName, HMODULE hModule, const wchar_t* pResType)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);

	HRSRC hResHandle = FindResource(hModule, pName, pResType ? pResType : L"RT_RCDATA");
	if (hResHandle)
	{
		HGLOBAL hGlobal = LoadResource(hModule, hResHandle);
		if (hGlobal)
		{
			DWORD size = SizeofResource(hModule, hResHandle);
			if (size > 0)
				return (const char*)LockResource(hGlobal);
		}
	}
	return "";
}
CXTextA rc_findFileToTextAByID(int id, HMODULE hModule, const wchar_t* pResType)
{
	return rc_findFileToTextA(MAKEINTRESOURCE(id), hModule, pResType);
}

HICON rc_findIcon(const wchar_t* pName, HMODULE hModule)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);
	return LoadIcon(hModule, pName);
}
HICON rc_findIconByID(int id, HMODULE hModule)
{
	return rc_findIcon(MAKEINTRESOURCE(id), hModule);
}

HICON rc_findIcon2(const wchar_t* pName, int cx, int cy, HMODULE hModule)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);
	return (HICON)LoadImage(hModule, pName, IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
}
HICON rc_findIcon2ByID(int id, int cx, int cy, HMODULE hModule)
{
	return rc_findIcon2(MAKEINTRESOURCE(id), cx, cy, hModule);
}

HCURSOR rc_findCursor(const wchar_t* pName, HMODULE hModule)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);
	return LoadCursor(hModule, pName);
}
HCURSOR rc_findCursorByID(int id, HMODULE hModule)
{
	return rc_findCursor(MAKEINTRESOURCE(id), hModule);
}

HBITMAP rc_findBitmap(const wchar_t* pName, HMODULE hModule)
{
	if (NULL == hModule) hModule = GetModuleHandle(NULL);
	return LoadBitmap(hModule, pName);
}
HBITMAP rc_findBitmapByID(int id, HMODULE hModule)
{
	return rc_findBitmap(MAKEINTRESOURCE(id), hModule);
}

CXText rc_findString(int id, HMODULE hModule)
{
	CXText retText;
	if (NULL == hModule) hModule = GetModuleHandle(NULL);
	retText.resize(1024);

	LoadStringW(hModule, id, (wchar_t*)retText.get(), 1024);
	return retText;
}

//-----------------------
LONG reg_openKey(HKEY hKey, LPCWSTR lpSubKey, HKEY& hResult)
{
	return RegOpenKeyEx(hKey, lpSubKey, 0, KEY_READ | KEY_WOW64_64KEY, &hResult);
}

LONG reg_enumValue(HKEY hKey, DWORD dwIndex, CXText& retNmae, CXBytes& retBytes, UINT& retType)
{
	wchar_t szName[MAXBYTE] = { 0 };
	DWORD dwBufferSize = MAXBYTE;
	DWORD dwSize;
	LONG ret = RegEnumValue(hKey, dwIndex, szName, &dwBufferSize, NULL, NULL, NULL, &dwSize);
	if (ERROR_SUCCESS == ret)
	{
		dwBufferSize = MAXBYTE;
		retBytes.resize(dwSize);
		ret = RegEnumValue(hKey, dwIndex, szName, &dwBufferSize, NULL, (DWORD*)&retType, (BYTE*)retBytes.get(), &dwSize);
		if (ERROR_SUCCESS == ret)
		{
			retNmae = szName;
		}
	}
	return ret;
}

BOOL reg_setValueText(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, const wchar_t* pValue)
{
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKeyEx(hRoot, pPath, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKey))
	{
		return FALSE;
	}
	if (ERROR_SUCCESS != RegSetValueEx(hKey, pName, 0, REG_SZ, (BYTE*)pValue, (DWORD)wcslen(pValue) * 2))
	{
		RegCloseKey(hKey);
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}

BOOL reg_setValueInt(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, DWORD dwValue)
{
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKeyEx(hRoot, pPath, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKey))
	{
		return FALSE;
	}
	if (ERROR_SUCCESS != RegSetValueEx(hKey, pName, 0, REG_DWORD, (BYTE*)&dwValue, 4))
	{
		RegCloseKey(hKey);
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}

BOOL reg_setValueInt64(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, INT64 dwValue)
{
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKeyEx(hRoot, pPath, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKey))
	{
		return FALSE;
	}
	if (ERROR_SUCCESS != RegSetValueEx(hKey, pName, 0, REG_QWORD, (BYTE*)&dwValue, 4))
	{
		RegCloseKey(hKey);
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}

BOOL reg_setValueData(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, BYTE* data, DWORD dataSize)
{
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKeyEx(hRoot, pPath, 0, KEY_WRITE | KEY_WOW64_64KEY, &hKey))
	{
		return FALSE;
	}
	if (ERROR_SUCCESS != RegSetValueEx(hKey, pName, 0, REG_BINARY, (BYTE*)data, dataSize))
	{
		RegCloseKey(hKey);
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}

LONG reg_queryValueData(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, CXBytes& retBytes, DWORD& retType)
{
	HKEY hKey;
	LONG ret = RegOpenKeyEx(hRoot, pPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
	if (ERROR_SUCCESS == ret)
	{
		DWORD  dwSize;
		ret = RegQueryValueEx(hKey, pName, 0, &retType, NULL, &dwSize);
		if (ERROR_SUCCESS == ret)
		{
			retBytes.resize(dwSize);
			ret = RegQueryValueEx(hKey, pName, 0, &retType, (BYTE*)retBytes.get(), &dwSize);
		}
		RegCloseKey(hKey);
	}
	return ret;
}

BOOL reg_queryValueText(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, CXText& retValue)
{
	BOOL bResult = FALSE;
	HKEY hKey;
	LONG ret = RegOpenKeyEx(hRoot, pPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
	if (ERROR_SUCCESS == ret)
	{
		wchar_t data[4096] = { 0 };
		DWORD   bufferSize = 4096 * 2;
		DWORD   type;
		ret = RegQueryValueEx(hKey, pName, 0, &type, (BYTE*)data, &bufferSize);
		if (ERROR_SUCCESS == ret)
		{
			if (REG_SZ == type)
			{
				retValue = data;
				bResult = TRUE;
			}
		}
		RegCloseKey(hKey);
	}
	return bResult;
}

BOOL reg_queryValueInt(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, UINT& retValue)
{
	BOOL bResult = FALSE;
	HKEY hKey;
	LONG ret = RegOpenKeyEx(hRoot, pPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
	if (ERROR_SUCCESS == ret)
	{
		DWORD   bufferSize = 4;
		DWORD   type;
		ret = RegQueryValueEx(hKey, pName, 0, &type, (BYTE*)&retValue, &bufferSize);
		if (ERROR_SUCCESS == ret)
		{
			if (REG_DWORD == type)
			{
				bResult = TRUE;
			}
		}
		RegCloseKey(hKey);
	}
	return bResult;
}

BOOL reg_queryValueInt64(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName, INT64& retValue)
{
	BOOL bResult = FALSE;
	HKEY hKey;
	LONG ret = RegOpenKeyEx(hRoot, pPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
	if (ERROR_SUCCESS == ret)
	{
		DWORD   bufferSize = 8;
		DWORD   type;
		ret = RegQueryValueEx(hKey, pName, 0, &type, (BYTE*)retValue, &bufferSize);
		if (ERROR_SUCCESS == ret)
		{
			if (REG_QWORD == type)
			{
				bResult = TRUE;
			}
		}
		RegCloseKey(hKey);
	}
	return bResult;
}

BOOL reg_isExist(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName)
{
	BOOL bResult = FALSE;
	HKEY hKey;
	if (ERROR_SUCCESS == RegOpenKeyEx(hRoot, pPath, NULL, KEY_READ | KEY_WOW64_64KEY, &hKey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, pName, 0, NULL, NULL, NULL))
		{
			bResult = TRUE;
		}
		RegCloseKey(hKey);
	}
	return bResult;
}

LONG reg_createKey(HKEY hRoot, LPCWSTR pSubKey)
{
	HKEY  hResult;
	DWORD dwDisposition = 0;
	LONG  ret = RegCreateKeyEx(hRoot, pSubKey, 0, NULL, 0, 0, NULL, &hResult, &dwDisposition);
	if (ERROR_SUCCESS == ret)
	{
		RegCloseKey(hResult);
	} else if (REG_OPENED_EXISTING_KEY==dwDisposition) //密钥已存在，并且只是打开而未更改。
	{
		return ERROR_SUCCESS;
	}
	return ret;
}

//@别名  注册表_删除值(主键句柄, 子键路径字符串, 名称)
LONG reg_deleteVaule(HKEY hRoot, const wchar_t* pPath, const wchar_t* pName)
{
	HKEY hKey = NULL;
	LONG ret = RegOpenKeyEx(hRoot, pPath, 0, KEY_ALL_ACCESS, &hKey);
	if (ERROR_SUCCESS == ret)
	{
		ret = RegDeleteValue(hKey, pName);
		RegCloseKey(hKey);
	}
	return ret;
}

//@别名  注册表_删除键(主键句柄, 子键路径字符串)
LONG reg_deleteKey(HKEY hKey, const wchar_t* pPath)
{
	return RegDeleteKeyEx(hKey, pPath, KEY_WOW64_64KEY, 0);
}

CXText xcl_GetModuleFileName(HMODULE hModule)
{
	wchar_t  buf[MAX_PATH] = { 0 };
	DWORD ret = GetModuleFileName(hModule, buf, MAX_PATH);
	if (ret > 0)
		return  buf;
	return L"";
}


int  xcl_textToInt(const CXText& text)
{
	return _wtoi(text.get());
}
int  xcl_textToInt(const CXTextA& text)
{
	return atoi(text.get());
}
int  xcl_textToInt(const wchar_t* text)
{
	return _wtoi(text);
}
int  xcl_textToInt(const char* text)
{
	return atoi(text);
}

UINT  xcl_textToUint(const CXText& text)
{
	return (UINT)_wtoi(text.get());
}
UINT  xcl_textToUint(const CXTextA& text)
{
	return (UINT)atoi(text.get());
}
UINT  xcl_textToUint(const wchar_t* text)
{
	return (UINT)_wtoi(text);
}
UINT  xcl_textToUint(const char* text)
{
	return (UINT)atoi(text);
}

INT64  xcl_textToInt64(const CXText& text) 
{
	return _wtoi64(text.get());
}
INT64  xcl_textToInt64(const CXTextA& text)
{
	return _atoi64(text.get());
}
INT64  xcl_textToInt64(const wchar_t* text)
{
	return _wtoi64(text);
}
INT64  xcl_textToInt64(const char* text)
{
	return _atoi64(text);
}

UINT64  xcl_textToUint64(const CXText& text)
{
	return (UINT64)_wtoi64(text.get());
}
UINT64  xcl_textToUint64(const CXTextA& text)
{
	return (UINT64)_atoi64(text.get());
}
UINT64  xcl_textToUint64(const wchar_t* text)
{
	return (UINT64)_wtoi64(text);
}
UINT64  xcl_textToUint64(const char* text)
{
	return (UINT64)_atoi64(text);
}

float  xcl_textToFloat(const CXText& text)
{
	return (float)_wtof(text.get());
}
float  xcl_textToFloat(const CXTextA& text)
{
	return (float)atof(text.get());
}
float  xcl_textToFloat(const wchar_t* text)
{
	return (float)_wtof(text);
}
float  xcl_textToFloat(const char* text)
{
	return (float)atof(text);
}

double  xcl_textToDouble(const CXText& text)
{
	return _wtof(text.get());
}
double  xcl_textToDouble(const CXTextA& text)
{
	return atof(text.get());
}
double  xcl_textToDouble(const wchar_t* text)
{
	return _wtof(text);
}
double  xcl_textToDouble(const char* text)
{
	return atof(text);
}
