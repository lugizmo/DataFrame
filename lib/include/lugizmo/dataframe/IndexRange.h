// Filename: IndexRange.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_INDEX_RANGE_H
#define LUGIZMO_DF_INDEX_RANGE_H

#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

#include "IndexBase.h"

#include "lugizmo/Assert.h"
#include "lugizmo/container/Concepts.h"

namespace lugizmo {

    /**
     * @brief   A key type usable as a strided range index.
     * @details The index represents `begin, begin + step, begin + 2*step, ...` purely as the three
     *          scalars `(lower, upper, step)` and computes the bijection between positions `[0, N)`
     *          and keys in closed form, without storing any element:
     *
     *            - `position(key) = (key - lower) / step`
     *            - `key(position) = lower + position * step`
     *
     *          A type qualifies when it is totally ordered and "affine" over a difference type
     *          `D = decltype(a - b)`: subtracting two keys yields a `D`, a `D` divides another `D`
     *          to an integer count, and a key advances by an integer multiple of `D`. This admits
     *          integral types, `char`, scoped enums, pointers, and `std::chrono` durations and
     *          time points (where `step` is a duration).
     *
     * @tparam T Candidate key type.
     */
    template<typename T>
    concept DFRngKey =
            std::totally_ordered<T> &&
            requires(T a, T b, std::ptrdiff_t n)
            {
                a - b; // difference type D
                (a - b) / (a - b); // D / D -> integer count
                (a - b) % (a - b); // D % D -> alignment remainder
                { b + n * (a - b) } -> std::convertible_to<T>; // advance by an integer step count
            };

    /**
     * @brief   Value-semantic description of a strided range `[lower, upper)` with a `step`.
     * @details Stored as three scalars and iterable; iteration yields `lower, lower + step, ...`
     *          while strictly below `upper`. Returned by `DFRangeIndex::Keys()`.
     *
     * @tparam T Key type spanned by the range.
     */
    template<typename T = size_t>
    struct DFRangeIndexBounds
    {
        // Checked on instantiation (not on naming) so the type can still be named in a
        // std::conditional_t branch that is not selected (e.g., for unique-indexed dataframes).
        static_assert(DFRngKey<T>, "DFRangeIndexBounds requires a key type that satisfies DFRangeKey.");

        /// @brief Difference (step) type, e.g. `int` for integral keys or a duration for time points.
        using DiffType = decltype(std::declval<T>() - std::declval<T>());

        T        lower;
        T        upper;
        DiffType step = DiffType{1}; // contiguous by default; may be omitted in aggregate init

        /**
         * @brief  Returns the exact number of on-grid keys in `[lower, upper)`.
         * @return Count of keys `lower + k*step` that are `< upper`, i.e. `ceil((upper - lower) / step)`,
         *         or `0` when the range is empty. This equals the number of keys for which
         *         `DFRangeIndex::Has` is `true`.
         */
        [[nodiscard]] constexpr auto Size() const noexcept -> size_t
        {
            LUGIZMO_ASSERT(lower <= upper, "DFRangeIndexBounds requires lower <= upper.");
            if(upper <= lower) return 0;

            // Ceiling division so an unaligned `upper` still counts the last element.
            auto const span = upper - lower;
            return static_cast<size_t>((span + step - DiffType{1}) / step);
        }

        /// @copydoc Size()
        [[nodiscard]] constexpr auto size() const noexcept -> size_t
        {
            return Size();
        }

        /// @brief Returns whether the range spans no keys.
        [[nodiscard]] constexpr auto Empty() const noexcept -> bool { return upper <= lower; }

        /// @copydoc Empty()
        [[nodiscard]] constexpr auto empty() const noexcept -> bool { return Empty(); }

        /**
         * @brief Random-access iterator over the keys of the range, advancing by `step`.
         */
        struct Iterator
        {
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using iterator_category = std::random_access_iterator_tag;
            using reference         = T;
            using pointer           = T const*;

            T        value; // current key
            DiffType step;  // stride between consecutive keys

            // Dereference
            constexpr auto operator*() const noexcept -> T { return value; }
            constexpr auto operator->() const noexcept -> T const* { return &value; }

            // Increment / decrement (by one stride)
            constexpr Iterator& operator++() noexcept { value = static_cast<T>(value + step); return *this; }
            constexpr Iterator& operator--() noexcept { value = static_cast<T>(value - step); return *this; }

            constexpr Iterator operator++(int) noexcept { auto tmp = *this; ++(*this); return tmp; }
            constexpr Iterator operator--(int) noexcept { auto tmp = *this; --(*this); return tmp; }

