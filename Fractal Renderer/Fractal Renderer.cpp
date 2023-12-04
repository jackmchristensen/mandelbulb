#include <windows.h>

//Imgui headers for UI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "DebugCallback.h"
#include "InitShader.h"    //Functions for loading shaders from text files
#include "Camera.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#include "stb/stb_image_write.h"

void idle();

namespace window
{
   const char* const title = USER_NAME " " PROJECT_NAME; //defined in project settings
   int size[2] = {1920, 1080};
   float clear_color[4] = {0.0, 0.0, 0.0, 0.0f};
}

namespace scene
{
    Camera camera;
    float fov = 60.f;

   float angle = glm::pi<float>() / 2.0;
   float scale = 1.0f;
   
   const std::string shader_dir = "shaders/";
   const std::string vertex_shader("ray_march_vs.glsl");
   const std::string fragment_shader("ray_march_fs.glsl");

   float yaw = -90.f;
   float pitch = 0.f;

   GLuint shader = -1;

   float order = 8.f;

   int AA_levels = 1;
   float ray_scalar = 1.0;

   GLuint fbo = -1;

   bool viewDepth = false;
   float focusDist = 0.0;
   int iterations = 15;

   int color_palette = 0;

   glm::vec3 color1 = glm::vec3(0.0, 0.0, 1.0); // Blue
   glm::vec3 color2 = glm::vec3(0.0, 1.0, 1.0); // Cyan
   glm::vec3 color3 = glm::vec3(0.0, 1.0, 0.0); // Green
   glm::vec3 color4 = glm::vec3(1.0, 1.0, 0.0); // Yellow
   glm::vec3 color5 = glm::vec3(1.0, 0.0, 0.0); // Red
}

namespace mouse
{
    bool altPressed = false;

    double xlast;
    double ylast;
    double xoffset;
    double yoffset;

    float sensitivity = 0.1f;
}

namespace grid
{
    std::vector <float> points;
    unsigned int vbo = -1;
    unsigned int vao = -1;

    const int pos_loc = 0;
}

