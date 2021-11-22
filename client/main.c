/*Gruppo 4 - Giuliano Galloppi N86001508 - Progetto di LSO A.A. 2020/21 GARA DI ARITMETICA*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>
#include <string.h>

#define PORTA_TCP 7295

/*Variabili globali*/
int PORTA;
char ip[INET_ADDRSTRLEN];

char testoD[100];
char a[100];
char b[100];
char c[100];
char d[100];

int tempoDelServerInt = 0;
char tempoDelServer[20];
fd_set readfds;

/*Prototipi delle funzioni*/
void intro();
int controlloArg(int argc, char** argv); 
int initSocketClient(int* socket_fd, struct sockaddr_in* serverAddr,int argc);
void riceviDomandaDaSocket(int sd); //Riceve la domanda dal socket sd e la memorizza nelle var. globali
void printDomanda();

void letturaConSceltaSelect(int sd, int sec, fd_set* readfds, char rispostaC[]); //Legge da 'sd' entro un timeout di 'sec' e memorizza la risposta in 'rispostaC'
void initTimeoutSelect(int sd, int sec, fd_set* readfds); 
void letturaConTimeout(int timeout,fd_set* readfds1, int* valoreLetto); //Lettura con timeout di una stringa da stdin senza controllo

void uscitaHandler(int segnale); //Handler per i segnali SIGINT, SIGTSTP e SIGPIPE
void checkUscita(int sd,char rispostaClient[]);




int main(int argc, char** argv) {
    controlloArg(argc,argv);
    printf("\nGIULIANO GALLOPPI N86001508 : Progetto di LSO lato client A.A. 2020/2021 - Gara di Aritmetica\n\n");
    signal(SIGINT,uscitaHandler);
	signal(SIGTSTP,uscitaHandler);
    signal(SIGPIPE,uscitaHandler);
    intro();

    char msg[100];
    struct sockaddr_in IndirizzoServer;
    int sd;
    int nDomandePartita; //totale delle domande a cui rispondere
    int nDomandaIngresso; //numero della domanda da cui iniziamo a rispondere all'ingresso in partita
    int inPartita; //numero dei giocatori che stanno giocando alla partita
    int flagIngresso = 0;

    initSocketClient(&sd,&IndirizzoServer,argc);

    printf("Vuoi connetterti al server ed iniziare a giocare ?\n1 - Si, inizia partita.\n2 - No, esci.\n");
    int choice;
    scanf("%d",&choice);
    if(choice!=1){
        close(sd);
        exit(2);
    }

    if (connect(sd , (struct sockaddr *) &IndirizzoServer , sizeof(IndirizzoServer)) < 0){
        perror("connect");
        exit(1);
    }

    char nomeClient[100];
	char mioIndirizzo[INET_ADDRSTRLEN];
    printf("\nInserisci nome giocatore:\n");
    scanf("%s",nomeClient);
    send(sd,nomeClient,100,0);
	recv(sd,mioIndirizzo,INET_ADDRSTRLEN,0);

    char nDomandePartita_str[100],nDomandaIngresso_str[100],inPartita_str[100];
    recv(sd,nDomandePartita_str,100,0);
    recv(sd,nDomandaIngresso_str,100,0);
    recv(sd,inPartita_str,100,0);
    nDomandePartita=atoi(nDomandePartita_str);
    nDomandaIngresso=atoi(nDomandaIngresso_str);
    inPartita = atoi(inPartita_str);
    

    printf("\nIngresso in partita: DOMANDA %d/%d\n",nDomandaIngresso,nDomandePartita);

    if(inPartita>=2){ //uno che giocava + io che sono entrato
        printf("Ti sei unito ad una partita gia' iniziata.\n");
        flagIngresso = 1;
    }

    int count=nDomandaIngresso;
    while(count<=nDomandePartita){

        recv(sd,tempoDelServer,20,0);
        tempoDelServerInt = atoi(tempoDelServer);

        riceviDomandaDaSocket(sd);
        printf("\n[Gioca]: %s - Tempo per rispondere: %ss\n",nomeClient,tempoDelServer);
        printDomanda();

        char rispostaClient[20];
        letturaConSceltaSelect(STDIN_FILENO,tempoDelServerInt,&readfds,rispostaClient); ///Lettura della risposta del client con controllo di validita'
        send(sd,rispostaClient,20,0);//INVIA LA RISPOSTA DEL CLIENT AL SERVER

        checkUscita(sd,rispostaClient); //Il client puo' abbandonare la partita inserendo 1

        recv(sd,msg,100,0);//Ricezione messaggio se la risposta e' corretta o meno
        printf("%s\n",msg);

        recv(sd,msg,100,0); //Ricezione del messaggio che comunica se il client e' in attesa che gli altri finiscano o se e' stato l'ultimo a rispondere
        printf("%s",msg);

        send(sd,"invio nuovo giro",100,0);
        count++;
    }

    printf("Domande finite! Consulta la classifica per il tuo punteggio!\n");
    char classificaString[4096];
    recv(sd,classificaString,4096,0);
    write(STDOUT_FILENO,classificaString,strlen(classificaString));
    write(STDOUT_FILENO,"\n",strlen("\n"));
   
    
    write(STDOUT_FILENO,"\n",strlen("\n"));
    write(STDOUT_FILENO,nomeClient,strlen(nomeClient));
    write(STDOUT_FILENO," il tuo IP/CLIENT ADDRESS e' : ",strlen(" il tuo IP/CLIENT ADDRESS e' : "));
    write(STDOUT_FILENO,mioIndirizzo,strlen(mioIndirizzo));
    write(STDOUT_FILENO,"\n",strlen("\n"));
    
    printf("Partita conclusa.\nArrivederci!\n");
    sleep(1);
    return 0;
}

