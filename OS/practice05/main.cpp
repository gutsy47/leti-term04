// Вариант 4. Функция pselect()

#include <iostream>
#include <fstream>
#include <csignal>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/select.h>


#define SEMAPHORE_NAME "/semaphore"
#define FILE_NAME "output.txt"
#ifndef MESSAGE
#define MESSAGE '1'
#endif


/// Custom SIGINT handler
void handleSignal(int sig) {
    std::cout << "\nInterrupt signal: " << sig << "\n";
    sem_unlink(SEMAPHORE_NAME);
    exit(0);
}


int main() {
    // Initialization
    signal(SIGINT, handleSignal);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Программа " << MESSAGE << " начала свою работу\n";

    // Open semaphore
    sem_t *sem = sem_open(SEMAPHORE_NAME, O_CREAT, 0664, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    std::cout << "Семафор успешно создан\n";

    // Open file
    std::ofstream file(FILE_NAME, std::ios::app);
    if (!file.is_open()) {
        perror("file");
        sem_close(sem);
        sem_unlink(SEMAPHORE_NAME);
        return 1;
    }
    std::cout << "Файл открыт\n";

    // Main loop
    timespec timeout = {.tv_sec = 1, .tv_nsec = 0};
    fd_set readfds;
    while (true) {
        sem_wait(sem);
        std::cout << "Программа вошла в критический участок\n";
        for (int i = 0; i < 5; ++i) {
            // Critical section
            file << MESSAGE;
            file.flush();
            std::cout << MESSAGE;
            std::cout.flush();
            sleep(1);
        }
        sem_post(sem);
        std::cout << "\nПрограмма вышла из критического участка\n";

        // Use pselect() to detect anything from STDIN
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int rv = pselect(1, &readfds, nullptr, nullptr, &timeout, nullptr);
        if (rv && FD_ISSET(STDIN_FILENO, &readfds)) {
            // If STDIN has something then read, if read is '\n' then exit
            char ch;
            read(STDIN_FILENO, &ch, 1);
            if (ch == '\n') break;
        }
    }

    file.close();
    sem_close(sem);
    sem_unlink(SEMAPHORE_NAME);

    return 0;
}