package me.dcnick3.pqrs.model

import com.squareup.moshi.JsonClass

@JsonClass(generateAdapter = true)
data class QrCode(
    val content: String,
    val top_left: Vector,
    val top_right: Vector,
    val bottom_left: Vector,
    val bottom_right: Vector,
)