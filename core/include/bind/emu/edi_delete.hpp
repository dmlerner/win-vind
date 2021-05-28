#ifndef _EDI_DELETE_HPP
#define _EDI_DELETE_HPP

#include "bind/binded_func_creator.hpp"

namespace vind
{
    //Delete
    struct EdiDeleteHighlightText : public BindedFuncCreator<EdiDeleteHighlightText> {
        explicit EdiDeleteHighlightText() ;
        static void sprocess() ;
        static void sprocess(NTypeLogger& parent_lgr) ;
        static void sprocess(const CharLogger& parent_lgr) ;
    } ;

    class EdiNDeleteLine : public BindedFuncCreator<EdiNDeleteLine> {
    private:
        struct Impl ;
        std::unique_ptr<Impl> pimpl ;

    public:
        void sprocess(unsigned int repeat_num=1) const ;
        void sprocess(NTypeLogger& parent_lgr) const ;
        void sprocess(const CharLogger& parent_lgr) const ;

        explicit EdiNDeleteLine() ;
        virtual ~EdiNDeleteLine() noexcept ;

        EdiNDeleteLine(EdiNDeleteLine&&) ;
        EdiNDeleteLine& operator=(EdiNDeleteLine&&) ;
        EdiNDeleteLine(const EdiNDeleteLine&)            = delete ;
        EdiNDeleteLine& operator=(const EdiNDeleteLine&) = delete ;
    } ;


    class EdiNDeleteLineUntilEOL : public BindedFuncCreator<EdiNDeleteLineUntilEOL> {
    private:
        struct Impl ;
        std::unique_ptr<Impl> pimpl ;

    public:
        void sprocess(unsigned int repeat_num=1) const ;
        void sprocess(NTypeLogger& parent_lgr) const ;
        void sprocess(const CharLogger& parent_lgr) const ;

        explicit EdiNDeleteLineUntilEOL() ;
        virtual ~EdiNDeleteLineUntilEOL() noexcept ;

        EdiNDeleteLineUntilEOL(EdiNDeleteLineUntilEOL&&) ;
        EdiNDeleteLineUntilEOL& operator=(EdiNDeleteLineUntilEOL&&) ;
        EdiNDeleteLineUntilEOL(const EdiNDeleteLineUntilEOL&)            = delete ;
        EdiNDeleteLineUntilEOL& operator=(const EdiNDeleteLineUntilEOL&) = delete ;
    } ;

    class EdiNDeleteAfter : public BindedFuncCreator<EdiNDeleteAfter> {
    private:
        struct Impl ;
        std::unique_ptr<Impl> pimpl ;

    public:
        void sprocess(unsigned int repeat_num=1) const ;
        void sprocess(NTypeLogger& parent_lgr) const ;
        void sprocess(const CharLogger& parent_lgr) const ;

        explicit EdiNDeleteAfter() ;
        virtual ~EdiNDeleteAfter() noexcept ;

        EdiNDeleteAfter(EdiNDeleteAfter&&) ;
        EdiNDeleteAfter& operator=(EdiNDeleteAfter&&) ;
        EdiNDeleteAfter(const EdiNDeleteAfter&)            = delete ;
        EdiNDeleteAfter& operator=(const EdiNDeleteAfter&) = delete ;
    } ;

    class EdiNDeleteBefore : public BindedFuncCreator<EdiNDeleteBefore> {
    private:
        struct Impl ;
        std::unique_ptr<Impl> pimpl ;

    public:
        void sprocess(unsigned int repeat_num=1) const ;
        void sprocess(NTypeLogger& parent_lgr) const ;
        void sprocess(const CharLogger& parent_lgr) const ;

        explicit EdiNDeleteBefore() ;
        virtual ~EdiNDeleteBefore() noexcept ;

        EdiNDeleteBefore(EdiNDeleteBefore&&) ;
        EdiNDeleteBefore& operator=(EdiNDeleteBefore&&) ;
        EdiNDeleteBefore(const EdiNDeleteBefore&)            = delete ;
        EdiNDeleteBefore& operator=(const EdiNDeleteBefore&) = delete ;
    } ;
}

#endif
