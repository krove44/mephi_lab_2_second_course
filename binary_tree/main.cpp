#include <iostream>
#include "avl_tree.h"


int main() {
    avl_tree<int> sp(5);
    sp.add(6);
    sp.add(7);
    std::cout << sp.find(6) << std::endl;
    sp.remove(6);
    std::cout << sp.find(6) << std::endl;
    auto sp1 = sp.where([](int x) {
        if (x % 2 == 1) {
            return x;
        }
        return 0;
    });
    int z = sp.reduce(0, [](int s, int k) { return s + k; });
    std::cout << z << std::endl;
    avl_tree<int> sp2 = sp.map<int>([](int k){return k * 2;});
    auto e = sp.copy(8);
    std::string s = sp.to_string();
    std::cout << s;
    std::string ss = "1 2 3";
    auto r = from_string(ss);
    return 0;
}