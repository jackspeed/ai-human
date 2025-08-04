package com.ai.human;


import static ai.guiji.duix.sdk.client.Constant.CALLBACK_EVENT_INIT_ERROR;
import static ai.guiji.duix.sdk.client.Constant.CALLBACK_EVENT_INIT_READY;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.ai.human.databinding.ActivityAiHumanBinding;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;

import ai.guiji.duix.sdk.client.Callback;
import ai.guiji.duix.sdk.client.DUIX;
import ai.guiji.duix.sdk.client.render.DUIXRenderer;

public class AiHumanActivity extends AppCompatActivity {

    private @NonNull ActivityAiHumanBinding binding;
    private DUIX mDUIXRender;
    private final int GL_CONTEXT_VERSION = 2;

    public static void start(Activity mainActivity) {
        Intent intent = new Intent(mainActivity, AiHumanActivity.class);
        mainActivity.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initView();
        initData();
        initListener();
    }

    public void initView() {
        binding = ActivityAiHumanBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
    }


    public void initData() {
        DUIXRenderer sink = new DUIXRenderer(getApplicationContext(), binding.glTextureView);
        binding.glTextureView.setEGLContextClientVersion(GL_CONTEXT_VERSION);
        binding.glTextureView.setEGLConfigChooser(8, 8, 8, 8, 16, 0);// 透明
        binding.glTextureView.setOpaque(false);         // 透明
        binding.glTextureView.setRenderer(sink);
        binding.glTextureView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);     // 一定要在设置完Render之后再调用
        mDUIXRender = new DUIX(getApplicationContext(), Consts.mModelPath, sink, new Callback() {
            @Override
            public void onEvent(String event, String msg, Object info) {
                switch (event) {
                    case CALLBACK_EVENT_INIT_READY:
                        Log.d("初始化成功 ready", msg);
                        break;
                    case CALLBACK_EVENT_INIT_ERROR:
                        Log.e("初始化失败 error", msg);
                        break;

                }
            }
        });
        mDUIXRender.init();
    }


    @SuppressLint("ClickableViewAccessibility")
    public void initListener() {
        binding.btnPlayPCM.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                playPCMStream();
            }
        });
        binding.btnPlayWAV.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                playWAVFile();
            }
        });
        binding.btnRandomMotion.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //mDUIXRender.startRandomMotion(true);
                mDUIXRender.startMotion("打招呼", true);
            }
        });
        binding.btnStopPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mDUIXRender.stopAudio();
            }
        });
    }

    private void playPCMStream() {
        new Thread(() -> {
            if (mDUIXRender != null) {
                mDUIXRender.startPush();
            }

            try (InputStream inputStream = getAssets().open("pcm/2.pcm")) {
                byte[] buffer = new byte[320];
                int length;

                while ((length = inputStream.read(buffer)) > 0) {
                    byte[] data = Arrays.copyOfRange(buffer, 0, length);
                    if (mDUIXRender != null) {
                        mDUIXRender.pushPcm(data);
                    }
                }

                if (mDUIXRender != null) {
                    mDUIXRender.stopPush();
                }

            } catch (IOException e) {
                e.printStackTrace();
            }
        }).start();
    }

    private void playWAVFile() {
        new Thread(() -> {
            String wavName = "1.wav";
            File wavFile = new File(getApplicationContext().getExternalCacheDir(), wavName);

            if (!wavFile.exists()) {
                try {
                    InputStream inputStream = getAssets().open("wav/" + wavName);

                    File cacheDir = getApplicationContext().getExternalCacheDir();
                    if (cacheDir != null && !cacheDir.exists()) {
                        cacheDir.mkdirs();
                    }

                    FileOutputStream out = new FileOutputStream(wavFile);
                    byte[] buffer = new byte[1024];
                    int length;

                    while ((length = inputStream.read(buffer)) > 0) {
                        out.write(buffer, 0, length);
                    }

                    out.close();
                    inputStream.close();

                } catch (IOException e) {
                    e.printStackTrace();
                    return;
                }
            }

            if (mDUIXRender != null) {
                mDUIXRender.playAudio(wavFile.getAbsolutePath());
            }
        }).start();
    }


    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }


}