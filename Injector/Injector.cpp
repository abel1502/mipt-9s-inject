#include <abel/Handle.hpp>
#include <abel/Process.hpp>
#include <abel/Thread.hpp>
#include <abel/Pipe.hpp>
#include <abel/ArgParse.hpp>
#include <abel/Concurrency.hpp>

#include <cstdio>
#include <cstdint>
#include <string_view>
#include <string>
#include <span>
#include <vector>
#include <memory>
#include <iterator>

using FN_LoadLibraryW = decltype(LoadLibraryW);
using FN_GetLastError = decltype(GetLastError);

BYTE shellcode_data_x64[] = {
    0x40,
    0x53,  // push        rbx
    0x48,
    0x83,
    0xEC,
    0x20,  // sub         rsp,20h
    0x48,
    0x8B,
    0xD9,  // mov         rbx, rcx
    0x48,
    0x81,
    0xC1,
    0x10,
    0x01,
    0x00,
    0x00,  // add         rcx, 110h
    0xFF,
    0x93,
    0x00,
    0x01,
    0x00,
    0x00,  // call        qword ptr[rbx + 100h]
    0x48,
    0x85,
    0xC0,  // test        rax, rax
    0x74,
    0x06,  // je          error
    0x48,
    0x83,
    0xC4,
    0x20,  // add         rsp, 20h
    0x5B,  // pop         rbx
    0xC3,  // ret
    // error:
    0x48,
    0x8B,
    0x83,
    0x08,
    0x01,
    0x00,
    0x00,  // mov         rax, qword ptr[rbx + 108h]
    0x48,
    0x83,
    0xC4,
    0x20,  // add         rsp, 20h
    0x5B,  // pop         rbx
    0x48,
    0xFF,
    0xE0  // jmp         rax
};

typedef struct _INJECTION_CONTEXT {
    BYTE shellcode[0x100];            // 0x000
    FN_LoadLibraryW *rpLoadLibrary;   // 0x100
    FN_GetLastError *rpGetLastError;  // 0x108
    WCHAR DllName[0x100];             // 0x110
} INJECTION_CONTEXT, *PINJECTION_CONTEXT;

DWORD InjectorThread(LPVOID pContext) noexcept {
    PINJECTION_CONTEXT rpInjectionContext = (PINJECTION_CONTEXT)pContext;
    HMODULE h = rpInjectionContext->rpLoadLibrary(rpInjectionContext->DllName);
    if (nullptr != h) {
        return 0;
    }
    return rpInjectionContext->rpGetLastError();
}

int main() {
    // TODO: Read until 0xCCCCCCCC
    std::vector<BYTE> func_body{};
    std::copy_n((BYTE *)&InjectorThread, sizeof(shellcode_data_x64), std::back_inserter(func_body));
    for (unsigned i = 0; i < sizeof(shellcode_data_x64); ++i) {
        printf("%02x %02x\n", func_body[i], shellcode_data_x64[i]);
    }

    return 0;

    int res = -1;

#if 0
    HMODULE h = LoadLibrary(L"DllToInject.dll");    //path is relative cause dll is places in same dir as exe!
                                                    //TODO: need absolute path
    if (nullptr == h) {
        printf("Dll load failed code=%x ! \n", GetLastError());
        goto error1;
    }
    printf("Dll was successfully loaded! \n");
#else
    DWORD tid;

    INJECTION_CONTEXT InjectionContext;
    memset(&InjectionContext, 0, sizeof(InjectionContext));
    memcpy(InjectionContext.shellcode, shellcode_data_x64, sizeof(shellcode_data_x64));
    InjectionContext.rpLoadLibrary = LoadLibrary;
    // TODO: see comment above, for well-known dll asumme that/ p===rp
    InjectionContext.rpGetLastError = GetLastError;
    // TODO: see comment above, for well-known dll asumme that p===rp
    PCWSTR dllName = L"aaaDllToInject.dll";
    size_t dllNameSize = (wcslen(dllName) + 1) * sizeof(WCHAR);
    memcpy(InjectionContext.DllName, dllName, dllNameSize);

    HANDLE hProcess =
        OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    // TODO: check
    PINJECTION_CONTEXT rpInjectionContext = (PINJECTION_CONTEXT)VirtualAllocEx(
        hProcess, NULL, sizeof(*rpInjectionContext), MEM_COMMIT, PAGE_EXECUTE_READWRITE
    );
    // TODO: check
    SIZE_T bytesWritten;
    WriteProcessMemory(hProcess, rpInjectionContext, &InjectionContext, sizeof(InjectionContext), &bytesWritten);
    // TODO: check

#if 0
    HANDLE h = CreateRemoteThread(hProcess, NULL, 0, InjectorThread, rpInjectionContext, 0, &tid);
#else
    HANDLE h = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)rpInjectionContext, rpInjectionContext, 0, &tid);
#endif
    if (nullptr == h) {
        printf("Inject thread creation failed code=%x ! \n", GetLastError());
        goto error1;
    }
    WaitForSingleObject(h, INFINITE);
    DWORD result;
    if (!GetExitCodeThread(h, &result)) {
        printf("GetExitCodeThread failed code =%x ! \n", GetLastError());
        goto error2;
    }

    if (0 != result) {
        printf("Inject thread failed with code =%x ! \n", result);
    }
    res = 0;  // all is ok
#endif

error2:
    CloseHandle(h);
error1:
    return res;
}
