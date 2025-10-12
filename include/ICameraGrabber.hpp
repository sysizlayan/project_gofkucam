#ifndef ICAMERA_GRABBER_HPP_
#define ICAMERA_GRABBER_HPP_

#include <memory>

namespace GofkuCam
{

class ICameraGrabber {
public:
    virtual void start_req() = 0;
    virtual void running_entry() = 0;
    virtual void frame_timer_timeout() = 0;
    virtual void stop_req() = 0;
    virtual void stream_end() = 0;
    virtual bool is_opened() = 0;
    virtual ~ICameraGrabber() = default;
}; // class ICameraGrabber

using ICameraGrabberPtr = std::shared_ptr<ICameraGrabber>;

} // namespace GofkuCam

#endif
