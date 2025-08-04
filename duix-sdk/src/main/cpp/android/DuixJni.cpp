#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <vector>
#include <unistd.h>
#include "gjsimp.h"
#include "JniHelper.h"
#include "aesmain.h"
#include "jmat.h"
#include "Log.h"

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON
       //
       //
#define TAG  "tooken"
#ifdef DEBUGME
#define JNIEXPORT 
#define JNI_OnLoad
#define jint int
#define jlong long
#define jstring string
#define JNICALL 
#define JavaVM void
#define LOGI(...)
#define JNIEnv void
#define jobject void*
#endif
extern "C" {

  static dhduix_t* g_digit = 0;
  static JMat*    g_gpgmat = NULL;
  static int  g_width = 540;
  static int  g_height = 960;
  static int  g_taskid = -1;

  JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGD(TAG, "JNI_OnLoad");
    //g_digit = new GDigit(g_width,g_height,g_msgcb);
    JniHelper::sJavaVM = vm;
    return JNI_VERSION_1_4;
  }

  JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI(TAG, "unload");
    if(g_digit){
      dhduix_free(g_digit);
      g_digit = nullptr;
    }
  }

  static std::string getStringUTF(JNIEnv *env, jstring obj) {
    char *c_str = (char *) env->GetStringUTFChars(obj, nullptr);
    std::string tmpString = std::string(c_str);
    env->ReleaseStringUTFChars(obj, c_str);
    return tmpString;
  }


  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_alloc(JNIEnv *env, jobject thiz,
      jint taskid,jint mincalc,jint width,jint height){
    LOGI(TAG, "create");
    g_taskid = taskid;
    dhduix_alloc(&g_digit,mincalc,width,height);
    return 0;
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_free(JNIEnv *env, jobject thiz,jint taskid){
    if(g_taskid==taskid){
      dhduix_free(g_digit);
      g_digit = nullptr;
    }
    return 0;
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_initPcmex(JNIEnv *env, jobject thiz, 
      jint maxsize,jint minoff,jint minblock,jint maxblock,jint rgb){
    if(!g_digit)return -1;
    int rst = dhduix_initPcmex(g_digit,maxsize,minoff,minblock,maxblock,rgb);
    return rst;
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_initWenet(JNIEnv *env, jobject thiz,
      jstring fnwenet){
    if(!g_digit)return -1;
    std::string str = getStringUTF(env,fnwenet);
    char* ps = (char*)(str.c_str());
    int rst = dhduix_initWenet(g_digit,ps);
    return rst;
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_initMunet(JNIEnv *env, jobject thiz,
      jstring fnparam,jstring fnbin,jstring fnmask){
    if(!g_digit)return -1;
    std::string sparam = getStringUTF(env,fnparam);
    std::string sbin = getStringUTF(env,fnbin);
    std::string smask = getStringUTF(env,fnmask);
    int rst = dhduix_initMunet(g_digit,(char*)sparam.c_str(),(char*)sbin.c_str(),(char*)smask.c_str());
    return rst;
  }

  JNIEXPORT jlong JNICALL Java_ai_guiji_duix_DuixNcnn_newsession(JNIEnv *env, jobject thiz){
    if(!g_digit)return -1;
    uint64_t sessid = dhduix_newsession(g_digit);
    return (jlong)sessid;
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_pushpcm(JNIEnv *env, jobject thiz, 
      jlong sessid,jbyteArray arrbuf,jint size,jint kind){
    if(!g_digit)return -1;
    jbyte *pcmbuf = (jbyte *) env->GetPrimitiveArrayCritical(arrbuf, 0);
    uint64_t sid = sessid;
    int rst = dhduix_pushpcm(g_digit,sid,(char*)pcmbuf,size,kind);
    env->ReleasePrimitiveArrayCritical(arrbuf,pcmbuf, 0);
    return rst;
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_finsession(JNIEnv *env, jobject thiz,jlong sessid){
    if(!g_digit)return -1;
    uint64_t sid = sessid;
    return dhduix_finsession(g_digit,sid);
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_consession(JNIEnv *env, jobject thiz,jlong sessid){
    if(!g_digit)return -1;
    uint64_t sid = sessid;
    return dhduix_consession(g_digit,sid);
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_allcnt(JNIEnv *env, jobject thiz,jlong sessid){
    if(!g_digit)return -1;
    uint64_t sid = sessid;
    return dhduix_allcnt(g_digit,sid);
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_readycnt(JNIEnv *env, jobject thiz,jlong sessid){
    if(!g_digit)return -1;
    uint64_t sid = sessid;
    return dhduix_readycnt(g_digit,sid);
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_fileload(JNIEnv* env, jobject thiz,
      jstring picfile, jstring mskfile,jint width,jint height,
      jbyteArray arrpic,jbyteArray arrmsk,jint bursize){
  //
    std::string s_pic = getStringUTF(env,picfile);
    std::string s_msk = getStringUTF(env,mskfile);
    jbyte *picbuf = (jbyte *) env->GetPrimitiveArrayCritical(arrpic, 0);
    JMat* mat_pic = new JMat(width,height,(uint8_t*)picbuf);
    mat_pic->loadjpg(s_pic,1);
    env->ReleasePrimitiveArrayCritical( arrpic,picbuf, 0);

    if(s_msk.length()){
        jbyte *mskbuf = (jbyte *) env->GetPrimitiveArrayCritical(arrmsk, 0);
        JMat* mat_msk = new JMat(width,height,(uint8_t*)mskbuf);
        mat_msk->loadjpg(s_msk,1);
        env->ReleasePrimitiveArrayCritical( arrmsk,mskbuf, 0);
    }
    return 0;
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_bufrst(JNIEnv* env, jobject thiz,
      jlong sessid, jintArray arrbox, jint inx,
      jbyteArray arrimg,jint imgsize){
    if(!g_digit)return -1;
    uint64_t sid = sessid;
    jint *boxData = (jint*) env->GetPrimitiveArrayCritical( arrbox, 0);
    jbyte *imgbuf = (jbyte*) env->GetPrimitiveArrayCritical(arrimg, 0);
    int bnfinx = inx;
    int rst = dhduix_simpinx(g_digit,sid,(uint8_t*)imgbuf, 0,0, 
        (int*)boxData,NULL,NULL,bnfinx);
    env->ReleasePrimitiveArrayCritical( arrimg,imgbuf, 0);
    env->ReleasePrimitiveArrayCritical( arrbox, boxData, 0);
    return rst;
  }

  JNIEXPORT jint JNICALL Java_ai_guiji_duix_DuixNcnn_filerst(JNIEnv* env, jobject thiz,
      jlong sessid,jstring picfile, jstring mskfile,
      jintArray arrbox, jstring fgfile,jint inx,
      jbyteArray arrimg,jbyteArray arrmsk,jint imgsize){
    if(!g_digit)return -1;
    uint64_t sid = sessid;
    std::string s_pic = getStringUTF(env,picfile);
    std::string s_msk = getStringUTF(env,mskfile);
    std::string s_fg = getStringUTF(env,fgfile);
    jint *boxData = (jint*) env->GetPrimitiveArrayCritical( arrbox, 0);
    jbyte *imgbuf = (jbyte*) env->GetPrimitiveArrayCritical(arrimg, 0);
    jbyte *mskbuf = (jbyte*) env->GetPrimitiveArrayCritical(arrmsk, 0);
    int rst = dhduix_fileinx(g_digit,sid,
        (char*)s_pic.c_str(),(int*)boxData,
        (char*)s_msk.c_str(),(char*)s_fg.c_str(),
        inx,(char*)imgbuf,(char*)mskbuf,imgsize);
    env->ReleasePrimitiveArrayCritical( arrimg,imgbuf, 0);
    env->ReleasePrimitiveArrayCritical( arrmsk,mskbuf, 0);
    env->ReleasePrimitiveArrayCritical( arrbox, boxData, 0);
    return rst;
  }

    JNIEXPORT jint JNICALL
        Java_ai_guiji_duix_DuixNcnn_startgpg(JNIEnv *env, jobject thiz, jstring picfn,jstring gpgfn){
            std::string s_pic = getStringUTF(env,picfn);
            std::string s_gpg = getStringUTF(env,gpgfn);
            if(!g_gpgmat)g_gpgmat = new JMat();
            int rst = g_gpgmat->loadjpg(s_pic);
            if(rst)return rst;
            rst = g_gpgmat->savegpg(s_gpg);
            return rst;
        }

    JNIEXPORT jint JNICALL
        Java_ai_guiji_duix_DuixNcnn_processmd5(JNIEnv *env, jobject thiz, jint kind,jstring infn,jstring outfn){
            std::string s_in = getStringUTF(env,infn);
            std::string s_out = getStringUTF(env,outfn);
            int rst = mainenc(kind,(char*)s_in.c_str(),(char*)s_out.c_str());
            return rst;
        }

    JNIEXPORT jint JNICALL
        Java_ai_guiji_duix_DuixNcnn_stopgpg(JNIEnv *env, jobject thiz){
            if(g_gpgmat){
                delete g_gpgmat;
                g_gpgmat = NULL;
            }
            return 0;
    }
}

