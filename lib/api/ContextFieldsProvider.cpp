// Copyright (c) Microsoft. All rights reserved.
#include "pal/PAL.hpp"

#include "ContextFieldsProvider.hpp"
#include "LogSessionData.hpp"

#include "utils/Utils.hpp"

namespace ARIASDK_NS_BEGIN
{

    ContextFieldsProvider::ContextFieldsProvider()
        : ContextFieldsProvider(nullptr)
    {
    }

    ContextFieldsProvider::ContextFieldsProvider(ContextFieldsProvider* parent)
        : m_parent(parent)
    {
        if (!m_parent)
        {
            PAL::registerSemanticContext(this);
        }
    }

    ContextFieldsProvider::ContextFieldsProvider(ContextFieldsProvider const& copy)
    {
        m_commonContextFields = copy.m_commonContextFields;
        m_customContextFields = copy.m_customContextFields;
        m_commonContextEventToConfigIds = copy.m_commonContextEventToConfigIds;
        m_ticketsMap = copy.m_ticketsMap;
    }

    ContextFieldsProvider& ContextFieldsProvider::operator=(ContextFieldsProvider const& copy)
    {
        m_commonContextFields = copy.m_commonContextFields;
        m_customContextFields = copy.m_customContextFields;
        m_commonContextEventToConfigIds = copy.m_commonContextEventToConfigIds;
        m_ticketsMap = copy.m_ticketsMap;
        return *this;
    }


    ContextFieldsProvider::~ContextFieldsProvider()
    {
        if (!m_parent)
        {
            PAL::unregisterSemanticContext(this);
        }
    }

