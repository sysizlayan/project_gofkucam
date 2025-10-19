#ifndef ICAT_DETECTOR_HPP_
#define ICAT_DETECTOR_HPP_

#include <memory>
#include "qpcpp.hpp"

namespace GofkuCam
{

class ICatDetector {
public:
    // State Entry/Exit functions
    virtual void operating_entry(QP::QEvt const * const e) = 0;
    virtual void operating_exit(QP::QEvt const * const e) = 0;
    virtual void waiting_detectors_entry(QP::QEvt const * const e) = 0;
    virtual void waiting_detectors_exit(QP::QEvt const * const e) = 0;
    virtual void waiting_object_detection_entry(QP::QEvt const * const e) = 0;
    virtual void waiting_object_detection_exit(QP::QEvt const * const e) = 0;
    virtual void waiting_depth_map_entry(QP::QEvt const * const e) = 0;
    virtual void waiting_depth_map_exit(QP::QEvt const * const e) = 0;
    virtual void determine_cat_feeding_entry(QP::QEvt const * const e) = 0;
    virtual void determine_cat_feeding_exit(QP::QEvt const * const e) = 0;
    virtual void not_started_entry(QP::QEvt const * const e) = 0;
    virtual void not_started_exit(QP::QEvt const * const e) = 0;

    // Event handler functions
    virtual void start_req(QP::QEvt const * const e) = 0;
    virtual void stop_req(QP::QEvt const * const e) = 0;
    virtual void stream_end(QP::QEvt const * const e) = 0;
    virtual void running_entry(QP::QEvt const * const e) = 0;
    virtual void frame_captured(QP::QEvt const * const e) = 0;
    virtual void object_detection_completed(QP::QEvt const * const e) = 0;
    virtual void depth_estimation_completed(QP::QEvt const * const e) = 0;
    virtual void frame_timer_timeout(QP::QEvt const * const e) = 0;
    virtual void cat_feeding_determined(QP::QEvt const * const e) = 0;
    virtual void capture_error(QP::QEvt const * const e) = 0;
    
    virtual ~ICatDetector() = default;
}; // class ICatDetector

using ICatDetectorPtr = std::shared_ptr<ICatDetector>;

} // namespace GofkuCam

#endif
