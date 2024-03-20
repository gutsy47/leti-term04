// Вариант 9.3, пространство имен PID.

#include <iostream>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>


#define STACK_SIZE (1024 * 1024)


/// Print error message and exit with code 1
void errExit(const std::string &msg) {
    perror(msg.c_str());
    exit(1);
}


/// Child process in new PID namespace
static int childFunc(void *args) {
    auto arg = (int*) args;

    std::cout << "Дочерний процесс запущен. PID: " << getpid() << "; PPID: " << getppid() << std::endl;
    std::cout << "Переданный аргумент: " << *arg << std::endl;

    return 0;
}


int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Основная программа запущена. PID: " << getpid() << std::endl;

    // Create child process in new PID namespace with some int argument
    char *stack = (char*) malloc(STACK_SIZE);
    char *stackTop = stack + STACK_SIZE;
    int arg = 12345;
    int child_pid = clone(childFunc, stackTop, CLONE_NEWPID | SIGCHLD, &arg);
    if (child_pid == -1) errExit("clone");
    std::cout << "PID после создания дочернего процесса: " << getpid() << std::endl;

    // Wait for child process to finish
    int status;
    if (waitpid(child_pid, &status, 0) == -1) errExit("waitpid");
    std::cout << "PID после завершения дочернего процесса: " << getpid() << std::endl;

    free(stack);
    return 0;
}