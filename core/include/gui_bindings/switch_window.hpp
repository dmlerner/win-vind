#ifndef _SWITCH_WINDOW_HPP
#define _SWITCH_WINDOW_HPP

#include "binded_func_with_creator.hpp"

struct SwitchWindow : public BindedFuncWithCreator<SwitchWindow>
{
    static void sprocess(
            const bool first_call,
            const unsigned int repeat_num,
            VKCLogger* const parent_vkclgr,
            const CharLogger* const parent_charlgr) ;
    static const std::string sname() noexcept ;
} ;

#endif
