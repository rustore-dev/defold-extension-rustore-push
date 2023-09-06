package ru.rustore.defoldpush;

public interface IPushListener {
    public void onNewToken(String pushToken, String errorMessage);
    public void onMessage(String json, boolean wasActivated);
}
