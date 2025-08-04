#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#define tam 9
// Estructura para memoria compartida
typedef struct {
	int ocupado;
	int subido;
	int espera;
	int totalBloqueado;
}balsa;
balsa *ptr;
void down(int sem);
void up(int sem);
int main() {
	printf("Ejercicio 3, Grupo 18, 2024\n");
	printf("Integrantes:\n");
	printf("Antonio Bortoli- antoniobortoli2002@gmail.com\n");
	printf("Bautista Luetich- bauluetich@gmail.com\n");
	printf("Francisco Soltermann- frasoltermann@gmail.com\n");
	int idMem; //memoria
	int camionBloq, lleno, mutex; //semaforos
	key_t memoria = ftok(".", 'B');
	if(memoria==-1){
		perror("ftok");
		exit(EXIT_FAILURE);
	}
	if((idMem=shmget(memoria, sizeof(balsa), IPC_CREAT | 0666))==-1) {
		perror("shmget");
		exit(EXIT_FAILURE);
	}
	ptr=(balsa *)shmat(idMem, NULL, 0);
	ptr->ocupado=tam;
	ptr->totalBloqueado=0;
	ptr->subido=0;
	ptr->espera=0;
	if(ptr==(void *)-1){
		perror("Error enlazando memoria compartida");
		exit(EXIT_FAILURE);
	}
	//creo los semaforos
	camionBloq=semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	lleno=semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	mutex=semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	//Inicializo valores de semáforos
	union semun{
		int val;
	} arg;
	arg.val=0;
	semctl(camionBloq, 0, SETVAL, arg); //bloquado=0
	arg.val=0;
	semctl(lleno, 0, SETVAL, arg);//salida=0
	arg.val=1;
	semctl(mutex, 0, SETVAL, arg);//mutex
	//Proceso balsa
	if(fork()==0){
		while(true){
			down(lleno);
			down(mutex);
			printf("Se lleno el espacio, La balsa parte a destino\n");
			sleep(1);
			int bloqueados=ptr->totalBloqueado;
			int i;
			for (i=0;i<bloqueados;i++){
				up(camionBloq);
			}
			ptr->ocupado=tam;
			ptr->totalBloqueado=0;
			ptr->subido=0;
			ptr->espera=0;
			printf("Balsa volvio vacia, Lista para cargar.\n");
			up(mutex);
		}
		exit(EXIT_SUCCESS);
	}
	//Proceso camiones
	while(true){
		if (fork()==0){
			srand(time(NULL));
			int tl=((rand()%2)+1); // genera camion con (2) o sin acoplado(1)
			bool bandera=false;
			while(true && !bandera){
				down(mutex);
				if(ptr->ocupado>=1 && tl==1){
					ptr->ocupado-=tl;
					ptr->subido++;
					printf("Camion sin acoplado cargado, Espacio
						   cargado: %d\n", 9-(ptr->ocupado));
				}
				else if(ptr->ocupado>=2 && tl==2){
					ptr->ocupado-=tl;
					ptr->subido++;
					printf("Camion con acoplado cargado, Espacio
						   cargado: %d\n", 9-ptr->ocupado);
				}
				else{
					// Se bloquea el camión, no hay espacio
					ptr->espera++;
					ptr->totalBloqueado++;
					up(mutex);
					down(camionBloq);
				}
				//Si está lleno, señaliza la salida
				if (ptr->ocupado==0){
					up(lleno);
				}
				up(mutex);
				bandera=true;
			}
			exit(EXIT_SUCCESS);
		}
		sleep(1);//camiones cada 1 sg
	}
	//limpieza de recursos
	shmdt(ptr);
	shmctl(idMem, IPC_RMID, NULL);
	semctl(lleno, 0, IPC_RMID);
	semctl(mutex, 0, IPC_RMID);
	semctl(camionBloq, 0, IPC_RMID);
	return 0;
}
void down(int sem){
	struct sembuf op={0,-1,0};
	semop(sem, &op, 1);
}
	void up(int sem){
		struct sembuf op={0,1,0};
		semop(sem, &op, 1);
	}
