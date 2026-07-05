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
#include "IndexUnique.h"

namespace lugizmo {

    namespace internal {

        /**
         * @brief Random-access iterator over a potentially strided `DFView`.
         *
         * The iterator retains the view's base address and stride together with a
         * logical position. Consequently, an end iterator does not require forming
         * a pointer beyond the storage backing a strided view.
         *
         * Dereferencing requires a position in the half-open range `[begin, end)`.
         * Ordering, arithmetic between two iterators, and iterator difference require
         * both iterators to originate from the same view.
         *
         * @tparam T Element type, including its const qualification.
         * @tparam I Index type used to keep iterators from unrelated `DFView`
         *           specializations type-distinct.
         */
        template<typename T, typename I>
        class DFViewIterator
        {
            T*             data;
            std::ptrdiff_t stride;
            std::ptrdiff_t position;

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
            constexpr DFViewIterator(T* dataPtr, std::ptrdiff_t stride, std::ptrdiff_t position = 0) noexcept;

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
     * @brief Non-owning one-dimensional view over one field or record of a dataframe.
     *
     * `DFView` presents the selected values as a random-access range. Depending on
     * the dataframe layout and selected axis, consecutive logical values may be
     * separated by a stride in the underlying storage. Positional access and
     * iteration hide that stride.
     *
     * @par Element constness
     * Element mutability is determined by `T`, in the same way as for `std::span`.
     * A const `DFView<T, I>` object still provides `T&` when `T` is mutable. A
     * `DFView<T const, I>` provides read-only access.
     *
     * @par Ownership, lifetime, and invalidation
     * The view owns neither the values nor the index used for key lookup. Both must
     * outlive the view. The values must also outlive every iterator, pointer, or
     * reference obtained from it; iterators do not retain or use the index. Copying
     * a view is cheap and does not extend either lifetime.
     *
     * A view produced by `DataFrame` is invalidated by destruction or movement of
     * that dataframe and by any structural operation that changes its fields,
     * records, storage, or ordering. Value-only updates do not invalidate the view.
     * After invalidation, using the view or anything obtained from it is undefined.
     *
     * @par Index contract
     * The stored index describes the keys along the viewed axis: record keys for a
     * field view and field keys for a record view. It must continue to describe the
     * same logical positions for the lifetime of the view. A null index disables
     * keyed lookup but does not affect positional access or iteration.
     *
     * @par Standard range integration
     * `DFView` is a random-access, sized, common view and a borrowed range. Standard
     * adaptor closures can therefore be applied directly with `operator|`. Iterators
     * may outlive the `DFView` wrapper, but never the underlying values.
     *
     * @tparam T Element type, including its const qualification.
     * @tparam I Index type whose `KeyType` and `Position` operation provide keyed
     *           lookup along the viewed axis.
     */
    template<typename T, typename I>
    class DFView : public std::ranges::view_interface<DFView<T, I>>
    {
        template<typename Ti, typename Ii, typename Vi>
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

        /// @brief Constructs a full field or record view after its axis position was resolved.
        template<typename Layout>
        DFView(MDSpanDF<Layout> original, IndexT originalIndex, size_t index, bool isFieldView) noexcept;

        /// @brief Constructs a half-open positional subrange after its axis position was resolved.
        template<typename Layout>
        DFView(MDSpanDF<Layout> original, IndexT originalIndex, size_t index, size_t begin, size_t end, bool isFieldView) noexcept;

    public:

        // ======== TYPES ==========================================================================================================================================================

        /// @brief Random-access iterator whose reference type follows the const qualification of `T`.
        using Iterator = internal::DFViewIterator<T, I>;

        /// @brief Reverse iterator over the same strided sequence.
        using reverse_iterator = std::reverse_iterator<Iterator>; // NOLINT(readability-identifier-naming)

        static_assert(std::random_access_iterator<Iterator>, "Validation for iterator requirement failed.");
        static_assert(std::random_access_iterator<reverse_iterator>, "Validation for reverse iterator requirement failed.");

        // ======== CONSTRUCTION ===================================================================================================================================================

        /**
         * @brief Constructs an empty view with no index.
         * @post `Empty()` is true and `begin() == end()`.
         */
        DFView() noexcept;

