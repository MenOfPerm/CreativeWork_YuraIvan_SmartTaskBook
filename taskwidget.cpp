#include "taskwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QDebug>
#include <QDateTime>

class TaskItemWidget : public QWidget {
public:
    TaskItemWidget(const QString &text, bool overdue, bool completed, int priority, QWidget *parent = nullptr)
        : QWidget(parent) {
        QHBoxLayout *layout = new QHBoxLayout(this);

        // Цветной квадрат
        QLabel *colorSquare = new QLabel(this);
        colorSquare->setFixedSize(20, 20);
        QPixmap pixmap(20, 20);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);

        qDebug() << "Task status - Overdue:" << overdue << "Completed:" << completed << "Priority:" << priority;
        if (completed) {
            // Выполненные задачи — зеленые
            painter.setBrush(QColor(0, 255, 0)); // Ярко-зеленый
        } else if (overdue) {
            // Просроченные (невыполненные) задачи — красные
            painter.setBrush(QColor(255, 0, 0)); // Ярко-красный
        } else {
            // Не выполненные и не просроченные — синий, зависящий от приоритета
            float b = priority / 3.0f * 255; // От 85 (приоритет 1) до 255 (приоритет 3)
            painter.setBrush(QColor(0, 0, static_cast<int>(b)));
        }

        painter.drawRect(0, 0, 20, 20);
        colorSquare->setPixmap(pixmap);
        layout->addWidget(colorSquare);

        // Текст задачи
        QLabel *label = new QLabel(text, this);
        layout->addWidget(label);

        setLayout(layout);
    }
};

TaskWidget::TaskWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Поля ввода
    layout->addWidget(new QLabel("Управление задачами", this));
    descriptionInput = new QLineEdit(this);
    descriptionInput->setPlaceholderText("Описание задачи");
    layout->addWidget(descriptionInput);

    dueDateInput = new QLineEdit(this);
    dueDateInput->setPlaceholderText("Срок (ДД.ММ.ГГГГ или ДД.ММ.ГГГГ ЧЧ:ММ)");
    layout->addWidget(dueDateInput);

    priorityInput = new QLineEdit(this);
    priorityInput->setPlaceholderText("Приоритет (1-3)");
    layout->addWidget(priorityInput);

    noteInput = new QLineEdit(this);
    noteInput->setPlaceholderText("Заметка");
    layout->addWidget(noteInput);

    // Кнопки
    QPushButton *addButton = new QPushButton("Добавить задачу", this);
    connect(addButton, &QPushButton::clicked, this, &TaskWidget::onAddTask);

    QPushButton *completeButton = new QPushButton("Отметить выполненной", this);
    connect(completeButton, &QPushButton::clicked, this, &TaskWidget::onCompleteTask);

    QPushButton *deleteButton = new QPushButton("Удалить задачу", this);
    connect(deleteButton, &QPushButton::clicked, this, &TaskWidget::onDeleteTask);

    QPushButton *adviceButton = new QPushButton("Получить совет", this);
    connect(adviceButton, &QPushButton::clicked, this, &TaskWidget::onGetAdvice);

    QPushButton *noteButton = new QPushButton("Показать заметку", this);
    connect(noteButton, &QPushButton::clicked, this, &TaskWidget::onShowNote);

    // Применяем стиль для всех кнопок
    QString buttonStyle = R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 5px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
    )";
    addButton->setStyleSheet(buttonStyle);
    completeButton->setStyleSheet(buttonStyle);
    deleteButton->setStyleSheet(buttonStyle);
    adviceButton->setStyleSheet(buttonStyle);
    noteButton->setStyleSheet(buttonStyle);

    layout->addWidget(addButton);
    layout->addWidget(completeButton);
    layout->addWidget(deleteButton);
    layout->addWidget(adviceButton);
    layout->addWidget(noteButton);

    // Список задач
    taskList = new QListWidget(this);
    taskList->setMinimumHeight(300); // Увеличиваем высоту списка
    layout->addWidget(taskList);

    setLayout(layout);
    updateTaskList();
}

