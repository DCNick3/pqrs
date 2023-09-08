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
        val result = module.scanGrayscale(buffer, width, height, pixelStride, rowStride);

        val moshi: Moshi = Moshi.Builder()
            .add(VectorAdapter())
            .build()

        val jsonAdapter: JsonAdapter<ScanResult> = moshi.adapter(ScanResult::class.java).nonNull()

        return jsonAdapter.fromJson(result) as ScanResult
    }
}