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

#include <chrono>
#include <random>
#include <algorithm>
#include <cmath>
#include <string>

#include "person.h"
#include "histogram.h"
#include "../mephi_lab2/Sequence/ArraySequence.h"
#include "mephi_lab_1_second_course/avl_tree.h"

// ============================================================================
// БЛОК 1: КОНФИГУРАЦИЯ И КОНСТАНТЫ (CONSTEXPR)
// Все настраиваемые параметры вынесены сюда для удобного изменения в одном месте
// ============================================================================
namespace Config {
    // Размеры главного окна и панелей
    constexpr int kWindowWidth         = 1480;
    constexpr int kWindowHeight        = 860;
    constexpr int kMainMargin          = 12;
    constexpr int kMainSpacing         = 12;
    constexpr int kLeftPanelWidth      = 370;
    constexpr int kRightPanelWidth     = 360;
    constexpr int kGraphMinWidth       = 520;
    constexpr int kGraphMinHeight      = 450;

    // Ограничения таблиц
    constexpr int kStudentTableColumns = 4;
    constexpr int kStatsTableColumns   = 3;
    constexpr int kMaxTableRenderRows  = 500; // Лимит отрисовки строк для защиты от фризов UI

    // Параметры ввода группы студента
    constexpr int kGroupMin            = 100;
    constexpr int kGroupMax            = 999;
    constexpr int kGroupDefault        = 511;

    // Параметры ввода среднего балла
    constexpr double kScoreMin         = 2.0;
    constexpr double kScoreMax         = 5.0;
    constexpr double kScoreStep        = 0.1;
    constexpr int kScoreDecimals       = 2;
    constexpr double kScoreDefault     = 4.5;

    // Параметры генератора случайной выборки
    constexpr int kGenCountMin         = 1;
    constexpr int kGenCountMax         = 50000;
    constexpr int kGenCountDefault     = 30;
    constexpr double kRandomGroupMean  = 513.0;
    constexpr double kRandomGroupStd   = 2.0;
    constexpr int kRandomGroupClampMin = 510;
    constexpr int kRandomGroupClampMax = 518;
    constexpr double kRandomScoreMin   = 2.5;
    constexpr double kRandomScoreMax   = 5.0;

    // Параметры 3D-графика и камеры
    constexpr float kDefaultCameraAngle = 30.0f;
    constexpr float kRotationStepDeg    = 0.25f;
    constexpr float kFullCircleDeg      = 360.0f;
    constexpr int kRotationIntervalMs   = 30;
    constexpr float kBarThickness       = 0.8f;
    constexpr float kAxisMinScore       = 5.5f;
    constexpr float kAxisMinCount       = 5.0f;

    // Параметры таймера пошаговой симуляции
    constexpr int kSpeedSliderMinMs    = 50;
    constexpr int kSpeedSliderMaxMs    = 1500;
    constexpr int kSpeedSliderDefaultMs= 450;
    constexpr int kSpeedSliderWidth    = 90;

    // Цветовая палитра логов (HEX)
    constexpr const char* kColorDefault = "#c9d1d9";
    constexpr const char* kColorInfo    = "#58a6ff";
    constexpr const char* kColorSuccess = "#3fb950";
    constexpr const char* kColorWarning = "#d29922";
    constexpr const char* kColorError   = "#f85149";
    constexpr const char* kColorNewNode = "#d2a8ff";
    constexpr const char* kColorUpdNode = "#79c0ff";
    constexpr const char* kColorMuted   = "#8b949e";
} // namespace Config


// ============================================================================
// БЛОК 2: СТРУКТУРЫ И ФУНКЦИИ АГРЕГАЦИИ ДАННЫХ ДЛЯ ГИСТОГРАММЫ
// ============================================================================

/**
 * @brief Статистика по конкретной учебной группе (хранится как значение в АВЛ-дереве)
 */
struct GroupStats {
    unsigned int count = 0;
    double score_group = 0.0;
};

/**
 * @brief Извлекает ключ (номер группы) из объекта студента
 */
static unsigned int extract_group(const Person& p) {
    return p.get_group();
}

/**
 * @brief Обновляет агрегированную статистику группы при добавлении нового студента
 */
static void update_stat(GroupStats& stats, const Person& p) {
    stats.count++;
    stats.score_group = (stats.score_group * (stats.count - 1) + p.get_score()) / stats.count;
}

