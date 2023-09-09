package me.dcnick3.pqrs

import java.nio.ByteBuffer

internal object PqrsModule {
    init {
        System.loadLibrary("pqrs-android")
        this.printVersion()
    }

    private external fun printVersion()
    external fun scanGrayscale(buffer: ByteBuffer, bufferSize: Int, width: Int, height: Int, pixelStride: Int, rowStride: Int): String;
}