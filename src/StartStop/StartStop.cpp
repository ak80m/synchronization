// StartStop.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <mutex>

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



int main()
{
    std::shared_ptr<Manager> m;
    // A sequence of m->start() & m->stop();
    m->terminate();
    m->start(); // does nothing
    m.reset();
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
