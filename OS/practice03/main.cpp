// Вариант 18, функция getmntent()

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <pthread.h>
#include <mntent.h>
#include <fcntl.h>


int pipe_fd[2];


/// Custom SIGINT handler
void handleSignal(int sig) {
    std::cout << "\nInterrupt signal: " << sig << "\n";
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    exit(0);
}


/// Thread parameters
struct TArgs {
    int status = 1; // 1 if thread is active else 0
};


/// Write data into mntent
static void *writeRoutine(void *args) {
    auto *arg = (TArgs *) args;

    // Fetch data and format message
    std::cout << "Поток записи открывает файл описаний файловых систем\n";
    FILE *fp = setmntent("/etc/fstab", "r");
    if (!fp) {
        perror("setmntent");
        arg->status = 0;
        pthread_exit(nullptr);
    }

    std::cout << "Файл открыт. Поток записи начал свою работу\n";
    char buf[256];
    struct mntent *fs;
    while (arg->status) {
        // Format message and write data
        fs = getmntent(fp);
        if (fs) snprintf(buf, sizeof(buf), "device: %s, dir: %s", fs->mnt_fsname, fs->mnt_dir);
        else    snprintf(buf, sizeof(buf), "End of mntent");
        ssize_t rv = write(pipe_fd[1], buf, strlen(buf));

        // Analyze return value
        if (rv == -1) {
            perror("write");
            arg->status = 0;
            pthread_exit(nullptr);
        } else {
            std::cout << "WRITE | " << buf << std::endl;
        }
        sleep(1);
    }

    endmntent(fp);
    std::cout << "Поток записи завершил свою работу\n";
    int *ret = new int(16);
    pthread_exit(ret);
}


/// Read data from mntent
static void *readRoutine(void *args) {
    auto *arg = (TArgs *) args;

    std::cout << "Поток чтения начал свою работу\n";
    char buf[256];
    while (arg->status) {
        // Clear buffer. Read data from pipe
        memset(buf, 0, sizeof(buf));
        ssize_t rv = read(pipe_fd[0], buf, sizeof(buf));

        // Analyze return value
        if (rv == 0) {
            std::cout << "Поток чтения достиг конца файла\n";
            sleep(1);
        } else if (rv == -1) {
            if (errno == EINTR) continue;
            perror("read");
            sleep(1);
        } else if (rv > 0) {
            std::cout << " READ | " << buf << std::endl;
        }
    }

    std::cout << "Поток чтения завершил свою работу\n";
    int *ret = new int(17);
    pthread_exit(ret);
}

int main(int argc, char* argv[]) {
    // Initialization
    signal(SIGINT, handleSignal);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Программа начала работу\n";

    // Create a pipe depending on the argv
    int response;
    if (argc != 2) {
        std::cerr << "Неправильное количество аргументов\n";
        return 1;
    }
    if (strcmp(argv[1], "pipe") == 0) {
        response = pipe(pipe_fd);
    } else if (strcmp(argv[1], "pipe2") == 0) {
        response = pipe2(pipe_fd, O_NONBLOCK);
    } else if (strcmp(argv[1], "fcntl") == 0) {
        response = pipe(pipe_fd);
        fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK);
        fcntl(pipe_fd[1], F_SETFL, O_NONBLOCK);
    } else {
        std::cerr << "Неправильное значение аргумента\n";
        return 2;
    }

    // Check the result
    if (response == 0) {
        std::cout << "Создан канал с помощью: " << argv[1] << std::endl;
    } else {
        perror("pipe");
        return -1;
    }

    // Create two POSIX Threads
    pthread_t wThread, rThread;
    TArgs arg1 = {1}, arg2 = {1};
    pthread_create(&wThread, nullptr, writeRoutine, &arg1);
    pthread_create(&rThread, nullptr, readRoutine, &arg2);

    // Wait for getchar()
    std::cout << "Нажмите <ENTER> для завершения работы...\n";
    getchar();
    arg1.status = 0;
    arg2.status = 0;
    std::cout << "Клавиша нажата. Ожидание завершения работы потоков...\n";

    // Wait for the threads to finish. Complete the program
    int *ret1, *ret2;
    int res1 = pthread_join(wThread, (void **) &ret1);
    int res2 = pthread_join(rThread, (void **) &ret2);
    std::cout << "Поток записи завершился с кодом выхода: " << res1 << ", вернув: " << (ret1 ? *ret1 : 0) << std::endl;
    if (res1 != 0) std::cerr << strerror(res2);
    std::cout << "Поток чтения завершился с кодом выхода: " << res2 << ", вернув: " << (ret2 ? *ret2 : 0) << std::endl;
    if (res2 != 0) std::cerr << strerror(res2);
    delete ret1;
    delete ret2;

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    std::cout << "Программа завершила работу\n";
    return 0;
}