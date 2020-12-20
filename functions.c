#include "argus.h"

extern int* childpids; // Pids dos processos filho
extern int nPids; // Numero de pids em childpids
extern int tempomaxexec; // Variável de controlo do tempo máximo de execução
extern int maxPipeTime; // Variável de controlo do tempo máximo de inatividade de pipes
extern char** tarefasExec; // Guarda comando da tarefa
extern int* pidsExec; // Array de pids em execução
extern int* nTarefasExec; // Array que guarda o numero das tarefas
extern int used; // Numero de pids no array
extern int tam; // Tamanho do array
extern int fd_pipePro[2]; // Pipe entre processo principal e cada processo filho
extern int nTarefa; // Número da próxima tarefa
extern int statusID; // Identificador de tipo erro
extern int actualStatus; // Estado atual do processo 

// Função que imprime o output da tarefa desejada
int output(int n,int logs,int wr){
    int idx = open("log.idx",O_RDONLY);
    char* index = malloc(1000*sizeof(char));
    char* buffer = malloc(1000*sizeof(char));
    int readIdx = read(idx,index,1000);
    char* nTarefa1 = malloc(5*sizeof(char));
    char* indInicial1 = malloc(5*sizeof(char));
    char* nTarefa2 = malloc(5*sizeof(char));
    char* indInicial2 = malloc(5*sizeof(char));
    int fl=0,ofs=0,readLogs=0;
    while( readIdx > 0 && fl!=1){
        index = mySep(indInicial1,index,'\n');
        indInicial1 = mySep(nTarefa1,indInicial1,' ');
        if(atoi(nTarefa1)==n){
            index = mySep(indInicial2,index,'\n');
            indInicial2 = mySep(nTarefa2,indInicial2,' ');
            if(nTarefa2!=NULL && atoi(nTarefa2)==n+1){
                int dif = atoi(indInicial2)-atoi(indInicial1);
                lseek(logs,atoi(indInicial1),SEEK_SET);
                readLogs =read(logs,buffer,dif);
                if(readLogs!=0) write(wr,buffer,dif);
                fl=1;   
            }
            else{
                lseek(logs,atoi(indInicial1),SEEK_SET);
                readLogs = read(logs,buffer,1000);
                if(readLogs == 0) fl = 1;
                if(readLogs>0){
                    write(wr,buffer,readLogs);
                    fl=1;
                }
            } 
        }
        if(strlen(index)==0)  fl=1;
    }
    
    return 0;
}

// Envia os conteúdos do ficheiro historico para o cliente
void histTerm(int fd){
    int tarefas;
    char buf[100];
    int readBytes = 0;
    tarefas = open("../SO/TarefasTerminadas.txt",O_RDONLY);
    while((readBytes = read(tarefas,buf,100)) > 0) {
        write(fd,buf,readBytes);
    }
    write(fd,"\n",1);
    close(tarefas);
}

// Separa uma string por um delimitador ou '\n', sendo o primeiro token guardado na variavel tok e o resto da string é devolvida
char* mySep(char* tok, char *buf, char delim) {
    int i;
    for(i = 0; buf[i]!=delim && buf[i] != '\n' && buf[i]; i++) {
        tok[i] = buf[i];
    }
    tok[i] = '\0';
    return buf+i+1;
}

