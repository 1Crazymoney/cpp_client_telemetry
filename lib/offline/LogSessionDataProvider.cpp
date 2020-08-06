#include "LogSessionDataProvider.hpp"
#include "utils/FileUtils.hpp"
#include "utils/StringUtils.hpp"
#include "utils/Utils.hpp"

#include<cstring>
#include<cstdlib>
#include<errno.h>

extern int errno;

namespace ARIASDK_NS_BEGIN
{
    static const char* sessionFirstLaunchTimeName = "sessionfirstlaunchtime";
    static const char* sessionSdkUidName = "sessionsdkuid";

    std::shared_ptr<LogSessionData> LogSessionDataProvider::GetLogSessionData()
    {
        if (m_storageType == SessionStorageType::FileStore ) {
            return GetLogSessionDataFromFile();
        } else {
            return GetLogSessionDataFromDB();
        }
    }

    std::shared_ptr<LogSessionData> LogSessionDataProvider::GetLogSessionDataFromDB()
    {
        std::string sessionSDKUid;
        uint64_t sessionFirstTimeLaunch = 0;
        if (nullptr != m_offlineStorage)
        {
            sessionSDKUid = m_offlineStorage->GetSetting(sessionSdkUidName);
            sessionFirstTimeLaunch = convertStrToLong(m_offlineStorage->GetSetting(sessionFirstLaunchTimeName));
            if ((sessionFirstTimeLaunch == 0) || sessionSDKUid.empty()) {
                sessionFirstTimeLaunch = PAL::getUtcSystemTimeMs();
                sessionSDKUid = PAL::generateUuidString();
                if (!m_offlineStorage->StoreSetting(sessionFirstLaunchTimeName, std::to_string(sessionFirstTimeLaunch))) {
                    LOG_WARN("Unable to save session analytics to DB for %d", sessionFirstLaunchTimeName);
                }
                if (!m_offlineStorage->StoreSetting(sessionSdkUidName, sessionSDKUid)) {
                    LOG_WARN("Unable to save session analytics to DB for %s", sessionSDKUid.c_str());
                }
            }
        }
        return std::make_shared<LogSessionData>(sessionFirstTimeLaunch, sessionSDKUid);
    }

    std::shared_ptr<LogSessionData> LogSessionDataProvider::GetLogSessionDataFromFile()
    {
        std::string sessionSDKUid;
        uint64_t sessionFirstTimeLaunch = 0;
        std::string sessionPath = m_cacheFilePath.empty() ? "" : (m_cacheFilePath + ".ses").c_str();
        if (!sessionPath.empty()) {
            if (MAT::FileExists(sessionPath.c_str())) {
                auto content = MAT::FileGetContents(sessionPath.c_str());
                if (!parse (content, sessionFirstTimeLaunch, sessionSDKUid)) {
                    sessionFirstTimeLaunch = PAL::getUtcSystemTimeMs();
                    sessionSDKUid = PAL::generateUuidString();
                    writeFileContents(sessionPath, sessionFirstTimeLaunch, sessionSDKUid);
                }
            } else {
                sessionFirstTimeLaunch = PAL::getUtcSystemTimeMs();
                sessionSDKUid = PAL::generateUuidString();
                writeFileContents(sessionPath, sessionFirstTimeLaunch, sessionSDKUid);
            }
        }
        return std::make_shared<LogSessionData>(sessionFirstTimeLaunch, sessionSDKUid);
    }

    bool LogSessionDataProvider::parse(
        const std::string &content,
        uint64_t &sessionFirstTimeLaunch,
        std::string &sessionSDKUid)
    {
        if (content.empty()) {
            return false;
        }
        std::vector<std::string> v;
        StringUtils::SplitString(content, '\n', v);
        if (v.size() != 2) {
           return false;
        }
        remove_eol(v[0]);
        remove_eol(v[1]);
        sessionFirstTimeLaunch = convertStrToLong(v[0]);
        if (sessionFirstTimeLaunch == 0 ) {
            return false;
        }
        sessionSDKUid =  v[1];
        return true;
    }

    uint64_t LogSessionDataProvider::convertStrToLong(const std::string& s)
    {
        uint64_t res = 0ull;
        char *endptr = nullptr;
        res = std::strtol(s.c_str(), &endptr, 10);
        if (errno == ERANGE && (res == LONG_MAX || res == 0 ))
        {
            LOG_WARN ("Converted value falls out of uint64_t range.");
            res = 0;
        } 
        else if ( 0 != errno  && 0 == res )
        {
            LOG_WARN("Conversion cannot be performed.");
        }
        else if (std::strlen(endptr) > 0)
        {
            LOG_WARN ("Conversion cannot be performed. Alphanumeric characters present");
            res = 0;
        } 
        return res;
    }

    void LogSessionDataProvider::writeFileContents(
        const std::string &path,
        uint64_t sessionFirstTimeLaunch,
        const std::string &sessionSDKUid)
    {
        std::string contents;
        contents += toString(sessionFirstTimeLaunch);
        contents += '\n';
        contents += sessionSDKUid;
        contents += '\n';
        if (!MAT::FileWrite(path.c_str(), contents.c_str()))
        {
            LOG_WARN("Unable to save session analytics to %s", path.c_str());
        }
    }

    void LogSessionDataProvider::remove_eol(std::string& result)
    {
        if (!result.empty() && result[result.length() - 1] == '\n')
        {
            result.erase(result.length() - 1);
        }
    }
}
ARIASDK_NS_END
