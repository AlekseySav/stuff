#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

template<typename T>
struct storage
{
    std::aligned_storage_t<sizeof(T)> data;
    constexpr T* ptr() { return (T*)data.__data; } 
};

// implements both SharedPtr and WeakPtr
struct smart_ptr
{
    /*
    * control_block_base
    *                     -> control_block
    *                                      -> control_block_make [ for makeShared ]
    *                                      -> control_block_ptr
    */
    struct control_block_base
    {
        size_t n_shared, n_weak;

        control_block_base() : n_shared(1), n_weak(1) { }

        virtual void destroy() = 0;
        virtual void dealloc() = 0;
        virtual void* ptr() = 0;
    };

    template<typename Y, typename Alloc, typename Deleter>
    struct control_block : public control_block_base
    {
        using YAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Y>;
        using YTraits = typename std::allocator_traits<Alloc>::template rebind_traits<Y>;

        YAlloc alloc;
        Deleter del;

        control_block(const Alloc& a, const Deleter& d) : control_block_base(), alloc(a), del(d) { }
    };

    template<typename Y, typename Alloc, typename Deleter = std::default_delete<Y>>
    struct control_block_make : public control_block<Y, Alloc, Deleter>
    {
        using super = control_block<Y, Alloc, Deleter>;
        using self = control_block_make<Y, Alloc, Deleter>;

        storage<Y> data;

        template<typename... Args>
        control_block_make(const Alloc& a, Args&&... args) : super(a, Deleter())
        {
            super::YTraits::construct(this->alloc, data.ptr(), std::forward<Args>(args)...);
        }

        virtual void destroy() override
        {
            super::YTraits::destroy(this->alloc, data.ptr());
        }

        virtual void dealloc() override
        {
            using CBAlloc = typename super::YTraits::template rebind_alloc<self>;
            using CBTraits = typename super::YTraits::template rebind_traits<self>;
            CBAlloc copy(this->alloc);
            CBTraits::deallocate(copy, this, 1);
        }

        virtual void* ptr() override
        {
            return data.ptr();
        }
    };

    template<typename Y, typename Alloc, typename Deleter>
    struct control_block_ptr : public control_block<Y, Alloc, Deleter>
    {
        using super = control_block<Y, Alloc, Deleter>;
        using self = control_block_ptr<Y, Alloc, Deleter>;

        Y* data;

        control_block_ptr(const Alloc& a, const Deleter& d, Y* value) : super(a, d), data(value)
        { }

        virtual void destroy() override
        {
            this->del(data);
        }

        virtual void dealloc() override
        {
            using CBAlloc = typename super::YTraits::template rebind_alloc<self>;
            using CBTraits = typename super::YTraits::template rebind_traits<self>;
            CBAlloc copy(this->alloc);
            CBTraits::deallocate(copy, this, 1);
        }

        virtual void* ptr() override
        {
            return data;
        }
    };

    /*
     * smart_ptr
     */

    control_block_base* cb;

    smart_ptr(smart_ptr::control_block_base* cb) : cb(cb) { }
    smart_ptr() : cb(nullptr) { }

    template<typename Y, typename Alloc, typename Deleter>
    void make(Y* ptr, const Alloc& a, const Deleter& d)
    {
        using block = smart_ptr::control_block_ptr<Y, Alloc, Deleter>;
        using traits = typename std::allocator_traits<Alloc>;
        using CBAlloc = typename traits::template rebind_alloc<block>;
        using CBTraits = typename traits::template rebind_traits<block>;

        if (!ptr)
        {
            cb = nullptr;
            return;
        }

        CBAlloc copy(a);
        block* b = CBTraits::allocate(copy, 1);
        try {
            new (b) block(a, d, ptr);
        }
        catch (...) {
            CBTraits::deallocate(copy, b, 1);
            throw std::current_exception();
        }
        cb = b;
    }

    control_block_base* make_weak() const
    {
        if (cb) cb->n_weak++;
        return cb;
    }

