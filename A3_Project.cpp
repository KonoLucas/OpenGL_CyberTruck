// Assignment 3
// Team members: 1. Hui Zhang 40170679
//               2. Zexu Hao 40233332
//               3. Mingming Zhang 40258080

/*
 * 3D Coordinate System Definition:
 * - X-axis: Right (positive direction points to the right of the screen)
 * - Y-axis: Up (positive direction points upwards)
 * - Z-axis: Backward (positive direction points INTO the screen, negative points OUT)
 * 
 * Camera View Space:
 * - The view matrix transforms world coordinates to camera space
 * - Camera looks along the negative Z-axis (into the screen)
 * - viewMatrix columns:
 *   [0]: Right vector (X-axis)
 *   [1]: Up vector (Y-axis)
 *   [2]: Back vector (negative Z-axis)
 */

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
 #include <algorithm>
 #include <cfloat>
 
 // Global OpenGL objects and matrices
 GLuint VAO;  // Vertex Array Object
 GLuint VBO;  // Vertex Buffer Object
 GLuint shaderProgram;  // Shader program ID
 glm::mat4 modelMatrix = glm::mat4(1.0f);  // Model transformation matrix
 glm::mat4 viewMatrix;  // View (camera) matrix
 glm::mat4 projectionMatrix;  // Projection matrix
 
 // Model data containers
 std::vector<float> vertices;  // Vertex position data
 std::vector<glm::vec3> materialColors;  // Color for each mesh
 std::vector<unsigned int> meshVertexCounts;  // Vertex count for each mesh
 
 // Vertex shader source code
 const char* vertexShaderSource = R"(
     #version 330 core
     layout (location = 0) in vec3 aPos;  // Vertex position attribute
     uniform mat4 model;  // Model matrix
     uniform mat4 view;   // View matrix
     uniform mat4 projection;  // Projection matrix
     
     void main() {
         // Transform vertex position to clip space
         gl_Position = projection * view * model * vec4(aPos, 1.0);
     }
 )";
 
 // Fragment shader source code
 const char* fragmentShaderSource = R"(
     #version 330 core
     out vec4 FragColor;  // Output color
     uniform vec3 materialColor;  // Material color uniform
     
     void main() {
         FragColor = vec4(materialColor, 1.0);  // Set fragment color
     }
 )";
 
 /**
  * Compiles a shader from source code
  * @param type Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
  * @param source Shader source code
  * @return Compiled shader ID
  */
 GLuint compileShader(GLenum type, const char* source) {
     GLuint shader = glCreateShader(type);
     glShaderSource(shader, 1, &source, nullptr);
     glCompileShader(shader);
 
     // Check for compilation errors
     GLint success;
     glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
     if (!success) {
         char infoLog[512];
         glGetShaderInfoLog(shader, 512, nullptr, infoLog);
         std::cerr << "Failed to compile shader: " << infoLog << std::endl;
     }
     return shader;
 }
 
 /**
  * Loads a 3D model from an OBJ file using Assimp
  * @param path Path to the OBJ file
  * @param vertices Output vector for vertex data
  * @param materialColors Output vector for material colors
  * @return true if loading succeeded, false otherwise
  */
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
 
     // Process each mesh in the scene
     for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
         aiMesh* mesh = scene->mMeshes[m];
         
         // Store vertex positions
         for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
             vertices.push_back(mesh->mVertices[i].x);
             vertices.push_back(mesh->mVertices[i].y);
             vertices.push_back(mesh->mVertices[i].z);
         }
 
         // Get material color
         unsigned int materialIndex = mesh->mMaterialIndex;
         aiMaterial* material = scene->mMaterials[materialIndex];
         aiColor4D diffuseColor;
         if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS) {
             materialColors.push_back(glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b));
         }
         else {
             // Default gray color if no material color is found
             materialColors.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
         }
     }
 
     std::cout << "Loaded " << vertices.size() / 3 << " vertices from " << path << std::endl;
     return true;
 }
 
 /**
  * Initializes OpenGL state and loads the model
  */
 void initGL() {
     // Load the 3D model
     if (!loadOBJ("car.obj", vertices, materialColors)) {
         std::cerr << "Cannot load car.obj" << std::endl;
         exit(EXIT_FAILURE);
     }
 
     // Enable depth testing for 3D rendering
     glEnable(GL_DEPTH_TEST);
     glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
 
     // Create and configure VAO and VBO
     glGenVertexArrays(1, &VAO);
     glGenBuffers(1, &VBO);
     glBindVertexArray(VAO);
     glBindBuffer(GL_ARRAY_BUFFER, VBO);
     glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
 
     // Set vertex attribute pointers
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
     glEnableVertexAttribArray(0);
 
     // Compile and link shaders
     GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
     GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
 
     shaderProgram = glCreateProgram();
     glAttachShader(shaderProgram, vertexShader);
     glAttachShader(shaderProgram, fragmentShader);
     glLinkProgram(shaderProgram);
 
     // Check for linking errors
     GLint success;
     glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
     if (!success) {
         char infoLog[512];
         glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
         std::cerr << "Shader program linking failed: " << infoLog << std::endl;
     }
     glDeleteShader(vertexShader);
     glDeleteShader(fragmentShader);
 
     // Calculate model bounds for proper scaling
     float minX = FLT_MAX, maxX = -FLT_MAX, minY = FLT_MAX, maxY = -FLT_MAX, minZ = FLT_MAX, maxZ = -FLT_MAX;
     for (size_t i = 0; i < vertices.size(); i += 3) {
         minX = std::min(minX, vertices[i]);
         maxX = std::max(maxX, vertices[i]);
         minY = std::min(minY, vertices[i + 1]);
         maxY = std::max(maxY, vertices[i + 1]);
         minZ = std::min(minZ, vertices[i + 2]);
         maxZ = std::max(maxZ, vertices[i + 2]);
     }
 
     // Scale the model to fit in view
     float scaleFactor = 5.0f;
     float maxBound = std::max({ maxX - minX, maxY - minY, maxZ - minZ });
     modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleFactor / maxBound));
     
     // Set up view matrix (camera)
     viewMatrix = glm::lookAt(
         glm::vec3(0.0f, 0.0f, (maxBound / scaleFactor) * 2.0f),  // Camera position
         glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
         glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector (Y-axis)
     );
     
     // Set up perspective projection
     projectionMatrix = glm::perspective(
         glm::radians(45.0f),  // Field of view
         800.0f / 600.0f,      // Aspect ratio
         0.1f,                 // Near clipping plane
         100.0f                // Far clipping plane
     );
 
     // Load mesh vertex counts for rendering
     Assimp::Importer importer;
     const aiScene* scene = importer.ReadFile("car.obj", aiProcess_Triangulate | aiProcess_FlipUVs);
     if (scene) {
         for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
             aiMesh* mesh = scene->mMeshes[m];
             meshVertexCounts.push_back(mesh->mNumVertices);
         }
     }
 }
 
 /**
  * Processes keyboard input for controlling the model
  * @param window GLFW window handle
  * @param translation Current model translation
  * @param rotationAngle Current rotation angle
  * @param rotationPending Flag for rotation key state
  * @param scaleZ Current Z-axis scale factor
  */
 void processInput(GLFWwindow* window, glm::vec3& translation, float& rotationAngle,
     bool& rotationPending, float& scaleZ) {
     // Close window on ESC
     if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
         glfwSetWindowShouldClose(window, true);
 
     const float moveSpeed = 0.001f;  // Movement speed
     const float rotationSpeed = glm::radians(30.0f);  // Rotation speed
 
     // W/S: Move along camera's UP vector (screen Y-axis)
     if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
         translation += glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]) * moveSpeed;
     }
     if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
         translation -= glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]) * moveSpeed;
     }
 
     // A/D: Move along camera's RIGHT vector (screen X-axis)
     if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
         translation -= glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]) * moveSpeed;
     }
     if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
         translation += glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]) * moveSpeed;
     }
 
     // Q/E: Rotate around world Y-axis
     if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && !rotationPending) {
         rotationAngle -= 30.0f;
         rotationPending = true;
     }
     if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !rotationPending) {
         rotationAngle += 30.0f;
         rotationPending = true;
     }
     if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) {
         rotationPending = false;
     }
 
     // R/F: Scale along Z-axis
     if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
         scaleZ += 0.001f;
         if (scaleZ > 5.0f) scaleZ = 5.0f;  // Maximum scale limit
     }
     if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
         scaleZ -= 0.001f;
         if (scaleZ < 0.1f) scaleZ = 0.1f;  // Minimum scale limit
     }
 }
 
 /**
  * Renders the model with current transformations
  * @param translation Current translation
  * @param rotationAngle Current rotation angle
  * @param scaleZ Current Z-axis scale
  */
 void render(glm::vec3 translation, float rotationAngle, float scaleZ) {
     // Clear buffers
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glUseProgram(shaderProgram);
 
     // Apply transformations
     glm::mat4 transformation = glm::mat4(1.0f);  // Identity matrix
     transformation = glm::translate(transformation, translation);  // Apply translation
     transformation = glm::rotate(transformation, glm::radians(rotationAngle), 
                                glm::vec3(0.0f, 1.0f, 0.0f));  // Apply rotation around Y-axis
     transformation = glm::scale(transformation, glm::vec3(1.0f, 1.0f, scaleZ));  // Apply Z-scale
 
     // Pass matrices to shader
     glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, 
                       glm::value_ptr(modelMatrix * transformation));
     glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, 
                       glm::value_ptr(viewMatrix));
     glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, 
                       glm::value_ptr(projectionMatrix));
 
     // Draw each mesh with its material color
     glBindVertexArray(VAO);
     unsigned int vertexOffset = 0;
     for (size_t i = 0; i < meshVertexCounts.size(); i++) {
         glUniform3fv(glGetUniformLocation(shaderProgram, "materialColor"), 1, 
                     glm::value_ptr(materialColors[i]));
         glDrawArrays(GL_TRIANGLES, vertexOffset, meshVertexCounts[i]);
         vertexOffset += meshVertexCounts[i];
     }
 }
 
 int main() {
     // Initialize GLFW
     if (!glfwInit()) {
         std::cerr << "GLFW initialization failed!" << std::endl;
         return -1;
     }
 
     // Create window
     GLFWwindow* window = glfwCreateWindow(800, 600, "Car Model Viewer", nullptr, nullptr);
     if (!window) {
         std::cerr << "Window creation failed!" << std::endl;
         glfwTerminate();
         return -1;
     }
     glfwMakeContextCurrent(window);
 
     // Initialize GLEW
     if (glewInit() != GLEW_OK) {
         std::cerr << "GLEW initialization failed!" << std::endl;
         glfwTerminate();
         return -1;
     }
 
     // Initialize OpenGL and load model
     initGL();
 
     // Initialize transformation parameters
     glm::vec3 translation(0.0f, 0.0f, 0.0f);  // Model position
     float rotationAngle = 0.0f;  // Rotation angle around Y-axis
     bool rotationPending = false;  // Rotation key state flag
     float scaleZ = 1.0f;  // Z-axis scale factor
 
     // Main render loop
     while (!glfwWindowShouldClose(window)) {
         processInput(window, translation, rotationAngle, rotationPending, scaleZ);
         render(translation, rotationAngle, scaleZ);
         glfwSwapBuffers(window);
         glfwPollEvents();
     }
 
     // Clean up resources
     glDeleteVertexArrays(1, &VAO);
     glDeleteBuffers(1, &VBO);
     glDeleteProgram(shaderProgram);
     glfwTerminate();
     return 0;
 }