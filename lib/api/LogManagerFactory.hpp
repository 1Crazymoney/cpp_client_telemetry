#ifndef LOGMANAGERFACTORY_HPP
#define LOGMANAGERFACTORY_HPP

#include "Enums.hpp"
#include "ILogConfiguration.hpp"
#include "ILogManager.hpp"

#include "NullObjects.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace ARIASDK_NS_BEGIN {

    constexpr const char * ANYHOST = "*";

    // set of modules assigned to this ILogManager pool
    typedef std::pair<std::set<std::string>, ILogManager*> Pool;

    // registry of all hosts and guests
    typedef std::map<std::string, Pool> Registry;

    class LogManagerFactory
    {

        /// <summary>
        /// Shared pool is for hosts/guests that will share resources.
        /// </summary>
        Registry shared;

        /// <summary>
        /// Exclusive pool is for hosts that don't want to share resources.
        /// </summary>
        Registry exclusive;

    protected:

        LogManagerFactory();

        void rehome(const std::string& host, const std::string& name);

        const ILogManager* find(const std::string& name);

        ILogManager* lease(ILogConfiguration& configuration);

        bool release(const std::string & name, const std::string& host);

        bool release(const std::string & name);

        bool release(ILogConfiguration& configuration);

        virtual ~LogManagerFactory();

        void dump();

        void parseConfig(ILogConfiguration& c, std::string& name, std::string& host);

        //  C++11 Magic Statics (N2660)
        static LogManagerFactory& instance() {
            static LogManagerFactory impl;
            return impl;
        }

    public:

        static const NullLogManager   NULL_LOGMANAGER;

        /// <summary>
        /// Creates a new ILogManager instance
        /// </summary>
        /// <param name="configuration">Configuration settings to apply to the telemetry logging system.</param>
        /// <returns>An ILogManager telemetry logging system instance created with the specified configuration.</returns>
        static ILogManager* Create(ILogConfiguration& configuration);

        /// <summary>
        /// Destroys a ILogManager instance
        /// </summary>
        static status_t Destroy(ILogManager*);

        static ILogManager * Get(
            ILogConfiguration & logConfiguration,
            status_t &status
#ifdef ANDROID
            , JNIEnv * env
            , jclass contextClass
            , jobject contextObject
#endif
        )
        {
            ILogManager * result = instance().lease(logConfiguration);
            status = (result != nullptr)?
                STATUS_SUCCESS :
                STATUS_EFAIL;
            return result;
        }

        static ILogManager* Get(
            const char * module,
            status_t& status
#ifdef ANDROID
            JNIEnv *env,
            jclass contextClass,
            jobject  contextObject,
#endif
        )
        {
            ILogConfiguration config = 
            {
                { "name", module },
                { "version", "0.0.0" },
                { "config", {  } }
            };
            ILogManager * result = instance().lease(config);
            status = (result != nullptr) ?
                STATUS_SUCCESS :
                STATUS_EFAIL;
            return result;
        }

        static status_t Release(const char * module)
        {
            return (instance().release(module)) ?
                STATUS_SUCCESS :
                STATUS_EFAIL;
        }

        static status_t Release(ILogConfiguration & config)
        {
            return (instance().release(config)) ?
                STATUS_SUCCESS :
                STATUS_EFAIL;
        }

    };

} ARIASDK_NS_END

#endif
