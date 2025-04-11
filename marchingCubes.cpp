#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
#include <cmath>
#include "TriTable.hpp"

class Axes
{

    glm::vec3 origin;
    glm::vec3 extents;

    glm::vec3 xcol = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ycol = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 zcol = glm::vec3(0.0f, 0.0f, 1.0f);

public:
    Axes(glm::vec3 orig, glm::vec3 ex) : origin(orig), extents(ex) {}

    void draw()
    {

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor3f(xcol.x, xcol.y, xcol.z);
        glVertex3f(origin.x, origin.y, origin.z);
        glVertex3f(origin.x + extents.x, origin.y, origin.z);

        glVertex3f(origin.x + extents.x, origin.y, origin.z);
        glVertex3f(origin.x + extents.x, origin.y, origin.z + 0.1);
        glVertex3f(origin.x + extents.x, origin.y, origin.z);
        glVertex3f(origin.x + extents.x, origin.y, origin.z - 0.1);

        glColor3f(ycol.x, ycol.y, ycol.z);
        glVertex3f(origin.x, origin.y, origin.z);
        glVertex3f(origin.x, origin.y + extents.y, origin.z);

        glVertex3f(origin.x, origin.y + extents.y, origin.z);
        glVertex3f(origin.x, origin.y + extents.y, origin.z + 0.1);
        glVertex3f(origin.x, origin.y + extents.y, origin.z);
        glVertex3f(origin.x, origin.y + extents.y, origin.z - 0.1);

        glColor3f(zcol.x, zcol.y, zcol.z);
        glVertex3f(origin.x, origin.y, origin.z);
        glVertex3f(origin.x, origin.y, origin.z + extents.z);

        glVertex3f(origin.x, origin.y, origin.z + extents.z);
        glVertex3f(origin.x + 0.1, origin.y, origin.z + extents.z);

        glVertex3f(origin.x, origin.y, origin.z + extents.z);
        glVertex3f(origin.x - 0.1, origin.y, origin.z + extents.z);
        glEnd();

        glPopMatrix();
    }
};

float cam_r = 8.66f;
float cam_theta = glm::radians(45.0f);
float cam_phi = glm::radians(55.0f);
double lastX, lastY;
bool mousePressed = false;

float gridMin = -5.0f;
float gridMax = 5.0f;
float stepSize = 0.5f;

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mousePressed = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
        }
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (mousePressed)
    {
        float dx = float(xpos - lastX);
        float dy = float(ypos - lastY);
        lastX = xpos;
        lastY = ypos;
        cam_theta += glm::radians(dx * 0.5f);
        cam_phi += glm::radians(dy * 0.5f);
        if (cam_phi < 0.1f)
            cam_phi = 0.1f;
        if (cam_phi > glm::pi<float>() - 0.1f)
            cam_phi = glm::pi<float>() - 0.1f;
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_UP)
        {
            cam_r -= 0.5f;
            if (cam_r < 1.0f)
                cam_r = 1.0f;
        }
        if (key == GLFW_KEY_DOWN)
        {
            cam_r += 0.5f;
        }
    }
}

glm::vec3 computeCameraPos()
{
    float x = cam_r * sin(cam_phi) * cos(cam_theta);
    float y = cam_r * cos(cam_phi);
    float z = cam_r * sin(cam_phi) * sin(cam_theta);
    return glm::vec3(x, y, z);
}

glm::vec3 vertexInterp(float isovalue, const glm::vec3 &p1, const glm::vec3 &p2, float valp1, float valp2)
{
    if (fabs(isovalue - valp1) < 0.00001f)
        return p1;
    if (fabs(isovalue - valp2) < 0.00001f)
        return p2;
    if (fabs(valp1 - valp2) < 0.00001f)
        return p1;
    float mu = (isovalue - valp1) / (valp2 - valp1);
    return p1 + mu * (p2 - p1);
}

