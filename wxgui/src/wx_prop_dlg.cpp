#include "wx_prop_dlg.hpp"

#include <unordered_map>
#include <string>
#include <sstream>
#include <vector>

#include <windows.h>

#include "disable_gcc_warning.hpp"
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/defs.h>
#include <wx/font.h>
#include <wx/fontenum.h>
#include <wx/generic/stattextg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/window.h>
#include "enable_gcc_warning.hpp"

#include "wx_settings.hpp"
#include "wx_shortcut_apps.hpp"
#include "wx_bindings.hpp"

#include "io_params.hpp"
#include "msg_logger.hpp"
#include "ui_translator.hpp"
#include "wx_constant.hpp"
#include "wx_system_tray.hpp"

#include "common/key_absorber.hpp"
#include "common/mode_manager.hpp"
#include "common_bindings/change_mode.hpp"
#include "win_vind.hpp"

#define KEY_COLOR "#1e96ff"
#define TXT_COLOR "gray"
#define TXT_SIZE "small"
#define CREATE_ITEM(KEY, VAL) "<tt><span size='"TXT_SIZE"' foreground='" KEY_COLOR \
                              "'>" KEY \
                              "</span> <span size='"TXT_SIZE"' foreground='" TXT_COLOR \
                              "'>" VAL "</span></tt>"

namespace wxGUI
{
    using namespace UITrans ;
    constexpr auto APP_NAME = wxT("win-vind") ;

    struct PropDlg::Impl {
        std::vector<PanelCore*> panels{} ;

        wxButton* ok_btn ;
        wxButton* cl_btn ;
        wxButton* ap_btn ;

        wxWindow* const parent ;

        wxFont* font ;

        explicit Impl(wxWindow* const parent_ptr)
        : panels(),
          ok_btn(nullptr),
          cl_btn(nullptr),
          ap_btn(nullptr),
          parent(parent_ptr),
          font(nullptr)
        {
            font = wxFont::New(9, wxFONTFAMILY_TELETYPE, wxFONTFLAG_DEFAULT) ;
        }

        void translate() noexcept {
            ok_btn->SetLabel(trans("buttons/ok")) ;
            cl_btn->SetLabel(trans("buttons/cancel")) ;
            ap_btn->SetLabel(trans("buttons/apply")) ;

            //create font
            font->SetPointSize(ioParams::get_vi("gui_font_size")) ;
            const auto fontname = ioParams::get_vs("gui_font_name") ;
            if(wxFontEnumerator().IsValidFacename(fontname)) {
                font->SetFaceName(fontname) ;
            }
            else {
                ERROR_PRINT("The font name " + fontname + " is not available.") ;
            }

            search_child_recursively(parent) ;
        }

        void search_child_recursively(wxWindow* parent_ptr) {
            for(auto node = parent_ptr->GetChildren().GetFirst() ; node ; node = node->GetNext()) {
                auto ptr = node->GetData() ;
                ptr->SetFont(*font) ;
                search_child_recursively(ptr) ;
            }
        }

        ~Impl() noexcept = default ;
        Impl(Impl&&)                 = delete ;
        Impl& operator=(Impl&&)      = delete ;
        Impl(const Impl&)            = delete ;
        Impl& operator=(const Impl&) = delete ;
    } ;