    void ContextFieldsProvider::writeToRecord(::AriaProtocol::Record& record)
    {
        // Append parent scope context variables if not detached from parent
        if (m_parent)
        {
            m_parent->writeToRecord(record);
        }

        if (record.data.size() == 0)
        {
            ::AriaProtocol::Data data;
            record.data.push_back(data);
        }
        if (record.extApp.size() == 0)
        {
            ::AriaProtocol::App app;
            record.extApp.push_back(app);
        }

        if (record.extDevice.size() == 0)
        {
            ::AriaProtocol::Device device;
            record.extDevice.push_back(device);
        }

        if (record.extOs.size() == 0)
        {
            ::AriaProtocol::Os os;
            record.extOs.push_back(os);
        }

        if (record.extUser.size() == 0)
        {
            ::AriaProtocol::User user;
            record.extUser.push_back(user);
        }

        if (record.extLoc.size() == 0)
        {
            ::AriaProtocol::Loc loc;
            record.extLoc.push_back(loc);
        }

        if (record.extNet.size() == 0)
        {
            ::AriaProtocol::Net net;
            record.extNet.push_back(net);
        }

        if (record.extProtocol.size() == 0)
        {
            ::AriaProtocol::Protocol proto;
            record.extProtocol.push_back(proto);
        }

        if (record.extM365a.size() == 0)
        {
            ::AriaProtocol::M365a m365a;
            record.extM365a.push_back(m365a);
        }

        std::map<std::string, ::AriaProtocol::Value>& ext = record.data[0].properties;
        {
            LOCKGUARD(m_lock);

            std::string value = m_commonContextFields[COMMONFIELDS_APP_EXPERIMENTIDS].as_string;
            if (!value.empty())
            {// for ECS set event specific config ids
                std::string eventName = record.name;
                if (!eventName.empty())
                {
                    const auto& iter = m_commonContextEventToConfigIds.find(eventName);
                    if (iter != m_commonContextEventToConfigIds.end())
                    {
                        value = iter->second;
                    }
                }

                record.extApp[0].expId = value;
            }

            if (!m_commonContextFields.empty())
            {
                if (m_commonContextFields.find(COMMONFIELDS_APP_EXPERIMENT_IMPRESSION_ID) != m_commonContextFields.end())
                {
                    AriaProtocol::Value temp;
                    EventProperty prop = m_commonContextFields[COMMONFIELDS_APP_EXPERIMENT_IMPRESSION_ID];
                    temp.stringValue = prop.as_string;

                    ext[COMMONFIELDS_APP_EXPERIMENT_IMPRESSION_ID] = temp;
                }

                if (m_commonContextFields.find(COMMONFIELDS_APP_EXPERIMENTETAG) != m_commonContextFields.end())
                {
                    AriaProtocol::Value temp;
                    EventProperty prop = m_commonContextFields[COMMONFIELDS_APP_EXPERIMENTETAG];
                    temp.stringValue = prop.as_string;

                    ext[COMMONFIELDS_APP_EXPERIMENTETAG] = temp;
                }

                auto iter = m_commonContextFields.find(COMMONFIELDS_APP_ID);
                if (iter != m_commonContextFields.end())
                {
                    record.extApp[0].id = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_APP_VERSION);
                if (iter != m_commonContextFields.end())
                {
                    record.extApp[0].ver = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_APP_LANGUAGE);
                if (iter != m_commonContextFields.end())
                {
                    record.extApp[0].locale = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_DEVICE_ID);
                if (iter != m_commonContextFields.end())
                {
                    // Ref: https://osgwiki.com/wiki/CommonSchema/device_id
                    // Use "c:" prefix
                    std::string temp("c:");
                    const char *deviceId = iter->second.as_string;
                    if (deviceId != nullptr)
                    {
                        size_t len = strlen(deviceId);
                        // Strip curly braces from GUID while populating localId.
                        // Otherwise 1DS collector would not strip the prefix.
                        if ((deviceId[0] == '{') && (deviceId[len - 1] == '}'))
                        {
                            temp.append(deviceId + 1, len - 2);
                        }
                        else
                        {
                            temp.append(deviceId);
                        }
                    }
                    record.extDevice[0].localId = temp;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_DEVICE_MAKE);
                if (iter != m_commonContextFields.end())
                {
                    record.extProtocol[0].devMake = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_DEVICE_MODEL);
                if (iter != m_commonContextFields.end())
                {
                    record.extProtocol[0].devModel = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_DEVICE_CLASS);
                if (iter != m_commonContextFields.end())
                {
                    record.extDevice[0].deviceClass = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_COMMERCIAL_ID);
                if (iter != m_commonContextFields.end())
                {
                    record.extM365a[0].enrolledTenantId = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_OS_NAME);
                if (iter != m_commonContextFields.end())
                {
                    record.extOs[0].name = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_OS_BUILD);
                if (iter != m_commonContextFields.end())
                {
                    //EventProperty prop = (*m_commonContextFieldsP)[COMMONFIELDS_OS_VERSION];
                    record.extOs[0].ver = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_USER_ID);
                if (iter != m_commonContextFields.end())
                {
                    record.extUser[0].localId = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_USER_LANGUAGE);
                if (iter != m_commonContextFields.end())
                {
                    record.extUser[0].locale = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_USER_TIMEZONE);
                if (iter != m_commonContextFields.end())
                {
                    record.extLoc[0].timezone = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_USER_MSAID);
                if (iter != m_commonContextFields.end())
                {
                    record.extDevice[0].authSecId = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_NETWORK_COST);
                if (iter != m_commonContextFields.end())
                {
                    record.extNet[0].cost = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_NETWORK_PROVIDER);
                if (iter != m_commonContextFields.end())
                {
                    record.extNet[0].provider = iter->second.as_string;
                }

                iter = m_commonContextFields.find(COMMONFIELDS_NETWORK_TYPE);
                if (iter != m_commonContextFields.end())
                {
                    record.extNet[0].type = iter->second.as_string;
                }
            }

            if (m_ticketsMap.size() > 0)
            {
                std::vector<std::string> tickets;
                for (auto const& field : m_ticketsMap)
                {
                    tickets.push_back(field.second);
                }
                AriaProtocol::Protocol temp;
                temp.ticketKeys.push_back(tickets);
                record.extProtocol.push_back(temp);
            }

            for (auto const& field : m_customContextFields)
            {
                if (field.second.piiKind != PiiKind_None)
                {
                    AriaProtocol::PII pii;
                    pii.Kind = static_cast<AriaProtocol::PIIKind>(field.second.piiKind);
                    AriaProtocol::Value temp;
                    AriaProtocol::Attributes attrib;
                    attrib.pii.push_back(pii);


                    temp.attributes.push_back(attrib);

                    temp.stringValue = field.second.to_string();
                    record.data[0].properties[field.first] = temp;
                }
                else
                {
                    std::vector<uint8_t> guid;
                    uint8_t guid_bytes[16] = { 0 };

                    switch (field.second.type)
                    {
                    case EventProperty::TYPE_STRING:
                    {
                        AriaProtocol::Value temp;
                        temp.stringValue = field.second.to_string();
                        record.data[0].properties[field.first] = temp;
                        break;
                    }
                    case EventProperty::TYPE_INT64:
                    {
                        AriaProtocol::Value temp;
                        temp.type = ::AriaProtocol::ValueKind::ValueInt64;
                        temp.longValue = field.second.as_int64;
                        record.data[0].properties[field.first] = temp;
                        break;
                    }
                    case EventProperty::TYPE_DOUBLE:
                    {
                        AriaProtocol::Value temp;
                        temp.type = ::AriaProtocol::ValueKind::ValueDouble;
                        temp.doubleValue = field.second.as_double;
                        record.data[0].properties[field.first] = temp;
                        break;
                    }
                    case EventProperty::TYPE_TIME:
                    {
                        AriaProtocol::Value temp;
                        temp.type = ::AriaProtocol::ValueKind::ValueDateTime;
                        temp.longValue = field.second.as_time_ticks.ticks;
                        record.data[0].properties[field.first] = temp;
                        break;
                    }
                    case EventProperty::TYPE_BOOLEAN:
                    {
                        AriaProtocol::Value temp;
                        temp.type = ::AriaProtocol::ValueKind::ValueBool;
                        temp.longValue = field.second.as_bool;
                        record.data[0].properties[field.first] = temp;
                        break;
                    }
                    case EventProperty::TYPE_GUID:
                    {
                        GUID_t temp = field.second.as_guid;
                        temp.to_bytes(guid_bytes);
                        guid = std::vector<uint8_t>(guid_bytes, guid_bytes + sizeof(guid_bytes) / sizeof(guid_bytes[0]));

                        AriaProtocol::Value tempValue;
                        tempValue.type = ::AriaProtocol::ValueKind::ValueGuid;
                        tempValue.guidValue.push_back(guid);
                        record.data[0].properties[field.first] = tempValue;
                        break;
                    }
                    default:
                    {
                        // Convert all unknown types to string
                        AriaProtocol::Value temp;
                        temp.stringValue = field.second.to_string();
                        record.data[0].properties[field.first] = temp;
                    }
                    }
                }
            }
            LOG_TRACE("Record=%p decorated with SemanticContext=%p", &record, this);
        }
    }

