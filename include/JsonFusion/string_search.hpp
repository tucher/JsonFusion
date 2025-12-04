#pragma once

#include <array>
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <algorithm>

namespace JsonFusion {
namespace string_search {

struct StringDescr {
    std::string_view name;
    std::size_t originalIndex;

};


template<std::size_t N, std::size_t MaxStringLength>
struct PerfectHashDFA {
    static constexpr std::size_t AlphabetSize = 256;

    // We use an unsigned state type, reserving 0 for "dead".
    using state_t = std::uint16_t;

    // MaxStates is a safe upper bound: 1 root + N * MaxStringLength
    static constexpr std::size_t MaxStates = 1 + N * MaxStringLength;

    static_assert(MaxStates + 1 <= std::numeric_limits<state_t>::max(),
                  "Too many states for chosen state_t");

    static constexpr state_t Dead = 0;
    static constexpr state_t Root = 1;

    // State transition table:
    // trans[s][b] -> next state
    // We have MaxStates+1 so that index "MaxStates" is usable, plus 0 for Dead.
    std::array<std::array<state_t, AlphabetSize>, MaxStates + 1> trans{};
    // For each state, index of the key it accepts, or -1 if non-accepting.
    std::array<int, MaxStates + 1> accept{};
    // Number of states actually used (for debug / info, not needed at runtime).
    std::size_t states_used = 0;

    // -------- constexpr build from keys --------

    constexpr PerfectHashDFA(const std::array<StringDescr, N>& keys)
        : trans{}, accept{}, states_used(Root + 1) // next free state
    {
        // Initialize accept to -1 and transitions to Dead (0)
        for (std::size_t s = 0; s <= MaxStates; ++s) {
            accept[s] = -1;
            for (std::size_t b = 0; b < AlphabetSize; ++b) {
                trans[s][b] = Dead;
            }
        }

        // Root state exists
        // states_used already set to Root+1 (i.e. next free ID)

        // Build the trie
        for (std::size_t key_index = 0; key_index < N; ++key_index) {
            std::string_view sv = keys[key_index].name;
            state_t cur = Root;

            for (unsigned char ch : sv) {
                state_t& next = trans[cur][ch];

                if (next == Dead) {
                    // Create new state
                    next = static_cast<state_t>(states_used);
                    ++states_used;
                }
                cur = next;
            }

            // Mark accepting state with index of the key
            // (If duplicates exist, you may want to handle specially.)
            accept[cur] = static_cast<int>(key_index);
        }
    }

    // Simple full-string lookup (not incremental).
    // Returns -1 if not found.
    constexpr int lookup(std::string_view s) const {
        state_t cur = Root;
        for (unsigned char ch : s) {
            cur = trans[cur][ch];
            if (cur == Dead) {
                return -1;
            }
        }
        return accept[cur];
    }

    // -------- Incremental runner API --------

    struct Runner {
        const PerfectHashDFA* dfa;
        state_t state;

        constexpr Runner(const PerfectHashDFA& d)
            : dfa(&d), state(Root) {}

        // Feed one byte; returns false if we hit a dead state.
        constexpr bool step(unsigned char byte) {
            if (state == Dead) return false;
            state = dfa->trans[state][byte];
            return state != Dead;
        }

        // Convenience overload if you have char
        constexpr bool step(char byte) {
            return step(static_cast<unsigned char>(byte));
        }

        // Feed a block of bytes; stops early if dead.
        constexpr bool step(std::string_view sv) {
            for (unsigned char c : sv) {
                if (!step(c)) return false;
            }
            return true;
        }

        // Are we already in a dead state?
        [[nodiscard]] constexpr bool is_dead() const {
            return state == Dead;
        }

        // Are we exactly at an accepting state?
        [[nodiscard]] constexpr bool is_accepting() const {
            return state != Dead && dfa->accept[state] != -1;
        }

        // Get index in original array, or -1 if not at an accepting state.
        [[nodiscard]] constexpr int index() const {
            if (state == Dead) return -1;
            return dfa->accept[state];
        }

        // The current DFA state is your collision-free "hash" of the prefix.
        [[nodiscard]] constexpr state_t hash() const {
            return state;
        }

        // Reset to the beginning (root)
        constexpr void reset() { state = Root; }
    };
};

template <std::size_t N, std::size_t MaxStringLength>
struct DFARunner {
    using DFA = string_search::PerfectHashDFA<N, MaxStringLength>;
    const DFA & dfa;

    char buffer[MaxStringLength];
    std::size_t length = 0;

    constexpr DFARunner(const DFA& d)
        : dfa(d) {}


    constexpr bool step(char ch) {
        if (length < MaxStringLength) {
            buffer[length++] = ch;
            return true;
        }
        // Field name too long, won't match anything
        return false;
    }

