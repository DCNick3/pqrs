plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    id("com.google.devtools.ksp").version("1.8.22-1.0.11")
    `maven-publish`
    signing
    id("me.qoomon.git-versioning").version("6.4.1")
}

version = "0.0.0-SNAPSHOT"
gitVersioning.apply {
    refs {
        branch(".+") { version = "\${ref}-SNAPSHOT" }
        tag("v(?<version>.*)") { version = "\${ref.version}" }
    }
}


android {
    namespace = "me.dcnick3.pqrs"
    compileSdk = 33

    defaultConfig {
        consumerProguardFiles("proguard-rules.pro")
        externalNativeBuild {
            cmake {
                cppFlags += ""
            }
        }
        minSdk = 9
        testInstrumentationRunner = "android.support.test.runner.AndroidJUnitRunner"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    kotlinOptions {
        jvmTarget = "1.8"
    }
    kotlin {
        jvmToolchain(8)
    }
    externalNativeBuild {
        cmake {
            path = file("CMakeLists.txt")
            version = "3.22.1"
        }
    }
    buildTypes {
        getByName("debug") {
            isJniDebuggable = true
        }
    }
}

dependencies {
    implementation("com.squareup.moshi:moshi:1.15.0")
    ksp("com.squareup.moshi:moshi-kotlin-codegen:1.15.0")
    androidTestImplementation("junit:junit:4.13.2")
    androidTestImplementation("com.android.support.test:runner:1.0.2")
}

publishing {
    repositories {
        if (version.toString().endsWith("SNAPSHOT")) {
            maven("https://s01.oss.sonatype.org/content/repositories/snapshots/") {
                name = "sonatypeSnapshotRepository"
                credentials(PasswordCredentials::class)
            }
        } else {
            maven("https://s01.oss.sonatype.org/service/local/staging/deploy/maven2/") {
                name = "sonatypeReleaseRepository"
                credentials(PasswordCredentials::class)
            }
        }
    }

    publications {
        register<MavenPublication>("release") {
            groupId = "me.dcnick3.pqrs"
            artifactId = "pqrs-android"

            pom {
                name.set("pqrs")
                description.set("Portable QR Scanning library ")
                url.set("https://github.com/DCNick3/pqrs")
                licenses {
                    license {
                        name.set("Mozilla Public License, Version 2.0")
                        url.set("https://www.mozilla.org/en-US/MPL/2.0/")
                    }
                }
                developers {
                    developer {
                        id.set("dcnick3")
                        name.set("⭐️NINIKA⭐️")
                        email.set("moslike6@gmail.com")
                    }
                }
                scm {
                    connection.set("scm:git:git://github.com/DCNick3/pqrs.git")
                    developerConnection.set("scm:git:ssh://github.com/DCNick3/pqrs.git")
                    url.set("https://github.com/DCNick3/pqrs")
                }
            }

            afterEvaluate {
                from(components["release"])
            }
        }
    }
}

signing {
    sign(publishing.publications["release"])
}