void TaskWidget::updateTaskList() {
    taskList->clear();
    QVector<Task> tasks = taskBook.getTasks();
    for (int i = 0; i < tasks.size(); ++i) {
        bool overdue = taskBook.isTaskOverdue(tasks[i]);
        qDebug() << "Task" << i << "Overdue status:" << overdue;
        QString notePreview = tasks[i].note.left(20); // Показываем первые 20 символов заметки
        if (tasks[i].note.length() > 20) notePreview += "...";
        QString itemText = QString("%1 | Срок: %2 | Статус: %3 | Приоритет: %4 | Заметка: %5%6")
                               .arg(tasks[i].description)
                               .arg(tasks[i].dueDate)
                               .arg(tasks[i].isCompleted ? "Выполнено" : "Не выполнено")
                               .arg(tasks[i].priority)
                               .arg(notePreview)
                               .arg(overdue ? " | ПРОСРОЧЕНО" : "");

        // Создаем пользовательский виджет для элемента списка
        TaskItemWidget *itemWidget = new TaskItemWidget(itemText, overdue, tasks[i].isCompleted, tasks[i].priority);
        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(itemWidget->sizeHint());
        taskList->addItem(item);
        taskList->setItemWidget(item, itemWidget);
    }
}

void TaskWidget::onAddTask() {
    QString description = descriptionInput->text().trimmed();
    QString dueDate = dueDateInput->text().trimmed();
    bool ok;
    int priority = priorityInput->text().toInt(&ok);
    if (!ok || priority < 1 || priority > 3) priority = 1; // Значение по умолчанию
    QString note = noteInput->text().trimmed();

    // Проверяем, указана ли дата
    if (!description.isEmpty() && !dueDate.isEmpty()) {
        // Проверяем формат даты
        QDateTime due;
        if (dueDate.contains(" ")) {
            // Пользователь указал время, пробуем оба формата
            due = QDateTime::fromString(dueDate, "dd.MM.yyyy hh:mm");
            if (!due.isValid()) {
                due = QDateTime::fromString(dueDate, "dd.MM.yyyy  hh:mm");
            }
        } else {
            // Пользователь указал только дату, добавляем время по умолчанию (12:00)
            dueDate += " 12:00";
            due = QDateTime::fromString(dueDate, "dd.MM.yyyy hh:mm");
        }

        if (!due.isValid()) {
            QMessageBox::warning(this, "Ошибка", "Неверный формат даты! Используйте ДД.ММ.ГГГГ или ДД.ММ.ГГГГ ЧЧ:ММ (например, 12.03.2026 или 12.03.2026 14:30).");
            return;
        }

        taskBook.addTask(description, dueDate, priority, note);
        updateTaskList();
        descriptionInput->clear();
        dueDateInput->clear();
        priorityInput->clear();
        noteInput->clear();
    } else {
        QMessageBox::warning(this, "Ошибка", "Заполните описание и срок!");
    }
}

void TaskWidget::onCompleteTask() {
    int index = taskList->currentRow();
    if (index >= 0) {
        taskBook.completeTask(index);
        updateTaskList();
    } else {
        QMessageBox::warning(this, "Ошибка", "Выберите задачу!");
    }
}

void TaskWidget::onDeleteTask() {
    int index = taskList->currentRow();
    if (index >= 0) {
        taskBook.deleteTask(index);
        updateTaskList();
    } else {
        QMessageBox::warning(this, "Ошибка", "Выберите задачу!");
    }
}

void TaskWidget::onGetAdvice() {
    int index = taskList->currentRow();
    if (index >= 0) {
        QVector<Task> tasks = taskBook.getTasks();
        taskBook.getAIAdvice(tasks[index], [this, &tasks, index](const QString& advice) {
            if (advice.contains("Ошибка:") || advice.isEmpty()) {
                // Если API вернул ошибку или пустой ответ, используем локальную логику
                QString localAdvice = taskBook.getSimpleAdvice(tasks[index]);
                QMessageBox::information(this, "Совет", localAdvice + "\n(Совет от локальной логики, API недоступен)");
            } else {
                QMessageBox::information(this, "Совет от AI", advice);
            }
        });
    } else {
        QMessageBox::warning(this, "Ошибка", "Выберите задачу!");
    }
}

void TaskWidget::onShowNote() {
    int index = taskList->currentRow();
    if (index >= 0) {
        QVector<Task> tasks = taskBook.getTasks();
        QString note = tasks[index].note;
        QMessageBox::information(this, "Заметка", note.isEmpty() ? "Заметка отсутствует." : note);
    } else {
        QMessageBox::warning(this, "Ошибка", "Выберите задачу!");
    }
}
