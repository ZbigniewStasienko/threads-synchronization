#include <GLFW/glfw3.h>
#include <unistd.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <time.h>
#include <chrono>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

struct ThreadData {
    float speed;
    float x;
    float y;
    float direction;
    int stand;
    volatile bool active;
    volatile bool finished;
    volatile bool waiting;
    thread threadId;
    float color[3];

    ThreadData(float s)
        : speed(s), x(-0.9f), y(0.0f), direction(0.0f), active(true), finished(false), waiting(false), stand(2) {
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

volatile int currentColorIndex = 0;
volatile float numOfWaitingThreads = 0;
volatile bool running = true;

thread colorChangeThread;
vector<shared_ptr<ThreadData>> threadsData;
mutex threadMutex;
mutex counterMutex;
condition_variable cv;
vector<mutex> standMutexes(3);
bool isStandAvailable[3] = {true, true, true};


void counter() {
    system("clear");
    printf("Average number of threads waiting in queue: %.2f \n", numOfWaitingThreads / 3);
    fflush(stdout);
}

void threadFunction(shared_ptr<ThreadData> data) {
    bool reachedCenter = false;
    bool reachedStand = false;
    float threadSpeed = data->speed;
    bool toStand = false;
    bool shouldWait;
    float moveX = 0.005f;
    unique_lock<mutex> lock;

    while (data->active) {

        shouldWait = false;

        {
            lock_guard<mutex> lk(threadMutex);
            if (reachedCenter) {
                for (const auto& dataCheck : threadsData) {
                    if (dataCheck->waiting && (dataCheck->stand == data->stand) && data->x + 0.025f >= dataCheck->x && !reachedStand && !toStand) {
                        shouldWait = true;
                        break;
                    }
                }
            }
        }

        if (!shouldWait) {
            data->x += moveX;
            data->y += data->direction;
        } else {
            data->waiting = true;
            {
                lock_guard<mutex> lk(counterMutex);
                numOfWaitingThreads++;
                counter();
            }    
            unique_lock<mutex> lock(standMutexes[data->stand - 1]);
            cv.wait(lock, [&] { return isStandAvailable[data->stand - 1] || !running; });
            if (!running) {
                break;
            }
            isStandAvailable[data->stand - 1] = false;
            toStand = true;
        }

        if (!reachedCenter && data->x >= 0.0f) {
            reachedCenter = true;
            if (currentColorIndex == 0) {
                data->direction = 0.005;
                data->stand = 1;
            } else if (currentColorIndex == 1) {
                data->direction = -0.005;
                data->stand = 3;
            }
        }

        if (data->direction != 0.0f && data->x >= 0.475f) {
            data->direction = 0.0f;
        }

        if (!reachedStand && data->x >= 0.97f) {
            if(!toStand){
                {
                    lock_guard<mutex> lk(threadMutex);
                    numOfWaitingThreads++;
                    counter();
                }
            }
            {
                lock = unique_lock<mutex>(standMutexes[data->stand - 1]);
                reachedStand = true;
                data->waiting = true;
                isStandAvailable[data->stand - 1] = false;
                usleep(1000000);
                isStandAvailable[data->stand - 1] = true;
                cv.notify_one();
                {
                    lock_guard<mutex> lk(counterMutex);
                    numOfWaitingThreads--;
                    counter();
                }
                data->waiting = true;
            }
            toStand = false;
        }

        if (data->x >= 1.0f) {
            data->active = false;
        }

        usleep(threadSpeed);
    }

    {
        lock_guard<mutex> lk(threadMutex);
        data->finished = true;
    }
}

void colorChangeFunction() {
    int changeColorTime = 2;
    while (running) {
        currentColorIndex = (currentColorIndex + 1) % 3;
        sleep(changeColorTime);
    }
}

void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLineWidth(2.0f);

    {
        lock_guard<mutex> lk(threadMutex);
        for (const auto& data : threadsData) {
            if (data->active) {
                glColor3fv(data->color);
                glBegin(GL_LINES);
                glVertex2f(data->x, data->y - 0.05f);
                glVertex2f(data->x, data->y + 0.05f);
                glEnd();
            }
        }
    }

    glColor3f(colorSquare[currentColorIndex][0], colorSquare[currentColorIndex][1], colorSquare[currentColorIndex][2]);
    glBegin(GL_LINES);
    glVertex2f(0.0f, -0.1f);
    glVertex2f(0.0f, 0.1f);
    glEnd();

    float halfSide = 0.035f;
    float x = 1.0f;
    float y = 0.475f;

    glColor3f(1.0f, 0.0f, 0.0f);
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
        running = false;
        cv.notify_all();
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

    colorChangeThread = thread(colorChangeFunction);

    auto lastCreationTime = chrono::steady_clock::now();

    counter();
    while (running) {
        auto currentTime = chrono::steady_clock::now();
        float elapsed = chrono::duration<float>(currentTime - lastCreationTime).count();
        float generationTime = 0.6f;
        if (elapsed >= generationTime) {
            float speed = static_cast<float>(rand() % 10000 + 5000);
            auto data = make_shared<ThreadData>(speed);
            data->threadId = thread(threadFunction, data);
            {
                lock_guard<mutex> lk(threadMutex);
                threadsData.push_back(data);
            }
            lastCreationTime = currentTime;
        }
        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();

        {
            vector<shared_ptr<ThreadData>> activeThreads;
            {
                lock_guard<mutex> lk(threadMutex);
                for (auto& data : threadsData) {
                    if (!data->finished) {
                        activeThreads.push_back(data);
                    } else if (data->threadId.joinable()) {
                        data->threadId.join();
                    }
                }
            }
            threadsData.swap(activeThreads);
        }        
    }

    for (auto& thread : threadsData) {
        thread->active = false;
        if (thread->threadId.joinable()) {
            thread->threadId.join();
        }
    }

    if (colorChangeThread.joinable()) {
        colorChangeThread.join();
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    usleep(250000);

    glfwSetWindowShouldClose(window, GL_TRUE);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
