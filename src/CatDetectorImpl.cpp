#include "CatDetectorImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "GofkuCamCommon.hpp"
#include "ObjectDetector.hpp"
#include "qp.hpp"
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>

//#define MINI_PROFILER

namespace GofkuCam
{

CatDetectorImpl::CatDetectorImpl(QP::QActive * const owner, LoggerInterfacePtr logger)
    : m_owner{owner}
    , m_logger(logger)
{
    m_logger->info("Cat Detector initialized");
}

// State Entry/Exit implementations
void CatDetectorImpl::operating_entry(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Entering OPERATING state");
}

void CatDetectorImpl::operating_exit(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Exiting OPERATING state");
}

void CatDetectorImpl::waiting_detectors_entry(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Entering WAITING_DETECTORS state");
}

void CatDetectorImpl::waiting_detectors_exit(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Exiting WAITING_DETECTORS state");
}

void CatDetectorImpl::waiting_object_detection_entry(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Entering WAITING_OBJECT_DETECTION state");
}

void CatDetectorImpl::waiting_object_detection_exit(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Exiting WAITING_OBJECT_DETECTION state");
}

void CatDetectorImpl::waiting_depth_map_entry(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Entering WAITING_DEPTH_MAP state");
}

void CatDetectorImpl::waiting_depth_map_exit(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Exiting WAITING_DEPTH_MAP state");
}

void CatDetectorImpl::determine_cat_feeding_entry(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Entering DETERMINE_CAT_FEEDING state");
}

void CatDetectorImpl::determine_cat_feeding_exit(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Exiting DETERMINE_CAT_FEEDING state");
}

void CatDetectorImpl::not_started_entry(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Entering NOT_STARTED state");
}

void CatDetectorImpl::not_started_exit(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Exiting NOT_STARTED state");
}

// Event handler implementations
void CatDetectorImpl::start_req(QP::QEvt const * const e)
{
    (void)e;
    m_logger->info("Cat Detector start requested");
    m_owner->subscribe(FRAME_CAPTURED_SIG);
    m_owner->subscribe(STREAM_ENDED_SIG);
    m_owner->subscribe(CAPTURE_ERROR_SIG);
    m_owner->subscribe(OBJECT_DETECTION_COMPLETED_SIG);
    m_owner->subscribe(DEPTH_ESTIMATION_COMPLETED_SIG);
}

void CatDetectorImpl::stop_req(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Cat Detector stop requested");
}

void CatDetectorImpl::stream_end(QP::QEvt const * const e)
{
    (void)e;
    m_logger->info("Stream ended, stopping Cat Detector");
}

void CatDetectorImpl::running_entry(QP::QEvt const * const e)
{
    (void)e;
    m_logger->info("Cat Detector running entry");
}

void CatDetectorImpl::frame_captured(QP::QEvt const * const e)
{
    m_logger->trace("New frame to Cat Detector");

    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    #endif
    
    FramePtr copy_of_frame = std::make_shared<Frame>(Q_EVT_CAST(FrameCapturedEvt)->m_frame->clone());
    m_logger->info("Cat detector input size: " + std::to_string(copy_of_frame->cols) + "x" + std::to_string(copy_of_frame->rows));

    #ifdef MINI_PROFILER
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    m_logger->info("Cat detection took " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "[ms]");
    #endif
}

void CatDetectorImpl::object_detection_completed(QP::QEvt const * const e)
{
    m_logger->info("Object detection completed event received in Cat Detector");
    auto odce = Q_EVT_CAST(ObjectDetectionCompletedEvt);
    std::vector<Detection>& cat_or_dog_detections = *odce->m_cat_or_dog_detections;
    m_logger->info("CAT Number of cat or dog detections: " + std::to_string(cat_or_dog_detections.size()));
}

void CatDetectorImpl::depth_estimation_completed(QP::QEvt const * const e)
{
    m_logger->info("Depth estimation completed event received in Cat Detector");
    auto dece = Q_EVT_CAST(DepthEstimationCompletedEvt);
    FramePtr depth_frame = std::make_shared<Frame>(dece->m_depth_frame->clone());
    m_logger->info("CAT Depth frame size: " + std::to_string(depth_frame->cols) + "x" + std::to_string(depth_frame->rows));
}

void CatDetectorImpl::frame_timer_timeout(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Frame timer timeout");
}

void CatDetectorImpl::cat_feeding_determined(QP::QEvt const * const e) {
    (void)e;
    m_logger->info("Cat feeding status determined");
}

void CatDetectorImpl::capture_error(QP::QEvt const * const e) {
    (void)e;
    m_logger->error("Camera capture error occurred");
}

} // namespace GofkuCam