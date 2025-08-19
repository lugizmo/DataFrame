// Filename: View.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_VIEW_H
#define LUGIZMO_DF_VIEW_H

#include <array>
#include <cstddef>
#include <mdspan>
#include <optional>

#include "IndexUnique.h"

namespace lugizmo {

    /**
     * TODO doc
     * TODO make view not eagerly create view so that it can be used
     *      lazy with ranges.
     * @tparam T
     */
    template<typename T, typename I>
    class DFView
    {
        static constexpr bool IsConstView = std::is_const_v<T>;

        template<typename Ti, typename Ii>
        friend class DFViewIndexed;

        using Extents = std::dextents<size_t, 1>;
        using Strides = std::layout_stride::mapping<Extents>;
        using IndexT  = I const*;
        using KeyType = typename I::KeyType;
        using MDSpan  = std::mdspan<T, Extents, std::layout_stride>;

        template<typename Layout>
        using MDSpanDF = std::mdspan<T, std::dextents<size_t, 2>, Layout>;

        IndexT dfIndex;
        MDSpan view;

        /// record TODO doc
        ///        TODO test layout_left
        template<typename Layout>
        DFView(MDSpanDF<Layout> original, IndexT const originalIndex, size_t const index, bool const isFieldView) noexcept :
            dfIndex(originalIndex)
        {
            static_assert(std::is_same_v<Layout, std::layout_right> || std::is_same_v<Layout, std::layout_left>);

            if constexpr (std::is_same_v<Layout, std::layout_right>)
            {
                if(isFieldView)
                {
                    // needs stride span for fields
                    auto mapping = Strides(Extents(original.extent(0)), std::array<size_t, 1>{original.extent(1)});
                    view = MDSpan(original.data_handle() + index, mapping);
                }
                else
                {
                    auto mapping = Strides(Extents(original.extent(1)), std::array<size_t, 1>{1});
                    view = MDSpan(original.data_handle() + index * original.extent(1), mapping);
                }
            }
            else if constexpr(std::is_same_v<Layout, std::layout_left>)
            {
                if(isFieldView)
                {
                    auto mapping = Strides(Extents(original.extent(0)), std::array<size_t, 1>{1});
                    view = MDSpan(original.data_handle() + index, mapping);
                }
                else
                {
                    // needs stride span for records
                    auto mapping = Strides(Extents(original.extent(1)), std::array<size_t, 1>{original.extent(0)});
                    view = MDSpan(original.data_handle() + index * original.extent(0), mapping);
                }
            }
        }

        template<typename Layout>
        DFView(MDSpanDF<Layout> original, IndexT const originalIndex, size_t const index, size_t const begin, size_t const end, bool const isFieldView) noexcept :
            dfIndex(originalIndex)
        {
            assert(begin <= end && end <= isFieldView ? original.extent(0) : original.extent(1));
            auto const newExtent = end - begin;

            if(isFieldView)
            {
                if constexpr (std::is_same_v<Layout, std::layout_right>)
                {
                    auto mapping = Strides(Extents(newExtent), std::array<size_t, 1>{original.extent(1)});
                    view = MDSpan(original.data_handle() + begin * original.extent(1) + index, mapping);
                }
                else if constexpr (std::is_same_v<Layout, std::layout_left>)
                {
                    auto mapping = Strides(Extents(newExtent), std::array<size_t, 1>{1});
                    view = MDSpan(original.data_handle() + index * original.extent(0) + begin, mapping);
                }
            }
            else
            {
                if constexpr (std::is_same_v<Layout, std::layout_right>)
                {
                    auto mapping = Strides(Extents(newExtent), std::array<size_t, 1>{1});
                    view = MDSpan(original.data_handle() + index * original.extent(1) + begin, mapping);
                }
                else if constexpr (std::is_same_v<Layout, std::layout_left>)
                {
                    auto mapping = Strides(Extents(newExtent), std::array<size_t, 1>{original.extent(0)});
                    view = MDSpan(original.data_handle() + begin * original.extent(0) + index, mapping);
                }
            }
        }

    public:

        class Iterator
        {
            T*     ptr;
            size_t stride;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = T;
            using pointer           = std::conditional_t<IsConstView, T const*, T*>;
            using reference         = std::conditional_t<IsConstView, T const&, T&>;

            constexpr Iterator() noexcept : ptr(nullptr), stride(0) {}
            constexpr Iterator(T* data, size_t const stride) noexcept : ptr(data), stride(stride) {}

            constexpr Iterator(Iterator const& other) noexcept = default;
            constexpr auto operator=(Iterator const& other) -> Iterator& = default;

            constexpr auto operator*() const noexcept -> reference { return *ptr; }
            constexpr auto operator->() const noexcept -> pointer  { return ptr; }
            constexpr auto operator*() noexcept -> reference { return *ptr; }
            constexpr auto operator->() noexcept -> pointer  { return ptr; }

