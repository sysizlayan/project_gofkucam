#ifndef IDETECTOR_CONTROLLER_HPP_
#define IDETECTOR_CONTROLLER_HPP_

#include <memory>
#include "qpcpp.hpp"

namespace GofkuCam
{

class IDetectorController {
public:
    virtual void start_req(QP::QEvt const * const e) = 0;
    virtual void stream_end(QP::QEvt const * const e) = 0;
    virtual void running_entry(QP::QEvt const * const e) = 0;
    virtual void frame_captured(QP::QEvt const * const e) = 0;
    virtual void calculating_entry(QP::QEvt const * const e) = 0;
    virtual void idle_entry(QP::QEvt const * const e) = 0;

    virtual ~IDetectorController() = default;
}; // class IDetectorController

using IDetectorControllerPtr = std::shared_ptr<IDetectorController>;

} // namespace GofkuCam

#endif
