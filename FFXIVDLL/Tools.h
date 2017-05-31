#pragma once
#include<Windows.h>
#include<stdint.h>
#include<string>

namespace Tools {
	void MillisToSystemTime(UINT64 millis, SYSTEMTIME *st);
	void MillisToLocalTime(UINT64 millis, SYSTEMTIME *st);
	uint64_t GetLocalTimestamp();
	DWORD GetMainThreadID(DWORD dwProcID);
	bool BinaryCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
	DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char * szMask);
	bool TestValidString(char* p);
	bool DirExists(const std::wstring& dirName_in);
	class ByteQueue {
	public:
		ByteQueue(size_t capacity) : _capacity(capacity) { init(); }
		~ByteQueue() { delete[] _buf; }
		size_t read(void* data, size_t bytes)
		{
			bytes = min(bytes, getUsed());
			const size_t bytes_read1 = min(bytes, _capacity - _rptr);
			memcpy(data, _buf + _rptr, bytes_read1);
			memcpy((char*)data + bytes_read1, _buf, bytes - bytes_read1);
			updateIndex(_rptr, bytes);
			return bytes;
		}

		size_t peek(void* data, size_t bytes)
		{
			bytes = min(bytes, getUsed());
			const size_t bytes_read1 = min(bytes, _capacity - _rptr);
			memcpy(data, _buf + _rptr, bytes_read1);
			memcpy((char*)data + bytes_read1, _buf, bytes - bytes_read1);
			return bytes;
		}

		size_t waste(size_t bytes)
		{
			bytes = min(bytes, getUsed());
			const size_t bytes_read1 = min(bytes, _capacity - _rptr);
			updateIndex(_rptr, bytes);
			return bytes;
		}

		size_t write(const void* data, size_t bytes)
		{
			bytes = min(bytes, getFree());
			const size_t bytes_write1 = min(bytes, _capacity - _wptr);
			memcpy(_buf + _wptr, data, bytes_write1);
			memcpy(_buf, (char*)data + bytes_write1, bytes - bytes_write1);
			updateIndex(_wptr, bytes);
			return bytes;
		}
		bool isFull() { return (_wptr + 1) % (_capacity) == _rptr; }
		bool isEmpty() { return _wptr == _rptr; }
		size_t getUsed()
		{
			return (_capacity - _rptr + _wptr) % _capacity;
		}

		size_t getFree()
		{
			return (_capacity - 1 - _wptr + _rptr) % _capacity;
		}
	private:
		//only _capacity-1 is used, one is to identify full or empty.
		void init() {
			_buf = new unsigned char[_capacity];
			_wptr = 0; _rptr = 0; _used_size = 0;
		}
		unsigned char* _buf;
		size_t _capacity, _wptr, _rptr, _used_size;

		void updateIndex(size_t& index, size_t bytes)
		{
			index = (index + bytes) % _capacity;
		}
	};
}