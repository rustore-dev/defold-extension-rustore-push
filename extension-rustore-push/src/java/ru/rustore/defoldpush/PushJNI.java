package ru.rustore.defoldpush;

public class PushJNI implements IPushListener {

    public PushJNI() {
    }

    public native void onNewToken(String pushToken, String errorMessage);
    
    public native void onMessage(String json, boolean wasActivated);
}
