package ru.rustore.defoldpush;

import android.content.Intent;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;

public class PushDefoldActivity extends AppCompatActivity{
    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);

        // Processing intents

        Class<?> gameActivityClass = null;
        try {
            gameActivityClass = Class.forName(Push.DEFOLD_ACTIVITY);
        } catch(ClassNotFoundException ex) {
            Log.w(Push.TAG, ex.getMessage());
        }

        if (gameActivityClass != null)
        {
            Intent newIntent = new Intent(this, gameActivityClass);
            startActivity(newIntent);
        }

        finish();
    }
}
