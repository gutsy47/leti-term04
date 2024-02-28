// Дочерняя программа
// Вариант 4. Функция execle()

#include <iostream>
#include <unistd.h>

int main(int argc, char* argv[], char *envp[]) {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Дочерний процесс начал работу\n";
    std::cout << "Current PID: " << getpid() << std::endl;
    std::cout << " Parent PID: " << getppid() << std::endl;

    std::cout << "Аргументы командной строки: " << std::endl;
    for (int i = 1; i < argc; ++i) {
        std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
        sleep(1);
    }

    std::cout << "Переменные окружения: " << std::endl;
    if (envp) {
        for (char **env = envp; *env; env++) {
            std::cout << *env << std::endl;
        }
    }

    exit(10);
}