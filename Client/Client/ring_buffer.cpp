#include "ring_buffer.h"

RingBuffer::RingBuffer(size_t capacity) : buffer(capacity) {}

bool RingBuffer::write(const uint8_t* data, size_t size) {
    std::unique_lock<std::mutex> lock(mtx);
    size_t available = buffer.size() - (tail - head);

    if (size > available) return false;

    size_t first_part = std::min(size, buffer.size() - (head % buffer.size()));
    memcpy(&buffer[head % buffer.size()], data, first_part);
    if (first_part < size) {
        memcpy(&buffer[0], data + first_part, size - first_part);
    }
    head += size;
    cv.notify_one();
    return true;
}

size_t RingBuffer::read(uint8_t* dest, size_t required) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&]() { return (head - tail) >= required || !active; });

    if (!active) return 0;

    size_t available = head - tail;
    size_t read_size = std::min(available, required);

    size_t first_part = std::min(read_size, buffer.size() - (tail % buffer.size()));
    memcpy(dest, &buffer[tail % buffer.size()], first_part);
    if (first_part < read_size) {
        memcpy(dest + first_part, &buffer[0], read_size - first_part);
    }
    tail += read_size;
    return read_size;
}

void RingBuffer::stop() {
    std::lock_guard<std::mutex> lock(mtx);
    active = false;
    cv.notify_all();
}