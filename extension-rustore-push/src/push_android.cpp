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
    jobject              m_ClientIdCallbackJNI;
    jmethodID            m_Start;
    jmethodID            m_Stop;
    jmethodID            m_NewToken;
    jmethodID            m_DeleteToken;
    jmethodID            m_TopicSubscribe;
    jmethodID            m_TopicUnsubscribe;
    jmethodID            m_ShowToast;
    jmethodID            m_CopyToClipboard;

    dmScript::LuaCallbackInfo* m_NewTokenListener;
    dmScript::LuaCallbackInfo* m_MessagesListener;
    
    dmScript::LuaCallbackInfo* m_CallbackClientId;

    dmScript::LuaCallbackInfo* m_Callback; // callback for delete and topics

    dmRustorePush::CommandQueue m_CommandQueue;

};

static Push g_Push;

static int Push_NewTokenListener(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    if (g_Push.m_NewTokenListener)
        dmScript::DestroyCallback(g_Push.m_NewTokenListener);

    g_Push.m_NewTokenListener = dmScript::CreateCallback(L, 1);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    env->CallVoidMethod(g_Push.m_Push, g_Push.m_NewToken, dmGraphics::GetNativeAndroidActivity());

    return 0;
}

static int Push_MessagesListener(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    if (g_Push.m_MessagesListener)
        dmScript::DestroyCallback(g_Push.m_MessagesListener);

    g_Push.m_MessagesListener = dmScript::CreateCallback(L, 1);

    return 0;
}

static int Push_DeleteToken(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    if (g_Push.m_Callback)
        dmScript::DestroyCallback(g_Push.m_Callback);

    g_Push.m_Callback = dmScript::CreateCallback(L, 1);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    env->CallVoidMethod(g_Push.m_Push, g_Push.m_DeleteToken, dmGraphics::GetNativeAndroidActivity());

    return 0;
}

static int Push_SetClientIdCallback(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    if (g_Push.m_CallbackClientId)
        dmScript::DestroyCallback(g_Push.m_CallbackClientId);

    g_Push.m_CallbackClientId = dmScript::CreateCallback(L, 1);

    return 0;
}

static int Push_TopicSubscribe(lua_State* L)
{
    int top = lua_gettop(L);

    const char* topic = luaL_checkstring(L, 1);

    if (g_Push.m_Callback)
        dmScript::DestroyCallback(g_Push.m_Callback);

    g_Push.m_Callback = dmScript::CreateCallback(L, 2);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    jstring topic_name = env->NewStringUTF(topic);
    env->CallVoidMethod(g_Push.m_Push, g_Push.m_TopicSubscribe, dmGraphics::GetNativeAndroidActivity(), topic_name);

    env->DeleteLocalRef(topic_name);

    assert(top == lua_gettop(L));
    return 0;
}

static int Push_TopicUnsubscribe(lua_State* L)
{
    int top = lua_gettop(L);

    const char* topic = luaL_checkstring(L, 1);

    if (g_Push.m_Callback)
        dmScript::DestroyCallback(g_Push.m_Callback);

    g_Push.m_Callback = dmScript::CreateCallback(L, 2);

    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    jstring topic_name = env->NewStringUTF(topic);
    env->CallVoidMethod(g_Push.m_Push, g_Push.m_TopicUnsubscribe, dmGraphics::GetNativeAndroidActivity(), topic_name);

    env->DeleteLocalRef(topic_name);

    assert(top == lua_gettop(L));
    return 0;
}

static int ShowToast(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    const char* msg = (char*)luaL_checkstring(L, 1);
    jstring jmsg = env->NewStringUTF(msg);
    
    env->CallVoidMethod(g_Push.m_Push, g_Push.m_ShowToast, dmGraphics::GetNativeAndroidActivity(), jmsg);

    env->DeleteLocalRef(jmsg);

    thread.Detach();

    return 0;
}

