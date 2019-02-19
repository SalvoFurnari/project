#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

int menu(void) {
	int scelta;

	printf(" \n\n******** MENU' - Comunicazione Server ***********\n");
	printf("1.  Registrazione \n");
	printf("2.  Login\n");
	printf("3.  Richiedi File al Server\n");
	printf("4.  Invia File al Server\n");
	printf("0.  Termina Connessione\n\n");
	printf("--->  ");
	do {
		scanf("%d", &scelta);
	} while ((scelta<0) && (scelta>9));

	return scelta;
}


int sock;
int main(int argc,char *argv[]){
    int scelta = 0;
    size_t len;
    char buff[200], auxbuff[200]={0}, nomeutente[50], password[50], nomefile[50];
    struct sockaddr_in daddr;
    struct hostent *hent;
    FILE *fp;
    int bytesReceived = 0;
    char recvBuff[1024];
    char listbuff[10000]={0};
    int indice;

    sock=socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1) {
        perror("Errore creazione socket");
        exit(1);
    }

    daddr.sin_family=AF_INET;
    daddr.sin_port=htons(7777);
    hent=gethostbyname(argv[1]);
    daddr.sin_addr=*(struct in_addr*)*hent->h_addr_list;
    if(connect(sock, (struct sockaddr*)&daddr, sizeof(daddr))==-1) {
         perror("Errore Connect");
         exit(1);
    }
    printf("Connessione riuscita!\n");

    do{
		scelta = menu();

		switch (scelta){
		case 1:
            strcpy(buff, "register ");
			printf("Inserisci Nome Utente: "); 
			scanf("%s", nomeutente);
            strcat(buff, nomeutente);
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);
            if(len > 0)
               buff[len]='\0';
            printf("%s", buff);
            scanf("%s", password);
            strcpy(buff, password);
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);
            if(len > 0)
               buff[len]='\0';
            printf("%s", buff);
			break;
		case 2:
			strcpy(buff, "login");
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);
            if(len > 0)
               buff[len]='\0';
            printf("%s", buff);
            scanf("%s", nomeutente);
            strcpy(buff, nomeutente);
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);
            if(len > 0)
               buff[len]='\0';
            printf("%s", buff);
            scanf("%s", password);
            strcpy(buff, password);
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);
            if(len > 0)
               buff[len]='\0';
            printf("%s", buff);
			break;
		case 3:
			strcpy(buff, "send ");
            printf("Inserisci Estensione File: "); 
			scanf("%s", nomefile);
            strcat(buff, nomefile);
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);     //mi dice se avevo fatto il login
            if(len > 0)
               buff[len]='\0';
            printf("%s", buff);
            fflush(stdout);
            if(strcmp("\nDevi effettuare il login!", buff)==0) 
                    break;
            len = read(sock, (void *)listbuff, 9999);  //Lista dei file
            if(len > 0)
               listbuff[len]='\0';
            for(indice=0; indice<len; indice++) {
                if (listbuff[indice]=='?')
                    break;
            }
            strcpy(&listbuff[indice+2], "");
            printf("%s", listbuff);
            fflush(stdout);
           // printf("Quale vuoi? ");
            scanf("%s", auxbuff);   //Acquisizione scelta
            write(sock, (void *)auxbuff, strlen(auxbuff));
            fp = fopen(auxbuff, "ab"); 
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
            printf("\nDownload completato\n");
            fclose(fp);
			break;
		case 4:
			strcpy(buff, "receive ");
			printf("Inserisci Nome File: "); 
			scanf("%s", nomefile);
            strcat(buff, nomefile);
            write(sock, (void *)buff, strlen(buff));
            len = read(sock, (void *)buff, 199);     //mi dice se avevo fatto il login
            if(len > 0)
               buff[len]='\0';
            printf("%s", buff);
            fflush(stdout);
            if(strcmp("\nDevi effettuare il login!", buff)==0) 
                    break;
            printf("Inserisci Percorso File: ");  //dove si trova nel Client
			scanf("%s", nomefile);
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
		              printf("Upload completato\n");
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
			break;
         }       
      } while (scelta != 0);
      printf("Disconnessione\n");
      close(sock);
      return 0;
}
