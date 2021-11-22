/*Gruppo 4 - Giuliano Galloppi N86001508 - Progetto di LSO A.A. 2020/21 GARA DI ARITMETICA*/
#include <time.h>
#define INFINITY_EXAMPLE 86400

struct giocatore{ //Struttura giocatore(client)
    char nomeGiocatore[100];
    char clientAddress[50]; //Sara' univoco quindi usato come 'id' per ogni giocatore
    int tempoTotaleRisposte;//misurato in secondi
    int totRisposte;//misurato in secondi
    int tempoMedio;//misurato in secondi
    int postoClass;
    struct giocatore* nextGiocatore;
};
typedef struct giocatore Giocatore;
typedef Giocatore* ClassificaListPtr; //Classifica e' vista come una lista di giocatori

ClassificaListPtr initClassificaNodeList(char nomeGiocatore[], char clientAddress[], int tempoTotaleRisposte, int totRisposte, int tempoMedio, int postoClass);//Inizializza e ritorna un puntatore alla classifica con un nuovo nodo aggiunto ad essa
ClassificaListPtr appendNodeList(ClassificaListPtr L, char nomeGiocatore[], char clientAddress[], int tempoTotaleRisposte, int totRisposte, int tempoMedio, int postoClass);// Aggiunge un nodo alla fine della classifica controllandone l'esistenza
ClassificaListPtr removeNodeList(ClassificaListPtr L, char clientAddress[]);//Rimuove il nodo con clientAddress specificato dalla classifica
void freeList(ClassificaListPtr L);// Dealloca la classifica interamente
ClassificaListPtr randomList(int index, int mod);//Ritorna un puntatore con una classifica randomica per test
void printListaGiocatori(ClassificaListPtr inizioLista); //Stampa la lista dei giocatori completa
int countNodes(ClassificaListPtr list); //Ritorna il numero di giocatori(nodi) presenti in classifica(lista)

ClassificaListPtr searchClient(ClassificaListPtr inizioLista, char clientAddress[]); //Ritorna il puntatore al nodo del client/giocatore , NULL altrimenti
int containsClient(ClassificaListPtr inizioLista, char clientAddress[]); //Ritorna 1 se il client e' in classifica e quindi sta giocando, 0 altrimenti

void aggiornaPunteggioClient(ClassificaListPtr inizioLista, char clientAddress[], int tempoRisposta, int nRisposte); //Aggiorna il tempo medio delle risposte corrette del client d'indirizzo 'clientAddress'
void swapNode(ClassificaListPtr a, ClassificaListPtr b);
void bubbleSort(ClassificaListPtr start); //Ordina la classifica per chi ha tempo medio delle risposte minore degli altri
void setPosizioni(ClassificaListPtr* classifica); //Setta il valore 'postoClass' di ogni giocatore in ordine crescente(da usare dopo bubblesort)
void printClassifica(ClassificaListPtr inizioLista);//Stampa la classifica

int getJustHoursBySeconds(int seconds); //Ritorna solo le ore che ci sono nel tempo ottenuto da quei secondi (prende solo HH da HH:MM:SS)
int getJustMinutesBySeconds(int n); //Ritorna solo i minuti che ci sono nel tempo ottenuto da quei secondi (prende solo MM da HH:MM:SS)
int getJustSecondsBySeconds(int n); //Ritorna solo i secondi che ci sono nel tempo ottenuto da quei secondi (prende solo SS da HH:MM:SS)
char* convertTimeToString(int totalSeconds);//Converte i secondi in una stringa del formato HH:MM:SS

void getSortedClassificaToString(ClassificaListPtr inizioLista,char classificaStr[]); //Converte inizioLista in stringa e la mette in classificaStr

ClassificaListPtr initTemplateClassifica(); //Ritorna un puntatore ad una classifica secondo un template di prova