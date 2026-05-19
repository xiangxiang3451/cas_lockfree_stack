#ifndef LOCK_FREE_STACK_H
#define LOCK_FREE_STACK_H

#include <atomic>
#include "HazardPointer.h"

class LockFreeStack {
private:
    struct Node {
        int data;
        Node* next;

        explicit Node(int value)
            : data(value),
              next(nullptr) {
        }
    };

    struct RetiredNode {
        Node* data;
        RetiredNode* next;

        explicit RetiredNode(Node* node)
            : data(node),
              next(nullptr) {
        }
    };

private:
    std::atomic<Node*> head;
    std::atomic<long long> retryCount;
    std::atomic<RetiredNode*> retiredNodes;

private:
    void addRetiredNode(Node* node) {
        RetiredNode* retired = new RetiredNode(node);

        retired->next = retiredNodes.load(std::memory_order_relaxed);

        while (!retiredNodes.compare_exchange_weak(
            retired->next,
            retired,
            std::memory_order_release,
            std::memory_order_relaxed)) {
        }
    }

    void deleteNodesWithNoHazards() {
        RetiredNode* current =
            retiredNodes.exchange(nullptr, std::memory_order_acquire);

        while (current != nullptr) {
            RetiredNode* next = current->next;

            if (outstandingHazardPointersFor(current->data)) {
                RetiredNode* oldHead =
                    retiredNodes.load(std::memory_order_relaxed);

                do {
                    current->next = oldHead;
                } while (!retiredNodes.compare_exchange_weak(
                    oldHead,
                    current,
                    std::memory_order_release,
                    std::memory_order_relaxed));
            } else {
                delete current->data;
                delete current;
            }

            current = next;
        }
    }

    void deleteAllRetiredNodes() {
        RetiredNode* current =
            retiredNodes.exchange(nullptr, std::memory_order_acquire);

        while (current != nullptr) {
            RetiredNode* next = current->next;
            delete current->data;
            delete current;
            current = next;
        }
    }

public:
    LockFreeStack()
        : head(nullptr),
          retryCount(0),
          retiredNodes(nullptr) {
    }

    LockFreeStack(const LockFreeStack&) = delete;
    LockFreeStack& operator=(const LockFreeStack&) = delete;

    ~LockFreeStack() {
        clear();
        deleteAllRetiredNodes();
    }

    void push(int value) {
        Node* newNode = new Node(value);
        Node* oldHead = head.load(std::memory_order_relaxed);

        while (true) {
            newNode->next = oldHead;

            if (head.compare_exchange_weak(
                    oldHead,
                    newNode,
                    std::memory_order_release,
                    std::memory_order_relaxed)) {
                return;
            }

            retryCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool pop(int& value) {
        std::atomic<void*>& hazardPointer =
            getHazardPointerForCurrentThread();

        Node* oldHead = head.load(std::memory_order_acquire);

        while (true) {
            Node* temp = nullptr;

            do {
                temp = oldHead;
                hazardPointer.store(oldHead, std::memory_order_seq_cst);
                oldHead = head.load(std::memory_order_acquire);
            } while (oldHead != temp);

            if (oldHead == nullptr) {
                hazardPointer.store(nullptr, std::memory_order_release);
                return false;
            }

            Node* next = oldHead->next;

            if (head.compare_exchange_strong(
                    oldHead,
                    next,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                value = oldHead->data;

                hazardPointer.store(nullptr, std::memory_order_release);

                addRetiredNode(oldHead);
                deleteNodesWithNoHazards();

                return true;
            }

            retryCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool isEmpty() const {
        return head.load(std::memory_order_acquire) == nullptr;
    }

    long long getRetryCount() const {
        return retryCount.load(std::memory_order_relaxed);
    }

    void resetRetryCount() {
        retryCount.store(0, std::memory_order_relaxed);
    }

    void clear() {
        int value = 0;

        while (pop(value)) {
        }
    }
};

#endif