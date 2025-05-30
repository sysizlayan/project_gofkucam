#ifndef GOFKUCAM_EXCEPTIONS_HPP
#define GOFKUCAM_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace GofkuCam
{
class BaseGofkuCamFailure : public std::runtime_error {
public:
    explicit BaseGofkuCamFailure(const std::string& message)
        : std::runtime_error(message) {}
};

class ConfigFailure : public BaseGofkuCamFailure {
public:
    explicit ConfigFailure(const std::string& message)
        : BaseGofkuCamFailure(message) {}
};

class CameraConnectionFailure : public BaseGofkuCamFailure {
public:
    explicit CameraConnectionFailure(const std::string& message)
        : BaseGofkuCamFailure(message) {}
};

class FrameCaptureFailure : public BaseGofkuCamFailure {
public:
    explicit FrameCaptureFailure(const std::string& message)
        : BaseGofkuCamFailure(message) {}
};

class ErrorTimerTimedOut : public BaseGofkuCamFailure {
public:
    explicit ErrorTimerTimedOut(const std::string& message)
        : BaseGofkuCamFailure(message) {}
};
}
#endif // GOFKUCAM_EXCEPTIONS_HPP