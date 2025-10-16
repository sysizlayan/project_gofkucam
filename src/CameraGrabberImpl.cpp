#include "CameraGrabberImpl.hpp"
#include "Config.hpp"
#include "DepthEstimatorController.hpp"
#include "Evts.hpp"
#include "qp.hpp"
#include <cstdint>
#include <memory>
namespace GofkuCam {

CameraGrabberImpl::CameraGrabberImpl(QP::QActive * owner, LoggerInterfacePtr logger, std::vector<std::shared_ptr<DepthEstimatorController>> depth_estimator_controllers)
   : m_owner(owner)
   , m_logger(logger)
   , m_cap(nullptr)
   , m_source(Config::config().get<std::string>("stream_address"))
   , m_frame_timer{m_owner, EvtSignals::FRAME_TIMER_TIMEOUT_SIG}
   , m_polling_timer{m_owner, EvtSignals::POLLING_TIMER_TIMEOUT_SIG}
   , m_is_new_frame_available(false)
   , m_current_frame(std::make_shared<Frame>())
   , m_depth_estimator_controllers(depth_estimator_controllers)
{
   for(int i=0; i<NUM_DEPTH_ESTIMATORS;i++)
   {
      m_estimator_available[i] = true;
   }
}

void CameraGrabberImpl::start_req(QP::QEvt const * const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->info("Camera grabber start requested");
   m_cap = std::make_unique<cv::VideoCapture>(m_source);

   if(!m_cap->isOpened())
   {
      m_logger->error("Failed to open video source: " + m_source);
      StreamEnded *see = Q_NEW(StreamEnded, STREAM_ENDED_SIG);
      QP::QF::PUBLISH(see, this);
   }
}

void CameraGrabberImpl::frame_timer_timeout(QP::QEvt const * const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("Frame timer timeout");
   
   if (m_is_new_frame_available)
   {
      FrameCapturedEvt* fce = Q_NEW(FrameCapturedEvt, FRAME_CAPTURED_SIG);
      fce->m_frame = m_current_frame;
      m_logger->trace("Captured a new frame");
      for(int i=0; i<NUM_DEPTH_ESTIMATORS;i++)
      {
         m_logger->info("Estimator availability: " + std::to_string(m_estimator_available[i]) + " " +std::to_string(i));
      }
      for(int i=0; i<NUM_DEPTH_ESTIMATORS;i++)
      {
         if(m_estimator_available[i])
         {
            m_logger->trace("Depth estimator id: " + std::to_string(i) + " will process the frame.");

            m_estimator_available[i] = false;
            m_depth_estimator_controllers[i]->post_(fce, this);
            break; // Post to only one available estimator
         }
         else 
         {
            m_logger->trace("Depth estimator id: " + std::to_string(i) + " is busy.");
         }
      }
      QP::QF::PUBLISH(fce, this);

      m_frame_timer.armX(Config::config().get<int>("frame_interval_ms"), 0);
      m_is_new_frame_available = false;
   }
   else
   {
      m_polling_timer.disarm();
      m_logger->error("Captured empty frame");
      StreamEnded *see = Q_NEW(StreamEnded, STREAM_ENDED_SIG);
      QP::QF::PUBLISH(see, this);
   }
}

void CameraGrabberImpl::stop_req(QP::QEvt const * const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->info("Camera grabber stop requested");
   m_cap->release();

   m_cap.reset();
   m_cap = nullptr;
}

void CameraGrabberImpl::stream_end(QP::QEvt const * const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->info("Camera stream ended");
}

bool CameraGrabberImpl::is_opened()
{
   return (m_cap && m_cap->isOpened());
}

void CameraGrabberImpl::poll_the_camera(QP::QEvt const * const e)
{
   (void)e; // Suppress unused parameter warning
   if (m_cap && m_cap->isOpened())
   {
      auto frame = std::make_shared<Frame>();
      *m_cap >> *frame;
      if (!frame->empty())
      {
         m_current_frame = frame;
         m_is_new_frame_available = true;
         m_logger->trace("Polled a new frame");
      }
      else
      {
         m_logger->trace("Polled empty frame");
      }
      m_polling_timer.armX(10, 0);
   }
   else
   {
      m_polling_timer.disarm();
      m_logger->error("Capture system got broken!");
      StreamEnded *see = Q_NEW(StreamEnded, STREAM_ENDED_SIG);
      QP::QF::PUBLISH(see, this);
   }
}

void CameraGrabberImpl::depth_estimation_completed(QP::QEvt const * const e)
{
   std::int8_t detector_id =  Q_EVT_CAST(DepthEstimationCompleted)->m_id;
   if(detector_id >=0 && detector_id < NUM_DEPTH_ESTIMATORS)
   {
      m_logger->error("Depth estimation completed from estimator id: " + std::to_string(detector_id));
      m_estimator_available[detector_id] = true;
   }
}

void CameraGrabberImpl::running_entry(QP::QEvt const * const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->info("Camera grabber running entry");
   m_frame_timer.armX(Config::config().get<int>("frame_interval_ms"), 0);
   m_polling_timer.armX(10, 0);
}
} // namespace GofkuCam
