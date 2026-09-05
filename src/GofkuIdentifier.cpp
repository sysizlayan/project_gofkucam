#include "GofkuIdentifier.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numeric>
#include <sstream>

namespace GofkuCam
{

GofkuIdentifier::GofkuIdentifier(const std::string& model_path, LoggerInterfacePtr logger, bool is_gpu)
    : m_model_path(model_path)
    , m_logger(logger)
    , m_is_gpu(is_gpu)
    , m_env(nullptr)
    , m_session(nullptr)
{
    init_session();
    std::stringstream ss;
    ss << "Loaded GofkuIdentifier Model: " << m_model_path << "\n"
       << "Input shape: " << inputImageShape.width << "x" << inputImageShape.height << "\n"
       << "Number of input nodes: " << numInputNodes << "\n"
       << "Number of output nodes: " << numOutputNodes << "\n";
    m_logger->info(ss.str());
}

void GofkuIdentifier::init_session()
{
    m_session_options = Ort::SessionOptions();
    m_session_options.SetIntraOpNumThreads(4);
    m_session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    std::vector<std::string> availableProviders = Ort::GetAvailableProviders();
    std::stringstream ss;
    ss << "Available execution providers for GofkuIdentifier: ";
    for (const auto& provider : availableProviders)
    {
        ss << provider << " ";
    }
    m_logger->info(ss.str());

    auto cudaAvailable = std::find(availableProviders.begin(), availableProviders.end(), "CUDAExecutionProvider");
    auto is_coreml_available = std::find(availableProviders.begin(), availableProviders.end(), "CoreMLExecutionProvider");
    OrtCUDAProviderOptions cudaOption;

    if (m_is_gpu && cudaAvailable != availableProviders.end())
    {
        m_logger->info("GofkuIdentifier using CUDA!");
        m_session_options.AppendExecutionProvider_CUDA(cudaOption);
    }
    else if (m_is_gpu && is_coreml_available != availableProviders.end())
    {
        m_logger->info("GofkuIdentifier using CoreML!");
        std::filesystem::path cache_dir = std::filesystem::current_path() / ".cache" / "coreml" / "gofku_identifier";
        std::error_code ec;
        std::filesystem::create_directories(cache_dir, ec);

        std::unordered_map<std::string, std::string> provider_options;
        provider_options["ModelFormat"] = "MLProgram";
        provider_options["MLComputeUnits"] = "CPUAndGPU";
        provider_options["RequireStaticInputShapes"] = "0";
        provider_options["EnableOnSubgraphs"] = "1";
        provider_options["ModelCacheDirectory"] = cache_dir.string();
        m_session_options.AppendExecutionProvider("CoreML", provider_options);
    }
    else
    {
        m_logger->info("GofkuIdentifier using CPU");
    }

    m_env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "GOFKU_IDENTIFIER");
    m_session = Ort::Session(m_env, m_model_path.c_str(), m_session_options);

    Ort::AllocatorWithDefaultOptions allocator;
    inputNodeNameAllocatedStrings.clear();
    inputNames.clear();
    outputNodeNameAllocatedStrings.clear();
    outputNames.clear();

    numInputNodes = m_session.GetInputCount();
    for (size_t i = 0; i < numInputNodes; ++i)
    {
        auto input_name = m_session.GetInputNameAllocated(i, allocator);
        inputNodeNameAllocatedStrings.push_back(std::move(input_name));
        inputNames.push_back(inputNodeNameAllocatedStrings.back().get());
    }

    numOutputNodes = m_session.GetOutputCount();
    for (size_t i = 0; i < numOutputNodes; ++i)
    {
        auto output_name = m_session.GetOutputNameAllocated(i, allocator);
        outputNodeNameAllocatedStrings.push_back(std::move(output_name));
        outputNames.push_back(outputNodeNameAllocatedStrings.back().get());
    }

    // Retrieve input tensor shape information
    if (numInputNodes > 0)
    {
        Ort::TypeInfo inputTypeInfo = m_session.GetInputTypeInfo(0);
        std::vector<int64_t> inputTensorShapeVec = inputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();
        if (inputTensorShapeVec.size() >= 4 && inputTensorShapeVec[2] > 0 && inputTensorShapeVec[3] > 0)
        {
            inputImageShape = cv::Size(static_cast<int>(inputTensorShapeVec[3]), static_cast<int>(inputTensorShapeVec[2]));
        }
        else
        {
            inputImageShape = cv::Size(224, 224);
        }
    }

    m_classes = {"gofret", "haku"};
}

void GofkuIdentifier::recover()
{
    try
    {
        m_logger->warn("Attempting to recover GofkuIdentifier session after failure...");
        init_session();
        m_logger->info("GofkuIdentifier session recovered successfully.");
    }
    catch (const std::exception& e)
    {
        m_logger->error(std::string("Failed to recover GofkuIdentifier session: ") + e.what());
    }
}

