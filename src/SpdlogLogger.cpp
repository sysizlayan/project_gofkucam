#include "SpdlogLogger.hpp"
#include <memory>
#include "Config.hpp"

namespace GofkuCam
{
SpdlogLogger::SpdlogLogger(const std::string& logger_name)
   : _stdout_sink{std::make_shared<spdlog::sinks::stdout_color_sink_mt>()}
   , _stderr_sink{std::make_shared<spdlog::sinks::stderr_color_sink_mt>()}
   , _stdout_logger{std::make_shared<spdlog::logger>(logger_name, _stdout_sink)}
   , _stderr_logger{std::make_shared<spdlog::logger>(logger_name, _stderr_sink)}
{

   std::string log_level = Config::config().get<std::string>("logging");

   if(log_level == "TRACE")
      _stdout_logger->set_level(spdlog::level::trace);
   else if(log_level == "DEBUG")
      _stdout_logger->set_level(spdlog::level::debug);
   else if(log_level == "INFO")
      _stdout_logger->set_level(spdlog::level::info);
   else if(log_level == "WARN")
      _stdout_logger->set_level(spdlog::level::warn);
   else if(log_level == "ERROR")
      _stdout_logger->set_level(spdlog::level::err);
   else
      _stdout_logger->set_level(spdlog::level::off);
}

inline void
SpdlogLogger::info(const std::string& msg) {
    _stdout_logger->info(msg);
}

inline void
SpdlogLogger::warn(const std::string& msg) {
    _stderr_logger->warn(msg);
}

inline void
SpdlogLogger::error(const std::string& msg) {
    _stderr_logger->error(msg);
}

inline void
SpdlogLogger::debug(const std::string& msg) {
    _stdout_logger->error(msg);
}

inline void
SpdlogLogger::trace(const std::string& msg) {
    _stdout_logger->trace(msg);
}
} // namespace GofkuCam