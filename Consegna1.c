#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/sysinfo.h>


typedef struct StructLista {
    char info[256];
    struct StructLista *next;
}NodoLista;
typedef NodoLista* Lista;

Lista lis=NULL;
DIR *dir;
struct dirent *de;
pthread_mutex_t Tmutex;
int time_to_exit=0, cartelle=0;
char buff[256];
char memory[256];


void CancellaElementoLista(Lista *lis, char* memory){  //se non ci sono più Cartelle, termina la scansione
  Lista paux;
  if (*lis != NULL) {
    dir=opendir((*lis)->info);
    if (dir != NULL) { 
      cartelle++;
      strcpy(memory, (*lis)->info);
      paux = *lis;
      *lis = (*lis)->next;
      free(paux);
    }
    else 
      CancellaElementoLista(&(*lis)->next, memory);
  }
  else time_to_exit=1;
} 

void InserimentoInTesta(Lista *lis, char* elem){
	Lista pt=NULL;
	
	if ((pt = (Lista)malloc(sizeof(NodoLista))) == NULL){
		printf("Errore allocazione memoria");
		exit(1);
	}
	pt->next = *lis;
	strcpy(pt->info, elem);
	*lis = pt;
}
void InserisciElementoInListaOrdinata(Lista *lis, char* elem){
	if (*lis == NULL)
		InserimentoInTesta(lis, elem);
	else 
		if (strcmp((*lis)->info, elem) > 0) 
			InserimentoInTesta(lis, elem);
		else 
			InserisciElementoInListaOrdinata(&(*lis)->next, elem);
}

void VisitaLista(Lista lis) {
    while (lis != NULL) {
      printf("%s\n", lis->info);
      lis = lis->next;
    }
}

void *thread(void *arg) {   //Thread per la Lista
   while (!time_to_exit) { 
      pthread_mutex_lock(&Tmutex);
      CancellaElementoLista(&lis, memory);
      if (dir!=NULL) {  //se NULL, mi dice di Terminare
          while ((de = readdir(dir)) != NULL) { 
              if ((strcmp(de->d_name, "."))!=0 && (strcmp(de->d_name, ".."))!=0) {
                 strcpy(buff, memory);
                 strcat(buff, "/");
                 strcat(buff, de->d_name);
                 InserisciElementoInListaOrdinata(&lis, buff);
              }
          }
          closedir(dir);
      }
      pthread_mutex_unlock(&Tmutex);
   }
}

int main(int argc,char *argv[]) {
   int i, res;
   int proc;
   proc=get_nprocs();
   printf("\nHai %d core", proc);
   pthread_t vet[proc];
    // opendir() restituisce un puntatore di tipo DIR.  
   dir = opendir(argv[1]); 
  
   if (dir == NULL)  // opendir restituisce NULL se non riesce ad aprire la directory 
   { 
        perror("Impossibile aprire la directory"); 
        exit(1); 
   }
   while ((de = readdir(dir)) != NULL) { 
        if ((strcmp(de->d_name, "."))!=0 && (strcmp(de->d_name, ".."))!=0) {
           strcpy(buff, argv[1]);
           strcat(buff, "/");
           strcat(buff, de->d_name);
           InserisciElementoInListaOrdinata(&lis, buff); 
        }
   }
   closedir(dir);
   
   res = pthread_mutex_init(&Tmutex, NULL);
   if (res != 0) { perror("Inizializzazione Mutex fallita"); exit(EXIT_FAILURE); }
   
   printf("\nCreo i Thread\n");
   for(i=0; i<proc; i++) {
      res=pthread_create(&vet[i], NULL, thread, NULL);
      if (res != 0) { perror("Creazione Thread fallita"); exit(EXIT_FAILURE); }
   }

   while (!time_to_exit) {
      sleep(0.5);
      printf("Ho trovato %d Cartelle\n", cartelle);
   }
   printf("\nEcco la lista completa\n");
   VisitaLista(lis);
   printf("Totale Cartelle=%d\n", cartelle);
   for(i=0; i<proc; i++) {
      res=pthread_join(vet[i], NULL);
      if (res != 0) { perror("Join fallita"); exit(EXIT_FAILURE); }
   }
   return 0;
}
