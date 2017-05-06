#include <jni.h>
#include <android/log.h>
#define TAG "sdlpal-jni"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#define EXTERN_C_LINKAGE extern "C"

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"
#include "util.h"

char externalStoragePath[1024];
char midiInterFile[1024];

static int jstring_to_utf8(JNIEnv* env, jstring j_str, char *buffer, int capacity)
{
    jsize length = env->GetStringUTFLength(j_str);
    const char * const base = env->GetStringUTFChars(j_str, NULL);
    if (base == NULL)
    {
        return 0;
    }
    if (capacity <= length)
        length = capacity - 1;
    strncpy(buffer, base, length);
    env->ReleaseStringUTFChars(j_str, base);
    buffer[length] = '\0';
    return length;
}

static JavaVM* gJVM;

EXTERN_C_LINKAGE jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_2) != JNI_OK) {
        return -1;
    }
    gJVM = vm;
    return JNI_VERSION_1_2;
}

static JNIEnv* getJNIEnv()
{
    JNIEnv* env;
    if (gJVM->GetEnv((void**)&env, JNI_VERSION_1_2) != JNI_OK) {
        return NULL;
    }
    return env;
}

/* 
 * Class:     io_github_sdlpal_PalActivity 
 * Method:    setExternalStorage 
 * Signature: (Ljava/lang/String;)V
 */  
EXTERN_C_LINKAGE
JNIEXPORT void JNICALL Java_io_github_sdlpal_PalActivity_setExternalStorage(JNIEnv *env, jclass cls, jstring j_str)  
{
    jstring_to_utf8(env, j_str, externalStoragePath, sizeof(externalStoragePath) - 8);
    strcat(externalStoragePath, "/sdlpal/");
    LOGV("JNI got externalStoragePath:%s", externalStoragePath);
}

/* 
 * Class:     io_github_sdlpal_PalActivity 
 * Method:    setMIDIInterFile 
 * Signature: (Ljava/lang/String;)V
 */  
EXTERN_C_LINKAGE
JNIEXPORT void JNICALL Java_io_github_sdlpal_PalActivity_setMIDIInterFile(JNIEnv *env, jclass cls, jstring j_str)  
{
    jstring_to_utf8(env, j_str, midiInterFile, sizeof(midiInterFile));
    LOGV("JNI got midi inter filename:%s", midiInterFile);
}

EXTERN_C_LINKAGE
void* JNI_mediaplayer_load(const char *filename)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("io/github/sdlpal/PalActivity");
    jmethodID mid = env->GetStaticMethodID(clazz, "JNI_mediaplayer_load", "(Ljava/lang/String;)Landroid/media/MediaPlayer;");
    jstring str = env->NewStringUTF(filename);
    jobject player = env->CallStaticObjectMethod(clazz, mid, str);
    env->DeleteLocalRef(str);
    return env->NewGlobalRef(player);
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_free(void *player)
{
    getJNIEnv()->DeleteGlobalRef((jobject)player);
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_play(void *player, int looping)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "setLooping", "(Z)V"), looping ? JNI_TRUE : JNI_FALSE);
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "start", "()V"));
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_stop(void *player)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "stop", "()V"));
}

EXTERN_C_LINKAGE
int JNI_mediaplayer_isplaying(void *player)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    return env->CallBooleanMethod((jobject)player, env->GetMethodID(clazz, "isPlaying", "()Z"));
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_setvolume(void *player, int volume)
{
    float vol = (float)volume / 127.0f;
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "setVolume", "(FF)V"), vol, vol);
}

EXTERN_C_LINKAGE
LPCSTR
UTIL_BasePath(
   VOID
)
{
    return externalStoragePath;
}

EXTERN_C_LINKAGE
LPCSTR
UTIL_SavePath(
   VOID
)
{
    return externalStoragePath;
}

EXTERN_C_LINKAGE
BOOL
UTIL_GetScreenSize(
   DWORD *pdwScreenWidth,
   DWORD *pdwScreenHeight
)
{
    *pdwScreenWidth  = 640;
    *pdwScreenHeight = 400;
    return TRUE;
}

EXTERN_C_LINKAGE
BOOL
UTIL_IsAbsolutePath(
	LPCSTR  lpszFileName
)
{
	return lpszFileName[0] == '/';
}

EXTERN_C_LINKAGE
INT
UTIL_Platform_Init(
   int argc,
   char* argv[]
)
{
   gConfig.fLaunchSetting = FALSE;
   return 0;
}

EXTERN_C_LINKAGE
VOID
UTIL_Platform_Quit(
   VOID
)
{
}

#ifdef ENABLE_NEWLOG

static int maxLogLevel = LOG_WARNING;

PAL_C_LINKAGE VOID
UTIL_SetLogLevel(
	int             level
)
{
	if (level >= LOG_EMERG && level < LOG_LAST_PRIORITY)
	{
		maxLogLevel = level;
	}
}

EXTERN_C_LINKAGE
VOID
UTIL_WriteLog(
   int             Priority,
   const char     *Fmt,
   ...
)
{
	if (Priority < LOG_EMERG || Priority > maxLogLevel)
	{
		return;
	}
    
    switch(Priority)
    {
    case LOG_EMERG:   Priority = ANDROID_LOG_FATAL; break;
    case LOG_ALERT:   Priority = ANDROID_LOG_FATAL; break;
    case LOG_CRIT:    Priority = ANDROID_LOG_ERROR; break;
    case LOG_ERR:     Priority = ANDROID_LOG_ERROR; break;
    case LOG_WARNING: Priority = ANDROID_LOG_WARN; break;
    case LOG_NOTICE:  Priority = ANDROID_LOG_INFO; break;
    case LOG_INFO:    Priority = ANDROID_LOG_INFO; break;
    case LOG_DEBUG:   Priority = ANDROID_LOG_DEBUG; break;
    default:          Priority = ANDROID_LOG_VERBOSE; break;
    }

    va_list ap;
    va_start(ap, Fmt);
    __android_log_vprint(ANDROID_LOG_VERBOSE, TAG, Fmt, ap);
    va_end(ap);
}

#endif