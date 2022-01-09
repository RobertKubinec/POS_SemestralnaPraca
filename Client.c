#include "Client.h"

int main(int argc, char *argv[]) {
    printf("#### Ahoj Klient, vitaj v hre Piskvorky! ####\n");
    printf("Tvoj znak: 'X'\n");
    printf("*********************************************\n");
    int sockfd, portno;

    struct sockaddr_in server_addr;
    struct hostent *server;

    char buffer[256];
    int pocetKrokov = 0;
    char hraciePole[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int vysledok = 0;

    if (argc < 3) {
        printf("Malo argumentov!\n");
        exit(1);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // check if socket is created properly
    if (sockfd < 0) {
        printf("Error opening socket!\n");
        exit(1);
    }

    // vytvorenie struckruty pre server address

    server = gethostbyname(
            argv[1]); // pouzijeme funkciu gethostbyname na ziskanie informacii o pocitaci, ktoreho hostname je v prvom argumente
    if (server == NULL)                     //gethostbyname - zisti specificke info o servery
    {
        fprintf(stderr, "Error, no such host!\n");
        return 2;
    }

    bzero((char *) &server_addr,
          sizeof(server_addr)); //(bzero - vynuluje hodnotu) vunulujeme a zinicializujeme adresu, na ktoru sa budeme pripajat
    server_addr.sin_family = AF_INET;
    bcopy(              //kopiruje konkretne byty na specificku poziciu
            (char *) server->h_addr,  //zoebrie zo server h_addr
            (char *) &server_addr.sin_addr.s_addr, //nakopiruje ju na poziciu sin_addr a s_addr
            server->h_length
    );
    server_addr.sin_port = htons(portno); // nastavenie portu //htons - transformacia z little endian na big endian



    // connect to server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed!\n");
        exit(1);
    }

    pthread_t vlaknoOdosli, vlaknoNacitaj;
    pthread_mutex_t mut;
    pthread_cond_t cond_odosli, cond_nacitaj;

    pthread_mutex_init(&mut, NULL);
    pthread_cond_init(&cond_odosli, NULL);
    pthread_cond_init(&cond_nacitaj, NULL);

    DATA data = {buffer, &sockfd, &pocetKrokov, hraciePole, vysledok, &cond_odosli, &cond_nacitaj, &mut};

    pthread_create(&vlaknoNacitaj, NULL, &nacitaj, &data);
    pthread_create(&vlaknoOdosli, NULL, &odosli, &data);

    pthread_join(vlaknoOdosli, NULL);
    pthread_join(vlaknoNacitaj, NULL);


    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&cond_odosli);
    pthread_cond_destroy(&cond_nacitaj);

    close(sockfd);

    return 0;
}

void *nacitaj(void *data) {
    // read back from server
    DATA *d = data;
    int policko;
//    int hodnota = 0;

    while (d->vysledok == 0) {
        pthread_mutex_lock(d->mutex);
        bzero(d->odpoved, 256); // clear buffer

        while (*d->pocetKrokov % 2 == 0) {
            pthread_cond_wait(d->cond_nacitaj, d->mutex);
        }

        if (d->vysledok == 0) {
            printf("Na rade je Server!\n");
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
            d->hraciePole[policko] = 'O';

            zobraz(d->hraciePole);

            // print
            printf("-----------------------------------\n");
            printf("Server --> zadal 'O' na poziciu: %c\n", *d->odpoved);
            printf("-----------------------------------\n");
            (*d->pocetKrokov)++;
            d->vysledok = kontrola(d->hraciePole, 'O', *d->pocetKrokov);
            printf("Pocet krokov: %d\n", *d->pocetKrokov);
            pthread_cond_signal(d->cond_odosli);
        }
        pthread_mutex_unlock(d->mutex);
    }

    return 0;
}

