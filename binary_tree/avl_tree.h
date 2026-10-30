#pragma once
#include <memory>
#include <string>
#include <ranges>

template<typename T, typename V>
struct Node {
    std::unique_ptr<Node<T, V>> left;
    std::unique_ptr<Node<T, V>> right;
    T key;
    V value;
    int height;
    Node(T k, V v) : left(nullptr), right(nullptr), key(k), value(v), height(0){};
};

template <typename T, typename V>
class avl_tree {
    std::unique_ptr<Node<T, V>> head;

    void rotate_right(std::unique_ptr<Node<T, V>>& p) {
        std::unique_ptr<Node<T, V>> x = std::move(p->left);
        p->left = std::move(x->right);
        update_height(p);
        x->right = std::move(p);
        update_height(x);
        p = std::move(x);
    }

    void rotate_left(std::unique_ptr<Node<T, V>>& p) {
        std::unique_ptr<Node<T, V>> x = std::move(p->right);
        p->right = std::move(x->left);
        update_height(p);
        x->left = std::move(p);
        update_height(x);
        p = std::move(x);
    }

    int get_height(std::unique_ptr<Node<T, V>>& p) {
        if (!p) {
            return -1;
        }
        return p->height;
    }

    int balance_factor(const std::unique_ptr<Node<T, V>>& p) {
        if (!p) {
            return -1;
        }
        return get_height(p->left) - get_height(p->right);
    }

    void update_height(std::unique_ptr<Node<T, V>>& p) {
        p->height = 1 + std::max(get_height(p->left), get_height(p->right));
    }

    void balance(std::unique_ptr<Node<T, V>>& slot) {
        update_height(slot);
        int bf = balance_factor(slot);
        if (bf > 1) {
            if (balance_factor(slot->left) < 0) {
                rotate_left(slot->left);
            }
            rotate_right(slot);
        }
        else if (bf < -1) {
            if (balance_factor(slot->right) > 0) {
                rotate_right(slot->right);
            }
            rotate_left(slot);
        }
    }

    void rec_add(T key, V val, std::unique_ptr<Node<T, V>>& slot) {
        if (!slot) {
            slot = std::make_unique<Node<T, V>>(key, val);
        }
        else if (key > slot->key) {
            rec_add(key, val, slot->right);
        }
        else if (key < slot->key) {
            rec_add(key, val, slot->left);
        }
        else {
            slot->value = val;
        }
        balance(slot);
    }

    void rec_remove(T key, std::unique_ptr<Node<T, V>>& slot) {
        if (!slot) {
            return;
        }
        if (key > slot->key) {
            rec_remove(key, slot->right);
        }
        else if (key < slot->key) {
            rec_remove(key, slot->left);
        }
        else {
            if (!slot->left) {
                slot = std::move(slot->right);
            }
            else if (!slot->right) {
                slot = std::move(slot->left);
            }
            else {
                Node<T, V>* start = slot->right.get();
                while (start->left) {
                    start = start->left.get();
                }
                std::swap(start->key, slot->key);
                std::swap(start->value, slot->value);
                rec_remove(key, slot->right);
            }
        }
        if (slot) balance(slot);
    }

    template <typename F>
    void in_order(Node<T, V>* n, F&& f) {
        if (!n) {
            return;
        }
        in_order(n->left.get(), f);
        f(n->key, n->value);
        in_order(n->right.get(), f);
    }

    std::unique_ptr<Node<T, V>> clone(Node<T, V>* n) {
        if (!n) {
            return nullptr;
        }
        std::unique_ptr<Node<T, V>> c = std::make_unique<Node<T, V>>(n->key, n->value);
        c->height = n->height;
        c->left = clone(n->left.get());
        c->right = clone(n->right.get());
        return c;
    }

public:
    avl_tree() : head(nullptr){}
    avl_tree(T k, V v) : head(std::make_unique<Node<T, V>>(k, v)) {}
    avl_tree(std::unique_ptr<Node<T, V>> root) : head(std::move(root)) {}

    void add(T key, V val) {
        rec_add(key, val, head);
    }

    bool find(T key) const {
        Node<T, V>* start = head.get();
        while (start) {
            if (key == start->key) {
                return true;
            }
            if (key > start->key) {
                start = start->right.get();
            }
            else {
                start = start->left.get();
            }
        }
        return false;
    }

    V& get(T key) const {
        Node<T, V>* start = head.get();
        while (start) {
            if (key == start->key) {
                return start->value;
            }
            if (key > start->key) {
                start = start->right.get();
            }
            else {
                start = start->left.get();
            }
        }
        throw std::runtime_error("Нет такого ключа");
    }

    void remove(T key) {
        rec_remove(key, head);
    }

    template <typename F>
    void for_each(F f) {
        in_order(head.get(), f);
    }

    template <typename P>
    avl_tree<T, V> where(P pred) {
        avl_tree res;
        for_each([&](T& k, V& v) {
            if (pred(k, v)) res.add(k, v);
        });
        return res;
    }

    template <typename U, typename W, typename F>
    avl_tree<U, W> map(F f)  {
        avl_tree<U, W> res;
        for_each([&](T& k, V& v) {
            auto mapped = f(k, v);
            res.add(mapped.first, mapped.second);
        });
        return res;
    }

    template <typename A, typename F>
    A reduce(A init, F f) {
        for_each([&](T& k, V& v) {
            init = f(init, k, v);
        });
        return init;
    }

    avl_tree copy(const T& key) {
        Node<T, V>* start = head.get();
        while (start && !(start->key == key)) {
            if (key > start->key) {
                start = start->right.get();
            }
            else {
                start = start->left.get();
            }
        }
        return avl_tree(clone(start));
    }

    // std::string to_string() {
    //     std::string sany = "";
    //     in_order(head.get(),[&](T& k, V& v) {
    //         sany += std::to_string(k) + " ";
    //     });
    //     if (sany.size() > 0) {
    //         sany.erase(sany.size() - 1);
    //     }
    //     return sany;
    // }
};
