#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <ranges>
#include <set>
#include <string>
#include <vector>

#define class struct
#include "../avl_tree.h"
#undef class

namespace {

//доп функции

template <typename T>
std::vector<T> keys(avl_tree<T>& t) {
    std::vector<T> v;
    t.for_each([&](T& k) { v.push_back(k); });
    return v;
}

// Возвращает высоту, пересчитанную по детям, и помечает ошибку, если нарушен AVL-инвариант.
template <typename T>
int check_node(const Node<T>* n, bool& ok) {
    if (!n) return -1;
    int hl = check_node<T>(n->left.get(), ok);
    int hr = check_node<T>(n->right.get(), ok);
    if (std::abs(hl - hr) > 1) ok = false;
    if (n->height != 1 + std::max(hl, hr)) ok = false;
    return 1 + std::max(hl, hr);
}

// Дерево корректно: сбалансировано, высоты верны, ключи при обходе in-order не убывают.
template <typename T>
bool is_avl(avl_tree<T>& t) {
    bool ok = true;
    check_node<T>(t.head.get(), ok);
    auto v = keys(t);
    return ok && std::is_sorted(v.begin(), v.end());
}

template <typename T>
int tree_height(avl_tree<T>& t) {
    return t.head ? t.head->height : -1;
}

std::vector<int> range(int from, int to) {
    std::vector<int> v(to - from + 1);
    std::iota(v.begin(), v.end(), from);
    return v;
}

avl_tree<int> make_tree(const std::vector<int>& xs) {
    avl_tree<int> t;
    for (int x : xs) t.add(x);
    return t;
}

avl_tree<int> perfect7() { return make_tree({4, 2, 6, 1, 3, 5, 7}); }

void expect_root2(const std::vector<int>& order) {
    avl_tree<int> t = make_tree(order);
    ASSERT_TRUE(t.head != nullptr);
    EXPECT_EQ(t.head->key, 2);
    ASSERT_TRUE(t.head->left != nullptr);
    ASSERT_TRUE(t.head->right != nullptr);
    EXPECT_EQ(t.head->left->key, 1);
    EXPECT_EQ(t.head->right->key, 3);
    EXPECT_EQ(t.head->height, 1);
    EXPECT_EQ(t.head->left->height, 0);
    EXPECT_EQ(t.head->right->height, 0);
}

}

//базовые операции

TEST(AvlBasic, EmptyTree) {
    avl_tree<int> t;
    EXPECT_FALSE(t.find(1));
    EXPECT_TRUE(keys(t).empty());
    EXPECT_EQ(t.to_string(), "");
    EXPECT_EQ(t.reduce(42, [](int a, int k) { return a + k; }), 42);
    t.remove(5);  // удаление из пустого дерева ничего не ломает
    EXPECT_TRUE(t.head == nullptr);
}

TEST(AvlBasic, ConstructorWithKey) {
    avl_tree<int> t(7);
    EXPECT_TRUE(t.find(7));
    EXPECT_FALSE(t.find(8));
    EXPECT_EQ(tree_height(t), 0);
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlBasic, AddAndFind) {
    avl_tree<int> t = make_tree({50, 30, 70, 20, 40, 60, 80});
    for (int k : {20, 30, 40, 50, 60, 70, 80}) EXPECT_TRUE(t.find(k)) << k;
    for (int k : {0, 25, 55, 90}) EXPECT_FALSE(t.find(k)) << k;
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlBasic, InOrderIsSorted) {
    avl_tree<int> t = make_tree({5, 3, 9, 1, 4, 8, 10, 2, 7, 6});
    EXPECT_EQ(keys(t), range(1, 10));
}

TEST(AvlBasic, NegativeKeys) {
    avl_tree<int> t = make_tree({-5, 0, 5, -10, 10});
    EXPECT_EQ(keys(t), (std::vector<int>{-10, -5, 0, 5, 10}));
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlBasic, DuplicatesAreStoredAndTreeStaysBalanced) {
    avl_tree<int> t;
    t.add(5);
    t.add(5);
    t.add(5);
    EXPECT_EQ(keys(t), (std::vector<int>{5, 5, 5}));
    EXPECT_TRUE(is_avl(t));
}