        /**
         * @brief Creates a view over all fields of one record.
         *
         * @param original Original matrix whose first extent is records and the second extent is fields.
         * @param dfIndex  Optional pointer to the field index used by `Contains` and `At`.
         * @param recIndex Zero-based position of the selected record.
         *
         * @pre `recIndex < original.extent(0)`.
         *
         * @return Non-owning view containing `original.extent(1)` values.
         */
        template<typename Layout>
        [[nodiscard]] static auto RecordView(MDSpanDF<Layout> original, IndexT dfIndex, size_t recIndex) noexcept -> DFView;

        /**
         * @brief Creates a view over all records of one field.
         *
         * @param original Original matrix whose first extent is records and the second extent is fields.
         * @param dfIndex  Optional pointer to the record index used by `Contains` and `At`.
         * @param fldIndex Zero-based position of the selected field.
         *
         * @pre `fldIndex < original.extent(1)`.
         *
         * @return Non-owning view containing `original.extent(0)` values.
         */
        template<typename Layout>
        [[nodiscard]] static auto FieldView(MDSpanDF<Layout> original, IndexT dfIndex, size_t fldIndex) noexcept -> DFView;

        /**
         * @brief Creates a view over a half-open field range of one record.
         *
         * @param original Original matrix whose first extent is records and the second extent is fields.
         * @param dfIndex  Optional pointer to the full field index used by `Contains` and `At`.
         * @param recIndex Zero-based position of the selected record.
         * @param fldBegin First included field position.
         * @param fldEnd   One-past-last included field position.
         *
         * @pre `recIndex < original.extent(0)`.
         * @pre `fldBegin <= fldEnd <= original.extent(1)`.
         *
         * @return Non-owning view containing `fldEnd - fldBegin` values.
         */
        template<typename Layout>
        [[nodiscard]] static auto RecordView(MDSpanDF<Layout> original, IndexT dfIndex, size_t recIndex, size_t fldBegin, size_t fldEnd) noexcept -> DFView;

        /**
         * @brief Creates a view over a half-open record range of one field.
         *
         * @param original Original matrix whose first extent is records and the second extent is fields.
         * @param dfIndex  Optional pointer to the full record index used by `Contains` and `At`.
         * @param fldIndex Zero-based position of the selected field.
         * @param recBegin First included record position.
         * @param recEnd   One-past-last included record position.
         *
         * @pre `fldIndex < original.extent(1)`.
         * @pre `recBegin <= recEnd <= original.extent(0)`.
         *
         * @return Non-owning view containing `recEnd - recBegin` values.
         */
        template<typename Layout>
        [[nodiscard]] static auto FieldView(MDSpanDF<Layout> original, IndexT dfIndex, size_t fldIndex, size_t recBegin, size_t recEnd) noexcept -> DFView;

        // ======== CAPACITY =======================================================================================================================================================

        /// @return Number of logical values in the view.
        [[nodiscard]] constexpr auto Size() const noexcept -> size_t;

        /// @return True when the view contains no logical values.
        [[nodiscard]] constexpr auto Empty() const noexcept -> bool;

        // ======== POSITIONAL ACCESS ==============================================================================================================================================

        /// @brief Returns the value at a zero-based logical position without bound checking.
        /// @pre `i < Size()`.
        [[nodiscard]] constexpr auto operator[](size_t i) noexcept -> T&;

        /// @copydoc operator[](size_t)
        [[nodiscard]] constexpr auto operator[](size_t i) const noexcept -> T&;

        /// @return Pointer to the value at `i`, or null when `i >= Size()`.
        [[nodiscard]] constexpr auto operator()(size_t i) noexcept -> T*;

        /// @copydoc operator()(size_t)
        [[nodiscard]] constexpr auto operator()(size_t i) const noexcept -> T*;

        /// @return Pointer to the first value, or null when the view is empty.
        [[nodiscard]] auto Front() noexcept -> T*;

        /// @copydoc Front()
        [[nodiscard]] auto Front() const noexcept -> T*;

        /// @return Pointer to the final value or null when the view is empty.
        [[nodiscard]] auto Back() noexcept -> T*;

        /// @copydoc Back()
        [[nodiscard]] auto Back() const noexcept -> T*;

        // ======== KEY ACCESS =====================================================================================================================================================

