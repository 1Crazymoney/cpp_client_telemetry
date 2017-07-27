//------------------------------------------------------------------------------
// This code was generated by a tool.
//
//   Tool : bondjson2cpp 2016.12.18.1
//   File : AriaProtocol.json
//
// Changes to this file may cause incorrect behavior and will be lost when
// the code is regenerated.
// <auto-generated />
//------------------------------------------------------------------------------

#pragma once
#include "BondConstTypes.hpp"

namespace bond_lite {

template<typename TWriter>
void Serialize(TWriter& writer, ::AriaProtocol::PII const& value, bool isBase)
{
    writer.WriteStructBegin(nullptr, isBase);

    static_assert(sizeof(value.ScrubType) == 4, "Invalid size of enum");
    if (value.ScrubType != ::AriaProtocol::PIIScrubber::NotSet) {
        writer.WriteFieldBegin(BT_INT32, 1, nullptr);
        writer.WriteInt32(static_cast<int32_t>(value.ScrubType));
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_INT32, 1, nullptr);
    }

    static_assert(sizeof(value.Kind) == 4, "Invalid size of enum");
    if (value.Kind != ::AriaProtocol::PIIKind::NotSet) {
        writer.WriteFieldBegin(BT_INT32, 2, nullptr);
        writer.WriteInt32(static_cast<int32_t>(value.Kind));
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_INT32, 2, nullptr);
    }

    if (!value.RawContent.empty()) {
        writer.WriteFieldBegin(BT_STRING, 3, nullptr);
        writer.WriteString(value.RawContent);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 3, nullptr);
    }

    writer.WriteStructEnd(isBase);
}

template<typename TWriter>
void Serialize(TWriter& writer, ::AriaProtocol::CustomerContent const& value, bool isBase)
{
    writer.WriteStructBegin(nullptr, isBase);

    static_assert(sizeof(value.Kind) == 4, "Invalid size of enum");
    if (value.Kind != ::AriaProtocol::CustomerContentKind::NotSet) {
        writer.WriteFieldBegin(BT_INT32, 1, nullptr);
        writer.WriteInt32(static_cast<int32_t>(value.Kind));
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_INT32, 1, nullptr);
    }

    if (!value.RawContent.empty()) {
        writer.WriteFieldBegin(BT_STRING, 2, nullptr);
        writer.WriteString(value.RawContent);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 2, nullptr);
    }

    writer.WriteStructEnd(isBase);
}

template<typename TWriter>
void Serialize(TWriter& writer, ::AriaProtocol::Record const& value, bool isBase)
{
    writer.WriteStructBegin(nullptr, isBase);

    if (!value.Id.empty()) {
        writer.WriteFieldBegin(BT_STRING, 1, nullptr);
        writer.WriteString(value.Id);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 1, nullptr);
    }

    if (value.Timestamp != 0) {
        writer.WriteFieldBegin(BT_INT64, 3, nullptr);
        writer.WriteInt64(value.Timestamp);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_INT64, 3, nullptr);
    }

    if (!value.Type.empty()) {
        writer.WriteFieldBegin(BT_STRING, 5, nullptr);
        writer.WriteString(value.Type);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 5, nullptr);
    }

    if (!value.EventType.empty()) {
        writer.WriteFieldBegin(BT_STRING, 6, nullptr);
        writer.WriteString(value.EventType);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 6, nullptr);
    }

    if (!value.Extension.empty()) {
        writer.WriteFieldBegin(BT_MAP, 13, nullptr);
        writer.WriteMapContainerBegin(value.Extension.size(), BT_STRING, BT_STRING);
        for (auto const& item2 : value.Extension) {
            writer.WriteString(item2.first);
            writer.WriteString(item2.second);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 13, nullptr);
    }

    static_assert(sizeof(value.RecordType) == 4, "Invalid size of enum");
    if (value.RecordType != ::AriaProtocol::RecordType::NotSet) {
        writer.WriteFieldBegin(BT_INT32, 24, nullptr);
        writer.WriteInt32(static_cast<int32_t>(value.RecordType));
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_INT32, 24, nullptr);
    }

    if (!value.PIIExtensions.empty()) {
        writer.WriteFieldBegin(BT_MAP, 30, nullptr);
        writer.WriteMapContainerBegin(value.PIIExtensions.size(), BT_STRING, BT_STRUCT);
        for (auto const& item2 : value.PIIExtensions) {
            writer.WriteString(item2.first);
            Serialize(writer, item2.second, false);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 30, nullptr);
    }

    if (!value.TypedExtensionBoolean.empty()) {
        writer.WriteFieldBegin(BT_MAP, 31, nullptr);
        writer.WriteMapContainerBegin(value.TypedExtensionBoolean.size(), BT_STRING, BT_BOOL);
        for (auto const& item2 : value.TypedExtensionBoolean) {
            writer.WriteString(item2.first);
            writer.WriteBool(item2.second);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 31, nullptr);
    }

    if (!value.TypedExtensionDateTime.empty()) {
        writer.WriteFieldBegin(BT_MAP, 32, nullptr);
        writer.WriteMapContainerBegin(value.TypedExtensionDateTime.size(), BT_STRING, BT_INT64);
        for (auto const& item2 : value.TypedExtensionDateTime) {
            writer.WriteString(item2.first);
            writer.WriteInt64(item2.second);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 32, nullptr);
    }

    if (!value.TypedExtensionInt64.empty()) {
        writer.WriteFieldBegin(BT_MAP, 33, nullptr);
        writer.WriteMapContainerBegin(value.TypedExtensionInt64.size(), BT_STRING, BT_INT64);
        for (auto const& item2 : value.TypedExtensionInt64) {
            writer.WriteString(item2.first);
            writer.WriteInt64(item2.second);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 33, nullptr);
    }

    if (!value.TypedExtensionDouble.empty()) {
        writer.WriteFieldBegin(BT_MAP, 34, nullptr);
        writer.WriteMapContainerBegin(value.TypedExtensionDouble.size(), BT_STRING, BT_DOUBLE);
        for (auto const& item2 : value.TypedExtensionDouble) {
            writer.WriteString(item2.first);
            writer.WriteDouble(item2.second);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 34, nullptr);
    }

    if (!value.TypedExtensionGuid.empty()) {
        writer.WriteFieldBegin(BT_MAP, 35, nullptr);
        writer.WriteMapContainerBegin(value.TypedExtensionGuid.size(), BT_STRING, BT_LIST);
        for (auto const& item2 : value.TypedExtensionGuid) {
            writer.WriteString(item2.first);
            writer.WriteContainerBegin(item2.second.size(), BT_UINT8);
            for (auto const& item3 : item2.second) {
                writer.WriteUInt8(item3);
            }
            writer.WriteContainerEnd();
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 35, nullptr);
    }

    if (!value.CustomerContentExtensions.empty()) {
        writer.WriteFieldBegin(BT_MAP, 36, nullptr);
        writer.WriteMapContainerBegin(value.CustomerContentExtensions.size(), BT_STRING, BT_STRUCT);
        for (auto const& item2 : value.CustomerContentExtensions) {
            writer.WriteString(item2.first);
            Serialize(writer, item2.second, false);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 36, nullptr);
    }

    writer.WriteStructEnd(isBase);
}