// ---------- повороты ----------

TEST(AvlRotations, LeftLeft)   { expect_root2({3, 2, 1}); }
TEST(AvlRotations, RightRight) { expect_root2({1, 2, 3}); }
TEST(AvlRotations, LeftRight)  { expect_root2({3, 1, 2}); }
TEST(AvlRotations, RightLeft)  { expect_root2({1, 3, 2}); }

TEST(AvlRotations, RotationAtNonRootLevel) {
    // перекос возникает не в корне, а ниже
    avl_tree<int> t = make_tree({10, 5, 20, 30, 40});
    EXPECT_TRUE(is_avl(t));
    EXPECT_EQ(keys(t), (std::vector<int>{5, 10, 20, 30, 40}));
    EXPECT_EQ(t.head->right->key, 30);
}

//балансировка на больших данных

TEST(AvlBalance, HeightIsLogarithmicForAscendingInput) {
    for (int n : {1, 2, 3, 10, 100, 1000, 5000}) {
        avl_tree<int> t;
        for (int i = 0; i < n; ++i) t.add(i);
        EXPECT_TRUE(is_avl(t)) << "n=" << n;
        EXPECT_LE(tree_height(t), 1.45 * std::log2(n + 2)) << "n=" << n;
    }
}

TEST(AvlBalance, HeightIsLogarithmicForDescendingInput) {
    for (int n : {1, 2, 3, 10, 100, 1000, 5000}) {
        avl_tree<int> t;
        for (int i = n; i > 0; --i) t.add(i);
        EXPECT_TRUE(is_avl(t)) << "n=" << n;
        EXPECT_LE(tree_height(t), 1.45 * std::log2(n + 2)) << "n=" << n;
    }
}

TEST(AvlBalance, InvariantHoldsAfterEveryRandomInsert) {
    auto xs = range(0, 499);
    std::mt19937 rng(12345);
    std::shuffle(xs.begin(), xs.end(), rng);
    avl_tree<int> t;
    for (int x : xs) {
        t.add(x);
        ASSERT_TRUE(is_avl(t)) << "after adding " << x;
    }
    EXPECT_EQ(keys(t), range(0, 499));
}

//удаление

TEST(AvlRemove, Leaf) {
    avl_tree<int> t = perfect7();
    t.remove(1);
    EXPECT_FALSE(t.find(1));
    EXPECT_EQ(keys(t), range(2, 7));
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlRemove, NodeWithOneChild) {
    avl_tree<int> t = make_tree({2, 1, 3, 4});
    t.remove(3);  // у тройки только правый ребёнок 4
    EXPECT_EQ(keys(t), (std::vector<int>{1, 2, 4}));
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlRemove, NodeWithTwoChildrenReplacedBySuccessor) {
    avl_tree<int> t = perfect7();
    t.remove(4);  // корень с двумя детьми
    EXPECT_EQ(keys(t), (std::vector<int>{1, 2, 3, 5, 6, 7}));
    EXPECT_EQ(t.head->key, 5);
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlRemove, OnlyElementLeavesEmptyTree) {
    avl_tree<int> t(1);
    t.remove(1);
    EXPECT_TRUE(t.head == nullptr);
    EXPECT_FALSE(t.find(1));
}

