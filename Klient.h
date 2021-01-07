//
// Created by matob on 2. 1. 2021.
//

#ifndef CHATAPP_KLIENT_H
#define CHATAPP_KLIENT_H

#include "RSA.h"


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
    pthread_mutexattr_t mutat;
    pthread_condattr_t condat;
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

pthread_t * prijSprav;
struct sockaddr_in serv_addr;
struct sockaddr_in serv_addrStart;
char prezyvka[256];
KLIENT *klData;
int * makeNewChatIds;
int * addToChatIds;
int countAddToChat = 0;
int countMakeNewChat = 0;
int countChatToConnect = 0;

unsigned long long int * writeAndReadSocketCipher(unsigned long long int * input, int serv_address, bool connectS, int* destSourcSock) {
    unsigned long long int * buffer= malloc(256*sizeof (unsigned long long int));
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
    //bzero(buffer, 256*sizeof(unsigned long long int));
    if(serv_address != 1) {
        int n = write(sockfd, input, 256*sizeof(unsigned long long int));
    }
    bzero(buffer, 256*sizeof(unsigned long long int));
    int n = read(sockfd, buffer, 256*sizeof(unsigned long long int));
    if (n < 0)
    {
        perror("Error reading from socket");
        return 6;
    }
    return buffer;
}

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
    bzero(buffer, 256);
    strcpy(buffer, input);
    if(serv_address != 1) {
        int n = write(sockfd, buffer, strlen(buffer));
    }
    bzero(buffer,256);
    int n = read(sockfd, buffer, 255);
    if (n < 0)
    {
        perror("Error reading from socket");
        return 6;
    }
    return buffer;
}

