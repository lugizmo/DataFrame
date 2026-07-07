// Filename: ForEach.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DATAFRAME_ALGORITHM_FOR_EACH_H
#define LUGIZMO_DATAFRAME_ALGORITHM_FOR_EACH_H

#include <algorithm>
#include <functional>
#include <ranges>
#include <utility>

namespace lgz {

    template<typename T, typename F, typename R, typename L>
    struct DataFrame;

    /**
     * @brief  Applies `function` to each projected element in range order.
     * @return Standard `std::ranges::for_each` result containing the final iterator and the moved function object.
     */
    template<std::ranges::input_range Range, typename Function, typename Projection = std::identity>
    requires std::indirectly_unary_invocable<Function, std::projected<std::ranges::iterator_t<Range>, Projection>>
    constexpr auto ForEach(Range&& range, Function function, Projection projection = {})
    {
        return std::ranges::for_each(std::forward<Range>(range), std::move(function), std::move(projection));
    }

    /**
     * @brief   Applies `function` to every dataframe value in physical storage order.
     * @details Mutable and const element access follows the qualification of `dataframe`.
     *
     * @return Standard `std::ranges::for_each` result over `dataframe.Values()`.
     */
    template<typename T, typename F, typename R, typename L, typename Function, typename Projection = std::identity>
    constexpr auto ForEach(DataFrame<T, F, R, L>& dataframe, Function function, Projection projection = {})
    {
        return ForEach(dataframe.Values(), std::move(function), std::move(projection));
    }

    /**
     * @copydoc ForEach(DataFrame<T, F, R, L>&, Function, Projection)
     */
    template<typename T, typename F, typename R, typename L, typename Function, typename Projection = std::identity>
    constexpr auto ForEach(DataFrame<T, F, R, L> const& dataframe, Function function, Projection projection = {})
    {
        return ForEach(dataframe.Values(), std::move(function), std::move(projection));
    }

} // namespace lgz

#endif // LUGIZMO_DATAFRAME_ALGORITHM_FOR_EACH_H
