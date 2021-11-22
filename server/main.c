/*Gruppo 4 - Giuliano Galloppi N86001508 - Progetto di LSO A.A. 2020/21 GARA DI ARITMETICA*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "domande.h"
#include "classifica.h"

#define PORTA_TCP 7295
#define LEN_QUEUE_LISTEN 5
#define SECONDI 60

struct infoClient{ //Struttura che contiene le informazioni da passare al thread
    int sd;
    char clientAddressString[INET_ADDRSTRLEN];
};
typedef struct infoClient InfoClient;
typedef InfoClient* InfoClientPtr;

/* VARIABILI GLOBALI */
int PORTA; //Riferimento alla porta tcp, se sare quella di default o quella in ingresso
int nGiocatore = 1;  //VARIABILE GLOBALE USATA QUANDO SI GENERANO AUTOMATICAMENTE NOMI GIOCATORI, da sostituire SCELTO DAL CLIENT OPPURE INVIARE AL CLIENT IL NOME ASSEGNATOGLI

pthread_t tidTimer; //tid del thread che gestisce il timer
pthread_mutex_t semaforoTimer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condizioneTimer = PTHREAD_COND_INITIALIZER;
int flagTimer = 0; //FALSO

pthread_mutex_t semaforoCurrentTime = PTHREAD_MUTEX_INITIALIZER; //Mutex del tempo corrente
pthread_cond_t condizioneAggiornamentoTempo = PTHREAD_COND_INITIALIZER; //Condizione sul tempo corrente
int tempoAggiornato = 1; //Condizione di aggiornamento del tempo corrente per ogni invio di domanda
int currentTempoTimer = 60;//Var globale del tempo corrente usata dai thread che gestiscono il client

//CLIENT CHE STANNO ANCORA RISPONDENDO
pthread_mutex_t semaforoStannoRispondendo = PTHREAD_MUTEX_INITIALIZER;
int stannoRispondendo = 0; //variabile globale che indica quanti giocatori stanno rispondendo a quella domanda, ++ quando domanda e' inviata, -- quando ricevi risposta
pthread_mutex_t semaforoProssimaDomanda = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condizioneProssimaDomanda = PTHREAD_COND_INITIALIZER;//ATTESA CHE I GIOCATORI RISPONDANO PER PASSARE ALLA PROSSIMA DOMANDA


pthread_mutex_t semaforoClassifica = PTHREAD_MUTEX_INITIALIZER;
ClassificaListPtr classifica = NULL;
int fdDomande;
DomandaPtr d;
int totGlobaleDomandeInviate = 1; //Totale delle domande estratte ed inviate ad i client
int totDomandeNelFile;

pthread_mutex_t semaforoLog = PTHREAD_MUTEX_INITIALIZER; //Mutex per sincronizzare i log
pthread_mutex_t semaforoReset = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t semaforoInPartita = PTHREAD_MUTEX_INITIALIZER;
int gInPartita = 0; //variabile globale che indica quanti giocatori sono in partita

/*Prototipi delle funzioni*/
int controlloArg(int argc, char** argv);
int initSocketServer(int *socket_fd, struct sockaddr_in serverAddr);

void *threadServerClient(void *arg); //Funzione di start del thread che gestisce il client
void *threadTimer(void *arg); //Funzione di start del timer

void checkAbbandonoClient(int sd, char rispostaClient[], char clientAddressString[], int count, int fineDom);
void aggiornaClassificaTempoMedioCorrette(int sd, char playerName[], char rispostaClient[], char clientAddressString[], int tempoRispostaClient , int* totDomandeRisposteClientPtr);
void resetPartita();//Resetta una partita
void incrementaInPartita();//Incrementa il numero dei giocatori in partita
void decrementaInPartita();//Decrementa il numero dei giocatori in partita

void getLogTimestampClient(char timeLogStr[], char dateLogStr[]);
void printLogInClient(char clientAddressString[]);
void printLogOutClient(char clientAddressString[]);


