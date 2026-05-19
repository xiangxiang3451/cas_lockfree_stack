#ifndef MUTEX_STACK_H
#define MUTEX_STACK_H

#include <mutex>
#include <stack>

class MutexStack {
private:
    std::stack<int> data;
    mutable std::mutex mtx;

public:
    MutexStack() = default;

    MutexStack(const MutexStack&) = delete;
    MutexStack& operator=(const MutexStack&) = delete;

    void push(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        data.push(value);
    }

    bool pop(int& value) {
        std::lock_guard<std::mutex> lock(mtx);

        if (data.empty()) {
            return false;
        }

        value = data.top();
        data.pop();

        return true;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.empty();
    }

    long long getRetryCount() const {
        return 0;
    }

    void resetRetryCount() {
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx);

        while (!data.empty()) {
            data.pop();
        }
    }
};

#endif