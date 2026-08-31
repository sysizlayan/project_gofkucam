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
   logger->info("GofkuCam started!");
   m_qp_frame->start(false);
   return 0;
}