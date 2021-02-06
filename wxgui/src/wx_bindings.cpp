#include "wx_bindings.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <unordered_map>
#include <array>

#include "disable_gcc_warning.hpp"
#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include "enable_gcc_warning.hpp"

#include "disable_gcc_warning.hpp"
#include <nlohmann/json.hpp>
#include "enable_gcc_warning.hpp"

#include "io_params.hpp"
#include "msg_logger.hpp"
#include "path.hpp"
#include "ui_translator.hpp"
#include "utility.hpp"
#include "wx_constant.hpp"

namespace wxGUI
{
    namespace BindingsEvt {
        enum : unsigned int {
            DEFAULT = 10200,
            SELECT_FUNC,
            ADD_CMD,
            DEL_CMD,
            CHOICE_MODE,
            CHOIDE_LINKED_MODE,
            EDIT_WITH_VIM,
        } ;
    }

    static const std::array<std::string, 8> g_modes_label {
        "GUI Normal",
        "GUI Insert",
        "GUI Visual",

        "Edi Normal",
        "Edi Insert",
        "Edi Visual",
        "Edi Visual Line",

        "Command"
    } ;

    static const std::array<std::string, 8> g_modes_key {
        "guin",
        "guii",
        "guiv",

        "edin",
        "edii",
        "ediv",
        "edivl",

        "cmd"
    } ;

    static const auto g_modes_last_idx = g_modes_key.size() ;

    struct BindingsPanel::Impl {
        wxSearchCtrl* search = nullptr ;
        wxListBox* func_list = nullptr ;

        wxStaticText* id_label = nullptr ;
        wxStaticText* id       = nullptr ;

        wxChoice* mode        = nullptr ;
        wxChoice* linked_mode = nullptr ;

        wxStaticText* mode_related_text = nullptr ;

        wxListBox* cmds     = nullptr ;
        wxTextCtrl* new_cmd = nullptr ;

        wxBoxSizer* right_sizer      = nullptr ;
        wxStaticBoxSizer* cmds_sizer = nullptr ;

        wxButton* cmd_add_btn   = nullptr ;
        wxButton* cmd_del_btn   = nullptr ;
        wxButton* edit_with_vim = nullptr ;
        wxButton* def_btn       = nullptr ;

        nlohmann::json parser{} ;

        // mode_links states copying from each modes.
        // Each element has following maps.
        using linkmap_t = std::array<std::size_t, 8> ;
        std::unordered_map<std::string, linkmap_t> mode_links{} ;

        void initialize() {
            mode_links.clear() ;
        }
        const nlohmann::json& get_selected_func_json() {
            const auto index = func_list->GetSelection() ;
            if(index == wxNOT_FOUND) {
                throw RUNTIME_EXCEPT("Cannot get a selection of func_list.") ;
            }
            return parser.at(index) ;
        }

        const std::string get_selected_func_name() {
            return get_selected_func_json()["name"].get<std::string>() ;
        }

        void update_static_obj() {
            decltype(auto) obj = get_selected_func_json() ;
            const auto& idstr = obj["name"].get<std::string>() ;
            id->SetLabelText(idstr) ;

            //detect which a mode a target mode copys.
            linkmap_t linkmap ;
            linkmap.fill(g_modes_last_idx) ;
            bool initial_select_flag = false ;

            for(std::size_t i = 0 ; i < g_modes_last_idx ; i ++) {
                const auto& val = obj[g_modes_key[i]] ;
                if(val.empty()) {
                    continue ;
                }

                const auto& head = val[0].get<std::string>() ;
                const auto bracket1 = head.find_first_of('<') ;
                const auto bracket2 = head.find_last_of('>') ;

                if(bracket1 != std::string::npos && bracket2 != std::string::npos) {
                    try {
                        const auto inner_str = head.substr(bracket1 + 1, bracket2 - 1) ;
                        for(std::size_t j = 0 ; j < g_modes_last_idx ; j ++) {
                            if(inner_str == g_modes_key[j]) {
                                linkmap[i] = j ;
                                break ;
                            }
                        }
                    }
                    catch(const std::out_of_range&) {} //ignore other system keys
                }
                if(!initial_select_flag) {
                    mode->SetSelection(i) ;
                    initial_select_flag = true ;
                }
            }
            mode_links[idstr] = std::move(linkmap) ;
        }

        void update_linked_mode() {
            const auto mode_idx = mode->GetSelection() ;
            if(mode_idx == wxNOT_FOUND) {
                throw RUNTIME_EXCEPT("Could not get a selection of mode choices.") ;
            }
            try {
                const auto links = mode_links.at(get_selected_func_name()) ;
                try {
                    linked_mode->SetSelection(links.at(mode_idx)) ;
                }
                catch(const std::out_of_range&) {
                    throw RUNTIME_EXCEPT("Invalid mode index is passed.") ;
                }
            }
            catch(const std::out_of_range&) {
                throw RUNTIME_EXCEPT("The link dependencies of a selected function are not registered.") ;
            }
        }

