#ifndef QPFRAME_HPP
#define QPFRAME_HPP
#include <memory>
#include <thread>

#include "Evts.hpp"
#include "LoggerInterface.hpp"
#include "DetectorController.hpp"
#include "CameraGrabber.hpp"
#include "CatDetector.hpp"
#include "DepthEstimatorController.hpp"
#include "MqttClient.hpp"

#include <variant>
constexpr size_t NUM_STORED_EVENTS = 100;


namespace GofkuCam
{
using EventTyp = std::variant<FrameCapturedEvt, FrameTimerTimeout, StreamEnded, CaptureError, StartRequested, StopRequested, PollingTimerTimeout, ObjectDetectionCompletedEvt, DepthEstimationCompletedEvt, CatFeedingDetermined, ReconnectionTimerTimerTimeout, DisconnectedFromBroker, ConnectedToBroker>;
class QPFrame
/**
 * @class QPFrame
 * @brief Factory-pattern-like class responsible for constructing and starting all active objects and their required interfaces, 
 * holding the thread of QP framework and ticking mechanism.
 *
 * QPFrame serves as the main entry point for initializing and managing the lifecycle of active objects within the system.
 * It encapsulates the creation of controllers, frame grabbing strategies, and manages event queues and memory pools required
 * for event-driven processing. The class ensures that all dependencies are properly constructed and started, providing a
 * centralized location for system startup and shutdown logic.
 *
 * @note This class is designed to be used with the QP framework (Quantum Leaps Framework, state-machine.com) for active object-based applications.
 */
{

public:
    QPFrame(LoggerInterfacePtr);
    virtual ~QPFrame() = default;
    void start(bool should_use_a_dedicated_thread = false);
private:
    LoggerInterfacePtr  m_logger;
    std::shared_ptr<DetectorController>   m_detector_controller;
    std::shared_ptr<DepthEstimatorController>   m_depth_estimator_controller;
    std::shared_ptr<CameraGrabber> m_camera_grabber;
    std::shared_ptr<CatDetector> m_cat_detector;
    std::shared_ptr<MqttClient> m_mqtt_client;


    QP::QEvtPtr m_gofkucam_controller_queue[NUM_STORED_EVENTS];
    QP::QEvtPtr m_gofkucam_depth_estimator_queue[NUM_STORED_EVENTS];
    QP::QEvtPtr m_gofkucam_cat_detector_queue[NUM_STORED_EVENTS];
    QP::QEvtPtr m_gofkucam_camera_grabber_queue[NUM_STORED_EVENTS];
    QP::QEvtPtr m_gofkucam_mqtt_event_queue[10];

    QP::QSubscrList m_subscrSto[MAX_GOFKU_CAM_SIG];

    QF_MPOOL_EL(EventTyp)    m_event_memory_pool[5*NUM_STORED_EVENTS];
    //QF_MPOOL_EL(FrameEvts)   m_frame_events_pool[NUM_STORED_EVENTS];
    //QF_MPOOL_EL(CatFeedingDetermined) m_cat_events_pool[NUM_STORED_EVENTS];

    std::thread         aoThread_;
    void                ao_thread_func();
};

} // namespace GofkuCam

#endif // QPFRAME_HPP
