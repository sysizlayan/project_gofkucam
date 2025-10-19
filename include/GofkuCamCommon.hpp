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

    // Struct to represent a bounding box
    struct BoundingBox {
        int x;
        int y;
        int width;
        int height;

        BoundingBox() : x(0), y(0), width(0), height(0) {}
        BoundingBox(int x_, int y_, int width_, int height_)
            : x(x_), y(y_), width(width_), height(height_) {}
    };

    // Struct to represent a detection.
    struct Detection {
        BoundingBox box;
        float conf{};
        int classId{};
    };

    extern std::atomic<Frame*> g_depth_visualization_frame;
    extern std::atomic<Frame*> g_detection_visualization_frame;
}

#endif
