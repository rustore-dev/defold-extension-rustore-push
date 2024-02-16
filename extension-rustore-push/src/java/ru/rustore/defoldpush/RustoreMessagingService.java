package ru.rustore.defoldpush;

import ru.rustore.sdk.pushclient.messaging.service.RuStoreMessagingService;
import ru.rustore.sdk.pushclient.messaging.model.RemoteMessage;
import ru.rustore.sdk.pushclient.messaging.model.Notification;

import android.util.Log;

public class RustoreMessagingService extends RuStoreMessagingService {
    private String TAG = "PushListenerService";

    @Override
    public void onNewToken(String token) {
        Log.d(TAG, "onNewToken token = " + token);
        Push.getInstance().sendToken(token);
    }

    @Override
    public void onMessageReceived(RemoteMessage message) {
        Log.d(TAG, "onMessageReceived = " + message);
        if (message.getData().size() > 0) {
            boolean withNotification = false;
            if(message.getNotification() != null && message.getNotification().getTitle() != null) {
                withNotification = true;
                Log.w(TAG, "with notification title");
            }
            Log.d(TAG, "onMessageReceived from = " + message.getFrom() + " data = " + message.getData());
            Push.getInstance().showNotification(this, message.getFrom(), message.getData(), withNotification);
        }
   }
}
