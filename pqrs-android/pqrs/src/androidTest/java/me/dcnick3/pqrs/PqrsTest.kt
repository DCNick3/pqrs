package me.dcnick3.pqrs

import android.graphics.BitmapFactory
import android.util.Log
import junit.framework.TestCase.assertEquals
import org.junit.Test
import java.nio.ByteBuffer


class PqrsTest {
    private fun testImage(resourceName: String) {
        val classLoader: ClassLoader = PqrsTest::class.java.getClassLoader() as ClassLoader
        val stream = classLoader.getResourceAsStream(resourceName)
        val bitmap = BitmapFactory.decodeStream(stream)

        val bytes: Int = bitmap.width * bitmap.height;
        val buffer = ByteBuffer.allocateDirect(bytes)
        for (j in 0 until bitmap.height)
            for (i in 0 until bitmap.width) {
                val p = bitmap.getPixel(i, j)
                val R = p shr 16 and 0xff
                val G = p shr 8 and 0xff
                val B = p and 0xff

                val pixel = ((R + G + B) / 3).toByte()
                buffer.put(j * bitmap.width + i, pixel);
            }

        val result = PqrsScanner().scanGrayscale(buffer, bitmap.width, bitmap.height, 1, bitmap.width);

        Log.v("pqrs-android", "Result: $result")

        assertEquals(result.qrs.size, 1)

        val qr = result.qrs[0]

        Log.v("pqrs-android", "Scanned QR content: " + qr.content);
    }

    @Test
    fun testPqrsScansQr1() {
        testImage("basic_testcases/1.png")
    }
    @Test
    fun testPqrsScansQr2() {
        testImage("basic_testcases/2.png")
    }
    @Test
    fun testPqrsScansQr3() {
        testImage("basic_testcases/3.png")
    }
    @Test
    fun testPqrsScansQr4() {
        testImage("basic_testcases/4.png")
    }
    @Test
    fun testPqrsScansQr5() {
        testImage("basic_testcases/5.png")
    }
    @Test
    fun testPqrsScansQr6() {
        testImage("basic_testcases/6.png")
    }
}