        bool update_bindings_state() {
            const auto linked_idx = linked_mode->GetSelection() ;
            if(linked_idx == wxNOT_FOUND) {
                throw RUNTIME_EXCEPT("Could not get a selection of liked mode choices.") ;
            }

            if(linked_idx == g_modes_last_idx) {
                cmds->Enable() ;
                new_cmd->Enable() ;
                cmd_add_btn->Enable() ;
                cmd_del_btn->Enable() ;
                return true ;
            }
            else {
                cmds->Disable() ;
                new_cmd->Disable() ;
                cmd_add_btn->Disable() ;
                cmd_del_btn->Disable() ;
                return false ;
            }
        }

        void update_bindings() {
            cmds->Clear() ;
            update_linked_mode() ;
            if(update_bindings_state()) {
                auto mode_idx = mode->GetSelection() ;
                if(mode_idx == wxNOT_FOUND) {
                    ERROR_PRINT("Mode Choise is not selected.") ;
                    return ;
                }

                decltype(auto) obj = get_selected_func_json() ;
                auto modekey = g_modes_key[mode_idx] ;
                right_sizer->Show(cmds_sizer) ;
                for(const auto cmd : obj[modekey]) {
                    cmds->Append(cmd.get<std::string>()) ;
                }
            }
        }

        void update_func_list() {
            func_list->Clear() ;
            for(const auto& obj : parser) {
                try {
                    func_list->Append(wxString::FromUTF8(obj.at(
                        ioParams::get_vs("ui_lang")
                    ).get<std::string>().c_str())) ;
                }
                catch(const std::exception&) {continue ;}
            }
        }

        void update_labels() noexcept {
            id_label->SetLabel(UITrans::trans("notify/preferences/bindings/id")) ;
            cmds_sizer->GetStaticBox()->SetLabel(UITrans::trans("notify/preferences/bindings/cmds")) ;

            cmd_add_btn->SetLabel(UITrans::trans("buttons/add")) ;
            cmd_del_btn->SetLabel(UITrans::trans("buttons/del")) ;
            def_btn->SetLabel(UITrans::trans("buttons/default")) ;
        }
    } ;

