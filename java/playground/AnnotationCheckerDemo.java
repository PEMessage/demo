import java.lang.annotation.*; // for @interface

// for AnnotationCheckerDemo
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;

// for App
import java.util.HashMap;
import java.util.Map;
import java.lang.reflect.InvocationTargetException;

// See: https://www.baeldung.com/java-custom-annotation
@Retention(RetentionPolicy.RUNTIME) // has runtime visibility
@Target(ElementType.METHOD) // apply it to method
@interface Test {
}


@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
@interface Cmd {
    String value() default ""; // When creating custom annotations with methods
                               // 1. methods must have no parameters, and cannot throw an exception.
                               // 2. the return types are restricted to primitives, Strings Class Enum
}

class SampleClass {
    @Test
    public void testMethod1() {}

    @Cmd
    public static void cmdMethod1() {
        System.out.println("-- This is cmdMethod1");
    }

    @Cmd
    public static void cmdMethod2() {
        System.out.println("-- This is cmdMethod2");
    }

    @Cmd("aliasMethod")
    public static void cmdMethod3() {
        System.out.println("-- This is cmdMethod3");
    }

    public void regularMethod() {}
}


public class AnnotationCheckerDemo {
    public static void main(String[] args) throws Exception {
        System.out.println("-------------------------");
        System.out.println("Class: " + ClassContainAnnotation.class.getName());
        System.out.println("-------------------------");
        ClassContainAnnotation.testMain(args);
        System.out.println("-------------------------");
        System.out.println("Class: " + App.class.getName());
        System.out.println("-------------------------");
        App.testMain(args);
    }
}

class ClassContainAnnotation {
    private static boolean check(String className, Class<? extends Annotation> annotationClass)
            throws Exception {
        Class<?> cls = Class.forName(className);
        Method[] ms = cls.getDeclaredMethods();
        for (Method m : ms) {
            if (m.isAnnotationPresent(annotationClass)) {
                return true;
            }
        }
        return false;
    }
    public static void testMain(String[] args) throws Exception {
        System.out.println("Class: " + ClassContainAnnotation.class.getName());
        // Check for different annotations in SampleClass
        boolean hasTest = ClassContainAnnotation.check("SampleClass", Test.class);
        boolean hasCmd = ClassContainAnnotation.check("SampleClass", Cmd.class);

        System.out.println("Contains @Test methods: " + hasTest);
        System.out.println("Contains @Cmd methods: " + hasCmd);
    }
}

class App {
    private static final Map<String, Method> COMMAND_MAP = new HashMap<>();
    static {
        try {
            Class<?> classes = Class.forName("SampleClass");
            // Class<?> classes = SampleClass.class;
            Method[] methods = classes.getMethods();
            for (Method method : methods) {
                Cmd annotation = method.getAnnotation(Cmd.class);
                if (annotation != null) {
                    // Use either the annotation value or method name as the command
                    String commandName = annotation.value().isEmpty() 
                        ? method.getName() 
                        : annotation.value();
                    COMMAND_MAP.put(commandName, method);
                }
            }

        } catch (ClassNotFoundException e) {
            e.printStackTrace();
            System.exit(1);
        }
    }
    private static void runSingleCommand(String command) throws IllegalAccessException, InvocationTargetException {
        // System.out.println(COMMAND_MAP);
        Method method = COMMAND_MAP.get(command);
        method.invoke(null);  // Assuming all methods are static
                              // System.out.println("----- Completed " + command + " -----\n");
    }

    private static void runAllCommands() throws IllegalAccessException, InvocationTargetException {
        for (Map.Entry<String, Method> entry : COMMAND_MAP.entrySet()) {
            String command = entry.getKey();
            System.out.println(command); // debug
            runSingleCommand(command);
        }
    }
    public static void testMain(String[] args) throws Exception {
        System.out.println(COMMAND_MAP);
        runAllCommands();
        
    }

}
