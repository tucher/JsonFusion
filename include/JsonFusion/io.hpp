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


namespace io_details {
struct limitless_sentinel {};

constexpr bool operator==(const std::back_insert_iterator<std::string>&,
                                 const limitless_sentinel&) noexcept {
    return false;
}

constexpr bool operator==(const limitless_sentinel&,
                                 const std::back_insert_iterator<std::string>&) noexcept {
    return false;
}

constexpr bool operator!=(const std::back_insert_iterator<std::string>& it,
                                 const limitless_sentinel& s) noexcept {
    return !(it == s);
}

constexpr bool operator!=(const limitless_sentinel& s,
                                 const std::back_insert_iterator<std::string>& it) noexcept {
    return !(it == s);
}
}

} // namespace JsonFusion

