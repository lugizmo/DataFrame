// Filename: ViewIndexed.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_VIEW_INDEXED_H
#define LUGIZMO_DF_VIEW_INDEXED_H

#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

#include "lugizmo/Assert.h"

#include "IndexRange.h"
#include "View.h"

namespace lugizmo {

    namespace internal {

        template<typename Reference>
        using DFIndexedMember = std::conditional_t<std::is_lvalue_reference_v<Reference>, Reference, std::remove_cvref_t<Reference>>;

        template<typename Entry, typename ZippedEntry>
        [[nodiscard]] constexpr auto MakeDFIndexedEntry(ZippedEntry&& zippedEntry) noexcept -> Entry
        {
            return Entry{std::get<0>(std::forward<ZippedEntry>(zippedEntry)),
                         std::get<1>(std::forward<ZippedEntry>(zippedEntry))};
        }

        /**
         * @brief Named-entry facade over a `zip_view` iterator.
         *
         * All positioning and synchronization remain delegated to the zip
         * iterator. This facade only changes its dereference result.
         */
        template<typename ZippedIterator, typename Entry>
        class DFIndexedIterator
        {
            template<typename, typename>
            friend class DFIndexedIterator;

            ZippedIterator current;

        public:

            class Pointer;

            // NOLINTBEGIN(readability-identifier-naming)
            using iterator_category = std::random_access_iterator_tag;
            using iterator_concept  = std::random_access_iterator_tag;
            using difference_type   = std::iter_difference_t<ZippedIterator>;
            using reference         = Entry;
            using value_type        = reference;
            using pointer           = Pointer;
            // NOLINTEND(readability-identifier-naming)

            class Pointer
            {
                reference entry;

            public:
                constexpr explicit Pointer(reference value) noexcept : entry(std::move(value)) {}
                [[nodiscard]] constexpr auto operator->() noexcept -> reference* { return &entry; }
                [[nodiscard]] constexpr auto operator->() const noexcept -> reference const* { return &entry; }
            };

            constexpr DFIndexedIterator() noexcept = default;
            constexpr explicit DFIndexedIterator(ZippedIterator iterator) noexcept : current(std::move(iterator)) {}

            constexpr DFIndexedIterator(DFIndexedIterator const&) noexcept = default;
            constexpr DFIndexedIterator(DFIndexedIterator&&) noexcept = default;
            constexpr auto operator=(DFIndexedIterator const&) noexcept -> DFIndexedIterator& = default;
            constexpr auto operator=(DFIndexedIterator&&) noexcept -> DFIndexedIterator& = default;
            constexpr ~DFIndexedIterator() noexcept = default;

            template<typename OtherIterator>
                requires std::convertible_to<OtherIterator, ZippedIterator>
            constexpr DFIndexedIterator(DFIndexedIterator<OtherIterator, Entry> other) noexcept : current(std::move(other.current))
            {}

            [[nodiscard]] constexpr auto operator*() const noexcept -> reference { return MakeDFIndexedEntry<Entry>(*current); }
            [[nodiscard]] constexpr auto operator->() const noexcept -> Pointer { return Pointer(operator*()); }
            [[nodiscard]] constexpr auto operator[](difference_type const n) const noexcept -> reference { return MakeDFIndexedEntry<Entry>(current[n]); }

            constexpr auto operator++() noexcept -> DFIndexedIterator& { ++current; return *this; }
            constexpr auto operator++(int) noexcept -> DFIndexedIterator { auto previous = *this; ++*this; return previous; }
            constexpr auto operator--() noexcept -> DFIndexedIterator& { --current; return *this; }
            constexpr auto operator--(int) noexcept -> DFIndexedIterator { auto previous = *this; --*this; return previous; }
            constexpr auto operator+=(difference_type const n) noexcept -> DFIndexedIterator& { current += n; return *this; }
            constexpr auto operator-=(difference_type const n) noexcept -> DFIndexedIterator& { current -= n; return *this; }

