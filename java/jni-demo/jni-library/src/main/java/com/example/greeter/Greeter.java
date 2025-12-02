
package com.example.greeter;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.net.URL;
import java.nio.file.Files;

public class Greeter {

    static {
        System.loadLibrary( "jni-library");
    }

    public native String sayHello(String name);
}
