#include <GL/glut.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <string>

// Camera settings
float cameraDistance = 5.0f;
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;

// Model data
const std::string modelPath = "C:\\Users\\hp\\Downloads\\DroneProject\\Drone.obj";
const aiScene* scene = nullptr;
Assimp::Importer importer;

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

// Recursive function to render the model
void renderNode(const aiNode* node, const aiScene* scene) {
    aiMatrix4x4 transform = node->mTransformation;
    transform.Transpose();

    glPushMatrix();
    glMultMatrixf(reinterpret_cast<float*>(&transform));

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

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
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        renderNode(node->mChildren[i], scene);
    }

    glPopMatrix();
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera transformations
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

// Reshape callback
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Mouse interaction
int lastMouseX, lastMouseY;
void mouseMotion(int x, int y) {
    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
        cameraDistance -= dy * 0.1f;
        if (cameraDistance < 1.0f) cameraDistance = 1.0f;
    } else {
        cameraAngleY += dx * 0.01f;
        cameraAngleX += dy * 0.01f;
    }

    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        lastMouseX = x;
        lastMouseY = y;
    }
}

// Keyboard interaction
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'w': cameraPosY += 0.1f; break;
        case 's': cameraPosY -= 0.1f; break;
        case 'a': cameraPosX -= 0.1f; break;
        case 'd': cameraPosX += 0.1f; break;
    }
    glutPostRedisplay();
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
