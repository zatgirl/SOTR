#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int TEMPO = 8000000;
int newsockfd;
int x = 0, com[1];
int Luminosidade_atual, Brilho_intens = 0, Trafego_atual, Abertura_semaforo = 0;
  
void send_values(int val, char text[]){
    char valores[128], texto[128];
	bzero(valores, sizeof(valores));
	bzero(texto, sizeof(texto));
	sprintf(valores, "%d", val);
	sprintf(texto,"%s:%s", text, valores);
	int n = write(newsockfd,texto,50);
	if (n < 0) {
        	printf("Erro escrevendo no socket!\n");
                exit(1);
	}
}
//----- ANÁLISE DOS DADOS RECEBIDOS PELO CLIENTE -----
void *cliente(void *arg){
    int i;
    int n;
    char buffer[256];
    
    while(1){
    	bzero(buffer,sizeof(buffer));
     	n=read(newsockfd,buffer,50);
        x=1;
                if(strcmp(buffer, "T+\n")==0){
    		pthread_mutex_lock(&mutex);
		com[0] = 1;   //Diminui tempo de abertura do semaforo
		if(Abertura_semaforo == 3){
		    printf("\nTempo maximo atingido\n");
		}else{
    		    printf("\nTempo de abertura ajustado em: [%d]\n", Abertura_semaforo+com[0]);
		}
    		pthread_cond_signal(&cond);
    		pthread_mutex_unlock(&mutex);
    	}else if(strcmp(buffer, "T-\n")==0){
    		pthread_mutex_lock(&mutex);
    		com[0] = -1;   //Aumenta tempo de abertura de semaforo
		if(Abertura_semaforo == 0){
		    printf("\nTempo minimo atingido\n");
		}else{
    		    printf("\nTempo de abertura ajustado em: [%d]\n", Abertura_semaforo-com[0]);
		}
    		pthread_cond_signal(&cond);
    		pthread_mutex_unlock(&mutex);
    	}else{
            if(strlen(buffer)>0)
            	//printf("Recebeu: %s - %lu\n", buffer,strlen(buffer));
            if (n < 0) {
                printf("Erro lendo do socket!\n");
                exit(1);
            }
        }
    }
}
//----- FUNÇÕES INFO, MAKE E WAIT RETIRADAS DE UM REPOSITÓRIO NO GITHUB, LINK NA BIBLIOGRAFIA DO TRABALHO -----
struct periodic_info {
        int sig;
        sigset_t alarm_sig;
};
static int make_periodic(int unsigned period, struct periodic_info *info){
        static int next_sig;
        int ret;
        unsigned int ns;
        unsigned int sec;
        struct sigevent sigev;
        timer_t timer_id;
        struct itimerspec itval;

        /* Initialise next_sig first time through. We can't use static
           initialisation because SIGRTMIN is a function call, not a constant */
        if (next_sig == 0)
                next_sig = SIGRTMIN;
        /* Check that we have not run out of signals */
        if (next_sig > SIGRTMAX)
                return -1;
        info->sig = next_sig;
        next_sig++;
        /* Create the signal mask that will be used in wait_period */
        sigemptyset(&(info->alarm_sig));
        sigaddset(&(info->alarm_sig), info->sig);

        /* Create a timer that will generate the signal we have chosen */
        sigev.sigev_notify = SIGEV_SIGNAL;
        sigev.sigev_signo = info->sig;
        sigev.sigev_value.sival_ptr = (void *)&timer_id;
        ret = timer_create(CLOCK_MONOTONIC, &sigev, &timer_id);
        if (ret == -1)
        return ret;

        /* Make the timer periodic */
        sec = period / 1000000;
        ns = (period - (sec * 1000000)) * 1000;
        itval.it_interval.tv_sec = sec;
        itval.it_interval.tv_nsec = ns;
        itval.it_value.tv_sec = sec;
        itval.it_value.tv_nsec = ns;
        ret = timer_settime(timer_id, 0, &itval, NULL);
        return ret;
}
static void wait_period(struct periodic_info *info){
        int sig;
        sigwait(&(info->alarm_sig), &sig);
}
// ----- SENSORES DE LUMINOSIDADE E TRÁFEGO -----
static void *le_sensor(void *arg){
        struct periodic_info info;
        make_periodic(TEMPO, &info);
        while(1){
                pthread_mutex_lock(&mutex);
                Luminosidade_atual = 0+rand()%1024;           
                Trafego_atual = 0+rand()%100;
                pthread_mutex_unlock(&mutex);
                wait_period(&info);
        }       return NULL;
}
// ----- CONTROLE DO BRILHO CONFORME A INCIDÊNCIA DE LUZ RECEBIDA PELO SENSOR -----      
static void *le_luminosidade(void *arg){
        struct periodic_info info;
        make_periodic(TEMPO, &info);
        while (1) {
		pthread_mutex_lock(&mutex);
                if(Luminosidade_atual < 100){
                        Brilho_intens = 1;
                }
                if((Luminosidade_atual > 100) && (Luminosidade_atual < 300)){
                        Brilho_intens = 2; 
                }
		if((Luminosidade_atual > 300) && (Luminosidade_atual < 350)){
                        Brilho_intens = 3; 
                }
		if((Luminosidade_atual > 350) && (Luminosidade_atual < 550)){
                        Brilho_intens = 4; 
                }
		if((Luminosidade_atual > 550) && (Luminosidade_atual < 770)){
                        Brilho_intens = 5; 
                }
		if((Luminosidade_atual > 770) && (Luminosidade_atual < 940)){
                        Brilho_intens = 6; 
                }
		if((Luminosidade_atual > 940) && (Luminosidade_atual < 960)){
                        Brilho_intens = 7; 
                }                        
		if((Luminosidade_atual > 960) && (Luminosidade_atual < 990)){
                        Brilho_intens = 8; 
                }
		if((Luminosidade_atual > 990) && (Luminosidade_atual < 1024)){
                        Brilho_intens = 9; 
                }        
		pthread_mutex_unlock(&mutex);
                wait_period(&info);
        }       return NULL;
}
// ----- CONTROLE DA ABERTURA DO SEMÁFORO CONFORME A QUANTIDADE DE TRAFEGO RECEBIDA PELO SENSOR -----     
static void *le_trafego(void *arg){
        struct periodic_info info;
        make_periodic(TEMPO, &info);
     	while (1) {
		pthread_mutex_lock(&mutex);
                if((Trafego_atual > 0) && (Trafego_atual <40)){
                        Abertura_semaforo = 1; 
                        //printf("\nTempo de abertura do semaforo ajustado em %d minutos, trafego leve", Abertura_semaforo);
                }
                if((Trafego_atual > 40) && (Trafego_atual < 70)){
                        Abertura_semaforo = 2; 
                        //printf("\nTempo de abertura do semaforo ajustado em %d minutos, trafego moderado", Abertura_semaforo);
                }
				if((Trafego_atual > 70) && (Trafego_atual < 100)){
                        Abertura_semaforo = 3; 
                        //printf("\nTempo de abertura do semaforo ajustado em %d minutos, trafego intenso", Abertura_semaforo);
                }
	        //---- CONTROLE MANUAL ----
		if(com[0] == -1){
			Abertura_semaforo --;
		}
		if(com[0] == 0){
		        Abertura_semaforo = Abertura_semaforo;
		}
		if(com[0] == 1){
			Abertura_semaforo ++;
		}
		pthread_mutex_unlock(&mutex);
                wait_period(&info);
        }       return NULL;
}
// ----- RECEPÇÃO DOS DADOS E EXIBIÇÃO PERIÓDICA DOS DADOS A CADA 8 SEGUNDOS ----- 
static void *le_status(void *arg){
        struct periodic_info info;

        make_periodic(TEMPO, &info);
     	while (1) {
		pthread_mutex_lock(&mutex);

		printf("\n------------------------------------STATUS-------------------------------------------");
                printf("\nLuminosidade atual: %d", Luminosidade_atual);
                printf("\nTrafego atual: %d", Trafego_atual);
	 	printf("\nBrilho do semaforo ajustado em %d %%, pois a luminosidade esta em %d", Brilho_intens*10, Luminosidade_atual);
		printf("\nTempo de abertura do semaforo ajustado em %d minutos de acordo com o fluxo do trafego", Abertura_semaforo);
		printf("\n-------------------------------------------------------------------------------------\n");
		pthread_mutex_unlock(&mutex);
                wait_period(&info);
        }
        return NULL;
}
// ----- CONEXÃO COM O SOCKET E CRIAÇÃO DAS THREADS ----- 
int main(int argc, char *argv[]) {
    
    pthread_t t, sensor, luminosidade, trafego, status;
    sigset_t alarm_sig;

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    
    int portno, sockfd;
    com[0] = 0;//ajuste manual semaforo
    sigemptyset(&alarm_sig);
    for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
        sigaddset(&alarm_sig, i);
    sigprocmask(SIG_BLOCK, &alarm_sig, NULL);

    if (argc < 2) {
        printf("Erro, porta nao definida!\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Erro abrindo o socket!\n");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("Erro fazendo bind!\n");
        exit(1);
    }
    listen(sockfd,1);

    pthread_create(&sensor, NULL, le_sensor, NULL);
    pthread_create(&luminosidade, NULL, le_luminosidade, NULL);
    pthread_create(&trafego, NULL, le_trafego, NULL);
    pthread_create(&status, NULL, le_status, NULL);

    while (1) {     

        newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
	
	pthread_mutex_lock(&mutex);
	if (newsockfd < 0) {
            printf("Erro no accept!\n");
            exit(1);
	}
       	 pthread_create(&t, NULL, cliente, NULL);       
       	 pthread_mutex_unlock(&mutex);   
    }
    return 0; 
}