int executar(char * buf) {
    int filho = -1;
    int logs, idx;
    int indInicial;
    char* inicial;
    char* tar;
    // Criação de um processo por cada tarefa requisitada
    if((filho=fork()) == 0) {
        // Abertura de fd para escrita de output em ficheiros
        logs = open("logs.txt",O_WRONLY | O_CREAT | O_APPEND,0666);
        idx = open("log.idx",O_WRONLY | O_CREAT | O_APPEND,0666);
        // Escrita do indice de alocação do output de cada tarefa
        int nTarefaN= count(nTarefa)+1;
        tar = malloc(nTarefaN*sizeof(char));
        sprintf(tar,"%d ",nTarefa);
        write(idx,tar,nTarefaN);
        indInicial = lseek(logs,0, SEEK_END);
        int indInicialN= count(indInicial)+1;
        char* inicial = malloc(indInicialN*sizeof(char));
        sprintf(inicial,"%d\n",indInicial);
        write(idx,inicial,indInicialN);
        // Inicialização de variáveis de controlo e identificação
        statusID = 1;
        nPids = 0;
        char**ex, **line;
        line = malloc(10 * sizeof(char*));
        ex = malloc(10 * sizeof(char*));
        int indexEx = 0, indexLine = 0, nmrPipes = 0;
        // Partir comando por pipes
        line[indexLine++] = strtok(buf,"|");
        while((line[indexLine++] = strtok(NULL,"|\n")) != NULL);
        indexLine--;
        // Criação do numero de pipes necessários
        nmrPipes = indexLine - 2;
        int fd_pipe[nmrPipes+1][2];
        pipe(fd_pipe[0]);
        // Partir primeiro comando em espaços 
        ex[indexEx++] = strtok(line[0]," \n");
        while((ex[indexEx++] = strtok(NULL," \n")) != NULL);
        indexEx--;
        ex[indexEx]=NULL;
        // Executar comando sem pipes
        if(nmrPipes < 0) {
            if((childpids[(nPids)++] = fork()) == 0) {
                dup2(logs,1);
                execvp(ex[0],ex);
                _exit(1);
            }
            if(tempomaxexec > 0)
                alarm(tempomaxexec);
        }
        // Executar comando com apenas 1 fd_pipe[0]
        else if(nmrPipes == 0) {
            if((childpids[(nPids)++] = fork()) == 0) {
                actualStatus = 0;
                statusID = 2;
                nPids = 0;
                // Execução do primeiro comando reencaminhando o output para o pipe anonimo
                if((childpids[(nPids)] = fork()) == 0) {
                    dup2(fd_pipe[0][1],1);
                    close(fd_pipe[0][1]);
                    close(fd_pipe[0][0]);
                    execvp(ex[0],ex);
                    _exit(1);
                }
                (nPids)++;
                close(fd_pipe[0][1]);
                if(maxPipeTime > 0) alarm(maxPipeTime);
                // Partir o segundo comando
                indexEx = 0;
                ex[indexEx++] = strtok(line[1]," \n");
                while((ex[indexEx++] = strtok(NULL," \n")) != NULL);
                indexEx--;
                ex[indexEx]=NULL;
                // Execução do ultimo comando
                if((childpids[(nPids)] = fork()) == 0) {
                    dup2(fd_pipe[0][0],0);
                    close(fd_pipe[0][0]);
                    dup2(logs,1);
                    close(logs);
                    execvp(ex[0],ex);
                    _exit(1);
                }
                nPids++;
                close(fd_pipe[0][0]);
                wait(NULL);
                _exit(actualStatus);
            }
            if(tempomaxexec > 0)
                alarm(tempomaxexec);
        }
        // Caso haja 2 ou mais pipes
        else if(nmrPipes > 0) {
            if((childpids[(nPids)++] = fork()) == 0) {
                actualStatus = 0;
                statusID = 2;
                nPids = 0;
                int pipenmr;
                // Execução do primeiro comando
                if((childpids[(nPids)++] = fork()) == 0) {
                    dup2(fd_pipe[0][1],1);
                    close(fd_pipe[0][1]);
                    execvp(ex[0],ex);
                    _exit(1);
                }
                close(fd_pipe[0][1]);
                // Execução das n - 1 ultimos comandos, todos encaminnhados por pipes anonimos
                for(pipenmr = 0; pipenmr < nmrPipes; pipenmr++) {
                    pipe(fd_pipe[pipenmr+1]);
                    // Partir o comando
                    indexEx = 0;
                    ex[indexEx++] = strtok(line[pipenmr+1]," \n");
                    while((ex[indexEx++] = strtok(NULL," \n")) != NULL);
                    indexEx--;
                    ex[indexEx]=NULL;
                    // Executar o comando
                    if((childpids[(nPids)++] = fork()) == 0) {
                        dup2(fd_pipe[pipenmr][0],0);
                        dup2(fd_pipe[pipenmr+1][1],1);
                        close(fd_pipe[pipenmr+1][1]);
                        close(fd_pipe[pipenmr][0]);
                        execvp(ex[0],ex);
                        _exit(1);
                    }
                    close(fd_pipe[pipenmr][0]);
                    close(fd_pipe[pipenmr+1][1]);
                }
                if(maxPipeTime > 0) alarm(maxPipeTime);
                close(fd_pipe[pipenmr][1]);
                indexEx = 0;
                ex[indexEx++] = strtok(line[pipenmr+1]," \n");
                while((ex[indexEx++] = strtok(NULL," \n")) != NULL);
                indexEx--;
                ex[indexEx] = NULL;
                // Execução do ultimo comando encaminhando o output para o ficheiro log
                if((childpids[(nPids)++] = fork()) == 0) {
                    dup2(fd_pipe[pipenmr][0],0);
                    close(fd_pipe[pipenmr][0]);
                    dup2(logs,1);
                    close(logs);
                    execvp(ex[0],ex);
                    _exit(1);
                }
                wait(NULL);
                close(fd_pipe[pipenmr][0]);
                _exit(actualStatus);
            }
            if(tempomaxexec > 0)
                alarm(tempomaxexec);
        }
        // Recebe o estado de saída
        int status;
        wait(&status);
        status = WEXITSTATUS(status);
        // Caso tenha havido interrupção por tempo de execução
        if(status != 2 && actualStatus != 0) status = 1;
        if(status == 0 && actualStatus != 0) status = 1;
        // Envia o estado para o processo pai
        write(fd_pipePro[1],&status,sizeof(int));
        // Sinaliza que terminou
        kill(getppid(),SIGUSR1);
        actualStatus = status;
        close(logs);
        close(idx);
        _exit(actualStatus);
    }
    adicionarTarefa(filho, buf); 
    return 0;
}

