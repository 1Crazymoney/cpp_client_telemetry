#include "common/Common.hpp"
#include "filter/MengpingEventFilter.hpp"
#include "filter/MengpingEventFilterRegulator.hpp"
#include <vector>
using namespace testing;
using namespace ARIASDK_NS;

PIEventFilter newMengpingEventFilter()
{
	return (PIEventFilter) new MengpingEventFilter();
}


TEST(MengpingDemo, Demo)
{
	std::vector<const char*> filterStrings;
	std::vector<uint32_t> filterRates;
	filterStrings.push_back("100");
	filterRates.push_back(100);
	filterStrings.push_back("50");
	filterRates.push_back(50);
	filterStrings.push_back("0");
	filterRates.push_back(0);
	const char** s = filterStrings.data();
	const uint32_t* r = filterRates.data();

	MengpingEventFilterRegulator regulator(newMengpingEventFilter);
	regulator.SetExclusionFilter("token", s, r, 3);
	IEventFilter &filter = regulator.GetTenantFilter("token");

	int p100, p50, p0;
	p100 = p50 = p0 = 0;
	for (int i = 0; i < 100; i++)
	{
		if (filter.IsEventExcluded("100")) p100++;
		if (filter.IsEventExcluded("50")) p50++;
		if (filter.IsEventExcluded("0")) p0++;
	}

	std::cout << p100 << " " << p50 << " " << p0 << std::endl;
}
