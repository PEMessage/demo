package org.example;

public class App {
    public static void main(String[] args) {
        System.out.println("=== @Special classes (compile-time registration, zero reflection) ===\n");

        for (SpecialRegistry.Entry entry : SpecialRegistry.getAll()) {
            System.out.println("Found: " + entry.type.getSimpleName()
                    + "  (name=" + entry.name + ")");

            try {
                String desc = (String) entry.type.getMethod("describe").invoke(null);
                System.out.println("  -> " + desc);
            } catch (Exception e) {
                System.out.println("  -> (no describe method)");
            }
            System.out.println();
        }

        System.out.println("=== class != null check (no reflection): ===");
        System.out.println("ServiceA.class == null ? " + (ServiceA.class == null));
        System.out.println("We found all classes through SpecialRegistry (compile-time generated).");
    }
}
