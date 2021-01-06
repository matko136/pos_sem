//
// Created by matob on 2. 1. 2021.
//

#ifndef CHATAPP_SERVER_H
#define CHATAPP_SERVER_H
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
    int cislo;
    char* nazov;
    int pocetSprav;
} CHATVLAKNOZDIEL;

typedef struct chatVlakno {
    int cislo;
    char* nazov;
    char** spravy;
    int pocetSprav;
    int* klienti;
    int pocetKlientov;
    int* klientSprav;
    key_t shm_key_zdiel_Vlak;
} CHATVLAKNO;

typedef struct server {
    int pocetKlientov;
    int * klientiId;
    char ** prezyvky;
    char ** hesla;
    int sock;
    int sockPripojenie;
    ZDIEL * zdiel;
    CHATVLAKNO ** chatvlakno;
    int pocVlakien;
} SERVER;

void zmenRiadok(char * subor, int cisloRiadku, char * novyText) {
    FILE * fptr;
    fptr = fopen(subor, "r");
    FILE * fptrTemp;
    fptrTemp = fopen("temporary.txt", "w");
    char buffer[256];
    bzero(buffer, 256);
    int count = 0;
    while ((fgets(buffer, 256, fptr))) {
        count++;
        if(count == cisloRiadku)
            fputs(novyText, fptrTemp);
        else
            fputs(buffer, fptrTemp);
    }
    fclose(fptr);
    fclose(fptrTemp);
    remove(subor);
    rename("temporary.txt", subor);
}

key_t vytvorZdielaneVlakno(int cisloVlakna, char * nazov, int pocetSprav) {
    const key_t shm_key = (key_t)rand()%INT_MAX;
    int shmid = shmget(shm_key, sizeof(CHATVLAKNOZDIEL), 0666|IPC_CREAT|IPC_EXCL);

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
    CHATVLAKNOZDIEL* zdielVlak = (CHATVLAKNOZDIEL*)addr;
    zdielVlak->nazov = nazov;
    //strcpy(zdielVlak->nazov, nazov);
    zdielVlak->pocetSprav = pocetSprav;
    zdielVlak->cislo = cisloVlakna;
    pthread_mutex_t mut;
    pthread_cond_t odoslana;
    zdielVlak->mutex = mut;
    zdielVlak->odoslana = odoslana;

    pthread_mutexattr_t mutattr;
    pthread_mutexattr_init(&mutattr);
    pthread_mutexattr_setpshared(&mutattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&zdielVlak->mutex, &mutattr);

    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&zdielVlak->odoslana, &condattr);
    return shm_key;
}