            constexpr auto operator++() -> Iterator& { ptr += stride; return *this; }
            constexpr auto operator++(int) -> Iterator { Iterator tmp = *this; ++(*this); return tmp;}
            constexpr auto operator--() -> Iterator& { ptr -= stride; return *this; }
            constexpr auto operator--(int) -> Iterator { Iterator tmp = *this; --(*this); return tmp; }
            constexpr auto operator+(difference_type const n) const -> Iterator { return Iterator(ptr + n * stride, stride); }
            constexpr auto operator+=(difference_type const  n) -> Iterator& { ptr += n * stride; return *this; }
            constexpr auto operator-(difference_type const  n) const -> Iterator { return Iterator(ptr - n * stride, stride); }
            constexpr auto operator-(Iterator const& other) const -> difference_type { return (ptr - other.ptr) / stride; }
            constexpr auto operator-=(difference_type const  n) -> Iterator& { ptr -= n * stride; return *this; }
            constexpr auto operator[](difference_type const  n) const -> reference { return *(ptr + n * stride); }

            constexpr friend auto operator+(difference_type n, const Iterator& it) -> Iterator { return it + n; }

            constexpr bool operator==(Iterator const& other) const noexcept { return ptr == other.ptr; }
            constexpr bool operator!=(Iterator const& other) const noexcept { return ptr != other.ptr; }
            constexpr bool operator<(Iterator const& other)  const noexcept { return ptr < other.ptr; }
            constexpr bool operator<=(Iterator const& other) const noexcept { return ptr <= other.ptr; }
            constexpr bool operator>(Iterator const& other)  const noexcept { return ptr > other.ptr; }
            constexpr bool operator>=(Iterator const& other) const noexcept { return ptr >= other.ptr; }
        };

        static_assert(std::random_access_iterator<Iterator>, "Validation for iterator requirement failed.");

        // empty view
        DFView() noexcept :
            dfIndex(nullptr)
        {
            auto mapping = Strides(Extents(0), std::array{size_t{0}});
            view = MDSpan(nullptr, std::move(mapping));
        }

        template<typename Layout>
        [[nodiscard]]
        static auto RecordView(MDSpanDF<Layout> original, IndexT const dfIndex, size_t const recIndex) noexcept -> DFView
        {
            return DFView(original, dfIndex, recIndex, false);
        }

        template<typename Layout>
        [[nodiscard]]
        static auto FieldView(MDSpanDF<Layout> original, IndexT const dfIndex, size_t const fldIndex) noexcept -> DFView
        {
            return DFView(original, dfIndex, fldIndex, true);
        }

        template<typename Layout>
        [[nodiscard]]
        static auto RecordView(MDSpanDF<Layout> original, IndexT const dfIndex, size_t const recIndex, size_t const fldBegin, size_t const fldEnd) noexcept -> DFView
        {
            return DFView(original, dfIndex, recIndex, fldBegin, fldEnd, false);
        }

        template<typename Layout>
        [[nodiscard]]
        static auto FieldView(MDSpanDF<Layout> original, IndexT const dfIndex, size_t const fldIndex, size_t const recBegin, size_t const recEnd) noexcept -> DFView
        {
            return DFView(original, dfIndex, fldIndex, recBegin, recEnd, true);
        }

        [[nodiscard]] constexpr auto Size() const noexcept -> size_t { return view.extent(0); }
        [[nodiscard]] constexpr auto Empty() const noexcept -> bool  { return Size() == 0; }

        [[nodiscard]] constexpr auto operator[](size_t i) noexcept -> T& { return view[i]; }
        [[nodiscard]] constexpr auto operator[](size_t i) const noexcept -> const T& { return view[i]; }

        [[nodiscard]] constexpr auto operator()(size_t i) noexcept -> std::optional<T*> { return i < view.extent(0) ? &view[i] : std::nullopt; }
        [[nodiscard]] constexpr auto operator()(size_t i) const noexcept -> std::optional<T const*> { return i < view.extent(0) ? &view[i] : std::nullopt; }

        [[nodiscard]] auto Contains(KeyType const& key) -> bool;

        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*;
        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T const*;

        [[nodiscard]] auto TryAt(KeyType const& key) const noexcept -> std::optional<T>;

        [[nodiscard]] auto Unwrap(KeyType const& key) noexcept -> RemovedOptional<T>* requires OptionalType<T>;
        [[nodiscard]] auto Unwrap(KeyType const& key) const noexcept -> RemovedOptional<T> const* requires OptionalType<T>;
        [[nodiscard]] auto TryUnwrap(KeyType const& key) const noexcept -> std::optional<RemovedOptional<T>> requires OptionalType<T>;

