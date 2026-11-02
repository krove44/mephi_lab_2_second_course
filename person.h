#pragma once
#include <string>
#include <utility>

class Person {
    std::string first_name;
    std::string second_name;
    unsigned int group;
    double score;
public:
    Person(std::string f, std::string s, unsigned int g, double sc) : first_name(std::move(f)), second_name(std::move(s)), group(g), score(sc){};

    unsigned int get_group() const {
        return group;
    }
    double get_score() const {
        return score;
    }
    std::string get_first_name() const {
        return first_name;
    }
    std::string get_second_name() const {
        return second_name;
    }


};