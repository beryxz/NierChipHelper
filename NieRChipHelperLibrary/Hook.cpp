#include "pch.h"
#include "Hook.h"


ID3D11Device* g_pDevice = NULL;
ID3D11DeviceContext* g_pContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;

D3D11PresentHook g_pHookD3D11Present = NULL;

BOOL g_bInitialized = FALSE;
BOOL g_ShowMenu = FALSE;

HWND g_Hwnd = NULL;
WNDPROC g_originalWndProcHandler = nullptr;
HMODULE g_hDllModule = nullptr;


Hook::Hook(HMODULE hDllModule)
{
	g_hDllModule = hDllModule;
}

Hook::~Hook()
{
	if (g_bInitialized) terminate();
	if (isConsoleActive) toggleConsole();
}

BOOL Hook::toggleConsole()
{
	if (isConsoleActive)
	{
		if (fclose(fOutStream)) return FALSE;
		if (fclose(fErrStream)) return FALSE;
		if (!FreeConsole()) return FALSE;

		isConsoleActive = FALSE;
		return TRUE;
	}
	else
	{
		if (!AllocConsole()) return FALSE;
		if (freopen_s(&fOutStream, "CONOUT$", "w", stdout)) return FALSE;
		if (freopen_s(&fErrStream, "CONOUT$", "w", stderr)) return FALSE;

		isConsoleActive = TRUE;
		return TRUE;
	}
}

void Hook::initialize()
{
	// Hook D3D11
	getD3D11PresentAddr();
	detourDirectX();
	while (!g_bInitialized) {
		Sleep(1000);
	}
}

void Hook::unhook() {
	if (g_originalWndProcHandler != nullptr) {
		std::cout << "[+] Unhooking WndProcHandler" << std::endl;
		(WNDPROC)SetWindowLongPtr(g_Hwnd, GWLP_WNDPROC, (LONG_PTR)g_originalWndProcHandler);
		
		std::cout << "[+] Unhooking Dear Imgui" << std::endl;
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	if (bPresentHooked) {
		std::cout << "[+] Removing DirectX Detour" << std::endl;
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(LPVOID&)g_pHookD3D11Present, (PBYTE)Hook::Present);
		DetourTransactionCommit();
	}
}

void Hook::terminate()
{
	unhook();
	Sleep(200);

	g_pDevice->Release();
	g_pContext->Release();
	g_pSwapChain->Release();
	g_pRenderTargetView->Release();
}

LRESULT CALLBACK Hook::DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Hook::hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGuiIO& io = ImGui::GetIO();
	POINT mPos;
	GetCursorPos(&mPos);
	ScreenToClient(g_Hwnd, &mPos);
	ImGui::GetIO().MousePos.x = (float)mPos.x;
	ImGui::GetIO().MousePos.y = (float)mPos.y;

	if (uMsg == WM_KEYUP)
	{
		if (wParam == VK_F2)
		{
			g_ShowMenu = !g_ShowMenu;
		}

	}

	if (g_ShowMenu)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		return true;
	}

	return CallWindowProc(g_originalWndProcHandler, hWnd, uMsg, wParam, lParam);
}

HRESULT Hook::GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	HRESULT ret = pSwapChain->GetDevice(__uuidof(ID3D11Device), (PVOID*)ppDevice);

	if (SUCCEEDED(ret))
		(*ppDevice)->GetImmediateContext(ppContext);

	return ret;
}

void Hook::getD3D11PresentAddr()
{
	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, Hook::DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);
	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = 2;
	sd.BufferDesc.Height = 2;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	D3D_FEATURE_LEVEL FeatureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
	UINT numFeatureLevelsRequested = 1;
	D3D_FEATURE_LEVEL FeatureLevelsSupported;
	HRESULT hr;
	IDXGISwapChain* swapchain = 0;
	ID3D11Device* dev = 0;
	ID3D11DeviceContext* devcon = 0;

	if (FAILED(hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0,
		&FeatureLevelsRequested,
		numFeatureLevelsRequested,
		D3D11_SDK_VERSION,
		&sd,
		&swapchain,
		&dev,
		&FeatureLevelsSupported,
		&devcon)))
	{
		std::cout << "[-] Failed to hook Present with VT method." << std::endl;
		return;
	}

	DWORD_PTR* pSwapChainVtable = NULL;
	pSwapChainVtable = (DWORD_PTR*)swapchain;
	pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];
	g_pHookD3D11Present = (D3D11PresentHook)(DWORD_PTR*)pSwapChainVtable[8];

	dev->Release();
	swapchain->Release();
	devcon->Release();
}

void Hook::detourDirectX()
{
	std::cout << "[*] Adding DirectX Detour" << std::endl;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	// Detours the original fnIDXGISwapChainPresent with our Present
	DetourAttach(&(LPVOID&)g_pHookD3D11Present, (PBYTE)Hook::Present);
	DetourTransactionCommit();
	bPresentHooked = true;
}

HRESULT __fastcall Hook::Present(IDXGISwapChain* pChain, UINT SyncInterval, UINT Flags)
{
	if (!g_bInitialized) {
		std::cout << "[+] Present Hook initialization" << std::endl;
		if (FAILED(Hook::GetDeviceAndCtxFromSwapchain(pChain, &g_pDevice, &g_pContext)))
			return g_pHookD3D11Present(pChain, SyncInterval, Flags);
		g_pSwapChain = pChain;
		DXGI_SWAP_CHAIN_DESC sd;
		pChain->GetDesc(&sd);
		g_Hwnd = sd.OutputWindow;

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(g_Hwnd);
		ImGui_ImplDX11_Init(g_pDevice, g_pContext);
		ImGui::GetIO().ImeWindowHandle = g_Hwnd;

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();
		io.Fonts->Build();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		if (!loadCustomDearImguiFonts(g_hDllModule)) {
			std::cout << "\t[!] Error loading custom fonts" << std::endl;
		}

		//Set g_originalWndProcHandler to the Address of the Original WndProc function
		g_originalWndProcHandler = (WNDPROC)SetWindowLongPtr(g_Hwnd, GWLP_WNDPROC, (LONG_PTR)Hook::hWndProc);

		ID3D11Texture2D* pBackBuffer;

		pChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		g_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
		pBackBuffer->Release();

		g_bInitialized = TRUE;
	}
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();

	ImGui::NewFrame();
	
	customImguiDrawAlways();
	if (g_ShowMenu)
	{
		ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
		ImGui::Begin("Chip Helper##ChipHelperMain", NULL, ImGuiWindowFlags_NoCollapse);
		customImguiDrawMenu();
		ImGui::End();
	}

	ImGui::EndFrame();

	ImGui::Render();

	g_pContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return g_pHookD3D11Present(pChain, SyncInterval, Flags);
}
