package ru.rustore.defoldpush;

import android.net.Uri;
import com.google.gson.TypeAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;
import java.io.IOException;

public class UriTypeAdapter extends TypeAdapter<Uri> {
    @Override
    public void write(JsonWriter out, Uri value) throws IOException {
        out.value(value != null ? value.toString() : null);
    }

    @Override
    public Uri read(JsonReader in) throws IOException {
        String uriString = in.nextString();
        return Uri.parse(uriString);
    }
}
