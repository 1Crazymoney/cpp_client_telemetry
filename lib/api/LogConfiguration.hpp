#ifndef ARIA_LOGCONFIGURATION_HPP
#define ARIA_LOGCONFIGURATION_HPP

#include "ILogConfiguration.hpp"
#include "Version.hpp"
#include "Enums.hpp"
#include "ctmacros.hpp"
#include <map>
#include <mutex>

namespace Microsoft {
    namespace Applications {
        namespace Telemetry {

            class LogConfiguration : public ILogConfiguration
            {
            private:
                ACTTraceLevel m_minimumTraceLevel;
                SdkModeTypes m_sdkmode;
                std::mutex m_mutex;
                std::map<std::string, std::string>  strProps;
                std::map<std::string, uint32_t>     intProps;
                std::map<std::string, bool>         boolProps;

            public:

                virtual ACTStatus SetMinimumTraceLevel(ACTTraceLevel minimumTraceLevel) override;
                virtual ACTTraceLevel GetMinimumTraceLevel() const override;
                virtual ACTStatus SetSdkModeType(SdkModeTypes sdkmode) override;
                virtual SdkModeTypes GetSdkModeType() const override;
                virtual ACTStatus SetProperty(char const* key, char const* value) override;
                virtual ACTStatus SetIntProperty(char const* key, uint32_t value) override;
                virtual ACTStatus SetBoolProperty(char const* key, bool value) override;
                virtual ACTStatus SetPointerProperty(char const* key, void* value) override;
                virtual char const* GetProperty(char const* key, ACTStatus& error) const override;
                virtual uint32_t GetIntProperty(char const* key, ACTStatus& error) const override;
                virtual bool GetBoolProperty(char const* key, ACTStatus& error) const override;
                virtual void* GetPointerProperty(char const* key, ACTStatus& error) const override;

                ///<summary>LogConfiguration constructor</summary>
                LogConfiguration();

                ///<summary>LogConfiguration copy-constructor</summary>
                LogConfiguration(const LogConfiguration &src);

                ///<summary>LogConfiguration move-constructor</summary>
                LogConfiguration(LogConfiguration&& src) noexcept;

                ///<summary>LogConfiguration assignment operator with copy semantics</summary>
                LogConfiguration& ARIASDK_LIBABI_CDECL operator=(const LogConfiguration &src);

                ///<summary>LogConfiguration destructor</summary>
                virtual ~LogConfiguration();

            };
        }
    }
} // namespace Microsoft::Applications::Telemetry
#endif //MYAPPLICATION_EVENTPROPERTIES_H