int edgeIndex[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

std::vector<float> marchingCubes(std::function<float(float, float, float)> f, float isovalue, float gridMin, float gridMax, float stepSize)
{
    std::vector<float> vertices;
    glm::vec3 vertexOffset[8] = {
        glm::vec3(0, 0, 0),
        glm::vec3(stepSize, 0, 0),
        glm::vec3(stepSize, 0, stepSize),
        glm::vec3(0, 0, stepSize),
        glm::vec3(0, stepSize, 0),
        glm::vec3(stepSize, stepSize, 0),
        glm::vec3(stepSize, stepSize, stepSize),
        glm::vec3(0, stepSize, stepSize)};

    for (float x = gridMin; x < gridMax; x += stepSize)
    {
        for (float y = gridMin; y < gridMax; y += stepSize)
        {
            for (float z = gridMin; z < gridMax; z += stepSize)
            {
                glm::vec3 cubePos(x, y, z);
                float cubeValues[8];
                glm::vec3 cubeVerts[8];
                for (int i = 0; i < 8; i++)
                {
                    cubeVerts[i] = cubePos + vertexOffset[i];
                    cubeValues[i] = f(cubeVerts[i].x, cubeVerts[i].y, cubeVerts[i].z);
                }
                int cubeIndex = 0;
                for (int i = 0; i < 8; i++)
                {
                    if (cubeValues[i] < isovalue)
                        cubeIndex |= (1 << i);
                }
                if (marching_cubes_lut[cubeIndex][0] == -1)
                    continue;
                for (int i = 0; marching_cubes_lut[cubeIndex][i] != -1; i += 3)
                {
                    glm::vec3 triVerts[3];
                    for (int j = 0; j < 3; j++)
                    {
                        int edge = marching_cubes_lut[cubeIndex][i + j];
                        int v1 = edgeIndex[edge][0];
                        int v2 = edgeIndex[edge][1];
                        triVerts[j] = vertexInterp(isovalue, cubeVerts[v1], cubeVerts[v2], cubeValues[v1], cubeValues[v2]);
                    }
                    for (int j = 0; j < 3; j++)
                    {
                        vertices.push_back(triVerts[j].x);
                        vertices.push_back(triVerts[j].y);
                        vertices.push_back(triVerts[j].z);
                    }
                }
            }
        }
    }
    return vertices;
}

std::vector<float> computeNormals(const std::vector<float> &vertices)
{
    std::vector<float> normals;
    for (size_t i = 0; i < vertices.size(); i += 9)
    {
        glm::vec3 p0(vertices[i], vertices[i + 1], vertices[i + 2]);
        glm::vec3 p1(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
        glm::vec3 p2(vertices[i + 6], vertices[i + 7], vertices[i + 8]);
        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 n = glm::normalize(glm::cross(edge1, edge2));
        for (int j = 0; j < 3; j++)
        {
            normals.push_back(n.x);
            normals.push_back(n.y);
            normals.push_back(n.z);
        }
    }
    return normals;
}

void writePLY(const std::vector<float> &vertices, const std::vector<float> &normals, const std::string &fileName)
{
    std::ofstream ofs(fileName);
    if (!ofs)
    {
        std::cerr << "Cannot open file " << fileName << " for writing.\n";
        return;
    }
    int numVertices = vertices.size() / 3;
    int numFaces = numVertices / 3;
    ofs << "ply\nformat ascii 1.0\n";
    ofs << "element vertex " << numVertices << "\n";
    ofs << "property float x\nproperty float y\nproperty float z\n";
    ofs << "property float nx\nproperty float ny\nproperty float nz\n";
    ofs << "element face " << numFaces << "\n";
    ofs << "property list uchar int vertex_indices\n";
    ofs << "end_header\n";
    for (int i = 0; i < numVertices; i++)
    {
        ofs << vertices[3 * i] << " " << vertices[3 * i + 1] << " " << vertices[3 * i + 2] << " ";
        ofs << normals[3 * i] << " " << normals[3 * i + 1] << " " << normals[3 * i + 2] << "\n";
    }
    for (int i = 0; i < numFaces; i++)
    {
        ofs << "3 " << 3 * i << " " << 3 * i + 1 << " " << 3 * i + 2 << "\n";
    }
    ofs.close();
    std::cout << "PLY file written: " << fileName << "\n";
}

GLuint compileShader(const char *vertexSource, const char *fragmentSource)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed:\n"
                  << infoLog << "\n";
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed:\n"
                  << infoLog << "\n";
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed:\n"
                  << infoLog << "\n";
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

void createMeshBuffers(GLuint &VAO, GLuint &VBO, const std::vector<float> &vertices, const std::vector<float> &normals)
{
    std::vector<float> interleaved;
    int numVerts = vertices.size() / 3;
    for (int i = 0; i < numVerts; i++)
    {
        interleaved.push_back(vertices[3 * i]);
        interleaved.push_back(vertices[3 * i + 1]);
        interleaved.push_back(vertices[3 * i + 2]);
        interleaved.push_back(normals[3 * i]);
        interleaved.push_back(normals[3 * i + 1]);
        interleaved.push_back(normals[3 * i + 2]);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float), interleaved.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

GLuint createLineVAO(const std::vector<float> &lineData)
{
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return VAO;
}

const char *lineVertexSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 MVP;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
}
)";

const char *lineFragmentSource = R"(
#version 330 core
uniform vec3 lineColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(lineColor, 1.0);
}
)";

