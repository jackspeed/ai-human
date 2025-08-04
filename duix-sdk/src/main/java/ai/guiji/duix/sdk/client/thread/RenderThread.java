package ai.guiji.duix.sdk.client.thread;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;


import java.io.File;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;
import java.util.concurrent.ConcurrentLinkedQueue;

import ai.guiji.duix.DuixNcnn;
import ai.guiji.duix.sdk.client.audio.AudioPlayer;
import ai.guiji.duix.sdk.client.bean.ImageFrame;
import ai.guiji.duix.sdk.client.loader.ModelInfo;
import ai.guiji.duix.sdk.client.loader.ModelInfoLoader;
import ai.guiji.duix.sdk.client.render.RenderSink;
import ai.guiji.duix.sdk.client.util.Logger;


public class RenderThread extends Thread {

    private static final int MSG_RENDER_STEP = 1;                   // 请求下一帧渲染
    private static final int MSG_STOP_RENDER = 2;                   // 停止渲染
    private static final int MSG_QUIT = 3;                          // 退出线程
    private static final int MSG_STOP_PUSH_AUDIO = 5;               // 停止音频推送
    private static final int MSG_STOP_PLAY_AUDIO = 6;               // 停止音频播放

    private static final int MSG_REQUIRE_MOTION = 7;                // 请求播放一个指定动作区间
    private static final int MSG_REQUIRE_MOTION_RANDOM = 8;         // 请求随机播放一个动作区间
    private static final int MSG_START_PUSH_AUDIO = 11;             // 启动音频推送
    private static final int MSG_PUSH_AUDIO = 12;                   // 推送播放音频


    private volatile boolean isRendering = false;                     // 为false时终止线程
    RenderHandler mHandler;                                 // 使用该处理器来调度线程的事件

    private final Object mReadyFence = new Object();        // 给isReady加一个对象锁

    private final Object mBnfFence = new Object();        // 给isReady加一个对象锁

    private final Context mContext;
    private DuixNcnn scrfdncnn;

    private final RenderCallback callback;

    private RenderSink mRenderSink;

    private ConcurrentLinkedQueue<ModelInfo.Frame> mPreviewQueue;       // 播放帧

    private boolean requireMotion = false;                  // 请求播放动作
    private ModelInfo.Region prepareActionRegion;           // 准备在静默节点或动作节点播放完播放的动作区间

    private ModelInfo mModelInfo;                           // 模型的全部信息都放在这里面
    private ByteBuffer rawBuffer;
    private ByteBuffer maskBuffer;
    private final File modelDir;

    private AudioPlayer audioPlayer;
    private long mCurrentBnfSession = -1;
    private long mLastBnfSession = -1;

    private float mVolume;

    private int scrfRst;
    private boolean isLip = false;      // 用于统计是否正在渲染口型

    private Reporter mReporter;

    public RenderThread(Context context, File modelDir, RenderSink renderSink, float volume, RenderCallback callback, Reporter reporter) {
        this.mContext = context;
        this.modelDir = modelDir;
        this.mRenderSink = renderSink;
        this.callback = callback;
        this.mReporter = reporter;
        this.mVolume  = volume;
    }

    public void setReporter(Reporter reporter){
        this.mReporter = reporter;
    }

