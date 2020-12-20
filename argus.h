#ifndef argus_h
#define argus_h

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


int output(int n,int logs,int wr);
char* mySep(char* tok, char* buf, char delim);
int count(int numero);
int executar(char* buf);
void histTerm();
int terminarTarefa(char*command);
void adicionarTarefa(int filho, char* buf);

#endif

