#include <iostream>
#include <memory>

// Existing classes with draw methods
class Circuit {
public:
    void draw() const {
        std::cout << "Circuit\n";
    }
};

class Square {
public:
    void draw() const {
        std::cout << "Square\n";
    }
};

// External polymorphism setup
class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void draw() const = 0;
};

template <typename T>
class DrawableAdapter : public Drawable {
    const T& obj;
public:
    DrawableAdapter(const T& t) : obj(t) {}
    void draw() const override {
        obj.draw();
    }
};

class Shape {
    std::unique_ptr<Drawable> drawable;
public:
    template <typename T>
    Shape(const T& obj) : drawable(std::make_unique<DrawableAdapter<T>>(obj)) {}
    
    void draw() const {
        drawable->draw();
    }
};

int main() {
    Circuit circuit;
    Square square;
    
    Shape shape1(circuit);
    Shape shape2(square);
    
    shape1.draw(); // Output: Circuit
    shape2.draw(); // Output: Square
    
    return 0;
}
