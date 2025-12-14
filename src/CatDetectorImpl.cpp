#include "CatDetectorImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "GofkuCamCommon.hpp"
#include "ObjectDetector.hpp"
#include "qp.hpp"
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <memory>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>

// #define MINI_PROFILER

namespace GofkuCam
{

CatDetectorImpl::CatDetectorImpl(QP::QActive* const owner, LoggerInterfacePtr logger)
    : m_owner{owner}
    , m_logger(logger)
    , m_current_frame_copy(nullptr)
    , m_current_depth_map(nullptr)
    , m_detected_haku(nullptr)
    , m_detected_gofret(nullptr)
    , m_detected_haku_distance(0.0)
    , m_detected_gofret_distance(0.0)
{
   m_logger->info("Cat Detector initialized");
}

// State Entry/Exit implementations
void CatDetectorImpl::operating_entry(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Entering OPERATING state");
}

void CatDetectorImpl::operating_exit(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Exiting OPERATING state");
}

void CatDetectorImpl::waiting_detectors_entry(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Entering WAITING_DETECTORS state");
}

void CatDetectorImpl::waiting_detectors_exit(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Exiting WAITING_DETECTORS state");
}

void CatDetectorImpl::waiting_object_detection_entry(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Entering WAITING_OBJECT_DETECTION state");
}

void CatDetectorImpl::waiting_object_detection_exit(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Exiting WAITING_OBJECT_DETECTION state");
}

void CatDetectorImpl::waiting_depth_map_entry(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Entering WAITING_DEPTH_MAP state");
}

void CatDetectorImpl::waiting_depth_map_exit(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Exiting WAITING_DEPTH_MAP state");
}

void CatDetectorImpl::determine_cat_feeding_entry(QP::QEvt const* const e)
{
   // // Display the frame
   static Frame display_frame;
   static int lower_threshold = Config::config().get<int>("haku_feeding_distance_lower_threshold");
   static int upper_threshold = Config::config().get<int>("haku_feeding_distance_upper_threshold");

   (void)e;
   m_logger->info("Entering DETERMINE_CAT_FEEDING state");

   std::unique_ptr<cv::Scalar> color_ptr{nullptr};

   if (m_current_depth_map != nullptr && !m_current_depth_map->empty() && m_detected_haku != nullptr)
   {
      int roi_size = Config::config().get<int>("depth_mean_roi_size");
      double half_roi = static_cast<double>(roi_size) / 2.0;

      double anchor_x =
          static_cast<int>(m_detected_haku->box.x + (static_cast<double>(m_detected_haku->box.width) / 2 - half_roi));
      double anchor_y =
          static_cast<int>(m_detected_haku->box.y + (static_cast<double>(m_detected_haku->box.height) / 2 - half_roi));

      cv::Rect haku_roi_rect(anchor_x, anchor_y, roi_size, roi_size);
      // haku_roi_rect = haku_roi_rect & cv::Rect(0, 0,
      // m_current_depth_map->cols, m_current_depth_map->rows);
      m_logger->trace("Haku ROI rect: x=" + std::to_string(haku_roi_rect.x) + ", y=" + std::to_string(haku_roi_rect.y) +
                     ", width=" + std::to_string(haku_roi_rect.width) +
                     ", height=" + std::to_string(haku_roi_rect.height));
      if (haku_roi_rect.area() > 0)
      {
         cv::Scalar mean_scalar = cv::mean((*m_current_depth_map)(haku_roi_rect));
         if (m_current_depth_map->channels() >= 3)
         {
            m_detected_haku_distance = (mean_scalar[0] + mean_scalar[1] + mean_scalar[2]) / 3.0;
         }
         else
         {
            m_detected_haku_distance = mean_scalar[0];
         }
      }

      m_logger->info("Haku distance " + std::to_string(static_cast<int>(m_detected_haku_distance)) + " at " +
                     std::to_string(m_detected_haku->box.x) + ", " + std::to_string(m_detected_haku->box.y));

      if(m_detected_haku_distance >= lower_threshold
      //&& m_detected_haku_distance <= upper_threshold
      )
      {
         m_logger->error("Haku is feeding! Distance: " + std::to_string(static_cast<int>(m_detected_haku_distance)));
         color_ptr = std::make_unique<cv::Scalar>(0, 0, 0); // Black

      }
      else
      {
         color_ptr = std::make_unique<cv::Scalar>(255, 255, 255); // White

      }
      cv::putText(*m_current_depth_map, "Haku:" + std::to_string(static_cast<int>(m_detected_haku_distance)),
                  cv::Point(anchor_x, anchor_y), cv::FONT_HERSHEY_COMPLEX, 1.5, *color_ptr, 2);
   }

   if (m_current_depth_map != nullptr && !m_current_depth_map->empty() && m_detected_gofret != nullptr)
   {
      int roi_size = Config::config().get<int>("depth_mean_roi_size");
      double half_roi = static_cast<double>(roi_size) / 2.0;

      double anchor_x =
          static_cast<int>(m_detected_gofret->box.x + (static_cast<double>(m_detected_gofret->box.width) / 2 - half_roi));
      double anchor_y =
          static_cast<int>(m_detected_gofret->box.y + (static_cast<double>(m_detected_gofret->box.height) / 2 - half_roi));

      cv::Rect gofret_roi_rect(anchor_x, anchor_y, roi_size, roi_size);
      // haku_roi_rect = haku_roi_rect & cv::Rect(0, 0,
      // m_current_depth_map->cols, m_current_depth_map->rows);
      m_logger->trace("Gofret ROI rect: x=" + std::to_string(gofret_roi_rect.x) + ", y=" + std::to_string(gofret_roi_rect.y) +
                     ", width=" + std::to_string(gofret_roi_rect.width) +
                     ", height=" + std::to_string(gofret_roi_rect.height));
      if (gofret_roi_rect.area() > 0)
      {
         cv::Scalar mean_scalar = cv::mean((*m_current_depth_map)(gofret_roi_rect));
         if (m_current_depth_map->channels() >= 3)
         {
            m_detected_gofret_distance = (mean_scalar[0] + mean_scalar[1] + mean_scalar[2]) / 3.0;
         }
         else
         {
            m_detected_gofret_distance = mean_scalar[0];
         }
      }

      m_logger->info("Gofret distance " + std::to_string(static_cast<int>(m_detected_gofret_distance)) + " at " +
                     std::to_string(m_detected_gofret->box.x) + ", " + std::to_string(m_detected_gofret->box.y));
      cv::putText(*m_current_depth_map, "Gofret:" + std::to_string(static_cast<int>(m_detected_gofret_distance)),
                  cv::Point(anchor_x, anchor_y), cv::FONT_HERSHEY_COMPLEX, 1.5, cv::Scalar(255, 255, 255), 2);
   }

   CatFeedingDetermined* cfd = Q_NEW(CatFeedingDetermined, CAT_FEEDING_DETERMINED_SIG);
   cfd->m_haku_status = std::make_shared<HakuStatus>();
   cfd->m_haku_status->m_is_haku_in_dangerous_zone = (m_detected_haku_distance >= lower_threshold);
   cfd->m_haku_status->m_hakus_distance = m_detected_haku_distance;
   QP::QF::PUBLISH(cfd, m_owner);

   cv::resize(*m_current_depth_map, display_frame, cv::Size(640, 480));
   g_depth_visualization_frame.store(&display_frame);
}

void CatDetectorImpl::determine_cat_feeding_exit(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Exiting DETERMINE_CAT_FEEDING state " + std::to_string(static_cast<int>(e->sig)));
}

void CatDetectorImpl::not_started_entry(QP::QEvt const* const e)
{
   (void)e;
   m_logger->warn("Entering NOT_STARTED state");
}

void CatDetectorImpl::not_started_exit(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Exiting NOT_STARTED state");
}

// Event handler implementations
void CatDetectorImpl::start_req(QP::QEvt const* const e)
{
   (void)e;
   m_logger->info("Cat Detector start requested");
   m_owner->subscribe(FRAME_CAPTURED_SIG);
   m_owner->subscribe(STREAM_ENDED_SIG);
   m_owner->subscribe(CAPTURE_ERROR_SIG);
   m_owner->subscribe(OBJECT_DETECTION_COMPLETED_SIG);
   m_owner->subscribe(DEPTH_ESTIMATION_COMPLETED_SIG);
   m_owner->subscribe(CAT_FEEDING_DETERMINED_SIG);
}

void CatDetectorImpl::stop_req(QP::QEvt const* const e)
{
   (void)e;
   m_logger->warn("Cat Detector stop requested");
}

void CatDetectorImpl::stream_end(QP::QEvt const* const e)
{
   (void)e;
   m_logger->error("Stream ended, stopping Cat Detector");
}

void CatDetectorImpl::running_entry(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Cat Detector running entry");
}

void CatDetectorImpl::frame_captured(QP::QEvt const* const e)
{
   m_logger->info("New frame to Cat Detector");

#ifdef MINI_PROFILER
   std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#endif

   FramePtr copy_of_frame = std::make_shared<Frame>(Q_EVT_CAST(FrameCapturedEvt)->m_frame->clone());
   m_logger->trace("Cat detector input size: " + std::to_string(copy_of_frame->cols) + "x" +
                  std::to_string(copy_of_frame->rows));
   m_current_frame_copy.swap(copy_of_frame);

#ifdef MINI_PROFILER
   std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
   m_logger->info("Cat detection took " +
                  std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
#endif
}

void CatDetectorImpl::object_detection_completed(QP::QEvt const* const e)
{
   m_logger->trace("Object detection completed event received in Cat Detector");
   auto odce = Q_EVT_CAST(ObjectDetectionCompletedEvt);
   std::vector<Detection>& cat_or_dog_detections = *odce->m_cat_or_dog_detections;
   m_logger->trace("CAT Number of cat or dog detections: " + std::to_string(cat_or_dog_detections.size()));
   m_detected_haku = nullptr;
   m_detected_gofret = nullptr;

   for (auto& detection : cat_or_dog_detections)
   {
      if (detection.average_of_detection >= Config::config().get<int>("haku_pixel_threshold"))
      {
         m_detected_haku = std::make_shared<Detection>(detection);
         m_logger->trace("Haku detected with average pixel value: " + std::to_string(detection.average_of_detection));
      }
      else
      {
         m_detected_gofret = std::make_shared<Detection>(detection);
         m_logger->trace("Gofret detected with average pixel value: " + std::to_string(detection.average_of_detection));
      }
   }
}

void CatDetectorImpl::depth_estimation_completed(QP::QEvt const* const e)
{
   m_logger->trace("Depth estimation completed event received in Cat Detector");
   auto dece = Q_EVT_CAST(DepthEstimationCompletedEvt);
   FramePtr depth_frame = std::make_shared<Frame>(dece->m_depth_frame->clone());
   m_current_depth_map.swap(depth_frame);
   m_logger->trace("CAT Depth frame size: " + std::to_string(m_current_depth_map->cols) + "x" +
                  std::to_string(m_current_depth_map->rows));
   m_detected_gofret_distance = 0;
   m_detected_gofret_distance = 0; // Reset distances
}

void CatDetectorImpl::frame_timer_timeout(QP::QEvt const* const e)
{
   (void)e;
   m_logger->trace("Frame timer timeout");
}

void CatDetectorImpl::cat_feeding_determined(QP::QEvt const* const e)
{
   (void)e;
   m_logger->info("TEST TEST Cat feeding status determined");
}

void CatDetectorImpl::capture_error(QP::QEvt const* const e)
{
   (void)e;
   m_logger->error("Camera capture error occurred");
}

} // namespace GofkuCam