    void ContextFieldsProvider::ClearExperimentIds()
    {
        // Clear the common ExperimentIds
        SetCommonField(COMMONFIELDS_APP_EXPERIMENTIDS, "");

        // Clear the map of all ExperimentsIds (that's associated with event)
        m_commonContextEventToConfigIds.clear();
    }

    void ContextFieldsProvider::SetEventExperimentIds(std::string const& eventName, std::string const& experimentIds)
    {
        if (eventName.empty())
        {
            return;
        }

        std::string eventNameNormalized = toLower(eventName);
        if (!experimentIds.empty())
        {
            m_commonContextEventToConfigIds[eventNameNormalized] = experimentIds;
        }
        else
        {
            m_commonContextEventToConfigIds.erase(eventNameNormalized);
        }
    }

    void ContextFieldsProvider::SetCommonField(const std::string& name, const EventProperty& value)
    {
        LOCKGUARD(m_lock);
        m_commonContextFields[name] = value;
    }

    void ContextFieldsProvider::SetCustomField(const std::string& name, const EventProperty& value)
    {
        LOCKGUARD(m_lock);
        m_customContextFields[name] = value;
    }

    void ContextFieldsProvider::SetTicket(TicketType type, const std::string& ticketValue)
    {
        LOCKGUARD(m_lock);
        if (!ticketValue.empty())
        {
            m_ticketsMap[type] = ticketValue;
        }
    }

    void ContextFieldsProvider::SetParentContext(ContextFieldsProvider* parent)
    {
        m_parent = parent;
    }

} ARIASDK_NS_END
