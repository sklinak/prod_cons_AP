#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <cstdint>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ---------------- Blocking Queue ----------------
template<typename T>
class BlockingQueue {
private:
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;

public:
    void push(const T& value) {
        {
            std::unique_lock<std::mutex> lock(m);
            q.push(value);
        }
        cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&] { return !q.empty(); });
        T value = q.front();
        q.pop();
        return value;
    }
};

// ---------------- Result Collection Structure ----------------
class ResultCollector {
private:
    std::vector<bool> processed_rows;
    std::mutex m;
    std::condition_variable cv;

public:
    explicit ResultCollector(int height) : processed_rows(height, false) {}

    void markRowDone(int row) {
        {
            std::unique_lock<std::mutex> lock(m);
            processed_rows[row] = true;
        }
        cv.notify_all();
    }

    bool isAllDoneLocked() const {
        for (bool done : processed_rows) {
            if (!done) return false;
        }
        return true;
    }

    void waitAllDone() {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&] { return isAllDoneLocked(); });
    }

};

// ---------------- Task Structure ----------------
struct Task {
    uint8_t* data = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;
    int row = 0;
    bool is_poison = false;
    ResultCollector* collector = nullptr;
};

// ---------------- Row Inversion ----------------
void invertRow(const Task& t) {
    uint8_t* line = t.data + t.row * t.width * t.channels;

    for (int x = 0; x < t.width; ++x) {
        for (int c = 0; c < t.channels; ++c) {
            line[x * t.channels + c] =
                static_cast<uint8_t>(255 - line[x * t.channels + c]);
        }
    }
}

// ---------------- Consumer ----------------
void consumer(BlockingQueue<Task>& queue) {
    while (true) {
        Task task = queue.pop();

        if (task.is_poison)
            break;

        invertRow(task);

        if (task.collector) {
            task.collector->markRowDone(task.row);
        }
    }
}

// ---------------- Producer ----------------
void producer(const std::string& filename,
              BlockingQueue<Task>& queue,
              int consumersCount,
              ResultCollector& collector) {

    int w = 0, h = 0, ch = 0;
    uint8_t* img = stbi_load(filename.c_str(), &w, &h, &ch, 0);

    if (!img) {
        std::cout << "Ошибка загрузки " << filename << std::endl;
        for (int i = 0; i < consumersCount; ++i) {
            Task poison{};
            poison.is_poison = true;
            queue.push(poison);
        }
        return;
    }

    std::cout << "Загружено: " << filename << std::endl;

    for (int r = 0; r < h; ++r) {
        Task t;
        t.data = img;
        t.width = w;
        t.height = h;
        t.channels = ch;
        t.row = r;
        t.is_poison = false;
        t.collector = &collector;
        queue.push(t);
    }

    collector.waitAllDone();

    for (int i = 0; i < consumersCount; ++i) {
        Task poison{};
        poison.is_poison = true;
        queue.push(poison);
    }

    std::string out =
        filename.substr(0, filename.find_last_of('.')) + "_inverted.png";

    stbi_write_png(out.c_str(), w, h, ch, img, w * ch);

    std::cout << "Сохранено: " << out << std::endl;

    stbi_image_free(img);
}

// ---------------- main ----------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " image.png\n";
        return 1;
    }

    int w = 0, h = 0, ch = 0;
    uint8_t* temp_img = stbi_load(argv[1], &w, &h, &ch, 0);
    if (!temp_img) {
        std::cout << "Ошибка загрузки " << argv[1] << std::endl;
        return 1;
    }
    stbi_image_free(temp_img);

    ResultCollector collector(h);
    BlockingQueue<Task> queue;

    int consumersCount = 4;

    std::vector<std::thread> consumers;
    consumers.reserve(consumersCount);
    for (int i = 0; i < consumersCount; ++i) {
        consumers.emplace_back(consumer, std::ref(queue));
    }

    std::thread prod(producer,
                     std::string(argv[1]),
                     std::ref(queue),
                     consumersCount,
                     std::ref(collector));

    prod.join();

    for (auto& t : consumers) {
        t.join();
    }

    std::cout << "Работа завершена.\n";
    return 0;
}
