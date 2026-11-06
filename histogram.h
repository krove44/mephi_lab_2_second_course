#pragma once
#include <concepts>
#include <functional>
#include <iterator>

template <typename C>
concept IterableContainer = requires(C c) {
    { std::begin(c) };
    { std::end(c) };
    typename C::value_type;
};

template<typename T, typename V, typename D>
concept IDictionary = requires (D dict, T key, V val) {
    {dict.find(key)} -> std::same_as<bool>;
    { dict.add(key, val) };
    { dict.get(key) } -> std::convertible_to<V&>;
};
template <typename T, typename V, typename Dict>
requires IDictionary<T, V, Dict>
class histogram {
    Dict dict_;
public:
    histogram() : dict_(){};
    template<typename InputIt>
    void build(InputIt first, InputIt last, std::function<T(std::iter_reference_t<InputIt>)> get_key, std::function<void(V&, std::iter_reference_t<InputIt>)> update_func, const V& init_val) {
        for (auto it = first; it != last; ++it) {
            T key = get_key(*it);
            if (!dict_.find(key)) {
                dict_.add(key, init_val);
            }
            update_func(dict_.get(key), *it);
        }
    }

    template<IterableContainer Container>
    void build(const Container& container, std::function<T(const typename Container::value_type&)> get_key, std::function<void(V&, const typename Container::value_type&)> update_func, const V& init_val) {
        build(std::begin(container), std::end(container), get_key, update_func, init_val);
    }

    template<typename Data>
    void build(std::initializer_list<Data> ilist, std::function<T(const Data&)> get_key, std::function<void(V&, const Data&)> update_func, const V& init_val) {
        build(ilist.begin(), ilist.end(), get_key, update_func, init_val);
    }
    
    template<typename Data>
    void remove(const Data& item, std::function<T(const Data&)> get_key, std::function<void(V&, const Data&)> remove_func) {
        T key = get_key(item);
        if (dict_.find(key)) {
            remove_func(dict_.get(key), item);
        }
    }

    Dict&  get_tree() {
        return dict_;
    }

};
