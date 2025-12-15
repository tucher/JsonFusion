#pragma once

#include <iterator>

namespace JsonFusion {

// 1) Iterator you can:
//    - read as *it   (convertible to char)
//    - advance as it++ / ++it
template <class It>
concept CharInputIterator =
    std::input_iterator<It> &&
    std::convertible_to<std::iter_reference_t<It>, char>;

// 2) Matching "end" type you can:
//    - compare as it == end / it != end
template <class It, class Sent>
concept CharSentinelFor =
    CharInputIterator<It> &&
    std::sentinel_for<Sent, It>;


// 1) Iterator you can:
//    - write as *it   (convertible to char)
//    - advance as it++ / ++it
template <class It>
concept CharOutputIterator =
    std::output_iterator<It, char>;

// 2) Matching "end" type you can:
//    - compare as it == end / it != end
template <class Sent, class It>
concept CharSentinelForOut =
    std::sentinel_for<Sent, It>;




} // namespace JsonFusion