    constexpr int result() {
        return dfa.lookup(std::string_view(buffer, length));
    }
};

/*
int test() {
    constexpr std::array<StringDescr, 8> keys{
        StringDescr{"hello", 0},
        StringDescr{"world", 0},
        StringDescr{"hella", 0},
        StringDescr{"ab", 0},
        StringDescr{"hell", 0},
        StringDescr{"a", 0},
        StringDescr{"help", 0},
        StringDescr{"b", 0}
    };

    constexpr std::size_t MaxStringLength = 6;

    // Build DFA at compile time
    static constexpr PerfectHashDFA<keys.size(), MaxStringLength> dfa{keys};
    static_assert(dfa.lookup("hello") == 0);
    static_assert(dfa.lookup("world") == 1);
    static_assert(dfa.lookup("hell")  == 4);
    static_assert(dfa.lookup("hella")  == 2);
    static_assert(dfa.lookup("help")  == 6);
    static_assert(dfa.lookup("he")    == -1);
    static_assert(dfa.lookup("helmet") == -1);
    static_assert(dfa.lookup("helmewefwefwefewft") == -1);
    static_assert(dfa.lookup("h") == -1);
    static_assert(dfa.lookup("0") == -1);
    static_assert(dfa.lookup("") == -1);
    static_assert(dfa.lookup("a") == 5);
    static_assert(dfa.lookup("b") == 7);

    return 0;
}

*/

// Binary search strategy: incrementally narrows candidate range per character
struct IncrementalBinaryFieldSearch {
    using StringDescr = string_search::StringDescr;
    using It = const StringDescr*;

    It first;         // current candidate range [first, last)
    It last;
    It original_begin; // original begin, for reset
    It original_end;  // original end, used as "empty result"
    std::size_t depth = 0; // how many characters have been fed

    constexpr IncrementalBinaryFieldSearch(It begin, It end)
        : first(begin), last(end), original_begin(begin), original_end(end), depth(0) {}

    // Feed next character; narrows [first, last) by character at position `depth`.
    // Returns true if any candidates remain after this step.
    constexpr bool step(char ch) {
        if (first == last)
            return false;

        // projection: StringDescr -> char at current depth
        auto char_at_depth = [this](const StringDescr& f) -> char {
            std::string_view s = f.name;
            // sentinel: '\0' means "past the end"
            return depth < s.size() ? s[depth] : '\0';
        };

        // lower_bound: first element with char_at_depth(elem) >= ch
        auto lower = std::ranges::lower_bound(
            first, last, ch, {}, char_at_depth
            );

        // upper_bound: first element with char_at_depth(elem) > ch
        auto upper = std::ranges::upper_bound(
            lower, last, ch, {}, char_at_depth
            );

        first = lower;
        last  = upper;
        ++depth;
        return first != last;
    }

    // Return:
    //   - pointer to unique StringDescr if exactly one matches AND
    //     fully typed (depth >= name.size())
    //   - original_end if 0 matches OR >1 matches OR undertyped.
    constexpr It result() const {
        if (first == last)
            return original_end; // 0 matches


        // exactly one candidate
        const StringDescr& candidate = *first;

        // undertyping: not all characters of the name have been given
        if (depth != candidate.name.size())
            return original_end;

        return first;
    }
    // Reset for next key
    constexpr void reset() {
        first = original_begin;
        last = original_end;
        depth = 0;
    }
};

// Linear search strategy: buffers the field name, does simple linear search at the end
template<std::size_t MaxLen>
struct BufferedLinearFieldSearch {
    using It = const StringDescr*;

    It begin;         // field descriptors range [begin, end)
    It end;
    char buffer[MaxLen];
    std::size_t length = 0;
    bool overflown = false;

    constexpr BufferedLinearFieldSearch(It begin_, It end_)
        : begin(begin_), end(end_), buffer{}, length(0), overflown(false) {}

    // Simply buffer the character, no searching yet
    constexpr  bool step(char ch) {
        if (length < MaxLen) {
            buffer[length++] = ch;
            return true;
        }
        // Field name too long, won't match anything
        overflown = true;
        return false;
    }

    // Perform linear search through all fields
    constexpr It result() const {
        if(overflown) return end;
        std::string_view key(buffer, length);
        for (It it = begin; it != end; ++it) {
            if (it->name == key) {
                return it;
            }
        }
        return end; // not found
    }
    // Reset for next key
    constexpr void reset() {
        length = 0;
        overflown = false;
    }
    constexpr char*  get_buffer() {return buffer;}
    constexpr std::size_t & current_length() {
        return length;
    }
    constexpr void set_overflow(){overflown = true;}
};

// Adapter that selects strategy at compile time based on field count and max length
template<bool useBinarySearch, std::size_t MaxFieldLen>
struct AdaptiveStringSearch {
    using It = const StringDescr*;

    static_assert(!useBinarySearch, "DOn't");
    static constexpr bool useLinear = !useBinarySearch;

    using SearchImpl = std::conditional_t<
        useLinear,
        BufferedLinearFieldSearch<MaxFieldLen>,
        IncrementalBinaryFieldSearch
        >;

    SearchImpl impl;

    constexpr AdaptiveStringSearch(It begin, It end)
        : impl(begin, end) {}

    constexpr  bool step(char ch) {
        return impl.step(ch);
    }

    constexpr It result() const {
        return impl.result();
    }
    constexpr void reset() {
        impl.reset();
    }
    constexpr char*  buffer() {
        return impl.get_buffer();
    }
    constexpr std::size_t & current_length() {
        return impl.current_length();
    }
    constexpr void set_overflow(){impl.set_overflow();}

};


}
}
