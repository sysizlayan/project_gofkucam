#include "ObjectDetector.hpp"
#include "LoggerInterface.hpp"
#include <memory>
#include <sstream>
#include "GofkuCamExceptions.hpp"
#include <algorithm>
#include <random>
#include <fstream>

namespace GofkuCam
{

// Implementation of ObjectDetector constructor
ObjectDetector::ObjectDetector(const std::string &model_path, const std::string &label_file_path, LoggerInterfacePtr logger, bool is_gpu)
    : m_model_path{model_path}
    , m_label_file_path{label_file_path}
    , m_logger{logger}
    , m_is_gpu{is_gpu}
    , m_env{nullptr}
    , m_session{nullptr}
    , isDynamicInputShape(false)
    , inputImageShape(0, 0)
    , numInputNodes(0)
    , numOutputNodes(0)
{
    init_session();

    // Load class names and generate corresponding colors
    m_classes      = get_class_names(m_label_file_path);
    m_class_colors = ObjectDetector::generate_colors(m_classes, 0);

    std::stringstream ss1;
    ss1 << "Loaded Model: " << m_model_path << "\n"
        << "Input shape: " << inputImageShape.width << "x" << inputImageShape.height << "\n"
        << "Number of input nodes: " << numInputNodes << "\n"
        << "Number of output nodes: " << numOutputNodes << "\n"
        << "Number of classes loaded: " << m_classes.size() << "\n";
    m_logger->info(ss1.str());
}

void ObjectDetector::init_session()
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
        m_logger->info("Using CoreML!");
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
    m_env       = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "GOFKU_CAM");
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

void ObjectDetector::recover()
{
    try
    {
        m_logger->warn("Attempting to recover ObjectDetector session after failure...");
        init_session();
        m_logger->info("ObjectDetector session recovered successfully.");
    }
    catch (const std::exception& e)
    {
        m_logger->error(std::string("Failed to recover ObjectDetector session: ") + e.what());
    }
}

// Detect function implementation
std::vector<Detection> ObjectDetector::detect(FramePtr image, float confThreshold, float iouThreshold) {
    if (!image || image->empty()) {
        m_logger->warn("ObjectDetector::detect called with empty image");
        return {};
    }

    float* blobPtr = nullptr; // Pointer to hold preprocessed image data
    // Define the shape of the input tensor (batch size, channels, height, width)
    std::vector<int64_t> inputTensorShape = {1, 3, inputImageShape.height, inputImageShape.width};

    // Preprocess the image and obtain a pointer to the blob
    Frame preprocessedImage = preprocess(image, blobPtr, inputTensorShape);
    if (!blobPtr) {
        m_logger->error("ObjectDetector preprocessing failed to allocate blob");
        return {};
    }

    // Compute the total number of elements in the input tensor
    size_t inputTensorSize = ObjectDetector::vector_product(inputTensorShape);

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
        m_logger->error(std::string("Ort::Exception in ObjectDetector::detect: ") + e.what());
        recover();
        return {};
    }
    catch (const std::exception& e) {
        m_logger->error(std::string("std::exception in ObjectDetector::detect: ") + e.what());
        recover();
        return {};
    }

    if (outputTensors.empty()) {
        return {};
    }

    // Determine the resized image shape based on input tensor shape
    cv::Size resizedImageShape(static_cast<int>(inputTensorShape[3]), static_cast<int>(inputTensorShape[2]));

    // Postprocess the output tensors to obtain detections
    std::vector<Detection> detections = postprocess(image->size(), resizedImageShape, outputTensors, confThreshold, iouThreshold);

    return detections; // Return the vector of detections
}

// Preprocess function implementation
Frame ObjectDetector::preprocess(FramePtr image, float *&blob, std::vector<int64_t> &inputTensorShape) {
    //ScopedTimer timer("preprocessing");

    Frame resizedImage;
    // Resize and pad the image using letterBox utility
    ObjectDetector::letter_box(image, resizedImage, inputImageShape, cv::Scalar(114, 114, 114), isDynamicInputShape, false, true, 32);

    // Update input tensor shape based on resized image dimensions
    inputTensorShape[2] = resizedImage.rows;
    inputTensorShape[3] = resizedImage.cols;

    // Convert image to float and normalize to [0, 1]
    resizedImage.convertTo(resizedImage, CV_32FC3, 1 / 255.0f);

    // Allocate memory for the image blob in CHW format
    blob = new float[resizedImage.cols * resizedImage.rows * resizedImage.channels()];

    // Split the image into separate channels and store in the blob
    std::vector<Frame> chw(resizedImage.channels());
    for (int i = 0; i < resizedImage.channels(); ++i) {
        chw[i] = Frame(resizedImage.rows, resizedImage.cols, CV_32FC1, blob + i * resizedImage.cols * resizedImage.rows);
    }
    cv::split(resizedImage, chw); // Split channels into the blob

    //DEBUG_PRINT("Preprocessing completed")

    return resizedImage;
}

