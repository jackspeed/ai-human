package com.ai.human;


import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.ai.human.databinding.ActivityLauncherBinding;
import com.blankj.utilcode.util.ToastUtils;
import com.hjq.permissions.OnPermissionCallback;
import com.hjq.permissions.XXPermissions;
import com.hjq.permissions.permission.PermissionLists;
import com.hjq.permissions.permission.base.IPermission;

import java.io.File;
import java.util.List;

import ai.guiji.duix.sdk.client.VirtualModelUtil;

public class LauncherActivity extends AppCompatActivity {

    private static final String TAG = "LauncherActivity";
    private @NonNull ActivityLauncherBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initView();
        initData();
        initListener();
    }

    public void initView() {
        binding = ActivityLauncherBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
    }

    @SuppressLint("WrongConstant")
    public void initData() {
        checkAndRequirePermission();
    }

    private final VirtualModelUtil.ModelDownloadCallback modelDownloadCallback = new VirtualModelUtil.ModelDownloadCallback() {
        @Override
        public void onDownloadProgress(String url, long current, long total) {
            int progress = (int) (current * 100 / total);
            Log.d(TAG, "onDownloadProgress:" + url + ",progress: " + progress);
        }

        @Override
        public void onUnzipProgress(String url, long current, long total) {
            int progress = (int) (current * 100 / total);
            Log.d(TAG, "onUnzipProgress:" + url + ",progress: " + progress);
        }

        @Override
        public void onDownloadComplete(String url, File dir) {
            Log.d(TAG, "onDownloadComplete:" + url + ".File: " + dir.toString());
        }

        @Override
        public void onDownloadFail(String url, int code, String msg) {
            Log.d(TAG, "onDownloadFail:" + url + ",code:" + code + ",msg: " + msg);
        }
    };


    public void initListener() {
        binding.bntChooseMode.setOnClickListener(v -> {
            boolean baseConfig = VirtualModelUtil.checkBaseConfig(getApplicationContext());
            if (!baseConfig) {
                VirtualModelUtil.baseConfigDownload(getApplicationContext(),Consts. mBaseConfigUrl, modelDownloadCallback);
            }
            // 模型下载
            boolean checkModel = VirtualModelUtil.checkModel(getApplicationContext(), Consts. mModelPath);
            if (!checkModel) {
                VirtualModelUtil.modelDownload(getApplicationContext(),Consts.  mModelPath, modelDownloadCallback);
            }
            if (baseConfig && checkModel) {
                AiHumanActivity.start(LauncherActivity.this);
            }
        });
    }

    private final OnPermissionCallback callback = new OnPermissionCallback() {

        @Override
        public void onGranted(@NonNull List<IPermission> permissions, boolean allGranted) {
            if (!allGranted) {
                ToastUtils.showLong("获取部分权限成功，但部分权限未正常授予");
                return;
            }
            ToastUtils.showLong("获取权限成功");
        }

        @Override
        public void onDenied(@NonNull List<IPermission> permissions, boolean doNotAskAgain) {
            if (doNotAskAgain) {
                ToastUtils.showLong("被永久拒绝授权，请手动授予录音和日历权限");
                // 如果是被永久拒绝就跳转到应用权限系统设置页面
                XXPermissions.startPermissionActivity(LauncherActivity.this, permissions);
            } else {
                ToastUtils.showLong("获取权限失败");
            }
        }
    };

    private void checkAndRequirePermission() {
        IPermission[] permissions = getPermissionsForSdk();

        if (XXPermissions.isGrantedPermissions(this, permissions)) {
            ToastUtils.showLong("获取权限成功");
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            XXPermissions.with(this).permissions(permissions).request(callback);

        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                ToastUtils.showLong("请手动授予“管理所有文件”权限");
                Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                startActivity(intent);
                return;
            }
            // 这里继续请求 camera 权限
            XXPermissions.with(this)
                    .permission(PermissionLists.getCameraPermission())
                    .request(callback);

        } else {
            XXPermissions.with(this).permissions(permissions).request(callback);
        }
    }

    private static IPermission[] getPermissionsForSdk() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return new IPermission[]{
                    PermissionLists.getReadMediaImagesPermission(),
                    PermissionLists.getReadMediaVideoPermission(),
                    PermissionLists.getRecordAudioPermission(),
                    PermissionLists.getCameraPermission()
            };
        } else {
            return new IPermission[]{
                    PermissionLists.getReadExternalStoragePermission(),
                    PermissionLists.getWriteExternalStoragePermission(),
                    PermissionLists.getRecordAudioPermission(),
                    PermissionLists.getCameraPermission()
            };
        }
    }

}