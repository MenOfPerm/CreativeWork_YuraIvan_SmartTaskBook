#ifndef SMARTTASKBOOK_H
#define SMARTTASKBOOK_H

#include "task.h"
#include <QVector>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

class SmartTaskBook : public QObject {
    Q_OBJECT
public:
    SmartTaskBook(QObject *parent = nullptr);
    void addTask(const QString& description, const QString& dueDate, int priority, const QString& note);
    void completeTask(int index);
    void deleteTask(int index);
    QVector<Task> getTasks() const;
    bool isTaskOverdue(const Task& task) const;
    QString getSimpleAdvice(const Task& task) const;
    void getAIAdvice(const Task& task, std::function<void(const QString&)> callback) const;

private slots:
    void onAIAdviceReply(QNetworkReply *reply);

private:
    QVector<Task> tasks;
    QString filename = "tasks.txt";
    QNetworkAccessManager *networkManager;
    void saveTasks();
    void loadTasks();
    int calculateDueDays(const QString& dueDate) const;

    // Временное хранилище для callback'а
    mutable std::function<void(const QString&)> currentCallback;
};

#endif // SMARTTASKBOOK_H