    control_block_base* make_shared() const
    {
        if (cb) cb->n_shared++;
        return cb;
    }

    void kill_weak()
    {
        if (!cb) return;
        if (!--cb->n_weak)
            cb->dealloc();
    }
    
    void kill_shared()
    {
        if (!cb) return;
        if (!--cb->n_shared)
        {
            cb->destroy();
            kill_weak();
        }
    }

    void move_weak(smart_ptr&& from)
    {
        kill_weak();
        cb = from.cb;
        from.cb = nullptr;
    }

    void move_shared(smart_ptr&& from)
    {
        kill_shared();
        cb = from.cb;
        from.cb = nullptr;
    }

    template<typename T>
    T* ptr_as() const { return cb ? (T*)cb->ptr() : nullptr; }
    
    template<typename T>
    T& ref_as() const { return *ptr_as<T>(); }

    size_t n_shared() const { return cb ? cb->n_shared : 0; }
    
};

template<typename T>
class SharedPtr
{
    template<typename U>
    friend class WeakPtr;
    template<typename U>
    friend class SharedPtr;
    template<typename T1, typename Alloc, typename... Args>
    friend SharedPtr<T1> allocateShared(const Alloc& a, Args&&... args);

    smart_ptr impl;
    SharedPtr(const smart_ptr& impl) : impl(impl.cb) { }

public:
    SharedPtr() : impl(nullptr) { }

    template<typename Y, typename = typename std::enable_if_t<std::is_convertible_v<Y*, T*>>>
    SharedPtr(Y* ptr)
    {
        impl.make(ptr, std::allocator<Y>(), std::default_delete<Y>());
    }

    template<typename Y, typename Alloc, typename Deleter, typename = typename std::enable_if_t<std::is_convertible_v<Y*, T*>>>
    SharedPtr(Y* ptr, const Deleter& d, const Alloc& a)
    {
        impl.make(ptr, a, d);
    }

    template<typename Deleter>
    SharedPtr(T* ptr, const Deleter& d)
    {
        impl.make(ptr, std::allocator<T>(), d);
    }

    template<typename Deleter, typename Alloc>
    SharedPtr(T* ptr, const Deleter& d, const Alloc& a)
    {
        impl.make(ptr, a, d);
    }

    template<typename Y, typename Deleter, typename = typename std::enable_if_t<std::is_convertible_v<Y*, T*>>>
    SharedPtr(Y* ptr, const Deleter& d)
    {
        impl.make(ptr, std::allocator<Y>(), d);
    }

    SharedPtr(SharedPtr<T>&& shared) : impl(shared.impl.cb)
    {
        shared.impl.cb = nullptr;
    }

    SharedPtr(const SharedPtr<T>& shared) : impl(shared.impl.make_shared())
    { }

    template<typename Y, typename = typename std::enable_if_t<std::is_assignable_v<T, Y>>>
    SharedPtr(const SharedPtr<Y>& shared) : impl(shared.impl.make_shared())
    { }

    void swap(SharedPtr<T>& other)
    {
        std::swap(impl, other.impl);
    }

    SharedPtr<T>& operator=(const SharedPtr<T>& shared)
    {
        impl.move_shared(SharedPtr<T>(shared).impl);
        return *this;
    }

    template<typename Y, typename = typename std::enable_if_t<std::is_assignable_v<T, Y>>>
    SharedPtr<T>& operator=(const SharedPtr<Y>& shared)
    {
        impl.move_shared(SharedPtr<T>(shared).impl);
        return *this;
    }

    SharedPtr<T>& operator=(SharedPtr<T>&& shared)
    {
        impl.move_shared(std::move(shared.impl));
        return *this;
    }

    template<typename Y, typename = typename std::enable_if_t<std::is_assignable_v<T, Y>>>
    SharedPtr<T>& operator=(SharedPtr<Y>&& shared)
    {
        impl.move_shared(std::move(shared.impl));
        return *this;
    }

