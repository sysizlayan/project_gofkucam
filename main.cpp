#include <memory>
#include <opencv2/highgui.hpp>
#include <string>
#include "LoggerInterface.hpp"
#include "QPFrame.hpp"
#include "SpdlogLogger.hpp"
#include "GofkuCamCommon.hpp"


using namespace GofkuCam;

std::atomic<Frame*> GofkuCam::g_depth_visualization_frame{nullptr};
std::atomic<Frame*> GofkuCam::g_detection_visualization_frame{nullptr};

int main(int argc, char **argv)
{
   (void)argc;
   (void)argv;

   LoggerInterfacePtr logger = std::make_shared<SpdlogLogger>("GofkuCam");
   std::unique_ptr<QPFrame> m_qp_frame = std::make_unique<QPFrame>(logger);

   // Start the active object controller to generate timed events
   // This iniates a detached thread and has its own infinite loop
   m_qp_frame->start();
   int seconds_from_start = 0;
   logger->info("GofkuCam started!");
   // Convert to tensor
   while (true)
   {
      if(GofkuCam::g_detection_visualization_frame.load() != nullptr)
      {
         cv::imshow("Detections", *(GofkuCam::g_detection_visualization_frame.load()));
         cv::waitKey(1); // Allow the window to update, wait 1ms
      }
      if(GofkuCam::g_depth_visualization_frame.load() != nullptr)
      {
         cv::imshow("Depth Map", *(GofkuCam::g_depth_visualization_frame.load()));
         cv::waitKey(1);
      }
      // Simulate some processing
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      seconds_from_start++;
      if(seconds_from_start % 10 == 0)
      {
         logger->info("Running main loop for  " + std::to_string(seconds_from_start/10) + " seconds");
      }
   }
   return 0;
}