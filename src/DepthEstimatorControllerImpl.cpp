#include "DepthEstimatorControllerImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "ObjectDetector.hpp"
#include "qp.hpp"
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>

#define MINI_PROFILER
namespace GofkuCam
{

DepthEstimatorControllerImpl::DepthEstimatorControllerImpl(QP::QActive * const owner, LoggerInterfacePtr logger, std::int8_t estimator_id)
    : m_owner{owner}
    , m_logger(logger)
    , m_depth_estimator(std::make_shared<DepthEstimator>(
                    Config::config().get<std::string>("depth_model_path"),
                    logger))
    , m_estimator_id(estimator_id)
{
    m_logger->info("Depth estimator with model: " + Config::config().get<std::string>("depth_model_path"));

}

void DepthEstimatorControllerImpl::start_req(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Depth estimator controller start requested");
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
    m_logger->info("New frame");
    m_frame_to_process = std::make_shared<Frame>(Q_EVT_CAST(FrameCapturedEvt)->m_frame->clone());
    m_logger->trace("Captured frame with size: " + std::to_string(m_frame_to_process->cols) + "x" + std::to_string(m_frame_to_process->rows));
}

void DepthEstimatorControllerImpl::calculating_entry(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->trace("Depth estimator controller calculating entry for estimator id: " + std::to_string(m_estimator_id));
    try
    {
        #ifdef MINI_PROFILER
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        #endif
        FramePtr depth_map = m_depth_estimator->estimate_depth(m_frame_to_process);
        if (depth_map && !depth_map->empty())
        {
            m_logger->info("Depth map estimated with size: " + std::to_string(depth_map->cols) + "x" + std::to_string(depth_map->rows));
            // You can further process or visualize the depth map as needed
        }
        #ifdef MINI_PROFILER
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        m_logger->info("Depth estimaton took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms] for estimator id: " + std::to_string(m_estimator_id));
        #endif

        // Display the frame
        // cv::imshow("GofkuCam Depth", *depth_map);
        // cv::waitKey(2); // Allow the window to update, wait 1ms
        DepthEstimationCompleted *dec = Q_NEW(DepthEstimationCompleted, DEPTH_ESTIMATION_COMPLETED_SIG);
        dec->m_id = m_estimator_id;
        QP::QF::PUBLISH(dec, this);
        m_owner->post_(dec, this);
    }
    catch (...)
    {
        m_logger->error(std::string("Error during depth estimation:!"));
        DepthEstimationFailed *def = Q_NEW(DepthEstimationFailed, DEPTH_ESTIMATION_FAILED_SIG);
        m_owner->post_(def, this);
    }
}

void DepthEstimatorControllerImpl::idle_entry(QP::QEvt const * const e) {
    
}

} // namespace GofkuCam