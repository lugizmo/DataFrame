// Filename: Span.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_SPAN_H
#define LUGIZMO_DF_SPAN_H

#include <cstddef>
#include <mdspan>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>

#include "lugizmo/Assert.h"
#include "IndexUnique.h"

namespace lgz {

    /**
     * @brief Non-owning contiguous DataFrame view with keyed lookup.
     *
     * `DFSpan` combines the contiguous storage and iterator guarantees of
     * `std::span` with the index-aware API of `DFView`. Element constness is
     * determined by `T`; constness of the wrapper does not make mutable elements
     * read-only.
     *
     * The values and attached index are not owned and must outlive the span.
     * Structural DataFrame changes invalidate the span and everything obtained
     * from it. A null index disables keyed lookup without affecting positional
     * access.
     *
     * @tparam T Element type, including its const qualification.
     * @tparam I Index type describing keys along the viewed axis.
     */
    template<typename T, typename I>
    class DFSpan : public std::ranges::view_interface<DFSpan<T, I>>
    {
        using IndexT  = I const*;
        using KeyType = I::KeyType;

        template<typename Layout>
        using MDSpanDF = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>;

        IndexT      dfIndex;
        std::size_t indexOffset;
        std::span<T> values;

        [[nodiscard]] auto LocalPosition(KeyType const& key) const noexcept -> std::optional<std::size_t>
        {
            if(dfIndex == nullptr) return std::nullopt;

            auto const position = dfIndex->Position(key);
            if(!position.has_value() || *position < indexOffset) return std::nullopt;

            auto const localPosition = *position - indexOffset;
            if(localPosition >= values.size()) return std::nullopt;
            return localPosition;
        }

        DFSpan(T* const data, std::size_t const size, IndexT const index, std::size_t const offset) noexcept :
            dfIndex(index),
            indexOffset(offset),
            values(data, size)
        {
        }

    public:

        // ======== TYPES ==========================================================================================================================================================

        using Iterator         = typename std::span<T>::iterator;
        using reverse_iterator = typename std::span<T>::reverse_iterator; // NOLINT(readability-identifier-naming)

        // ======== CONSTRUCTION ===================================================================================================================================================

        constexpr DFSpan() noexcept : dfIndex(nullptr), indexOffset(0), values() {}

        /**
         * @brief Creates a contiguous view over all fields of a row-major record.
         * @pre `recIndex < original.extent(0)`.
         */
        [[nodiscard]] static auto RecordView(MDSpanDF<std::layout_right> original, IndexT const index,
                                             std::size_t const recIndex) noexcept -> DFSpan
        {
            LUGIZMO_ASSERT_TRACE(original.extent(0) >= 0 && original.extent(1) >= 0,
                                 "DFSpan requires non-negative matrix extents.");
            [[maybe_unused]] auto const rowCount = original.extent(0) < 0 ? 0UZ : static_cast<std::size_t>(original.extent(0));
            auto const colCount = original.extent(1) < 0 ? 0UZ : static_cast<std::size_t>(original.extent(1));
            LUGIZMO_ASSERT_TRACE(recIndex < rowCount, "DFSpan::RecordView received an out-of-bounds record position.");

            auto* const data = original.data_handle() == nullptr ? nullptr : original.data_handle() + recIndex * colCount;
            return DFSpan(data, colCount, index, 0);
        }

        /**
         * @brief Creates a contiguous half-open field subrange of a row-major record.
         * @pre `recIndex < original.extent(0)` and `fldBegin <= fldEnd <= original.extent(1)`.
         */
        [[nodiscard]] static auto RecordView(MDSpanDF<std::layout_right> original, IndexT const index,
                                             std::size_t const recIndex, std::size_t const fldBegin,
                                             std::size_t const fldEnd) noexcept -> DFSpan
        {
            LUGIZMO_ASSERT_TRACE(original.extent(0) >= 0 && original.extent(1) >= 0,
                                 "DFSpan requires non-negative matrix extents.");
            [[maybe_unused]] auto const rowCount = original.extent(0) < 0 ? 0UZ : static_cast<std::size_t>(original.extent(0));
            auto const colCount = original.extent(1) < 0 ? 0UZ : static_cast<std::size_t>(original.extent(1));
            LUGIZMO_ASSERT_TRACE(recIndex < rowCount, "DFSpan::RecordView received an out-of-bounds record position.");
            LUGIZMO_ASSERT_TRACE(fldBegin <= fldEnd && fldEnd <= colCount,
                                 "DFSpan::RecordView received invalid field subrange bounds.");

            auto* const data = original.data_handle() == nullptr ? nullptr : original.data_handle() + recIndex * colCount + fldBegin;
            return DFSpan(data, fldEnd - fldBegin, index, fldBegin);
        }

