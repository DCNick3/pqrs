package me.dcnick3.pqrs.model

import com.squareup.moshi.JsonClass

@JsonClass(generateAdapter = true)
data class ScanResult (
    val finders: List<Vector>,
    val qrs: List<QrCode>,
)