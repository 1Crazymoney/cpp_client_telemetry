#include "mat/config.h"
#ifdef HAVE_MAT_EXP
#define LOG_MODULE DBG_API

//#pragma unmanaged

#include "AFDClient.hpp"
#include "AFDClientConfig.hpp"
#include "AFDConfigCache.hpp"
#include "AFDClientUtils.hpp"
#include "exp/JsonHelper.hpp"

#include "json.hpp"

#include <iostream>
#include <iterator>
#include <algorithm>
#include <string>
#include <memory>
#include <math.h>

using namespace std;
using namespace nlohmann;

using namespace MAT;
using namespace PAL;

namespace Microsoft {
    namespace Applications {
        namespace Experimentation {
            namespace AFD
            {

                IAFDClient* IAFDClient::CreateInstance()
                {
                    return (IAFDClient*)(new AFDClient());
                }

                void IAFDClient::DestroyInstance(IAFDClient** ppAFDClient)
                {
                    if (ppAFDClient != NULL && *ppAFDClient != NULL)
                    {
                        delete (AFDClient*)(*ppAFDClient);
                        *ppAFDClient = NULL;
                    }
                }

                /******************************************************************************
                * AFDClient::AFDClient
                *
                * C'tor
                *
                ******************************************************************************/
                AFDClient::AFDClient()
                    :m_configCache(NULL),
                    m_configActive(NULL),
                    m_EXPCommon(this, Retry_Queue_Name),
                    m_minExpireTimeInSecs(DEFAULT_EXPIRE_INTERVAL_IN_SECONDS_MIN)
                {
                    LOG_TRACE("AFDClient c'tor: this=0x%x", this);
                }

                /******************************************************************************
                * AFDClient::~AFDClient
                *
                * D'tor
                *
                ******************************************************************************/
                AFDClient::~AFDClient()
                {
                    LOG_TRACE("AFDClient d'tor: this=0x%x", this);

                    Stop();

                    if (m_configCache != NULL)
                    {
                        delete m_configCache;
                        m_configCache = NULL;
                    }
                }

                /******************************************************************************
                * AFDClient::_ValidateAFDClientConfiguration
                *
                * Validate the AFDClientConfiguration struct specified by client application
                *
                ******************************************************************************/
                void AFDClient::_ValidateAFDClientConfiguration(const AFDClientConfiguration& config)
                {
                    if (config.clientId.empty() ||
                        config.clientVersion.empty() ||
                        config.cacheFilePathName.empty())
                    {
                        LOG_ERROR("_ValidateAFDClientConfiguration: Invalid AFDClientConfiguration specified");
                    }
                }

                /******************************************************************************
                * AFDClient::Initialize
                *
                * Initialize the AFDClient with the specified AFDClientConfiguration
                *
                ******************************************************************************/
                void AFDClient::Initialize(const AFDClientConfiguration& config)
                {
                    // Validate the AFDClientConfiguration struct
                    _ValidateAFDClientConfiguration(config);

                    std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_lock);

                    if (config.serverUrls.empty())
                    {
                        // Use DEFAULT_INT_AFD_SERVER_URL_1 or DEFAULT_INT_AFD_SERVER_URL_2 for test purpose
                        std::string serverUrl1 = DEFAULT_PROD_AFD_SERVER_URL_1;
                        LOG_TRACE("Initialize: Added default AFD ServerUrl=%s", serverUrl1.c_str());
                        m_EXPCommon.m_serverUrls.push_back(serverUrl1);

                        std::string serverUrl2 = DEFAULT_PROD_AFD_SERVER_URL_2;
                        LOG_TRACE("Initialize: Added default AFD ServerUrl=%s", serverUrl2.c_str());
                        m_EXPCommon.m_serverUrls.push_back(serverUrl2);
                    }
                    else
                    {
                        for (size_t i = 0; i < config.serverUrls.size(); ++i)
                        {
                            std::string serverUrl = config.serverUrls[i];
                            LOG_TRACE("Initialize: Added AFD ServerUrl=%s", serverUrl.c_str());

                            m_EXPCommon.m_serverUrls.push_back(serverUrl);
                        }
                    }

                    if (!config.clientId.empty())
                    {
                    	m_EXPCommon.m_configActiveRequestParams[EXPCLIENT_RP_KEY_CLIENTID] = config.clientId;
                    	m_EXPCommon.m_configActiveDeviceId = config.clientId;
                    }

                    if (!config.impressionGuid.empty())
                    {
                        m_EXPCommon.m_configActiveRequestParams["ig"] = config.impressionGuid;
                        //m_configActiveHeaders["X-MSEDGE-IG"] = config.impressionGuid;
                    }
                    if (!config.market.empty())
                    {
                        m_EXPCommon.m_configActiveRequestParams["mkt"] = config.market;
                        //m_configActiveHeaders["X-MSEDGE-MARKET"] = config.market;
                    }
                    if (config.existingUser == 1)
                    {
                        m_EXPCommon.m_configActiveHeaders["X-MSEDGE-EXISTINGUSER"] = "1";
                    }
                    if (config.corpnet == 0)
                    {
                        m_EXPCommon.m_configActiveRequestParams["corpnet"] = "0";
                    }
                    if (!config.setflight.empty())
                    {
                        m_EXPCommon.m_configActiveRequestParams["setflight"] = config.setflight;
                    }

                    m_EXPCommon.m_configActiveRequestName = m_EXPCommon.GetRequestName("");

