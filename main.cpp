#include <memory>
#include <string>
#include "LoggerInterface.hpp"
#include "QPFrame.hpp"
#include "SpdlogLogger.hpp"



using namespace GofkuCam;
int main(int argc, char **argv)
{
   (void)argc;
   (void)argv;

   LoggerInterfacePtr logger = std::make_shared<SpdlogLogger>("GofkuCam");
   QPFrame m_qp_frame(logger);

   // Start the active object controller to generate timed events
   // This iniates a detached thread and has its own infinite loop
   m_qp_frame.start();
   int seconds_from_start = 0;
   logger->info("GofkuCam started!");
   // Convert to tensor
   while (true)
   {
      // Simulate some processing
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      seconds_from_start++;
      logger->info("Running main loop for  " + std::to_string(seconds_from_start) + " seconds");
   }
   return 0;
}