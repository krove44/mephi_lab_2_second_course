#include <gtest/gtest.h>
#include <vector>
#include "person.h"
#include "histogram.h"
#include "binary_tree/avl_tree.h"
#include <list>
#include <fstream>
#include <sstream>

struct GroupStats {
    int count = 0;
};

unsigned int extract_group(const Person& p) {
    return p.get_group();
}

void update_stat(GroupStats& stats, const Person& p) {
    stats.count++;
}

TEST(HistogramTest, Vector) {
    std::vector<Person> students = {
        Person("Иван", "Иванов", 101),
        Person("Петр", "Петров", 101),
        Person("Анна", "Смирнова", 102)
    };

    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;

    hist.build(students, extract_group, update_stat, GroupStats{0});

    auto& tree = hist.get_tree();

    EXPECT_TRUE(tree.find(101));
    EXPECT_TRUE(tree.find(102));

    EXPECT_EQ(tree.get(101).count, 2);
    EXPECT_EQ(tree.get(102).count, 1);
}

TEST(HistogramTest, List) {
    std::list<Person> student_list = {
        Person("Алексей", "Петров", 513),
        Person("Анна", "Смирнова", 513)
    };

    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;
    hist.build(student_list, extract_group, update_stat, GroupStats{0});

    auto& tree = hist.get_tree();
    EXPECT_TRUE(tree.find(513));
    EXPECT_EQ(tree.get(513).count, 2);
}

TEST(HistogramTest, InitializerList) {
    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;
    hist.build({Person("Алексей", "Петров", 513), Person("Анна", "Смирнова", 513)}, extract_group, update_stat, GroupStats{0});

    auto& tree = hist.get_tree();
    EXPECT_TRUE(tree.find(513));
    EXPECT_EQ(tree.get(513).count, 2);
}

TEST(HistogramFileTest, TXT) {
    std::ifstream in_file("data.txt");
    ASSERT_TRUE(in_file.is_open());

    std::istream_iterator<unsigned int> file_begin(in_file);
    std::istream_iterator<unsigned int> file_end;

    auto extract_key = [](unsigned int val) {
        return val;
    };
    auto update_stat = [](GroupStats& stats, unsigned int) {
        stats.count++;
    };

    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;
    hist.build(file_begin, file_end, extract_key, update_stat, GroupStats{0});

    auto& tree = hist.get_tree();

    EXPECT_TRUE(tree.find(511));
    EXPECT_EQ(tree.get(511).count, 2);
    EXPECT_EQ(tree.get(513).count, 3);
}


