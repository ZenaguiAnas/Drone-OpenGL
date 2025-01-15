#define STB_IMAGE_IMPLEMENTATION
#include <GL/glut.h>
#include "bin/Debug/AntTweakBar.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <cfloat>
#include <string>
#include "bin/Debug/stb_image.h"

// Existing camera settings
float cameraDistance = 10.0f;
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;

// New selection and manipulation settings
struct MeshInfo {
    bool isVisible = true;
    bool isSelected = false;
    GLenum displayMode = GL_FILL;  // GL_FILL, GL_LINE, GL_POINT
    float position[3] = {0.0f, 0.0f, 0.0f};
};

std::map<unsigned int, MeshInfo> meshInfoMap;
int selectedMeshIndex = -1;

// Model data
const aiScene* scene = nullptr;
Assimp::Importer importer;
std::string modelPath = "/home/bakr/Drone.obj";

// Texture variables
GLuint textureID;
std::string texturePath = "/home/bakr/Downloads/bmetal.jpg";

// Material properties
float materialColor[3] = {0.8f, 0.8f, 0.8f}; // RGB color
float materialShininess = 50.0f;  // Default shininess value

// Light sources (directional and point lights)
bool lightEnabled[3] = {true, true, false}; // Light 0 and 1 are enabled by default, Light 2 is disabled
float lightPosition[3][4] = {{1.0f, 1.0f, 1.0f, 0.0f},   // Directional light
                             {2.0f, 2.0f, 0.0f, 1.0f},   // Point light 1
                             {-2.0f, -2.0f, 0.0f, 1.0f}}; // Point light 2
float lightColor[3][3] = {{1.0f, 1.0f, 1.0f}, // Light 0 color (white)
                          {1.0f, 0.5f, 0.0f}, // Light 1 color (orange)
                          {0.0f, 0.0f, 1.0f}}; // Light 2 color (blue)

// AntTweakBar handle
TwBar* tweakBar;


// Mouse state tracking variables
bool isDragging = false;
int lastMouseX = 0;
int lastMouseY = 0;

bool showCollisionHighlights = true;

// Selection buffer
GLuint selectBuf[512];

