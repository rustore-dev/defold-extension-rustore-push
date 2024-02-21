package ru.rustore.defoldpush;

import java.io.PrintStream;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class PushDispatchActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        Bundle extras = getIntent().getExtras();
        if (extras != null) {
            String payload = extras.getString("payload");
            String from = extras.getString("from");

            Push.getInstance().onPush(this, from, payload, true);
            
            Intent intent = getPackageManager().getLaunchIntentForPackage(getPackageName());
            intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
            startActivity(intent);
        } else {
            Log.e(Push.TAG, "Unable to queue message. extras is null");
        }

        super.onCreate(savedInstanceState);
        finish();
    }
}
