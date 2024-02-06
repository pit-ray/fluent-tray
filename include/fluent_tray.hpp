#ifndef _FLUENT_TRAY_HPP
#define _FLUENT_TRAY_HPP

#include <chrono>
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#if defined(DEBUG)
#include <iostream>
#endif

#include <windows.h>


namespace fluent_tray
{
    static constexpr int MESSAGE_ID = WM_APP + 25 ;

    namespace util
    {
        bool string2wstring(const std::string& str, std::wstring& wstr) {
            if(str.empty()) {
                wstr.clear() ;
                return true ;
            }

            auto needed_size = MultiByteToWideChar(
                    CP_UTF8, 0,
                    str.c_str(), static_cast<int>(str.length()),
                    NULL, 0) ;
            if(needed_size <= 0) {
                return false ;
            }

            wstr.resize(needed_size) ;
            if(MultiByteToWideChar(
                        CP_UTF8, 0,
                        str.c_str(), static_cast<int>(str.length()),
                        &wstr[0], static_cast<int>(wstr.length())) <= 0) {
                return false;
            }
            return true ;
        }
    }

    enum class Status
    {
        RUNNING,
        SHOULD_STOP,
        STOPPED 
    } ;

    class FluentMenu {
    private:
        std::string label_ ;
        std::filesystem::path icon_path_ ;
        bool toggle_ ;

    public:
        explicit FluentMenu(
            const std::string& label_text="",
            const std::filesystem::path& icon_path="",
            bool toggle=false)
        : label_(label_text),
          toggle_(toggle),
          icon_path_(icon_path)
        {

        }

        // Copy
        FluentMenu(const FluentMenu&) = default ;
        FluentMenu& operator=(const FluentMenu&) = default ;

        // Move
        FluentMenu(FluentMenu&&) = default ;
        FluentMenu& operator=(FluentMenu&&) = default ;

        ~FluentMenu() noexcept = default ;
    } ;


    class FluentTray {
    private:
        std::vector<FluentMenu> menus_ ;

        std::wstring tooltip_text_ ;

        HWND hwnd_ ;
        NOTIFYICONDATAW icon_data_ ;

        LONG popup_width_ ;
        LONG popup_height_ ;
        Status status_ ;

        static LRESULT CALLBACK callback(
                HWND hwnd,
                UINT msg,
                WPARAM wparam,
                LPARAM lparam) {

            auto addr1 = static_cast<std::size_t>(GetWindowLongW(hwnd, 0)) ;
            auto addr2 = static_cast<std::size_t>(GetWindowLongW(hwnd, sizeof(LONG))) ;
            auto self = reinterpret_cast<FluentTray*>((addr1 << sizeof(LONG) * CHAR_BIT) + addr2) ;
            if(!self) {
                return DefWindowProc(hwnd, msg, wparam, lparam) ;
            }

            //massage process
            switch(msg) {
                case WM_CREATE: {
                    return 0 ;
                }
                case WM_DESTROY: {
                    self->status_ = Status::SHOULD_STOP ;
                    return 0 ;
                }

                case WM_QUIT: {
                    self->status_ = Status::SHOULD_STOP ;
                    return 0 ;
                }

                case WM_CLOSE: {
                    self->status_ = Status::SHOULD_STOP ;
                    return 0 ;
                }

                case WM_CTLCOLORSTATIC: {
                    break ;
                }

                case WM_SHOWWINDOW: {
                    if(!wparam) {
                        break ;
                    }

                    return 0 ;
                }

                case MESSAGE_ID: {
                    //On NotifyIcon
                    switch(lparam) {
                        case WM_RBUTTONUP: {
                            if(!self->show_popup_window()) {
                                return 0 ;
                            }
                            break ;
                        }
                    }
                    return 0 ;
                }
            }

            return DefWindowProc(hwnd, msg, wparam, lparam) ;
        }

        bool show_popup_window() {
            POINT cursor_pos ;
            if(!GetCursorPos(&cursor_pos)) {
                return false ;
            }

            RECT work_rect ;
            if(!SystemParametersInfo(
                    SPI_GETWORKAREA, 0, reinterpret_cast<PVOID>(&work_rect), 0)) {
                return false ;
            }

            auto screen_width = GetSystemMetrics(SM_CXSCREEN) ;
            auto screen_height = GetSystemMetrics(SM_CYSCREEN) ;

            auto work_width = work_rect.right - work_rect.left ;
            auto work_height = work_rect.bottom - work_rect.top ;

            auto taskbar_width = screen_width - work_width ;
            auto taskbar_height = screen_height - work_height ;

            auto pos = cursor_pos ;
            if(taskbar_width == 0) {  // horizontal taskbar
                if(cursor_pos.y <= taskbar_height) {
                    //top
                    pos.y = taskbar_height ;
                }
                else {
                    //bottom
                    // add 10% offset
                    pos.y = screen_height - (popup_height_ + 11 * taskbar_height / 10) ;
                }
                pos.x = cursor_pos.x - popup_width_ / 2 ;
            }
            else {  // vertical taskbar
                if(pos.x <= taskbar_width) {
                    //left
                    pos.x = taskbar_width ;
                }
                else {
                    //right
                    // add 10% offset
                    pos.x = popup_width_ + 11 * taskbar_width / 10 ;
                }

                pos.y = cursor_pos.y - popup_height_ / 2 ;
            }

            if(!SetWindowPos(
                    hwnd_, HWND_TOP, pos.x, pos.y,
                    0, 0, SWP_NOSIZE | SWP_SHOWWINDOW)) {
                return false ;
            }

            if(!SetForegroundWindow(hwnd_)) {
                return false ;
            }

            return true ;
        }

