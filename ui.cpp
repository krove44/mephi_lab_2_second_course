#include <iostream>
#include <string>
#include <vector>
#include <variant>

namespace std {
    inline string to_string(const string& s) {
        return s;
    }
}

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/event.hpp"

#include "avl_tree.h"

using namespace ftxui;

enum class TreeType { INT, DOUBLE, STRING };

int main() {
    std::variant<std::monostate, avl_tree<int>, avl_tree<double>, avl_tree<std::string>> tree_variant;
    TreeType current_type = TreeType::INT;

    int active_tab = 0;

    auto screen = ScreenInteractive::TerminalOutput();

    int type_menu_selected = 0;
    std::vector<std::string> type_entries = {
        "1. Целые числа (int)",
        "2. Дробные числа (double)",
        "3. Текст (string)"
    };
    std::string type_error_msg = "";

    auto type_menu = Menu(&type_entries, &type_menu_selected);
    auto type_submit = Button("Выбрать", [&] {
        if (type_menu_selected == 0) {
            current_type = TreeType::INT;
            tree_variant = avl_tree<int>();
            active_tab = 1;
        } else if (type_menu_selected == 1) {
            current_type = TreeType::DOUBLE;
            tree_variant = avl_tree<double>();
            active_tab = 1;
        } else if (type_menu_selected == 2) {
            current_type = TreeType::STRING;
            tree_variant = avl_tree<std::string>();
            active_tab = 1;
        }
    });

    auto type_container = Container::Vertical({
        type_menu,
        type_submit
    });

    int menu_selected = 0;
    std::vector<std::string> menu_entries = {
        "1. Вставить элемент",
        "2. Удалить элемент",
        "3. Найти элемент",
        "4. Выход"
    };

    std::string input_value = "";
    std::string status_message = "Дерево создано. Введите значение.";

    auto menu = Menu(&menu_entries, &menu_selected);
    InputOption input_option;
    Component input = Input(&input_value, "Ввод...", input_option);

    auto submit_button = Button("Выполнить", [&] {
        if (menu_selected == 3) { screen.Exit(); return; }
        if (input_value.empty()) { status_message = "Ошибка: Пустой ввод!"; return; }

        try {
            if (current_type == TreeType::INT) {
                if (auto* t = std::get_if<avl_tree<int>>(&tree_variant)) {
                    int val = std::stoi(input_value);
                    if (menu_selected == 0) t->add(val);
                    else if (menu_selected == 1) t->remove(val);
                    else if (menu_selected == 2) status_message = t->find(val) ? "НАЙДЕН" : "НЕ НАЙДЕН";
                }
            }
            else if (current_type == TreeType::DOUBLE) {
                if (auto* t = std::get_if<avl_tree<double>>(&tree_variant)) {
                    double val = std::stod(input_value);
                    if (menu_selected == 0) t->add(val);
                    else if (menu_selected == 1) t->remove(val);
                    else if (menu_selected == 2) status_message = t->find(val) ? "НАЙДЕН" : "НЕ НАЙДЕН";
                }
            }
            else if (current_type == TreeType::STRING) {
                if (auto* t = std::get_if<avl_tree<std::string>>(&tree_variant)) {
                    if (menu_selected == 0) t->add(input_value);
                    else if (menu_selected == 1) t->remove(input_value);
                    else if (menu_selected == 2) status_message = t->find(input_value) ? "НАЙДЕН" : "НЕ НАЙДЕН";
                }
            }
            if (menu_selected == 0 || menu_selected == 1) status_message = "Операция выполнена.";
        } catch (...) {
            status_message = "Ошибка: Неверный формат данных!";
        }
        input_value.clear();
    });

    auto main_container = Container::Vertical({
        menu,
        Container::Horizontal({ input, submit_button })
    });

    auto global_container = Container::Tab({
        type_container,
        main_container
    }, &active_tab);

    auto renderer = Renderer(global_container, [&] {

        if (active_tab == 0) {
            return window(text(" Настройка AVL-дерева "), vbox({
                text("Выберите тип данных, который будет храниться в дереве:"),
                separator(),
                type_menu->Render(),
                separator(),
                type_submit->Render() | center,
                text(type_error_msg) | color(Color::Red) | center
            })) | center;
        }

        std::string tree_content = "";
        std::string type_str = "";

        if (auto* t = std::get_if<avl_tree<int>>(&tree_variant)) {
            tree_content = t->to_string();
            type_str = "INT";
        } else if (auto* t = std::get_if<avl_tree<double>>(&tree_variant)) {
            tree_content = t->to_string();
            type_str = "DOUBLE";
        } else if (auto* t = std::get_if<avl_tree<std::string>>(&tree_variant)) {
            tree_content = t->to_string();
            type_str = "STRING";
        } else {
            tree_content = "{ Внутренняя ошибка типа }";
            type_str = "UNKNOWN";
        }

        if (tree_content.empty()) tree_content = "{ Дерево пустое }";

        return hbox({
            vbox({
                text(" AVL-ДЕРЕВО ") | bold | center | color(Color::Cyan), separator(),
                menu->Render(),
            }) | border | size(WIDTH, EQUAL, 25),

            vbox({
                text(" ПАНЕЛЬ УПРАВЛЕНИЯ ") | bold | center | color(Color::Cyan), separator(),
                hbox({
                    text(" Значение: ") | center,
                    input->Render() | size(WIDTH, EQUAL, 20) | border,
                    submit_button->Render() | border
                }),
                separator(),
                text(" Тип дерева: " + type_str) | bold | color(Color::Magenta),
                text(" Статус: " + status_message) | color(Color::Yellow),
                separator(),
                text(" СОДЕРЖИМОЕ ДЕРЕВА (in-order): ") | bold, text("") | size(HEIGHT, EQUAL, 1),
                paragraph(tree_content) | color(Color::Green)
            }) | border | flex,
        });
    });

    auto event_listener = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Character((char)17) || event == Event::Escape) {
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(event_listener);
    return 0;
}