#pragma once
#include "IInformationProvider.hpp"
#include <string>

namespace Microsoft { namespace Applications { namespace Telemetry {
namespace PAL {

class ISystemInformation : public IInformationProvider
{
public:
    /// <summary>
    /// Gets the App ID.
    /// </summary>
    /// <returns>The App ID</returns>
    virtual std::string const& GetAppId() const = 0;

    /// <summary>
    /// Gets the App verison.
    /// </summary>
    /// <returns>The App version</returns>
    virtual std::string const& GetAppVersion() const = 0;

    /// <summary>
    /// Gets the App language.
    /// </summary>
    virtual std::string const& GetAppLanguage() const = 0;

    /// <summary>
    /// Gets the OS Full verison for the current device
    /// </summary>
    /// <returns>The OS full verison for the current device</returns>
    virtual std::string const& GetOsFullVersion() const = 0;

    /// <summary>
    /// Gets the OS major verison for the current device
    /// </summary>
    /// <returns>The OS major verison for the current device</returns>
    virtual std::string const& GetOsMajorVersion() const = 0;

    /// <summary>
    /// The name of the OS.The SDK should ensure this is a limited normalized set. Examples such as "iOS" or "Windows Phone".
    /// </summary>
    /// <returns>The OS Platofrm for the current device</returns>
    virtual std::string const& GetOsName() const = 0;

    /// <summary>
    /// Gets the user language.
    /// </summary>
    /// <returns>The user language</returns>
    virtual std::string const& GetUserLanguage() const = 0;

    /// <summary>
    /// Gets the user time zone.
    /// </summary>
    /// <returns>The user time zone</returns>
    virtual std::string const& GetUserTimeZone() const = 0;

    /// <summary>
    /// Gets the advertising Id, if enabled, for the current user.
    /// </summary>
    /// <returns>Advertising Id</returns>
    virtual std::string const& GetUserAdvertisingId() const = 0;

    /// <summary>
    /// Gets the Device class like desktop, tablet, phone, xbox.
    /// </summary>
    /// <returns>Advertising Id</returns>
    virtual std::string const& GetDeviceClass() const = 0;
};

} // PlatformAbstraction
}}}
