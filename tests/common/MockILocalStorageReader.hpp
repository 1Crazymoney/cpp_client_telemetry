// Copyright (c) Microsoft. All rights reserved.

#pragma once

#include "Common.hpp"
#include "controlplane/ILocalStorage.hpp"
#include <gmock/gmock.h>

namespace testing
{
    class MockILocalStorageReader : public ARIASDK_NS::ControlPlane::ILocalStorageReader
    {
    public:
        MOCK_CONST_METHOD1(ReadTenantData, ARIASDK_NS::ControlPlane::TenantDataPtr(const ARIASDK_NS::GUID_t& ariaTenantId));
        MOCK_METHOD1(RegisterChangeEventHandler, void(const ARIASDK_NS::ControlPlane::ILocalStorageChangeEventHandler * const handler));
        MOCK_METHOD1(UnregisterChangeEventHandler, void(const ARIASDK_NS::ControlPlane::ILocalStorageChangeEventHandler * const handler));
    };

    class NotifiableMockILocalStorageReader : public MockILocalStorageReader
    {
    public:
        ARIASDK_NS::ControlPlane::ILocalStorageChangeEventHandler * m_handler;

        NotifiableMockILocalStorageReader()
        {
            m_handler = nullptr;
        }

        void RegisterChangeEventHandler(const ARIASDK_NS::ControlPlane::ILocalStorageChangeEventHandler * const handler) override
        {
            m_handler = (ARIASDK_NS::ControlPlane::ILocalStorageChangeEventHandler * const)handler;
            MockILocalStorageReader::RegisterChangeEventHandler(handler);
        }

        void UnregisterChangeEventHandler(const ARIASDK_NS::ControlPlane::ILocalStorageChangeEventHandler * const handler) override
        {
            m_handler = nullptr;
            MockILocalStorageReader::UnregisterChangeEventHandler(handler);
        }
    };
} // namespace testing
