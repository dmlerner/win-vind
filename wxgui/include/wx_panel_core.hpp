#ifndef _WX_PANEL_CORE_HPP
#define _WX_PANEL_CORE_HPP
#include "wxvcdef.hpp"

#include "disable_gcc_warning.hpp"
#include <wx/panel.h>
#include <wx/bookctrl.h>
#include "enable_gcc_warning.hpp"

namespace wxGUI
{
    class PanelCore : public wxPanel
    {
    private:
        wxBookCtrlBase* const pbk ;
        const std::string uip ;

        virtual void translate()      = 0 ;
        virtual void do_load_config() = 0 ;
        virtual void do_save_config() = 0 ;

        void trans_page() ;

    public:
        explicit PanelCore(wxBookCtrlBase* const p_book_ctrl, const std::string ui_path_code) ;
        virtual ~PanelCore() noexcept ;

        void load_config() ;
        void save_config() ;

        PanelCore(PanelCore&&)                 = delete ;
        PanelCore& operator=(PanelCore&&)      = delete ;
        PanelCore(const PanelCore&)            = delete ;
        PanelCore& operator=(const PanelCore&) = delete ;
    } ;
}

#endif