TEST(AvlRemove, MissingKeyChangesNothing) {
    avl_tree<int> t = perfect7();
    t.remove(100);
    t.remove(0);
    EXPECT_EQ(keys(t), range(1, 7));
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlRemove, TriggersRebalance) {
    // 10 -> (5 -> 2), 15. После удаления 15 у корня BF = 2, нужен правый поворот.
    avl_tree<int> t = make_tree({10, 5, 15, 2});
    t.remove(15);
    ASSERT_TRUE(t.head != nullptr);
    EXPECT_EQ(t.head->key, 5);
    EXPECT_EQ(t.head->left->key, 2);
    EXPECT_EQ(t.head->right->key, 10);
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlRemove, UpdatesHeightsOnTheWayUp) {
    avl_tree<int> t = make_tree({10, 5, 15, 2});
    EXPECT_EQ(tree_height(t), 2);
    t.remove(2);
    EXPECT_EQ(tree_height(t), 1);
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlRemove, EverythingInRandomOrderKeepsInvariants) {
    const int n = 300;
    auto order = range(0, n - 1);
    std::mt19937 rng(2024);
    std::shuffle(order.begin(), order.end(), rng);
    avl_tree<int> t = make_tree(order);
    std::shuffle(order.begin(), order.end(), rng);

    int left = n;
    for (int x : order) {
        t.remove(x);
        --left;
        ASSERT_FALSE(t.find(x)) << "key " << x;
        ASSERT_TRUE(is_avl(t)) << "after removing " << x;
        ASSERT_EQ(static_cast<int>(keys(t).size()), left);
    }
    EXPECT_TRUE(t.head == nullptr);
}

TEST(AvlRandomized, MatchesStdSet) {
    std::mt19937 rng(7);
    std::uniform_int_distribution<int> key(0, 99), op(0, 1);
    avl_tree<int> t;
    std::set<int> ref;
    for (int i = 0; i < 3000; ++i) {
        int k = key(rng);
        if (op(rng) == 0) {
            if (ref.insert(k).second) t.add(k);  // без дубликатов
        } else {
            ref.erase(k);
            t.remove(k);
        }
        ASSERT_EQ(t.find(k), ref.count(k) > 0) << "step " << i;
        ASSERT_TRUE(is_avl(t)) << "step " << i;
    }
    EXPECT_EQ(keys(t), std::vector<int>(ref.begin(), ref.end()));
}

// where / map / reduce

TEST(AvlWhere, FiltersByPredicate) {
    avl_tree<int> t = make_tree(range(1, 20));
    auto even = t.where([](int k) { return k % 2 == 0; });
    std::vector<int> expected;
    for (int i = 2; i <= 20; i += 2) expected.push_back(i);
    EXPECT_EQ(keys(even), expected);
    EXPECT_TRUE(is_avl(even));
    EXPECT_EQ(keys(t), range(1, 20));  // исходное дерево не изменилось
}

TEST(AvlWhere, NothingMatches) {
    avl_tree<int> t = make_tree(range(1, 10));
    auto none = t.where([](int) { return false; });
    EXPECT_TRUE(none.head == nullptr);
}

TEST(AvlWhere, EverythingMatches) {
    avl_tree<int> t = make_tree(range(1, 10));
    auto all = t.where([](int) { return true; });
    EXPECT_EQ(keys(all), range(1, 10));
    EXPECT_TRUE(is_avl(all));
}

TEST(AvlMap, SameType) {
    avl_tree<int> t = make_tree(range(1, 5));
    auto doubled = t.map<int>([](int k) { return k * 2; });
    EXPECT_EQ(keys(doubled), (std::vector<int>{2, 4, 6, 8, 10}));
    EXPECT_TRUE(is_avl(doubled));
}

TEST(AvlMap, OrderReversingFunctionStillGivesValidTree) {
    avl_tree<int> t = make_tree(range(1, 50));
    auto neg = t.map<int>([](int k) { return -k; });
    EXPECT_TRUE(is_avl(neg));
    auto v = keys(neg);
    EXPECT_EQ(v.front(), -50);
    EXPECT_EQ(v.back(), -1);
}

TEST(AvlMap, ToAnotherType) {
    avl_tree<int> t = make_tree({1, 2, 3});
    auto halves = t.map<double>([](int k) { return k * 0.5; });
    EXPECT_EQ(keys(halves), (std::vector<double>{0.5, 1.0, 1.5}));
}