/**
 * @brief Пересчитывает статистику группы при удалении студента из выборки
 */
static void remove_stat(GroupStats& stats, const Person& p) {
    if (stats.count == 0) {
        return;
    }
    if (stats.count == 1) {
        stats.count = 0;
        stats.score_group = 0.0;
    } else {
        stats.score_group = (stats.score_group * stats.count - p.get_score()) / (stats.count - 1);
        stats.count--;
    }
}


// ============================================================================
// БЛОК 3: ГЛАВНОЕ ОКНО ПРИЛОЖЕНИЯ (UI ДАШБОРД)
// ============================================================================
class HistogramDashboard : public QMainWindow {
private:
    // --- Компоненты 3D-визуализации ---
    Q3DBars *graph               = nullptr;
    QBar3DSeries *series         = nullptr;

    // --- Таблицы и консоль логов ---
    QTextEdit *logConsole        = nullptr;
    QTableWidget *studentTable   = nullptr;
    QTableWidget *statsTable     = nullptr;

    // --- Таймеры анимации ---
    QTimer *stepTimer            = nullptr;
    QTimer *rotationTimer        = nullptr;

    // --- Элементы управления (Ввод и настройка) ---
    QLineEdit *firstNameInput    = nullptr;
    QLineEdit *lastNameInput     = nullptr;
    QSpinBox *groupInput         = nullptr;
    QDoubleSpinBox *scoreInput   = nullptr;
    QSpinBox *genCountInput      = nullptr;
    QSlider *speedSlider         = nullptr;
    QComboBox *comboMetric       = nullptr;
    QComboBox *comboMesh         = nullptr;
    QComboBox *comboTheme        = nullptr;

    // --- Кнопки действий ---
    QPushButton *btnAddManual      = nullptr;
    QPushButton *btnGenerate       = nullptr;
    QPushButton *btnFastBench      = nullptr;
    QPushButton *btnRemoveSelected = nullptr;
    QPushButton *btnClear          = nullptr;
    QPushButton *btnPlayPause      = nullptr;
    QPushButton *btnRotate         = nullptr;

    // --- Информационные карточки (верхний бар) ---
    QLabel *lblTotalStudents     = nullptr;
    QLabel *lblUniqueGroups      = nullptr;
    QLabel *lblExecTime          = nullptr;

    // --- Данные и состояние симуляции ---
    ArraySequence<Person> students; // Пользовательская последовательность вместо std::vector
    size_t currentIndex = 0;
    histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>> hist;
    float cameraAngle = Config::kDefaultCameraAngle;
    bool isRotating = false;

public:
    explicit HistogramDashboard(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupWindow();
        setupUiLayout();
        setupTimersAndSignals();

        // Загружаем стартовый набор данных и запускаем пошаговую визуализацию
        loadInitialDataset();
        stepTimer->start(speedSlider->value());
        btnPlayPause->setText("Пауза");
    }

private:
    // ========================================================================
    // ПОДБЛОК 3.1: ИНИЦИАЛИЗАЦИЯ И КОМПОНОВКА ИНТЕРФЕЙСА (UI LAYOUT)
    // ========================================================================

    /**
     * @brief Базовая настройка главного окна и применение темы оформления
     */
    void setupWindow() {
        setWindowTitle("Анализ учебных групп на базе АВЛ-дерева");
        resize(Config::kWindowWidth, Config::kWindowHeight);
        applyCleanStyle();
    }

    /**
     * @brief Сборка трех основных панелей окна (Левая, Центральная, Правая)
     */
    void setupUiLayout() {
        QWidget *central = new QWidget(this);
        QHBoxLayout *mainLayout = new QHBoxLayout(central);
        mainLayout->setContentsMargins(Config::kMainMargin, Config::kMainMargin,
                                       Config::kMainMargin, Config::kMainMargin);
        mainLayout->setSpacing(Config::kMainSpacing);

        mainLayout->addWidget(createLeftPanel());
        mainLayout->addWidget(createCenterPanel(), 1);
        mainLayout->addWidget(createRightPanel());

        setCentralWidget(central);
    }

