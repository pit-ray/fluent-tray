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

#ifndef DWMWA_COLOR_DEFAULT
#define DWMWA_WINDOW_CORNER_PREFERENCE static_cast<DWMWINDOWATTRIBUTE>(33)
typedef enum {
    DWMWCP_DEFAULT = 0,
    DWMWCP_DONOTROUND = 1,
    DWMWCP_ROUND = 2,
    DWMWCP_ROUNDSMALL = 3
} DWM_WINDOW_CORNER_PREFERENCE;
#endif

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

        bool wstring2string(const std::wstring& wstr, std::string& str) {
            if(wstr.empty()) {
                str.clear() ;
                return true ;
            }

            auto needed_size = WideCharToMultiByte(
                    CP_UTF8, 0,
                    wstr.c_str(), static_cast<int>(wstr.length()),
                    NULL, 0,
                    NULL, NULL) ;
            if(needed_size <= 0) {
                return false ;
            }

            str.resize(needed_size) ;
            if(WideCharToMultiByte(
                        CP_UTF8, 0,
                        wstr.c_str(), static_cast<int>(wstr.length()),
                        &str[0], static_cast<int>(str.size()),
                        NULL, NULL) <= 0) {
                return false ;
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
            auto lower_mask = util::bit2mask(bits) ;

            upper = static_cast<OutType>(reinterpret_cast<std::size_t>(value) >> bits) ;
            lower = static_cast<OutType>(reinterpret_cast<std::size_t>(value) & lower_mask) ;
        }

        template <typename InType, typename OutType>
        inline void concatenate_bits(InType upper, InType lower, OutType& out) noexcept {
            constexpr auto bits = type2bit<InType>() ;
            auto lower_mask = util::bit2mask(bits) ;

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

    enum class TrayStatus : unsigned char
    {
        RUNNING,
        SHOULD_STOP,
        FAILED,
        STOPPED,
    } ;

    class FluentMenu {
    private:
        std::wstring label_ ;
        HICON hicon_ ;

        bool togglable_ ;
        bool checked_ ;
        std::wstring checkmark_ ;

        HWND hwnd_ ;
        HMENU hmenu_ ;

        bool under_line_ ;

        COLORREF text_color_ ;
        COLORREF back_color_ ;
        COLORREF border_color_ ;
        HBRUSH back_brush_ ;

        std::function<bool(void)> callback_ ;
        std::function<bool(void)> unchecked_callback_ ;

    public:
        explicit FluentMenu(
            bool togglable=false,
            const std::function<bool(void)>& callback=[] {return true ;},
            const std::function<bool(void)>& unchecked_callback=[] {return true ;})
        : label_(),
          hicon_(NULL),
          togglable_(togglable),
          checked_(false),
          checkmark_(),
          hwnd_(NULL),
          hmenu_(NULL),
          under_line_(false),
          text_color_(RGB(0, 0, 0)),
          back_color_(RGB(255, 255, 255)),
          border_color_(RGB(128, 128, 128)),
          back_brush_(NULL),
          callback_(callback),
          unchecked_callback_(unchecked_callback)
        {}

        // Copy
        FluentMenu(const FluentMenu&) = default ;
        FluentMenu& operator=(const FluentMenu&) = default ;

        // Move
        FluentMenu(FluentMenu&&) = default ;
        FluentMenu& operator=(FluentMenu&&) = default ;

        ~FluentMenu() noexcept {
            if(back_brush_) {
                DeleteObject(back_brush_) ;
            }
        }

        bool create_menu(
                HINSTANCE hinstance,
                HWND parent_hwnd,
                std::size_t id,
                const std::filesystem::path& icon_path="",
                const std::string& label_text="",
                const std::string& checkmark="✓") {

            // Convert strings to the wide-strings
            if(!util::string2wstring(label_text, label_)) {
                return false ;
            }
            if(!util::string2wstring(checkmark, checkmark_)) {
                return false ;
            }

            auto style = WS_CHILD | WS_VISIBLE | BS_FLAT | BS_LEFT | BS_OWNERDRAW ;

            hmenu_ = reinterpret_cast<HMENU>(id) ;
            hwnd_ = CreateWindowW(
                TEXT("BUTTON"), label_.c_str(), style,
                0, 0, 100, 100,
                parent_hwnd, hmenu_,
                hinstance, NULL) ;
            if(!hwnd_) {
                return false ;
            }

            // Hide dash lines when selecting.
            SendMessageW(
                hwnd_, WM_CHANGEUISTATE,
                WPARAM(MAKELONG(UIS_SET, UISF_HIDEFOCUS)), 0) ;

            if(!icon_path.empty()) {
                if(!std::filesystem::exists(icon_path)) {
                    return false ;
                }

                std::wstring icon_path_wide ;
                if(!util::string2wstring(icon_path.u8string(), icon_path_wide)) {
                    return false ;
                }

                hicon_ = static_cast<HICON>(LoadImageW(
                        NULL, icon_path_wide.c_str(),
                        IMAGE_ICON, 0, 0, LR_LOADFROMFILE)) ;
                if(!hicon_) {
                    return false ;
                }
            }

            return true ;
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

        bool get_label(std::string& str) const {
            return util::wstring2string(label_, str) ;
        }

        bool process_click_event() {
            if(togglable_) {
                checked_ = !checked_ ;
                if(!checked_) {
                    return unchecked_callback_() ;
                }
            }
            return callback_() ;
        }

        bool is_mouse_over() const {
            POINT pos ;
            if(!GetCursorPos(&pos)) {
                return false ;
            }
            auto detected_hwnd = WindowFromPoint(pos) ;
            if(!detected_hwnd) {
                return false ;
            }
            return detected_hwnd == hwnd_ ;
        }

        void enable_under_line() noexcept {
            under_line_ = true ;
        }
        void disable_under_line() noexcept {
            under_line_ = false ;
        }

        bool togglable() const noexcept {
            return togglable_ ;
        }

        void check() noexcept {
            checked_ = true ;
        }
        void uncheck() noexcept {
            checked_ = false ;
        }
        bool is_checked() const noexcept {
            return checked_ ;
        }

        bool set_color(
                const COLORREF& text_color=CLR_INVALID,
                const COLORREF& back_color=CLR_INVALID,
                const COLORREF& border_color=CLR_INVALID) noexcept {
            if(text_color != CLR_INVALID) {
                text_color_ = text_color ;
            }
            if(back_color != CLR_INVALID) {
                back_color_ = back_color ;
            }
            if(border_color != CLR_INVALID) {
                border_color_ = border_color ;
            }

            // Create brush handle to draw a background of window.
            if(back_brush_) {
                // Release old handle.
                if(!DeleteObject(back_brush_)) {
                    return false ;
                }
            }
            back_brush_ = CreateSolidBrush(back_color_) ;
            if(back_brush_ == NULL) {
                return false ;
            }

            return true ;
        }

        HBRUSH background_brush() const noexcept {
            return back_brush_ ;
        }

        bool calculate_required_size(HFONT font_, SIZE& size) {
            auto hdc = GetDC(hwnd_) ;
            if(font_) {
                if(!SelectObject(hdc, font_)) {
                    return false ;
                }
            }

            if(!GetTextExtentPoint32W(
                    hdc, label_.c_str(),
                    static_cast<int>(label_.length()), &size)) {
                return false ;
            }

            LONG checkmark_size, label_height, icon_size, margin ;
            if(!calculate_layouts(
                    hdc, checkmark_size, label_height, icon_size, margin)) {
                return false ;
            }

            size.cx += margin + checkmark_size + margin + icon_size + margin ;

            return true ;
        }

        bool draw_menu(
                LPDRAWITEMSTRUCT item,
                HFONT font_) {
            if(SetTextColor(item->hDC, text_color_) == CLR_INVALID) {
                return false ;
            }

            if(SetBkColor(item->hDC, back_color_) == CLR_INVALID) {
                return false ;
            }

            if(font_) {
                if(!SelectObject(item->hDC, font_)) {
                    return false ;
                }
            }

            LONG checkmark_size, label_height, icon_size, margin ;
            if(!calculate_layouts(
                    item->hDC,
                    checkmark_size, label_height, icon_size, margin)) {
                return false ;
            }

            auto& rect = item->rcItem ;

            auto y_center = rect.top + (rect.bottom - rect.top) / 2 ;
            auto x = rect.left + margin ;

            if(togglable_ && checked_) {
                if(!TextOutW(
                        item->hDC, x, y_center - label_height / 2, checkmark_.c_str(),
                        static_cast<int>(checkmark_.length()))) {
                    return false ;
                }
            }
            x += checkmark_size + margin ;

            if(hicon_) {
                if(!DrawIconEx(
                        item->hDC, x, y_center - SM_CYICON / 2, hicon_,
                        icon_size, icon_size, 0, NULL, DI_NORMAL)) {
                    return false ;
                }
            }
            x += icon_size + margin ;

            if(!TextOutW(
                    item->hDC, x, y_center - label_height / 2, label_.c_str(),
                    static_cast<int>(label_.length()))) {
                return false ;
            }

            if(under_line_) {
                auto original_obj = SelectObject(item->hDC, GetStockObject(DC_PEN)) ;
                if(SetDCPenColor(item->hDC, border_color_) == CLR_INVALID) {
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

    private:
        bool calculate_layouts(
                HDC hdc,
                LONG& checkmark_size,
                LONG& label_height,
                LONG& icon_size,
                LONG& margin) {
            SIZE size ;
            if(!GetTextExtentPoint32W(hdc, L" ", 1, &size)) {
                return false ;
            }
            checkmark_size = size.cy ;
            margin = size.cy / 2 ;
            label_height = size.cy ;
            icon_size = 4 * label_height / 5 ;
            return true ;
        }

    } ;


    class FluentTray {
    private:
        std::vector<FluentMenu> menus_ ;
        std::vector<bool> mouse_is_over_ ;

        std::wstring app_name_ ;

        HWND hwnd_ ;
        bool visible_ ;
        HINSTANCE hinstance_ ;
        NOTIFYICONDATAW icon_data_ ;

        TrayStatus status_ ;

        std::size_t next_menu_id_ ;

        LONG menu_x_margin_ ;
        LONG menu_y_margin_ ;

        LONG menu_x_pad_ ;
        LONG menu_y_pad_ ;

        COLORREF text_color_ ;
        COLORREF back_color_ ;
        COLORREF ash_color_ ;
        HBRUSH back_brush_ ;

        LONG menu_font_size_ ;
        HFONT font_ ;

    public:
        explicit FluentTray()
        : menus_(),
          mouse_is_over_(),
          app_name_(),
          hinstance_(reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL))),
          hwnd_(NULL),
          visible_(false),
          icon_data_(),
          status_(TrayStatus::STOPPED),
          next_menu_id_(1),
          menu_x_margin_(5),
          menu_y_margin_(5),
          menu_x_pad_(5),
          menu_y_pad_(5),
          text_color_(RGB(30, 30, 30)),
          back_color_(RGB(200, 200, 200)),
          ash_color_(RGB(100, 100, 100)),
          back_brush_(NULL),
          menu_font_size_(0),
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
            if(back_brush_ != NULL) {
                DeleteObject(back_brush_) ;
            }
        }

        bool create_tray(
                const std::string& app_name,
                const std::filesystem::path& icon_path="",
                LONG menu_x_margin=5,
                LONG menu_y_margin=5,
                LONG menu_x_pad=5,
                LONG menu_y_pad=5, 
                BYTE opacity=255,
                bool round_corner=true) {
            if(!util::string2wstring(app_name, app_name_)) {
                return false ;
            }

            menu_x_margin_ = menu_x_margin ;
            menu_y_margin_ = menu_y_margin ;
            menu_x_pad_ = menu_x_pad ;
            menu_y_pad_ = menu_y_pad ;

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
            winc.lpszClassName = app_name_.c_str() ;

            if(!RegisterClassW(&winc)) {
                return false ;
            }

            hwnd_ = CreateWindowExW(
                WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                app_name_.c_str(),
                app_name_.c_str(),
                WS_POPUPWINDOW,
                0, 0, 100, 100,
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

            if(!SetLayeredWindowAttributes(hwnd_, 0, opacity, LWA_ALPHA)) {
                return false ;
            }

            // Set rounded window for Windows 11 only.
            if(round_corner) {
                auto pref = DWMWCP_ROUND ;
                if(DwmSetWindowAttribute(
                        hwnd_,
                        DWMWA_WINDOW_CORNER_PREFERENCE,
                        &pref, sizeof(pref)) != S_OK) {
                    return false ;
                }
            }

            if(!change_icon(icon_path)) {
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
                bool togglable=false,
                const std::string& checkmark="✓",
                const std::function<bool(void)>& callback=[] {return true ;},
                const std::function<bool(void)>& unchecked_callback=[] {return true ;}) {
            FluentMenu menu(togglable, callback, unchecked_callback) ;
            if(!menu.create_menu(
                    hinstance_, hwnd_, next_menu_id_,
                    icon_path, label_text, checkmark)) {
                return false ;
            }

            menus_.push_back(std::move(menu)) ;
            mouse_is_over_.push_back(false) ;
            next_menu_id_ ++ ;
            return true ;
        }

        void add_line() {
            if(!menus_.empty()) {
                menus_.back().enable_under_line() ;
            }
        }

        bool update() {
            if(status_ == TrayStatus::FAILED) {
                status_ = TrayStatus::STOPPED ;
                return false ;
            }

            MSG msg ;
            get_message(msg) ;

            if(GetForegroundWindow() != hwnd_ && visible_) {
                hide_popup_window() ;
            }

            for(std::size_t i = 0 ; i < menus_.size() ; i ++) {
                auto& menu = menus_[i] ;
                if(menu.is_mouse_over()) {
                    if(!mouse_is_over_[i]) {
                        if(!menu.set_color(
                                text_color_, ash_color_, ash_color_)) {
                            return false ;
                        }
                        if(!InvalidateRect(menu.window_handle(), NULL, TRUE)) {
                            return false ;
                        }
                    }
                    mouse_is_over_[i] = true ;
                }
                else {
                    if(mouse_is_over_[i]) {
                        if(!menu.set_color(
                                text_color_, back_color_, ash_color_)) {
                            return false ;
                        }
                        if(!InvalidateRect(menu.window_handle(), NULL, TRUE)) {
                            return false ;
                        }
                    }
                    mouse_is_over_[i] = false ;
                }
            }

            return true ;
        }

        bool update_with_loop(
            std::chrono::milliseconds sleep_time=std::chrono::milliseconds(1)) {

            while(true) {
                if(status_ == TrayStatus::SHOULD_STOP) {
                    status_ = TrayStatus::STOPPED ;
                    break ;
                }

                if(!update()) {
                    return false ;
                }

                Sleep(static_cast<int>(sleep_time.count())) ;
            }
            return true ;
        }

        TrayStatus get_status() const noexcept {
            return status_ ;
        }

        void stop() noexcept {
            status_ = TrayStatus::SHOULD_STOP ;
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

            if(!std::filesystem::exists(icon_path)) {
                return false ;
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
                    NULL, icon_path_wide.c_str(),
                    IMAGE_ICON, 0, 0, LR_LOADFROMFILE)) ;
            wcscpy_s(icon_data_.szTip, app_name_.c_str()) ;
            icon_data_.dwState = NIS_SHAREDICON ;
            icon_data_.dwStateMask = NIS_SHAREDICON ;

            if(!Shell_NotifyIconW(NIM_ADD, &icon_data_)) {
                return false ;
            }
            hide_popup_window() ;

            return true ;
        }

        bool hide_popup_window() {
            if(!ShowWindow(hwnd_, SW_HIDE)) {
                return false ;
            }
            visible_ = false ;

            std::fill(mouse_is_over_.begin(), mouse_is_over_.end(), false) ;
            return true ;
        }

        bool show_popup_window() {
            LONG max_label_size = 0 ;
            for(auto& menu : menus_) {
                SIZE size ;
                if(!menu.calculate_required_size(font_, size)) {
                    return false ;
                }
                if(max_label_size < size.cx) {
                    max_label_size = size.cx ;
                }
            }

            // Update the sizes
            auto menu_width = max_label_size + 2 * menu_x_pad_ ;
            auto menu_height = menu_font_size_ + 2 * menu_y_pad_ ;
            auto popup_width = 2 * menu_x_margin_ + menu_width ;
            auto popup_height = static_cast<LONG>(
                menus_.size() * (menu_y_margin_ + menu_height) + menu_y_margin_) ;

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
                    pos.y = screen_height - (popup_height + 12 * taskbar_height / 10) ;
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
                    // add 20% offset
                    pos.x = popup_width + 12 * taskbar_width / 10 ;
                }

                pos.y = cursor_pos.y - popup_height / 2 ;
            }

            if(!SetWindowPos(
                    hwnd_, HWND_TOP,
                    pos.x, pos.y, popup_width, popup_height,
                    SWP_SHOWWINDOW)) {
                return false ;
            }

            for(LONG i = 0 ; i < menus_.size() ; i ++) {
                auto& menu = menus_[i] ;
                auto y = menu_y_margin_ + i * (menu_height + menu_y_margin_) ;
                if(!SetWindowPos(
                        menu.window_handle(), HWND_TOP,
                        menu_x_margin_, y,
                        menu_width, menu_height,
                        SWP_SHOWWINDOW)) {
                    return false ;
                }

                if(!menu.set_color(text_color_, back_color_, ash_color_)) {
                    return false ;
                }
            }
            std::fill(mouse_is_over_.begin(), mouse_is_over_.end(), false) ;


            if(!SetForegroundWindow(hwnd_)) {
                return false ;
            }

            visible_ = true ;

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
            else {
                logfont.lfHeight = 20 ;
            }
            if(font_weight != 0) {
                logfont.lfWeight = font_weight ;
            }
            else {
                logfont.lfWeight = FW_MEDIUM ;
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
                unsigned char color_decay=10) {
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
                ash_value = static_cast<decltype(ash_value)>(
                    (std::min)(ash_value + color_decay, 255)) ;
            }
            else {
                ash_value = static_cast<decltype(ash_value)>(
                    (std::max)(ash_value - color_decay, 0)) ;
            }
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

            if(back_brush_) {
                // Release old handle.
                if(!DeleteObject(back_brush_)) {
                    return false ;
                }
            }
            back_brush_ = CreateSolidBrush(back_color_) ;
            if(back_brush_ == NULL) {
                return false ;
            }

            if(!SetClassLongPtr(
                    hwnd_, GCLP_HBRBACKGROUND,
                    reinterpret_cast<LONG_PTR>(back_brush_))) {
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
                    self->stop() ;
                    return 0 ;
                }
            }
            else if(msg == WM_ACTIVATE && wparam == WA_INACTIVE) {
                if(auto self = get_instance()) {
                    if(!self->hide_popup_window()) {
                        self->fail() ;
                    }
                    return 0 ;
                }
            }
            else if(msg == WM_DRAWITEM) {
                if(auto self = get_instance()) {
                    auto item = reinterpret_cast<LPDRAWITEMSTRUCT>(lparam) ;
                    auto menu_idx = self->get_menu_index_from_window(item->hwndItem) ;
                    if(menu_idx < 0) {
                        return FALSE ;
                    }
                    auto& menu = self->menus_[menu_idx] ;
                    if(!menu.draw_menu(item, self->font_)) {
                        self->fail() ;
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
                    auto& menu = self->menus_[menu_idx] ;
                    return reinterpret_cast<LRESULT>(menu.background_brush()) ;
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
                        self->stop() ;
                        return FALSE ;
                    }
                    if(menu.togglable()) {
                        // Update the toggle menu for checkmark
                        if(!InvalidateRect(menu.window_handle(), NULL, TRUE)) {
                            return false ;
                        }
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

        void get_message(MSG& message) {
            if(PeekMessage(&message, hwnd_, 0, 0, PM_REMOVE)) {
                DispatchMessage(&message) ;
            }
        }

        void fail() noexcept {
            status_ = TrayStatus::FAILED ;
        }
    } ;
}

#endif
