// Copyright (c) Microsoft. All rights reserved.

#include "MockIHttpClient.hpp"
#include "MockILogManagerInternal.hpp"
#include "MockIOfflineStorage.hpp"
#include "MockIOfflineStorageObserver.hpp"
#include "MockIRuntimeConfig.hpp"
#include "MockISemanticContext.hpp"
#include "MockISqlite3Proxy.hpp"
#include "MockITelemetrySystem.hpp"

namespace testing {


    // Constructors and destructors of mock classes are compiled separately
    // to speed up compilation. See GMock documentation for the rationale:
    // https://github.com/google/googlemock/blob/master/googlemock/docs/CookBook.md#making-the-compilation-faster


    MockIHttpClient::MockIHttpClient() {}
    MockIHttpClient::~MockIHttpClient() {}

    MockILogManagerInternal::MockILogManagerInternal() {}
    MockILogManagerInternal::~MockILogManagerInternal() {}

    MockIOfflineStorageObserver::MockIOfflineStorageObserver() {}
    MockIOfflineStorageObserver::~MockIOfflineStorageObserver() {}

    MockIOfflineStorage::MockIOfflineStorage() {}
    MockIOfflineStorage::~MockIOfflineStorage() {}

    MockIRuntimeConfig::MockIRuntimeConfig() : ARIASDK_NS::RuntimeConfig_Default() {}
    MockIRuntimeConfig::~MockIRuntimeConfig() {}

    MockISemanticContext::MockISemanticContext() {}
    MockISemanticContext::~MockISemanticContext() {}

    MockISqlite3Proxy::MockISqlite3Proxy() {}
    MockISqlite3Proxy::~MockISqlite3Proxy() {}

    ITelemetrySystem& getSystem()
    {
        static testing::MockITelemetrySystem system;
        return system;
    }

} // namespace testing
