#include <GL/glut.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

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
const std::string modelPath = "/home/bakr/Drone.obj";
const aiScene* scene = nullptr;
Assimp::Importer importer;

// Selection buffer
GLuint selectBuf[512];

// Lighting settings
void setupLighting() {
    GLfloat light_position[] = { 0.0f, 5.0f, 5.0f, 1.0f };
    GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
}

// Recursive rendering function
void renderNode(const aiNode* node, const aiScene* scene, bool selectMode = false) {
    aiMatrix4x4 transform = node->mTransformation;
    transform.Transpose();

    glPushMatrix();
    glMultMatrixf(reinterpret_cast<float*>(&transform));

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
    }

    glPopMatrix();
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
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Set camera position
    gluLookAt(cameraDistance * sin(cameraAngleY) * cos(cameraAngleX),
              cameraDistance * sin(cameraAngleX),
              cameraDistance * cos(cameraAngleY) * cos(cameraAngleX),
              cameraPosX, cameraPosY, 0.0f,
              0.0f, 1.0f, 0.0f);

    if (scene) {
        renderNode(scene->mRootNode, scene);
    }

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
void mouseMotion(int x, int y) {
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
}

// Main function and setup
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Drone Viewer");

    if (!initialize()) {
        return -1;
    }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
