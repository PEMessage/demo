package org.example;

import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableList;
import com.google.common.graph.Traverser;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.stream.Collectors;

class Item {
    public String name;
    public Item parent = null;
    public Map<String, Item> children = new HashMap<String, Item>();

    // Optional field
    public int depth = 0;
    
    public Item(String name) {
        this.name = name;
    }
    
    public void addChild(Item child) {
        Preconditions.checkArgument(!children.containsKey(child.name), 
            "Item with name '%s' already exists", child.name);
        child.parent = this;

        Traverser.<Item>forTree(item -> item.children.values())
            .breadthFirst(child)
            .forEach(item -> {
                item.depth += this.depth + 1;
            });
        children.put(child.name, child);
    }

    public void addChild(Item child, String[] rpath) {
        Item current = this;

        for (String segment : rpath) {
            if (!current.children.containsKey(segment)) {
                Item newItem = new Item(segment);
                current.addChild(newItem);
            }
            current = current.children.get(segment);
        }
        current.addChild(child);
    }
    
    public Iterator<Item> parentChainIterator() {
        return Traverser.<Item>forTree(item -> {
            if (item.parent != null) {
                return ImmutableList.of(item.parent);
            }
            return ImmutableList.of();
        }).breadthFirst(this).iterator();
    }
    
    public List<Item> getPath() {
        List<Item> path = new ArrayList<>();
        parentChainIterator().forEachRemaining(v -> path.add(0,v));
        return path;
    }

    public List<String> getPathString() {
        return getPath().stream()
                   .map(x -> x.name)
                   .collect(Collectors.toList());

    }

    public String toStringAll() {
        return toStringAll(0);
    }

    public String toStringAll(int indent) {
        StringBuilder sb = new StringBuilder();

        // Use Traverser for breadth-first traversal
        Traverser.<Item>forTree(item -> item.children.values())
            .breadthFirst(this)
            .forEach(item -> {
                // Calculate indentation based on depth
                int depth = item.depth - this.depth;
                int currentIndent = indent + (depth * 2);

                // Add indentation
                for (int i = 0; i < currentIndent; i++) {
                    sb.append(" ");
                }

                // Add item representation
                sb.append(item.toString());
                sb.append("\n");
            });

        return sb.toString();
    }

    public String toString() {
        return "Item{name='" + name + "'}: " + String.join(".", getPathString()) + ": " + this.depth;
    }
}
