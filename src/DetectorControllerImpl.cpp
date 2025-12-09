#include "DetectorControllerImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "ObjectDetector.hpp"
#include "GofkuCamCommon.hpp"
#include "qp.hpp"
#include <memory>
#include <chrono>

#define MINI_PROFILER
//#define SAVE_DETECTIONS_FRAMES

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
    , m_class_names(m_detector->get_class_names(Config::config().get<std::string>("yolo_labels_path")))
    , m_class_colors(ObjectDetector::generate_colors(m_class_names, 0))
{
    m_logger->info("Detector constructed with model: " + Config::config().get<std::string>("yolo_model_path"));

}

void DetectorControllerImpl::start_req(QP::QEvt const * const e)
{
    (void)e; // Suppress unused parameter warning
    m_logger->info("Detector controller start requested");

    m_owner->subscribe(FRAME_CAPTURED_SIG);
    m_owner->subscribe(STREAM_ENDED_SIG);
    m_owner->subscribe(CAPTURE_ERROR_SIG);
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
    m_logger->trace("New frame to object detector");

    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    #endif
        FramePtr copy_of_frame = std::make_shared<Frame>(Q_EVT_CAST(FrameCapturedEvt)->m_frame->clone());

        if (copy_of_frame && !copy_of_frame->empty())
        {
            m_logger->info("Object detection input size: " + std::to_string(copy_of_frame->cols) + "x" + std::to_string(copy_of_frame->rows));
            // You can further process or visualize the depth map as needed
        }

        std::vector<Detection> detections = m_detector->detect(copy_of_frame);

        std::vector<Detection> cat_or_dog_detections{};
        // Log dog and cat detections
        for (const auto& original_detection : detections) {
            Detection detection = original_detection;
            std::string class_name = m_class_names[detection.classId];
            if (class_name == "dog" || class_name == "cat")
            {
                if (copy_of_frame && !copy_of_frame->empty()) {
                    cv::Rect roi_rect(detection.box.x, detection.box.y, detection.box.width, detection.box.height);
                    roi_rect = roi_rect & cv::Rect(0, 0, copy_of_frame->cols, copy_of_frame->rows);
                    
                    if (roi_rect.area() > 0) {
                        cv::Scalar mean_scalar = cv::mean((*copy_of_frame)(roi_rect));
                        if (copy_of_frame->channels() >= 3) {
                             detection.average_of_detection = (mean_scalar[0] + mean_scalar[1] + mean_scalar[2]) / 3.0;
                        } else {
                             detection.average_of_detection = mean_scalar[0];
                        }
                    }
                }

                m_logger->info("Detected " + class_name + " with confidence: " + 
                            std::to_string(detection.conf) + 
                            " at position [" + std::to_string(detection.box.x) + 
                            ", " + std::to_string(detection.box.y) +
                            "], WitdhHeight: [" +
                            std::to_string(detection.box.width) + 
                            ", " + std::to_string(detection.box.height) + "]");
                cat_or_dog_detections.push_back(detection);
            }
        }

        m_detector->draw_bounding_box(copy_of_frame, cat_or_dog_detections, m_class_names, m_class_colors);

        if(!cat_or_dog_detections.empty())
        {
            ObjectDetectionCompletedEvt* odce = Q_NEW(ObjectDetectionCompletedEvt, OBJECT_DETECTION_COMPLETED_SIG);
            odce->m_cat_or_dog_detections = std::make_shared<std::vector<Detection>>();
            for(auto& item:cat_or_dog_detections)
                odce->m_cat_or_dog_detections->push_back(item);
            m_logger->trace("Cat and dog detections are sent");
            QP::QF::PUBLISH(odce, this);
        }

        #ifdef SAVE_DETECTIONS_FRAMES
        // Save frame if it contains a dog or cat detection
        for (const auto& detection : detections) {
            std::string class_name = m_class_names[detection.classId];
            if (class_name == "dog" || class_name == "cat") {
                // Generate timestamp for the filename
                auto now = std::chrono::system_clock::now();
                auto now_time = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << "captured_frames/" << class_name << "_" 
                << std::put_time(std::localtime(&now_time), "%Y%m%d_%H%M%S")
                << "_conf" << std::fixed << std::setprecision(2) << detection.conf
                << ".png";
                
                cv::imwrite(ss.str(), *frame);
                m_logger->info("Saved frame to: " + ss.str());
                break;  // Save the frame only once even if multiple detections exist
            }
        }
        #endif

    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    m_logger->info("Object detection took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
    #endif

    // // Display the frame
    static Frame display_frame;
    cv::resize(*copy_of_frame, display_frame, cv::Size(640, 480));
    g_detection_visualization_frame.store(&display_frame);
}

} // namespace GofkuCam