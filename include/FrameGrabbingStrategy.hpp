#ifndef FRAMEGRABBINGSTRATEGY_HPP
#define FRAMEGRABBINGSTRATEGY_HPP

#include <memory>
#include <opencv2/opencv.hpp>
#include "GofkuCamCommon.hpp"

namespace GofkuCam
{
// Strategy interface for frame grabbing
class FrameGrabbingStrategy
{
public:
   virtual ~FrameGrabbingStrategy() = default;

   virtual void open() = 0;
   
   virtual void open(const std::string& source) = 0;
   
   virtual void close() = 0;

   virtual void grabFrame(Frame&) = 0;

   virtual void subscribeToFrameUpdates(ActiveObjPtr) = 0;

   virtual void unsubscribeFromFrameUpdates(ActiveObjPtr) = 0;
   
   virtual void notify_new_frame() = 0;
};

using FrameGrabbingStrategyPtr = std::shared_ptr<FrameGrabbingStrategy>;

} // namespace GofkuCam

#endif // FRAMEGRABBINGSTRATEGY_HPP
