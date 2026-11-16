plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.papyrus.sakura"
    ndkVersion = "30.0.16248370"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        applicationId = "com.papyrus.sakura"
        minSdk = 24
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters.add("arm64-v8a")
            debugSymbolLevel = "full"
        }

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17"
                cppFlags += "-O3"
            }
        }
    }

    buildTypes {
        release {
            optimization {
                enable = true
                packageScope = setOf("androidx.**", "kotlin.**", "kotlinx.**")
            }
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }


    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    buildFeatures {
        viewBinding = true
    }
}

dependencies {
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.constraintlayout)
    implementation(libs.androidx.core.ktx)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(libs.androidx.junit)
}

//注册copy任务 rootProject就是android目录
//直接构造对象以避开配置缓存检查
tasks.register<Copy>("copyModelsToAssets") {
    val projectRoot = File(rootProject.projectDir.parentFile.parentFile, "models")
    val assetsDir = File(projectDir, "src/main/assets/models")
    from(projectRoot)
    into(assetsDir)

    onlyIf { projectRoot.exists() }
}

//在run时挂载models
tasks.named("preBuild") {
    dependsOn("copyModelsToAssets")
}