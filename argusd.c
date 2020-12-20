#include "argus.h"

int* childpids; // Pids dos processos filho
int nPids; // Numero de pids em childpids
int tempomaxexec; // Variável de controlo do tempo máximo de execução
int maxPipeTime; // Variável de controlo do tempo máximo de inatividade de pipes
char** tarefasExec; // Guarda comando da tarefa
int* pidsExec; // Array de pids em execução
int* nTarefasExec; // Array que guarda o numero das tarefas
int used; // Numero de pids no array
int tam; // Tamanho do array
int fd_pipePro[2]; // Pipe entre processo principal e cada processo filho
int nTarefa; // Número da próxima tarefa
int statusID; // Identificador de tipo estado
int actualStatus; // Estado atual do processo 

void alrm_hand(int signum) {
    actualStatus = statusID;
    // Envio do sinal para os processos filhos para o caso de haver processos netos e matar todos os processos relativos
    for(int x = 0; x < nPids; x++) {
        kill(childpids[x],SIGALRM);
    }
    wait(NULL);
    // Terminação dos processos filho
    for(int x = 0; x < nPids; x++) {
        if(childpids[x] != -1)
            kill(childpids[x],SIGKILL);
    }
}

// Sinal que mata os filhos recursivamente e depois mata-se a si próprio
void sigusr2_handler(int signum) {
    for(int x = 0; x < nPids; x++) {
        if(childpids[x] != -1)
            kill(childpids[x],SIGUSR2);
    }
    wait(NULL);
    kill(getpid(),SIGKILL);
}

void sigusr1_handler(int signum) {
    int pidusr, status, x, fdterminadas, numTarefa,flag=0;
    char* buf, *command;
    buf = malloc(200 * sizeof(char));
    // Leitura do tipo de saída do processo filho e do pid do mesmo
    read(fd_pipePro[0],&status,sizeof(int));
    pidusr = wait(NULL);
    // Extração de informações acerca do processo que o terminou e remoção do mesmo dos processos em execução
    for(x = 0; x < used && flag !=1 ; x++) {
        if(pidsExec[x] == pidusr) {
            pidsExec[x] = -1;
            command = strdup(tarefasExec[x]);
            free(tarefasExec[x]);
            numTarefa = nTarefasExec[x];
            nTarefasExec[x] = -1;
            flag=1;
        }
    }
    if((fdterminadas = open("../SO/TarefasTerminadas.txt",O_WRONLY | O_CREAT | O_APPEND, 0666)) < 0) {
        perror("File not found");
    }
    // Escrita no ficheiro de historico
    else {
        if(status == 0) {
            sprintf(buf,"#%d, concluida: %s",numTarefa,command);
            write(fdterminadas,buf,strlen(buf));
        }
        else if(status == 2) {
            sprintf(buf,"#%d, max inatividade: %s",numTarefa,command);
            write(fdterminadas,buf,strlen(buf));
        }
        else if(status == 1) {
            sprintf(buf,"#%d, max execucao: %s",numTarefa,command);
            write(fdterminadas,buf,strlen(buf));
        }
    }
    close(fdterminadas);
    free(buf);
}

