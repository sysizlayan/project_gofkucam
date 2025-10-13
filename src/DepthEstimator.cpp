#include "DepthEstimator.hpp"
#include "LoggerInterface.hpp"
#include <memory>
#include <sstream>
#include "GofkuCamExceptions.hpp"
#include "GofkuCamCommon.hpp"
#include <algorithm>
#include <random>
#include <fstream>

namespace GofkuCam
{

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
    // m_session_options = Ort::SessionOptions();
    // m_session_options.SetIntraOpNumThreads(4);
    // m_session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    // // Check what you have as execution providers
    // std::vector<std::string> availableProviders = Ort::GetAvailableProviders();
    // std::stringstream ss;
    // ss << "Available execution providers: ";
    // for (const auto& provider : availableProviders)
    // {
    //     ss << provider << "\n";
    // }
    // m_logger->info(ss.str());

    // auto cudaAvailable = std::find(availableProviders.begin(), availableProviders.end(), "CUDAExecutionProvider");
    // OrtCUDAProviderOptions cudaOption;
    // if (is_cuda && cudaAvailable != availableProviders.end())
    // {
    //     m_logger->info("Using CUDA!");
    //     m_session_options.AppendExecutionProvider_CUDA(cudaOption); // Append CUDA execution provider
    // }
    // else if(is_cuda && cudaAvailable == availableProviders.end())
    // {
    //     m_logger->warn("CUDAExecutionProvider is not available. Falling back to CPU.");
    // }
    // else
    // {
    //     m_logger->info("Using CPU");
    // }

    // // Initialize ONNX Runtime environment with warning level
    // m_env       = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "GOFKU_CAM");
    // m_session   = Ort::Session(m_env, m_model_path.c_str(), m_session_options);

    // Ort::AllocatorWithDefaultOptions allocator;

    // // Retrieve input tensor shape information
    // Ort::TypeInfo inputTypeInfo = m_session.GetInputTypeInfo(0);
    // std::vector<int64_t> inputTensorShapeVec = inputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();
    // isDynamicInputShape = (inputTensorShapeVec.size() >= 4) && (inputTensorShapeVec[2] == -1 && inputTensorShapeVec[3] == -1); // Check for dynamic dimensions

    // // Allocate and store input node names
    // auto input_name = m_session.GetInputNameAllocated(0, allocator);
    // inputNodeNameAllocatedStrings.push_back(std::move(input_name));
    // inputNames.push_back(inputNodeNameAllocatedStrings.back().get());

    // // Allocate and store output node names
    // auto output_name = m_session.GetOutputNameAllocated(0, allocator);
    // outputNodeNameAllocatedStrings.push_back(std::move(output_name));
    // outputNames.push_back(outputNodeNameAllocatedStrings.back().get());

    // // Set the expected input image shape based on the model's input tensor
    // if (inputTensorShapeVec.size() >= 4)
    // {
    //     inputImageShape = cv::Size(static_cast<int>(inputTensorShapeVec[3]), static_cast<int>(inputTensorShapeVec[2]));
    // }
    // else
    // {
    //     throw DetectorInilializationError("Tensor shape is not compatible with expected input dimensions.");
    // }

    // // Get the number of input and output nodes
    // numInputNodes = m_session.GetInputCount();
    // numOutputNodes = m_session.GetOutputCount();

    // // Load class names and generate corresponding colors
    // m_classes      = get_class_names(m_label_file_path);
    // m_class_colors = ObjectDetector::generate_colors(m_classes, 0);

    // std::stringstream ss1;
    // ss1 << "Loaded Model: " << m_model_path << "\n"
    //            << "Input shape: " << inputImageShape.width << "x" << inputImageShape.height << "\n"
    //            << "Number of input nodes: " << numInputNodes << "\n"
    //            << "Number of output nodes: " << numOutputNodes << "\n"
    //            << "Number of classes loaded: " << m_classes.size() << "\n";
    // m_logger->info(ss1.str());
}

// Detect function implementation
void DepthEstimator::estimate_depth(FramePtr image)
{
    // //ScopedTimer timer("Overall detection");

    // float* blobPtr = nullptr; // Pointer to hold preprocessed image data
    // // Define the shape of the input tensor (batch size, channels, height, width)
    // std::vector<int64_t> inputTensorShape = {1, 3, inputImageShape.height, inputImageShape.width};

    // // Preprocess the image and obtain a pointer to the blob
    // Frame preprocessedImage = preprocess(image, blobPtr, inputTensorShape);

    // // Compute the total number of elements in the input tensor
    // size_t inputTensorSize = ObjectDetector::vector_product(inputTensorShape);

    // // Create a vector from the blob data for ONNX Runtime input
    // std::vector<float> inputTensorValues(blobPtr, blobPtr + inputTensorSize);

    // delete[] blobPtr; // Free the allocated memory for the blob

    // // Create an Ort memory info object (can be cached if used repeatedly)
    // static Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    // // Create input tensor object using the preprocessed data
    // Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
    //     memoryInfo,
    //     inputTensorValues.data(),
    //     inputTensorSize,
    //     inputTensorShape.data(),
    //     inputTensorShape.size()
    // );

    // // Run the inference session with the input tensor and retrieve output tensors
    // std::vector<Ort::Value> outputTensors = m_session.Run(
    //     Ort::RunOptions{nullptr},
    //     inputNames.data(),
    //     &inputTensor,
    //     numInputNodes,
    //     outputNames.data(),
    //     numOutputNodes
    // );

    // // Determine the resized image shape based on input tensor shape
    // cv::Size resizedImageShape(static_cast<int>(inputTensorShape[3]), static_cast<int>(inputTensorShape[2]));

    // // Postprocess the output tensors to obtain detections
    // std::vector<Detection> detections = postprocess(image->size(), resizedImageShape, outputTensors, confThreshold, iouThreshold);

    // return detections; // Return the vector of detections
}

