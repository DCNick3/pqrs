//
// Created by dcnick3 on 1/28/21.
//

#pragma once

namespace pqrs {
    class direction8 {
    public:
        inline direction8 operator+(int v) const {
            return direction8((_value + v) % 8);
        }

        inline direction8 operator-(int v) const {
            return *this + (-v);
        }

        inline direction8 operator++() {
            direction8 r = *this;
            *this = *this + 1;
            return r;
        }

        inline direction8 operator--() {
            direction8 r = *this;
            *this = *this - 1;
            return r;
        }

        inline bool operator==(direction8 o) const { return _value == o._value; }
        inline bool operator!=(direction8 o) const { return _value != o._value; }

        //inline int v() const { return _value; }

        static const direction8 right;
        static const direction8 bottomright;
        static const direction8 bottom;
        static const direction8 bottomleft;
        static const direction8 left;
        static const direction8 topleft;
        static const direction8 top;
        static const direction8 topright;


    private:
        constexpr inline explicit direction8(int value) : _value(value) {}

        int _value;
    };

    inline constexpr direction8 direction8::right = direction8(0);
    inline constexpr direction8 direction8::bottomright = direction8(1);
    inline constexpr direction8 direction8::bottom = direction8(2);
    inline constexpr direction8 direction8::bottomleft = direction8(3);
    inline constexpr direction8 direction8::left = direction8(4);
    inline constexpr direction8 direction8::topleft = direction8(5);
    inline constexpr direction8 direction8::top = direction8(6);
    inline constexpr direction8 direction8::topright = direction8(7);
}