#if defined(DM_PLATFORM_ANDROID)
#ifndef DM_RUSTORE_PUSH_UTILS
#define DM_RUSTORE_PUSH_UTILS

#include <dmsdk/sdk.h>

namespace dmRustorePush
{
	enum ClientAdvertisingIdType
	{
		CLIENT_AID_NOT_AVAILABLE = 0,
		CLIENT_GAID 			 = 1,
		CLIENT_OAID 			 = 2,
	};

	enum CommandType
	{
		COMMAND_TYPE_NEW_TOKEN_RESULT  		  = 0,
		COMMAND_TYPE_PUSH_MESSAGE_RESULT  	  = 1,
		COMMAND_TYPE_DELETE_TOKEN_RESULT      = 2,
		COMMAND_TYPE_SUBSCRIBE_TOPIC_RESULT   = 3,
		COMMAND_TYPE_UNSUBSCRIBE_TOPIC_RESULT = 4,
	};

	struct Command
	{
		Command()
		{
			memset(this, 0, sizeof(Command));
		}
		dmScript::LuaCallbackInfo* m_Callback;

		uint32_t 	m_Command;
		const char* m_Result;
		const char* m_Error;
		bool     	m_WasActivated;
		const char* m_From;
		const char* m_Notification;
	};

	struct CommandQueue
	{
		dmArray<Command> m_Commands;
		dmMutex::HMutex  m_Mutex;
	};

	typedef void (*CommandFn)(Command* cmd, void* ctx);

	void QueueCreate(CommandQueue* queue);
	void QueueDestroy(CommandQueue* queue);
	void QueuePush(CommandQueue* queue, Command* cmd);
	void QueueFlush(CommandQueue* queue, CommandFn fn, void* ctx);

	void HandleCommand(dmRustorePush::Command* push, void* ctx);
}

#endif
#endif // DM_PLATFORM_ANDROID
