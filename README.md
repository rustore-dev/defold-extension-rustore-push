# Defold RuStore native extension for push messages

Defold [native extension](https://www.defold.com/manuals/extensions/) for Push Notifications functionality on Android for RuStore.

## Instalation

You can use the extension in your own project by adding this project as a Defold library dependency. Open your game.project file and in the dependencies field under project add:

<https://gitflic.ru/project/rustore/defold-extension-rustore-push/file/downloadAll?branch=master>

Or point to the ZIP file of a specific [release](https://gitflic.ru/project/rustore/defold-extension-rustore-push/release/).

## Configuration

### Rustore

Full documentation to register project and get service token on [Rustore help page][10]

### Game

The extension can be configured by adding the following fields to game.project(just open this file in any text editor)
Find `[android]` section, and add line:

```ini
[android]
rustore_project_id = %your project id%
package = %your package%
```

#### rustore_project_id

Your rustore project id from developer console, section pushes

#### package

Package name for your app, must be equal with rustore project

### Configuring the Manifest

To be able to receive push messages with `notification` and `android.notification.title`, you must make `PushDispatchActivity` the main activity. To do this, add the following entry to your project manifest:

```xml
<activity android:name="ru.rustore.defoldpush.PushDispatchActivity">
	<intent-filter>
		<action android:name="android.intent.action.MAIN" />
		<category android:name="android.intent.category.LAUNCHER" />
	</intent-filter>
</activity>
```

At the same time, you must remove the following `<intent-filter>` from the `com.dynamo.android.DefoldActivity activity`:

```xml
<intent-filter>
	<action android:name="android.intent.action.MAIN" />
	<category android:name="android.intent.category.LAUNCHER" />
</intent-filter>
```

To perform these actions, you can use an example `ExtendedAndroidManifest.xml`. Open your game.project and specify the path `/extension-rustore-push/manifests/android/ExtendedAndroidManifest.xml` in the `Manifest` field of the `Android` section.

### Default Push title and body

If you want add default push title and push body add folow params in `[android]` section

```ini
[android]
push_field_title = default push title
push_field_text = default push body
```

## Usage

Add this code to yor application.

```lua
local function listener(self, payload, activated)
    -- The payload arrives here.
    pprint(payload)
end

local function new_token(self, token, error)
    if token then
       print(token)
    else
       print(error.error)
    end
end

local function push_android()
    ruStorePush.set_client_id_callback(function(self)
        return "", ruStorePush.CLIENT_AID_NOT_AVAILABLE
    end)
    
    ruStorePush.set_on_token(new_token)
    ruStorePush.set_on_message(listener)
    
    print("Rustore pushes registered")
end

function init(self)
    local sysinfo = sys.get_sys_info()
    if sysinfo.system_name == "Android" then
        push_android()
    else
        print("Notifications work only Android")
    end

    msg.post(".", "acquire_input_focus")
end
```

Run and copy push token

```log
DEBUG:SCRIPT: Vdugxy8OC-fm2PfWsbBfbCJav4oskLDt
```

Now we can send push notification

```curl
curl --location 'https://vkpns.rustore.ru/v1/projects/%PROJECT_ID%/messages:send' \
--header 'Content-Type: application/json' \
--header 'Authorization: Bearer %SERVICE_TOKEN%' \
--data '{
   "message": {
      "token": "%PUSH_TOKEN%",
      "data": {
          "title": "Message title",
          "message": "message body",
          "some_key": "some value"
      }
    }
}
'
```

Replace `%PROJECT_ID%` with your project id from console.

Replace `%SERVICE_TOKEN%` with your service token from console.

Replace `%PUSH_TOKEN%` with your push token.

### Remove Push token

If you want to remove push token call function `delete_token`

```lua
ruStorePush.delete_token(function (self, error)
    if error then
        set_msg("Error deleting token: %s", error.error)
    else
        set_msg("push token deleted")
    end
end)
```

### Segments

If you want to add segments, you need to provide listener `set_client_id_callback`, listener must return AID, and type (GAID, OAID)

```lua
local function push_android()
    ruStorePush.set_client_id_callback(function(self)
        return "", ruStorePush.CLIENT_AID_NOT_AVAILABLE
    end)
    --- ....
    print("Rustore pushes registered")
end
```

### Topics

You can subscribe/unsubscribe from topics

```lua
ruStorePush.topic_subscribe("topic_name", function (self, error)
    if error then
        set_msg("Error to subscribe: %s", error.error)
    else
        set_msg("subscribe to: topic_name")
    end
end)

ruStorePush.topic_unsubscribe("topic_name", function (self, error)
    if error then
        set_msg("Error to subscribe: %s", error.error)
    else
        set_msg("unsubscribe from: topic_name")
    end
end)
```

### Important

Without installing `PushDispatchActivity` as the main activity do not send push messages with `notification` and `android.notification.title` it will be processed by RuStore. And do not work properly.

Send `Data` push with follow field:

- title - for push message title
- message - for push message body

[10]: https://www.rustore.ru/help/sdk/push-notifications/defold/2-3-0
