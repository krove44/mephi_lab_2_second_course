#include <iostream>
#include <vector>
#include <cassert>
#include "person.h"
#include "histogram.h"

using namespace std;

struct GroupStats {
    int count = 0;
};

int main() {
    vector<Person> students = {
        Person("Максим", "Минкин", 511),
        Person("Константин", "Пономарев", 511),
        Person("Иван", "Иванов", 512),
        Person("Анна", "Смирнова", 512),
        Person("Олег", "Тиньков", 512),
        Person("Алексей", "Петров", 513)
    };

    histogram<unsigned int, GroupStats> hist;

    auto extract_key = [](const Person& p) {
        return p.get_group();
    };

    auto update_stat = [](GroupStats& stats, const Person& p) {
        stats.count++;
    };

    GroupStats initial_stat{0};

    hist.build<Person>(students, extract_key, update_stat, initial_stat);

    const auto& tree = hist.get_tree();

    cout << "--- Результаты гистограммы ---" << endl;

    if (tree.find(511)) {
        cout << "Группа 511: " << tree.get(511).count << " чел. (Ожидается: 2)" << endl;
        assert(tree.get(511).count == 2);
    }

    if (tree.find(512)) {
        cout << "Группа 512: " << tree.get(512).count << " чел. (Ожидается: 3)" << endl;
        assert(tree.get(512).count == 3);
    }

    if (tree.find(513)) {
        cout << "Группа 513: " << tree.get(513).count << " чел. (Ожидается: 1)" << endl;
        assert(tree.get(513).count == 1);
    }

    return 0;
}
