#include "DepthEstimator.hpp"
#include "LoggerInterface.hpp"
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
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

float mean[3] = { 0.485, 0.456, 0.406};
float std[3] = { 0.229, 0.224, 0.225};

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
DepthEstimator::DepthEstimator(const std::string &model_path, LoggerInterfacePtr logger, bool is_gpu)
    : m_model_path(model_path)
    , m_logger(logger)
    , m_is_gpu{is_gpu}
    , m_env{nullptr}
    , m_session{nullptr}
    , isDynamicInputShape(false)
    , inputImageShape(0, 0)
    , numInputNodes(0)
    , numOutputNodes(0)
{
    init_session();

    std::stringstream ss1;
    ss1 << "Loaded Model: " << m_model_path << "\n"
        << "Input shape: "  << inputImageShape.width << "x" << inputImageShape.height << "\n"
        << "Number of input nodes: " << numInputNodes << "\n"
        << "Number of output nodes: " << numOutputNodes << "\n";
    m_logger->info(ss1.str());
}

void DepthEstimator::init_session()
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
    auto is_coreml_available = std::find(availableProviders.begin(), availableProviders.end(), "CoreMLExecutionProvider");
    OrtCUDAProviderOptions cudaOption;
    if (m_is_gpu && cudaAvailable != availableProviders.end())
    {
        m_logger->info("Using CUDA!");
        m_session_options.AppendExecutionProvider_CUDA(cudaOption);
    }
    else if (m_is_gpu && is_coreml_available != availableProviders.end())
    {
        m_logger->info("Using CoreML for depth estimator!");
        std::unordered_map<std::string, std::string> provider_options;
        provider_options["ModelFormat"] = "MLProgram";
        provider_options["MLComputeUnits"] = "CPUAndGPU";
        provider_options["RequireStaticInputShapes"] = "0";
        provider_options["EnableOnSubgraphs"] = "1";
        m_session_options.AppendExecutionProvider("CoreML", provider_options);
    }
    else
    {
        m_logger->info("Using CPU");
    }

    // Initialize ONNX Runtime environment with warning level
    m_env       = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "GOFKU_CAM_DEPTH");
    m_session   = Ort::Session(m_env, m_model_path.c_str(), m_session_options);

    Ort::AllocatorWithDefaultOptions allocator;

    inputNodeNameAllocatedStrings.clear();
    inputNames.clear();
    outputNodeNameAllocatedStrings.clear();
    outputNames.clear();

    // Retrieve input tensor shape information
    Ort::TypeInfo inputTypeInfo = m_session.GetInputTypeInfo(0);
    std::vector<int64_t> inputTensorShapeVec = inputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();
    isDynamicInputShape = (inputTensorShapeVec.size() >= 4) && (inputTensorShapeVec[2] == -1 && inputTensorShapeVec[3] == -1);

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
}

void DepthEstimator::recover()
{
    try
    {
        m_logger->warn("Attempting to recover DepthEstimator session after failure...");
        init_session();
        m_logger->info("DepthEstimator session recovered successfully.");
    }
    catch (const std::exception& e)
    {
        m_logger->error(std::string("Failed to recover DepthEstimator session: ") + e.what());
    }
}

// Detect function implementation
FramePtr DepthEstimator::estimate_depth(FramePtr image)
{
    if (!image || image->empty()) {
        m_logger->warn("DepthEstimator::estimate_depth called with empty image");
        return nullptr;
    }

    float* blobPtr = nullptr; // Pointer to hold preprocessed image data
    // Define the shape of the input tensor (batch size, channels, height, width)
    std::vector<int64_t> inputTensorShape = {1, 3, inputImageShape.height, inputImageShape.width};

    // Preprocess the image and obtain a pointer to the blob
    FramePtr preprocessedImage{preprocess(image, blobPtr, inputTensorShape)};
    if (!blobPtr) {
        m_logger->error("DepthEstimator preprocessing failed to allocate blob");
        return nullptr;
    }

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
    std::vector<Ort::Value> outputTensors;
    try {
        outputTensors = m_session.Run(
            Ort::RunOptions{nullptr},
            inputNames.data(),
            &inputTensor,
            numInputNodes,
            outputNames.data(),
            numOutputNodes
        );
    }
    catch (const Ort::Exception& e) {
        m_logger->error(std::string("Ort::Exception in DepthEstimator::estimate_depth: ") + e.what());
        recover();
        return nullptr;
    }
    catch (const std::exception& e) {
        m_logger->error(std::string("std::exception in DepthEstimator::estimate_depth: ") + e.what());
        recover();
        return nullptr;
    }

    if (outputTensors.empty()) {
        return nullptr;
    }

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
    m_logger->trace(ss.str()); 

    // Determine the resized image shape based on input tensor shape
    cv::Size resizedImageShape(static_cast<int>(inputTensorShape[3]), static_cast<int>(inputTensorShape[2]));

    return postprocess(outputTensor);
}


// Preprocess function implementation
FramePtr DepthEstimator::preprocess(FramePtr image, float *&blob, std::vector<int64_t> &inputTensorShape)
{
    FramePtr resized_image = std::make_shared<Frame>(image->clone());
    cv::resize(*resized_image, *resized_image, cv::Size(518, 518));

    // Convert image to float and normalize to [0, 1]
    resized_image->convertTo(*resized_image, CV_32FC3, 1 / 255.0f);

    // Update input tensor shape based on resized image dimensions
    inputTensorShape[2] = 518;
    inputTensorShape[3] = 518;

    // Allocate memory for the image blob in CHW format
    blob = new float[518 * 518 * resized_image->channels()];

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
    FramePtr depth_mat = std::make_shared<Frame>(height, width, CV_32F, const_cast<void*>(raw_data_ptr));
    cv::normalize(*depth_mat, *depth_mat, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::resize(*depth_mat, *depth_mat, cv::Size(1920, 1080));
    // Create a colormap from the depth data
    // cv::Mat colormap;
    // cv::applyColorMap(depth_mat, colormap, cv::COLORMAP_INFERNO);
    //cv::resize(colormap, colormap, cv::Size(518, 518));
    
    return depth_mat;
}

} // namespace GpfkuCam