void saveOpenGLRenderToFile(int width, int height, const char* filename) {
    // Allocate memory for the pixel data
    GLubyte* pixels = new GLubyte[3 * width * height];

    // Read the pixels from the framebuffer
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Flip the image on Y axis (OpenGL's origin is bottom-left, but most image formats use top-left)
    GLubyte* flippedPixels = new GLubyte[3 * width * height];
    for (int y = 0; y < height; y++) {
        memcpy(flippedPixels + (height - 1 - y) * width * 3, pixels + y * width * 3, width * 3);
    }

    std::string newFilename = filename;
    if (scene::viewDepth) newFilename += "_depth.png";
    else newFilename += ".png";
    // Write the image to file
    stbi_write_png(newFilename.c_str(), width, height, 3, flippedPixels, width * 3);

    // Free the allocated memory
    delete[] pixels;
    delete[] flippedPixels;
}

void renderVideo(float newOrder, int frame, GLFWwindow* window)
{
    scene::viewDepth = false;
    window::clear_color[0] = 1.0;
    window::clear_color[1] = 1.0;
    window::clear_color[2] = 1.0;
    glClearColor(window::clear_color[0], window::clear_color[1], window::clear_color[2], window::clear_color[3]);

    scene::order = newOrder;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(grid::vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, grid::points.size() / 3);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //draw_gui(window);

    glfwSwapBuffers(window);

    idle();

    std::string filename = "C:\\Users\\Jack\\Documents\\mandelbulb\\video\\output_" + std::to_string(frame);
    saveOpenGLRenderToFile(window::size[0], window::size[1], filename.c_str());
}

void renderVideoDepth(float newOrder, int frame, GLFWwindow* window)
{
    scene::viewDepth = true;
    window::clear_color[0] = 0.0;
    window::clear_color[1] = 0.0;
    window::clear_color[2] = 0.0;
    glClearColor(window::clear_color[0], window::clear_color[1], window::clear_color[2], window::clear_color[3]);

    scene::order = newOrder;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(grid::vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, grid::points.size() / 3);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //draw_gui(window);

    glfwSwapBuffers(window);

    idle();

    std::string filename = "C:\\Users\\Jack\\Documents\\mandelbulb\\video\\depth_" + std::to_string(frame);
    saveOpenGLRenderToFile(window::size[0], window::size[1], filename.c_str());
}

void color_palettes(int paletteNum)
{
    switch (paletteNum)
    {
    case 0:
    {
        scene::color1 = glm::vec3(0.0, 0.0, 1.0); // Blue
        scene::color2 = glm::vec3(0.0, 1.0, 1.0); // Cyan
        scene::color3 = glm::vec3(0.0, 1.0, 0.0); // Green
        scene::color4 = glm::vec3(1.0, 1.0, 0.0); // Yellow
        scene::color5 = glm::vec3(1.0, 0.0, 0.0); // Red
        break;
    }
    case 1:
    {
        scene::color1 = glm::vec3(0.969, 0.145, 0.522);
        scene::color2 = glm::vec3(0.447, 0.035, 0.718);
        scene::color3 = glm::vec3(0.227, 0.047, 0.064);
        scene::color4 = glm::vec3(0.263, 0.380, 0.933);
        scene::color5 = glm::vec3(0.298, 0.788, 0.941);
        break;
    }
    case 2:
    {
        scene::color1 = glm::vec3(0.051, 0.106, 0.165);
        scene::color2 = glm::vec3(0.106, 0.149, 0.231);
        scene::color3 = glm::vec3(0.255, 0.353, 0.467);
        scene::color4 = glm::vec3(0.467, 0.553, 0.663);
        scene::color5 = glm::vec3(0.878, 0.882, 0.867);
        break;
    }
    default:
    {
        break;
    }
    }
}

void init_framebuffer()
{
    glGenFramebuffers(1, &scene::fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, scene::fbo);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, window::size[0], window::size[1], 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Error: Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, scene::fbo);

   // glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void init_grid()
{
    float steps = 1.f / 256.f;

    for (float i = -1.0; i < 1.0; i += steps)
    {
        for (float j = -1.0; j < 1.0; j += steps)
        {
            for (float k = -1.0; k < 1.0; k += steps)
            {
                grid::points.push_back(i);
                grid::points.push_back(j);
                grid::points.push_back(k);
            }
        }
    }

    glBindAttribLocation(scene::shader, grid::pos_loc, "pos_attrib");

    glGenVertexArrays(1, &grid::vao);
    glBindVertexArray(grid::vao);

    glGenBuffers(1, &grid::vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grid::vbo);
    glBufferData(GL_ARRAY_BUFFER, grid::points.size() * sizeof(float), &grid::points[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(grid::pos_loc);
    glVertexAttribPointer(grid::pos_loc, 3, GL_FLOAT, 0, 0, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void init_canvas()
{
    grid::points.push_back(-1.0);
    grid::points.push_back(-1.0);
    grid::points.push_back(0.0);

    grid::points.push_back(-1.0);
    grid::points.push_back(1.0);
    grid::points.push_back(0.0);

    grid::points.push_back(1.0);
    grid::points.push_back(-1.0);
    grid::points.push_back(0.0);

    grid::points.push_back(1.0);
    grid::points.push_back(1.0);
    grid::points.push_back(0.0);

    glBindAttribLocation(scene::shader, grid::pos_loc, "pos_attrib");

    glGenVertexArrays(1, &grid::vao);
    glBindVertexArray(grid::vao);

    glGenBuffers(1, &grid::vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grid::vbo);
    glBufferData(GL_ARRAY_BUFFER, grid::points.size() * sizeof(float), &grid::points[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(grid::pos_loc);
    glVertexAttribPointer(grid::pos_loc, 3, GL_FLOAT, 0, 0, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//Draw the ImGui user interface
void draw_gui(GLFWwindow* window)
{
    //Begin ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //Draw Gui
    ImGui::Begin("Debug window");
    if (ImGui::Button("Quit"))
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Camera Position: (%.3f, %.3f, %.3f)", scene::camera.position().x, scene::camera.position().y, scene::camera.position().z);
    ImGui::Separator();
    if (ImGui::RadioButton("Color Palette 1", &scene::color_palette, 0)) color_palettes(scene::color_palette);
    if (ImGui::RadioButton("Color Palette 2", &scene::color_palette, 1)) color_palettes(scene::color_palette);
    if (ImGui::RadioButton("Color Palette 3", &scene::color_palette, 2)) color_palettes(scene::color_palette);
    ImGui::ColorEdit3("Color 1", glm::value_ptr(scene::color1));
    ImGui::ColorEdit3("Color 2", glm::value_ptr(scene::color2));
    ImGui::ColorEdit3("Color 3", glm::value_ptr(scene::color3));
    ImGui::ColorEdit3("Color 4", glm::value_ptr(scene::color4));
    ImGui::ColorEdit3("Color 5", glm::value_ptr(scene::color5));
    ImGui::Separator();
    ImGui::SliderFloat("Order", &scene::order, 1.0, 16.0);
    ImGui::SliderInt("Iterations", &scene::iterations, 1, 1000);
    ImGui::SliderFloat("FOV (degrees)", &scene::fov, 1.0, 179.0);
    ImGui::SliderFloat("Ray Scalar", &scene::ray_scalar, 0.001, 1.0);
    ImGui::SliderInt("AA Level", &scene::AA_levels, 1, 3);
    ImGui::Separator();
    ImGui::Checkbox("View Depth", &scene::viewDepth);
    if (scene::viewDepth) {
        ImGui::SliderFloat("Focus Distance", &scene::focusDist, 0.0, 10.0);
        window::clear_color[0] = 1.0;
        window::clear_color[1] = 1.0;
        window::clear_color[2] = 1.0;
        glClearColor(window::clear_color[0], window::clear_color[1], window::clear_color[2], window::clear_color[3]);
    }
    if (!scene::viewDepth)
    {
        window::clear_color[0] = 0.0;
        window::clear_color[1] = 0.0;
        window::clear_color[2] = 0.0;
        glClearColor(window::clear_color[0], window::clear_color[1], window::clear_color[2], window::clear_color[3]);
    }
   /*if(ImGui::Button("Render Image")) saveOpenGLRenderToFile(window::size[0], window::size[1], "C:\\Users\\Jack\\Documents\\mandelbulb\\renders\\output");
   if (ImGui::Button("Render Video")) {
       float frame = 1.0 / 24.0;
       for (int i = 1; i < 24*30; i++) {
           renderVideo((float)i * frame, i, window);
           renderVideoDepth((float)i * frame, i, window);
       }
   }*/
   ImGui::End();

   ImGui::Begin("Controls");
   ImGui::Text("Orbit: ALT+LMB");
   ImGui::Text("Pan:   ALT+MMB");
   ImGui::Text("Zoom:  ALT+RMB or Scrollwheel");
   ImGui::End();

   //End ImGui Frame
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


// This function gets called every time the scene gets redisplayed
void display(GLFWwindow* window)
{
    //glBindFramebuffer(GL_FRAMEBUFFER, scene::fbo);
   
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(grid::vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, grid::points.size() / 3);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_gui(window);

    glfwSwapBuffers(window);
}

void idle()
{
    glm::mat4 M = glm::rotate(scene::angle, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 V = glm::lookAt(scene::camera.position(), scene::camera.front(), scene::camera.up());
    glm::mat4 P = glm::perspective(glm::pi<float>() / 2.0f * (scene::fov / 90.0f), (float)window::size[0] / (float)window::size[1], 0.1f, 100.0f);

    glViewport(0, 0, window::size[0], window::size[1]);

    glUseProgram(scene::shader);

    //Get location for shader uniform variable
    int P_loc = glGetUniformLocation(scene::shader, "P");
    if (P_loc != -1)
    {
        glUniformMatrix4fv(P_loc, 1, false, glm::value_ptr(P));
    }
    int V_loc = glGetUniformLocation(scene::shader, "V");
    if (V_loc != -1)
    {
        glUniformMatrix4fv(V_loc, 1, false, glm::value_ptr(V));
    }
    int M_loc = glGetUniformLocation(scene::shader, "M");
    if (M_loc != -1)
    {
        glUniformMatrix4fv(M_loc, 1, false, glm::value_ptr(M));
    }

    int cam_pos_loc = glGetUniformLocation(scene::shader, "cam_pos");
    if (cam_pos_loc != -1)
    {
        glUniform3fv(cam_pos_loc, 1, glm::value_ptr(scene::camera.position()));
    }

    int window_width_loc = glGetUniformLocation(scene::shader, "window_width");
    if (window_width_loc != -1)
    {
        glUniform1i(window_width_loc, window::size[0]);
    }
    int window_height_loc = glGetUniformLocation(scene::shader, "window_height");
    if (window_height_loc != -1)
    {
        glUniform1i(window_height_loc, window::size[1]);
    }
    int AA_level_loc = glGetUniformLocation(scene::shader, "AA_level");
    if (AA_level_loc != -1)
    {
        glUniform1i(AA_level_loc, scene::AA_levels);
    }
    int ray_scalar_loc = glGetUniformLocation(scene::shader, "ray_scalar");
    if (ray_scalar_loc != -1)
    {
        glUniform1f(ray_scalar_loc, scene::ray_scalar);
    }
    int view_depth_loc = glGetUniformLocation(scene::shader, "view_depth");
    if (view_depth_loc != -1)
    {
        glUniform1f(view_depth_loc, scene::viewDepth);
    }
    int focus_dist_loc = glGetUniformLocation(scene::shader, "focus_dist");
    if (focus_dist_loc != -1)
    {
        glUniform1f(focus_dist_loc, scene::focusDist);
    }
    int iterations_loc = glGetUniformLocation(scene::shader, "num_iterations");
    if (iterations_loc != -1)
    {
        glUniform1i(iterations_loc, scene::iterations);
    }

    int color1_loc = glGetUniformLocation(scene::shader, "color1");
    if (color1_loc != -1)
    {
        glUniform3fv(color1_loc, 1, glm::value_ptr(scene::color1));
    }
    int color2_loc = glGetUniformLocation(scene::shader, "color2");
    if (color2_loc != -1)
    {
        glUniform3fv(color2_loc, 1, glm::value_ptr(scene::color2));
    }
    int color3_loc = glGetUniformLocation(scene::shader, "color3");
    if (color3_loc != -1)
    {
        glUniform3fv(color3_loc, 1, glm::value_ptr(scene::color3));
    }
    int color4_loc = glGetUniformLocation(scene::shader, "color4");
    if (color4_loc != -1)
    {
        glUniform3fv(color4_loc, 1, glm::value_ptr(scene::color4));
    }
    int color5_loc = glGetUniformLocation(scene::shader, "color5");
    if (color5_loc != -1)
    {
        glUniform3fv(color5_loc, 1, glm::value_ptr(scene::color5));
    }

    glUniform1f(glGetUniformLocation(scene::shader, "order"), scene::order);
}

void reload_shader()
{
   std::string vs = scene::shader_dir + scene::vertex_shader;
   std::string fs = scene::shader_dir + scene::fragment_shader;

   GLuint new_shader = InitShader(vs.c_str(), fs.c_str());

   if (new_shader == -1) // loading failed
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
   }
   else
   {
      glClearColor(window::clear_color[0], window::clear_color[1], window::clear_color[2], window::clear_color[3]);

      if (scene::shader != -1)
      {
         glDeleteProgram(scene::shader);
      }
      scene::shader = new_shader;
   }
}

//This function gets called when a key is pressed
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //std::cout << "key : " << key << ", " << char(key) << ", scancode: " << scancode << ", action: " << action << ", mods: " << mods << std::endl;

    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case 'r':
        case 'R':
            reload_shader();
            break;

        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_LEFT_ALT:
            mouse::altPressed = true;
            break;
        case GLFW_KEY_RIGHT_ALT:
            mouse::altPressed = true;
            break;
        }
    }

    if (action == GLFW_RELEASE)
    {
        switch (key)
        {
        case GLFW_KEY_LEFT_ALT:
            mouse::altPressed = false;
            break;
        case GLFW_KEY_RIGHT_ALT:
            mouse::altPressed = false;
            break;
        }
    }
}

void orbit_camera()
{
    scene::yaw += mouse::xoffset;
    scene::pitch += mouse::yoffset;

    scene::camera.orbit(scene::yaw, scene::pitch);
}

void pan_camera()
{
    float translateSensitivity = 0.02;

    scene::camera.pan((float)mouse::xoffset * -translateSensitivity, (float)mouse::yoffset * translateSensitivity);
}

void zoom_camera()
{
    scene::camera.zoom((float)mouse::yoffset * 0.01f);
}

void cursor_offset(double xcurrent, double ycurrent)
{
    mouse::xoffset = xcurrent - mouse::xlast;
    mouse::yoffset = ycurrent - mouse::ylast;
    mouse::xlast = xcurrent;
    mouse::ylast = ycurrent;

    mouse::xoffset *= mouse::sensitivity;
    mouse::yoffset *= mouse::sensitivity;
}

//This function gets called when the mouse moves over the window.
void mouse_cursor(GLFWwindow* window, double x, double y)
{
    if (mouse::altPressed)
    {
        cursor_offset(x, y);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) orbit_camera();
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) pan_camera();
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) zoom_camera();
    }
}

//This function gets called when a mouse button is pressed.
void mouse_button(GLFWwindow* window, int button, int action, int mods)
{
    //std::cout << "button : " << button << ", action: " << action << ", mods: " << mods << std::endl;
    if (action == GLFW_PRESS) glfwGetCursorPos(window, &mouse::xlast, &mouse::ylast);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    scene::camera.zoom((float)yoffset * -0.1f);
}

void window_size(GLFWwindow* window, int width, int height)
{
    window::size[0] = width;
    window::size[1] = height;
}

//Initialize OpenGL state. This function only gets called once.
void init()
{
   glewInit();
   RegisterDebugCallback();

   //init_framebuffer();

   std::ostringstream oss;
   //Get information about the OpenGL version supported by the graphics driver.	
   oss << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   oss << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   oss << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   oss << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;

   //Output info to console
   std::cout << oss.str();

   //Output info to file
   std::fstream info_file("info.txt", std::ios::out | std::ios::trunc);
   info_file << oss.str();
   info_file.close();

   reload_shader();

   //init_grid();
   init_canvas();

   //Set the color the screen will be cleared to when glClear is called
   glClearColor(window::clear_color[0], window::clear_color[1], window::clear_color[2], window::clear_color[3]);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_MULTISAMPLE);
}


// C++ programs start executing in the main() function.
int main(void)
{
   GLFWwindow* window;

   // Initialize the library
   if (!glfwInit())
   {
      return -1;
   }

#ifdef _DEBUG
   glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

   // Create a windowed mode window and its OpenGL context
   window = glfwCreateWindow(window::size[0], window::size[1], window::title, NULL, NULL);
   if (!window)
   {
      glfwTerminate();
      return -1;
   }

   //Register callback functions with glfw. 
   glfwSetKeyCallback(window, keyboard);
   glfwSetCursorPosCallback(window, mouse_cursor);
   glfwSetMouseButtonCallback(window, mouse_button);
   glfwSetWindowSizeCallback(window, window_size);
   glfwSetScrollCallback(window, scroll_callback);

   // Make the window's context current
   glfwMakeContextCurrent(window);

   glfwWindowHint(GLFW_SAMPLES, 4);

   init();

   // New in Lab 2: Init ImGui
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 150");

   // Loop until the user closes the window 
   while (!glfwWindowShouldClose(window))
   {
      idle();
      display(window);

      // Poll for and process events 
      glfwPollEvents();
   }

   // New in Lab 2: Cleanup ImGui
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   glfwTerminate();
   return 0;
}