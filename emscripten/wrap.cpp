//
// Created by dcnick3 on 1/25/21.
//

#include <pqrs/ppm.h>
#include <pqrs/grayscale.h>
#include <pqrs/binarization.h>

#include <emscripten/bind.h>

#include <string_view>

std::string getExceptionMessage(intptr_t exceptionPtr) {
    return std::string(reinterpret_cast<std::exception *>(exceptionPtr)->what());
}

static std::vector<char> buffer;
void resize_buffer(std::size_t size) {
    buffer.resize(size);
}

emscripten::val get_buffer() {
    return emscripten::val(
        emscripten::typed_memory_view(buffer.size(), buffer.data())
    );
}

std::string binarize_ppm(std::string const& ppm) {

    auto img = pqrs::load_ppm(ppm);
    auto gray = pqrs::grayscale(img);
    auto bin = pqrs::binarize(gray, 0.01);

    auto res = pqrs::save_ppm(bin);

    std::cout << "output size: " << res.size() << std::endl;

    return res;
}

EMSCRIPTEN_BINDINGS(pqrs) {
    //emscripten::function("resize_buffer", &resize_buffer);
    //emscripten::function("get_buffer", &get_buffer);
    emscripten::function("binarize_ppm", &binarize_ppm);
    emscripten::function("get_exception_message", &getExceptionMessage);
}
