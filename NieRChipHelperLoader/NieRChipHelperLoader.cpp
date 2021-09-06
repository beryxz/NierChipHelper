#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>

char* getProcessName(DWORD dwPid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPid);

    DWORD size = MAX_PATH;
    char procName[MAX_PATH];

    if (!QueryFullProcessImageName(hProcess, 0, procName, &size))
    {
        CloseHandle(hProcess);
        return NULL;
    }

    char* pExeName = strrchr(procName, '\\');
    pExeName = (pExeName ? pExeName + 1 : procName);
    std::cout << pExeName << std::endl;
    CloseHandle(hProcess);
    return pExeName;
}

// Returns the PID of procName if exists, 0 otherwise.
// If parentName is not NULL, the process to found has to be child of a process with name equal to parentName.
DWORD getProcessID(const char* procName, const char* parentName = NULL) {
    HANDLE procs = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    DWORD procId = 0;

    PROCESSENTRY32 proc;
    proc.dwSize = sizeof(PROCESSENTRY32);

    if (procs == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(procs, &proc)) {
        do {
            if (!_stricmp(proc.szExeFile, procName)) {
                if (parentName && strcmp(parentName, getProcessName(proc.th32ParentProcessID)))
                    continue;
                
                procId = proc.th32ProcessID;
                break;
            }
        } while (Process32Next(procs, &proc));
    }

    CloseHandle(procs);
    return procId;
}

bool injectDll(const char* procName, const char* filename) {
    HANDLE hProc;
    DWORD procId = getProcessID(procName);
    
    if (procId == 0) {
        return false;
    }

    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);

    if (hProc && hProc != INVALID_HANDLE_VALUE) {
        LPVOID allocMemAddr = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        DWORD res = WriteProcessMemory(hProc, allocMemAddr, filename, strlen(filename) + 1, NULL);
        
        HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, allocMemAddr, 0, NULL);

        if (hThread) {
            CloseHandle(hThread);
        }
    }

    if (hProc) {
        CloseHandle(hProc);
    }

    return true;
}

int main()
{
    const char* procName = "NieRAutomata.exe";
    const char* procParentName = "steam.exe";
    const char* libName = "NieRChipHelper.dll";

    std::cout <<
        R"r(=====================================================================)r" "\n"
        R"r(   _  _ _ ____ ____ ____ _  _ _ ___  _  _ ____ _    ___  ____ ____   )r" "\n"
        R"r(   |\ | | |___ |__/ |    |__| | |__] |__| |___ |    |__] |___ |__/   )r" "\n"
        R"r(   | \| | |___ |  \ |___ |  | | |    |  | |___ |___ |    |___ |  \   )r" "\n"
        R"r(                                                                     )r" "\n"
        R"r(======================================================( beryxz )=====)r" "\n\n";

    // Check for dll in cur dir
    char cwd[FILENAME_MAX];
    GetCurrentDirectory(FILENAME_MAX, cwd);
    std::string libPath(cwd);
    libPath.append("\\");
    libPath.append(libName);

    std::ifstream file(libPath, std::ifstream::in);
    if (!file.is_open())
    {
        std::cout << "[!] " << libName << " not found in current directory. Closing..." << std::endl;
        Sleep(5000);
        return 1;
    }

    // Wait for main process
    std::cout << "[*] Waiting for " << procName << std::endl;
    while (!getProcessID(procName, procParentName))
        Sleep(1000);

    // Load dll
    std::cout << "[+] Process found. Injecting dll in 10s" << std::endl;
    Sleep(10000); // sketchy way of waiting for the game to be loaded
    injectDll(
        procName,
        libPath.c_str()
    );
    std::cout << "[+] Injected!";

    Sleep(2000);
    return 0;
}
