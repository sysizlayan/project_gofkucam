#ifndef CAMERA_GRABBER_IMPL_HPP_
#define CAMERA_GRABBER_IMPL_HPP_

#include "ICameraGrabber.hpp"
#include "LoggerInterface.hpp"
#include <memory>
#include "GofkuCamCommon.hpp"
#include "opencv2/videoio.hpp"
#include "qp.hpp"

namespace GofkuCam
{

class CameraGrabberImpl : public ICameraGrabber {
private:
    QP::QActive * m_owner;
    LoggerInterfacePtr m_logger;
    std::unique_ptr<cv::VideoCapture> m_cap;
    std::string m_source;
    QP::QTimeEvt m_frame_timer;
    QP::QTimeEvt m_polling_timer;

public:
    explicit CameraGrabberImpl(QP::QActive * owner, LoggerInterfacePtr logger);
    
    // ICameraGrabber interface implementation
    void start_req() override;
    void running_entry() override;
    void frame_timer_timeout() override;
    void stop_req() override;
    void stream_end() override;
    bool is_opened() override;
}; // class CameraGrabberImpl

} // namespace GofkuCam

#endif // CAMERA_GRABBER_IMPL_HPP_