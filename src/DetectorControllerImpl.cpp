#include "DetectorControllerImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "ObjectDetector.hpp"
#include "qp.hpp"
#include <memory>

#define MINI_PROFILER
namespace GofkuCam
{

DetectorControllerImpl::DetectorControllerImpl(QP::QActive * const owner, LoggerInterfacePtr logger)
    : m_owner{owner}
    , m_logger(logger)
    , m_detector(std::make_shared<ObjectDetector>(
                    Config::config().get<std::string>("yolo_model_path"),
                    Config::config().get<std::string>("yolo_labels_path"),
                    logger,
                    Config::config().get<bool>("use_gpu")))
{
    m_logger->info("Detector constructed with model: " + Config::config().get<std::string>("yolo_model_path"));

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
    m_logger->info("New frame");
    std::shared_ptr<Frame> frame = Q_EVT_CAST(FrameCapturedEvt)->m_frame;
    m_logger->trace("Captured frame with size: " + std::to_string(frame->cols) + "x" + std::to_string(frame->rows));

    // // Here you would:
    // // 1. Get the frame from the event
    // // 2. Run object detection
    // // 3. Publish detection results
    
    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    #endif

    // TODO: Get frame from event and run detection
    std::vector<Detection> detections = m_detector->detect(frame);
    //m_detector->draw_bounding_box(frame, detections, const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &colors)
    
    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    m_logger->info("Detection took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
    #endif

    // Display the frame
    cv::imshow("GofkuCam Stream", *frame);
    cv::waitKey(1); // Allow the window to update, wait 1ms

}

} // namespace GofkuCam