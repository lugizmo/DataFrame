// Filename: View.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_VIEW_INDEXED_H
#define LUGIZMO_DF_VIEW_INDEXED_H

#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <span>

#include "lugizmo/Assert.h"
#include "lugizmo/container/OptionalRef.h"

#include "View.h"
#include "IndexRange.h"

namespace lugizmo {

    /*
     * TODO doc
     * TODO make view not eagerly create view so that it can be used
     *      lazy with ranges.
    */
    template<typename T, typename I>
    class DFViewIndexed
    {
        // Data Types
        using View = DFView<T, I>;

        template<typename Layout>
        using MDSpanDF = View::template MDSpanDF<Layout>;

        // Index Types
        using KeyType       = I::ConstKeyType;
        using Indices       = std::conditional_t<DFRngIndex<std::remove_const_t<I>>, DFRangeIndexBounds<std::remove_const_t<KeyType>>, std::span<KeyType>>;
        using IndexIterator = Indices::iterator;

        static_assert(not std::is_pointer_v<T>   && not std::is_pointer_v<KeyType>);
        static_assert(not std::is_reference_v<T> && not std::is_reference_v<KeyType>);
        static_assert(std::is_const_v<KeyType>, "The index should not be able to be mutated by this view.");

        static_assert(std::random_access_iterator<typename View::Iterator>);
        static_assert(std::random_access_iterator<typename Indices::iterator>);

        View    dataView;
        Indices indexSpan;

        explicit DFViewIndexed(View&& view, Indices const indices) noexcept :
            dataView(std::move(view)),
            indexSpan(indices)
        {
        }

    public:

        /**
         * @brief Read-only reference-like access to an index key.
         * @details The index iterator is stored by value. For unique indices it refers to
         *          the stored key; for range indices it owns the generated key position.
         */
        class IndexReference
        {
            IndexIterator position;

        public:
            constexpr explicit IndexReference(IndexIterator indexPosition) noexcept :
                position(indexPosition)
            {
            }

            [[nodiscard]] constexpr auto HasValue() const noexcept -> bool { return true; }
            [[nodiscard]] constexpr explicit operator bool() const noexcept { return true; }

            [[nodiscard]] constexpr auto Pointer() const noexcept -> KeyType const*
            {
                return std::to_address(position);
            }

            [[nodiscard]] constexpr auto Get() const noexcept -> KeyType const& { return *Pointer(); }
            [[nodiscard]] constexpr auto Value() const noexcept -> KeyType const& { return *Pointer(); }
            [[nodiscard]] constexpr auto operator*() const noexcept -> KeyType const& { return *Pointer(); }
            [[nodiscard]] constexpr auto operator->() const noexcept -> KeyType const* { return Pointer(); }
        };

        static_assert(std::is_trivially_copyable_v<IndexReference>, "Index reference should remain a lightweight iterator wrapper.");

        /**
         *  @brief Value returned by the Iterator when dereferenced.
         *         Basically a pair of dataframe value and the associated
         *         index (either the field or record).
         *
         *  @attention This class makes use of a reference wrapper and therefor
         *             the iterator and user is responsible to keep references
         *             valid until the iterator is used. Otherwise, undefined behaviour!
         */
        struct IteratorValue
        {
            OptionalRef<T>       val;
            IndexReference       idx;

            constexpr IteratorValue(T* value, IndexIterator indexPosition) noexcept :
                val(value),
                idx(indexPosition)
            {
                LUGIZMO_ASSERT_TRACE(value != nullptr, "IteratorValue requires a non-null value pointer.");
            }

            constexpr ~IteratorValue() noexcept = default;

            constexpr IteratorValue(IteratorValue const& other) noexcept                    = default;
            constexpr IteratorValue(IteratorValue && other) noexcept                        = default;
            constexpr auto operator=(IteratorValue&& other) noexcept -> IteratorValue&      = default;
            constexpr auto operator=(IteratorValue const& other) noexcept -> IteratorValue& = default;

            // NOLINTBEGIN(readability-identifier-naming)
            constexpr auto first()        noexcept -> T*             { return val.Pointer(); }
            constexpr auto first()  const noexcept -> T const*       { return val.Pointer(); }
            constexpr auto second() const noexcept -> KeyType const* { return idx.Pointer(); }
            // NOLINTEND(readability-identifier-naming)

            constexpr auto First()        noexcept -> T*             { return val.Pointer(); }
            constexpr auto First()  const noexcept -> T const*       { return val.Pointer(); }
            constexpr auto Second() const noexcept -> KeyType const* { return idx.Pointer(); }
        };

        static_assert(std::is_trivially_copyable_v<IteratorValue>, "Iterator value should just point/reference to the actual value.");

        /**
         *  @brief Implementation of an iterator for DFViewIndex.
         */
        class IteratorIdx
        {
            using DIt    = View::Iterator;                                                              // Referenced Data Iterator
            using IIt    = Indices::iterator;                                                           // Referenced Index Iterator

            using Val    = std::conditional_t<std::is_const_v<T>, IteratorValue const, IteratorValue>;  // Current value of the Iterator
            using OptVal = std::optional<IteratorValue>;                                                // Current value of the Iterator as Optional

