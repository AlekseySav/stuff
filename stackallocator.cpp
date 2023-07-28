#include <iostream>

template<size_t Size>
struct StackStorage
{
public:
    StackStorage();
    void* alloc(size_t align, size_t size);
private:
    char* top;
    char data[Size];
};


template<typename T, size_t Size>
struct StackAllocator
{
public:
    typedef T value_type;
    template<typename Other>
    struct rebind { typedef StackAllocator<Other, Size> other; };
public:
    template<typename Other>
    StackAllocator(const StackAllocator<Other, Size>& a) noexcept;
    StackAllocator(const StackAllocator& a) noexcept = default;
    StackAllocator(StackStorage<Size>& storage) noexcept;

    T* allocate(const size_t size) const;
    void deallocate(T* const, size_t) const noexcept;

    bool operator==(const StackAllocator& a) const noexcept;
    bool operator!=(const StackAllocator& a) const noexcept;
private:
    StackStorage<Size>* storage;

    template<typename Other, size_t S>
    friend class StackAllocator;
};


// iterates through Node with Node* prev, Node* next, T value fields
template<typename T, typename Node, bool Const>
class Iter
{
public:
    typedef std::conditional_t<Const, const T, T> value_t;
    typedef std::conditional_t<Const, const Node, Node> node_t;
    typedef Iter<T, Node, Const> iter_t;
    typedef ptrdiff_t diff_t;
public:
    typedef value_t value_type;
    typedef diff_t difference_type;
    typedef value_t* pointer;
    typedef value_t& reference;
    typedef std::reverse_iterator<iter_t> reverse_t;
    typedef std::bidirectional_iterator_tag iterator_category;
public:
    explicit Iter(node_t* node);
    Iter(const Iter<T, Node, false>& it);

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

    constexpr node_t* asNode() { return node; }
private:
    node_t* node;
    template<typename _T, typename _Node, bool _Const>
    friend class Iter;
};


template<typename T, typename Alloc = std::allocator<T>>
class List
{
public:
    template<typename P>
    struct Buffer
    {
        alignas(alignof(P)) char src[sizeof(P)];
        P* get() const { return (P*)src; }
    };

    struct Node
    {
        T& value() const { return *buf.get(); }
        ~Node() { buf.get()->~T(); }
        
        Buffer<T> buf;
        Node* prev, * next;
    };

    using iterator = Iter<T, Node, false>;
    using const_iterator = Iter<T, Node, true>;
    using r_iterator = typename iterator::reverse_t;
    using const_r_iterator = typename const_iterator::reverse_t;
    using reverse_iterator = r_iterator;
    using const_reverse_iterator = const_r_iterator;
public:
    List();
    List(Alloc a);
    List(size_t n, Alloc a = Alloc());
    List(size_t n, const T& object, Alloc a = Alloc());
    List(const List& list);
    List(List&& list);
    ~List();

    List& operator=(const List& list);
    List& operator=(List&& list);

    void push_back();
    void push_back(const T& object);
    void push_front(const T& object);
    void pop_back();
    void pop_front();

    void insert(const_iterator it, const T& object);
    void erase(const_iterator it);

    iterator emplace_node(const_iterator it, Node* node)
    {
        count++;
        Node* at = (Node*)it.asNode();
        node->next = at;
        node->prev = at->prev;
        at->prev->next = node;
        at->prev = node;
        return iterator(node);
    }

    template<typename... Args>
    iterator emplace(const_iterator it, Args&&... args)
    {
        return emplace_node(it, create_node(std::forward<Args>(args)...));
    }

    template<typename... Args>
    Node* create_node(Args&&... args)
    {
        Node* res = make_node(std::forward<Args>(args)...);
        count--;
        return res;
    }

    void remove_node(Node* node)
    {
        delete_node(node);
        count++;
    }

    size_t size() const;
    Alloc get_allocator() const;

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

    using base_traits = std::allocator_traits<Alloc>;
    using allocator = typename base_traits::template rebind_alloc<Node>;
    using traits =  typename base_traits::template rebind_traits<Node>;
private:
    template<typename... Args>
    Node* make_node(Args&&... args);
    void delete_node(Node* node);
    constexpr Node* head() const { return fake->next; }
    constexpr Node* tail() const { return fake->prev; }
    constexpr Node*& head() { return fake->next; }
    constexpr Node*& tail() { return fake->prev; }
private:
    allocator alloc;
    size_t count;
    Node* fake;
};

// stack storage & allocator
#if 1
template<size_t Size>
StackStorage<Size>::StackStorage() : top(data)
{ }

template<size_t Size>
void* StackStorage<Size>::alloc(size_t align, size_t size)
{
    if ((size_t)top % align)
        top += align - (size_t)top % align;
    top += size;
    return &top[-size];
}

template<typename T, size_t Size>
template<typename Other>
StackAllocator<T, Size>::StackAllocator(const StackAllocator<Other, Size>& a) noexcept : storage(a.storage)
{ }