int main(int argc, char *argv[]) {
    controlloArg(argc,argv);
    printf("\nGIULIANO GALLOPPI N86001508 : Progetto di LSO lato server A.A. 2020/21 - Gara di Aritmetica\n\n");
    printf("Gara di Aritmetica : Server.\n");


    if ((fdDomande = open("domande.txt", O_RDONLY)) < 0) { //ATTENTO ALLE  DI PARENTESI NELL'IF PERCHE' NON TI FA L'ASSEGNAZIONE
        perror("Errore apertura file domande");
        exit(1);
    }

    d = initDom();
    totDomandeNelFile = contaDomande(fdDomande,7);

    printf("Numero di domande nel file domande.txt = %d\n",totDomandeNelFile);

    leggiDomandaDaFile(fdDomande, &d);
    classifica = NULL;

    int sdServer;
    struct sockaddr_in serverAddr;
    struct sockaddr_in client_connAddr;
    socklen_t client_len;

    int scelta;
    printf("0 per avviare il server, qualsiasi altro numero per uscire.\n");
    scanf("%d", &scelta);
    switch (scelta) {
        case 0:
            if(initSocketServer(&sdServer,serverAddr) < 0 ){ //init e mette in listen il socket, gli passiamo l'address del socket senno' non si modifica
                exit(1);
            }
            printf("Server avviato.\n");

            pthread_create(&tidTimer, NULL, threadTimer, NULL);
            sleep(1);

            while(1){
                client_len = sizeof(client_connAddr);
                int sd = accept(sdServer, (struct sockaddr *) &client_connAddr, &client_len);
                incrementaInPartita();

                char clientAddressString[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_connAddr, clientAddressString,sizeof(clientAddressString)); //CONVERSIONE DELL'INDIRIZZO DEL CLIENT NELLA STRINGA 'clientAddressString'
                printLogInClient(clientAddressString);

                InfoClientPtr info = (InfoClientPtr) malloc(sizeof(struct infoClient));
                info->sd = sd;
                strcpy(info->clientAddressString, clientAddressString);

                pthread_t tidThreadServerClient;
                pthread_create(&tidThreadServerClient, NULL, threadServerClient, (void *) info);
            }
            break;
        default:
            printf("Uscita.\n");
            //exit(1);
            break;
    }
    close(fdDomande);
    return 0;
}



void *threadTimer(void *arg) {
    pthread_mutex_lock(&semaforoTimer);

    while (!flagTimer) {
        pthread_mutex_lock(&semaforoCurrentTime);
        currentTempoTimer = SECONDI; //Ad ogni nuovo giro di domanda il tempo a disposizione e' di 60 secondi
        tempoAggiornato = 1;
        pthread_cond_broadcast(&condizioneAggiornamentoTempo);
        pthread_mutex_unlock(&semaforoCurrentTime);

        pthread_cond_wait(&condizioneTimer, &semaforoTimer); //Qui il thread del timer attende che un client inizi a giocare cosi' da avviare il timer
        pthread_mutex_unlock(&semaforoTimer);

        //Qui il timer e' partito
        struct timeval timeStamp;//Ottengo il timestamp appena il timer parte
        gettimeofday(&timeStamp, NULL);

        while (flagTimer) { //Col timer attivo ad ogni giro viene confrontato un secondo timestamp col primo per tenere il tempo

            struct timeval x;
            time_t resultTime;
            struct tm *convertedTime;
            gettimeofday(&x, NULL);

            resultTime = (x.tv_sec - timeStamp.tv_sec);
            convertedTime = localtime(&resultTime);
            pthread_mutex_lock(&semaforoCurrentTime);
            currentTempoTimer = SECONDI - ((convertedTime->tm_min * 60) + (convertedTime->tm_sec)); //Lo faccio per il valore intero, ma potrei anche evitarlo
            pthread_mutex_unlock(&semaforoCurrentTime);

        }
        pthread_mutex_lock(&semaforoTimer); //Riacquisisco il lock del timer poiche' rifacendo il giro il thread andra' in wait di un client che lo svegli

    }

    pthread_exit(0);
}