    PropDlg::PropDlg()
    : wxPropertySheetDialog(nullptr, wxID_ANY, trans("notify/preferences")),
      pimpl(std::make_unique<Impl>(this)),
      ptbi(std::make_unique<SystemTray>(ioParams::get_vs("icon_style"), APP_NAME, this))
    {
        SetIcon(wxIcon(ioParams::get_vs("icon_style"), wxBITMAP_TYPE_ICO)) ;

        SetBackgroundColour(wxColour(*wxWHITE)) ;
        for(auto node = GetChildren().GetFirst() ; node ; node = node->GetNext()) {
            node->GetData()->SetBackgroundColour(wxColour(*wxWHITE)) ;
        }

        pimpl->panels.emplace_back(new SettingsPanel(GetBookCtrl())) ;
        pimpl->panels.emplace_back(new BindingsPanel(GetBookCtrl())) ;
        pimpl->panels.emplace_back(new ShortcutAppsPanel(GetBookCtrl())) ;

        wxSizerFlags flags ;
        flags.Border(wxALL, BORDER).Expand() ;

        auto btn_sizer = new wxBoxSizer(wxHORIZONTAL) ;

        auto usage = new wxGenericStaticText(this, wxID_ANY, wxT("")) ;
        usage->SetLabelMarkup(
            CREATE_ITEM("Up/Down",      "Select,  ")
            CREATE_ITEM("Left/Right",   "Focus,  ")
            CREATE_ITEM("gt/gT",        "Tab,  ")
            CREATE_ITEM("&lt;Esc&gt;",  "Normal,  ")
            CREATE_ITEM("i/a",          "Insert,  ")
            CREATE_ITEM("o",            "Decide")
        ) ;
        btn_sizer->Add(usage, flags) ;

        btn_sizer->AddStretchSpacer() ;

        pimpl->ok_btn = new wxButton(this, wxID_OK,  wxT("OK"), wxDefaultPosition, wxSize(-1, -1)) ;
        btn_sizer->Add(pimpl->ok_btn, flags) ;
        pimpl->cl_btn = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxSize(-1, -1)) ;
        btn_sizer->Add(pimpl->cl_btn, flags) ;
        pimpl->ap_btn = new wxButton(this, wxID_APPLY, wxT("Apply"), wxDefaultPosition, wxSize(-1, -1)) ;
        btn_sizer->Add(pimpl->ap_btn, flags) ;

        GetInnerSizer()->Add(btn_sizer, flags) ; //The inner sizer contains the book control and button sizer.

        LayoutDialog() ;
        Centre() ;
        SetSize(wxSize(WIDTH(), HEIGHT())) ;

        load_config() ;
        save_config() ; //to arrage style

        Bind(wxEVT_BUTTON, [this](auto&) {
            save_config() ;
            win_vind::load_config() ;
            load_config() ;
        }, wxID_APPLY) ;

        Bind(wxEVT_BUTTON, [this](auto&) {
            save_config() ;
            win_vind::load_config() ;
            Show(false) ;
        }, wxID_OK) ;

        Bind(wxEVT_BUTTON, [this](auto&) {
            Show(false) ;
        }, wxID_CANCEL) ;

        Bind(wxEVT_CLOSE_WINDOW, [this](auto&) {
            Show(false) ;
        }) ;

        Bind(wxEVT_CHAR_HOOK, [](auto& e) {
            switch(e.GetKeyCode()) {
                case WXK_ESCAPE: //disable closing window by ESC
                    return ;

                case WXK_RETURN: //disable pressing OK by Enter
                    return ;

                default:
                    e.Skip() ; //others
                    break ;
            }
        }) ;
    }

    PropDlg::~PropDlg() noexcept = default ;

    void PropDlg::save_config() {
        for(auto& p : pimpl->panels) {
            p->save_config() ;
        }
        if(!ioParams::save_config()) {
            ERROR_PRINT("failed saving config") ;
        }
        pimpl->translate() ;
    }

    void PropDlg::load_config() {
        if(!ioParams::load_config()) {
            ERROR_PRINT(" failed loading config") ;
        }
        SetLabel(trans("notify/preferences")) ;

        pimpl->translate() ;
        for(auto& p : pimpl->panels) {
            p->load_config() ;
        }
    }

    bool PropDlg::Show(bool show) {
        static auto l_mode = ModeManager::Mode::Insert ;
        static auto l_is_absorbed = false ;
        static auto l_is_cached   = false ;

        auto result = wxPropertySheetDialog::Show(show) ;

        //true is shown. false is hidden.
        if(show) {
            load_config() ;

            if(!SetForegroundWindow(GetHandle())) {
                ERROR_PRINT("Preferences Window was not brought to the foreground") ;
            } //shown as most top window

            if(!l_is_cached) {
                l_mode = ModeManager::get_mode() ;
                l_is_absorbed = KeyAbsorber::is_absorbed() ;
                l_is_cached = true ;
                MyConfigWindowNormal::sprocess(true, 1, nullptr, nullptr) ;
            }
        }
        else {
            if(l_is_cached) {
                ModeManager::change_mode(l_mode) ;

                KeyAbsorber::close_all_ports_with_refresh() ;
                if(!l_is_absorbed) {
                    KeyAbsorber::unabsorb() ;
                }

                l_is_cached = false ;
            }
        }

        return result ;
    }
}
