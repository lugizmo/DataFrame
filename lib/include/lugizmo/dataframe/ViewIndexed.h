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

namespace lgz {

    namespace internal {

        /**
         * @brief Stores lvalue references as references and materializes generated values.
         *
         * Unique-index iterators yield references to stored keys. Range-index
         * iterators generate keys by value; those values must be owned by the
         * resulting entry rather than retained as dangling rvalue references.
         */
        template<typename Reference>
        using DFIndexedMember = std::conditional_t<std::is_lvalue_reference_v<Reference>, Reference, std::remove_cvref_t<Reference>>;

        /// @brief Converts a tuple-like zip result into the public named entry proxy.
        template<typename Entry, typename ZippedEntry>
        [[nodiscard]] constexpr auto MakeDFIndexedEntry(ZippedEntry&& zippedEntry) noexcept -> Entry
        {
            return Entry{std::get<1>(std::forward<ZippedEntry>(zippedEntry)), std::get<0>(std::forward<ZippedEntry>(zippedEntry))};
        }

        /**
         * @brief Named-entry facade over a `zip_view` iterator.
         *
         * All positioning and synchronization remain delegated to the zip
         * iterator. This facade only changes its dereference result from a
         * tuple-like proxy to the public named `Entry` proxy.
         *
         * Dereference and positional indexing return `Entry` by value. The
         * entry's `val` member still refers to dataframe storage. Its `key`
         * member either refers to a stored key or owns a generated key.
         * Dereference and `operator[]` require a valid entry position. Iterator
         * difference and ordering require both operands to originate from the
         * same indexed view.
         *
         * @tparam ZippedIterator Iterator supplied by the internal zip view.
         * @tparam Entry Public proxy type returned by `DFViewIndexed`.
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

            /**
             * @brief Temporary arrow proxy for an entry returned by value.
             *
             * The pointer produced by `operator->` refers to the `Entry` stored
             * inside this proxy. It is valid only until the end of the full
             * expression. References contained in that entry retain their normal
             * underlying dataframe/index lifetimes.
             */
            class Pointer
            {
                reference entry;

            public:
                /// @brief Owns the temporary entry used for arrow access.
                constexpr explicit Pointer(reference value) noexcept : entry(std::move(value)) {}

                /// @return Pointer valid only for the lifetime of this arrow proxy.
                [[nodiscard]] constexpr auto operator->() noexcept -> reference* { return &entry; }

                /// @copydoc operator->()
                [[nodiscard]] constexpr auto operator->() const noexcept -> reference const* { return &entry; }
            };

            /// @brief Constructs a singular iterator.
            constexpr DFIndexedIterator() noexcept = default;

            /// @brief Wraps an iterator from the synchronized zip view.
            constexpr explicit DFIndexedIterator(ZippedIterator iterator) noexcept : current(std::move(iterator)) {}

            constexpr DFIndexedIterator(DFIndexedIterator const&) noexcept = default;
            constexpr DFIndexedIterator(DFIndexedIterator&&) noexcept = default;
            constexpr auto operator=(DFIndexedIterator const&) noexcept -> DFIndexedIterator& = default;
            constexpr auto operator=(DFIndexedIterator&&) noexcept -> DFIndexedIterator& = default;
            constexpr ~DFIndexedIterator() noexcept = default;

            /// @brief Converts a compatible iterator, such as mutable to const traversal.
            template<typename OtherIterator>
                requires std::convertible_to<OtherIterator, ZippedIterator>
            constexpr DFIndexedIterator(DFIndexedIterator<OtherIterator, Entry> other) noexcept : current(std::move(other.current))
            {}

            /// @return Named proxy for the entry at the current position.
            [[nodiscard]] constexpr auto operator*() const noexcept -> reference { return MakeDFIndexedEntry<Entry>(*current); }

            /// @return Temporary arrow proxy valid for the current full expression.
            [[nodiscard]] constexpr auto operator->() const noexcept -> Pointer { return Pointer(operator*()); }

            /// @return Named proxy at offset `n` from the current position.
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

