#include <iostream>
#include <string>
#include <variant>
#include <cassert>
#include <vector>

namespace {

    template<typename Target, typename T, typename... Types>
    struct contains_s
    {
        static const bool value = contains_s<Target, Types...>::value;
    };

    template<typename Target, typename... Types>
    struct contains_s<Target, Target, Types...>
    {
        static const bool value = true;
    };

    template<typename Target, typename T>
    struct contains_s<Target, T>
    {
        static const bool value = false;
    };

    template<typename Target>
    struct contains_s<Target, Target>
    {
        static const bool value = true;
    };

    template<typename Target, typename T, typename... Types>
    struct index_of_s
    {
        static const size_t value = index_of_s<Target, Types...>::value + 1;
    };

    template<typename Target, typename... Types>
    struct index_of_s<Target, Target, Types...>
    {
        static const size_t value = 0;
    };

    template<typename Target>
    struct index_of_s<Target, Target>
    {
        static const size_t value = 0;
    };

    template<typename Target, typename T>
    struct index_of_s<Target, T>
    { };

    template<size_t S, typename T, typename... Types>
    struct type_of_s
    {
        using value = typename type_of_s<S - 1, Types...>::value;
    };

    template<typename T, typename... Types>
    struct type_of_s<0, T, Types...>
    {
        using value = T;
    };

    template<typename T>
    struct type_of_s<0, T>
    {
        using value = T;
    };
}

namespace {
    struct empty_type {};
    struct really_bad_type {};

    template<typename T, typename Y>
    struct promotion_s { static const bool value = false; };

    template<> struct promotion_s<char, int> { static const bool value = true; };
    template<> struct promotion_s<short, int> { static const bool value = true; };
    template<> struct promotion_s<unsigned char, int> { static const bool value = true; };
    template<> struct promotion_s<unsigned short, int> { static const bool value = true; };
    template<> struct promotion_s<float, double> { static const bool value = true; };

    template<typename T, typename Y>
    constexpr bool promotion_v = promotion_s<T, Y>::value;

    template<bool C1, bool C2, typename T1, typename T2>
    struct pick_s
    {
        using value = std::conditional_t<C1,
            std::conditional_t<C2, really_bad_type, T1>,
            std::conditional_t<C2, T2, empty_type>>;
    };

    template<typename Target, typename T, typename... Types>
    struct same_s
    {
        using prev = same_s<Target, Types...>;
        using value = typename pick_s<!std::is_same_v<typename prev::value, empty_type>, std::is_same_v<Target, std::remove_const_t<T>>, typename prev::value, T>::value;
    };

    template<typename Target, typename T>
    struct same_s<Target, T> { using value = empty_type; };

    template<typename Target, typename T, typename... Types>
    struct prom_s
    {
        using prev = prom_s<Target, Types...>;
        using value = typename pick_s<!std::is_same_v<typename prev::value, empty_type>, promotion_v<Target, T>, typename prev::value, T>::value;
    };

    template<typename Target, typename T>
    struct prom_s<Target, T> { using value = empty_type; };

    template<typename Target, typename T, typename... Types>
    struct cons_s
    {
        using prev = cons_s<Target, Types...>;
        using value = typename pick_s<!std::is_same_v<typename prev::value, empty_type>, std::is_constructible_v<T, Target>, typename prev::value, T>::value;
    };

    template<typename Target, typename T>
    struct cons_s<Target, T> { using value = empty_type; };

    template<typename Same, typename Prom, typename Cons>
    struct choose_best_s { using value = Same; };

    template<typename Prom, typename Cons>
    struct choose_best_s<empty_type, Prom, Cons> { using value = Prom; };

    template<typename Cons>
    struct choose_best_s<empty_type, empty_type, Cons> { using value = Cons; };

    template<>
    struct choose_best_s<empty_type, empty_type, empty_type> { using value = really_bad_type; };
}

template<typename Target, typename... Types>
constexpr bool contains = contains_s<Target, Types...>::value;

template<typename Target, typename... Types>
constexpr size_t index_of = index_of_s<Target, Types...>::value;

template<size_t Size, typename... Types>
using type_of = typename type_of_s<Size, Types...>::value;

// really_bad_type if unable to choose
template<typename Target, typename... Types>
using choose_best = typename choose_best_s< typename same_s<Target, Types..., empty_type>::value, 
                                            typename prom_s<Target, Types..., empty_type>::value,
                                            typename cons_s<Target, Types..., empty_type>::value
                                            >::value;