cv::Mat GofkuIdentifier::preprocess(const cv::Mat& image, float*& blob, std::vector<int64_t>& inputTensorShape)
{
    // 1. Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);

    // 2. Resize to (256, 256) matching training eval_transform (image_size + 32)
    cv::Mat resized;
    cv::resize(rgb, resized, cv::Size(256, 256));

    // 3. Center crop to 224x224
    int offset_x = (256 - inputImageShape.width) / 2;
    int offset_y = (256 - inputImageShape.height) / 2;
    cv::Rect crop_roi(offset_x, offset_y, inputImageShape.width, inputImageShape.height);
    cv::Mat center_cropped = resized(crop_roi);

    // 4. Convert to float and scale to [0, 1]
    cv::Mat float_img;
    center_cropped.convertTo(float_img, CV_32FC3, 1.0f / 255.0f);

    // 5. Normalize with ImageNet mean and std
    // mean: [0.485, 0.456, 0.406], std: [0.229, 0.224, 0.225]
    std::vector<cv::Mat> channels(3);
    cv::split(float_img, channels);
    channels[0] = (channels[0] - 0.485f) / 0.229f;
    channels[1] = (channels[1] - 0.456f) / 0.224f;
    channels[2] = (channels[2] - 0.406f) / 0.225f;

    // 6. Set input tensor shape (NCHW: 1 x 3 x H x W)
    inputTensorShape = {1, 3, inputImageShape.height, inputImageShape.width};

    // 7. Allocate blob and copy in CHW format
    size_t plane_size = inputImageShape.height * inputImageShape.width;
    blob = new float[3 * plane_size];
    std::memcpy(blob, channels[0].data, plane_size * sizeof(float));
    std::memcpy(blob + plane_size, channels[1].data, plane_size * sizeof(float));
    std::memcpy(blob + 2 * plane_size, channels[2].data, plane_size * sizeof(float));

    return center_cropped;
}

ClassificationResult GofkuIdentifier::classify(const cv::Mat& image)
{
    ClassificationResult result;
    if (image.empty())
    {
        m_logger->warn("GofkuIdentifier::classify called with empty image");
        return result;
    }

    float* blob = nullptr;
    std::vector<int64_t> inputTensorShape;
    preprocess(image, blob, inputTensorShape);
    if (!blob)
    {
        m_logger->error("GofkuIdentifier preprocessing failed to allocate blob");
        return result;
    }

    size_t inputTensorSize = std::accumulate(inputTensorShape.begin(), inputTensorShape.end(), 1ull, std::multiplies<size_t>());
    std::vector<float> inputTensorValues(blob, blob + inputTensorSize);
    delete[] blob;

    static Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo,
        inputTensorValues.data(),
        inputTensorSize,
        inputTensorShape.data(),
        inputTensorShape.size()
    );

    std::vector<Ort::Value> outputTensors;
    try
    {
        outputTensors = m_session.Run(
            Ort::RunOptions{nullptr},
            inputNames.data(),
            &inputTensor,
            numInputNodes,
            outputNames.data(),
            numOutputNodes
        );
    }
    catch (const Ort::Exception& e)
    {
        m_logger->error(std::string("Ort::Exception in GofkuIdentifier::classify: ") + e.what());
        recover();
        return result;
    }
    catch (const std::exception& e)
    {
        m_logger->error(std::string("std::exception in GofkuIdentifier::classify: ") + e.what());
        recover();
        return result;
    }

    if (outputTensors.empty())
    {
        return result;
    }

    return postprocess(outputTensors[0]);
}

ClassificationResult GofkuIdentifier::postprocess(const Ort::Value& output_tensor)
{
    ClassificationResult result;
    if (!output_tensor.IsTensor())
    {
        m_logger->error("GofkuIdentifier output is not a tensor");
        return result;
    }

    const float* data = output_tensor.GetTensorData<float>();
    float raw_gofret = data[0];
    float raw_haku = data[1];

    // Check if output is already probabilities or raw logits
    float prob_g = raw_gofret;
    float prob_h = raw_haku;
    if (std::abs((raw_gofret + raw_haku) - 1.0f) > 0.05f || raw_gofret < 0.0f || raw_haku < 0.0f)
    {
        float max_val = std::max(raw_gofret, raw_haku);
        float exp_g = std::exp(raw_gofret - max_val);
        float exp_h = std::exp(raw_haku - max_val);
        float sum_exp = exp_g + exp_h;
        prob_g = exp_g / sum_exp;
        prob_h = exp_h / sum_exp;
    }

    result.prob_gofret = prob_g;
    result.prob_haku = prob_h;

    if (prob_h >= prob_g)
    {
        result.class_id = 1;
        result.class_name = "haku";
        result.confidence = prob_h;
    }
    else
    {
        result.class_id = 0;
        result.class_name = "gofret";
        result.confidence = prob_g;
    }

    m_logger->debug("GofkuIdentifier result: " + result.class_name +
                    " (conf: " + std::to_string(result.confidence) +
                    ", Haku: " + std::to_string(result.prob_haku) +
                    ", Gofret: " + std::to_string(result.prob_gofret) + ")");

    return result;
}

} // namespace GofkuCam

