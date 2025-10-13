#include "DepthEstimator.hpp"
#include "LoggerInterface.hpp"
#include <memory>
#include <sstream>
#include "GofkuCamExceptions.hpp"
#include "GofkuCamCommon.hpp"
#include <algorithm>
#include <random>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <stdexcept> // For std::runtime_error


namespace GofkuCam
{

float mean[3] = { 123.675, 116.28, 103.53 };
float std[3] = { 58.395, 57.12, 57.375 };
// Helper to get OpenCV type from ONNX Runtime type
int GetCvType(ONNXTensorElementDataType onnx_type) {
    switch (onnx_type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:   return CV_32F;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:   return CV_8U;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8:    return CV_8S;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16:  return CV_16U;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16:   return CV_16S;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:   return CV_32S;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:  return CV_64F;
        default:
            throw std::runtime_error("Unsupported ONNXTensorElementDataType for OpenCV conversion.");
    }
}


// Implementation of ObjectDetector constructor
DepthEstimator::DepthEstimator(const std::string &model_path, LoggerInterfacePtr logger, bool is_cuda)
    : m_model_path(model_path)
    , m_logger(logger)
    , m_env{nullptr}
    , m_session_options{nullptr}
    , m_session{nullptr}
    , isDynamicInputShape(false)
    , inputImageShape(0, 0)
    , numInputNodes(0)
    , numOutputNodes(0)
{
    m_session_options = Ort::SessionOptions();
    m_session_options.SetIntraOpNumThreads(4);
    m_session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    // Check what you have as execution providers
    std::vector<std::string> availableProviders = Ort::GetAvailableProviders();
    std::stringstream ss;
    ss << "Available execution providers: ";
    for (const auto& provider : availableProviders)
    {
        ss << provider << "\n";
    }
    m_logger->info(ss.str());

    auto cudaAvailable = std::find(availableProviders.begin(), availableProviders.end(), "CUDAExecutionProvider");
    OrtCUDAProviderOptions cudaOption;
    if (is_cuda && cudaAvailable != availableProviders.end())
    {
        m_logger->info("Using CUDA!");
        m_session_options.AppendExecutionProvider_CUDA(cudaOption); // Append CUDA execution provider
    }
    else if(is_cuda && cudaAvailable == availableProviders.end())
    {
        m_logger->warn("CUDAExecutionProvider is not available. Falling back to CPU.");
    }
    else
    {
        m_logger->info("Using CPU");
    }

    // Initialize ONNX Runtime environment with warning level
    m_env       = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "GOFKU_CAM_DEPTH");
    m_session   = Ort::Session(m_env, m_model_path.c_str(), m_session_options);

    Ort::AllocatorWithDefaultOptions allocator;

    // Retrieve input tensor shape information
    Ort::TypeInfo inputTypeInfo = m_session.GetInputTypeInfo(0);
    std::vector<int64_t> inputTensorShapeVec = inputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();
    isDynamicInputShape = (inputTensorShapeVec.size() >= 4) && (inputTensorShapeVec[2] == -1 && inputTensorShapeVec[3] == -1); // Check for dynamic dimensions

    // Allocate and store input node names
    auto input_name = m_session.GetInputNameAllocated(0, allocator);
    inputNodeNameAllocatedStrings.push_back(std::move(input_name));
    inputNames.push_back(inputNodeNameAllocatedStrings.back().get());

    // Allocate and store output node names
    auto output_name = m_session.GetOutputNameAllocated(0, allocator);
    outputNodeNameAllocatedStrings.push_back(std::move(output_name));
    outputNames.push_back(outputNodeNameAllocatedStrings.back().get());

    // Set the expected input image shape based on the model's input tensor
    if (inputTensorShapeVec.size() >= 4)
    {
        inputImageShape = cv::Size(static_cast<int>(inputTensorShapeVec[3]), static_cast<int>(inputTensorShapeVec[2]));
    }
    else
    {
        throw DetectorInilializationError("Tensor shape is not compatible with expected input dimensions.");
    }

    // Get the number of input and output nodes
    numInputNodes = m_session.GetInputCount();
    numOutputNodes = m_session.GetOutputCount();

    std::stringstream ss1;
    ss1 << "Loaded Model: " << m_model_path << "\n"
        << "Input shape: "  << inputImageShape.width << "x" << inputImageShape.height << "\n"
        << "Number of input nodes: " << numInputNodes << "\n"
        << "Number of output nodes: " << numOutputNodes << "\n";
    m_logger->info(ss1.str());
}

