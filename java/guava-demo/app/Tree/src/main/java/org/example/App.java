package org.example;

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

        Leaf l1 = new Leaf("Leaf1");
        pictures.addChild(l1);

        Leaf l2 = new Leaf("Leaf2");
        downloads.addChild(l2, "2025-09-09/news".split("/"));

        System.out.println(root.toStringAll());
    }
}