template<typename... Types>
union Variant_u {};

template<typename T, typename... Types>
union Variant_u<T, Types...>
{
    T object;
    Variant_u<Types...> rest;

    Variant_u() {}
    ~Variant_u() {}

    template<size_t N> constexpr auto& get()
    {
        if constexpr (N == 0) return object;
        else return rest.template get<N - 1>();
    }

    template<typename U> constexpr auto& put(const U& value)
    {
        if constexpr (std::is_same_v<T, std::decay_t<U>>) new(std::launder(&object)) T(value);
        else rest.put(std::forward<U>(value));
    }
};


template<typename... Types>
class Variant
{
public:
    ~Variant() { if (buffer) buffer->~base(); if (buffer) delete (char*)buffer; }
    Variant() : Variant(type_of<0, Types...>()) {}

    Variant(const Variant& v) : buffer(v.buffer->copy()), index(v.index) {}

    Variant(Variant&& v) : buffer(v.buffer->move()), index(v.index) {}

    template<typename T,
        typename = std::enable_if_t<!std::is_same_v<choose_best<T, Types...>, really_bad_type>>>
    Variant(T&& t) :
        buffer(new derived<choose_best<T, Types...>>(std::forward<T>(t))),
        index(index_of<choose_best<T, Types...>, Types...>) {}

    Variant& operator=(Variant&& v)
    {
        this->~Variant();
        buffer = v.buffer->move();
        index = v.index;
        return *this;
    }

    Variant& operator=(const Variant& v)
    {
        Variant tmp(v);
        std::swap(*this, tmp);
        return *this;
    }

    template<typename T,
        typename = std::enable_if_t<!std::is_same_v<choose_best<T, Types...>, really_bad_type>>>
    Variant& operator=(T&& t)
    {
        Variant v(std::forward<T>(t));
        *this = std::move(v);
        return *this;
    }

    template<typename T, typename... Args>
    T& emplace(Args&&... args)
    {
        this->~Variant();
        buffer = new derived<T>(std::forward<Args>(args)...);
        index = index_of<T, Types...>;
        return data<T>();
    }

    template<typename T, typename U, typename... Args>
    T& emplace(std::initializer_list<U> init, Args&&... args)
    {
        this->~Variant();
        buffer = new derived<T>(init, std::forward<Args>(args)...);
        index = index_of<T, Types...>;
        return data<T>();
    }

    template<size_t S, typename... Args>
    type_of<S, Types...>& emplace(Args&&... args)
    {
        this->~Variant();
        buffer = new derived<type_of<S, Types...>>(std::forward<Args>(args)...);
        index = S;
        return data<type_of<S, Types...>>();
    }

    bool valueless_by_exception() { return index == -1; }
private:
    template<typename T>
    constexpr T& data() { return static_cast<derived<T>*>(buffer)->object; }

    struct base
    {
        virtual base* copy() = 0;
        virtual base* move() = 0;
        virtual ~base() {}
    };

    template<typename T>
    struct derived : base
    {
        T object;

        template<typename... Args>
        derived(Args&&... args) : object(std::forward<Args>(args)...) {}

        virtual base* copy() override { return new derived<T>(object); }
        virtual base* move() override { return new derived<T>(std::move(object)); }
    };

    base* buffer = nullptr;
    size_t index;

    template<typename T, typename... Ts>
    friend bool holds_alternative(Variant<Ts...>& v);
    template<typename T, typename... Ts>
    friend T& get(Variant<Ts...>& v);
    template<size_t S, typename... Ts>
    friend type_of<S, Ts...>& get(Variant<Ts...>& v);
};


template<typename T, typename... Types>
bool holds_alternative(Variant<Types...>& v)
{
    return index_of<T, Types...> == v.index;
}

template<typename T, typename... Types>
T& get(Variant<Types...>& v)
{
    if (v.index != index_of<T, Types...>)
        throw std::bad_variant_access();
    return v.template data<T>();
}

template<typename T, typename... Types>
const T& get(const Variant<Types...>& v)
{
    return get<T>(static_cast<Variant<Types...>&>(v));
}

template<typename T, typename... Types>
T&& get(Variant<Types...>&& v)
{
    return get<T>(static_cast<Variant<Types...>&>(v));
}

template<size_t S, typename... Types>
type_of<S, Types...>& get(Variant<Types...>& v)
{
    return get<type_of<S, Types...>>(v);
}
