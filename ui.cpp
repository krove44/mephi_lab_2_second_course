#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QString>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>

#include <QtDataVisualization/Q3DBars>
#include <QtDataVisualization/Q3DTheme>
#include <QtDataVisualization/Q3DCamera>
#include <QtDataVisualization/QBar3DSeries>
#include <QtDataVisualization/QBarDataProxy>
#include <QtDataVisualization/QCategory3DAxis>
#include <QtDataVisualization/QValue3DAxis>

#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <cmath>

#include "person.h"
#include "histogram.h"
#include "binary_tree/avl_tree.h"

struct GroupStats {
    unsigned int count = 0;
    double score_group = 0.0;
};

static unsigned int extract_group(const Person& p) {
    return p.get_group();
}

static void update_stat(GroupStats& stats, const Person& p) {
    stats.count++;
    stats.score_group = (stats.score_group * (stats.count - 1) + p.get_score()) / stats.count;
}

class HistogramDashboard : public QMainWindow {
    Q3DBars *graph;
    QBar3DSeries *series;
    QTextEdit *logConsole;
    QTableWidget *studentTable;
    QTableWidget *statsTable;
    QTimer *stepTimer;
    QTimer *rotationTimer;

    QLineEdit *firstNameInput;
    QLineEdit *lastNameInput;
    QSpinBox *groupInput;
    QDoubleSpinBox *scoreInput;
    QSpinBox *genCountInput;
    QSlider *speedSlider;
    QComboBox *comboMetric;
    QPushButton *btnPlayPause;
    QPushButton *btnRotate;

    QLabel *lblTotalStudents;
    QLabel *lblUniqueGroups;
    QLabel *lblExecTime;

    std::vector<Person> students;
    size_t currentIndex = 0;
    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;
    float cameraAngle = 30.0f;
    bool isRotating = false;

public:
    HistogramDashboard(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Анализ учебных групп на базе АВЛ-дерева");
        resize(1480, 860);
        applyCleanStyle();

        QWidget *central = new QWidget(this);
        QHBoxLayout *mainLayout = new QHBoxLayout(central);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(12);

        // ================= ЛЕВАЯ ПАНЕЛЬ: УПРАВЛЕНИЕ И ДАННЫЕ =================
        QWidget *leftPanel = new QWidget();
        leftPanel->setFixedWidth(370);
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        // 1. Ручной ввод
        QGroupBox *manualBox = new QGroupBox("Ручной ввод студента");
        QVBoxLayout *manualLayout = new QVBoxLayout(manualBox);
        firstNameInput = new QLineEdit();
        firstNameInput->setPlaceholderText("Имя (например, Константин)");
        lastNameInput = new QLineEdit();
        lastNameInput->setPlaceholderText("Фамилия (например, Пономарев)");

        QHBoxLayout *grpScoreLayout = new QHBoxLayout();
        groupInput = new QSpinBox();
        groupInput->setRange(100, 999);
        groupInput->setValue(511);
        groupInput->setPrefix("Группа: ");

        scoreInput = new QDoubleSpinBox();
        scoreInput->setRange(2.0, 5.0);
        scoreInput->setSingleStep(0.1);
        scoreInput->setDecimals(2);
        scoreInput->setValue(4.5);
        scoreInput->setPrefix("Балл: ");

        grpScoreLayout->addWidget(groupInput);
        grpScoreLayout->addWidget(scoreInput);

        QPushButton *btnAddManual = new QPushButton("Добавить в список");
        manualLayout->addWidget(firstNameInput);
        manualLayout->addWidget(lastNameInput);
        manualLayout->addLayout(grpScoreLayout);
        manualLayout->addWidget(btnAddManual);

        // 2. Генератор и Бенчмарк
        QGroupBox *autoBox = new QGroupBox("Генерация выборки и расчет");
        QVBoxLayout *autoLayout = new QVBoxLayout(autoBox);
        genCountInput = new QSpinBox();
        genCountInput->setRange(1, 50000);
        genCountInput->setValue(30);
        genCountInput->setSuffix(" чел.");

        QPushButton *btnGenerate = new QPushButton("Сгенерировать и запустить");
        QPushButton *btnFastBench = new QPushButton("Мгновенный расчет всей очереди");
        btnFastBench->setObjectName("accentButton");

        autoLayout->addWidget(genCountInput);
        autoLayout->addWidget(btnGenerate);
        autoLayout->addWidget(btnFastBench);

        // 3. Таблица исходных данных
        QGroupBox *tableBox = new QGroupBox("Список студентов в очереди");
        QVBoxLayout *tableLayout = new QVBoxLayout(tableBox);
        studentTable = new QTableWidget(0, 4);
        studentTable->setHorizontalHeaderLabels({"Имя", "Фамилия", "Группа", "Балл"});
        studentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        studentTable->verticalHeader()->setVisible(false);
        studentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        studentTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        QPushButton *btnClear = new QPushButton("Очистить все данные");
        tableLayout->addWidget(studentTable);
        tableLayout->addWidget(btnClear);

        leftLayout->addWidget(manualBox);
        leftLayout->addWidget(autoBox);
        leftLayout->addWidget(tableBox);

        // ================= ЦЕНТРАЛЬНАЯ ПАНЕЛЬ: 3D СЦЕНА И СТАТИСТИКА =================
        QWidget *centerPanel = new QWidget();
        QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
        centerLayout->setContentsMargins(0, 0, 0, 0);

        // Верхний бар метрик
        QHBoxLayout *statsLayout = new QHBoxLayout();
        lblTotalStudents = createStatCard("ВСЕГО СТУДЕНТОВ", "0");
        lblUniqueGroups = createStatCard("УНИКАЛЬНЫХ ГРУПП", "0");
        lblExecTime = createStatCard("ВРЕМЯ ОПЕРАЦИИ", "0 мкс");
        statsLayout->addWidget(lblTotalStudents);
        statsLayout->addWidget(lblUniqueGroups);
        statsLayout->addWidget(lblExecTime);

        // 3D График (вращение зажатой правой кнопкой мыши и зум колесиком включены по умолчанию)
        graph = new Q3DBars();
        QWidget *graphContainer = QWidget::createWindowContainer(graph);
        graphContainer->setMinimumSize(520, 450);

        graph->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetIsometricRightHigh);
        cameraAngle = graph->scene()->activeCamera()->xRotation();

