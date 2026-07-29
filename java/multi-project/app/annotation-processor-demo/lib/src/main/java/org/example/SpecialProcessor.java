package org.example;

import com.squareup.javapoet.*;

import javax.annotation.processing.AbstractProcessor;
import javax.annotation.processing.RoundEnvironment;
import javax.annotation.processing.SupportedAnnotationTypes;
import javax.annotation.processing.SupportedSourceVersion;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.*;
import javax.tools.JavaFileObject;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
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

        // invoke JavaPoet version (active)
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

    // ============================================================
    // Approach 1: JavaPoet (active)
    // ============================================================
    private void generateRegistry(String packageName, List<TypeElement> classes) {
        ClassName registryClass = ClassName.get(packageName, "SpecialRegistry");
        ClassName entryClass = registryClass.nestedClass("Entry");

        ParameterizedTypeName classOfWildcard = ParameterizedTypeName.get(
                ClassName.get(Class.class), WildcardTypeName.subtypeOf(Object.class));

        TypeSpec entryType = TypeSpec.classBuilder("Entry")
                .addModifiers(Modifier.PUBLIC, Modifier.STATIC)
                .addField(FieldSpec.builder(classOfWildcard, "type",
                        Modifier.PUBLIC, Modifier.FINAL).build())
                .addField(FieldSpec.builder(String.class, "name",
                        Modifier.PUBLIC, Modifier.FINAL).build())
                .addMethod(MethodSpec.constructorBuilder()
                        .addParameter(classOfWildcard, "type")
                        .addParameter(String.class, "name")
                        .addStatement("this.type = type")
                        .addStatement("this.name = name")
                        .build())
                .build();

        ParameterizedTypeName listOfEntries = ParameterizedTypeName.get(
                ClassName.get(List.class), entryClass);

        CodeBlock.Builder staticBlock = CodeBlock.builder();
        for (TypeElement cls : classes) {
            Special sp = cls.getAnnotation(Special.class);
            String name = (sp != null) ? sp.name() : "";
            staticBlock.addStatement("ENTRIES.add(new $T($T.class, $S))",
                    entryClass, ClassName.get(cls), name);
        }

        TypeSpec registry = TypeSpec.classBuilder("SpecialRegistry")
                .addModifiers(Modifier.PUBLIC)
                .addType(entryType)
                .addField(FieldSpec.builder(listOfEntries, "ENTRIES")
                        .addModifiers(Modifier.PRIVATE, Modifier.STATIC, Modifier.FINAL)
                        .initializer("new $T<>()", ArrayList.class)
                        .build())
                .addStaticBlock(staticBlock.build())
                .addMethod(MethodSpec.methodBuilder("getAll")
                        .addModifiers(Modifier.PUBLIC, Modifier.STATIC)
                        .returns(listOfEntries)
                        .addStatement("return $T.unmodifiableList(ENTRIES)", Collections.class)
                        .build())
                .build();

        try {
            JavaFile.builder(packageName, registry)
                    .skipJavaLangImports(true)
                    .build()
                    .writeTo(processingEnv.getFiler());
            System.out.println("[SpecialProcessor] generated SpecialRegistry with "
                    + classes.size() + " entries (via JavaPoet)");
        } catch (IOException e) {
            processingEnv.getMessager().printMessage(
                    javax.tools.Diagnostic.Kind.ERROR,
                    "Failed to generate SpecialRegistry: " + e.getMessage());
        }
    }

    // ============================================================
    // Approach 2: raw println (reference only, not in use)
    // ============================================================
    @SuppressWarnings("unused")
    private void generateRegistryManual(String packageName, List<TypeElement> classes) {
        try {
            JavaFileObject file = processingEnv.getFiler()
                    .createSourceFile((packageName.isEmpty() ? "" : packageName + ".")
                            + "SpecialRegistryManual");
            try (PrintWriter out = new PrintWriter(file.openWriter())) {
                if (!packageName.isEmpty()) {
                    out.println("package " + packageName + ";");
                    out.println();
                }
                out.println("import java.util.Collections;");
                out.println("import java.util.List;");
                out.println("import java.util.ArrayList;");
                out.println();
                out.println("public class SpecialRegistryManual {");
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
            System.out.println("[SpecialProcessor] generated SpecialRegistryManual with "
                    + classes.size() + " entries (via println)");
        } catch (IOException e) {
            processingEnv.getMessager().printMessage(
                    javax.tools.Diagnostic.Kind.ERROR,
                    "Failed: " + e.getMessage());
        }
    }
}
