#include "DetectorControllerImpl.hpp"
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

DetectorControllerImpl::DetectorControllerImpl(QP::QActive * const owner, LoggerInterfacePtr logger)
    : m_owner{owner}
    , m_logger(logger)
    , m_detector(std::make_shared<ObjectDetector>(
                    Config::config().get<std::string>("yolo_model_path"),
                    Config::config().get<std::string>("yolo_labels_path"),
                    logger,
                    Config::config().get<bool>("use_gpu")))
    , m_depth_estimator(std::make_shared<DepthEstimator>(
                    Config::config().get<std::string>("depth_model_path"),
                    logger))
    , m_class_names(m_detector->get_class_names(Config::config().get<std::string>("yolo_labels_path")))
    , m_class_colors(ObjectDetector::generate_colors(m_class_names, 0))
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

    std::vector<Detection> detections = m_detector->detect(frame);
    std::vector<Detection> cat_or_dog_detections{};
    // Log dog and cat detections
    for (const auto& detection : detections) {
        std::string class_name = m_class_names[detection.classId];
        if (class_name == "dog" || class_name == "cat")
        {
            m_logger->info("Detected " + class_name + " with confidence: " + 
                         std::to_string(detection.conf) + 
                         " at position [" + std::to_string(detection.box.x) + 
                         ", " + std::to_string(detection.box.y) + 
                         ", " + std::to_string(detection.box.width) + 
                         ", " + std::to_string(detection.box.height) + "]");
            cat_or_dog_detections.push_back(detection);
        }
    }

    FramePtr depth_map = m_depth_estimator->estimate_depth(frame);
    if (depth_map && !depth_map->empty())
    {
        m_logger->info("Depth map estimated with size: " + std::to_string(depth_map->cols) + "x" + std::to_string(depth_map->rows));
        // You can further process or visualize the depth map as needed
    }

    // // Save frame if it contains a dog or cat detection
    // for (const auto& detection : detections) {
    //     std::string class_name = m_class_names[detection.classId];
    //     if (class_name == "dog" || class_name == "cat") {
    //         // Generate timestamp for the filename
    //         auto now = std::chrono::system_clock::now();
    //         auto now_time = std::chrono::system_clock::to_time_t(now);
    //         std::stringstream ss;
    //         ss << "captured_frames/" << class_name << "_" 
    //            << std::put_time(std::localtime(&now_time), "%Y%m%d_%H%M%S")
    //            << "_conf" << std::fixed << std::setprecision(2) << detection.conf
    //            << ".png";
            
    //         cv::imwrite(ss.str(), *frame);
    //         m_logger->info("Saved frame to: " + ss.str());
    //         break;  // Save the frame only once even if multiple detections exist
    //     }
    // }

    m_detector->draw_bounding_box(frame, cat_or_dog_detections, m_class_names, m_class_colors);
    
    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    m_logger->info("Detection took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
    #endif

    // Display the frame
    cv::imshow("GofkuCam Stream", *frame);
    cv::imshow("GofkuCam Depth", *depth_map);
    cv::waitKey(2); // Allow the window to update, wait 1ms

}

} // namespace GofkuCam