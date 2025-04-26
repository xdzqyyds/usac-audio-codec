#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>

class RingBuffer {
public:
    RingBuffer(size_t capacity);
    bool write(const uint8_t* data, size_t size);
    size_t read(uint8_t* dest, size_t required);
    void stop();

private:
    std::vector<uint8_t> buffer;
    size_t head = 0;
    size_t tail = 0;
    bool active = true;
    std::mutex mtx;
    std::condition_variable cv;
};