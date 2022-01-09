#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

void *odosli(void *data);

void *nacitaj(void *data);

void zobraz(char pole[]);

int kontrola(char pole[], char ch, int pocetKrokov);

int strlength(char *s);

typedef struct data {
    char *odpoved;
    int *socket;
    int *pocetKrokov;
    char *hraciePole;
    int vysledok;

    pthread_cond_t *cond_odosli;
    pthread_cond_t *cond_nacitaj;
    pthread_mutex_t *mutex;
} DATA;

#endif