void* threadServerClient(void *arg) {
    InfoClient current = *((InfoClient *) arg);
    int sd = current.sd;
    char clientAddressString[INET_ADDRSTRLEN];
    strcpy(clientAddressString, current.clientAddressString);

    char rispostaClient[20]; //risposta del client alla domanda
    int tempoRispostaClient; //tempo impiegato dal client a rispondere
    int totDomandeRisposteClient = 1; //totale risposte date dal client

    char playerName[100];
    recv(sd,playerName,100,0);
	send(sd,clientAddressString,INET_ADDRSTRLEN,0); //Invio dell'ip al client per riconoscersi nella classifica


    int fineDom = totDomandeNelFile;
    char domandeNelFileStr[100];
    sprintf(domandeNelFileStr, "%d", totDomandeNelFile);

    char globaleDomandeInviateStr[100];
    sprintf(globaleDomandeInviateStr, "%d",totGlobaleDomandeInviate); //entro sulla domanda inviata n (cioe' sto rispondendo alla dom N e poi incr)

    pthread_mutex_lock(&semaforoInPartita);
    char inPartitaStr[100];
    sprintf(inPartitaStr, "%d",gInPartita);
    pthread_mutex_unlock(&semaforoInPartita);

    send(sd, domandeNelFileStr, 100, 0);
    send(sd, globaleDomandeInviateStr, 100, 0);
    send(sd, inPartitaStr, 100, 0);


    int count = totGlobaleDomandeInviate; //numero della domanda sulla quale il client si collega

    while (count <= fineDom) {//Il ciclo parte da 1 poiche' la prima domanda e' gia' estratta nel main
        count = count + 1;

        pthread_mutex_lock(&semaforoCurrentTime);
        if(!tempoAggiornato){ //Se rifacendo il giro il tempo non si e' resettato a 60 non si puo' continuare
            pthread_cond_wait(&condizioneAggiornamentoTempo,&semaforoCurrentTime);
        }
        char inviaCurrentTime[20];
        int tempoPerRispondere = currentTempoTimer;
        sprintf(inviaCurrentTime, "%d", currentTempoTimer);
        pthread_mutex_unlock(&semaforoCurrentTime);

        send(sd, inviaCurrentTime, 20, 0);

        pthread_mutex_lock(&semaforoTimer); //Quando si collega il client indico che il flagTimer e' vero, cosi' se spento posso farlo ripartire
        flagTimer = 1; //IL Timer scorre parallelamente a questo thread che ha inviato la domanda ed il tempo corrente
        pthread_mutex_unlock(&semaforoTimer);

        sendDomanda(sd, d);
        pthread_cond_signal(&condizioneTimer);//Avvia il timer

        pthread_mutex_lock(&semaforoStannoRispondendo); //Incrementa il numero di giocatori che stanno rispondendo alla domanda corrente
        stannoRispondendo++;
        pthread_mutex_unlock(&semaforoStannoRispondendo);

        recv(sd, rispostaClient, 20, 0); //Ricezione risposta del client
        checkAbbandonoClient(sd,rispostaClient,clientAddressString,count,fineDom);

        pthread_mutex_lock(&semaforoCurrentTime);//Salvataggio del tempo impiegato dal client a rispondere
        tempoRispostaClient = tempoPerRispondere - currentTempoTimer; //Poiche' il tempo e' a decrescere da 60 a 0 (cioe' in quanti sec ha risposto max fino a 60)
        pthread_mutex_unlock(&semaforoCurrentTime);

        aggiornaClassificaTempoMedioCorrette(sd,playerName,rispostaClient,clientAddressString,tempoRispostaClient,&totDomandeRisposteClient);

        pthread_mutex_lock(&semaforoStannoRispondendo); //Decrementa il numero di giocatori che stanno rispondendo alla domanda corrente
        stannoRispondendo--;
        pthread_mutex_unlock(&semaforoStannoRispondendo);

        char messaggioGenerico[100];
        pthread_mutex_lock(&semaforoProssimaDomanda);
        if (stannoRispondendo!=0) { //Se qualcun'altro deve ancora rispondere allora aspetta che gli altri finiscano
            strcpy(messaggioGenerico, "In attesa che tutti i giocatori rispondano, poi si passera' alla prossima domanda\n");//Invia messaggio al client che e' in attesa
            send(sd, messaggioGenerico, 100, 0);

            pthread_cond_wait(&condizioneProssimaDomanda, &semaforoProssimaDomanda); //Mette in attesa il thread finche' non abbiano risposto tutti e quindi passare alla prossima domanda
        }
        else { //Qui procede l'ultimo client che doveva rispondere
            strcpy(messaggioGenerico, "Tutti i giocatori hanno risposto, ecco la prossima domanda.\n");
            send(sd, messaggioGenerico, 100, 0);

            totGlobaleDomandeInviate++; //Incremento il contatore delle domande estratte

            if (count <= fineDom) {
                leggiDomandaDaFile(fdDomande, &d); //Legge la prossima domanda
            }

            pthread_mutex_lock(&semaforoTimer);
            flagTimer = 0; //Fermo il timer per riportarlo all'inizio
            tempoAggiornato = 0; //Sincronizzo i thread in modo che aspettino il tempo riparta da 60
            pthread_mutex_unlock(&semaforoTimer);

            pthread_cond_broadcast(&condizioneProssimaDomanda); //Sveglio tutti i thread in attesa che gli altri finissero di rispondere e cosi' procedere alla prossima domanda
        }
        pthread_mutex_unlock(&semaforoProssimaDomanda);

        char confermaNuovoGiro[100];
        recv(sd, confermaNuovoGiro, 100,0); //qui tutti i client mandano la conferma del nuovo giro altrimenti il thread si blocca sulla recv
    }


    char classificaString[4096];
    pthread_mutex_lock(&semaforoClassifica);
    getSortedClassificaToString(classifica,classificaString);
    send(sd, classificaString, 4096, 0); //Invio della classifica al giocatore a fine partita
    pthread_mutex_unlock(&semaforoClassifica);


    printLogOutClient(clientAddressString);
    resetPartita();
    pthread_exit(0);
}

