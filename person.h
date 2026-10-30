#pragma once
#include <string>
#include <utility>

class Person {
    std::string first_name;
    std::string second_name;
    unsigned int group;
public:
    Person(std::string f, std::string s, unsigned int g) : first_name(std::move(f)), second_name(std::move(s)), group(g){};
};