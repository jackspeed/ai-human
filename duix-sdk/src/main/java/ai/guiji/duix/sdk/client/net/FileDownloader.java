package ai.guiji.duix.sdk.client.net;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

import ai.guiji.duix.sdk.client.util.Logger;


public class FileDownloader {

    private String url;
    private String path;

    private Callback callback;

    public FileDownloader(String url, String path, Callback callback){
        this.url = url;
        this.path = path;
        this.callback = callback;
    }

    public boolean download(){
        try {
            URL httpUrl = new URL(url);
            HttpURLConnection conn = (HttpURLConnection) httpUrl.openConnection();
            conn.setConnectTimeout(15000);
            conn.setReadTimeout(15000);
            long contentLength = conn.getContentLengthLong();
            conn.connect();
            int httpCode = conn.getResponseCode();//获取HTTP状态码
            if (httpCode == HttpURLConnection.HTTP_OK) {
                File tmpFile = new File(path + ".tmp");
                File parent = tmpFile.getParentFile();
                if (parent != null && !parent.exists()) {
                    if (!parent.mkdirs()) {
                        return false;
                    }
                }
                if (tmpFile.exists()) {
                    tmpFile.delete();
                }
                FileOutputStream fileOutputStream = new FileOutputStream(tmpFile);
                long downloadLength = 0;
                int len;
                byte[] data = new byte[1024];
                InputStream is = conn.getInputStream();
                while ((len = is.read(data)) != -1) {
                    fileOutputStream.write(data, 0, len);
                    downloadLength += len;
                    if (callback != null){
                        callback.onProgress(downloadLength , contentLength);
                    }
                }
                fileOutputStream.flush();
                is.close();
                fileOutputStream.close();
                File target = new File(path);
                if (tmpFile.renameTo(target)) {
                    return true;
                }
            }
        } catch (Exception e){
            Logger.d("download error:" + e);
        }
        return false;
    }

    public interface Callback {
        void onProgress(long current, long total);
    }

}