template<typename T, size_t Size>
StackAllocator<T, Size>::StackAllocator(StackStorage<Size>& storage) noexcept : storage(&storage)
{ }

template<typename T, size_t Size>
T* StackAllocator<T, Size>::allocate(const size_t size) const
{
    return (T*)storage->alloc(alignof(T), size * sizeof(T));
}

template<typename T, size_t Size>
void StackAllocator<T, Size>::deallocate(T* const, size_t) const noexcept
{ }

template<typename T, size_t Size>
bool StackAllocator<T, Size>::operator==(const StackAllocator& a) const noexcept
{
    return storage == a.storage;
}

template<typename T, size_t Size>
bool StackAllocator<T, Size>::operator!=(const StackAllocator& a) const noexcept
{
    return storage != a.storage;
}

#endif

// iterator
#if 2
template<typename T, typename Node, bool Const>
Iter<T, Node, Const>::Iter(node_t* node) : node(node)
{ }

template<typename T, typename Node, bool Const>
Iter<T, Node, Const>::Iter(const Iter<T, Node, false>& it) : node(it.node)
{ }

template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::value_t& Iter<T, Node, Const>::operator*()
{
    return node->value();
}

template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::value_t* Iter<T, Node, Const>::operator->()
{
    return &node->value();
}


template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::iter_t& Iter<T, Node, Const>::operator++()
{
    node = node->next;
    return *this;
}

template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::iter_t& Iter<T, Node, Const>::operator--()
{
    node = node->prev;
    return *this;
}

template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::iter_t Iter<T, Node, Const>::operator++(int)
{
    iter_t copy = *this;
    ++*this;
    return copy;
}

template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::iter_t Iter<T, Node, Const>::operator--(int)
{
    iter_t copy = *this;
    --*this;
    return copy;
}


template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::iter_t& Iter<T, Node, Const>::operator+=(diff_t n)
{
    if (n < 0)
        *this -= -n;
    while (n--)
        ++*this;
    return *this;
}

template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::iter_t& Iter<T, Node, Const>::operator-=(diff_t n)
{
    if (n < 0)
        *this += -n;
    while (n--)
        --*this;
    return *this;
}

template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::iter_t Iter<T, Node, Const>::operator+(diff_t n) const
{
    iter_t copy = *this;
    return copy += n;
}

template<typename T, typename Node, bool Const>
typename Iter<T, Node, Const>::iter_t Iter<T, Node, Const>::operator-(diff_t n) const
{
    iter_t copy = *this;
    return copy -= n;
}

template<typename T, typename Node, bool Const>
bool Iter<T, Node, Const>::operator==(const iter_t& it) const
{
    return node == it.node;
}

template<typename T, typename Node, bool Const>
bool Iter<T, Node, Const>::operator!=(const iter_t& it) const
{
    return !(*this == it);
}
#endif

// constructors, destructors, operator=
#if 3
template<typename T, typename Alloc>
List<T, Alloc>::List() : alloc(Alloc()), count(0), fake(traits::allocate(alloc, 1))
{
    fake->next = fake->prev = fake;
}

template<typename T, typename Alloc>
List<T, Alloc>::List(Alloc a) : alloc(a), count(0), fake(traits::allocate(alloc, 1))
{
    fake->next = fake->prev = fake;
}

template<typename T, typename Alloc>
List<T, Alloc>::List(size_t n, Alloc a) : List(a)
{
    List<T, Alloc> tmp(a);
    while (n--)
        tmp.push_back();
    *this = std::move(tmp);
}

template<typename T, typename Alloc>
List<T, Alloc>::List(size_t n, const T& object, Alloc a) : List(a)
{
    while (n--)
        push_back(object);
}

template<typename T, typename Alloc>
List<T, Alloc>::List(const List& list) : List(base_traits::select_on_container_copy_construction(list.alloc))
{
    for (auto& i : list)
        push_back(i);
}

template<typename T, typename Alloc>
List<T, Alloc>::List(List&& list) : alloc(list.alloc), count(list.count), fake(list.fake)
{
    list.fake = nullptr;
}

template<typename T, typename Alloc>
List<T, Alloc>::~List()
{
    if (!fake)
        return;
    while (size())
        pop_back();
    traits::deallocate(alloc, fake, 1);
}

template<typename T, typename Alloc>
List<T, Alloc>& List<T, Alloc>::operator=(const List& list)
{
    List<T, Alloc> tmp(base_traits::propagate_on_container_copy_assignment::value ?
        list.alloc : (const allocator)base_traits::select_on_container_copy_construction(list.alloc));
    for (auto& i : list)
        tmp.push_back(i);
    *this = std::move(tmp);
    return *this;
}

