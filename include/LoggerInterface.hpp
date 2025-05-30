#ifndef LOGGER_INTERFACE_H
#define LOGGER_INTERFACE_H

#include <string>
#include <memory>

namespace GofkuCam
{
struct LoggerInterface {

    virtual void info(const std::string&) = 0;

    virtual void warn(const std::string&) = 0;

    virtual void error(const std::string&) = 0;

    virtual void debug(const std::string&) = 0;

    virtual void trace(const std::string&) = 0;

    virtual ~LoggerInterface() {}
};

using LoggerInterfacePtr = std::shared_ptr<LoggerInterface>;

} // namespace GofkuCam

#endif //LOGGER_INTERFACE_H