    /**
     * @brief Non-owning view pairing dataframe values with their logical index keys.
     *
     * `DFViewIndexed` presents one dataframe field or record as a synchronized
     * random-access range. Each position produces an `Entry` containing the value
     * in `val` and the corresponding record or field key in `key`.
     *
     * @par Element constness
     * Value mutability is determined by `T`, not by constness of the view wrapper.
     * A const `DFViewIndexed<T, I>` still exposes mutable values when `T` is
     * mutable. `DFViewIndexed<T const, I>` provides read-only values. Index keys
     * cannot be used to mutate the underlying index. A generated key owned by an
     * entry may be modified locally without affecting that index.
     *
     * @par Entry proxy contract
     * Entries are lightweight proxy objects returned by value. `Entry::val`
     * refers to dataframe storage. For a stored unique index, `Entry::key` refers
     * to the stored key; for a generated range index, it owns the generated key.
     * Use `auto` or `auto&&` when naming a dereferenced entry; `auto&` cannot bind
     * to the temporary proxy. Copying an entry copies its references and any
     * generated key, but never copies the dataframe value.
     *
     * Iterator arrow access uses a temporary pointer proxy. Expressions such as
     * `iterator->val` are valid, but the pointer returned by `operator->` must not
     * be retained beyond the full expression.
     *
     * @par Ownership, lifetime, and invalidation
     * The view owns neither dataframe values nor stored index keys. The dataframe
     * storage and index must outlive the view and every iterator, pointer, or
     * reference obtained from it. A generated range key owned by an `Entry`
     * remains valid for the lifetime of that entry.
     *
     * Destruction or movement of the dataframe and structural operations that
     * change storage, fields, records, indices, or ordering invalidate the view
     * and its iterators. Updating existing values does not invalidate them.
     * `Values()` and `Indices()` return non-owning component views with the same
     * lifetime and invalidation requirements.
     *
     * @par Pairing invariant
     * The value and index component views must have equal sizes and describe the
     * same logical positions. Construction validates their sizes when trace
     * assertions are enabled.
     *
     * @par Standard range integration
     * `DFViewIndexed` is a random-access, sized, common view, borrowed range, and
     * viewable range. Standard adaptor closures can be applied directly. Iterators
     * may outlive the lightweight wrapper, but never the referenced dataframe
     * storage or stored index.
     *
     * Inherited lowercase `front()` and `back()` follow the standard non-empty
     * precondition. Uppercase `Front()` and `Back()` provide empty-safe optional
     * access.
     *
     * @tparam T Dataframe element type, including const qualification.
     * @tparam I Index type describing the keys paired with the selected values.
     * @tparam V Underlying value view; `DFView<T, I>` by default and `DFSpan<T, I>`
     *           for a naturally contiguous axis.
     */
    template<typename T, typename I, typename V = DFView<T, I>>
    class DFViewIndexed : public std::ranges::view_interface<DFViewIndexed<T, I, V>>
    {
        using View      = V;
        using Index     = std::remove_const_t<I>;
        using KeyType   = Index::ConstKeyType;
        using IndexView = Index::KeyView;

        template<typename Layout>
        using MDSpanDF = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>;

        using ZippedView      = decltype(std::views::zip(std::declval<View&>(), std::declval<IndexView&>()));
        using ConstZippedView = decltype(std::views::zip(std::declval<View const&>(), std::declval<IndexView const&>()));

        static_assert(not std::is_pointer_v<T>   && not std::is_pointer_v<KeyType>);
        static_assert(not std::is_reference_v<T> && not std::is_reference_v<KeyType>);
        static_assert(std::is_const_v<KeyType>, "The index should not be able to be mutated by this view.");
        static_assert(std::ranges::random_access_range<ZippedView>);
        static_assert(std::ranges::random_access_range<ConstZippedView>);

        View      dataView;
        IndexView indexView;

        /**
         * @brief Constructs the paired view from equally sized component views.
         * @pre `view.Size() == std::ranges::size(indices)`.
         */
        explicit DFViewIndexed(View view, IndexView indices) noexcept;

        /// @return Internal synchronized zip view over the two components.
        [[nodiscard]] auto Zipped() noexcept -> ZippedView;

        /// @copydoc Zipped()
        [[nodiscard]] auto Zipped() const noexcept -> ConstZippedView;

    public:

