#ifndef HAZARD_POINTER_H
#define HAZARD_POINTER_H

#include <atomic>
#include <thread>
#include <stdexcept>

constexpr unsigned MAX_HAZARD_POINTERS = 128;

struct HazardPointer {
    std::atomic<std::thread::id> id;
    std::atomic<void*> pointer;

    HazardPointer()
        : id(std::thread::id()),
          pointer(nullptr) {
    }
};

inline HazardPointer hazardPointers[MAX_HAZARD_POINTERS];

class HazardPointerOwner {
private:
    HazardPointer* hp;

public:
    HazardPointerOwner(const HazardPointerOwner&) = delete;
    HazardPointerOwner& operator=(const HazardPointerOwner&) = delete;

    HazardPointerOwner()
        : hp(nullptr) {
        for (unsigned i = 0; i < MAX_HAZARD_POINTERS; ++i) {
            std::thread::id emptyId;

            if (hazardPointers[i].id.compare_exchange_strong(
                    emptyId,
                    std::this_thread::get_id())) {
                hp = &hazardPointers[i];
                break;
            }
        }

        if (hp == nullptr) {
            throw std::runtime_error("No hazard pointer available");
        }
    }

    std::atomic<void*>& getPointer() {
        return hp->pointer;
    }

    ~HazardPointerOwner() {
        hp->pointer.store(nullptr, std::memory_order_release);
        hp->id.store(std::thread::id(), std::memory_order_release);
    }
};

inline std::atomic<void*>& getHazardPointerForCurrentThread() {
    thread_local static HazardPointerOwner hazard;
    return hazard.getPointer();
}

inline bool outstandingHazardPointersFor(void* p) {
    for (unsigned i = 0; i < MAX_HAZARD_POINTERS; ++i) {
        if (hazardPointers[i].pointer.load(std::memory_order_acquire) == p) {
            return true;
        }
    }

    return false;
}

#endif