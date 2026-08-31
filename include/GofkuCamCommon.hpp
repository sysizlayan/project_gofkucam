#ifndef GOFKUCAMCOMMON_HPP_
#define GOFKUCAMCOMMON_HPP_

#include "qpcpp.hpp"
#include <atomic>
#include <opencv2/opencv.hpp>

namespace GofkuCam
{
using Frame = cv::Mat;
using FramePtr = std::shared_ptr<Frame>;
using ActiveObjPtr = std::shared_ptr<QP::QActive>;
constexpr int TICKS_PER_SEC = 1000;
constexpr int TICKS_BEFORE_START = 10000;
// Struct to represent a bounding box
struct BoundingBox
{
   int x;
   int y;
   int width;
   int height;

   BoundingBox() : x(0), y(0), width(0), height(0)
   {
   }
   BoundingBox(int x_, int y_, int width_, int height_) : x(x_), y(y_), width(width_), height(height_)
   {
   }
};

// Struct to represent a detection.
struct Detection
{
   BoundingBox box;
   double average_of_detection;
   float conf{};
   int classId{};
};

// Struct to represent a detection.
struct HakuStatus
{
   bool m_is_haku_in_dangerous_zone;
   double m_hakus_distance;
   HakuStatus() : m_is_haku_in_dangerous_zone(false), m_hakus_distance(0.0)
   {
   }
};

extern std::atomic<Frame*> g_depth_visualization_frame;
extern std::atomic<Frame*> g_detection_visualization_frame;
} // namespace GofkuCam

#endif