            DIt    ptr;
            IIt    indices;
            mutable OptVal current;
            mutable OptVal indexed;

            auto RefreshCurrent() const noexcept -> Val&
            {
                current.emplace(ptr.operator->(), indices);
                return *current;
            }

            void InvalidateCaches() noexcept
            {
                current.reset();
                indexed.reset();
            }

        public:

            // NOLINTBEGIN(readability-identifier-naming)
            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = Val;
            using pointer           = Val*;
            using reference         = Val&;
            // NOLINTEND(readability-identifier-naming)

            static_assert(std::is_same_v<decltype(std::declval<DIt>().operator->()), T*>);
            static_assert(std::is_same_v<decltype(std::declval<IIt>().operator->()), KeyType const*>);

            IteratorIdx() noexcept :
                ptr(),
                indices(),
                current(),
                indexed()
            {
            }

            IteratorIdx(DIt data, IIt indices) noexcept :
                ptr(data),
                indices(indices),
                current(),
                indexed()
            {
            }

            IteratorIdx(IteratorIdx const& other) noexcept :
                ptr(other.ptr),
                indices(other.indices),
                current(),
                indexed()
            {
            }

            IteratorIdx(IteratorIdx&& other) noexcept :
                ptr(std::move(other.ptr)),
                indices(std::move(other.indices)),
                current(),
                indexed()
            {
            }

            auto operator=(IteratorIdx const& other) noexcept -> IteratorIdx&
            {
                if(this == &other) return *this;

                ptr     = other.ptr;
                indices = other.indices;
                InvalidateCaches();
                return *this;
            }

            auto operator=(IteratorIdx&& other) noexcept -> IteratorIdx&
            {
                if(this == &other) return *this;

                ptr     = std::move(other.ptr);
                indices = std::move(other.indices);
                InvalidateCaches();
                return *this;
            }

            ~IteratorIdx() noexcept = default;

            auto operator*()  const noexcept -> reference
            {
                return RefreshCurrent();
            }
            auto operator->() const noexcept -> pointer
            {
                return &RefreshCurrent();
            }

            auto operator*()  noexcept -> reference
            {
                return RefreshCurrent();
            }
            auto operator->() noexcept -> pointer
            {
                return &RefreshCurrent();
            }

            auto operator++() -> IteratorIdx&
            {
                ptr.operator++();
                indices.operator++();
                InvalidateCaches();

                return *this;
            }

            auto operator++(int) -> IteratorIdx
            {
                IteratorIdx tmp = *this;
                ++(*this);
                return tmp;
            }

            auto operator--() -> IteratorIdx&
            {
                ptr.operator--();
                indices.operator--();
                InvalidateCaches();

                return *this;
            }

            auto operator--(int) -> IteratorIdx
            {
                IteratorIdx tmp = *this;
                --(*this);
                return tmp;
            }

            auto operator+(difference_type const n) const -> IteratorIdx
            {
                return IteratorIdx(ptr + n, indices + n);
            }

            auto operator+=(difference_type const n) -> IteratorIdx&
            {
                ptr.operator+=(n);
                indices += n;
                InvalidateCaches();

                return *this;
            }

            auto operator-(difference_type const n) const -> IteratorIdx
            {
                return IteratorIdx(ptr - n, indices - n);
            }

            auto operator-(IteratorIdx const& other) const -> difference_type
            {
                auto const dataDistance  = ptr - other.ptr;
                LUGIZMO_ASSERT_TRACE(dataDistance == (indices - other.indices), "DFViewIndexed iterator data and index positions diverged.");
                return dataDistance;
            }

            auto operator-=(difference_type const n) -> IteratorIdx&
            {
                ptr.operator-=(n);
                indices.operator-=(n);
                InvalidateCaches();

                return *this;
            }

            auto operator[](difference_type const n) const -> reference
            {
                indexed.emplace((ptr + n).operator->(), indices + n);
                return *indexed;
            }

            friend auto operator+(difference_type n, const IteratorIdx& it) -> IteratorIdx
            {
                return it + n;
            }

            auto operator==(IteratorIdx const& other) const noexcept -> bool { return ptr == other.ptr; }
            auto operator!=(IteratorIdx const& other) const noexcept -> bool { return ptr != other.ptr; }

            auto operator<(IteratorIdx  const& other) const noexcept -> bool { return ptr < other.ptr; }
            auto operator<=(IteratorIdx const& other) const noexcept -> bool { return ptr <= other.ptr; }
            auto operator>(IteratorIdx  const& other) const noexcept -> bool { return ptr > other.ptr; }
            auto operator>=(IteratorIdx const& other) const noexcept -> bool { return ptr >= other.ptr; }
        };

        static_assert(std::random_access_iterator<IteratorIdx>, "Validation for iterator requirement failed.");

        // empty view
        DFViewIndexed() noexcept : dataView(), indexSpan() {}

        template <typename Layout>
        [[nodiscard]]
        static auto RecordView(MDSpanDF<Layout> original, I const* index, size_t const recIndex, Indices const fldIndices) noexcept -> DFViewIndexed
        {
            auto dfView = View::RecordView(original, index, recIndex);
            return DFViewIndexed(std::move(dfView), fldIndices);
        }

