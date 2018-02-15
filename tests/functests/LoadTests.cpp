// Copyright (c) Microsoft. All rights reserved.
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include "Common/Common.hpp"
#include "common/HttpServer.hpp"
#include "common/MockIRuntimeConfig.hpp"
#include "utils/Utils.hpp"
#include <api/ILogManagerInternal.hpp>
#include <bond_lite/All.hpp>
#include "bond/generated/AriaProtocol_types.hpp"
#include "bond/generated/AriaProtocol_readers.hpp"
#include <fstream>

#ifndef ARIASDK_PAL_SKYPE
#include "pdh.h"
#endif
using namespace testing;
using namespace ARIASDK_NS;

char const* const TEST_STORAGE_FILENAME = "LoadTests.db";


class EventSender : public Thread
{
  public:
    EventSender(ILogger* logger, EventLatency latency)
      : m_logger(logger),
        m_latency(latency)
    {
    }

    void start()
    {
        startThread();
    }

    void stop()
    {
        joinThread();
    }

  protected:
    virtual void onThread() override
    {
        int i = 0;
        while (!shouldTerminate()) {
            EventProperties event("event" + toString(++i));
            event.SetLatency(m_latency);
            event.SetProperty("data", std::string(static_cast<size_t>(m_prg.getRandomDouble() * 3072), '\42'));
            m_logger->LogEvent(event);
            if (i % 5) {
                PAL::sleep(static_cast<unsigned>(5 + m_prg.getRandomDouble() * 100));
            }
        }
    }

  private:
    ILogger*                   m_logger;
    EventLatency               m_latency;
    PAL::PseudoRandomGenerator m_prg;
};

class LoadTests : public Test,
                  public HttpServer::Callback
{
  protected:
    std::string                  serverAddress;
    LogConfiguration             configuration;
    MockIRuntimeConfig           runtimeConfig;
    HttpServer                   server;
    std::unique_ptr<ILogManagerInternal> logManager;
    unsigned                     recordsReceived;
    PDH_HQUERY                   cpuQuery;
    PDH_HQUERY                   diskQuery;
    PDH_HCOUNTER                 cpuTotal;
    PDH_HCOUNTER                 diskTotal;

  public:
    virtual void SetUp() override
    {
        int port = server.addListeningPort(0);
        std::ostringstream os;
        os << "localhost:" << port;
        serverAddress = "http://" + os.str() + "/";
        server.setServerName(os.str());
        server.addHandler("/", *this);

        //configuration.runtimeConfig = &runtimeConfig;
        configuration.SetProperty("cacheFilePath", TEST_STORAGE_FILENAME); 
        ::remove(TEST_STORAGE_FILENAME);

        EXPECT_CALL(runtimeConfig, SetDefaultConfig(_)).WillRepeatedly(DoDefault());
        EXPECT_CALL(runtimeConfig, GetCollectorUrl()).WillRepeatedly(Return(serverAddress));
        EXPECT_CALL(runtimeConfig, IsHttpRequestCompressionEnabled()).WillRepeatedly(Return(false));
        EXPECT_CALL(runtimeConfig, GetOfflineStorageMaximumSizeBytes()).WillRepeatedly(Return(UINT_MAX));
        EXPECT_CALL(runtimeConfig, GetEventLatency(_, _)).WillRepeatedly(Return(EventLatency_Unspecified));
        EXPECT_CALL(runtimeConfig, GetMetaStatsSendIntervalSec()).WillRepeatedly(Return(0));
        EXPECT_CALL(runtimeConfig, GetMetaStatsTenantToken()).WillRepeatedly(Return("metastats-tenant-token"));
        EXPECT_CALL(runtimeConfig, GetMaximumRetryCount()).WillRepeatedly(Return(1));
        EXPECT_CALL(runtimeConfig, GetUploadRetryBackoffConfig()).WillRepeatedly(Return("E,3000,3000,2,0"));
        EXPECT_CALL(runtimeConfig, GetMinimumUploadBandwidthBps()).WillRepeatedly(Return(512));
        EXPECT_CALL(runtimeConfig, GetMaximumUploadSizeBytes()).WillRepeatedly(Return(1 * 1024 * 1024));
        EXPECT_CALL(runtimeConfig, DecorateEvent(_, _, _)).WillRepeatedly(Return());

        logManager.reset(ILogManagerInternal::Create(configuration, &runtimeConfig));

        PdhOpenQuery(NULL, NULL, &cpuQuery);
        PdhAddEnglishCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
        PdhCollectQueryData(cpuQuery);

        PdhOpenQuery(NULL, NULL, &diskQuery);
        PdhAddEnglishCounter(cpuQuery, "\\PhysicalDisk(_Total)\\Disk Transfers/sec", NULL, &diskTotal);
        PdhCollectQueryData(diskQuery);

        recordsReceived = 0;
        server.start();
    }

    virtual void TearDown() override
    {
        if (cpuQuery)
        {
            PdhCloseQuery(cpuQuery);
        }

        logManager->FlushAndTeardown();
        ::remove(TEST_STORAGE_FILENAME);
        server.stop();
    }

    virtual int onHttpRequest(HttpServer::Request const& request, HttpServer::Response& response) override
    {
        UNREFERENCED_PARAMETER(response);
        UNREFERENCED_PARAMETER(request);
        auto payload = decodeRequest(request, false);
        recordsReceived += static_cast<unsigned int>(payload.size());

        return 200;
    }

    std::vector<AriaProtocol::CsEvent> decodeRequest(HttpServer::Request const& request, bool decompress)
    {
        std::vector<AriaProtocol::CsEvent> vector;

        if (decompress) {
            // TODO
        }

        size_t data = 0;
        size_t length = 0;
        while (data < request.content.size())
        {
            AriaProtocol::CsEvent result;
            length = request.content.size() - data;
            std::vector<uint8_t> test(request.content.data() + data, request.content.data() + data + length);
            size_t index = 3;
            bool found = false;
            while (index < length)
            {
                while (index < length && test[index] != '\x3')
                {
                    index++;
                }

                if (index < length)
                {
                    if (test[index + 1] == '3' && test[index + 2] == '.')
                    {
                        found = true;
                        break;
                    }
                    index++;
                }
            }
            if (!found)
            {
                index += 1;
            }
            std::vector<uint8_t> input(request.content.data() + data, request.content.data() + data + index - 1);

            bond_lite::CompactBinaryProtocolReader reader(input);
            EXPECT_THAT(bond_lite::Deserialize(reader, result), true);
            data += index - 1;
            vector.push_back(result);

        }
        return vector;
    }

    void waitForTotalRecordsReceived(unsigned timeout, unsigned expectedCount)
    {
        auto start = PAL::getUtcSystemTimeMs();
        while (recordsReceived < expectedCount) {
            if (PAL::getUtcSystemTimeMs() - start >= timeout * 1000) {
                GTEST_FATAL_FAILURE_("Didn't receive request within given timeout");
            }
            PAL::sleep(500);
        }
    }
};

