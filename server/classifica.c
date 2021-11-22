/*Gruppo 4 - Giuliano Galloppi N86001508 - Progetto di LSO A.A. 2020/21 GARA DI ARITMETICA*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "classifica.h"

ClassificaListPtr initClassificaNodeList(char nomeGiocatore[], char clientAddress[], int tempoTotaleRisposte, int totRisposte, int tempoMedio, int postoClass) {//Inizializza e ritorna un puntatore alla classifica con un nuovo nodo aggiunto ad essa
    ClassificaListPtr L = (ClassificaListPtr)malloc(sizeof(struct giocatore));

    strcpy(L->nomeGiocatore,nomeGiocatore);
    L->tempoTotaleRisposte = tempoTotaleRisposte;
    L->totRisposte = totRisposte;
    L->tempoMedio = tempoMedio;
    strcpy(L->clientAddress,clientAddress);
    L->postoClass = postoClass;
    L->nextGiocatore = NULL;

    return L;
}

ClassificaListPtr appendNodeList(ClassificaListPtr L, char nomeGiocatore[], char clientAddress[], int tempoTotaleRisposte, int totRisposte, int tempoMedio, int postoClass) {//Aggiunge un nodo alla fine della lista controllandone l'esistenza
    if (L != NULL) {
        if ((strcmp(L->clientAddress,clientAddress)!=0)){
            L->nextGiocatore = appendNodeList(L->nextGiocatore, nomeGiocatore, clientAddress, tempoTotaleRisposte, totRisposte, tempoMedio, postoClass);
        }
    } else {
        L = initClassificaNodeList(nomeGiocatore, clientAddress, tempoTotaleRisposte, totRisposte, tempoMedio,postoClass);
    }
    return L;
}

ClassificaListPtr removeNodeList(ClassificaListPtr L, char clientAddress[]) {//Rimuove il nodo con clientAddress specificato dalla classifica
    if (L != NULL) {
        if ((strcmp(L->clientAddress,clientAddress)==0)){
            ClassificaListPtr tmp = L->nextGiocatore;
            free(L);
            return tmp;
        }
        L->nextGiocatore = removeNodeList(L->nextGiocatore, clientAddress);
    }
    return L;
}

void freeList(ClassificaListPtr L) {// Dealloca la classifica interamente
    if (L != NULL) {
        freeList(L->nextGiocatore);
        free(L);
    }
}

ClassificaListPtr randomList(int index, int mod) {
    srand((unsigned int)time(NULL));
    ClassificaListPtr L = NULL;
    int i = 0;
    char indirizzo[50];
    char nome[50];
    for (i = 0; i < index; i++) {
        sprintf(indirizzo,"indirizzo %d",i);
        sprintf(nome,"Test G%d",i);
        L = appendNodeList(L, nome ,indirizzo,rand()%mod,rand()%mod,rand()%mod,i);
    }
    return L;
}

void printListaGiocatori(ClassificaListPtr inizioLista){ //Stampa la lista dei giocatori completa
    int i = 1;
    if(inizioLista==NULL){
        puts("La lista della classifica e' vuota.\n");
    }
    else{
        printf("\n++++INIZIO STAMPA CLASSIFICA++++\n");
        printf("TOTALE GIOCATORI IN CLASSIFICA = %d\n",countNodes(inizioLista));
        printf("\n\n[CLIENT ADDRESS]    [NOME GIOCATORE]  [TEMPO TOTALE RISPOSTE] [TOT RISP CORRETTE]  [TEMPO MEDIO] [POS CLASSIFICA]\n");
        while (inizioLista != NULL){
            printf("%-20s",inizioLista->clientAddress);
            printf("%-18s",inizioLista->nomeGiocatore);
            printf("%-23d",inizioLista->tempoTotaleRisposte);
            printf("%-23d",inizioLista->totRisposte);
            if(inizioLista->tempoMedio==INFINITY_EXAMPLE){
                printf("99:99:99");
            } else {
                printf("%-12s",convertTimeToString(inizioLista->tempoMedio));
            }
            printf("%d\n",inizioLista->postoClass);
            i++;
            inizioLista = inizioLista->nextGiocatore;
        }
        puts("+++++FINE LISTA CLASSIFICA+++++\n");
    }
}

int countNodes(ClassificaListPtr list){ //Ritorna il numero di giocatori(nodi) presenti in classifica(lista)
    int n=0;
    while(list!=NULL){
        n++;
        list = list->nextGiocatore;
    }
    return n;
}

ClassificaListPtr searchClient(ClassificaListPtr inizioLista, char clientAddress[]){ //Ritorna il puntatore al nodo del client/giocatore , NULL altrimenti
    int trovato = 0;
    while((inizioLista!=NULL)&&(trovato==0)){
        if((strcmp(inizioLista->clientAddress,clientAddress))==0){
            trovato = 1;
        } else {
            inizioLista = inizioLista->nextGiocatore;
        }
    }
    return inizioLista;
}

int containsClient(ClassificaListPtr inizioLista, char clientAddress[]){ //Ritorna 1 se il client e' in classifica e quindi sta giocando
    int trovato = 0;
    if ((searchClient(inizioLista,clientAddress))!=NULL){
        trovato = 1;
    }
    return trovato;
}

void aggiornaPunteggioClient(ClassificaListPtr inizioLista, char clientAddress[], int tempoRisposta, int nRisposte) {
    //La media del tempo e' approssimata per difetto da 0 a 4 ed eccesso da 5 a 9
    ClassificaListPtr client = searchClient(inizioLista,clientAddress);
    client->tempoTotaleRisposte = client->tempoTotaleRisposte + tempoRisposta;
    client->tempoMedio = client->tempoTotaleRisposte / nRisposte;
    client->totRisposte += 1;
}

void swapNode(ClassificaListPtr a, ClassificaListPtr b){
    char nomeTmp[50];
    char clientTmp[50];
    int tempoTotaleRisposteTmp;
    int totRisposteTmp;
    int tempoMedioTmp;
    int postoClassTmp;

    strcpy(nomeTmp,a->nomeGiocatore);
    strcpy(clientTmp,a->clientAddress);
    tempoTotaleRisposteTmp = a->tempoTotaleRisposte;
    totRisposteTmp = a->totRisposte;
    tempoMedioTmp = a->tempoMedio;

    strcpy(a->nomeGiocatore,b->nomeGiocatore);
    strcpy(a->clientAddress,b->clientAddress);
    a->tempoTotaleRisposte = b->tempoTotaleRisposte;
    a->totRisposte = b->totRisposte;
    a->tempoMedio = b->tempoMedio;

    strcpy(b->nomeGiocatore,nomeTmp);
    strcpy(b->clientAddress,clientTmp);
    b->tempoTotaleRisposte = tempoTotaleRisposteTmp;
    b->totRisposte = totRisposteTmp;
    b->tempoMedio = tempoMedioTmp;

}

void bubbleSort(ClassificaListPtr start){
    int swapped;
    ClassificaListPtr ptr1;
    ClassificaListPtr lptr = NULL;

    if (start == NULL)
        return;

    do
    {
        swapped = 0;
        ptr1 = start;

        while (ptr1->nextGiocatore != lptr)
        {
            if (ptr1->tempoMedio > ptr1->nextGiocatore->tempoMedio)
            {
                swapNode(ptr1, ptr1->nextGiocatore);
                swapped = 1;
            }
            ptr1 = ptr1->nextGiocatore;
        }
        lptr = ptr1;
    }
    while (swapped);
}

void setPosizioni(ClassificaListPtr* classifica){ //Setta il valore 'postoClass' di ogni giocatore in ordine crescente(da usare dopo bubblesort)
    ClassificaListPtr il = *classifica;
    ClassificaListPtr inizioLista = *classifica;
    int i = 1;
    if(inizioLista==NULL){
        puts("La lista della classifica e' vuota.\n");
    }
    else{
        while (inizioLista != NULL){
            inizioLista->postoClass = i;
            i++;
            inizioLista = inizioLista->nextGiocatore;
        }
    }
    *classifica = il;
}

void printClassifica(ClassificaListPtr inizioLista){ //Stampa la classifica
    int i = 1;
    if(inizioLista==NULL){
        puts("La lista della classifica e' vuota.\n");
    }
    else{
        printf("\n++++INIZIO STAMPA CLASSIFICA++++\n");
        printf("TOTALE GIOCATORI IN CLASSIFICA = %d\n",countNodes(inizioLista));
        printf("\n\n[POSIZIONE]  [CLIENT ADDRESS]    [NOME GIOCATORE]  [TEMPO MEDIO]\n");
        while (inizioLista != NULL){
            printf("%-13d",i);
            printf("%-20s",inizioLista->clientAddress);
            printf("%-18s",inizioLista->nomeGiocatore);
            if(inizioLista->tempoMedio==INFINITY_EXAMPLE){
                printf("99:99:99\n");
            } else {
                printf("%s\n",convertTimeToString(inizioLista->tempoMedio));
            }
            i++;
            inizioLista = inizioLista->nextGiocatore;
        }
        puts("+++++FINE LISTA CLASSIFICA+++++\n");
    }
}

//Convertono i secondi totali in modo da ritornare l'ora esatta divisa in ore , min, e sec
int getJustHoursBySeconds(int seconds){ //Ritorna solo le ore che ci sono nel tempo ottenuto da quei secondi (prende solo HH da HH:MM:SS)
    int n;
    n = (seconds % (24*3600)) / 3600;
    return n;
}

int getJustMinutesBySeconds(int n){ //Ritorna solo i minuti che ci sono nel tempo ottenuto da quei secondi (prende solo MM da HH:MM:SS)
    int day = n / (24 * 3600);
    n = n % (24 * 3600);  //resto di secondi togliendo i giorni
    int hour = n / 3600;

    n %= 3600; //resto di secondi togliendo le ore
    int minutes = n / 60 ;
    return minutes;
}

int getJustSecondsBySeconds(int n){ //Ritorna solo i secondi che ci sono nel tempo ottenuto da quei secondi (prende solo SS da HH:MM:SS)
    int day = n / (24 * 3600);

    n = n % (24 * 3600);  //resto di secondi togliendo i giorni
    int hour = n / 3600;

    n %= 3600; //resto di secondi togliendo le ore
    int minutes = n / 60 ;

    n %= 60; //resto di secondi togliendo i minuti
    int seconds = n;
    return seconds;
}

char* convertTimeToString(int totalSeconds){ //mode indica se vogliamo convertire i secondi TOTALMENTE o in modo da ottenere un tempo corretto da quei secondi totali

    char* result = malloc(10*sizeof(char));
    int ore, min, sec;
    char h[10],m[10],s[10];

    if(totalSeconds!=0){
        ore= getJustHoursBySeconds(totalSeconds);
        min = getJustMinutesBySeconds(totalSeconds);
        sec = getJustSecondsBySeconds(totalSeconds);

        if((ore>=0) && (ore<=9)){
            sprintf(h,"0%d",ore);
        }else{
            sprintf(h,"%d",ore);
        }

        if((min>=0) && (min<=9)){
            sprintf(m,"0%d",min);
        }else{
            sprintf(m,"%d",min);
        }

        if((sec>=0) && (sec<=9)){
            sprintf(s,"0%d",sec);
        }else{
            sprintf(s,"%d",sec);
        }

        sprintf(result,"%s:%s:%s",h,m,s);

    }else{
        strcpy(result,"00:00:00");
    }
    return result;
}

void getSortedClassificaToString(ClassificaListPtr inizioLista,char classificaStr[]){ //Ordina prima la classifica per il tempo medio minore e poi la copia in dest
    char result[4096];
    char classString[4096];
    int i = 1;
    if(inizioLista==NULL){
        puts("La lista della classifica e' vuota.\n");
    }

    else{
        bubbleSort(inizioLista);
        sprintf(result,"\n\n\t\t\tCLASSIFICA:\n\n[POSIZIONE]  [CLIENT ADDRESS]    [NOME GIOCATORE]  [TEMPO MEDIO]\n");

        while (inizioLista != NULL){
            sprintf(classString,"%-13d",i);
            strcat(result,classString);

            sprintf(classString,"%-20s",inizioLista->clientAddress);
            strcat(result,classString);

            sprintf(classString,"%-18s",inizioLista->nomeGiocatore);
            strcat(result,classString);

            if(inizioLista->tempoMedio==INFINITY_EXAMPLE){
                sprintf(classString,"99:99:99\n");
                strcat(result,classString);
            }else {
                sprintf(classString, "%s\n", convertTimeToString(inizioLista->tempoMedio));
                strcat(result, classString);
            }

            i++;
            inizioLista = inizioLista->nextGiocatore;
        }

        strcat(result,"\0");
    }

    strcpy(classificaStr,result);
}

ClassificaListPtr initTemplateClassifica(){ //Ritorna un puntatore ad una classifica secondo un template di prova

    ClassificaListPtr classifica = NULL;
    classifica = appendNodeList(classifica,"Giulio","indirizzo Giulio",20,1,20,-2);

    aggiornaPunteggioClient(classifica,"indirizzo Giulio",37,2);
    classifica = appendNodeList(classifica,"Pinco","indirizzo 301",20,1,333,-2);
    classifica = appendNodeList(classifica,"Pallino","indirizzo 302",20,1,26362,-2);
    classifica = appendNodeList(classifica,"Panco","indirizzo 303",20,1,78,-2);

    return classifica;
}