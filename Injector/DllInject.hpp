#pragma once

#include <abel/Error.hpp>
#include <abel/Handle.hpp>
#include <abel/RemotePtr.hpp>

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

    static DWORD payload(InjectionCtx *ctx);

public:
    InjectionCtx(const char *dllName);

    InjectionCtx(InjectionCtx&&) = delete;
    InjectionCtx(const InjectionCtx&) = delete;
    InjectionCtx& operator=(InjectionCtx&&) = delete;
    InjectionCtx& operator=(const InjectionCtx&) = delete;

    void inject(Handle process);

};

}  // namespace abel
