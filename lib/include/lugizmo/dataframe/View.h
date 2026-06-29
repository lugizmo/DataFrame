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

#include "lugizmo/Assert.h"
#include "lugizmo/container/Concepts.h"
#include "lugizmo/container/OptionalRef.h"
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
        template<typename Ti, typename Ii>
        friend class DFViewIndexed;

        using Extents       = std::dextents<std::ptrdiff_t, 1>;
        using Strides       = std::layout_stride::mapping<Extents>;
        using IndexT        = I const*;
        using KeyType       = I::KeyType;
        using MDSpan        = std::mdspan<T, Extents, std::layout_stride>;
        using CopiedValue   = std::remove_const_t<T>;
        using UnwrappedType = std::conditional_t<std::is_const_v<T>, RemovedOptional<T> const, RemovedOptional<T>>;

        template<typename Layout>
        using MDSpanDF = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>;

        IndexT dfIndex;
        size_t indexOffset;
        MDSpan view;

        [[nodiscard]]
        auto LocalPosition(KeyType const& key) const noexcept -> std::optional<size_t>
        {
            if(dfIndex == nullptr) return std::nullopt;

            auto const position = dfIndex->Position(key);
            if(!position.has_value() || *position < indexOffset) return std::nullopt;

            auto const localPosition = *position - indexOffset;
            if(localPosition >= static_cast<size_t>(view.extent(0))) return std::nullopt;

            return localPosition;
        }

        /// record TODO doc
        ///        TODO test layout_left
        template<typename Layout>
        DFView(MDSpanDF<Layout> original, IndexT const originalIndex, size_t const index, bool const isFieldView) noexcept :
            dfIndex(originalIndex),
            indexOffset(0)
        {
            static_assert(std::is_same_v<Layout, std::layout_right> || std::is_same_v<Layout, std::layout_left>);

            using IIdx = Extents::index_type;
            if constexpr(std::is_same_v<Layout, std::layout_right>)
            {
                if(isFieldView)
                {
                    // fields: stride by number of columns
                    auto const stride = static_cast<IIdx>(original.extent(1));
                    auto mapping = Strides(Extents(static_cast<IIdx>(original.extent(0))), std::array{stride});
                    auto const off = static_cast<IIdx>(index);
                    view = MDSpan(original.data_handle() + off, mapping);
                }
                else
                {
                    auto mapping = Strides(Extents(static_cast<IIdx>(original.extent(1))), std::array{static_cast<IIdx>(1)});
                    auto const off = static_cast<IIdx>(index) * static_cast<IIdx>(original.extent(1));
                    view = MDSpan(original.data_handle() + off, mapping);
                }
            }
            else if constexpr(std::is_same_v<Layout, std::layout_left>)
            {
                if(isFieldView)
                {
                    auto mapping = Strides(Extents(static_cast<IIdx>(original.extent(0))), std::array{static_cast<IIdx>(1)});
                    auto const off = static_cast<IIdx>(index);
                    view = MDSpan(original.data_handle() + off, mapping);
                }
                else
                {
                    auto const stride = static_cast<IIdx>(original.extent(0));
                    auto mapping = Strides(Extents(static_cast<IIdx>(original.extent(1))), std::array{stride});
                    auto const off = static_cast<IIdx>(index) * stride;
                    view = MDSpan(original.data_handle() + off, mapping);
                }
            }
        }

        template<typename Layout>
        DFView(MDSpanDF<Layout> original, IndexT const originalIndex, size_t const index, size_t const begin, size_t const end, bool const isFieldView) noexcept :
            dfIndex(originalIndex),
            indexOffset(begin)
        {
            LUGIZMO_ASSERT_TRACE(begin <= end && end <= (isFieldView ? static_cast<std::size_t>(original.extent(0)) : static_cast<std::size_t>(original.extent(1))),
                            "DFView sub-range constructor received invalid begin/end bounds.");
            std::size_t const newExtentSZ = end - begin;

            using IIdx           = Extents::index_type;
            auto const newExtent = static_cast<IIdx>(newExtentSZ);

            if(isFieldView)
            {
                if constexpr(std::is_same_v<Layout, std::layout_right>)
                {
                    auto const stride = static_cast<IIdx>(original.extent(1));
                    auto mapping = Strides(Extents(newExtent), std::array{stride});

                    auto const off = static_cast<IIdx>(begin) * stride + static_cast<IIdx>(index);
                    view = MDSpan(original.data_handle() + off, mapping);
                }
                else if constexpr(std::is_same_v<Layout, std::layout_left>)
                {
                    auto mapping = Strides(Extents(newExtent), std::array{static_cast<IIdx>(1)});

                    auto const off = static_cast<IIdx>(index) * static_cast<IIdx>(original.extent(0)) + static_cast<IIdx>(begin);
                    view = MDSpan(original.data_handle() + off, mapping);
                }
            }
            else
            {
                if constexpr(std::is_same_v<Layout, std::layout_right>)
                {
                    auto mapping = Strides(Extents(newExtent), std::array{static_cast<IIdx>(1)});

                    auto const off = static_cast<IIdx>(index) * static_cast<IIdx>(original.extent(1)) + static_cast<IIdx>(begin);
                    view = MDSpan(original.data_handle() + off, mapping);
                }
                else if constexpr(std::is_same_v<Layout, std::layout_left>)
                {
                    auto const stride = static_cast<IIdx>(original.extent(0));
                    auto mapping = Strides(Extents(newExtent), std::array{stride});

                    auto const off = static_cast<IIdx>(begin) * stride + static_cast<IIdx>(index);
                    view = MDSpan(original.data_handle() + off, mapping);
                }
            }
        }

    public:

        class Iterator
        {
            T*             ptr;
            std::ptrdiff_t stride;

        public:

            // NOLINTBEGIN(readability-identifier-naming)
            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = std::remove_const_t<T>;
            using pointer           = T*;
            using reference         = T&;
            // NOLINTEND(readability-identifier-naming)

            constexpr Iterator() noexcept : ptr(nullptr), stride(0) {}
            constexpr Iterator(T* data, std::ptrdiff_t const stride) noexcept : ptr(data), stride(stride) {}

            constexpr Iterator(Iterator const& other) noexcept = default;
            constexpr Iterator(Iterator&& other) noexcept = default;
            constexpr auto operator=(Iterator const& other) noexcept -> Iterator& = default;
            constexpr auto operator=(Iterator&& other) noexcept -> Iterator& = default;
            constexpr ~Iterator() noexcept = default;

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

            constexpr auto operator==(Iterator const& other) const noexcept -> bool { return ptr == other.ptr; }
            constexpr auto operator!=(Iterator const& other) const noexcept -> bool { return ptr != other.ptr; }
            constexpr auto operator<(Iterator const& other)  const noexcept -> bool { return ptr < other.ptr; }
            constexpr auto operator<=(Iterator const& other) const noexcept -> bool { return ptr <= other.ptr; }
            constexpr auto operator>(Iterator const& other)  const noexcept -> bool { return ptr > other.ptr; }
            constexpr auto operator>=(Iterator const& other) const noexcept -> bool { return ptr >= other.ptr; }
        };

        using reverse_iterator = std::reverse_iterator<Iterator>; // NOLINT(readability-identifier-naming)

        static_assert(std::random_access_iterator<Iterator>, "Validation for iterator requirement failed.");
        static_assert(std::random_access_iterator<reverse_iterator>, "Validation for reverse iterator requirement failed.");

        // empty view
        DFView() noexcept :
            dfIndex(nullptr),
            indexOffset(0)
        {
            auto mapping = Strides(Extents(0), std::array<std::ptrdiff_t, 1>{1});
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

        [[nodiscard]] constexpr auto Size() const noexcept -> size_t { return view.extent(0) < 0 ? 0 : static_cast<size_t>(view.extent(0)); }
        [[nodiscard]] constexpr auto Empty() const noexcept -> bool  { return Size() == 0; }

        [[nodiscard]] constexpr auto operator[](size_t i) noexcept -> T& { return view[i]; }
        [[nodiscard]] constexpr auto operator[](size_t i) const noexcept -> T& { return view[i]; }

        [[nodiscard]]
        constexpr auto operator()(size_t i) noexcept -> OptionalRef<T>
        {
            return i < static_cast<size_t>(view.extent(0)) ? OptionalRef<T>(view[i]) : OptionalRef<T>();
        }

        [[nodiscard]]
        constexpr auto operator()(size_t i) const noexcept -> OptionalRef<T>
        {
            return i < static_cast<size_t>(view.extent(0)) ? OptionalRef<T>(view[i]) : OptionalRef<T>();
        }

        [[nodiscard]] auto Contains(KeyType const& key) const noexcept -> bool;

        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*;
        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T*;

        [[nodiscard]] auto TryAt(KeyType const& key) const noexcept -> std::optional<CopiedValue>;

        [[nodiscard]] auto Unwrap(KeyType const& key) noexcept -> UnwrappedType* requires OptionalType<T>;
        [[nodiscard]] auto Unwrap(KeyType const& key) const noexcept -> UnwrappedType* requires OptionalType<T>;
        [[nodiscard]] auto TryUnwrap(KeyType const& key) const noexcept -> std::optional<RemovedOptional<T>> requires OptionalType<T>;

        [[nodiscard]] auto Front() noexcept -> T*;
        [[nodiscard]] auto Front() const noexcept -> T*;
        [[nodiscard]] auto Back() noexcept -> T*;
        [[nodiscard]] auto Back() const noexcept -> T*;

        [[nodiscard]]
        constexpr auto begin() noexcept -> Iterator // NOLINT(readability-identifier-naming)
        {
            return Iterator(view.data_handle(), view.mapping().stride(0));
        }

        [[nodiscard]]
        constexpr auto end() noexcept -> Iterator // NOLINT(readability-identifier-naming)
        {
            return Iterator(view.data_handle() + static_cast<std::ptrdiff_t>(view.extent(0)) * view.mapping().stride(0), view.mapping().stride(0));
        }

        [[nodiscard]]
        constexpr auto begin() const noexcept -> Iterator // NOLINT(readability-identifier-naming)
        {
            return Iterator(view.data_handle(), view.mapping().stride(0));
        }

        [[nodiscard]]
        constexpr auto end() const noexcept -> Iterator // NOLINT(readability-identifier-naming)
        {
            return Iterator(view.data_handle() + static_cast<std::ptrdiff_t>(view.extent(0)) * view.mapping().stride(0), view.mapping().stride(0));
        }

        [[nodiscard]] constexpr auto cbegin() const noexcept -> Iterator { return begin(); } // NOLINT(readability-identifier-naming)
        [[nodiscard]] constexpr auto cend() const noexcept -> Iterator { return end(); }     // NOLINT(readability-identifier-naming)

        [[nodiscard]]
        constexpr auto rbegin() noexcept -> reverse_iterator // NOLINT(readability-identifier-naming)
        {
            return reverse_iterator(end());
        }

        [[nodiscard]]
        constexpr auto rend() noexcept -> reverse_iterator // NOLINT(readability-identifier-naming)
        {
            return reverse_iterator(begin());
        }

        [[nodiscard]]
        constexpr auto rbegin() const noexcept -> reverse_iterator // NOLINT(readability-identifier-naming)
        {
            return reverse_iterator(end());
        }

        [[nodiscard]]
        constexpr auto rend() const noexcept -> reverse_iterator // NOLINT(readability-identifier-naming)
        {
            return reverse_iterator(begin());
        }

        [[nodiscard]]
        constexpr auto crbegin() const noexcept -> reverse_iterator // NOLINT(readability-identifier-naming)
        {
            return reverse_iterator(cend());
        }

        [[nodiscard]]
        constexpr auto crend() const noexcept -> reverse_iterator // NOLINT(readability-identifier-naming)
        {
            return reverse_iterator(cbegin());
        }

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
    auto DFView<T, I>::Contains(KeyType const& key) const noexcept -> bool
    {
        return LocalPosition(key).has_value();
    }

    template <typename T, typename I>
    auto DFView<T, I>::At(KeyType const& key) noexcept -> T*
    {
        auto const position = LocalPosition(key);
        return position.has_value() ? &view[*position] : nullptr;
    }

    template<typename T, typename I>
    auto DFView<T, I>::At(KeyType const& key) const noexcept -> T*
    {
        auto const position = LocalPosition(key);
        return position.has_value() ? &view[*position] : nullptr;
    }

    template<typename T, typename I>
    auto DFView<T, I>::TryAt(KeyType const& key) const noexcept -> std::optional<CopiedValue>
    {
        auto* ref = At(key);
        return ref != nullptr ? std::optional<CopiedValue>(*ref) : std::nullopt;
    }

    template<typename T, typename I>
    auto DFView<T, I>::Unwrap(KeyType const& key) noexcept -> UnwrappedType* requires OptionalType<T>
    {
        auto* ref = At(key);
        if(ref == nullptr)       return nullptr;
        if(not ref->has_value()) return nullptr;

        return &(*ref).value();
    }

    template<typename T, typename I>
    auto DFView<T, I>::Unwrap(KeyType const& key) const noexcept -> UnwrappedType* requires OptionalType<T>
    {
        auto* ref = At(key);
        if(ref == nullptr)       return nullptr;
        if(not ref->has_value()) return nullptr;

        return &(*ref).value();
    }

    template <typename T, typename I>
    auto DFView<T, I>::TryUnwrap(KeyType const &key) const noexcept -> std::optional<RemovedOptional<T>>
        requires OptionalType<T>
    {
        auto const* ref = Unwrap(key);
        if(ref == nullptr) return std::nullopt;

        return {*ref};
    }

    template<typename T, typename I>
    auto DFView<T, I>::Front() noexcept -> T*
    {
        if(Empty()) return nullptr;
        return &view[0];
    }

    template<typename T, typename I>
    auto DFView<T, I>::Front() const noexcept -> T*
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
    auto DFView<T, I>::Back() const noexcept -> T*
    {
        if(Empty()) return nullptr;
        return &view[Size() - 1];
    }

} // namespace lugizmo

#endif // LUGIZMO_DF_VIEW_H