    BindingsPanel::BindingsPanel(wxBookCtrlBase* const p_book_ctrl)
    : PanelCore(p_book_ctrl, "notify/preferences/bindings"),
      pimpl(std::make_unique<Impl>())
    {
        wxSizerFlags flags ;
        flags.Border(wxALL, BORDER) ;

        auto root_sizer = new wxBoxSizer(wxHORIZONTAL) ;
        {
            const auto c_left_width = static_cast<int>(WIDTH() * 0.4) ;
            auto left_sizer = new wxBoxSizer(wxVERTICAL) ;
            pimpl->search = new wxSearchCtrl(
                    this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                    wxSize(c_left_width, -1)) ;
            left_sizer->Add(pimpl->search, flags) ;

            pimpl->func_list = new wxListBox(
                this, BindingsEvt::SELECT_FUNC, wxDefaultPosition,
                wxSize(c_left_width, HEIGHT()), wxArrayString{}, wxLB_SINGLE
            ) ;
            left_sizer->Add(pimpl->func_list, flags) ;

            root_sizer->Add(left_sizer, flags) ;
        }
 
        //load bindings from json
        do_load_config() ;

        root_sizer->AddStretchSpacer() ;
        {
            pimpl->right_sizer = new wxBoxSizer(wxVERTICAL) ;
            const auto c_right_width = static_cast<int>(WIDTH() * 0.5) ;
            {
               auto id_sizer = new wxBoxSizer(wxHORIZONTAL) ;
                auto fl = flags ;
                fl.Align(wxALIGN_CENTER_VERTICAL) ;
                pimpl->id_label = new wxStaticText(this, wxID_ANY, wxT("Identifier: ")) ;
                id_sizer->Add(pimpl->id_label, std::move(fl)) ;
                pimpl->id = new wxStaticText(this, wxID_ANY, wxT("undefined")) ;
                id_sizer->Add(pimpl->id, flags) ;
                pimpl->right_sizer->Add(id_sizer, flags) ;
            }
            {
                auto mode_sizer = new wxBoxSizer(wxHORIZONTAL) ;
                wxArrayString modes ;

                for(auto& l : g_modes_label) {
                    modes.Add(wxString(l)) ;
                }
                pimpl->mode = new wxChoice(
                        this, BindingsEvt::CHOICE_MODE, wxDefaultPosition,
                        wxDefaultSize, modes) ;
                pimpl->mode->SetSelection(0) ;
                mode_sizer->Add(pimpl->mode, flags) ;

                pimpl->mode_related_text = new wxStaticText(this, wxID_ANY, wxT("copy from")) ;
                mode_sizer->Add(pimpl->mode_related_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, BORDER) ;

                modes.Insert(wxT("None"), g_modes_last_idx) ;
                pimpl->linked_mode = new wxChoice(
                        this, BindingsEvt::CHOIDE_LINKED_MODE, wxDefaultPosition,
                        wxDefaultSize, modes) ;
                pimpl->linked_mode->SetSelection(g_modes_last_idx) ;
                mode_sizer->Add(pimpl->linked_mode, flags) ;

                pimpl->right_sizer->Add(mode_sizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, BORDER) ;
            }
            {
                pimpl->cmds_sizer = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Commands")) ;
                pimpl->cmds = new wxListBox(
                    this, wxID_ANY, wxDefaultPosition,
                    wxSize(c_right_width, HEIGHT() / 4),
                    wxArrayString{}, wxLB_SINGLE
                ) ;
                pimpl->cmds_sizer->Add(pimpl->cmds, flags) ;

                auto ctrls_sizer = new wxBoxSizer(wxHORIZONTAL) ;
                auto fl = flags ;
                fl.Align(wxALIGN_CENTER_VERTICAL) ;
                pimpl->new_cmd = new wxTextCtrl(
                    this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                    wxSize(c_right_width / 2, wxDefaultCoord)
                ) ;
                ctrls_sizer->Add(pimpl->new_cmd, fl) ;

                pimpl->cmd_add_btn = new wxButton(this, BindingsEvt::ADD_CMD, wxT("Add")) ;
                ctrls_sizer->Add(pimpl->cmd_add_btn, fl) ;
                pimpl->cmd_del_btn = new wxButton(this, BindingsEvt::DEL_CMD, wxT("Delete")) ;
                ctrls_sizer->Add(pimpl->cmd_del_btn, fl) ;
                pimpl->cmds_sizer->Add(ctrls_sizer, flags) ;
                pimpl->right_sizer->Add(pimpl->cmds_sizer, flags) ;
            }

            pimpl->right_sizer->AddStretchSpacer() ;
            pimpl->edit_with_vim = new wxButton(
                    this, BindingsEvt::EDIT_WITH_VIM, wxT("Edit With Vim"),
                    wxDefaultPosition, wxSize(c_right_width, HEIGHT() / 10)) ;
            pimpl->right_sizer->Add(pimpl->edit_with_vim, 0, wxALL | wxEXPAND, BORDER) ;

            pimpl->right_sizer->AddStretchSpacer() ;
            auto df = flags;
            pimpl->def_btn = new wxButton(this, BindingsEvt::DEFAULT, wxT("Return to Default")) ;
            pimpl->right_sizer->Add(pimpl->def_btn, std::move(df.Right())) ;

            root_sizer->Add(pimpl->right_sizer, 0, wxEXPAND | wxALL | wxALIGN_CENTRE_HORIZONTAL, BORDER) ;
        }
        SetSizerAndFit(root_sizer) ;

        //left list is selected
        Bind(wxEVT_LISTBOX, [this](auto&) {
            pimpl->update_static_obj() ;
            pimpl->update_bindings() ;
        }, BindingsEvt::SELECT_FUNC) ;

        Bind(wxEVT_CHOICE, [this](auto&) {
            pimpl->update_bindings() ;
        }, BindingsEvt::CHOICE_MODE) ;

        Bind(wxEVT_CHOICE, [this](auto&) {
            const auto target_idx = pimpl->mode->GetSelection() ;
            if(target_idx == wxNOT_FOUND) {
                throw RUNTIME_EXCEPT("Could not get a selection of mode choices.") ;
            }

            const auto linked_idx = pimpl->linked_mode->GetSelection() ;
            if(linked_idx == wxNOT_FOUND) {
                throw RUNTIME_EXCEPT("Could not get a selection of liked mode choices.") ;
            }

            try {
                pimpl->mode_links.at(pimpl->get_selected_func_name()).at(target_idx) = linked_idx ;
            }
            catch(const std::out_of_range&) {
                throw RUNTIME_EXCEPT("The link dependencies of a selected function are not registered.") ;
            }

            pimpl->update_bindings_state() ;
        }, BindingsEvt::CHOIDE_LINKED_MODE) ;

        //Cmds Add Button
        Bind(wxEVT_BUTTON, [this](auto&) {
            const auto str = pimpl->new_cmd->GetLineText(0) ;
            if(str.empty()) return ;

            const auto index = pimpl->func_list->GetSelection() ;
            if(index == wxNOT_FOUND) return ;

            try {
                const auto mode_idx = pimpl->mode->GetSelection() ;
                if(mode_idx != wxNOT_FOUND) {
                    auto& c = pimpl->parser.at(index).at(g_modes_key[mode_idx]) ;
                    if(!c.contains(str)) {
                        if(pimpl->cmds->IsEmpty()) {
                            c.clear() ;
                        }
                        c.push_back(str.ToStdString()) ;
                        pimpl->cmds->Append(str) ;
                    }
                }
            }
            catch(const std::exception& e) {
                ERROR_PRINT(e.what()) ;
            }

            pimpl->new_cmd->Clear() ;
        }, BindingsEvt::ADD_CMD) ;

        //Cmds Delete Button
        Bind(wxEVT_BUTTON, [this](auto&) {
            const auto index = pimpl->cmds->GetSelection() ;
            if(index == wxNOT_FOUND) return ;

            const auto func_index = pimpl->func_list->GetSelection() ;
            if(func_index == wxNOT_FOUND) return ;

            pimpl->cmds->Delete(index) ;
            try {pimpl->parser.at(func_index).at("cmd").erase(index) ;}
            catch(const nlohmann::json::exception&){return ;}
        }, BindingsEvt::DEL_CMD) ;

        //Return to Default Button
        Bind(wxEVT_BUTTON, [this](auto&) {
            //read default json (partial)
            try {
                const auto index = pimpl->func_list->GetSelection() ;
                if(index == wxNOT_FOUND) return ;

                std::ifstream ifs(Path::Default::BINDINGS()) ;
                nlohmann::json p{} ;
                ifs >> p ;

                pimpl->parser.at(index).clear() ;
                pimpl->parser.at(index) = std::move(p.at(index)) ;

                pimpl->update_static_obj() ;
                pimpl->update_bindings() ;
            }
            catch(const std::exception& e) {
                ERROR_PRINT(std::string(e.what()) + "BindingsEvt::DEFAULT)") ;
                return ;
            }
        }, BindingsEvt::DEFAULT) ;
    }
    BindingsPanel::~BindingsPanel() noexcept = default ;
    void BindingsPanel::do_load_config() {
        std::ifstream ifs(Path::BINDINGS()) ;
        ifs >> pimpl->parser ;
    }

