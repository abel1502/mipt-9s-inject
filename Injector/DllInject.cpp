#include "DllInject.hpp"

#include <abel/Thread.hpp>

namespace abel {

[[noinline]] DWORD InjectionCtx::payload(InjectionCtx *ctx) {
    HMODULE h = ctx->func_LoadLibrary(ctx->dll_name.data());
    if (h) {
        return ctx->func_GetLastError();
    }
    return 0;
}

InjectionCtx::InjectionCtx(const char *dllName) {
    errno_t err = strncpy_s(dll_name.data(), dll_name.size(), dllName, dll_name.size() - 1);
    if (err) {
        fail("DLL name too large for injection");
    }
    dll_name[dll_name.size() - 1] = 0;

    std::copy_n((const BYTE *)&payload, shellcode.size(), shellcode.begin());
    if (std::find(shellcode.begin(), shellcode.end(), 0xCC) == shellcode.end()) {
        fail("Shellcode too large for injection");
    }
}

void InjectionCtx::inject(Handle process) {
    auto remote_ptr = process.virtual_alloc<InjectionCtx>(MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!remote_ptr) {
        fail("Failed to allocate memory for injection");
    }

    remote_ptr.write(*this);

    OwningHandle thread = Thread::create_remote(
        process,
        (LPTHREAD_START_ROUTINE)remote_ptr.raw(),
        &remote_ptr.raw()->shellcode
    ).handle;
}

}  // namespace abel
