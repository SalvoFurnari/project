#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>

#define DIMUTENTE 30

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

int RicercaFile(Lista lis, char* elem, char* nomefile){
   int len, esito=0;
   char* last;
   while (lis != NULL) {
       len=strlen(lis->info);
       last=&lis->info[len-strlen(elem)];      
       if (strcmp(last, elem)==0) { 
          strcpy(nomefile, lis->info);
          esito=1;
          return esito;
       }
       lis=lis->next;
   }
   return esito;
}

int RicercaFileLoop(Lista lis, char* elem, char* percorso){
   int len, esito=0;
   char* last;
   while (lis != NULL) {
       len=strlen(lis->info);
       last=&lis->info[len-strlen(elem)];      
       if (strcmp(last, elem)==0) { 
          strcat(percorso, lis->info);
          strcat(percorso, "\n");
          esito=1;
       }
       lis=lis->next;
   }
   strcat(percorso, "Quale vuoi? ");
   return esito;
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


pthread_mutex_t mutexpass;
char cwd[100]; //directory attuale
void *worker_thread(void *arg);

int main(int argc,char *argv[]) {
    int servsock, clisock;
    pthread_t wt;
    struct sockaddr_in baddr;

   int i, res;
   getcwd(cwd, 99);
   printf("Directory attuale: %s", cwd);
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

   /***********************Indicizzazione completata********/

    /* Inizializzazione mutex */
    pthread_mutex_init(&mutexpass, NULL);
    
    /*Creazione socket */
    servsock = socket(AF_INET, SOCK_STREAM, 0);
    if(servsock == -1) {
        perror("Errore creazione socket");
        exit(1);
    }
    
    baddr.sin_family = AF_INET;
    baddr.sin_port = htons(7777);
    baddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    /* bind socket */
    if(bind(servsock, (struct sockaddr *)&baddr, sizeof(baddr)) == -1) {
        perror("Errore bind");
        close(servsock);
        exit(1);
    }
    
    /* listen socket */
    if(listen(servsock, 5) == -1) {
        perror("Errore listen");
        close(servsock);
        exit(1);
    }
    
    while(1) {
        /* Il server si mette in attesa di una connessione */
        printf("Attendo connessione di un client...\n");
        clisock = accept(servsock, NULL, NULL);
        if(clisock == -1) {
            perror("Errore accept");
            continue;
        }
        
        /* Una volta avviata una nuova connessione la si fa gestire
         * ad un worker_thread */
        printf("Nuova connessione\n");
        if(pthread_create(&wt, NULL, worker_thread, (void *)&clisock) != 0) {
            fprintf(stderr, "Errore creazione thread\n");
        }
        
        /** Questa sleep serve ad aspettare che il worker_thread 
         * legga e memorizzi il valore di clisock */
        sleep(1);
    }
}

void *worker_thread(void *arg) {
    int sock;
    int pass=0, esito;
    char buff[200], buff2[200], auxbuff[200], verifica[50]; 
    char utente[DIMUTENTE], password[50], nomefile[100]; 
    char listbuff[10000]={0};
    size_t len;
    FILE *fd, *fp;
    int bytesReceived = 0;
    char recvBuff[1024];
    long size;
    
    sock = (*(int *)arg);
    fd=fopen("utenti.txt", "a+");
    if (fd==NULL) {
       perror("Errore apertura file");
       exit(1);
    } 
    
    while(1) {
        len = read(sock, (void *)buff, 199);
        if(len > 0)
            buff[len]='\0';
        if(buff[len - 1] == '\n')
            buff[len-1]='\0';
        
        /* Controlliamo quale comando è arrivato */
        if(len >= 9 && strncmp(buff, "register ", 9) == 0) {
            /* Se è arrivato register vuol dire che la parte successiva è il nome utente
             * questo lo andiamo a copiare nella variabile utente*/
            strcpy(utente, &buff[9]);
            fflush(stdout);
            /* snprintf scrive l'output di una printf in una stringa ( massimo n caratteri ) */
            snprintf(buff, 200, "\nWelcome %s!\n", utente);
            /* inviamo la risposta */
            write(sock, (void *)buff, strlen(buff));
            strcpy(buff, "\nInserisci password: ");
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);
            if(len > 0) 
               buff[len]='\0';
            pthread_mutex_lock(&mutexpass);  //accesso al file di testo condiviso
            fprintf(fd, "%s %s\n", utente, buff);
            fflush(fd);
            pthread_mutex_unlock(&mutexpass);
            strcpy(buff, "Password registrata!\n");
            write(sock, (void *)buff, strlen(buff));
        } else if(len >= 4 && strncmp(buff, "login", 5) == 0) {
            strcpy(buff, "\nInserisci Nome Utente: ");
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);
            if(len > 0)
               buff[len]='\0';
            if(buff[len - 1] == '\n')
               buff[len-1]='\0';
            strcat(buff, " ");
            strcpy(buff2, "\nInserisci Password: ");
            write(sock, (void *)buff2, strlen(buff2));
            len = read(sock, (void *)buff2, 199);
            if(len > 0)
               buff2[len]='\0';
            if(buff2[len - 1] == '\n')
               buff2[len-1]='\0';
            strcat(buff, buff2);
            printf("\n%s", buff);
            fflush(stdout);
            pthread_mutex_lock(&mutexpass);
            rewind(fd);
            while (fgets(verifica, 49, fd)) {
                 if(strncmp(verifica, buff, strlen(buff))==0) {
                    strcpy(buff, "\nLogin Effettuato!\n");
                    write(sock, (void *)buff, strlen(buff));
                    pass=1;
                    break;
                 }
            }
            pthread_mutex_unlock(&mutexpass);
            if (pass==0) {
               strcpy(buff, "\nPassword o Nome utente non trovato");
               write(sock, (void *)buff, strlen(buff));
            }
        } else if(len >= 5 && strncmp(buff, "send ", 5) == 0) {
            if (pass==0) {
               strcpy(auxbuff, "\nDevi effettuare il login!");
               write(sock, (void *)auxbuff, strlen(auxbuff));
               continue;
            }
            strcpy(auxbuff, "\nOk");
            write(sock, (void *)auxbuff, strlen(auxbuff));
            strcpy(buff2, &buff[5]);                     //uso buff2 come stringa per la ricerca
            pthread_mutex_lock(&Tmutex);
            printf("\nCerco i File");
            esito=RicercaFileLoop(lis, buff2, listbuff);     //Lista dei file
            if (esito==0) {
               printf("\nFile non trovato");
               continue;
            }
            pthread_mutex_unlock(&Tmutex);
            printf("\nFile trovati\n%s", listbuff);
            fflush(stdout);
            write(sock, (void *)listbuff, strlen(listbuff));
            len = read(sock, (void *)buff2, 199);
            if(len > 0)
               buff2[len]='\0';
            esito=RicercaFile(lis, buff2, nomefile);     //Nomefile è il Percorso
            strcpy(listbuff, "");
            fp = fopen(nomefile,"rb");
            if(fp==NULL)  {
            printf("Errore nell'apertura del file");
            break;   
            }   

        /* Legge dati dal file e li invia */
            while(1)  {
            /* Legge dal file segmenti di 1024 byte */
               unsigned char buff[1024]={0};
               int nread = fread(buff,1,1024,fp);        
            /* Se la lettura ha avuto successo, invio i dati. */
                if(nread > 0)  {
                   write(sock, buff, nread);
                   sleep(1);
                }
                if (nread < 1024)  {
                   if (feof(fp))    {
                      printf("Fine del file\n");
		              printf("Trasferimento file completato\n");
                      sleep(1);
		           }
                if (ferror(fp))
                    printf("Errore in lettura\n");
                break;
                }
            }
            strcpy (buff, "Ho finito");
            write(sock, (void *)buff, strlen(buff));
            fclose(fp);
        } else if(len >= 5 && strncmp(buff, "receive ", 8) == 0) {
            if (pass==0) {
               strcpy(auxbuff, "\nDevi effettuare il login!");
               write(sock, (void *)auxbuff, strlen(auxbuff));
               continue;
            }
            strcpy(auxbuff, "\nOk");
            write(sock, (void *)auxbuff, strlen(auxbuff));
            strcpy(nomefile, &buff[8]);
            printf("\nNome File: %s\n",nomefile);
	        printf("Sto ricevendo il file...");
   	        fp = fopen(nomefile, "ab"); 
    	    if(NULL == fp) {
       	      printf("Errore nell' apertura del file");
              break;
    	    }
    /* Riceve i dati a gruppi di 1024 bytes */
            while((bytesReceived = read(sock, recvBuff, 1024)) > 0) { 
               if (strncmp("Ho finito", recvBuff, 9)==0) break;
               printf("Ricevuti: %d Byte", bytesReceived);
	           fflush(stdout);
               fwrite(recvBuff, 1,bytesReceived,fp);
               fflush(fp);
            }
            if(bytesReceived < 0)  {
                printf("\n Errore in lettura dal socket \n");
            }
            printf("\nHo ricevuto il file\n");
            strcat(cwd, "/");
            strcat(cwd, nomefile);
            pthread_mutex_lock(&Tmutex);
            InserisciElementoInListaOrdinata(&lis, cwd);
            pthread_mutex_unlock(&Tmutex);
            getcwd(cwd, 99);
            printf("\nEcco la lista completa\n");
            VisitaLista(lis);
            fclose(fp);
        } else if(len >= 5 && strncmp(buff, "close", 5) == 0) {
            /* Nel caso di close interrompiamo il ciclo */
            break;
        } else {
            snprintf(buff, 200, "\nComando non riconosciuto!\n");
            write(sock, (void *)buff, strlen(buff));
        }
    }
    
    printf("Client disconnesso\n");
    close(sock);
}

