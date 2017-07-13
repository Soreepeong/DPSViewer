#pragma once
#include<Windows.h>
#include<stdint.h>
#include<string>
#include<condition_variable>
#include<deque>
#include<mutex>

namespace Tools {
	void MillisToSystemTime(UINT64 millis, SYSTEMTIME *st);
	void MillisToLocalTime(UINT64 millis, SYSTEMTIME *st);
	uint64_t GetLocalTimestamp();
	DWORD GetMainThreadID(DWORD dwProcID);
	bool BinaryCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
	DWORD_PTR FindPattern(DWORD_PTR dwAddress, DWORD_PTR dwLen, BYTE *bMask, char * szMask);
	bool TestValidString(char* p);
	bool TestValidPtr(void* p, int len);
	bool DirExists(const std::wstring& dirName_in);
	void DebugPrint(LPCTSTR lpszFormat, ...);
	class ByteQueue {
	private:
		std::recursive_mutex              d_mutex;
		uint64_t mLastUse;
	public:
		ByteQueue(size_t capacity) : _capacity(capacity) { init(); }
		ByteQueue() : _capacity(1048576*8) { init(); }
		~ByteQueue() { delete[] _buf; }
		size_t read(void* data, size_t bytes)
		{
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			mLastUse = GetTickCount64();
			bytes = min(bytes, getUsed());
			const size_t bytes_read1 = min(bytes, _capacity - _rptr);
			memcpy(data, _buf + _rptr, bytes_read1);
			memcpy((char*)data + bytes_read1, _buf, bytes - bytes_read1);
			updateIndex(_rptr, bytes);
			return bytes;
		}

		size_t peek(void* data, size_t bytes)
		{
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			mLastUse = GetTickCount64();
			bytes = min(bytes, getUsed());
			const size_t bytes_read1 = min(bytes, _capacity - _rptr);
			memcpy(data, _buf + _rptr, bytes_read1);
			memcpy((char*)data + bytes_read1, _buf, bytes - bytes_read1);
			return bytes;
		}

		size_t waste(size_t bytes)
		{
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			mLastUse = GetTickCount64();
			bytes = min(bytes, getUsed());
			const size_t bytes_read1 = min(bytes, _capacity - _rptr);
			updateIndex(_rptr, bytes);
			return bytes;
		}

		size_t write(const void* data, size_t bytes)
		{
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			mLastUse = GetTickCount64();
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
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			mLastUse = GetTickCount64();
			return (_capacity - _rptr + _wptr) % _capacity;
		}

		size_t getFree()
		{
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			mLastUse = GetTickCount64();
			return (_capacity - 1 - _wptr + _rptr) % _capacity;
		}

		bool isStall(int dur) {
			return GetTickCount64() - mLastUse > dur;
		}

		bool isStall() {
			return isStall(30000); // 30 sec
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
	template <typename T>
	class bqueue {
	private:
		std::recursive_mutex              d_mutex;
		std::condition_variable d_condition;
		std::deque<T>           d_queue;
	public:
		void push(T const& value) {
			{
				std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
				d_queue.push_front(value);
				if (d_queue.size() > 400) {
					d_queue.pop_back();
				}
			}
			this->d_condition.notify_one();
		};
		T pop() {
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			this->d_condition.wait(lock, [=] { return !this->d_queue.empty(); });
			T rc(std::move(this->d_queue.back()));
			this->d_queue.pop_back();
			return rc;
		};
		bool tryPop(T *res) {
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			if (this->d_queue.empty())
				return 0;
			T rc(std::move(this->d_queue.back()));
			this->d_queue.pop_back();
			*res = rc;
			return 1;
		};
		void clear() {
			std::lock_guard<std::recursive_mutex> lock(this->d_mutex);
			this->d_queue.clear();
		}
	};
}