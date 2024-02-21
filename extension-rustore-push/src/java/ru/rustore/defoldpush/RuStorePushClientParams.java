package ru.rustore.defoldpush;

import java.util.HashMap;
import java.util.Map;

import android.content.Context;

import com.vk.push.common.clientid.ClientIdCallback;

import ru.rustore.sdk.pushclient.common.logger.DefaultLogger;
import ru.rustore.sdk.pushclient.common.logger.Logger;
import ru.rustore.sdk.pushclient.provider.AbstractRuStorePushClientParams;

public class RuStorePushClientParams extends AbstractRuStorePushClientParams {
    RuStorePushClientParams(Context context) {
        super(context);
    }

    @Override
    public Logger getLogger() {
        return new DefaultLogger(Push.TAG);
    }

    @Override
    public boolean getTestModeEnabled() {
        return false;
    }

    @Override
    public Map<String, Object> getInternalConfig() {
       HashMap<String, Object> map =  new HashMap<>(1);
       map.put("type", "defold");
       return map;
    }

    @Override
    public ClientIdCallback getClientIdCallback() {
        return Push.getInstance();
    }
}

