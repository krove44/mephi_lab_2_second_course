#include <gtest/gtest.h>
#include <vector>
#include "person.h"
#include "histogram.h"
#include "mephi_lab_1_second_course/avl_tree.h"
#include <list>
#include <fstream>
#include <sstream>

struct GroupStats {
    unsigned int count = 0;
    double score_group = 0.0;
};

unsigned int extract_group(const Person& p) {
    return p.get_group();
}

auto update_stat = [](GroupStats& stats, const Person& p) {
    stats.count++;
    stats.score_group = (stats.score_group * (stats.count - 1) + p.get_score()) / stats.count;
};

TEST(HistogramTest, Vector) {
    std::vector<Person> students = {
        Person("Иван", "Иванов", 101, 4.0),
        Person("Петр", "Петров", 101, 5.0),
        Person("Анна", "Смирнова", 102, 3.0)
    };

    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;

    hist.build(students, extract_group, update_stat, GroupStats{0, 0.0});

    auto& tree = hist.get_tree();

    EXPECT_TRUE(tree.find(101));
    EXPECT_TRUE(tree.find(102));

    EXPECT_EQ(tree.get(101).count, 2);
    EXPECT_DOUBLE_EQ(tree.get(101).score_group, 4.5);

    EXPECT_EQ(tree.get(102).count, 1);
    EXPECT_DOUBLE_EQ(tree.get(102).score_group, 3.0);
}

TEST(HistogramTest, List) {
    std::list<Person> student_list = {
        Person("Алексей", "Петров", 513, 3.5),
        Person("Анна", "Смирнова", 513, 4.5)
    };

    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;
    hist.build(student_list, extract_group, update_stat, GroupStats{0, 0.0});

    auto& tree = hist.get_tree();
    EXPECT_TRUE(tree.find(513));
    EXPECT_EQ(tree.get(513).count, 2);
    EXPECT_DOUBLE_EQ(tree.get(513).score_group, 4.0);
}

TEST(HistogramTest, InitializerList) {
    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;
    hist.build(
        {Person("Алексей", "Петров", 513, 4.0), Person("Анна", "Смирнова", 513, 5.0)},
        extract_group,
        update_stat,
        GroupStats{0, 0.0}
    );

    auto& tree = hist.get_tree();
    EXPECT_TRUE(tree.find(513));
    EXPECT_EQ(tree.get(513).count, 2);
    EXPECT_DOUBLE_EQ(tree.get(513).score_group, 4.5);
}

TEST(HistogramFileTest, TXT) {
    std::ifstream in_file("data.txt");
    ASSERT_TRUE(in_file.is_open());

    std::istream_iterator<unsigned int> file_begin(in_file);
    std::istream_iterator<unsigned int> file_end;

    auto extract_key = [](unsigned int val) {
        return val;
    };

    auto update_stat_file = [](GroupStats& stats, unsigned int) {
        stats.count++;
    };

    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;
    hist.build(file_begin, file_end, extract_key, update_stat_file, GroupStats{0, 0.0});

    auto& tree = hist.get_tree();

    if (tree.find(511)) {
        EXPECT_EQ(tree.get(511).count, 2);
        EXPECT_DOUBLE_EQ(tree.get(511).score_group, 0.0);
    }

    if (tree.find(513)) {
        EXPECT_EQ(tree.get(513).count, 3);
        EXPECT_DOUBLE_EQ(tree.get(513).score_group, 0.0);
    }
}
