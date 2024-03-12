// Очередь POSIX (7.2). Вариант 18 - функция getmntent()

#include <iostream>
#include <pthread.h>
#include <mqueue.h>
#include <mntent.h>
#include <csignal>
#include <sys/capability.h>


// Determine the operating mode of the program: 1 - WRITE, 0 - READ
#ifndef MODE
#define MODE 1
#endif

#define QUEUE_NAME "/queue"
mqd_t mqdes;           // Message queue descriptor
mq_attr attr{};        // Message queue attributes
bool thrStatus = true; // Thread status. 1 - running, 0 - stopped

/// Print error message and exit with code 1
void errExit(const std::string &msg) {
    perror(msg.c_str());
    exit(1);
}


/// Handle SIGINT
void handleSignal(int sig) {
    std::cout << "\nInterrupt signal: " << sig << "\n";
    if (mq_unlink(QUEUE_NAME) == -1) errExit("mq_unlink");
    exit(0);
}


/// Print queue attributes
void mqPrintAttrs() {
    if (mq_getattr(mqdes, &attr) == -1) errExit("mq_getattr");
    std::cout << "   flags = " << attr.mq_flags << std::endl;
    std::cout << " max msg = " << attr.mq_maxmsg << std::endl;
    std::cout << "msg size = " << attr.mq_msgsize << std::endl;
    std::cout << "cur msgs = " << attr.mq_curmsgs << std::endl;
}


/// Write data to queue
static void *writeRoutine(void *args) {
    auto *status = (bool *) args;

    // Fetch data and format message
    std::cout << "Поток записи открывает файл описаний файловых систем\n";
    FILE *fp = setmntent("/proc/mounts", "r");
    if (!fp) {
        perror("setmntent");
        *status = false;
        pthread_exit(nullptr);
    }

    std::cout << "Файл открыт. Поток записи начал свою работу\n";
    struct mntent *fs;
    char data[attr.mq_msgsize];
    while (*status) {
        // Fetch mntent data into buffer
        fs = getmntent(fp);
        if (fs) snprintf(data, sizeof(data), "name: %s\tdir: %s\ttype: %s", fs->mnt_fsname, fs->mnt_dir, fs->mnt_type);
        else    snprintf(data, sizeof(data), "EOF: No more data in /proc/mounts");
        std::cout << "WRITE " << data << std::endl;

        // Try to send data into the queue
        if (mq_send(mqdes, data, sizeof(data), 0) == -1) {
            perror("mq_send");
            mq_getattr(mqdes, &attr);
            while (attr.mq_curmsgs == attr.mq_maxmsg) {
                std::cout << "Очередь переполнена. Ожидание чтения сообщений...\n";
                mq_getattr(mqdes, &attr);
                sleep(1);
            }
        }
        sleep(1);
    }
    endmntent(fp);
    std::cout << "Поток записи завершил свою работу\n";

    int *ret = new int(16);
    pthread_exit(ret);
}


/// Get data from queue
static void *readRoutine(void *args) {
    auto *status = (bool *) args;

    std::cout << "Поток чтения начал свою работу\n";
    char data[attr.mq_msgsize];
    while (*status) {
        // Try to receive message
        if (mq_receive(mqdes, data, sizeof(data), nullptr) == -1) {
            perror("mq_receive");
            sleep(1);
            continue;
        }

        // Message received. Print data
        std::cout << "READ " << data << std::endl;
    }
    std::cout << "Поток чтения завершил свою работу\n";

    int *ret = new int(17);
    pthread_exit(ret);
}


int main() {
    // Initialization
    signal(SIGINT, handleSignal);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Программа начала свою работу в режиме: " << MODE << " (1 - WRITE, 0 - READ)\n";

    // Create or open message queue with default parameters
    if (MODE == 1) mqdes = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, nullptr);
    else           mqdes = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, nullptr);
    if (mqdes == (mqd_t)(-1)) errExit("mq_open");
    std::cout << "Очередь открыта. Параметры:\n";
    mqPrintAttrs();

    // Set mq_maxmsg and mq_msgsize if not set.
    if (attr.mq_msgsize != 64) {
        // The mq_maxmsg can't exceed the default value (def=10) without extended rights or SU
        // set maxmsg=15 if CAP_SYS_RESOURCE==eip else 5
        cap_t caps = cap_get_proc();
        if (!caps) errExit("cap_get_proc");
        cap_flag_value_t cap_value;
        if (cap_get_flag(caps, CAP_SYS_RESOURCE, CAP_EFFECTIVE, &cap_value) == -1) {
            cap_free(caps);
            errExit("cap_get_flag");
        }
        attr.mq_maxmsg = (cap_value == CAP_SET) ? 15 : 5;
        cap_free(caps);

        attr.mq_msgsize = 64;

        // set_attr() only affects flags, so the way to change maxmsg & msgsize is to reopen the mq
        mq_close(mqdes);
        mq_unlink(QUEUE_NAME);
        if (MODE == 1) mqdes = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attr);
        else           mqdes = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attr);
        if (mqdes == (mqd_t)(-1)) errExit("mq_open");
        std::cout << "Очередь была переоткрыта. Установлены новые параметры:\n";
        mqPrintAttrs();
    } else {
        std::cout << "Параметры maxmsg и msgsize уже были установлены, изменений не требуется\n";
    }

    // Set O_NONBLOCK via mq_setattr() if not set
    if (attr.mq_flags == 0) {
        attr.mq_flags = O_NONBLOCK;
        if (mq_setattr(mqdes, &attr, nullptr) == -1) errExit("mq_setattr");
        std::cout << "Установлен флаг очереди O_NONBLOCK с помощью mq_setattr:\n";
        mqPrintAttrs();
    } else {
        std::cout << "Параметр flags уже был установлен, изменений не требуется\n";
    }

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
    mq_close(mqdes);
    if (mq_unlink(QUEUE_NAME) == -1) errExit("mq_unlink");
    return 0;
}