// Lighting settings
void setupLighting() {
    GLfloat light_position[] = { 0.0f, 5.0f, 5.0f, 1.0f };
    GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
// Function prototypes
void toggleCollisionHighlights();
bool checkCollision(const aiMesh* mesh1, const aiMesh* mesh2);
void drawCollisionHighlight(const aiMesh* mesh);


int selectedObjectIndex = -1; // No object selected by default
bool animateSelectedObject = false;
float animationAngle = 0.0f; // Rotation angle
const float animationSpeed = 2.0f; // Speed of rotation (degrees per frame)
// Recursive rendering function
void renderNode(const aiNode* node, const aiScene* scene, bool selectMode = false) {
    aiMatrix4x4 transform = node->mTransformation;
    transform.Transpose();


void renderSelectedObject(const aiMesh* mesh) {
    glPushMatrix();

    // Rotate around the object's center
    if (animateSelectedObject) {
        glRotatef(animationAngle, 0.0f, 1.0f, 0.0f);
    }

    glColor3f(0.5f, 0.8f, 1.0f); // Highlight color for the selected object
    glBegin(GL_TRIANGLES);
    for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
        aiFace face = mesh->mFaces[j];
        for (unsigned int k = 0; k < face.mNumIndices; k++) {
            unsigned int index = face.mIndices[k];
            if (mesh->HasNormals()) {
                aiVector3D normal = mesh->mNormals[index];
                glNormal3f(normal.x, normal.y, normal.z);
            }
            if (mesh->HasTextureCoords(0)) {
                aiVector3D texCoord = mesh->mTextureCoords[0][index];
                glTexCoord2f(texCoord.x, texCoord.y);
            }
            aiVector3D vertex = mesh->mVertices[index];
            glVertex3f(vertex.x, vertex.y, vertex.z);
        }
    }
    glEnd();

    glPopMatrix();
}

// Update animation angle
void updateAnimation(int value) {
    if (animateSelectedObject) {
        animationAngle += animationSpeed;
        if (animationAngle >= 360.0f) {
            animationAngle -= 360.0f; // Keep the angle within [0, 360]
        }
    }
    glutPostRedisplay(); // Request a redraw
    glutTimerFunc(16, updateAnimation, 0); // ~60 FPS (16 ms interval)
}

// Function to toggle object selection
void toggleObjectSelection(int objectIndex) {
    if (selectedObjectIndex == objectIndex) {
        selectedObjectIndex = -1; // Deselect
    } else {
        selectedObjectIndex = objectIndex; // Select object
    }
    glutPostRedisplay();
}

// Function to render and animate a selected object

// Helper function to calculate bounding boxes
void calculateBoundingBox(const aiMesh* mesh, aiVector3D& min, aiVector3D& max) {
    min = aiVector3D(FLT_MAX, FLT_MAX, FLT_MAX);
    max = aiVector3D(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        aiVector3D vertex = mesh->mVertices[i];
        min.x = std::min(min.x, vertex.x);
        min.y = std::min(min.y, vertex.y);
        min.z = std::min(min.z, vertex.z);
        max.x = std::max(max.x, vertex.x);
        max.y = std::max(max.y, vertex.y);
        max.z = std::max(max.z, vertex.z);
    }
}

// Collision detection
bool checkCollision(const aiMesh* mesh1, const aiMesh* mesh2) {
    aiVector3D min1, max1, min2, max2;
    calculateBoundingBox(mesh1, min1, max1);
    calculateBoundingBox(mesh2, min2, max2);

    return (min1.x <= max2.x && max1.x >= min2.x) &&
           (min1.y <= max2.y && max1.y >= min2.y) &&
           (min1.z <= max2.z && max1.z >= min2.z);
}

// Toggle collision highlights
void toggleCollisionHighlights() {
    showCollisionHighlights = !showCollisionHighlights;
    glutPostRedisplay();
}

// Render collision highlights
void drawCollisionHighlight(const aiMesh* mesh) {
    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINES);
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            unsigned int index1 = face.mIndices[j];
            unsigned int index2 = face.mIndices[(j + 1) % face.mNumIndices];

            aiVector3D vertex1 = mesh->mVertices[index1];
            aiVector3D vertex2 = mesh->mVertices[index2];

            glVertex3f(vertex1.x, vertex1.y, vertex1.z);
            glVertex3f(vertex2.x, vertex2.y, vertex2.z);
        }
    }
    glEnd();
}

// Function to load the model and calculate bounding box
float calculateInitialDistance(const aiScene* scene) {
    aiVector3D min(FLT_MAX, FLT_MAX, FLT_MAX);
    aiVector3D max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            aiVector3D vertex = mesh->mVertices[j];
            min.x = std::min(min.x, vertex.x);
            min.y = std::min(min.y, vertex.y);
            min.z = std::min(min.z, vertex.z);
            max.x = std::max(max.x, vertex.x);
            max.y = std::max(max.y, vertex.y);
            max.z = std::max(max.z, vertex.z);
        }
    }

    aiVector3D size = max - min;
    return std::max({size.x, size.y, size.z}) * 2.0f; // Set distance based on model size
}

void loadModel(const std::string& path) {
    scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Model loaded successfully: " << path << std::endl;
    cameraDistance = calculateInitialDistance(scene); // Adjust camera distance
}

