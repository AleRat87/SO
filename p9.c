#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
int main(int argc, char *argv[])
{
  char *file = argv[1];
  int input_fd;
  char buffer[1024];
  ssize_t bytes_read;
  int status;

  int pfd[2];
  FILE *stream;
  
  if(pipe(pfd)<0)
    {
      printf("Eroare la crearea pipe-ului\n");
      exit(1);
    }

  int pid;
  pid=fork();//pornim procesul
  if(pid ==0)//daca e proces copil; PRIMUL PROCES Pe care il avem deja in proiect
    {
      input_fd = open(file, O_RDONLY);//deschide fisierul pentru citire
      close(pfd[0]); //inchide capatul de citire al pipeului
 
      while( (bytes_read = read (input_fd, buffer,sizeof(buffer))) > 0)
	{
	  write(pfd[1], buffer, bytes_read);//scrie in pipe
    
	}//primul proces citeste datele din pipe si le redirectioneaza catre al doilea pipe
      close(pfd[1]);//inchide capatul de scriere
      exit(0);
    }

 int pid1=fork();
 if(pid1 == 0)///cream alt proces copil care redirecteaza inputul catre bash si 
   {
     close(pfd[1]);
     dup2(pfd[0],0);
     close(pfd[0]);
     execlp("bash", "bash", "p9a.sh","c", NULL);
     printf("Eroare la executie\n");
   }
 close(pfd[0]);
 close(pfd[1]);
 while( (pid = wait(&status)) != -1)
   {
   }
 //mai trebuie sa creez un pipe la care redirectez stdout la capatul de scriere si in procesul parinte sa redirectez capatul de intrare la pipe
     //parintele citest
 // if(WIFEXITED(status))
 //{
 //  printf("%d", vstatus);
 //}
   
}
