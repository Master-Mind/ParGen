#pragma once
class [[ParGen::Serialize()]] SimpleClass {
public:
    int field1;
    double field2;

    void method1() {}
    double method2(int param) { return param * 2.0; }
};


class [[ParGen::Serialize()]] ConstructorClass {
public:
    ConstructorClass(): field1(0), field2(0)
    {
    }
    [[ParGen::Constructor(1,2, {0,1}), ParGen::Reflect()]]
    ConstructorClass(int a, double b) : field1(a), field2(b) {}
    ~ConstructorClass() {}

    int field1;
    double field2;
};

class BaseClass {
public:
	virtual ~BaseClass() = default;
	virtual void method() {}
};

class [[ParGen::Serialize()]] DerivedClass : public BaseClass {
public:
    void method() override {}
};

template <typename T>
class [[ParGen::Serialize()]] TemplateClass {
public:
    T value;

    T getValue() { return value; }
    void setValue(T newValue) { value = newValue; }
};

class [[ParGen::Serialize()]] AccessModifierClass {
private:
    int privateField;

public:
    int publicField;

protected:
    int protectedField;

public:
    AccessModifierClass(int private_field, int public_field, int protected_field);

public:
    void publicMethod() {}
protected:
    void protectedMethod() {}
private:
    void privateMethod() {}
};

class [[ParGen::Reflect()]] CustomContainer {
public:
    int data[10];

    int* begin() { return &data[0]; }
    int* end() { return &data[10]; }
};

class VirtualBase {
public:
	virtual ~VirtualBase() = default;
	virtual void virtualMethod() = 0; // pure virtual method
};


class [[ParGen::Reflect()]] DerivedFromVirtual : public VirtualBase {
public:
    void virtualMethod() override {
        // Implementation of the virtual method
    }
};

class PureVirtualBase {
public:
	virtual ~PureVirtualBase() = default;
	virtual void pureVirtualMethod() = 0; // pure virtual method
};

class [[ParGen::Reflect()]] DerivedFromPureVirtual : public PureVirtualBase {
public:
    void pureVirtualMethod() override {
        // Implementation of the pure virtual method
    }
};

namespace duplicatesInANamespace
{
    class [[ParGen::Reflect()]] CustomContainer {
    public:
        int data[10];

        int* begin() { return &data[0]; }
        int* end() { return &data[10]; }
    };

    class VirtualBase {
    public:
        virtual ~VirtualBase() = default;
        virtual void virtualMethod() = 0; // pure virtual method
    };


    class [[ParGen::Reflect()]] DerivedFromVirtual : public VirtualBase {
    public:
        void virtualMethod() override {
            // Implementation of the virtual method
        }
    };
	namespace nested
	{
        class PureVirtualBase {
        public:
            virtual ~PureVirtualBase() = default;
            virtual void pureVirtualMethod() = 0; // pure virtual method
        };
        
        class [[ParGen::Reflect()]] DerivedFromPureVirtual : public PureVirtualBase {
        public:
            void pureVirtualMethod() override {
                // Implementation of the pure virtual method
            }
        };
	}
}
