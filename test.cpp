#include <gtest/gtest.h>
#include <vector>
#include "person.h"
#include "histogram.h"

struct GroupStats {
    int count = 0;
};

unsigned int extract_group(const Person& p) {
    return p.get_group();
}

void update_stat(GroupStats& stats, const Person& p) {
    stats.count++;
}

TEST(HistogramTest, BasicGrouping) {
    std::vector<Person> students = {
        Person("Иван", "Иванов", 101),
        Person("Петр", "Петров", 101),
        Person("Анна", "Смирнова", 102)
    };

    histogram<unsigned int, GroupStats> hist;

    hist.build(students, extract_group, update_stat, GroupStats{0});

    auto& tree = hist.get_tree();

    EXPECT_TRUE(tree.find(101));
    EXPECT_TRUE(tree.find(102));

    EXPECT_EQ(tree.get(101).count, 2);
    EXPECT_EQ(tree.get(102).count, 1);
}

TEST(HistogramTest, EmptyDataHandling) {
    std::vector<Person> empty_students;
    histogram<unsigned int, GroupStats> hist;

    hist.build(empty_students, extract_group, update_stat, GroupStats{0});
    auto& tree = hist.get_tree();

    EXPECT_FALSE(tree.find(101));
}

TEST(HistogramTest, MissingKeyThrowsException) {
    std::vector<Person> students = {
        Person("Олег", "Тиньков", 512)
    };

    histogram<unsigned int, GroupStats> hist;
    hist.build(students, extract_group, update_stat, GroupStats{0});
    auto& tree = hist.get_tree();

    EXPECT_THROW(tree.get(999), std::runtime_error);
}
