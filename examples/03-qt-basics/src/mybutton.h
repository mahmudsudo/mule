#pragma once
#include <QPushButton>
#include <iostream>

class MyButton : public QPushButton {
    Q_OBJECT
public:
    MyButton(const QString &text, QWidget *parent = nullptr) : QPushButton(text, parent) {
        connect(this, &QPushButton::clicked, this, &MyButton::onClicked);
    }

private slots:
    void onClicked() {
        std::cout << "Button clicked!" << std::endl;
    }
};
