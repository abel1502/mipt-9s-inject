#include "DllInject.hpp"

#include <abel/Thread.hpp>

namespace abel {

__declspec(noinline) DWORD InjectionCtx::payload(InjectionCtx *ctx) {
    //ctx->func_MessageBox(nullptr, "Injected!", "DllInject", MB_OK);
    HMODULE h = ctx->func_LoadLibrary(ctx->dll_name.data());
    if (!h) {
        return ctx->func_GetLastError();
    }
    return 0;
}

std::array<BYTE, 0x100> InjectionCtx::cached_payload = {
    0x40,
    0x53,
    0x48,
    0x83,
    0xEC,
    0x20,
    0x48,
    0x8B,
    0x81,
    0x08,
    0x01,
    0x00,
    0x00,
    0x48,
    0x8B,
    0xD9,
    0xFF,
    0xD0,
    0x48,
    0x85,
    0xC0,
    0x75,
    0x0F,
    0x48,
    0x8B,
    0x83,
    0x10,
    0x01,
    0x00,
    0x00,
    0x48,
    0x83,
    0xC4,
    0x20,
    0x5B,
    0x48,
    0xFF,
    0xE0,
    0x33,
    0xC0,
    0x48,
    0x83,
    0xC4,
    0x20,
    0x5B,
    0xC3,
    0xCC,
    0xCC,
    0x48,
    0x89,
    0x5C,
    0x24,
    0x18,
    0x48,
    0x89,
    0x6C,
    0x24,
    0x20,
    0x56,
    0x57,
    0x41,
    0x56,
    0x48,
    0x81,
    0xEC,
    0x90,
    0x00,
    0x00,
    0x00,
    0x48,
    0x8B,
    0x05,
    0x14,
    0x4D,
    0x00,
    0x00,
    0x48,
    0x33,
    0xC4,
    0x48,
    0x89,
    0x84,
    0x24,
    0x80,
    0x00,
    0x00,
    0x00,
    0x49,
    0x8B,
    0xD8,
    0x48,
    0x8B,
    0xFA,
    0x48,
    0x8B,
    0xE9,
    0x48,
    0x89,
    0x54,
    0x24,
    0x60,
    0x45,
    0x33,
    0xF6,
    0x44,
    0x89,
    0x74,
    0x24,
    0x40,
    0xC7,
    0x44,
    0x24,
    0x20,
    0x40,
    0x00,
    0x00,
    0x00,
    0x33,
    0xD2,
    0x41,
    0xB9,
    0x00,
    0x10,
    0x00,
    0x00,
    0x41,
    0xB8,
    0x18,
    0x02,
    0x00,
    0x00,
    0x48,
    0x8B,
    0xCB,
    0xFF,
    0x15,
    0x4C,
    0x2D,
    0x00,
    0x00,
    0x48,
    0x8B,
    0xF0,
    0x48,
    0x85,
    0xC0,
    0x0F,
    0x84,
    0x15,
    0x01,
    0x00,
    0x00,
    0x48,
    0x85,
    0xDB,
    0x0F,
    0x84,
    0x19,
    0x01,
    0x00,
    0x00,
    0x4C,
    0x89,
    0x74,
    0x24,
    0x20,
    0x41,
    0xB9,
    0x18,
    0x02,
    0x00,
    0x00,
    0x4C,
    0x8B,
    0xC5,
    0x48,
    0x8B,
    0xD0,
    0x48,
    0x8B,
    0xCB,
    0xFF,
    0x15,
    0x65,
    0x2C,
    0x00,
    0x00,
    0x85,
    0xC0,
    0x0F,
    0x84,
    0xD0,
    0x00,
    0x00,
    0x00,
    0x4C,
    0x8D,
    0x8E,
    0x18,
    0x01,
    0x00,
    0x00,
    0x0F,
    0x57,
    0xC0,
    0x0F,
    0x11,
    0x44,
    0x24,
    0x48,
    0x4C,
    0x89,
    0x74,
    0x24,
    0x48,
    0x44,
    0x89,
    0x74,
    0x24,
    0x50,
    0xC7,
    0x44,
    0x24,
    0x40,
    0x02,
    0x00,
    0x00,
    0x00,
    0xC7,
    0x44,
    0x24,
    0x68,
    0x18,
    0x00,
    0x00,
    0x00,
    0x4C,
    0x89,
    0x74,
    0x24,
    0x70,
    0x44,
    0x89,
    0x74,
    0x24,
    0x78,
    0x48,
    0x8D,
    0x44,
    0x24,
    0x50,
    0x48,
    0x89,
    0x44,
    0x24,
    0x30,
};

InjectionCtx::InjectionCtx(const char *dllName) {
    errno_t err = strncpy_s(dll_name.data(), dll_name.size(), dllName, dll_name.size() - 1);
    if (err) {
        fail("DLL name too large for injection");
    }
    dll_name[dll_name.size() - 1] = 0;

    if constexpr (recompute_shellcode) {
        std::copy_n((const BYTE *)&payload, shellcode.size(), shellcode.begin());
        if (std::find(shellcode.begin(), shellcode.end(), 0xCC) == shellcode.end()) {
            fail("Shellcode too large for injection");
        }
    } else {
        shellcode = cached_payload;
    }

    if constexpr (dump_shellcode) {
        printf("{\n");
        for (auto byte : shellcode) {
            printf("0x%02X, ", byte);
        }
        printf("\n};\n");
    }
}

OwningHandle InjectionCtx::inject(Handle process) {
    auto remote_ptr = process.virtual_alloc<InjectionCtx>(MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!remote_ptr) {
        fail("Failed to allocate memory for injection");
    }

    remote_ptr.write(*this);

    return Thread::create_remote(
        process,
        (LPTHREAD_START_ROUTINE)&remote_ptr.raw()->shellcode,
        remote_ptr.raw()
    ).handle;
}

}  // namespace abel
