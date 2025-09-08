package org.example.frameworks;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class ClassScanner {

    /**
     * Scans all classes in the specified package for those annotated with @Subcmds
     * @param packageName The package to scan
     * @return List of classes with @Subcmds annotation
     * @throws ClassNotFoundException
     * @throws IOException
     */
    public static List<Class<?>> findSubcmdClasses(String packageName) 
            throws ClassNotFoundException, IOException {
        ClassLoader classLoader = Thread.currentThread().getContextClassLoader();
        String path = packageName.replace('.', '/');
        Enumeration<URL> resources = classLoader.getResources(path);
        
        List<Class<?>> Classes = new ArrayList<>();
        
        while (resources.hasMoreElements()) {
            URL resource = resources.nextElement();
            // System.out.println(resource); 
            // Out: file:/path/to/app/build/classes/java/main/org/example
            File directory = new File(resource.getFile());
            // System.out.println(directory);
            // Out: /path/to/app/build/classes/java/main/org/example
            if (directory.exists()) {
                Classes.addAll(findClassesInDirectory(directory, packageName));
            }
        }
        
        return Classes;
    }

    private static List<Class<?>> findClassesInDirectory(File directory, String packageName) 
            throws ClassNotFoundException {
        List<Class<?>> classes = new ArrayList<>();
        if (!directory.exists()) {
            return classes;
        }
        
        File[] files = directory.listFiles();
        if (files == null) {
            return classes;
        }
        
        for (File file : files) {
            if (file.isDirectory()) {
                classes.addAll(findClassesInDirectory(file, 
                    packageName + "." + file.getName()));
            } else if (file.getName().endsWith(".class")) {
                String className = packageName + '.' + 
                    file.getName().substring(0, file.getName().length() - 6);
                // No magic here, using `basename(file)`, does it work on jar file?
                Class<?> clazz = Class.forName(className);
                classes.add(clazz);
            }
        }
        return classes;
    }

    // Example usage
    public static void testClassScanner() {
        try {
            List<Class<?>> subcmdClasses = findSubcmdClasses("org.example");
            for (Class<?> clazz : subcmdClasses) {
                System.out.println(clazz.getName());
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
