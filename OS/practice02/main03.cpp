// Вариант 2, программа 3 – pthread_mutex_timedlock()

#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <csignal>


/// Global mutex identification
pthread_mutex_t MUTEX;


/// Custom SIGINT handler
void handleSignal(int sig) {
    std::cout << "\nInterrupt signal: " << sig << "\n";
    pthread_mutex_destroy(&MUTEX);
    exit(0);
}

/// Thread parameters
struct TArgs {
    unsigned short id = 0;        // Thread ID
    int status = 1;               // 1 if thread is active else 0
    char msg = 'a';               // This char will be printed
    unsigned short msgCount = 10; // Count of times the char will be printed
};


/// Thread routine template
static void *startRoutine(void *args) {
    auto *arg = (TArgs *) args;

    std::cout << "\nПоток #" << arg->id << " начал свою работу (msg='" << arg->msg << "')\n";
    while (arg->status) {
        // Try to lock the mutex via pthread_mutex_timedlock for 1 second
        while (true) {
            struct timespec ts{};
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;
            int rv = pthread_mutex_timedlock(&MUTEX, &ts);
            if (rv == 0) break;
            else         std::cerr << "Ошибка потока #" << arg->id << ": " << strerror(rv) << std::endl;
        }

        // Critical section
        for (int i = 0; i < arg->msgCount; ++i) {
            std::cout << arg->msg;
            fflush(stdout);
            sleep(1);
        }

        // "Job" outside the section
        pthread_mutex_unlock(&MUTEX);
        sleep(1);
    }
    std::cout << "\nПоток #" << arg->id << " завершил свою работу (msg='" << arg->msg << "')\n";

    // Return value = 15 + arg->id
    int *ret = new int(15 + arg->id);
    pthread_exit(ret);
}


int main() {
    // Initialization
    signal(SIGINT, handleSignal);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    pthread_mutex_init(&MUTEX, nullptr);
    std::cout << "Программа начала работу\n";

    // Create two POSIX Threads
    pthread_t thread1, thread2;
    TArgs arg1 = {1, 1, '1', 10};
    TArgs arg2 = {2, 1, '2', 10};
    pthread_create(&thread1, nullptr, startRoutine, &arg1);
    pthread_create(&thread2, nullptr, startRoutine, &arg2);

    // Wait for getchar()
    std::cout << "Нажмите <ENTER> для завершения работы...\n";
    getchar();
    arg1.status = 0;
    arg2.status = 0;
    std::cout << "Клавиша нажата. Ожидание завершения работы потоков...\n";

    // Wait for the threads to finish. Complete the program
    int *ret1, *ret2;
    int res1 = pthread_join(thread1, (void **) &ret1);
    int res2 = pthread_join(thread2, (void **) &ret2);
    std::cout << "Поток #1 завершился с кодом выхода: " << res1 << ", вернув: " << (ret1 ? *ret1 : 0) << std::endl;
    if (res1 != 0) std::cerr << strerror(res2);
    std::cout << "Поток #2 завершился с кодом выхода: " << res2 << ", вернув: " << (ret2 ? *ret2 : 0) << std::endl;
    if (res2 != 0) std::cerr << strerror(res2);
    delete ret1;
    delete ret2;

    pthread_mutex_destroy(&MUTEX);
    std::cout << "Программа завершила работу\n";
    return 0;
}