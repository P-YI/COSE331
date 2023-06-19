#include "scene.h"
#include "binary/teapot.h"
#include "binary/rgb.h"
#include "binary/cloud.h"
#include "binary/tex_flower.h"
#include "checker.h"

Shader* Scene::vertexShader = nullptr;
Shader* Scene::fragmentShader = nullptr;
Program* Scene::program = nullptr;
Camera* Scene::camera = nullptr;
Object* Scene::teapot = nullptr;
Texture* Scene::diffuse = nullptr;
Texture* Scene::dissolve = nullptr;
Material* Scene::material = nullptr;
Light* Scene::light = nullptr;

int Scene::width = 0;
int Scene::height = 0;

// Arcball variables
float lastMouseX = 0, lastMouseY = 0;
float currentMouseX = 0, currentMouseY = 0;

void Scene::setup(AAssetManager* aAssetManager) {
    Asset::setManager(aAssetManager);

    Scene::vertexShader = new Shader(GL_VERTEX_SHADER, "vertex.glsl");
    Scene::fragmentShader = new Shader(GL_FRAGMENT_SHADER, "fragment.glsl");

    Scene::program = new Program(Scene::vertexShader, Scene::fragmentShader);

    Scene::camera = new Camera(Scene::program);
    Scene::camera->eye = vec3(20.0f, 30.0f, 20.0f);

    Scene::light = new Light(program);
    Scene::light->position = vec3(15.0f, 15.0f, 0.0f);

    //////////////////////////////
    /* TODO: Problem 2 : Change the texture of the teapot
     *  Modify and fill in the lines below.
     */

    for (Texel &texel : rgbTexels) {
        unsigned char temp = texel.green;
        texel.green = texel.blue;
        texel.blue = texel.red;
        texel.red = temp;
    }

    Scene::diffuse  = new Texture(Scene::program, 0, "textureDiff", rgbTexels, rgbSize);
    //////////////////////////////

    Scene::material = new Material(Scene::program, diffuse, dissolve);
    Scene::teapot = new Object(program, material, teapotVertices, teapotIndices);
}

void Scene::screen(int width, int height) {
    Scene::camera->aspect = (float) width/height;
    Scene::width = width;
    Scene::height = height;
}

void Scene::update(float deltaTime) {
    static float time = 0.0f;

    Scene::program->use();

    Scene::camera->update();
    Scene::light->update();

    Scene::teapot->draw();

    time += deltaTime;
}

void Scene::mouseDownEvents(float x, float y) {
    lastMouseX = currentMouseX = x;
    lastMouseY = currentMouseY = y;
}

void Scene::mouseMoveEvents(float x, float y) {
    //////////////////////////////
    /* TODO: Problem 3 : Implement Phong lighting
     *  Fill in the lines below.
     */

    currentMouseX = x;
    currentMouseY = y;

    auto get_arcball_vector = [](float x, float y, int screen_width, int screen_height)
            -> glm::vec3 {
        glm::vec3 P = glm::vec3(1.0f * x / screen_width * 2 - 1.0f, 1.0f * y / screen_height * 2 - 1.0f, 0);
        P.y = -P.y;
        float OP_squared = P.x * P.x + P.y * P.y;
        if (OP_squared <= 1.0f)
            P.z = sqrt(1.0f - OP_squared);
        else
            P = glm::normalize(P);
        return P;
    };

    glm::vec3 v_curr = get_arcball_vector(x, y, Scene::width, Scene::height);
    glm::vec3 v_last = get_arcball_vector(lastMouseX, lastMouseY, Scene::width, Scene::height);

    glm::mat4 viewMat = glm::lookAt(Scene::camera->eye, Scene::camera->at, Scene::camera->up);
    glm::mat4 projMat = glm::perspective(glm::radians(Scene::camera->fovy), Scene::camera->aspect,
                                         Scene::camera->zNear, Scene::camera->zFar);
    glm::mat4 axis_inv = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    projMat = axis_inv * projMat;

    glm::vec3 rot_axis = glm::cross(v_last, v_curr);
    float angle = glm::acos(glm::dot(v_last, v_curr));
    glm::mat4 rotate = glm::rotate(angle, rot_axis);

    Scene::light->position = glm::vec3(glm::inverse(viewMat) *
                                        glm::inverse(projMat) * rotate * projMat * viewMat *
                                        glm::vec4(Scene::light->position, 1.0f));

    lastMouseX = currentMouseX;
    lastMouseY = currentMouseY;

    //////////////////////////////
}