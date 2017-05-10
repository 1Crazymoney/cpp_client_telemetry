// Copyright (c) Microsoft. All rights reserved.

#include "common/Common.hpp"
#include "api/ContextFieldsProvider.hpp"

using namespace testing;
using namespace ARIASDK_NS;


TEST(EventPropertiesTests, Construction)
{
    EventProperties ep("test");
    EXPECT_THAT(ep.GetName(), Eq("test"));
    EXPECT_THAT(ep.GetPriority(), EventPriority_Unspecified);
    EXPECT_THAT(ep.GetTimestamp(), 0ll);
    EXPECT_THAT(ep.GetProperties(), IsEmpty());
    EXPECT_THAT(ep.GetPiiProperties(), IsEmpty());
}

TEST(EventPropertiesTests, Name)
{
    EventProperties ep("");
    EXPECT_THAT(ep.GetName(), Eq("undefined"));

    ep.SetName("Abcde123");
    EXPECT_THAT(ep.GetName(), Eq("abcde123"));

    ep.SetName("");
    EXPECT_THAT(ep.GetName(), Eq("abcde123"));

    ep.SetName("Weird. Characters_are_weird");
    EXPECT_THAT(ep.GetName(), Eq("abcde123"));
}

TEST(EventPropertiesTests, Timestamp)
{
    EventProperties ep("test");
    EXPECT_THAT(ep.GetTimestamp(), 0ll);

    ep.SetTimestamp(1234567890123456789ll);
    EXPECT_THAT(ep.GetTimestamp(), 1234567890123456789ll);

    ep.SetTimestamp(0);
    EXPECT_THAT(ep.GetTimestamp(), 0ll);

    ep.SetTimestamp(-1);
    EXPECT_THAT(ep.GetTimestamp(), -1);
}

TEST(EventPropertiesTests, Priority)
{
    EventProperties ep("test");
    EXPECT_THAT(ep.GetPriority(), EventPriority_Unspecified);

    ep.SetPriority(EventPriority_Off);
    EXPECT_THAT(ep.GetPriority(), EventPriority_Off);

    ep.SetPriority(EventPriority_Low);
    EXPECT_THAT(ep.GetPriority(), EventPriority_Low);

    ep.SetPriority(EventPriority_Normal);
    EXPECT_THAT(ep.GetPriority(), EventPriority_Normal);

    ep.SetPriority(EventPriority_High);
    EXPECT_THAT(ep.GetPriority(), EventPriority_High);

    ep.SetPriority(EventPriority_Immediate);
    EXPECT_THAT(ep.GetPriority(), EventPriority_Immediate);

    ep.SetPriority(EventPriority_Unspecified);
    EXPECT_THAT(ep.GetPriority(), EventPriority_Unspecified);
}

TEST(EventPropertiesTests, Properties)
{
    EventProperties ep("test");
    EXPECT_THAT(ep.GetProperties(), IsEmpty());

    ep.SetProperty("one",                          std::string("two"));
    ep.SetProperty("Weird.Characters_are_weird", std::string("value #2"));
    ep.SetProperty("public",                       "value #3", PiiKind_None);
    EXPECT_THAT(ep.GetProperties(), SizeIs(3));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("one", EventProperty("two"))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("Weird.Characters_are_weird", EventProperty("value #2"))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("public", EventProperty("value #3"))));
    EXPECT_THAT(ep.GetPiiProperties(), IsEmpty());
}

TEST(EventPropertiesTests, NumericProperties)
{
    EventProperties ep("test");
    EXPECT_THAT(ep.GetProperties(), IsEmpty());

    ep.SetProperty("char",     static_cast<char>(-123));
    ep.SetProperty("int",      static_cast<int>(-123123));
    ep.SetProperty("long",     static_cast<long>(-123123123));
    ep.SetProperty("uint8",    static_cast<uint8_t>(255));
    ep.SetProperty("unsigned", static_cast<unsigned>(999999999));
    ep.SetProperty("ull",      static_cast<unsigned long long>(9999999999999999999ull));
    ep.SetProperty("float",    static_cast<float>(1234.5f));
    ep.SetProperty("double",   static_cast<double>(-9876.543));
//    ep.SetProperty("lodouble", static_cast<long double>(-98769876.5435431));
    ep.SetProperty("true",     true);
    ep.SetProperty("false",    false);
    EXPECT_THAT(ep.GetProperties(), SizeIs(10));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("char", EventProperty(-123))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("int",  EventProperty(-123123))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("long", EventProperty(-123123123))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("uint8", EventProperty(255))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("unsigned", 999999999)));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("ull", EventProperty(9999999999999999999))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("float", EventProperty(1234.500000))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("double", EventProperty(-9876.543000))));
//    EXPECT_THAT(ep.GetProperties(), Contains(Pair("lodouble", EventProperty("-98769876.543543"))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("true", EventProperty(true))));
    EXPECT_THAT(ep.GetProperties(), Contains(Pair("false", EventProperty(false))));
    EXPECT_THAT(ep.GetPiiProperties(), IsEmpty());
}

TEST(EventPropertiesTests, PiiProperties)
{
    EventProperties ep("test");
    EXPECT_THAT(ep.GetPiiProperties(), IsEmpty());

    ep.SetProperty("secret",                       "dn=value",    PiiKind_DistinguishedName);
    ep.SetProperty("Weird.Characters_are_weird", "12.34.56.78", PiiKind_IPv4Address);
    EXPECT_THAT(ep.GetPiiProperties(), SizeIs(2));
    EXPECT_THAT(ep.GetPiiProperties(), Contains(Pair("secret",                       Pair("dn=value",    PiiKind_DistinguishedName))));
    EXPECT_THAT(ep.GetPiiProperties(), Contains(Pair("Weird.Characters_are_weird", Pair("12.34.56.78", PiiKind_IPv4Address))));
}