bool isRequiredAvailableMemory()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG fourGB = 4294967296;
    if (memInfo.ullAvailPhys > fourGB)
    {
        return true;
    }
    else
    {
        return false;
    }
}


TEST_F(LoadTests, StartupAndShutdownIsFast)
{
#ifdef ARIASDK_PAL_SKYPE
    // Avoid counting cost of the fat RootTools initialization/shutdown.
    auf::init();
#endif
    
    unsigned const RESTART_COUNT           = 100;
    unsigned int MAX_TIME_PER_RESTART_MS = 1000;

    unsigned int maxtimeperrestart = MAX_TIME_PER_RESTART_MS;

    double avgCpuLoad = 0;
    PDH_FMT_COUNTERVALUE counterVal;	
    PDH_FMT_COUNTERVALUE diskVal;
    double avgDiskLoad = 0;
    

    int64_t time = PAL::getMonotonicTimeMs();
    for (unsigned i = 0; i < RESTART_COUNT; i++)
    {
        logManager.reset();
        logManager.reset(ILogManagerInternal::Create(configuration, &runtimeConfig));
    }

    time = PAL::getMonotonicTimeMs() - time;
    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    PdhCollectQueryData(diskQuery);
    PdhGetFormattedCounterValue(diskTotal, PDH_FMT_DOUBLE, NULL, &diskVal);
    avgCpuLoad = avgCpuLoad + counterVal.doubleValue;
    avgDiskLoad = avgDiskLoad + diskVal.doubleValue;

    //printf("\nAvg Processor (_Total) Processor Time = %f", NULL, avgCpuLoad);
    //printf("\navg PhysicalDisk (_Total)  Disk Read Timee = %f", NULL, avgDiskLoad);

    if (avgCpuLoad > 75)
    {
        maxtimeperrestart = MAX_TIME_PER_RESTART_MS + MAX_TIME_PER_RESTART_MS + MAX_TIME_PER_RESTART_MS;
    }
    else if (avgCpuLoad > 40)
    {
        maxtimeperrestart = MAX_TIME_PER_RESTART_MS + MAX_TIME_PER_RESTART_MS;
    }
    
    if (!isRequiredAvailableMemory())
    {
        maxtimeperrestart = maxtimeperrestart + MAX_TIME_PER_RESTART_MS;
    }	
    
    EXPECT_THAT(time / RESTART_COUNT, Lt(maxtimeperrestart));

#ifdef ARIASDK_PAL_SKYPE
    auf::stop();
#endif
}

TEST_F(LoadTests, ManyStartupsAndShutdownsAreHandledSafely)
{
    unsigned const EVENTS_COUNT  = 100;
    unsigned const RESTART_COUNT = 100;

    // Store some events so that system has to do something each time after startup.
    ContextFieldsProvider temp;
    ILogger* logger = logManager->GetLogger("loadtests-tenant-token", &temp);
    for (unsigned i = 0; i < EVENTS_COUNT; i++) {
        EventProperties event("event" + toString(i));
        event.SetProperty("data", std::string(1234, '\42'));
        logger->LogEvent(event);
    }

    for (unsigned i = 0; i < RESTART_COUNT; i++) {
        logManager.reset();
        logManager.reset(ILogManagerInternal::Create(configuration, &runtimeConfig));
    }

    // All events should eventually come.
    waitForTotalRecordsReceived(60, EVENTS_COUNT + 1); // +1 for meta stats
}

TEST_F(LoadTests, ManyEventsFromManyThreadsAreHandledSafely)
{
    unsigned const THREAD_COUNT     = 8;
    unsigned const TEST_DURATION_MS = 30000;

    EXPECT_CALL(runtimeConfig, GetMetaStatsSendIntervalSec()).WillRepeatedly(Return(3));
    ContextFieldsProvider temp;
    ILogger* const logger1 = logManager->GetLogger("loadtests1-tenant-token", &temp);
    ILogger* const logger2 = logManager->GetLogger("loadtests2-tenant-token", &temp);
    ILogger* logger = logger1;

    std::vector<std::unique_ptr<EventSender>> senders;
    for (unsigned i = 0; i < THREAD_COUNT; i++) {
        senders.emplace_back(new EventSender(logger, EventLatency_RealTime));
        senders.back()->start();
        logger = (logger == logger1) ? logger2 : logger1;
    }

    PAL::sleep(TEST_DURATION_MS);

    for (auto& sender : senders) {
        sender->stop();
    }
    senders.clear();
}
