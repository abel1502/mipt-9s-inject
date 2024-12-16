#define WIN32_LEAN_AND_MEAN

#include <abel/Error.hpp>
#include <abel/Handle.hpp>

#include <Windows.h>
#include <Richedit.h>
#include <cstdio>
#include <iterator>
#include <ranges>
#include <format>

#include "DllState.hpp"

class child_windows : public std::ranges::view_interface<child_windows> {
protected:
    HWND root;

public:
    struct iterator {
        HWND root;
        HWND current = nullptr;

        using iterator_category = std::input_iterator_tag;
        using value_type = HWND;
        using difference_type = std::ptrdiff_t;

        HWND operator*() const {
            return current;
        }

        iterator &operator++() {
            current = FindWindowExA(root, current, nullptr, nullptr);
            if (!current) {
                root = nullptr;
            }
            return *this;
        }

        bool operator==(const iterator &other) const {
            return root == other.root && current == other.current;
        }

        bool operator!=(const iterator &other) const {
            return !(*this == other);
        }
    };

    child_windows(HWND root) :
        root(root) {
    }

    child_windows(const child_windows &) = default;
    child_windows &operator=(const child_windows &) = default;

    child_windows(child_windows &&) = default;
    child_windows &operator=(child_windows &&) = default;

    iterator begin() {
        return iterator(root);
    }

    iterator end() {
        return iterator(nullptr);
    }
};

class MyDllState : public abel::DllState<MyDllState> {
protected:
    DWORD my_pid = 0;
    HWND target = nullptr;  // TODO: HWND wrapper?
    WNDPROC old_proc = nullptr;

    // Return true to stop
    bool visit_window(HWND hwnd) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid != my_pid) {
            return false;
        }

        target = hwnd;

        return true;
    }

    static LRESULT new_proc(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam
    ) {
        auto &self = get();
        bool should_stop = false;

        if (msg != WM_SETCURSOR && msg != WM_MOUSEMOVE && msg != WM_NCHITTEST) {
            self.log("> {} {} {} {}", (void *)hwnd, msg, wparam, lparam);
        }

        if (msg == WM_CHAR) {
            // TODO?
        }

        if (msg == EM_REPLACESEL) {
            std::string_view text = (const char *)lparam;
            const char *new_text = "<pay $123 to Joe Biden to unlock this feature>";
            self.log("Replace selection: was \"{}\", is \"{}\"", text, new_text);
            lparam = (LPARAM)new_text;
        }

        if (msg == WM_KEYDOWN && wparam == VK_RETURN) {
            self.log("Enter pressed");

            std::array<char, 4096> last_text_arr{};

            GetWindowTextA(hwnd, last_text_arr.data(), (int)last_text_arr.size());

            self.log("Last text: \"{}\"", last_text_arr.data());

            auto last_text = std::string_view(last_text_arr.data());

            if (last_text.ends_with("/stop")) {
                self.log("Stopping...");
                should_stop = true;
            } else if (last_text.ends_with("/blood")) {
                SendMessageA(hwnd, EM_SETBKGNDCOLOR, 0, RGB(230, 0, 0));
            } else if (last_text.ends_with("/white")) {
                SendMessageA(hwnd, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
            }
        }

        LRESULT result = CallWindowProcA(self.old_proc, hwnd, msg, wparam, lparam);

        if (should_stop) {
            MyDllState::destroy();
        }

        return result;
    }

public:
    MyDllState() :
        abel::DllState<MyDllState>("D:\\Workspace\\.MIPT\\KP\\sysprog\\Inject\\dll_log.txt") {

        log("Attached");
    }

    ~MyDllState() {
        if (old_proc) {
            SetWindowLongPtrA(target, GWLP_WNDPROC, (LONG_PTR)old_proc);
        }

        log("Detached");
    }

    void run() noexcept {
        try {
            log("Running...");

            my_pid = GetCurrentProcessId();

            log("Enumerating windows...");

            bool success = !EnumWindows([](HWND hwnd, LPARAM) -> BOOL {\
                return !get().visit_window(hwnd);
            }, 0);

            if (!success) {
                abel::fail("Root window not found");
            }

            log("Root window: {}", (uintptr_t)target);

            HWND text_input_par = FindWindowExA(target, nullptr, "NotepadTextBox", nullptr);
            HWND text_input = FindWindowExA(text_input_par, nullptr, "RichEditD2DPT", nullptr);

            if (!text_input) {
                abel::fail("Text input window not found");
            }

            log("Found textbox child window: {:#016x}", (uintptr_t)text_input);

            target = text_input;

            #if 0
            for (auto hwnd : child_windows(target)) {
                std::array<char, 256> class_name_arr;
                int result = GetClassNameA(hwnd, class_name_arr.data(), (int)class_name_arr.size());
                if (!result) {
                    continue;
                }
                std::string_view class_name = std::string_view(class_name.data(), result);

                log("Found child window: {}", class_name);

                if (class_name == "NotepadTextBox") {
                    // TODO
                }
            }
            #endif

            old_proc = (WNDPROC)SetWindowLongPtrA(target, GWLP_WNDPROC, (LONG_PTR)&new_proc);
            if (!old_proc) {
                abel::fail("Failed to set window proc");
            }

            log("Successfully substituted window proc!");
        } catch (std::exception &e) {
            log("Error: {}", e.what());
        }
    }
};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    //printf("DllMain! %d\n", ul_reason_for_call);
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        try {
            MyDllState::create().run();
            SetLastError(0);
        } catch (std::exception &e) {
            MessageBoxA(NULL, e.what(), "DllToInject", MB_OK);
        }
        break;
    case DLL_PROCESS_DETACH:
        try {
            MyDllState::destroy();
            SetLastError(0);
        } catch (std::exception &e) {
            MessageBoxA(NULL, e.what(), "DllToInject", MB_OK);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
