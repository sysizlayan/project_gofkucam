#ifndef OBJECTDETECTOR_HPP
#define OBJECTDETECTOR_HPP

#include "Signals.hpp"
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include "LoggerInterface.hpp"

/**
 * @brief Confidence threshold for filtering detections.
 */
const float CONFIDENCE_THRESHOLD = 0.4f;

/**
 * @brief  IoU threshold for filtering detections.
 */
const float IOU_THRESHOLD = 0.45f;

/**
 * @brief Struct to represent a bounding box.
 */


namespace GofkuCam
{

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

/**
 * @brief Struct to represent a detection.
 */
struct Detection {
    BoundingBox box;
    float conf{};
    int classId{};
};

class ObjectDetector
{
public:
    ObjectDetector(const std::string &model_path, const std::string &label_file_path, LoggerInterfacePtr logger, bool is_cuda = false);
    ~ObjectDetector() = default;
    ObjectDetector(const ObjectDetector &) = delete; // Disable copy constructor
    ObjectDetector &operator=(const ObjectDetector &) = delete; // Disable copy assignment operator

    std::vector<Detection> detect(Frame &image, float confThreshold = 0.4f, float iouThreshold = 0.45f);

    void draw_bounding_box(Frame &image, const std::vector<Detection> &detections,
                            const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &colors);

    void draw_bounding_box_mask(Frame &image, const std::vector<Detection> &detections,
                                const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &classColors,
                                float maskAlpha);

private:
    std::string m_model_path;                   // Path to the ONNX model file
    std::string m_label_file_path;                  // Path to the labels file containing class names
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
    Frame preprocess(const Frame &image, float *&blob, std::vector<int64_t> &inputTensorShape);
    
    /**
     * @brief Postprocesses the model output to extract detections.
     * 
     * @param originalImageSize Size of the original input image.
     * @param resizedImageShape Size of the image after preprocessing.
     * @param outputTensors Vector of output tensors from the model.
     * @param confThreshold Confidence threshold to filter detections.
     * @param iouThreshold IoU threshold for Non-Maximum Suppression.
     * @return std::vector<Detection> Vector of detections.
     */
    std::vector<Detection> postprocess(const cv::Size &originalImageSize, const cv::Size &resizedImageShape,
                                      const std::vector<Ort::Value> &outputTensors,
                                      float confThreshold, float iouThreshold);

    std::vector<std::string> get_class_names(const std::string &path);

    static std::vector<cv::Scalar> generate_colors(const std::vector<std::string> &classNames, int seed);

    static void nmx_boxes(const std::vector<BoundingBox>& boundingBoxes,
                const std::vector<float>& scores,
                float scoreThreshold,
                float nmsThreshold,
                std::vector<int>& indices);

    static BoundingBox scale_coordinates(const cv::Size &imageShape, BoundingBox coords,
                            const cv::Size &imageOriginalShape, bool p_Clip);

    static void letter_box(const cv::Mat& image, cv::Mat& outImage,
                        const cv::Size& newShape,
                        const cv::Scalar& color,
                        bool auto_,
                        bool scaleFill,
                        bool scaleUp,
                        int stride);

    static size_t vector_product(const std::vector<int64_t> &vector);

    template <typename T>
    static typename std::enable_if<std::is_arithmetic<T>::value, T>::type clamp(const T &value, const T &low, const T &high);
};

using YOLO12DetectorPtr = std::shared_ptr<ObjectDetector>;
}

#endif // OBJECTDETECTOR_HPP