// Function to load a texture using stb_image
GLuint loadTexture(const std::string& path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        exit(EXIT_FAILURE);
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, (channels == 4 ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texID;
}


// Updated renderNode function to apply textures
void renderNode(const aiNode* node, const aiScene* scene, int nodeIndex = 0) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        unsigned int meshID = node->mMeshes[i];

        if (!meshInfoMap.count(meshID)) {
            meshInfoMap[meshID] = MeshInfo();
        }

        if (!meshInfoMap[meshID].isVisible) {
            continue;
        }

        if (selectMode) {
            glLoadName(meshID);
        }

        glPushMatrix();
        glTranslatef(meshInfoMap[meshID].position[0],
                     meshInfoMap[meshID].position[1],
                     meshInfoMap[meshID].position[2]);

        glPolygonMode(GL_FRONT_AND_BACK, meshInfoMap[meshID].displayMode);

        if (!selectMode) {
            if (meshInfoMap[meshID].isSelected) {
                glColor3f(1.0f, 0.5f, 0.0f);
            } else {
                glColor3f(0.8f, 0.8f, 0.8f);
            }
        }
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        // Render selected object in isolation
        if (selectedObjectIndex == nodeIndex) {
            renderSelectedObject(mesh);
        } else if (selectedObjectIndex == -1) { // Render all objects if no selection
            glColor3f(materialColor[0], materialColor[1], materialColor[2]);

            glEnable(GL_TEXTURE_2D); // Enable texturing
            glBindTexture(GL_TEXTURE_2D, textureID);

            glBegin(GL_TRIANGLES);
            for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    unsigned int index = face.mIndices[k];
                    if (mesh->HasNormals()) {
                        aiVector3D normal = mesh->mNormals[index];
                        glNormal3f(normal.x, normal.y, normal.z);
                    }
                    if (mesh->HasTextureCoords(0)) {
                        aiVector3D texCoord = mesh->mTextureCoords[0][index];
                        glTexCoord2f(texCoord.x, texCoord.y);
                    }
                    aiVector3D vertex = mesh->mVertices[index];
                    glVertex3f(vertex.x, vertex.y, vertex.z);
                }
            }
            glEnd();

            glDisable(GL_TEXTURE_2D); // Disable texturing after use

            // Highlight collisions
            if (showCollisionHighlights) {
                for (unsigned int j = 0; j < scene->mNumMeshes; ++j) {
                    if (mesh != scene->mMeshes[j] && checkCollision(mesh, scene->mMeshes[j])) {
                        drawCollisionHighlight(mesh);
                    }
                }
            }
        }
        glBegin(GL_TRIANGLES);
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            const aiFace& face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                int index = face.mIndices[k];
                if (mesh->HasNormals()) {
                    glNormal3fv(&mesh->mNormals[index].x);
                }
                if (mesh->HasTextureCoords(0)) {
                    glTexCoord2fv(&mesh->mTextureCoords[0][index].x);
                }
                glVertex3fv(&mesh->mVertices[index].x);
            }
        }
        glEnd();

        glPopMatrix();
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        renderNode(node->mChildren[i], scene, selectMode);
        renderNode(node->mChildren[i], scene, i);
    }
}


// Initialize OpenGL settings
void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat materialSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);

    // Load texture
    textureID = loadTexture(texturePath);
}

// Initialize AntTweakBar
void initTweakBar() {
    TwInit(TW_OPENGL, nullptr);
    tweakBar = TwNewBar("Material and Lighting");

    // Material properties
    TwAddVarRW(tweakBar, "Color", TW_TYPE_COLOR3F, &materialColor, " label='Material Color' ");
    TwAddVarRW(tweakBar, "Shininess", TW_TYPE_FLOAT, &materialShininess, " label='Material Shininess' min=0 max=128 step=1 ");

    // Light controls
    TwAddVarRW(tweakBar, "Light 0", TW_TYPE_BOOL32, &lightEnabled[0], " label='Directional Light' ");
    TwAddVarRW(tweakBar, "Light 1", TW_TYPE_BOOL32, &lightEnabled[1], " label='Point Light 1' ");
    TwAddVarRW(tweakBar, "Light 2", TW_TYPE_BOOL32, &lightEnabled[2], " label='Point Light 2' ");
    TwAddVarRW(tweakBar, "Highlight Collisions", TW_TYPE_BOOL32, &showCollisionHighlights, " label='Highlight Collisions' ");

}

// Set light properties
void setLights() {
    // Light 0 (Directional Light)
    if (lightEnabled[0]) {
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition[0]);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor[0]);
    } else {
        glDisable(GL_LIGHT0);
    }

    // Light 1 (Point Light 1)
    if (lightEnabled[1]) {
        glEnable(GL_LIGHT1);
        glLightfv(GL_LIGHT1, GL_POSITION, lightPosition[1]);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor[1]);
    } else {
        glDisable(GL_LIGHT1);
    }

    // Light 2 (Point Light 2)
    if (lightEnabled[2]) {
        glEnable(GL_LIGHT2);
        glLightfv(GL_LIGHT2, GL_POSITION, lightPosition[2]);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, lightColor[2]);
    } else {
        glDisable(GL_LIGHT2);
    }
}