void intro(){
    printf("\t\tGara di Aritmetica : Client\n\n");
    
    printf("\t\t********ISTRUZIONI********\nTi sarà presentata una serie di domande d' aritmetica.\n");
    printf("Hai 60 secondi per rispondere ad ogni domanda.\n");

    printf("Se ti capitera' di unirti ad una partita in corso,\nper rispondere avrai i secondi restanti dal primo invio di quella domanda.\n\n");
    printf("VINCE chi fa' il MIGLIOR TEMPO MEDIO di risposte corrette alle domande.\n");
    printf("Se dovessi sbagliare tutte le risposte il tuo tempo sara' 99:99:99.\n");
    //printf("\nAd esempio se rispondi correttamente a 3 domande, la tua posizione in classifica\n");
    //printf("sara' determinata dalla somma di quei 3 tempi/3 domande corrette\n");

    printf("\nBenvenuto e buon divertimento!\n\n");
}

int controlloArg(int argc, char** argv){//Accetta o nessun arg oppure porta ed ip

    if((argc > 3 ) || (argc == 2)){ //0='./nomeProg', 1=indirizzoIP, 2=portaTCP -> 3 elementi
        printf("Numero argomenti non validi.\nUsage : %s <Indirizzo IP> <TCP Port>\n", argv[0]);
        exit (1);
    }
    else if (argc == 3){ //Set IP e porta TCP
    	int val1 = atoi(argv[2]);
    	int val2 = atoi(argv[2]);
        if( (val1<0) || (val2> 65535) ){//Controllo porta TCP valida u_int16_t
            printf("Numero di porta non consentito.\nScegli una porta tra 0 e 65535\nUsage : %s <TCP Port>\n", argv[0]);
            exit(2);
        }
        PORTA = atoi(argv[2]);
        if(strcmp(argv[1], "localhost")==0){
            strcpy(ip, "127.0.0.1");
        } else {
            strcpy(ip,argv[1]);
        }
    }
    else if(argc==1){ //Default
        //printf("Stai utilizzando la PORTA TCP di default: 7295 e IP di default: 127.0.0.1\n");
        PORTA = PORTA_TCP;
    }

    return 0;
}

int initSocketClient(int* socket_fd, struct sockaddr_in* serverAddress, int argc){
    int sd = *socket_fd;
    struct sockaddr_in serverAddr = *serverAddress;
    sd = socket(PF_INET , SOCK_STREAM , 0);
    if (sd < 0) perror("socket") , exit(1);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORTA);
    if(argc==1){ //ip di default
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        inet_aton(ip, &serverAddr.sin_addr);
    }
    
    *socket_fd = sd;
    *serverAddress = serverAddr;
    return 0;
}

