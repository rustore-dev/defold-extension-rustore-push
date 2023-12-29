package ru.rustore.defoldpush;

public interface IPushListener {
    public void onNewToken(String pushToken, String errorMessage);
    public void onMessage(String json, boolean wasActivated, String from);
    public void onDeleteToken(String errorMessage);
    public void onSubscribeToTopic(String errorMessage);
    public void onUnsubscribeFromTopic(String errorMessage);
}