    @Override
    public void run() {
        super.run();
        Looper.prepare();
        mHandler = new RenderHandler(this);
        mPreviewQueue = new ConcurrentLinkedQueue<>();
        audioPlayer = new AudioPlayer(new AudioPlayer.AudioPlayerCallback() {
            @Override
            public void onPlayStart() {
                callback.onPlayStart();
            }

            @Override
            public void onPlayEnd() {
                mCurrentBnfSession = -1;
                callback.onPlayEnd();
            }

            @Override
            public void onPlayError(int code, String message) {
                callback.onPlayError(code, message);
            }
        }, mVolume);

        scrfdncnn = new DuixNcnn();
        String duixDir = mContext.getExternalFilesDir("duix").getAbsolutePath();
        ModelInfo info = ModelInfoLoader.load(mContext, scrfdncnn, duixDir + "/model/gj_dh_res", modelDir.getAbsolutePath());
        if (info != null) {
            try {
                scrfdncnn.alloc(0, 20, info.getWidth(), info.getHeight());
                scrfdncnn.initPcmex(0,10,20,50,0);
                scrfdncnn.initMunet(info.getUnetparam(), info.getUnetbin(), info .getUnetmsk());
                scrfdncnn.initWenet(info.getWenetfn());
                mModelInfo = info;
                Logger.d("分辨率: " + mModelInfo.getWidth() + "x" + mModelInfo.getHeight());
                rawBuffer = ByteBuffer.allocate(mModelInfo.getWidth() * mModelInfo.getHeight() * 3);
                maskBuffer = ByteBuffer.allocate(mModelInfo.getWidth() * mModelInfo.getHeight() * 3);
                if (!mModelInfo.isHasMask()) {
                    // 用纯白填充mask
                    Arrays.fill(maskBuffer.array(), (byte) 255);
                }
                Logger.d("模型初始化完成");
                if (callback != null) {
                    callback.onInitResult(0, 0, mModelInfo.toString(), mModelInfo);
                }
            } catch (Exception e){
                if (callback != null) {
                    callback.onInitResult(-1002, -1001, "Model loading exception: " + e, null);
                }
            }
        } else {
            if (callback != null) {
                callback.onInitResult(-1002, -1000, "Model configuration read exception", null);
            }
        }

        synchronized (mReadyFence) {
            mReadyFence.notify();
        }
        isRendering = true;
        handleAudioStep();
        Looper.loop();
        synchronized (mBnfFence) {
            // 线程最后释放NCNN
            scrfdncnn.free(0);
        }
        Logger.d("NCNN释放");
        if (audioPlayer != null) {
            audioPlayer.release();
            audioPlayer = null;
        }
        synchronized (mReadyFence) {
            mHandler = null;
        }
    }

    public void setVolume(float volume){
        if (audioPlayer != null){
            audioPlayer.setVolume(volume);
        }
    }

    public void stopPreview() {
        if (mHandler != null) {
            mHandler.sendEmptyMessage(MSG_STOP_RENDER);
        }
    }

    public void startPush() {
        if (mHandler != null) {
            mHandler.sendEmptyMessage(MSG_START_PUSH_AUDIO);
        }
    }

    public void pushAudio(byte[] data){
        if (mHandler != null) {
            Message message = new Message();
            message.what = MSG_PUSH_AUDIO;
            message.obj = data;
            mHandler.sendMessage(message);
        }
    }

    public void stopPush() {
        if (mHandler != null) {
            mHandler.sendEmptyMessage(MSG_STOP_PUSH_AUDIO);
        }
    }

    public void stopPlayAudio(){
        if (mHandler != null) {
            mHandler.sendEmptyMessage(MSG_STOP_PLAY_AUDIO);
        }
    }

    public void requireMotion(String name, boolean now) {
        if (mHandler != null) {
            Message message = new Message();
            message.what = MSG_REQUIRE_MOTION;
            message.obj = name;
            message.arg1 = now ? 0 : 1;
            mHandler.sendMessage(message);
        }
    }

    public void requireRandomMotion(boolean now){
        if (mHandler != null) {
            Message message = new Message();
            message.what = MSG_REQUIRE_MOTION_RANDOM;
            message.arg1 = now ? 0 : 1;
            mHandler.sendMessage(message);
        }
    }

