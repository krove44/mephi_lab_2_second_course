#pragma once
#include <memory>
#include <string>
#include <ranges>

template<typename T>
struct Node {
    std::unique_ptr<Node<T>> left;
    std::unique_ptr<Node<T>> right;
    T key;
    int height;
    Node(T k) : left(nullptr), right(nullptr), key(k), height(0){};
};



template <typename T>
class avl_tree {
    std::unique_ptr<Node<T>> head;

    void rotate_right(std::unique_ptr<Node<T>>& p) {
        std::unique_ptr<Node<T>> x = std::move(p->left);
        p->left = std::move(x->right);
        update_height(p);
        x->right = std::move(p);
        update_height(x);
        p = std::move(x);
    }

    void rotate_left(std::unique_ptr<Node<T>>& p) {
        std::unique_ptr<Node<T>> x = std::move(p->right);
        p->right = std::move(x->left);
        update_height(p);
        x->left = std::move(p);
        update_height(x);
        p = std::move(x);
    }

    int get_height(std::unique_ptr<Node<T>>& p) {
        if (!p) {
            return -1;
        }
        return p->height;
    }

    int balance_factor(const std::unique_ptr<Node<T>>& p) {
        if (!p) {
            return -1;
        }
        return get_height(p->left) - get_height(p->right);
    }

    void update_height(std::unique_ptr<Node<T>>& p) {
        p->height = 1 + std::max(get_height(p->left), get_height(p->right));
    }

    void balance(std::unique_ptr<Node<T>>& slot) {
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

    void rec_add(T value, std::unique_ptr<Node<T>>& slot) {
        if (!slot) {
            slot = std::make_unique<Node<T>>(value);
        }
        else if (value > slot->key) {
            rec_add(value,slot->right);
        }
        else {
            rec_add(value,slot->left);
        }
        balance(slot);
    }

    void rec_remove(T value, std::unique_ptr<Node<T>>& slot) {
        if (!slot) {
            return;
        }
        if (value > slot->key) {
            rec_remove(value, slot->right);
        }
        else if (value < slot->key) {
            rec_remove(value, slot->left);
        }
        else {
            if (!slot->left) {
                slot = std::move(slot->right);
            }
            else if (!slot->right) {
                slot = std::move(slot->left);
            }
            else {
                Node<T>* start = slot->right.get();
                while (start->left) {
                    start = start->left.get();
                }
                std::swap(start->key, slot->key);
                rec_remove(value, slot->right);
            }
        }
        if (slot) balance(slot);
    }

    template <typename F>
    void in_order(Node<T>* n, F&& f) {
        if (!n) {
            return;
        }
        in_order(n->left.get(), f);
        f(n->key);
        in_order(n->right.get(), f);
    }

    std::unique_ptr<Node<T>> clone(Node<T>* n) {
        if (!n) {
            return nullptr;
        }
        std::unique_ptr<Node<T>> c = std::make_unique<Node<T>>(n->key);
        c->height = n->height;
        c->left = clone(n->left.get());
        c->right = clone(n->right.get());
        return c;
    }



public:
    avl_tree() : head(nullptr){}
    avl_tree(T k) : head(std::make_unique<Node<T>>(k)) {}
    avl_tree(std::unique_ptr<Node<T>> root) : head(std::move(root)) {}

    void add(T val) {
        rec_add(val, head);
    }

    bool find(T val) {
            Node<T>* start = head.get();
            while (start) {
                if (val == start->key) {
                    return true;
                }
                if (val > start->key) {
                    start = start->right.get();
                }
                else {
                    start = start->left.get();
                }
            }
            return false;
        }

    void remove(T val) {
        rec_remove(val, head);
    }

    template <typename F>
    void for_each(F f) {
        in_order(head.get(), f);
    }

    template <typename P>
    avl_tree<T> where(P pred) {
        avl_tree res;
        for_each([&](T& k) {
            if (pred(k)) res.add(k);
        });
        return res;
    }

    template <typename U, typename F>
    avl_tree<U> map(F f)  {
        avl_tree<U> res;
        for_each([&](T& k) { res.add(f(k)); });
        return res;
    }

    template <typename A, typename F>
    A reduce(A init, F f) {
        for_each([&](T& k) { init = f(init, k); });
        return init;
    }
    
    avl_tree copy(const T& key) {
        Node<T>* start = head.get();
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

    std::string to_string() {
        std::string sany = "";
        in_order(head.get(),[&](T& k) {
            sany += std::to_string(k) + " ";
        });
        if (sany.size() > 0) {
            sany.erase(sany.size() - 1);
        }
        return sany;
    }
};

avl_tree<std::string> from_string(std::string& s) {
        avl_tree<std::string> tree;
        for (auto part : std::string_view(s) | std::views::split(' ')) {
            std::string elem(part.begin(), part.end());
            tree.add(elem);
        }
        return tree;
    }