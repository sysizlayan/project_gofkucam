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

#define MINI_PROFILER

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
    m_logger->info("Depth estimator controller start requested");
}

void CatDetectorImpl::stream_end(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Stream ended, stopping depth estimator controller");
}

void CatDetectorImpl::running_entry(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Depth estimator controller running entry");
}

void CatDetectorImpl::frame_captured(QP::QEvt const * const e)
{
    m_logger->info("New frame to depth estimator");
    std::shared_ptr<Frame> frame = Q_EVT_CAST(FrameCapturedEvt)->m_frame;
    m_logger->trace("Captured frame with size: " + std::to_string(frame->cols) + "x" + std::to_string(frame->rows));

    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    #endif
    FramePtr copy_of_frame = std::make_shared<Frame>(frame->clone());

    
    
    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    m_logger->info("Depth estimaton took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
    #endif

    // // // Display the frame
    // static Frame display_frame;
    // cv::resize(*depth_map, display_frame, cv::Size(640, 480));
    // g_depth_visualization_frame.store(&display_frame);
}

} // namespace GofkuCam