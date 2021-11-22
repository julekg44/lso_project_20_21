/*Gruppo 4 - Giuliano Galloppi N86001508 - Progetto di LSO A.A. 2020/21 GARA DI ARITMETICA*/
struct domanda{ //Struttura che definisce una domanda
    char testo[100];
    char a[100];
    char b[100];
    char c[100];
    char d[100];
    char correctAnswer[100];
};
typedef struct domanda Domanda;
typedef Domanda* DomandaPtr;


DomandaPtr initDom(); //Inizializza un puntatore a Domanda e lo ritorna
void printDomanda(DomandaPtr d); //Stampa una domanda d

void leggiRigaDaFile(int fd, char buffer[]); //Legge una riga dal file e la inserisce in 'buffer' posizionando l'offset all'inizio del rigo successivo
int leggiRigaDaFileInt(int fd, char buffer[]); //Ritorna 0 se il file ancora non e' finito dopo aver letto la riga

void leggiDomandaDaFile(int fd, DomandaPtr* dom); //Legge una domanda dal fd e la inserisce in 'd'
DomandaPtr leggiDomandaDaFilePtr(int fd); //Ritorna un puntatore alla domanda letta dal fd
int contaDomande(int fd, int nRigheDomandaPlusUno); //Ritorna il numero di domande contenute nel fd, il secondo argomento richiede da quante righe e' composta una domanda piu' 1

void sendDomanda(int sd, DomandaPtr d); //Invia testo e risposte della domanda 'd' al socket sd
int isCorrectWithSend(int sd, DomandaPtr d,char rispostaClient[],char messaggioCorretto[], char messaggioSbagliato[]); //Invia messaggi sul sd e ritorna 1 se la risposta del client e' corretta 0 altrimenti