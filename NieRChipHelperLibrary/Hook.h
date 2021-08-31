#pragma once

#include <stdio.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <array>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN

#include <d3d11.h>       // D3D interface
#include <dxgi.h>        // DirectX driver interface

#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d11.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "detours.h"

#include "exception.h"
#include "reclass.h"
#include "Nier.h"

// ImGui Exportation
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// DirectX Exportation
typedef HRESULT(__stdcall* D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

// Dear ImGui
void customImguiDrawAlways();
void customImguiDrawMenu();


extern ID3D11Device* g_pDevice;
extern ID3D11DeviceContext* g_pContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_pRenderTargetView;

extern D3D11PresentHook g_pHookD3D11Present;

extern BOOL g_bInitialized;
extern BOOL g_ShowMenu;

extern HWND g_Hwnd;
extern WNDPROC g_originalWndProcHandler;



class Hook {
public:
	~Hook();

	BOOL toggleConsole();
	void initialize();
	void terminate();

	// Dear ImGui functions
	static LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Helpers
	static HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext);
	
	// Hooks
	static LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static HRESULT __fastcall Present(IDXGISwapChain* pChain, UINT SyncInterval, UINT Flags);


private:
	BOOL isConsoleActive = FALSE;
	FILE* fOutStream = nullptr;
	FILE* fErrStream = nullptr;

	BOOL bPresentHooked = FALSE;

	void getD3D11PresentAddr();
	void detourDirectX();
	void unhook();
};