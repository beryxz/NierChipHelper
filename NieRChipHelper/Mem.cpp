#include "pch.h"
#include "Mem.h"

bool Mem::detour32(_In_ void* hookAddr, _In_ void* hookFunc, _In_ int len)
{
	if (len < 5) return false;

	// change protection
	DWORD oldProtection;
	VirtualProtect(hookAddr, len, PAGE_EXECUTE_READWRITE, &oldProtection);

	// Set mem to NOP
	memset(hookAddr, 0x90, len);

	// change first 5 bytes to "JMP hookFunc"
	DWORD relativeAddr = ((DWORD*)hookFunc - (DWORD*)hookAddr) - 0x5;
	*(BYTE*)(hookAddr) = 0xE9;
	*(DWORD*)((DWORD)hookAddr + 1) = relativeAddr;

	// reset original protection
	DWORD tmp;
	VirtualProtect(hookAddr, len, oldProtection, &tmp);

	return true;
}

_Success_(return)
bool Mem::detour64(_In_ uintptr_t addrToHook, _In_ LPVOID hookFunc, _In_ int len, _Out_ hook_t* hook)
{
	if (len < 5) return false;
	hook->len = len;

	/*
		Allocate memory, then set near/far jump to custom code and jumpback
	*/
	LPVOID pAllocMem;
	for (uintptr_t i = 0; i < 65535; i++)
	{
		pAllocMem = VirtualAlloc((LPVOID)((uintptr_t)addrToHook - (i * 16384)), 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (pAllocMem != NULL)
			break;
	}
	if (!pAllocMem) return false;

	if (uintptr_t(pAllocMem) - uintptr_t(hookFunc) > 0xFFFFFFFF)
	{
		// set long jump to custom code
		memset(pAllocMem, 0x0, 14);
		*(WORD*)((BYTE*)pAllocMem) = 0x25FF;
		*(uintptr_t*)((BYTE*)pAllocMem + 6) = (uintptr_t)hookFunc;

		// jumpback
		*(BYTE*)((BYTE*)pAllocMem + 14) = 0xE9;
		*(uintptr_t*)((BYTE*)pAllocMem + 15) = (addrToHook - ((uintptr_t)pAllocMem + 14 + 5)) + len;
	}
	else
	{
		*(BYTE*)((BYTE*)pAllocMem) = 0xE9;
		*(uintptr_t*)((BYTE*)pAllocMem + 1) = (uintptr_t)hookFunc - (uintptr_t)pAllocMem - 5;

		*(BYTE*)((BYTE*)pAllocMem + 5) = 0xE9;
		*(uintptr_t*)((BYTE*)pAllocMem + 6) = addrToHook - ((uintptr_t)pAllocMem + 5 + 5) + len;
	}

	hook->pAllocatedMem = pAllocMem;
	hook->pHookedAddr = (LPVOID)addrToHook;
	hook->pOriginalBytes = new BYTE[len];
	memcpy(hook->pOriginalBytes, (LPVOID)addrToHook, len);

	/*
		Modify Game memory to jump to our allocated memory
	*/
	DWORD oldProtection;
	VirtualProtect((LPVOID)addrToHook, len, PAGE_EXECUTE_READWRITE, &oldProtection);

	memset((LPVOID)addrToHook, 0x90, len);

	DWORD relativeAddr = (DWORD)pAllocMem - (DWORD)addrToHook - 0x5;
	*(BYTE*)(addrToHook) = 0xE9;
	*(DWORD*)((uintptr_t)addrToHook + 1) = relativeAddr;

	DWORD tmp;
	VirtualProtect((LPVOID)addrToHook, len, oldProtection, &tmp);

	return true;
}

void Mem::patch(_In_ void* addr, _In_ BYTE* patchBytes, _In_ int len)
{
	DWORD oldProtection;
	VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &oldProtection);

	memcpy_s(addr, len, patchBytes, len);

	DWORD tmp;
	VirtualProtect(addr, len, oldProtection, &tmp);
}

Mem::hook_t::~hook_t()
{
	if (pOriginalBytes != nullptr) {
		delete[] pOriginalBytes;
	}

	if (pAllocatedMem != nullptr) {
		VirtualFree(pAllocatedMem, 0, MEM_RELEASE);
	}
}