int controlloArg(int argc, char** argv){
    if (argc > 2) { //0='./nomeProg', 1=portaTCP
        printf("Numero argomenti non validi.\nUsage : %s <TCP Port>\n", argv[0]);
        exit(1);
    }
    else if (argc == 2){ //Abbiamo specificato la porta tcp
        //Controllo porta TCP valida u_int16_t
        int val1 = atoi(argv[1]);
        int val2 = atoi(argv[1]);
        if( (val1<0) || (val2> 65535) ){
            printf("Numero di porta non consentito.\nScegli una porta tra 0 e 65535\nUsage : %s <TCP Port>\n\n", argv[0]);
            exit(2);
        }
        PORTA = atoi(argv[1]);
    }

    else if(argc==1){
        printf("Stai utilizzando la PORTA TCP di default: 7295\n");
        PORTA = PORTA_TCP;
    }
    return 0;
}

int initSocketServer(int *socket_fd, struct sockaddr_in serverAddr) {
    int sd = *socket_fd;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORTA); //porta
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // cosi' accetta connessioni dirette a qualunque indirizzo
    memset(&(serverAddr.sin_zero), '\0', 8);
    sd = socket(PF_INET, SOCK_STREAM, 0);

    if (sd < 0) {
        perror("funzione socket() fallita\n");
        return -1;
    }

    int val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0) {
        perror("setsockopt() error");
    }

    if (bind(sd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) { // qui facciamo il cast poiche' bind vuole un indirizzo generico
        perror("errore bind del socket");
        return -1;
    }

    if(listen(sd, LEN_QUEUE_LISTEN) < 0){
        perror("Errore listen.\n");
        return -1;
    }
    *socket_fd = sd;
    return 0;
}