// Postprocess function to convert raw model output into detections
std::vector<Detection> ObjectDetector::postprocess(
    const cv::Size &originalImageSize,
    const cv::Size &resizedImageShape,
    const std::vector<Ort::Value> &outputTensors,
    float confThreshold,
    float iouThreshold)
{
    //ScopedTimer timer("postprocessing"); // Measure postprocessing time

    std::vector<Detection> detections;
    const float* rawOutput = outputTensors[0].GetTensorData<float>(); // Extract raw output data from the first output tensor
    const std::vector<int64_t> outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

    // Determine the number of features and detections
    const size_t num_features = outputShape[1];
    const size_t num_detections = outputShape[2];

    // Early exit if no detections
    if (num_detections == 0) {
        return detections;
    }

    // Calculate number of classes based on output shape
    const int numClasses = static_cast<int>(num_features) - 4;
    if (numClasses <= 0) {
        // Invalid number of classes
        return detections;
    }

    // Reserve memory for efficient appending
    std::vector<BoundingBox> boxes;
    boxes.reserve(num_detections);
    std::vector<float> confs;
    confs.reserve(num_detections);
    std::vector<int> classIds;
    classIds.reserve(num_detections);
    std::vector<BoundingBox> nms_boxes;
    nms_boxes.reserve(num_detections);

    // Constants for indexing
    const float* ptr = rawOutput;

    for (size_t d = 0; d < num_detections; ++d)
    {
        // Extract bounding box coordinates (center x, center y, width, height)
        float centerX = ptr[0 * num_detections + d];
        float centerY = ptr[1 * num_detections + d];
        float width = ptr[2 * num_detections + d];
        float height = ptr[3 * num_detections + d];

        // Find class with the highest confidence score
        int classId = -1;
        float maxScore = -FLT_MAX;
        for (int c = 0; c < numClasses; ++c)
        {
            const float score = ptr[d + (4 + c) * num_detections];
            if (score > maxScore)
            {
                maxScore = score;
                classId = c;
            }
        }

        // Proceed only if confidence exceeds threshold
        if (maxScore > confThreshold)
        {
            // Convert center coordinates to top-left (x1, y1)
            float left = centerX - width / 2.0f;
            float top = centerY - height / 2.0f;

            // Scale to original image size
            BoundingBox scaledBox = scale_coordinates(
                resizedImageShape,
                BoundingBox(left, top, width, height),
                originalImageSize,
                true
            );

            // Round coordinates for integer pixel positions
            BoundingBox roundedBox;
            roundedBox.x = std::round(scaledBox.x);
            roundedBox.y = std::round(scaledBox.y);
            roundedBox.width = std::round(scaledBox.width);
            roundedBox.height = std::round(scaledBox.height);

            // Adjust NMS box coordinates to prevent overlap between classes
            BoundingBox nmsBox = roundedBox;
            nmsBox.x += classId * 7680; // Arbitrary offset to differentiate classes
            nmsBox.y += classId * 7680;

            // Add to respective containers
            nms_boxes.emplace_back(nmsBox);
            boxes.emplace_back(roundedBox);
            confs.emplace_back(maxScore);
            classIds.emplace_back(classId);
        }
    }

    // Apply Non-Maximum Suppression (NMS) to eliminate redundant detections
    std::vector<int> indices;
    ObjectDetector::nmx_boxes(nms_boxes, confs, confThreshold, iouThreshold, indices);

    // Collect filtered detections into the result vector
    detections.reserve(indices.size());
    for (const int idx : indices)
    {
        detections.emplace_back(Detection{
            boxes[idx],       // Bounding box
            0.0,              // average_of_detection
            confs[idx],       // Confidence score
            classIds[idx]     // Class ID
        });
    }
    return detections;
}


