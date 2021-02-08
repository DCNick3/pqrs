//
// Created by dcnick3 on 1/28/21.
//

#pragma once

namespace pqrs {
    class direction4 {
    public:
        inline direction4 operator+(int v) const {
            return direction4((_value + v) % 4);
        }

        inline direction4 operator-(int v) const {
            return *this + (-v);
        }

        inline direction4 operator++() {
            direction4 r = *this;
            *this = *this + 1;
            return r;
        }

        inline direction4 operator--() {
            direction4 r = *this;
            *this = *this - 1;
            return r;
        }

        inline bool operator==(direction4 o) const { return _value == o._value; }
        inline bool operator!=(direction4 o) const { return _value != o._value; }

        [[nodiscard]] inline int v() const { return _value; }

        static const direction4 right;
        static const direction4 bottom;
        static const direction4 left;
        static const direction4 top;

        constexpr inline explicit direction4(int value) : _value(value) {}
    private:

        int _value;
    };

    inline constexpr direction4 direction4::right = direction4(0);
    inline constexpr direction4 direction4::bottom = direction4(1);
    inline constexpr direction4 direction4::left = direction4(2);
    inline constexpr direction4 direction4::top = direction4(3);
}