package ru.rustore.defoldpush;

import com.vk.push.common.clientid.ClientId;
import com.vk.push.common.clientid.ClientIdCallback;

public class ClientIdCallbackJNI implements ClientIdCallback {
    public ClientIdCallbackJNI() {
    }

    public native ClientId getClientId();
}
