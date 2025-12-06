#ifndef DEPTHESTIMATOR_HPP
#define DEPTHESTIMATOR_HPP

#include "GofkuCamCommon.hpp"
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
#include <memory>
#include "LoggerInterface.hpp"

namespace GofkuCam
{


class DepthEstimator
{
public:
    DepthEstimator(const std::string &model_path, LoggerInterfacePtr logger, bool is_gpu = false);
    ~DepthEstimator() = default;
    DepthEstimator(const DepthEstimator &) = delete; // Disable copy constructor
    DepthEstimator &operator=(const DepthEstimator &) = delete; // Disable copy assignment operator

    FramePtr estimate_depth(FramePtr image);

private:
    std::string m_model_path;                   // Path to the ONNX model file
    LoggerInterfacePtr m_logger; // Logger for debug messages
    Ort::Env m_env;                         // ONNX Runtime environment
    Ort::SessionOptions m_session_options;   // Session options for ONNX Runtime
    Ort::Session m_session;                 // ONNX Runtime session for running inference

    bool isDynamicInputShape{};                    // Flag indicating if input shape is dynamic
    cv::Size inputImageShape;                      // Expected input image shape for the model

    // Vectors to hold allocated input and output node names
    std::vector<Ort::AllocatedStringPtr> inputNodeNameAllocatedStrings;
    std::vector<const char *> inputNames;
    std::vector<Ort::AllocatedStringPtr> outputNodeNameAllocatedStrings;
    std::vector<const char *> outputNames;

    size_t numInputNodes, numOutputNodes;          // Number of input and output nodes in the model

    std::vector<std::string> m_classes;            // Vector of class names loaded from file
    std::vector<cv::Scalar> m_class_colors;            // Vector of colors for each class

    /**
     * @brief Preprocesses the input image for model inference.
     * 
     * @param image Input image.
     * @param blob Reference to pointer where preprocessed data will be stored.
     * @param inputTensorShape Reference to vector representing input tensor shape.
     * @return Frame Resized image after preprocessing.
     */
    FramePtr preprocess(FramePtr image, float *&blob, std::vector<int64_t> &inputTensorShape);
    
    /**
     * @brief Postprocesses the model output to extract detections.
     * 
     * @param originalImageSize Size of the original input image.
     * @param resizedImageShape Size of the image after preprocessing.
     * @param outputTensors Vector of output tensors from the model.
     * @return std::vector<Detection> Vector of detections.
     */
    FramePtr postprocess(const Ort::Value &ort_value);

    void letter_box(FramePtr image, FramePtr outImage,
        const cv::Size& newShape,
        const cv::Scalar& color,
        bool auto_,
        bool scaleFill,
        bool scaleUp,
        int stride);

};

using DepthEstimatorPtr = std::shared_ptr<DepthEstimator>;
}

#endif // DEPTHESTIMATOR_HPP
