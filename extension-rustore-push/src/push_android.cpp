#if defined(DM_PLATFORM_ANDROID)
#include <jni.h>
#include <unistd.h>
#include <stdlib.h>
#include <dmsdk/sdk.h>
#include <dmsdk/dlib/android.h>
#include "push_utils.h"

#define LIB_NAME "ruStorePush"

struct Push
{
    Push()
    {
        memset(this, 0, sizeof(*this));
    }

    jobject              m_Push;
    jobject              m_PushJNI;
    jmethodID            m_Start;
    jmethodID            m_Stop;
    jmethodID            m_NewToken;

    dmScript::LuaCallbackInfo* m_CallbackNewToken;
    dmScript::LuaCallbackInfo* m_Listener;
    dmRustorePush::CommandQueue m_CommandQueue;

};

static Push g_Push;

static int Push_NewToken(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    if (g_Push.m_CallbackNewToken)
        dmScript::DestroyCallback(g_Push.m_CallbackNewToken);

    g_Push.m_CallbackNewToken = dmScript::CreateCallback(L, 1);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    env->CallVoidMethod(g_Push.m_Push, g_Push.m_NewToken, dmGraphics::GetNativeAndroidActivity());

    return 0;
}

static int ShowToast(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    jclass cls = dmAndroid::LoadClass(env, "ru.rustore.defoldpush.Push");
    jmethodID method = env->GetStaticMethodID(cls, "showToast", "(Landroid/app/Activity;Ljava/lang/String;)V");

    const char* msg = (char*)luaL_checkstring(L, 1);
    jstring jmsg = env->NewStringUTF(msg);

    env->CallStaticVoidMethod(cls, method, dmGraphics::GetNativeAndroidActivity(), jmsg);

    thread.Detach();

    return 0;
}

static int CopyToClipboard(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    jclass cls = dmAndroid::LoadClass(env, "ru.rustore.defoldpush.Push");
    jmethodID method = env->GetStaticMethodID(cls, "copyToClipboard", "(Landroid/app/Activity;Ljava/lang/String;)V");

    const char* text = (char*)luaL_checkstring(L, 1);
    jstring jtext = env->NewStringUTF(text);

    env->CallStaticVoidMethod(cls, method, dmGraphics::GetNativeAndroidActivity(), jtext);

    env->DeleteLocalRef(jtext);

    thread.Detach();

    return 0;
}

static int Push_SetListener(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    if (g_Push.m_Listener)
        dmScript::DestroyCallback(g_Push.m_Listener);

    g_Push.m_Listener = dmScript::CreateCallback(L, 1);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();


    return 0;
}

static const luaL_reg Push_methods[] =
{
    {"set_on_token", Push_NewToken},
    {"set_on_message", Push_SetListener},
    {"show_toast", ShowToast},
    {"copy_to_clipboard", CopyToClipboard},

    {0, 0}
};

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_ru_rustore_defoldpush_PushJNI_onNewToken(JNIEnv* env, jobject obj, jstring pushToken, jstring errorMessage)
{
    const char* ri = 0;
    const char* em = 0;

    if (pushToken)
    {
        ri = env->GetStringUTFChars(pushToken, 0);
    }
    if (errorMessage)
    {
        em = env->GetStringUTFChars(errorMessage, 0);
    }

    dmRustorePush::Command cmd;
    cmd.m_Callback = g_Push.m_CallbackNewToken;
    cmd.m_Command = dmRustorePush::COMMAND_TYPE_NEW_TOKEN_RESULT;
    if (ri) {
        cmd.m_Result = strdup(ri);
        env->ReleaseStringUTFChars(pushToken, ri);
    }
    if (em) {
        cmd.m_Error = strdup(em);
        env->ReleaseStringUTFChars(errorMessage, em);
    }
    dmRustorePush::QueuePush(&g_Push.m_CommandQueue, &cmd);
}


