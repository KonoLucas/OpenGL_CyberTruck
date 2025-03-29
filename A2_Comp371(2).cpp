#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

GLuint VAO, VBO, shaderProgram;
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;
std::vector<float> vertices;
std::vector<glm::vec3> materialColors;
std::vector<unsigned int> meshVertexCounts;

// 顶点着色器
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

// 片段着色器
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 materialColor;
    void main() {
        FragColor = vec4(materialColor, 1.0);
    }
)";

// 编译着色器
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Failed to compile shader: " << infoLog << std::endl;
    }
    return shader;
}

// 加载 OBJ 文件和材质
bool loadOBJ(const std::string& path, std::vector<float>& vertices, std::vector<glm::vec3>& materialColors) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp load failed: " << importer.GetErrorString() << std::endl;
        return false;
    }

    // 加载所有网格的顶点和材质
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            vertices.push_back(mesh->mVertices[i].x);
            vertices.push_back(mesh->mVertices[i].y);
            vertices.push_back(mesh->mVertices[i].z);
        }

        // 获取材质
        unsigned int materialIndex = mesh->mMaterialIndex;
        aiMaterial* material = scene->mMaterials[materialIndex];
        aiColor4D diffuseColor;
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS) {
            materialColors.push_back(glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b));
        }
        else {
            materialColors.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
        }
    }

    std::cout << "Loaded " << vertices.size() / 3 << " vertices from " << path << std::endl;
    return true;
}

void initGL() {
    // 加载 OBJ 文件和材质
    if (!loadOBJ("car.obj", vertices, materialColors)) {
        std::cerr << "Cannot load car.obj" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    // 创建并绑定 VAO 和 VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 设置顶点属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 编译并链接着色器
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 设置矩阵
    float minX = FLT_MAX, maxX = -FLT_MAX, minY = FLT_MAX, maxY = -FLT_MAX, minZ = FLT_MAX, maxZ = -FLT_MAX;
    for (size_t i = 0; i < vertices.size(); i += 3) {
        minX = std::min(minX, vertices[i]);
        maxX = std::max(maxX, vertices[i]);
        minY = std::min(minY, vertices[i + 1]);
        maxY = std::max(maxY, vertices[i + 1]);
        minZ = std::min(minZ, vertices[i + 2]);
        maxZ = std::max(maxZ, vertices[i + 2]);
    }

    float scaleFactor = 5.0f; // 放大 10 倍
    float maxBound = std::max({ maxX - minX, maxY - minY, maxZ - minZ });
    modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleFactor / maxBound));
    viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, (maxBound / scaleFactor) * 2.0f), // 调整摄像机距离
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    projectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // 计算每个网格的顶点数量
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("car.obj", aiProcess_Triangulate | aiProcess_FlipUVs);
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];
        meshVertexCounts.push_back(mesh->mNumVertices);
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    // 传递矩阵到着色器
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    // 绘制每个网格
    glBindVertexArray(VAO);
    unsigned int vertexOffset = 0;
    for (size_t i = 0; i < meshVertexCounts.size(); i++) {
        glUniform3fv(glGetUniformLocation(shaderProgram, "materialColor"), 1, glm::value_ptr(materialColors[i]));
        glDrawArrays(GL_TRIANGLES, vertexOffset, meshVertexCounts[i]);
        vertexOffset += meshVertexCounts[i];
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed!" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Car Model Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed!" << std::endl;
        glfwTerminate();
        return -1;
    }

    initGL();

    while (!glfwWindowShouldClose(window)) {
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}