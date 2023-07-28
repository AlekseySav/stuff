
#include <vector>
#include <limits>

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
    Iter(const Iter& it);
    ~Iter() { }

    iter_t& operator=(const iter_t& it) { node = (node_t*)it.node; return *this; }

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
    
    operator Iter<T, Node, true> () const { return Iter<T, Node, true>(node); }

    bool operator==(const iter_t& it) const;
    bool operator!=(const iter_t& it) const;

    constexpr node_t* asNode() const { return node; }
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

    const_iterator emplace_node(const_iterator it, Node* node)
    {
        count++;
        Node* at = (Node*)it.asNode();
        node->next = at;
        node->prev = at->prev;
        at->prev->next = node;
        at->prev = node;
        return const_iterator(node);
    }

    template<typename... Args>
    iterator emplace(const_iterator it, Args&&... args)
    {
        return iterator(const_cast<typename iterator::node_t*>(emplace_node(it, create_node(std::forward<Args>(args)...)).asNode()));
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

    typedef typename Alloc::template rebind<Node>::other allocator;
    typedef std::allocator_traits<allocator> traits;
    typedef std::allocator_traits<Alloc> base_traits;
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
Iter<T, Node, Const>::Iter(const Iter& it) : node(it.node)
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
    list.count = 0;
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
    list.count = 0;
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
        throw std::current_exception();
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


template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = std::allocator<std::pair<Key, Value>>>
class UnorderedMap
{
public:
    using NodeType = std::pair<const Key, Value>;

    struct Item
    {
        NodeType kv;
        size_t hash;
    };

    using iterator = Iter<NodeType, typename List<NodeType, Alloc>::Node, false>;
    using const_iterator = Iter<NodeType, typename List<NodeType, Alloc>::Node, true>;
    using allocator = typename Alloc::template rebind<NodeType>::other;
public:
    iterator begin() { return list.begin(); }
    iterator end() { return list.end(); }
    const_iterator begin() const { return list.begin(); }
    const_iterator end() const { return list.end(); }
    const_iterator cbegin() const { return list.cbegin(); }
    const_iterator cend() const { return list.cend(); }

    size_t max_size() const { return std::numeric_limits<size_t>::max(); }
    size_t size() const { return list.size(); }
    float max_load_factor() const { return max_factor; }
    void max_load_factor(float factor) { max_factor = factor; }
    float load_factor() const { return (float)size() / buckets.size(); }

    const_iterator find(const Key& key) const
    {
        size_t hash = get_hash(key);
        typename List<NodeType, Alloc>::const_iterator it = find_by_hash(key, hash);
        if (get_hash(it->first) == hash)
            return it;
        return end();
    }

    iterator find(const Key& key)
    {
        size_t hash = get_hash(key);
        typename List<NodeType, Alloc>::const_iterator it = find_by_hash(key, hash);
        if (it != list.end() && get_hash(it->first) == hash)
            return iterator((typename iterator::node_t*)it.asNode());
        return end();
    }

    void reserve(size_t n)
    {
        if (buckets.size() >= n)
            return;
        buckets.resize(n, nullptr);
        rehash();
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        rehash();
        typename List<NodeType, Alloc>::Node* node = list.create_node(std::forward<Args>(args)...);
        size_t hash = get_hash(node->value().first);

        typename List<NodeType, Alloc>::const_iterator it = find_by_hash(node->value().first, hash);
        bool ins = it == list.end() || get_hash(it->first) != hash;
        if (ins)
            it = list.emplace_node(it, node);
        else
            list.remove_node(node);
        update_bucket(it, hash);

        return std::make_pair(iterator((typename iterator::node_t*)it.asNode()), ins);
    }

    std::pair<iterator, bool> insert(const NodeType& kv)
    {
        rehash();
        size_t hash = get_hash(kv.first);
        typename List<NodeType, Alloc>::const_iterator it = find_by_hash(kv.first, hash);
        bool ins = it == list.end() || get_hash(it->first) != hash;
        if (ins)
            it = list.emplace(it, kv);
        update_bucket(it, hash);
        return std::make_pair(iterator((typename iterator::node_t*)it.asNode()), ins);
    }

    template<typename Pair>
    std::pair<iterator, bool> insert(Pair&& kv)
    {
        rehash();
        size_t hash = get_hash(kv.first);
        typename List<NodeType, Alloc>::const_iterator it = find_by_hash(kv.first, hash);
        bool ins = it == list.end() || get_hash(it->first) != hash;
        if (ins)
            it = list.emplace(it, std::move(kv));
        update_bucket(it, hash);
        return std::make_pair(iterator((typename iterator::node_t*)it.asNode()), ins);
    }

    template<typename InputIterator>
    void insert(InputIterator a, InputIterator b)
    {
        size_t count = 0;
        for (auto it = a; it != b; ++it)
            count++;

        reserve(buckets.size() + count);
        for (auto it = a; it != b; ++it)
        {
            const auto& n = *it;
            insert(n);
        }
    }

    template<typename InputIterator>
    void erase(InputIterator it)
    {
        buckets[get_hash(it->first)] = nullptr;
        list.erase(it);
    }

    template<typename InputIterator>
    void erase(InputIterator a, InputIterator b)
    {
        auto next = a;
        for (auto it = a; it != b; it = next)
        {
            next = it;
            ++next;
            erase(it);
        }
    }

    Value& operator[](const Key& key)
    {
        const_iterator it = find(key);
        if (it == end())
            it = emplace(key, Value()).first;
        return (Value&)it.asNode()->value().second;
    }

    Value& at(const Key& key)
    {
        const_iterator it = find(key);
        if (it == end())
            throw std::out_of_range("no key");
        return (Value&)it.asNode()->value().second;
    }

    const Value& at(const Key& key) const
    {
        const_iterator it = find(key);
        if (it == end())
            throw std::out_of_range("no key");
        return it.asNode()->value().second;
    }

    UnorderedMap() : max_factor(0.66f), buckets(2) { }
    UnorderedMap(UnorderedMap&& map) : max_factor(map.max_factor), list(std::move(map.list)), buckets(std::move(map.buckets)) { }
    UnorderedMap(const UnorderedMap& map) : max_factor(map.max_factor), buckets(map.buckets.size())
    {
        for (auto& i : map)
            insert(i);
    }

    UnorderedMap& operator=(UnorderedMap&& map)
    {
        max_factor = map.max_factor;
        buckets = std::move(map.buckets);
        list = std::move(map.list);
        return *this;
    }

    UnorderedMap& operator=(const UnorderedMap& map)
    {
        UnorderedMap copy(map);
        *this = std::move(copy);
        return *this;
    }
    
private:
    size_t get_hash(const Key& key) const
    {
        return Hash()(key) % buckets.size();
    }

    void update_bucket(const_iterator it, size_t hash)
    {
        if (!buckets[hash])
            buckets[hash] = (typename List<NodeType, Alloc>::Node*)it.asNode();
    }

    const_iterator find_by_hash(const Key& key, size_t hash) const
    {
        if (!buckets[hash])
            return begin();
        typename List<NodeType, Alloc>::const_iterator it(buckets[hash]);
        for (; it != cend() && get_hash(it->first) == hash; ++it)
        {
            if (Equal()(it->first, key))
                return it;
        }
        return it;
    }

    void rehash()
    {
        if (load_factor() < max_load_factor())
            return;
        UnorderedMap copy;
        copy.buckets.resize(buckets.size() * 2 * (1 / max_factor));
        for (iterator it = begin(); it != end(); it++)
            copy.update_bucket(it, copy.get_hash(it->first));
        copy.list = std::move(list);
        *this = std::move(copy);
    }
private:
    float max_factor;
    List<NodeType, Alloc> list;
    List<Hash> hash_cache;
    std::vector<typename List<NodeType, Alloc>::Node*, typename Alloc::template rebind<typename List<NodeType, Alloc>::Node*>::other> buckets;
};
