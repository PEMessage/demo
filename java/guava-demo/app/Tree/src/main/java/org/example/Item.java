package org.example;
import com.google.common.collect.TreeTraverser;
import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterators;
import com.google.common.collect.Maps;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;

import com.google.common.graph.Traverser;
import java.util.Iterator;

class Item {
    public String name;
    public Item parent;
    public Map<String, Item> children = Maps.newHashMap();
    
    public Item(String name) {
        this.name = name;
    }
    
    public void addChild(Item child) {
        Preconditions.checkArgument(!children.containsKey(child.name), 
            "Item with name '%s' already exists", child.name);
        child.parent = this;
        children.put(child.name, child);
    }
    
    public Iterator<Item> parentChainIterator() {
        return Traverser.<Item>forTree(item -> {
            if (item.parent != null) {
                return ImmutableList.of(item.parent);
            }
            return ImmutableList.of();
        }).breadthFirst(this).iterator();
    }
    
    /**
     * Alternative: Get path using parent chain traverser
     */
    public List<Item> getPathUsingTraverser() {
        List<Item> path = new ArrayList<>();
        parentChainIterator().forEachRemaining(path::add);
        return path;
    }

    
}
