#pragma once

#include <iostream>
#include <vector>

/*
 * iteratable needs diff_t sub(T**, T**) const, T** add(T**, diff_t) const, bool valid(T**) const
 */
template<typename T, typename Diff, typename Iteratable, bool Const>
class Iter
{
public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef std::conditional_t<Const, const T, T> value_t;
    typedef std::conditional_t<Const, const Iteratable, Iteratable> iteratable_t;
    typedef Iter<T, Diff, Iteratable, Const> iter_t;
    typedef Diff diff_t;
public:
    typedef value_t value_type;
    typedef diff_t difference_type;
    typedef value_t* pointer;
    typedef value_t& reference;
    typedef std::reverse_iterator<iter_t> reverse_t;
public:
    Iter(iteratable_t& iteratable, value_t** ptr);

    diff_t operator-(const iter_t& it) const;

    value_t& operator*();
    value_t* operator->();

    iter_t& operator++();
    iter_t& operator--();
    iter_t operator++(int);
    iter_t operator--(int);

    iter_t& operator+=(diff_t n);
    iter_t& operator-=(diff_t n);
    iter_t operator+(diff_t n) const;
    iter_t operator-(diff_t n) const;

    bool operator==(const iter_t& it) const;
    bool operator!=(const iter_t& it) const;
    bool operator<(const iter_t& it) const;
    bool operator<=(const iter_t& it) const;
    bool operator>(const iter_t& it) const;
    bool operator>=(const iter_t& it) const;
private:
    iteratable_t& iteratable;
    value_t** ptr;
};

template<typename T>
class Deque
{
public:
    typedef Iter<T, int, Deque<T>, false> iterator;
    typedef Iter<T, int, Deque<T>, true> const_iterator;
    typedef typename iterator::reverse_t r_iterator;
    typedef typename const_iterator::reverse_t const_r_iterator;
public:
    Deque();
    Deque(const Deque& dq);
    Deque(int count, const T& source = T());
    ~Deque();

    Deque<T>& operator=(const Deque<T>& dq);

    void push_back(const T& object);
    void push_front(const T& object);
    void pop_back();
    void pop_front();

    void insert(iterator it, const T& object);
    void erase(iterator it);

    size_t size() const;

    T& operator[](size_t i);
    const T& operator[](size_t i) const;
    
    T& at(size_t i);
    const T& at(size_t i) const;
    
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;
    r_iterator rbegin();
    r_iterator rend();
    const_r_iterator rbegin() const;
    const_r_iterator rend() const;
    const_r_iterator crbegin() const;
    const_r_iterator crend() const;

    int sub(const T** a, const T** b) const;
    const T** add(const T** a, int delta) const;
    T** add(T** a, int delta) const;
    bool valid(const T** a) const;
private:
    int& inc(int& i);
    int& dec(int& i);

    void create_object(T*& at, const T& object);
    void remove_object(T*& at);
    void realloc(int new_capacity);
private:
    T** data;
    int head, tail, capacity, count;

int pushes = 0, pops = 0;

};