        [[nodiscard]] auto Front() noexcept -> T*;
        [[nodiscard]] auto Front() const noexcept -> T const*;
        [[nodiscard]] auto Back() noexcept -> T*;
        [[nodiscard]] auto Back() const noexcept -> T const*;

        [[nodiscard]]
        constexpr auto begin() noexcept -> Iterator
        {
            return Iterator(view.data_handle(), view.mapping().stride(0));
        }

        [[nodiscard]]
        constexpr auto end() noexcept -> Iterator
        {
            return Iterator(view.data_handle() + view.extent(0) * view.mapping().stride(0), view.mapping().stride(0));
        }

        [[nodiscard]]
        constexpr auto begin() const noexcept -> Iterator
        {
            return Iterator(view.data_handle(), view.mapping().stride(0));
        }

        [[nodiscard]]
        constexpr auto end() const noexcept -> Iterator
        {
            return Iterator(view.data_handle() + view.extent(0) * view.mapping().stride(0), view.mapping().stride(0));
        }

        [[nodiscard]] constexpr auto cbegin() const noexcept -> Iterator { return begin(); }
        [[nodiscard]] constexpr auto cend() const noexcept -> Iterator { return end(); }

        template <typename RangeAdaptor>
        [[nodiscard]]
        friend auto operator|(DFView& v, RangeAdaptor&& adaptor)
        {
            return std::forward<RangeAdaptor>(adaptor)(std::ranges::subrange(v.begin(), v.end()));
        }

        template <typename RangeAdaptor>
        [[nodiscard]]
        friend auto operator|(DFView const& v, RangeAdaptor&& adaptor)
        {
            return std::forward<RangeAdaptor>(adaptor)(std::ranges::subrange(v.cbegin(), v.cend()));
        }
    };

    static_assert(std::ranges::range<DFView<int, DFUniqueIndex<int>>>, "Validation for range requirement failed.");
    static_assert(std::ranges::range<DFView<int const, DFUniqueIndex<int>>>, "Validation for range requirement failed.");

    template<typename T, typename I>
    auto DFView<T, I>::Contains(KeyType const& key) -> bool
    {
        if(not dfIndex) return false;
        return dfIndex->Contains(key);
    }

    template <typename T, typename I>
    auto DFView<T, I>::At(KeyType const& key) noexcept -> T*
    {
        if(not dfIndex) return nullptr;

        auto posOpt = dfIndex->Position(key);
        if(!posOpt) return {};

        size_t const pos = posOpt.value();
        if(pos >= view.extent(0)) return nullptr;

        return &view[pos];
    }

    template<typename T, typename I>
    auto DFView<T, I>::At(KeyType const& key) const noexcept -> T const*
    {
        if(not dfIndex) return nullptr;

        auto posOpt = dfIndex->Position(key);
        if (!posOpt) return {};

        size_t const pos = posOpt.value();
        if (pos >= view.extent(0)) return nullptr;

        return &view[pos];
    }

    template<typename T, typename I>
    auto DFView<T, I>::TryAt(KeyType const& key) const noexcept -> std::optional<T>
    {
        auto* ref = At(key);
        return std::optional<T>(ref ? *ref : std::nullopt);
    }

    template<typename T, typename I>
    auto DFView<T, I>::Unwrap(KeyType const& key) noexcept -> RemovedOptional<T>* requires OptionalType<T>
    {
        auto* ref = At(key);
        if(not ref)              return nullptr;
        if(not ref->has_value()) return nullptr;

        return &(*ref).value();
    }

    template<typename T, typename I>
    auto DFView<T, I>::Unwrap(KeyType const& key) const noexcept -> RemovedOptional<T> const* requires OptionalType<T>
    {
        auto const* ref = At(key);
        if(not ref)              return nullptr;
        if(not ref->has_value()) return nullptr;

        return &(*ref).value();
    }

    template <typename T, typename I>
    auto DFView<T, I>::TryUnwrap(KeyType const &key) const noexcept -> std::optional<RemovedOptional<T>>
        requires OptionalType<T>
    {
        auto const *ref = Unwrap(key);
        if (not ref) return std::nullopt;

        return {*ref};
    }

    template<typename T, typename I>
    auto DFView<T, I>::Front() noexcept -> T*
    {
        if(Empty()) return nullptr;
        return &view[0];
    }

    template<typename T, typename I>
    auto DFView<T, I>::Front() const noexcept -> T const*
    {
        if(Empty()) return nullptr;
        return &view[0];
    }

    template<typename T, typename I>
    auto DFView<T, I>::Back() noexcept -> T*
    {
        if(Empty()) return nullptr;
        return &view[Size() - 1];
    }

    template<typename T, typename I>
    auto DFView<T, I>::Back() const noexcept -> T const*
    {
        if(Empty()) return nullptr;
        return &view[Size() - 1];
    }

} // namespace lugizmo

#endif // LUGIZMO_DF_VIEW_H