        // ======== TYPES ==========================================================================================================================================================

        /// @brief Reference type used by `Entry::val`; follows the const qualification of `T`.
        using ValueReference = std::ranges::range_reference_t<View>;

        /// @brief Read-only stored-key reference or owned generated-key value used by `Entry::key`.
        using IndexReference = internal::DFIndexedMember<std::ranges::range_reference_t<IndexView>>;

        /**
         * @brief Named reference-like result produced by an indexed view.
         *
         * `key` either refers to a stored index key or owns a generated key.
         * `val` refers to the dataframe element. Public members
         * intentionally also enable structured binding as `auto [key, val]`.
         *
         * The entry itself is returned by value. Retaining `val`, or `key` when
         * it is a reference, requires the underlying dataframe/index to remain
         * alive and unmodified structurally.
         */
        struct Entry
        {
            /// @brief Corresponding read-only stored key or owned generated key.
            IndexReference key;

            /// @brief Reference to the dataframe value at this logical position.
            ValueReference val;
        };

        /// @brief Entry returned through a const view wrapper; element constness still follows `T`.
        using ConstEntry    = Entry;

        /// @brief Random-access iterator returning named entry proxies by value.
        using Iterator      = internal::DFIndexedIterator<std::ranges::iterator_t<ZippedView>, Entry>;

        /// @brief Iterator used by const view wrappers; value constness still follows `T`.
        using ConstIterator = internal::DFIndexedIterator<std::ranges::iterator_t<ConstZippedView>, Entry>;

        /// @brief Reverse iterator for mutable view wrappers.
        using ReverseIterator      = std::reverse_iterator<Iterator>;

        /// @brief Reverse iterator for const view wrappers.
        using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

        static_assert(sizeof(Iterator) == sizeof(std::ranges::iterator_t<ZippedView>),
                      "The named-entry facade must not add iterator state.");

        // ======== CONSTRUCTION ===================================================================================================================================================

        /**
         * @brief Constructs an empty view with no values or index entries.
         * @post `Empty()` is true and `begin() == end()`.
         */
        DFViewIndexed() noexcept;

        /**
         * @brief Creates an indexed view over all fields of one record.
         *
         * @param original   Matrix whose first extent is records and second extent is fields.
         * @param index      Field index used for keyed value lookup.
         * @param recIndex   Zero-based record position to view.
         * @param fldIndices Field keys paired with the record values.
         *
         * @pre `recIndex < original.extent(0)`.
         * @pre `std::ranges::size(fldIndices) == original.extent(1)`.
         * @return Non-owning paired view containing `original.extent(1)` entries.
         */
        template<typename Layout>
        [[nodiscard]] static auto RecordView(MDSpanDF<Layout> original, I const* index, size_t recIndex, IndexView fldIndices) noexcept -> DFViewIndexed;

        /**
         * @brief Creates an indexed view over all records of one field.
         *
         * @param original   Matrix whose first extent is records and second extent is fields.
         * @param index      Record index used for keyed value lookup.
         * @param fldIndex   Zero-based field position to view.
         * @param recIndices Record keys paired with the field values.
         *
         * @pre `fldIndex < original.extent(1)`.
         * @pre `std::ranges::size(recIndices) == original.extent(0)`.
         * @return Non-owning paired view containing `original.extent(0)` entries.
         */
        template<typename Layout>
        [[nodiscard]] static auto FieldView(MDSpanDF<Layout> original, I const* index, size_t fldIndex, IndexView recIndices) noexcept -> DFViewIndexed;

        // ======== CAPACITY =======================================================================================================================================================

        /// @return Number of paired entries.
        [[nodiscard]] auto Size() const noexcept -> size_t;

        /// @return True when the view contains no entries.
        [[nodiscard]] auto Empty() const noexcept -> bool;

        // ======== COMPONENT VIEWS ================================================================================================================================================

        /**
         * @brief Returns the dataframe-value component as its original view.
         * @return Non-owning view with the same value order and mutability as this view.
         */
        [[nodiscard]] auto Values() const noexcept -> View;

        /**
         * @brief Returns the read-only index component in the same logical order.
         * @return Non-owning stored-key view or generated range-key view.
         */
        [[nodiscard]] auto Indices() const noexcept -> IndexView;

