#define STB_IMAGE_IMPLEMENTATION

#include "utilities/glfw_tool.h"
#include "utilities/shader_t.h"
#include "utilities/camera.h"


const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
const unsigned short NUM_PATCH_PTS = 4;

// timing
float deltaTime = 0.0f;    // time between current frame and last frame
float lastFrame = 0.0f;

int main() {

    utilities::camera cam(glm::vec2(SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f), glm::vec3(0.0f, 0.0f, 3.0f));

    GLFWwindow *window;
    try {
        window = utilities::init_window("3D Perlin Map", cam, SCR_WIDTH, SCR_HEIGHT);
    } catch (std::runtime_error &error) {
        std::cout << error.what() << std::endl;
        return -1;
    } catch (std::exception &error) {
        std::cout << error.what() << std::endl;
        return -1;
    }

    GLint maxTessLevel;
    glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &maxTessLevel);
    std::cout << "Max available tess level: " << maxTessLevel << std::endl;

    // enable depth test
    glEnable(GL_DEPTH_TEST);

//    utilities::shader_t shader_program(std::string("../shaders/"), std::string("PerlinMap.vert"),
//                                       std::string("PerlinMap.frag"),std::string("PerlinMap.tesc"),std::string("PerlinMap.tese"));

    utilities::shader_t shader_program(std::string("../shaders/"), std::string("PerlinMap.vert"),
                                       std::string("PerlinMap.frag"), std::string("PerlinMap.tesc"),
                                       std::string("PerlinMap.tese"));

    int map_width = 0;
    int map_height = 0;
    // load height map
    unsigned int height_map_id = utilities::load_texture("../assets/images/", "render.png", map_width, map_height);
    glBindTexture(GL_TEXTURE_2D, height_map_id);
    glActiveTexture(GL_TEXTURE0);
    shader_program.set_int("world_map", 0);

    std::vector<float> vertices;

    const unsigned patch_numbers = 20;

    auto map_width_f = static_cast<float>(map_width);
    auto map_height_f = static_cast<float>(map_height);

    // compute the coordinates of every corner of each panel

    // compute the reciprocal once to minimize performance impact
    const float patch_reciprocal = 1.0f / static_cast<float>(patch_numbers);

    // divide the width and height
    float width_offset_factor = static_cast<float>(map_width) * patch_reciprocal;
    float height_offset_factor = static_cast<float>(map_height) * patch_reciprocal;

    for (auto i = 0; i <= patch_numbers - 1; ++i) {
        auto x = static_cast<float>(i);

        for (auto j = 0; j <= patch_numbers - 1; ++j) {

            auto z = static_cast<float>(j);

            // coordinates of the left lower corner of the panel
            vertices.push_back(-map_width_f * 0.5f + x * width_offset_factor);
            vertices.push_back(0.0f);
            vertices.push_back(-map_height_f * 0.5f + z * height_offset_factor);
            vertices.push_back(x * patch_reciprocal);
            vertices.push_back(z * patch_reciprocal);

            // coordinates of the right lower corner of the panel
            vertices.push_back(-map_width_f * 0.5f + (x + 1) * width_offset_factor);
            vertices.push_back(0.0f);
            vertices.push_back(-map_height_f * 0.5f + z * height_offset_factor);
            vertices.push_back((x + 1) * patch_reciprocal);
            vertices.push_back(z * patch_reciprocal);

            // coordinates of the left upper corner of the panel
            vertices.push_back(-map_width_f * 0.5f + x * width_offset_factor);
            vertices.push_back(0.0f);
            vertices.push_back(-map_height_f * 0.5f + (z + 1) * height_offset_factor);
            vertices.push_back(x * patch_reciprocal);
            vertices.push_back((z + 1) * patch_reciprocal);

            // coordinates of the right upper corner of the panel
            vertices.push_back(-map_width_f * 0.5f + (x + 1) * width_offset_factor);
            vertices.push_back(0.0f);
            vertices.push_back(-map_height_f * 0.5f + (z + 1) * height_offset_factor);
            vertices.push_back((x + 1) * patch_reciprocal);
            vertices.push_back((z + 1) * patch_reciprocal);
        }
    }

    std::cout << "Loaded " << patch_numbers * patch_numbers << " patches of 4 control points each" << std::endl;
    std::cout << "Processing " << patch_numbers * patch_numbers * 4 << " vertices in vertex shader" << std::endl;
    std::cout << "vertices size: " << vertices.size() << std::endl;

    // configure the cube's VAO (and terrainVBO)
    unsigned int terrainVAO, terrainVBO;
    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(float) * vertices.size()), &vertices[0],
                 GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // texCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    // Specify the number of vertices per patch
    glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

    shader_program.use();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    while (!glfwWindowShouldClose(window)) {

        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        utilities::process_input(window, cam, deltaTime);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = cam.get_projection_matrix(SCR_WIDTH, SCR_HEIGHT, 0.1f, 100000.0f);

        // camera/view transformation
        glm::mat4 view = cam.get_view_matrix();

        // calculate the model matrix for each object and pass it to shader before drawing
        glm::mat4 model = glm::mat4(1.0f);

        shader_program
                .set_mat4("projection", projection)
                .set_mat4("view", view)
                .set_mat4("model", model);

        // render the triangle
        glBindVertexArray(terrainVAO);

        glDrawArrays(GL_PATCHES, 0, static_cast<GLsizei>(NUM_PATCH_PTS * patch_numbers * patch_numbers));

        // swap the color buffer
        glfwSwapBuffers(window);
        // checks if any events are triggered per frame
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteProgram(shader_program.id);

    glfwTerminate();
    return 0;
}