            // Arithmetic (advance by `n` strides)
            constexpr Iterator& operator+=(difference_type n) noexcept
            {
                value = static_cast<T>(value + n * step);
                return *this;
            }

            constexpr Iterator& operator-=(difference_type n) noexcept
            {
                value = static_cast<T>(value - n * step);
                return *this;
            }

            friend constexpr Iterator operator+(Iterator it, difference_type n) noexcept { it += n; return it; }
            friend constexpr Iterator operator+(difference_type n, Iterator it) noexcept { it += n; return it; }
            friend constexpr Iterator operator-(Iterator it, difference_type n) noexcept { it -= n; return it; }
            friend constexpr difference_type operator-(Iterator a, Iterator b) noexcept
            {
                return static_cast<difference_type>((a.value - b.value) / a.step);
            }

            constexpr reference operator[](difference_type n) const noexcept { return static_cast<T>(value + n * step); }

            // Comparisons (by current key)
            constexpr bool operator==(Iterator const& other) const noexcept { return value == other.value; }
            constexpr bool operator!=(Iterator const& other) const noexcept { return value != other.value; }

            constexpr bool operator<(Iterator  const& other) const noexcept { return value < other.value; }
            constexpr bool operator<=(Iterator const& other) const noexcept { return value <= other.value; }
            constexpr bool operator>(Iterator  const& other) const noexcept { return value > other.value; }
            constexpr bool operator>=(Iterator const& other) const noexcept { return value >= other.value; }
        };

        using iterator = Iterator;
        [[nodiscard]] constexpr Iterator begin() const noexcept { return Iterator{.value = lower, .step = step}; }
        [[nodiscard]] constexpr Iterator end() const noexcept
        {
            auto const count = static_cast<std::ptrdiff_t>(Size());
            return Iterator{.value = static_cast<T>(lower + count * step), .step = step};
        }
    };

    /**
     * @brief   Strided range index mapping a key range `[lower, upper)` to positions `[0, Size())`.
     * @details Like a pandas `RangeIndex`: holds only `(lower, upper, step)` and resolves
     *          `position <-> key` in closed form, with no per-element storage. `step` defaults to
     *          one unit (the contiguous case). See `DFRangeKey` for the admitted key types.
     *
     * @tparam T Key type spanned by the range.
     */
    template<typename T = size_t>
    struct DFRangeIndex final : DFBaseRangeIndex<DFRangeIndex<T>, T>
    {
        static_assert(DFRngKey<T>, "DFRangeIndex requires a key type that satisfies DFRangeKey.");

        using KeyType      = T;
        using ConstKeyType = KeyType const;
        using KeyView      = DFRangeIndexBounds<T>;
        using DiffType     = DFRangeIndexBounds<T>::DiffType;

        /**
         * @brief Constructs an empty range.
         */
        explicit constexpr DFRangeIndex() noexcept :
            bounds{.lower = T{}, .upper = T{}, .step = DiffType{1}}
        {
        }

        /**
         * @brief Constructs a range `[lower, upper)` with the given stride.
         *
         * @details An invalid range (`lower > upper`) collapses to empty; a non-positive `step`
         *          falls back to one unit.
         *
         * @param[in] lower First key (position `0`).
         * @param[in] upper Exclusive upper bound.
         * @param[in] step  Stride between consecutive keys; defaults to one unit.
         */
        explicit constexpr DFRangeIndex(KeyType const lower, KeyType const upper, DiffType const step = DiffType{1}) noexcept :
            bounds{
                    .lower = lower <= upper ? lower : T{},
                    .upper = lower <= upper ? upper : T{},
                    .step = step > DiffType{0} ? step : DiffType{1}
            }
        {
        }

        constexpr DFRangeIndex(DFRangeIndex const&) noexcept = default;
        constexpr auto operator=(DFRangeIndex const&) noexcept -> DFRangeIndex& = default;

        constexpr DFRangeIndex(DFRangeIndex&&) noexcept = default;
        constexpr auto operator=(DFRangeIndex&&) noexcept -> DFRangeIndex& = default;

        ~DFRangeIndex() = default;

