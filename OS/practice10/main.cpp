// Вариант 10.4. Задание 1

#include <iostream>
#include <ev.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <mntent.h>
#include <fcntl.h>


int pipe_fd[2];
FILE *fp;


/// Custom SIGINT handler
void handleSignal(int sig) {
    std::cout << "\nInterrupt signal: " << sig << "\n";
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    exit(0);
}


/// Print error message and exit with code 1
void errExit(const std::string &msg) {
    perror(msg.c_str());
    exit(1);
}


/// Return 0 if pipe successfully created using specified <type> (pipe, pipe2, fcntl)
int createPipe(char* type) {
    if (!type) {
        std::cout << "Тип потока не был передан\n";
        exit(1);
    }

    if (strcmp(type, "pipe") == 0)
        return pipe(pipe_fd);

    if (strcmp(type, "pipe2") == 0)
        return pipe2(pipe_fd, O_NONBLOCK);

    if (strcmp(type, "fcntl") == 0) {
        int response = pipe(pipe_fd);
        fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK);
        fcntl(pipe_fd[1], F_SETFL, O_NONBLOCK);
        return response;
    }

    std::cout << "Неправильное значение аргумента\n";
    exit(1);
}


/// Terminate watchers
static void stdin_cb(EV_P, ev_io *w, int revents) {
    std::cout << "ENTER нажат. Остановка наблюдения за событиями...\n";
    ev_io_stop(EV_A, w);
    ev_break(EV_A, EVBREAK_ALL);
}


/// Write mntent data to the pipe
static void timer_cb(EV_P, ev_timer *w, int revents) {
    char buf[256];
    struct mntent *fs;
    fs = getmntent(fp);
    if (fs) snprintf(buf, sizeof(buf), "device: %s, dir: %s", fs->mnt_fsname, fs->mnt_dir);
    else    snprintf(buf, sizeof(buf), "End of mntent");

    ssize_t rv = write(pipe_fd[1], buf, strlen(buf));
    if (rv == -1) errExit("write");
    else          std::cout << "WRITE | " << buf << std::endl;
}


/// Read data from the pipe
static void pipe_fd_cb(EV_P, ev_io *w, int revents) {
    char buf[128];
    ssize_t rv = read(pipe_fd[0], buf, sizeof(buf));
    if (rv < 0) perror("read");
    else        std::cout << " READ | " << buf << std::endl;
}


int main(int argc, char* argv[]) {
    // Initialization
    signal(SIGINT, handleSignal);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Программа начала работу\n";

    // Create a pipe depending on the argv
    if (argc != 2) {
        std::cerr << "Неправильное количество аргументов\n";
        std::cout << "Необходимо передать метод инициализации (pipe, pipe2, fcntl), например:\n";
        std::cout << "./main pipe2\n";
        return 1;
    }
    if (createPipe(argv[1]) == 0) std::cout << "Канал успешно создан\n";
    else errExit("pipe");

    // Open /proc/mounts via mntent
    fp = setmntent("/proc/mounts", "r");
    if (!fp) errExit("setmntent");
    std::cout << "Файл /proc/mounts открыт\n";

    // Main loop
    struct ev_loop *loop = EV_DEFAULT;

    // Handle STDIN input. Stop the loop on ENTER pressed
    ev_io stdin_watcher;
    ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, &stdin_watcher);

    // Timer with one-second cycle. Write to the pipe on event
    ev_timer timer_watcher;
    ev_timer_init(&timer_watcher, timer_cb, 0., 1.);
    ev_timer_start(loop, &timer_watcher);

    // Handle PIPE output. Read from the pipe on event
    ev_io pipe_fd_watcher;
    ev_io_init(&pipe_fd_watcher, pipe_fd_cb, pipe_fd[0], EV_READ);
    ev_io_start(loop, &pipe_fd_watcher);

    std::cout << "Нажмите <ENTER> для завершения работы...\n";
    ev_run(loop, 0);

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    endmntent(fp);
    ev_loop_destroy(loop);
    std::cout << "Программа завершила работу\n";
    return 0;
}