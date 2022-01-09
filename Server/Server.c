#include "Server.h"

int main(int argc, char *argv[]) {
    printf("#### Ahoj Server, vitaj v hre Piskvorky! ####\n");
    printf("Tvoj znak: 'O'\n");
    printf("*********************************************\n");

    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr, client_addr;

    char buffer[256];
    int pocetKrokov = 0;
    char hraciePole[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int vysledok = 0;

    socklen_t len;
    if (argc < 2) {
        printf("Malo argumentov!\n");
        exit(1);
    }

    portno = atoi(argv[1]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error socket!\n");
        exit(1);
    }

    // struktura pre server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // bind socket to the address and port
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error binding socket!\n");
    }

    // listen
    listen(sockfd, 5);

    len = sizeof(client_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &len);
    if (newsockfd < 0) {
        printf("Error accepting request!\n");
        exit(1);
    }

    pthread_t vlaknoOdosli, vlaknoNacitaj;
    pthread_mutex_t mut;
    pthread_cond_t cond_odosli, cond_nacitaj;

    pthread_mutex_init(&mut, NULL);
    pthread_cond_init(&cond_odosli, NULL);
    pthread_cond_init(&cond_nacitaj, NULL);

    DATA data = {buffer, &newsockfd, &pocetKrokov, hraciePole, vysledok, &cond_odosli, &cond_nacitaj, &mut};

    pthread_create(&vlaknoNacitaj, NULL, &nacitaj, &data);
    pthread_create(&vlaknoOdosli, NULL, &odosli, &data);

    pthread_join(vlaknoOdosli, NULL);
    pthread_join(vlaknoNacitaj, NULL);

    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&cond_odosli);
    pthread_cond_destroy(&cond_nacitaj);

    // close sockets
    close(newsockfd);
    close(sockfd);

    return 0;
}

void *nacitaj(void *data) {
    DATA *d = data;
    int policko;

    //citaj data od clienta
    while (d->vysledok == 0) {
        pthread_mutex_lock(d->mutex);
        bzero(d->odpoved, 256); // clear buffer

        while (*d->pocetKrokov % 2 == 1) {
            pthread_cond_wait(d->cond_nacitaj, d->mutex);
        }

        if (d->vysledok == 0) {
            printf("Na rade je Klient!\n");
            int n = read(*d->socket, d->odpoved, 256);

            if (n < 0) {
                printf("Error reading!\n");
                exit(1);
            }

            for (int i = 0; i < 9; ++i) {
                if (*d->odpoved == d->hraciePole[i]) {
                    policko = i;
//                    hodnota = 0;
                    break;
                }
//                else {
//                    hodnota = -1;
//                }
            }
            d->hraciePole[policko] = 'X';

            zobraz(d->hraciePole);

            printf("-----------------------------------\n");
            printf("Client --> zadal 'X' na poziciu: %c\n", *d->odpoved);
            printf("-----------------------------------\n");
            (*d->pocetKrokov)++;
            d->vysledok = kontrola(d->hraciePole, 'X', *d->pocetKrokov);
            printf("Pocet krokov: %d\n", *d->pocetKrokov);
            pthread_cond_signal(d->cond_odosli);
        }
        pthread_mutex_unlock(d->mutex);
    }
    return 0;
}

void *odosli(void *data) {
    DATA *d = data;
    // msg to client from server
    int policko;
    int hodnota = 0;

    while (d->vysledok == 0) {
        pthread_mutex_lock(d->mutex);

        if (hodnota == 0) {
            zobraz(d->hraciePole);
        }

        while (*d->pocetKrokov % 2 == 0) {
            pthread_cond_wait(d->cond_odosli, d->mutex);
        }

        if (d->vysledok == 0) {
            printf("Zadaj svoj vyber - (1-9): \n");
            fgets(d->odpoved, 256, stdin);

            for (int i = 0; i < 9; ++i) {
                if (*d->odpoved == d->hraciePole[i]) {
                    policko = i;
                    hodnota = 0;
                    break;
                } else {
                    hodnota = -1;
                }
            }

            if ((strlength(d->odpoved) - 1) > 1) {
                hodnota = -1;
            }

            if (hodnota == -1) {
                printf("Policko je uz obsadene alebo si nezadal hodnotu z intervalu - (1-9)!\n");
                printf("Zadaj hodnotu v platnom intervale!\n");
            } else {
                int n = write(*d->socket, d->odpoved, strlen(d->odpoved));

                if (n < 0) {
                    printf("Error writin\n");
                    exit(1);
                }

                d->hraciePole[policko] = 'O';
                (*d->pocetKrokov)++;
                d->vysledok = kontrola(d->hraciePole, 'O', *d->pocetKrokov);
                printf("Pocet krokov: %d\n", *d->pocetKrokov);
                pthread_cond_signal(d->cond_nacitaj);
            }
        }
        pthread_mutex_unlock(d->mutex);
    }
    return 0;
}

void zobraz(char pole[]) {
    printf("\t\t\t\t\t\t\t -------|-------|-------\n");
    printf("\t\t\t\t\t\t\t    %c   |   %c   |   %c   \n", pole[0], pole[1], pole[2]);
    printf("\t\t\t\t\t\t\t -------|-------|-------\n");
    printf("\t\t\t\t\t\t\t    %c   |   %c   |   %c   \n", pole[3], pole[4], pole[5]);
    printf("\t\t\t\t\t\t\t -------|-------|-------\n");
    printf("\t\t\t\t\t\t\t    %c   |   %c   |   %c   \n", pole[6], pole[7], pole[8]);
    printf("\t\t\t\t\t\t\t -------|-------|-------\n");
}

int kontrola(char pole[], char ch, int pocetKrokov) {
    // riadok
    for (int i = 0; i < 3; ++i) {
        if (pole[i * 3] == ch && pole[i * 3 + 1] == ch && pole[i * 3 + 2] == ch) {
            printf("Vyhral hrac %c: - %s\n", ch, pocetKrokov % 2 == 0 ? "Server" : "Client");
            return pocetKrokov % 2 + 1;
        }
    }
    // stlpec
    for (int i = 0; i < 3; ++i) {
        if (pole[i] == ch && pole[i + 3] == ch && pole[i + 6] == ch) {
            printf("Vyhral hrac %c: - %s\n", ch, pocetKrokov % 2 == 0 ? "Server" : "Client");
            return pocetKrokov % 2 + 1;
        }
    }
    if (pole[0] == ch && pole[4] == ch && pole[8] == ch) {
        printf("Vyhral hrac: %c - %s\n", ch, pocetKrokov % 2 == 0 ? "Server" : "Client");
        return pocetKrokov % 2 + 1;
    } else if (pole[2] == ch && pole[4] == ch && pole[6] == ch) {
        printf("Vyhral hrac: %c - %s\n", ch, pocetKrokov % 2 == 0 ? "Server" : "Client");
        return pocetKrokov % 2 + 1;
    } else if (pocetKrokov == 9) {
        printf("Remiza!\n");
        return 3;
    } else {
        return 0;
    }
}

int strlength(char *s) {
    int c = 0;
    while (*s != '\0') {
        c++;
        s++;
    }
    return c;
}
