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
#include "Klient.h"
#include "RSA.h"

void *prijimajSpravy(void *arg) {
    KLIENT *data = (KLIENT *) arg;
    ZDIEL *zdiel = data->zdiel;
    char buffer[256];
    while (1) {
        if (data->aktChat != 0) {
            while (data->pocetNacitSprav[data->aktChat - 1] < data->chatvlakno[data->aktChat - 1]->pocetSprav) {
                odosliPoziadavku(4, zdiel, buffer, data->cisloKlient, data->chatvlakno[data->aktChat - 1]->cislo,
                                 data->pocetNacitSprav[data->aktChat - 1]);
                data->pocetNacitSprav[data->aktChat - 1]++;
            }
        }
        sleep(1);
    }
}


void *odosielajSpravu(void *arg) {
    KLIENT *data = (KLIENT *) arg;
    ZDIEL *zdiel = data->zdiel;
    char buffer[256];
    char buff[7];

    while (1) {
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        char s[7];
        memcpy(s, buffer, 5);

        if (strcmp(s, ";nast") == 0) {
            if (data->aktChat == 0) {
                printf("Pre nastavenie chatu vojdite do chatu\n");
            } else {
                printf("---nastavenie---\n");
                printf("Zadajte cislo volby ktoru chcete vykonat:\n");
                printf("1. pridat klienta do chatu\n");
                printf("2. zrusit\n");
                printf("Zadajte cislo: \n");
                bzero(buffer, 256);
                fgets(buffer, 255, stdin);
                int vybrCislo = atoi(buffer);
                if (vybrCislo == 1) {
                    odosliPoziadavku(9, zdiel, buffer, 0, data->chatvlakno[data->aktChat - 1]->cislo, 0);
                    if (countAddToChat == 0) {
                        printf("Neexistuju ziadny dalsi uzivatelia, ktori by sa mohli pridat do chatu!\n");
                    } else {
                        bzero(buffer, 256);
                        while (atoi(buffer) < 1 || atoi(buffer) > countAddToChat) {
                            printf("Zadajte cislo: \n");
                            bzero(buffer, 256);
                            fgets(buffer, 255, stdin);
                        }
                        vybrCislo = atoi(buffer);
                        odosliPoziadavku(8, zdiel, buffer, data->cisloKlient,
                                         data->chatvlakno[data->aktChat - 1]->cislo, addToChatIds[vybrCislo - 1]);
                    }
                } else {
                    //nic
                }
            }

        } else if (strcmp(s, ";menu") == 0) {

            data->aktChat = 0;
            printf("Zadajte cislo volby ktoru chcete vykonat:\n");
            printf("1. pripojit sa k existujucemu chatu\n");
            printf("2. vybrat cloveka na zalozenie noveho chatu\n");
            printf("3. odist\n");
            bzero(buffer, 256);
            fgets(buffer, 255, stdin);
            char s[2];
            memcpy(s, buffer, 1);
            if (strcmp(s, "1") == 0) {
                odosliPoziadavku(5, zdiel, buffer, data->cisloKlient, 0, 0);
                if (countChatToConnect == 0) {
                    printf("Neexistuju ziadne chaty!\n");
                } else {
                    printf("Zadajte cislo: \n");
                    bzero(buffer, 256);
                    fgets(buffer, 255, stdin);
                    int vybrCislo = atoi(buffer);
                    data->aktChat = vybrCislo;
                }
            } else if (strcmp(s, "2") == 0) {
                odosliPoziadavku(6, zdiel, buffer, data->cisloKlient, 0, 0);
                if (makeNewChatIds[0] == 0) {
                    printf("Ziadny dalsi klienti, skuste to neskor\n");
                } else {
                    bzero(buffer, 256);
                    while (atoi(buffer) < 1 || atoi(buffer) > countMakeNewChat) {
                        printf("Zadajte cislo: \n");
                        bzero(buffer, 256);
                        fgets(buffer, 255, stdin);
                    }
                    int vybrCislo = atoi(buffer);
                    printf("Zadajte nazov chatu: \n");
                    bzero(buffer, 256);
                    fgets(buffer, 255, stdin);
                    odosliPoziadavku(7, zdiel, buffer, data->cisloKlient, 0, makeNewChatIds[vybrCislo - 1]);
                }
            } else {
                pthread_cancel(prijSprav);
                pthread_exit(NULL);
            }
        } else if (strcmp(s, ";odist") == 0) {
            pthread_cancel(prijSprav);
            pthread_exit(NULL);
        } else {
            if (data->aktChat != 0) {
                odosliPoziadavku(3, zdiel, buffer, data->cisloKlient, data->chatvlakno[data->aktChat - 1]->cislo, 0);
                data->chatvlakno[data->aktChat - 1]->pocetSprav++;
            } else {
                printf("Pred pisanim spravy vyberte chat zadanim ';menu'\n");
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int n;

    char buffer[256];

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        return 1;
    }
    struct hostent *server;
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        return 2;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(
            (char *) server->h_addr,
            (char *) &serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(atoi(argv[2]));

    bzero((char *) &serv_addrStart, sizeof(serv_addrStart));
    serv_addrStart.sin_family = AF_INET;
    bcopy(
            (char *) server->h_addr,
            (char *) &serv_addrStart.sin_addr.s_addr,
            server->h_length
    );
    serv_addrStart.sin_port = htons(atoi(argv[3]));

    bzero(buffer, 256);
    int sockfd;
    char *spravy = writeAndReadSocket(buffer, 1, true, &sockfd);
    const key_t shm_key = (key_t) atoi(spravy);
    free(spravy);
    printf("%d\n", (int) shm_key);
    int shmid = shmget(shm_key, sizeof(ZDIEL), 0666);
    if (shmid < 0) {
        perror("Failed to open shared memory block:");
        return 10;
    }

    void *addr = shmat(shmid, NULL, 0);

    if (addr == NULL) {
        perror("Failed to attach shared memory block:");
        return 11;
    }
    struct zdiel *zdiel = (struct zdiel *) addr;

    char *pass;
    int cisloKlienta = 1;
    printf("Zadajte cislo volby ktoru chcete vykonat:\n");
    printf("1. prihlasit sa\n");
    printf("2. zaregistrovat sa\n");
    printf("3. odist\n");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);
    char s[2];
    memcpy(s, buffer, 1);
    if (strcmp(s, "1") == 0) {
        cisloKlienta = odosliPoziadavku(1, zdiel, buffer, 0, 0, 0);
    } else if (strcmp(s, "2") == 0) {
        cisloKlienta = odosliPoziadavku(2, zdiel, buffer, 0, 0, 0);
    } else {
        printf("Ukoncuje sa\n");
        sleep(3);
        return 0;
    }

    pthread_t klient;
    pthread_t  prijimanieSprav;
    int pocetNacSprav[20];
    CHATVLAKNOZDIEL vlakna[20];
    KLIENT klientDat = {cisloKlienta, pocetNacSprav, pass, 0, vlakna, zdiel, 0};
    klData = &klientDat;
    pthread_create(&klient, NULL, &odosielajSpravu, &klientDat);
    pthread_create(&prijimanieSprav, NULL, &prijimajSpravy, &klientDat);
    pthread_join(klient, NULL);
    return 0;
}

