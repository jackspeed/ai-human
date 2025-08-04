package ai.guiji.duix.sdk.client;

import android.content.Context;
import android.text.TextUtils;

import java.io.File;

import ai.guiji.duix.sdk.client.net.DownloadZipService;


public class VirtualModelUtil {

    public static boolean checkBaseConfig(Context context){
        String duixDir = context.getExternalFilesDir("duix").getAbsolutePath();
        File baseDir = new File(duixDir + "/model", "gj_dh_res");
        File baseTag = new File(duixDir + "/model/tmp", "gj_dh_res");
        return baseDir.exists() && baseTag.exists();
    }

    public static boolean checkModel(Context context, String name){
        if (!TextUtils.isEmpty(name)){
            String duixDir = context.getExternalFilesDir("duix").getAbsolutePath();
            if (name.startsWith("https://") || name.startsWith("http://")){
                String dirName = "";
                try {
                    dirName = name.substring(name.lastIndexOf("/") + 1).replace(".zip", "");
                }catch (Exception ignore){
                }
                if (!TextUtils.isEmpty(dirName)){
                    File modelDir = new File(duixDir + "/model", dirName);
                    File modelTag = new File(duixDir + "/model/tmp", dirName);
                    return modelDir.exists() && modelTag.exists();
                } else {
                    return false;
                }
            } else {
                File modelDir = new File(duixDir + "/model", name);
                File modelTag = new File(duixDir + "/model/tmp", name);
                return modelDir.exists() && modelTag.exists();
            }
        } else {
            return false;
        }
    }

    public static void baseConfigDownload(Context context, ModelDownloadCallback callback){
        String url = Constant.BASE_DOWNLOAD_URL;
        baseConfigDownload(context, url, callback);
    }
    /**
     * 基础配置文件下载
     */
    public static void baseConfigDownload(Context context, String url, ModelDownloadCallback callback){
        String duixDir = context.getExternalFilesDir("duix").getAbsolutePath();
        File baseDir = new File(duixDir + "/model", "gj_dh_res");
        DownloadZipService.downloadAndUnzip(context, url, baseDir, new DownloadZipService.Callback() {
            @Override
            public void onDownloadProgress(long current, long total) {
                if (callback != null){
                    callback.onDownloadProgress(url, current, total);
                }
            }

            @Override
            public void onUnzipProgress(long current, long total) {
                if (callback != null){
                    callback.onUnzipProgress(url, current, total);
                }
            }

            @Override
            public void onComplete(File baseDirFile) {
                // init model
                if (callback != null){
                    callback.onDownloadComplete(url, baseDirFile);
                }
            }

            @Override
            public void onError(int code, String msg) {
                if (callback != null){
                    callback.onDownloadFail(url, code, msg);
                }
            }
        }, true);
    }

    /**
     * 模型文件下载
     */
    public static void modelDownload(Context context, String modelUrl, ModelDownloadCallback callback){
        String duixDir = context.getExternalFilesDir("duix").getAbsolutePath();
        if (!TextUtils.isEmpty(modelUrl) && (modelUrl.startsWith("https://") || modelUrl.startsWith("http://"))){
            String dirName = "";
            try {
                dirName = modelUrl.substring(modelUrl.lastIndexOf("/") + 1).replace(".zip", "");
            }catch (Exception ignore){
            }
            if (!TextUtils.isEmpty(dirName)){
                File modelDir = new File(duixDir + "/model", dirName);
                // 下载模型文件
                DownloadZipService.downloadAndUnzip(context, modelUrl, modelDir, new DownloadZipService.Callback() {
                    @Override
                    public void onDownloadProgress(long current, long total) {
                        if (callback != null){
                            callback.onDownloadProgress(modelUrl, current, total);
                        }
                    }

                    @Override
                    public void onUnzipProgress(long current, long total) {
                        if (callback != null){
                            callback.onUnzipProgress(modelUrl, current, total);
                        }
                    }

                    @Override
                    public void onComplete(File modelFile) {
                        // init model
                        if (callback != null){
                            callback.onDownloadComplete(modelUrl, modelFile);
                        }
                    }

                    @Override
                    public void onError(int code, String msg) {
                        if (callback != null){
                            callback.onDownloadFail(modelUrl, code, msg);
                        }
                    }
                }, true);
            } else {
                if (callback != null){
                    callback.onDownloadFail(modelUrl, -1004, "Illegal model url[" + modelUrl + "]");
                }
            }
        } else {
            if (callback != null){
                callback.onDownloadFail(modelUrl, -1003, "Illegal download url[" + modelUrl + "]");
            }
        }
    }

    public interface ModelDownloadCallback {

        void onDownloadProgress(String url, long current, long total);

        void onUnzipProgress(String url, long current, long total);

        void onDownloadComplete(String url, File dir);

        /**
         * -1000    Compressed file download failed
         * -1001    An exception occurred while decompressing the file
         * -1002    Target folder not found
         * -1003    Illegal download url
         * -1004    Illegal model url
         * -1005    Service not initialized
         */
        void onDownloadFail(String url, int code, String msg);
    }
}