    /**
     * @brief Создает левую панель: ручной ввод, генератор выборки и таблица очереди
     */
    QWidget* createLeftPanel() {
        QWidget *leftPanel = new QWidget();
        leftPanel->setFixedWidth(Config::kLeftPanelWidth);
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        // 1. Группа ручного ввода студента
        QGroupBox *manualBox = new QGroupBox("Ручной ввод студента");
        QVBoxLayout *manualLayout = new QVBoxLayout(manualBox);

        firstNameInput = new QLineEdit();
        firstNameInput->setPlaceholderText("Имя (например, Константин)");
        lastNameInput = new QLineEdit();
        lastNameInput->setPlaceholderText("Фамилия (например, Пономарев)");

        QHBoxLayout *grpScoreLayout = new QHBoxLayout();
        groupInput = new QSpinBox();
        groupInput->setRange(Config::kGroupMin, Config::kGroupMax);
        groupInput->setValue(Config::kGroupDefault);
        groupInput->setPrefix("Группа: ");

        scoreInput = new QDoubleSpinBox();
        scoreInput->setRange(Config::kScoreMin, Config::kScoreMax);
        scoreInput->setSingleStep(Config::kScoreStep);
        scoreInput->setDecimals(Config::kScoreDecimals);
        scoreInput->setValue(Config::kScoreDefault);
        scoreInput->setPrefix("Балл: ");

        grpScoreLayout->addWidget(groupInput);
        grpScoreLayout->addWidget(scoreInput);

        btnAddManual = new QPushButton("Добавить в список");
        manualLayout->addWidget(firstNameInput);
        manualLayout->addWidget(lastNameInput);
        manualLayout->addLayout(grpScoreLayout);
        manualLayout->addWidget(btnAddManual);

        // 2. Группа генератора случайных данных и быстрого бенчмарка
        QGroupBox *autoBox = new QGroupBox("Генерация выборки и расчет");
        QVBoxLayout *autoLayout = new QVBoxLayout(autoBox);

        genCountInput = new QSpinBox();
        genCountInput->setRange(Config::kGenCountMin, Config::kGenCountMax);
        genCountInput->setValue(Config::kGenCountDefault);
        genCountInput->setSuffix(" чел.");

        btnGenerate = new QPushButton("Сгенерировать и запустить");
        btnFastBench = new QPushButton("Мгновенный расчет всей очереди");
        btnFastBench->setObjectName("accentButton");

        autoLayout->addWidget(genCountInput);
        autoLayout->addWidget(btnGenerate);
        autoLayout->addWidget(btnFastBench);

        // 3. Таблица исходной очереди студентов и кнопки удаления/очистки
        QGroupBox *tableBox = new QGroupBox("Список студентов в очереди");
        QVBoxLayout *tableLayout = new QVBoxLayout(tableBox);

        studentTable = new QTableWidget(0, Config::kStudentTableColumns);
        studentTable->setHorizontalHeaderLabels({"Имя", "Фамилия", "Группа", "Балл"});
        studentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        studentTable->verticalHeader()->setVisible(false);
        studentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        studentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        studentTable->setSelectionMode(QAbstractItemView::SingleSelection);

        QHBoxLayout *tableBtnLayout = new QHBoxLayout();
        btnRemoveSelected = new QPushButton("Удалить выбранного");
        btnRemoveSelected->setObjectName("dangerButton");
        btnClear = new QPushButton("Очистить все");

        tableBtnLayout->addWidget(btnRemoveSelected);
        tableBtnLayout->addWidget(btnClear);

        tableLayout->addWidget(studentTable);
        tableLayout->addLayout(tableBtnLayout);

        leftLayout->addWidget(manualBox);
        leftLayout->addWidget(autoBox);
        leftLayout->addWidget(tableBox);

        return leftPanel;
    }

    /**
     * @brief Создает центральную панель: карточки метрик, 3D-сцена и панель управления видом
     */
    QWidget* createCenterPanel() {
        QWidget *centerPanel = new QWidget();
        QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
        centerLayout->setContentsMargins(0, 0, 0, 0);

        // 1. Верхний бар с карточками статистики
        QHBoxLayout *statsLayout = new QHBoxLayout();
        lblTotalStudents = createStatCard("ВСЕГО СТУДЕНТОВ", "0");
        lblUniqueGroups  = createStatCard("УНИКАЛЬНЫХ ГРУПП", "0");
        lblExecTime      = createStatCard("ВРЕМЯ ОПЕРАЦИИ", "0 мкс");
        statsLayout->addWidget(lblTotalStudents);
        statsLayout->addWidget(lblUniqueGroups);
        statsLayout->addWidget(lblExecTime);

        // 2. Инициализация 3D-графика
        graph = new Q3DBars();
        QWidget *graphContainer = QWidget::createWindowContainer(graph);
        graphContainer->setMinimumSize(Config::kGraphMinWidth, Config::kGraphMinHeight);

        graph->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetIsometricRightHigh);
        cameraAngle = graph->scene()->activeCamera()->xRotation();

