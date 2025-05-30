#ifndef SPDLOG_LOGGER_H
#define SPDLOG_LOGGER_H

#include "LoggerInterface.hpp"

#define SPDLOG_USE_STD_FORMAT
#include "spdlog/sinks/stdout_color_sinks.h"

namespace GofkuCam
{
class SpdlogLogger : public LoggerInterface {
public:
    SpdlogLogger(const std::string&);

    void info(const std::string& msg) final;

    void warn(const std::string& msg) final;

    void error(const std::string& msg) final;

    void debug(const std::string& msg) final;

    void trace(const std::string& msg) final;
private:
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> _stdout_sink;
    std::shared_ptr<spdlog::sinks::stderr_color_sink_mt> _stderr_sink;
    std::shared_ptr<spdlog::logger> _stdout_logger;
    std::shared_ptr<spdlog::logger> _stderr_logger;
};

} // namespace GofkuCam

#endif //SPDLOG_LOGGER_H