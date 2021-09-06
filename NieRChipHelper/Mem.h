#pragma once

#include <Windows.h>

class Mem
{
public:

	struct hook_t {
		void* pAllocatedMem; // Allocated memory for JUMP FAR and JUMPBACK
		void* pHookedAddr;
		int len;
		BYTE* pOriginalBytes;

		hook_t() : pAllocatedMem(nullptr), pHookedAddr(nullptr), len(0), pOriginalBytes(nullptr) {};
		~hook_t();
	};

	static bool detour32(
		_In_ void* hookAddr,
		_In_ void* hookFunc,
		_In_ int len
	);
	
	_Success_(return)
	static bool detour64(
		_In_ uintptr_t addrToHook,
		_In_ LPVOID hookFunc,
		_In_ int len,
		_Out_ hook_t* hook
	);

	static void patch(
		_In_ void* addr,
		_In_ BYTE* patchBytes,
		_In_ int len
	);
};

