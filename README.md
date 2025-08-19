硅基智能数字人Demo  JDK17  集成TTS音频流

    @Override
    public void onSynthesizeStart(String utteranceId) {
    //开始push音频
        if (mDUIXRender != null) {
            mDUIXRender.startPush();
        }
    }

    @Override
    public void onSynthesizeDone(String utteranceId) {
        if (mDUIXRender != null) {
        //结束push音频
            mDUIXRender.stopPush();
        }
    }

    @Override
    public void onSynthesizeError(String utteranceId) {
        if (mDUIXRender != null) {
        //合成报错，结束push音频
            mDUIXRender.stopPush();
        }
    }

    @Override
    public void onSynthesizeDataArrived(String s, byte[] bytes, int progress, int engineType) {
    //持续接收到TTS引擎合成结果，推送到数字人接口
        //Log.d(TAG, "合成DataArrived：" + s + " progress:" + progress + " engineType:" + engineType);
        if (mDUIXRender != null) {
            mDUIXRender.pushPcm(bytes);
            //savePcmToFile(bytes);
        }
    }


请遵守官方license：https://github.com/duixcom/Duix-Mobile
