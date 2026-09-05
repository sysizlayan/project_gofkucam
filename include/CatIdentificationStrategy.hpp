#ifndef CAT_IDENTIFICATION_STRATEGY_HPP_
#define CAT_IDENTIFICATION_STRATEGY_HPP_

#include "GofkuCamCommon.hpp"
#include "GofkuIdentifier.hpp"
#include "LoggerInterface.hpp"
#include <memory>
#include <opencv2/core/types.hpp>
#include <string>

namespace GofkuCam
{

enum class CatIdentity
{
    UNKNOWN,
    HAKU,
    GOFRET
};

struct CatIdentificationResult
{
    CatIdentity identity{CatIdentity::UNKNOWN};
    double confidence{0.0};
    std::string details;
};

class ICatIdentificationStrategy
{
public:
    virtual ~ICatIdentificationStrategy() = default;
    virtual CatIdentificationResult identify(const cv::Mat& frame, const Detection& detection) = 0;
};

using CatIdentificationStrategyPtr = std::shared_ptr<ICatIdentificationStrategy>;

class PixelAverageIdentificationStrategy : public ICatIdentificationStrategy
{
public:
    PixelAverageIdentificationStrategy(int threshold, LoggerInterfacePtr logger);
    CatIdentificationResult identify(const cv::Mat& frame, const Detection& detection) override;

private:
    int m_threshold;
    LoggerInterfacePtr m_logger;
};

class NeuralCatIdentificationStrategy : public ICatIdentificationStrategy
{
public:
    NeuralCatIdentificationStrategy(std::shared_ptr<GofkuIdentifier> identifier, LoggerInterfacePtr logger);
    CatIdentificationResult identify(const cv::Mat& frame, const Detection& detection) override;

private:
    std::shared_ptr<GofkuIdentifier> m_identifier;
    LoggerInterfacePtr m_logger;
};

class CatIdentificationStrategyFactory
{
public:
    static CatIdentificationStrategyPtr createStrategy(LoggerInterfacePtr logger);
};

} // namespace GofkuCam

#endif // CAT_IDENTIFICATION_STRATEGY_HPP_