void riceviDomandaDaSocket(int sd){ //Riceve la domanda dal socket sd e la memorizza nelle var. globali
    recv(sd ,testoD, 100,0);
    recv(sd , a , 100,0);
    recv(sd , b , 100,0);
    recv(sd , c , 100,0);
    recv(sd , d , 100,0);
}

void printDomanda(){
    printf("  %s\n",testoD);
    printf("  %s\n",a);
    printf("  %s\n",b);
    printf("  %s\n",c);
    printf("  %s\n",d);
}


void letturaConSceltaSelect(int sd, int sec, fd_set* readfds, char rispostaC[]) { //Legge da 'sd' entro un timeout di 'sec' e memorizza la risposta in 'rispostaC'
    char rispostaClient[20];
    struct timeval mioTimeout;
    fd_set r_fds = *readfds; 

    mioTimeout.tv_sec = sec;
    mioTimeout.tv_usec = 0;

    int count = 0;
    int exit = 0;
    int risultato;

    printf("Scegli tra a,b,c,d - 1 per uscire:\n");
    while (exit == 0) {
        fd_set r_fds1;
        FD_ZERO(&r_fds1); 
        FD_SET(sd, &r_fds1); 

        if (count == 1) {
            printf("Carattere non consentito, riprova.\n");
        }

        if ((risultato = select(sd + 1, &r_fds1, NULL, NULL, &mioTimeout)) < 0) {
            perror("errore select");
        } else { 

            if (FD_ISSET(sd, &r_fds1)) { 

                scanf("%s", rispostaClient);
                count = 1;
                FD_CLR(sd, &r_fds1);

                if( (strcmp(rispostaClient,"a\0")==0) || (strcmp(rispostaClient,"b\0")==0) || (strcmp(rispostaClient,"c\0")==0) || (strcmp(rispostaClient,"d\0")==0) || (strcmp(rispostaClient,"1\0")==0)){
                    exit = 1;
                }

            } else {
                printf("Timed out.\n");
                rispostaClient[0] = '0';
                rispostaClient[1] = '\0';
                exit = 1;
            }
        }

    }
    printf("\n");
    *readfds = r_fds;
    strcpy(rispostaC, rispostaClient);
}


void initTimeoutSelect(int sd, int sec, fd_set* readfds){
    struct timeval mioTimeout;
    fd_set r_fds = *readfds;  

    mioTimeout.tv_sec = sec;

    FD_ZERO(&r_fds); //set a 0
    FD_SET(sd,&r_fds); //add del fd al set

    select(sd+1,&r_fds,NULL,NULL,&mioTimeout);
    *readfds = r_fds;
}

void letturaConTimeout(int timeout,fd_set* readfds1, int* valoreLetto){ //Lettura con timeout di una stringa da stdin senza controllo
    fd_set readfds = *readfds1;
    FD_ZERO(&readfds);
    initTimeoutSelect(STDIN_FILENO,timeout, &readfds);
    if (FD_ISSET(STDIN_FILENO,&readfds) ){ 
        scanf("%d",valoreLetto);
    } else {
        printf("Avanzamento");
        *valoreLetto = 0;
    }
    *readfds1 = readfds;
}


void uscitaHandler(int segnale){ /*Handler per i segnali SIGINT, SIGTSTP e SIGPIPE*/
	if(segnale==SIGINT)
	printf("\nÈ stato inviato il segnale %d (di interruzione) da tastiera!\nChiusura in corso...\n", segnale);
	else if(segnale==SIGTSTP)
	printf("\nÈ stato inviato il segnale %d (di stop al processo) da tastiera!\nChiusura in corso...\n", segnale);
	else {
        printf("\n\n");
        printf("  ATTENZIONE:\n");
        printf("  Chiusura del programma a causa di un problema al server\n\n");
    }
    exit(1);
}

void checkUscita(int sd,char rispostaClient[]){
    if(strcmp(rispostaClient,"1\0")==0){ //Il client puo' abbandonare la partita inserendo 1
        printf("Grazie per aver giocato\nArrivederci!\n");
        sleep(1);
        exit(1);
    }
}
