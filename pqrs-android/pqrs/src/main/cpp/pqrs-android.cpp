#include <pqrs/qr_easy_pipeline.h>
#include <xtensor/xadapt.hpp>
#include <jni.h>
#include <android/log.h>
#include <json11.hpp>

#define TAG "pqrs"

extern "C"
JNIEXPORT void JNICALL
Java_me_dcnick3_pqrs_PqrsModule_printVersion(JNIEnv *env, jobject thiz) {
    __android_log_print(ANDROID_LOG_VERBOSE, TAG, "pqrs library initialized");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_me_dcnick3_pqrs_PqrsModule_scanGrayscale(JNIEnv *env, jobject thiz,
                                           jobject buffer,
                                           jint width,
                                           jint height,
                                           jint pixel_stride,
                                           jint row_stride
) {
    auto const* ptr = reinterpret_cast<std::uint8_t const*>(env->GetDirectBufferAddress(buffer));
    auto size = static_cast<std::size_t>(env->GetDirectBufferCapacity(buffer));

    assert(ptr != nullptr && "The passed buffer is not direct, GetDirectBufferAddress returned NULL");
    assert(pixel_stride == 1);
    assert(pixel_stride * row_stride * height == size);

    std::array<std::size_t, 2> shape = { static_cast<std::size_t>(height), static_cast<std::size_t>(width) };
    std::array<std::size_t, 2> strides = { static_cast<std::size_t>(row_stride), static_cast<std::size_t>(pixel_stride) };

    __android_log_print(ANDROID_LOG_VERBOSE, TAG,
                        "scanning image %p %zu {%zu x %zu}", ptr, size, shape[1], shape[0]
    );

    auto a = xt::adapt(ptr, size, xt::no_ownership(), shape, strides);

    pqrs::gray_u8 b = a;

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

        res_qrs.push_back(obj);
    }

    json11::Json::object res;
    res.emplace("qrs", res_qrs);
    res.emplace("finders", res_finders);

    auto res_str = json11::Json(res).dump();

    return env->NewStringUTF(res_str.c_str());
}