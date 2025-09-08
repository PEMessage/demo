package org.example;


public class Leaf extends Item {
    public Leaf(String name) {
        super(name);
    }
    public String toString() {
        return super.toString() + " [*]";
    }


}
