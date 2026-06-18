#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <functional>

int main() {

    Controller controller;
    Renderer renderer(controller.getDocument());

    controller.createCircleShape();
    controller.removeShape(0);

    return 0;
}
