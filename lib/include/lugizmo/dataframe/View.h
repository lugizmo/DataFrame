// Filename: View.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_VIEW_H
#define LUGIZMO_DF_VIEW_H

#include <array>
#include <cstddef>
#include <iterator>
#include <mdspan>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

#include "lugizmo/Assert.h"
#include "lugizmo/container/OptionalRef.h"
#include "IndexUnique.h"

namespace lugizmo {

    namespace internal {

        template<typename T, typename I>
        class DFViewIterator
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

            // ======== CONSTRUCTION ===============================================================================================================================================

            constexpr DFViewIterator() noexcept;
            constexpr DFViewIterator(T* data, std::ptrdiff_t stride) noexcept;

            constexpr DFViewIterator(DFViewIterator const& other) noexcept = default;
            constexpr DFViewIterator(DFViewIterator&& other) noexcept = default;
            constexpr auto operator=(DFViewIterator const& other) noexcept -> DFViewIterator& = default;
            constexpr auto operator=(DFViewIterator&& other) noexcept -> DFViewIterator& = default;
            constexpr ~DFViewIterator() noexcept = default;

            // ======== ACCESS =====================================================================================================================================================

            [[nodiscard]] constexpr auto operator*() const noexcept -> reference;
            [[nodiscard]] constexpr auto operator->() const noexcept -> pointer;
            [[nodiscard]] constexpr auto operator*() noexcept -> reference;
            [[nodiscard]] constexpr auto operator->() noexcept -> pointer;
            [[nodiscard]] constexpr auto operator[](difference_type n) const -> reference;

            // ======== MOVEMENT ===================================================================================================================================================

            constexpr auto operator++() -> DFViewIterator&;
            constexpr auto operator++(int) -> DFViewIterator;
            constexpr auto operator--() -> DFViewIterator&;
            constexpr auto operator--(int) -> DFViewIterator;
            [[nodiscard]] constexpr auto operator+(difference_type n) const -> DFViewIterator;
            constexpr auto operator+=(difference_type n) -> DFViewIterator&;
            [[nodiscard]] constexpr auto operator-(difference_type n) const -> DFViewIterator;
            [[nodiscard]] constexpr auto operator-(DFViewIterator const& other) const -> difference_type;
            constexpr auto operator-=(difference_type n) -> DFViewIterator&;

            // ======== COMPARISON =================================================================================================================================================

            [[nodiscard]] constexpr auto operator==(DFViewIterator const& other) const noexcept -> bool;
            [[nodiscard]] constexpr auto operator!=(DFViewIterator const& other) const noexcept -> bool;
            [[nodiscard]] constexpr auto operator<(DFViewIterator const& other) const noexcept -> bool;
            [[nodiscard]] constexpr auto operator<=(DFViewIterator const& other) const noexcept -> bool;
            [[nodiscard]] constexpr auto operator>(DFViewIterator const& other) const noexcept -> bool;
            [[nodiscard]] constexpr auto operator>=(DFViewIterator const& other) const noexcept -> bool;
        };

