#include <mutex>
#include <queue>


class ThreadSafeQueue {
    public:
        ThreadSafeQueue() = default;

        void push(int value) {
            std::lock_guard<std::mutex> lock(mutex);
            queue_.push(value);
        }

        int pop() {
            std::lock_guard<std::mutex> lock(mutex);
            if (!queue_.empty()) {
                int value = queue_.front();
                queue_.pop();
                return value;
            }
            return -1; // Indicates empty queue
        }

        bool isEmpty() {
            std::lock_guard<std::mutex> lock(mutex);
            return queue_.empty();
        }
    private:
        std::queue<int> queue_;
        std::mutex mutex;
};