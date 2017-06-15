#pragma once

#include "ECSClientConfig.hpp"
#include "Misc.hpp"
#include <json.hpp>
#include <string>
#include <map>

// forward-declaration
namespace common
{
    class IOfflineStorage;
}
using nlohmann::json;
namespace Microsoft { namespace Applications { namespace Experimentation { namespace ECS {

// struct to hold an individual config retrieved from ECS
struct ECSConfig
{
    std::string     requestName;
    std::string     etag;
    std::int64_t    expiryUtcTimestamp;
    json            configSettings;
	std::string     clientVersion;

    ECSConfig()
    {
        etag = DEFAULT_CONFIG_ETAG;
        expiryUtcTimestamp = 0;
    }

	std::int64_t GetExpiryTimeInSec() const
    {
        std::int64_t currUtcTimestamp = common::GetCurrentTimeStamp();

        return (expiryUtcTimestamp <= currUtcTimestamp) ? 0 : (expiryUtcTimestamp - currUtcTimestamp);
    }

};

class ECSConfigCache
{
public:
    ECSConfigCache(const std::string& storagePath);
    ~ECSConfigCache();

    bool LoadConfig();
    bool SaveConfig(const ECSConfig& config);
    void StopAndDestroyOfflineStorage();

    ECSConfig* AddConfig(const ECSConfig& config);
    ECSConfig* GetConfigByRequestName(const std::string& requestName);

private:
    common::IOfflineStorage* _CreateOfflineStorage(const std::string& storagePath);

private:
    std::string m_OfflineStoragePath;
    common::IOfflineStorage* m_pOfflineStorage;

    std::map<std::string, ECSConfig> m_configs;

#ifdef _USE_TEST_INJECTION_ECSCLIENT_
    _USE_TEST_INJECTION_ECSCLIENT_
#endif
};

}}}}