#ifndef GOFKU_IDENTIFIER_HPP
#define GOFKU_IDENTIFIER_HPP

#include "GofkuCamCommon.hpp"
#include "LoggerInterface.hpp"
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace GofkuCam
{

struct ClassificationResult
{
    int class_id{-1};
    std::string class_name;
    float confidence{0.0f};
    float prob_gofret{0.0f};
    float prob_haku{0.0f};
};

class GofkuIdentifier
{
public:
    GofkuIdentifier(const std::string& model_path, LoggerInterfacePtr logger, bool is_gpu = false);
    ~GofkuIdentifier() = default;
    GofkuIdentifier(const GofkuIdentifier&) = delete;
    GofkuIdentifier& operator=(const GofkuIdentifier&) = delete;

    ClassificationResult classify(const cv::Mat& image);
    void recover();

private:
    void init_session();
    cv::Mat preprocess(const cv::Mat& image, float*& blob, std::vector<int64_t>& inputTensorShape);
    ClassificationResult postprocess(const Ort::Value& output_tensor);

    std::string m_model_path;
    LoggerInterfacePtr m_logger;
    bool m_is_gpu{false};
    Ort::Env m_env;
    Ort::SessionOptions m_session_options;
    Ort::Session m_session;

    cv::Size inputImageShape{224, 224};
    std::vector<Ort::AllocatedStringPtr> inputNodeNameAllocatedStrings;
    std::vector<const char*> inputNames;
    std::vector<Ort::AllocatedStringPtr> outputNodeNameAllocatedStrings;
    std::vector<const char*> outputNames;

    size_t numInputNodes{0};
    size_t numOutputNodes{0};

    std::vector<std::string> m_classes;
};

using GofkuIdentifierPtr = std::shared_ptr<GofkuIdentifier>;

} // namespace GofkuCam

#endif // GOFKU_IDENTIFIER_HPP

