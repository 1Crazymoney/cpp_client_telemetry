#include "DebugCallback.hpp"

unsigned   latency[MAX_LATENCY_SAMPLES] = { 0 };

std::atomic<unsigned>   eps(0);
std::atomic<unsigned>   numLogged0(0);
std::atomic<unsigned>   numLogged(0);
std::atomic<unsigned>   numSent(0);
std::atomic<unsigned>   numDropped(0);
std::atomic<unsigned>   numReject(0);
std::atomic<unsigned>   numCached(0);
std::atomic<unsigned>   logLatMin(100);
std::atomic<unsigned>   logLatMax(0);
unsigned long           testStartMs;

/// <summary>
/// The network cost names
/// </summary>
const char* networkCostNames[] = {
    "Unknown",
    "Unmetered",
    "Metered",
    "Roaming",
};

/// <summary>
/// Resets this instance.
/// </summary>
void MyDebugEventListener::reset()
{
    testStartMs = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    eps = 0;
    numLogged0 = 0;
    numLogged = 0;
    numSent = 0;
    numDropped = 0;
    numReject = 0;
    numCached = 0;
    logLatMin = 100;
    logLatMax = 0;
}

/// <summary>
/// The DebugEventListener constructor.
/// </summary>
/// <param name="evt"></param>
void MyDebugEventListener::OnDebugEvent(DebugEvent &evt)
{
    // lock for the duration of the print, so that we don't mess up the prints
    std::lock_guard<std::mutex> lock(dbg_callback_mtx);
    unsigned long ms;

    switch (evt.type) {
    case EVT_LOG_EVENT:
        // Track LogEvent latency here
        if (evt.param1 < logLatMin)
            logLatMin = evt.param1;
        if (evt.param1 > logLatMax)
            logLatMax = evt.param1;
    case EVT_LOG_LIFECYCLE:
    case EVT_LOG_FAILURE:
    case EVT_LOG_PAGEVIEW:
    case EVT_LOG_PAGEACTION:
    case EVT_LOG_SAMPLEMETR:
    case EVT_LOG_AGGRMETR:
    case EVT_LOG_TRACE:
    case EVT_LOG_USERSTATE:
    case EVT_LOG_SESSION:
        // printf("OnEventLogged:      seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        numLogged++;
        ms = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
        {
            eps = (1000 * numLogged) / (ms - testStartMs);
            if ((numLogged % 500) == 0)
            {
                printf("EPS=%u\n", eps.load() );
            }
        }
        break;
    case EVT_REJECTED:
        numReject++;
        printf("OnEventRejected:    seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        break;
    case EVT_ADDED:
        printf("OnEventAdded:       seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        break;
    case EVT_CACHED:
        numCached += evt.param1;
        // printf("OnEventCached:      seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        break;
    case EVT_DROPPED:
        numDropped += evt.param1;
        printf("OnEventDropped:     seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        break;
    case EVT_SENT:
        numSent += evt.param1;
        printf("OnEventsSent:       seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        break;
    case EVT_STORAGE_FULL:
        printf("OnStorageFull:      seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        if (evt.param1 >= 75) {
            // UploadNow must NEVER EVER be called from Aria callback thread, so either use this structure below
            // or notify the main app that it has to do the profile timers housekeeping / force the upload...
            std::thread([]() { LogManager::UploadNow(); }).detach();
        }
        break;

    case EVT_CONN_FAILURE:
    case EVT_HTTP_FAILURE:
    case EVT_COMPRESS_FAILED:
    case EVT_UNKNOWN_HOST:
    case EVT_SEND_FAILED:
        printf("OnEventsSendFailed: seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        break;
    case EVT_HTTP_ERROR:
        printf("OnHttpError:        seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u, data=%p, size=%d\n",
            evt.seq, evt.ts, evt.type, evt.param1, evt.param2, evt.data, evt.size);
        break;
    case EVT_HTTP_OK:
        printf("OnHttpOK:           seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u, data=%p, size=%d\n",
            evt.seq, evt.ts, evt.type, evt.param1, evt.param2, evt.data, evt.size);
        break;
    case EVT_SEND_RETRY:
        printf("OnSendRetry:        seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u, data=%p, size=%d\n",
            evt.seq, evt.ts, evt.type, evt.param1, evt.param2, evt.data, evt.size);
        break;
    case EVT_SEND_RETRY_DROPPED:
        printf("OnSendRetryDropped: seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u, data=%p, size=%d\n",
            evt.seq, evt.ts, evt.type, evt.param1, evt.param2, evt.data, evt.size);
        break;
    case EVT_NET_CHANGED:
        printf("OnNetChanged:       seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u [%s]\n",
            evt.seq, evt.ts, evt.type, evt.param1, evt.param2, networkCostNames[evt.param1]);
        if (evt.param2)
        {
            printf("Malwarebytes Antiexploit has been detected! Network cost is unknown.\n");
        }
        break;
    case EVT_UNKNOWN:
    default:
        printf("OnEventUnknown:     seq=%llu, ts=%llu, type=0x%08x, p1=%u, p2=%u\n", evt.seq, evt.ts, evt.type, evt.param1, evt.param2);
        break;
    };

};
