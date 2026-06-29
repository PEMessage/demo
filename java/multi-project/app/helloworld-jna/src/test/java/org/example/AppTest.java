package org.example;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class AppTest {

    @Test
    void testSerializeDeserialize() {
        App.Person p1 = new App.Person();
        App.setName(p1, "Bob");
        p1.age = 42;
        p1.salary = 50000.0;

        byte[] data = App.serialize(p1);
        App.Person p2 = App.deserialize(data);

        assertEquals("Bob", App.getName(p2));
        assertEquals(42, p2.age);
        assertEquals(50000.0, p2.salary);
    }

    @Test
    void testEmptyName() {
        App.Person p1 = new App.Person();
        App.setName(p1, "");
        p1.age = 0;
        p1.salary = 0.0;

        byte[] data = App.serialize(p1);
        App.Person p2 = App.deserialize(data);

        assertEquals("", App.getName(p2));
        assertEquals(0, p2.age);
        assertEquals(0.0, p2.salary);
    }

    @Test
    void testBoundaryValues() {
        App.Person p1 = new App.Person();
        App.setName(p1, "Z");
        p1.age = Integer.MAX_VALUE;
        p1.salary = Double.MAX_VALUE;

        byte[] data = App.serialize(p1);
        App.Person p2 = App.deserialize(data);

        assertEquals("Z", App.getName(p2));
        assertEquals(Integer.MAX_VALUE, p2.age);
        assertEquals(Double.MAX_VALUE, p2.salary);
    }

    @Test
    void testNegativeValues() {
        App.Person p1 = new App.Person();
        App.setName(p1, "Negative");
        p1.age = Integer.MIN_VALUE;
        p1.salary = Double.MIN_VALUE;

        byte[] data = App.serialize(p1);
        App.Person p2 = App.deserialize(data);

        assertEquals("Negative", App.getName(p2));
        assertEquals(Integer.MIN_VALUE, p2.age);
        assertEquals(Double.MIN_VALUE, p2.salary);
    }

    @Test
    void testMaxLengthName() {
        App.Person p1 = new App.Person();
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 31; i++) {
            sb.append('A');
        }
        String longName = sb.toString();
        App.setName(p1, longName);
        p1.age = 10;
        p1.salary = 100.0;

        byte[] data = App.serialize(p1);
        App.Person p2 = App.deserialize(data);

        assertEquals(longName, App.getName(p2));
        assertEquals(10, p2.age);
        assertEquals(100.0, p2.salary);
    }

    @Test
    void testDifferentSizeInput() {
        // data shorter than struct: padded with zeros
        byte[] shortData = new byte[]{1, 2, 3, 4, 5};

        App.Person p = App.deserialize(shortData);
        assertNotNull(p);
    }

    @Test
    void testByteArrayLength() {
        App.Person p = new App.Person();
        byte[] data = App.serialize(p);
        assertEquals(p.size(), data.length);
    }
}