    private void handleAudioStep() {
        if (isRendering) {
            long useTime = renderStep();
            long delay = 40 - (useTime);
            if (delay < 0) {
                Logger.w("渲染耗时过高: " + (useTime) + "(>40ms)");
                delay = 0;
            }
            if (mHandler != null) {
                mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_RENDER_STEP), delay);
            }
        } else {
            if (mPreviewQueue != null) {
                mPreviewQueue.clear();
            }
            if (mHandler != null) {
                mHandler.sendEmptyMessage(MSG_QUIT);
            }
        }
    }

    private long renderStep() {
        long startTime = System.currentTimeMillis();
        ModelInfo.Frame frame;
        if (requireMotion) {
            // 收到动作的通知
            requireMotion = false;
            if (prepareActionRegion != null){
                mPreviewQueue.clear();
                Logger.d("发现想要播放的动作区间region: " + prepareActionRegion);
                mPreviewQueue.addAll(prepareActionRegion.frames);
            }
        }
        if (mPreviewQueue.isEmpty()) {
            // 先假设把静默的都加进来
            ModelInfo.Region silenceRegion = mModelInfo.getSilenceRegion();
            mPreviewQueue.addAll(silenceRegion.frames);
            List<ModelInfo.Frame> copiedList = new ArrayList<>(silenceRegion.frames);
            // 反向的也加进来
            Collections.reverse(copiedList);
            mPreviewQueue.addAll(copiedList);
        }
        frame = mPreviewQueue.poll();

        if (frame != null) {
            int readyCnt = scrfdncnn.readycnt(mCurrentBnfSession);
            if (readyCnt > 0 && audioPlayer != null){
                if (mLastBnfSession != mCurrentBnfSession){
                    mLastBnfSession = mCurrentBnfSession;
                    // 通知新的一段读取完成了,准备播放
                    audioPlayer.startPlay();
                }
                int bnfIndex = audioPlayer.getPlayIndex();
                Logger.i("scrfdncnn readyCnt: " + readyCnt + " bnfIndex: " + bnfIndex);
                scrfRst = scrfdncnn.filerst(mCurrentBnfSession, !TextUtils.isEmpty(frame.sgPath) ? frame.sgPath : frame.rawPath, !TextUtils.isEmpty(frame.maskPath) ? frame.maskPath : "", frame.rect, "", bnfIndex, rawBuffer.array(),  maskBuffer.array(),mModelInfo.getWidth() * mModelInfo.getHeight() * 3);
                isLip = true;
                if (scrfRst < 0){
                    Logger.i("scrfdncnn.filerst bnf index: " + bnfIndex + " rst: " + scrfRst);
                }
            } else {
                isLip = false;
                scrfRst = scrfdncnn.fileload(!TextUtils.isEmpty(frame.sgPath) ? frame.sgPath : frame.rawPath, !TextUtils.isEmpty(frame.maskPath) ? frame.maskPath : "", mModelInfo.getWidth(), mModelInfo.getHeight(), rawBuffer.array(), maskBuffer.array(), mModelInfo.getWidth() * mModelInfo.getHeight() * 3);
                if (scrfRst < 0){
                    Logger.i("scrfdncnn.fileload rst: " + scrfRst);
                }
            }
            if (frame.startFlag){
                callback.onMotionPlayStart(frame.actionName);
            }
            if (frame.endFlag){
                callback.onMotionPlayComplete(frame.actionName);
            }
            if (mRenderSink != null) {
                mRenderSink.onVideoFrame(new ImageFrame(rawBuffer, maskBuffer, mModelInfo.getWidth(), mModelInfo.getHeight()));
            }
        }
        long useTime = System.currentTimeMillis() - startTime;
        if (mReporter != null){
            mReporter.onRenderStat(scrfRst, isLip, useTime);
        }
        return useTime;
    }

    private void handleStopRender() {
        Logger.i("handleStopRender");
        if (isRendering) {
            isRendering = false;
        } else {
            mHandler.sendEmptyMessage(MSG_QUIT);
        }
    }

    private void handleStartPushAudio(){
        if (mCurrentBnfSession > 0){
            scrfdncnn.finsession(mCurrentBnfSession);
        }
        mCurrentBnfSession = scrfdncnn.newsession();
        if (audioPlayer != null && isRendering){
            audioPlayer.pushStart();
        }
    }

    private void handlePushAudio(byte[] data){
        if (audioPlayer != null && isRendering){
            scrfdncnn.pushpcm(mCurrentBnfSession, data, data.length, 0);
            audioPlayer.pushData(ByteBuffer.wrap(data));
        }
    }

    private void handleStopPushAudio() {
        if (scrfdncnn != null && isRendering){
            scrfdncnn.finsession(mCurrentBnfSession);
        }
        if (audioPlayer != null){
            audioPlayer.pushDone();
        }
    }

    private void handleStopPlayAudio(){
        if (scrfdncnn != null && isRendering){
            scrfdncnn.finsession(mCurrentBnfSession);
            mCurrentBnfSession = -1;
            if (audioPlayer != null){
                audioPlayer.stop();
            }
        }
    }

    private void handleRequireMotion(String name, boolean now) {
        ModelInfo.Region matchRegion = null;
        for (ModelInfo.Region region : mModelInfo.getMotionRegions()){
            if (name != null && name.equals(region.name)){
                matchRegion = region;
            }
        }
        if (matchRegion != null){
            if (now){
                prepareActionRegion = matchRegion;
                requireMotion = true;
            } else {
                Logger.d("在播放队列最后插入动作区间region: " + matchRegion);
                mPreviewQueue.addAll(matchRegion.frames);
            }
        }
    }

    private void handleRequireMotionRandom(boolean now){
        if (!mModelInfo.getMotionRegions().isEmpty()){
            int randomIndex = new Random().nextInt(mModelInfo.getMotionRegions().size());
            ModelInfo.Region region = mModelInfo.getMotionRegions().get(randomIndex);
            if (now){
                requireMotion = true;
                prepareActionRegion = region;
            } else {
                Logger.d("在播放队列最后插入随机动作区间region: " + region);
                mPreviewQueue.addAll(region.frames);
            }
        }
    }

    static class RenderHandler extends Handler {

        private final WeakReference<RenderThread> encoderWeakReference;

        public RenderHandler(RenderThread render) {
            encoderWeakReference = new WeakReference<>(render);
        }

        @Override
        public void handleMessage(Message msg) {
            int what = msg.what;
            RenderThread render = encoderWeakReference.get();
            if (render == null) {
                return;
            }
            switch (what) {
                case MSG_RENDER_STEP:
                    render.handleAudioStep();
                    break;
                case MSG_STOP_RENDER:
                    render.handleStopRender();
                    break;
                case MSG_STOP_PUSH_AUDIO:
                    render.handleStopPushAudio();
                    break;
                case MSG_REQUIRE_MOTION:
                    String name = (String)msg.obj;
                    render.handleRequireMotion(name, msg.arg1 == 0);
                    break;
                case MSG_REQUIRE_MOTION_RANDOM:
                    render.handleRequireMotionRandom(msg.arg1 == 0);
                    break;
                case MSG_QUIT:
                    Logger.i("duix thread quit!");
                    Looper myLooper = Looper.myLooper();
                    if (myLooper != null) {
                        myLooper.quit();
                    }
                    break;
                case MSG_START_PUSH_AUDIO:
                    render.handleStartPushAudio();
                    break;
                case MSG_PUSH_AUDIO:
                    byte[] data = (byte[])msg.obj;
                    render.handlePushAudio(data);
                    break;
                case MSG_STOP_PLAY_AUDIO:
                    render.handleStopPlayAudio();
                    break;
            }
        }

    }

    public interface RenderCallback {
        void onInitResult(int code, int subCode, String message, ModelInfo modelInfo);

        void onPlayStart();

        void onPlayEnd();

        void onPlayError(int code, String msg);

        void onMotionPlayStart(String name);

        void onMotionPlayComplete(String name);
    }

    public interface Reporter {
        void onRenderStat(int resultCode, boolean isLip, long useTime);
    }
}
