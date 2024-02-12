#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <cstring>


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
    
    // Индивидуальное задание:
    // Вывести значение атрибута «detach state» потока, установленного «по умолчанию»,
    // изменить его значение и проиллюстрировать изменение поведения потока. Объяснить суть атрибута.

    // Суть атрибута.
    // Атрибут detach state имеет два значения:
    // - DETACHED: По завершении работы поток автоматически освобождает свои ресурсы
    // - JOINABLE: По завершении работы поток оставляет свои ресурсы в системе,
    //             другой поток должен освободить эти ресурсы с помощью pthread_join(),
    //             иначе может произойти утечка памяти

    // Вывести значение атрибута, установленного по умолчанию
    int detachState;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_getdetachstate(&attr, &detachState);
    std::cout << "Значение атрибута detach state по умолчанию: ";
    if (detachState == PTHREAD_CREATE_JOINABLE)
        std::cout << "JOINABLE" << std::endl;
    else if (detachState == PTHREAD_CREATE_DETACHED)
        std::cout << "DETACHED" << std::endl;
    else
        std::cout << "НЕИЗВЕСТНО" << std::endl;

    // Create two POSIX Threads
    pthread_t thread1 = 1;
    pthread_t thread2 = 2;
    TArgs arg1 = {1, 1, 'H'};
    TArgs arg2 = {2, 1, 'A'};
    pthread_create(&thread1, nullptr, startRoutine, &arg1);
    pthread_create(&thread2, nullptr, startRoutine, &arg2);

    // Изменить значение атрибута. При попытке применить pthread_join получим ошибку EINVAL (errno = 22)
    if (pthread_detach(thread2) != 0)
        std::cerr << "Ошибка при отсоединении потока\n";
    else
        std::cout << "Поток #2 отсоединён\n";
    // Или можно установить DETACHED при создании потока так:
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    // pthread_create(&thread2, &attr, startRoutine, &arg2);

    // Wait for getchar()
    std::cout << "Нажмите <ENTER> для завершения работы" << std::endl;
    getchar();
    arg1.status = 0;
    arg2.status = 0;
    std::cout << "Клавиша нажата. Ожидание завершения работы потоков..." << std::endl;

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

    printf("Программа завершила работу\n");
    return 0;
}