// Detect function implementation
FramePtr DepthEstimator::estimate_depth(FramePtr image)
{
    // //ScopedTimer timer("Overall detection");

    float* blobPtr = nullptr; // Pointer to hold preprocessed image data
    // Define the shape of the input tensor (batch size, channels, height, width)
    std::vector<int64_t> inputTensorShape = {1, 3, inputImageShape.height, inputImageShape.width};

    // Preprocess the image and obtain a pointer to the blob
    FramePtr preprocessedImage{preprocess(image, blobPtr, inputTensorShape)};

    // Compute the total number of elements in the input tensor
    size_t inputTensorSize = std::accumulate(inputTensorShape.begin(), inputTensorShape.end(), 1ull, std::multiplies<size_t>());

    // Create a vector from the blob data for ONNX Runtime input
    std::vector<float> inputTensorValues(blobPtr, blobPtr + inputTensorSize);

    delete[] blobPtr; // Free the allocated memory for the blob

    // Create an Ort memory info object (can be cached if used repeatedly)
    static Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    // Create input tensor object using the preprocessed data
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo,
        inputTensorValues.data(),
        inputTensorSize,
        inputTensorShape.data(),
        inputTensorShape.size()
    );

    // Run the inference session with the input tensor and retrieve output tensors
    std::vector<Ort::Value> outputTensors = m_session.Run(
        Ort::RunOptions{nullptr},
        inputNames.data(),
        &inputTensor,
        numInputNodes,
        outputNames.data(),
        numOutputNodes
    );
    Ort::Value &outputTensor = outputTensors[0];

    std::stringstream ss;
    ss << "\nInput tensor shape: [";
    for (size_t i = 0; i < inputTensorShape.size(); ++i)
    {
        ss << inputTensorShape[i];
        if (i < inputTensorShape.size() - 1) ss << ", ";
    }
    ss << "]\n";
    ss<< "Output tensor shapes: ";
    for (size_t i = 0; i < outputTensors.size(); ++i)
    {
        auto shape = outputTensors[i].GetTensorTypeAndShapeInfo().GetShape();
        ss << "[";
        for (size_t j = 0; j < shape.size(); ++j)
        {
            ss << shape[j];
            if (j < shape.size() - 1) ss << ", ";
        }
        ss << "] ";
    }
    ss << "\n";
    m_logger->info(ss.str()); 

    // Determine the resized image shape based on input tensor shape
    cv::Size resizedImageShape(static_cast<int>(inputTensorShape[3]), static_cast<int>(inputTensorShape[2]));

    return postprocess(outputTensor);
}


// Preprocess function implementation
FramePtr DepthEstimator::preprocess(FramePtr image, float *&blob, std::vector<int64_t> &inputTensorShape)
{
    FramePtr resized_image = std::make_shared<Frame>();
    // Resize and pad the image using letterBox utility
    DepthEstimator::letter_box(image, resized_image, inputImageShape, cv::Scalar(114, 114, 114), isDynamicInputShape, false, true, 32);

    // Update input tensor shape based on resized image dimensions
    inputTensorShape[2] = resized_image->rows;
    inputTensorShape[3] = resized_image->cols;

    // for (int k = 0; k < 3; k++)
    // {
    //     for (int i = 0; i < resized_image->rows; i++)
    //     {
    //         for (int j = 0; j < resized_image->cols; j++)
    //         {
    //             resized_image->at<cv::Vec3b>(i, j) = (resized_image->at<cv::Vec3b>(i, j)[k] - mean[k]) / std[k];
    //         }
    //     }
    // }

    // Convert image to float and normalize to [0, 1]
    resized_image->convertTo(*resized_image, CV_32FC3, 1 / 255.0f);

    // Allocate memory for the image blob in CHW format
    blob = new float[resized_image->cols * resized_image->rows * resized_image->channels()];

    // Split the image into separate channels and store in the blob
    std::vector<Frame> chw(resized_image->channels());
    for (int i = 0; i < resized_image->channels(); ++i)
    {
        chw[i] = Frame(resized_image->rows, resized_image->cols, CV_32FC1, blob + i * resized_image->cols * resized_image->rows);
    }
    cv::split(*resized_image, chw); // Split channels into the blob

    return resized_image;
}