std::vector<std::string> ObjectDetector::get_class_names(const std::string &path)
{
    std::vector<std::string> class_names;
    std::ifstream class_name_file(path);

    if (class_name_file)
    {
        std::string line;
        while (getline(class_name_file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            class_names.emplace_back(line);
        }
    }
    else
    {
        m_logger->error("Failed to open class names file: " + path);
    }

    return class_names;
}

template <typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type ObjectDetector::clamp(const T &value, const T &low, const T &high)
{
    // Ensure the range [low, high] is valid; swap if necessary
    T validLow = low < high ? low : high;
    T validHigh = low < high ? high : low;

    // Clamp the value to the range [validLow, validHigh]
    if (value < validLow)
        return validLow;
    if (value > validHigh)
        return validHigh;
    return value;
}

size_t ObjectDetector::vector_product(const std::vector<int64_t> &vector)
{
    return std::accumulate(vector.begin(), vector.end(), 1ull, std::multiplies<size_t>());
}


void ObjectDetector::letter_box(FramePtr image, Frame& outImage,
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
            cv::resize(*image, outImage, cv::Size(newUnpadW, newUnpadH), 0, 0, cv::INTER_LINEAR);
        } 
        else
        {
            // Avoid unnecessary copying if dimensions are the same
            outImage = *image;
        }

        // Apply padding to reach the desired shape
        cv::copyMakeBorder(outImage, outImage, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, color);
        return; // Exit early since padding is already applied
    }

    // Resize the image if the new dimensions differ
    if (image->cols != newUnpadW || image->rows != newUnpadH)
    {
        cv::resize(*image, outImage, cv::Size(newUnpadW, newUnpadH), 0, 0, cv::INTER_LINEAR);
    } 
    else
    {
        // Avoid unnecessary copying if dimensions are the same
        outImage = *image;
    }

    // Calculate separate padding for left/right and top/bottom to handle odd padding
    int padLeft = dw / 2;
    int padRight = dw - padLeft;
    int padTop = dh / 2;
    int padBottom = dh - padTop;

    // Apply padding to reach the desired shape
    cv::copyMakeBorder(outImage, outImage, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, color);
}

BoundingBox ObjectDetector::scale_coordinates(const cv::Size &imageShape, BoundingBox coords,
                        const cv::Size &imageOriginalShape, bool p_Clip)
{
    BoundingBox result;
    float gain = std::min(static_cast<float>(imageShape.height) / static_cast<float>(imageOriginalShape.height),
                            static_cast<float>(imageShape.width) / static_cast<float>(imageOriginalShape.width));

    int padX = static_cast<int>(std::round((imageShape.width - imageOriginalShape.width * gain) / 2.0f));
    int padY = static_cast<int>(std::round((imageShape.height - imageOriginalShape.height * gain) / 2.0f));

    result.x = static_cast<int>(std::round((coords.x - padX) / gain));
    result.y = static_cast<int>(std::round((coords.y - padY) / gain));
    result.width = static_cast<int>(std::round(coords.width / gain));
    result.height = static_cast<int>(std::round(coords.height / gain));

    if (p_Clip)
    {
        result.x = clamp(result.x, 0, imageOriginalShape.width);
        result.y = clamp(result.y, 0, imageOriginalShape.height);
        result.width = clamp(result.width, 0, imageOriginalShape.width - result.x);
        result.height = clamp(result.height, 0, imageOriginalShape.height - result.y);
    }
    return result;
}