int main(int argc, char const *argv[]) {
    int fdfifo, fdfile, wrfifo, fileTarefa;
    char * buf, *option;
    // Abrir ficheiro da tarefa para saber o número da ultima tarefa da ultima seesão
    if((fileTarefa = open("../SO/fileTarefa.txt",O_RDWR | O_CREAT, 0666)) < 0) {
        perror("File not found");
        exit(1);
    }
    // Leitura do número da tarefa seguinte
    int currentTarefa = 0;
    char* linhaTarefa = malloc(10*sizeof(char));
    read(fileTarefa,linhaTarefa,10);
    currentTarefa = atoi(linhaTarefa);
    nTarefa=currentTarefa;
    free(linhaTarefa);
    // Inicialização de variáveis
    if((fdfile = open("../SO/logs.txt",O_RDWR | O_APPEND | O_CREAT, 0666)) < 0) {
        perror("File not found");
        exit(1);
    }
    pipe(fd_pipePro);
    tempomaxexec = -1;
    maxPipeTime = -1;
    tam = 1;
    used=0;
    childpids = malloc(10 * sizeof(int));
    nPids = 0;
    tarefasExec = malloc(sizeof(char*));
    nTarefasExec = malloc(sizeof(int));
    pidsExec = malloc(sizeof(int));
    for(int d = 0; d < tam; d++) {
        nTarefasExec[d] = -1;
        pidsExec[d] = -1;
    }
    option = malloc(25 * sizeof(char));
    buf = malloc(100 * sizeof(char));
    // Criação de manuseadores de sinais
    if(signal(SIGALRM,alrm_hand) == SIG_ERR) {
        perror("Signal");
        exit(1);
    }
    if(signal(SIGUSR1,sigusr1_handler) == SIG_ERR) {
        perror("Signal");
        exit(1);
    }
    if(signal(SIGUSR2,sigusr2_handler) == SIG_ERR) {
        perror("Signal");
        exit(1);
    }
    // Abertura do pipe de comunicação Cliente -> Servidor
    fdfifo = open("../SO/fifo",O_RDONLY);
    while(fdfifo > 0) {
        int readBytes = 0;
        // Leitura input do cliente
        while((readBytes = read(fdfifo,buf,100)) > 0) {
            // Abertura do pipe de comunicação Servidor -> Cliente
            wrfifo = open("../SO/wr",O_WRONLY);
            // Separação da string de input por opção e argumento
            buf = mySep(option,buf,' ');
            // Verificação de qual ação a fazer
            if(strcmp(option,"-i") == 0 || strcmp(option,"tempo-inactividade") == 0) {
                maxPipeTime = atoi(buf);
                write(wrfifo,"Novo tempo máximo de inatividade\n",35);
            }
            else if(strcmp(option,"-m") == 0 || strcmp(option,"tempo-execucao") == 0) {
                tempomaxexec = atoi(buf);
                write(wrfifo,"Novo tempo máximo de execucao\n",32);
            }
            else if(strcmp(option,"-e") == 0 || strcmp(option,"executar") == 0) {
                executar(buf);
                char * temp = malloc(25 * sizeof(char));
                sprintf(temp,"Nova tarefa: %d\n",nTarefa - 1);
                write(wrfifo,temp,strlen(temp));
                close(wrfifo);
            }
            else if(strcmp(option,"-l") == 0 || strcmp(option,"listar") == 0) {
                for(int n = 0; n<used; n++) {
                    if(pidsExec[n]!=-1) {
                        char* s = malloc(100*sizeof(char));
                        sprintf(s, "#%i: %s \n",nTarefasExec[n], tarefasExec[n]);
                        write(wrfifo, s, strlen(s));
                    }
                }
            }
            else if(strcmp(option,"-t") == 0 || strcmp(option,"terminar") == 0) {
                int r = terminarTarefa(buf);
                if(r==0)  write(wrfifo, "Tarefa terminada\n", 18);
                else if (r==-1) write(wrfifo, "Não é possível terminar a tarefa\n", 37);
                else write(wrfifo, "Tarefa não está em execução\n", 33);
            }
            else if(strcmp(option,"-r") == 0 || strcmp(option,"historico") == 0) {
                histTerm(wrfifo);
            }
            else if(strcmp(option,"-h") == 0 || strcmp(option,"ajuda") == 0) {
                write(wrfifo," tempo-inatividade segs  ou  -i segs\n tempo-execucao segs  ou  -m segs\n executar p1 | p2 ... | pn  ou  -e 'p1 | p2 ... | pn'\n listar  ou  -l\n terminar n  ou  -t n\n historico  ou  -r\n ajuda  ou  -h\n output n  ou  -o n\nsair\n",223);
            }
            else if(strcmp(option,"-o") == 0 || strcmp(option,"output") == 0) {
                output(atoi(buf),fdfile,wrfifo);
            }
            close(wrfifo);
        }
    }
    // Trecho de código caso a abertura do canal de comunicação Cliente -> Servidor falhe
    if(fdfifo < 0) {
        perror("Negative fd");
    }
    close(fileTarefa);  
    close(fdfifo);
    close(fdfile);
    return 0;
}