
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
#include <limits.h>
#include "Server.h"
#include "RSA.h"
pthread_t serv;
pthread_t servPrijmKlien;
key_t shm_key_glob;
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
    ZDIEL*
            zdiel = data->zdiel;
    int sockfd = data->sock;
    listen(sockfd, 5);
    while(1) {
        int newsockfd;
        socklen_t cli_len;
        struct sockaddr_in cli_addr;
        cli_len = sizeof(cli_addr);
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 0) {
            pthread_cond_wait(&zdiel->odoslana, &zdiel->mutex);
        }
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            return 3;
        }
        obsluhujKlienta(newsockfd, data);
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
    int sockfd = data->sockPripojenie;
    listen(sockfd, 5);
    while(1) {
        int newsockfd;
        socklen_t cli_len;
        struct sockaddr_in cli_addr;
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            return 3;
        }
        char buffer[256];
        bzero(buffer, 256);
        int a = (int)shm_key_glob;
        sprintf(buffer, "%d", a);
        int n = write(newsockfd, buffer,strlen(buffer));

        if (n < 0)
        {
            perror("Error writing to socket");
            return 5;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    initFirst1000Primes();

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

    int sockPrip;
    struct sockaddr_in serv_addrPrip;

    if (argc < 2) {
        fprintf(stderr, "usage %s port\n", argv[0]);
        return 1;
    }

    bzero((char *) &serv_addrPrip, sizeof(serv_addrPrip));
    serv_addrPrip.sin_family = AF_INET;
    serv_addrPrip.sin_addr.s_addr = INADDR_ANY;
    serv_addrPrip.sin_port = htons(atoi(argv[2]));

    sockPrip = socket(AF_INET, SOCK_STREAM, 0);
    if (sockPrip < 0) {
        perror("Error creating socket");
        return 1;
    }

    if (bind(sockPrip, (struct sockaddr *) &serv_addrPrip, sizeof(serv_addrPrip)) < 0) {
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

    const key_t shm_key = (key_t)rand()%INT_MAX;
    shm_key_glob = shm_key;
    int shmid = shmget(shm_key, sizeof(ZDIEL), 0666|IPC_CREAT|IPC_EXCL);

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
    int klientiId[10];
    char ** prezyvky = (char **)malloc(10*sizeof(char *));
    for(int i = 0; i< 10; i++) {
        prezyvky[i] = (char *)malloc(256*sizeof(char));
    }
    char ** hesla = (char **)malloc(10*sizeof(char *));
    for(int i = 0; i< 10; i++) {
        hesla[i] = (char *)malloc(256*sizeof(char));
    }


    CHATVLAKNO ** chatVlakna = (CHATVLAKNO **)malloc(10*sizeof(CHATVLAKNO *));
    char ** spravyPole[10];
    for(int i = 0; i < 10; i++) {
        chatVlakna[i] = (CHATVLAKNO *)malloc(sizeof(CHATVLAKNO));
        char ** spravy = (char **)malloc(1000*sizeof(char *));
        for(int j = 0; j < 1000; j++) {
            spravy[j] = (char *)malloc(256*sizeof(char));
        }
        int * klientSpr = (int *)malloc(1000*sizeof(int));
        int * klienti = (int *)malloc(10*sizeof(int));
        char * nazov = (char *)malloc(256*sizeof(char));
        spravyPole[i] = spravy;
        chatVlakna[i]->spravy = spravy;
        chatVlakna[i]->klientSprav = klientSpr;
        chatVlakna[i]->nazov = nazov;
        chatVlakna[i]->klienti = klienti;
        chatVlakna[i]->pocetSprav = 0;
        chatVlakna[i]->shm_key_zdiel_Vlak = 0;
    }

    SERVER serv = {0, klientiId, prezyvky, hesla, sockfd, sockPrip, zdielane, chatVlakna, 0};

    int num;
    char line[256];
    bzero(line, 256);
    FILE *fptr;
    fptr = fopen("serv_ud.txt","r");
    if(fptr != NULL) {
        fscanf(fptr,"%s", &line);
        fscanf(fptr, "%d", &num);
        serv.pocVlakien = num;
        fscanf(fptr, "%d", &num);
        serv.pocetKlientov = num;
        FILE *fptr2;
        fptr2 = fopen("serv_hsl.txt","r");
        fscanf(fptr2,"%s", &line);
        for(int i = 0; i < serv.pocetKlientov; i++) {
            bzero(line, 256);
            fscanf(fptr,"%s", &line);
            strcat(line, "\n");
            strcpy(serv.prezyvky[i], line);
            bzero(line, 256);
            fscanf(fptr2, "%s", &line);
            strcat(line, "\n");
            strcpy(serv.hesla[i], line);
        }
        fclose(fptr);
        fclose(fptr2);

        for(int i = 0; i < serv.pocVlakien; i++) {
            FILE * fptr3;
            char filename[256];
            bzero(filename, 256);
            sprintf(filename,"%d", i+1);
            strcat(filename,"vlak_ud.txt");
            fptr3 = fopen(filename, "r");
            fscanf(fptr3,"%s", &line);
            fscanf(fptr3, "%d", &num);
            serv.chatvlakno[i]->cislo = num;
            bzero(line, 256);
            fscanf(fptr3,"%s", &line);
            strcat(line, "\n");
            strcpy(serv.chatvlakno[i]->nazov, line);
            fscanf(fptr3, "%d", &num);
            serv.chatvlakno[i]->pocetKlientov = num;
            for(int j = 0; j < serv.chatvlakno[i]->pocetKlientov; j++) {
                fscanf(fptr3, "%d", &num);
                serv.chatvlakno[i]->klienti[j] = num;
            }
            fclose(fptr3);
            FILE * fptr4;
            bzero(filename, 256);
            sprintf(filename,"%d", i+1);
            strcat(filename,"vlak_spravy.txt");
            fptr4 = fopen(filename, "r");
            fscanf(fptr4,"%s", &line);
            fscanf(fptr4, "%d\n", &num);
            serv.chatvlakno[i]->pocetSprav = num;
            for(int j = 0; j < serv.chatvlakno[i]->pocetSprav; j++) {
                bzero(line, 256);
                fgets(line, 256, fptr4);
                //fscanf(fptr4,"%s", &line);
                strcpy(serv.chatvlakno[i]->spravy[j], line);
                fscanf(fptr4, "%d\n", &num);
                serv.chatvlakno[i]->klientSprav[j] = num;
            }
            serv.chatvlakno[i]->shm_key_zdiel_Vlak = vytvorZdielaneVlakno(serv.chatvlakno[i]->cislo, serv.chatvlakno[i]->nazov, serv.chatvlakno[i]->pocetSprav);
            fclose(fptr4);
        }
    } else {
        fptr = fopen("serv_ud.txt","w");
        bzero(line, 256);
        strcpy(line, "---Server---udaje---prezyvky---");
        fprintf(fptr,"%s\n", line);
        fprintf(fptr,"%d\n", 0);
        fprintf(fptr,"%d\n", 0);
        fclose(fptr);
        FILE *fptr2;
        fptr2 = fopen("serv_hsl.txt","w");
        bzero(line, 256);
        strcpy(line, "---Server---hesla---");
        fprintf(fptr2,"%s\n", line);
        fclose(fptr2);
    }

    pthread_create(&server, NULL, &obsluhujChat, &serv);
    pthread_create(&servPrijmKlient, NULL, &manazujKlientov, &serv);
    pthread_create(&ukonci, NULL, &skonci, NULL);
    pthread_join(server, NULL);
    pthread_join(ukonci, NULL);
    pthread_join(servPrijmKlient, NULL);

    pthread_cond_destroy(&zdielane->odoslana);
    pthread_cond_destroy(&zdielane->prijata);
    pthread_cond_destroy(&zdielane->aktualizSpravy);
    pthread_mutex_destroy(&zdielane->mutex);

    pthread_mutexattr_destroy(&mutattr);
    pthread_condattr_destroy(&condattr1);
    pthread_condattr_destroy(&condattr2);
    pthread_condattr_destroy(&condattr3);

    for(int i = 0; i < 10; i++) {
        const key_t zdielKey = serv.chatvlakno[i]->shm_key_zdiel_Vlak;
        int shmid = shmget(zdielKey, sizeof(CHATVLAKNOZDIEL), 0666);
        if(shmid < 0)
        {
            perror("Failed to open shared memory block:");
            return 10;
        }

        void* addr = shmat(shmid, NULL, 0);

        if(addr == NULL)
        {
            perror("Failed to attach shared memory block:");
            return 11;
        }
        CHATVLAKNOZDIEL* vlZdiel = (CHATVLAKNOZDIEL*)addr;
        pthread_mutex_destroy(&vlZdiel->mutex);
        pthread_cond_destroy(&vlZdiel->odoslana);
        pthread_mutexattr_destroy(&vlZdiel->mutat);
        pthread_condattr_destroy(&vlZdiel->condat);
    }

    for(int i = 0; i < 10 ; i++) {
        free(chatVlakna[i]->klienti);
        free(chatVlakna[i]->nazov);
        free(chatVlakna[i]->klientSprav);
        free(chatVlakna[i]);
    }
    free(chatVlakna);
    for(int i = 0; i < 10; i++) {
        free(prezyvky[i]);
        free(hesla[i]);
    }
    free(prezyvky);
    free(hesla);
    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 1000; j++) {
            free((spravyPole[i])[j]);
        }
        free(spravyPole[i]);
    }

    if(shmdt(addr) != 0)
    {
        perror("Failed to detach shared memory block:");
        return 1;
    }
    close(sockfd);



    return 0;
}
