#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <algorithm>
#include <cctype>

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/event.hpp"

#include "avl_tree.h"

using namespace ftxui;

enum class TreeType { INT, DOUBLE, STRING };

// Безопасная функция для преобразования в строку любого типа (вместо старого хака)
template <typename T>
std::string stringify(const T& val) {
    if constexpr (std::is_same_v<T, std::string>) {
        return val;
    } else {
        return std::to_string(val);
    }
}

int main() {
    // Деревья теперь имеют два шаблонных параметра <Ключ, Значение>
    std::variant<std::monostate, avl_tree<int, int>, avl_tree<double, double>, avl_tree<std::string, std::string>> tree_variant;
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

    int menu_selected = 0;
    int prev_menu_selected = -1;
    std::vector<std::string> menu_entries = {
        "1. Вставить (add)",
        "2. Удалить (remove)",
        "3. Найти (find)",
        "4. Преобразовать (map)",
        "5. Отфильтровать (where)",
        "6. Свернуть (reduce)",
        "7. Поддерево (copy)",
        "8. Сменить тип / Сброс",
        "9. Выход"
    };

    int func_selected = 0;
    std::vector<std::string> func_entries;

    std::string input_value = "";
    std::string status_message = "Дерево создано. Выберите операцию.";

    auto update_func_entries = [&]() {
        func_entries.clear();
        func_selected = 0;

        if (menu_selected == 3) { // map
            if (current_type == TreeType::INT) {
                func_entries = {"x * 2 (Умножить на 2)", "x + 10 (Прибавить 10)", "x * x (Квадрат)", "|x| (Модуль)"};
            } else if (current_type == TreeType::DOUBLE) {
                func_entries = {"x * 2.0 (Умножить на 2)", "x / 2.0 (Разделить на 2)", "x + 1.5 (Прибавить 1.5)", "x * x (Квадрат)"};
            } else if (current_type == TreeType::STRING) {
                func_entries = {"ВЕРХНИЙ РЕГИСТР", "нижний регистр", "Добавить '!' в конец", "Реверс строки"};
            }
        } else if (menu_selected == 4) { // where
            if (current_type == TreeType::INT) {
                func_entries = {"Четные ключи (k % 2 == 0)", "Нечетные ключи", "Положительные (k > 0)", "Отрицательные (k < 0)"};
            } else if (current_type == TreeType::DOUBLE) {
                func_entries = {"Положительные (k > 0.0)", "Отрицательные (k < 0.0)", "Больше 10.0 (k > 10.0)", "Целая часть четная"};
            } else if (current_type == TreeType::STRING) {
                func_entries = {"Длина > 3 символов", "Длина <= 3 символов", "Начинается с заглавной", "Не пустые"};
            }
        } else if (menu_selected == 5) { // reduce
            if (current_type == TreeType::INT || current_type == TreeType::DOUBLE) {
                func_entries = {"Сумма ключей", "Произведение ключей", "Количество узлов"};
            } else if (current_type == TreeType::STRING) {
                func_entries = {"Конкатенация через пробел", "Конкатенация слитно", "Суммарная длина строк"};
            }
        }
    };

    auto type_menu = Menu(&type_entries, &type_menu_selected);
    auto type_submit = Button("Выбрать", [&] {
        if (type_menu_selected == 0) {
            current_type = TreeType::INT;
            tree_variant = avl_tree<int, int>();
        } else if (type_menu_selected == 1) {
            current_type = TreeType::DOUBLE;
            tree_variant = avl_tree<double, double>();
        } else if (type_menu_selected == 2) {
            current_type = TreeType::STRING;
            tree_variant = avl_tree<std::string, std::string>();
        }
        status_message = "Дерево создано. Введите значение или выберите операцию.";
        menu_selected = 0;
        prev_menu_selected = -1;
        update_func_entries();
        active_tab = 1;
    });

    auto type_container = Container::Vertical({ type_menu, type_submit });

    auto menu = Menu(&menu_entries, &menu_selected);
    auto func_menu = Radiobox(&func_entries, &func_selected);

    InputOption input_option;
    Component input = Input(&input_value, "Ввод (будет и ключом, и значением)...", input_option);

    auto needs_input = [&]() {
        return menu_selected == 0 || menu_selected == 1 || menu_selected == 2 || menu_selected == 6;
    };
    auto needs_func = [&]() {
        return menu_selected == 3 || menu_selected == 4 || menu_selected == 5;
    };

    auto submit_button = Button("Выполнить", [&] {
        if (menu_selected == 8) { screen.Exit(); return; }
        if (menu_selected == 7) { active_tab = 0; return; }
        if (needs_input() && input_value.empty()) { status_message = "Ошибка: Пустой ввод!"; return; }

        try {
            if (current_type == TreeType::INT) {
                if (auto* t = std::get_if<avl_tree<int, int>>(&tree_variant)) {
                    if (needs_input()) {
                        int val = std::stoi(input_value);
                        if (menu_selected == 0) { t->add(val, val); status_message = "Узел [" + std::to_string(val) + ":" + std::to_string(val) + "] добавлен."; }
                        else if (menu_selected == 1) { t->remove(val); status_message = "Узел удален (если существовал)."; }
                        else if (menu_selected == 2) { status_message = t->find(val) ? "Результат: НАЙДЕН" : "Результат: НЕ НАЙДЕН"; }
                        else if (menu_selected == 6) {
                            *t = t->copy(val);
                            status_message = "Текущее дерево заменено на поддерево с корнем " + std::to_string(val);
                        }
                    } else if (menu_selected == 3) {
                        if (func_selected == 0) *t = t->map<int, int>([](int k, int v) { return std::make_pair(k * 2, v * 2); });
                        else if (func_selected == 1) *t = t->map<int, int>([](int k, int v) { return std::make_pair(k + 10, v + 10); });
                        else if (func_selected == 2) *t = t->map<int, int>([](int k, int v) { return std::make_pair(k * k, v * v); });
                        else if (func_selected == 3) *t = t->map<int, int>([](int k, int v) { return std::make_pair(std::abs(k), std::abs(v)); });
                        status_message = "Операция map выполнена.";
                    } else if (menu_selected == 4) {
                        if (func_selected == 0) *t = t->where([](int k, int v) { return k % 2 == 0; });
                        else if (func_selected == 1) *t = t->where([](int k, int v) { return k % 2 != 0; });
                        else if (func_selected == 2) *t = t->where([](int k, int v) { return k > 0; });
                        else if (func_selected == 3) *t = t->where([](int k, int v) { return k < 0; });
                        status_message = "Операция where выполнена.";
                    } else if (menu_selected == 5) {
                        bool is_empty = true;
                        t->for_each([&](int, int) { is_empty = false; });
                        if (is_empty) {
                            status_message = "Ошибка: Дерево пустое!";
                        } else {
                            if (func_selected == 0) {
                                int res = t->reduce(0, [](int acc, int k, int v) { return acc + k; });
                                status_message = "Результат reduce (Сумма ключей): " + std::to_string(res);
                            } else if (func_selected == 1) {
                                int res = t->reduce(1, [](int acc, int k, int v) { return acc * k; });
                                status_message = "Результат reduce (Произведение ключей): " + std::to_string(res);
                            } else if (func_selected == 2) {
                                int res = t->reduce(0, [](int acc, int k, int v) { return acc + 1; });
                                status_message = "Результат reduce (Количество): " + std::to_string(res);
                            }
                        }
                    }
                }
            }
            else if (current_type == TreeType::DOUBLE) {
                if (auto* t = std::get_if<avl_tree<double, double>>(&tree_variant)) {
                    if (needs_input()) {
                        double val = std::stod(input_value);
                        if (menu_selected == 0) { t->add(val, val); status_message = "Узел добавлен."; }
                        else if (menu_selected == 1) { t->remove(val); status_message = "Узел удален."; }
                        else if (menu_selected == 2) { status_message = t->find(val) ? "Результат: НАЙДЕН" : "Результат: НЕ НАЙДЕН"; }
                        else if (menu_selected == 6) {
                            *t = t->copy(val);
                            status_message = "Дерево заменено на поддерево с корнем " + std::to_string(val);
                        }
                    } else if (menu_selected == 3) {
                        if (func_selected == 0) *t = t->map<double, double>([](double k, double v) { return std::make_pair(k * 2.0, v * 2.0); });
                        else if (func_selected == 1) *t = t->map<double, double>([](double k, double v) { return std::make_pair(k / 2.0, v / 2.0); });
                        else if (func_selected == 2) *t = t->map<double, double>([](double k, double v) { return std::make_pair(k + 1.5, v + 1.5); });
                        else if (func_selected == 3) *t = t->map<double, double>([](double k, double v) { return std::make_pair(k * k, v * v); });
                        status_message = "Операция map выполнена.";
                    } else if (menu_selected == 4) {
                        if (func_selected == 0) *t = t->where([](double k, double v) { return k > 0.0; });
                        else if (func_selected == 1) *t = t->where([](double k, double v) { return k < 0.0; });
                        else if (func_selected == 2) *t = t->where([](double k, double v) { return k > 10.0; });
                        else if (func_selected == 3) *t = t->where([](double k, double v) { return static_cast<long long>(k) % 2 == 0; });
                        status_message = "Операция where выполнена.";
                    } else if (menu_selected == 5) {
                        bool is_empty = true;
                        t->for_each([&](double, double) { is_empty = false; });
                        if (is_empty) {
                            status_message = "Ошибка: Дерево пустое!";
                        } else {
                            if (func_selected == 0) {
                                double res = t->reduce(0.0, [](double acc, double k, double v) { return acc + k; });
                                status_message = "Результат reduce (Сумма ключей): " + std::to_string(res);
                            } else if (func_selected == 1) {
                                double res = t->reduce(1.0, [](double acc, double k, double v) { return acc * k; });
                                status_message = "Результат reduce (Произведение ключей): " + std::to_string(res);
                            } else if (func_selected == 2) {
                                int res = t->reduce(0, [](int acc, double k, double v) { return acc + 1; });
                                status_message = "Результат reduce (Количество): " + std::to_string(res);
                            }
                        }
                    }
                }
            }
            else if (current_type == TreeType::STRING) {
                if (auto* t = std::get_if<avl_tree<std::string, std::string>>(&tree_variant)) {
                    if (needs_input()) {
                        if (menu_selected == 0) { t->add(input_value, input_value); status_message = "Узел добавлен."; }
                        else if (menu_selected == 1) { t->remove(input_value); status_message = "Узел удален."; }
                        else if (menu_selected == 2) { status_message = t->find(input_value) ? "Результат: НАЙДЕН" : "Результат: НЕ НАЙДЕН"; }
                        else if (menu_selected == 6) {
                            *t = t->copy(input_value);
                            status_message = "Дерево заменено на поддерево с корнем \"" + input_value + "\"";
                        }
                    } else if (menu_selected == 3) {
                        if (func_selected == 0) {
                            *t = t->map<std::string, std::string>([](std::string k, std::string v) {
                                std::transform(k.begin(), k.end(), k.begin(), ::toupper);
                                std::transform(v.begin(), v.end(), v.begin(), ::toupper);
                                return std::make_pair(k, v);
                            });
                        } else if (func_selected == 1) {
                            *t = t->map<std::string, std::string>([](std::string k, std::string v) {
                                std::transform(k.begin(), k.end(), k.begin(), ::tolower);
                                std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                                return std::make_pair(k, v);
                            });
                        } else if (func_selected == 2) {
                            *t = t->map<std::string, std::string>([](std::string k, std::string v) { return std::make_pair(k + "!", v + "!"); });
                        } else if (func_selected == 3) {
                            *t = t->map<std::string, std::string>([](std::string k, std::string v) {
                                std::reverse(k.begin(), k.end());
                                std::reverse(v.begin(), v.end());
                                return std::make_pair(k, v);
                            });
                        }
                        status_message = "Операция map выполнена.";
                    } else if (menu_selected == 4) {
                        if (func_selected == 0) *t = t->where([](const std::string& k, const std::string& v) { return k.length() > 3; });
                        else if (func_selected == 1) *t = t->where([](const std::string& k, const std::string& v) { return k.length() <= 3; });
                        else if (func_selected == 2) *t = t->where([](const std::string& k, const std::string& v) { return !k.empty() && std::isupper(static_cast<unsigned char>(k[0])); });
                        else if (func_selected == 3) *t = t->where([](const std::string& k, const std::string& v) { return !k.empty(); });
                        status_message = "Операция where выполнена.";
                    } else if (menu_selected == 5) {
                        bool is_empty = true;
                        t->for_each([&](const std::string&, const std::string&) { is_empty = false; });
                        if (is_empty) {
                            status_message = "Ошибка: Дерево пустое!";
                        } else {
                            if (func_selected == 0) {
                                std::string res = t->reduce(std::string(""), [](std::string acc, const std::string& k, const std::string& v) {
                                    return acc.empty() ? k : acc + " " + k;
                                });
                                status_message = "Результат reduce: " + res;
                            } else if (func_selected == 1) {
                                std::string res = t->reduce(std::string(""), [](std::string acc, const std::string& k, const std::string& v) {
                                    return acc + k;
                                });
                                status_message = "Результат reduce: " + res;
                            } else if (func_selected == 2) {
                                size_t res = t->reduce(size_t(0), [](size_t acc, const std::string& k, const std::string& v) {
                                    return acc + k.length();
                                });
                                status_message = "Результат (Суммарная длина ключей): " + std::to_string(res);
                            }
                        }
                    }
                }
            }
        } catch (...) {
            status_message = "Ошибка: Неверный формат данных!";
        }
        input_value.clear();
    });

    auto input_maybe = Maybe(input, needs_input);
    auto func_maybe = Maybe(func_menu, needs_func);

    auto main_container = Container::Horizontal({
        menu,
        Container::Vertical({
            input_maybe,
            func_maybe,
            submit_button
        })
    });

    auto global_container = Container::Tab({ type_container, main_container }, &active_tab);

    auto renderer = Renderer(global_container, [&] {
        if (menu_selected != prev_menu_selected) {
            prev_menu_selected = menu_selected;
            update_func_entries();
        }

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

        // Обход дерева вручную для вывода (метод to_string() больше не нужен)
        if (auto* t = std::get_if<avl_tree<int, int>>(&tree_variant)) {
            t->for_each([&](int k, int v) { tree_content += "[" + stringify(k) + ":" + stringify(v) + "] "; });
            type_str = "INT";
        } else if (auto* t = std::get_if<avl_tree<double, double>>(&tree_variant)) {
            t->for_each([&](double k, double v) { tree_content += "[" + stringify(k) + ":" + stringify(v) + "] "; });
            type_str = "DOUBLE";
        } else if (auto* t = std::get_if<avl_tree<std::string, std::string>>(&tree_variant)) {
            t->for_each([&](const std::string& k, const std::string& v) { tree_content += "[" + stringify(k) + ":" + stringify(v) + "] "; });
            type_str = "STRING";
        } else {
            tree_content = "{ Внутренняя ошибка типа }";
            type_str = "UNKNOWN";
        }

        if (tree_content.empty()) {
            tree_content = "{ Дерево пустое }";
        } else {
            tree_content.pop_back(); // Удаляем лишний пробел в конце
        }

        Element control_area;
        if (needs_input()) {
            control_area = hbox({
                text(" Значение: ") | center,
                input->Render() | size(WIDTH, EQUAL, 25) | border,
                submit_button->Render() | border
            });
        } else if (needs_func()) {
            control_area = vbox({
                text(" Выберите функцию преобразования / условия:") | bold,
                func_menu->Render() | border,
                submit_button->Render() | border | align_right
            });
        } else {
            control_area = hbox({
                text(" Подтвердите действие: ") | center,
                submit_button->Render() | border
            });
        }

        return hbox({
            vbox({
                text(" ОПЕРАЦИИ ") | bold | center | color(Color::Cyan),
                separator(),
                menu->Render(),
            }) | border | size(WIDTH, EQUAL, 30),

            vbox({
                text(" ПАНЕЛЬ УПРАВЛЕНИЯ ") | bold | center | color(Color::Cyan),
                separator(),
                control_area,
                separator(),
                text(" Тип дерева: " + type_str) | bold | color(Color::Magenta),
                text(" Статус: " + status_message) | color(Color::Yellow),
                separator(),
                text(" СОДЕРЖИМОЕ ДЕРЕВА [Ключ:Значение]: ") | bold,
                text("") | size(HEIGHT, EQUAL, 1),
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
