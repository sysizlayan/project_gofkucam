#include "CatDetectorImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "GofkuCamCommon.hpp"
#include "ObjectDetector.hpp"
#include "qp.hpp"
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>

//#define MINI_PROFILER

namespace GofkuCam
{

CatDetectorImpl::CatDetectorImpl(QP::QActive * const owner, LoggerInterfacePtr logger)
    : m_owner{owner}
    , m_logger(logger)
{
    m_logger->info("Cat Detector initialized");

}

void CatDetectorImpl::start_req(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Cat Detector  start requested");
}

void CatDetectorImpl::stream_end(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Stream ended, stopping Cat Detector ");
}

void CatDetectorImpl::running_entry(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Cat Detector  running entry");
}

void CatDetectorImpl::frame_captured(QP::QEvt const * const e)
{
    m_logger->info("New frame to Cat Detector");

    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    #endif
    FramePtr copy_of_frame = std::make_shared<Frame>(Q_EVT_CAST(FrameCapturedEvt)->m_frame->clone());
    m_logger->trace("Captured frame with size: " + std::to_string(copy_of_frame->cols) + "x" + std::to_string(copy_of_frame->rows));

    
    
    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    m_logger->info("Cat detection took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
    #endif

    // // // Display the frame
    // static Frame display_frame;
    // cv::resize(*depth_map, display_frame, cv::Size(640, 480));
    // g_depth_visualization_frame.store(&display_frame);
}

void CatDetectorImpl::object_detection_completed(QP::QEvt const * const e)
{
    m_logger->info("Object detection completed event received in Cat Detector");
    auto odce = Q_EVT_CAST(ObjectDetectionCompletedEvt);
    std::vector<Detection>& cat_or_dog_detections = *odce->m_cat_or_dog_detections;
    m_logger->info("CAT Number of cat or dog detections: " + std::to_string(cat_or_dog_detections.size()));
}

void CatDetectorImpl::depth_estimation_completed(QP::QEvt const * const e)
{
    m_logger->info("Depth estimation completed event received in Cat Detector");
    auto dece = Q_EVT_CAST(DepthEstimationCompletedEvt);
    FramePtr depth_frame = std::make_shared<Frame>(dece->m_depth_frame->clone());
    m_logger->info("CAT Depth frame size: " + std::to_string(depth_frame->cols) + "x" + std::to_string(depth_frame->rows));
    // Further processing can be done here
}

} // namespace GofkuCam