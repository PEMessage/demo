import java.lang.annotation.*; // for @interface

// for AnnotationCheckerDemo
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;

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

    @Cmd("execute")
    public void commandMethod() {}

    public void regularMethod() {}
}


public class AnnotationCheckerDemo {
    public static void main(String[] args) throws Exception {
        // Check for different annotations in SampleClass
        boolean hasTest = classContainAnnotation("SampleClass", Test.class);
        boolean hasCmd = classContainAnnotation("SampleClass", Cmd.class);

        System.out.println("Contains @Test methods: " + hasTest);
        System.out.println("Contains @Cmd methods: " + hasCmd);
    }

    private static boolean classContainAnnotation(String className, Class<? extends Annotation> annotationClass)
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
}
