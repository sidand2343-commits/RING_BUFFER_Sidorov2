#include <iostream>
#include <stdexcept>
#include <optional>
#include <utility>
#include <string>

// ============================================================================
// Entity implementation (Ring Buffer)
// ============================================================================

class RingBuffer {
public:
    // Overflow behavior policy
    enum class OverflowPolicy {
        Reject,    // Reject new element (throw exception)
        Overwrite  // Evict oldest element
    };

    // Constructor
    explicit RingBuffer(size_t capacity, OverflowPolicy policy = OverflowPolicy::Reject)
        : capacity_(capacity), size_(0), head_(0), policy_(policy)
    {
        if (capacity_ == 0) {
            throw std::invalid_argument("Capacity must be greater than zero");
        }
        data_ = new int[capacity_];
    }

    // Destructor
    ~RingBuffer() {
        delete[] data_;
    }

    // Copy constructor
    RingBuffer(const RingBuffer& other)
        : capacity_(other.capacity_), size_(other.size_), head_(0), policy_(other.policy_)
    {
        data_ = new int[capacity_];
        // Copy elements in logical order, starting from index 0.
        // This prevents errors when capacity is changed later.
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[(other.head_ + i) % other.capacity_];
        }
    }

    // Assignment operator (Copy-and-Swap idiom)
    RingBuffer& operator=(const RingBuffer& other) {
        if (this != &other) {
            RingBuffer temp(other); // Create a copy
            std::swap(data_, temp.data_);
            std::swap(capacity_, temp.capacity_);
            std::swap(size_, temp.size_);
            std::swap(head_, temp.head_);
            std::swap(policy_, temp.policy_);
        }
        return *this;
    }

    // Add element.
    // Returns std::optional<int>:
    // - if Overwrite mode and buffer was full, returns the EVICTED element.
    // - if element added successfully (or buffer wasn't full), returns std::nullopt.
    // - if Reject mode and buffer is full, throws exception.
    std::optional<int> push(int value) {
        if (size_ == capacity_) {
            if (policy_ == OverflowPolicy::Reject) {
                throw std::overflow_error("Buffer is full (Reject mode)");
            }

            // Overwrite mode: evict oldest element (located at head_ position)
            int overwritten_value = data_[head_];
            data_[head_] = value;
            head_ = (head_ + 1) % capacity_;
            return overwritten_value;
        }

        // Buffer not full, just add to the end
        size_t tail = (head_ + size_) % capacity_;
        data_[tail] = value;
        ++size_;
        return std::nullopt;
    }

    // Extract element
    int pop() {
        if (size_ == 0) {
            throw std::underflow_error("Buffer is empty");
        }
        int value = data_[head_];
        head_ = (head_ + 1) % capacity_;
        --size_;
        return value;
    }

    // View first element (without extraction)
    int front() const {
        if (size_ == 0) throw std::underflow_error("Buffer is empty");
        return data_[head_];
    }

    // View last element
    int back() const {
        if (size_ == 0) throw std::underflow_error("Buffer is empty");
        return data_[(head_ + size_ - 1) % capacity_];
    }

    // Change capacity
    void resize(size_t new_capacity) {
        if (new_capacity < size_) {
            throw std::invalid_argument("New capacity cannot be less than current size");
        }
        if (new_capacity == capacity_) return;

        int* new_data = new int[new_capacity];
        // Move elements to new block starting from position 0 in logical order
        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = data_[(head_ + i) % capacity_];
        }

        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
        head_ = 0; // Read position resets to 0, write will be at index size_
    }

    // Getters and checks
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == capacity_; }

private:
    int* data_;
    size_t capacity_;
    size_t size_;
    size_t head_;             // Index of oldest element (read position)
    OverflowPolicy policy_;
};

// ============================================================================
// Demonstration program
// ============================================================================

// Helper function to print buffer contents without destroying it
void printBuffer(const RingBuffer& buf, const std::string& name) {
    std::cout << name << " [size=" << buf.size() << ", cap=" << buf.capacity() << "]: ";
    RingBuffer temp_copy = buf; // Use a copy to avoid modifying the original
    while (!temp_copy.empty()) {
        std::cout << temp_copy.pop() << " ";
    }
    std::cout << "\n";
}

int main() {
    setlocale(LC_ALL, "");

    try {
        std::cout << "--- 1. Basic operations and wrap-around ---\n";
        RingBuffer buf1(3);
        buf1.push(10); buf1.push(20); buf1.push(30); // Fill buffer
        buf1.pop(); buf1.pop();                      // Free space (only 30 remains)
        buf1.push(40); buf1.push(50);                // Add elements, positions wrap around
        printBuffer(buf1, "buf1 (wrap-around)");     // Expected: 30 40 50


        std::cout << "\n--- 2. Reject mode (refuse on overflow) ---\n";
        RingBuffer buf2(2, RingBuffer::OverflowPolicy::Reject);
        buf2.push(1); buf2.push(2);
        try {
            buf2.push(3); // Should throw exception
            std::cout << "ERROR: Exception was not thrown!\n";
        } catch (const std::overflow_error& e) {
            std::cout << "Success: caught exception -> " << e.what() << "\n";
        }


        std::cout << "\n--- 3. Overwrite mode ---\n";
        RingBuffer buf3(3, RingBuffer::OverflowPolicy::Overwrite);
        buf3.push(1); buf3.push(2); buf3.push(3);
        std::cout << "Adding 4 to full buffer...\n";
        auto overwritten = buf3.push(4);
        if (overwritten.has_value()) {
            std::cout << "Success: evicted element -> " << overwritten.value() << "\n";
        }
        printBuffer(buf3, "buf3 (overwrite)"); // Expected: 2 3 4


        std::cout << "\n--- 4. Copy independence ---\n";
        RingBuffer original(3);
        original.push(100); original.push(200);
        RingBuffer copy = original;

        original.push(300); // Modify original
        original.pop();     // Modify original

        printBuffer(original, "original"); // Expected: 200 300
        printBuffer(copy, "copy");         // Expected: 100 200 (copy unchanged)


        std::cout << "\n--- 5. Capacity change (Resize) ---\n";
        RingBuffer buf4(4);
        buf4.push(1); buf4.push(2); buf4.push(3); buf4.push(4);
        buf4.pop(); buf4.pop();
        buf4.push(5); buf4.push(6); // Data split by block boundary: [3, 4, 5, 6] physically may be stored as [5, 6, 3, 4]

        std::cout << "Before capacity change:\n";
        printBuffer(buf4, "buf4");

        buf4.resize(6); // Increase
        std::cout << "After increasing to 6:\n";
        printBuffer(buf4, "buf4"); // Order should be preserved: 3 4 5 6

        buf4.resize(4); // Decrease (but not below current size=4)
        std::cout << "After decreasing to 4:\n";
        printBuffer(buf4, "buf4");

        try {
            buf4.resize(2); // Try to decrease below size
        } catch (const std::invalid_argument& e) {
            std::cout << "Success: decrease below size rejected -> " << e.what() << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Critical error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}