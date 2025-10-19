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
    
    // IDetectorController interface implementation
    void start_req(QP::QEvt const * const e) override;
    void stream_end(QP::QEvt const * const e) override;
    void running_entry(QP::QEvt const * const e) override;
    void frame_captured(QP::QEvt const * const e) override;
    void object_detection_completed(QP::QEvt const * const e) override;
    void depth_estimation_completed(QP::QEvt const * const e) override;
}; // class CatDetectorImpl

} // namespace GofkuCam

#endif // CAT_DETECTOR_IMPL_HPP_