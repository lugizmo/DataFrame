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
#include <mdspan>
#include <span>

#include "Index.h"
#include "View.h"

namespace lugizmo {

    /*
     * TODO doc
     * TODO make view not eagerly create view so that it can be used
     *      lazy with ranges.
    */
    template <typename T, typename I>
    class DFViewIndexed
    {
        static_assert(not std::is_pointer_v<T>   && not std::is_pointer_v<I>);
        static_assert(not std::is_reference_v<T> && not std::is_reference_v<I>);
        static_assert(std::is_const_v<I>);

        using View    = DFView<T>;
        using Indices = std::span<I>;
        using Extents = typename View::Extents;
        using MDSpan  = typename View::MDSpan;

        template<typename Layout>
        using MDSpanDF = typename View::template MDSpanDF<Layout>;

        View    view;
        Indices index;

        template<typename Layout>
        explicit DFViewIndexed(DFView<Layout>&& view, Indices const indices) noexcept :
            view(std::forward<DFView<Layout>>(view)),
            index(indices)
        {
        }

    public:

        template<typename Val = T, typename Idx = I const>
        struct IteratorValue
        {
            Val& val;
            Idx& idx;

            auto first()        -> T&       { return val; }
            auto first()  const -> T const& { return val; }
            auto second() const -> I const& { return idx; }
        };

        template<bool Const>
        class IteratorBase
        {
            using BaseIt   = typename View::Iterator;
            using IdxIt    = typename Indices::iterator;
            using Val      = IteratorValue<T>;
            using ConstVal = IteratorValue<T const>;

            BaseIt ptr;
            IdxIt  indices;

            std::optional<Val> current;

            void SetCurrent(BaseIt& p, IdxIt& i)
            {
                assert(current.has_value());
                current.emplace(p.operator*(), i.operator*());
            }

        public:

            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = Val;
            using pointer           = std::conditional_t<Const, Val const*, Val*>;
            using reference         = std::conditional_t<Const, Val const&, Val&>;

            static_assert(std::is_same_v<decltype(std::declval<BaseIt>().operator->()), T*>);
            static_assert(std::is_same_v<decltype(std::declval<IdxIt>().operator->()), I const*>);

            IteratorBase() noexcept :
                ptr(nullptr),
                indices(nullptr),
                current()
            {
            }

            IteratorBase(BaseIt data, IdxIt indices) noexcept :
                ptr(data),
                indices(indices),
                current(Val{.val = data.operator*(), .idx = indices.operator*()})
            {
            }

            IteratorBase(IteratorBase const&)            = default;
            IteratorBase& operator=(IteratorBase const&) = default;

            auto operator*()  const noexcept -> reference { assert(current.has_value()); return *current;  }
            auto operator->() const noexcept -> pointer   { assert(current.has_value()); return &*current; }

            auto operator*()  noexcept -> reference { assert(current.has_value()); return *current; }
            auto operator->() noexcept -> pointer   { assert(current.has_value()); return &*current; }

            auto operator++() -> IteratorBase&
            {
                ptr.operator++();
                indices.operator++();
                SetCurrent(ptr, indices);

                return *this;
            }

            auto operator++(int) -> IteratorBase
            {
                IteratorBase tmp = *this;
                ++(*this);
                return tmp;
            }

            auto operator--() -> IteratorBase&
            {
                ptr.operator--();
                indices.operator--();
                SetCurrent(ptr, indices);

                return *this;
            }

            auto operator--(int) -> IteratorBase
            {
                IteratorBase tmp = *this;
                --(*this);
                return tmp;
            }

            auto operator+(difference_type const n) const -> IteratorBase
            {
                return IteratorIdx(ptr + n * ptr.stride, indices + n);
            }

            auto operator+=(difference_type const n) -> IteratorBase&
            {
                ptr.operator+=(n);
                indices += n;
                SetCurrent(ptr, indices);

                return *this;
            }

            auto operator-(difference_type const n) const -> IteratorBase
            {
                return IteratorIdx(ptr - n * ptr.stride, indices - n);
            }

            auto operator-(IteratorBase const& other) const -> difference_type
            {
                return (ptr - other.ptr) / ptr.stride;
            }

            auto operator-=(difference_type const n) -> IteratorBase&
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

            friend auto operator+(difference_type n, const IteratorBase& it) -> IteratorBase
            {
                return it + n;
            }

            bool operator==(IteratorBase const& other) const noexcept { return ptr == other.ptr; }
            bool operator!=(IteratorBase const& other) const noexcept { return ptr != other.ptr; }

            bool operator<(IteratorBase  const& other) const noexcept { return ptr < other.ptr; }
            bool operator<=(IteratorBase const& other) const noexcept { return ptr <= other.ptr; }
            bool operator>(IteratorBase  const& other) const noexcept { return ptr > other.ptr; }
            bool operator>=(IteratorBase const& other) const noexcept { return ptr >= other.ptr; }
        };

        using Iterator      = IteratorBase<false>;
        using ConstIterator = IteratorBase<true>;

        // empty view
        DFViewIndexed() noexcept : view(), index() {}

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

        [[nodiscard]] auto Size() const noexcept -> size_t { return view.view.extent(0); }
        [[nodiscard]] auto Empty() const noexcept -> bool  { return view.Size() == 0; }

        [[nodiscard]] auto operator[](size_t const i) noexcept -> IteratorValue<>
        {
            return {.val = view[i], .idx = index[i]};
        }

        [[nodiscard]] auto operator[](size_t const i) const noexcept -> IteratorValue<T const>
        {
            return {.val = view[i], .idx = index[i]};
        }

        [[nodiscard]] auto operator()(size_t const i) noexcept -> std::optional<IteratorValue<>>
        {
            if(i >= view.Size()) return std::nullopt;
            return {.val = view[i], .idx = index[i]};
        }

        [[nodiscard]] auto operator()(size_t const i) const noexcept -> std::optional<IteratorValue<T const>>
        {
            if(i >= view.Size()) return std::nullopt;
            return {.val = view[i], .idx = index[i]};
        }

        [[nodiscard]]
        auto begin() noexcept -> Iterator
        {
            return Iterator(view.begin(), index.begin());
        }

        [[nodiscard]]
        auto end() noexcept -> Iterator
        {
            return Iterator(view.end(), index.end());
        }

        [[nodiscard]]
        auto begin() const noexcept -> ConstIterator
        {
            return ConstIterator(view.begin(), index.begin());
        }

        [[nodiscard]] auto end() const noexcept -> ConstIterator
        {
            return ConstIterator(view.end(), index.end());
        }

        [[nodiscard]]
        auto cbegin() const& noexcept -> ConstIterator
        {
            return ConstIterator(view.cbegin(), index.cbegin());
        }

        [[nodiscard]] auto cend() const& noexcept -> ConstIterator
        {
            return ConstIterator(view.cend(), index.cend());
        }

        template <typename RangeAdaptor>
        // TODO add when implemented [[nodiscard]]
        friend auto operator|(DFViewIndexed& view, RangeAdaptor&& adaptor);

        template <typename RangeAdaptor>
        // TODO add when implemented [[nodiscard]]
        friend auto operator|(const DFViewIndexed& view, RangeAdaptor&& adaptor);
    };
}

#endif // LUGIZMO_DF_VIEW_INDEXED_H
