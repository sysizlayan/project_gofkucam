#include <memory>
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

   // Convert to tensor
   while (true)
   {
      // Simulate some processing
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      logger->info("Running main loop...");
   }
   return 0;
}