        graph->setActiveTheme(new Q3DTheme(Q3DTheme::ThemeQt));
        graph->activeTheme()->setBackgroundEnabled(true);
        graph->activeTheme()->setGridEnabled(true);

        graph->setBarThickness(Config::kBarThickness);
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

        // 3. Нижняя панель управления 3D-сценой и анимацией
        QGroupBox *ctrlBox = new QGroupBox("Управление построением и видом (вращение мышью — зажатая ПКМ)");
        QHBoxLayout *ctrlLayout = new QHBoxLayout(ctrlBox);

        btnPlayPause = new QPushButton("Пошаговый пуск");
        btnRotate    = new QPushButton("Включить вращение");

        comboMetric = new QComboBox();
        comboMetric->addItems({
            "Ось Y: Количество и ср. балл (2 ряда)",
            "Ось Y: Только количество",
            "Ось Y: Только средний балл"
        });

        comboMesh = new QComboBox();
        comboMesh->addItems({"Скошенный блок", "Цилиндр", "Пирамида", "Прямоугольник"});

        comboTheme = new QComboBox();
        comboTheme->addItems({"Тема: Светлая", "Тема: Темная", "Тема: Синяя", "Тема: Классическая"});

        speedSlider = new QSlider(Qt::Horizontal);
        speedSlider->setRange(Config::kSpeedSliderMinMs, Config::kSpeedSliderMaxMs);
        speedSlider->setValue(Config::kSpeedSliderDefaultMs);
        speedSlider->setFixedWidth(Config::kSpeedSliderWidth);

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

