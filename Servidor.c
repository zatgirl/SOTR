#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define TOTAL 5
#define TRUE 1
#define TAM 150       //estoque
#define M 3



pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;				//inicializa o mutex
pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;			       //inicializa a variável de condição

struct nodo {
    int newsockfd;
};									     //cria um socket específico para cada cliente
struct nodo nodo[5];
//declaração de variáveis
double quant = 0, num;
char buffer[256], str[256];

char *comando;
int valor[M];


void *cliente(void *arg) {
    long cid = (long)arg;
    int i, n, x;
    while (TRUE) {
        bzero(buffer,sizeof(buffer));					   //zera o valor do buffer	
        n = read(nodo[cid].newsockfd,buffer,50);			  //lê o valor digitado pelo cliente
        if(strcmp(buffer, "estoque")==10){                              //verifica se o usuario digitou o comando correto
        	printf("A Maquina tem %lf bolinhas em estoque\n", quant);//
        }	
	num = strtod(buffer, &comando);      				 //buffer = comando + num ---> num deve receber a parte numerica
	valor[M] = strtod(buffer, &comando);  
	strcpy(str,comando);						 //passa os caracteres para uma string			
        if (n <= 0) {						 	 //se n<=0 causa erro na conexao
            printf("Erro lendo do socket id %lu!\n", cid);
            close(nodo[cid].newsockfd);
            nodo[cid].newsockfd = -1;

            pthread_exit(NULL);
        }
        // MUTEX LOCK - GERAL
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);	//				//libera as threads em espera
       
        for (i = 0;i < TOTAL; i++) {
            if ((i != cid) && (nodo[i].newsockfd != -1)) {
                n = write(nodo[i].newsockfd,buffer,50);
                if (n <= 0) {							//se n<=0 causa erro
                    printf("Erro escrevendo no socket id %i!\n", i);
//                    close(nodo[i].newsockfd);
//                    nodo[i].newsockfd = -1;
                }
            }
        }
        pthread_mutex_unlock(&mutex);
        // MUTEX UNLOCK - GERAL
    }
}

void *Comprar(){
    while(TRUE){
    	if(strcmp(str, " comprar" )==10){				// verifica se str é compativel com o comando
	    	pthread_mutex_lock(&mutex);				//sinaliza o acesso exclusivo protegendo a região critica
	    	printf("Digite o valor das moedas (R$2,50)\n");
		int i, m;
			for (i=0;i<2;i++){
	    		m = valor[i];
			}
			if(num>quant){						//verifica se o valor digitado não vai retirar demais
	    		double c = (TAM - quant);			// atualiza c com o valor restante
	    		printf("Erro: Nao tem estoque\n");	  //printa na tela do sistema	
			pthread_cond_wait(&cond, &mutex);		//coloca a thread em espera
		    	pthread_mutex_unlock(&mutex);			//libera o mutex lock
			}
			else{								
		    	quant = quant - num;						// quant recebe o valor atualizado
		    printf("Bolinha saiu!\n");
			printf("Agora a maquina possui %lf bolinhas em estoque\n", quant);
			
		    	pthread_cond_wait(&cond, &mutex);                    	//coloca a thread em espera
		    	pthread_mutex_unlock(&mutex);					//libera o mutex lock
			}	
		}
	}
}

void *Repor() {
    while (TRUE){
    	if (strcmp(str, " repor")==10){                                   //verifica se str é compativel com o comando
	    	pthread_mutex_lock(&mutex);					//sinaliza o acesso exclusivo protegendo a região critica
	    	if(quant+num>TAM){					       //verifica se o valor digitado não supera o valor limite
	    		double c = (TAM - quant);			      //atualiza c com o valor de espaço livre
	    		printf("O valor a ser adicionado supera a capacidade maxima\n");
			pthread_cond_wait(&cond, &mutex);			//coloca a thread em espera
		    	pthread_mutex_unlock(&mutex);				//libera o mutex lock

			}
		else{
			quant = quant + num;						// quant recebe o valor atualizado de liquido
			double c = (TAM - quant);					//atualiza c com o valor de espaço livre
			printf("Valor adicionado com sucesso\n");
			printf("Agora a Maquinha possui %lf bolinhas, faltando %lf L para estar completo\n", quant, c);	
			pthread_cond_wait(&cond, &mutex);					 //coloca a thread em espera
	       	pthread_mutex_unlock(&mutex);						//libera o mutex lock
    }	}
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int sockfd, portno;
    pthread_t comprar, repor, estoque;

    long i;

    if (argc < 2) {										//verifica possíveis erros
        printf("Erro, porta nao definida!\n");
        printf("./SOFTWARE PORTA");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Erro abrindo o socket!\n");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));                                            //parte responsável pelo
    portno = atoi(argv[1]);                                                                  //endereço do servidor   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {		//verifica possíveis erros
        printf("Erro fazendo bind!\n");
        exit(1);
    }
    listen(sockfd,5);

    for (i = 0; i < TOTAL; i++) {
      nodo[i].newsockfd = -1;
    }
    while (1) {
        for (i = 0; i < TOTAL; i++) {
          if (nodo[i].newsockfd == -1) break;
        }
        nodo[i].newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        // MUTEX LOCK - GERAL
        pthread_mutex_lock(&mutex);					
        pthread_create(&comprar, NULL, Comprar, NULL);//		//cria as threads
    	pthread_create(&repor, NULL, Repor, NULL);//
    	//pthread_create(&estoque, NULL, Estoque, NULL);//
        if (nodo[i].newsockfd < 0) {
            printf("Erro no accept!\n");
            exit(1);
        }
        //pthread_create(&t, NULL, cliente, (void *)i);
        pthread_mutex_unlock(&mutex);
        // MUTEX UNLOCK - GERAL
    }
    pthread_join(comprar,NULL);
    pthread_join(repor,NULL);                         //join
    pthread_join(estoque,NULL);
    
    //    close(sockfd);
    return 0; 
}
