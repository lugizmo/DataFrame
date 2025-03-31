// Filename: View.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_VIEW_INDEXED_H
#define LUGIZMO_DF_VIEW_INDEXED_H

#include <cstddef>
#include <cassert>
#include <iterator>
#include <utility>
#include <span>

#include "lugizmo/memory/References.h"

#include "View.h"
#include "lugizmo/container/Concepts.h"

namespace lugizmo {

    /*
     * TODO doc
     * TODO make view not eagerly create view so that it can be used
     *      lazy with ranges.
    */
    template <typename T, typename I>
    class DFViewIndexed
    {
        static constexpr bool IsConstView = std::is_const_v<T>;

        static_assert(not std::is_pointer_v<T>   && not std::is_pointer_v<I>);
        static_assert(not std::is_reference_v<T> && not std::is_reference_v<I>);
        static_assert(std::is_const_v<I>, "The index should not be able to be mutated by this view.");

        using View    = DFView<T>;
        using Indices = std::span<I>;
        using Extents = typename View::Extents;
        using MDSpan  = typename View::MDSpan;

        static_assert(std::random_access_iterator<typename View::Iterator>);
        static_assert(std::random_access_iterator<typename Indices::iterator>);

        template<typename Layout>
        using MDSpanDF = typename View::template MDSpanDF<Layout>;

        View    dataView;
        Indices indexSpan;

        template<typename Layout>
        explicit DFViewIndexed(DFView<Layout>&& view, Indices const indices) noexcept :
            dataView(std::forward<DFView<Layout>>(view)),
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
            NullableAssignableReferenceWrapper<T> val;
            NullableAssignableReferenceWrapper<I> idx;

            constexpr IteratorValue(T* v, I* i) noexcept : val(v), idx(i) { assert(v != nullptr && i != nullptr); }
            constexpr ~IteratorValue() noexcept = default;

            constexpr IteratorValue(IteratorValue const& other) noexcept                    = default;
            constexpr IteratorValue(IteratorValue && other) noexcept                        = default;
            constexpr auto operator=(IteratorValue&& other) noexcept -> IteratorValue&      = default;
            constexpr auto operator=(IteratorValue const& other) noexcept -> IteratorValue& = default;

            constexpr auto first()        noexcept -> T*       { return val; }
            constexpr auto first()  const noexcept -> T const* { return val; }
            constexpr auto second() const noexcept -> I const* { return idx; }

            constexpr auto First()        noexcept -> T*       { return val; }
            constexpr auto First()  const noexcept -> T const* { return val; }
            constexpr auto Second() const noexcept -> I const* { return idx; }
        };

        static_assert(std::is_trivially_copyable_v<IteratorValue>, "Iterator value should just point/reference to the actual value.");

        /**
         *  @brief Implementation of an iterator for DFViewIndex.
         */
        class IteratorIdx
        {
            using DIt    = typename DFView<T>::Iterator;                                    // Referenced Data Iterator
            using IIt    = typename Indices::iterator;                                      // Referenced Index Iterator

            using Val    = std::conditional_t<IsConstView, IteratorValue const, IteratorValue>;   // Current value of the Iterator
            using OptVal = std::optional<IteratorValue>;                                          // Current value of the Iterator as Optional

            DIt    ptr;
            IIt    indices;
            mutable OptVal current;

            void SetCurrent(DIt& p, IIt& i)
            {
                assert(current.has_value());
                current.emplace(Val(p.operator->(), i.operator->()));
            }

        public:

            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = Val;
            using pointer           = Val*;
            using reference         = Val&;

            static_assert(std::is_same_v<decltype(std::declval<DIt>().operator->()), T*>);
            static_assert(std::is_same_v<decltype(std::declval<IIt>().operator->()), I const*>);

            IteratorIdx() noexcept :
                ptr(),
                indices(),
                current()
            {
            }

            IteratorIdx(DIt data, IIt indices) noexcept :
                ptr(data),
                indices(indices),
                current(Val(data.operator->(), indices.operator->()))
            {
            }

            IteratorIdx(IteratorIdx const& other) noexcept
            {
                if (this != &other)
                {
                    ptr = other.ptr;
                    indices = other.indices;
                    current = other.current;
                }
            }

            auto operator=(IteratorIdx const& other) noexcept -> IteratorIdx&
            {
                if (this != &other)
                {
                    ptr = other.ptr;
                    indices = other.indices;
                    current = other.current;
                }

                return *this;
            }

