#ifndef _KEY_LOGGER_BASE_HPP
#define _KEY_LOGGER_BASE_HPP

#include "key_absorber.hpp"
#include "key_log.hpp"

#include <memory>
#include <vector>

namespace vind
{
    namespace core
    {
        class KeyLoggerBase {
        private:
            struct Impl ;
            std::unique_ptr<Impl> pimpl ;

        public:
            using Data = std::vector<KeyLog> ;

            explicit KeyLoggerBase() ;
            virtual ~KeyLoggerBase() noexcept ;

            KeyLoggerBase(const KeyLoggerBase&) ;
            KeyLoggerBase& operator=(const KeyLoggerBase&) ;

            KeyLoggerBase(KeyLoggerBase&&) ;
            KeyLoggerBase& operator=(KeyLoggerBase&&) ;

            Data::iterator begin() noexcept ;
            Data::iterator end() noexcept ;

            Data::const_iterator begin() const noexcept ;
            Data::const_iterator end() const noexcept ;

            Data::const_iterator cbegin() const noexcept ;
            Data::const_iterator cend() const noexcept ;

            Data::reference latest() ;
            Data::const_reference latest() const ;

            std::size_t size() const noexcept ;
            bool empty() const noexcept ;

            virtual void clear() noexcept ;

            virtual void logging(const KeyLog& log) ;
            virtual void logging(KeyLog&& log) ;

            virtual void logging(const KeyLog::Data& lograw) ;
            virtual void logging(KeyLog::Data&& lograw) ;

            void remove_from_back(std::size_t num_from_back) ;
            void remove_from_top(std::size_t num_from_top) ;

            Data::const_reference at(std::size_t n) const ;

            bool operator==(const KeyLoggerBase& rhs) const ;
            bool operator!=(const KeyLoggerBase& rhs) const ;

            virtual int logging_state(KeyLog log=get_pressed_list()) = 0 ;
        } ;

#ifdef DEBUG
        std::string print(const KeyLog& lgr) ;
        std::string print(const KeyLoggerBase& lgr) ;
#endif
    }
}

#endif
