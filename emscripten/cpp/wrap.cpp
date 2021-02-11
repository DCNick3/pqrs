//
// Created by dcnick3 on 1/25/21.
//

#include <pqrs/qr_easy_pipeline.h>

#include <xtensor/xadapt.hpp>

#include <emscripten/bind.h>

#include <json11.hpp>

std::string getExceptionMessage(intptr_t exceptionPtr) {
    return std::string(reinterpret_cast<std::exception *>(exceptionPtr)->what());
}



std::string scan_qr_codes(std::uint32_t data_ptr,
                                       std::size_t width, std::size_t height) {

    /* hacky hacks! */

    auto* data = reinterpret_cast<std::uint8_t const*>(data_ptr);

    std::array<std::size_t, 3> shape = { height, width, 4 };
    auto a = xt::adapt(data, width * height * 4, xt::no_ownership(), shape);
    // one copy here, one up in js
    // can we do better?
    pqrs::coloralpha_u8 b = a;

    auto qrs = pqrs::easy_scan_qr_codes(std::move(b));

    // serialize output
    // embind does not (adequately) support vectors, so just use json here

    json11::Json::array array;

    auto vec = [](pqrs::vector2d vec) -> json11::Json {
        return json11::Json::array { vec.x(), vec.y() };
    };

    for (auto const& q : qrs) {
        json11::Json::object obj;

        auto const& content = q._decoded_content;

        obj.emplace("content", content);
        obj.emplace("bottom_right", vec(q.bottom_right()));
        obj.emplace("bottom_left", vec(q.bottom_left()));
        obj.emplace("top_left", vec(q.top_left()));
        obj.emplace("top_right", vec(q.top_right()));

        array.push_back(obj);
    }

    return json11::Json(array).dump();
}

EMSCRIPTEN_BINDINGS(pqrs) {
    //emscripten::function("resize_buffer", &resize_buffer);
    //emscripten::function("get_buffer", &get_buffer);
    emscripten::function("scan_qr_codes", &scan_qr_codes, emscripten::allow_raw_pointers());
    emscripten::function("get_exception_message", &getExceptionMessage);
}
