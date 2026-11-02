#include <iostream>
#include <vector>
#include <cassert>
#include "person.h"
#include "histogram.h"
#include "binary_tree/avl_tree.h"

using namespace std;

struct GroupStats {
    unsigned int count = 0;
    double score_group = 0;
};

int main() {
    vector<Person> students = {
        Person("Максим", "Минкин", 511, 5.0),
        Person("Константин", "Пономарев", 511, 5.0),
        Person("Иван", "Иванов", 512, 2.0),
        Person("Анна", "Смирнова", 512, 2.0),
        Person("Олег", "Тиньков", 512, 2.0),
        Person("Алексей", "Петров", 513, 1.0)
    };

    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;

    auto extract_key = [](const Person& p) {
        return p.get_group();
    };

    auto update_stat = [](GroupStats& stats, const Person& p) {
        stats.count++;
        stats.score_group = (stats.score_group*(stats.count-1) + p.get_score())/stats.count;
    };

    GroupStats initial_stat{0, 0};

    hist.build<Person>(students, extract_key, update_stat, initial_stat);

    const auto& tree = hist.get_tree();

    cout << "--- Результаты гистограммы ---" << endl;

    if (tree.find(511)) {
        cout << "Группа 511: " << tree.get(511).count << " чел. (Ожидается: 2)" << endl;
        assert(tree.get(511).count == 2 && tree.get(511).score_group == 5.0);
    }

    if (tree.find(512)) {
        cout << "Группа 512: " << tree.get(512).count << " чел. (Ожидается: 3)" << endl;
        assert(tree.get(512).count == 3 && tree.get(512).score_group == 2.0);
    }

    if (tree.find(513)) {
        cout << "Группа 513: " << tree.get(513).count << " чел. (Ожидается: 1)" << endl;
        assert(tree.get(513).count == 1 && tree.get(513).score_group == 1.0);
    }

    return 0;
}
