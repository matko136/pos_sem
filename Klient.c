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
pthread_t prijSprav;
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

typedef struct klient {
    int cisloKlient;
    int servsock;
    int pocetNacitSprav;
    ZDIEL* zdiel;
} KLIENT;

void* prijimajSpravy(void* arg){
    KLIENT* data = (KLIENT*)arg;
    ZDIEL* zdiel = data->zdiel;
    while(1) {
        while(data->pocetNacitSprav == zdiel->pocetSprav){
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        while(data->pocetNacitSprav < zdiel->pocetSprav) {
            if(zdiel->klientSprav[data->pocetNacitSprav] != data->cisloKlient) {
                printf("Klient %d: %s\n", zdiel->klientSprav[data->pocetNacitSprav], zdiel->spravy[data->pocetNacitSprav]);
            }
            data->pocetNacitSprav++;
        }
    }
}

void* odosielajSpravu(void* arg){
    KLIENT* data = (KLIENT*)arg;
    ZDIEL* zdiel = data->zdiel;
    char buffer[256];
    char buff[7];
    while(1) {
        //printf("Zadaj spravu: ");
        bzero(buffer,256);
        fgets(buffer, 255, stdin);
        char s[7];
        memcpy(s, buffer, 6);
        if(strcmp(s,";odist") == 0) {
            pthread_cancel(prijSprav);
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 1) {
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        int n = write(data->servsock, buffer, strlen(buffer));
        if (n < 0)
        {
            perror("Error writing to socket");
            return 5;
        }
        zdiel->nova = 1;
        zdiel->klientSprav[zdiel->pocetSprav] = data->cisloKlient;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_signal(&zdiel->odoslana);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    char buffer[256];

    if (argc < 3)
    {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        return 1;
    }

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        return 2;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(
            (char*)server->h_addr,
            (char*)&serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(atoi(argv[2]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket");
        return 3;
    }

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error connecting to socket");
        return 4;
    }

    bzero(buffer,256);
    n = read(sockfd, buffer, 255);
    //printf("%d\n",atoi(buffer));
    if (n < 0)
    {
        perror("Error reading from socket");
        return 6;
    }

    const key_t shm_key = (key_t)atoi(buffer);
    int shmid = shmget(shm_key, sizeof(ZDIEL), 0666);
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

    pthread_t klient;
    pthread_t  prijimanieSprav;
    struct zdiel* zdielane = (struct zdiel*)addr;
    //pthread_mutex_lock(zdiel->mutex);
    //printf("%d",zdielane->pocetKlien);
    // printf(zdiel->klientiSock[zdiel->pocetKlien-1]);
    KLIENT klientDat = {zdielane->pocetKlien, sockfd, 0, zdielane};
    //pthread_mutex_unlock(zdiel->mutex);
    pthread_create(&klient, NULL, &odosielajSpravu, &klientDat);
    pthread_create(&prijimanieSprav, NULL, &prijimajSpravy, &klientDat);
    pthread_join(klient, NULL);
    //pthread_join(prijimajSpravy, NULL);

    close(zdielane->klientiSock[klientDat.cisloKlient-1]);

    return 0;
}

