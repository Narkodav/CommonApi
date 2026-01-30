#pragma once
#include "../Namespaces.h"

#include <type_traits>

//bit based flag array class
template <typename FlagBits>
    requires std::is_enum_v<FlagBits>
class Flag
{
private:
    using UnderlyingType = std::underlying_type_t<FlagBits>;
    UnderlyingType flags;

public:
    Flag() : flags(0) {}

    Flag(const Flag&) = default;
    Flag& operator=(const Flag&) = default;
    Flag(Flag&&) = default;
    Flag& operator=(Flag&&) = default;

    inline void set(FlagBits flag) {
        flags |= static_cast<UnderlyingType>(flag);
    }

    template <typename... FlagBits>
    inline void set(FlagBits... bits) {
        ((this->flags |= static_cast<UnderlyingType>(bits)), ...);
    }

    inline bool has(FlagBits flag) const {
        return (flags & static_cast<UnderlyingType>(flag)) == static_cast<UnderlyingType>(flag);
    }

    inline void clear() { flags = 0; }

    inline void clear(FlagBits flag) {
        flags &= ~static_cast<UnderlyingType>(flag);
    }

    inline void toggle(FlagBits flag) {
        flags ^= static_cast<UnderlyingType>(flag);
    }

    inline UnderlyingType raw() const {
        return flags;
    }
};

