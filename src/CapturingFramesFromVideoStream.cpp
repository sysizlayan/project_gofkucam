#include "CapturingFramesFromVideoStream.hpp"
#include "Evts.hpp"
#include "GofkuCamExceptions.hpp"
#include "Signals.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/videoio.hpp"
#include <memory>
#include <sstream>
#include <thread>

namespace GofkuCam
{

CapturingFramesFromVideoStream::CapturingFramesFromVideoStream(const std::string source, LoggerInterfacePtr logger) 
   : m_source(source)
   , m_logger(logger)
   , m_running(false)
   , m_cap(nullptr)
   , m_latest_frame(nullptr)
   , m_thread()
{
   
}

CapturingFramesFromVideoStream::~CapturingFramesFromVideoStream()
{
   close();
}

void CapturingFramesFromVideoStream::open(const std::string &source)
{
   m_source = source;
   open();
}

void CapturingFramesFromVideoStream::open()
{
   m_logger->info("Starting the video stream!");
   m_logger->info(m_source);
   if(!m_running)
   {
      m_cap = std::make_unique<cv::VideoCapture>(m_source);
      
      auto latest_frame_ptr = std::make_shared<cv::Mat>();
      m_latest_frame.store(latest_frame_ptr);

      if (!m_cap->isOpened())
      {
         std::stringstream ss;
         ss<<"Could not connect to the camera: " << m_source;
         throw CameraConnectionFailure(ss.str());
      }

      m_running = true;
      m_thread = std::thread(&CapturingFramesFromVideoStream::acquisition_loop, this);
   }
   else
   {
      close();
      open();
   }
}

void CapturingFramesFromVideoStream::close()
{
   if (m_running)
   {
      m_running = false;
      if (m_thread.joinable())
      {
         m_thread.join();
      }
      m_cap->release();
      cv::destroyAllWindows();

      m_cap.reset();
      m_cap = nullptr;
   }
}

void CapturingFramesFromVideoStream::grabFrame(Frame& frame)
{
   if(!m_running)
   {
      throw FrameCaptureFailure("Frame cannot be captured");
   }
   frame = *m_latest_frame.load();
}

void CapturingFramesFromVideoStream::acquisition_loop()
{
   static int i = 0;
   while(m_running)
   {
      i++;
      std::stringstream ss;
      ss<<"Frame " << i << " arrived.";
      m_logger->trace(ss.str());

      *m_cap >> *m_latest_frame.load();
      if ((m_latest_frame.load())->empty())
      {
         m_logger->error("Frame is received empty");
      }
      //*m_latest_frame.load() = frame;
      notify_new_frame();
   }
   m_cap->release();
}

void CapturingFramesFromVideoStream::subscribeToFrameUpdates(ActiveObjPtr observer)
{
   observer->subscribe(FRAME_CAPTURED_SIG);
}

void CapturingFramesFromVideoStream::unsubscribeFromFrameUpdates(ActiveObjPtr observer)
{
   observer->unsubscribe(FRAME_CAPTURED_SIG);
}

void CapturingFramesFromVideoStream::notify_new_frame()
{
   FrameCapturedEvt *fce = Q_NEW(FrameCapturedEvt, FRAME_CAPTURED_SIG);
   QP::QF::PUBLISH(fce, this);
}

}