TEST(AvlMap, ToStrings) {
    avl_tree<int> t = make_tree(range(1, 12));
    auto s = t.map<std::string>([](int k) { return std::to_string(k); });
    std::vector<std::string> expected;
    for (int i = 1; i <= 12; ++i) expected.push_back(std::to_string(i));
    std::sort(expected.begin(), expected.end());  // строки сортируются лексикографически
    EXPECT_EQ(keys(s), expected);
    EXPECT_TRUE(is_avl(s));
}

TEST(AvlReduce, Sum) {
    avl_tree<int> t = make_tree(range(1, 100));
    EXPECT_EQ(t.reduce(0, [](int acc, int k) { return acc + k; }), 5050);
}

TEST(AvlReduce, Maximum) {
    avl_tree<int> t = make_tree({5, 99, -3, 42});
    EXPECT_EQ(t.reduce(std::numeric_limits<int>::min(),
                       [](int acc, int k) { return std::max(acc, k); }),
              99);
}

TEST(AvlReduce, GoesInAscendingOrder) {
    avl_tree<int> t = make_tree({3, 1, 2});
    std::string s = t.reduce(std::string{}, [](std::string acc, int k) {
        return acc + std::to_string(k) + ",";
    });
    EXPECT_EQ(s, "1,2,3,");
}

// извлечение поддерева (copy)

TEST(AvlCopy, SubtreeOfInnerNode) {
    avl_tree<int> t = perfect7();
    auto sub = t.copy(2);
    EXPECT_EQ(keys(sub), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(sub.head->key, 2);
    EXPECT_EQ(tree_height(sub), 1);
    EXPECT_TRUE(is_avl(sub));
}

TEST(AvlCopy, WholeTreeWhenKeyIsRoot) {
    avl_tree<int> t = perfect7();
    auto sub = t.copy(4);
    EXPECT_EQ(keys(sub), range(1, 7));
    EXPECT_TRUE(is_avl(sub));
}

TEST(AvlCopy, Leaf) {
    avl_tree<int> t = perfect7();
    auto sub = t.copy(7);
    EXPECT_EQ(keys(sub), (std::vector<int>{7}));
}

TEST(AvlCopy, MissingKeyGivesEmptyTree) {
    avl_tree<int> t = perfect7();
    auto sub = t.copy(100);
    EXPECT_TRUE(sub.head == nullptr);
}

TEST(AvlCopy, CopyIsIndependentFromOriginal) {
    avl_tree<int> t = perfect7();
    auto sub = t.copy(2);
    sub.add(100);
    EXPECT_FALSE(t.find(100));
    t.remove(1);
    EXPECT_TRUE(sub.find(1));
    EXPECT_EQ(keys(t), (std::vector<int>{2, 3, 4, 5, 6, 7}));
    EXPECT_TRUE(is_avl(t));
    EXPECT_TRUE(is_avl(sub));
}

// строковое представление

TEST(AvlString, ToStringIsInOrderWithTrailingSpace) {
    avl_tree<int> t = make_tree({2, 1, 3});
    EXPECT_EQ(t.to_string(), "1 2 3");
}

TEST(AvlString, FromStringBuildsTreeOfWords) {
    std::string s = "banana apple cherry";
    auto t = from_string(s);
    EXPECT_EQ(keys(t), (std::vector<std::string>{"apple", "banana", "cherry"}));
    EXPECT_TRUE(is_avl(t));
}

TEST(AvlString, FromStringSingleWord) {
    std::string s = "hello";
    auto t = from_string(s);
    EXPECT_EQ(keys(t), (std::vector<std::string>{"hello"}));
}

TEST(AvlString, RoundTripDoesNotAddEmptyElement) {
    // to_string() оставляет пробел в конце, from_string() делит по пробелу.
    avl_tree<int> t = make_tree({1, 2, 3});
    std::string s = t.to_string();
    auto back = from_string(s);
    EXPECT_EQ(keys(back), (std::vector<std::string>{"1", "2", "3"}));
}