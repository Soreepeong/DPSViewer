#pragma once
#include<Windows.h>
#include<string>
#include<condition_variable>
#include<deque>
class ExternalPipe {
	friend class Hooks;
private:
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

	HANDLE hPipeRead, hPipeWrite;
	HANDLE hPipeReaderThread = INVALID_HANDLE_VALUE;
	HANDLE hPipeWriterThread = INVALID_HANDLE_VALUE;
	HANDLE hNewChatItemEvent = INVALID_HANDLE_VALUE;
	HANDLE hUnloadEvent;
	bqueue<std::string> injectQueue;
	bqueue<std::string> streamQueue;


public:
	ExternalPipe(HANDLE unloadEvent);
	~ExternalPipe();

	void AddChat(std::string str);
	void SendInfo(std::string str);

	DWORD PipeReaderThread();
	DWORD PipeWriterThread();
	static DWORD WINAPI PipeReaderThreadExternal(PVOID param) { ((ExternalPipe*)param)->PipeReaderThread(); return 0; }
	static DWORD WINAPI PipeWriterThreadExternal(PVOID param) { ((ExternalPipe*)param)->PipeWriterThread(); return 0; }
};

