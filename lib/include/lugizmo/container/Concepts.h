// Filename: Concepts.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_CONTAINER_CONCEPTS_H
#define LUGIZMO_CONTAINER_CONCEPTS_H

#include <utility>
#include <iterator>
#include <type_traits>

namespace lugizmo {

    /**
     *  @brief   Checks if a class is an iterable and therefor
     *           implements t.begin() and t.end() returning an iterator.
     *  @details No support for c-array with std::begin/end just use
     *           std::span or similar type.
     */
    template<typename T>
    concept Iterable = requires(T t)
    {
        { t.begin() } -> std::input_or_output_iterator;
        { t.end() }   -> std::input_or_output_iterator;
    };

    /**
     *  @brief Base for iterable depth from type.
     *  @see IterableDepth<T> for more information.
     */
    template <typename T>
    struct IterableDepth : std::integral_constant<std::size_t, 0> {};

    /**
     *  @brief Compute the depth of iterables containing iterables.
     */
    template <typename T>
    requires Iterable<T>
    struct IterableDepth<T> : std::integral_constant<std::size_t, 1 + IterableDepth<std::iter_value_t<decltype(std::begin(std::declval<T>()))>>::value> {};

    /**
     *  @brief Helper struct for recursive iterable containing
     *         iterables concept.
     *  @see   IterableOfIterables use/call this to make the check!
     */
    template <typename, std::size_t>
    struct IsIterableOfIterables : std::false_type {};

    /**
     *  @brief Helper struct for recursive iterable containing
     *         iterables concept handling base case of depth == 1.#
     *  @see   IterableOfIterables use/call this to make the check!
     */
    template<typename T>
    struct IsIterableOfIterables<T, 1> : std::bool_constant<Iterable<T>> {};

    /**
     *  @brief Recursive check on the iterables.
     *  @see   IterableOfIterables use/call this to make the check!
     */
    template <typename T, std::size_t Depth>
    requires (Depth > 1)
    struct IsIterableOfIterables<T, Depth> : std::bool_constant<Iterable<T> &&
                                           IsIterableOfIterables<std::iter_value_t<decltype(std::begin(std::declval<T>()))>, Depth - 1>::value> {};

    /**
     *  @brief   Check if a class is an iterable of iterables in a recursive
     *           manner. So you could have a multiple layers of iterables.
     *  @details No support for c-array with std::begin/end just use
     *           std::span or similar type.
     */
    template <typename T>
    concept IterableOfIterables = IsIterableOfIterables<T, IterableDepth<T>::value>::value;

    /**
     *  @brief   Checks if a class is an iterable of iterables and therefor
     *           all base and contained classes implement t.begin() and
     *           t.end() returning an iterator.
     *  @details No support for c-array with std::begin/end just use
     *           std::span or similar type.
     */
    template<typename T>
    concept IterableOfIterable = Iterable<T> && Iterable<decltype(*std::declval<T>().begin())>;

    /**
     * @brief Concept for checking if a type is an iterable with minimal requirements (supports range-based for loops).
     */
    template<typename T>
    concept MinimalIterable = requires(T t) {
        std::ranges::begin(t);
        std::ranges::end(t);
    };

    /**
     * @brief Concept for checking if a type is an iterable of iterables.
     *        This checks only the minimal requirements.
     *
     * This ensures that:
     *  - The outer type is iterable (minimal).
     *  - The elements of the outer type are also iterable (minimal).
     */
    template<typename T>
    concept MinimalIterableOfIterable =
        MinimalIterable<T> &&
        MinimalIterable<std::ranges::range_value_t<T>>;
    }

#endif // LUGIZMO_CONTAINER_CONCEPTS_H
