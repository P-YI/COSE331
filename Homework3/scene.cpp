#include "scene.h"
#include "binary/animation.h"
#include "binary/skeleton.h"
#include "binary/player.h"

Shader* Scene::vertexShader = nullptr;
Shader* Scene::fragmentShader = nullptr;
Program* Scene::program = nullptr;
Camera* Scene::camera = nullptr;
Object* Scene::player = nullptr;
Texture* Scene::diffuse = nullptr;
Material* Scene::material = nullptr;
Object* Scene::lineDraw = nullptr;
Texture* Scene::lineColor = nullptr;
Material* Scene::lineMaterial = nullptr;

bool Scene::upperFlag = true;
bool Scene::lowerFlag = true;

void Scene::setup(AAssetManager* aAssetManager) {
    Asset::setManager(aAssetManager);

    Scene::vertexShader = new Shader(GL_VERTEX_SHADER, "vertex.glsl");
    Scene::fragmentShader = new Shader(GL_FRAGMENT_SHADER, "fragment.glsl");

    Scene::program = new Program(Scene::vertexShader, Scene::fragmentShader);

    Scene::camera = new Camera(Scene::program);
    Scene::camera->eye = vec3(0.0f, 0.0f, 80.0f);

    Scene::diffuse = new Texture(Scene::program, 0, "textureDiff", playerTexels, playerSize);
    Scene::material = new Material(Scene::program, diffuse);
    Scene::player = new Object(program, material, playerVertices, playerIndices);
    player->worldMat = scale(vec3(1.0f / 3.0f));

    Scene::lineColor = new Texture(Scene::program, 0, "textureDiff", {{0xFF, 0x00, 0x00}}, 1);
    Scene::lineMaterial = new Material(Scene::program, lineColor);
    Scene::lineDraw = new Object(program, lineMaterial, {{}}, {{}}, GL_LINES);
}

void Scene::screen(int width, int height) {
    Scene::camera->aspect = (float) width/height;
}

void Scene::update(float deltaTime) {
    Scene::program->use();
    Scene::camera->update();

    /*
     *
     * Write your code.
     *
     */

    static float uTime = 0.0f;
    static float lTime = 0.0f;
    static vector<Vertex> animatedP = playerVertices;
    float inter_rate = lTime - (int)lTime;
    vector<float> curr_motion = motions[(int)lTime];
    vector<float> next_motion = motions[((int)lTime + 1) % 4];
    vector<mat4> M_i_a, M_i_p, M_i_d;
    vector<mat4> matrix_palette;


    if (Scene::upperFlag) {uTime = fmod((uTime + deltaTime), 4.0f);}
    if (Scene::lowerFlag) {lTime = fmod((lTime + deltaTime), 4.0f);}

    for (vec3 offset: jOffsets)
        {M_i_p.push_back(glm::translate(offset));}

    M_i_d.push_back(inverse(M_i_p[0]));
    for (int i = 1; i < jNames.size(); i++)
        {M_i_d.push_back(glm::inverse(M_i_p[i]) * M_i_d[jParents[i]]);}


    mat4 translate = glm::translate(mix(vec3(curr_motion[0], curr_motion[1], curr_motion[2]),
                                        vec3(next_motion[0], next_motion[1], next_motion[2]),
                                        inter_rate));

    quat curr_rotation = glm::quat_cast(rotate(radians(curr_motion[5]), vec3(0.0f, 0.0f, 1.0f))
                                        * rotate(radians(curr_motion[3]), vec3(1.0f, 0.0f, 0.0f))
                                        * rotate(radians(curr_motion[4]), vec3(0.0f, 1.0f, 0.0f)));

    quat next_rotation = glm::quat_cast(rotate(radians(next_motion[5]), vec3(0.0f, 0.0f, 1.0f))
                                        * rotate(radians(next_motion[3]), vec3(1.0f, 0.0f, 0.0f))
                                        * rotate(radians(next_motion[4]), vec3(0.0f, 1.0f, 0.0f)));

    mat4 rotation = glm::mat4_cast(slerp(curr_rotation, next_rotation, inter_rate));
    M_i_a.push_back(M_i_p[0] * translate * rotation);


    for (int i = 1; i < jNames.size(); i++) {
        float time = (i <= 11 ? lTime : uTime);
        inter_rate = time - (int)time;
        int currTime = (int)time;
        curr_motion = motions[currTime];
        next_motion = motions[(currTime + 1) % 4];

        vec3 curr_rot_angles = vec3(radians(curr_motion[i*3 + 3]),
                                  radians(curr_motion[i*3 + 4]),
                                  radians(curr_motion[i*3 + 5]));
        vec3 next_rot_angles = vec3(radians(next_motion[i*3 + 3]),
                                  radians(next_motion[i*3 + 4]),
                                  radians(next_motion[i*3 + 5]));

        mat4 currR = glm::rotate(curr_rot_angles.z, vec3(0.0f, 0.0f, 1.0f))
                                * rotate(curr_rot_angles.x, vec3(1.0f, 0.0f, 0.0f))
                                * rotate(curr_rot_angles.y, vec3(0.0f, 1.0f, 0.0f));

        mat4 nextR = glm::rotate(next_rot_angles.z, vec3(0.0f, 0.0f, 1.0f))
                                * rotate(next_rot_angles.x, vec3(1.0f, 0.0f, 0.0f))
                                * rotate(next_rot_angles.y, vec3(0.0f, 1.0f, 0.0f));

        curr_rotation = glm::quat_cast(currR);
        next_rotation = glm::quat_cast(nextR);

        rotation = glm::mat4_cast(slerp(curr_rotation, next_rotation, inter_rate));
        M_i_a.push_back(M_i_a[jParents[i]] * M_i_p[i] * rotation);
    }


    for (int i = 0; i < jNames.size(); i++) {matrix_palette.push_back(M_i_a[i] * M_i_d[i]);}


    for (int i = 0; i < playerVertices.size(); i++) {
        vec3 pos = animatedP[i].pos;
        vec4 weight = animatedP[i].weight;
        ivec4 bone = animatedP[i].bone;
        mat4 M = mat4(0.0f);

        for (int j = 0; j < 4; j++) {
            if (bone[j] == -1)
                continue;
            M += weight[j] * matrix_palette[bone[j]];
        }

        playerVertices[i].pos = vec3(M * vec4(pos, 1.0f));
    }

    // Line Drawer
    // glLineWidth(20);
    // Scene::lineDraw->load({{vec3(-20.0f, 0.0f, 0.0f)}, {vec3(20.0f, 0.0f, 0.0f)}}, {0, 1});
    // Scene::lineDraw->draw();

    Scene::player->load(playerVertices, playerIndices);
    Scene::player->draw();
}

void Scene::setUpperFlag(bool flag)
{
    Scene::upperFlag = flag;
}

void Scene::setLowerFlag(bool flag)
{
    Scene::lowerFlag = flag;
}