static int CopyToClipboard(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dmAndroid::ThreadAttacher thread;
    JNIEnv* env = thread.GetEnv();

    const char* text = (char*)luaL_checkstring(L, 1);
    jstring jtext = env->NewStringUTF(text);

    env->CallVoidMethod(g_Push.m_Push, g_Push.m_CopyToClipboard, dmGraphics::GetNativeAndroidActivity(), jtext);

    env->DeleteLocalRef(jtext);

    thread.Detach();

    return 0;
}

static const luaL_reg Push_methods[] =
{
    {"set_on_token", Push_NewTokenListener},
    {"set_on_message", Push_MessagesListener},
    {"set_client_id_callback", Push_SetClientIdCallback},
    {"delete_token", Push_DeleteToken},
    {"topic_subscribe", Push_TopicSubscribe},
    {"topic_unsubscribe", Push_TopicUnsubscribe},
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

    if (pushToken) {
        ri = env->GetStringUTFChars(pushToken, 0);
    }
    if (errorMessage) {
        em = env->GetStringUTFChars(errorMessage, 0);
    }

    dmRustorePush::Command cmd;
    cmd.m_Callback = g_Push.m_NewTokenListener;
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

JNIEXPORT void JNICALL Java_ru_rustore_defoldpush_PushJNI_onDeleteToken(JNIEnv* env, jobject obj, jstring errorMessage)
{
    const char* em = 0;

    if (errorMessage) {
        em = env->GetStringUTFChars(errorMessage, 0);
    }

    dmRustorePush::Command cmd;
    cmd.m_Callback = g_Push.m_Callback;
    cmd.m_Command = dmRustorePush::COMMAND_TYPE_DELETE_TOKEN_RESULT;
    
    if (em) {
        cmd.m_Error = strdup(em);
        env->ReleaseStringUTFChars(errorMessage, em);
    }
    dmRustorePush::QueuePush(&g_Push.m_CommandQueue, &cmd);
    g_Push.m_Callback = 0;
}


JNIEXPORT void JNICALL Java_ru_rustore_defoldpush_PushJNI_onMessage(JNIEnv* env, jobject obj, jstring json, bool wasActivated, jstring from)
{
    const char* j = 0;
    const char* f = 0;
    
    if (json) {
        j = env->GetStringUTFChars(json, 0);
    }
    if (from) {
        f = env->GetStringUTFChars(from, 0);
    }

    dmRustorePush::Command cmd;
    cmd.m_Callback = g_Push.m_MessagesListener;
    cmd.m_Command = dmRustorePush::COMMAND_TYPE_PUSH_MESSAGE_RESULT;
    cmd.m_Result = strdup(j);
    cmd.m_WasActivated = wasActivated;
    cmd.m_From = strdup(f);

    dmRustorePush::QueuePush(&g_Push.m_CommandQueue, &cmd);

    if (j) {
        env->ReleaseStringUTFChars(json, j);
    }
    env->ReleaseStringUTFChars(from, f);
}

JNIEXPORT void JNICALL Java_ru_rustore_defoldpush_PushJNI_onSubscribeToTopic(JNIEnv* env, jobject obj, jstring errorMessage)
{
    const char* em = 0;

    if (errorMessage) {
        em = env->GetStringUTFChars(errorMessage, 0);
    }

    dmRustorePush::Command cmd;
    cmd.m_Callback = g_Push.m_Callback;
    cmd.m_Command = dmRustorePush::COMMAND_TYPE_SUBSCRIBE_TOPIC_RESULT;
    
    if (em) {
        cmd.m_Error = strdup(em);
        env->ReleaseStringUTFChars(errorMessage, em);
    }
    dmRustorePush::QueuePush(&g_Push.m_CommandQueue, &cmd);
    g_Push.m_Callback = 0;
}

JNIEXPORT void JNICALL Java_ru_rustore_defoldpush_PushJNI_onUnsubscribeFromTopic(JNIEnv* env, jobject obj, jstring errorMessage)
{
    const char* em = 0;

    if (errorMessage) {
        em = env->GetStringUTFChars(errorMessage, 0);
    }

    dmRustorePush::Command cmd;
    cmd.m_Callback = g_Push.m_Callback;
    cmd.m_Command = dmRustorePush::COMMAND_TYPE_UNSUBSCRIBE_TOPIC_RESULT;
    
    if (em) {
        cmd.m_Error = strdup(em);
        env->ReleaseStringUTFChars(errorMessage, em);
    }
    dmRustorePush::QueuePush(&g_Push.m_CommandQueue, &cmd);
    g_Push.m_Callback = 0;
}

// -------------------------- Analytic ID Callback

JNIEXPORT jobject JNICALL Java_ru_rustore_defoldpush_ClientIdCallbackJNI_getClientId(JNIEnv* env, jobject obj)
{
    if (g_Push.m_CallbackClientId) {
        if (!dmScript::IsCallbackValid(g_Push.m_CallbackClientId)) {
            return NULL;
        }
        lua_State* L = dmScript::GetCallbackLuaContext(g_Push.m_CallbackClientId);
        int top = lua_gettop(L);

        if (!dmScript::SetupCallback(g_Push.m_CallbackClientId)) {
            return NULL;
        }

        int ret = dmScript::PCall(L, 1, LUA_MULTRET); // in self, ret adverisement_id + type
        if (ret != 0) {
            return NULL;
        }
        (void)ret;

        int aidType = luaL_checkinteger(L, -1);
        lua_pop(L, 1);

        if (lua_type(L, -1) != LUA_TSTRING) {
            lua_pop(L, 1);
            return NULL;
        }

        const char* aid = lua_tostring(L, -1);
        lua_pop(L, 1);
        
        dmScript::TeardownCallback(g_Push.m_CallbackClientId);

        assert(top == lua_gettop(L));

        if (aidType <= dmRustorePush::CLIENT_AID_NOT_AVAILABLE || aidType > dmRustorePush::CLIENT_OAID) {
            return NULL;
        }

        dmAndroid::ThreadAttacher threadAttacher;
        JNIEnv* env = threadAttacher.GetEnv();
        jclass client_id_class = dmAndroid::LoadClass(env, "com.vk.push.common.clientid.ClientId");
        jmethodID client_id_class_constructor = env->GetMethodID( client_id_class, "<init>", "(Ljava/lang/String;Lcom/vk/push/common/clientid/ClientIdType;)V");

        jclass client_id_type_class = dmAndroid::LoadClass(env, "com.vk.push.common.clientid.ClientIdType");
        jfieldID fd_client_id_type_gaid = env->GetStaticFieldID(client_id_type_class, "GAID", "Lcom/vk/push/common/clientid/ClientIdType;");
        jfieldID fd_client_id_type_oaid = env->GetStaticFieldID(client_id_type_class, "OAID", "Lcom/vk/push/common/clientid/ClientIdType;");

        jobject client_id_type = NULL;
        if (aidType == dmRustorePush::CLIENT_GAID) {
            client_id_type = env->GetStaticObjectField(client_id_type_class, fd_client_id_type_gaid);
        } else if (aidType == dmRustorePush::CLIENT_OAID) {
            client_id_type = env->GetStaticObjectField(client_id_type_class, fd_client_id_type_oaid);
        }

        jstring aid_string = env->NewStringUTF(aid);
        jobject client_id = env->NewObject(client_id_class, client_id_class_constructor, aid_string, client_id_type);
        
        env->DeleteLocalRef(aid_string);
        env->DeleteLocalRef(client_id_type);

        return client_id;
    }

    return NULL;
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
    jclass client_id_callback_jni_class = dmAndroid::LoadClass(env, "ru.rustore.defoldpush.ClientIdCallbackJNI");

    g_Push.m_Start = env->GetMethodID(push_class, "start", "(Landroid/app/Activity;Lru/rustore/defoldpush/IPushListener;Lcom/vk/push/common/clientid/ClientIdCallback;Ljava/lang/String;)V");
    g_Push.m_Stop = env->GetMethodID(push_class, "stop", "()V");
    g_Push.m_NewToken = env->GetMethodID(push_class, "newToken", "(Landroid/app/Activity;)V");
    g_Push.m_DeleteToken = env->GetMethodID(push_class, "deleteToken", "(Landroid/app/Activity;)V");
    g_Push.m_TopicSubscribe = env->GetMethodID(push_class, "topicSubscribe", "(Landroid/app/Activity;Ljava/lang/String;)V");
    g_Push.m_TopicUnsubscribe = env->GetMethodID(push_class, "topicUnsubscribe", "(Landroid/app/Activity;Ljava/lang/String;)V");
    g_Push.m_ShowToast = env->GetMethodID(push_class, "showToast", "(Landroid/app/Activity;Ljava/lang/String;)V");
    g_Push.m_CopyToClipboard = env->GetMethodID(push_class, "copyToClipboard", "(Landroid/app/Activity;Ljava/lang/String;)V");
    
    jmethodID get_instance_method = env->GetStaticMethodID(push_class, "getInstance", "()Lru/rustore/defoldpush/Push;");
    g_Push.m_Push = env->NewGlobalRef(env->CallStaticObjectMethod(push_class, get_instance_method));

    jmethodID set_listener = env->GetMethodID(push_class, "setApplicationListener", "(Landroid/app/Activity;)V");
    env->CallVoidMethod(g_Push.m_Push, set_listener, dmGraphics::GetNativeAndroidActivity());

    jmethodID jni_push_constructor = env->GetMethodID(push_jni_class, "<init>", "()V");
    g_Push.m_PushJNI = env->NewGlobalRef(env->NewObject(push_jni_class, jni_push_constructor));

    jmethodID jni_client_id_constructor = env->GetMethodID(client_id_callback_jni_class, "<init>", "()V");
    g_Push.m_ClientIdCallbackJNI = env->NewGlobalRef(env->NewObject(client_id_callback_jni_class, jni_client_id_constructor));

    const char* project_title = dmConfigFile::GetString(params->m_ConfigFile, "project.title", "");
    jstring project_title_string = env->NewStringUTF(project_title);

    env->CallVoidMethod(
        g_Push.m_Push, 
        g_Push.m_Start, 
        dmGraphics::GetNativeAndroidActivity(), 
        g_Push.m_PushJNI, 
        g_Push.m_ClientIdCallbackJNI, 
        project_title_string
    );

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
        env->DeleteGlobalRef(g_Push.m_ClientIdCallbackJNI);
    }
    g_Push.m_Push = NULL;
    g_Push.m_PushJNI = NULL;
    g_Push.m_ClientIdCallbackJNI = NULL;

    dmRustorePush::QueueDestroy(&g_Push.m_CommandQueue);

    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializePush(dmExtension::Params* params)
{
    lua_State* L = params->m_L;
    int top = lua_gettop(L);
    luaL_register(L, LIB_NAME, Push_methods);

#define SETCONSTANT(name, val) \
        lua_pushnumber(L, (lua_Number) val); \
        lua_setfield(L, -2, #name);\

    SETCONSTANT(CLIENT_AID_NOT_AVAILABLE, dmRustorePush::CLIENT_AID_NOT_AVAILABLE);
    SETCONSTANT(CLIENT_GAID, dmRustorePush::CLIENT_GAID);
    SETCONSTANT(CLIENT_OAID, dmRustorePush::CLIENT_OAID);

#undef SETCONSTANT


    lua_pop(L, 1);
    assert(top == lua_gettop(L));
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizePush(dmExtension::Params* params)
{
    if (g_Push.m_MessagesListener) {
        dmScript::DestroyCallback(g_Push.m_MessagesListener);
        g_Push.m_MessagesListener = 0;
    }
    if (g_Push.m_NewTokenListener) {
        dmScript::DestroyCallback(g_Push.m_NewTokenListener);
        g_Push.m_NewTokenListener = 0;
    }
    if (g_Push.m_CallbackClientId) {
        dmScript::DestroyCallback(g_Push.m_CallbackClientId);
        g_Push.m_CallbackClientId = 0;
    }

    if (g_Push.m_Callback) {
        dmScript::DestroyCallback(g_Push.m_Callback);
        g_Push.m_Callback = 0;
    }

    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(RustorePushExtExternal, "RuStorePush", AppInitializePush, AppFinalizePush, InitializePush, UpdatePush, 0, FinalizePush)
#endif // DM_PLATFORM_ANDROID
