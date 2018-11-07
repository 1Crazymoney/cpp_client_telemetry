#ifndef SYSTEMINFORMATIONIMPL_HPP
#define SYSTEMINFORMATIONIMPL_HPP
// Copyright (c) Microsoft Corporation. All rights reserved.
#include <pal/PAL.hpp>

#include "ISystemInformation.hpp"
#include "IInformationProvider.hpp"
#include "InformationProviderImpl.hpp"

#include "IPropertyChangedCallback.hpp"

#include <string>

namespace PAL_NS_BEGIN {

    class SystemInformationImpl : public ISystemInformation
    {
    public:
        static ISystemInformation* Create();

        // IInformationProvider API
        virtual int RegisterInformationChangedCallback(IPropertyChangedCallback* pCallback) override;
        virtual void UnRegisterInformationChangedCallback(int callbackToken) override;

        // ISystemInformation API
        virtual std::string const& GetAppId() const override { return m_app_id; };
        virtual std::string const& GetAppVersion() const override { return m_app_version; }
        virtual std::string const& GetAppLanguage() const override { return m_app_language; }

        virtual std::string const& GetOsFullVersion() const override { return m_os_full_version; };
        virtual std::string const& GetOsMajorVersion() const override { return m_os_major_version; };
        virtual std::string const& GetOsName() const override { return m_os_name; };

        virtual std::string const& GetUserLanguage() const override { return m_user_language; };
        virtual std::string const& GetUserTimeZone() const override { return m_user_timezone; };
        virtual std::string const& GetUserAdvertisingId() const override { return m_user_advertising_id; };

        virtual std::string const& GetDeviceClass() const override { return m_device_class; };

    private:
        std::string m_app_id;
        std::string m_app_version;
        std::string m_app_language;

        std::string m_os_major_version;
        std::string m_os_full_version;
        std::string m_os_name;

        std::string m_user_language;
        std::string m_user_timezone;
        std::string m_user_advertising_id;

        std::string m_device_class;

        InformatonProviderImpl m_info_helper;

        // Disable copy constructor and assignment operator.
        SystemInformationImpl(SystemInformationImpl const& other);
        SystemInformationImpl& operator=(SystemInformationImpl const& other);

        SystemInformationImpl();
        ~SystemInformationImpl();
    };

} PAL_NS_END

#endif
