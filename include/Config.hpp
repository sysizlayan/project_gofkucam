#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "GofkuCamExceptions.hpp"
#include "nlohmann/json.hpp"
#include <fstream>

#define CONFIG_FILE_PATH "/home/sysizlayan/git/project_gofkucam/config/config.json"
namespace GofkuCam
{

class Config
{
public:
    // Get the singleton instance
    static Config& config()
    {
        static Config m_config;
        return m_config;
    }

    // Delete copy constructor and assignment operator
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    ~Config() = default;

    template <typename T>
    const T get(const std::string& key) const;
    
private:
    nlohmann::json m_config_json; // JSON object to hold the configuration

    // Private constructor to prevent instantiation
    Config();
};


template <typename T>
inline const T Config::get(const std::string& key) const
{
    try
    {
        return m_config_json["config"][key].get<T>();
    }
    catch(...)
    {
        throw ConfigFailure("Config cannot take '" + key);
    }
}

inline Config::Config()
{
    // Load the configuration from a JSON file
    std::ifstream config_file;
    try
    {
        config_file.open(CONFIG_FILE_PATH);
        config_file >> m_config_json;
        config_file.close();
    }
    catch(...)
    {
        throw ConfigFailure("Could not open configuration file: " + std::string(CONFIG_FILE_PATH));
    }
}

}
#endif // CONFIG_HPP