    void reset(T* ptr = nullptr)
    {
        impl.kill_shared();
        impl.make(ptr, std::allocator<T>(), std::default_delete<T>());
    }

    ~SharedPtr()
    {
        impl.kill_shared();
    }

    constexpr T* get() { return impl.ptr_as<T>(); }
    constexpr const T* get() const { return impl.ptr_as<const T>(); }
    constexpr T* operator->() { return get(); }
    constexpr const T* operator->() const { return get(); }
    constexpr T& operator*() { return impl.ref_as<T>(); }
    constexpr const T& operator*() const { return impl.ref_as<const T>(); }
    constexpr size_t use_count() const { return impl.n_shared(); }
};

template<typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& a, Args&&... args)
{
    using cb = smart_ptr::control_block_make<T, Alloc>;
    using traits = typename std::allocator_traits<Alloc>;
    using CBAlloc = typename traits::template rebind_alloc<cb>;
    using CBTraits = typename traits::template rebind_traits<cb>;

    CBAlloc copy(a);
    cb* block = CBTraits::allocate(copy, 1);
    try {
        new (block) cb(a, std::forward<Args>(args)...);
    }
    catch (...) {
        CBTraits::deallocate(copy, block, 1);
        throw std::current_exception();
    }
    return SharedPtr<T>(smart_ptr(block));
}

template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args)
{
    return allocateShared<T, std::allocator<T>>(std::allocator<T>(), std::forward<Args>(args)...);
}


template<typename T>
class WeakPtr
{
    template<typename U>
    friend class WeakPtr;
    template<typename U>
    friend class SharedPtr;

    smart_ptr impl;
public:
    WeakPtr() : impl() { }

    template<typename U, typename = typename std::enable_if_t<std::is_constructible_v<T, U>>>
    WeakPtr(const SharedPtr<U>& shared) : impl(shared.impl.cb)
    {
        if (impl.cb) impl.cb->n_weak++;
    }

    WeakPtr(const SharedPtr<T>& shared) : impl(shared.impl.cb)
    {
        if (impl.cb) impl.cb->n_weak++;
    }

    template<typename U, typename = typename std::enable_if_t<std::is_assignable_v<T, U>>>
    WeakPtr(const WeakPtr<U>& weak) : impl(weak.impl.cb)
    {
        if (impl.cb) impl.cb->n_weak++;
    }

    WeakPtr(const WeakPtr<T>& weak) : impl(weak.impl.cb)
    {
        if (impl.cb) impl.cb->n_weak++;
    }

    template<typename U, typename = typename std::enable_if_t<std::is_assignable_v<T, U>>>
    WeakPtr(WeakPtr<U>&& weak) : impl(weak.impl.cb)
    {
        weak.impl.cb = nullptr;
    }

    WeakPtr(WeakPtr<T>&& weak) : impl(weak.impl.cb)
    {
        weak.impl.cb = nullptr;
    }

    template<typename U, typename = typename std::enable_if_t<std::is_assignable_v<T, U>>>
    WeakPtr<T>& operator=(const WeakPtr<U>& other)
    {
        impl.move_weak(WeakPtr<T>(other).impl);
        return *this;
    }

    WeakPtr<T>& operator=(const WeakPtr<T>& other)
    {
        impl.move_weak(WeakPtr<T>(other).impl);
        return *this;
    }

    template<typename U, typename = typename std::enable_if_t<std::is_assignable_v<T, U>>>
    WeakPtr<T>& operator=(WeakPtr<U>&& other)
    {
        impl.move_weak(std::move(other.impl));
        return *this;
    }

    WeakPtr<T>& operator=(WeakPtr<T>&& other)
    {
        impl.move_weak(std::move(other.impl));
        return *this;
    }

    ~WeakPtr()
    {
        impl.kill_weak();
    }

    constexpr size_t use_count() const { return impl.n_shared(); }
    constexpr bool expired() const { return !impl.n_shared(); }

    SharedPtr<T> lock() const
    {
        impl.cb->n_shared++;
        return impl;
    }
};