void ObjectDetector::nmx_boxes(const std::vector<BoundingBox>& boundingBoxes,
            const std::vector<float>& scores,
            float scoreThreshold,
            float nmsThreshold,
            std::vector<int>& indices)
{
    indices.clear();

    const size_t numBoxes = boundingBoxes.size();
    if (numBoxes == 0)
    {
        return;
    }

    // Step 1: Filter out boxes with scores below the threshold
    // and create a list of indices sorted by descending scores
    std::vector<int> sortedIndices;
    sortedIndices.reserve(numBoxes);
    for (size_t i = 0; i < numBoxes; ++i)
    {
        if (scores[i] >= scoreThreshold)
        {
            sortedIndices.push_back(static_cast<int>(i));
        }
    }

    // If no boxes remain after thresholding
    if (sortedIndices.empty())
    {
        return;
    }

    // Sort the indices based on scores in descending order
    std::sort(sortedIndices.begin(), sortedIndices.end(),
            [&scores](int idx1, int idx2) {
                return scores[idx1] > scores[idx2];
            });

    // Step 2: Precompute the areas of all boxes
    std::vector<float> areas(numBoxes, 0.0f);
    for (size_t i = 0; i < numBoxes; ++i)
    {
        areas[i] = boundingBoxes[i].width * boundingBoxes[i].height;
    }

    // Step 3: Suppression mask to mark boxes that are suppressed
    std::vector<bool> suppressed(numBoxes, false);

    // Step 4: Iterate through the sorted list and suppress boxes with high IoU
    for (size_t i = 0; i < sortedIndices.size(); ++i)
    {
        int currentIdx = sortedIndices[i];
        if (suppressed[currentIdx])
        {
            continue;
        }

        // Select the current box as a valid detection
        indices.push_back(currentIdx);

        const BoundingBox& currentBox = boundingBoxes[currentIdx];
        const float x1_max = currentBox.x;
        const float y1_max = currentBox.y;
        const float x2_max = currentBox.x + currentBox.width;
        const float y2_max = currentBox.y + currentBox.height;
        const float area_current = areas[currentIdx];

        // Compare IoU of the current box with the rest
        for (size_t j = i + 1; j < sortedIndices.size(); ++j)
        {
            int compareIdx = sortedIndices[j];
            if (suppressed[compareIdx])
            {
                continue;
            }

            const BoundingBox& compareBox = boundingBoxes[compareIdx];
            const float x1 = std::max(x1_max, static_cast<float>(compareBox.x));
            const float y1 = std::max(y1_max, static_cast<float>(compareBox.y));
            const float x2 = std::min(x2_max, static_cast<float>(compareBox.x + compareBox.width));
            const float y2 = std::min(y2_max, static_cast<float>(compareBox.y + compareBox.height));

            const float interWidth = x2 - x1;
            const float interHeight = y2 - y1;

            if (interWidth <= 0 || interHeight <= 0)
            {
                continue;
            }

            const float intersection = interWidth * interHeight;
            const float unionArea = area_current + areas[compareIdx] - intersection;
            const float iou = (unionArea > 0.0f) ? (intersection / unionArea) : 0.0f;

            if (iou > nmsThreshold)
            {
                suppressed[compareIdx] = true;
            }
        }
    }
}

std::vector<cv::Scalar> ObjectDetector::generate_colors(const std::vector<std::string> &classNames, int seed)
{
    // Static cache to store colors based on class names to avoid regenerating
    static std::unordered_map<size_t, std::vector<cv::Scalar>> colorCache;

    // Compute a hash key based on class names to identify unique class configurations
    size_t hashKey = 0;
    for (const auto& name : classNames) {
        hashKey ^= std::hash<std::string>{}(name) + 0x9e3779b9 + (hashKey << 6) + (hashKey >> 2);
    }

    // Check if colors for this class configuration are already cached
    auto it = colorCache.find(hashKey);
    if (it != colorCache.end()) {
        return it->second;
    }

    // Generate unique random colors for each class
    std::vector<cv::Scalar> colors;
    colors.reserve(classNames.size());

    std::mt19937 rng(seed); // Initialize random number generator with fixed seed
    std::uniform_int_distribution<int> uni(0, 255); // Define distribution for color values

    for (size_t i = 0; i < classNames.size(); ++i) {
        colors.emplace_back(cv::Scalar(uni(rng), uni(rng), uni(rng))); // Generate random BGR color
    }

    // Cache the generated colors for future use
    colorCache.emplace(hashKey, colors);

    return colorCache[hashKey];
}