void checkAbbandonoClient(int sd, char rispostaClient[], char clientAddressString[], int count, int fineDom){ //Controllo se il client ha deciso di uscire termino il thread
    if(strcmp(rispostaClient,"1\0")==0){
        pthread_mutex_lock(&semaforoClassifica);
        pthread_mutex_lock(&semaforoStannoRispondendo);

        classifica = removeNodeList(classifica,clientAddressString);
        write(STDOUT_FILENO,"\nClient ",strlen("\nClient "));
        write(STDOUT_FILENO,clientAddressString,strlen(clientAddressString));
        write(STDOUT_FILENO," ha abbandonato la partita\n",strlen(" ha abbandonato la partita\n"));
        printLogOutClient(clientAddressString);

        stannoRispondendo--;
        decrementaInPartita();

        if(gInPartita==0){ //Se nessun giocatore resta in partita allora resetto
            resetPartita();
            pthread_mutex_lock(&semaforoTimer);
            flagTimer = 0; //Ripristino il timer per una prossima partita
            tempoAggiornato = 0; //Sincronizzo i thread in modo che aspettano che il tempo riparta da 60
            pthread_mutex_unlock(&semaforoTimer);
            pthread_mutex_unlock(&semaforoStannoRispondendo);
            pthread_mutex_unlock(&semaforoClassifica);
            pthread_exit(0);
        }

        if((stannoRispondendo==0)&&(gInPartita!=0)){ //Se e' l'ultimo client ad aver abbandonato deve fare quello che avrebbe fatto l'ultimo client a rispondere
            totGlobaleDomandeInviate++;
            if (count <= fineDom) {
                leggiDomandaDaFile(fdDomande, &d);
            }

            pthread_mutex_lock(&semaforoTimer);
            flagTimer = 0;
            tempoAggiornato = 0; //Sincronizzo i thread in modo che aspettano che il tempo riparta da 60
            pthread_mutex_unlock(&semaforoTimer);

            pthread_cond_broadcast(&condizioneProssimaDomanda);
        }

        pthread_mutex_unlock(&semaforoStannoRispondendo);
        pthread_mutex_unlock(&semaforoClassifica);
        pthread_exit(0);
    }
}

void aggiornaClassificaTempoMedioCorrette(int sd, char playerName[], char rispostaClient[], char clientAddressString[], int tempoRispostaClient , int* totDomandeRisposteClientPtr){
    //L'ordine della classifica e' basato sul tempo medio impiegato per rispondere correttamente ad una domanda
    //Se un client non avra' mai risposto correttamente avra' un punteggio uguale ad un valore infinito
    //Che sara' poi visualizzato in classifica come 99:99:99 , questo assegnando ai tempi del giocatore il valore INFINITY_EXAMPLE

    pthread_mutex_lock(&semaforoClassifica);
    int correct;
    int totDomandeRisposteClient = *totDomandeRisposteClientPtr;
    int tempoInizialeInfinito = INFINITY_EXAMPLE; //Tempo fittizzio che rappresenta infinito, questo posizionera' chi sbaglia tutte le risp in fondo alla classifica

    if ((containsClient(classifica, clientAddressString)) == 1) {
        correct = isCorrectWithSend(sd, d, rispostaClient, "Risposta ESATTA punteggio aggiornato!\0","Risposta Sbagliata! Resta il vecchio punteggio\0");
        if (correct == 1) {
            //Se alla prima risposta corretta c'e'il punteggio infinito, lo resetto per iniziare a sommare i punteggi corretti
            Giocatore* current = searchClient(classifica,clientAddressString);
            if(current->tempoTotaleRisposte==tempoInizialeInfinito){
                current->tempoTotaleRisposte = 0;
            }
            aggiornaPunteggioClient(classifica, clientAddressString, tempoRispostaClient, totDomandeRisposteClient);
            totDomandeRisposteClient = totDomandeRisposteClient+1;
        }
    }
    else { //In questo else si va' solo la prima volta che si esegue l'append di un giocatore
        correct = isCorrectWithSend(sd, d, rispostaClient, "Risposta ESATTA!\0","Risposta SBAGLIATA!\0");
        if(correct == 1){
            int tMedio = (tempoRispostaClient/totDomandeRisposteClient);
            classifica = appendNodeList(classifica, playerName, clientAddressString, tempoRispostaClient, totDomandeRisposteClient, tMedio,-1);
            totDomandeRisposteClient = totDomandeRisposteClient+1;
        }else{ //RISPOSTA SBAGLIATA allora il tempo di risposta e tempo medio del client da aggiungere al suo totale e' INFINITO
            tempoRispostaClient = tempoInizialeInfinito;
            classifica = appendNodeList(classifica, playerName, clientAddressString, tempoRispostaClient, 0, tempoRispostaClient,-1);
        }
        nGiocatore++;
    }

    bubbleSort(classifica);
    *totDomandeRisposteClientPtr = totDomandeRisposteClient;
    pthread_mutex_unlock(&semaforoClassifica);
}

