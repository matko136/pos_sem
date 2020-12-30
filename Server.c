
#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netdb.h>
pthread_t serv;
pthread_t servPrijmKlien;
typedef struct zdiel {
    pthread_mutex_t mutex;
    pthread_cond_t odoslana;
    pthread_cond_t prijata;
    pthread_cond_t aktualizSpravy;
    char spravy[1000][256];
    int klientSprav[1000];
    int klientiSock[10];
    int pocetKlien;
    int pocetSprav;
    int nova;
} ZDIEL;
typedef struct server {
    int sock;
    struct sockaddr_in serv_addr;
    ZDIEL* zdiel;
} SERVER;

void* skonci() {
    while(1) {
        printf("Pre ukoncenie zadajte 1:");
        char buff[2];
        bzero(buff,1);
        fgets(buff, 2, stdin);
        if (buff[0] == '1') {
            break;
        }
    }
    pthread_cancel(serv);
    pthread_cancel(servPrijmKlien);
    pthread_exit(NULL);
}

void* obsluhujChat(void* arg) {
    serv = pthread_self();
    SERVER* data = (SERVER*)arg;
    ZDIEL* zdiel = data->zdiel;
    char buffer[256];
    while(1) {
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 0) {
            pthread_cond_wait(&zdiel->odoslana, &zdiel->mutex);
        }
        bzero(buffer,256);
        int n = read(zdiel->klientiSock[zdiel->klientSprav[zdiel->pocetSprav]-1], buffer, 255);
        if (n < 0) {
            perror("Error reading from socket");
            return 4;
        }
        strcpy(zdiel->spravy[zdiel->pocetSprav++], buffer);
        zdiel->nova = 0;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_broadcast(&zdiel->prijata);
    }
    return NULL;
}

void* manazujKlientov(void* arg) {
    servPrijmKlien = pthread_self();
    SERVER* data = (SERVER*)arg;
    ZDIEL*
            zdiel = data->zdiel;
    int sockfd = data->sock;
    listen(sockfd, 5);
    struct sockaddr_in serv_addr = data->serv_addr;
    while(1) {
        int newsockfd;
        socklen_t cli_len;
        struct sockaddr_in cli_addr;
        cli_len = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
        pthread_mutex_lock(&zdiel->mutex);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            return 3;
        }
        zdiel->pocetKlien++;
        zdiel->klientiSock[zdiel->pocetKlien-1] = newsockfd;
        pthread_mutex_unlock(&zdiel->mutex);
    }
    return NULL;
}






int main(int argc, char *argv[]) {

    int sockfd;
    struct sockaddr_in serv_addr;
    int n;
    char buffer[256];

    if (argc < 2) {
        fprintf(stderr, "usage %s port\n", argv[0]);
        return 1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return 1;
    }

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket address");
        return 2;
    }

    pthread_t server;
    pthread_t servPrijmKlient;
    pthread_t ukonci;
    pthread_mutex_t mut;
    pthread_cond_t cond1;
    pthread_cond_t cond2;
    pthread_cond_t cond3;



    const key_t shm_key = (key_t)41478545;
    int shmid = shmget(shm_key, sizeof(ZDIEL), 0600|IPC_CREAT|IPC_EXCL);

    if(shmid < 0)
    {
        perror("Failed to create shared memory block:");
        return 10;
    }
    void* addr = shmat(shmid, NULL, 0);
    if(addr == NULL)
    {
        perror("Failed to attach shared memory block:");
        return 11;
    }
    struct zdiel* zdielane = (struct zdiel*)addr;
    zdielane->mutex = mut;
    zdielane->odoslana = cond1;
    zdielane->prijata = cond2;
    zdielane->aktualizSpravy = cond3;
    zdielane->pocetKlien = 0;
    zdielane->pocetSprav = 0;
    zdielane->nova = 0;

    pthread_mutexattr_t mutattr;
    pthread_mutexattr_init(&mutattr);
    pthread_mutexattr_setpshared(&mutattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&zdielane->mutex, &mutattr);

    pthread_condattr_t condattr1;
    pthread_condattr_init(&condattr1);
    pthread_condattr_setpshared(&condattr1, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&zdielane->odoslana, &condattr1);

    pthread_condattr_t condattr2;
    pthread_condattr_init(&condattr2);
    pthread_condattr_setpshared(&condattr2, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&zdielane->prijata, &condattr2);

    pthread_condattr_t condattr3;
    pthread_condattr_init(&condattr3);
    pthread_condattr_setpshared(&condattr3, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&zdielane->aktualizSpravy, &condattr3);
    //ZDIEL zdiel = {&mut, &cond1, &cond2, &cond3, &spravy, &klientSpravy,"" , &klientiSock, 0, 0,0};
    SERVER serv = {sockfd, serv_addr, zdielane};

    pthread_create(&server, NULL, &obsluhujChat, &serv);
    pthread_create(&servPrijmKlient, NULL, &manazujKlientov, &serv);
    pthread_create(&ukonci, NULL, &skonci, NULL);
    pthread_join(ukonci, NULL);
    pthread_join(server, NULL);
    pthread_join(servPrijmKlient, NULL);

    pthread_cond_destroy(&zdielane->odoslana);
    pthread_cond_destroy(&zdielane->prijata);
    pthread_cond_destroy(&zdielane->aktualizSpravy);
    pthread_mutex_destroy(&zdielane->mutex);

    if(shmdt(addr) != 0)
    {
        perror("Failed to detach shared memory block:");
        return 1;
    }
    close(sockfd);


    return 0;
}