    void BindingsPanel::do_save_config() {
        for(auto& func : pimpl->parser) {
            for(std::size_t i = 0 ; i < g_modes_last_idx ; i ++) {
                try {
                    const auto linked_idx = pimpl->mode_links.at(func.at("name"))[i] ;
                    if(linked_idx != g_modes_last_idx) {
                        func.at(g_modes_key[i]) = {"<" + g_modes_key[linked_idx] + ">"} ;
                    }
                }
                catch(const std::out_of_range&) {
                    continue ;
                }
            }
        }

        std::ofstream ofs(Path::BINDINGS()) ;

        const auto& ui_langs = ioParams::get_choices("ui_lang") ;

        ofs << "[\n" ;
        for(auto func = pimpl->parser.cbegin() ; 
                func != pimpl->parser.cend() ; func ++) {
            if(func != pimpl->parser.cbegin()) {
                ofs << ",\n" ;
            }
            ofs << "    {\n" ;

            ofs << "        \"name\": " << (*func)["name"].dump() << ",\n" ;

            for(const auto& modestr : g_modes_key) {
                ofs << "        \"" << modestr << "\": " ;

                try {
                    ofs << (*func).at(modestr).dump() ;
                }
                catch(const std::out_of_range&) {
                    ofs << "[]" ;
                }
                ofs << ",\n" ;
            }

            for(auto lang = ui_langs.cbegin() ;
                    lang != ui_langs.cend() ; lang ++) {

                if(lang != ui_langs.cbegin()) {
                    ofs << ",\n" ;
                }
                try {
                    ofs << "        \"" << (*lang).at("value") << "\": " ;
                    try {
                        ofs << (*func).at((*lang).at("value")) ;
                    }
                    catch(const std::out_of_range&) {}
                }
                catch(const std::out_of_range&) {
                    continue ;
                }
            }

            ofs << "\n    }" ;
        }

        ofs << "\n]" ;
    }

    void BindingsPanel::translate() {
        pimpl->initialize() ;

        pimpl->update_func_list() ;
        pimpl->func_list->SetSelection(0) ;

        pimpl->update_static_obj() ;
        pimpl->update_bindings() ;

        pimpl->update_labels() ;
        Layout() ;
    }
}
