#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "targetver.h"

#pragma comment(lib, "ws2_32.lib")
#include <Winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <OleAuto.h>
#include <atomic>
#include "zlib.h"
#include "MinHook.h"
#include "FFXIVDLL.h"
#pragma comment(lib, "psapi.lib")

extern FFXIVDLL *ffxivDll;

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
			                  { (punk)->Release(); (punk) = NULL; }