            [[nodiscard]] constexpr auto operator+(difference_type const n) const noexcept -> DFIndexedIterator { return DFIndexedIterator(current + n); }
            [[nodiscard]] constexpr auto operator-(difference_type const n) const noexcept -> DFIndexedIterator { return DFIndexedIterator(current - n); }
            [[nodiscard]] constexpr auto operator-(DFIndexedIterator const& other) const noexcept -> difference_type { return current - other.current; }

            [[nodiscard]] constexpr auto operator==(DFIndexedIterator const& other) const noexcept -> bool = default;
            [[nodiscard]] constexpr auto operator<(DFIndexedIterator const& other) const noexcept -> bool { return current < other.current; }
            [[nodiscard]] constexpr auto operator<=(DFIndexedIterator const& other) const noexcept -> bool { return current <= other.current; }
            [[nodiscard]] constexpr auto operator>(DFIndexedIterator const& other) const noexcept -> bool { return current > other.current; }
            [[nodiscard]] constexpr auto operator>=(DFIndexedIterator const& other) const noexcept -> bool { return current >= other.current; }

            friend constexpr auto operator+(difference_type const n, DFIndexedIterator const& iterator) noexcept -> DFIndexedIterator
            {
                return iterator + n;
            }

        };

    } // namespace internal

    template<typename T, typename I>
    class DFViewIndexed : public std::ranges::view_interface<DFViewIndexed<T, I>>
    {
        using View      = DFView<T, I>;
        using Index     = std::remove_const_t<I>;
        using KeyType   = Index::ConstKeyType;
        using IndexView = Index::KeyView;

        template<typename Layout>
        using MDSpanDF = View::template MDSpanDF<Layout>;

        using ZippedView      = decltype(std::views::zip(std::declval<View&>(), std::declval<IndexView&>()));
        using ConstZippedView = decltype(std::views::zip(std::declval<View const&>(), std::declval<IndexView const&>()));

        static_assert(not std::is_pointer_v<T>   && not std::is_pointer_v<KeyType>);
        static_assert(not std::is_reference_v<T> && not std::is_reference_v<KeyType>);
        static_assert(std::is_const_v<KeyType>, "The index should not be able to be mutated by this view.");
        static_assert(std::ranges::random_access_range<ZippedView>);
        static_assert(std::ranges::random_access_range<ConstZippedView>);

        View      dataView;
        IndexView indexView;

        explicit DFViewIndexed(View view, IndexView indices) noexcept;

        [[nodiscard]] auto Zipped() noexcept -> ZippedView;
        [[nodiscard]] auto Zipped() const noexcept -> ConstZippedView;

    public:

        // ======== TYPES ==========================================================================================================================================================

        using ValueReference = std::ranges::range_reference_t<View>;
        using IndexReference = internal::DFIndexedMember<std::ranges::range_reference_t<IndexView>>;

        /**
         * @brief Named reference-like result produced by an indexed view.
         *
         * `value` refers to the dataframe element. `index` either refers to a
         * stored unique-index key or owns a generated range-index key. Public
         * members intentionally also enable structured binding as
         * `auto [value, index]`.
         */
        struct Entry
        {
            ValueReference val;
            IndexReference idx;
        };

        using ConstEntry    = Entry;
        using Iterator      = internal::DFIndexedIterator<std::ranges::iterator_t<ZippedView>, Entry>;
        using ConstIterator = internal::DFIndexedIterator<std::ranges::iterator_t<ConstZippedView>, Entry>;
        using ReverseIterator      = std::reverse_iterator<Iterator>;
        using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

        static_assert(sizeof(Iterator) == sizeof(std::ranges::iterator_t<ZippedView>),
                      "The named-entry facade must not add iterator state.");

        // ======== CONSTRUCTION ===================================================================================================================================================

