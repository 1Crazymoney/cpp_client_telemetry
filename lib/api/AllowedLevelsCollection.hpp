// Copyright (c) Microsoft. All rights reserved.
#ifndef ALLOWEDLEVELSCOLLECTION_HPP
#define ALLOWEDLEVELSCOLLECTION_HPP

#include <mutex>
#include <vector>
#include <algorithm>
#include <cstdint>

#include "Version.hpp"
#include "ctmacros.hpp"

namespace ARIASDK_NS_BEGIN
{

    class AllowedLevelsCollection
    {
    public:
        AllowedLevelsCollection() noexcept = default;
        AllowedLevelsCollection(std::initializer_list<uint8_t>&& allowedLevels) noexcept;

        bool IsLevelInCollection(uint8_t level) const noexcept;
        std::size_t GetSize() const noexcept;
        uint8_t operator[](int index) const noexcept;

        void UpdateAllowedLevels(const std::vector<uint8_t>& levels) noexcept;

    private:
        mutable std::mutex m_allowedLevelsLock;
        std::vector<uint8_t> m_allowedLevels;
    };

} ARIASDK_NS_END

#endif // ALLOWEDLEVELSCOLLECTION_HPP