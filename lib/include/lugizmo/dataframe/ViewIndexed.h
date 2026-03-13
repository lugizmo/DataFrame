// Filename: View.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_VIEW_INDEXED_H
#define LUGIZMO_DF_VIEW_INDEXED_H

#include <cstddef>
#include <iterator>
#include <utility>
#include <span>

#include "lugizmo/Assert.h"
#include "lugizmo/Error.h"
#include "lugizmo/container/Concepts.h"
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
        using KeyType = I::ConstKeyType;
        using Indices = std::conditional_t<DFSeqIndex<std::remove_const_t<I>>, DFRangeIndexBounds<std::remove_const_t<KeyType>>, std::span<KeyType>>;

        static_assert(not std::is_pointer_v<T>   && not std::is_pointer_v<KeyType>);
        static_assert(not std::is_reference_v<T> && not std::is_reference_v<KeyType>);
        static_assert(std::is_const_v<KeyType>, "The index should not be able to be mutated by this view.");

        static_assert(std::random_access_iterator<typename View::Iterator>);
        static_assert(std::random_access_iterator<typename Indices::iterator>);

        View    dataView;
        Indices indexSpan;

        explicit DFViewIndexed(View&& view, Indices const indices) noexcept :
            dataView(std::forward<View>(view)),
            indexSpan(indices)
        {
        }

    public:

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
            OptionalRef<KeyType> idx;

            constexpr IteratorValue(T* v, KeyType* i) noexcept : val(v), idx(i)
            {
                LUGIZMO_ASSERT_EXP(v != nullptr && i != nullptr, "IteratorValue requires non-null value and index pointers.");
            }
            constexpr ~IteratorValue() noexcept = default;

            constexpr IteratorValue(IteratorValue const& other) noexcept                    = default;
            constexpr IteratorValue(IteratorValue && other) noexcept                        = default;
            constexpr auto operator=(IteratorValue&& other) noexcept -> IteratorValue&      = default;
            constexpr auto operator=(IteratorValue const& other) noexcept -> IteratorValue& = default;

            constexpr auto first()        noexcept -> T*             { return val.Pointer(); }
            constexpr auto first()  const noexcept -> T const*       { return val.Pointer(); }
            constexpr auto second() const noexcept -> KeyType const* { return idx.Pointer(); }

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

            void SetCurrent(DIt& p, IIt& i)
            {
                LUGIZMO_ASSERT_EXP(current.has_value(), "IteratorIdx expected current value storage to be initialized.");
                current.emplace(Val(p.operator->(), i.operator->()));
            }

        public:

            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = Val;
            using pointer           = Val*;
            using reference         = Val&;

            static_assert(std::is_same_v<decltype(std::declval<DIt>().operator->()), T*>);
            static_assert(std::is_same_v<decltype(std::declval<IIt>().operator->()), KeyType const*>);

            IteratorIdx() noexcept :
                ptr(),
                indices(),
                current()
            {
            }

            IteratorIdx(DIt data, IIt indices) noexcept :
                ptr(data),
                indices(indices),
                current(Val(this->ptr.operator->(), this->indices.operator->()))
            {
            }

            IteratorIdx(IteratorIdx const& other) noexcept = default;
            auto operator=(IteratorIdx const& other) noexcept -> IteratorIdx& = default;

            auto operator*()  const noexcept -> reference
            {
                LUGIZMO_ASSERT(current.has_value(), "Cannot dereference DFViewIndexed end iterator.");
                return *current;
            }
            auto operator->() const noexcept -> pointer
            {
                LUGIZMO_ASSERT(current.has_value(), "Cannot dereference DFViewIndexed end iterator.");
                return &*current;
            }

            auto operator*()  noexcept -> reference
            {
                LUGIZMO_ASSERT(current.has_value(), "Cannot dereference DFViewIndexed end iterator.");
                return *current;
            }
            auto operator->() noexcept -> pointer
            {
                LUGIZMO_ASSERT(current.has_value(), "Cannot dereference DFViewIndexed end iterator.");
                return &*current;
            }

            auto operator++() -> IteratorIdx&
            {
                ptr.operator++();
                indices.operator++();
                SetCurrent(ptr, indices);

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
                SetCurrent(ptr, indices);

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
                return IteratorIdx(ptr + n * ptr.stride, indices + n);
            }

            auto operator+=(difference_type const n) -> IteratorIdx&
            {
                ptr.operator+=(n);
                indices += n;
                SetCurrent(ptr, indices);

                return *this;
            }

            auto operator-(difference_type const n) const -> IteratorIdx
            {
                return IteratorIdx(ptr - n * ptr.stride, indices - n);
            }

            auto operator-(IteratorIdx const& other) const -> difference_type
            {
                return (ptr - other.ptr) / ptr.stride;
            }

            auto operator-=(difference_type const n) -> IteratorIdx&
            {
                ptr.operator-=(n);
                indices.operator-=(n);
                SetCurrent(ptr, indices);

                return *this;
            }

            // TODO: check why argument is not used
            auto operator[](difference_type const) const -> reference
            {
                LUGIZMO_ASSERT(current.has_value(), "Cannot index DFViewIndexed iterator without a current value.");
                return *current;
            }

            friend auto operator+(difference_type n, const IteratorIdx& it) -> IteratorIdx
            {
                return it + n;
            }

            bool operator==(IteratorIdx const& other) const noexcept { return ptr == other.ptr; }
            bool operator!=(IteratorIdx const& other) const noexcept { return ptr != other.ptr; }

            bool operator<(IteratorIdx  const& other) const noexcept { return ptr < other.ptr; }
            bool operator<=(IteratorIdx const& other) const noexcept { return ptr <= other.ptr; }
            bool operator>(IteratorIdx  const& other) const noexcept { return ptr > other.ptr; }
            bool operator>=(IteratorIdx const& other) const noexcept { return ptr >= other.ptr; }
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
            return IteratorValue(&dataView[i], &indexSpan[i]);
        }

        [[nodiscard]] auto operator[](size_t const i) const noexcept -> IteratorValue
        {
            return IteratorValue(&dataView[i], &indexSpan[i]);
        }

        [[nodiscard]] auto operator()(size_t const i) noexcept -> std::optional<IteratorValue>
        {
            if(i >= dataView.Size()) return std::nullopt;
            return IteratorValue(&dataView[i], &indexSpan[i]);
        }

        [[nodiscard]] auto operator()(size_t const i) const noexcept -> std::optional<IteratorValue>
        {
            if(i >= dataView.Size()) return std::nullopt;
            return IteratorValue(&dataView[i], &indexSpan[i]);
        }

        [[nodiscard]]
        auto begin() noexcept -> IteratorIdx
        {
            return IteratorIdx(dataView.begin(), indexSpan.begin());
        }

        [[nodiscard]]
        auto end() noexcept -> IteratorIdx
        {
            return IteratorIdx(dataView.end(), indexSpan.end());
        }

        [[nodiscard]]
        auto begin() const noexcept -> IteratorIdx
        {
            return IteratorIdx(dataView.cbegin(), indexSpan.begin());
        }

        [[nodiscard]]
        auto end() const noexcept -> IteratorIdx
        {
            return IteratorIdx(dataView.cend(), indexSpan.end());
        }

        [[nodiscard]]
        auto cbegin() const& noexcept -> IteratorIdx
        {
            return IteratorIdx(dataView.cbegin(), indexSpan.begin());
        }

        [[nodiscard]]
        auto cend() const& noexcept -> IteratorIdx
        {
            return IteratorIdx(dataView.cend(), indexSpan.end());
        }

        [[nodiscard]] auto Contains(KeyType const& key) noexcept -> bool;

        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*;
        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T const*;
        [[nodiscard]] auto TryAt(KeyType const& key) const noexcept -> std::optional<T>;

        [[nodiscard]] auto Unwrap(KeyType const& key) noexcept -> RemovedOptional<T>* requires OptionalType<T>;
        [[nodiscard]] auto Unwrap(KeyType const& key) const noexcept -> RemovedOptional<T> const* requires OptionalType<T>;
        [[nodiscard]] auto TryUnwrap(KeyType const& key) const noexcept -> std::optional<RemovedOptional<T>> requires OptionalType<T>;

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
        // TODO store additionally a reference to the
        //      index type so that we don't have linear search here
        return std::ranges::find(indexSpan, key) != indexSpan.end();
    }

        template<typename T, typename  I>
        auto DFViewIndexed<T, I>::At(KeyType const& key) noexcept -> T*
        {
            // TODO store additionally a reference to the
            //      index type so that we don't have linear search here
            auto pos = std::ranges::find(indexSpan, key);
            if(pos == indexSpan.end()) return nullptr;

            LUGIZMO_ASSERT_EXP(indexSpan.size() == dataView.Size(), "DFViewIndexed invariant failed: index and data view size mismatch.");
            return &dataView[static_cast<size_t>(std::distance(indexSpan.begin(), pos))];
        }

        template<typename T, typename  I>
        auto DFViewIndexed<T, I>::At(KeyType const& key) const noexcept -> T const*
        {
            // TODO store additionally a reference to the
            //      index type so that we don't have linear search here
            auto pos = std::ranges::find(indexSpan, key);
            if(pos == indexSpan.end()) return nullptr;

            LUGIZMO_ASSERT_EXP(indexSpan.size() == dataView.Size(), "DFViewIndexed invariant failed: index and data view size mismatch.");
            return &dataView[static_cast<size_t>(std::distance(indexSpan.begin(), pos))];
        }

        template<typename T, typename  I>
        auto DFViewIndexed<T, I>::TryAt(KeyType const& key) const noexcept -> std::optional<T>
        {
            auto* ref = At(key);
            return std::optional<T>(ref != nullptr ? std::optional<T>(*ref) : std::optional<T>());
        }

        template<typename T, typename  I>
        auto DFViewIndexed<T, I>::Unwrap(KeyType const& key) noexcept -> RemovedOptional<T>* requires OptionalType<T>
        {
            auto* ref = At(key);
            if(not ref)              return nullptr;
            if(not ref->has_value()) return nullptr;

            return &(*ref).value();
        }

        template<typename T, typename  I>
        auto DFViewIndexed<T, I>::Unwrap(KeyType const& key) const noexcept -> RemovedOptional<T> const* requires OptionalType<T>
        {
            auto* ref = At(key);
            if(not ref)              return nullptr;
            if(not ref->has_value()) return nullptr;

            return &(*ref).value();
        }

        template<typename T, typename  I>
        auto DFViewIndexed<T, I>::TryUnwrap(KeyType const& key) const noexcept -> std::optional<RemovedOptional<T>> requires OptionalType<T>
        {
            auto const* ref = Unwrap(key);
            if(not ref) return std::nullopt;

            return {*ref};
        }

        //[[nodiscard]] auto Front() noexcept -> T*;
        //[[nodiscard]] auto Front() const noexcept -> T const*;
        //[[nodiscard]] auto Back() noexcept -> T*;
        //[[nodiscard]] auto Back() const noexcept -> T const*;
}

#endif // LUGIZMO_DF_VIEW_INDEXED_H
