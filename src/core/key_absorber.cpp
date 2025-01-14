#include "key_absorber.hpp"

#include "err_logger.hpp"
#include "keycode_def.hpp"
#include "keycodecvt.hpp"
#include "mapgate.hpp"

#include <chrono>
#include <windows.h>

#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>

#include "util/debug.hpp"
#include "util/def.hpp"
#include "util/interval_timer.hpp"
#include "util/keybrd.hpp"
#include "util/keystroke_repeater.hpp"
#include "util/winwrap.hpp"

#if defined(DEBUG)
#include <iostream>
#endif


#define KEYUP_MASK 0x0001


/*Absorber Overview
                       _____
                      /     \      ->     _____
  [User's Keyboard]  /       \           /     \

   ____________                         __________
  |            |                       |          |
  |  Keyboard  |   a key is pressed    |    OS    | [Hooked]
  |____________|   ================>>  |__________| LowLevelKeyboardProc

   __________
  |          |  WM_KEYDOWN       save state as variable
  |    OS    |  WM_SYSKEYDOWN        send no message
  |__________|  =============>>   LowLevelKeyboardProc  =========== Other Application

*/

namespace
{
    // Bit-base flags can be used to save memory,
    // but, the variable should be separated 
    // to minimize the overhead of bitwise operation in LowLevelKeyboardProc.
    std::array<bool, 256> g_low_level_state{false} ;
    std::array<bool, 256> g_real_state{false} ;
    std::array<bool, 256> g_state{false} ;  //Keyboard state win-vind understands.
    std::array<bool, 256> g_opened{false} ;

    bool g_absorbed_flag{true} ;

    std::array<std::chrono::system_clock::time_point, 256>
        g_time_stamps{std::chrono::system_clock::now()} ;

    auto uninstaller = [](HHOOK* p_hook) {
        if(p_hook != nullptr) {
            if(*p_hook != NULL) {
                if(!UnhookWindowsHookEx(*p_hook)) {
                    PRINT_ERROR("cannot unhook LowLevelKeyboardProc") ;
                }
                *p_hook = NULL ;
            }

            delete p_hook ;
            p_hook = nullptr ;
        }

        // prohibit to keep pressing after termination.
        for(int i = 0 ; i < 256 ; i ++) {
            if(g_real_state[i]) {
                vind::util::release_keystate(static_cast<vind::KeyCode>(i)) ;
            }
        }
    } ;

    std::unique_ptr<HHOOK, decltype(uninstaller)> p_handle(NULL, uninstaller) ;

    LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if(nCode < HC_ACTION) { //not processed
            return CallNextHookEx(NULL, nCode, wParam, lParam) ;
        }

        auto kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam) ;
        auto code = static_cast<vind::KeyCode>(kbd->vkCode) ;
        if(!(kbd->flags & LLKHF_INJECTED)) {
            // The message is not generated with SendInput.
            auto state = !(wParam & KEYUP_MASK) ;

            g_low_level_state[code] = state ;
            g_time_stamps[code] = std::chrono::system_clock::now() ;

            static auto& instance = vind::core::MapGate::get_instance() ;
            if(auto repcode = vind::core::get_representative_key(code)) {
                if(instance.map_syncstate(repcode, state) ||
                        instance.map_syncstate(code, state)) {
                    return 1 ;
                }

                g_real_state[repcode] = state ;
                g_state[repcode]      = state ;
            }
            else if(instance.map_syncstate(code, state)) {
                return 1 ;
            }

            g_real_state[code] = state ;
            g_state[code]      = state ;
        }

        if(g_opened[code]) {
            return CallNextHookEx(NULL, HC_ACTION, wParam, lParam) ;
        }
        if(g_absorbed_flag) {
            return 1 ;
        }
        return CallNextHookEx(NULL, HC_ACTION, wParam, lParam) ;
    }
}


namespace vind
{
    namespace core
    {
        void install_absorber_hook() {
            g_low_level_state.fill(false) ;
            g_real_state.fill(false) ;
            g_state.fill(false) ;
            g_opened.fill(false) ;

            p_handle.reset(new HHOOK(NULL)) ; //added ownership
            if(p_handle == nullptr) {
                throw RUNTIME_EXCEPT("Cannot alloc a hook handle") ;
            }

            *p_handle = SetWindowsHookEx(
                WH_KEYBOARD_LL,
                LowLevelKeyboardProc,
                NULL, 0
            ) ;

            if(*p_handle == NULL) {
                throw RUNTIME_EXCEPT("KeyAbosorber's hook handle is null") ;
            }
        }

