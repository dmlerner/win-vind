#include "background.hpp"

#include "exception.hpp"
#include "g_params.hpp"
#include "key_absorber.hpp"

#include "opt/option.hpp"
#include "opt/optionlist.hpp"
#include "util/winwrap.hpp"


namespace vind
{
    namespace core
    {
        struct Background::Impl {
            std::vector<opt::Option::SPtr> opts_ ;

            template <typename T>
            Impl(T&& opts)
            : opts_(std::forward<T>(opts))
            {}
        } ;

        Background::Background()
        : pimpl(std::make_unique<Impl>(opt::all_global_options()))
        {}

        Background::Background(const std::vector<opt::Option::SPtr>& opts)
        : pimpl(std::make_unique<Impl>(opts))
        {}

        Background::Background(std::vector<opt::Option::SPtr>&& opts)
        : pimpl(std::make_unique<Impl>(std::move(opts)))
        {}

        Background::~Background() noexcept = default ;

        void Background::update() {
            util::get_win_message() ;

            Sleep(5) ;

            for(const auto& op : pimpl->opts_) {
                op->process() ;
            }

            refresh_toggle_state() ;

            if(is_really_pressed(KEYCODE_F8, KEYCODE_F9)) {
                throw SafeForcedTermination() ;
            }
        }
    }
}
