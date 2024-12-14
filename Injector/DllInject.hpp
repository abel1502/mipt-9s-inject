#pragma once

#include <abel/Error.hpp>
#include <abel/Handle.hpp>

#include <Windows.h>
#include <array>
#include <utility>
#include <string_view>
#include <cstring>
#include <span>

namespace abel {

class InjectionCtx {
protected:
    std::array<char, 64> dll_name{};
    decltype(&LoadLibraryA) func_LoadLibrary = &LoadLibraryA;
    decltype(&GetLastError) func_GetLastError = &GetLastError;
    std::array<BYTE, 0x100> shellcode;

    static DWORD payload(InjectionCtx *ctx) {
        HMODULE h = ctx->func_LoadLibrary(ctx->dll_name.data());
        if (h) {
            return ctx->func_GetLastError();
        }
        return 0;
    }

public:
    InjectionCtx(const char *dllName) {
        const char *end = strncpy(dll_name.data(), dllName, dll_name.size());
        if (end == dll_name.data() + dll_name.size()) {
            fail("DLL name too large for injection");
        }
        dll_name[dll_name.size() - 1] = 0;

        std::copy_n((const BYTE *)&payload, shellcode.size(), shellcode.begin());
        if (std::find(shellcode.begin(), shellcode.end(), 0xCC) == shellcode.end()) {
            fail("Shellcode too large for injection");
        }
    }

    InjectionCtx(InjectionCtx&&) = delete;
    InjectionCtx(const InjectionCtx&) = delete;
    InjectionCtx& operator=(InjectionCtx&&) = delete;
    InjectionCtx& operator=(const InjectionCtx&) = delete;

    void inject(Handle process) {

    }

};

}  // namespace abel
