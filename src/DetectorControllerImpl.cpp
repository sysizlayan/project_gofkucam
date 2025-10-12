#include "DetectorControllerImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "ObjectDetector.hpp"
#include "qp.hpp"
#include <memory>

namespace GofkuCam
{

DetectorControllerImpl::DetectorControllerImpl(QP::QActive * const owner, LoggerInterfacePtr logger)
    : m_owner{owner}
    , m_logger(logger)
    , m_detector(std::shared_ptr<ObjectDetector>())
{
}

void DetectorControllerImpl::start_req(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Detector controller start requested");
}

void DetectorControllerImpl::stream_end(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Stream ended, stopping detector controller");
}

void DetectorControllerImpl::running_entry(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Detector controller running entry");
}

void DetectorControllerImpl::frame_captured(QP::QEvt const * const e)
{
    // m_logger->trace("Processing captured frame for object detection");
    
    // // Here you would:
    // // 1. Get the frame from the event
    // // 2. Run object detection
    // // 3. Publish detection results
    
    // #ifdef MINI_PROFILER
    // std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    // #endif

    // // TODO: Get frame from event and run detection
    // // std::vector<Detection> results = m_detector->detect(frame);
    
    // #ifdef MINI_PROFILER
    // std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    // m_logger->info("Detection took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
    // #endif
    m_logger->info("New frame");
    std::shared_ptr<Frame> frame = Q_EVT_CAST(FrameCapturedEvt)->m_frame;
    m_logger->trace("Captured frame with size: " + std::to_string(frame->cols) + "x" + std::to_string(frame->rows));

    // Display the frame
    cv::imshow("GofkuCam Stream", *frame);
    cv::waitKey(1); // Allow the window to update, wait 1ms

}

} // namespace GofkuCam