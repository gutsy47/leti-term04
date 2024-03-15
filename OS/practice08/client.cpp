// Клиент. Протокол UDP, сокет INET. Вариант 18 - функция getmntent()

#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <csignal>


struct sockaddr_in addr{}; // Server address
int sockRecv; // Receiving socket
int sockSend; // Sending socket
bool isRecvRun = true; // Receiving thread: 1 - running, 0 - stopped
bool isSendRun = true; // Sending thread: 1 - running, 0 - stopped


/// Print error message and exit with code 1
void errExit(const std::string &msg) {
    perror(msg.c_str());
    exit(1);
}


/// Handle SIGINT
void handleSignal(int sig) {
    std::cout << "\nInterrupt signal: " << sig << "\n";
    close(sockRecv);
    close(sockSend);
    exit(0);
}


/// Receive answer from the server or wait 1 more second if nothing in socket
static void *recvRoutine(void *args) {
    auto *status = (bool *) args;

    char msg[64];
    struct sockaddr_in server{};
    socklen_t addrLen = sizeof(server);
    std::cout << "Поток приема начал работу\n";
    while (*status) {
        ssize_t recvBytes = recvfrom(sockSend, msg, sizeof(msg), 0, (sockaddr*) &server, &addrLen);
        if (recvBytes >= 0) {
            std::cout << "RECV " << inet_ntoa(server.sin_addr) << ':' << server.sin_port << " `" << msg << "`\n";
        } else {
            perror("recv");
            sleep(1);
        }
    }
    std::cout << "Поток приема завершил работу\n";

    int *ret = new int(17);
    pthread_exit(ret);
}


/// Send request to the server each 1 second
static void *sendRoutine(void *args) {
    auto *status = (bool *) args;

    std::cout << "Поток отправки начал работу\n\n";
    unsigned short i = 0;
    while (*status) {
        std::string x = "REQ#" + std::to_string(i++);
        sendto(sockSend, x.c_str(), sizeof(x.c_str()), 0, (sockaddr*) &addr, sizeof(addr));
        std::cout << "SENT " << inet_ntoa(addr.sin_addr) << ':' << addr.sin_port << " `" << x << "`\n";
        sleep(1);
    }
    std::cout << "Поток отправки завершил работу\n";

    int *ret = new int(16);
    pthread_exit(ret);
}


int main() {
    // Initialization
    signal(SIGINT, handleSignal);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::cout << "Клиент запущен\n";

    // Sockets
    sockRecv = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockRecv < 0) errExit("sockRecv");
    fcntl(sockRecv, F_SETFL, O_NONBLOCK);
    sockSend = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockSend < 0) errExit("sockSend");
    fcntl(sockSend, F_SETFL, O_NONBLOCK);
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(7000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
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
    close(sockSend);
    close(sockRecv);
    return 0;
}