        // ======== CAPACITY =======================================================================================================================================================

        [[nodiscard]] constexpr auto Size() const noexcept -> std::size_t { return values.size(); }
        [[nodiscard]] constexpr auto Empty() const noexcept -> bool { return values.empty(); }

        // ======== POSITIONAL ACCESS ==============================================================================================================================================

        [[nodiscard]] constexpr auto operator[](std::size_t const position) noexcept -> T& { return values[position]; }
        [[nodiscard]] constexpr auto operator[](std::size_t const position) const noexcept -> T& { return values[position]; }

        [[nodiscard]] constexpr auto operator()(std::size_t const position) noexcept -> T*
        {
            return position < Size() ? values.data() + position : nullptr;
        }

        [[nodiscard]] constexpr auto operator()(std::size_t const position) const noexcept -> T*
        {
            return position < Size() ? values.data() + position : nullptr;
        }

        [[nodiscard]] constexpr auto Front() noexcept -> T* { return Empty() ? nullptr : values.data(); }
        [[nodiscard]] constexpr auto Front() const noexcept -> T* { return Empty() ? nullptr : values.data(); }
        [[nodiscard]] constexpr auto Back() noexcept -> T* { return Empty() ? nullptr : values.data() + Size() - 1; }
        [[nodiscard]] constexpr auto Back() const noexcept -> T* { return Empty() ? nullptr : values.data() + Size() - 1; }

        // ======== KEY ACCESS =====================================================================================================================================================

        [[nodiscard]] auto Contains(KeyType const& key) const noexcept -> bool
        {
            return LocalPosition(key).has_value();
        }

        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*
        {
            auto const position = LocalPosition(key);
            return position.has_value() ? values.data() + *position : nullptr;
        }

        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T*
        {
            auto const position = LocalPosition(key);
            return position.has_value() ? values.data() + *position : nullptr;
        }

        // ======== ITERATORS ======================================================================================================================================================
        // NOLINTBEGIN(readability-identifier-naming)

        [[nodiscard]] constexpr auto begin() noexcept -> Iterator { return values.begin(); }
        [[nodiscard]] constexpr auto end() noexcept -> Iterator { return values.end(); }
        [[nodiscard]] constexpr auto begin() const noexcept -> Iterator { return values.begin(); }
        [[nodiscard]] constexpr auto end() const noexcept -> Iterator { return values.end(); }
        [[nodiscard]] constexpr auto cbegin() const noexcept -> Iterator { return values.begin(); }
        [[nodiscard]] constexpr auto cend() const noexcept -> Iterator { return values.end(); }
        [[nodiscard]] constexpr auto rbegin() noexcept -> reverse_iterator { return values.rbegin(); }
        [[nodiscard]] constexpr auto rend() noexcept -> reverse_iterator { return values.rend(); }
        [[nodiscard]] constexpr auto rbegin() const noexcept -> reverse_iterator { return values.rbegin(); }
        [[nodiscard]] constexpr auto rend() const noexcept -> reverse_iterator { return values.rend(); }
        [[nodiscard]] constexpr auto crbegin() const noexcept -> reverse_iterator { return values.rbegin(); }
        [[nodiscard]] constexpr auto crend() const noexcept -> reverse_iterator { return values.rend(); }

        // NOLINTEND(readability-identifier-naming)
    };

} // namespace lgz

// ======== STANDARD RANGE CUSTOMIZATION ===========================================================================================================================================

template<typename T, typename I>
inline constexpr bool std::ranges::enable_borrowed_range<lgz::DFSpan<T, I>> = true; // NOLINT(readability-identifier-naming)

static_assert(std::ranges::contiguous_range<lgz::DFSpan<int, lgz::DFUniqueIndex<int>>>);
static_assert(std::ranges::contiguous_range<lgz::DFSpan<int const, lgz::DFUniqueIndex<int>>>);
static_assert(std::ranges::sized_range<lgz::DFSpan<int, lgz::DFUniqueIndex<int>>>);
static_assert(std::ranges::common_range<lgz::DFSpan<int, lgz::DFUniqueIndex<int>>>);
static_assert(std::ranges::view<lgz::DFSpan<int, lgz::DFUniqueIndex<int>>>);
static_assert(std::ranges::borrowed_range<lgz::DFSpan<int, lgz::DFUniqueIndex<int>>>);

#endif // LUGIZMO_DF_SPAN_H