        /**
         * @brief  Checks whether a key belongs to this view.
         * @return True only when the index contains `key` and its position lies in
         *         this view's positional range. Returns false when no index is attached.
         */
        [[nodiscard]] auto Contains(KeyType const& key) const noexcept -> bool;

        /**
         * @brief  It looks up a value by its key without copying it.
         * @return Pointer to the corresponding value, or null when the key is absent,
         *         outside a subrange, or no index is attached.
         */
        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*;

        /// @copydoc At(KeyType const&)
        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T*;

        // ======== ITERATORS ======================================================================================================================================================

        // NOLINTBEGIN(readability-identifier-naming)
        /// @return Iterator to the first logical value.
        [[nodiscard]] constexpr auto begin() noexcept -> Iterator;

        /// @return Iterator one past the final logical value.
        [[nodiscard]] constexpr auto end() noexcept -> Iterator;

        /// @copydoc begin()
        [[nodiscard]] constexpr auto begin() const noexcept -> Iterator;

        /// @copydoc end()
        [[nodiscard]] constexpr auto end() const noexcept -> Iterator;

        /// @return Iterator to the first value; element mutability still follows `T`.
        [[nodiscard]] constexpr auto cbegin() const noexcept -> Iterator;

        /// @return Iterator one past the final value; element mutability still follows `T`.
        [[nodiscard]] constexpr auto cend() const noexcept -> Iterator;

        /// @return Reverse iterator to the final logical value.
        [[nodiscard]] constexpr auto rbegin() noexcept -> reverse_iterator;

        /// @return Reverse iterator one past the first logical value.
        [[nodiscard]] constexpr auto rend() noexcept -> reverse_iterator;

        /// @copydoc rbegin()
        [[nodiscard]] constexpr auto rbegin() const noexcept -> reverse_iterator;

        /// @copydoc rend()
        [[nodiscard]] constexpr auto rend() const noexcept -> reverse_iterator;

        /// @return Reverse iterator to the final value; element mutability still follows `T`.
        [[nodiscard]] constexpr auto crbegin() const noexcept -> reverse_iterator;

        /// @return Reverse iterator one past the first value; element mutability still follows `T`.
        [[nodiscard]] constexpr auto crend() const noexcept -> reverse_iterator;
        // NOLINTEND(readability-identifier-naming)
    };

    // ======== ITERATOR: CONSTRUCTION =============================================================================================================================================

    template<typename T, typename I>
    constexpr internal::DFViewIterator<T, I>::DFViewIterator() noexcept :
        data(nullptr),
        stride(0),
        position(0)
    {}

    template<typename T, typename I>
    constexpr internal::DFViewIterator<T, I>::DFViewIterator(T* const dataPtr, std::ptrdiff_t const stride, std::ptrdiff_t const position) noexcept :
        data(dataPtr),
        stride(stride),
        position(position)
    {}

