#ifndef TL_RETRO_COMMON_H_
#define TL_RETRO_COMMON_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>


#ifdef __cplusplus
extern "C"
{
#endif
    JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_commonstop(JNIEnv * env, jobject obj);

    JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_stateSave(JNIEnv * env, jobject obj,jstring file);

    JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_stateLoad(JNIEnv * env, jobject obj,jstring file);

    JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_retroPause(JNIEnv * env, jobject obj);

    JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_retroResume(JNIEnv * env, jobject obj);

    JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_retroIsPause(JNIEnv * env, jobject obj);
	JNIEXPORT void JNICALL Java_com_audio_AudioPCM_registerAudio(JNIEnv * env, jobject obj,jobject audio);
	JNIEXPORT void JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_retroSetMute(JNIEnv * env, jobject obj, jint mute);
#ifdef __cplusplus
}
#endif


#endif