        graph->setActiveTheme(new Q3DTheme(Q3DTheme::ThemeQt));
        graph->activeTheme()->setBackgroundEnabled(true);
        graph->activeTheme()->setGridEnabled(true);

        graph->setBarThickness(0.8f);
        graph->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftMedium);
        graph->rowAxis()->setTitle("Поток");
        graph->columnAxis()->setTitle("Учебные группы");
        graph->columnAxis()->setTitleVisible(true);
        graph->valueAxis()->setTitle("Количество студентов");
        graph->valueAxis()->setTitleVisible(true);
        graph->valueAxis()->setLabelFormat("%d");

        series = new QBar3DSeries();
        series->setMesh(QAbstract3DSeries::MeshBevelBar);
        series->setItemLabelFormat("@rowLabel | Группа @colLabel: @valueLabel");
        graph->addSeries(series);

        // Панель управления 3D и анимацией
        QGroupBox *ctrlBox = new QGroupBox("Управление построением и видом (вращение мышью — зажатая ПКМ)");
        QHBoxLayout *ctrlLayout = new QHBoxLayout(ctrlBox);

        btnPlayPause = new QPushButton("Пошаговый пуск");
        btnRotate = new QPushButton("Включить вращение");

        comboMetric = new QComboBox();
        comboMetric->addItems({"Ось Y: Количество и ср. балл (2 ряда)", "Ось Y: Только количество", "Ось Y: Только средний балл"});

        QComboBox *comboMesh = new QComboBox();
        comboMesh->addItems({"Скошенный блок", "Цилиндр", "Пирамида", "Прямоугольник"});

        QComboBox *comboTheme = new QComboBox();
        comboTheme->addItems({"Тема: Светлая", "Тема: Темная", "Тема: Синяя", "Тема: Классическая"});

        speedSlider = new QSlider(Qt::Horizontal);
        speedSlider->setRange(50, 1500);
        speedSlider->setValue(450);
        speedSlider->setFixedWidth(90);

        ctrlLayout->addWidget(btnPlayPause);
        ctrlLayout->addWidget(new QLabel("Шаг (мс):"));
        ctrlLayout->addWidget(speedSlider);
        ctrlLayout->addWidget(btnRotate);
        ctrlLayout->addWidget(comboMetric);
        ctrlLayout->addWidget(comboMesh);
        ctrlLayout->addWidget(comboTheme);

        centerLayout->addLayout(statsLayout);
        centerLayout->addWidget(graphContainer, 1);
        centerLayout->addWidget(ctrlBox);

        // ================= ПРАВАЯ ПАНЕЛЬ: СВОДКА ПО ГРУППАМ И ЖУРНАЛ =================
        QWidget *rightPanel = new QWidget();
        rightPanel->setFixedWidth(360);
        QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->setContentsMargins(0, 0, 0, 0);

        QGroupBox *statsGroupBox = new QGroupBox("Текущая статистика по группам");
        QVBoxLayout *statsGroupLayout = new QVBoxLayout(statsGroupBox);
        statsTable = new QTableWidget(0, 3);
        statsTable->setHorizontalHeaderLabels({"Группа", "Студентов", "Средний балл"});
        statsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        statsTable->verticalHeader()->setVisible(false);
        statsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        statsGroupLayout->addWidget(statsTable);

        QGroupBox *logBox = new QGroupBox("Журнал операций АВЛ-дерева");
        QVBoxLayout *logLayout = new QVBoxLayout(logBox);
        logConsole = new QTextEdit();
        logConsole->setReadOnly(true);
        logConsole->setObjectName("terminal");
        logLayout->addWidget(logConsole);

        rightLayout->addWidget(statsGroupBox, 1);
        rightLayout->addWidget(logBox, 1);

        mainLayout->addWidget(leftPanel);
        mainLayout->addWidget(centerPanel, 1);
        mainLayout->addWidget(rightPanel);
        setCentralWidget(central);

        // ================= ТАЙМЕРЫ И СИГНАЛЫ =================
        stepTimer = new QTimer(this);
        rotationTimer = new QTimer(this);

        connect(stepTimer, &QTimer::timeout, this, &HistogramDashboard::stepSimulation);

        // Медленное вращение по кнопке
        connect(rotationTimer, &QTimer::timeout, this, [this]() {
            cameraAngle += 0.25f;
            if (cameraAngle >= 360.0f) cameraAngle -= 360.0f;
            graph->scene()->activeCamera()->setXRotation(cameraAngle);
        });

        connect(btnAddManual, &QPushButton::clicked, this, &HistogramDashboard::addManualStudent);
        connect(btnGenerate, &QPushButton::clicked, this, &HistogramDashboard::generateRandomStudents);
        connect(btnFastBench, &QPushButton::clicked, this, &HistogramDashboard::runInstantBenchmark);
        connect(btnClear, &QPushButton::clicked, this, &HistogramDashboard::clearAll);

        connect(btnPlayPause, &QPushButton::clicked, this, [this]() {
            if (stepTimer->isActive()) {
                stepTimer->stop();
                btnPlayPause->setText("Продолжить");
                logMsg("Пошаговое построение приостановлено.", "#d29922");
            } else {
                if (students.empty()) {
                    logMsg("Список студентов пуст. Добавьте или сгенерируйте данные.", "#f85149");
                    return;
                }
                if (currentIndex >= students.size()) {
                    currentIndex = 0;
                    hist = histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>>();
                }
                stepTimer->start(speedSlider->value());
                btnPlayPause->setText("Пауза");
                logMsg("Запуск пошагового построения гистограммы...", "#58a6ff");
            }
        });

        connect(speedSlider, &QSlider::valueChanged, this, [this](int val) {
            if (stepTimer->isActive()) stepTimer->setInterval(val);
        });

        connect(btnRotate, &QPushButton::clicked, this, [this]() {
            isRotating = !isRotating;
            if (isRotating) {
                // Синхронизируем угол с текущим положением камеры (если крутили мышкой)
                cameraAngle = graph->scene()->activeCamera()->xRotation();
                rotationTimer->start(30);
                btnRotate->setText("Остановить вращение");
            } else {
                rotationTimer->stop();
                btnRotate->setText("Включить вращение");
            }
        });

        connect(comboMetric, &QComboBox::currentIndexChanged, this, [this](int) {
            refresh3DView();
        });

        connect(comboMesh, &QComboBox::currentIndexChanged, this, [this](int idx) {
            if (idx == 0) series->setMesh(QAbstract3DSeries::MeshBevelBar);
            else if (idx == 1) series->setMesh(QAbstract3DSeries::MeshCylinder);
            else if (idx == 2) series->setMesh(QAbstract3DSeries::MeshPyramid);
            else series->setMesh(QAbstract3DSeries::MeshBar);
        });

        connect(comboTheme, &QComboBox::currentIndexChanged, this, [this](int idx) {
            if (idx == 0) graph->activeTheme()->setType(Q3DTheme::ThemeQt);
            else if (idx == 1) graph->activeTheme()->setType(Q3DTheme::ThemeIsabelle);
            else if (idx == 2) graph->activeTheme()->setType(Q3DTheme::ThemeDigia);
            else graph->activeTheme()->setType(Q3DTheme::ThemeRetro);
        });

        // Загружаем стартовый набор и запускаем пошаговое построение
        loadInitialDataset();
        stepTimer->start(speedSlider->value());
        btnPlayPause->setText("Пауза");
    }

