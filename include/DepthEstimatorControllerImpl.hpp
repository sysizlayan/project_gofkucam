#ifndef DEPTH_ESTIMATOR_CONTROLLER_IMPL_HPP_
#define DEPTH_ESTIMATOR_CONTROLLER_IMPL_HPP_

#include "IDetectorController.hpp"
#include "LoggerInterface.hpp"
#include "ObjectDetector.hpp"
#include "DepthEstimator.hpp"
#include <memory>

namespace GofkuCam
{

class DepthEstimatorControllerImpl : public IDetectorController {
private:
    QP::QActive * m_owner; 
    LoggerInterfacePtr m_logger;
    DepthEstimatorPtr m_depth_estimator;
public:
    DepthEstimatorControllerImpl(QP::QActive * const owner, LoggerInterfacePtr logger);
    
    // IDetectorController interface implementation
    void start_req(QP::QEvt const * const e) override;
    void stream_end(QP::QEvt const * const e) override;
    void running_entry(QP::QEvt const * const e) override;
    void frame_captured(QP::QEvt const * const e) override;
}; // class DetectorControllerImpl

} // namespace GofkuCam

#endif // DETECTOR_CONTROLLER_IMPL_HPP_