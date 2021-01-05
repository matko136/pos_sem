//
// Created by matob on 2. 1. 2021.
//

#ifndef CHATAPP_KLIENT_H
#define CHATAPP_KLIENT_H




typedef struct zdiel {
    pthread_mutex_t mutex;
    pthread_cond_t odoslana;
    pthread_cond_t prijata;
    pthread_cond_t aktualizSpravy;
    int nova;
} ZDIEL;

typedef struct chatVlaknoZdiel {
    pthread_mutex_t mutex;
    pthread_cond_t odoslana;
    int cislo;
    char* nazov;
    int pocetSprav;
} CHATVLAKNOZDIEL;

typedef struct klient {
    int cisloKlient;
    int * pocetNacitSprav;
    char * heslo;
    int pocetVlakien;
    CHATVLAKNOZDIEL ** chatvlakno;
    ZDIEL* zdiel;
    int aktChat;
} KLIENT;

pthread_t prijSprav;
struct sockaddr_in serv_addr;
struct sockaddr_in serv_addrStart;
char prezyvka[256];
int pocetVlakien;
KLIENT *klData;
int * makeNewChatIds;
int * addToChatIds;
int countAddToChat = 0;
int countMakeNewChat = 0;
int countChatToConnect = 0;

char* writeAndReadSocket(char* input, int serv_address, bool connectS, int* destSourcSock) {
    char * buffer= malloc(256*sizeof (char));
    int sockfd;
    if(connectS) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Error creating socket");
            return 3;
        }
        if (serv_address == 1) {
            if (connect(sockfd, (struct sockaddr *) &serv_addrStart, sizeof(serv_addrStart)) < 0) {
                perror("Error connecting to socket");
                return 4;
            }
        } else {
            if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                perror("Error connecting to socket");
                return 4;
            }
        }
        *destSourcSock = sockfd;
    } else {
        sockfd = *destSourcSock;
    }
    //bzero(buffer, 256);
    //strcpy(buffer, input);
    if(serv_address != 1) {
        int n = write(sockfd, input, strlen(buffer));
    }
    /*if (n < 0)
    {
        perror("Error writing to socket");
        return 5;
    }*/
    bzero(buffer,256);
    int n = read(sockfd, buffer, 255);
    if (n < 0)
    {
        perror("Error reading from socket");
        return 6;
    }
    return buffer;
}

