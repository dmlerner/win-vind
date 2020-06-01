#include "edi_replace.hpp"

#include <windows.h>
#include <iostream>

#include "keybrd_eventer.hpp"
#include "key_absorber.hpp"
#include "vkc_converter.hpp"

namespace EREPUtility {

    template <typename FuncT>
    inline static bool _is_loop_for_input(FuncT&& func) {
        //reset keys downed in order to call this function.
        for(const auto& key : KeyAbsorber::get_downed_list()) {
            if(!KeybrdEventer::is_release_keystate(key)) {
                return false ;
            }
        }

        const auto toggle_keys = KeyAbsorber::get_downed_list() ;
        const KeyLog shifts_log{VKC_LSHIFT, VKC_RSHIFT} ;

        MSG msg ;
        while(true) {
            if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg) ;
                DispatchMessage(&msg) ;
            }

            if(KeyAbsorber::is_downed(VKC_ESC)) {
                return true ;
            }

            const auto log = KeyAbsorber::get_downed_list() - toggle_keys ;

            const auto unshifted_log = log - shifts_log ;
            if(unshifted_log == log) {
                //not shifted
                for(const auto& key : unshifted_log) {
                    //For example, if replace by 'i' and 'i' key is downed,
                    //immediately will call "insert-mode", so release 'i'.
                    if(!KeybrdEventer::is_release_keystate(key)) {
                        return false ;
                    }

                    if(!VKCConverter::get_ascii(key)) {
                        continue ;
                    }

                    if(func(key)) {
                        return true ;
                    }
                }
            }
            else {
                //shifted
                for(const auto& key : unshifted_log) {
                    if(!KeybrdEventer::is_release_keystate(key)) {
                        return false ;
                    }

                    if(!VKCConverter::get_shifted_ascii(key)) {
                        continue ;
                    }

                    if(func(key, true)) {
                        return true ;
                    }
                }
            }

            Sleep(10) ;
        }

        return true ;
    }
}


//EdiNRplaceChar
const std::string EdiNReplaceChar::sname() noexcept
{
    return "edi_n_replace_char" ;
}
bool EdiNReplaceChar::sprocess(const bool first_call)
{
    if(!first_call) {
        return true ;
    }

    return EREPUtility::_is_loop_for_input([](const auto& vkcs, const bool shifted=false) {
        if(!KeybrdEventer::is_pushup(VKC_DELETE)) {
            return false ;
        }

        if(shifted) {
            if(!KeybrdEventer::is_pushup(VKC_LSHIFT, vkcs)) {
                return false ;
            }
        }
        else {
            if(!KeybrdEventer::is_pushup(vkcs)) {
                return false ;
            }
        }

        if(!KeybrdEventer::is_pushup(VKC_LEFT)) {
            return false ;
        }

        return true ; //terminate rooping
    }) ;
}


//EdiNReplaceSequence
const std::string EdiNReplaceSequence::sname() noexcept
{
    return "edi_n_replace_sequence" ;
}
bool EdiNReplaceSequence::sprocess(const bool first_call)
{
    if(!first_call) {
        return true ;
    }

    return EREPUtility::_is_loop_for_input([](const auto& vkcs, const bool shifted=false) {
        if(!KeybrdEventer::is_pushup(VKC_DELETE)) {
            return false ;
        }

        if(shifted) {
            if(!KeybrdEventer::is_pushup(VKC_LSHIFT, vkcs)) {
                return false ;
            }
        }
        else {
            if(!KeybrdEventer::is_pushup(vkcs)) {
                return false ;
            }
        }

        return false ; //continue rooping
    }) ;
}