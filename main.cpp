#include <QApplication>
#include "taskwidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    TaskWidget window;
    window.setWindowTitle("Менеджер задач");
    window.resize(600, 400);
    window.show();
    return app.exec();
}