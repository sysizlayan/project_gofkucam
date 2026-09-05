#include <memory>
#include <opencv2/highgui.hpp>
#include <string>
#include "Config.hpp"
#include "LoggerInterface.hpp"
#include "QPFrame.hpp"
#include "SpdlogLogger.hpp"
#include "GofkuCamCommon.hpp"
#include <thread>
#include "Config.hpp"
#include <csignal>

using namespace GofkuCam;

std::atomic<Frame*>  GofkuCam::g_depth_visualization_frame{nullptr};
std::atomic<Frame*>  GofkuCam::g_detection_visualization_frame{nullptr};
LoggerInterfacePtr   GofkuCam::g_logger;

// Global atomic flag for handling interrupts
std::atomic<bool> is_interrupted{false};

// SIGINT handler function
void signal_handler(int signal)
{
   if (signal == SIGINT)
   {
      is_interrupted.store(true);
      QP::QF::stop();
   }
}

int main(int argc, char **argv)
{  
   (void)argc;
   (void)argv;

   GofkuCam::g_logger = std::make_shared<SpdlogLogger>("GofkuCam");
   std::unique_ptr<QPFrame> m_qp_frame = std::make_unique<QPFrame>(GofkuCam::g_logger);

   GofkuCam::g_logger->info("GofkuCam constructed!");
   bool use_detached_thread_for_QP = Config::config().get<bool>("use_detached_thread");
   
   // Start the active object controller to generate timed events
   m_qp_frame->start(use_detached_thread_for_QP);
   while(!use_detached_thread_for_QP) // In case we are not using a dedicated thread, keep the main thread alive
   {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      GofkuCam::g_logger->info("GofkuCam is running");
   }
   cv::destroyAllWindows();
   GofkuCam::g_logger->info("Received interrupt signal, shutting down gracefully...");
   std::this_thread::sleep_for(std::chrono::milliseconds(2*Config::config().get<int>("frame_interval_ms")));
   return 0;
}