// Postprocess function to convert raw model output into detections
FramePtr DepthEstimator::postprocess(const Ort::Value &ort_value)
{
    if (!ort_value.IsTensor())
    {
        throw std::runtime_error("Ort::Value is not a tensor.");
    }

    // Get tensor infoF OrtVa
    Ort::TensorTypeAndShapeInfo type_info = ort_value.GetTensorTypeAndShapeInfo();
    ONNXTensorElementDataType onnx_type = type_info.GetElementType();
    std::vector<int64_t> shape = type_info.GetShape();

    // EXPECTING SHAPE: 1xHxW (e.g., 1x518x518)
    if (shape.size() != 3 || shape[0] != 1)
    {
        throw std::runtime_error("Unsupported tensor shape for grayscale conversion. Expected 1xHxW.");
    }

    int height = static_cast<int>(shape[1]);
    int width = static_cast<int>(shape[2]);
    int channels = 1; // Grayscale

    // Determine OpenCV data type
    int cv_depth = GetCvType(onnx_type);

    // Get a pointer to the raw data
    const void* raw_data_ptr = ort_value.GetTensorRawData();

    // Create cv::Mat. Since it's grayscale, we just need height, width, and type.
    // The Mat will be a view into the Ort::Value's memory.
    cv::Mat depth_mat(height, width, CV_32F, const_cast<void*>(raw_data_ptr));
    cv::normalize(depth_mat, depth_mat, 0, 255, cv::NORM_MINMAX, CV_8U);

    // Create a colormap from the depth data
    cv::Mat colormap;
    cv::applyColorMap(depth_mat, colormap, cv::COLORMAP_INFERNO);
    //cv::resize(colormap, colormap, cv::Size(518, 518));
    
    return std::make_shared<Frame>(colormap);
}


void DepthEstimator::letter_box(FramePtr image, FramePtr outImage,
                    const cv::Size& newShape,
                    const cv::Scalar& color,
                    bool auto_,
                    bool scaleFill,
                    bool scaleUp,
                    int stride)
{
    // Calculate the scaling ratio to fit the image within the new shape
    float ratio = std::min(static_cast<float>(newShape.height) / image->rows,
                        static_cast<float>(newShape.width) / image->cols);

    // Prevent scaling up if not allowed
    if (!scaleUp) {
        ratio = std::min(ratio, 1.0f);
    }

    // Calculate new dimensions after scaling
    int newUnpadW = static_cast<int>(std::round(image->cols * ratio));
    int newUnpadH = static_cast<int>(std::round(image->rows * ratio));

    // Calculate padding needed to reach the desired shape
    int dw = newShape.width - newUnpadW;
    int dh = newShape.height - newUnpadH;

    if (auto_)
    {
        // Ensure padding is a multiple of stride for model compatibility
        dw = (dw % stride) / 2;
        dh = (dh % stride) / 2;
    } 
    else if (scaleFill)
    {
        // Scale to fill without maintaining aspect ratio
        newUnpadW = newShape.width;
        newUnpadH = newShape.height;
        ratio = std::min(static_cast<float>(newShape.width) / image->cols,
                        static_cast<float>(newShape.height) / image->rows);
        dw = 0;
        dh = 0;
    } 
    else
    {
        // Evenly distribute padding on both sides
        // Calculate separate padding for left/right and top/bottom to handle odd padding
        int padLeft = dw / 2;
        int padRight = dw - padLeft;
        int padTop = dh / 2;
        int padBottom = dh - padTop;

        // Resize the image if the new dimensions differ
        if (image->cols != newUnpadW || image->rows != newUnpadH)
        {
            cv::resize(*image, *outImage, cv::Size(newUnpadW, newUnpadH), 0, 0, cv::INTER_LINEAR);
        } 
        else
        {
            // Avoid unnecessary copying if dimensions are the same
            *outImage = *image;
        }

        // Apply padding to reach the desired shape
        cv::copyMakeBorder(*outImage, *outImage, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, color);
        return; // Exit early since padding is already applied
    }

    // Resize the image if the new dimensions differ
    if (image->cols != newUnpadW || image->rows != newUnpadH)
    {
        cv::resize(*image, *outImage, cv::Size(newUnpadW, newUnpadH), 0, 0, cv::INTER_LINEAR);
    } 
    else
    {
        // Avoid unnecessary copying if dimensions are the same
        *outImage = *image;
    }

    // Calculate separate padding for left/right and top/bottom to handle odd padding
    int padLeft = dw / 2;
    int padRight = dw - padLeft;
    int padTop = dh / 2;
    int padBottom = dh - padTop;

    // Apply padding to reach the desired shape
    cv::copyMakeBorder(*outImage, *outImage, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, color);
}


} // namespace GpfkuCam