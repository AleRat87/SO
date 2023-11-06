#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void citire_director(char *director)
{
  DIR *dir;
  if((dir = opendir(director)) == NULL)
    {
      perror("Eroare deschidere director");
      exit(1);
    }
  struct dirent *entry;
  struct stat st_file;
  char str[500];
  while((entry=readdir(dir))!=NULL)
    {
      if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
	{
	  printf("%s", entry->d_name);
	  sprintf(str, "%s/%s", director, entry->d_name);
	  stat(str, &st_file);
	  
	}
    } 
}

int main(int argc, char *argv[])
{
  char *file = argv[1];
  if(argc != 2)
    {
      perror("Usage ./program ");
      exit(1);
    }
  int file_descriptor = open(file, O_RDONLY);
  if(file_descriptor == -1)
    {
      perror("Eroare deschidere fisier");
      exit(1);
    }
  lseek(file_descriptor, 18, SEEK_SET);
  int latime;
  read(file_descriptor, &latime, sizeof(int));
  citire_director(argv[1]);
  return 0;
}
