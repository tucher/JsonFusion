#pragma once
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace JsonFusion {

template <typename CharT, std::size_t N> struct ConstString
{
    constexpr bool check()  const {
        for(int i = 0; i < N; i ++) {
            if(std::uint8_t(m_data[i]) < 32) return false;
        }
        return true;
    }
    constexpr ConstString(const CharT (&foo)[N+1]) {
        for(std::size_t i = 0; i < N+1; i ++) {
            m_data[i] = foo[i];
        }
    }
    CharT m_data[N+1];
    static constexpr std::size_t Length = N;
    static constexpr std::size_t CharSize = sizeof (CharT);
    constexpr std::string_view toStringView() const {
        return {&m_data[0], &m_data[Length]};
    }
};
template <typename CharT, std::size_t N>
ConstString(const CharT (&str)[N])->ConstString<CharT, N-1>;

template<class Template>
struct is_const_string : std::false_type {};

template<typename CharT, std::size_t N>
struct is_const_string<ConstString<CharT, N>> : std::true_type {};


}
