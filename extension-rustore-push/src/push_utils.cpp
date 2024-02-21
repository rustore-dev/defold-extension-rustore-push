#if defined(DM_PLATFORM_ANDROID)
#include <dmsdk/sdk.h>
#include <stdlib.h> // free

#include "push_utils.h"

static void PushError(lua_State* L, const char* error)
{
    if (error != 0) {
        lua_newtable(L);
        lua_pushstring(L, "error");
        lua_pushstring(L, error);
        lua_rawset(L, -3);
    } else {
        lua_pushnil(L);
    }
}

static void HandleNewTokenResult(const dmRustorePush::Command* cmd)
{
    if (!dmScript::IsCallbackValid(cmd->m_Callback))
    {
        return;
    }

    lua_State* L = dmScript::GetCallbackLuaContext(cmd->m_Callback);
    DM_LUA_STACK_CHECK(L, 0);

    if (!dmScript::SetupCallback(cmd->m_Callback))
    {
        return;
    }

    if (cmd->m_Result) {
        lua_pushstring(L, cmd->m_Result);
        lua_pushnil(L);
    } else {
        lua_pushnil(L);
        PushError(L, cmd->m_Error);
        dmLogError("HandleNewTokenResult: %s", cmd->m_Error);
    }

    int ret = dmScript::PCall(L, 3, 0);
    (void)ret;

    dmScript::TeardownCallback(cmd->m_Callback);
}


static void HandlePushMessageResult(const dmRustorePush::Command* cmd)
{
    if (!dmScript::IsCallbackValid(cmd->m_Callback))
    {
        return;
    }

    lua_State* L = dmScript::GetCallbackLuaContext(cmd->m_Callback);
    DM_LUA_STACK_CHECK(L, 0);

    if (!dmScript::SetupCallback(cmd->m_Callback))
    {
        return;
    }

    const char* json = (const char*) cmd->m_Result;

    dmScript::JsonToLua(L, json, strlen(json));
    lua_pushboolean(L, cmd->m_WasActivated);
    lua_pushstring(L, cmd->m_From);

    int ret = dmScript::PCall(L, 4, 0);
    (void)ret;

    dmScript::TeardownCallback(cmd->m_Callback);
}

static void HandleDeleteTokenResult(const dmRustorePush::Command* cmd)
{
    if (!dmScript::IsCallbackValid(cmd->m_Callback))
    {
        return;
    }

    lua_State* L = dmScript::GetCallbackLuaContext(cmd->m_Callback);
    DM_LUA_STACK_CHECK(L, 0);

    if (!dmScript::SetupCallback(cmd->m_Callback))
    {
        return;
    }

    if (cmd->m_Error) {
        PushError(L, cmd->m_Error);
        dmLogError("HandleDeleteTokenResult: %s", cmd->m_Error);
    } else {
        lua_pushnil(L);
    }

    int ret = dmScript::PCall(L, 2, 0);
    (void)ret;

    dmScript::TeardownCallback(cmd->m_Callback);
}

static void HandleTopicSubscribeResult(const dmRustorePush::Command* cmd)
{
    if (!dmScript::IsCallbackValid(cmd->m_Callback))
    {
        return;
    }

    lua_State* L = dmScript::GetCallbackLuaContext(cmd->m_Callback);
    DM_LUA_STACK_CHECK(L, 0);

    if (!dmScript::SetupCallback(cmd->m_Callback))
    {
        return;
    }

    if (cmd->m_Error) {
        PushError(L, cmd->m_Error);
        dmLogError("HandleTopicSubscribeResult: %s", cmd->m_Error);
    } else {
        lua_pushnil(L);
    }

    int ret = dmScript::PCall(L, 2, 0);
    (void)ret;

    dmScript::TeardownCallback(cmd->m_Callback);
}

static void HandleTopicUnsubscribeResult(const dmRustorePush::Command* cmd)
{
    if (!dmScript::IsCallbackValid(cmd->m_Callback))
    {
        return;
    }

    lua_State* L = dmScript::GetCallbackLuaContext(cmd->m_Callback);
    DM_LUA_STACK_CHECK(L, 0);

    if (!dmScript::SetupCallback(cmd->m_Callback))
    {
        return;
    }

    if (cmd->m_Error) {
        PushError(L, cmd->m_Error);
        dmLogError("HandleTopicUnsubscribeResult: %s", cmd->m_Error);
    } else {
        lua_pushnil(L);
    }

    int ret = dmScript::PCall(L, 2, 0);
    (void)ret;

    dmScript::TeardownCallback(cmd->m_Callback);
}

void dmRustorePush::HandleCommand(dmRustorePush::Command* cmd, void* ctx)
{
    switch (cmd->m_Command) {
        case dmRustorePush::COMMAND_TYPE_NEW_TOKEN_RESULT:    HandleNewTokenResult(cmd); break;
        case dmRustorePush::COMMAND_TYPE_PUSH_MESSAGE_RESULT: HandlePushMessageResult(cmd); break;
        case dmRustorePush::COMMAND_TYPE_DELETE_TOKEN_RESULT: HandleDeleteTokenResult(cmd); break;
        case dmRustorePush::COMMAND_TYPE_SUBSCRIBE_TOPIC_RESULT: HandleTopicSubscribeResult(cmd); break;
        case dmRustorePush::COMMAND_TYPE_UNSUBSCRIBE_TOPIC_RESULT: HandleTopicUnsubscribeResult(cmd); break;
        default: assert(false);
    }

    free((void*)cmd->m_Result);
    free((void*)cmd->m_Error);
    free((void*)cmd->m_From);

    // free callbacks
    switch (cmd->m_Command) {
        case dmRustorePush::COMMAND_TYPE_DELETE_TOKEN_RESULT:
        case dmRustorePush::COMMAND_TYPE_SUBSCRIBE_TOPIC_RESULT:
        case dmRustorePush::COMMAND_TYPE_UNSUBSCRIBE_TOPIC_RESULT:
            if(dmScript::IsCallbackValid(cmd->m_Callback))
                dmScript::DestroyCallback(cmd->m_Callback);
            break;
    }
}

void dmRustorePush::QueueCreate(CommandQueue* queue)
{
    queue->m_Mutex = dmMutex::New();
}

void dmRustorePush::QueueDestroy(CommandQueue* queue)
{
    {
        DM_MUTEX_SCOPED_LOCK(queue->m_Mutex);
        queue->m_Commands.SetSize(0);
    }
    dmMutex::Delete(queue->m_Mutex);
}

void dmRustorePush::QueuePush(CommandQueue* queue, Command* cmd)
{
    DM_MUTEX_SCOPED_LOCK(queue->m_Mutex);

    if(queue->m_Commands.Full())
    {
        queue->m_Commands.OffsetCapacity(2);
    }
    queue->m_Commands.Push(*cmd);
}

void dmRustorePush::QueueFlush(CommandQueue* queue, CommandFn fn, void* ctx)
{
    assert(fn != 0);
    if (queue->m_Commands.Empty())
    {
        return;
    }

    dmArray<Command> tmp;
    {
        DM_MUTEX_SCOPED_LOCK(queue->m_Mutex);
        tmp.Swap(queue->m_Commands);
    }

    for(uint32_t i = 0; i != tmp.Size(); ++i)
    {
        fn(&tmp[i], ctx);
    }
}

#endif // DM_PLATFORM_ANDROID
