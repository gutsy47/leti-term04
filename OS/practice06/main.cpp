// Стандарт SVID. Вариант 18 - функция getmntent()

#include <iostream>
#include <csignal>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <mntent.h>
#include <cstring>


// Determine the operating mode of the program: 1 - WRITE, 0 - READ
#ifndef MODE
#define MODE 1
#endif

#define SEM_WRITE_NAME "/sem-w"
#define SEM_READ_NAME "/sem-r"
#define SHM_NAME "shared-memory"
sem_t *semWrite;       // Write semaphore pointer
sem_t *semRead;        // Read semaphore pointer
bool thrStatus = true; // Thread status. 1 if thread is running else 0
int shmId;             // Shared memory id
void *shm;             // Local shared memory pointer


/// Custom SIGINT handler
void handleSignal(int sig) {
    std::cout << "\nInterrupt signal: " << sig << "\n";
    sem_unlink(SEM_READ_NAME);
    sem_unlink(SEM_WRITE_NAME);
    exit(0);
}


/// Write data to shared memory
static void *writeRoutine(void *args) {
    auto *status = (bool *) args;

    // Fetch data and format message
    std::cout << "Поток записи открывает файл описаний файловых систем\n";
    FILE *fp = setmntent("/etc/fstab", "r");
    if (!fp) {
        perror("setmntent");
        *status = false;
        pthread_exit(nullptr);
    }

    std::cout << "Файл открыт. Поток записи начал свою работу\n";
    struct mntent *fs;
    char data[64];
    while (*status) {
        // Fetch data into the message and "send" it to the reader by copying into the shared memory
        fs = getmntent(fp);
        if (fs) snprintf(data, sizeof(data), "device: %s, dir: %s", fs->mnt_fsname, fs->mnt_dir);
        else    snprintf(data, sizeof(data), "End of mntent");
        std::cout << data << std::endl;
        memcpy(shm, data, sizeof(data));

        // Release write semaphore. Wait for read
        sem_post(semWrite);
        sem_wait(semRead);
        sleep(1);
    }
    endmntent(fp);
    std::cout << "Поток записи завершил свою работу\n";

    int *ret = new int(16);
    pthread_exit(ret);
}


/// Get data from shared memory
static void *readRoutine(void *args) {
    auto *status = (bool *) args;

    std::cout << "Поток чтения начал свою работу\n";
    char data[64];
    while (*status) {
        sem_wait(semWrite); // Wait for new data
        memcpy(data, shm, sizeof(data)); // Copy data from shm
        sem_post(semRead); // Release read semaphore

        // Process data
        std::cout << data << std::endl;
    }
    std::cout << "Поток чтения завершил свою работу\n";

    int *ret = new int(17);
    pthread_exit(ret);
}


/// Print error message and exit with error code 1
void errExit(const std::string &msg) {
    perror(msg.c_str());
    exit(1);
}


int main() {
    // Initialization
    signal(SIGINT, handleSignal);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Программа начала свою работу в режиме: " << MODE << " (1 - WRITE, 0 - READ)\n";

    // Create file if not exist for the ftok() function
    int fd = open(SHM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd != -1) close(fd);
    // Get unique key by shared memory name
    key_t key = ftok(SHM_NAME, 1);
    if (key < 0) errExit("ftok");
    // Create | Get memory ID
    shmId = shmget(key, sizeof(int), 0664 | IPC_CREAT);
    if (shmId < 0) errExit("shmget");
    // Attach to shared memory
    shm = shmat(shmId, nullptr, 0);
    if (shm == (void*) -1) {
        shmctl(shmId, IPC_RMID, nullptr);
        errExit("shmat");
    }
    std::cout << "Разделяемая память успешно присоединена\n";

    // Semaphores
    semWrite = sem_open(SEM_WRITE_NAME, O_CREAT, 0664, 1);
    semRead  = sem_open(SEM_READ_NAME, O_CREAT, 0664, 1);
    if (semWrite == SEM_FAILED || semRead == SEM_FAILED) {
        perror("sem_open");
        if (semWrite == SEM_FAILED) {
            sem_close(semWrite);
            sem_unlink(SEM_WRITE_NAME);
        }
        if (semRead == SEM_FAILED) {
            sem_close(semRead);
            sem_unlink(SEM_READ_NAME);
        }
        shmdt(shm);
        shmctl(shmId, IPC_RMID, nullptr);
        return 1;
    }
    std::cout << "Семафоры готовы к работе\n";

    // Thread
    pthread_t thread;
    if (MODE == 1) pthread_create(&thread, nullptr, writeRoutine, &thrStatus);
    else           pthread_create(&thread, nullptr, readRoutine, &thrStatus);

    // Wait till user's input and the thread completion
    std::cout << "Нажмите <ENTER> для завершения работы...\n";
    getchar();
    thrStatus = false;
    std::cout << "Ожидание завершения работы потока\n";
    int *rv;
    int exit = pthread_join(thread, (void **) &rv);
    std::cout << "Поток завершился с кодом выхода: " << exit << ", вернув: " << (rv ? *rv : 0) << '\n';

    // Clean up
    sem_close(semRead);
    sem_unlink(SEM_READ_NAME);
    sem_close(semWrite);
    sem_unlink(SEM_WRITE_NAME);
    shmdt(shm);
    shmctl(shmId, IPC_RMID, nullptr);
    return 0;
}