int obsluhujKlienta(int newsockfd, SERVER* data) {
    char buffer[256];
    bzero(buffer, 256);
    int n = read(newsockfd, buffer, 255);
    int ret = -1;
    if(buffer[0] == '1') {
        char meno[256];
        char heslo[256];
        write(newsockfd, buffer,strlen(buffer));
        bzero(meno, 256);
        read(newsockfd, meno, 255);
        write(newsockfd, buffer,strlen(buffer));
        bzero(heslo, 256);
        read(newsockfd, heslo, 255);
        bool success = false;
        char prezyvka[256];
        bzero(prezyvka, 256);
        char hsl[256];
        bzero(hsl, 256);
        for(int i = 0; i < data->pocetKlientov; i++) {
            strcpy(prezyvka, data->prezyvky[i]);
            if (strcmp(data->prezyvky[i], meno) == 0) {
                strcpy(hsl, data->hesla[i]);
                if (strcmp(data->hesla[i], heslo) == 0) {
                    success = true;
                    ret = i + 1;
                }
                break;
            }
        }

        bzero(buffer, 256);
        sprintf(buffer, "%d", ret);
        write(newsockfd, buffer,strlen(buffer));
    } else if(buffer[0] == '2') {
        char meno[256];
        char heslo[256];
        write(newsockfd, buffer,strlen(buffer));
        bzero(meno, 256);
        read(newsockfd, meno, 255);
        usleep(100);
        write(newsockfd, buffer,strlen(buffer));
        bzero(heslo, 256);
        read(newsockfd, heslo, 255);
        bool success = true;
        ret = 0;
        for(int i = 0; i < data->pocetKlientov; i++) {
            if (strcmp(data->prezyvky[i], meno) == 0) {
                success = false;
                break;
            }
        }
        if(success) {
            strcpy(data->prezyvky[data->pocetKlientov], meno);
            strcpy(data->hesla[data->pocetKlientov++], heslo);
            ret = data->pocetKlientov;
            char newLine[256];
            bzero(newLine, 256);
            sprintf(newLine, "%d\n", data->pocetKlientov);
            zmenRiadok("serv_ud.txt", 3, newLine);
            FILE * fptr;
            fptr = fopen("serv_ud.txt", "a");
            fprintf(fptr, "%s", data->prezyvky[data->pocetKlientov-1]);
            fclose(fptr);
            FILE * fptr2;
            fptr2 = fopen("serv_hsl.txt", "a");
            fprintf(fptr2, "%s", data->hesla[data->pocetKlientov-1]);
            fclose(fptr2);
        } else {
            ret = -1;
        }
        bzero(buffer, 256);
        sprintf(buffer, "%d", ret);
        write(newsockfd, buffer,strlen(buffer));
    } else if(buffer[0] == '3') {
        char sprava[256];
        int cisloVlakna = 0;
        int cisloKlienta = 0;
        write(newsockfd, buffer,strlen(buffer));
        bzero(sprava, 256);
        read(newsockfd, sprava, 255);
        write(newsockfd, buffer,strlen(buffer));
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloVlakna = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloKlienta = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));
        char addr1[256];
        char addr2[256];
        strcpy(data->chatvlakno[cisloVlakna-1]->spravy[data->chatvlakno[cisloVlakna-1]->pocetSprav],sprava);
        data->chatvlakno[cisloVlakna-1]->klientSprav[data->chatvlakno[cisloVlakna-1]->pocetSprav++] = cisloKlienta;

        FILE * fptr;
        char filename[256];
        bzero(filename, 256);
        sprintf(filename,"%d", cisloVlakna);
        strcat(filename,"vlak_spravy.txt");
        char newLine[256];
        bzero(newLine, 256);
        sprintf(newLine, "%d\n", data->chatvlakno[cisloVlakna-1]->pocetSprav);
        zmenRiadok(filename, 2, newLine);
        fptr = fopen(filename, "a");
        fprintf(fptr, "%s", data->chatvlakno[cisloVlakna-1]->spravy[data->chatvlakno[cisloVlakna-1]->pocetSprav-1]);
        fprintf(fptr, "%d\n", data->chatvlakno[cisloVlakna-1]->klientSprav[data->chatvlakno[cisloVlakna-1]->pocetSprav-1]);
        fclose(fptr);
    } else if(buffer[0] == '4') {
        int cisloVlakna = 0;
        int cisloKlienta = 0;
        int cisloSpravy = 0;
        write(newsockfd, buffer,strlen(buffer));
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloVlakna = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloKlienta = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloSpravy = atoi(buffer);
        bzero(buffer, 256);
        strcpy(buffer, data->prezyvky[data->chatvlakno[cisloVlakna-1]->klientSprav[cisloSpravy]-1]);
        write(newsockfd, buffer,strlen(buffer));
        read(newsockfd, buffer, 255);
        bzero(buffer, 256);
        strcpy(buffer, data->chatvlakno[cisloVlakna-1]->spravy[cisloSpravy]);
        write(newsockfd, buffer,strlen(buffer));
    } else if(buffer[0] == '5') {
        write(newsockfd, buffer,strlen(buffer));
        int cisloKl = 0;
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloKl = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));
        for(int i = 0; i < data->pocVlakien; i++) {
            for(int j = 0; j < data->chatvlakno[i]->pocetKlientov; j++) {
                if(cisloKl == data->chatvlakno[i]->klienti[j]) {
                    read(newsockfd, buffer, 255);
                    bzero(buffer, 256);
                    strcpy(buffer, data->chatvlakno[i]->nazov);
                    write(newsockfd, buffer,strlen(buffer));
                    bzero(buffer, 256);
                    read(newsockfd, buffer, 255);
                    bzero(buffer, 256);
                    sprintf(buffer, "%d", (int)data->chatvlakno[i]->shm_key_zdiel_Vlak);
                    write(newsockfd, buffer,strlen(buffer));
                    break;
                }
            }
        }
        read(newsockfd, buffer, 255);
        bzero(buffer, 256);
        strcpy(buffer, "-1");
        write(newsockfd, buffer,strlen(buffer));
    } else if(buffer[0] == '6') {
        write(newsockfd, buffer,strlen(buffer));
        int cisloKl = 0;
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloKl = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));
        for(int i = 0; i < data->pocetKlientov; i++) {
            if(cisloKl != i+1) {
                read(newsockfd, buffer, 255);
                bzero(buffer, 256);
                sprintf(buffer,"%d", i+1);
                write(newsockfd, buffer,strlen(buffer));
                read(newsockfd, buffer, 255);
                bzero(buffer, 256);
                strcpy(buffer, data->prezyvky[i]);
                write(newsockfd, buffer,strlen(buffer));
            }
        }
        read(newsockfd, buffer, 255);
        bzero(buffer, 256);
        strcpy(buffer, "-1");
        write(newsockfd, buffer,strlen(buffer));
    } else if(buffer[0] == '7') {
        write(newsockfd, buffer,strlen(buffer));
        int cisloKl = 0;
        int cisloKlToAdd = 0;
        char nazov[256];
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloKl = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloKlToAdd = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));
        bzero(nazov, 256);
        read(newsockfd, nazov, 255);
        write(newsockfd, buffer,strlen(buffer));

        key_t shm_key = vytvorZdielaneVlakno(++data->pocVlakien, nazov, 0);
        data->chatvlakno[data->pocVlakien-1]->cislo = data->pocVlakien;
        strcpy(data->chatvlakno[data->pocVlakien-1]->nazov, nazov);
        data->chatvlakno[data->pocVlakien-1]->pocetKlientov = 2;
        data->chatvlakno[data->pocVlakien-1]->shm_key_zdiel_Vlak = shm_key;
        data->chatvlakno[data->pocVlakien-1]->klienti[0] = cisloKl;
        data->chatvlakno[data->pocVlakien-1]->klienti[1] = cisloKlToAdd;
        FILE * fptr;
        char filename[256];
        bzero(filename, 256);
        sprintf(filename,"%d", data->pocVlakien);
        strcat(filename,"vlak_ud.txt");
        fptr = fopen(filename, "w");
        char line[256];
        bzero(line, 256);
        strcpy(line, "----Vlakno---udaje---klienti---");
        fprintf(fptr,"%s\n", line);
        fprintf(fptr, "%d\n", data->pocVlakien);
        fprintf(fptr, "%s", nazov);
        fprintf(fptr, "%d\n", data->chatvlakno[data->pocVlakien-1]->pocetKlientov);
        fprintf(fptr, "%d\n", data->chatvlakno[data->pocVlakien-1]->klienti[0]);
        fprintf(fptr, "%d\n", data->chatvlakno[data->pocVlakien-1]->klienti[1]);
        fclose(fptr);
        FILE * fptr2;
        bzero(filename, 256);
        sprintf(filename,"%d", data->pocVlakien);
        strcat(filename,"vlak_spravy.txt");
        fptr2 = fopen(filename, "w");
        bzero(line, 256);
        strcpy(line, "---Vlakno---spravy---klienti---");
        fprintf(fptr2,"%s\n", line);
        fprintf(fptr2, "%d\n", 0);
        fclose(fptr2);
        char newLine[256];
        bzero(newLine, 256);
        sprintf(newLine, "%d\n", data->pocVlakien);
        zmenRiadok("serv_ud.txt", 2, newLine);
    }
    else if(buffer[0] == '8') { //nastavenie pridanie klienta
        write(newsockfd, buffer,strlen(buffer)); // neviem naco

        int cisloKl = 0;
        int cisloKlToAdd = 0;

        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloKlToAdd = atoi(buffer);

        write(newsockfd, buffer,strlen(buffer));

        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        int cisVlak = atoi(buffer);

        write(newsockfd, buffer,strlen(buffer));

        data->chatvlakno[cisVlak-1]->klienti[data->chatvlakno[cisVlak-1]->pocetKlientov++] = cisloKlToAdd;
        FILE * fptr;
        char filename[256];
        bzero(filename, 256);
        sprintf(filename,"%d", cisVlak);
        strcat(filename,"vlak_ud.txt");
        char newLine[256];
        bzero(newLine, 256);
        sprintf(newLine, "%d\n", data->chatvlakno[cisVlak-1]->pocetKlientov);
        zmenRiadok(filename, 4, newLine);
        fptr = fopen(filename, "a");
        fprintf(fptr, "%d\n", cisloKlToAdd);
        fclose(fptr);
    }
    else if(buffer[0] == '9') {
        write(newsockfd, buffer,strlen(buffer));
        int cisloVlak = 0;
        bzero(buffer, 256);
        read(newsockfd, buffer, 255);
        cisloVlak = atoi(buffer);
        write(newsockfd, buffer,strlen(buffer));////odpovedanie na strane klienta
        for(int i = 0; i < data->pocetKlientov; i++) {
            bool success = true;
            for (int j = 0; j < data->chatvlakno[cisloVlak-1]->pocetKlientov; j++) {
                if (data->chatvlakno[cisloVlak-1]->klienti[j] == i+1) {
                    success = false;
                    break;
                }
            }
            if(success) {
                read(newsockfd, buffer, 255);
                bzero(buffer, 256);
                sprintf(buffer,"%d", i+1);
                write(newsockfd, buffer,strlen(buffer));
                read(newsockfd, buffer, 255);
                bzero(buffer, 256);
                strcpy(buffer, data->prezyvky[i]);
                write(newsockfd, buffer,strlen(buffer));
            }
        }
        read(newsockfd, buffer, 255);
        bzero(buffer, 256);
        strcpy(buffer, "-1");
        write(newsockfd, buffer,strlen(buffer));
    }
    return ret;
}
#endif //CHATAPP_SERVER_H
