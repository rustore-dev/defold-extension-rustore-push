package ru.rustore.defoldpush;

import com.vk.push.common.clientid.ClientId;
import com.vk.push.common.clientid.ClientIdCallback;

public class PushJNI implements IPushListener {

    public PushJNI() {
    }
    @Override
    public native void onNewToken(String pushToken, String errorMessage);

    @Override
    public native void onMessage(String json, boolean wasActivated, String from, String notification);
    
    @Override
    public native void onDeleteToken(String errorMessage);

    @Override
    public native void onSubscribeToTopic(String errorMessage);
    
    @Override
    public native void onUnsubscribeFromTopic(String errorMessage);

}
