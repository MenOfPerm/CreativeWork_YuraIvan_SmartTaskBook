#include "smarttaskbook.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QUrl>

SmartTaskBook::SmartTaskBook(QObject *parent) : QObject(parent) {
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &SmartTaskBook::onAIAdviceReply);
    loadTasks();
}

void SmartTaskBook::addTask(const QString& description, const QString& dueDate, int priority, const QString& note) {
    Task task;
    task.description = description;
    task.dueDate = dueDate;
    task.priority = priority;
    task.note = note;
    task.isCompleted = false;
    tasks.append(task);
    saveTasks();
}

void SmartTaskBook::completeTask(int index) {
    if (index >= 0 && index < tasks.size()) {
        tasks[index].isCompleted = true;
        saveTasks();
    }
}

void SmartTaskBook::deleteTask(int index) {
    if (index >= 0 && index < tasks.size()) {
        tasks.remove(index);
        saveTasks();
    }
}

QVector<Task> SmartTaskBook::getTasks() const {
    return tasks;
}

bool SmartTaskBook::isTaskOverdue(const Task& task) const {
    if (task.isCompleted) return false;
    QDateTime current = QDateTime::currentDateTime();
    QDateTime due;

    // Проверяем формат даты
    if (task.dueDate.contains(" ")) {
        // Полный формат с временем
        due = QDateTime::fromString(task.dueDate, "dd.MM.yyyy hh:mm");
        if (!due.isValid()) {
            due = QDateTime::fromString(task.dueDate, "dd.MM.yyyy  hh:mm");
        }
    } else {
        // Только дата, добавляем время по умолчанию (12:00)
        QString dueDateWithTime = task.dueDate + " 12:00";
        due = QDateTime::fromString(dueDateWithTime, "dd.MM.yyyy hh:mm");
    }

    if (!due.isValid()) {
        qDebug() << "Invalid due date format in isTaskOverdue:" << task.dueDate << "for task:" << task.description;
        return false;
    }

    qDebug() << "Checking overdue: Current" << current.toString("dd.MM.yyyy hh:mm")
             << "vs Due" << due.toString("dd.MM.yyyy hh:mm") << "Result:" << (current > due);
    return current > due;
}

int SmartTaskBook::calculateDueDays(const QString& dueDate) const {
    QDateTime current = QDateTime::currentDateTime();
    QDateTime due;

    // Проверяем формат даты
    if (dueDate.contains(" ")) {
        // Полный формат с временем
        due = QDateTime::fromString(dueDate, "dd.MM.yyyy hh:mm");
        if (!due.isValid()) {
            due = QDateTime::fromString(dueDate, "dd.MM.yyyy  hh:mm");
        }
    } else {
        // Только дата, добавляем время по умолчанию (12:00)
        QString dueDateWithTime = dueDate + " 12:00";
        due = QDateTime::fromString(dueDateWithTime, "dd.MM.yyyy hh:mm");
    }

    if (!due.isValid()) {
        qDebug() << "Invalid due date format in calculateDueDays:" << dueDate;
        qDebug() << "Current date:" << current.toString("dd.MM.yyyy hh:mm");
        return 0;
    }

    int days = current.daysTo(due);
    qDebug() << "Calculating days: Current" << current.toString("dd.MM.yyyy hh:mm")
             << "Due" << due.toString("dd.MM.yyyy hh:mm") << "Days left:" << days;
    return days;
}

QString SmartTaskBook::getSimpleAdvice(const Task& task) const {
    if (isTaskOverdue(task)) {
        return "Задача просрочена! Выполните её немедленно!";
    } else if (task.priority == 3) {
        return "Высокий приоритет. Выполните скоро.";
    } else if (task.priority == 2) {
        return "Средний приоритет. Планируйте выполнение.";
    } else {
        return "Низкий приоритет. Можно отложить.";
    }
}

void SmartTaskBook::getAIAdvice(const Task& task, std::function<void(const QString&)> callback) const {
    currentCallback = callback;

    // Формируем промпт для API
    int daysLeft = calculateDueDays(task.dueDate);
    QString priorityStr = (task.priority == 3) ? "высокий" : (task.priority == 2) ? "средний" : "низкий";
    QString prompt = QString(
                         "Оцени срочность выполнения задачи. Ответ должен быть коротким, строго по делу, на русском языке, только о срочности. "
                         "Учитывай: название задачи: '%1', заметка: '%2', осталось дней до дедлайна: %3, приоритет: %4.")
                         .arg(task.description)
                         .arg(task.note.isEmpty() ? "нет заметки" : task.note)
                         .arg(daysLeft)
                         .arg(priorityStr);

    // Кодируем промпт для URL
    QString encodedPrompt = QUrl::toPercentEncoding(prompt);
    QString urlString = QString("https://text.pollinations.ai/%1").arg(encodedPrompt);

    // Выводим полный промпт и закодированный URL для отладки
    qDebug() << "Original prompt:" << prompt;
    qDebug() << "Encoded URL:" << urlString;

    // Создаём и настраиваем запрос
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Отправляем GET-запрос
    qDebug() << "Sending GET request to API with URL:" << urlString;
    networkManager->get(request);
}

void SmartTaskBook::onAIAdviceReply(QNetworkReply *reply) {
    QString response;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray rawData = reply->readAll();
        qDebug() << "Raw API response:" << rawData;

        // Проверяем, является ли ответ JSON
        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("text")) {
                response = obj["text"].toString().trimmed();
            } else {
                response = "Ошибка: API вернул JSON, но без поля 'text'.";
                qDebug() << "API response does not contain 'text' field.";
            }
        } else {
            // Предполагаем, что это чистый текст
            response = QString::fromUtf8(rawData).trimmed();
        }

        // Проверяем, не пустой ли ответ
        if (response.isEmpty()) {
            response = "Ошибка: API вернул пустой ответ.";
            qDebug() << "API returned an empty response.";
        } else {
            qDebug() << "Processed AI Advice response:" << response;
        }
    } else {
        response = "Ошибка: не удалось получить совет от AI (" + reply->errorString() + ").";
        qDebug() << "AI Advice error:" << reply->errorString();
    }

    if (currentCallback) {
        currentCallback(response);
    }
    reply->deleteLater();
}

void SmartTaskBook::saveTasks() {
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const Task& task : tasks) {
            out << task.description << "|"
                << task.dueDate << "|"
                << (task.isCompleted ? "1" : "0") << "|"
                << task.priority << "|"
                << task.note << "\n";
        }
        file.close();
    }
}

void SmartTaskBook::loadTasks() {
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        tasks.clear();
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split("|");
            if (parts.size() == 5) {
                Task task;
                task.description = parts[0];
                task.dueDate = parts[1];
                task.isCompleted = (parts[2] == "1");
                task.priority = parts[3].toInt();
                task.note = parts[4];
                tasks.append(task);
            }
        }
        file.close();
    }
}
