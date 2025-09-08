package org.example;
import com.google.common.collect.TreeTraverser;

public class App {
    public String getGreeting() {
        return "Hello World!";
    }
    public static void main(String[] args) {
        System.out.println(new App().getGreeting());


        Item root = new Item("root");
        
        Item documents = new Item("documents");
        Item downloads = new Item("downloads");
        Item pictures = new Item("pictures");
        root.addChild(documents);
        root.addChild(downloads);
        root.addChild(pictures);

        Leaf p1 = new Leaf("p1");
        pictures.addChild(p1);

        System.out.println(root.toStringAll());
    }
}