        template <typename Layout>
        [[nodiscard]]
        static auto FieldView(MDSpanDF<Layout> original, I const* index, size_t const fldIndex, Indices const recIndices) noexcept -> DFViewIndexed
        {
            auto dfView = View::FieldView(original, index, fldIndex);
            return DFViewIndexed(std::move(dfView), recIndices);
        }

        [[nodiscard]] auto Size() const noexcept -> size_t { return dataView.view.extent(0) < 0 ? 0UZ : static_cast<size_t>(dataView.view.extent(0)); }
        [[nodiscard]] auto Empty() const noexcept -> bool  { return dataView.Size() == 0; }

        [[nodiscard]] auto operator[](size_t const i) noexcept -> IteratorValue
        {
            auto const offset = static_cast<std::ptrdiff_t>(i);
            return IteratorValue((dataView.begin() + offset).operator->(), indexSpan.begin() + offset);
        }

        [[nodiscard]] auto operator[](size_t const i) const noexcept -> IteratorValue
        {
            auto const offset = static_cast<std::ptrdiff_t>(i);
            return IteratorValue((dataView.begin() + offset).operator->(), indexSpan.begin() + offset);
        }

        [[nodiscard]] auto operator()(size_t const i) noexcept -> std::optional<IteratorValue>
        {
            if(i >= dataView.Size()) return std::nullopt;
            return (*this)[i];
        }

        [[nodiscard]] auto operator()(size_t const i) const noexcept -> std::optional<IteratorValue>
        {
            if(i >= dataView.Size()) return std::nullopt;
            return (*this)[i];
        }

        [[nodiscard]]
        auto begin() noexcept -> IteratorIdx // NOLINT(readability-identifier-naming)
        {
            return IteratorIdx(dataView.begin(), indexSpan.begin());
        }

        [[nodiscard]]
        auto end() noexcept -> IteratorIdx // NOLINT(readability-identifier-naming)
        {
            return IteratorIdx(dataView.end(), indexSpan.end());
        }

        [[nodiscard]]
        auto begin() const noexcept -> IteratorIdx // NOLINT(readability-identifier-naming)
        {
            return IteratorIdx(dataView.cbegin(), indexSpan.begin());
        }

        [[nodiscard]]
        auto end() const noexcept -> IteratorIdx // NOLINT(readability-identifier-naming)
        {
            return IteratorIdx(dataView.cend(), indexSpan.end());
        }

        [[nodiscard]]
        auto cbegin() const& noexcept -> IteratorIdx // NOLINT(readability-identifier-naming)
        {
            return IteratorIdx(dataView.cbegin(), indexSpan.begin());
        }

        [[nodiscard]]
        auto cend() const& noexcept -> IteratorIdx // NOLINT(readability-identifier-naming)
        {
            return IteratorIdx(dataView.cend(), indexSpan.end());
        }

        [[nodiscard]] auto Contains(KeyType const& key) noexcept -> bool;

        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*;
        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T const*;

        //[[nodiscard]] auto Front() noexcept -> T*;
        //[[nodiscard]] auto Front() const noexcept -> T const*;
        //[[nodiscard]] auto Back() noexcept -> T*;
        //[[nodiscard]] auto Back() const noexcept -> T const*;

        template <typename RangeAdaptor>
        [[nodiscard]]
        friend auto operator|(DFViewIndexed& view, RangeAdaptor&& adaptor)
        {
            return std::forward<RangeAdaptor>(adaptor)(std::ranges::subrange(view.begin(), view.end()));
        }

        template <typename RangeAdaptor>
        [[nodiscard]]
        friend auto operator|(DFViewIndexed const& view, RangeAdaptor&& adaptor)
        {
            return std::forward<RangeAdaptor>(adaptor)(std::ranges::subrange(view.begin(), view.end()));
        }
    };

    static_assert(std::ranges::range<DFViewIndexed<int, DFUniqueIndex<int const>>>, "Validation for range requirement failed.");
    static_assert(std::ranges::range<DFViewIndexed<int const, DFUniqueIndex<int const>>>, "Validation for range requirement failed.");

    template<typename T, typename  I>
    auto DFViewIndexed<T, I>::Contains(KeyType const& key) noexcept -> bool
    {
        return dataView.Contains(key);
    }

    template<typename T, typename  I>
    auto DFViewIndexed<T, I>::At(KeyType const& key) noexcept -> T*
    {
        return dataView.At(key);
    }

    template<typename T, typename  I>
    auto DFViewIndexed<T, I>::At(KeyType const& key) const noexcept -> T const*
    {
        return dataView.At(key);
    }

        //[[nodiscard]] auto Front() noexcept -> T*;
        //[[nodiscard]] auto Front() const noexcept -> T const*;
        //[[nodiscard]] auto Back() noexcept -> T*;
        //[[nodiscard]] auto Back() const noexcept -> T const*;

} // namespace lugizmo

#endif // LUGIZMO_DF_VIEW_INDEXED_H
