package me.dcnick3.pqrs.model

import com.squareup.moshi.FromJson
import com.squareup.moshi.Moshi
import com.squareup.moshi.ToJson

data class Vector(
    val x: Float,
    val y: Float,
)

class VectorAdapter {
    @ToJson
    fun toJson(value: Vector): List<Float> {
        return listOf(value.x, value.y)
    }

    @FromJson
    fun fromJson(json: List<Float>): Vector {
        assert(json.count() == 2);

        return Vector(json[0], json[1])
    }
}