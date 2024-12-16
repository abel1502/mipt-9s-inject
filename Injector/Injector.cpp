#include <abel/Handle.hpp>
#include <abel/Process.hpp>
#include <abel/Thread.hpp>
#include <abel/Pipe.hpp>
#include <abel/ArgParse.hpp>
#include <abel/Concurrency.hpp>

#include "DllInject.hpp"

#include <cstdio>
#include <cstdint>
#include <string_view>
#include <string>
#include <span>
#include <vector>
#include <memory>
#include <iterator>

int main() {
    try {
        auto notepad = abel::Process::find("Notepad.exe");

        auto remote_thread = abel::InjectionCtx("D:\\Workspace\\.MIPT\\KP\\sysprog\\Inject\\x64\\Release\\DllToInject.dll").inject(notepad);
        remote_thread.wait();

        if (remote_thread.get_exit_code_thread() != 0) {
            printf("! %d\n", remote_thread.get_exit_code_thread());
            abel::fail_ec("Injected thread failed", remote_thread.get_exit_code_thread());
        }
    } catch (std::exception &e) {
        printf("ERROR! %s (%d)\n", e.what(), GetLastError());
        return -1;
    }

    return 0;
}
