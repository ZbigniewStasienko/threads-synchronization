## Description

An application in C++ for Linux, whose main aim is to demonstrate how threads are synchronized.

- Subsequent clients (threads) appear at the system's entry at regular intervals, moving at constant, randomly chosen speeds, first to the distributor and then to the service stations. At the service stations, they stop for a few seconds and then leave the system.
- The distributor switches between directions, where it directs clients, at a fixed interval. Clients do not wait either before the distributor or before the service stations.
- The program ends by pressing the spacebar.
- Threads form a queue in front of the station.
- The console has a counter showing the average number of threads waiting in the queue.

<p align="center">
  <img src="https://github.com/ZbigniewStasienko/threads-synchronization/assets/140521815/0f010d1d-0cfb-4126-b4b9-9cbf7d041841" alt="Image description">
</p>

## How to run

### Prerequisites
Following libraries/tools have to be instaled on your **Linux** system:
- GLFW
- OpenGL
- pthread
- C++ Standard Library
- Compiler (C++ compiler that supports at least C++11)

### Running the application

If you have the appropriate tools to run programs with make commands, you can use following command in projects directory: 
```bash
make run
```
Otherwise, run the program with the following instructions (while in the project directory):

```bash
g++ project.cpp -o bin/project -lglfw -lGL -lpthread
./bin/project
```
