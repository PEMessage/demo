package org.example;

import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.HexFormat;

public class App {

    @Structure.FieldOrder({"name", "age", "salary"})
    public static class Person extends Structure {
        public byte[] name = new byte[32];
        public int age;
        public double salary;

        public Person() {
            super();
        }
    }

    public static byte[] serialize(Person person) {
        person.write();
        return person.getPointer().getByteArray(0, person.size());
    }

    public static Person deserialize(byte[] data) {
        Person person = new Person();
        byte[] buf = new byte[person.size()];
        System.arraycopy(data, 0, buf, 0, Math.min(data.length, buf.length));
        person.getPointer().write(0, buf, 0, buf.length);
        person.read();
        return person;
    }

    public static String getName(Person person) {
        int end = 0;
        while (end < person.name.length && person.name[end] != 0) {
            end++;
        }
        return new String(person.name, 0, end);
    }

    public static void setName(Person person, String s) {
        Arrays.fill(person.name, (byte) 0);
        byte[] bytes = s.getBytes();
        System.arraycopy(bytes, 0, person.name, 0, Math.min(bytes.length, person.name.length - 1));
    }

    public static void main(String[] args) {
        Person p1 = new Person();
        setName(p1, "Alice");
        p1.age = 30;
        p1.salary = 75000.0;

        System.out.println("Original:  name=" + getName(p1) + ", age=" + p1.age + ", salary=" + p1.salary);

        byte[] data = serialize(p1);
        System.out.println("Serialized (" + data.length + " bytes): " + HexFormat.of().formatHex(data));

        Person p2 = deserialize(data);
        System.out.println("Restored:  name=" + getName(p2) + ", age=" + p2.age + ", salary=" + p2.salary);

        boolean ok = getName(p1).equals(getName(p2)) && p1.age == p2.age && p1.salary == p2.salary;
        System.out.println("Roundtrip: " + (ok ? "OK" : "FAIL"));
    }
}