// iterator implementation
#if 1
template<typename T, typename Diff, typename Iteratable, bool Const>
Iter<T, Diff, Iteratable, Const>::Iter(iteratable_t& iteratable, value_t** ptr) : iteratable(iteratable), ptr(ptr)
{ }

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::diff_t Iter<T, Diff, Iteratable, Const>::operator-(const iter_t& it) const
{
    return iteratable.sub((const T**)ptr, (const T**)it.ptr);
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::value_t& Iter<T, Diff, Iteratable, Const>::operator*()
{
    return **ptr;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::value_t* Iter<T, Diff, Iteratable, Const>::operator->()
{
    return *ptr;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::iter_t& Iter<T, Diff, Iteratable, Const>::operator++()
{
    return *this += 1;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::iter_t& Iter<T, Diff, Iteratable, Const>::operator--()
{
    return *this -= 1;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::iter_t Iter<T, Diff, Iteratable, Const>::operator++(int)
{
    iter_t it = *this;
    ++*this;
    return it;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::iter_t Iter<T, Diff, Iteratable, Const>::operator--(int)
{
    iter_t it = *this;
    --*this;
    return it;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::iter_t& Iter<T, Diff, Iteratable, Const>::operator+=(diff_t n)
{
    ptr = iteratable.add(ptr, n);
    return *this;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::iter_t& Iter<T, Diff, Iteratable, Const>::operator-=(diff_t n)
{
    return *this += -n;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::iter_t Iter<T, Diff, Iteratable, Const>::operator+(diff_t n) const
{
    iter_t it = *this;
    return it += n;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
typename Iter<T, Diff, Iteratable, Const>::iter_t Iter<T, Diff, Iteratable, Const>::operator-(diff_t n) const
{
    iter_t it = *this;
    return it -= n;
}


template<typename T, typename Diff, typename Iteratable, bool Const>
bool Iter<T, Diff, Iteratable, Const>::operator==(const iter_t& it) const
{
    if(!iteratable.valid((const T**)ptr) && !iteratable.valid((const T**)it.ptr))
        return true;
    return ptr == it.ptr;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
bool Iter<T, Diff, Iteratable, Const>::operator!=(const iter_t& it) const
{
    return !(*this == it);
}

template<typename T, typename Diff, typename Iteratable, bool Const>
bool Iter<T, Diff, Iteratable, Const>::operator<(const iter_t& it) const
{
    return *this - it < 0;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
bool Iter<T, Diff, Iteratable, Const>::operator<=(const iter_t& it) const
{
    return *this - it <= 0;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
bool Iter<T, Diff, Iteratable, Const>::operator>(const iter_t& it) const
{
    return *this - it > 0;
}

template<typename T, typename Diff, typename Iteratable, bool Const>
bool Iter<T, Diff, Iteratable, Const>::operator>=(const iter_t& it) const
{
    return *this - it >= 0;
}
#endif

// deque implementation
#if 1
template<typename T>
Deque<T>::Deque() : data(nullptr), head(0), tail(0), capacity(0), count(0)
{ }

template<typename T>
Deque<T>::Deque(const Deque& dq) : data(nullptr), head(0), tail(0), capacity(0), count(0)
{
    try {
        for(const T& i : dq)
            push_back(i);
    }
    catch (...) {
        while (size())
            pop_back();
        throw std::current_exception();
    }
}

template<typename T>
Deque<T>::Deque(int count, const T& source) : data(nullptr), head(0), tail(0), capacity(0), count(0)
{
    try {
        while(count--)
            push_back(source);
    }
    catch (...) {
        while (size())
            pop_back();
        throw std::current_exception();
    }
}

template<typename T>
Deque<T>::~Deque()
{
    if(data) {
        while(size())
           pop_back();
        delete[] data;
    }
}

template<typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& dq)
{
    Deque<T> tmp;
    for(const T& i : dq)
        tmp.push_back(i);
    data = tmp.data;
    head = tmp.head;
    tail = tmp.tail;
    capacity = tmp.capacity;
    count = tmp.count;
    tmp.data = nullptr;
    return *this;
}

template<typename T>
void Deque<T>::push_back(const T& object)
{
    if(count + 1 >= capacity)
        realloc(std::max(capacity + capacity / 2, capacity + 2));

    create_object(data[tail], object);
    pushes++;
    inc(tail);
    count++;
}

template<typename T>
void Deque<T>::push_front(const T& object)
{
    if(count + 1 >= capacity)
        realloc(std::max(capacity + capacity / 2, capacity + 2));

    create_object(data[(head - 1 + capacity) % capacity], object);
    pushes++;
    dec(head);
    count++;
}

template<typename T>
void Deque<T>::pop_back()
{
    if(!size())
        throw std::out_of_range("pop_back from empty deque");

    remove_object(data[(tail - 1 + capacity) % capacity]);
    pops++;
    dec(tail);
    count--;
}

template<typename T>
void Deque<T>::pop_front()
{
    if(!size())
        throw std::out_of_range("pop_front from empty deque");

    remove_object(data[head]);
    pops++;
    inc(head);
    count--;
}

template<typename T>
void Deque<T>::insert(iterator itt, const T& object)
{
    int n = itt - begin();

    push_front(object);
    iterator ittt = begin() + n;
    for(auto it = begin(); it != ittt; ++it)
        std::swap(*it, *(it + 1));
}

template<typename T>
void Deque<T>::erase(iterator it)
{
    for(; it != begin(); --it)
        std::swap(*it, *(it - 1));
    pop_front();
}

template<typename T>
size_t Deque<T>::size() const
{
    return count;
}

template<typename T>
T& Deque<T>::operator[](size_t i)
{
    return *data[((int)i + head) % capacity];
}

template<typename T>
const T& Deque<T>::operator[](size_t i) const
{
    return *data[((int)i + head) % capacity];
}

template<typename T>
T& Deque<T>::at(size_t i)
{
    if(i >= size()) throw std::out_of_range("deque out of range");
    return (*this)[i];
}

template<typename T>
const T& Deque<T>::at(size_t i) const
{
    if(i >= size()) throw std::out_of_range("deque out of range");
    return (*this)[i];
}

template<typename T>
typename Deque<T>::iterator Deque<T>::begin()
{
    return iterator(*this, &data[head]);
}

template<typename T>
typename Deque<T>::iterator Deque<T>::end()
{
    return iterator(*this, &data[tail]);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::begin() const
{
    return const_iterator(*this, (const T**)&data[head]);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::end() const
{
    return const_iterator(*this, (const T**)&data[tail]);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cbegin() const
{
    return const_iterator(*this, (const T**)&data[head]);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cend() const
{
    return const_iterator(*this, (const T**)&data[tail]);
}

template<typename T>
typename Deque<T>::r_iterator Deque<T>::rbegin()
{
    return r_iterator(iterator(*this, &data[tail]));
}

template<typename T>
typename Deque<T>::r_iterator Deque<T>::rend()
{
    return r_iterator(iterator(*this, &data[head]));
}

template<typename T>
typename Deque<T>::const_r_iterator Deque<T>::rbegin() const
{
    return const_r_iterator(const_iterator(*this, (const T**)&data[tail]));
}

template<typename T>
typename Deque<T>::const_r_iterator Deque<T>::rend() const
{
    return const_r_iterator(const_iterator(*this, (const T**)&data[head]));
}

template<typename T>
typename Deque<T>::const_r_iterator Deque<T>::crbegin() const
{
    return const_r_iterator(const_iterator(*this, (const T**)&data[tail]));
}

template<typename T>
typename Deque<T>::const_r_iterator Deque<T>::crend() const
{
    return const_r_iterator(const_iterator(*this, (const T**)&data[head]));
}

template<typename T>
int Deque<T>::sub(const T** a, const T** b) const
{
    int x = (a - (const T**)data - head) % capacity;
    int y = (b - (const T**)data - head) % capacity;
    if (x < 0 && valid((const T**)data + x + capacity))
        x += capacity;
    return x - y;
}

template<typename T>
const T** Deque<T>::add(const T** a, int delta) const
{
    a += delta;
    if(a >= (const T**)data + capacity)
        a -= capacity;
    if(a < (const T**)data)
        a += capacity;
    return a;
}

template<typename T>
T** Deque<T>::add(T** a, int delta) const
{
    return (T**)add((const T**)a, delta);
}

template<typename T>
bool Deque<T>::valid(const T** a) const
{
    if (!(a < (const T**)data + capacity && a >= (const T**)data))
        return false;
    if (head > tail)
        return a < (const T**)&data[tail] || a >= (const T**)&data[head];
    return a < (const T**)&data[tail] && a >= (const T**)&data[head];
}

template<typename T>
int& Deque<T>::inc(int& i)
{
    i = (i + 1) % capacity;
    return i;
}

template<typename T>
int& Deque<T>::dec(int& i)
{
    i = (i - 1 + capacity) % capacity;
    return i;
}

template<typename T>
void Deque<T>::create_object(T*& at, const T& object)
{
    T* data = new T(object);
    at = data;
}

template<typename T>
void Deque<T>::remove_object(T*& at)
{
    delete at;
}

template<typename T>
void Deque<T>::realloc(int new_capacity)
{
    T** new_data = new T*[new_capacity];
    int new_head = new_capacity - count;
    int new_tail = 0;

    if(data) {
        int t = new_head;
        for(T& i : *this) {
            new_data[t] = &i;
            t = (t + 1) % new_capacity;
        }
        delete[] data;
    }

    capacity = new_capacity;
    data = new_data;
    head = new_head % capacity;
    tail = new_tail;
}

#endif
