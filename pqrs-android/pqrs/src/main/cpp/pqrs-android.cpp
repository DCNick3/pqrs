// we ALWAYS need asserts
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <pqrs/qr_easy_pipeline.h>
#include <xtensor/xadapt.hpp>
#include <xtensor/xview.hpp>
#include <jni.h>
#include <android/log.h>
#include <json11.hpp>

#define TAG "pqrs"

//#define assert()

extern "C"
JNIEXPORT void JNICALL
Java_me_dcnick3_pqrs_PqrsModule_printVersion(JNIEnv *env, jobject thiz) {
    __android_log_print(ANDROID_LOG_VERBOSE, TAG, "pqrs library initialized");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_me_dcnick3_pqrs_PqrsModule_scanGrayscale(JNIEnv *env, jobject thiz,
                                           jobject buffer,
                                           jint buffer_size,
                                           jint width,
                                           jint height,
                                           jint pixel_stride,
                                           jint row_stride
) {
    auto const* ptr = reinterpret_cast<std::uint8_t const*>(env->GetDirectBufferAddress(buffer));
    auto size = static_cast<std::size_t>(buffer_size);

    assert(ptr != nullptr && "The passed buffer is not direct, GetDirectBufferAddress returned NULL");
    assert(pixel_stride == 1 && "The passed buffer does not use 1 pixel stride");
    assert(pixel_stride * row_stride * height == size && "The passed buffer size does not match the passed dimensions");

    std::array<std::size_t, 2> shape = { static_cast<std::size_t>(height), static_cast<std::size_t>(width) };
    std::array<std::size_t, 2> init_shape = { static_cast<std::size_t>(height), static_cast<std::size_t>(row_stride) };
    std::array<std::size_t, 2> strides = { static_cast<std::size_t>(row_stride), static_cast<std::size_t>(pixel_stride) };

    __android_log_print(ANDROID_LOG_VERBOSE, TAG,
                        "scanning image %p %zu {%zu x %zu} / [%zu, %zu]", ptr, size, shape[1], shape[0], strides[0], strides[1]
    );

    pqrs::gray_u8 b(shape);

    // I tried fucking around with views, but it seems futile...
    // it's just easier to copy it manually lol
    for (int j = 0; j < height; ++j) {
        memcpy(
                b.data() + j * width * pixel_stride,
                ptr + j * row_stride * pixel_stride,
                width * pixel_stride
        );
    }

    auto result = pqrs::easy_scan_qr_codes(b);


    json11::Json::array res_qrs;
    json11::Json::array res_finders;

    auto vec = [](pqrs::vector2d vec) -> json11::Json {
        return json11::Json::array { vec.x(), vec.y() };
    };

    for (auto const& finder : result.finder_patterns) {
        res_finders.push_back(vec(finder.center));
    }

    for (auto const& q : result.qrs) {
        json11::Json::object obj;

        auto const& content = q._decoded_content;

        obj.emplace("content", content);
        obj.emplace("bottom_right", vec(q.bottom_right()));
        obj.emplace("bottom_left", vec(q.bottom_left()));
        obj.emplace("top_left", vec(q.top_left()));
        obj.emplace("top_right", vec(q.top_right()));

        res_qrs.emplace_back(obj);
    }

    json11::Json::object res;
    res.emplace("qrs", res_qrs);
    res.emplace("finders", res_finders);

    auto res_str = json11::Json(res).dump();

    return env->NewStringUTF(res_str.c_str());
}