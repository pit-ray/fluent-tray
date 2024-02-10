#ifndef _FLUENT_TRAY_HPP
#define _FLUENT_TRAY_HPP

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#if defined(DEBUG)
#include <iostream>
#endif

#include <windows.h>

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi")


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

        inline constexpr std::size_t bit2mask(std::size_t bits) noexcept {
            return (static_cast<std::size_t>(1) << bits) - 1 ;
        }

        template <typename Type>
        inline constexpr int type2bit() noexcept {
            return static_cast<int>(sizeof(Type) * CHAR_BIT) ;
        }

        template <typename InType, typename OutType>
        inline void split_bits(InType value, OutType& upper, OutType& lower) noexcept {
            constexpr auto bits = type2bit<OutType>() ;
            constexpr auto lower_mask = util::bit2mask(bits) ;

            upper = static_cast<OutType>(reinterpret_cast<std::size_t>(value) >> bits) ;
            lower = static_cast<OutType>(reinterpret_cast<std::size_t>(value) & lower_mask) ;
        }

        template <typename InType, typename OutType>
        inline void concatenate_bits(InType upper, InType lower, OutType& out) noexcept {
            constexpr auto bits = type2bit<InType>() ;
            constexpr auto lower_mask = util::bit2mask(bits) ;

            auto out_upper = static_cast<std::size_t>(upper) << bits ;
            auto out_lower = static_cast<std::size_t>(lower) & lower_mask ;
            out = reinterpret_cast<OutType>(out_upper | out_lower) ;
        }

        inline unsigned char rgb2gray(const COLORREF& rgb) {
            auto r = GetRValue(rgb) ;
            auto g = GetGValue(rgb) ;
            auto b = GetBValue(rgb) ;
            return static_cast<unsigned char>(0.2126 * r + 0.7152 * g + 0.0722 * b) ;
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
        HMENU hmenu_ ;

        std::function<bool(void)> callback_ ;

    public:
        explicit FluentMenu(
            const std::string& label_text="",
            const std::filesystem::path& icon_path="",
            bool toggle=false,
            const std::function<bool(void)>& callback=[]{return true ;})
        : label_(label_text),
          toggle_(toggle),
          icon_path_(icon_path),
          hwnd_(NULL),
          hmenu_(NULL),
          callback_(callback)
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

            auto style = WS_CHILD | WS_VISIBLE | BS_FLAT | BS_LEFT | BS_OWNERDRAW ;

            hmenu_ = reinterpret_cast<HMENU>(id) ;
            hwnd_ = CreateWindowW(
                TEXT("BUTTON"), label_text_wide.c_str(), style,
                0, 0, 100, 100,
                parent_hwnd, hmenu_,
                hinstance, NULL) ;

            // Hide dash lines when selecting.
            SendMessageW(
                hwnd_, WM_CHANGEUISTATE,
                WPARAM(MAKELONG(UIS_SET, UISF_HIDEFOCUS)), 0) ;
            return hwnd_ != NULL ;
        }

        HWND window_handle() const noexcept {
            return hwnd_ ;
        }

        HMENU menu_handle() const noexcept {
            return hmenu_ ;
        }

        std::size_t id() const noexcept {
            return reinterpret_cast<std::size_t>(hmenu_) ;
        }

        const std::string& label() const noexcept {
            return label_ ;
        }

        bool process_click_event() {
            return callback_() ;
        }
    } ;


    class FluentTray {
    private:
        std::vector<FluentMenu> menus_ ;
        std::vector<bool> lines_ ;

        std::string app_name_ ;
        std::filesystem::path icon_path_ ;

        HWND hwnd_ ;
        HINSTANCE hinstance_ ;
        NOTIFYICONDATAW icon_data_ ;
        HRGN hrgn_ ;

        LONG popup_width_ ;
        LONG popup_height_ ;
        int popup_corner_round_ ;
        Status status_ ;

        std::size_t next_menu_id_ ;

        LONG menu_width_ ;
        LONG menu_height_ ;

        LONG menu_font_size_ ;

        LONG menu_x_offset_ ;
        LONG menu_y_offset_ ;

        LONG menu_label_x_offset_ ;

        LONG menu_x_pad_ ;
        LONG menu_y_pad_ ;

        COLORREF text_color_ ;
        COLORREF back_color_ ;
        COLORREF ash_color_ ;
        HBRUSH back_brash_ ;

        HFONT font_ ;

    public:
        explicit FluentTray(
            const std::string& app_name,
            const std::filesystem::path& icon_path="")
        : menus_(),
          lines_(),
          app_name_(app_name),
          icon_path_(icon_path),
          hinstance_(reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL))),
          hwnd_(NULL),
          icon_data_(),
          hrgn_(NULL),
          popup_width_(200),
          popup_height_(100),
          popup_corner_round_(20),
          status_(Status::STOPPED),
          next_menu_id_(1),
          menu_width_(100),
          menu_height_(100),
          menu_font_size_(12),
          menu_x_offset_(5),
          menu_y_offset_(5),
          menu_label_x_offset_(50),
          menu_x_pad_(10),
          menu_y_pad_(10),
          text_color_(RGB(30, 30, 30)),
          back_color_(RGB(200, 200, 200)),
          ash_color_(RGB(100, 100, 100)),
          back_brash_(NULL),
          font_(NULL)
        {}

        // Copy
        FluentTray(const FluentTray&) = delete ;
        FluentTray& operator=(const FluentTray&) = delete ;

        // Move
        FluentTray(FluentTray&&) = default ;
        FluentTray& operator=(FluentTray&&) = default ;

        ~FluentTray() noexcept {
            if(font_ != NULL) {
                DeleteObject(font_) ;
            }
            if(back_brash_ != NULL) {
                DeleteObject(back_brash_) ;
            }
        }

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

            // To access the this pointer inside the callback function,
            // the address divide into the two part address.
            LONG upper_addr, lower_addr ;
            util::split_bits(this, upper_addr, lower_addr) ;

            SetLastError(0) ;
            if(!SetWindowLongW(hwnd_, 0, upper_addr) && GetLastError() != 0) {
                return false ;
            }
            SetLastError(0) ;
            if(!SetWindowLongW(hwnd_, sizeof(LONG), lower_addr) && GetLastError() != 0) {
                return false ;
            }

            if(!SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA)) {
                return false ;
            }

            // Set rounded window for Windows 11 only.
            auto pref = DWMWCP_ROUND ;
            if(DwmSetWindowAttribute(
                    hwnd_,
                    DWMWA_WINDOW_CORNER_PREFERENCE,
                    &pref, sizeof(pref)) != S_OK) {
                return false ;
            }

            if(!change_icon(icon_path_)) {
                return false ;
            }

            if(!set_font()) {
                return false ;
            }

            if(!set_color()) {
                return false ;
            }

            return true ;
        }

        bool add_menu(
                const std::string& label_text="",
                const std::filesystem::path& icon_path="",
                bool toggle=false,
                const std::function<bool(void)>& callback=[]{return true ;}) {
            FluentMenu menu(
                label_text, icon_path, toggle, callback) ;
            if(!menu.create_menu(hinstance_, hwnd_, next_menu_id_)) {
                return false ;
            }

            menus_.push_back(std::move(menu)) ;
            lines_.push_back(false) ;
            next_menu_id_ ++ ;
            return true ;
        }

        void add_line() {
            if(!lines_.empty()) {
                lines_.back() = true ;
            }
        }

        bool update() {
            MSG msg ;
            get_message(msg) ;

            if(!check_if_foreground()) {
                hide_popup_window() ;
            }

            return true ;
        }

        bool update_parallel(
            std::chrono::milliseconds sleep_time=std::chrono::milliseconds(5)) {

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
                hide_popup_window() ;
            }
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
            hide_popup_window() ;

            return true ;
        }

        bool hide_popup_window() {
            ShowWindow(hwnd_, SW_HIDE) ;
            return true ;
        }

        bool show_popup_window() {
            LONG max_label_length = 0 ;
            for(auto& menu : menus_) {
                if(max_label_length < menu.label().length()) {
                    max_label_length = static_cast<LONG>(menu.label().length()) ;
                }
            }

            // Update the sizes
            menu_width_ = max_label_length * 2 * menu_font_size_ / 3 + menu_x_pad_ ;
            menu_height_ = menu_font_size_ + menu_y_pad_ ;
            popup_width_ = 2 * menu_x_offset_ + menu_width_ ;
            popup_height_ = static_cast<LONG>(
                menus_.size() * (menu_y_offset_ + menu_height_) + menu_y_offset_) ;

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
                    // add 20% offset
                    pos.y = screen_height - (popup_height_ + 12 * taskbar_height / 10) ;
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
                    // add 20% offset
                    pos.x = popup_width_ + 12 * taskbar_width / 10 ;
                }

                pos.y = cursor_pos.y - popup_height_ / 2 ;
            }

            if(!SetWindowPos(
                    hwnd_, HWND_TOP,
                    pos.x, pos.y, popup_width_, popup_height_,
                    SWP_SHOWWINDOW)) {
                return false ;
            }

            /*
            hrgn_ = CreateRoundRectRgn(
                0, 0, popup_width_, popup_height_,
                popup_corner_round_, popup_corner_round_) ;
            if(hrgn_) {
                if(!SetWindowRgn(hwnd_, hrgn_, TRUE)) {
                    return false ;
                }
            }
            /
            */

            auto menu_top = menu_y_offset_ ;
            for(auto& menu : menus_) {
                if(!SetWindowPos(
                        menu.window_handle(), HWND_TOP,
                        menu_x_offset_, menu_top, menu_width_, menu_height_,
                        SWP_SHOWWINDOW)) {
                    return false ;
                }
                menu_top += menu_height_ + menu_y_offset_ ;
            }

            if(!SetForegroundWindow(hwnd_)) {
                return false ;
            }

            return true ;
        }

        bool set_font(
                LONG font_size=0,
                LONG font_weight=0,
                const std::string& font_name="") {
            NONCLIENTMETRICS metrics ;
            metrics.cbSize = sizeof(metrics) ;

            if(!SystemParametersInfo(
                    SPI_GETNONCLIENTMETRICS,
                    metrics.cbSize, &metrics, 0)) {
                return false ;
            }

            auto& logfont = metrics.lfCaptionFont ;
            if(font_size != 0) {
                logfont.lfHeight = font_size ;
            }
            if(font_weight != 0) {
                logfont.lfWeight = font_weight ;
            }

            if(!font_name.empty()) {
                std::wstring font_name_wide ;
                if(!util::string2wstring(font_name, font_name_wide)) {
                    return false ;
                }
                auto dst = logfont.lfFaceName ;

                if(font_name_wide.size() < LF_FACESIZE) {
                    std::memcpy(dst, font_name_wide.c_str(), sizeof(WCHAR) * font_name_wide.length()) ;
                    dst[font_name_wide.size()] = L'\0' ;
                }
                else {
                    std::memcpy(dst, font_name_wide.c_str(), sizeof(WCHAR) * (LF_FACESIZE - 1)) ;
                    dst[LF_FACESIZE - 1] = L'\0' ;
                }
            }

            auto font = CreateFontIndirectW(&logfont) ;
            if(!font) {
                return false ;
            }
            font_ = font ;
            menu_font_size_ = std::abs(logfont.lfHeight) ;

            return true;
        }

        bool set_color(
                const COLORREF& text_color=CLR_INVALID,
                const COLORREF& back_color=CLR_INVALID,
                unsigned char color_decay=16) {
            if(back_color == CLR_INVALID) {
                // Get Taskbar color
                APPBARDATA abd ;
                abd.cbSize = sizeof(abd) ;
                if(!SHAppBarMessage(ABM_GETTASKBARPOS, &abd)) {
                    return false ;
                }

                if(auto dc = GetDC(NULL)) {
                    // Get Taskbar color
                    back_color_ = GetPixel(dc, abd.rc.left + 1, abd.rc.top + 1) ;
                    if(back_color_ == CLR_INVALID) {
                        // if failed, use COLOR_WINDOW color.
                        back_color_ = GetSysColor(COLOR_WINDOW) ;
                    }
                    if(!ReleaseDC(NULL, dc)) {
                        return false ;
                    }
                }
            }
            else {
                back_color_ = back_color ;
            }

            auto back_gray_color_ = util::rgb2gray(back_color_) ;

            unsigned char ash_value = back_gray_color_ ;
            if(back_gray_color_ < 128) {
                ash_value = (std::min)(ash_value + color_decay, 255) ;
            }
            else {
                ash_value = (std::max)(ash_value - color_decay, 0) ;
            }
            std::cout << (int)ash_value << std::endl ;
            ash_color_ = RGB(ash_value, ash_value, ash_value) ;

            if(text_color == CLR_INVALID) {
                text_color_ = GetSysColor(COLOR_WINDOWTEXT) ;
                if(back_gray_color_ < 128) {
                    // if dark background, use light text color.
                    text_color_ = 0x00FFFFFF & ~text_color_ ;
                }
            }
            else {
                text_color_ = text_color ;
            }

            // Create brash handle to draw a background of window.
            if(back_brash_) {
                // Release old handle.
                if(!DeleteObject(back_brash_)) {
                    return false ;
                }
            }
            back_brash_ = CreateSolidBrush(back_color_) ;
            if(back_brash_ == NULL) {
                return false ;
            }

            if(!SetClassLongPtr(
                    hwnd_, GCLP_HBRBACKGROUND,
                    reinterpret_cast<LONG_PTR>(back_brash_))) {
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
                auto upper_addr = GetWindowLongW(hwnd, 0) ;
                if(!upper_addr) {
                    return reinterpret_cast<FluentTray*>(nullptr) ;
                }

                auto lower_addr = GetWindowLongW(hwnd, sizeof(LONG)) ;
                if(!lower_addr) {
                    return reinterpret_cast<FluentTray*>(nullptr) ;
                }

                FluentTray* self ;
                util::concatenate_bits(upper_addr, lower_addr, self) ;
                return self ;
            } ;

            if(msg == WM_DESTROY || msg == WM_QUIT || msg == WM_CLOSE) {
                if(auto self = get_instance()) {
                    // self->status_ = Status::SHOULD_STOP ;
                    self->stop() ;
                    return 0 ;
                }
            }
            else if(msg == WM_ACTIVATE && wparam == WA_INACTIVE) {
                if(auto self = get_instance()) {
                    self->hide_popup_window() ;
                    return 0 ;
                }
            }
            else if(msg == WM_DRAWITEM) {
                if(auto self = get_instance()) {
                    if(!self->draw_menu(reinterpret_cast<LPDRAWITEMSTRUCT>(lparam))) {
                        return FALSE ;
                    }
                    return TRUE ;
                }
            }
            else if(msg == WM_CTLCOLORBTN) {
                if(auto self = get_instance()) {
                    auto menu_idx = self->get_menu_index_from_window(reinterpret_cast<HWND>(lparam)) ;
                    if(menu_idx < 0) {
                        return DefWindowProc(hwnd, msg, wparam, lparam) ;
                    }
                    if(!self->set_color_settings(reinterpret_cast<HDC>(wparam))) {
                        return DefWindowProc(hwnd, msg, wparam, lparam) ;
                    }
                    return reinterpret_cast<LRESULT>(self->back_brash_) ;
                }
            }
            else if(msg == WM_COMMAND) {
                if(auto self = get_instance()) {
                    auto menu_idx = self->get_menu_index_from_id(LOWORD(wparam)) ;
                    if(menu_idx < 0) {
                        return FALSE ;
                    }
                    auto& menu = self->menus_[menu_idx] ;
                    if(!menu.process_click_event()) {
                        self->status_ = Status::SHOULD_STOP ;
                        return FALSE ;
                    }
                    return TRUE ;
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

        int get_menu_index_from_window(HWND hwnd) {
            int i = 0 ;
            for(auto& m : menus_) {
                if(m.window_handle() == hwnd) {
                    return i ;
                }
                i ++ ;
            }
            return -1 ;
        }

        int get_menu_index_from_id(WORD id) {
            int i = 0 ;
            for(auto& m : menus_) {
                if(m.id() == static_cast<std::size_t>(id)) {
                    return i ;
                }
                i ++ ;
            }
            return -1 ;
        }

        bool set_color_settings(HDC hdc) {
            if(SetTextColor(hdc, text_color_) == CLR_INVALID) {
                return false ;
            }
            if(SetBkColor(hdc, back_color_) == CLR_INVALID) {
                return false ;
            }
            return true ;
        }

        bool draw_menu(const LPDRAWITEMSTRUCT& item) {
            auto menu_idx = get_menu_index_from_window(item->hwndItem) ;
            if(menu_idx < 0) {
                return false ;
            }
            auto& menu = menus_[menu_idx] ;

            if(!set_color_settings(item->hDC)) {
                return false ;
            }

            std::wstring label_wide ;
            if(!util::string2wstring(menu.label(), label_wide)) {
                return false ;
            }

            if(font_) {
                if(!SelectObject(item->hDC, font_)) {
                    return false ;
                }
            }

            if(!TextOutW(
                item->hDC,
                item->rcItem.left + menu_label_x_offset_,
                item->rcItem.top + (item->rcItem.bottom - item->rcItem.top - menu_font_size_) / 2,
                label_wide.c_str(), static_cast<int>(label_wide.length()))) {
                return false ;
            }

            if(lines_[menu_idx]) {
                auto original_obj = SelectObject(item->hDC, GetStockObject(DC_PEN)) ;
                if(SetDCPenColor(item->hDC, ash_color_) == CLR_INVALID) {
                    return false ;
                }

                auto lx = item->rcItem.left ;
                auto ly = item->rcItem.bottom - 1 ;
                auto rx = item->rcItem.right ;
                auto ry = ly + 1 ;
                if(!Rectangle(item->hDC, lx, ly, rx, ry)) {
                    return false ;
                }

                if(!SelectObject(item->hDC, original_obj)) {
                    return false ;
                }
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