    // ======== ITERATOR: ACCESS ===================================================================================================================================================

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator*() const noexcept -> reference
    {
        return *(data + position * stride);
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator->() const noexcept -> pointer
    {
        return data + position * stride;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator*() noexcept -> reference
    {
        return *(data + position * stride);
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator->() noexcept -> pointer
    {
        return data + position * stride;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator[](difference_type const n) const -> reference
    {
        return *(data + (position + n) * stride);
    }

    // ======== ITERATOR: MOVEMENT =================================================================================================================================================

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator++() -> DFViewIterator&
    {
        ++position;
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
        --position;
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
        return DFViewIterator(data, stride, position + n);
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator+=(difference_type const n) -> DFViewIterator&
    {
        position += n;
        return *this;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator-(difference_type const n) const -> DFViewIterator
    {
        return DFViewIterator(data, stride, position - n);
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator-(DFViewIterator const& other) const -> difference_type
    {
        return position - other.position;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator-=(difference_type const n) -> DFViewIterator&
    {
        position -= n;
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
        return data == other.data && position == other.position;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator!=(DFViewIterator const& other) const noexcept -> bool
    {
        return !(*this == other);
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator<(DFViewIterator const& other) const noexcept -> bool
    {
        return position < other.position;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator<=(DFViewIterator const& other) const noexcept -> bool
    {
        return position <= other.position;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator>(DFViewIterator const& other) const noexcept -> bool
    {
        return position > other.position;
    }

    template<typename T, typename I>
    constexpr auto internal::DFViewIterator<T, I>::operator>=(DFViewIterator const& other) const noexcept -> bool
    {
        return position >= other.position;
    }

    // ======== CONSTRUCTION =======================================================================================================================================================

    template<typename T, typename I>
    template<typename Layout>
    DFView<T, I>::DFView(MDSpanDF<Layout> original, IndexT const originalIndex, size_t const index, bool const isFieldView) noexcept :
        dfIndex(originalIndex),
        indexOffset(0)
    {
        static_assert(std::is_same_v<Layout, std::layout_right> || std::is_same_v<Layout, std::layout_left>);
        LUGIZMO_ASSERT_TRACE(index < (isFieldView ? static_cast<size_t>(original.extent(1)) : static_cast<size_t>(original.extent(0))),
                            "DFView constructor received an out-of-bounds field or record position.");

        using IIdx = Extents::index_type;
        auto const dataAt = [dataPtr = original.data_handle()](IIdx const offset) -> T* {
            return dataPtr == nullptr ? nullptr : dataPtr + offset;
        };

        if constexpr(std::is_same_v<Layout, std::layout_right>)
        {
            if(isFieldView)
            {
                // fields: stride by number of columns
                auto const stride = static_cast<IIdx>(original.extent(1));
                auto mapping = Strides(Extents(static_cast<IIdx>(original.extent(0))), std::array{stride});
                auto const off = static_cast<IIdx>(index);
                view = MDSpan(dataAt(off), mapping);
            }
            else
            {
                auto mapping = Strides(Extents(static_cast<IIdx>(original.extent(1))), std::array{static_cast<IIdx>(1)});
                auto const off = static_cast<IIdx>(index) * static_cast<IIdx>(original.extent(1));
                view = MDSpan(dataAt(off), mapping);
            }
        }
        else if constexpr(std::is_same_v<Layout, std::layout_left>)
        {
            if(isFieldView)
            {
                auto mapping = Strides(Extents(static_cast<IIdx>(original.extent(0))), std::array{static_cast<IIdx>(1)});
                auto const off = static_cast<IIdx>(index);
                view = MDSpan(dataAt(off), mapping);
            }
            else
            {
                auto const stride = static_cast<IIdx>(original.extent(0));
                auto mapping = Strides(Extents(static_cast<IIdx>(original.extent(1))), std::array{stride});
                auto const off = static_cast<IIdx>(index) * stride;
                view = MDSpan(dataAt(off), mapping);
            }
        }
    }

    template<typename T, typename I>
    template<typename Layout>
    DFView<T, I>::DFView(MDSpanDF<Layout> original, IndexT const originalIndex, size_t const index, size_t const begin, size_t const end, bool const isFieldView) noexcept :
        dfIndex(originalIndex),
        indexOffset(begin)
    {
        LUGIZMO_ASSERT_TRACE(index < (isFieldView ? static_cast<size_t>(original.extent(1)) : static_cast<size_t>(original.extent(0))),
                            "DFView sub-range constructor received an out-of-bounds field or record position.");
        LUGIZMO_ASSERT_TRACE(begin <= end && end <= (isFieldView ? static_cast<std::size_t>(original.extent(0)) : static_cast<std::size_t>(original.extent(1))),
                            "DFView sub-range constructor received invalid begin/end bounds.");
        std::size_t const newExtentSZ = end - begin;

        using IIdx           = Extents::index_type;
        auto const newExtent = static_cast<IIdx>(newExtentSZ);
        auto const dataAt = [dataPtr = original.data_handle()](IIdx const offset) -> T* {
            return dataPtr == nullptr ? nullptr : dataPtr + offset;
        };

        if(isFieldView)
        {
            if constexpr(std::is_same_v<Layout, std::layout_right>)
            {
                auto const stride = static_cast<IIdx>(original.extent(1));
                auto mapping = Strides(Extents(newExtent), std::array{stride});

                auto const off = static_cast<IIdx>(begin) * stride + static_cast<IIdx>(index);
                view = MDSpan(dataAt(off), mapping);
            }
            else if constexpr(std::is_same_v<Layout, std::layout_left>)
            {
                auto mapping = Strides(Extents(newExtent), std::array{static_cast<IIdx>(1)});

                auto const off = static_cast<IIdx>(index) * static_cast<IIdx>(original.extent(0)) + static_cast<IIdx>(begin);
                view = MDSpan(dataAt(off), mapping);
            }
        }
        else
        {
            if constexpr(std::is_same_v<Layout, std::layout_right>)
            {
                auto mapping = Strides(Extents(newExtent), std::array{static_cast<IIdx>(1)});

                auto const off = static_cast<IIdx>(index) * static_cast<IIdx>(original.extent(1)) + static_cast<IIdx>(begin);
                view = MDSpan(dataAt(off), mapping);
            }
            else if constexpr(std::is_same_v<Layout, std::layout_left>)
            {
                auto const stride = static_cast<IIdx>(original.extent(0));
                auto mapping = Strides(Extents(newExtent), std::array{stride});

                auto const off = static_cast<IIdx>(begin) * stride + static_cast<IIdx>(index);
                view = MDSpan(dataAt(off), mapping);
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
    constexpr auto DFView<T, I>::operator()(size_t const i) noexcept -> T*
    {
        return i < Size() ? std::addressof(view[i]) : nullptr;
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::operator()(size_t const i) const noexcept -> T*
    {
        return i < Size() ? std::addressof(view[i]) : nullptr;
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
        return Iterator(view.data_handle(), view.mapping().stride(0), static_cast<std::ptrdiff_t>(view.extent(0)));
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::begin() const noexcept -> Iterator
    {
        return Iterator(view.data_handle(), view.mapping().stride(0));
    }

    template<typename T, typename I>
    constexpr auto DFView<T, I>::end() const noexcept -> Iterator
    {
        return Iterator(view.data_handle(), view.mapping().stride(0), static_cast<std::ptrdiff_t>(view.extent(0)));
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

} // namespace lugizmo

// ======== STANDARD RANGE CUSTOMIZATION ===========================================================================================================================================

template<typename T, typename I>
inline constexpr bool std::ranges::enable_borrowed_range<lugizmo::DFView<T, I>> = true; // NOLINT(readability-identifier-naming)

static_assert(std::ranges::random_access_range<lugizmo::DFView<int, lugizmo::DFUniqueIndex<int>>>, "DFView must model random_access_range.");
static_assert(std::ranges::random_access_range<lugizmo::DFView<int const, lugizmo::DFUniqueIndex<int>>>, "Read-only DFView must model random_access_range.");
static_assert(std::ranges::sized_range<lugizmo::DFView<int, lugizmo::DFUniqueIndex<int>>>, "DFView must model sized_range.");
static_assert(std::ranges::common_range<lugizmo::DFView<int, lugizmo::DFUniqueIndex<int>>>, "DFView must model common_range.");
static_assert(std::ranges::view<lugizmo::DFView<int, lugizmo::DFUniqueIndex<int>>>, "DFView must model view.");
static_assert(std::ranges::borrowed_range<lugizmo::DFView<int, lugizmo::DFUniqueIndex<int>>>, "DFView must model borrowed_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFView<int, lugizmo::DFUniqueIndex<int>>&>, "DFView lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFView<int, lugizmo::DFUniqueIndex<int>>>, "Temporary DFView objects must model viewable_range.");

static_assert(std::ranges::random_access_range<lugizmo::DFView<int, lugizmo::DFRangeIndex<int>>>, "DFView must model random_access_range.");
static_assert(std::ranges::random_access_range<lugizmo::DFView<int const, lugizmo::DFRangeIndex<int>>>, "Read-only DFView must model random_access_range.");
static_assert(std::ranges::sized_range<lugizmo::DFView<int, lugizmo::DFRangeIndex<int>>>, "DFView must model sized_range.");
static_assert(std::ranges::common_range<lugizmo::DFView<int, lugizmo::DFRangeIndex<int>>>, "DFView must model common_range.");
static_assert(std::ranges::view<lugizmo::DFView<int, lugizmo::DFRangeIndex<int>>>, "DFView must model view.");
static_assert(std::ranges::borrowed_range<lugizmo::DFView<int, lugizmo::DFRangeIndex<int>>>, "DFView must model borrowed_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFView<int, lugizmo::DFRangeIndex<int>>&>, "DFView lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFView<int, lugizmo::DFRangeIndex<int>>>, "Temporary DFView objects must model viewable_range.");


#endif // LUGIZMO_DF_VIEW_H