// Process selection using OpenGL's selection mechanism
void processSelection(int x, int y) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glSelectBuffer(512, selectBuf);
    glRenderMode(GL_SELECT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    gluPickMatrix(x, viewport[3] - y, 5.0, 5.0, viewport);
    gluPerspective(45.0, (double)viewport[2] / viewport[3], 1.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(cameraDistance * sin(cameraAngleY) * cos(cameraAngleX),
              cameraDistance * sin(cameraAngleX),
              cameraDistance * cos(cameraAngleY) * cos(cameraAngleX),
              cameraPosX, cameraPosY, 0.0f,
              0.0f, 1.0f, 0.0f);

    glInitNames();
    glPushName(0);

    renderNode(scene->mRootNode, scene, true);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    GLint hits = glRenderMode(GL_RENDER);

    if (hits > 0) {
        selectedMeshIndex = selectBuf[3];
        meshInfoMap[selectedMeshIndex].isSelected = true;
    } else {
        selectedMeshIndex = -1;
    }
}

// Initialize OpenGL and Assimp
bool initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);
    if (!scene) {
        std::cerr << "Failed to load model: " << importer.GetErrorString() << std::endl;
        return false;
    }

    setupLighting();
    return true;
}

// Display callback
// Render scene
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Set camera position
    glTranslatef(0.0f, 0.0f, -cameraDistance);
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);
    glTranslatef(-cameraPosX, -cameraPosY, 0.0f);

    // Set lights
    setLights();
    // Set camera position
    gluLookAt(cameraDistance * sin(cameraAngleY) * cos(cameraAngleX),
              cameraDistance * sin(cameraAngleX),
              cameraDistance * cos(cameraAngleY) * cos(cameraAngleX),
              cameraPosX, cameraPosY, 0.0f,
              0.0f, 1.0f, 0.0f);

    // Update material properties based on the tweak bar values
    GLfloat materialShininessValue[] = {materialShininess};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialShininessValue);

    // Set the diffuse material color
    glColor3f(materialColor[0], materialColor[1], materialColor[2]);

    // Render the model
    if (scene && scene->mRootNode) {
        renderNode(scene->mRootNode, scene);
    }

    // Draw AntTweakBar
    TwDraw();

    glutSwapBuffers();
}

// Window reshape callback
void reshape(int w, int h) {
    if (h == 0) h = 1;  // Prevent division by zero

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

// Mouse button callback
void mouseButton(int button, int state, int x, int y) {
    static int lastX = 0, lastY = 0;

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            processSelection(x, y);
            lastX = x;
            lastY = y;
        }
    }
}

// Mouse motion callback
// Handle mouse motion for AntTweakBar and camera
void mouseMotion(int x, int y) {
    if (!TwEventMouseMotionGLUT(x, y) && isDragging) {
        cameraAngleY += (x - lastMouseX) * 0.2f;
        cameraAngleX += (y - lastMouseY) * 0.2f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
    static int lastX = 0, lastY = 0;

    int dx = x - lastX;
    int dy = y - lastY;

    if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
        // Zoom with Ctrl + drag
        cameraDistance -= dy * 0.1f;
        if (cameraDistance < 1.0f) cameraDistance = 1.0f;
    } else {
        // Rotate camera
        cameraAngleY += dx * 0.01f;
        cameraAngleX += dy * 0.01f;

        // Limit vertical rotation
        if (cameraAngleX > 1.5f) cameraAngleX = 1.5f;
        if (cameraAngleX < -1.5f) cameraAngleX = -1.5f;
    }

    lastX = x;
    lastY = y;
    glutPostRedisplay();
}

