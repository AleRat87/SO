#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <string.h>
#include <dirent.h>

// Funcție pentru a verifica dacă este un fișier BMP
int is_bmp_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return (ext != NULL && strcmp(ext, ".bmp") == 0);
}

struct bmp_info_header {
    int width;
    int height;
} info_header;

void get_bmp_info(const char *filename)
{
    // Deschidem fisierul BMP
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului bmp");
        return;
    }

    // Setăm poziția curentă la începutul header-ului BMP
    if (lseek(fd, 18, SEEK_SET) == -1) {
        perror("Eroare la setarea poziției curente");
        close(fd);
        return;
    }

    // Citim inalțimea și lățimea imaginii BMP
    if (read(fd, &info_header, sizeof(struct bmp_info_header)) != sizeof(struct bmp_info_header)) {
        perror("Eroare la citirea inaltimei si latimii BMP");
        close(fd);
        return;
    }

    // Închidem fisierul BMP
    close(fd);
}

void process_file(const char *filename, const char *output_file)
{

    // Obținem dimensiunea fișierului folosind funcția stat
    struct stat file_stat;
    if (lstat(filename, &file_stat) == -1) {
        perror("Eroare la obtinerea informatiilor despre fisier");
        return;
    }

    // Obținem identificatorul utilizatorului și timpul ultimei modificări
    char time_string[20];
    if(localtime(&file_stat.st_mtime) == NULL)
    {
        perror("Eroare obtinre timp modificare");
        return;
    }
    strftime(time_string, sizeof(time_string), "%d.%m.%Y", localtime(&file_stat.st_mtime));

    char permissions_user[4];
    char permissions_group[4];
    char permissions_other[4];
    sprintf(permissions_user, "%c%c%c",
            (file_stat.st_mode & S_IRUSR) ? 'R' : '-',
            (file_stat.st_mode & S_IWUSR) ? 'W' : '-',
            (file_stat.st_mode & S_IXUSR) ? 'X' : '-');
 
    sprintf(permissions_group, "%c%c%c",
            (file_stat.st_mode & S_IRGRP) ? 'R' : '-',
            (file_stat.st_mode & S_IWGRP) ? 'W' : '-',
            (file_stat.st_mode & S_IXGRP) ? 'X' : '-');
 
    sprintf(permissions_other, "%c%c%c",
            (file_stat.st_mode & S_IROTH) ? 'R' : '-',
            (file_stat.st_mode & S_IWOTH) ? 'W' : '-',
            (file_stat.st_mode & S_IXOTH) ? 'X' : '-');
    
    char stats[1024];
    int stat_file = open(output_file, O_WRONLY | O_APPEND);
    if (stat_file == -1) {
        perror("Eroare deschidere fisier de statistici");
        return;
    }

    if (S_ISLNK(file_stat.st_mode)) {
        char target[1024];
        
        ssize_t len = readlink(filename, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            sprintf(stats, "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier target: %ld\ndrepturi de acces user legatura: %s\ndrepturi de acces grup legatura: %s\ndrepturi de acces altii legatura: %s\n", filename, file_stat.st_size, len, permissions_user, permissions_group, permissions_other);
        }
    }else if (S_ISREG(file_stat.st_mode))
    {
        // Verificăm dacă fișierul are extensia ".bmp"
        if (is_bmp_file(filename)) {
            get_bmp_info(filename);
            sprintf(stats, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, info_header.height, info_header.width, (long)file_stat.st_size, file_stat.st_uid, time_string, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
        }
        else {
            //fisier obisnuit
            sprintf(stats, "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_size, file_stat.st_uid, time_string, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
        }
    }else if (S_ISDIR(file_stat.st_mode))
    {
        sprintf(stats, "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_uid, permissions_user, permissions_group, permissions_other);
    }else{
        //Nu se scrie nimic in fisierul statistica.txt
    }
    
    if (write(stat_file, stats, strlen(stats)) == -1) {
        perror("Eroare scriere in fisier de statistici");
    }
 
    close(stat_file);
}

void citire_director(const char *director, const char *output_file)
{
    DIR *dir;
    if((dir = opendir(director)) == NULL)
    {
        perror("Eroare deschidere director");
        exit(1);
    }
    struct dirent *entry;

    char str[1024];
    while((entry=readdir(dir))!=NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
	        sprintf(str, "%s/%s", director, entry->d_name);
            process_file(str, output_file);
        }
    } 
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <fisier_intrare>\n", argv[0]);
        return 1;
    }

    // Deschidem fisierul statistica.txt pentru scriere
    int output_fd = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (output_fd == -1) {
        perror("Eroare la crearea fisierului statistica.txt");
        return 1;
    }

    // Inchidem fisierul statistica.txt
    close(output_fd);
    citire_director(argv[1], "statistica.txt");

    return 0;
}