private:
    QLabel* createStatCard(const QString& title, const QString& val) {
        QLabel *lbl = new QLabel(QString("<div style='color:#8b949e; font-size:11px;'>%1</div>"
                                         "<div style='color:#58a6ff; font-size:19px; font-weight:bold; margin-top:4px;'>%2</div>")
                                         .arg(title, val));
        lbl->setObjectName("statCard");
        lbl->setAlignment(Qt::AlignCenter);
        return lbl;
    }

    void updateStatCard(QLabel* card, const QString& title, const QString& val) {
        card->setText(QString("<div style='color:#8b949e; font-size:11px;'>%1</div>"
                              "<div style='color:#58a6ff; font-size:19px; font-weight:bold; margin-top:4px;'>%2</div>")
                              .arg(title, val));
    }

    void logMsg(const QString& text, const QString& color = "#c9d1d9") {
        logConsole->append(QString("<span style='color:%1;'>%2</span>").arg(color, text));
    }

    void loadInitialDataset() {
        std::vector<Person> init = {
            Person("Максим", "Минкин", 511, 5.0),
            Person("Константин", "Пономарев", 511, 5.0),
            Person("Иван", "Иванов", 512, 3.5),
            Person("Анна", "Смирнова", 512, 4.5),
            Person("Олег", "Тиньков", 512, 4.0),
            Person("Алексей", "Петров", 513, 3.0),
            Person("Елена", "Соколова", 511, 4.4),
            Person("Дмитрий", "Орлов", 514, 4.8),
            Person("София", "Волкова", 514, 4.2),
            Person("Артем", "Лебедев", 513, 4.0)
        };
        for (const auto& p : init) {
            appendStudentToQueue(p);
        }
        logMsg("Начальный список загружен. Построение диаграммы...", "#58a6ff");
    }

    void appendStudentToQueue(const Person& p) {
        students.push_back(p);
        int row = studentTable->rowCount();
        studentTable->insertRow(row);
        studentTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p.get_first_name())));
        studentTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.get_second_name())));
        studentTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.get_group())));
        studentTable->setItem(row, 3, new QTableWidgetItem(QString::number(p.get_score(), 'f', 2)));
        updateStatCard(lblTotalStudents, "ВСЕГО СТУДЕНТОВ", QString::number(students.size()));
    }

    void addManualStudent() {
        QString fn = firstNameInput->text().trimmed();
        QString ln = lastNameInput->text().trimmed();
        if (fn.isEmpty()) fn = "Студент";
        if (ln.isEmpty()) ln = "Неизвестный";
        unsigned int grp = groupInput->value();
        double sc = scoreInput->value();

        appendStudentToQueue(Person(fn.toStdString(), ln.toStdString(), grp, sc));
        firstNameInput->clear();
        lastNameInput->clear();
        logMsg(QString("Добавлен студент: %1 %2 (Гр. %3, Балл: %4)")
               .arg(fn, ln).arg(grp).arg(sc, 0, 'f', 2), "#3fb950");

        // Если таймер стоял в конце очереди, запускаем обработку нового элемента в реальном времени
        if (!stepTimer->isActive() && currentIndex == students.size() - 1) {
            stepTimer->start(speedSlider->value());
            btnPlayPause->setText("Пауза");
        }
    }

    void generateRandomStudents() {
        static const std::vector<std::string> firstNames = {
            "Александр", "Дмитрий", "Максим", "Сергей", "Андрей", "Алексей",
            "Артем", "Илья", "Мария", "Анна", "Виктория", "Екатерина"
        };
        static const std::vector<std::string> lastNames = {
            "Смирнов", "Иванов", "Кузнецов", "Соколов", "Попов",
            "Лебедев", "Козлов", "Новиков", "Морозов", "Петров"
        };

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> fnDist(0, firstNames.size() - 1);
        std::uniform_int_distribution<> lnDist(0, lastNames.size() - 1);
        std::normal_distribution<> grpDist(513.0, 2.0);
        std::uniform_real_distribution<> scoreDist(2.5, 5.0);

        int count = genCountInput->value();
        studentTable->setUpdatesEnabled(false);
        for (int i = 0; i < count; ++i) {
            unsigned int grp = std::max(510, std::min(518, (int)std::round(grpDist(gen))));
            double sc = std::round(scoreDist(gen) * 10.0) / 10.0;
            Person p(firstNames[fnDist(gen)], lastNames[lnDist(gen)], grp, sc);
            students.push_back(p);

            if (studentTable->rowCount() < 500) {
                int row = studentTable->rowCount();
                studentTable->insertRow(row);
                studentTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p.get_first_name())));
                studentTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.get_second_name())));
                studentTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.get_group())));
                studentTable->setItem(row, 3, new QTableWidgetItem(QString::number(p.get_score(), 'f', 2)));
            }
        }
        studentTable->setUpdatesEnabled(true);
        updateStatCard(lblTotalStudents, "ВСЕГО СТУДЕНТОВ", QString::number(students.size()));
        logMsg(QString("Сгенерировано %1 студентов. Идет построение в реальном времени...").arg(count), "#58a6ff");

        if (!stepTimer->isActive()) {
            stepTimer->start(speedSlider->value());
            btnPlayPause->setText("Пауза");
        }
    }

    void runInstantBenchmark() {
        stepTimer->stop();
        btnPlayPause->setText("Пошаговый пуск");

        if (students.empty()) {
            logMsg("Очередь пуста. Сначала сгенерируйте или добавьте данные.", "#f85149");
            return;
        }

        hist = histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>>();

        auto start = std::chrono::high_resolution_clock::now();
        hist.build(students, extract_group, update_stat, GroupStats{0, 0.0});
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        currentIndex = students.size();

        updateStatCard(lblExecTime, "ВРЕМЯ ПОСТРОЕНИЯ", QString("%1 мкс").arg(duration));
        logMsg("----------------------------------", "#8b949e");
        logMsg("Расчет всей выборки завершен.", "#3fb950");
        logMsg(QString("Обработано записей: %1").arg(students.size()), "#c9d1d9");
        logMsg(QString("Затрачено времени: %1 мкс (%2 мс)").arg(duration).arg(duration / 1000.0, 0, 'f', 3), "#58a6ff");
        logMsg("----------------------------------", "#8b949e");

        refresh3DView();
    }

    void clearAll() {
        stepTimer->stop();
        btnPlayPause->setText("Пошаговый пуск");
        students.clear();
        currentIndex = 0;
        hist = histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>>();
        studentTable->setRowCount(0);
        statsTable->setRowCount(0);
        updateStatCard(lblTotalStudents, "ВСЕГО СТУДЕНТОВ", "0");
        updateStatCard(lblUniqueGroups, "УНИКАЛЬНЫХ ГРУПП", "0");
        updateStatCard(lblExecTime, "ВРЕМЯ ОПЕРАЦИИ", "0 мкс");
        series->dataProxy()->resetArray(new QBarDataArray());
        logConsole->clear();
        logMsg("Все данные очищены. Дерево сброшено.", "#d29922");
    }

    void stepSimulation() {
        if (currentIndex >= students.size()) {
            stepTimer->stop();
            btnPlayPause->setText("Повторить сначала");
            logMsg("Обработка всех элементов очереди завершена.", "#3fb950");
            return;
        }

        if ((int)currentIndex < studentTable->rowCount()) {
            studentTable->selectRow((int)currentIndex);
        }

        const Person& p = students[currentIndex];
        unsigned int grp = p.get_group();

        auto t0 = std::chrono::high_resolution_clock::now();
        auto& tree = hist.get_tree();
        bool existed = tree.find(grp);
        if (!existed) {
            tree.add(grp, GroupStats{0, 0.0});
        }
        update_stat(tree.get(grp), p);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

        const GroupStats& st = tree.get(grp);

        if (!existed) {
            logMsg(QString("[%1 %2 | балл %3] -> Новая группа %4. Вставка в дерево (%5 нс)")
                   .arg(QString::fromStdString(p.get_first_name()), QString::fromStdString(p.get_second_name()))
                   .arg(p.get_score(), 0, 'f', 1).arg(grp).arg(ns), "#d2a8ff");
        } else {
            logMsg(QString("[%1 %2 | балл %3] -> Группа %4: %5 чел., ср. балл %6 (%7 нс)")
                   .arg(QString::fromStdString(p.get_first_name()), QString::fromStdString(p.get_second_name()))
                   .arg(p.get_score(), 0, 'f', 1).arg(grp).arg(st.count).arg(st.score_group, 0, 'f', 2).arg(ns), "#79c0ff");
        }

        updateStatCard(lblExecTime, "ВРЕМЯ ОПЕРАЦИИ", QString("%1 нс").arg(ns));
        refresh3DView();
        currentIndex++;
    }

    void refresh3DView() {
        auto& tree = hist.get_tree();
        QBarDataArray *dataSet = new QBarDataArray;
        QStringList colCategories;
        QStringList rowCategories;

        int uniqueNodes = 0;
        unsigned int maxCount = 0;
        int metricMode = comboMetric->currentIndex();

        statsTable->setRowCount(0);

        if (metricMode == 0) {
            // Оба показателя одновременно (2 ряда столбцов в 3D: Количество и Средний балл)
            QBarDataRow *rowCount = new QBarDataRow;
            QBarDataRow *rowScore = new QBarDataRow;

            tree.for_each([&](unsigned int& group, GroupStats& stats) {
                *rowCount << static_cast<float>(stats.count);
                *rowScore << static_cast<float>(stats.score_group);
                colCategories << QString::number(group);
                uniqueNodes++;
                if (stats.count > maxCount) maxCount = stats.count;

                int r = statsTable->rowCount();
                statsTable->insertRow(r);
                statsTable->setItem(r, 0, new QTableWidgetItem(QString::number(group)));
                statsTable->setItem(r, 1, new QTableWidgetItem(QString::number(stats.count)));
                statsTable->setItem(r, 2, new QTableWidgetItem(QString::number(stats.score_group, 'f', 2)));
            });

            if (uniqueNodes > 0) {
                dataSet->append(rowCount);
                dataSet->append(rowScore);
            } else {
                delete rowCount;
                delete rowScore;
            }

            rowCategories << "Количество (чел.)" << "Средний балл";
            graph->rowAxis()->setLabels(rowCategories);
            graph->valueAxis()->setTitle("Значение (чел. / ср. балл)");
            graph->valueAxis()->setLabelFormat("%.2f");
            graph->valueAxis()->setRange(0.0f, std::max(5.5f, static_cast<float>(maxCount) + 1.0f));
            series->setItemLabelFormat("@rowLabel (Гр. @colLabel): @valueLabel");
        } else {
            // Один выбранный ряд
            QBarDataRow *dataRow = new QBarDataRow;

            tree.for_each([&](unsigned int& group, GroupStats& stats) {
                if (metricMode == 1) {
                    *dataRow << static_cast<float>(stats.count);
                } else {
                    *dataRow << static_cast<float>(stats.score_group);
                }
                colCategories << QString::number(group);
                uniqueNodes++;
                if (stats.count > maxCount) maxCount = stats.count;

                int r = statsTable->rowCount();
                statsTable->insertRow(r);
                statsTable->setItem(r, 0, new QTableWidgetItem(QString::number(group)));
                statsTable->setItem(r, 1, new QTableWidgetItem(QString::number(stats.count)));
                statsTable->setItem(r, 2, new QTableWidgetItem(QString::number(stats.score_group, 'f', 2)));
            });

            if (uniqueNodes > 0) {
                dataSet->append(dataRow);
            } else {
                delete dataRow;
            }

            if (metricMode == 1) {
                rowCategories << "Количество";
                graph->rowAxis()->setLabels(rowCategories);
                graph->valueAxis()->setTitle("Количество студентов");
                graph->valueAxis()->setLabelFormat("%.0f");
                graph->valueAxis()->setRange(0.0f, std::max(5.0f, static_cast<float>(maxCount) + 1.0f));
                series->setItemLabelFormat("Группа @colLabel: @valueLabel чел.");
            } else {
                rowCategories << "Средний балл";
                graph->rowAxis()->setLabels(rowCategories);
                graph->valueAxis()->setTitle("Средний балл");
                graph->valueAxis()->setLabelFormat("%.2f");
                graph->valueAxis()->setRange(0.0f, 5.5f);
                series->setItemLabelFormat("Группа @colLabel: ср. балл @valueLabel");
            }
        }

        graph->columnAxis()->setLabels(colCategories);
        series->dataProxy()->resetArray(dataSet);
        updateStatCard(lblUniqueGroups, "УНИКАЛЬНЫХ ГРУПП", QString::number(uniqueNodes));
    }

    void applyCleanStyle() {
        setStyleSheet(R"(
            QMainWindow, QWidget {
                background-color: #0d1117;
                color: #c9d1d9;
                font-family: -apple-system, 'Segoe UI', Roboto, sans-serif;
                font-size: 13px;
            }
            QGroupBox {
                border: 1px solid #30363d;
                border-radius: 6px;
                margin-top: 10px;
                padding-top: 14px;
                background-color: #161b22;
                font-weight: 600;
                color: #e6edf3;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px;
            }
            QPushButton {
                background-color: #21262d;
                border: 1px solid #30363d;
                border-radius: 5px;
                padding: 6px 12px;
                color: #c9d1d9;
                font-weight: 500;
            }
            QPushButton:hover {
                background-color: #30363d;
                border-color: #8b949e;
                color: #ffffff;
            }
            QPushButton#accentButton {
                background-color: #238636;
                border-color: #2ea043;
                color: #ffffff;
            }
            QPushButton#accentButton:hover {
                background-color: #2ea043;
            }
            QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
                background-color: #0d1117;
                border: 1px solid #30363d;
                border-radius: 5px;
                padding: 5px;
                color: #e6edf3;
            }
            QTableWidget {
                background-color: #0d1117;
                border: 1px solid #30363d;
                gridline-color: #21262d;
                color: #c9d1d9;
            }
            QHeaderView::section {
                background-color: #161b22;
                color: #8b949e;
                padding: 5px;
                border: 1px solid #21262d;
                font-weight: 600;
            }
            QTextEdit#terminal {
                background-color: #090d13;
                border: 1px solid #30363d;
                border-radius: 5px;
                font-family: 'Menlo', 'Monaco', 'Courier New', monospace;
                font-size: 12px;
                padding: 6px;
            }
            QLabel#statCard {
                background-color: #161b22;
                border: 1px solid #30363d;
                border-bottom: 2px solid #58a6ff;
                border-radius: 6px;
                padding: 10px;
            }
        )");
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    HistogramDashboard window;
    window.show();
    return app.exec();
}
