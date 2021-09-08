#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define total 150 

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;				
pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;			       

struct nodo {
    int newsockfd;
};									     
struct nodo nodo[5];
//declaração de variáveis
double quant = 0, j = 0, rest;
char buffer[256], str[256];

char *comando;
int valor, n;

void *cliente(void *arg) {
    long cid = (long)arg;
    int i;
    while (1) {
        bzero(buffer,sizeof(buffer));					   	
        n = read(nodo[cid].newsockfd,buffer,50);			  
        if(strcmp(buffer, " estoque")==10){                               
        	printf("A Maquina tem %lf bolinhas em estoque\n", quant);
        }	
	//j = strtod(buffer, &comando);      	
	//strcpy(str,comando);	
	valor = atoi(buffer);					 			
        if (n <= 0) {						 	 
            printf("Erro lendo do socket id %lu!\n", cid);
            close(nodo[cid].newsockfd);
            nodo[cid].newsockfd = -1;

            pthread_exit(NULL);
        }
        // MUTEX LOCK - GERAL
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);					
       
        for (i = 0;i < 5; i++) {
            if ((i != cid) && (nodo[i].newsockfd != -1)) {
                n = write(nodo[i].newsockfd,buffer,50);
                if (n <= 0) {							
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
    while(1){
    	if(valor == 250){				
	    	pthread_mutex_lock(&mutex);				
		int i;
			if(j>quant){						
	    		double rest = (total - quant);			
	    		printf("Erro: Sem estoque\n");	  	
			pthread_cond_wait(&cond, &mutex);		
		    	pthread_mutex_unlock(&mutex);			
			}
			else{								
		    	quant = quant - j;						
		    printf("Bolinha saiu!\n");
			printf("Agora a maquina possui %lf bolinhas em estoque\n", quant);
			
		    	pthread_cond_wait(&cond, &mutex);                    	
		    	pthread_mutex_unlock(&mutex);					
			}	
		}
	}
}

void *Repor() {
    while (1){
    	if (strcmp(str, " repor")==10){                                   
	    	pthread_mutex_lock(&mutex);					
	    	if(quant+j>total){					       
	    		double rest = (total - quant);			      
	    		printf("O valor a ser adicionado supera a capacidade maxima\n");
				pthread_cond_wait(&cond, &mutex);			
		    	pthread_mutex_unlock(&mutex);				
			}else{
			quant = quant + j;				
			printf("Agora a Maquinha possui %lf bolinhas\n", quant);	
			pthread_cond_wait(&cond, &mutex);					 
	       	pthread_mutex_unlock(&mutex);						
    		}	
		}
    }
}

/*void *Estoque(){
	while (1) {
        bzero(buffer,sizeof(buffer));					   	
        n = read(nodo[cid].newsockfd,buffer,50);			  
        if(strcmp(buffer, "estoque")==10){                               
        	printf("A Maquina tem %lf bolinhas em estoque\n", quant);
        }	
	}
}*/

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int sockfd, portno;
    pthread_t comprar, repor, estoque;

    long i;

    if (argc < 2) {									
        printf("Erro, porta nao definida!\n");
        printf("./SOFTWARE PORTA");
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
    listen(sockfd,5);

    for (i = 0; i < 5; i++) {
      nodo[i].newsockfd = -1;
    }
    while (1) {
        for (i = 0; i < 5; i++) {
          if (nodo[i].newsockfd == -1) break;
        }
        nodo[i].newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        // MUTEX LOCK - GERAL
        pthread_mutex_lock(&mutex);					
        pthread_create(&comprar, NULL, Comprar, NULL);
    	pthread_create(&repor, NULL, Repor, NULL);
    	//pthread_create(&estoque, NULL, Estoque, NULL);
        if (nodo[i].newsockfd < 0) {
            printf("Erro no accept!\n");
            exit(1);
        }
        //pthread_create(&t, NULL, cliente, (void *)i);
        pthread_mutex_unlock(&mutex);
        // MUTEX UNLOCK - GERAL
    }
    pthread_join(comprar,NULL);
    pthread_join(repor,NULL);                         
    pthread_join(estoque,NULL);
    
    //    close(sockfd);
    return 0; 
}
