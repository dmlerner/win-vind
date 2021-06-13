#ifndef _WINDOW_UTILITY_HPP
#define _WINDOW_UTILITY_HPP

#include <windows.h>

#include <memory>
#include <unordered_map>

#include "util/def.hpp"

namespace vind
{
    namespace windowutil {
        bool is_visible_hwnd(HWND hwnd) ;
        bool is_window_mode(HWND hwnd, RECT& rect) ;

        void resize(
                HWND hwnd,
                LONG left, LONG top,
                LONG width, LONG height) ;

        void batch_resize(const std::unordered_map<HWND, RECT>& rects) ;

        struct ForegroundInfo {
            HWND hwnd ;
            HMONITOR hmonitor ;
            RECT rect ;

            explicit ForegroundInfo()
            : hwnd(NULL),
              hmonitor(NULL),
              rect{0, 0, 0, 0}
            {
                hwnd = GetForegroundWindow() ;
                if(hwnd == NULL) {
                    throw RUNTIME_EXCEPT("Could not get a position of a mouse cursor.") ;
                }

                hmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST) ;

                if(!GetWindowRect(hwnd, &rect)) {
                    throw RUNTIME_EXCEPT("Could not get a rectangle of a foreground window.") ;
                }
            }
        } ;
    }
}

#endif