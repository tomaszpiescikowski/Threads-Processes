/*
Wątki:
	"Pasterz"

Praca wykonana przez:
	Tomasz Piescikowski
	Informatyka, 2 rok
	145418, I2.1
	28.02.2021

*/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define N 4		//liczba zwierzat
#define D 5	//liczba krokow do zagrody
#define K 3		//maksymalna roznica krokow miedzy zwierzeciem a pasterzem

pthread_mutex_t zamek; 
pthread_cond_t warunkowa;
pthread_t tid[N];

int liczbaKrokowPasterza = 0;
int liczbaPrzydzielonychIndeksow = 0;
int tablicaKrokow[N];
__thread int wlasnyIndex;

pthread_t pthread_self(void);

void red(); 
void yellow(); 
void green();
void reset();

int poziomPrzejscZwierzat() {
	int min=100000;
	for(int i=0; i<N; i++) {
		if(tablicaKrokow[i] < min) min = tablicaKrokow[i];
	}
	return min;
}

void * krokZwierzecia(void * arg) {
	for(int i=0; i<D; i++) {
		pthread_mutex_lock(&zamek);
		////////////////////////////////////////////////////////////////////////
		if(i == 0) {
			wlasnyIndex = liczbaPrzydzielonychIndeksow;
			green(); printf("Owca %d ma index %d\n\n", pthread_self(), wlasnyIndex); reset();
			liczbaPrzydzielonychIndeksow++;
		}
		////////////////////////////////////////////////////////////////////////
		
		while (tablicaKrokow[wlasnyIndex] - K >= liczbaKrokowPasterza)
			pthread_cond_wait(&warunkowa, &zamek);
			
		tablicaKrokow[wlasnyIndex] += 1;
		green(); printf("Owca %d\nLiczba krokow pasterza: %d\nOstatnia owca jest na %d kroku\nMoja liczba krokow: %d\n\n", pthread_self(), liczbaKrokowPasterza, poziomPrzejscZwierzat(), tablicaKrokow[wlasnyIndex]); reset();

		if(liczbaKrokowPasterza < poziomPrzejscZwierzat() + K)
			pthread_cond_signal(&warunkowa);

		////////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&zamek);
	}
	
}

void krokPasterza() {
	for(int i=0; i<D; i++) {
		pthread_mutex_lock(&zamek);
		////////////////////////////////////////////////////////////////////////

		if (liczbaKrokowPasterza >= poziomPrzejscZwierzat() + K) {
			yellow(); printf("Pasterz czeka na ostatnia owce\n\n"); reset();
			pthread_cond_wait(&warunkowa, &zamek);
		}
		
		liczbaKrokowPasterza += 1;
		pthread_cond_broadcast(&warunkowa);
		yellow(); printf("Pasterz robi krok\nLiczba krokow pasterza: %d\nOstatnia owca jest na %d kroku\n\n", liczbaKrokowPasterza, poziomPrzejscZwierzat()); reset();

		////////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&zamek);
	}
}

int main() {	
	int temp;		
	for(int i=0; i<N; i++) tablicaKrokow[i] = 0;

	if (pthread_mutex_init(&zamek, NULL) != 0) {
		perror("Blad inicjalizacji mutexa");
		exit(1);
	}
	
	red(); printf("Pasterz rozpoczyna zaganianie owiec\n\n"); reset();

	for(int i=0; i<N; i++) {
        temp = pthread_create(&(tid[i]), NULL, &krokZwierzecia, NULL);
        if (temp != 0) {
            perror("Blad tworzenia watku");
			exit(1);
		}
    }
	
	krokPasterza();

	for(int i=0; i<N; i++) pthread_join(tid[i], NULL);
    pthread_mutex_destroy(&zamek);
	red(); printf("\nOwce z pasterzem bezpiecznie dotarly do zagrody\n"); reset();

	return 0;
}

void red() {
	printf("\033[1;31m");
}
void yellow() {
	printf("\033[1;33m");
}
void green() {
	printf("\033[1;32m");
}
void reset() {
	printf("\033[0m");
}