#include "CatIdentificationStrategy.hpp"
#include "Config.hpp"
#include <algorithm>

namespace GofkuCam
{

PixelAverageIdentificationStrategy::PixelAverageIdentificationStrategy(int threshold, LoggerInterfacePtr logger)
    : m_threshold(threshold)
    , m_logger(logger)
{
    m_logger->info("Initialized PixelAverageIdentificationStrategy with threshold: " + std::to_string(m_threshold));
}

CatIdentificationResult PixelAverageIdentificationStrategy::identify(const cv::Mat& frame, const Detection& detection)
{
    (void)frame;
    CatIdentificationResult result;
    result.confidence = detection.average_of_detection;

    if (detection.average_of_detection >= m_threshold)
    {
        result.identity = CatIdentity::HAKU;
        result.details = "pixel average: " + std::to_string(detection.average_of_detection) +
                         " >= threshold: " + std::to_string(m_threshold);
    }
    else
    {
        result.identity = CatIdentity::GOFRET;
        result.details = "pixel average: " + std::to_string(detection.average_of_detection) +
                         " < threshold: " + std::to_string(m_threshold);
    }

    return result;
}

NeuralCatIdentificationStrategy::NeuralCatIdentificationStrategy(std::shared_ptr<GofkuIdentifier> identifier, LoggerInterfacePtr logger)
    : m_identifier(identifier)
    , m_logger(logger)
{
    m_logger->info("Initialized NeuralCatIdentificationStrategy");
}

CatIdentificationResult NeuralCatIdentificationStrategy::identify(const cv::Mat& frame, const Detection& detection)
{
    CatIdentificationResult result;
    if (frame.empty())
    {
        m_logger->warn("Cannot run neural identification: frame is empty");
        return result;
    }

    cv::Rect roi(detection.box.x, detection.box.y, detection.box.width, detection.box.height);
    roi = roi & cv::Rect(0, 0, frame.cols, frame.rows);

    if (roi.width <= 0 || roi.height <= 0)
    {
        m_logger->warn("Cannot run neural identification: invalid ROI");
        return result;
    }

    cv::Mat crop = frame(roi);
    ClassificationResult cr = m_identifier->classify(crop);

    result.confidence = cr.confidence;
    result.details = "model: " + cr.class_name +
                     " (conf: " + std::to_string(cr.confidence) +
                     ", Haku: " + std::to_string(cr.prob_haku) +
                     ", Gofret: " + std::to_string(cr.prob_gofret) + ")";

    if (cr.class_name == "haku")
    {
        result.identity = CatIdentity::HAKU;
    }
    else if (cr.class_name == "gofret")
    {
        result.identity = CatIdentity::GOFRET;
    }
    else
    {
        result.identity = CatIdentity::UNKNOWN;
    }

    return result;
}

CatIdentificationStrategyPtr CatIdentificationStrategyFactory::createStrategy(LoggerInterfacePtr logger)
{
    std::string strategy_name = Config::config().get<std::string>("cat_identification_strategy", "pixel_average");
    logger->info("Creating CatIdentificationStrategy for configured type: " + strategy_name);

    if (strategy_name == "neural_model" || strategy_name == "neural_network" || strategy_name == "model")
    {
        std::string model_path = Config::config().get<std::string>("gofku_identifier_model_path", "extern/gofku_identifier/models/gofku_classifier.onnx");
        bool use_gpu = Config::config().get<bool>("use_gpu", true);

        try
        {
            auto identifier = std::make_shared<GofkuIdentifier>(model_path, logger, use_gpu);
            return std::make_shared<NeuralCatIdentificationStrategy>(identifier, logger);
        }
        catch (const std::exception& e)
        {
            logger->error("Failed to initialize GofkuIdentifier (" + std::string(e.what()) + "). Falling back to pixel average strategy.");
        }
    }

    int threshold = Config::config().get<int>("haku_pixel_threshold", 100);
    return std::make_shared<PixelAverageIdentificationStrategy>(threshold, logger);
}

} // namespace GofkuCam