                    // Start with random server from the list of servers available
                    m_EXPCommon.m_serverUrlIdx = ((unsigned)PAL::getUtcSystemTime()) % (m_EXPCommon.m_serverUrls.size());

                    m_configCache = new AFDConfigCache(config.cacheFilePathName);
                    if (!m_configCache)
                    {
                        LOG_ERROR("Initialize: Failed to create local config cache");
                    }

                    m_AFDClientConfiguration = config;

                    m_EXPCommon.m_status = EXP_INITIALIZED;
                    LOG_TRACE("Initialize: AFDClient successfully initialized");
                }

                /******************************************************************************
                * AFDClient::AddListener
                *
                * AddListener
                *
                ******************************************************************************/
                bool AFDClient::AddListener(IAFDClientCallback *listener)
                {
                    LOG_TRACE("AddListener[%d]: AFDClient=0x%x, listener=0x%x", __LINE__, this, listener);

                    std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_smalllock);

                    if (m_listeners.find(listener) == m_listeners.end())
                    {
                        m_listeners.insert(listener);
                        return true;
                    }

                    return false;
                }

                /******************************************************************************
                * AFDClient::RemoveListener
                *
                * RemoveListener
                *
                ******************************************************************************/
                bool AFDClient::RemoveListener(IAFDClientCallback *listener)
                {
                    LOG_TRACE("RemoveListener[%d]: AFDClient=0x%x, listener=0x%x", __LINE__, this, listener);

                    std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_smalllock);

                    if (m_listeners.find(listener) != m_listeners.end())
                    {
                        m_listeners.erase(listener);
                        return true;
                    }

                    return false;
                }

                /******************************************************************************
                * AFDClient::RegisterLogger
                *
                * Register a logger with AFD
                *
                ******************************************************************************/
                bool AFDClient::RegisterLogger(ILogger* pLogger, const string& agentName)
                {
                    LOG_TRACE("RegisterLogger[%d]: this=0x%x, ILogger=0x%x, agent=%s", __LINE__, this, pLogger, agentName.c_str());
                    m_EXPCommon.RegisterLogger(pLogger, agentName);
                    // Update the logger with the EXP configuration info like Etag if this function is called after ExpCommon is started
                    std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_lock);

