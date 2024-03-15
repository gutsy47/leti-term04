// Сервер. Протокол UDP, сокет INET. Вариант 18 - функция getmntent()

#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <mntent.h>
#include <csignal>
#include <cstring>
#include <queue>


struct sockaddr_in addr{}; // Server address
int sockRecv; // Receiving socket
int sockSend; // Sending socket
bool isRecvRun = true; // Receiving thread: 1 - running, 0 - stopped
bool isSendRun = true; // Sending thread: 1 - running, 0 - stopped
std::queue<std::string> requests; // Client requests queue
pthread_mutex_t mutex; // Mutex used to protect requests queue


/// Print error message and exit with code 1
void errExit(const std::string &msg) {
    perror(msg.c_str());
    exit(1);
}


/// Handle SIGINT
void handleSignal(int sig) {
    std::cout << "\nInterrupt signal: " << sig << "\n";
    pthread_mutex_destroy(&mutex);
    close(sockRecv);
    close(sockSend);
    exit(0);
}


/// Wait for request from the client, push msg to the queue
static void *recvRoutine(void *args) {
    auto *status = (bool *) args;

    char msg[64];
    socklen_t addrLen = sizeof(addr);
    std::cout << "Поток приема начал работу\n";
    while (*status) {
        ssize_t recvBytes = recvfrom(sockRecv, msg, sizeof(msg), 0, (sockaddr*) &addr, &addrLen);
        if (recvBytes >= 0) {
            pthread_mutex_lock(&mutex);       // Lock the queue (critical section)
            requests.emplace(msg, recvBytes); // Copy data to the queue
            pthread_mutex_unlock(&mutex);     // Unlock the queue
            std::cout << "RECV " << inet_ntoa(addr.sin_addr) << ':' << addr.sin_port << " `" << msg << "`\n";
        } else {
            perror("recv");
            sleep(1);
        }
    }
    std::cout << "Поток приема завершил работу\n";

    int *ret = new int(17);
    pthread_exit(ret);
}


/// Answer the request. Pop from queue and send to the client
static void *sendRoutine(void *args) {
    auto *status = (bool *) args;

    // Open /proc/mounts via mntent before sending
    std::cout << "Поток отправки открывает /proc/mounts\n";
    FILE *fp = setmntent("/proc/mounts", "r");
    if (!fp) {
        perror("setmntent");
        *status = false;
        pthread_exit(nullptr);
    }

    char msg[64];
    struct mntent *fs;
    std::cout << "Поток отправки начал работу\n\n";
    while (*status) {
        // Parse queue
        pthread_mutex_lock(&mutex);
        if (requests.empty()) {
            // Nothing to do, unlock and wait
            pthread_mutex_unlock(&mutex);
            sleep(1);
            continue;
        }
        strcpy(msg, requests.front().c_str());
        requests.pop();
        pthread_mutex_unlock(&mutex);
        int i = (int) std::strtol(&msg[4], nullptr, 10); // Request number

        // Generate the message
        fs = getmntent(fp);
        if (fs) snprintf(msg, sizeof(msg), "ANS#%d={name:%s, dir:%s}", i, fs->mnt_fsname, fs->mnt_dir);
        else    snprintf(msg, sizeof(msg), "ANS#%d=EOF:No more data in /proc/mounts", i);

        // Send the message
        sendto(sockSend, msg, sizeof(msg), 0, (sockaddr*) &addr, sizeof(addr));
        std::cout << "SENT " << inet_ntoa(addr.sin_addr) << ':' << addr.sin_port << " `" << msg << "`\n";
    }
    endmntent(fp);
    std::cout << "Поток отправки завершил работу\n";

    int *ret = new int(16);
    pthread_exit(ret);
}


int main() {
    // Initialization
    signal(SIGINT, handleSignal);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    pthread_mutex_init(&mutex, nullptr);
    std::cout << "Сервер запущен\n";

    // Sockets
    sockRecv = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockRecv < 0) errExit("sockRecv");
    fcntl(sockRecv, F_SETFL, O_NONBLOCK);
    sockSend = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockSend < 0) errExit("sockSend");
    fcntl(sockSend, F_SETFL, O_NONBLOCK);

    int optVal = 1;
    if (setsockopt(sockRecv, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) < 0) errExit("setsockopt");
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(7000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(sockRecv, (sockaddr*) &addr, sizeof(addr)) < 0) errExit("bind");
    std::cout << "Открыты сокеты для приема и отправки\n";

    // Threads
    pthread_t thrRecv;
    pthread_t thrSend;
    pthread_create(&thrRecv, nullptr, recvRoutine, &isRecvRun);
    pthread_create(&thrSend, nullptr, sendRoutine, &isSendRun);

    // Wait till user's input and the thread completion
    std::cout << "Нажмите <ENTER> для выхода\n";
    getchar();
    isSendRun = false;
    isRecvRun = false;

    // Wait for the threads to finish. Complete the program
    int *rvSend, *rvRecv;
    int resSend = pthread_join(thrSend, (void **) &rvSend);
    int resRecv = pthread_join(thrRecv, (void **) &rvRecv);
    std::cout << "thrRecv завершился с кодом: " << resRecv << ", вернув " << (rvRecv ? *rvRecv : 0) << std::endl;
    std::cout << "thrSend завершился с кодом: " << resSend << ", вернув " << (rvSend ? *rvSend : 0) << std::endl;
    delete rvSend;
    delete rvRecv;

    // Clean up
    pthread_mutex_destroy(&mutex);
    close(sockSend);
    close(sockRecv);
    return 0;
}