void ObjectDetector::draw_bounding_box(FramePtr image, const std::vector<Detection> &detections,
                            const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &colors)
{
    // Iterate through each detection to draw bounding boxes and labels
    for (const auto& detection : detections) {
        // Skip detections below the confidence threshold
        if (detection.conf <= CONFIDENCE_THRESHOLD)
            continue;

        // Ensure the object ID is within valid range
        if (detection.classId < 0 || static_cast<size_t>(detection.classId) >= classNames.size())
            continue;

        // Select color based on object ID for consistent coloring
        const cv::Scalar& color = colors[detection.classId % colors.size()];

        // Draw the bounding box rectangle
        cv::rectangle(*image, cv::Point(detection.box.x, detection.box.y),
                        cv::Point(detection.box.x + detection.box.width, detection.box.y + detection.box.height),
                        color, 2, cv::LINE_AA);

        // Prepare label text with class name and confidence percentage
        std::string label = classNames[detection.classId] + ": " + std::to_string(static_cast<int>(detection.conf * 100)) + "%";

        // Define text properties for labels
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = std::min(image->rows, image->cols) * 0.0008;
        const int thickness = std::max(1, static_cast<int>(std::min(image->rows, image->cols) * 0.002));
        int baseline = 0;

        // Calculate text size for background rectangles
        cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, &baseline);

        // Define positions for the label
        int labelY = std::max(detection.box.y, textSize.height + 5);
        cv::Point labelTopLeft(detection.box.x, labelY - textSize.height - 5);
        cv::Point labelBottomRight(detection.box.x + textSize.width + 5, labelY + baseline - 5);

        // Draw background rectangle for label
        cv::rectangle(*image, labelTopLeft, labelBottomRight, color, cv::FILLED);

        // Put label text
        cv::putText(*image, label, cv::Point(detection.box.x + 2, labelY - 2), fontFace, fontScale, cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);
    }
}

/**
    * @brief Draws bounding boxes and semi-transparent masks on the image based on detections.
    * 
    * @param image Image on which to draw.
    * @param detections Vector of detections.
    * @param classNames Vector of class names corresponding to object IDs.
    * @param classColors Vector of colors for each class.
    * @param maskAlpha Alpha value for the mask transparency.
    */
void ObjectDetector::draw_bounding_box_mask(FramePtr image, const std::vector<Detection> &detections,
                                            const std::vector<std::string> &classNames, const std::vector<cv::Scalar> &classColors,
                                            float maskAlpha)
{
    // Validate input image
    if (image->empty())
    {
        m_logger->error("Empty image provided to drawBoundingBoxMask.");
        return;
    }

    const int imgHeight = image->rows;
    const int imgWidth = image->cols;

    // Precompute dynamic font size and thickness based on image dimensions
    const double fontSize = std::min(imgHeight, imgWidth) * 0.0006;
    const int textThickness = std::max(1, static_cast<int>(std::min(imgHeight, imgWidth) * 0.001));

    // Create a mask image for blending (initialized to zero)
    Frame maskImage(image->size(), image->type(), cv::Scalar::all(0));

    // Pre-filter detections to include only those above the confidence threshold and with valid class IDs
    std::vector<const Detection*> filteredDetections;
    for (const auto& detection : detections)
    {
        if (detection.conf > CONFIDENCE_THRESHOLD && 
            detection.classId >= 0 && 
            static_cast<size_t>(detection.classId) < classNames.size())
        {
            filteredDetections.emplace_back(&detection);
        }
    }

    // Draw filled rectangles on the mask image for the semi-transparent overlay
    for (const auto* detection : filteredDetections)
    {
        cv::Rect box(detection->box.x, detection->box.y, detection->box.width, detection->box.height);
        const cv::Scalar &color = classColors[detection->classId];
        cv::rectangle(maskImage, box, color, cv::FILLED);
    }

    // Blend the maskImage with the original image to apply the semi-transparent masks
    cv::addWeighted(maskImage, maskAlpha, *image, 1.0f, 0, *image);

    // Draw bounding boxes and labels on the original image
    for (const auto* detection : filteredDetections)
    {
        cv::Rect box(detection->box.x, detection->box.y, detection->box.width, detection->box.height);
        const cv::Scalar &color = classColors[detection->classId];
        cv::rectangle(*image, box, color, 2, cv::LINE_AA);

        std::string label = classNames[detection->classId] + ": " + std::to_string(static_cast<int>(detection->conf * 100)) + "%";
        int baseLine = 0;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, fontSize, textThickness, &baseLine);

        int labelY = std::max(detection->box.y, labelSize.height + 5);
        cv::Point labelTopLeft(detection->box.x, labelY - labelSize.height - 5);
        cv::Point labelBottomRight(detection->box.x + labelSize.width + 5, labelY + baseLine - 5);

        // Draw background rectangle for label
        cv::rectangle(*image, labelTopLeft, labelBottomRight, color, cv::FILLED);

        // Put label text
        cv::putText(*image, label, cv::Point(detection->box.x + 2, labelY - 2), cv::FONT_HERSHEY_SIMPLEX, fontSize, cv::Scalar(255, 255, 255), textThickness, cv::LINE_AA);
    }
}


} // namespace GpfkuCam