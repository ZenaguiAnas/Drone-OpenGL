#include <GL/glut.h>
#include <AntTweakBar.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <cfloat>

// Camera parameters
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
float cameraDistance = 5.0f;
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;

// Model data
const aiScene* scene = nullptr;
Assimp::Importer importer;
std::string modelPath = "C:\\Users\\hp\\Downloads\\Drone.obj";

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

// Recursive function to render the model
void renderNode(const aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        // Apply material color to the model
        glColor3f(materialColor[0], materialColor[1], materialColor[2]);

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
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        renderNode(node->mChildren[i], scene);
    }
}

// Initialize OpenGL settings
void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    // Set initial material properties
    GLfloat materialSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};  // White specular highlight
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
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

// Handle mouse motion for AntTweakBar and camera
void mouseMotion(int x, int y) {
    if (!TwEventMouseMotionGLUT(x, y) && isDragging) {
        cameraAngleY += (x - lastMouseX) * 0.2f;
        cameraAngleX += (y - lastMouseY) * 0.2f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
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
void keyboard(unsigned char key, int x, int y) {
    if (!TwEventKeyboardGLUT(key, x, y)) {
        switch (key) {
            case 'w': cameraPosY += 0.1f; break;
            case 's': cameraPosY -= 0.1f; break;
            case 'a': cameraPosX -= 0.1f; break;
            case 'd': cameraPosX += 0.1f; break;
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

    // Start main loop
    glutMainLoop();
    return 0;
}
