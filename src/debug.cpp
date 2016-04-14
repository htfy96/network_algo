#include "debug.hpp"

#ifdef MYDEBUG
    #include "spdlog/spdlog.h"
    #include <utility>

bool LoggerSingleton::init = false;

spdlog::logger *LoggerSingleton::getLogger()
{
    static auto logger_ = spdlog::stdout_logger_mt("consoleBasic", true);
    if (!LoggerSingleton::init)
    {
        LoggerSingleton::init = true;
        spdlog::set_pattern("[%x %X.%e][%n|%l] %v");
    }
    logger_->set_level(spdlog::level::debug);
    return logger_.get();
}
    

#endif