            auto operator*()  const noexcept -> reference { assert(current.has_value()); return *current; }
            auto operator->() const noexcept -> pointer   { assert(current.has_value()); return &*current; }

            auto operator*()  noexcept -> reference { assert(current.has_value()); return *current; }
            auto operator->() noexcept -> pointer   { assert(current.has_value()); return &*current; }

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

            auto operator[](difference_type const n) const -> reference
            {
                return {.val = *current.val, .idx = *current.idx};
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
        static auto RecordView(MDSpanDF<Layout> original, size_t const recIndex, Indices const fldIndices) noexcept -> DFViewIndexed
        {
            auto dfView = View::RecordView(original, recIndex);
            return DFViewIndexed(std::move(dfView), fldIndices);
        }

        template <typename Layout>
        [[nodiscard]]
        static auto FieldView(MDSpanDF<Layout> original, size_t const fldIndex, Indices const recIndices) noexcept -> DFViewIndexed
        {
            auto dfView = View::FieldView(original, fldIndex);
            return DFViewIndexed(std::move(dfView), recIndices);
        }

        [[nodiscard]] auto Size() const noexcept -> size_t { return dataView.view.extent(0); }
        [[nodiscard]] auto Empty() const noexcept -> bool  { return dataView.Size() == 0; }

        [[nodiscard]] auto operator[](size_t const i) noexcept -> IteratorValue
        {
            return {.val = dataView[i], .idx = indexSpan[i]};
        }

        [[nodiscard]] auto operator[](size_t const i) const noexcept -> IteratorValue
        {
            return {.val = dataView[i], .idx = indexSpan[i]};
        }

        [[nodiscard]] auto operator()(size_t const i) noexcept -> std::optional<IteratorValue>
        {
            if(i >= dataView.Size()) return std::nullopt;
            return {.val = dataView[i], .idx = indexSpan[i]};
        }

        [[nodiscard]] auto operator()(size_t const i) const noexcept -> std::optional<IteratorValue>
        {
            if(i >= dataView.Size()) return std::nullopt;
            return {.val = dataView[i], .idx = indexSpan[i]};
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

        template<typename Idx>
        auto Get(Idx const& index) -> std::optional<NullableAssignableReferenceWrapper<T>>
        {
            // TODO store additionally a reference to the
            //      index type so that we don't have linear search here
            auto pos = std::ranges::find(indexSpan, index);
            if(pos == indexSpan.end()) return std::nullopt;

            assert(indexSpan.size() == dataView.Size());
            return NullableAssignableReferenceWrapper<T>{&dataView[std::distance(indexSpan.begin(), pos)]};
        }

        template<typename Idx>
        auto Get(Idx const& index) const -> std::optional<NullableAssignableReferenceWrapper<T> const>
        {
            // TODO store additionally a reference to the
            //      index type so that we don't have linear search here
            auto pos = std::ranges::find(indexSpan, index);
            if(pos == indexSpan.end()) return std::nullopt;

            assert(indexSpan.size() == dataView.Size());
            return NullableAssignableReferenceWrapper<T>{&dataView[std::distance(indexSpan.begin(), pos)]};
        }

        template<typename Idx>
        auto GetFlattenOpt(Idx const& index) -> std::optional<NullableAssignableReferenceWrapper<RemovedOptional<T>>>
        {
            if constexpr(not OptionalType<T>)
            {
                return Get(index);
            }
            else
            {
                auto ref = Get(index);
                if(not ref.has_value())               return std::nullopt;
                if(not ref.value().Get().has_value()) return std::nullopt;

                return NullableAssignableReferenceWrapper<RemovedOptional<T>>{&ref.value().Get().value()};
            }
        }

        template<typename Idx>
        auto GetFlattenOpt(Idx const& index) const -> std::optional<NullableAssignableReferenceWrapper<RemovedOptional<T>> const>
        {
            if constexpr(not OptionalType<T>)
            {
                return Get(index);
            }
            else
            {
                auto ref = Get(index);
                if(not ref.has_value())               return std::nullopt;
                if(not ref.value().Get().has_value()) return std::nullopt;

                return NullableAssignableReferenceWrapper<RemovedOptional<T>>{&ref.value().Get().value()};
            }
        }

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

    static_assert(std::ranges::range<DFViewIndexed<int, int const>>, "Validation for range requirement failed.");
    static_assert(std::ranges::range<DFViewIndexed<int const, int const>>, "Validation for range requirement failed.");
}

#endif // LUGIZMO_DF_VIEW_INDEXED_H
