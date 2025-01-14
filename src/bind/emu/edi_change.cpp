#include "edi_change.hpp"

#include "bind/mode/change_mode.hpp"
#include "bind/safe_repeater.hpp"
#include "core/g_params.hpp"
#include "core/mode.hpp"
#include "core/ntype_logger.hpp"
#include "edi_change_mode.hpp"
#include "simple_text_register.hpp"
#include "text_util.hpp"
#include "util/def.hpp"
#include "util/keybrd.hpp"

namespace vind
{
    namespace bind
    {
        //ChangeHighlightText (Visual only)
        ChangeHighlightText::ChangeHighlightText()
        : BindedFuncVoid("change_highlight_text")
        {}
        void ChangeHighlightText::sprocess() {
            using util::pushup ;
            pushup(KEYCODE_LCTRL, KEYCODE_X) ;
            if(core::get_global_mode_flags() & core::ModeFlags::VISUAL_LINE) {
                set_register_type(RegType::Lines) ;
            }
            else {
                set_register_type(RegType::Chars) ;
            }
            ToInsert::sprocess(false) ;
        }
        void ChangeHighlightText::sprocess(core::NTypeLogger& parent_lgr) {
            if(!parent_lgr.is_long_pressing()) {
                sprocess() ;
            }
        }
        void ChangeHighlightText::sprocess(const core::CharLogger& UNUSED(parent_lgr)) {
            sprocess() ;
        }


        //ChangeLine
        ChangeLine::ChangeLine()
        : ChangeBaseCreator("change_line")
        {}
        void ChangeLine::sprocess(unsigned int repeat_num) {
            auto res = get_selected_text([] {
                util::pushup(KEYCODE_HOME) ;
                util::pushup(KEYCODE_LSHIFT, KEYCODE_END) ;
                util::pushup(KEYCODE_LCTRL, KEYCODE_C) ;
            }) ;
            if(res.str.empty()) {
                ToInsert::sprocess(false) ;
                return ;
            }

            auto pos = res.str.find_first_not_of(" \t") ; //position except for space or tab
            if(pos == std::string::npos) { //space only
                ToInsertEOL::sprocess(false) ;
                return ;
            }
            util::pushup(KEYCODE_HOME) ;

            safe_for(pos, [] {
                util::pushup(KEYCODE_RIGHT) ;
            }) ;
            ChangeUntilEOL::sprocess(repeat_num, &res) ;
        }
        void ChangeLine::sprocess(core::NTypeLogger& parent_lgr) {
            if(!parent_lgr.is_long_pressing()) {
                sprocess(parent_lgr.get_head_num()) ;
            }
        }
        void ChangeLine::sprocess(const core::CharLogger& UNUSED(parent_lgr)) {
            sprocess(1) ;
        }


        //ChangeChar
        ChangeChar::ChangeChar()
        : ChangeBaseCreator("change_char")
        {}
        void ChangeChar::sprocess(unsigned int repeat_num) {
            safe_for(repeat_num, [] {
                util::pushup(KEYCODE_LSHIFT, KEYCODE_RIGHT) ;
            }) ;

            if(core::get_b("charcache")) {
                util::pushup(KEYCODE_LCTRL, KEYCODE_X) ;
                set_register_type(RegType::Chars) ;
            }
            else {
                util::pushup(KEYCODE_DELETE) ;
            }

            ToInsert::sprocess(false) ;
        }
        void ChangeChar::sprocess(core::NTypeLogger& parent_lgr) {
            if(!parent_lgr.is_long_pressing()) {
                sprocess(parent_lgr.get_head_num()) ;
            }
        }
        void ChangeChar::sprocess(const core::CharLogger& UNUSED(parent_lgr)) {
            sprocess(1) ;
        }


        //ChangeUntilEOL
        ChangeUntilEOL::ChangeUntilEOL()
        : ChangeBaseCreator("change_until_EOL")
        {}
        /* Actually, If N >= 2
         *
         * Command: 2C
         * 
         * If the caret is positioned the head of a line.
         *
         * Original Vim:
         * [Before]
         *      |   AAAAAA
         *          BBBBBB
         *          CCCCCC
         * [After]
         *      |
         *          CCCCC
         *
         * win-vind:
         * [Before]
         *      |   AAAAAA
         *          BBBBBB
         *          CCCCCC
         * [After]
         *         |CCCCCC
         *
         * In future, must fix.
         *
         */
        void ChangeUntilEOL::sprocess(
                unsigned int repeat_num,
                const SelectedTextResult* const exres) {

            safe_for(repeat_num - 1, [] {
                util::pushup(KEYCODE_LSHIFT, KEYCODE_DOWN) ;
            }) ;

            if(!select_line_until_EOL(exres)) {
                clear_clipboard_with_null() ;
            }
            else {
                util::pushup(KEYCODE_LCTRL, KEYCODE_X) ;
                set_register_type(RegType::Chars) ;
            }
            ToInsert::sprocess(false) ;
        }
        void ChangeUntilEOL::sprocess(core::NTypeLogger& parent_lgr) {
            if(!parent_lgr.is_long_pressing()) {
                sprocess(parent_lgr.get_head_num()) ;
            }
        }
        void ChangeUntilEOL::sprocess(const core::CharLogger& UNUSED(parent_lgr)) {
            sprocess(1) ;
        }
    }
}
