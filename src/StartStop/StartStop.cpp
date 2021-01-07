// StartStop.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <mutex>
#include <vector>
#include <algorithm>

class Manager {
public:
    Manager() : mStarted(false), mTerminated(false) {}
    void start() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (mTerminated) {
            return;
        }
        if (mStarted) {
            return;
        }
        // start a thread
        mStarted = true;
    }
    void stop() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (!mStarted) {
            return;
        }
        // stop a thread
        mStarted = false;
    }
    void terminate() {
        std::lock_guard<std::mutex> lk(mMutex);
        stop();
        mTerminated = true;
    }
private:
    std::mutex mMutex;
    bool mStarted;
    bool mTerminated;
};

class SharedObject {
public:
    SharedObject() {
        std::lock_guard<std::mutex> lk(mMutex);
        mStarted = false;
    }
    ~SharedObject() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (mStarted) {
            // do something
        }
    }
    void method() {}
private:
    std::mutex mMutex;
    bool mStarted;
};

class ClassA {
public:
    std::mutex mMutex;
    std::shared_ptr<SharedObject> mSharedObject;
    void init() {
        std::lock_guard<std::mutex> lk(mMutex);
        mSharedObject = std::shared_ptr<SharedObject>(new SharedObject());
    }
    void deinit() {
        std::lock_guard<std::mutex> lk(mMutex);
        mSharedObject.reset();
    }
    void method1() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (mSharedObject) { mSharedObject->method(); }
    }
    void method2() {
        std::shared_ptr<SharedObject> sharedObject;
        { // scope for lock
            std::lock_guard<std::mutex> lk(mMutex);
            sharedObject = mSharedObject;
        }
        if (sharedObject) { sharedObject->method(); }
    }
};

class ClassB {
public:

    std::shared_ptr<SharedObject> mSharedObject;
    void method1() {
        mSharedObject = std::shared_ptr<SharedObject>(new SharedObject());
    }
    void method2() {
        mSharedObject.reset();
    }
    void method3() {
        if (mSharedObject) { mSharedObject->method(); }
    }
    void method4() {
        std::shared_ptr<SharedObject> sharedObject = mSharedObject;
        if (sharedObject) { sharedObject->method(); }
    }
};

bool val_comparator(std::shared_ptr< SharedObject> i, std::shared_ptr< SharedObject> j) {
    return (i < j); 
}

bool ref_comparator(std::shared_ptr< SharedObject>& i, std::shared_ptr< SharedObject>& j) {
    return (i < j);
}


int main()
{
    std::shared_ptr<Manager> m;
    // A sequence of m->start() & m->stop();
    m->terminate();
    m->start(); // does nothing
    m.reset();

    std::shared_ptr<SharedObject> e1;
    std::shared_ptr<SharedObject> e2 = e1;

    std::vector<std::shared_ptr< SharedObject>> vector;
    std::sort(vector.begin(), vector.end(), val_comparator);
    std::sort(vector.begin(), vector.end(), ref_comparator);


    // e1 and e2 and used and released in separate threads
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
