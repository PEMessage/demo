// q-gcc: `pkg-config glfw3 gl --cflags --libs` --
// Thanks to: https://discourse.glfw.org/t/compiling-a-glfw-demo-file-without-glad/1682/2
// we don't use GLAD at linux

#include <GLFW/glfw3.h>
#include <iostream>


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}
