#ifndef VIDEOCAPTURESTRATEGY_HPP
#define VIDEOCAPTURESTRATEGY_HPP

#include "FrameGrabbingStrategy.hpp"

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <opencv2/opencv.hpp>

#include "LoggerInterface.hpp"

namespace GofkuCam
{

// Concrete strategy using OpenCV's VideoCapture
class CapturingFramesFromVideoStream : public FrameGrabbingStrategy
{
public:
    CapturingFramesFromVideoStream(const std::string source, LoggerInterfacePtr logger);
    ~CapturingFramesFromVideoStream() override;

    void open();
    void open(const std::string& source) override;
    void close() override;

    void grabFrame(Frame& frame) override;

    void subscribeToFrameUpdates(ActiveObjPtr observer) override;
    void unsubscribeFromFrameUpdates(ActiveObjPtr observer) override;
    void notify_new_frame() override;
private:
    std::string m_source;
    LoggerInterfacePtr m_logger;

    std::atomic<bool> m_running;
    std::unique_ptr<cv::VideoCapture> m_cap;
    std::atomic<FramePtr> m_latest_frame;
    
    std::thread m_thread;
    void acquisition_loop();

};

} // namespace GofkuCam

#endif // VIDEOCAPTURESTRATEGY_HPP