template<typename TWriter>
void Serialize(TWriter& writer, ::AriaProtocol::DataPackage const& value, bool isBase)
{
    writer.WriteStructBegin(nullptr, isBase);

    if (!value.Type.empty()) {
        writer.WriteFieldBegin(BT_STRING, 1, nullptr);
        writer.WriteString(value.Type);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 1, nullptr);
    }

    if (!value.Source.empty()) {
        writer.WriteFieldBegin(BT_STRING, 2, nullptr);
        writer.WriteString(value.Source);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 2, nullptr);
    }

    if (!value.Version.empty()) {
        writer.WriteFieldBegin(BT_STRING, 3, nullptr);
        writer.WriteString(value.Version);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 3, nullptr);
    }

    if (!value.Ids.empty()) {
        writer.WriteFieldBegin(BT_MAP, 4, nullptr);
        writer.WriteMapContainerBegin(value.Ids.size(), BT_STRING, BT_STRING);
        for (auto const& item2 : value.Ids) {
            writer.WriteString(item2.first);
            writer.WriteString(item2.second);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 4, nullptr);
    }

    if (!value.DataPackageId.empty()) {
        writer.WriteFieldBegin(BT_STRING, 5, nullptr);
        writer.WriteString(value.DataPackageId);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_STRING, 5, nullptr);
    }

    if (value.Timestamp != 0) {
        writer.WriteFieldBegin(BT_INT64, 6, nullptr);
        writer.WriteInt64(value.Timestamp);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_INT64, 6, nullptr);
    }

    if (value.SchemaVersion != 1) {
        writer.WriteFieldBegin(BT_INT32, 7, nullptr);
        writer.WriteInt32(value.SchemaVersion);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_INT32, 7, nullptr);
    }

    if (!value.Records.empty()) {
        writer.WriteFieldBegin(BT_LIST, 8, nullptr);
        writer.WriteContainerBegin(value.Records.size(), BT_STRUCT);
        for (auto const& item2 : value.Records) {
            Serialize(writer, item2, false);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_LIST, 8, nullptr);
    }

    writer.WriteStructEnd(isBase);
}

template<typename TWriter>
void Serialize(TWriter& writer, ::AriaProtocol::ClientToCollectorRequest const& value, bool isBase)
{
    writer.WriteStructBegin(nullptr, isBase);

    if (!value.DataPackages.empty()) {
        writer.WriteFieldBegin(BT_LIST, 1, nullptr);
        writer.WriteContainerBegin(value.DataPackages.size(), BT_STRUCT);
        for (auto const& item2 : value.DataPackages) {
            Serialize(writer, item2, false);
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_LIST, 1, nullptr);
    }

    if (value.RequestRetryCount != 0) {
        writer.WriteFieldBegin(BT_INT32, 2, nullptr);
        writer.WriteInt32(value.RequestRetryCount);
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_INT32, 2, nullptr);
    }

    if (!value.TokenToDataPackagesMap.empty()) {
        writer.WriteFieldBegin(BT_MAP, 3, nullptr);
        writer.WriteMapContainerBegin(value.TokenToDataPackagesMap.size(), BT_STRING, BT_LIST);
        for (auto const& item2 : value.TokenToDataPackagesMap) {
            writer.WriteString(item2.first);
            writer.WriteContainerBegin(item2.second.size(), BT_STRUCT);
            for (auto const& item3 : item2.second) {
                Serialize(writer, item3, false);
            }
            writer.WriteContainerEnd();
        }
        writer.WriteContainerEnd();
        writer.WriteFieldEnd();
    } else {
        writer.WriteFieldOmitted(BT_MAP, 3, nullptr);
    }

    writer.WriteStructEnd(isBase);
}

} // namespace bond_lite
