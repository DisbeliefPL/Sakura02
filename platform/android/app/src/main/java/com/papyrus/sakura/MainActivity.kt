package com.papyrus.sakura

import android.icu.number.IntegerWidth
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import androidx.core.content.pm.ShortcutInfoCompat
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.util.Log
import com.papyrus.sakura.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream
import java.lang.Thread

class MainActivity : AppCompatActivity() {

    companion object
    {
        init
        {
            //使用sakura动态库
            System.loadLibrary("sakura_android")
        }
    }

    external fun setSurface(surface: Surface?)
    external fun loadModel(paramPath: String, binPath: String)
    external fun drawPic(imagePath: String)

    @Volatile
    private var isModelLoaded = false


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        supportActionBar?.hide()

        val surfaceView = findViewById<SurfaceView>(R.id.surfaceView)

        fun copyAssetsToFile(assetName: String, targetFile: File)
        {
            assets.open(assetName).use { input ->
                FileOutputStream(targetFile).use { output ->
                    input.copyTo(output)
                }
            }
        }
        val paramFile = File(cacheDir, "realesr-animevideov3-x4.param")
        val binFile = File(cacheDir, "realesr-animevideov3-x4.bin")
        val imgFile = File(cacheDir, "test.jpg")
        Thread {
            copyAssetsToFile("models/realesr-animevideov3-x4.param", paramFile)
            copyAssetsToFile("models/realesr-animevideov3-x4.bin", binFile)
            copyAssetsToFile("TestImg/test.png", imgFile)
            loadModel(paramFile.absolutePath, binFile.absolutePath)
            Log.e("Sakura", "模型加载完成")
            isModelLoaded = true
        }.start()


        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                Log.e("Sakura", "Surface创建准备传jni")
                Thread{
                    setSurface(holder.surface)
                    Log.e("Sakura", "Surface加载完成")
                    while (!isModelLoaded){
                        Thread.sleep(100)
                    }
                    drawPic(imgFile.absolutePath)
                    Log.e("Sakura", "成功绘制")
                }.start()
            }
            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int){}
            override fun surfaceDestroyed(holder: SurfaceHolder) {
                Log.e("Sakura","销毁Surface,jni释放")
                setSurface(null)
            }
        })

    }

}