package ru.rustore.defoldpush;

import java.io.PrintStream;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class PushDispatchActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        boolean pushlaunch = false;

        Bundle extras = getIntent().getExtras();
        if (extras != null) {
            if (extras.containsKey("rustorepushlaunch")) {
                pushlaunch = !extras.getBoolean("rustorepushlaunch");
            }

            if (pushlaunch)
            {
                String payload = extras.getString("payload");
                String from = extras.getString("from");
                String notification = extras.getString("notification");

                Push.getInstance().onPush(this, from, payload, true, notification);

                Intent intent = getPackageManager().getLaunchIntentForPackage(getPackageName());
                intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
                startActivity(intent);
            }
        }

        if (!pushlaunch) {
            Class<?> gameActivityClass = null;
            try {
                gameActivityClass = Class.forName(Push.DEFOLD_ACTIVITY);
            } catch(ClassNotFoundException ex) {
                Log.w(Push.TAG, ex.getMessage());
            }

            if (gameActivityClass != null)
            {
                Intent intent = new Intent(this, gameActivityClass);
                startActivity(intent);
            }
        }

        super.onCreate(savedInstanceState);
        finish();
    }
}
