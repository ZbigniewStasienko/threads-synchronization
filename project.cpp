#include <GLFW/glfw3.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <time.h>
#include <chrono>
#include <stdio.h>

using namespace std;

struct ThreadData {
    float speed;
    float x;
    float y;
    float direction;
    bool active;
    bool finished;
    pthread_t threadId;
    float color[3];

    ThreadData(float s)
        : speed(s), x(-0.9f), y(0.0f), direction(0.0f), active(true), finished(false) {
        color[0] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        color[1] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        color[2] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
};



float colorSquare[3][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {1.0f, 0.5f, 0.0f}
};

int currentColorIndex = 0;
pthread_mutex_t colorMutex;
bool running = true;

vector<shared_ptr<ThreadData>> threadsData;
pthread_mutex_t threadMutex;

void* threadFunction(void* arg) {
    auto data = *static_cast<shared_ptr<ThreadData>*>(arg);
    bool reachedCenter = false;
    bool reachedStand = false;

    while (data->active) {
        pthread_mutex_lock(&threadMutex);
        data->x += data->speed;
        data->y += data->direction;
        if (!reachedCenter) {
            if (data->x >= 0.0f) {
                reachedCenter = true;
                pthread_mutex_lock(&colorMutex);
                int targetColorIndex = currentColorIndex;
                pthread_mutex_unlock(&colorMutex);
                if (targetColorIndex == 0) {
                    data->direction = (data->speed)/2;
                } else if (targetColorIndex == 1) {
                    data->direction = -(data->speed)/2;
                }
            }
        }
        if (!reachedStand) {
            if (data->x >= 0.95f) {
                reachedStand = true;
                data->direction = 0.0f;
                pthread_mutex_unlock(&threadMutex);
                usleep(1000000);
                pthread_mutex_lock(&threadMutex);
            }
        }
        if (data->x >= 1.0f) {
            data->active = false;
        }
        pthread_mutex_unlock(&threadMutex);
        for (int i = 0; i < 10; ++i) {
            if (!data->active) break;
            usleep(5000);
        }
    }
    pthread_mutex_lock(&threadMutex);
    data->finished = true;
    pthread_mutex_unlock(&threadMutex);
    return NULL;
}

void* colorChangeFunction(void* arg) {
    int changeColorTime = 2;
    while (running) {
        pthread_mutex_lock(&colorMutex);
        currentColorIndex = (currentColorIndex + 1) % 3;
        pthread_mutex_unlock(&colorMutex);
        sleep(changeColorTime);
    }
    return NULL;
}

void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLineWidth(2.0f);

    pthread_mutex_lock(&threadMutex);
    for (const auto& data : threadsData) {
        if (data->active) {
            glColor3fv(data->color);
            glBegin(GL_LINES);
            glVertex2f(data->x, data->y - 0.05f);
            glVertex2f(data->x, data->y + 0.05f);
            glEnd();
        }
    }
    pthread_mutex_unlock(&threadMutex);

    pthread_mutex_lock(&colorMutex);
    glColor3f(colorSquare[currentColorIndex][0], colorSquare[currentColorIndex][1], colorSquare[currentColorIndex][2]);
    pthread_mutex_unlock(&colorMutex);

    glLineWidth(4.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, -0.1f);
    glVertex2f(0.0f, 0.1f);
    glEnd();

    float halfSide = 0.035f;

    glColor3f(1.0f, 0.0f, 0.0f);
    float x = 1.0f;
    float y = 0.475f;
    glBegin(GL_QUADS);
    glVertex2f(x - halfSide, y + halfSide);
    glVertex2f(x + halfSide, y + halfSide);
    glVertex2f(x + halfSide, y - halfSide);
    glVertex2f(x - halfSide, y - halfSide);
    glEnd();

    glColor3f(1.0f, 0.5f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x - halfSide, 0.0f + halfSide);
    glVertex2f(x + halfSide, 0.0f + halfSide);
    glVertex2f(x + halfSide, 0.0f - halfSide);
    glVertex2f(x - halfSide, 0.0f - halfSide);
    glEnd();

    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x - halfSide, -y + halfSide);
    glVertex2f(x + halfSide, -y + halfSide);
    glVertex2f(x + halfSide, -y - halfSide);
    glVertex2f(x - halfSide, -y - halfSide);
    glEnd();

    glColor3f(1.0, 1.0, 1.0);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

int main() {
    srand(static_cast<unsigned int>(time(0)));
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(700, 480, "Threads Visualization", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, keyCallback);
    glfwMakeContextCurrent(window);
    glOrtho(-1.0, 1.2, -1.0, 1.0, -1.0, 1.0);

    pthread_mutex_init(&threadMutex, NULL);
    pthread_mutex_init(&colorMutex, NULL);

    pthread_t colorChangeThread;
    pthread_create(&colorChangeThread, NULL, colorChangeFunction, NULL);

    auto lastCreationTime = chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
        auto currentTime = chrono::steady_clock::now();
        float elapsed = chrono::duration<float>(currentTime - lastCreationTime).count();
        float newThreadIncidence = 1.5f;
        if (elapsed >= newThreadIncidence) {
            float speed = (float)(rand() % 4 + 1) / 150.0f;
            auto data = make_shared<ThreadData>(speed);
            pthread_create(&data->threadId, NULL, threadFunction, &data);
            threadsData.push_back(data);
            lastCreationTime = currentTime;
        }
        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();

        pthread_mutex_lock(&threadMutex);
        auto it = remove_if(threadsData.begin(), threadsData.end(), [](const shared_ptr<ThreadData>& thread) {
            return thread->finished;
        });
        threadsData.erase(it, threadsData.end());
        pthread_mutex_unlock(&threadMutex);
    }

    running = false;
    pthread_join(colorChangeThread, NULL);

    pthread_mutex_lock(&threadMutex);
    for (auto& thread : threadsData) {
        thread->active = false;
    }
    pthread_mutex_unlock(&threadMutex);

    for (auto& thread : threadsData) {
        pthread_join(thread->threadId, NULL);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();

    pthread_mutex_destroy(&threadMutex);
    pthread_mutex_destroy(&colorMutex);

    threadsData.clear();

    return 0;
}

