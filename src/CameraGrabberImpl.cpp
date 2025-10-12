#include "CameraGrabberImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "qp.hpp"
#include <memory>
namespace GofkuCam {

CameraGrabberImpl::CameraGrabberImpl(QP::QActive * owner, LoggerInterfacePtr logger)
   : m_owner(owner)
   , m_logger(logger)
   , m_cap(nullptr)
   , m_source(Config::config().get<std::string>("stream_address"))
   , m_frame_timer{m_owner, EvtSignals::FRAME_TIMER_TIMEOUT_SIG}
   , m_polling_timer{m_owner, EvtSignals::POLLING_TIMER_TIMEOUT_SIG}
   , m_is_new_frame_available(false)
   , m_current_frame(std::make_shared<Frame>())
{
}

void CameraGrabberImpl::start_req()
{
   m_logger->info("Camera grabber start requested");
   m_cap = std::make_unique<cv::VideoCapture>(m_source);
}

void CameraGrabberImpl::frame_timer_timeout()
{
   m_logger->trace("Frame timer timeout");
   
   if (m_is_new_frame_available)
   {
      FrameCapturedEvt* fce = Q_NEW(FrameCapturedEvt, FRAME_CAPTURED_SIG);
      fce->m_frame = m_current_frame;
      m_logger->trace("Captured a new frame");
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

void CameraGrabberImpl::stop_req()
{
   m_logger->info("Camera grabber stop requested");
   m_cap->release();

   m_cap.reset();
   m_cap = nullptr;
}

void CameraGrabberImpl::stream_end()
{
   m_logger->info("Camera stream ended");
}

bool CameraGrabberImpl::is_opened()
{
   return (m_cap && m_cap->isOpened());
}
void CameraGrabberImpl::poll_the_camera()
{
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
      m_polling_timer.armX(30, 0);
   }
   else
   {
      m_polling_timer.disarm();
      m_logger->error("Capture system got broken!");
      StreamEnded *see = Q_NEW(StreamEnded, STREAM_ENDED_SIG);
      QP::QF::PUBLISH(see, this);
   }
}
void CameraGrabberImpl::running_entry()
{
   m_logger->info("Camera grabber running entry");
   m_frame_timer.armX(Config::config().get<int>("frame_interval_ms"), 0);
   m_polling_timer.armX(30, 0);
}
} // namespace GofkuCam
