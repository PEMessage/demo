#include <iostream>

// QuickRun able
//
// QwQ-32B
// Part 1
    // Please write a c++ demo
    // 1. have 3 class IInterface , Base1 , Base2 , Final
    // 2. inherit chian like this Final -> Base2 -> Base1 -> Interface
    // 3. IInterface  define a pure virtual fucntion f1
    // 4. all class overrider f1, and the body is {cout <<  "This is [Classname HERE]" }
    // 5. in main function, create each class object , and call f1
    // 6. create pointer to each class, call f1,
    // 7. reset pointer to subclass , call f1
// Part 2
    // Reset pointers to subclasses and call f1 this step example not enought
    //
    // IInterface could reset to Base1 Base2 Final
    // Base1  could reset to Base2 Final and  itself
    // all call should be
    //
    // 3 + 3 + 2 +1

class PreInterface {
public:
    void f1() {
        std::cout << "This is PreInterface" << std::endl;
    }
};
// 1. Interface class with pure virtual function
class IInterface : PreInterface {
public:
    virtual void f1() = 0;
    virtual ~IInterface() = default; // Virtual destructor for proper cleanup
};

// 2. Base1 inherits from IInterface
class Base1 : public IInterface {
public:
    void f1() override {
        std::cout << "This is Base1" << std::endl;
    }
};

// 3. Base2 inherits from Base1
class Base2 : public Base1 {
public:
    void f1() override {
        std::cout << "This is Base2" << std::endl;
    }
};

// 4. Final inherits from Base2
class Final : public Base2 {
public:
    void f1() override {
        std::cout << "This is Final" << std::endl;
    }
};

int main() {
    // 5. Create objects of each class
    Base1 obj1;
    Base2 obj2;
    Final obj3;

    // Direct calls
    std::cout << "Direct calls:" << std::endl;
    obj1.f1();
    obj2.f1();
    obj3.f1();

    // 6. Create pointers to each class and call f1
    std::cout << "\nPointer calls:" << std::endl;
    IInterface* iPtr = &obj1;
    Base1* base1Ptr = &obj2;
    Base2* base2Ptr = &obj3;

    iPtr->f1();     // Calls Base1::f1()
    base1Ptr->f1(); // Calls Base2::f1()
    base2Ptr->f1(); // Calls Final::f1()

    // 7. Reset pointers to subclasses and call f1
    std::cout << "\nPolymorphic calls:" << std::endl;
    Base1* dyn1 = new Base1();
    Base2* dyn2 = new Base2();
    Final* dyn3 = new Final();

    IInterface* d_iPtr1 = dyn1;
    IInterface* d_iPtr2 = dyn2;
    IInterface* d_iPtr3 = dyn3;

    Base1* d_base1Ptr1 = dyn1;
    Base1* d_base1Ptr2 = dyn2;
    Base1* d_base1Ptr3 = dyn3;

    Base2* d_base2Ptr1 = dyn2;
    Base2* d_base2Ptr2 = dyn3;

    Final* d_finalPtr = dyn3;

    std::cout << "\nFor d_iPtr:" << std::endl;
    d_iPtr1->f1();     // Base1::f1()
    d_iPtr2->f1();     // Base2::f1()
    d_iPtr3->f1();     // Final::f1()

    std::cout << "\nFor d_base1Ptr1:" << std::endl;
    d_base1Ptr1->f1(); // Base1::f1()
    d_base1Ptr2->f1(); // Base2::f1()
    d_base1Ptr3->f1(); // Final::f1()

    std::cout << "\nFor d_base2Ptr1:" << std::endl;
    d_base2Ptr1->f1(); // Base2::f1()
    d_base2Ptr2->f1(); // Final::f1()

    std::cout << "\nFor d_finalPtr1:" << std::endl;
    d_finalPtr->f1();  // Final::f1()

    delete dyn1;
    delete dyn2;
    delete dyn3;


    return 0;
}
