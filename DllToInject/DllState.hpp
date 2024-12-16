#pragma once

#include <abel/Handle.hpp>
#include <abel/Error.hpp>

#include <Windows.h>
#include <concepts>
#include <string_view>
#include <utility>
#include <memory>
#include <format>

namespace abel {

template <typename T>
class DllState {
protected:
    static inline std::unique_ptr<T> instance_ = nullptr;

    OwningHandle log_file;

    DllState(const char *name) :
        log_file(OwningHandle(CreateFileA(  // TODO: Wrap in a helper?
            name,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        )).validate()) {
    }

public:
    // static_assert(std::derived_from<T, DllState>, "DllState requires CRTP");

    DllState(const DllState &) = delete;
    DllState &operator=(const DllState &) = delete;
    DllState(DllState &&) = delete;
    DllState &operator=(DllState &&) = delete;

    ~DllState() = default;

    template <typename ... As>
    static T &create(As ... args) {
        if (instance_) {
            fail("DllState already created");
        }
        instance_ = std::make_unique<T>(std::forward<As>(args)...);
        return get();
    }

    static T &get() {
        if (!instance_) {
            fail("DllState not created");
        }
        return *instance_;
    }

    static void destroy() {
        instance_.reset();
    }

    void log_raw(std::string_view message) {
        log_file.write_string(message);
        log_file.write_string("\n");
    }

    template <typename ... As>
    void log(std::format_string<As...> fmt, As &&...args) {
        log_raw(std::format(fmt, std::forward<As>(args)...));
    }

};

}  // namespace abel
