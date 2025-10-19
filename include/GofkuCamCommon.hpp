#ifndef GOFKUCAMCOMMON_HPP_
#define GOFKUCAMCOMMON_HPP_

#include "qpcpp.hpp"
#include <opencv2/opencv.hpp>
#include <atomic>

namespace GofkuCam
{
    using Frame = cv::Mat;
    using FramePtr = std::shared_ptr<Frame>;
    using ActiveObjPtr = std::shared_ptr<QP::QActive>;
    constexpr int TICKS_PER_SEC = 1000;

    extern std::atomic<Frame*> g_depth_visualization_frame;
    extern std::atomic<Frame*> g_detection_visualization_frame;
}

#endif