// Preprocess function implementation
FramePtr DepthEstimator::preprocess(FramePtr image, float *&blob, std::vector<int64_t> &inputTensorShape)
{
    FramePtr resizedImage = std::make_shared<Frame>();

    return resizedImage;
    // //ScopedTimer timer("preprocessing");

    // Frame resizedImage;
    // // Resize and pad the image using letterBox utility
    // ObjectDetector::letter_box(image, resizedImage, inputImageShape, cv::Scalar(114, 114, 114), isDynamicInputShape, false, true, 32);

    // // Update input tensor shape based on resized image dimensions
    // inputTensorShape[2] = resizedImage.rows;
    // inputTensorShape[3] = resizedImage.cols;

    // // Convert image to float and normalize to [0, 1]
    // resizedImage.convertTo(resizedImage, CV_32FC3, 1 / 255.0f);

    // // Allocate memory for the image blob in CHW format
    // blob = new float[resizedImage.cols * resizedImage.rows * resizedImage.channels()];

    // // Split the image into separate channels and store in the blob
    // std::vector<Frame> chw(resizedImage.channels());
    // for (int i = 0; i < resizedImage.channels(); ++i) {
    //     chw[i] = Frame(resizedImage.rows, resizedImage.cols, CV_32FC1, blob + i * resizedImage.cols * resizedImage.rows);
    // }
    // cv::split(resizedImage, chw); // Split channels into the blob

    // //DEBUG_PRINT("Preprocessing completed")

    // return resizedImage;
}

// Postprocess function to convert raw model output into detections
FramePtr DepthEstimator::postprocess(
    const cv::Size &originalImageSize,
    const cv::Size &resizedImageShape,
    const std::vector<Ort::Value> &outputTensors)
{
    FramePtr depthMap = std::make_shared<Frame>();
    return depthMap;

    // //ScopedTimer timer("postprocessing"); // Measure postprocessing time

    // std::vector<Detection> detections;
    // const float* rawOutput = outputTensors[0].GetTensorData<float>(); // Extract raw output data from the first output tensor
    // const std::vector<int64_t> outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

    // // Determine the number of features and detections
    // const size_t num_features = outputShape[1];
    // const size_t num_detections = outputShape[2];

    // // Early exit if no detections
    // if (num_detections == 0) {
    //     return detections;
    // }

    // // Calculate number of classes based on output shape
    // const int numClasses = static_cast<int>(num_features) - 4;
    // if (numClasses <= 0) {
    //     // Invalid number of classes
    //     return detections;
    // }

    // // Reserve memory for efficient appending
    // std::vector<BoundingBox> boxes;
    // boxes.reserve(num_detections);
    // std::vector<float> confs;
    // confs.reserve(num_detections);
    // std::vector<int> classIds;
    // classIds.reserve(num_detections);
    // std::vector<BoundingBox> nms_boxes;
    // nms_boxes.reserve(num_detections);

    // // Constants for indexing
    // const float* ptr = rawOutput;

    // for (size_t d = 0; d < num_detections; ++d)
    // {
    //     // Extract bounding box coordinates (center x, center y, width, height)
    //     float centerX = ptr[0 * num_detections + d];
    //     float centerY = ptr[1 * num_detections + d];
    //     float width = ptr[2 * num_detections + d];
    //     float height = ptr[3 * num_detections + d];

    //     // Find class with the highest confidence score
    //     int classId = -1;
    //     float maxScore = -FLT_MAX;
    //     for (int c = 0; c < numClasses; ++c)
    //     {
    //         const float score = ptr[d + (4 + c) * num_detections];
    //         if (score > maxScore)
    //         {
    //             maxScore = score;
    //             classId = c;
    //         }
    //     }

    //     // Proceed only if confidence exceeds threshold
    //     if (maxScore > confThreshold)
    //     {
    //         // Convert center coordinates to top-left (x1, y1)
    //         float left = centerX - width / 2.0f;
    //         float top = centerY - height / 2.0f;

    //         // Scale to original image size
    //         BoundingBox scaledBox = scale_coordinates(
    //             resizedImageShape,
    //             BoundingBox(left, top, width, height),
    //             originalImageSize,
    //             true
    //         );

    //         // Round coordinates for integer pixel positions
    //         BoundingBox roundedBox;
    //         roundedBox.x = std::round(scaledBox.x);
    //         roundedBox.y = std::round(scaledBox.y);
    //         roundedBox.width = std::round(scaledBox.width);
    //         roundedBox.height = std::round(scaledBox.height);

    //         // Adjust NMS box coordinates to prevent overlap between classes
    //         BoundingBox nmsBox = roundedBox;
    //         nmsBox.x += classId * 7680; // Arbitrary offset to differentiate classes
    //         nmsBox.y += classId * 7680;

    //         // Add to respective containers
    //         nms_boxes.emplace_back(nmsBox);
    //         boxes.emplace_back(roundedBox);
    //         confs.emplace_back(maxScore);
    //         classIds.emplace_back(classId);
    //     }
    // }

    // // Apply Non-Maximum Suppression (NMS) to eliminate redundant detections
    // std::vector<int> indices;
    // ObjectDetector::nmx_boxes(nms_boxes, confs, confThreshold, iouThreshold, indices);

    // // Collect filtered detections into the result vector
    // detections.reserve(indices.size());
    // for (const int idx : indices)
    // {
    //     detections.emplace_back(Detection{
    //         boxes[idx],       // Bounding box
    //         confs[idx],       // Confidence score
    //         classIds[idx]     // Class ID
    //     });
    // }
    // return detections;
}

} // namespace GpfkuCam