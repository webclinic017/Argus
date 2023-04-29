#include <memory>
#include <mutex>

template<typename T>
class ThreadSafeSharedPtr
{
public:
    // constructor that takes a shared_ptr
    ThreadSafeSharedPtr(std::shared_ptr<T> ptr) : m_ptr(std::move(ptr)), m_mutex() {}

    // constructor that takes a raw pointer
    ThreadSafeSharedPtr(T* ptr) : ThreadSafeSharedPtr(std::shared_ptr<T>(ptr)) {}

    // make_shared function that forwards arguments to T's constructor
    template<typename... Args>
    static ThreadSafeSharedPtr<T> make_shared(Args&&... args)
    {
        return ThreadSafeSharedPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));
    }

    // copy constructor
    ThreadSafeSharedPtr(const ThreadSafeSharedPtr& other) : m_ptr(other.m_ptr), m_mutex() {}

    // move constructor
    ThreadSafeSharedPtr(ThreadSafeSharedPtr&& other) noexcept : m_ptr(std::move(other.m_ptr)), m_mutex() {}

    // copy assignment operator
    ThreadSafeSharedPtr& operator=(const ThreadSafeSharedPtr& other)
    {
        if (this != &other)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_ptr = other.m_ptr;
        }
        return *this;
    }

    // move assignment operator
    ThreadSafeSharedPtr& operator=(ThreadSafeSharedPtr&& other) noexcept
    {
        if (this != &other)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_ptr = std::move(other.m_ptr);
        }
        return *this;
    }

    // dereference operator
    T& operator*() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return *m_ptr;
    }

    // arrow operator
    T* operator->() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ptr.get();
    }

    // get method to retrieve the shared pointer
    std::shared_ptr<T> get_shared_ptr() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ptr;
    }

private:
    std::shared_ptr<T> m_ptr;
    mutable std::mutex m_mutex;
};