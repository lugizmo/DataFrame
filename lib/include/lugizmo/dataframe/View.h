// Filename: View.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_VIEW_H
#define LUGIZMO_DF_VIEW_H

#include <mdspan>
#include <cstddef>
#include <array>
#include <optional>

namespace lugizmo {

    /**
     * TODO doc
     * TODO make view not eagerly create view so that it can be used
     *      lazy with ranges.
     * @tparam T
     */
    template <typename T>
    class DFView
    {
        static constexpr bool IsConstView = std::is_const_v<T>;

        template<typename Ti, typename Ii>
        friend class DFViewIndexed;

        using Extents = std::dextents<size_t, 1>;
        using Strides = std::layout_stride::mapping<Extents>;
        using MDSpan  = std::mdspan<T, Extents, std::layout_stride>;

        template<typename Layout>
        using MDSpanDF = std::mdspan<T, std::dextents<size_t, 2>, Layout>;

        MDSpan view;

        /// record TODO doc
        ///        TODO test layout_left
        template<typename Layout>
        DFView(MDSpanDF<Layout> original, size_t const index, bool const isFieldView) noexcept
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

            Iterator() noexcept : ptr(nullptr), stride(0) {}
            Iterator(T* data, size_t const stride) noexcept : ptr(data), stride(stride) {}

            Iterator(Iterator const& other) noexcept
            {
                if (this != &other)
                {
                    ptr = other.ptr;
                    stride = other.stride;
                }
            }

            auto operator=(Iterator const& other) -> Iterator&
            {
                if (this != &other)
                {
                    ptr = other.ptr;
                    stride = other.stride;
                }

                return *this;
            }

            auto operator*() const noexcept -> reference { return *ptr; }
            auto operator->() const noexcept -> pointer  { return ptr; }

            auto operator*() noexcept -> reference { return *ptr; }
            auto operator->() noexcept -> pointer  { return ptr; }

            auto operator++() -> Iterator&
            {
                ptr += stride;
                return *this;
            }

            auto operator++(int) -> Iterator
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            auto operator--() -> Iterator&
            {
                ptr -= stride;
                return *this;
            }

            auto operator--(int) -> Iterator
            {
                Iterator tmp = *this;
                --(*this);
                return tmp;
            }

            auto operator+(difference_type const n) const -> Iterator
            {
                return Iterator(ptr + n * stride, stride);
            }

            auto operator+=(difference_type const n) -> Iterator&
            {
                ptr += n * stride;
                return *this;
            }

            auto operator-(difference_type const n) const -> Iterator
            {
                return Iterator(ptr - n * stride, stride);
            }

            auto operator-(Iterator const& other) const -> difference_type
            {
                return (ptr - other.ptr) / stride;
            }

            auto operator-=(difference_type const n) -> Iterator&
            {
                ptr -= n * stride;
                return *this;
            }

            auto operator[](difference_type const n) const -> reference
            {
                return *(ptr + n * stride);
            }

            friend auto operator+(difference_type n, const Iterator& it) -> Iterator
            {
                return it + n;
            }

            bool operator==(Iterator const& other) const noexcept { return ptr == other.ptr; }
            bool operator!=(Iterator const& other) const noexcept { return ptr != other.ptr; }

            bool operator<(Iterator const& other)  const noexcept { return ptr < other.ptr; }
            bool operator<=(Iterator const& other) const noexcept { return ptr <= other.ptr; }
            bool operator>(Iterator const& other)  const noexcept { return ptr > other.ptr; }
            bool operator>=(Iterator const& other) const noexcept { return ptr >= other.ptr; }
        };

        static_assert(std::random_access_iterator<Iterator>, "Validation for iterator requirement failed.");

        // empty view
        DFView() noexcept
        {
            auto mapping = Strides(Extents(0), std::array{size_t{0}});
            view = MDSpan(nullptr, std::move(mapping));
        }

        template<typename Layout>
        [[nodiscard]]
        static auto RecordView(MDSpanDF<Layout> original, size_t const recIndex) noexcept -> DFView
        {
            return DFView(original, recIndex, false);
        }

        template<typename Layout>
        [[nodiscard]]
        static auto FieldView(MDSpanDF<Layout> original, size_t const fldIndex) noexcept -> DFView
        {
            return DFView(original, fldIndex, true);
        }

        [[nodiscard]] auto Size() const noexcept -> size_t { return view.extent(0); }
        [[nodiscard]] auto Empty() const noexcept -> bool  { return Size() == 0; }

        [[nodiscard]] auto operator[](size_t const i) noexcept -> T& { return view[i]; }
        [[nodiscard]] auto operator[](size_t const i) const noexcept -> const T& { return view[i]; }

        [[nodiscard]] auto operator()(size_t const i) noexcept -> std::optional<T*> { return i < Size() ? &view[i] : std::nullopt; }
        [[nodiscard]] auto operator()(size_t const i) const noexcept -> std::optional<T const*> { return  i <Size() ? &view[i] : std::nullopt; }

        [[nodiscard]]
        auto begin() noexcept -> Iterator
        {
            return Iterator(view.data_handle(), view.mapping().stride(0));
        }

        [[nodiscard]]
        auto end() noexcept -> Iterator
        {
            return Iterator(view.data_handle() + view.extent(0) * view.mapping().stride(0), view.mapping().stride(0));
        }

        [[nodiscard]]
        auto begin() const noexcept -> Iterator
        {
            return Iterator(view.data_handle(), view.mapping().stride(0));
        }

        [[nodiscard]]
        auto end() const noexcept -> Iterator
        {
            return Iterator(view.data_handle() + view.extent(0) * view.mapping().stride(0), view.mapping().stride(0));
        }

        [[nodiscard]]
        auto cbegin() const noexcept -> Iterator
        {
            return Iterator(view.data_handle(), view.mapping().stride(0));
        }

        [[nodiscard]]
        auto cend() const noexcept -> Iterator
        {
            return Iterator(view.data_handle(), view.mapping().stride(0));
        }

        template <typename RangeAdaptor>
        [[nodiscard]]
        friend auto operator|(DFView& view, RangeAdaptor&& adaptor)
        {
            return std::forward<RangeAdaptor>(adaptor)(std::ranges::subrange(view.begin(), view.end()));
        }

        template <typename RangeAdaptor>
        [[nodiscard]]
        friend auto operator|(DFView const& view, RangeAdaptor&& adaptor)
        {
            return std::forward<RangeAdaptor>(adaptor)(std::ranges::subrange(view.cbegin(), view.cend()));
        }
    };

    static_assert(std::ranges::range<DFView<int>>, "Validation for range requirement failed.");
    static_assert(std::ranges::range<DFView<int const>>, "Validation for range requirement failed.");
}

#endif // LUGIZMO_DF_VIEW_H
