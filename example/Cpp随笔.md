1. 关于C++11中的std::thread 和 std::this_thread 和 std::ref 和 std::bind
    使用std::thread可以直接创建一个线程,底层依旧是pthread_create,只不过封装的更好,更方便coder编写代码而已
    std::this_thread指的是当前线程
    std::ref(T value)用于创建一个包装器对象,该对象持有对一个对象的引用.std::ref的主要作用是避免引用折叠(reference collapsing).在某些情况下,直接传递引用可能会导致引用的引用,这在C++中是不允许的.使用std::ref来显式的传递引用.
    std::ref 和 std::bind一起使用的情况
    std::bind(func, std::ref(x));

2. 关于C++中的enable_shared_from_this<typename T>
    继承了这个类是为了让一个类能够安全地创建指向自身的std::shared_ptr.当有一个std::shared_ptr指向该类的实例的时候,才可以通过shared_from_this()来获得一个新的std::shared_ptr,否则会导致bad_weak_ptr.