        DFViewIndexed() noexcept;

        template<typename Layout>
        [[nodiscard]] static auto RecordView(MDSpanDF<Layout> original, I const* index, size_t recIndex, IndexView fldIndices) noexcept -> DFViewIndexed;

        template<typename Layout>
        [[nodiscard]] static auto FieldView(MDSpanDF<Layout> original, I const* index, size_t fldIndex, IndexView recIndices) noexcept -> DFViewIndexed;

        // ======== CAPACITY =======================================================================================================================================================

        [[nodiscard]] auto Size() const noexcept -> size_t;
        [[nodiscard]] auto Empty() const noexcept -> bool;

        // ======== COMPONENT VIEWS ================================================================================================================================================

        /// @brief Returns the dataframe-value component as its original strided view.
        [[nodiscard]] auto Values() const noexcept -> View;

        /// @brief Returns the read-only index component in the same logical order.
        [[nodiscard]] auto Indices() const noexcept -> IndexView;

        // ======== POSITIONAL ACCESS ==============================================================================================================================================

        [[nodiscard]] auto operator[](size_t i) noexcept -> Entry;
        [[nodiscard]] auto operator[](size_t i) const noexcept -> ConstEntry;
        [[nodiscard]] auto operator()(size_t i) noexcept -> std::optional<Entry>;
        [[nodiscard]] auto operator()(size_t i) const noexcept -> std::optional<ConstEntry>;

        /// @return First entry, or an empty optional when the view is empty.
        [[nodiscard]] auto Front() noexcept -> std::optional<Entry>;

        /// @copydoc Front()
        [[nodiscard]] auto Front() const noexcept -> std::optional<ConstEntry>;

        /// @return Final entry, or an empty optional when the view is empty.
        [[nodiscard]] auto Back() noexcept -> std::optional<Entry>;

        /// @copydoc Back()
        [[nodiscard]] auto Back() const noexcept -> std::optional<ConstEntry>;

        // ======== ITERATORS ======================================================================================================================================================

        // NOLINTBEGIN(readability-identifier-naming)
        [[nodiscard]] auto begin() noexcept -> Iterator;
        [[nodiscard]] auto end() noexcept -> Iterator;
        [[nodiscard]] auto begin() const noexcept -> ConstIterator;
        [[nodiscard]] auto end() const noexcept -> ConstIterator;
        [[nodiscard]] auto cbegin() const noexcept -> ConstIterator;
        [[nodiscard]] auto cend() const noexcept -> ConstIterator;
        [[nodiscard]] auto rbegin() noexcept -> ReverseIterator;
        [[nodiscard]] auto rend() noexcept -> ReverseIterator;
        [[nodiscard]] auto rbegin() const noexcept -> ConstReverseIterator;
        [[nodiscard]] auto rend() const noexcept -> ConstReverseIterator;
        [[nodiscard]] auto crbegin() const noexcept -> ConstReverseIterator;
        [[nodiscard]] auto crend() const noexcept -> ConstReverseIterator;
        // NOLINTEND(readability-identifier-naming)

        // ======== KEY ACCESS =====================================================================================================================================================

        [[nodiscard]] auto Contains(KeyType const& key) const noexcept -> bool;
        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*;
        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T*;
    };

    // ======== CONSTRUCTION =======================================================================================================================================================

    template<typename T, typename I>
    DFViewIndexed<T, I>::DFViewIndexed(View view, IndexView indices) noexcept :
        dataView(std::move(view)),
        indexView(std::move(indices))
    {
        LUGIZMO_ASSERT_TRACE(dataView.Size() == std::ranges::size(indexView),
                            "DFViewIndexed requires the data and index views to have equal sizes.");
    }

    template<typename T, typename I>
    DFViewIndexed<T, I>::DFViewIndexed() noexcept :
        dataView(),
        indexView()
    {}

