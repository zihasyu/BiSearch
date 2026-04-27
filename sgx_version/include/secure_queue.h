#ifndef SECURE_QUEUE_H
#define SECURE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <cstdint>
#include "sgx_common.h"

/*
 * EncryptedChunkPacket: the unit of data flowing from the Client thread
 * to the Server thread.  It simulates what would be transmitted over
 * TLS on a real network.
 */
struct EncryptedChunkPacket {
    ChunkMeta_t meta;                       /* plaintext metadata        */
    uint8_t     mac[SGX_AES_GCM_MAC_SIZE];  /* AES-GCM authentication   */
    uint8_t     iv[SGX_AES_GCM_IV_SIZE];    /* AES-GCM nonce            */
    size_t      encrypted_len;              /* ciphertext length         */
    std::vector<uint8_t> encrypted_payload; /* ciphertext                */
};

/*
 * ThreadSafeQueue: bounded, blocking MPSC queue that mimics the
 * network channel between Client and Server in a single-process setup.
 */
template <typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t max_size = 4096)
        : max_size_(max_size), done_(false) {}

    /* Push an item; blocks if the queue is full. */
    void Push(T item) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_full_.wait(lock, [this] { return queue_.size() < max_size_ || done_; });
        if (done_) return;
        queue_.push(std::move(item));
        cv_empty_.notify_one();
    }

    /* Pop an item; blocks if the queue is empty.
       Returns false when the queue is exhausted AND marked done. */
    bool Pop(T& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_empty_.wait(lock, [this] { return !queue_.empty() || done_; });
        if (queue_.empty() && done_) return false;
        item = std::move(queue_.front());
        queue_.pop();
        cv_full_.notify_one();
        return true;
    }

    /* Signal that no more items will be pushed. */
    void SetDone() {
        std::lock_guard<std::mutex> lock(mtx_);
        done_ = true;
        cv_empty_.notify_all();
        cv_full_.notify_all();
    }

    bool IsDone() const { return done_; }

private:
    std::queue<T>           queue_;
    std::mutex              mtx_;
    std::condition_variable cv_empty_;
    std::condition_variable cv_full_;
    size_t                  max_size_;
    std::atomic<bool>       done_;
};

#endif /* SECURE_QUEUE_H */
