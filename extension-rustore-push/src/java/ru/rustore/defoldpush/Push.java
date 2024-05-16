package ru.rustore.defoldpush;

import java.util.Map;

import kotlin.Unit;

import org.json.JSONObject;
import org.json.JSONException;

import android.app.Application;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.app.PendingIntent;
import android.content.ClipboardManager;
import android.content.ClipData;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.os.Build;
import android.util.Log;
import android.widget.Toast;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import androidx.core.app.NotificationCompat;

import ru.rustore.sdk.pushclient.RuStorePushClient;

import com.vk.push.common.clientid.ClientId;
import com.vk.push.common.clientid.ClientIdCallback;

public class Push implements ClientIdCallback  {
    public static final String TAG = "RustorePushProcessor";
    public static final String DEFOLD_ACTIVITY = "com.dynamo.android.DefoldActivity";
    public static final String ACTION_FORWARD_PUSH = "ru.rustore.defoldpush.FORWARD";
    public static final String NOTIFICATION_CHANNEL_ID = "com.dynamo.android.notification_channel";
    public static final String CLIP_DATA_TOOLTIP = "Copied Text";

    private static Push instance;

    private IPushListener listener = null;
    private ClientIdCallback clientIdCallback = null;

    private Gson gson = new GsonBuilder()
        .registerTypeAdapter(Uri.class, new UriTypeAdapter())
        .create();

    private static boolean defoldActivityVisible;
    public static boolean isDefoldActivityVisible() {
        Log.d(Push.TAG, "Tracking Activity isVisible= " + defoldActivityVisible);
        return defoldActivityVisible;
    }

    static final private class ActivityListener implements Application.ActivityLifecycleCallbacks {
        public void onActivityResumed(Activity activity) {
            if (activity.getLocalClassName().equals(DEFOLD_ACTIVITY)) {
                defoldActivityVisible = true;
                Log.d(Push.TAG, "Tracking Activity Resumed "+activity.getLocalClassName());
            }
        }

        public void onActivityPaused(Activity activity) {
            if (activity.getLocalClassName().equals(DEFOLD_ACTIVITY)) {
                defoldActivityVisible = false;
                Log.d(Push.TAG, "Tracking Activity Paused "+activity.getLocalClassName());
            }
        }

        public void onActivityDestroyed(Activity activity) {
        }

        public void onActivitySaveInstanceState(Activity activity, Bundle bundle) {
        }

        public void onActivityStopped(Activity activity) {
        }

        public void onActivityStarted(Activity activity) {
        }

        public void onActivityCreated(Activity activity, Bundle bundle) {
        }
    }

    public void setApplicationListener(Activity activity) {
        defoldActivityVisible = true;
        activity.getApplication().registerActivityLifecycleCallbacks(new ActivityListener());
    }

    public void start(
        Activity activity, 
        IPushListener listener,
        ClientIdCallback clientIdCallback,
        String projectTitle
    ) {
        Log.d(Push.TAG, "Push started");

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                NOTIFICATION_CHANNEL_ID, 
                projectTitle, 
                NotificationManager.IMPORTANCE_DEFAULT
            );

            channel.enableVibration(true);
            channel.setDescription("");