    template<typename T, typename I>
    template<typename Layout>
    auto DFViewIndexed<T, I>::RecordView(MDSpanDF<Layout> original, I const* index, size_t const recIndex, IndexView fldIndices) noexcept -> DFViewIndexed
    {
        return DFViewIndexed(View::RecordView(original, index, recIndex), std::move(fldIndices));
    }

    template<typename T, typename I>
    template<typename Layout>
    auto DFViewIndexed<T, I>::FieldView(MDSpanDF<Layout> original, I const* index, size_t const fldIndex, IndexView recIndices) noexcept -> DFViewIndexed
    {
        return DFViewIndexed(View::FieldView(original, index, fldIndex), std::move(recIndices));
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Zipped() noexcept -> ZippedView
    {
        return std::views::zip(dataView, indexView);
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Zipped() const noexcept -> ConstZippedView
    {
        return std::views::zip(dataView, indexView);
    }

    // ======== CAPACITY ===========================================================================================================================================================

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Size() const noexcept -> size_t
    {
        return dataView.Size();
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Empty() const noexcept -> bool
    {
        return dataView.Empty();
    }

    // ======== COMPONENT VIEWS ====================================================================================================================================================

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Values() const noexcept -> View
    {
        return dataView;
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Indices() const noexcept -> IndexView
    {
        return indexView;
    }

    // ======== POSITIONAL ACCESS ==================================================================================================================================================

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::operator[](size_t const i) noexcept -> Entry
    {
        return begin()[static_cast<std::ptrdiff_t>(i)];
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::operator[](size_t const i) const noexcept -> ConstEntry
    {
        return begin()[static_cast<std::ptrdiff_t>(i)];
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::operator()(size_t const i) noexcept -> std::optional<Entry>
    {
        if(i >= Size()) return std::nullopt;
        return (*this)[i];
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::operator()(size_t const i) const noexcept -> std::optional<ConstEntry>
    {
        if(i >= Size()) return std::nullopt;
        return (*this)[i];
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Front() noexcept -> std::optional<Entry>
    {
        if(Empty()) return std::nullopt;
        return (*this)[0];
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Front() const noexcept -> std::optional<ConstEntry>
    {
        if(Empty()) return std::nullopt;
        return (*this)[0];
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Back() noexcept -> std::optional<Entry>
    {
        if(Empty()) return std::nullopt;
        return (*this)[Size() - 1];
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Back() const noexcept -> std::optional<ConstEntry>
    {
        if(Empty()) return std::nullopt;
        return (*this)[Size() - 1];
    }

    // ======== ITERATORS ==========================================================================================================================================================

    // NOLINTBEGIN(readability-identifier-naming)

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::begin() noexcept -> Iterator
    {
        return Iterator(Zipped().begin());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::end() noexcept -> Iterator
    {
        return Iterator(Zipped().end());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::begin() const noexcept -> ConstIterator
    {
        return ConstIterator(Zipped().begin());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::end() const noexcept -> ConstIterator
    {
        return ConstIterator(Zipped().end());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::cbegin() const noexcept -> ConstIterator
    {
        return begin();
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::cend() const noexcept -> ConstIterator
    {
        return end();
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::rbegin() noexcept -> ReverseIterator
    {
        return ReverseIterator(end());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::rend() noexcept -> ReverseIterator
    {
        return ReverseIterator(begin());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::rbegin() const noexcept -> ConstReverseIterator
    {
        return ConstReverseIterator(end());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::rend() const noexcept -> ConstReverseIterator
    {
        return ConstReverseIterator(begin());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::crbegin() const noexcept -> ConstReverseIterator
    {
        return ConstReverseIterator(cend());
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::crend() const noexcept -> ConstReverseIterator
    {
        return ConstReverseIterator(cbegin());
    }

    // NOLINTEND(readability-identifier-naming)

    // ======== KEY ACCESS =========================================================================================================================================================

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::Contains(KeyType const& key) const noexcept -> bool
    {
        return dataView.Contains(key);
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::At(KeyType const& key) noexcept -> T*
    {
        return dataView.At(key);
    }

    template<typename T, typename I>
    auto DFViewIndexed<T, I>::At(KeyType const& key) const noexcept -> T*
    {
        return dataView.At(key);
    }

} // namespace lugizmo

template<typename T, typename I>
inline constexpr bool std::ranges::enable_borrowed_range<lugizmo::DFViewIndexed<T, I>> = true; // NOLINT(readability-identifier-naming)

static_assert(std::ranges::random_access_range<lugizmo::DFViewIndexed<int, lugizmo::DFUniqueIndex<int> const>>, "DFViewIndexed must model random_access_range.");
static_assert(std::ranges::random_access_range<lugizmo::DFViewIndexed<int const, lugizmo::DFUniqueIndex<int> const>>, "Read-only DFViewIndexed must model random_access_range.");
static_assert(std::ranges::sized_range<lugizmo::DFViewIndexed<int, lugizmo::DFUniqueIndex<int> const>>, "DFViewIndexed must model sized_range.");
static_assert(std::ranges::common_range<lugizmo::DFViewIndexed<int, lugizmo::DFUniqueIndex<int> const>>, "DFViewIndexed must model common_range.");
static_assert(std::ranges::view<lugizmo::DFViewIndexed<int, lugizmo::DFUniqueIndex<int> const>>, "DFViewIndexed must model view.");
static_assert(std::ranges::borrowed_range<lugizmo::DFViewIndexed<int, lugizmo::DFUniqueIndex<int> const>>, "DFViewIndexed must model borrowed_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFViewIndexed<int, lugizmo::DFUniqueIndex<int> const>&>, "DFViewIndexed lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFViewIndexed<int, lugizmo::DFUniqueIndex<int> const>>, "Temporary DFViewIndexed objects must model viewable_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFViewIndexed<int const, lugizmo::DFUniqueIndex<int> const>&>, "Read-only DFViewIndexed lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFViewIndexed<int const, lugizmo::DFUniqueIndex<int> const>>, "Temporary read-only DFViewIndexed objects must model viewable_range.");

static_assert(std::ranges::random_access_range<lugizmo::DFViewIndexed<int, lugizmo::DFRangeIndex<int> const>>, "DFViewIndexed must model random_access_range.");
static_assert(std::ranges::random_access_range<lugizmo::DFViewIndexed<int const, lugizmo::DFRangeIndex<int> const>>, "Read-only DFViewIndexed must model random_access_range.");
static_assert(std::ranges::sized_range<lugizmo::DFViewIndexed<int, lugizmo::DFRangeIndex<int> const>>, "DFViewIndexed must model sized_range.");
static_assert(std::ranges::common_range<lugizmo::DFViewIndexed<int, lugizmo::DFRangeIndex<int> const>>, "DFViewIndexed must model common_range.");
static_assert(std::ranges::view<lugizmo::DFViewIndexed<int, lugizmo::DFRangeIndex<int> const>>, "DFViewIndexed must model view.");
static_assert(std::ranges::borrowed_range<lugizmo::DFViewIndexed<int, lugizmo::DFRangeIndex<int> const>>, "DFViewIndexed must model borrowed_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFViewIndexed<int, lugizmo::DFRangeIndex<int> const>&>, "DFViewIndexed lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFViewIndexed<int, lugizmo::DFRangeIndex<int> const>>, "Temporary DFViewIndexed objects must model viewable_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFViewIndexed<int const, lugizmo::DFRangeIndex<int> const>&>, "Read-only DFViewIndexed lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lugizmo::DFViewIndexed<int const, lugizmo::DFRangeIndex<int> const>>, "Temporary read-only DFViewIndexed objects must model viewable_range.");


#endif // LUGIZMO_DF_VIEW_INDEXED_H
