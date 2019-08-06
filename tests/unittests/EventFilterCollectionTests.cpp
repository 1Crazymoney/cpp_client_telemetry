// Copyright (c) Microsoft. All rights reserved.

#include "common/Common.hpp"
#include "filter/EventFilterCollection.hpp"

using namespace testing;
using namespace MAT;

class TestEventFilterCollection : public EventFilterCollection
{
public:
    using EventFilterCollection::m_filters;
};

const char DefaultTestEventFilterName[] = "TestEventFilter";

class TestEventFilter : public IEventFilter
{
public:
    TestEventFilter() noexcept = default;
    TestEventFilter(const char* getNameReturnValue) noexcept
        : GetNameReturnValue(getNameReturnValue) { }
    TestEventFilter(bool canEventPropertiesBeSentReturnValue) noexcept
        : CanEventPropertiesBeSentReturnValue(canEventPropertiesBeSentReturnValue) { }

    const char* GetNameReturnValue{ DefaultTestEventFilterName };
    const char* GetName() const noexcept override { return GetNameReturnValue; }

    bool CanEventPropertiesBeSentReturnValue{ true };
    bool CanEventPropertiesBeSent(const EventProperties&) const noexcept override { return CanEventPropertiesBeSentReturnValue; }
};

TEST(EventFilterCollectionTests, Constructor_DefaultConstructed_NoRegisteredFilters)
{
    TestEventFilterCollection collection;
    EXPECT_EQ(collection.m_filters.size(), 0);
}

TEST(EventFilterCollectionTests, RegisterEventFilter_NullptrFilter_ThrowsArgumentException)
{
    try
    {
        TestEventFilterCollection collection;
        collection.RegisterEventFilter(nullptr);
        FAIL() << "No exception thrown";
    }
    catch (const std::invalid_argument& exception)
    {
        EXPECT_EQ(strcmp(exception.what(), "filter"), 0);
    }
    catch (...)
    {
        FAIL() << "Wrong throw.";
    }
}

TEST(EventFilterCollectionTests, RegisterEventFilter_ValidFilter_FilterSizeIsOne)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter()));
    EXPECT_EQ(collection.m_filters.size(), 1);
}

TEST(EventFilterCollectionTests, RegisterEventFilter_TwoValidFiltersWithTheSameName_FilterSizeIsTwo)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter()));
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter()));
    EXPECT_EQ(collection.m_filters.size(), 2);
}

TEST(EventFilterCollectionTests, UnregisterEventFilter_NullptrName_ThrowsArgumentException)
{
    try
    {
        TestEventFilterCollection collection;
        collection.UnregisterEventFilter(nullptr);
        FAIL() << "No exception thrown";
    }
    catch (const std::invalid_argument& exception)
    {
        EXPECT_EQ(strcmp(exception.what(), "filterName"), 0);
    }
    catch (...)
    {
        FAIL() << "Wrong throw.";
    }
}

TEST(EventFilterCollectionTests, UnregisterEventFilter_EventNameNotRegistered_DoesNotModifyCollection)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter()));
    collection.UnregisterEventFilter("NotTheDroidsYoureLookingFor");
    EXPECT_EQ(collection.m_filters.size(), 1);
}

TEST(EventFilterCollectionTests, UnregisterEventFilter_EventNameRegistered_ModifiesCollection)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter()));
    collection.UnregisterEventFilter(DefaultTestEventFilterName);
    EXPECT_EQ(collection.m_filters.size(), 0);
}

TEST(EventFilterCollectionTests, UnregisterEventFilter_EventNameRegisteredTwice_RemovesBoth)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter()));
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter()));
    collection.UnregisterEventFilter(DefaultTestEventFilterName);
    EXPECT_EQ(collection.m_filters.size(), 0);
}

TEST(EventFilterCollectionTests, UnregisterEventFilter_TwoDifferentlyNamedFilters_RemovesOne)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter("One")));
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter("Two")));
    collection.UnregisterEventFilter("One");
    EXPECT_EQ(collection.m_filters.size(), 1);
    EXPECT_EQ(strcmp(collection.m_filters[0]->GetName(), "Two"), 0);
}

TEST(EventFilterCollectionTests, UnregisterAllFilters_OneRegistered_ModifiesCollection)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter()));
    collection.UnregisterAllFilters();
    EXPECT_EQ(collection.m_filters.size(), 0);
}

TEST(EventFilterCollectionTests, UnregisterAllFilters_TwoRegistered_RemovesBoth)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter("One")));
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter("Two")));
    collection.UnregisterAllFilters();
    EXPECT_EQ(collection.m_filters.size(), 0);
}

TEST(EventFilterCollectionTests, CanEventPropertiesBeSent_OneRegisteredThatReturnsTrue_ReturnsTrue)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter(true)));
    EXPECT_TRUE(collection.CanEventPropertiesBeSent(EventProperties{}));
}

TEST(EventFilterCollectionTests, CanEventPropertiesBeSent_OneRegisteredThatReturnsFalse_ReturnsFalse)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter(false)));
    EXPECT_FALSE(collection.CanEventPropertiesBeSent(EventProperties{}));
}

TEST(EventFilterCollectionTests, CanEventPropertiesBeSent_TwoRegisteredFiltersThatReturnTrue_ReturnsTrue)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter(true)));
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter(true)));
    EXPECT_TRUE(collection.CanEventPropertiesBeSent(EventProperties{}));
}

TEST(EventFilterCollectionTests, CanEventPropertiesBeSent_TwoRegisteredFiltersThatReturnFalse_ReturnsFalse)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter(false)));
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter(false)));
    EXPECT_FALSE(collection.CanEventPropertiesBeSent(EventProperties{}));
}

TEST(EventFilterCollectionTests, CanEventPropertiesBeSent_TwoRegisteredFiltersOneTrueOneFalse_ReturnsFalse)
{
    TestEventFilterCollection collection;
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter(true)));
    collection.RegisterEventFilter(std::unique_ptr<IEventFilter>(new TestEventFilter(false)));
    EXPECT_FALSE(collection.CanEventPropertiesBeSent(EventProperties{}));
}