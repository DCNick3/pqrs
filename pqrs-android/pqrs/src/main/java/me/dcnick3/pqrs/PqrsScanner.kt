package me.dcnick3.pqrs

import com.squareup.moshi.JsonAdapter
import com.squareup.moshi.Moshi
import com.squareup.moshi.addAdapter
import me.dcnick3.pqrs.model.ScanResult
import me.dcnick3.pqrs.model.VectorAdapter
import java.nio.ByteBuffer

public class PqrsScanner {
    private val module = PqrsModule;

    fun scanGrayscale(buffer: ByteBuffer, width: Int, height: Int, pixelStride: Int, rowStride: Int): ScanResult {
        var bufferSize = buffer.limit()
        // HACK HACK HACK
        // camerax does a strange thing where it cuts of the excess stride in the last row
        // this make the image size shorter by (row_stride - width) * pixel_stride
        // so we need to add it back
        bufferSize += (rowStride - width) * pixelStride

        // the asserts are repeated in C++ code, but it's better to check them here too
        assert(buffer.isDirect)
        assert(pixelStride == 1)
        assert(bufferSize == pixelStride * rowStride * height)

        val result = module.scanGrayscale(buffer, bufferSize, width, height, pixelStride, rowStride);

        val moshi: Moshi = Moshi.Builder()
            .add(VectorAdapter())
            .build()

        val jsonAdapter: JsonAdapter<ScanResult> = moshi.adapter(ScanResult::class.java).nonNull()

        return jsonAdapter.fromJson(result) as ScanResult
    }
}