JNIEXPORT void JNICALL Java_ru_rustore_defoldpush_PushJNI_onMessage(JNIEnv* env, jobject obj, jstring json, bool wasActivated)
{
    const char* j = 0;

    if (json)
    {
        j = env->GetStringUTFChars(json, 0);
    }

    dmRustorePush::Command cmd;
    cmd.m_Callback = g_Push.m_Listener;
    cmd.m_Command = dmRustorePush::COMMAND_TYPE_PUSH_MESSAGE_RESULT;
    cmd.m_Result = strdup(j);
    cmd.m_WasActivated = wasActivated;
    dmRustorePush::QueuePush(&g_Push.m_CommandQueue, &cmd);
    if (j)
    {
        env->ReleaseStringUTFChars(json, j);
    }
}

#ifdef __cplusplus
}
#endif


static dmExtension::Result AppInitializePush(dmExtension::AppParams* params)
{
    dmRustorePush::QueueCreate(&g_Push.m_CommandQueue);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    jclass push_class = dmAndroid::LoadClass(env, "ru.rustore.defoldpush.Push");
    jclass push_jni_class = dmAndroid::LoadClass(env, "ru.rustore.defoldpush.PushJNI");

    g_Push.m_Start = env->GetMethodID(push_class, "start", "(Landroid/app/Activity;Lru/rustore/defoldpush/IPushListener;Ljava/lang/String;Ljava/lang/String;)V");
    g_Push.m_Stop = env->GetMethodID(push_class, "stop", "()V");
    g_Push.m_NewToken = env->GetMethodID(push_class, "newToken", "(Landroid/app/Activity;)V");

    jmethodID get_instance_method = env->GetStaticMethodID(push_class, "getInstance", "()Lru/rustore/defoldpush/Push;");
    g_Push.m_Push = env->NewGlobalRef(env->CallStaticObjectMethod(push_class, get_instance_method));

    jmethodID setListener = env->GetMethodID(push_class, "setApplicationListener", "(Landroid/app/Activity;)V");
    env->CallVoidMethod(g_Push.m_Push, setListener, dmGraphics::GetNativeAndroidActivity());

    jmethodID jni_constructor = env->GetMethodID(push_jni_class, "<init>", "()V");
    g_Push.m_PushJNI = env->NewGlobalRef(env->NewObject(push_jni_class, jni_constructor));

    const char* rustore_project_id = dmConfigFile::GetString(params->m_ConfigFile, "android.rustore_project_id", "");
    const char* project_title = dmConfigFile::GetString(params->m_ConfigFile, "project.title", "");
    jstring rustore_project_id_string = env->NewStringUTF(rustore_project_id);
    jstring project_title_string = env->NewStringUTF(project_title);
    env->CallVoidMethod(g_Push.m_Push, g_Push.m_Start, dmGraphics::GetNativeAndroidActivity(), g_Push.m_PushJNI, rustore_project_id_string, project_title_string);
    env->DeleteLocalRef(rustore_project_id_string);
    env->DeleteLocalRef(project_title_string);

    return dmExtension::RESULT_OK;
}

static dmExtension::Result UpdatePush(dmExtension::Params* params)
{
    dmRustorePush::QueueFlush(&g_Push.m_CommandQueue, dmRustorePush::HandleCommand, 0);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizePush(dmExtension::AppParams* params)
{
    {
        dmAndroid::ThreadAttacher threadAttacher;
        JNIEnv* env = threadAttacher.GetEnv();

        env->CallVoidMethod(g_Push.m_Push, g_Push.m_Stop);
        env->DeleteGlobalRef(g_Push.m_Push);
        env->DeleteGlobalRef(g_Push.m_PushJNI);
    }
    g_Push.m_Push = NULL;
    g_Push.m_PushJNI = NULL;

    dmRustorePush::QueueDestroy(&g_Push.m_CommandQueue);

    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializePush(dmExtension::Params* params)
{
    lua_State* L = params->m_L;
    int top = lua_gettop(L);
    luaL_register(L, LIB_NAME, Push_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizePush(dmExtension::Params* params)
{
    if (g_Push.m_Listener) {
        dmScript::DestroyCallback(g_Push.m_Listener);
        g_Push.m_Listener = 0;
    }
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(RustorePushExtExternal, "RuStorePush", AppInitializePush, AppFinalizePush, InitializePush, UpdatePush, 0, FinalizePush)
#endif // DM_PLATFORM_ANDROID
