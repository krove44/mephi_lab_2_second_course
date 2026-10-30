#pragma once
#include <functional>
#include <vector>
#include "binary_tree/avl_tree.h"

template<typename T, typename V>
class histogram {
    avl_tree<T, V> tree_;
public:
    template<typename Data>
    void build(const std::vector<Data>& data, const std::function<T(const Data&)>& get_key, const std::function<void(V&, const Data&)>& update_func, const V& init_val) {
        for (const auto& i : data) {
            T key = get_key(i);
            if (!tree_.find(key)) {
                tree_.add(key, init_val);
            }
            update_func(tree_.get(key), i);
        }
    };

    const avl_tree<T, V>&  get_tree() const {
        return tree_;
    }



};