bool strcmpare(char * cmp, char* get, char * wrt, int * sockfd) {
    char * prijate = writeAndReadSocket(wrt, 2,false, sockfd);
    strcpy(get, prijate);
    bool ret = strcmp(cmp, prijate) == 0;
    free(prijate);
    return ret;
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
            free(writeAndReadSocket(buffer,2,true, sockfd)); // odosl cisla poziad
            unsigned long long int keys[2];
            write(*sockfd, buffer,strlen(buffer));
            int n = read(*sockfd, keys, sizeof(keys)); // 1 read kluc
            unsigned long long int * cipher = zasifruj(meno, keys[0], keys[1]);
            free(writeAndReadSocketCipher(cipher,2,false, sockfd));//2 write zasif meno
            free(cipher);
            write(*sockfd, buffer,strlen(buffer));
            read(*sockfd, keys, sizeof(keys)); // 3 read kluc
            cipher = zasifruj(heslo, keys[0], keys[1]);
            unsigned long long int * genKeys = generujKluceRSA();
            write(*sockfd, genKeys, 2*sizeof(unsigned long long int)); // 4 write kluc
            read(*sockfd, buffer,255);
            unsigned long long int * ans = writeAndReadSocketCipher(cipher,2,false, sockfd); // 5 write heslo pouz
            free(cipher);
            char cislPouz[256];
            bzero(cislPouz, 256);
            sprintf(cislPouz, "%d", modularPow((unsigned long long int)ans[0],genKeys[2],genKeys[1]));
            free(ans);
            free(genKeys);
            if (strcmp(cislPouz,"-1") == 0) {
                printf("Zle zadane meno alebo heslo:\n");
            } else {
                printf("Uspesne prihlasenie\n");
                strcpy(prezyvka, meno);
                ret = atoi(cislPouz);
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
            free(writeAndReadSocket(buffer,2,true, sockfd));// cislo poz

            unsigned long long int keys[2];
            write(*sockfd, buffer,strlen(buffer));
            int n = read(*sockfd, keys, sizeof(keys)); // 1 read klucov
            unsigned long long int * cipher = zasifruj(meno, keys[0], keys[1]);
            free(writeAndReadSocketCipher(cipher,2,false, sockfd)); // 2 write sifr meno
            free(cipher);
            write(*sockfd, buffer,strlen(buffer));
            read(*sockfd, keys, sizeof(keys)); // 3 read kluc
            cipher = zasifruj(heslo, keys[0], keys[1]);
            unsigned long long int * genKeys = generujKluceRSA();
            write(*sockfd, genKeys, 2*sizeof(unsigned long long int)); // 4 write kluc
            read(*sockfd, buffer,255);
            unsigned long long int * ans = writeAndReadSocketCipher(cipher,2,false, sockfd); // 5 heslo pouz
            free(cipher);
            char cislPouz[256];
            bzero(cislPouz, 256);
            sprintf(cislPouz, "%d", modularPow((unsigned long long int)ans[0],genKeys[2],genKeys[1]));
            free(ans);
            free(genKeys);

            if(strcmp(cislPouz,"-1") == 0) {
                printf("Prezyvka uz pouzita, zadajte inu:\n");
            }  else {
                printf("Registracia uspesna, ste prihlaseni\n");
                strcpy(prezyvka, meno);
                ret = atoi(cislPouz);
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
        free(writeAndReadSocket(buffer,2,true, sockfd));
        write(*sockfd, buffer,strlen(buffer));
        char sprNaSif[256];
        //citam kluc od serveru
        unsigned long long int keys[2];
        int n = read(*sockfd, keys, sizeof(keys));
        //sifrujem spravu
        unsigned long long int * cipher = zasifruj(sprava, keys[0], keys[1]);
        //odoslanie zas spravy
        free(writeAndReadSocketCipher(cipher,2,false, sockfd)); // 2 write sifr sprava
        free(cipher);

        write(*sockfd, buffer,strlen(buffer));
        //citam kluc od serveru
        n = read(*sockfd, keys, sizeof(keys));
        //sifrujem vlakno
        bzero(sprNaSif, 256);
        sprintf(sprNaSif, "%d", cisloVlak);
        cipher = zasifruj(sprNaSif, keys[0], keys[1]);
        //odoslanie zas vlakna
        free(writeAndReadSocketCipher(cipher,2,false, sockfd)); // 2 write sifr vlakno
        free(cipher);

        write(*sockfd, buffer,strlen(buffer));
        //citam kluc od serveru
        n = read(*sockfd, keys, sizeof(keys));
        //sifrujem vlakno
        bzero(sprNaSif, 256);
        sprintf(sprNaSif,"%d",  cisloKl);
        cipher = zasifruj(sprNaSif, keys[0], keys[1]);
        //odoslanie zas vlakna
        free(writeAndReadSocketCipher(cipher,2,false, sockfd)); // 2 write sifr vlakno
        free(cipher);
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

        free(writeAndReadSocket(buffer,2,true, sockfd)); /// poslanie poziadavky serveru a chceck
        write(*sockfd, buffer,strlen(buffer)); ///
        unsigned long long int keys[2];
        int n = read(*sockfd, keys, sizeof(keys));
        //write(*sockfd, buffer,strlen(buffer)); ///
        ///odoslanie cisloVlakna

        unsigned long long int returnCislo[1];
        returnCislo[0] = modularPow((unsigned long long int)cisloVlak,keys[0],keys[1]);
        write(*sockfd, returnCislo,sizeof(returnCislo));
        //write(*sockfd, modularPow((unsigned long long int)cisloVlak,keys[0],keys[1]),  sizeof(unsigned long long int)); ///maybe treba premenu
        //read(*sockfd, keys, sizeof(keys));

        ///cisSprav
        n = read(*sockfd, keys, sizeof(keys));
        returnCislo[0] = modularPow((unsigned long long int)cisloSpr,keys[0],keys[1]);
        write(*sockfd, returnCislo,sizeof(returnCislo));
        //write(*sockfd, modularPow((unsigned long long int)cisloSpr,keys[0],keys[1]), sizeof(unsigned long long int)); ///maybe treba premenu
        n = read(*sockfd, keys, sizeof(keys));///

        //prijatieSpravy
        unsigned long long int * genKeys = generujKluceRSA();
        write(*sockfd, genKeys, 2*sizeof(unsigned long long int));
        unsigned long long int sifrSprava[256];
        read(*sockfd, sifrSprava, 256 * sizeof(unsigned long long int));
        char * tempPrijSprava = desifruj(sifrSprava, genKeys[2], genKeys[1]);
        strcpy(sprava, tempPrijSprava);
        free(tempPrijSprava);
        free(genKeys);


        //prijatiePrezyvky
        genKeys = generujKluceRSA();
        write(*sockfd, genKeys, 2*sizeof(unsigned long long int));
        read(*sockfd, sifrSprava, 256 *  sizeof(unsigned long long int));
        tempPrijSprava = desifruj(sifrSprava, genKeys[2], genKeys[1]);
        strcpy(prezyvkaKlienSpr, tempPrijSprava);
        free(tempPrijSprava);
        free(genKeys);




//        unsigned long long int * genKeys = generujKluceRSA();
//        write(*sockfd, genKeys, 2*sizeof(unsigned long long int)); // 3 write kluc
//        unsigned long long int sifCisSprava[1];
//        read(*sockfd, sifCisSprava[0], sizeof(unsigned long long int));
//        char * tempCisSprava = desifruj(sifCisSprava[0], genKeys[2], genKeys[1]);
//
//        bzero(buffer, 256);
//        sprintf(buffer,"%d", cisloVlak);
//        free(writeAndReadSocket(buffer, 2,false, sockfd));
//
//        bzero(buffer, 256);
//        sprintf(buffer,"%d", cisloKl);


//
//        free(writeAndReadSocket(buffer, 2,false, sockfd));
//        bzero(buffer, 256);
//        sprintf(buffer,"%d", cisloSpr);
//        bzero(prezyvkaKlienSpr, 256);

//        char * ans = writeAndReadSocket(buffer,2,false, sockfd);
//        strcpy(prezyvkaKlienSpr, ans);
//        free(ans);
//
//        bzero(sprava, 256);
//        ans = writeAndReadSocket(buffer,2,false, sockfd);
//        strcpy(sprava, ans);
//        free(ans);

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
        free(writeAndReadSocket(buffer,2,true, sockfd));

        int count = 0;
        char vlakno[256];
        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloKl);
        bzero(vlakno,256);
        free(writeAndReadSocket(buffer, 2,false, sockfd));
        //bool strcmpare(char * cmp, char* get, char * wrt, int sockfd)
        while(!strcmpare("-1", vlakno, buffer, sockfd)) {
            char key[256];
            bzero(key,256);
            char * ans = writeAndReadSocket(buffer, 2,false, sockfd);
            strcpy(key, ans);
            free(ans);
            printf("%d. %s", count+1, vlakno);
            count++;
            if(count > klData->pocetVlakien) {
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

                klData->chatvlakno[klData->pocetVlakien++] = vlZdiel;
            }
            bzero(vlakno,256);
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
        free(writeAndReadSocket(buffer,2,true, sockfd));

        char cisloKli[256];
        int count = 0;
        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloKl);
        bzero(cisloKli, 256);
        free(writeAndReadSocket(buffer, 2, false, sockfd));
        int ids[10];
        while(!strcmpare("-1", cisloKli, buffer, sockfd)) {
            count++;
            char prezyvkaa[256];
            bzero(prezyvkaa, 256);
            char * ans = writeAndReadSocket(buffer, 2, false, sockfd);
            strcpy(prezyvkaa, ans);
            free(ans);
            printf("%d. %s", count, prezyvkaa);
            ids[count-1] = atoi(cisloKli);
            bzero(cisloKli,256);
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
        free(writeAndReadSocket(buffer,2,true, sockfd));

        int klientId = cisloKl;
        int klientToAdd = cisloSpr;
        char nazov[256];
        bzero(nazov, 256);
        strcpy(nazov, sprava);
        bzero(buffer, 256);
        sprintf(buffer,"%d", klientId);
        free(writeAndReadSocket(buffer, 2, false, sockfd));
        bzero(buffer, 256);
        sprintf(buffer,"%d", klientToAdd);
        free(writeAndReadSocket(buffer, 2, false, sockfd));
        free(writeAndReadSocket(nazov, 2, false, sockfd));
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
        free(writeAndReadSocket(buffer,2,true, sockfd)); // na server '8'

        int klientToAdd = cisloSpr;

        bzero(buffer, 256);
        sprintf(buffer,"%d", klientToAdd);
        free(writeAndReadSocket(buffer, 2, false, sockfd));

        bzero(buffer, 256);
        sprintf(buffer,"%d", cisloVlak);
        free(writeAndReadSocket(buffer, 2, false, sockfd));

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
        free(writeAndReadSocket(buffer,2,true, sockfd));
        //////

        char cisloKli[256];

        bzero(buffer, 256);
        sprintf(buffer, "%d", cisloVlak); // odosielanie cislo vlakna
        free(writeAndReadSocket(buffer, 2, false, sockfd));

        int ids[10];
        while(!strcmpare("-1", cisloKli, buffer, sockfd)) {
            count++;
            char prezyvkaa[256];
            bzero(prezyvkaa, 256);
            char * ans = writeAndReadSocket(buffer, 2, false, sockfd);
            strcpy(prezyvkaa, ans);
            free(ans);
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
