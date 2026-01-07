/**
 * @file safe_queue.h
 * @brief [新增文件] 线程安全的任务队列 (生产者-消费者模型核心组件)
 * @details 使用 mutex 和 condition_variable 实现阻塞式出队
 */
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class SafeQueue {
public:
    SafeQueue() = default;
    ~SafeQueue() = default;

    // [生产者调用]：将数据放入队列
    void Push(T value) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::move(value));
        cond_.notify_one(); // 唤醒一个正在等待的消费者
    }

    // [消费者调用]：从队列取出数据
    // 返回值：true 表示成功取出，false 表示队列已停止且为空
    bool Pop(T& value) {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待直到队列有数据，或者收到停止信号
        cond_.wait(lock, [this] { return !queue_.empty() || stop_; });

        if (queue_.empty() && stop_) {
            return false;
        }

        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // 停止队列（通常在程序退出时调用）
    void Stop() {
        std::lock_guard<std::mutex> lock(mtx_);
        stop_ = true;
        cond_.notify_all(); // 唤醒所有等待的线程
    }

    bool Empty() {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

    size_t Size() {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable cond_;
    bool stop_ = false; // 停止标志
};