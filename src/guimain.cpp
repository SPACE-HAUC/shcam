/**
 * @file guimain.cpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
extern "C"
{
#include <ucam.h>
#include <gpiodev/gpiodev.h>

#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
}

#include <imgui/imgui.h>
#include <imgui_backends/imgui_impl_glfw.h>
#include <imgui_backends/imgui_impl_opengl2.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>
#if defined(_MSC_VER)
#error "NT kernel not supported"
#endif

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

const ImVec4 IMRED = ImVec4(1, 0, 0, 1); // RGBA
const ImVec4 IMGRN = ImVec4(0, 1, 0, 1);
const ImVec4 IMBLU = ImVec4(0, 0, 1, 1);
const ImVec4 IMCYN = ImVec4(0, 1, 1, 1);
const ImVec4 IMYLW = ImVec4(1, 1, 0, 1);
const ImVec4 IMMGN = ImVec4(1, 0, 1, 1);

#include <jpeglib.h>

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char *filename, GLuint *out_texture, int *out_width, int *out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char *image_data;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    /* More stuff */
    FILE *infile;      /* source file */
    JSAMPARRAY buffer; /* Output row buffer */
    int row_stride;    /* physical row width in output buffer */

    /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

    if ((infile = fopen(filename, "rb")) == NULL)
    {
        fprintf(stderr, "can't open %s\n", filename);
        return 0;
    }

    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    cinfo.err = jpeg_std_error(&jerr);
    /* Step 1: allocate and initialize JPEG decompression object */
    jpeg_create_decompress(&cinfo);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);

    /* Step 2: specify data source (eg, a file) */

    jpeg_stdio_src(&cinfo, infile);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* Step 3: read file parameters with jpeg_read_header() */

    fprintf(stderr, "%s: %d %d\n", __func__, __LINE__, jpeg_read_header(&cinfo, TRUE));
    /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.txt for more info.
   */
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* Step 4: set parameters for decompression */

    /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

    /* Step 5: Start decompressor */
    cinfo.out_color_space = JCS_EXT_RGBX;
    cinfo.scale_num = 768;
    cinfo.scale_denom = cinfo.image_height;
    (void)jpeg_start_decompress(&cinfo);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */
    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

    /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
    image_data = (unsigned char *)malloc(row_stride * cinfo.output_height);
    image_height = cinfo.output_height;
    image_width = cinfo.output_width;
    int loc = 0;
    fprintf(stderr, "%s: %d: Width = %d, Height = %d Size = %d\n", __func__, __LINE__, cinfo.output_width, cinfo.output_height, row_stride * cinfo.output_height);
    while (cinfo.output_scanline < cinfo.output_height)
    {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        /* Assume put_scanline_someplace wants a pointer and sample count. */
        memcpy(&(image_data[loc]), buffer[0], row_stride);
        loc += row_stride;
    }

    /* Step 7: Finish decompression */

    (void)jpeg_finish_decompress(&cinfo);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
    fclose(infile);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    fprintf(stderr, "%s: %d %d\n", __func__, __LINE__, image_texture);
    free(image_data);
    *out_texture = image_texture;
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    *out_width = image_width;
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    *out_height = image_height;
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    return true;
}

GLuint my_image_texture;
int my_image_width, my_image_height;

bool ImageWindowStat = false;

void MainWindow()
{
    bool dummy = false;
    ImGui::Begin("Main Window");
    ImGui::Checkbox("Display Image", &ImageWindowStat);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void ImageWindow(bool *active)
{
    ImGui::Begin("Image Display", active);
    if (my_image_texture != NULL)
    {
        ImGui::Text("pointer = %p", my_image_texture);
        ImGui::Text("size = %d x %d", my_image_width, my_image_height);
        ImGui::Image((void *)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));
    }
    ImGui::End();
}

int main(int argc, char *argv[])
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    GLFWwindow *window = glfwCreateWindow(1024, 720, "XBand Test System", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(5); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    io.Fonts->AddFontFromFileTTF("font/Roboto-Medium.ttf", 14.0f);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    LoadTextureFromFile("test.jpeg", &my_image_texture, &my_image_width, &my_image_height);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Windows
        MainWindow();
        if (ImageWindowStat)
            ImageWindow(&ImageWindowStat);
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
        // you may need to backup/reset/restore current shader using the commented lines below.
        //GLint last_program;
        //glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        //glUseProgram(0);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        //glUseProgram(last_program);

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }
    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}