        /**
         * @brief Checks whether a key is a member of the range.
         *
         * @details A member must be within `[lower, upper)` and land on the step grid. For the
         *          contiguous case (`step == 1`) every in-bounds key is a member, so the alignment
         *          check is skipped.
         *
         * @tparam C Key type comparable with `T`.
         *
         * @param[in] key Key to test.
         * @return `true` if the key is in range and on the step grid.
         */
        template<typename C>
        [[nodiscard]]
        auto Has(C const& key) const noexcept -> bool
        {
            static_assert(ComparableType<C, T>, "DFRangeIndex::Has requires a key comparable with the index key type.");

            // TODO put compare logic somewhere else
            bool inBounds = false;
            if constexpr(std::is_integral_v<C> && std::is_integral_v<T>)
            {
                inBounds = std::cmp_greater_equal(key, bounds.lower) && std::cmp_less(key, bounds.upper);
            }
            else if constexpr(std::is_arithmetic_v<C> && std::is_arithmetic_v<T>)
            {
                using CT    = std::common_type_t<C, T>;
                const CT k  = static_cast<CT>(key);
                const CT lo = static_cast<CT>(bounds.lower);
                const CT hi = static_cast<CT>(bounds.upper);
                inBounds = k >= lo && k < hi;
            }
            else
            {
                inBounds = key >= bounds.lower && key < bounds.upper;
            }

            if(!inBounds) return false;
            if(bounds.step == DiffType{1}) return true; // contiguous: in-bounds imply member

            // strided: the key must sit on the step grid
            return (static_cast<T>(key) - bounds.lower) % bounds.step == DiffType{0};
        }

        /**
         * @brief Returns the range description `(lower, upper, step)`, which is iterable.
         */
        [[nodiscard]]
        constexpr auto Keys() const noexcept -> KeyView
        {
            return bounds;
        }

        /**
         * @brief Returns the underlying bounds by reference.
         */
        [[nodiscard]]
        constexpr auto Bounds() const noexcept -> DFRangeIndexBounds<T> const&
        {
            return bounds;
        }

        /**
         * @brief Returns the stride between consecutive keys.
         */
        [[nodiscard]]
        constexpr auto Step() const noexcept -> DiffType
        {
            return bounds.step;
        }

        /**
         * @brief Returns the inclusive lower bound (the key at position `0`).
         */
        [[nodiscard]]
        constexpr auto LowerBound() const noexcept -> KeyType
        {
            return bounds.lower;
        }

        /**
         * @brief Returns the position of the lower bound, always `0`.
         */
        [[nodiscard]]
        constexpr auto LowerBoundPosition() const noexcept -> size_t
        {
            return 0;
        }

        /**
         * @brief Returns the exclusive upper bound (one past the last key).
         */
        [[nodiscard]]
        constexpr auto UpperBound() const noexcept -> KeyType
        {
            return bounds.upper;
        }

        /**
         * @brief Returns the position one past the last key, i.e. `Size()`.
         */
        [[nodiscard]]
        constexpr auto UpperBoundPosition() const noexcept -> size_t
        {
            return Size();
        }

        /**
         * @brief Checks whether a key lies within the half-open bounds `[lower, upper)`.
         *
         * @details Bounds test only; does not verify step alignment (use `Position` for that).
         *
         * @param[in] key Key to test.
         * @return `true` if `lower <= key < upper`.
         */
        [[nodiscard]]
        constexpr auto InBound(KeyType const key) const noexcept -> bool
        {
            return key >= bounds.lower && key < bounds.upper;
        }

        /**
         * @brief Moves the lower bound, growing or shrinking the range from the front.
         *
         * @param[in] key New lower bound.
         * @return Signed change in element count (positive added, negative removed), or
         *         `std::nullopt` if the move is rejected (no change, or past the upper bound).
         */
        [[maybe_unused]]
        constexpr auto SetLowerBound(KeyType const key) noexcept -> std::optional<ssize_t>
        {
            if(key > bounds.upper || key == bounds.lower) return std::nullopt;

            auto const oldSize = static_cast<ssize_t>(Size());
            bounds.lower = key;
            return static_cast<ssize_t>(Size()) - oldSize;
        }

        /**
         * @brief Moves the upper bound, growing or shrinking the range from the back.
         *
         * @param[in] key New upper bound.
         * @return Signed change in element count (positive added, negative removed), or
         *         `std::nullopt` if the move is rejected (no change, or past the lower bound).
         */
        [[maybe_unused]]
        constexpr auto SetUpperBound(KeyType const key) noexcept -> std::optional<ssize_t>
        {
            if(key < bounds.lower || key == bounds.upper) return std::nullopt;

            auto const oldSize = static_cast<ssize_t>(Size());
            bounds.upper = key;

            return static_cast<ssize_t>(Size()) - oldSize;
        }

        /**
         * @brief Sets the stride between consecutive keys.
         *
         * @details Establishes the grid spacing. Re-spacing a non-empty range is rejected, since it
         *          would change which keys map to the existing positions.
         *
         * @param[in] newStep New stride (must be positive).
         * @return `true` if the step was set (or already equal), `false` if rejected.
         */
        [[maybe_unused]]
        constexpr auto SetStep(DiffType const newStep) noexcept -> bool
        {
            if(newStep <= DiffType{0}) return false;
            if(newStep == bounds.step) return true;
            if(!Empty()) return false; // cannot re-space a populated range

            bounds.step = newStep;
            return true;
        }

