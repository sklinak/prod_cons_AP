# Parallel Image Inversion (Producer–Consumer Pattern)

## Description

This project implements a **parallel image processing algorithm** using the **Producer–Consumer pattern** with a custom **blocking queue** in C++.

The program loads an image, inverts its pixel colors row by row using multiple threads, and saves the processed image as a new file.

This project was developed for a **Computer Architecture & Operating Systems** course.

---

## ⚙️ How It Works

The system consists of:

### Producer

* Loads the image using `stb_image`
* Creates one task per image row
* Pushes tasks into a thread-safe blocking queue
* Waits until all rows are processed
* Sends termination signals (poison tasks) to consumers
* Saves the inverted image

###  Consumers (Worker Threads)

* Continuously pop tasks from the blocking queue
* Invert pixel values for the assigned row
* Notify the `ResultCollector` when processing is complete
* Stop when receiving a poison task

### Blocking Queue

A thread-safe queue implemented using:

* `std::mutex`
* `std::condition_variable`

Ensures proper synchronization between producer and consumers.

###  ResultCollector

Tracks processed rows and:

* Waits until all rows are completed
* Uses condition variables for synchronization

---

##  Concurrency Concepts Used

* Producer–Consumer pattern
* Thread synchronization
* Mutexes
* Condition variables
* Thread-safe data structures
* Graceful thread termination (poison pill pattern)

---

##  Dependencies

This project uses:

* C++17 (or later)
* `stb_image.h`
* `stb_image_write.h`

You must place:

```
stb_image.h
stb_image_write.h
```

in the same directory as the source file.

---

##  Build Instructions

Using **g++**:

```bash
g++ -std=c++17 -pthread main.cpp -o image_processor
```

---

##  Run

```bash
./image_processor image.png
```

Example:

```bash
./image_processor photo.jpg
```

---

##  Output

The program generates a new file:

```
originalname_inverted.png
```

Example:

```
photo.jpg → photo_inverted.png
```

---

##  Architecture Overview

```
Producer Thread
        ↓
   Blocking Queue
        ↓
Multiple Consumer Threads
        ↓
   ResultCollector
        ↓
    Final Image Save
```

---

##  Thread Configuration

By default:

```cpp
int consumersCount = 4;
```

You can change this value to experiment with performance.

---

## Educational Purpose

This project demonstrates:

* Practical multithreading in C++
* Safe inter-thread communication
* Real-world synchronization problems
* Efficient parallel data processing

---

