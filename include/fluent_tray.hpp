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

        HWND hwnd_ ;

    public:
        explicit FluentMenu(
            const std::string& label_text="",
            const std::filesystem::path& icon_path="",
            bool toggle=false)
        : label_(label_text),
          toggle_(toggle),
          icon_path_(icon_path),
          hwnd_(NULL)
        {}

        // Copy
        FluentMenu(const FluentMenu&) = default ;
        FluentMenu& operator=(const FluentMenu&) = default ;

        // Move
        FluentMenu(FluentMenu&&) = default ;
        FluentMenu& operator=(FluentMenu&&) = default ;

        ~FluentMenu() noexcept = default ;

        bool create_menu(
                HINSTANCE hinstance,
                HWND parent_hwnd,
                std::size_t id) {

            std::wstring label_text_wide ;
            util::string2wstring(label_, label_text_wide) ;

            hwnd_ = CreateWindowW(
                TEXT("BUTTON"), label_text_wide.c_str(),
                WS_CHILD | WS_VISIBLE,
                0, 0, 100, 100,
                parent_hwnd, reinterpret_cast<HMENU>(id),
                hinstance, NULL) ;
            return hwnd_ != NULL ;
        }

        HWND window_handle() const noexcept {
            return hwnd_ ;
        }

        const std::string& label() const noexcept {
            return label_ ;
        }
    } ;


    class FluentTray {
    private:
        std::vector<FluentMenu> menus_ ;

        std::string app_name_ ;
        std::filesystem::path icon_path_ ;

        HWND hwnd_ ;
        HINSTANCE hinstance_ ;
        NOTIFYICONDATAW icon_data_ ;

        LONG popup_width_ ;
        LONG popup_height_ ;
        Status status_ ;

        std::size_t next_menu_id_ ;

        LONG menu_font_width_ ;
        LONG menu_font_height_ ;

        LONG menu_x_offset_ ;
        LONG menu_y_offset_ ;

        LONG menu_x_pad_ ;
        LONG menu_y_pad_ ;

        COLORREF menu_text_color_ ;
        COLORREF menu_back_color_ ;

        const std::string font_name_ ;
        LOGFONTW logfont_ ;


    public:
        explicit FluentTray(
            const std::string& app_name,
            const std::filesystem::path& icon_path="",
            LONG font_size=12,
            LONG font_weight=FW_NORMAL,
            const std::string& font_name="Segoe UI")
        : menus_(),
          app_name_(app_name),
          icon_path_(icon_path),
          hinstance_(reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL))),
          hwnd_(NULL),
          icon_data_(),
          popup_width_(200),
          popup_height_(100),
          status_(Status::STOPPED),
          next_menu_id_(1),
          menu_font_width_(font_size / 2),
          menu_font_height_(font_size),
          menu_x_offset_(5),
          menu_y_offset_(5),
          menu_x_pad_(20),
          menu_y_pad_(20),
          menu_text_color_(RGB(30, 30, 30)),
          menu_back_color_(RGB(222, 222, 222)),
          logfont_(),
          font_name_(font_name)
        {
            logfont_.lfHeight = font_size ;
            logfont_.lfWeight = font_weight ;
        }

        // Copy
        FluentTray(const FluentTray&) = delete ;
        FluentTray& operator=(const FluentTray&) = delete ;

        // Move
        FluentTray(FluentTray&&) = default ;
        FluentTray& operator=(FluentTray&&) = default ;

        ~FluentTray() noexcept = default ;

        bool create_tray() {
            std::wstring app_name_wide ;
            if(!util::string2wstring(app_name_, app_name_wide)) {
                return false ;
            }

            WNDCLASSW winc ;
            winc.style = CS_HREDRAW | CS_VREDRAW ;
            winc.lpfnWndProc = &FluentTray::callback ;
            winc.cbClsExtra = 0 ;
            winc.cbWndExtra = sizeof(LONG) * 2 ;  // To store two-part address.
            winc.hInstance = hinstance_ ;
            winc.hIcon = LoadIcon(NULL, IDI_APPLICATION) ;
            winc.hCursor = LoadCursor(NULL, IDC_ARROW) ;
            winc.hbrBackground = GetSysColorBrush(COLOR_WINDOW) ;
            winc.lpszMenuName = NULL ;
            winc.lpszClassName = app_name_wide.c_str() ;

            if(!RegisterClassW(&winc)) {
                return false ;
            }

            hwnd_ = CreateWindowExW(
                WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                app_name_wide.c_str(),
                app_name_wide.c_str(),
                WS_POPUPWINDOW,
                0, 0, popup_width_, popup_height_,
                NULL, NULL,
                hinstance_, NULL
            ) ;
            if(!hwnd_) {
                return false ;
            }

            std::cout << "parent_hwnd: " << hwnd_ << std::endl ;

            // To access the this pointer inside the callback function,
            // the address divide into the two part address.
            constexpr auto long_bits = static_cast<int>(sizeof(LONG) * CHAR_BIT) ;
            auto addr = reinterpret_cast<std::size_t>(this) ;
            auto addr1 = static_cast<LONG>(addr >> long_bits) ;
            auto addr2 = static_cast<LONG>(addr & ((static_cast<std::size_t>(1) << long_bits) - 1)) ;
            SetLastError(0) ;
            if(!SetWindowLongW(hwnd_, 0, addr1) && GetLastError() != 0) {
                return false ;
            }
            SetLastError(0) ;
            if(!SetWindowLongW(hwnd_, sizeof(LONG), addr2) && GetLastError() != 0) {
                return false ;
            }

            std::cout << "set: " << std::hex << this << std::endl ;

            if(!SetLayeredWindowAttributes(hwnd_, 0, 200, LWA_ALPHA)) {
                return false ;
            }

            if(!change_icon(icon_path_)) {
                return false ;
            }

            /*
            if(!set_font()) {
                return false ;
            }
            if(!set_paint_settings(hwnd_)) {
                return 0 ;
            }
            static auto hfont = CreateFontIndirectW(&logfont_) ;
            if(!hfont) {
                return false ;
            }
            // static auto hfont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)) ;
            SendMessage(hwnd_, WM_SETFONT, WPARAM(hfont), MAKELPARAM(FALSE, 0));
            */

            return true ;
        }

        bool add_menu(
                const std::string& label_text="",
                const std::filesystem::path& icon_path="",
                bool toggle=false) {
            FluentMenu menu(label_text, icon_path, toggle) ;
            if(!menu.create_menu(hinstance_, hwnd_, next_menu_id_)) {
                return false ;
            }

            menus_.push_back(std::move(menu)) ;
            next_menu_id_ ++ ;
            return true ;
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
            std::chrono::milliseconds sleep_time=std::chrono::milliseconds(50)) {

            // TODO: launch async
            MSG msg ;
            while(true) {
                if(status_ == Status::SHOULD_STOP) {
                    status_ = Status::STOPPED ;
                    break ;
                }

                get_message(msg) ;

                Sleep(static_cast<int>(sleep_time.count())) ;
            }

            if(!check_if_foreground()) {
                hide_popup() ;
            }
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

        void stop() noexcept {
            status_ = Status::SHOULD_STOP ;
        }

        HWND window_handle() const noexcept {
            return hwnd_ ;
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

            std::wstring app_name_wide ;
            if(!util::string2wstring(app_name_, app_name_wide)) {
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
            wcscpy_s(icon_data_.szTip, app_name_wide.c_str()) ;
            icon_data_.dwState = NIS_SHAREDICON ;
            icon_data_.dwStateMask = NIS_SHAREDICON ;

            if(!Shell_NotifyIconW(NIM_ADD, &icon_data_)) {
                return false ;
            }
            hide_popup() ;

            return true ;
        }

        bool show_popup_window() {
            LONG max_label_length = 0 ;
            for(auto& menu : menus_) {
                if(max_label_length < menu.label().length()) {
                    max_label_length = static_cast<LONG>(menu.label().length()) ;
                }
            }
            auto menu_width = max_label_length * menu_font_width_ + menu_x_pad_ ;
            auto menu_height = menu_font_height_ + menu_y_pad_ ;
            auto popup_width = 2 * menu_x_offset_ + menu_width ;
            auto popup_height = static_cast<LONG>(
                menus_.size() * (menu_y_offset_ + menu_height) + menu_y_offset_) ;

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
                    pos.y = screen_height - (popup_height + 11 * taskbar_height / 10) ;
                }
                pos.x = cursor_pos.x - popup_width / 2 ;
            }
            else {  // vertical taskbar
                if(pos.x <= taskbar_width) {
                    //left
                    pos.x = taskbar_width ;
                }
                else {
                    //right
                    // add 10% offset
                    pos.x = popup_width + 11 * taskbar_width / 10 ;
                }

                pos.y = cursor_pos.y - popup_height / 2 ;
            }

            if(!SetWindowPos(
                    hwnd_, HWND_TOP,
                    pos.x, pos.y, popup_width, popup_height,
                    SWP_SHOWWINDOW)) {
                return false ;
            }

            auto menu_top = menu_y_offset_ ;
            for(auto& menu : menus_) {
                if(!SetWindowPos(
                        menu.window_handle(), HWND_TOP,
                        menu_x_offset_, menu_top, menu_width, menu_height,
                        SWP_SHOWWINDOW)) {
                    return false ;
                }
                menu_top += menu_height + menu_y_offset_ ;
            }

            if(!SetForegroundWindow(hwnd_)) {
                return false ;
            }

            return true ;
        }


    private:
        static LRESULT CALLBACK callback(
                HWND hwnd,
                UINT msg,
                WPARAM wparam,
                LPARAM lparam) {
            auto get_instance = [hwnd]() {
                auto addr1 = static_cast<std::size_t>(GetWindowLongW(hwnd, 0)) ;
                auto addr2 = static_cast<std::size_t>(GetWindowLongW(hwnd, sizeof(LONG))) ;
                auto self = reinterpret_cast<FluentTray*>((addr1 << sizeof(LONG) * CHAR_BIT) + addr2) ;
                return self ;
            } ;

            if(msg == WM_DESTROY || msg == WM_QUIT || msg == WM_CLOSE) {
                if(auto self = get_instance()) {
                    // self->status_ = Status::SHOULD_STOP ;
                    self->stop() ;
                    return 0 ;
                }
            }
            else if(msg== WM_ACTIVATE && wparam == WA_INACTIVE) {
                if(auto self = get_instance()) {
                    self->hide_popup() ;
                    return 0 ;
                }
            }
            else if(msg == MESSAGE_ID) {  //On NotifyIcon
                if(auto self = get_instance()) {
                    if(lparam == WM_LBUTTONUP || lparam == WM_RBUTTONUP) {
                        self->show_popup_window() ;
                        return 0 ;
                    }
                }
            }

            return DefWindowProc(hwnd, msg, wparam, lparam) ;
        }

        bool set_color_settings(HWND received_hwnd, HDC hdc) {
            for(auto& menu : menus_) {
                //Color Setting of Popup window
                if(received_hwnd == menu.window_handle()) {
                    if(SetTextColor(hdc, menu_text_color_) == CLR_INVALID) {
                        return false ;
                    }
                    if(SetBkColor(hdc, menu_back_color_) == CLR_INVALID) {
                        return false ;
                    }
                    return true ;
                }
            }
            return true ;
        }

        bool set_font() {
            //default setting
            logfont_.lfWidth = 0 ;
            logfont_.lfEscapement = 0 ;
            logfont_.lfOrientation = 0 ;
            logfont_.lfWeight = FW_LIGHT ;
            logfont_.lfItalic = FALSE ;
            logfont_.lfUnderline = FALSE ;
            logfont_.lfStrikeOut = FALSE ;
            logfont_.lfCharSet = ANSI_CHARSET ;
            logfont_.lfOutPrecision = OUT_TT_ONLY_PRECIS ;
            logfont_.lfClipPrecision = CLIP_DEFAULT_PRECIS ;
            logfont_.lfQuality = ANTIALIASED_QUALITY ;
            logfont_.lfPitchAndFamily = 0 ;

            if(font_name_.empty()) {
                logfont_.lfFaceName[0] = '\0' ;
            }
            else {
                std::wstring font_name_wide ;
                if(!util::string2wstring(font_name_, font_name_wide)) {
                    return false ;
                }
                auto dst = logfont_.lfFaceName ;

                if(font_name_wide.size() < LF_FACESIZE) {
                    std::memcpy(dst, font_name_wide.c_str(), sizeof(WCHAR) * font_name_wide.length()) ;
                    dst[font_name_wide.size()] = L'\0' ;
                }
                else {
                    std::memcpy(dst, font_name_wide.c_str(), sizeof(WCHAR) * (LF_FACESIZE - 1)) ;
                    dst[LF_FACESIZE - 1] = L'\0' ;
                }
            }

            return true;
        }

        bool set_paint_settings(HWND hwnd) {
            PAINTSTRUCT ps ;
            auto hdc = BeginPaint(hwnd, &ps) ;
            if(!hdc) {
                return false ;
            }

            auto font = CreateFontIndirectW(&logfont_) ;
            if(!font) {
                return false ;
            }

            if(!SelectObject(hdc, font)) {
                EndPaint(hwnd, &ps) ;
                return false ;
            }

            if(SetBkColor(hdc, menu_back_color_) == CLR_INVALID) {
                EndPaint(hwnd, &ps) ;
                return false ;
            }

            if(SetTextColor(hdc, menu_text_color_) == CLR_INVALID) {
                EndPaint(hwnd, &ps) ;
                return false ;
            }

            SendMessage(hwnd, WM_SETFONT, WPARAM(font), TRUE);

            if(!EndPaint(hwnd, &ps)) {
                return false ;
            }
            if(!DeleteObject(font)) {
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
    } ;
}

#endif
