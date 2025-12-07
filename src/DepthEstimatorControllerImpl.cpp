#include "DepthEstimatorControllerImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "GofkuCamCommon.hpp"
#include "qp.hpp"
#include <memory>
#include <chrono>

#define MINI_PROFILER
namespace GofkuCam
{

DepthEstimatorControllerImpl::DepthEstimatorControllerImpl(QP::QActive * const owner, LoggerInterfacePtr logger)
    : m_owner{owner}
    , m_logger(logger)
    , m_depth_estimator(std::make_shared<DepthEstimator>(
                    Config::config().get<std::string>("depth_model_path"),
                    logger,
                    Config::config().get<bool>("use_gpu")))
{
    m_logger->info("Depth estimator with model: " + Config::config().get<std::string>("depth_model_path"));
}

void DepthEstimatorControllerImpl::start_req(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Depth estimator controller start requested");

    m_owner->subscribe(FRAME_CAPTURED_SIG);
    m_owner->subscribe(STREAM_ENDED_SIG);
    m_owner->subscribe(CAPTURE_ERROR_SIG);
}

void DepthEstimatorControllerImpl::stream_end(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Stream ended, stopping depth estimator controller");
}

void DepthEstimatorControllerImpl::running_entry(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Depth estimator controller running entry");
}

void DepthEstimatorControllerImpl::frame_captured(QP::QEvt const * const e)
{
    m_logger->trace("New frame to depth estimator");

    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    #endif
        FramePtr copy_of_frame = std::make_shared<Frame>(Q_EVT_CAST(FrameCapturedEvt)->m_frame->clone());
        if (copy_of_frame && !copy_of_frame->empty())
        {
            m_logger->info("Depth Estimation input size: " + std::to_string(copy_of_frame->cols) + "x" + std::to_string(copy_of_frame->rows));
            // You can further process or visualize the depth map as needed
        }

        FramePtr depth_map = m_depth_estimator->estimate_depth(copy_of_frame);
        if (depth_map && !depth_map->empty())
        {
            m_logger->info("Depth map estimated with size: " + std::to_string(depth_map->cols) + "x" + std::to_string(depth_map->rows));
            // You can further process or visualize the depth map as needed
        }

        DepthEstimationCompletedEvt* dece = Q_NEW(DepthEstimationCompletedEvt, DEPTH_ESTIMATION_COMPLETED_SIG);
        dece->m_depth_frame = std::make_shared<Frame>(depth_map->clone());
        m_logger->trace("New Depth frame");
        QP::QF::PUBLISH(dece, this);

    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    m_logger->info("Depth estimaton took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
    #endif

    // // Display the frame
    static Frame display_frame;
    cv::resize(*depth_map, display_frame, cv::Size(640, 480));
    g_depth_visualization_frame.store(&display_frame);
}

} // namespace GofkuCam