        // ======== POSITIONAL ACCESS ==============================================================================================================================================

        /**
         * @brief Returns the entry at zero-based position `i` without bounds checking.
         * @pre `i < Size()`.
         */
        [[nodiscard]] auto operator[](size_t i) noexcept -> Entry;

        /// @copydoc operator[](size_t)
        [[nodiscard]] auto operator[](size_t i) const noexcept -> ConstEntry;

        /**
         * @brief Performs checked zero-based positional access.
         * @return Entry at `i`, or an empty optional when `i >= Size()`.
         */
        [[nodiscard]] auto operator()(size_t i) noexcept -> std::optional<Entry>;

        /// @copydoc operator()(size_t)
        [[nodiscard]] auto operator()(size_t i) const noexcept -> std::optional<ConstEntry>;

        /**
         * @brief Provides empty-safe access to the first entry.
         * @return First entry, or an empty optional when the view is empty.
         */
        [[nodiscard]] auto Front() noexcept -> std::optional<Entry>;

        /// @copydoc Front()
        [[nodiscard]] auto Front() const noexcept -> std::optional<ConstEntry>;

        /**
         * @brief Provides empty-safe access to the final entry.
         * @return Final entry, or an empty optional when the view is empty.
         */
        [[nodiscard]] auto Back() noexcept -> std::optional<Entry>;

        /// @copydoc Back()
        [[nodiscard]] auto Back() const noexcept -> std::optional<ConstEntry>;

        // ======== ITERATORS ======================================================================================================================================================

        // NOLINTBEGIN(readability-identifier-naming)
        /// @return Iterator to the first paired entry.
        [[nodiscard]] auto begin() noexcept -> Iterator;

        /// @return Iterator one past the final paired entry.
        [[nodiscard]] auto end() noexcept -> Iterator;

        /// @copydoc begin()
        [[nodiscard]] auto begin() const noexcept -> ConstIterator;

        /// @copydoc end()
        [[nodiscard]] auto end() const noexcept -> ConstIterator;

        /// @return Iterator to the first entry; value mutability still follows `T`.
        [[nodiscard]] auto cbegin() const noexcept -> ConstIterator;

        /// @return Iterator one past the final entry; value mutability still follows `T`.
        [[nodiscard]] auto cend() const noexcept -> ConstIterator;

        /// @return Reverse iterator to the final paired entry.
        [[nodiscard]] auto rbegin() noexcept -> ReverseIterator;

        /// @return Reverse iterator one past the first paired entry.
        [[nodiscard]] auto rend() noexcept -> ReverseIterator;

        /// @copydoc rbegin()
        [[nodiscard]] auto rbegin() const noexcept -> ConstReverseIterator;

        /// @copydoc rend()
        [[nodiscard]] auto rend() const noexcept -> ConstReverseIterator;

        /// @return Const reverse iterator to the final paired entry.
        [[nodiscard]] auto crbegin() const noexcept -> ConstReverseIterator;

        /// @return Const reverse iterator one past the first paired entry.
        [[nodiscard]] auto crend() const noexcept -> ConstReverseIterator;
        // NOLINTEND(readability-identifier-naming)

        // ======== KEY ACCESS =====================================================================================================================================================

        /**
         * @brief Checks whether `key` belongs to the index represented by this view.
         * @return True when keyed lookup resolves to a value in this view.
         */
        [[nodiscard]] auto Contains(KeyType const& key) const noexcept -> bool;

        /**
         * @brief Looks up a dataframe value by its paired index key.
         * @return Pointer to the value, or null when `key` is absent.
         */
        [[nodiscard]] auto At(KeyType const& key) noexcept -> T*;

        /// @copydoc At(KeyType const&)
        [[nodiscard]] auto At(KeyType const& key) const noexcept -> T*;
    };

    // ======== CONSTRUCTION =======================================================================================================================================================

    template<typename T, typename I, typename V>
    DFViewIndexed<T, I, V>::DFViewIndexed(View view, IndexView indices) noexcept :
        dataView(std::move(view)),
        indexView(std::move(indices))
    {
        LUGIZMO_ASSERT_TRACE(dataView.Size() == std::ranges::size(indexView),
                            "DFViewIndexed requires the data and index views to have equal sizes.");
    }

