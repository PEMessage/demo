package org.example;

import javax.annotation.processing.AbstractProcessor;
import javax.annotation.processing.RoundEnvironment;
import javax.annotation.processing.SupportedAnnotationTypes;
import javax.annotation.processing.SupportedSourceVersion;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.Element;
import javax.lang.model.element.PackageElement;
import javax.lang.model.element.TypeElement;
import javax.tools.JavaFileObject;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

@SupportedAnnotationTypes("org.example.Special")
@SupportedSourceVersion(SourceVersion.RELEASE_17)
public class SpecialProcessor extends AbstractProcessor {

    @Override
    public boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv) {
        List<TypeElement> specialClasses = new ArrayList<>();
        for (TypeElement annotation : annotations) {
            for (Element element : roundEnv.getElementsAnnotatedWith(annotation)) {
                TypeElement typeElement = (TypeElement) element;
                specialClasses.add(typeElement);
                Special sp = typeElement.getAnnotation(Special.class);
                System.out.println("[SpecialProcessor] found @Special on: "
                        + typeElement.getQualifiedName()
                        + " (name=" + (sp != null ? sp.name() : "N/A") + ")");
            }
        }

        if (specialClasses.isEmpty()) {
            return true;
        }

        String packageName = getPackageName(specialClasses.get(0));
        generateRegistry(packageName, specialClasses);
        return true;
    }

    private String getPackageName(TypeElement typeElement) {
        Element enclosing = typeElement.getEnclosingElement();
        if (enclosing instanceof PackageElement) {
            return ((PackageElement) enclosing).getQualifiedName().toString();
        }
        return "";
    }

    private void generateRegistry(String packageName, List<TypeElement> classes) {
        try {
            JavaFileObject file = processingEnv.getFiler()
                    .createSourceFile((packageName.isEmpty() ? "" : packageName + ".") + "SpecialRegistry");
            try (PrintWriter out = new PrintWriter(file.openWriter())) {
                if (!packageName.isEmpty()) {
                    out.println("package " + packageName + ";");
                    out.println();
                }
                out.println("import java.util.Collections;");
                out.println("import java.util.List;");
                out.println("import java.util.ArrayList;");
                out.println();
                out.println("public class SpecialRegistry {");
                out.println();
                out.println("    public static class Entry {");
                out.println("        public final Class<?> type;");
                out.println("        public final String name;");
                out.println();
                out.println("        Entry(Class<?> type, String name) {");
                out.println("            this.type = type;");
                out.println("            this.name = name;");
                out.println("        }");
                out.println("    }");
                out.println();
                out.println("    private static final List<Entry> ENTRIES = new ArrayList<>();");
                out.println();
                out.println("    static {");
                for (TypeElement cls : classes) {
                    Special sp = cls.getAnnotation(Special.class);
                    String name = (sp != null) ? sp.name() : "";
                    out.println("        ENTRIES.add(new Entry(" + cls.getSimpleName()
                            + ".class, \"" + name + "\"));");
                }
                out.println("    }");
                out.println();
                out.println("    public static List<Entry> getAll() {");
                out.println("        return Collections.unmodifiableList(ENTRIES);");
                out.println("    }");
                out.println("}");
            }
            System.out.println("[SpecialProcessor] generated SpecialRegistry with "
                    + classes.size() + " entries");
        } catch (IOException e) {
            processingEnv.getMessager().printMessage(
                    javax.tools.Diagnostic.Kind.ERROR,
                    "Failed to generate SpecialRegistry: " + e.getMessage());
        }
    }
}
