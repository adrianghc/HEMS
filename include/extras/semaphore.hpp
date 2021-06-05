/*
 * This is a semaphore implementation, lifted out of https://stackoverflow.com/a/27852868/12352701.
 * Author: Stack Overflow user David (https://stackoverflow.com/users/369872/david) 
 */

#ifndef EXTRAS_SEMAPHORE_HPP
#define EXTRAS_SEMAPHORE_HPP

#include <condition_variable>
#include <mutex>
#include <iostream>

template <typename Mutex, typename CondVar>
class basic_semaphore {
    public:
        using native_handle_type = typename CondVar::native_handle_type;

        explicit basic_semaphore(size_t count = 0);
        basic_semaphore(const basic_semaphore&) = delete;
        basic_semaphore(basic_semaphore&&) = delete;
        basic_semaphore& operator=(const basic_semaphore&) = delete;
        basic_semaphore& operator=(basic_semaphore&&) = delete;

        void notify();
        void wait();
        bool try_wait();
        template<class Rep, class Period>
        bool wait_for(const std::chrono::duration<Rep, Period>& d);
        template<class Clock, class Duration>
        bool wait_until(const std::chrono::time_point<Clock, Duration>& t);

        native_handle_type native_handle();

    private:
        Mutex   mMutex;
        CondVar mCv;
        size_t  mCount;
};

using semaphore = basic_semaphore<std::mutex, std::condition_variable>;

template <typename Mutex, typename CondVar>
basic_semaphore<Mutex, CondVar>::basic_semaphore(size_t count) : mCount{count} {}

template <typename Mutex, typename CondVar>
void basic_semaphore<Mutex, CondVar>::notify() {
    std::lock_guard<Mutex> lock{mMutex};
    ++mCount;
    mCv.notify_one();
}

template <typename Mutex, typename CondVar>
void basic_semaphore<Mutex, CondVar>::wait() {
    std::unique_lock<Mutex> lock{mMutex};
    mCv.wait(lock, [&]{ return mCount > 0; });
    --mCount;
}

template <typename Mutex, typename CondVar>
bool basic_semaphore<Mutex, CondVar>::try_wait() {
    std::lock_guard<Mutex> lock{mMutex};

    if (mCount > 0) {
        --mCount;
        return true;
    }

    return false;
}

template <typename Mutex, typename CondVar>
template <typename Rep, typename Period>
bool basic_semaphore<Mutex, CondVar>::wait_for(const std::chrono::duration<Rep, Period>& d) {
    std::unique_lock<Mutex> lock{mMutex};
    auto finished = mCv.wait_for(lock, d, [&]{ return mCount > 0; });

    if (finished) {
        --mCount;
    }

    return finished;
}

template <typename Mutex, typename CondVar>
template <typename Clock, typename Duration>
bool basic_semaphore<Mutex, CondVar>::wait_until(const std::chrono::time_point<Clock, Duration>& t) {
    std::unique_lock<Mutex> lock{mMutex};
    auto finished = mCv.wait_until(lock, t, [&]{ return mCount > 0; });

    if (finished) {
        --mCount;
    }

    return finished;
}

template <typename Mutex, typename CondVar>
typename basic_semaphore<Mutex, CondVar>::native_handle_type basic_semaphore<Mutex, CondVar>::native_handle() {
    return mCv.native_handle();
}

#endif /* EXTRAS_SEMAPHORE_HPP */