int odosliPoziadavku(int poziadavka, ZDIEL* zdiel, char * sprava, int cisloKl, int cisloVlak, int cisloSpr) {
    char buffer[256];
    char pass[256];
    int ret = 0;
    if(poziadavka == 1) {
        bool uspesne = false;
        while(!uspesne) {
            char meno[256];
            printf("Zadajte meno:\n");
            bzero(meno, 256);
            fgets(meno, 255, stdin);
            char heslo[256];
            printf("Zadajte heslo:\n");
            bzero(heslo, 256);
            fgets(heslo, 255, stdin);
            bzero(buffer, 256);
            strcpy(buffer, "1");

            pthread_mutex_lock(&zdiel->mutex);
            while(zdiel->nova == 1) {
                pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
            }
            zdiel->nova = 1;
            pthread_mutex_unlock(&zdiel->mutex);
            pthread_cond_signal(&zdiel->odoslana);
            int* sockfd = malloc(sizeof(int));
            writeAndReadSocket(buffer,2,true, sockfd);

            writeAndReadSocket(meno,2, false, sockfd);

            bzero(buffer, 256);
            strcpy(buffer, writeAndReadSocket(heslo,2, false, sockfd));

            /*bzero(buffer, 256);
            strcpy(buffer, "1");
            n = write(sockfd, buffer, strlen(buffer));
            n = read(sockfd, buffer, 255);
            bzero(buffer, 256);
            strcpy(buffer, meno);
            n = write(sockfd, buffer, strlen(buffer));
            n = read(sockfd, buffer, 255);
            bzero(buffer, 256);
            strcpy(buffer, heslo);
            n = write(sockfd, buffer, strlen(buffer));
            bzero(buffer, 256);
            n = read(sockfd, buffer, 255);*/
            if (strcmp(buffer,"-1") == 0) {
                printf("Zle zadane meno alebo heslo:\n");
            } else {
                printf("Uspesne prihlasenie\n");
                strcpy(prezyvka, meno);
                ret = atoi(buffer);
                strcpy(pass, heslo);
                uspesne = true;
            }
            free(sockfd);
        }
        return ret;
    } else if(poziadavka == 2) {
        bool uspesne = false;
        while(!uspesne) {
            char meno[256];
            printf("Zadajte meno:\n");
            bzero(meno, 256);
            fgets(meno, 255, stdin);
            char heslo[256];
            printf("Zadajte heslo:\n");
            bzero(heslo, 256);
            fgets(heslo, 255, stdin);
            bzero(buffer, 256);
            strcpy(buffer, "2");

            pthread_mutex_lock(&zdiel->mutex);
            while(zdiel->nova == 1) {
                pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
            }
            zdiel->nova = 1;
            pthread_mutex_unlock(&zdiel->mutex);
            pthread_cond_signal(&zdiel->odoslana);

            int* sockfd = malloc(sizeof(int));
            writeAndReadSocket(buffer,2,true, sockfd);

            writeAndReadSocket(meno,2,false, sockfd);

            bzero(buffer, 256);
            strcpy(buffer, writeAndReadSocket(heslo,2,false, sockfd));

            if(strcmp(buffer,"-1") == 0) {
                printf("Prezyvka uz pouzita, zadajte inu:\n");
            } else {
                printf("Registracia uspesna, ste prihlaseni\n");
                strcpy(prezyvka, meno);
                ret = atoi(buffer);
                strcpy(pass, heslo);
                uspesne = true;
            }
            free(sockfd);
        }
        return ret;
    } else if(poziadavka == 3) {
        bzero(buffer, 256);
        strcpy(buffer, "3");
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 1) {
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        zdiel->nova = 1;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_signal(&zdiel->odoslana);
        int* sockfd = malloc(sizeof(int));
        writeAndReadSocket(buffer,2,true, sockfd);

        bzero(buffer, 256);
        strcpy(buffer, sprava);
        writeAndReadSocket(buffer, 2,false, sockfd);

        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloVlak);
        writeAndReadSocket(buffer, 2,false, sockfd);

        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloKl);
        writeAndReadSocket(buffer, 2,false, sockfd);
        free(sockfd);
    } else if(poziadavka == 4) {
        char prezyvkaKlienSpr[256];
        char sprava[256];
        bzero(buffer, 256);
        strcpy(buffer, "4");
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 1) {
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        zdiel->nova = 1;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_signal(&zdiel->odoslana);
        int* sockfd = malloc(sizeof(int));
        writeAndReadSocket(buffer,2,true, sockfd);

        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloVlak);
        writeAndReadSocket(buffer, 2,false, sockfd);

        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloKl);
        writeAndReadSocket(buffer, 2,false, sockfd);

        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloSpr);
        bzero(prezyvkaKlienSpr, 256);
        strcpy(prezyvkaKlienSpr, writeAndReadSocket(buffer,2,false, sockfd));


        bzero(sprava, 256);
        strcpy(sprava, writeAndReadSocket(buffer,2,false, sockfd));

        if(strcmp(prezyvka,prezyvkaKlienSpr) == 0) {
            printf("Vy: %s\n", sprava);
        } else {
            prezyvkaKlienSpr[strcspn(prezyvkaKlienSpr, "\n")] = '\000';
            printf("%s: %s\n", prezyvkaKlienSpr, sprava);
        }
        free(sockfd);
        return 0;
    }else if(poziadavka == 5) {
        bzero(buffer, 256);
        strcpy(buffer, "5");
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 1) {
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        zdiel->nova = 1;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_signal(&zdiel->odoslana);
        int* sockfd = malloc(sizeof(int));
        writeAndReadSocket(buffer,2,true, sockfd);

        int count = 0;
        char vlakno[256];
        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloKl);
        bzero(vlakno,256);
        writeAndReadSocket(buffer, 2,false, sockfd);
        while(strcmp("-1",strcpy(vlakno, writeAndReadSocket(buffer, 2,false, sockfd))) != 0) {
            char key[256];
            bzero(key,256);
            strcpy(key, writeAndReadSocket(buffer, 2,false, sockfd));
            printf("%d. %s", count+1, vlakno);
            count++;
            if(count > pocetVlakien) {
                printf("Reg vlakno\n");
                const key_t shm_key = (key_t)atoi(key);
                int shmid = shmget(shm_key, sizeof(CHATVLAKNOZDIEL), 0666);
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
                klData->chatvlakno[pocetVlakien++] = vlZdiel;
            }
            bzero(vlakno,256);
            //strcpy(vlakno, writeAndReadSocket(buffer, 2));
        }
        if(count == 0) {
            printf("Ziadne vlakna neboli najdene\n");
        }
        countChatToConnect = count;
        free(sockfd);
        return 0;
    }else if(poziadavka == 6) {
        bzero(buffer, 256);
        strcpy(buffer, "6");
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 1) {
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        zdiel->nova = 1;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_signal(&zdiel->odoslana);
        int* sockfd = malloc(sizeof(int));
        writeAndReadSocket(buffer,2,true, sockfd);

        char cisloKli[256];
        int count = 0;
        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloKl);
        bzero(cisloKli, 256);
        writeAndReadSocket(buffer, 2, false, sockfd);
        int ids[10];
        while(strcmp("-1",strcpy(cisloKli, writeAndReadSocket(buffer, 2, false, sockfd)))) {
            count++;
            char prezyvkaa[256];
            bzero(prezyvkaa, 256);
            strcpy(prezyvkaa, writeAndReadSocket(buffer, 2, false, sockfd));
            printf("%d. %s", count, prezyvkaa);
            ids[count-1] = atoi(cisloKli);
            bzero(cisloKli,256);
            //strcpy(cisloKli, writeAndReadSocket(buffer, 2));
        }
        if(count == 0) {
            ids[0] = 0;
        }
        makeNewChatIds = ids;
        countMakeNewChat = count;
        free(sockfd);
        return 0;
    }else if(poziadavka == 7) {
        bzero(buffer, 256);
        strcpy(buffer, "7");
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 1) {
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        zdiel->nova = 1;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_signal(&zdiel->odoslana);
        int* sockfd = malloc(sizeof(int));
        writeAndReadSocket(buffer,2,true, sockfd);

        int klientId = cisloKl;
        int klientToAdd = cisloSpr;
        char nazov[256];
        bzero(nazov, 256);
        strcpy(nazov, sprava);
        bzero(buffer, 256);
        sprintf(buffer,"%d", klientId);
        writeAndReadSocket(buffer, 2, false, sockfd);
        bzero(buffer, 256);
        sprintf(buffer,"%d", klientToAdd);
        writeAndReadSocket(buffer, 2, false, sockfd);
        writeAndReadSocket(nazov, 2, false, sockfd);
        free(sockfd);
        return 0;
    } else if(poziadavka == 8) {
        bzero(buffer, 256);
        strcpy(buffer, "8");
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 1) {
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        zdiel->nova = 1;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_signal(&zdiel->odoslana);
        int* sockfd = malloc(sizeof(int));
        writeAndReadSocket(buffer,2,true, sockfd); // na server '8'

        //int klientId = cisloKl;
        int klientToAdd = cisloSpr;

        bzero(buffer, 256);
        sprintf(buffer,"%d", klientToAdd);
        writeAndReadSocket(buffer, 2, false, sockfd);

        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloVlak);
        writeAndReadSocket(buffer, 2, false, sockfd);

        free(sockfd);
        return 0;
    }
    else if(poziadavka == 9) {
        int count = 0;
        bzero(buffer, 256);
        strcpy(buffer, "9");
        pthread_mutex_lock(&zdiel->mutex);
        while(zdiel->nova == 1) {
            pthread_cond_wait(&zdiel->prijata, &zdiel->mutex);
        }
        zdiel->nova = 1;
        pthread_mutex_unlock(&zdiel->mutex);
        pthread_cond_signal(&zdiel->odoslana);
        int* sockfd = malloc(sizeof(int));
        writeAndReadSocket(buffer,2,true, sockfd);
        //////

        char cisloKli[256];

        bzero(buffer, 256);
        sprintf(buffer, "%d", cisloVlak); // odosielanie cislo vlakna
        writeAndReadSocket(buffer, 2, false, sockfd);

        int ids[10];
        while(strcmp("-1",strcpy(cisloKli, writeAndReadSocket(buffer, 2, false, sockfd)))) {
            count++;
            char prezyvkaa[256];
            bzero(prezyvkaa, 256);
            strcpy(prezyvkaa, writeAndReadSocket(buffer, 2, false, sockfd));
            printf("%d. %s", count, prezyvkaa);
            ids[count-1] = atoi(cisloKli);
            bzero(cisloKli,256);
        }
        if(count == 0) {
            ids[0] = 0;
        }
        addToChatIds = ids;
        countAddToChat = count;
        free(sockfd);
        return 0;
    } else {
        printf("Ukoncuje sa\n");
        sleep(3);
        return 0;
    }
}

#endif //CHATAPP_KLIENT_H