int main(int argc, char **argv)
{
    Axes worldaxes(glm::vec3(gridMin, gridMin, gridMin), glm::vec3(gridMax - gridMin, gridMax - gridMin, gridMax - gridMin));

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "exercise1", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    const char *vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    uniform mat4 MVP;
    uniform mat4 V;
    uniform vec3 LightDir;
    out vec3 FragPos;
    out vec3 Normal;
    out vec3 LightDirection;
    void main() {
        FragPos = aPos;
        vec4 NormalTest = vec4(aNormal, 0) * V;
        Normal = vec3(NormalTest.x, NormalTest.y, NormalTest.z);
        vec4 LightDirectionTest = vec4(LightDir, 1) * V;
        LightDirection = vec3(LightDirectionTest.x, LightDirectionTest.y, LightDirectionTest.z);
        gl_Position = MVP * vec4(aPos, 1.0);
    }
    )";

    const char *fragmentShaderSource = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    in vec3 LightDirection;
    uniform vec3 modelColor;
    uniform vec3 cameraPos;
    out vec4 FragColor;
    void main() {
        vec3 ambient = vec3(0.2);
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(LightDirection);
        float diff = clamp(dot(norm, lightDir), 0.0, 1.0);
        vec3 diffuse = diff * modelColor;
        vec3 viewDir = normalize(0 - cameraPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(clamp(dot(viewDir, reflectDir), 0, 1), 64);
        vec3 specular = vec3(1.0) * spec;
        vec3 result = (ambient * modelColor) + diffuse + specular;
        FragColor = vec4(result, 1.0);
    }
)";

    GLuint shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);
    GLuint lineShaderProgram = compileShader(lineVertexSource, lineFragmentSource);

    int fieldChoice = 1;
    if (argc > 1)
    {
        fieldChoice = std::atoi(argv[1]);
    }

    std::function<float(float, float, float)> scalarField;
    float isovalue;
    if (fieldChoice == 1)
    {
        scalarField = [](float x, float y, float z) -> float
        {
            return y - sin(x) * cos(z);
        };
        isovalue = 0.0f;
    }
    else if (fieldChoice == 2)
    {
        scalarField = [](float x, float y, float z) -> float
        {
            return x * x - y * y - z * z - z;
        };
        isovalue = -1.5f;
    }
    else
    {
        std::cerr << "Invalid field choice. Use 1 or 2." << std::endl;
        return -1;
    }

    std::vector<float> meshVertices = marchingCubes(scalarField, isovalue, gridMin, gridMax, stepSize);
    std::vector<float> meshNormals = computeNormals(meshVertices);
    writePLY(meshVertices, meshNormals, "exercise1.ply");

    GLuint meshVAO, meshVBO;
    createMeshBuffers(meshVAO, meshVBO, meshVertices, meshNormals);
    GLsizei meshVertexCount = meshVertices.size() / 3;

    std::vector<glm::vec3> corners = {
        glm::vec3(gridMin, gridMin, gridMin),
        glm::vec3(gridMax, gridMin, gridMin),
        glm::vec3(gridMax, gridMax, gridMin),
        glm::vec3(gridMin, gridMax, gridMin),
        glm::vec3(gridMin, gridMin, gridMax),
        glm::vec3(gridMax, gridMin, gridMax),
        glm::vec3(gridMax, gridMax, gridMax),
        glm::vec3(gridMin, gridMax, gridMax)};
    std::vector<float> boxVertices = {
        corners[0].x, corners[0].y, corners[0].z, corners[1].x, corners[1].y, corners[1].z,
        corners[1].x, corners[1].y, corners[1].z, corners[2].x, corners[2].y, corners[2].z,
        corners[2].x, corners[2].y, corners[2].z, corners[3].x, corners[3].y, corners[3].z,
        corners[3].x, corners[3].y, corners[3].z, corners[0].x, corners[0].y, corners[0].z,

        corners[4].x, corners[4].y, corners[4].z, corners[5].x, corners[5].y, corners[5].z,
        corners[5].x, corners[5].y, corners[5].z, corners[6].x, corners[6].y, corners[6].z,
        corners[6].x, corners[6].y, corners[6].z, corners[7].x, corners[7].y, corners[7].z,
        corners[7].x, corners[7].y, corners[7].z, corners[4].x, corners[4].y, corners[4].z,

        corners[0].x, corners[0].y, corners[0].z, corners[4].x, corners[4].y, corners[4].z,
        corners[1].x, corners[1].y, corners[1].z, corners[5].x, corners[5].y, corners[5].z,
        corners[2].x, corners[2].y, corners[2].z, corners[6].x, corners[6].y, corners[6].z,
        corners[3].x, corners[3].y, corners[3].z, corners[7].x, corners[7].y, corners[7].z};
    GLuint boxVAO = createLineVAO(boxVertices);
    GLsizei boxVertexCount = boxVertices.size() / 3;

    std::vector<float> axesVertices = {

        gridMin, gridMin, gridMin, gridMax, gridMin, gridMin,

        gridMax, gridMin, gridMin, gridMax - 0.25f, gridMin, gridMin + 0.1f,
        gridMax, gridMin, gridMin, gridMax - 0.25f, gridMin, gridMin - 0.1f,

        gridMin, gridMin, gridMin, gridMin, gridMax, gridMin,

        gridMin, gridMax, gridMin, gridMin, gridMax - 0.25f, gridMin + 0.1f,
        gridMin, gridMax, gridMin, gridMin, gridMax - 0.25f, gridMin - 0.1f,

        gridMin, gridMin, gridMin, gridMin, gridMin, gridMax,

        gridMin, gridMin, gridMax, gridMin + 0.1f, gridMin, gridMax - 0.25f,
        gridMin, gridMin, gridMax, gridMin - 0.1f, gridMin, gridMax - 0.25f};
    GLuint axesVAO = createLineVAO(axesVertices);
    GLsizei axesVertexCount = axesVertices.size() / 3;

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 cameraPos = computeCameraPos();
        glm::mat4 V = glm::lookAt(cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 P = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glm::mat4 M = glm::mat4(1.0f);
        glm::mat4 MVP = P * V * M;

        glUseProgram(shaderProgram);

        glUniform3fv(glGetUniformLocation(shaderProgram, "cameraPos"), 1, glm::value_ptr(cameraPos));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "V"), 1, GL_FALSE, glm::value_ptr(V));
        glUniform3f(glGetUniformLocation(shaderProgram, "LightDir"), 10.0f, 10.0f, 10.0f);

        glUniform3f(glGetUniformLocation(shaderProgram, "modelColor"), 0.0f, 0.8f, 0.8f);
        glBindVertexArray(meshVAO);
        glDrawArrays(GL_TRIANGLES, 0, meshVertexCount);
        glBindVertexArray(0);

        glUseProgram(lineShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(lineShaderProgram, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3f(glGetUniformLocation(lineShaderProgram, "lineColor"), 1.0f, 1.0f, 1.0f);
        glBindVertexArray(boxVAO);
        glDrawArrays(GL_LINES, 0, boxVertexCount);
        glBindVertexArray(0);

        glUseProgram(0);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf(glm::value_ptr(P));

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf(glm::value_ptr(V * M));

        glDisable(GL_DEPTH_TEST);
        worldaxes.draw();
        glEnable(GL_DEPTH_TEST);

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &meshVAO);
    glDeleteBuffers(1, &meshVBO);
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(lineShaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}