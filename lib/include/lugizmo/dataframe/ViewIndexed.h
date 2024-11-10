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

        class IteratorIdx
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
            using pointer           = Val*;
            using reference         = Val&;
            using const_pointer     = ConstVal const*;
            using const_reference   = ConstVal const&;

            static_assert(std::is_same_v<decltype(std::declval<BaseIt>().operator->()), T*>);
            static_assert(std::is_same_v<decltype(std::declval<IdxIt>().operator->()), I const*>);

            IteratorIdx() noexcept :
                ptr(nullptr),
                indices(nullptr),
                current()
            {
            }

            IteratorIdx(BaseIt data, IdxIt indices) noexcept :
                ptr(data),
                indices(indices),
                current(Val{.val = data.operator*(), .idx = indices.operator*()})
            {
            }

            IteratorIdx(IteratorIdx const&)            = default;
            IteratorIdx& operator=(IteratorIdx const&) = default;

            auto operator*()  const noexcept -> const_reference { assert(current.has_value()); return *current;  }
            auto operator->() const noexcept -> const_pointer   { assert(current.has_value()); return &*current; }

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

        [[nodiscard]]
        auto begin() & noexcept -> IteratorIdx
        {
            return IteratorIdx(view.begin(), index.begin());
        }

        [[nodiscard]]
        auto end() & noexcept -> IteratorIdx
        {
            return IteratorIdx(view.end(), index.end());
        }

        [[nodiscard]]
        auto begin() const& noexcept -> IteratorIdx
        {
            return IteratorIdx(view.begin(), index.begin());
        }

        [[nodiscard]] auto end() const& noexcept -> IteratorIdx
        {
            return IteratorIdx(view.end(), index.end());
        }
    };
}

#endif // LUGIZMO_DF_VIEW_INDEXED_H