        /**
         * @brief Moves both bounds at once, reporting the per-side element-count change.
         *
         * @details Deltas are in elements (not key span), so they apply directly to the row/column
         *          count for any step. Bound moves must stay on the step grid; an off-grid move is
         *          rejected so existing positions keep their keys.
         *
         * @param[in] lower Optional new lower bound (unchanged when `std::nullopt`).
         * @param[in] upper Optional new upper bound (unchanged when `std::nullopt`).
         * @return Pair `{lowerChange, upperChange}` of signed element-count deltas (each
         *         `std::nullopt` when that bound did not move); `{nullopt, nullopt}` if the
         *         requested bounds are invalid (`lower > upper`) or off the step grid.
         */
        [[maybe_unused]]
        constexpr auto SetLowerUpperBound(std::optional<KeyType> const lower,
                                          std::optional<KeyType> const upper) noexcept -> std::pair<std::optional<ssize_t>, std::optional<ssize_t>>
        {
            auto const newLower = lower.value_or(bounds.lower);
            auto const newUpper = upper.value_or(bounds.upper);

            if(newLower > newUpper) return { std::nullopt, std::nullopt };

            // Both bounds must land on the existing step grid (underflow-safe distance).
            auto const aligned = [&](KeyType const a, KeyType const b) -> bool
            {
                auto const dist = (a < b) ? (b - a) : (a - b);
                return dist % bounds.step == DiffType{0};
            };
            if(!aligned(newLower, bounds.lower) || !aligned(newUpper, bounds.upper)) return { std::nullopt, std::nullopt };

            // Element count of a `[lo, hi)` grid; never asserts (invalid/empty -> 0), so it is safe
            // to evaluate on configurations that are only intermediate (e.g. newLower > old upper).
            auto const count = [&](KeyType const lo, KeyType const hi) -> ssize_t
            {
                if(hi <= lo) return 0;
                auto const span = hi - lo;
                return static_cast<ssize_t>((span + bounds.step - DiffType{1}) / bounds.step);
            };

            // Per-side element deltas: the front holds the old upper, the back holds the new lower.
            auto const oldCount = count(bounds.lower, bounds.upper);
            auto const midCount = count(newLower, bounds.upper);
            auto const newCount = count(newLower, newUpper);

            std::optional<ssize_t> lowerDiff, upperDiff;
            if(newLower != bounds.lower) lowerDiff = midCount - oldCount;
            if(newUpper != bounds.upper) upperDiff = newCount - midCount;

            bounds.lower = newLower;
            bounds.upper = newUpper;

            return { lowerDiff, upperDiff };
        }

        /**
         * @brief Resolves a key to its position (forward direction of the bijection).
         *
         * @param[in] key Key to look up.
         * @return `(key - lower) / step`, or `std::nullopt` if the key is out of range or not on
         *         the step grid.
         */
        [[nodiscard]]
        constexpr auto Position(KeyType const key) const noexcept -> std::optional<size_t>
        {
            if(key < bounds.lower || key >= bounds.upper) return std::nullopt;

            auto const offset = key - bounds.lower;
            if(offset % bounds.step != DiffType{0}) return std::nullopt; // not on the step grid

            return static_cast<size_t>(offset / bounds.step);
        }

        /**
         * @brief Resolves a position back to its key (inverse direction of the bijection).
         *
         * @param[in] position Position within the range.
         * @return `lower + position * step`, or `std::nullopt` if `position >= Size()`.
         */
        [[nodiscard]]
        constexpr auto Key(size_t const position) const noexcept -> std::optional<KeyType>
        {
            if(position >= Size()) return std::nullopt;
            return static_cast<KeyType>(bounds.lower + static_cast<std::ptrdiff_t>(position) * bounds.step);
        }

        /**
         * @brief Returns the number of keys in the range.
         */
        [[nodiscard]]
        constexpr auto Size() const noexcept -> size_t
        {
            LUGIZMO_ASSERT_TRACE(bounds.lower <= bounds.upper, "DFRangeIndex invariant failed: lower bound must not exceed upper bound.");
            return bounds.Size();
        }

        /**
         * @brief Returns whether the range spans no keys.
         */
        [[nodiscard]]
        constexpr auto Empty() const noexcept -> bool
        {
            return bounds.Empty();
        }

    private:

        DFRangeIndexBounds<T> bounds;
    };
}

#endif // LUGIZMO_DF_INDEX_RANGE_H
