/*Gruppo 4 - Giuliano Galloppi N86001508 - Progetto di LSO A.A. 2020/21 GARA DI ARITMETICA*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>
#include<errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "domande.h"

DomandaPtr initDom(){ //Inizializza un puntatore a Domanda e lo ritorna
    DomandaPtr d = (DomandaPtr) malloc(sizeof(Domanda));
    return d;
}

void printDomanda(DomandaPtr d){ //Stampa una domanda d
    printf("\n----Print Domanda :----\n");
    printf("%s\n",d->testo);
    printf("%s\n",d->a);
    printf("%s\n",d->b);
    printf("%s\n",d->c);
    printf("%s\n----------------------\n",d->d);
}


void leggiRigaDaFile(int fd, char buffer[]){//Legge una riga dal file e la inserisce in 'buffer' posizionando l'offset all'inizio del rigo successivo
    char c = (char)0;
    int indice = 0;
    int nread = 0;
    char vettore[100];
    int flagEOF = 0;
    while((c!='\n')&&(c!=EOF)){

        if( (nread = read(fd,&c,1)) <0 ){ //leggo un carattere per volta
            perror("read error in leggiRigaDaFile");
        } else if(nread == 0) {//EOF
            perror("Il file e' finito");
            c = '\n';
            flagEOF = 1;
            break;
        } else {
            if(c!='\n'){
                vettore[indice] = c;
                indice++;
            }
        }
    }
    c = (char) 0;
    vettore[indice] = '\0';
    if(flagEOF==0){
        strcpy(buffer,vettore);
    }
}

int leggiRigaDaFileInt(int fd, char buffer[]){ //Ritorna 0 se il file ancora non e' finito dopo aver letto la riga
    char c = (char)0;
    int indice = 0;
    int nread = 0;
    char vettore[100];
    int flagEOF = 0;
    while((c!='\n')&&(c!=EOF)){
        if( (nread = read(fd,&c,1)) <0 ){
            perror("read error in leggiRigaDaFileInt");
        } else if(nread == 0) {//EOF
            c = '\n';
            flagEOF = 1;
            break;
        } else {
            if(c!='\n'){
                vettore[indice] = c;
                indice++;
            }
        }
    }
    c = (char) 0;
    vettore[indice] = '\0';
    if(flagEOF==0){
        strcpy(buffer,vettore);
    }
    return flagEOF;
}


void leggiDomandaDaFile(int fd, DomandaPtr* dom){ //Legge una domanda dal fd e la inserisce in 'd'
    DomandaPtr d = *dom;
    leggiRigaDaFile(fd,d->testo);
    leggiRigaDaFile(fd,d->a);
    leggiRigaDaFile(fd,d->b);
    leggiRigaDaFile(fd,d->c);
    leggiRigaDaFile(fd,d->d);
    leggiRigaDaFile(fd,d->correctAnswer);

    //adesso deve saltare il rigo per posizionare l'offset sulla domanda successiva
    char tmp = (char)0;
    read(fd,&tmp,1);
    tmp = (char)0;

    *dom = d;
}


DomandaPtr leggiDomandaDaFilePtr(int fd){ //Ritorna un puntatore alla domanda letta dal fd
    DomandaPtr d = malloc(sizeof(Domanda));
    leggiRigaDaFile(fd,d->testo);
    leggiRigaDaFile(fd,d->a);
    leggiRigaDaFile(fd,d->b);
    leggiRigaDaFile(fd,d->c);
    leggiRigaDaFile(fd,d->d);
    leggiRigaDaFile(fd,d->correctAnswer);

    //adesso deve saltare il rigo per posizionare l'offset sulla domanda successiva
    char tmp = (char)0;
    read(fd,&tmp,1);
    tmp = (char)0;

    return d;
}

int contaDomande(int fd, int nRigheDomandaPlusUno){ //Ritorna il numero di domande contenute nel fd, il secondo argomento richiede da quante righe e' composta una domanda piu' 1
    off_t current = lseek(fd,0,SEEK_CUR);
    lseek(fd,0,SEEK_SET);

    int finito = 0;
    int totDomande = 0;
    char tempBuffer[100];
    int i=0;

    while(!finito){
        i++;
        finito = leggiRigaDaFileInt(fd,tempBuffer);
    }

    lseek(fd,current,SEEK_SET);
    totDomande = i/nRigheDomandaPlusUno;
    return totDomande;
}

void sendDomanda(int sd, DomandaPtr d){ //Invia testo e risposte della domanda 'd' al socket sd
    send(sd,d->testo,100,0);
    send(sd,d->a,100,0);
    send(sd,d->b,100,0);
    send(sd,d->c,100,0);
    send(sd,d->d,100,0);

}

int isCorrectWithSend(int sd, DomandaPtr d,char rispostaClient[],char messaggioCorretto[], char messaggioSbagliato[]){ //Invia messaggi sul sd e ritorna 1 se la risposta del client e' corretta 0 altrimenti
    int trovato=0; //falso
    char msgCorrettaErrata[100];

    if(rispostaClient[0]==d->correctAnswer[0]){
        strcpy(msgCorrettaErrata,messaggioCorretto);
        send(sd,msgCorrettaErrata,100,0);
        trovato=1;
    } else {
        strcpy(msgCorrettaErrata,messaggioSbagliato);
        send(sd,msgCorrettaErrata,100,0);
    }

    return trovato;
}