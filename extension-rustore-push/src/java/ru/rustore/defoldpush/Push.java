package ru.rustore.defoldpush;

import android.app.Application;

import java.util.Map;

import org.json.JSONObject;
import org.json.JSONException;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Build;
import android.util.Log;

import androidx.core.app.NotificationCompat;

import ru.rustore.sdk.pushclient.RuStorePushClient;
import ru.rustore.sdk.pushclient.common.logger.DefaultLogger;
import ru.rustore.sdk.core.tasks.OnCompleteListener;

public class Push {
    public static final String TAG = "PushProcessor";
    public static final String DEFOLD_ACTIVITY = "com.dynamo.android.DefoldActivity";
    public static final String ACTION_FORWARD_PUSH = "ru.rustore.defoldpush.FORWARD";
    public static final String NOTIFICATION_CHANNEL_ID = "com.dynamo.android.notification_channel";

    private String rustoreProjectId = "";
    private static Push instance;

    private IPushListener listener = null;

    private static boolean defoldActivityVisible;
    public static boolean isDefoldActivityVisible() {
        Log.d(TAG, "Tracking Activity isVisible= " + defoldActivityVisible);
        return defoldActivityVisible;
    }

    final private class ActivityListener implements Application.ActivityLifecycleCallbacks {
        public final void onActivityResumed(Activity activity) {
            if (activity.getLocalClassName().equals(DEFOLD_ACTIVITY)) {
                defoldActivityVisible = true;
                Log.d(TAG, "Tracking Activity Resumed "+activity.getLocalClassName());
            }
        }

        public final void onActivityPaused(Activity activity) {
            if (activity.getLocalClassName().equals(DEFOLD_ACTIVITY)) {
                defoldActivityVisible = false;
                Log.d(TAG, "Tracking Activity Paused "+activity.getLocalClassName());
            }
        }

        public final void onActivityDestroyed(Activity activity) {
        }

        public final void onActivitySaveInstanceState(Activity activity, Bundle bundle) {
        }

        public final void onActivityStopped(Activity activity) {
        }

        public final void onActivityStarted(Activity activity) {
        }

        public final void onActivityCreated(Activity activity, Bundle bundle) {
        }
    }

    public void setApplicationListener(Activity activity) {
        defoldActivityVisible = true;
        activity.getApplication().registerActivityLifecycleCallbacks(new ActivityListener());
    }

    public void start(Activity activity, IPushListener listener, String rustoreProjectId, String projectTitle) {
        Log.d(TAG, String.format("Push started (%s %s)", listener, rustoreProjectId));

        NotificationChannel channel = new NotificationChannel(
            NOTIFICATION_CHANNEL_ID, 
            projectTitle, 
            NotificationManager.IMPORTANCE_DEFAULT
        );

        channel.enableVibration(true);
        channel.setDescription("");

        NotificationManager notificationManager = activity.getSystemService(NotificationManager.class);
        notificationManager.createNotificationChannel(channel);

        this.listener = listener;
        this.rustoreProjectId = rustoreProjectId;
    }

    public void stop() {
        Log.d(TAG, "Push stopped");
        this.listener = null;
    }

    public void newToken(final Activity activity) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                initRustore(activity);
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

    public void sendToken(String token) {
        sendOnNewTokenResult(token, null);
    }

    private void initRustore(Activity activity) {
        if (this.rustoreProjectId == null || this.rustoreProjectId == "") {
            Log.w(Push.TAG, "Rustore application Id must be set.");
            return;
        }

        RuStorePushClient.INSTANCE.init(
            activity.getApplication(),
            this.rustoreProjectId,
            new DefaultLogger(Push.TAG),
            null,
            null,
            false
        );
    }

    private void getToken(Activity activity) {
        RuStorePushClient.INSTANCE
            .getToken()
            .addOnCompleteListener(new OnCompleteListener<String>() {
                @Override
                public void onSuccess(String result) {
                    Log.d(Push.TAG, "getToken onSuccess token = " + result);
                    sendToken(result);
                }
                @Override
                public void onFailure(Throwable throwable) {
                    Log.e(Push.TAG, "getToken onFailure", throwable);
                    sendOnNewTokenResult(null, "Failed to get push token");
                }
         });
    }
    
    private void sendOnNewTokenResult(String pushToken, String errorMessage) {
        if (listener != null) {
            listener.onNewToken(pushToken, errorMessage);
        } else {
            Log.e(TAG, "No listener new token callback set");
        }
    }

    static JSONObject toJson(Map<String, String> bundle) {
        JSONObject o = new JSONObject();
        for (Map.Entry<String, String> entry : bundle.entrySet()) {
            try {
                o.put(entry.getKey(), entry.getValue());
            } catch (JSONException e) {
                Log.e(TAG, "failed to create json-object", e);
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

    public void showNotification(Context context, Map<String, String> extras, boolean withNotification) {
        JSONObject payloadJson = toJson(extras);
        String payloadString = payloadJson.toString();

        // if was notification push is showed
        if (isDefoldActivityVisible() || withNotification) {
            onPush(context, payloadString, false);
            return;
        }

        Intent intent = new Intent(context, PushDispatchActivity.class).setAction(ACTION_FORWARD_PUSH);
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);

        intent.putExtra("payload", payloadString);

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
            Log.w(TAG, "Missing text field in push message");
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
        Notification notification = builder.build();
        notification.defaults = Notification.DEFAULT_ALL;
        notification.flags |= Notification.FLAG_AUTO_CANCEL;
        nm.notify(id, notification);
    }

    public void onPush(Context context, String payload, boolean wasActivated) {
        if (listener != null) {
            listener.onMessage(payload, wasActivated);
            Log.d(Push.TAG, "send to listener");
            return;
        }
        Log.e(Push.TAG, "listener not inited");
    }
}