        void get_message(MSG& message) {
            if(PeekMessage(&message, hwnd_, 0, 0, PM_REMOVE)) {
                DispatchMessage(&message) ;
            }
        }

        bool check_if_foreground() {
            return GetForegroundWindow() == hwnd_ ;
        }

    public:
        explicit FluentTray(
            const std::string& app_name,
            const std::filesystem::path& icon_path="",
            const std::string& tooltip_text="")
        : menus_(),
          tooltip_text_(),
          hwnd_(NULL),
          icon_data_(),
          popup_width_(100),
          popup_height_(100),
          status_(Status::STOPPED)
        {
            std::wstring app_name_wide ;
            util::string2wstring(app_name, app_name_wide) ;

            util::string2wstring(tooltip_text, tooltip_text_) ;

            auto hinstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL)) ;

            WNDCLASSW winc ;
            winc.style = CS_HREDRAW | CS_VREDRAW ;
            winc.lpfnWndProc = &FluentTray::callback ;
            winc.cbClsExtra = 0 ;
            winc.cbWndExtra = sizeof(LONG) * 2 ;  // To store two-part address.
            winc.hInstance = hinstance,
            winc.hIcon = LoadIcon(NULL, IDI_APPLICATION) ;
            winc.hCursor = LoadCursor(NULL, IDC_ARROW) ;
            winc.hbrBackground = NULL ;
            winc.lpszMenuName = NULL ;
            winc.lpszClassName = app_name_wide.c_str() ;

            if(!RegisterClassW(&winc)) {
                return ;
            }

            hwnd_ = CreateWindowExW(
                WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                app_name_wide.c_str(),
                app_name_wide.c_str(),
                WS_POPUPWINDOW,
                0, 0, popup_width_, popup_height_,
                NULL, NULL,
                hinstance, NULL
            ) ;
            if(!hwnd_) {
                return ;
            }

            // To access the this pointer inside the callback function,
            // the address divide into the two part address.
            constexpr auto long_bits = static_cast<int>(sizeof(LONG) * CHAR_BIT) ;
            auto addr = reinterpret_cast<std::size_t>(this) ;
            auto addr1 = static_cast<LONG>(addr >> long_bits) ;
            auto addr2 = static_cast<LONG>(addr & ((static_cast<std::size_t>(1) << long_bits) - 1)) ;
            SetWindowLongW(hwnd_, 0, addr1) ;
            SetWindowLongW(hwnd_, sizeof(LONG), addr2) ;

            if(!SetLayeredWindowAttributes(hwnd_, 0, 200, LWA_ALPHA)) {
                return ;
            }

            change_icon(icon_path) ;
        }

        // Copy
        FluentTray(const FluentTray&) = delete ;
        FluentTray& operator=(const FluentTray&) = delete ;

        // Move
        FluentTray(FluentTray&&) = default ;
        FluentTray& operator=(FluentTray&&) = default ;

        ~FluentTray() noexcept = default ;

        void append_menu(const FluentMenu& menu) {
            menus_.push_back(menu) ;
        }
        void append_menu(FluentMenu&& menu) {
            menus_.push_back(std::move(menu)) ;
        }

        template <typename ...Args>
        void emplace_menu(Args&&... args) {
            menus_.emplace_back(std::forward<Args>(args)...) ;
        }

        bool update() {
            MSG msg ;
            get_message(msg) ;

            if(!check_if_foreground()) {
                hide_popup() ;
            }

            return true ;
        }

        bool update_parallel(
            std::chrono::milliseconds sleep_time=std::chrono::milliseconds(5)) {

            // TODO: launch async
            MSG msg ;
            while(true) {
                get_message(msg) ;

                Sleep(static_cast<int>(sleep_time.count())) ;

                if(status_ == Status::SHOULD_STOP) {
                    status_ = Status::STOPPED ;
                    break ;
                }
            }

            if(!check_if_foreground()) {
                hide_popup() ;
            }
            return true ;
        }

        bool change_icon(const std::filesystem::path& icon_path) {
            if(icon_data_.cbSize > 0) {
                if(!Shell_NotifyIconW(NIM_DELETE, &icon_data_)) {
                    return false ;
                }
            }

            ZeroMemory(&icon_data_, sizeof(icon_data_)) ;

            if(icon_path.empty()) {
                icon_data_.cbSize = 0 ;
                return true ;
            }

            std::wstring icon_path_wide ;
            if(!util::string2wstring(icon_path.u8string(), icon_path_wide)) {
                return false ;
            }

            icon_data_.cbSize = sizeof(icon_data_) ;
            icon_data_.hWnd = hwnd_ ;
            icon_data_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP ;
            icon_data_.uCallbackMessage = MESSAGE_ID ;
            icon_data_.hIcon = static_cast<HICON>(
                LoadImageW(
                    0, icon_path_wide.c_str(),
                    IMAGE_ICON, 0, 0, LR_LOADFROMFILE)) ;
            wcscpy_s(icon_data_.szTip, tooltip_text_.c_str()) ;
            icon_data_.dwState = NIS_SHAREDICON ;
            icon_data_.dwStateMask = NIS_SHAREDICON ;

            if(!Shell_NotifyIconW(NIM_ADD, &icon_data_)) {
                return false ;
            }
            hide_popup() ;

            return true ;
        }

        bool show_popup() {
            return SetForegroundWindow(hwnd_) != 0 ;
        }

        bool hide_popup() {
            ShowWindow(hwnd_, SW_HIDE) ;
            return true ;
        }

        Status get_status() const noexcept {
            return status_ ;
        }
    } ;
}

#endif
