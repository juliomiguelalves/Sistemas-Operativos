#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Separa a string por espaços e filtra ' e " , atualizando o numero de bytes
char* separateString(char * tok, char* buf, int * b) {
    int i, j = 0;
    char * new;
    new = malloc(100 * sizeof(char));
    for(i = 0; buf[i]!=' ' && buf[i] != '\n' && buf[i]; i++) {
        tok[i] = buf[i];
    }
    tok[i] = '\0';
    for(i = 0; buf[i]; i++) {
        if(buf[i] != '\'' && buf[i] != '\"')
            new[j++] = buf[i];
        else (*b)--;
    }
    return new;
}

int compareNString( char* buffer, char* string,int n){
    int i,ret=0;
    for(i=0;i<n;i++){
       if(buffer[i]!=string[i]) ret=1; 
    }

    return ret;
}


// Interpretador de comandos contínuo (shell)
void shellInterpreter(int fdToServer) {
    char* option, *buf;
    int bytesRead;
    int flag = 1;
    buf = malloc(100 * sizeof(char));
    option = malloc(25 * sizeof(char));
    while((bytesRead = read(0,buf,100)) > 0) {
        if(compareNString(buf,"sair\n",5)== 0) break;
        if(fork() == 0) {
            flag = 1;
            buf = separateString(option,buf,&bytesRead);
            // Validação do comando lido
            if( strcmp(option,"tempo-inactividade") != 0 && 
                strcmp(option,"tempo-execucao") != 0 && 
                strcmp(option,"executar") != 0 && 
                strcmp(option,"listar") != 0 &&
                strcmp(option,"terminar") != 0 && 
                strcmp(option,"historico") != 0 && 
                strcmp(option,"ajuda") != 0 && 
                strcmp(option,"output") != 0) {
                    flag = 0;
                    write(1,"Comando Inválido\n",19);
                }
            // Envia o comando para o servidor para o interpretar
            else if(write(fdToServer,buf,bytesRead) < 0) {
                perror("Fifo");
                exit(1);
            }
            if(flag) {
                int fdFromServer;
                fdFromServer = open("../SO/wr",O_RDONLY);
                while((bytesRead = read(fdFromServer,buf,500)) > 0) {
                    if(write(1,buf,bytesRead) < 0) {
                        perror("Write"),
                        exit(1);
                    }
                }
                close(fdFromServer);
            }
            _exit(0);
        }
        wait(NULL);        
    }
    free(buf); 
    free(option);
}

// Interpretador de comandos pela linha de comandos
void commandInterpreter(int fdToServer, char argv1[], char argv2[]) {
    char *args=malloc(150*sizeof(char));
    int bytesRead;
    int flag = 1;
    sprintf(args,"%s %s",argv1,argv2);
    if( strcmp(argv1,"-i") != 0 && 
        strcmp(argv1,"-m") != 0 && 
        strcmp(argv1,"-e") != 0 && 
        strcmp(argv1,"-l") != 0 && 
        strcmp(argv1,"-e") != 0 &&
        strcmp(argv1,"-l") != 0 && 
        strcmp(argv1,"-r") != 0 && 
        strcmp(argv1,"-t") != 0 && 
        strcmp(argv1,"-h") != 0 &&
        strcmp(argv1,"-o") != 0) {
            flag = 0;
            write(1,"Comando Inválido\n",19);
    }
    // Envio do comando para o servidor
    else if(write(fdToServer,args,strlen(args)) < 0) {
        perror("Write");
        exit(1);
    }
    int fdFromServer;
    fdFromServer = open("../SO/wr",O_RDONLY);
    // Lê resposta do servidor, se houver resposta
    if(flag) {
        while((bytesRead = read(fdFromServer,args,150)) > 0) {
            if(write(1,args,bytesRead) < 0) {
                perror("Write"),
                exit(1);
            }
        }
    }
    close(fdFromServer);
}

int main(int argc,char*argv[]) {
    int fdToServer;
    if((fdToServer = open("../SO/fifo", O_WRONLY)) < 0) {
        perror("Fifo error");
        exit(1);
    }
    if(argc == 1) {
        shellInterpreter(fdToServer);
    }
    else if(argc > 1) {
        commandInterpreter(fdToServer, argv[1], argv[2]);
    }
    close(fdToServer);
    return 0;
}