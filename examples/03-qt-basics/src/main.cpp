#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>
#include "mybutton.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    QVBoxLayout *layout = new QVBoxLayout(&window);

    MyButton *button = new MyButton("Click Me!", &window);
    layout->addWidget(button);

    window.show();
    return app.exec();
}
