/*
Mechanizmy IPC:
	"Czytelnicy i Pisarze"

Praca wykonana przez:
	Tomasz Piescikowski
	Informatyka, 2 rok
	145418, I2.1
	25.02.2021

*/
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
///////////////////////////////////////////	STALE
#define N 5
#define K 8
#define PUSTY 1	
#define PELNY 2
/////////////////////////////////////////// ZMIENNE 
int msgid, msgid2, msgid3;
int shmid;
int semid;
int *bufor;
int czas;
int parent;
/////////////////////////////////////////// STRUCT DO SEMAFOROW 
static struct sembuf buf;
void podnies(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = 1;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("Podnoszenie semafora");
      exit(1);
   }
}

void opusc(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = -1;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("Opuszczenie semafora");
      exit(1);
   }
}
/////////////////////////////////////////// STRUCT DO KOLEJKI
struct buf_elem {
	long mtype;
	int mvalue;
	int busy;
} elem;
/////////////////////////////////////////// DEKLARACJE FUNKCJI
void utworzKolejkeKomunikatow();
void utworzPamiecWspoldzielona();
void utworzSemafory();
void utworzProcesy();
void czytelnik();
void pisarz();
void red();
void yellow();
void green();
void reset();
/////////////////////////////////////////// GLOWNA PETLA
int main() {
/////////////////////////////////////////// TWORZENIE SRODOWISKA
	parent = getpid();
	utworzKolejkeKomunikatow();
	utworzPamiecWspoldzielona();
	utworzSemafory();
	utworzProcesy();
/////////////////////////////////////////// OTWARCIE CZYTELNI
	while(1)
	{
/////////////////////////////////////////// FAZA RELAKSU
		srand(time(NULL) ^ getpid());	
		czas = rand();
		sleep(czas % 2 + 3);
/////////////////////////////////////////// FAZA KORZYSTANIA Z CZYTELNI
		if(czas % 2 == 0) pisarz();
		else czytelnik();
	}
	

	return 0;
}
/////////////////////////////////////////// PAMIEC WSPOLDZIELONA
void utworzPamiecWspoldzielona() {
	shmid = (shmget(12345, (3)*sizeof(int), IPC_CREAT|0600));
	if (shmid == -1){
      perror("Utworzenie segmentu pamieci wspoldzielonej");
      exit(1);
    }

    bufor = (int*)shmat(shmid, NULL, 0);	
    if(bufor == NULL) {
    	perror("Przylaczenie segmentu pamieci wspoldzielonej");
    	exit(1);
    }
    
	#define liczbaCzytelnikow bufor[0]
	#define liczbaPisarzy bufor[1]
	#define zajety bufor[2]

	liczbaCzytelnikow = 0;
	liczbaPisarzy = 0;
	zajety = 0; 	//////////////  0 - wolny	1 - zajety
}

/////////////////////////////////////////// TWORZENIE PROCESOW
void utworzProcesy() {
	printf("Start za 2 sekundy...\n\n");
	for (int i = 0; i < N-1; ++i) {
		pid_t pid;
		if(pid != 0) pid = fork();
	}
	printf("Hello, im %d\n", getpid());
	sleep(2);
}

/////////////////////////////////////////// TWORZENIE TABLICY SEMAFOROW
void utworzSemafory() {

	semid = semget(4444, 3, IPC_CREAT|0600); //utworz tablice semaforow
    if (semid == -1){
      perror("Utworzenie tablicy semaforow");
      exit(1);
    }
    if (semctl(semid, 0, SETVAL, (int)1) == -1){  
      perror("Nadanie wartosci semaforowi 0");
      exit(1);
    }
    if (semctl(semid, 1, SETVAL, (int)1) == -1){  
      perror("Nadanie wartosci semaforowi 1");
      exit(1);
    }
    if (semctl(semid, 2, SETVAL, (int)0) == -1){  
      perror("Nadanie wartosci semaforowi 2");
      exit(1);
    }
}

/////////////////////////////////////////// TWORZENIE KOLEJKI KOMUNIKATOW
void utworzKolejkeKomunikatow() {
	msgid = msgget(12345, IPC_CREAT|IPC_EXCL|0600);
	if (msgid  == -1) {
		msgid = msgget(12345, IPC_CREAT|0600);
		if (msgid  == -1) {
			perror("Utworzenie kolejki komunikatow");
			exit(1);
		}
	}
	else {
		for(int i=0; i<K; i++) {
			elem.mtype = PUSTY;
			if(msgsnd(msgid, &elem, sizeof(elem.mvalue), 0) == -1) {
				perror("Wysylanie pustego komunikatu");
				exit(1);
			}
		}
	}

	msgid2 = msgget(123456, IPC_CREAT|IPC_EXCL|0600);
	if (msgid2  == -1) {
		msgid2 = msgget(123456, IPC_CREAT|0600);
		if (msgid2 == -1) {
			perror("Utworzenie kolejki komunikatow");
			exit(1);
		}
	}
	else {
		elem.mtype = PUSTY;
		if(msgsnd(msgid2, &elem, sizeof(elem.mvalue), 0) == -1) {
			perror("Wysylanie pustego komunikatu");
			exit(1);
		}
	}

	msgid3 = msgget(123454, IPC_CREAT|IPC_EXCL|0600);
	if (msgid3  == -1) {
		msgid3 = msgget(123454, IPC_CREAT|0600);
		if (msgid3  == -1) {
			perror("Utworzenie kolejki komunikatow");
			exit(1);
		}
	}
	else {
		for(int i=0; i<K; i++) {
			elem.mtype = PUSTY;
			if(msgsnd(msgid3, &elem, sizeof(elem.mvalue), 0) == -1) {
				perror("Wysylanie pustego komunikatu");
				exit(1);
			}
		}
	}
}

