
#include "controller.hpp"
#include "renderer.hpp"

int main() {

    Controller controller;
    Renderer renderer(controller.getDocument());

    controller.createCircleShape();
    controller.removeShape(0);

    return 0;
}