// Handle mouse events
void mouse(int button, int state, int x, int y) {
    if (!TwEventMouseButtonGLUT(button, state, x, y)) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            isDragging = true;
            lastMouseX = x;
            lastMouseY = y;
        } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
            isDragging = false;
        } else if (button == GLUT_RIGHT_BUTTON) {
            cameraDistance += (state == GLUT_DOWN) ? -0.5f : 0.5f;
            if (cameraDistance < 1.0f) cameraDistance = 1.0f;
            glutPostRedisplay();
        }
    }
}

// Handle keyboard inputs
// Keyboard callback
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        // Camera movement
        case 'w': cameraPosY += 0.1f; break;
        case 's': cameraPosY -= 0.1f; break;
        case 'a': cameraPosX -= 0.1f; break;
        case 'd': cameraPosX += 0.1f; break;

        // Object manipulation
        case 'h':  // Toggle visibility
            if (selectedMeshIndex >= 0) {
                meshInfoMap[selectedMeshIndex].isVisible =
                    !meshInfoMap[selectedMeshIndex].isVisible;
            }
            break;

        case 'm':  // Cycle display modes
            if (selectedMeshIndex >= 0) {
                auto& mode = meshInfoMap[selectedMeshIndex].displayMode;
                if (mode == GL_FILL) mode = GL_LINE;
                else if (mode == GL_LINE) mode = GL_POINT;
                else mode = GL_FILL;
            }
            break;

        // Selected object movement
        case 'i':  // Up
            if (selectedMeshIndex >= 0)
                meshInfoMap[selectedMeshIndex].position[1] += 0.1f;
            break;
        case 'k':  // Down
            if (selectedMeshIndex >= 0)
                meshInfoMap[selectedMeshIndex].position[1] -= 0.1f;
            break;
        case 'j':  // Left
            if (selectedMeshIndex >= 0)
                meshInfoMap[selectedMeshIndex].position[0] -= 0.1f;
            break;
        case 'l':  // Right
            if (selectedMeshIndex >= 0)
                meshInfoMap[selectedMeshIndex].position[0] += 0.1f;
            break;

        case 27:  // ESC key
            exit(0);
            break;
    }

    glutPostRedisplay();
    if (!TwEventKeyboardGLUT(key, x, y)) {
        switch (key) {
            case 'w': cameraPosY += 0.1f; break;
            case 's': cameraPosY -= 0.1f; break;
            case 'a': cameraPosX -= 0.1f; break;
            case 'd': cameraPosX += 0.1f; break;
            case '1': // Select first object
                toggleObjectSelection(0); // Select the first object
                break;
            case '2': // Select second object
                toggleObjectSelection(1); // Select the second object
                break;
            case '3': // Select third object
                toggleObjectSelection(2); // Select the third object
                break;
            case '4': // Select fourth object
                toggleObjectSelection(3); // Select the fourth object
                break;
            case '5': // Select fifth object
                toggleObjectSelection(4); // Select the fifth object
                break;
            case '6': // Select fourth object
                toggleObjectSelection(5); // Select the fourth object
                break;
            case '7': // Select fifth object
                toggleObjectSelection(6); // Select the fifth object
                break;
            case '8': // Select fifth object
                toggleObjectSelection(7); // Select the fifth object
                break;
            case '9': // Select fifth object
                toggleObjectSelection(8); // Select the fifth object
                break;
            case 'l': // Toggle animation
                animateSelectedObject = !animateSelectedObject;
                break;
            case 'r': // Reset camera
                cameraAngleX = 0.0f;
                cameraAngleY = 0.0f;
                cameraDistance = 5.0f;
                cameraPosX = 0.0f;
                cameraPosY = 0.0f;
                break;
            case 27: // Escape key
                TwTerminate();
                exit(0);
        }
        glutPostRedisplay();
    }
}

// Main function and setup

void initAnimation() {
    glutTimerFunc(16, updateAnimation, 0); // Start animation timer
}

// Handle window resize
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    TwWindowSize(w, h);
}

int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Drone Viewer with Material Editor and Lights");

    // Initialize OpenGL
    initOpenGL();

    // Initialize AntTweakBar
    initTweakBar();

    // Load the drone model
    loadModel(modelPath);

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);

    initAnimation();

    // Start main loop
    glutMainLoop();
    TwTerminate();
    return 0;
}