/////////////////////////////////////////// IMPLEMENTACJA CZYTELNIKA
void czytelnik() {
	green();
	if(msgrcv(msgid, &elem, sizeof(elem.mvalue), PUSTY, 0) == -1){
		perror("Wysylanie komunikatu");
		exit(1);
	}
		/////////////////////////////////////////// POCZEKALNIA
		opusc(semid, 1);
			printf("\nCzytelnik %d czeka na wyjscie pisarzy\n", getpid());
		podnies(semid, 1);

		do {} while(liczbaPisarzy > 0);
		liczbaCzytelnikow++;
		elem.mtype = PUSTY;
		///////////////////////////////////////////
	
	
	if(msgsnd(msgid, &elem, sizeof(elem.mvalue), 0) == -1){
		perror("Wysylanie elementu");
		exit(1);
	}

	/////////////////////////////////////////// SEKCJA KRYTYCZNA
	
	green();
	printf("\nCzytelnik %d", getpid());
	red(); printf(" w sekcji krytycznej\n"); green();
	sleep(2);

	if(msgrcv(msgid3, &elem, sizeof(elem.mvalue), PELNY, IPC_NOWAIT) == -1){
		printf("\nNie ma ksiazki do przeczytania\n");
	}
		if(elem.mvalue != 0)
			printf("\nCzytelnik %d przeczytal dzielo pisarza %d\n", getpid(), elem.mvalue);
		else	
			printf("\nCzytelnik %d nie mial ksiazki do przeczytania\n", getpid());	
	elem.mtype = PUSTY;
	if(msgsnd(msgid3, &elem, sizeof(elem.mvalue), IPC_NOWAIT) == -1){
		printf("\nNie udalo sie przeczytac ksiazki\n");
	}




	/////////////////////////////////////////// KONIEC SEKCJI KRYTYCZNEJ

	if(msgrcv(msgid, &elem, sizeof(elem.mvalue), PUSTY, 0) == -1){
		perror("Wysylanie komunikatu");
		exit(1);
	}
		/////////////////////////////////////////// WYJSCIE
		opusc(semid, 1);
			liczbaCzytelnikow--;
			printf("\nCzytelnik %d wychodzi z biblioteki\n", getpid());
		podnies(semid, 1);
		///////////////////////////////////////////

	if(msgsnd(msgid, &elem, sizeof(elem.mvalue), 0) == -1){
		perror("Wysylanie elementu");
		exit(1);
	}
	reset();
}


/////////////////////////////////////////// IMPLEMENTACJA PISARZA
void pisarz() {
	yellow();
	if(msgrcv(msgid2, &elem, sizeof(elem.mvalue), PUSTY, 0) == -1){
		perror("Wysylanie komunikatu");
		exit(1);
	}
	
	/////////////////////////////////////////// POCZEKALNIA
	printf("\nPisarz %d czeka na wejscie\n", getpid());
	liczbaPisarzy++;
	do {} while(zajety == 1 || liczbaCzytelnikow > 0);
	zajety = 1;
	elem.mtype = PUSTY;
	///////////////////////////////////////////

	if(msgsnd(msgid2, &elem, sizeof(elem.mvalue), 0) == -1){
		perror("Wysylanie elementu");
		exit(1);
	}
	
	/////////////////////////////////////////// SEKCJA KRYTYCZNA
	
	yellow();
	printf("\nPisarz %d", getpid());
	red(); printf(" w sekcji krytycznej\n"); yellow();
	sleep(2);

	if(msgrcv(msgid3, &elem, sizeof(elem.mvalue), PUSTY, IPC_NOWAIT) == -1){
		printf("\nBrak wolnych miejsc na polce\n");
	}

	elem.mvalue = getpid();
	elem.mtype = PELNY;

	if(msgsnd(msgid3, &elem, sizeof(elem.mvalue), IPC_NOWAIT) == -1){
		printf("\nNie udalo sie polozyc ksiazki\n");
	}







	/////////////////////////////////////////// KONIEC SEKCJI KRYTYCZNEJ

	if(msgrcv(msgid, &elem, sizeof(elem.mvalue), PUSTY, 0) == -1){
		perror("Wysylanie komunikatu");
		exit(1);
	}
		/////////////////////////////////////////// WYJSCIE
		opusc(semid, 0);
			liczbaPisarzy--;
			zajety = 0;
			printf("\nPisarz %d wychodzi z biblioteki\n", getpid());
		podnies(semid, 0);
		///////////////////////////////////////////

	if(msgsnd(msgid, &elem, sizeof(elem.mvalue), 0) == -1){
		perror("Wysylanie elementu");
		exit(1);
	}
	reset();
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