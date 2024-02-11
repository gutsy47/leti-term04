#include <iostream>
#include <unistd.h>
#include <pthread.h>


/// Thread parameters
struct TArgs {
    unsigned short id = 0;
    int status = 1;
    char msg = 'a';
};


/// Thread routine template
static void *startRoutine(void *args) {
    auto *arg = (TArgs *) args;

    // Worker
    std::cout << "Поток #" << arg->id << " начал свою работу (сообщение: \"" << arg->msg << "\")" << std::endl;
    while (arg->status) {
        std::cout << arg->msg;
        fflush(stdout);
        sleep(1);
    }
    std::cout << "Поток #" << arg->id << " закончил свою работу" << std::endl;

    // Return value = 15 + arg->id
    int *ret = new int(15 + arg->id);
    pthread_exit(ret);
}


int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Программа начала работу" << std::endl;

    // Create two POSIX Threads
    pthread_t thread1 = 1;
    pthread_t thread2 = 2;
    TArgs arg1 = {1, 1, 'H'};
    TArgs arg2 = {2, 1, 'A'};
    pthread_create(&thread1, nullptr, startRoutine, &arg1);
    pthread_create(&thread2, nullptr, startRoutine, &arg2);

    // Wait for getchar()
    std::cout << "Нажмите <ENTER> для завершения работы" << std::endl;
    getchar();
    arg1.status = 0;
    arg2.status = 0;
    std::cout << "Клавиша нажата. Ожидание завершения работы потоков..." << std::endl;

    // Wait for the threads to finish. Complete the program
    int *ret1, *ret2;
    pthread_join(thread1, (void **) &ret1);
    pthread_join(thread2, (void **) &ret2);
    std::cout << "Поток #1 завершился с кодом: " << *ret1 << std::endl;
    std::cout << "Поток #2 завершился с кодом: " << *ret2 << std::endl;

    printf("Программа завершила работу\n");
    return 0;
}