// q-gcc: `pkg-config glfw3 gl --cflags --libs` --
//
// Thanks to: https://discourse.glfw.org/t/compiling-a-glfw-demo-file-without-glad/1682/2
// we don't use GLAD at linux

#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <assert.h>


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    assert(glfwInit());
    // set opengl version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // set opengl to Core-profile(not Immediate-mode)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // NOTE: MacOS need this line
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);


    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    assert(window);
    glfwMakeContextCurrent(window);


    std::cout << "version: " << (const char*)glGetString(GL_VERSION) << std::endl;
    getchar();
}
