#ifndef TASK_H
#define TASK_H

#include <QString>

struct Task {
    QString description;
    QString dueDate; // Формат: ДД.ММ.ГГГГ ЧЧ:ММ
    bool isCompleted;
    int priority; // 1 - низкий, 2 - средний, 3 - высокий
    QString note;
};

#endif // TASK_H