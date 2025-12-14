#include "QPFrame.hpp"
#include <iostream>

#include "Evts.hpp"
#include <memory>
#include <sstream>
#include <thread>
#include <chrono>
#include "Config.hpp"
#include "GofkuCamCommon.hpp"


constexpr size_t KB = 1024;
constexpr size_t MB = 1024 * KB;

namespace GofkuCam
{
QPFrame::QPFrame(LoggerInterfacePtr logger)
   : m_logger(logger)
   , m_detector_controller(nullptr)
   , m_depth_estimator_controller(nullptr)
   , m_camera_grabber(nullptr)
   , m_cat_detector(nullptr)
   , m_mqtt_client(nullptr)
{
   //Initialize QFramework
   QP::QF::init();

   // Init Pub-sub
   QP::QF::psInit(m_subscrSto, Q_DIM(m_subscrSto));
   //QP::QF::poolInit(m_cat_events_pool, sizeof(m_cat_events_pool), sizeof(m_cat_events_pool[0]));
   QP::QF::poolInit(m_event_memory_pool, sizeof(m_event_memory_pool), sizeof(m_event_memory_pool[0]));
   //QP::QF::poolInit(m_frame_events_pool, sizeof(m_frame_events_pool), sizeof(m_frame_events_pool[0]));

   std::string source = Config::config().get<std::string>("stream_address");

   std::stringstream ss;
   ss<<"Using " << source << " as video source! QP Framework constructed.";
   m_logger->info(ss.str());
}

void QPFrame::start()
{
   aoThread_ = std::thread(&QPFrame::ao_thread_func, this);
   aoThread_.detach();
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void QPFrame::ao_thread_func()
{
   m_logger->info("QPFrame external thread started.");

   m_camera_grabber = std::make_shared<CameraGrabber>(m_logger, Config::config().get<std::string>("stream_address"));
   m_detector_controller = std::make_shared<DetectorController>(m_logger);
   m_depth_estimator_controller = std::make_shared<DepthEstimatorController>(m_logger);
   m_cat_detector = std::make_shared<CatDetector>(m_logger);
   m_mqtt_client = std::make_shared<MqttClient>(
      Config::config().get<std::string>("mqtt_broker_url"),
      Config::config().get<std::uint32_t>("mqtt_broker_port"),
      m_logger);

   m_camera_grabber->start(
      1, 
      m_gofkucam_camera_grabber_queue,
      Q_DIM(m_gofkucam_camera_grabber_queue),
      nullptr,
      0);

   m_detector_controller->start(
      2, 
      m_gofkucam_controller_queue,
      Q_DIM(m_gofkucam_controller_queue),
      nullptr,
      0);

   m_depth_estimator_controller->start(
      3, 
      m_gofkucam_controller_queue,
      Q_DIM(m_gofkucam_controller_queue),
      nullptr,
      0);

   m_cat_detector->start(
      4, 
      m_gofkucam_cat_detector_queue,
      Q_DIM(m_gofkucam_cat_detector_queue),
      nullptr,
      0);
   
   m_mqtt_client->start(
      5, 
      m_gofkucam_mqtt_event_queue,
      Q_DIM(m_gofkucam_mqtt_event_queue),
      nullptr,
      0);

   StartRequested* sr = Q_NEW(StartRequested, START_REQ_SIG);
   QP::QF::PUBLISH(sr, this);

   // Ticker loop
   // Does not return till SIGTERM!
   int result = QP::QF::run();

   std::stringstream ss;
   ss<<"Terminating QP Framework thread, result:" << result;
   m_logger->info(ss.str());
}
}

/*--------------------------------------------------------------------------*/
void QP::QF::onStartup()
{
   setTickRate(GofkuCam::TICKS_PER_SEC, 50); // desired tick rate/prio -> 1000 ticks per second!
}

/*--------------------------------------------------------------------------*/
void QP::QF::onCleanup()
{
}

//............................................................................
void QP::QF::onClockTick()
{
   QTimeEvt::TICK_X(0U, 0); // process time events at rate 0
   //  QS_RX_INPUT(); // handle the QS-RX input
   //  QS_OUTPUT();   // handle the QS output
}

/*--------------------------------------------------------------------------*/
// this function is used by the QP embedded systems-friendly assertions
extern "C" Q_NORETURN Q_onAssert(char const * const module, int_t const loc)
{
   std::cout<<"Assertion failed in "<< module<<" "<< loc<<std::endl;
   QP::QF::stop();  // stop ticking
   QS_ASSERTION(module, loc, 10000U); // report assertion to QS

   exit(-1);
}