template<typename T, typename Alloc>
List<T, Alloc>& List<T, Alloc>::operator=(List&& list)
{
    if (fake)
    {
        while (size())
            pop_back();
        traits::deallocate(alloc, fake, 1);
    }
    alloc = list.alloc;
    count = list.count;
    fake = list.fake;
    list.fake = nullptr;
    return *this;
}
#endif

// push, pop
#if 4

template<typename T, typename Alloc>
void List<T, Alloc>::push_back()
{
    Node* node = make_node();

    node->prev = tail();
    if (size())
        tail()->next = node;
    else
        head() = node;
    tail() = node;
}

template<typename T, typename Alloc>
void List<T, Alloc>::push_back(const T& object)
{
    Node* node = make_node(object);
    node->prev = tail();
    if (size())
        tail()->next = node;
    else
        head() = node;
    tail() = node;
}

template<typename T, typename Alloc>
void List<T, Alloc>::push_front(const T& object)
{
    Node* node = make_node(object);
    node->next = head();
    if (size())
        head()->prev = node;
    else
        tail() = node;
    head() = node;
}

template<typename T, typename Alloc>
void List<T, Alloc>::pop_back()
{
    Node* node = tail();
    tail() = tail()->prev;
    tail()->next = fake;
    delete_node(node);
    if (!size())
        head() = fake;
}

template<typename T, typename Alloc>
void List<T, Alloc>::pop_front()
{
    Node* node = head();
    head() = head()->next;
    head()->prev = fake;
    delete_node(node);
    if (!size())
        tail() = fake;
}

#endif

// insert, erase
#if 5

template<typename T, typename Alloc>
void List<T, Alloc>::insert(const_iterator it, const T& object)
{
    Node* node = make_node(object);
    Node* at = (Node*)it.asNode();
    node->next = at;
    node->prev = at->prev;
    at->prev->next = node;
    at->prev = node;
}

template<typename T, typename Alloc>
void List<T, Alloc>::erase(const_iterator it)
{
    Node* at = (Node*)it.asNode();
    at->prev->next = at->next;
    at->next->prev = at->prev;
    delete_node(at);
}

#endif

// size, get_allocator, begin, end, etc.
#if 6
template<typename T, typename Alloc>
size_t List<T, Alloc>::size() const
{
    return count;
}

template<typename T, typename Alloc>
Alloc List<T, Alloc>::get_allocator() const
{
    return Alloc(alloc);
}

template<typename T, typename Alloc>
typename List<T, Alloc>::iterator List<T, Alloc>::begin()
{
    return iterator(head());
}

template<typename T, typename Alloc>
typename List<T, Alloc>::iterator List<T, Alloc>::end()
{
    return iterator(fake);
}

template<typename T, typename Alloc>
typename List<T, Alloc>::const_iterator List<T, Alloc>::begin() const
{
    return const_iterator(head());
}

template<typename T, typename Alloc>
typename List<T, Alloc>::const_iterator List<T, Alloc>::end() const
{
    return const_iterator(fake);
}

template<typename T, typename Alloc>
typename List<T, Alloc>::const_iterator List<T, Alloc>::cbegin() const
{
    return const_iterator(head());
}

template<typename T, typename Alloc>
typename List<T, Alloc>::const_iterator List<T, Alloc>::cend() const
{
    return const_iterator(fake);
}

template<typename T, typename Alloc>
typename List<T, Alloc>::r_iterator List<T, Alloc>::rbegin()
{
    return r_iterator(end());
}

template<typename T, typename Alloc>
typename List<T, Alloc>::r_iterator List<T, Alloc>::rend()
{
    return r_iterator(begin());
}

template<typename T, typename Alloc>
typename List<T, Alloc>::const_r_iterator List<T, Alloc>::rbegin() const
{
    return const_r_iterator(end());
}

template<typename T, typename Alloc>
typename List<T, Alloc>::const_r_iterator List<T, Alloc>::rend() const
{
    return const_r_iterator(begin());
}

template<typename T, typename Alloc>
typename List<T, Alloc>::const_r_iterator List<T, Alloc>::crbegin() const
{
    return const_r_iterator(cend());
}

template<typename T, typename Alloc>
typename List<T, Alloc>::const_r_iterator List<T, Alloc>::crend() const
{
    return const_r_iterator(cbegin());
}

#endif

// private
#if 7
template<typename T, typename Alloc>
template<typename... Args>
typename List<T, Alloc>::Node* List<T, Alloc>::make_node(Args&&... args)
{
    Node* node = traits::allocate(alloc, 1);
    Alloc construct(alloc);
    try {
        base_traits::construct(construct, node->buf.get(), std::forward<Args>(args)...);
    }
    catch (...) {
        traits::deallocate(alloc, node, 1);
        throw;
    }
    count++;
    node->next = node->prev = fake;
    return node;
}

template<typename T, typename Alloc>
void List<T, Alloc>::delete_node(Node* node)
{
    traits::destroy(alloc, node);
    traits::deallocate(alloc, node, 1);
    count--;
}
#endif