void resetPartita(){
    pthread_mutex_lock(&semaforoReset);
    nGiocatore = 1;
    totGlobaleDomandeInviate = 1;
    lseek(fdDomande,0,SEEK_SET);
    d = leggiDomandaDaFilePtr((fdDomande));
    freeList(classifica);
    classifica = NULL;
    gInPartita = 0;
    pthread_mutex_unlock(&semaforoReset);
}

void incrementaInPartita(){
    pthread_mutex_lock(&semaforoInPartita);
    gInPartita++;
    pthread_mutex_unlock(&semaforoInPartita);
}

void decrementaInPartita(){
    pthread_mutex_lock(&semaforoInPartita);
    gInPartita--;
    pthread_mutex_unlock(&semaforoInPartita);
}

void getLogTimestampClient(char timeLogStr[], char dateLogStr[]){
    struct tm *tm;
    time_t t = time(NULL);
    char str_time[100],str_date[100];
    tm = localtime(&t);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", tm);
    strftime(str_date, sizeof(str_date), "%d/%m/%Y", tm);
    strcpy(timeLogStr,str_time);
    strcpy(dateLogStr,str_date);
}

void printLogInClient(char clientAddressString[]){
    pthread_mutex_lock(&semaforoLog);
    write(STDOUT_FILENO,"\n",strlen("\n"));
    write(STDOUT_FILENO,"Nuova connessione dal client d'indirizzo = ",strlen("Nuova connessione dal client d'indirizzo = "));
    write(STDOUT_FILENO,clientAddressString,strlen(clientAddressString));

    struct tm *tm;
    time_t t = time(NULL);
    char str_time[100],str_date[100];
    tm = localtime(&t);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", tm);
    strftime(str_date, sizeof(str_date), "%d/%m/%Y", tm);

    write(STDOUT_FILENO," in data: ",strlen("\" in data: "));
    write(STDOUT_FILENO,str_date,strlen(str_date));

    write(STDOUT_FILENO," alle ore: ",strlen(" alle ore: "));
    write(STDOUT_FILENO,str_time,strlen(str_time));

    write(STDOUT_FILENO,"\n",strlen("\n"));

    pthread_mutex_unlock(&semaforoLog);
}

void printLogOutClient(char clientAddressString[]){
    pthread_mutex_lock(&semaforoLog);
    write(STDOUT_FILENO,"Il client d'indirizzo : ",strlen("Il client d'indirizzo : "));
    write(STDOUT_FILENO,clientAddressString,strlen(clientAddressString));
    write(STDOUT_FILENO," si e' sconnesso dal server",strlen(" si e' sconnesso dal server"));

    struct tm *tm;
    time_t t = time(NULL);
    char str_time[100],str_date[100];
    tm = localtime(&t);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", tm);
    strftime(str_date, sizeof(str_date), "%d/%m/%Y", tm);

    write(STDOUT_FILENO," in data: ",strlen("\" in data: "));
    write(STDOUT_FILENO,str_date,strlen(str_date));

    write(STDOUT_FILENO," alle ore: ",strlen(" alle ore: "));
    write(STDOUT_FILENO,str_time,strlen(str_time));

    write(STDOUT_FILENO,"\n",strlen("\n"));
    pthread_mutex_unlock(&semaforoLog);
}
