#ifndef CAT_DETECTOR_IMPL_HPP_
#define CAT_DETECTOR_IMPL_HPP_

#include "ICatDetector.hpp"
#include "LoggerInterface.hpp"
#include <memory>

namespace GofkuCam
{

class CatDetectorImpl : public ICatDetector {
private:
    QP::QActive * m_owner; 
    LoggerInterfacePtr m_logger;
public:
    CatDetectorImpl(QP::QActive * const owner, LoggerInterfacePtr logger);
    
    // State Entry/Exit implementations
    void operating_entry(QP::QEvt const * const e) override;
    void operating_exit(QP::QEvt const * const e) override;
    void waiting_detectors_entry(QP::QEvt const * const e) override;
    void waiting_detectors_exit(QP::QEvt const * const e) override;
    void waiting_object_detection_entry(QP::QEvt const * const e) override;
    void waiting_object_detection_exit(QP::QEvt const * const e) override;
    void waiting_depth_map_entry(QP::QEvt const * const e) override;
    void waiting_depth_map_exit(QP::QEvt const * const e) override;
    void determine_cat_feeding_entry(QP::QEvt const * const e) override;
    void determine_cat_feeding_exit(QP::QEvt const * const e) override;
    void not_started_entry(QP::QEvt const * const e) override;
    void not_started_exit(QP::QEvt const * const e) override;

    // Event handler implementations
    void start_req(QP::QEvt const * const e) override;
    void stop_req(QP::QEvt const * const e) override;
    void stream_end(QP::QEvt const * const e) override;
    void running_entry(QP::QEvt const * const e) override;
    void frame_captured(QP::QEvt const * const e) override;
    void object_detection_completed(QP::QEvt const * const e) override;
    void depth_estimation_completed(QP::QEvt const * const e) override;
    void frame_timer_timeout(QP::QEvt const * const e) override;
    void cat_feeding_determined(QP::QEvt const * const e) override;
    void capture_error(QP::QEvt const * const e) override;
}; // class CatDetectorImpl

} // namespace GofkuCam

#endif // CAT_DETECTOR_IMPL_HPP_