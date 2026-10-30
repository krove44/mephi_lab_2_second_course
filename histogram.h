#pragma once
#include <vector>
#include "binary_tree/avl_tree.h"

template<typename T, typename V>
class histogram {
    avl_tree<T, V> tree_;
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

    const avl_tree<T, V>&  get_tree() const {
        return tree_;
    }

};
