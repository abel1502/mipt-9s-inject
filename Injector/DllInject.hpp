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
    std::array<char, 0x100> dll_name{};
    decltype(&MessageBoxA) func_MessageBox = &MessageBoxA;
    decltype(&LoadLibraryA) func_LoadLibrary = &LoadLibraryA;
    decltype(&GetLastError) func_GetLastError = &GetLastError;
    std::array<BYTE, 0x100> shellcode;

    static DWORD payload(InjectionCtx *ctx);
    static std::array<BYTE, 0x100> cached_payload;

    static constexpr bool recompute_shellcode = false;
    static constexpr bool dump_shellcode = false;

public:
    InjectionCtx(const char *dllName);

    InjectionCtx(InjectionCtx&&) = delete;
    InjectionCtx(const InjectionCtx&) = delete;
    InjectionCtx& operator=(InjectionCtx&&) = delete;
    InjectionCtx& operator=(const InjectionCtx&) = delete;

    // Returns a handle to the newly created thread
    OwningHandle inject(Handle process);

};

}  // namespace abel