    template<typename T, typename I, typename V>
    DFViewIndexed<T, I, V>::DFViewIndexed() noexcept :
        dataView(),
        indexView()
    {}

    template<typename T, typename I, typename V>
    template<typename Layout>
    auto DFViewIndexed<T, I, V>::RecordView(MDSpanDF<Layout> original, I const* index, size_t const recIndex, IndexView fldIndices) noexcept -> DFViewIndexed
    {
        return DFViewIndexed(View::RecordView(original, index, recIndex), std::move(fldIndices));
    }

    template<typename T, typename I, typename V>
    template<typename Layout>
    auto DFViewIndexed<T, I, V>::FieldView(MDSpanDF<Layout> original, I const* index, size_t const fldIndex, IndexView recIndices) noexcept -> DFViewIndexed
    {
        return DFViewIndexed(View::FieldView(original, index, fldIndex), std::move(recIndices));
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Zipped() noexcept -> ZippedView
    {
        return std::views::zip(dataView, indexView);
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Zipped() const noexcept -> ConstZippedView
    {
        return std::views::zip(dataView, indexView);
    }

    // ======== CAPACITY ===========================================================================================================================================================

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Size() const noexcept -> size_t
    {
        return dataView.Size();
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Empty() const noexcept -> bool
    {
        return dataView.Empty();
    }

    // ======== COMPONENT VIEWS ====================================================================================================================================================

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Values() const noexcept -> View
    {
        return dataView;
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Indices() const noexcept -> IndexView
    {
        return indexView;
    }

    // ======== POSITIONAL ACCESS ==================================================================================================================================================

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::operator[](size_t const i) noexcept -> Entry
    {
        return begin()[static_cast<std::ptrdiff_t>(i)];
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::operator[](size_t const i) const noexcept -> ConstEntry
    {
        return begin()[static_cast<std::ptrdiff_t>(i)];
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::operator()(size_t const i) noexcept -> std::optional<Entry>
    {
        if(i >= Size()) return std::nullopt;
        return (*this)[i];
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::operator()(size_t const i) const noexcept -> std::optional<ConstEntry>
    {
        if(i >= Size()) return std::nullopt;
        return (*this)[i];
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Front() noexcept -> std::optional<Entry>
    {
        if(Empty()) return std::nullopt;
        return (*this)[0];
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Front() const noexcept -> std::optional<ConstEntry>
    {
        if(Empty()) return std::nullopt;
        return (*this)[0];
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Back() noexcept -> std::optional<Entry>
    {
        if(Empty()) return std::nullopt;
        return (*this)[Size() - 1];
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Back() const noexcept -> std::optional<ConstEntry>
    {
        if(Empty()) return std::nullopt;
        return (*this)[Size() - 1];
    }

    // ======== ITERATORS ==========================================================================================================================================================

    // NOLINTBEGIN(readability-identifier-naming)

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::begin() noexcept -> Iterator
    {
        return Iterator(Zipped().begin());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::end() noexcept -> Iterator
    {
        return Iterator(Zipped().end());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::begin() const noexcept -> ConstIterator
    {
        return ConstIterator(Zipped().begin());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::end() const noexcept -> ConstIterator
    {
        return ConstIterator(Zipped().end());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::cbegin() const noexcept -> ConstIterator
    {
        return begin();
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::cend() const noexcept -> ConstIterator
    {
        return end();
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::rbegin() noexcept -> ReverseIterator
    {
        return ReverseIterator(end());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::rend() noexcept -> ReverseIterator
    {
        return ReverseIterator(begin());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::rbegin() const noexcept -> ConstReverseIterator
    {
        return ConstReverseIterator(end());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::rend() const noexcept -> ConstReverseIterator
    {
        return ConstReverseIterator(begin());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::crbegin() const noexcept -> ConstReverseIterator
    {
        return ConstReverseIterator(cend());
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::crend() const noexcept -> ConstReverseIterator
    {
        return ConstReverseIterator(cbegin());
    }

    // NOLINTEND(readability-identifier-naming)

    // ======== KEY ACCESS =========================================================================================================================================================

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::Contains(KeyType const& key) const noexcept -> bool
    {
        return dataView.Contains(key);
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::At(KeyType const& key) noexcept -> T*
    {
        return dataView.At(key);
    }

    template<typename T, typename I, typename V>
    auto DFViewIndexed<T, I, V>::At(KeyType const& key) const noexcept -> T*
    {
        return dataView.At(key);
    }

} // namespace lgz

template<typename T, typename I, typename V>
inline constexpr bool std::ranges::enable_borrowed_range<lgz::DFViewIndexed<T, I, V>> = true; // NOLINT(readability-identifier-naming)

static_assert(std::ranges::random_access_range<lgz::DFViewIndexed<int, lgz::DFUniqueIndex<int> const>>, "DFViewIndexed must model random_access_range.");
static_assert(std::ranges::random_access_range<lgz::DFViewIndexed<int const, lgz::DFUniqueIndex<int> const>>, "Read-only DFViewIndexed must model random_access_range.");
static_assert(std::ranges::sized_range<lgz::DFViewIndexed<int, lgz::DFUniqueIndex<int> const>>, "DFViewIndexed must model sized_range.");
static_assert(std::ranges::common_range<lgz::DFViewIndexed<int, lgz::DFUniqueIndex<int> const>>, "DFViewIndexed must model common_range.");
static_assert(std::ranges::view<lgz::DFViewIndexed<int, lgz::DFUniqueIndex<int> const>>, "DFViewIndexed must model view.");
static_assert(std::ranges::borrowed_range<lgz::DFViewIndexed<int, lgz::DFUniqueIndex<int> const>>, "DFViewIndexed must model borrowed_range.");
static_assert(std::ranges::viewable_range<lgz::DFViewIndexed<int, lgz::DFUniqueIndex<int> const>&>, "DFViewIndexed lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lgz::DFViewIndexed<int, lgz::DFUniqueIndex<int> const>>, "Temporary DFViewIndexed objects must model viewable_range.");
static_assert(std::ranges::viewable_range<lgz::DFViewIndexed<int const, lgz::DFUniqueIndex<int> const>&>, "Read-only DFViewIndexed lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lgz::DFViewIndexed<int const, lgz::DFUniqueIndex<int> const>>, "Temporary read-only DFViewIndexed objects must model viewable_range.");

static_assert(std::ranges::random_access_range<lgz::DFViewIndexed<int, lgz::DFRangeIndex<int> const>>, "DFViewIndexed must model random_access_range.");
static_assert(std::ranges::random_access_range<lgz::DFViewIndexed<int const, lgz::DFRangeIndex<int> const>>, "Read-only DFViewIndexed must model random_access_range.");
static_assert(std::ranges::sized_range<lgz::DFViewIndexed<int, lgz::DFRangeIndex<int> const>>, "DFViewIndexed must model sized_range.");
static_assert(std::ranges::common_range<lgz::DFViewIndexed<int, lgz::DFRangeIndex<int> const>>, "DFViewIndexed must model common_range.");
static_assert(std::ranges::view<lgz::DFViewIndexed<int, lgz::DFRangeIndex<int> const>>, "DFViewIndexed must model view.");
static_assert(std::ranges::borrowed_range<lgz::DFViewIndexed<int, lgz::DFRangeIndex<int> const>>, "DFViewIndexed must model borrowed_range.");
static_assert(std::ranges::viewable_range<lgz::DFViewIndexed<int, lgz::DFRangeIndex<int> const>&>, "DFViewIndexed lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lgz::DFViewIndexed<int, lgz::DFRangeIndex<int> const>>, "Temporary DFViewIndexed objects must model viewable_range.");
static_assert(std::ranges::viewable_range<lgz::DFViewIndexed<int const, lgz::DFRangeIndex<int> const>&>, "Read-only DFViewIndexed lvalues must model viewable_range.");
static_assert(std::ranges::viewable_range<lgz::DFViewIndexed<int const, lgz::DFRangeIndex<int> const>>, "Temporary read-only DFViewIndexed objects must model viewable_range.");


#endif // LUGIZMO_DF_VIEW_INDEXED_H
