#pragma once
#include <vector>
#include <concepts>

template<typename T, typename V, typename D>
concept IDictionary = requires (D dict, T key, V val) {
    {dict.find(key)} -> std::same_as<bool>;
    { dict.add(key, val) };
    { dict.get(key) } -> std::convertible_to<V&>;
};
template <typename T, typename V, typename Dict>
requires IDictionary<T, V, Dict>
class histogram {
    Dict tree_;
public:
    template<typename Data, typename GetKey, typename UpdateFunc>
    void build(const std::vector<Data>& data, GetKey get_key, UpdateFunc update_func, const V& init_val) {
        for (const auto& i : data) {
            T key = get_key(i);
            if (!tree_.find(key)) {
                tree_.add(key, init_val);
            }
            update_func(tree_.get(key), i);
        }
    }

    template<typename InputIt, typename GetKey, typename UpdateFunc>
    void build(InputIt first, InputIt last, GetKey get_key, UpdateFunc update_func, const V& init_val) {
        for (auto it = first; it != last; ++it) {
            T key = get_key(*it);
            if (!tree_.find(key)) {
                tree_.add(key, init_val);
            }
            update_func(tree_.get(key), *it);
        }
    }

    template<typename Container, typename GetKey, typename UpdateFunc>
    void build(const Container& container, GetKey get_key, UpdateFunc update_func, const V& init_val) {
        build(std::begin(container), std::end(container), get_key, update_func, init_val);
    }

    template<typename Data, typename GetKey, typename UpdateFunc>
    void build(std::initializer_list<Data> ilist, GetKey get_key, UpdateFunc update_func, const V& init_val) {
        build(ilist.begin(), ilist.end(), get_key, update_func, init_val);
    }

    Dict&  get_tree() {
        return tree_;
    }

};