        //
        // It makes the toggle keys behave just like any general keys. 
        // However, the current implementation has a small delay in releasing the state.
        // This is because LowLevelKeyboardProc (LLKP) and SetWindowsHookEx can only
        // capture the down event of the toggle key, not the up event, and the strange event cycle.
        // 
        // (1)
        // For example, we press CapsLock for 3 seconds. Then we will get the following signal in LLKP.
        //
        // Signal in LLKP:
        //
        //      ______________________
        // ____|               _ _ _ _
        //     0    1    2    3
        //
        // Thus, LLKP could not capture up event.
        //
        // (2)
        // So, let's use g_low_level_state to detect if the LLKP has been called or not.
        // Then, if the hook is not called for a while, it will release CapsLock.
        //
        // Signal out of LLKP:
        //
        //      __                          ___________________~~~~_____
        // ____|  |__(about 500ms)_________|                   ~~~~     |____
        //     0                                         1     ....  3 (has some delay)
        //
        // The toggle key is sending me a stroke instead of a key state (e.g. h.... hhhhhhh).
        // This means that there will be a buffer time of about 500ms after the first positive signal.
        // If you assign <CapsLock> to <Ctrl> and press <CapsLock-s>, 
        // as there is a high chance of collision during this buffer time, so it's so bad.
        //
        // (3)
        // As a result, I implemented to be released toggle keys
        // when LLKP has not been called for more than the buffer time.
        // This way, even if the key is actually released, if it is less than 500 ms,
        // it will continue to be pressed, causing a delay.
        //
        //
        // Unfortunately, for keymap and normal mode, the flow of key messages needs
        // to be stopped, and this can only be done using Hook.
        // Therefore, this is a challenging task.  If you have any ideas, we welcome pull requests.
        //
        void refresh_toggle_state() {
            static util::IntervalTimer timer(5'000) ;
            if(!timer.is_passed()) {
                return ;
            }

            static auto toggles = vind::core::get_toggle_keys() ;
            for(auto k : toggles) {
                if(!g_low_level_state[k]) {
                    continue ;
                }

                using namespace std::chrono ;
                if((system_clock::now() - g_time_stamps[k]) > 515ms) {
                    MapGate::get_instance().map_syncstate(k, false) ;
                    util::release_keystate(k) ;

                    g_real_state[k] = false ;
                    g_state[k]      = false ;

                    g_low_level_state[k] = false ;
                }
            }
        }

        bool is_pressed(KeyCode keycode) noexcept {
            return g_state[keycode] ;
        }
        bool is_really_pressed(KeyCode keycode) noexcept {
            return g_real_state[keycode] ;
        }

        KeyLog get_pressed_list() {
            KeyLog::Data res{} ;
            for(KeyCode i = 1 ; i < 255 ; i ++) {
                if(is_pressed(i)) res.insert(i) ;
            }
            return KeyLog(res) ;
        }

        //if this object is not hooked, can call following functions.
        bool is_absorbed() noexcept {
            return g_absorbed_flag ;
        }

        void absorb() noexcept {
            g_absorbed_flag = true ;
        }
        void unabsorb() noexcept {
            g_absorbed_flag = false ;
        }

        void close_some_ports(std::initializer_list<KeyCode>&& keys) noexcept {
            for(auto k : keys) {
                g_opened[k] = true ;
            }
        }
        void close_some_ports(
                std::initializer_list<KeyCode>::const_iterator begin,
                std::initializer_list<KeyCode>::const_iterator end) noexcept {
            for(auto itr = begin ; itr != end ; itr ++) {
                g_opened[*itr] = false ;
            }
        }

        void close_some_ports(std::vector<KeyCode>&& keys) noexcept {
            for(auto k : keys) {
                g_opened[k] = false ;
            }
        }
        void close_some_ports(
                std::vector<KeyCode>::const_iterator begin,
                std::vector<KeyCode>::const_iterator end) noexcept {
            for(auto itr = begin ; itr != end ; itr ++) {
                g_opened[*itr] = false ;
            }
        }

        void close_some_ports(const KeyLog::Data& keys) noexcept {
            for(auto k : keys) {
                g_opened[k] = false ;
            }
        }

        void close_port(KeyCode key) noexcept {
            g_opened[key] = false ;
        }

        void close_all_ports() noexcept {
            g_opened.fill(false) ;
        }

        void close_all_ports_with_refresh() {
            g_opened.fill(false) ;

            //if this function is called by pressed button,
            //it has to send message "KEYUP" to OS (not absorbed).
            for(KeyCode i = 0 ; i < 255 ; i ++) {
                if(g_state[i]) {
                    util::release_keystate(i) ;
                }
            }
        }

        void open_some_ports(std::initializer_list<KeyCode>&& keys) noexcept {
            for(auto k : keys) {
                g_opened[k] = true ;
            }
        }
        void open_some_ports(
                std::initializer_list<KeyCode>::const_iterator begin,
                std::initializer_list<KeyCode>::const_iterator end) noexcept {
            for(auto itr = begin ; itr != end ; itr ++) {
                g_opened[*itr] = true ;
            }
        }

        void open_some_ports(std::vector<KeyCode>&& keys) noexcept {
            for(auto k : keys) {
                g_opened[k] = true ;
            }
        }
        void open_some_ports(
                std::vector<KeyCode>::const_iterator begin,
                std::vector<KeyCode>::const_iterator end) noexcept {
            for(auto itr = begin ; itr != end ; itr ++) {
                g_opened[*itr] = true ;
            }
        }

        void open_some_ports(const KeyLog::Data& keys) noexcept {
            for(auto k : keys) {
                g_opened[k] = true ;
            }
        }

        void open_port(KeyCode key) noexcept {
            g_opened[key] = true ;
        }

        void release_virtually(KeyCode key) noexcept {
            g_state[key] = false ;
        }
        void press_virtually(KeyCode key) noexcept {
            g_state[key] = true ;
        }

        struct InstantKeyAbsorber::Impl {
            bool flag = false ;
        } ;
        InstantKeyAbsorber::InstantKeyAbsorber()
        : pimpl(std::make_unique<Impl>())
        {
            pimpl->flag = is_absorbed() ;
            close_all_ports_with_refresh() ;
            absorb() ;
        }
        InstantKeyAbsorber::~InstantKeyAbsorber() noexcept {
            if(!pimpl->flag) unabsorb() ;
        }
    }
}
