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
    // 1. glfw: initialize and configure
    // ------------------------------
    assert(glfwInit());
    // set opengl version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // set opengl to Core-profile(not Immediate-mode)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // NOTE: MacOS need this line
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);


    // 2. glfw: window creation
    //    opengl: let window become current opengl context
    /// -------------------------------------------
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    assert(window);
    // NOTICE: opengl API must be called after this line
    glfwMakeContextCurrent(window);
    std::cout << "version: " << (const char*)glGetString(GL_VERSION) << std::endl; // simplest opengl api


    // 3. Set callback winsize change: If windows size change `glViewport` reset it
    /// -------------------------------------------
    void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    // 4. render loop
    // -----------
    while(!glfwWindowShouldClose(window))
    {
        // input
        void processInput(GLFWwindow *window);
        processInput(window);

        // 5. put you render cmd here
        {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate(); // ASAN will compliant, let it go
    return 0;
}


// On ESC Pressed
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

