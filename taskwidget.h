#ifndef TASKWIDGET_H
#define TASKWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include "smarttaskbook.h"
#include <functional>

class TaskWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskWidget(QWidget *parent = nullptr);

private slots:
    void onAddTask();
    void onCompleteTask();
    void onDeleteTask();
    void onGetAdvice();
    void onShowNote();

private:
    void updateTaskList();
    SmartTaskBook taskBook;
    QListWidget *taskList;
    QLineEdit *descriptionInput;
    QLineEdit *dueDateInput;
    QLineEdit *priorityInput;
    QLineEdit *noteInput;
};

#endif // TASKWIDGET_H
