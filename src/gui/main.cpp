/*
             _                  _           __
   _      __(_)___       _   __(_)___  ____/ /
  | | /| / / / __ \_____| | / / / __ \/ __  /
  | |/ |/ / / / / /_____/ |/ / / / / / /_/ /
  |__/|__/_/_/ /_/      |___/_/_/ /_/\__,_/

       - Vim Key Binder for Windows -

MIT License

Copyright (c) 2020-2021 pit-ray
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR MENURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <atomic>

#include "util/disable_gcc_warning.hpp"

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/msgdlg.h>

#include "util/enable_gcc_warning.hpp"

#include "core/entry.hpp"
#include "core/err_logger.hpp"
#include "core/g_params.hpp"
#include "core/path.hpp"

#include "about.hpp"

#ifdef DEBUG
#include <iostream>
#endif


namespace
{
    inline void error_box(const wxString& msg) {
        wxMessageBox(msg, wxT("Error - win-vind"), wxOK | wxICON_EXCLAMATION) ;
    }

    std::atomic_bool g_runnable(true) ;
}

namespace vind
{
    namespace gui
    {
        //core system is wrought at another thread
        class SystemThread : public wxThread {
        public:
            core::VindEntry entry_ ;

            template <typename ExitFunc>
            SystemThread(ExitFunc&& exit_func)
            : wxThread(wxTHREAD_DETACHED),
              entry_(std::forward<ExitFunc>(exit_func))
            {}

        private:
            virtual ExitCode Entry() override {
                while(g_runnable.load()) {
                    try {
                        entry_.update() ;
                    }
                    catch(const std::exception& e) {
                        core::Logger::get_instance().error(e.what()) ;
                        break ;
                    }
                    catch(const core::SafeForcedTermination& e) {
                        core::Logger::get_instance().message(e.what()) ;
                        break ;
                    }
                    catch(...) {
                        core::Logger::get_instance().error("Fatal error occured.") ;
                        break ;
                    }
                }

                if(g_runnable.load()) {
                    g_runnable.store(false) ;
                    entry_.handle_system_call(SystemCall::TERMINATE) ;
                }

                return static_cast<ExitCode>(0) ;
            }
        } ;


        class App : public wxApp {
        private:
            std::unique_ptr<SystemThread> pst_ ;

            static std::string argument_func_ ;

        public:
            App()
            : pst_(nullptr)
            {}

            virtual ~App() noexcept {
                if(g_runnable.load()) {
                    // Core win_vind is running yet, so terminate core system.
                    g_runnable.store(false) ;
                }
            }

        private:
            bool OnInit() override {
                try {
                    if(!wxApp::OnInit()) {
                        return false ;
                    }

                    if(!wxTaskBarIcon::IsAvailable()) {
                        wxMessageBox(
                                wxT("Not supported System Tray"),
                                wxT("Warning - win-vind"),
                                wxOK | wxICON_EXCLAMATION) ;
                    }

                    pst_ = std::make_unique<SystemThread>([this] {
                        ExitMainLoop() ;
                    }) ;

                    pst_->entry_.init(argument_func_) ;

                    if(pst_->entry_.is_subprocess()) {
                        return false ;
                    }

                    // Root window
                    auto dlg = new AboutDialog(
#ifdef DEBUG
                            (vind::core::RESOUECE_ROOT_PATH() / vind::core::get_s("icon_style")).u8string(),
#else
                            vind::core::get_s("icon_style"),
#endif
                            "win-vind",
                            vind::core::get_i("gui_fontsize"),
                            vind::core::get_s("gui_fontname")) ;

                    dlg->Show(false) ;
                }
                catch(const std::exception& e) {
                    error_box(wxString::FromUTF8(e.what()) \
                            + wxT(" Could not initialize win-vind, so terminate." \
                            + wxT(" (Windows Error Code: ") + std::to_string(GetLastError()) + ")")) ;
                }
                return true ;
            }

            int MainLoop() override {
                pst_->Run() ;
                return wxApp::MainLoop() ;
            }

            void OnInitCmdLine(wxCmdLineParser& parser) override {
                parser.AddSwitch(
                        wxT("h"),
                        wxT("help"),
                        wxT("Print usage and exit"),
                        wxCMD_LINE_PARAM_OPTIONAL) ;

                parser.AddOption(
                        wxT("f"),
                        wxT("func"),
                        wxT("Identifier of the function to call in one-shot"),
                        wxCMD_LINE_VAL_STRING,
                        wxCMD_LINE_PARAM_OPTIONAL) ;
            }

            bool OnCmdLineParsed(wxCmdLineParser& parser) override {
                wxString fn ;
                if(parser.Found(wxT("help"))) {
                    parser.Usage() ;
                    return false ;
                }
                else if(parser.Found(wxT("func"), &fn)) {
                    argument_func_ = fn.ToStdString() ;
                }
                return true ;
            }

            bool OnExceptionInMainLoop() override {
                try {
                    throw ; //Rethrow the current exception.
                }
                catch(const std::exception& e) {
                    PRINT_ERROR(e.what()) ;
                }

                return false ; //exit program
            }

            void OnUnhandledException() override {
                try {
                    throw ; //Rethrow the current exception.
                }
                catch(const std::exception& e) {
                    PRINT_ERROR(e.what()) ;
                }
                //the program is already about to exit.
            }
        } ;

        std::string App::argument_func_ ;
    }
}


#include "util/disable_gcc_warning.hpp"

DECLARE_APP(vind::gui::App) ;
IMPLEMENT_APP(vind::gui::App) ;

#include "util/enable_gcc_warning.hpp"
