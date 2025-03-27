// Why:
//
// Q: 不明白为什么动态语言就不需要factory pattern呢？
// A:
//     因为factory pattern其实就是virtual constructor，
//     就是一种能用一个接口创建多种不同类型对象的方法。
//     对于动态语言，尤其是Python或Ruby这样的duck typing语言，这本来就是基本功能，不需要额外做什么。
// Q: 一个函数通过if else返回基类指针为啥不行
// A:
//     在C++中实现Factory，仅仅保证所有要构造的类型都来自同一个父类是不够的，
//     因为构造函数不能是虚函数，你想实现Factory，只能用类似这样的代码：if(type=="subtype1") return new subtype1(...);
//     else if(type=="subtype2") return new subtype2(...);
//     else if(type=="subtype3") return new subtype3(...);
//     这种做法在保留了Factory的所有缺点的同时完全没有体现出任何Factory pattern的优点，
//     新的类型非但不能在运行时加入，甚至想要在编译时加入都得改代码
// Q: 其他方式
// A: 
//     template<class T, class Base>
//     Base *creator(){
//       return new T;
//     }


#include <iostream>
#include <map>
#include <functional>
#include <string>
#include <memory>

using namespace std;

// 基类接口
class BaseInterface {
public:
    virtual ~BaseInterface() = default;
    virtual void print() const = 0;
};

// 子类型1
class Subtype1 : public BaseInterface {
public:
    explicit Subtype1(int value) : value_(value) {}
    explicit Subtype1() : value_(0) {}
    void print() const override {
        cout << "Subtype1 with value: " << value_ << endl;
    }

    // Why not:
    //  static BaseInterface* newInstance(...) { return new SubtypeX(...); }
    //  不同类的静态成员函数是不同的类型。即使它们有完全相同的签名（返回类型和参数列表），但因为它们属于不同的类，所以类型是不同的。
    //  1. 对于每一个Subtype，这个newInstance都是不同的类型，也就是说Subtype1::newInstance和Subtype2::newInstance的类型是不同的，
    //  2. 而且你还没法把这个newInstance放在BaseInterface里，因为它必须得是一个static函数，也不能继承也不能多态

    // 工厂方法
    static std::function<unique_ptr<BaseInterface>()> getFactoryMethod() {
        return []() {
            return make_unique<Subtype1>(0);
        };
    }

private:
    int value_;
};

// 子类型2
class Subtype2 : public BaseInterface {
public:
    explicit Subtype2(const string& name) : name_(name) {}
    explicit Subtype2() : name_("Default") {}
    void print() const override {
        cout << "Subtype2 with name: " << name_ << endl;
    }

    // 工厂方法
    static std::function<unique_ptr<BaseInterface>()> getFactoryMethod() {
        return []() {
            return make_unique<Subtype2>("Default");
        };
    }

private:
    string name_;
};

// 工厂类
class BaseFactory {
public:
    using SubtypeKey = string;
    using FactoryMethod = std::function<unique_ptr<BaseInterface>()>;
    

    void registerSubtype(const SubtypeKey& key, std::function<unique_ptr<BaseInterface>()> factoryMethod) {
        // 将带参数的工厂方法包装为无参数的工厂方法（简化示例）
        // 实际应用中可能需要更复杂的参数传递机制
        subtypeMap[key] = [factoryMethod]() {
            return factoryMethod(); 
        };
    }

    // 创建实例
    unique_ptr<BaseInterface> newInstance(const SubtypeKey& key) {
        auto it = subtypeMap.find(key);
        if (it != subtypeMap.end()) {
            return it->second();
        }
        return nullptr;
    }
    // Singleton<T> 有如下缺点：
    // 1. 它也没解决多线程重入导致重复构造的问题，当然这一点可以通过在这个template里加入一个pthread_once结局(static 的是一个指针，而不是对象本身)
    // 2. 它不能保证对象除此之外不能被任意new出来，因为你不能把T的构造函数设为private，除非你把这个template作为友元加入到T里
    // 3. 也是最重要的一点，如果有类型Base和子类Sub，Singleton<Base>和Singleton<Sub>之间没有继承关系，确切地说它们没有任何关系，也不存在可替换性
    // template<typename T>
    // struct Singleton {
    // public:
    //     static T& getInstance()
    //     {
    //         if (!instance)
    //             instance = new T();
    //         return *instance;
    //     }
    // private:
    //     static T* instance;
    // };
    // 优点如下：
    //  1. instance是按需构造的，
    //  2、这么做没有多线程的问题，static 是对象而不是指针！，保证了安全性
    //  3、如果你把这句话放在类外面甚至都不需要接触到T的源代码，
    //  4、如果我们返回Base的指针，然后再把这个singleton factory method用function包装起来，我们甚至有了一定的多态性，更接近Java Singleton的效果
    static BaseFactory* getInstance() {
        static BaseFactory instance;
        return &instance;
    }

private:
    map<SubtypeKey, FactoryMethod> subtypeMap;
    BaseFactory() = default;
};

int main() {
    auto factory = BaseFactory::getInstance();

    // 注册子类型的工厂方法, 如果BaseFactory是单例，则可以放在单例中运行这两句
    factory->registerSubtype("type1", Subtype1::getFactoryMethod());
    factory->registerSubtype("type2", Subtype2::getFactoryMethod());

    // 创建实例
    auto obj1 = factory->newInstance("type1");
    auto obj2 = factory->newInstance("type2");

    if (obj1) obj1->print();
    if (obj2) obj2->print();

    return 0;
}
