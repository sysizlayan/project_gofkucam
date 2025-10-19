#ifndef ICAT_DETECTOR_HPP_
#define ICAT_DETECTOR_HPP_

#include <memory>
#include "qpcpp.hpp"

namespace GofkuCam
{

class ICatDetector {
public:
    virtual void start_req(QP::QEvt const * const e) = 0;
    virtual void stream_end(QP::QEvt const * const e) = 0;
    virtual void running_entry(QP::QEvt const * const e) = 0;
    virtual void frame_captured(QP::QEvt const * const e) = 0;
    virtual ~ICatDetector() = default;
}; // class ICatDetector

using ICatDetectorPtr = std::shared_ptr<ICatDetector>;

} // namespace GofkuCam

#endif
