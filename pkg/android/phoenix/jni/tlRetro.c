


#include "tlRetro.h"

#include "../../../../core_info.h"
#include "../../../../content.h"
#include "../../../../configuration.h"
#include "../../../../dynamic.h"
#include "../../../../defaults.h"
#include "../../../../frontend/frontend.h"
#include "../../../../playlist.h"
#include "../../../../retroarch.h"
#include "../../../../runloop.h"
#include "../../../../verbosity.h"
#include "../../../../command.h"
#include "../../../../dirs.h"

JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_commonstop(JNIEnv * env, jobject obj){
    int nResult=0;
    //frame_limit_last_time = 0.0;
    command_event(CMD_EVENT_QUIT, NULL);
    return nResult;
}



JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_stateSave(JNIEnv * env, jobject obj,jstring file){
    int nResult=0;
    global_t *global           = global_get_ptr();
    const char* szFile = NULL;
    szFile = (*env)->GetStringUTFChars(env,file, 0);

    strlcpy(global->name.savestate, szFile,
                  sizeof(global->name.savestate));
    retroarch_override_setting_set(RARCH_OVERRIDE_SETTING_STATE_PATH, NULL);
    if (retroarch_override_setting_is_set(RARCH_OVERRIDE_SETTING_STATE_PATH, NULL) &&
         path_is_directory(global->name.savestate))
      dir_set(RARCH_DIR_SAVESTATE, global->name.savestate);

    if (szFile) (*env)->ReleaseStringUTFChars(env,file, szFile);
    command_event(CMD_EVENT_SAVE_STATE,NULL);
    return nResult;
}

JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_stateLoad(JNIEnv * env, jobject obj,jstring file){
    int nResult=0;
   global_t *global           = global_get_ptr();
    const char* szFile = NULL;
    szFile = (*env)->GetStringUTFChars(env,file, 0);

    strlcpy(global->name.savestate, szFile,
                  sizeof(global->name.savestate));
    retroarch_override_setting_set(RARCH_OVERRIDE_SETTING_STATE_PATH, NULL);
    if (retroarch_override_setting_is_set(RARCH_OVERRIDE_SETTING_STATE_PATH, NULL) &&
         path_is_directory(global->name.savestate))
      dir_set(RARCH_DIR_SAVESTATE, global->name.savestate);

    if (szFile) (*env)->ReleaseStringUTFChars(env,file, szFile);
    command_event(CMD_EVENT_LOAD_STATE,NULL);
    return nResult;
}

JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_retroPause(JNIEnv * env, jobject obj){
    int nResult=0;
    //frame_limit_last_time = 0.0;
    command_event(CMD_EVENT_PAUSE, NULL);
    return nResult;
}

JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_retroResume(JNIEnv * env, jobject obj){
    int nResult=0;
    //frame_limit_last_time = 0.0;
    command_event(CMD_EVENT_UNPAUSE, NULL);
    return nResult;
}
JNIEXPORT jint JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_retroIsPause(JNIEnv * env, jobject obj){
    return runloop_ctl(RUNLOOP_CTL_IS_PAUSED,  NULL);
}
extern bool audio_is_mute;
JNIEXPORT void JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_retroSetMute(JNIEnv * env, jobject obj, jint mute){
    audio_is_mute = !!mute;
}

///========================
#define AUDIO_METHOD_OPENAUDIO	"OpenAudio"
#define AUDIO_METHOD_OPENAUDIO_SIG	"(IIII)I"

#define AUDIO_METHOD_PLAYPCM "PlayPCM"
#define AUDIO_METHOD_PLAYPCM_SIG "([SI)I"

#define AUDIO_METHOD_DESTROY "DestroyAudio"
#define AUDIO_METHOD_DESTROY_SIG "(I)V"

static JNIEnv *g_env;
static jobject g_audio;
JNIEXPORT void JNICALL Java_com_audio_AudioPCM_registerAudio(JNIEnv * env, jobject obj,jobject audio)
{
	g_audio = audio;
	g_env = env;
}
#if 0
jmethodID findMethodID(const char* mname,const char* msig)
{
	jclass cls = NULL;
	jmethodID methodID;
	cls = (*g_env)->GetObjectClass(g_env, g_audio);	
	methodID = (*g_env)->GetMethodID(g_env,cls, mname,msig );
	(*g_env)->DeleteLocalRef(g_env,cls);
	return methodID;
}
int jniOpenAudio(unsigned rate, unsigned latency,    unsigned block_frames)
{
	jmethodID methodID = findMethodID(AUDIO_METHOD_OPENAUDIO, AUDIO_METHOD_OPENAUDIO_SIG);
	int ret = -1;
	if (methodID)
	{
		ret = (*g_env)->CallIntMethod(g_env,g_audio, methodID, 
			rate, latency, 16, block_frames);
	}
	
	return ret;
}
ssize_t jniplayPCM(void *data, const void *buf, size_t size)
{
	int ret  = 0;
	jmethodID methodID = NULL;
	
	methodID = findMethodID(AUDIO_METHOD_PLAYPCM, AUDIO_METHOD_PLAYPCM_SIG);
	if (methodID)
	{
		jshort *sampleData;
		jsize sampleCount = size >> 1;
		jshortArray sampleArray = (*g_env)->NewShortArray(g_env,sampleCount);
		sampleData = (*g_env)->GetShortArrayElements(g_env,sampleArray, 0);
		memcpy(sampleData, buf, size);
		ret = (*g_env)->CallIntMethod(g_env,g_audio, methodID,
			sampleArray, sampleCount);
		(*g_env)->ReleaseShortArrayElements(g_env,sampleArray, sampleData, JNI_ABORT);
		(*g_env)->DeleteLocalRef(g_env,sampleArray);
	}
	//env->DeleteLocalRef(audioObj);
	return (ret << 1);
}
void jniclosePCM(void *data)
{
	jmethodID methodID = findMethodID(AUDIO_METHOD_DESTROY, AUDIO_METHOD_DESTROY_SIG);
	
	if (methodID)
		(*g_env)->CallVoidMethod(g_env,g_audio, methodID, (jint)0);
}
#endif