        return centerPanel;
    }

    /**
     * @brief Создает правую панель: итоговая таблица по группам и лог операций АВЛ-дерева
     */
    QWidget* createRightPanel() {
        QWidget *rightPanel = new QWidget();
        rightPanel->setFixedWidth(Config::kRightPanelWidth);
        QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->setContentsMargins(0, 0, 0, 0);

        // 1. Сводная таблица статистики по группам
        QGroupBox *statsGroupBox = new QGroupBox("Текущая статистика по группам");
        QVBoxLayout *statsGroupLayout = new QVBoxLayout(statsGroupBox);

        statsTable = new QTableWidget(0, Config::kStatsTableColumns);
        statsTable->setHorizontalHeaderLabels({"Группа", "Студентов", "Средний балл"});
        statsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        statsTable->verticalHeader()->setVisible(false);
        statsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        statsGroupLayout->addWidget(statsTable);

        // 2. Терминал логов АВЛ-дерева
        QGroupBox *logBox = new QGroupBox("Журнал операций АВЛ-дерева");
        QVBoxLayout *logLayout = new QVBoxLayout(logBox);

        logConsole = new QTextEdit();
        logConsole->setReadOnly(true);
        logConsole->setObjectName("terminal");
        logLayout->addWidget(logConsole);

        rightLayout->addWidget(statsGroupBox, 1);
        rightLayout->addWidget(logBox, 1);

        return rightPanel;
    }

    // ========================================================================
    // ПОДБЛОК 3.2: НАСТРОЙКА ТАЙМЕРОВ, СИГНАЛОВ И СЛОТОВ
    // ========================================================================

    void setupTimersAndSignals() {
        stepTimer = new QTimer(this);
        rotationTimer = new QTimer(this);

        // Пошаговая обработка очереди
        connect(stepTimer, &QTimer::timeout, this, &HistogramDashboard::stepSimulation);

        // Плавное вращение камеры вокруг 3D-сцены
        connect(rotationTimer, &QTimer::timeout, this, [this]() {
            cameraAngle += Config::kRotationStepDeg;
            if (cameraAngle >= Config::kFullCircleDeg) {
                cameraAngle -= Config::kFullCircleDeg;
            }
            graph->scene()->activeCamera()->setXRotation(cameraAngle);
        });

        // Кнопки управления данными
        connect(btnAddManual,      &QPushButton::clicked, this, &HistogramDashboard::addManualStudent);
        connect(btnGenerate,       &QPushButton::clicked, this, &HistogramDashboard::generateRandomStudents);
        connect(btnFastBench,      &QPushButton::clicked, this, &HistogramDashboard::runInstantBenchmark);
        connect(btnRemoveSelected, &QPushButton::clicked, this, &HistogramDashboard::removeSelectedStudent);
        connect(btnClear,          &QPushButton::clicked, this, &HistogramDashboard::clearAll);

        // Управление паузой / запуском пошагового режима
        connect(btnPlayPause, &QPushButton::clicked, this, [this]() {
            if (stepTimer->isActive()) {
                stepTimer->stop();
                btnPlayPause->setText("Продолжить");
                logMsg("Пошаговое построение приостановлено.", Config::kColorWarning);
            } else {
                if (students.GetLenght() == 0) {
                    logMsg("Список студентов пуст. Добавьте или сгенерируйте данные.", Config::kColorError);
                    return;
                }
                if (currentIndex >= students.GetLenght()) {
                    currentIndex = 0;
                    hist = histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>>();
                }
                stepTimer->start(speedSlider->value());
                btnPlayPause->setText("Пауза");
                logMsg("Запуск пошагового построения гистограммы...", Config::kColorInfo);
            }
        });

        // Изменение скорости пошаговой анимации
        connect(speedSlider, &QSlider::valueChanged, this, [this](int val) {
            if (stepTimer->isActive()) {
                stepTimer->setInterval(val);
            }
        });

        // Переключение авто-вращения камеры
        connect(btnRotate, &QPushButton::clicked, this, [this]() {
            isRotating = !isRotating;
            if (isRotating) {
                cameraAngle = graph->scene()->activeCamera()->xRotation();
                rotationTimer->start(Config::kRotationIntervalMs);
                btnRotate->setText("Остановить вращение");
            } else {
                rotationTimer->stop();
                btnRotate->setText("Включить вращение");
            }
        });

        // Переключение отображаемой метрики на оси Y
        connect(comboMetric, &QComboBox::currentIndexChanged, this, [this](int) {
            refresh3DView();
        });

        // Смена формы 3D-столбцов
        connect(comboMesh, &QComboBox::currentIndexChanged, this, [this](int idx) {
            if (idx == 0)      series->setMesh(QAbstract3DSeries::MeshBevelBar);
            else if (idx == 1) series->setMesh(QAbstract3DSeries::MeshCylinder);
            else if (idx == 2) series->setMesh(QAbstract3DSeries::MeshPyramid);
            else               series->setMesh(QAbstract3DSeries::MeshBar);
        });

        // Смена цветовой темы 3D-графика
        connect(comboTheme, &QComboBox::currentIndexChanged, this, [this](int idx) {
            if (idx == 0)      graph->activeTheme()->setType(Q3DTheme::ThemeQt);
            else if (idx == 1) graph->activeTheme()->setType(Q3DTheme::ThemeIsabelle);
            else if (idx == 2) graph->activeTheme()->setType(Q3DTheme::ThemeDigia);
            else               graph->activeTheme()->setType(Q3DTheme::ThemeRetro);
        });
    }

    // ========================================================================
    // ПОДБЛОК 3.3: УПРАВЛЕНИЕ ДАННЫМИ И ОЧЕРЕДЬЮ СТУДЕНТОВ (ArraySequence)
    // ========================================================================

    /**
     * @brief Заполняет очередь демонстрационным набором студентов при старте
     */
    void loadInitialDataset() {
        ArraySequence<Person> init = {
            Person("Максим",     "Минкин",    511, 5.0),
            Person("Константин", "Пономарев", 511, 5.0),
            Person("Иван",       "Иванов",    512, 3.5),
            Person("Анна",       "Смирнова",  512, 4.5),
            Person("Олег",       "Тиньков",   512, 4.0),
            Person("Алексей",    "Петров",    513, 3.0),
            Person("Елена",      "Соколова",  511, 4.4),
            Person("Дмитрий",    "Орлов",     514, 4.8),
            Person("София",      "Волкова",   514, 4.2),
            Person("Артем",      "Лебедев",   513, 4.0)
        };

        for (size_t i = 0; i < init.GetLenght(); ++i) {
            appendStudentToQueue(init.Get(i));
        }
        logMsg("Начальный список загружен. Построение диаграммы...", Config::kColorInfo);
    }

    /**
     * @brief Добавляет студента в ArraySequence и отображает строку в таблице очереди
     */
    void appendStudentToQueue(const Person& p) {
        students.Append(p);

        int row = studentTable->rowCount();
        studentTable->insertRow(row);
        studentTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p.get_first_name())));
        studentTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.get_second_name())));
        studentTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.get_group())));
        studentTable->setItem(row, 3, new QTableWidgetItem(QString::number(p.get_score(), 'f', Config::kScoreDecimals)));

        updateStatCard(lblTotalStudents, "ВСЕГО СТУДЕНТОВ", QString::number(students.GetLenght()));
    }

    /**
     * @brief Обработчик кнопки ручного добавления студента
     */
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
               .arg(fn, ln).arg(grp).arg(sc, 0, 'f', Config::kScoreDecimals), Config::kColorSuccess);

        // Если симуляция дошла до конца и остановилась, подхватываем новый элемент
        if (!stepTimer->isActive() && currentIndex == students.GetLenght() - 1) {
            stepTimer->start(speedSlider->value());
            btnPlayPause->setText("Пауза");
        }
    }

    /**
     * @brief Удаляет выбранного в таблице студента из очереди и из гистограммы (через remove_stat)
     */
    void removeSelectedStudent() {
        int row = studentTable->currentRow();
        if (row < 0 || static_cast<size_t>(row) >= students.GetLenght()) {
            logMsg("Выберите студента в таблице слева для удаления.", Config::kColorWarning);
            return;
        }

        size_t idx = static_cast<size_t>(row);
        Person removedPerson = students.Get(idx);
        unsigned int grp = removedPerson.get_group();

        // Если этот студент уже был добавлен в гистограмму, откатываем его статистику через remove_stat
        if (idx < currentIndex) {
            auto t0 = std::chrono::high_resolution_clock::now();
            hist.remove<Person>(removedPerson, extract_group, remove_stat);
            auto t1 = std::chrono::high_resolution_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

            updateStatCard(lblExecTime, "ВРЕМЯ ОПЕРАЦИИ", QString("%1 нс").arg(ns));
            currentIndex--;

            auto& tree = hist.get_tree();
            if (tree.find(grp)) {
                const GroupStats& st = tree.get(grp);
                logMsg(QString("Удален [%1 %2 | балл %3] -> Группа %4: осталось %5 чел., ср. балл %6 (%7 нс)")
                       .arg(QString::fromStdString(removedPerson.get_first_name()),
                            QString::fromStdString(removedPerson.get_second_name()))
                       .arg(removedPerson.get_score(), 0, 'f', 1)
                       .arg(grp).arg(st.count)
                       .arg(st.score_group, 0, 'f', Config::kScoreDecimals)
                       .arg(ns), Config::kColorError);
            }
            refresh3DView();
        } else {
            logMsg(QString("Удален из очереди (еще не был в дереве): %1 %2 (Гр. %3)")
                   .arg(QString::fromStdString(removedPerson.get_first_name()),
                        QString::fromStdString(removedPerson.get_second_name()))
                   .arg(grp), Config::kColorWarning);
        }

        // Пересобираем ArraySequence без удаленного элемента
        ArraySequence<Person> updatedStudents;
        for (size_t i = 0; i < students.GetLenght(); ++i) {
            if (i != idx) {
                updatedStudents.Append(students.Get(i));
            }
        }
        students = updatedStudents;

        // Удаляем строку из визуальной таблицы и обновляем счетчик
        studentTable->removeRow(row);
        updateStatCard(lblTotalStudents, "ВСЕГО СТУДЕНТОВ", QString::number(students.GetLenght()));
    }

    /**
     * @brief Генерация случайной выборки студентов с использованием ArraySequence
     */
    void generateRandomStudents() {
        static const ArraySequence<std::string> firstNames = {
            "Александр", "Дмитрий", "Максим", "Сергей", "Андрей", "Алексей",
            "Артем", "Илья", "Мария", "Анна", "Виктория", "Екатерина"
        };
        static const ArraySequence<std::string> lastNames = {
            "Смирнов", "Иванов", "Кузнецов", "Соколов", "Попов",
            "Лебедев", "Козлов", "Новиков", "Морозов", "Петров"
        };

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> fnDist(0, firstNames.GetLenght() - 1);
        std::uniform_int_distribution<size_t> lnDist(0, lastNames.GetLenght() - 1);
        std::normal_distribution<> grpDist(Config::kRandomGroupMean, Config::kRandomGroupStd);
        std::uniform_real_distribution<> scoreDist(Config::kRandomScoreMin, Config::kRandomScoreMax);

        int count = genCountInput->value();
        studentTable->setUpdatesEnabled(false);

        for (int i = 0; i < count; ++i) {
            unsigned int grp = std::clamp(static_cast<int>(std::round(grpDist(gen))),
                                          Config::kRandomGroupClampMin,
                                          Config::kRandomGroupClampMax);
            double sc = std::round(scoreDist(gen) * 10.0) / 10.0;

            Person p(firstNames.Get(fnDist(gen)), lastNames.Get(lnDist(gen)), grp, sc);
            students.Append(p);

            // Ограничиваем число строк в QTableWidget для сохранения отзывчивости UI
            if (studentTable->rowCount() < Config::kMaxTableRenderRows) {
                int row = studentTable->rowCount();
                studentTable->insertRow(row);
                studentTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p.get_first_name())));
                studentTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.get_second_name())));
                studentTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.get_group())));
                studentTable->setItem(row, 3, new QTableWidgetItem(QString::number(p.get_score(), 'f', Config::kScoreDecimals)));
            }
        }

        studentTable->setUpdatesEnabled(true);
        updateStatCard(lblTotalStudents, "ВСЕГО СТУДЕНТОВ", QString::number(students.GetLenght()));
        logMsg(QString("Сгенерировано %1 студентов. Идет построение в реальном времени...").arg(count), Config::kColorInfo);

        if (!stepTimer->isActive()) {
            stepTimer->start(speedSlider->value());
            btnPlayPause->setText("Пауза");
        }
    }

    /**
     * @brief Полный сброс очереди, дерева, таблиц и 3D-графика
     */
    void clearAll() {
        stepTimer->stop();
        btnPlayPause->setText("Пошаговый пуск");

        students.Clear();
        currentIndex = 0;
        hist = histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>>();

        studentTable->setRowCount(0);
        statsTable->setRowCount(0);

        updateStatCard(lblTotalStudents, "ВСЕГО СТУДЕНТОВ", "0");
        updateStatCard(lblUniqueGroups,  "УНИКАЛЬНЫХ ГРУПП", "0");
        updateStatCard(lblExecTime,      "ВРЕМЯ ОПЕРАЦИИ", "0 мкс");

        series->dataProxy()->resetArray(new QBarDataArray());
        logConsole->clear();
        logMsg("Все данные очищены. Дерево сброшено.", Config::kColorWarning);
    }

    // ========================================================================
    // ПОДБЛОК 3.4: ЛОГИКА СИМУЛЯЦИИ, БЕНЧМАРКА И ОБНОВЛЕНИЯ 3D-СЦЕНЫ
    // ========================================================================

    /**
     * @brief Мгновенное построение гистограммы по всей очереди с замером времени
     */
    void runInstantBenchmark() {
        stepTimer->stop();
        btnPlayPause->setText("Пошаговый пуск");

        if (students.GetLenght() == 0) {
            logMsg("Очередь пуста. Сначала сгенерируйте или добавьте данные.", Config::kColorError);
            return;
        }

        hist = histogram<unsigned int, GroupStats, avl_tree<unsigned int, GroupStats>>();

        auto start = std::chrono::high_resolution_clock::now();
        // Передаем итераторы нашей последовательности ArraySequence
        hist.build(students.begin(), students.end(), extract_group, update_stat, GroupStats{0, 0.0});
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        currentIndex = students.GetLenght();

        updateStatCard(lblExecTime, "ВРЕМЯ ПОСТРОЕНИЯ", QString("%1 мкс").arg(duration));
        logMsg("----------------------------------", Config::kColorMuted);
        logMsg("Расчет всей выборки завершен.", Config::kColorSuccess);
        logMsg(QString("Обработано записей: %1").arg(students.GetLenght()), Config::kColorDefault);
        logMsg(QString("Затрачено времени: %1 мкс (%2 мс)").arg(duration).arg(duration / 1000.0, 0, 'f', 3), Config::kColorInfo);
        logMsg("----------------------------------", Config::kColorMuted);

        refresh3DView();
    }

    /**
     * @brief Один шаг пошаговой симуляции (обработка одного студента по таймеру)
     */
    void stepSimulation() {
        if (currentIndex >= students.GetLenght()) {
            stepTimer->stop();
            btnPlayPause->setText("Повторить сначала");
            logMsg("Обработка всех элементов очереди завершена.", Config::kColorSuccess);
            return;
        }

        if (static_cast<int>(currentIndex) < studentTable->rowCount()) {
            studentTable->selectRow(static_cast<int>(currentIndex));
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
                   .arg(p.get_score(), 0, 'f', 1).arg(grp).arg(ns), Config::kColorNewNode);
        } else {
            logMsg(QString("[%1 %2 | балл %3] -> Группа %4: %5 чел., ср. балл %6 (%7 нс)")
                   .arg(QString::fromStdString(p.get_first_name()), QString::fromStdString(p.get_second_name()))
                   .arg(p.get_score(), 0, 'f', 1).arg(grp).arg(st.count)
                   .arg(st.score_group, 0, 'f', Config::kScoreDecimals).arg(ns), Config::kColorUpdNode);
        }

        updateStatCard(lblExecTime, "ВРЕМЯ ОПЕРАЦИИ", QString("%1 нс").arg(ns));
        refresh3DView();
        currentIndex++;
    }

    /**
     * @brief Синхронизирует 3D-сцену и правую таблицу с текущим состоянием АВЛ-дерева
     */
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
            // Режим 0: Оба показателя одновременно (2 ряда в 3D: Количество и Средний балл)
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
                statsTable->setItem(r, 2, new QTableWidgetItem(QString::number(stats.score_group, 'f', Config::kScoreDecimals)));
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
            graph->valueAxis()->setRange(0.0f, std::max(Config::kAxisMinScore, static_cast<float>(maxCount) + 1.0f));
            series->setItemLabelFormat("@rowLabel (Гр. @colLabel): @valueLabel");
        } else {
            // Режим 1 или 2: Один выбранный ряд (только количество ИЛИ только средний балл)
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
                statsTable->setItem(r, 2, new QTableWidgetItem(QString::number(stats.score_group, 'f', Config::kScoreDecimals)));
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
                graph->valueAxis()->setRange(0.0f, std::max(Config::kAxisMinCount, static_cast<float>(maxCount) + 1.0f));
                series->setItemLabelFormat("Группа @colLabel: @valueLabel чел.");
            } else {
                rowCategories << "Средний балл";
                graph->rowAxis()->setLabels(rowCategories);
                graph->valueAxis()->setTitle("Средний балл");
                graph->valueAxis()->setLabelFormat("%.2f");
                graph->valueAxis()->setRange(0.0f, Config::kAxisMinScore);
                series->setItemLabelFormat("Группа @colLabel: ср. балл @valueLabel");
            }
        }

        graph->columnAxis()->setLabels(colCategories);
        series->dataProxy()->resetArray(dataSet);
        updateStatCard(lblUniqueGroups, "УНИКАЛЬНЫХ ГРУПП", QString::number(uniqueNodes));
    }

    // ========================================================================
    // ПОДБЛОК 3.5: ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ UI И СТИЛИЗАЦИЯ (QSS)
    // ========================================================================

    QLabel* createStatCard(const QString& title, const QString& val) {
        QLabel *lbl = new QLabel();
        lbl->setObjectName("statCard");
        lbl->setAlignment(Qt::AlignCenter);
        updateStatCard(lbl, title, val);
        return lbl;
    }

    void updateStatCard(QLabel* card, const QString& title, const QString& val) {
        card->setText(QString("<div style='color:#8b949e; font-size:11px;'>%1</div>"
                              "<div style='color:#58a6ff; font-size:19px; font-weight:bold; margin-top:4px;'>%2</div>")
                              .arg(title, val));
    }

    void logMsg(const QString& text, const QString& color = Config::kColorDefault) {
        logConsole->append(QString("<span style='color:%1;'>%2</span>").arg(color, text));
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
            QPushButton#dangerButton {
                background-color: #da3633;
                border-color: #f85149;
                color: #ffffff;
            }
            QPushButton#dangerButton:hover {
                background-color: #f85149;
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

// ============================================================================
// БЛОК 4: ТОЧКА ВХОДА В ПРИЛОЖЕНИЕ
// ============================================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    HistogramDashboard window;
    window.show();
    return app.exec();
}