        template<typename T, typename I>
        [[nodiscard]] constexpr auto operator+(typename DFViewIterator<T, I>::difference_type n, DFViewIterator<T, I> const& iterator) -> DFViewIterator<T, I>;

    } // namespace internal

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

        using Extents = std::dextents<std::ptrdiff_t, 1>;
        using Strides = std::layout_stride::mapping<Extents>;
        using IndexT  = I const*;
        using KeyType = I::KeyType;
        using MDSpan  = std::mdspan<T, Extents, std::layout_stride>;

        template<typename Layout>
        using MDSpanDF = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>;

        IndexT dfIndex;
        size_t indexOffset;
        MDSpan view;

        [[nodiscard]] auto LocalPosition(KeyType const& key) const noexcept -> std::optional<size_t>;

        /// record TODO doc
        ///        TODO test layout_left
        template<typename Layout>
        DFView(MDSpanDF<Layout> original, IndexT originalIndex, size_t index, bool isFieldView) noexcept;

        template<typename Layout>
        DFView(MDSpanDF<Layout> original, IndexT originalIndex, size_t index, size_t begin, size_t end, bool isFieldView) noexcept;

    public:

        // ======== TYPES ==========================================================================================================================================================

        using Iterator         = internal::DFViewIterator<T, I>;
        using reverse_iterator = std::reverse_iterator<Iterator>; // NOLINT(readability-identifier-naming)

        static_assert(std::random_access_iterator<Iterator>, "Validation for iterator requirement failed.");
        static_assert(std::random_access_iterator<reverse_iterator>, "Validation for reverse iterator requirement failed.");

        // ======== CONSTRUCTION ===================================================================================================================================================

        DFView() noexcept;

        template<typename Layout>
        [[nodiscard]] static auto RecordView(MDSpanDF<Layout> original, IndexT dfIndex, size_t recIndex) noexcept -> DFView;

        template<typename Layout>
        [[nodiscard]] static auto FieldView(MDSpanDF<Layout> original, IndexT dfIndex, size_t fldIndex) noexcept -> DFView;

        template<typename Layout>
        [[nodiscard]] static auto RecordView(MDSpanDF<Layout> original, IndexT dfIndex, size_t recIndex, size_t fldBegin, size_t fldEnd) noexcept -> DFView;

        template<typename Layout>
        [[nodiscard]] static auto FieldView(MDSpanDF<Layout> original, IndexT dfIndex, size_t fldIndex, size_t recBegin, size_t recEnd) noexcept -> DFView;

        // ======== CAPACITY =======================================================================================================================================================

        [[nodiscard]] constexpr auto Size() const noexcept -> size_t;
        [[nodiscard]] constexpr auto Empty() const noexcept -> bool;

        // ======== POSITIONAL ACCESS ==============================================================================================================================================

        [[nodiscard]] constexpr auto operator[](size_t i) noexcept -> T&;
        [[nodiscard]] constexpr auto operator[](size_t i) const noexcept -> T&;
        [[nodiscard]] constexpr auto operator()(size_t i) noexcept -> OptionalRef<T>;
        [[nodiscard]] constexpr auto operator()(size_t i) const noexcept -> OptionalRef<T>;

        [[nodiscard]] auto Front() noexcept -> T*;
        [[nodiscard]] auto Front() const noexcept -> T*;
        [[nodiscard]] auto Back() noexcept -> T*;
        [[nodiscard]] auto Back() const noexcept -> T*;

        // ======== KEY ACCESS =====================================================================================================================================================

        [[nodiscard]] auto Contains(KeyType const& key) const noexcept -> bool;
        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*;
        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T*;

        // ======== ITERATORS ======================================================================================================================================================

        // NOLINTBEGIN(readability-identifier-naming)
        [[nodiscard]] constexpr auto begin() noexcept -> Iterator;
        [[nodiscard]] constexpr auto end() noexcept -> Iterator;
        [[nodiscard]] constexpr auto begin() const noexcept -> Iterator;
        [[nodiscard]] constexpr auto end() const noexcept -> Iterator;
        [[nodiscard]] constexpr auto cbegin() const noexcept -> Iterator;
        [[nodiscard]] constexpr auto cend() const noexcept -> Iterator;
        [[nodiscard]] constexpr auto rbegin() noexcept -> reverse_iterator;
        [[nodiscard]] constexpr auto rend() noexcept -> reverse_iterator;
        [[nodiscard]] constexpr auto rbegin() const noexcept -> reverse_iterator;
        [[nodiscard]] constexpr auto rend() const noexcept -> reverse_iterator;
        [[nodiscard]] constexpr auto crbegin() const noexcept -> reverse_iterator;
        [[nodiscard]] constexpr auto crend() const noexcept -> reverse_iterator;
        // NOLINTEND(readability-identifier-naming)
    };

    // ======== ITERATOR: CONSTRUCTION =============================================================================================================================================

    template<typename T, typename I>
    constexpr internal::DFViewIterator<T, I>::DFViewIterator() noexcept :
        ptr(nullptr),
        stride(0)
    {}

    template<typename T, typename I>
    constexpr internal::DFViewIterator<T, I>::DFViewIterator(T* const data, std::ptrdiff_t const stride) noexcept :
        ptr(data),
        stride(stride)
    {}

    // ======== ITERATOR: ACCESS ===================================================================================================================================================

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator*() const noexcept -> reference
    {
        return *ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator->() const noexcept -> pointer
    {
        return ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator*() noexcept -> reference
    {
        return *ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator->() noexcept -> pointer
    {
        return ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator[](difference_type const n) const -> reference
    {
        return *(ptr + n * stride);
    }

    // ======== ITERATOR: MOVEMENT =================================================================================================================================================

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator++() -> DFViewIterator&
    {
        ptr += stride;
        return *this;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator++(int) -> DFViewIterator
    {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator--() -> DFViewIterator&
    {
        ptr -= stride;
        return *this;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator--(int) -> DFViewIterator
    {
        auto copy = *this;
        --(*this);
        return copy;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator+(difference_type const n) const -> DFViewIterator
    {
        return DFViewIterator(ptr + n * stride, stride);
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator+=(difference_type const n) -> DFViewIterator&
    {
        ptr += n * stride;
        return *this;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator-(difference_type const n) const -> DFViewIterator
    {
        return DFViewIterator(ptr - n * stride, stride);
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator-(DFViewIterator const& other) const -> difference_type
    {
        return (ptr - other.ptr) / stride;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator-=(difference_type const n) -> DFViewIterator&
    {
        ptr -= n * stride;
        return *this;
    }

    template<typename T, typename I>
    constexpr auto internal::operator+(DFViewIterator<T, I>::difference_type const n, DFViewIterator<T, I> const& iterator) -> DFViewIterator<T, I>
    {
        return iterator + n;
    }

    // ======== ITERATOR: COMPARISON ===============================================================================================================================================

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator==(DFViewIterator const& other) const noexcept -> bool
    {
        return ptr == other.ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator!=(DFViewIterator const& other) const noexcept -> bool
    {
        return ptr != other.ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator<(DFViewIterator const& other) const noexcept -> bool
    {
        return ptr < other.ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator<=(DFViewIterator const& other) const noexcept -> bool
    {
        return ptr <= other.ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator>(DFViewIterator const& other) const noexcept -> bool
    {
        return ptr > other.ptr;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator>=(DFViewIterator const& other) const noexcept -> bool
    {
        return ptr >= other.ptr;
    }

    // ======== CONSTRUCTION =======================================================================================================================================================

    template<typename T, typename I>
    template<typename Layout>
    DFView<T, I>::DFView(MDSpanDF<Layout> original, IndexT const originalIndex, size_t const index, bool const isFieldView) noexcept :
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

    template<typename T, typename I>
    template<typename Layout>
    DFView<T, I>::DFView(MDSpanDF<Layout> original, IndexT const originalIndex, size_t const index, size_t const begin, size_t const end, bool const isFieldView) noexcept :
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

    template<typename T, typename I>
    DFView<T, I>::DFView() noexcept :
        dfIndex(nullptr),
        indexOffset(0)
    {
        auto mapping = Strides(Extents(0), std::array<std::ptrdiff_t, 1>{1});
        view = MDSpan(nullptr, std::move(mapping));
    }

    template<typename T, typename I>
    template<typename Layout>
    auto DFView<T, I>::RecordView(MDSpanDF<Layout> original, IndexT const dfIndex, size_t const recIndex) noexcept -> DFView
    {
        return DFView(original, dfIndex, recIndex, false);
    }

    template<typename T, typename I>
    template<typename Layout>
    auto DFView<T, I>::FieldView(MDSpanDF<Layout> original, IndexT const dfIndex, size_t const fldIndex) noexcept -> DFView
    {
        return DFView(original, dfIndex, fldIndex, true);
    }

    template<typename T, typename I>
    template<typename Layout>
    auto DFView<T, I>::RecordView(MDSpanDF<Layout> original, IndexT const dfIndex, size_t const recIndex, size_t const fldBegin, size_t const fldEnd) noexcept -> DFView
    {
        return DFView(original, dfIndex, recIndex, fldBegin, fldEnd, false);
    }

    template<typename T, typename I>
    template<typename Layout>
    auto DFView<T, I>::FieldView(MDSpanDF<Layout> original, IndexT const dfIndex, size_t const fldIndex, size_t const recBegin, size_t const recEnd) noexcept -> DFView
    {
        return DFView(original, dfIndex, fldIndex, recBegin, recEnd, true);
    }

    // ======== CAPACITY ===========================================================================================================================================================

    template<typename T, typename I>
    constexpr auto DFView<T, I>::Size() const noexcept -> size_t
    {
        return view.extent(0) < 0 ? 0 : static_cast<size_t>(view.extent(0));
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::Empty() const noexcept -> bool
    {
        return Size() == 0;
    }

    // ======== POSITIONAL ACCESS ==================================================================================================================================================

    template<typename T, typename I>
    constexpr auto DFView<T, I>::operator[](size_t const i) noexcept -> T&
    {
        return view[i];
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::operator[](size_t const i) const noexcept -> T&
    {
        return view[i];
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::operator()(size_t const i) noexcept -> OptionalRef<T>
    {
        return i < static_cast<size_t>(view.extent(0)) ? OptionalRef<T>(view[i]) : OptionalRef<T>();
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::operator()(size_t const i) const noexcept -> OptionalRef<T>
    {
        return i < static_cast<size_t>(view.extent(0)) ? OptionalRef<T>(view[i]) : OptionalRef<T>();
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

    // ======== KEY ACCESS =========================================================================================================================================================

    template<typename T, typename I>
    auto DFView<T, I>::LocalPosition(KeyType const& key) const noexcept -> std::optional<size_t>
    {
        if(dfIndex == nullptr) return std::nullopt;

        auto const position = dfIndex->Position(key);
        if(!position.has_value() || *position < indexOffset) return std::nullopt;

        auto const localPosition = *position - indexOffset;
        if(localPosition >= static_cast<size_t>(view.extent(0))) return std::nullopt;

        return localPosition;
    }

    template<typename T, typename I>
    auto DFView<T, I>::Contains(KeyType const& key) const noexcept -> bool
    {
        return LocalPosition(key).has_value();
    }

    template<typename T, typename I>
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

    // ======== ITERATORS ==========================================================================================================================================================
    // NOLINTBEGIN(readability-identifier-naming)

    template<typename T, typename I>
    constexpr auto DFView<T, I>::begin() noexcept -> Iterator
    {
        return Iterator(view.data_handle(), view.mapping().stride(0));
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::end() noexcept -> Iterator
    {
        return Iterator(view.data_handle() + static_cast<std::ptrdiff_t>(view.extent(0)) * view.mapping().stride(0), view.mapping().stride(0));
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::begin() const noexcept -> Iterator
    {
        return Iterator(view.data_handle(), view.mapping().stride(0));
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::end() const noexcept -> Iterator
    {
        return Iterator(view.data_handle() + static_cast<std::ptrdiff_t>(view.extent(0)) * view.mapping().stride(0), view.mapping().stride(0));
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::cbegin() const noexcept -> Iterator
    {
        return begin();
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::cend() const noexcept -> Iterator
    {
        return end();
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::rbegin() noexcept -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::rend() noexcept -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::rbegin() const noexcept -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::rend() const noexcept -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::crbegin() const noexcept -> reverse_iterator
    {
        return reverse_iterator(cend());
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::crend() const noexcept -> reverse_iterator
    {
        return reverse_iterator(cbegin());
    }

    // NOLINTEND(readability-identifier-naming)
    // ======== RANGE ADAPTORS =====================================================================================================================================================

    template<typename T, typename I, typename RangeAdaptor>
    [[nodiscard]] auto operator|(DFView<T, I>& view, RangeAdaptor&& adaptor)
    {
        return std::forward<RangeAdaptor>(adaptor)(std::ranges::subrange(view.begin(), view.end()));
    }

    template<typename T, typename I, typename RangeAdaptor>
    [[nodiscard]] auto operator|(DFView<T, I> const& view, RangeAdaptor&& adaptor)
    {
        return std::forward<RangeAdaptor>(adaptor)(std::ranges::subrange(view.cbegin(), view.cend()));
    }

    static_assert(std::ranges::range<DFView<int, DFUniqueIndex<int>>>, "Validation for range requirement failed.");
    static_assert(std::ranges::range<DFView<int const, DFUniqueIndex<int>>>, "Validation for range requirement failed.");

} // namespace lugizmo

#endif // LUGIZMO_DF_VIEW_H
