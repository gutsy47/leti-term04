// Родительская программа
// Вариант 4. Функция execle()

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Родительская программа начала работу\n";
    std::cout << "Current PID: " << getpid() << std::endl;
    std::cout << " Parent PID: " << getpid() << std::endl;

    std::cout << "Аргументы командной строки: " << std::endl;
    for (int i = 1; i < argc; ++i) {
        std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        const char *envp[] = { "PATH=/bin:/usr/bin", nullptr };
        execle("./prog1", argv[0], argv[1], argv[2], nullptr, envp);

        // If execle() returns control, then something is wrong
        perror("execle");
        return 1;
    } else if (pid > 0) {
        // Parent process
        std::cout << "  Child PID: " << getpid() << std::endl;

        int status;
        while (waitpid(pid, &status, WNOHANG) == 0) {
            std::cout << "Ожидание...\n";
            usleep(500000);
        }

        if (WIFEXITED(status)) {
            std::cout << "Дочерний процесс завершился с кодом выхода " << WEXITSTATUS(status) << std::endl;
        }
    } else {
        perror("fork");
        return 1;
    }

    return 0;
}