#ifndef DEBUG_HPP
#define DEBUG_HPP

#ifdef MYDEBUG
#include "spdlog/spdlog.h"
#include <utility>
#include <memory>

    #define HIDDEN hidden
    //#define protected public
    //#define private public

    //static auto console__ = spdlog::stdout_logger_mt("console", true);
class LoggerSingleton
{
    public:
        static spdlog::logger *getLogger();
    private:
        LoggerSingleton() {}
        static bool init;
    public:
        LoggerSingleton(const LoggerSingleton&) = delete;
        LoggerSingleton& operator=(const LoggerSingleton&) = delete;
};

    #define LOGGER(OP, ...) do {LoggerSingleton::getLogger() -> OP ( __VA_ARGS__ ); } while(0);
    
#else
    #define HIDDEN
    #define LOGGER(OP, ...)
#endif

#endif
