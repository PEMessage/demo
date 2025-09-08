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


        TreeTraverser<Item> traverser = new TreeTraverser<Item>() {
            @Override
            public Iterable<Item> children(Item node) {
                return node.children.values();
            }
        };

        System.out.println("=== Pre-order Traversal ===");
        traverser.preOrderTraversal(root).forEach(node -> 
            System.out.println(node.name)
        );


    }
}

