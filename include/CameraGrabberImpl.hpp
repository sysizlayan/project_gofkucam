#ifndef CAMERA_GRABBER_IMPL_HPP_
#define CAMERA_GRABBER_IMPL_HPP_

#include "ICameraGrabber.hpp"
#include "LoggerInterface.hpp"
#include "DepthEstimatorController.hpp"
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
    bool m_is_new_frame_available;
    std::shared_ptr<Frame> m_current_frame;
    std::vector<std::shared_ptr<DepthEstimatorController>> m_depth_estimator_controllers;
    bool m_estimator_available[NUM_DEPTH_ESTIMATORS];
public:
    explicit CameraGrabberImpl(QP::QActive * owner, LoggerInterfacePtr logger, std::vector<std::shared_ptr<DepthEstimatorController>> depth_estimator_controllers);
    
    // ICameraGrabber interface implementation
    void start_req(QP::QEvt const * const e) override;
    void running_entry(QP::QEvt const * const e) override;
    void frame_timer_timeout(QP::QEvt const * const e) override;
    void stop_req(QP::QEvt const * const e) override;
    void stream_end(QP::QEvt const * const e) override;
    bool is_opened() override;  // No event parameter as this is a query
    void poll_the_camera(QP::QEvt const * const e) override;
    void depth_estimation_completed(QP::QEvt const * const e) override;
}; // class CameraGrabberImpl

} // namespace GofkuCam

#endif // CAMERA_GRABBER_IMPL_HPP_