#ifndef ICAMERA_GRABBER_HPP_
#define ICAMERA_GRABBER_HPP_

#include <memory>
#include "qpcpp.hpp"

namespace GofkuCam
{

class ICameraGrabber {
public:
    virtual void start_req(QP::QEvt const * const e) = 0;
    virtual void running_entry(QP::QEvt const * const e) = 0;
    virtual void frame_timer_timeout(QP::QEvt const * const e) = 0;
    virtual void stop_req(QP::QEvt const * const e) = 0;
    virtual void stream_end(QP::QEvt const * const e) = 0;
    virtual bool is_opened() = 0;  // No event parameter needed as this is a query
    virtual void poll_the_camera(QP::QEvt const * const e) = 0;
    virtual void depth_estimation_completed(QP::QEvt const * const e) = 0;
    virtual ~ICameraGrabber() = default;
}; // class ICameraGrabber

using ICameraGrabberPtr = std::shared_ptr<ICameraGrabber>;

} // namespace GofkuCam

#endif