            NotificationManager notificationManager = activity.getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }

        this.listener = listener;
        this.clientIdCallback = clientIdCallback;
    }

    public void stop() {
        Log.d(Push.TAG, "Push stopped");
        this.listener = null;
    }

    public void newToken(final Activity activity) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                getToken(activity);
            }
        });
    }

    public static Push getInstance() {
        if (instance == null) {
            instance = new Push();
        }
        return instance;
    }

    public ClientId getClientId() {
        if (clientIdCallback != null) {
            return clientIdCallback.getClientId();
        }
        return null;
    }

    public void sendToken(String token) {
        sendOnNewTokenResult(token, null);
    }

    private void getToken(Activity activity) {
        RuStorePushClient.INSTANCE
            .getToken()
            .addOnSuccessListener(result -> {
                Log.d(Push.TAG, "getToken onSuccess token = " + result);
                sendOnNewTokenResult(result, null);
            })
            .addOnFailureListener(throwable -> {
                Log.e(Push.TAG, "getToken onFailure", throwable);
                sendOnNewTokenResult(null, "Failed to get push token");
            });
    }

    public void deleteToken(final Activity activity) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                deleteToken();
            }
        });
    }

    private void deleteToken() {
        RuStorePushClient
            .INSTANCE
            .deleteToken()
            .addOnSuccessListener(result -> {
                Log.d(Push.TAG, "deleteToken onSuccess");
                sendOnDeleteTokenResult(null);
            })
            .addOnFailureListener(throwable -> {
                Log.e(Push.TAG, "deleteToken onFailure", throwable);
                sendOnDeleteTokenResult("Failed to delete push token");
            });
    }

    public void topicSubscribe(final Activity activity, String topic) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                subscribeToTopic(topic);
            }
        });
    }

    private void subscribeToTopic(String topic) {
        RuStorePushClient
            .INSTANCE
            .subscribeToTopic(topic)
            .addOnSuccessListener(result -> {
                Log.d(Push.TAG, "subscribeToTopic onSuccess");
                sendOnTopicSubscribe(null);
            })
            .addOnFailureListener(throwable -> {
                Log.e(Push.TAG, "subscribeToTopic onFailure", throwable);
                sendOnTopicSubscribe("Failed to subscribe to topic=" + topic);
            });
    }

    public void topicUnsubscribe(final Activity activity, String topic) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                unsubscribeFromTopic(topic);
            }
        });
    }
    private void unsubscribeFromTopic(String topic) {
        RuStorePushClient
            .INSTANCE
            .unsubscribeFromTopic(topic)
            .addOnSuccessListener(result -> {
                Log.d(Push.TAG, "unsubscribeFromTopic onSuccess");
                sendOnTopicUnsubscribe(null);
            })
            .addOnFailureListener(throwable -> {
                Log.e(Push.TAG, "unsubscribeFromTopic onFailure", throwable);
                sendOnTopicUnsubscribe("Failed to unsubscribe from topic=" + topic);
            });
    }
    
    private void sendOnNewTokenResult(String pushToken, String errorMessage) {
        if (listener != null) {
            listener.onNewToken(pushToken, errorMessage);
        } else {
            Log.e(Push.TAG, "No listener set");
        }
    }

    private void sendOnDeleteTokenResult(String errorMessage) {
        if (listener != null) {
            listener.onDeleteToken(errorMessage);
        } else {
            Log.e(Push.TAG, "No listener set");
        }
    }

    private void sendOnTopicSubscribe(String errorMessage) {
        if (listener != null) {
             listener.onSubscribeToTopic(errorMessage);
        } else {
            Log.e(Push.TAG, "No listener set");
        }
    }

    private void sendOnTopicUnsubscribe(String errorMessage) {
        if (listener != null) {
             listener.onUnsubscribeFromTopic(errorMessage);
        } else {
            Log.e(Push.TAG, "No listener set");
        }
    }

    static JSONObject toJson(Map<String, String> bundle) {
        JSONObject o = new JSONObject();
        for (Map.Entry<String, String> entry : bundle.entrySet()) {
            try {
                o.put(entry.getKey(), entry.getValue());
            } catch (JSONException e) {
                Log.e(Push.TAG, "failed to create json-object", e);
            }
        }
        
        return o;
    }

    private int createPendingIntentFlags(int flags) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            flags = flags | PendingIntent.FLAG_IMMUTABLE;
        }
        return flags;
    }

    public void showNotification(Context context, String from,  Map<String, String> extras, boolean withNotification, ru.rustore.sdk.pushclient.messaging.model.Notification notification) {
        JSONObject payloadJson = toJson(extras);
        String payloadString = payloadJson.toString();
        String notificationJson = gson.toJson(notification);

        // if was notification push is showed
        if (isDefoldActivityVisible() || withNotification) {
            onPush(context, from, payloadString, false, notificationJson);
            return;
        }

        Intent intent = new Intent(context, PushDispatchActivity.class).setAction(ACTION_FORWARD_PUSH);
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);

        intent.putExtra("payload", payloadString);
        intent.putExtra("from", from);
        intent.putExtra("notification", notificationJson);
        intent.putExtra("rustorepushlaunch", true);

        int id = (int) (System.currentTimeMillis() % Integer.MAX_VALUE);
        final int flags = createPendingIntentFlags(PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_ONE_SHOT);
        PendingIntent contentIntent = PendingIntent.getActivity(context, id, intent, flags);

        String fieldTitle = null;
        String fieldText = null;
        
        // Try to find field names from manifest file
        PackageManager pm = context.getPackageManager();
        try {
            ComponentName cn = new ComponentName(context, DEFOLD_ACTIVITY);
            ActivityInfo activityInfo = pm.getActivityInfo(cn, PackageManager.GET_META_DATA);

            Bundle bundle = activityInfo.metaData;
            if (bundle != null) {
                fieldTitle = bundle.getString("ru.rustore.defoldpush.field_title", "title");
                fieldText = bundle.getString("ru.rustore.defoldpush.field_text", "message");
            } else {
                Log.w(TAG, "Bundle was null, could not get meta data from manifest.");
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "Could not get activity info, needed to get push field conversion.");
        }

        ApplicationInfo info = context.getApplicationInfo();
        String title = info.loadLabel(pm).toString();
        if (fieldTitle != null && extras.get(fieldTitle) != null) {
            title = extras.get(fieldTitle);
        }

        String text = extras.get(fieldText);
        if (text == null) {
            Log.w(Push.TAG, "Missing text field in push message");
            text = "New message";
        }

        NotificationCompat.Builder builder = new NotificationCompat.Builder(context, NOTIFICATION_CHANNEL_ID)
            .setContentTitle(title)
            .setStyle(new NotificationCompat.BigTextStyle().bigText(text))
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .setContentText(text);

        // Find icons if they were supplied, fallback to app icon
        int smallIconId = context.getResources().getIdentifier("push_icon_small", "drawable", context.getPackageName());
        int largeIconId = context.getResources().getIdentifier("push_icon_large", "drawable", context.getPackageName());
        if (smallIconId == 0) {
            smallIconId = info.icon;
            if (smallIconId == 0) {
                smallIconId = android.R.color.transparent;
            }
        }

        if (largeIconId == 0) {
            largeIconId = info.icon;
            if (largeIconId == 0) {
                largeIconId = android.R.color.transparent;
            }
        }

        // Get bitmap for large icon resource
        try {
            Resources resources = pm.getResourcesForApplication(info);
            Bitmap largeIconBitmap = BitmapFactory.decodeResource(resources, largeIconId);
            builder.setLargeIcon(largeIconBitmap);
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "Could not get application resources.");
        }

        builder.setSmallIcon(smallIconId);
        builder.setContentIntent(contentIntent);

        NotificationManager nm = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        Notification notificationObject = builder.build();
        notificationObject.defaults = Notification.DEFAULT_ALL;
        notificationObject.flags |= Notification.FLAG_AUTO_CANCEL;
        nm.notify(id, notificationObject);
    }

    public void onPush(Context context, String from, String payload, boolean wasActivated, String notification) {
        if (listener != null) {
            listener.onMessage(payload, wasActivated, from, notification);
            Log.d(Push.TAG, "send to listener");
            return;
        }
        Log.e(Push.TAG, "listener not inited");
    }

    public void showToast(final Activity activity, String message) {
        activity.runOnUiThread(() -> Toast.makeText(activity, message, Toast.LENGTH_LONG).show());
    }

    public void copyToClipboard(final Activity activity, String text) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) return;

        if (activity != null) {
            activity.runOnUiThread(() -> {
                ClipboardManager clipboard = (ClipboardManager) activity.getSystemService(Context.CLIPBOARD_SERVICE);
                ClipData clip = ClipData.newPlainText(CLIP_DATA_TOOLTIP, text);
                clipboard.setPrimaryClip(clip);
            });
        }
    }
}