// Insere o número da tarefa no array de tarefas em execução e escreve no ficheiro
// Insere o número do pid da tarefa no array de pids de tarefas
// Insere o comando da tarefa no array de tarefas
void adicionarTarefa(int filho, char* buf){
    if(used==tam){
        nTarefasExec = realloc(nTarefasExec, 2*tam*sizeof(int));
        tarefasExec = realloc(tarefasExec, 2*tam*sizeof(char*));
        pidsExec = realloc(pidsExec, 2*tam*sizeof(char*));
        tam *= 2;
        for(int k = used; k<tam; k++){
            pidsExec[k] = -1;
        }
    }
    int f=0; 
    for(int k = 0; k<tam && f==0; k++){
        if(pidsExec[k]==-1){
            nTarefasExec[k] = nTarefa;
            tarefasExec[k] =strdup(buf);
            pidsExec[k] = filho;
            nTarefa++;
            used++;
            f=1; 
        }
    }
    int fileTarefa;
    if((fileTarefa = open("../SO/fileTarefa.txt",O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
        perror("File not found");
        exit(1);
    }
    int countTarefa=count(nTarefa);
    char* tarefaNumero = malloc(countTarefa*sizeof(char));
    sprintf(tarefaNumero,"%d",nTarefa);
    lseek(fileTarefa, 0, SEEK_SET);
    write(fileTarefa,tarefaNumero,countTarefa);
    close(fileTarefa);     
}

// Mata o filho responsável por uma dada tarefa
int terminarTarefa(char*command){
    int i,k = 1,flag=0;
    int tarefasTerminadas;
    if((tarefasTerminadas = open("../SO/TarefasTerminadas.txt",O_WRONLY | O_CREAT | O_APPEND,0666)) < 0) {
        perror("File not found");
        exit(1);
    }
    int n = atoi(command);
    for(i=0; i<used; i++){
        if(nTarefasExec[i]==n && flag!=1){
            if(pidsExec[i]!=-1){
                // Matar tarefa
                k = kill(pidsExec[i],SIGUSR2);
                // Copiar para ficheiro de terminadas
                char* s =  malloc(100*sizeof(char*));
                sprintf(s, "#%i, Interrompida: %s", n, tarefasExec[i]);
                write(tarefasTerminadas, s, strlen(s));
                pidsExec[i] = -1;
                flag=1;
                wait(NULL);
            }
        }
 	}
    close(tarefasTerminadas);
    return k;
}


// Conta o número de dígitos num inteiro, por exemplo, count(123) = 3
int count(int numero){
    int n = 0;
    if(numero==0) return 1;
    while(numero!=0){
        numero/=10;
        n++;
    }
    return n;
}