                    if (m_EXPCommon.m_status == EXP_STARTED || m_EXPCommon.m_status == EXP_SUSPENDED)
                    {
                        if (m_configActive && m_configActive->etag != DEFAULT_CONFIG_ETAG)
                        {
                            _UpdateLoggerWithEXPConfig(pLogger, agentName);
                        }
                    }
                    return true;
                }

                /******************************************************************************
                * AFDClient::_UpdateLoggerWithEXPConfig
                *
                * update one logger with config
                *
                ******************************************************************************/
                void AFDClient::_UpdateLoggerWithEXPConfig(ILogger* pLogger, const std::string& agentName)
                {
                    if (m_configActive)
                    {
                        if (pLogger && !agentName.empty())
                        {
                            ISemanticContext* pLoggerCtx = pLogger->GetSemanticContext();
                            assert(pLoggerCtx != NULL);

                            pLoggerCtx->SetAppExperimentETag(m_configActive->etag);

                            LOG_TRACE("_UpdateLoggerWithEXPConfig: logger(0x%x) added with ETag=%s", pLogger, m_configActive->etag.c_str());
                            if (!m_AFDClientConfiguration.impressionGuid.empty())
                            {
                                pLoggerCtx->SetAppExperimentImpressionId(m_AFDClientConfiguration.impressionGuid);
                                if (m_AFDClientConfiguration.verbose)
                                {
                                    vector<string> const& flights = m_configActive->flights;
                                    std::map<std::string, std::string> eventConfigIdMap;

                                    std::string allFights;
                                    bool addComma = false;
                                    for (size_t i = 0; i < flights.size(); i++)
                                    {
                                        if (!flights[i].empty())
                                        {
                                            if (addComma)
                                            {
                                                allFights.append(",");
                                            }
                                            else
                                            {
                                                addComma = true;
                                            }
                                            allFights.append(flights[i]);
                                        }
                                    }
                                    if (allFights.size() > 0)
                                    {
                                        LOG_TRACE("_UpdateLoggerWithEXPConfig: logger(0x%x) added with ETag=%s", pLogger, m_configActive->etag.c_str());
                                        pLoggerCtx->SetAppExperimentIds(allFights);
                                    }
                                }
                            }
                        }
                    }
                }

                /******************************************************************************
                * AFDClient::_UpdateLoggersWithEXPConfig
                *
                * Update registered loggers with config
                *
                ******************************************************************************/
                void AFDClient::_UpdateLoggersWithEXPConfig()
                {
                    for (std::map<ILogger *, string>::iterator it = m_EXPCommon.m_registeredLoggers.begin(); it != m_EXPCommon.m_registeredLoggers.end(); it++)
                    {
                        _UpdateLoggerWithEXPConfig(it->first, it->second);
                    }
                }
                /******************************************************************************
                * AFDClient::Start
                *
                * Start the AFD client
                *
                ******************************************************************************/
                bool AFDClient::Start()
                {
                    std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_lock);

                    // check status first, simply return if it hasn't been initialzied or has already started
                    if (m_EXPCommon.m_status != EXP_INITIALIZED &&
                        m_EXPCommon.m_status != EXP_STOPPED)
                    {
                        LOG_ERROR("Start: AFDClient hasn't been initialzied or has already started");
                        return false;
                    }

                    // load cached configuration from local cache
                    if (!m_configCache->LoadConfig())
                    {
                        LOG_WARN("Start: Failed to load configurations from local cache");
                    }
                    else
                    {
                        // point active config to the default configuration
                        m_configActive = m_configCache->GetConfigByRequestName(m_EXPCommon.m_configActiveRequestName);
                        if (m_configActive && m_configActive->etag != DEFAULT_CONFIG_ETAG)
                        {
                            m_EXPCommon.m_configActiveHeaders["If-None-Match"] = m_configActive->etag;

                            auto itVersion = m_configActive->configSettings.find("FlightingVersion");
                            if (m_configActive->configSettings.end() != itVersion)
                            {
                                if (itVersion.value().is_number())
                                {
                                    m_configActive->flightingVersion = itVersion.value().get<int>();
                                }
                            }


                            auto itImpressionId = m_configActive->configSettings.find("ImpressionId");
                            if (m_configActive->configSettings.end() != itImpressionId)
                            {
                                if (itImpressionId.value().is_string())
                                {
                                    m_AFDClientConfiguration.impressionGuid = itImpressionId.value().get<std::string>();
                                    m_EXPCommon.m_configActiveRequestParams["ig"] = m_AFDClientConfiguration.impressionGuid; //m_configActiveHeaders["X-MSEDGE-IG"] = config.impressionGuid;
                                }
                            }

                            LoadActiveConfigs();


                            // TODO: notify listener if the Etag of the active configuration is different from default
                            std::lock_guard<std::mutex> lock(m_EXPCommon.m_smalllock);

                            // log event thru all register loggers to indicate that EXP config is updated from local cache
                            _LogEXPConfigUpdateEvent(EXP_CUR_SUCCEEDED, EXP_CUS_LOCAL);

                            // update all registered loggers with the EXP configurations infos(such as Etag, configIDs)
                            _UpdateLoggersWithEXPConfig();

                            // notify listners if the active config is either updated on EXP server or changed to a different one
                            FireClientEvent(ET_CONFIG_UPDATE_SUCCEEDED, false);
                        }
                    }

                    std::vector<int> backoffTimes;
                    for (int index = 0; index < MAX_RETRY_TIMES; index++)
                    {
                        // back off formula is: 4^retry, ie.
                        // 1st retry is of 4^1 = 4 seconds delay,
                        // 2nd retry is of 4^2 = 16 seconds delay.
                        int backoffTimeInSec = (int)pow((double)DEFAULT_RETRY_TIME_FACTOR, (double)(index + 1));
                        backoffTimes.push_back(backoffTimeInSec);
                    }

                    m_EXPCommon.m_forceRefech = true;
                    m_EXPCommon.Start(backoffTimes);

                    // log EVENT_TYPE_ExpCommon_STATE_CHANGE event to all registered logger to indicate the ExpCommon state 
                    _LogEXPCleintStateChangeEvent(EXP_STARTED);

                    return true;
                }

                void AFDClient::LoadActiveConfigs()
                {
                    if (0 == m_configActive->configs.size())
                    {
                        auto itConfigs = m_configActive->configSettings.find("Configs");
                        if (m_configActive->configSettings.end() != itConfigs)
                        {
                            for (auto it = itConfigs.value().begin(); it != itConfigs.value().end(); ++it)
                            {
                                json val = it.value();
                                auto id = val.find("Id");
                                std::string idValue;
                                if (val.end() != id)
                                {
                                    idValue = id.value().get<std::string>();
                                }

                                auto parameters = val.find("Parameters");
                                std::string parametersValue;
                                if (val.end() != parameters)
                                {
                                    bool first = true;
                                    for (auto itParameter = val["Parameters"].begin(); itParameter != val["Parameters"].end(); ++itParameter)
                                    {
                                        if (first)
                                        {
                                            first = false;
                                        }
                                        else
                                        {
                                            parametersValue += ",";
                                        }
                                        std::string test(itParameter.key());
                                        test.append("=");
                                        json valParameter = itParameter.value();
                                        if (valParameter.is_string())
                                        {
                                            test.append(valParameter.get<std::string>());
                                        }

                                        if (valParameter.is_number_integer())
                                        {
                                            test.append(std::to_string(valParameter.get<int>()));
                                        }

                                        if (valParameter.is_boolean())
                                        {
                                            test.append(std::to_string(valParameter.get<int>()));
                                        }

                                        if (valParameter.is_number_float())
                                        {
                                            test.append(std::to_string(valParameter.get<int>()));
                                        }                                 

                                        parametersValue += test;
                                    }
                                }

                                if (!idValue.empty())
                                {
                                    m_configActive->configs[idValue] = parametersValue;
                                }
                            }
                        }
                    }
                }

                /******************************************************************************
                * AFDClient::Resume
                *
                * Resume the AFD client to retrieve configuration from AFD server
                *
                ******************************************************************************/
                bool AFDClient::Resume(bool fetchConfig)
                {
                    std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_lock);

                    // check status first, simply return if it hasn't been initialzied or has already started
                    if (m_EXPCommon.m_status != EXP_SUSPENDED)
                    {
                        LOG_ERROR("Resume: ExpCommon wasn't suspended");
                        return false;
                    }
                    m_EXPCommon.m_forceRefech = fetchConfig;
                    m_EXPCommon.Resume();
                    // log EVENT_TYPE_ExpCommon_STATE_CHANGE event to all registered loggers 
                    _LogEXPCleintStateChangeEvent(EXP_STARTED);
                    return true;
                }

                /******************************************************************************
                * AFDClient::Suspend
                *
                * Suspend the AFD client from retrieving configuration from AFD server
                *
                ******************************************************************************/
                bool AFDClient::Suspend()
                {
                    std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_lock);
                    // check status first, simply return if it hasn't been initialzied or has already started
                    if (m_EXPCommon.m_status != EXP_STARTED)
                    {
                        LOG_ERROR("Suspend: ExpCommon isn't started");
                        return false;
                    }
                    m_EXPCommon.Suspend();
                    // log EVENT_TYPE_ExpCommon_STATE_CHANGE event to all registered loggers
                    _LogEXPCleintStateChangeEvent(EXP_SUSPENDED);
                    return true;
                }

                /******************************************************************************
                * AFDClient::Stop
                *
                * Stop the AFD client
                *
                ******************************************************************************/
                bool AFDClient::Stop()
                {
                    std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_lock);

                    // check status first, simply return if not started
                    if (m_EXPCommon.m_status != EXP_STARTED && m_EXPCommon.m_status != EXP_SUSPENDED)
                    {
                        LOG_ERROR("Stop: ExpCommon isn't started");
                        return false;
                    }
                    // stop and destroy the offline storage used for local cache of configs
                    m_configCache->StopAndDestroyOfflineStorage();

                    m_EXPCommon.Stop();
                    // log EVENT_TYPE_ExpCommon_STATE_CHANGE event to all registered loggers
                    _LogEXPCleintStateChangeEvent(EXP_STOPPED);
                    return true;
                }

                /******************************************************************************
                * AFDClient::SetRequestParameters
                *
                * Set a list of request parameters for retrieve configurations from AFD server
                *
                ******************************************************************************/
                bool AFDClient::SetRequestParameters(const std::map<std::string, std::string>& requestParams)
                {
                    LOG_TRACE("SetRequestParameters[%d]: AFDClient=0x%x, request parameters count=%u", __LINE__, this, requestParams.size());

                    m_EXPCommon.SetRequestParameters(requestParams, false);

                    // log EVENT_TYPE_ExpCommon_STATE_CHANGE event to all registered logger to indicate the ExpCommon state 
                    _LogEXPCleintStateChangeEvent(EXP_REQUESTPARAMETER_CHANGED);
                    return true;
                }

                /******************************************************************************
                * AFDClient::SetRequestHeaders
                *
                * Set a list of request header for retrieve configurations from AFD server
                *
                ******************************************************************************/
                bool AFDClient::SetRequestHeaders(const std::map<std::string, std::string>& headerParams)
                {
                    std::map<std::string, std::string>::const_iterator iter;

                    for (iter = headerParams.begin(); iter != headerParams.end(); iter++)
                    {
                        m_EXPCommon.m_configActiveHeaders[iter->first] = iter->second;
                    }
                    return true;
                }

                /******************************************************************************
                * AFDClient::FireClientEvent
                *
                * Call client call back after configuration has been received
                *
                ******************************************************************************/

                void AFDClient::FireClientEvent(CommonClientEventType evtType, bool fConfigUpdateFromServer)
                {
                    LOG_TRACE("FireClientEvent[%d]:  AFDClient=0x%x, listener count=%u", __LINE__, this, m_listeners.size());

                    LOG_TRACE("FireClientEvent[%d]:  EventType=%d, ConfigUpdateFromAFD=%d", evtType, fConfigUpdateFromServer);

                    // notify listners if the active config is either updated on ECS server or changed to a different one
                    IAFDClientCallback::AFDClientEventContext evtContext = {};

                    evtContext.clientId = m_AFDClientConfiguration.clientId;
                    evtContext.clientVersion = m_AFDClientConfiguration.clientVersion;
                    evtContext.impressionID = m_AFDClientConfiguration.impressionGuid;
                    evtContext.flightingVersion = m_configActive->flightingVersion;
                    evtContext.requestParameters = m_EXPCommon.m_configActiveRequestParams;
                    evtContext.configUpdateFromAFD = fConfigUpdateFromServer;
                    if (m_configActive)
                    {
                        evtContext.configExpiryTimeInSec = (unsigned int)m_configActive->GetExpiryTimeInSec();
                        evtContext.features = m_configActive->features;
                        evtContext.flights = m_configActive->flights;
                        evtContext.configs = m_configActive->configs;
                    }

                    //pre-condition: m_smalllock is held in caller while this function is called
                    //std::lock_guard<std::mutex> lockguard(m_smalllock);
                    IAFDClientCallback::AFDClientEventType eventTypeLocal = IAFDClientCallback::AFDClientEventType::ET_CONFIG_UPDATE_SUCCEEDED;
                    if (evtType == CommonClientEventType::ET_CONFIG_UPDATE_FAILED)
                    {
                        eventTypeLocal = IAFDClientCallback::AFDClientEventType::ET_CONFIG_UPDATE_FAILED;
                    }

                    for (std::set<IAFDClientCallback *>::iterator it = m_listeners.begin(); it != m_listeners.end(); it++)
                    {
                        IAFDClientCallback* afdclientCallback = *it;

                        LOG_TRACE("_FireAFDClientEvent[%d]:: AFDClient=0x%x, listener=0x%x", __LINE__, this, afdclientCallback);
                        afdclientCallback->OnAFDClientEvent(eventTypeLocal, evtContext);
                    }
                }

                /******************************************************************************
                * AFDClient::_HandleHttpCallback
                *
                * Process the HTTPCallback within timer callback
                *
                ******************************************************************************/
                void AFDClient::HandleHttpCallback(
                    Message& msg,
                    bool& isActiveConfigUpdatedOnAFD,
                    bool& isActiveConfigUpdatedOnAFDSaveNeeded)
                {
                    LOG_TRACE("_HandleHttpCallback: HTTPstack error=%u, HTTP status code=%u",
                        msg.httpstackError, msg.statusCode);

                    isActiveConfigUpdatedOnAFD = true;
                    isActiveConfigUpdatedOnAFDSaveNeeded = false;

                    switch (msg.statusCode)
                    {
                    case 200:
                    {
                        std::string ref = msg.headers.get("X-MSEdge-Ref");

                        // Config retrieved successfully from AFD, update the local cache
                        LOG_TRACE("_HandleHttpCallback: config retrieved from AFD, ETag=%s", msg.headers.get("ETag").c_str());
                        m_configActive->expiryUtcTimestamp = PAL::getUtcSystemTime() + _GetExpiryTimeInSecFromHeader(msg);

                        std::string temp = msg.headers.get("X-MSEdge-Features");
                        m_configActive->features = splitString(temp, ',');
                        temp = msg.headers.get("X-MSEdge-Flight");
                        m_configActive->flights = splitString(temp, ',');

                        std::string etag = msg.headers.get("ETag");
                        if (etag.empty())
                        {
                            size_t index = ref.find("Ref A: ");
                            if (index != std::string::npos)
                            {
                                etag = ref.substr(sizeof("Ref A: ") - 1);
                                index = etag.find(" ");
                                if (index != std::string::npos)
                                {
                                    etag = etag.substr(0, index);
                                }
                            }
                        }
                        m_configActive->etag = etag;

                        //Update the active config version after getting new one from AFD server
                        m_configActive->clientVersion = m_AFDClientConfiguration.clientVersion;

                        //string test("{\"Features\":[\"heads\",\"rt-afdcpv01c\",\"rt-afdcpv02c\",\"rt-afdcpv03t\",\"control123\",\"twithig\"],\"Flights\":{\"rt-validation01\":\"rt-afdcpv01c\",\"rt-validation02\":\"rt-afdcpv02c\",\"rt-validation03\":\"rt-afdcpv03t\"},\"Configs\":[{\"Id\":\"Flight_tryexp2rows8columns\",\"Parameters\":{}},{\"Id\":\"Flight_tryexptvdemo3rows\",\"Parameters\":{}},{\"Id\":\"Flight_tryexptvdemodefault\",\"Parameters\":{}},{\"Id\":\"TryExP\",\"Parameters\":{}}],\"FlightingVersion\":505,\"ImpressionId\":\"F45813A6A5D049AE9C1EDA79E1FBE8D0\"}");
                       
                        try
                        {
                            if (msg.body.size() > 0)
                            {
                                m_configActive->configSettings = json::parse(msg.body.c_str());
                            }
                        }
                        catch (...)
                        {
                            LOG_TRACE("Json pasring failed");
                        }

                        if (0 == m_configActive->features.size())
                        {
                            auto itFeatures = m_configActive->configSettings.find("Features");

                            if (m_configActive->configSettings.end() != itFeatures)
                            {
                                for (auto it = itFeatures.value().begin(); it != itFeatures.value().end(); ++it)
                                {
                                    if (it.value().is_string())
                                    {
                                        m_configActive->features.push_back(it.value().get<std::string>());
                                    }
                                }
                            }
                        }

                        if (0 == m_configActive->flights.size())
                        {
                            auto itFlights = m_configActive->configSettings.find("Flights");
                            if (m_configActive->configSettings.end() != itFlights)
                            {
                                for (auto it = m_configActive->configSettings["Flights"].begin(); it != m_configActive->configSettings["Flights"].end(); ++it)
                                {
                                    std::string test(it.key());
                                    test.append("=");
                                    json val = it.value();
                                    if (val.is_string())
                                    {
                                        test.append(val.get<std::string>());
                                    }
                                    m_configActive->flights.push_back(test);
                                }
                            }
                        }

                        LoadActiveConfigs();
                        
                        auto itVersion = m_configActive->configSettings.find("FlightingVersion");
                        if (m_configActive->configSettings.end() != itVersion)
                        {
                            if (itVersion.value().is_number())
                            {
                                m_configActive->flightingVersion = itVersion.value().get<int>();
                            }
                        }

                        m_EXPCommon.m_configActiveHeaders["If-None-Match"] = m_configActive->etag;

                        auto itImpressionId = m_configActive->configSettings.find("ImpressionId");
                        if (m_configActive->configSettings.end() != itImpressionId)
                        {
                            if (itImpressionId.value().is_string())
                            {
                                m_AFDClientConfiguration.impressionGuid = itImpressionId.value().get<std::string>();
                                m_EXPCommon.m_configActiveRequestParams["ig"] = m_AFDClientConfiguration.impressionGuid;
                            }
                        }

                        /*/ add flights to Json
                        if (m_configActive->flights.size() > 0)//features are always in the Json, we need to add flights and features if we got flights
                        {
                            std::string str;
                            str += "{";

                            if (m_configActive->features.size() > 0)
                            {
                                str += "\"Features\":[";
                                std::vector<std::string>::iterator iter;
                                bool notfirst = false;
                                for (iter = m_configActive->features.begin(); iter < m_configActive->features.end(); iter++)
                                {
                                    if (notfirst)
                                    {
                                        str += ",";
                                    }
                                    else
                                    {
                                        notfirst = true;
                                    }
                                    str += "\"";
                                    str += *iter;
                                    str += "\"";
                                }
                                str += "]";
                            }

                            if (m_configActive->flights.size() > 0)
                            {
                                if (!str.empty()) str += ",";
                                str += "\"Flights\":[";
                                std::vector<std::string>::iterator iter;
                                bool notfirst = false;
                                for (iter = m_configActive->flights.begin(); iter < m_configActive->flights.end(); iter++)
                                {
                                    if (notfirst)
                                    {
                                        str += ",";
                                    }
                                    else
                                    {
                                        notfirst = true;
                                    }
                                    str += "\"";
                                    str += *iter;
                                    str += "\"";
                                }
                                str += "]";
                            }
                            str += "}";

                            (void)json::parse(str, m_configActive->configSettings);
                        }
                        */

                        // notify listners if the active config is updated on AFD server
                        isActiveConfigUpdatedOnAFD = true;
                        isActiveConfigUpdatedOnAFDSaveNeeded = true;
                        break;
                    }

                    case 304:
                        // AFD server returns HTTP status code 304 and empty configuration in case it generated 
                        // the same ETag as the one that was passed in the 'If-None-Match' header value of request
                        assert(isActiveConfigUpdatedOnAFD == false);

                        // Config settings not modified but the expiry time does, which requires a save to local cache
                        m_configActive->expiryUtcTimestamp = PAL::getUtcSystemTime() + _GetExpiryTimeInSecFromHeader(msg);
                        isActiveConfigUpdatedOnAFDSaveNeeded = true;
                        break;

                    case 401:
                        // TODO: handle invalid auto token
                        assert(false);
                        break;

                    default:
                        // for all other status code, treat them as error
                        isActiveConfigUpdatedOnAFD = true;
                        isActiveConfigUpdatedOnAFDSaveNeeded = false;

                        // Next time try a different URL if avail
                        m_EXPCommon.m_serverUrlIdx++;
                        m_EXPCommon.m_serverUrlIdx %= m_EXPCommon.m_serverUrls.size();
                        break;
                    }
                }

                std::int64_t AFDClient::_GetExpiryTimeInSecFromHeader(Message& msg)
                {
                    UNREFERENCED_PARAMETER(msg);

                    unsigned int timeoutinSec = m_AFDClientConfiguration.defaultExpiryTimeInMin * 60;

                    if (timeoutinSec == 0 || timeoutinSec > DEFAULT_EXPIRE_INTERVAL_IN_SECONDS_MAX)
                    {
                        timeoutinSec = DEFAULT_EXPIRE_INTERVAL_IN_SECONDS_MAX;
                    }

                    //make sure the relative expire time is no less than m_minExpireTimeInSecs
                    if (timeoutinSec < m_minExpireTimeInSecs)
                    {
                        LOG_WARN("_GetExpiryTimeInSecFromHeader: Expires time(%ld) from response header is less than min limit(%ld sec), use min.",
                            timeoutinSec, m_minExpireTimeInSecs);
                        timeoutinSec = m_minExpireTimeInSecs;
                    }

                    return timeoutinSec;
                }


                /******************************************************************************
                * AFDClient::HandleConfigReloadAndRefetch
                *
                * Reload from local cache or issue condig refetch request within timer callback
                *
                ******************************************************************************/
                void AFDClient::HandleConfigReload(
                    Message& msg,
                    bool& isActiveConfigSwitched,
                    bool& isActiveConfigSwitchedSaveNeeded)
                {
                    LOG_TRACE("_HandleConfigReload: HandleConfigReload for RequestName=%s", msg.requestName.c_str());

                    if (m_EXPCommon.m_status != EXP_STARTED)
                    {
                        LOG_TRACE("HandleConfigReload: ignored[Status=%d]", m_EXPCommon.m_status);
                        return;
                    }

                    if (m_configCache)
                    {
                        assert(msg.type == MT_RELOAD_CONFIG);

                        isActiveConfigSwitched = false;
                        isActiveConfigSwitchedSaveNeeded = false;

                        // Reload config from local cache and set it as active config if necessary
                        AFDConfig* pConfig = m_configCache->GetConfigByRequestName(msg.requestName);
                        if (pConfig == NULL)
                        {                            
                            unsigned int timeoutinSec = m_AFDClientConfiguration.defaultExpiryTimeInMin * 60;
                            if (timeoutinSec == 0 )
                            {
                                timeoutinSec = m_minExpireTimeInSecs;
                            }

                            AFDConfig config;
                            config.expiryUtcTimestamp = PAL::getUtcSystemTime() + timeoutinSec;
                            config.requestName = msg.requestName;
                            pConfig = m_configCache->AddConfig(config);
                        }

                        // Update the active config if necessary
                        if (m_configActive != pConfig)
                        {
                            m_configActive = pConfig;
                            m_EXPCommon.m_configActiveRequestName = pConfig->requestName;

                            isActiveConfigSwitched = true;
                            isActiveConfigSwitchedSaveNeeded = true;
                        }
                    }
                }

                /******************************************************************************
                * AFDClient::HandleConfigSave
                *
                * Reload from local cache or issue condig refetch request within timer callback
                *
                ******************************************************************************/
                void AFDClient::HandleConfigSave(bool isActiveConfigSwitchedSaveNeeded, bool isActiveConfigUpdatedOnEXPSaveNeeded)
                {
                    // The save config is pretty time-consuming, we need save it outside the m_smalllock
                    if (m_configCache && (isActiveConfigSwitchedSaveNeeded || isActiveConfigUpdatedOnEXPSaveNeeded) && m_configActive->etag != DEFAULT_CONFIG_ETAG)
                    {
                        m_configCache->SaveConfig(*m_configActive);
                    }
                }

                /******************************************************************************
                * AFDClient::HandleUpdateClient
                *
                * Update Client with config
                *
                ******************************************************************************/
                void AFDClient::HandleUpdateClient(bool isActiveConfigSwitched, bool isActiveConfigUpdatedOnEXP, bool isActiveConfigUpdatedOnEXPSaveNeeded)
                {
                    if (m_EXPCommon.m_status != EXP_STARTED)
                    {
                        return;
                    }
                    if (isActiveConfigSwitched || isActiveConfigUpdatedOnEXP)
                    {
                        if (isActiveConfigSwitched)
                        {
                            if (m_configActive->etag != DEFAULT_CONFIG_ETAG)
                            {
                                // update all registered loggers with the EXP configurations infos(such as Etag, configIDs)
                                _UpdateLoggersWithEXPConfig();

                                // log event to indicate that EXP config is updated from local cache
                                _LogEXPConfigUpdateEvent(EXP_CUR_SUCCEEDED, EXP_CUS_LOCAL);

                                // fire event to notify all listeners of this active config change
                                FireClientEvent(CommonClientEventType::ET_CONFIG_UPDATE_SUCCEEDED, false);
                            }
                        }
                        else
                        {
                            assert(isActiveConfigUpdatedOnEXP);

                            if (isActiveConfigUpdatedOnEXPSaveNeeded)
                            {
                                assert(m_configActive->etag != DEFAULT_CONFIG_ETAG);

                                // update all registered loggers with the EXP configurations infos(such as Etag, configIDs)
                                _UpdateLoggersWithEXPConfig();

                                // log event to indicate that EXP config is updated from local cache
                                _LogEXPConfigUpdateEvent(EXP_CUR_SUCCEEDED, EXP_CUS_SERVER);

                                // fire event to notify all listeners of this active config change
                                FireClientEvent(CommonClientEventType::ET_CONFIG_UPDATE_SUCCEEDED, true);
                            }
                            else
                            {
                                // Fetch from EXP server failed
                                // log event to indicate that EXP config is updated from local cache
                                _LogEXPConfigUpdateEvent(EXP_CUR_FAILED, EXP_CUS_SERVER);

                                // fire event to notify all listeners of this active config change
                                FireClientEvent(CommonClientEventType::ET_CONFIG_UPDATE_FAILED, true);
                            }
                        }
                    }
                }

                /******************************************************************************
                * AFDClient::FetchFromServerIfRequired
                *
                * Fetch the config from server if required conditions are met
                *
                ******************************************************************************/
                bool AFDClient::FetchFromServerIfRequired()
                {
                    // Check if the active config has expired or not, refetch config from EXP if expired.
                    // Otherwise set a timer to expire it and notify all listeners if it has now changed.
                    unsigned int expiryTimeInSec = static_cast<unsigned int>(m_configActive->GetExpiryTimeInSec());
                    if (expiryTimeInSec == 0 ||
                        m_EXPCommon.m_forceRefech ||
                        m_AFDClientConfiguration.clientVersion != m_configActive->clientVersion)   // UI version has changed, we need to fetch new one.
                    {
                        // if active config has expired or client chnaged version, refetch it from EXP server immediately

                        m_EXPCommon.m_forceRefech = false;
                        std::string url = m_EXPCommon.m_serverUrls.at(m_EXPCommon.m_serverUrlIdx);

                        m_EXPCommon.SendRequestAsync(url);
                        LOG_TRACE("_HandleConfigReloadAndRefetch: Config refetch request successfully sent to EXP.");

                        return true;
                    }
                    return false;
                }


                /******************************************************************************
                * AFDClient::_LogEXPConfigUpdateEvent
                *
                * Log the config update to registered loggers
                *
                ******************************************************************************/
                void AFDClient::_LogEXPConfigUpdateEvent(EXPConfigUpdateResult result, EXPConfigUpdateSource source)
                {
                    if (m_AFDClientConfiguration.enableAFDClientTelemetry)
                    {
                        //pre-condition: m_smalllock is held in caller while this function is called
                        //std::lock_guard<std::mutex> lockguard(m_smalllock);

                        EventProperties evtProperties(EVENT_TYPE_AFDCLIENT_CONFIG_UPDATE);

                        evtProperties.SetProperty(EVENT_FIELD_AFDCLIENT_CLIENTNAME, m_AFDClientConfiguration.clientId);
                        evtProperties.SetProperty(EVENT_FIELD_AFDCLIENT_CLIENTVERSION, m_AFDClientConfiguration.clientVersion);

                        evtProperties.SetProperty(EVENT_FIELD_AFDCLIENT_CONFIG_RESULT, ExpCommon::EXPConfigUpdateResult2STR[result]);
                        evtProperties.SetProperty(EVENT_FIELD_AFDCLIENT_CONFIG_SOURCE, ExpCommon::EXPConfigUpdateSource2STR[source]);

                        m_EXPCommon._LogEXPConfigEvent(evtProperties);
                    }
                }

                /******************************************************************************
                * AFDClient::_LogEXPCleintStateChangeEvent
                *
                * Log client state change event to registered loggers
                *
                ******************************************************************************/
                void AFDClient::_LogEXPCleintStateChangeEvent(EXPClientStatus status)
                {
                    if (m_AFDClientConfiguration.enableAFDClientTelemetry)
                    {
                        std::lock_guard<std::mutex> lockguard(m_EXPCommon.m_smalllock);

                        EventProperties evtProperties(EVENT_TYPE_AFDCLIENT_STATE_CHANGE);

                        evtProperties.SetProperty(EVENT_FIELD_AFDCLIENT_CLIENTNAME, m_AFDClientConfiguration.clientId);
                        evtProperties.SetProperty(EVENT_FIELD_AFDCLIENT_CLIENTVERSION, m_AFDClientConfiguration.clientVersion);

                        evtProperties.SetProperty(EVENT_FIELD_AFDCLIENT_STATE, ExpCommon::EXPClientStatus2STR[status]);

                        m_EXPCommon._LogEXPConfigEvent(evtProperties);
                    }
                }

                unsigned int AFDClient::GetExpiryTimeInSec()
                {
                    int64_t value = m_configActive->GetExpiryTimeInSec();
                    if (0 == value)
                    {
                        Message test(MT_UNKNOWN);
                        value = _GetExpiryTimeInSecFromHeader(test);
                    }

                    return static_cast<unsigned int>(value);
                }

                void AFDClient::SetRetryTimeFactor(int time)
                {
                    m_EXPCommon.m_retryTimeFactor = time;
                }

                json AFDClient::_GetActiveConfigVariant()
                {
                    return (m_configActive != NULL) ? m_configActive->configSettings : json();
                }

                json AFDClient::GetActiveConfigVariant()
                {
                    return (m_configActive != NULL) ? m_configActive->configSettings : json();
                }

                // IAFDClient APIs
                string AFDClient::GetETag()
                {
                    return (m_configActive != NULL) ? m_configActive->etag : "";
                }

                std::vector<std::string> AFDClient::GetFlights()
                {
                    return m_configActive->flights;
                }

                std::vector<std::string> AFDClient::GetFeatures()
                {
                    return m_configActive->features;
                }

                std::map<std::string, std::string> AFDClient::GetConfigs()
                {
                    return m_configActive->configs;
                }

                std::vector<std::string> AFDClient::GetKeys(
                    const std::string& agentName,
                    const std::string& keysPath)
                {
                    string fullPath = JsonHelper::Combine(agentName, keysPath, '/');

                    return JsonHelper::GetKeys(_GetActiveConfigVariant(), fullPath);
                }

                string AFDClient::GetSetting(
                    const std::string& agentName,
                    const std::string& settingPath,
                    const std::string& defaultValue)
                {
                    string fullPath = JsonHelper::Combine(agentName, settingPath, '/');

                    return JsonHelper::GetValueString(_GetActiveConfigVariant(), fullPath, defaultValue);
                }

                bool AFDClient::GetSetting(
                    const std::string& agentName,
                    const std::string& settingPath,
                    const bool defaultValue)
                {
                    string fullPath = JsonHelper::Combine(agentName, settingPath, '/');

                    return JsonHelper::GetValueBool(_GetActiveConfigVariant(), fullPath, defaultValue);
                }

                int AFDClient::GetSetting(
                    const std::string& agentName,
                    const std::string& settingPath,
                    const int defaultValue)
                {
                    string fullPath = JsonHelper::Combine(agentName, settingPath, '/');

                    return JsonHelper::GetValueInt(_GetActiveConfigVariant(), fullPath, defaultValue);
                }

                double AFDClient::GetSetting(
                    const std::string& agentName,
                    const std::string& settingPath,
                    const double defaultValue)
                {
                    string fullPath = JsonHelper::Combine(agentName, settingPath, '/');

                    return JsonHelper::GetValueDouble(_GetActiveConfigVariant(), fullPath, defaultValue);
                }

                std::vector<std::string> AFDClient::GetSettings(
                    const std::string& agentName,
                    const std::string& settingPath)
                {
                    string fullPath = JsonHelper::Combine(agentName, settingPath, '/');
                    return JsonHelper::GetValuesString(_GetActiveConfigVariant(), fullPath);
                }

                std::vector<int> AFDClient::GetSettingsAsInts(
                    const std::string& agentName,
                    const std::string& settingPath)
                {
                    string fullPath = JsonHelper::Combine(agentName, settingPath, '/');

                    return JsonHelper::GetValuesInt(_GetActiveConfigVariant(), fullPath);
                }

                std::vector<double> AFDClient::GetSettingsAsDbls(
                    const std::string& agentName,
                    const std::string& settingPath)
                {
                    string fullPath = JsonHelper::Combine(agentName, settingPath, '/');

                    return JsonHelper::GetValuesDouble(_GetActiveConfigVariant(), fullPath);
                }

                std::string AFDClient::GetAFDConfiguration()
                {
                    return (m_configActive != NULL) ? m_configActive->configSettings.dump() : "";
                }
            }
        }
    }
}
#endif
