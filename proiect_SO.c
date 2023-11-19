#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pwd.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>

// Funcție pentru a verifica dacă este un fișier BMP
int is_bmp_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return (ext != NULL && strcmp(ext, ".bmp") == 0);
}

struct bmp_info_header {
    int width;
    int height;
} info_header;

char* extract_filename(const char *path)
{
    char *filename = strdup(path);
    char *basename_result = basename(filename);
    char *result = strdup(basename_result);
    free(filename);
    return result;
}

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

    if (lseek(fd, 54, SEEK_SET) == -1) {
        perror("Eroare la setarea poziției curente");
        close(fd);
        return;
    }

    int nr_pixels = info_header.width *info_header.height;

    //Citim si procesam pixelii imaginii
    for(int i = 0; i < nr_pixels; i++)
    {
        unsigned char pixel[3];
        if(read(fd, pixel, sizeof(pixel)) != sizeof(pixel))
        {
            perror("Eroare la citirea pixelilor BMP\n");
            close(fd);
            return;
        }

        //Calculam intensitatea tonurilor de gri
        unsigned char P_gri = 0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0];

        //Suprascriem cele 3 valori cu valoarea P_gri
        pixel[0] = P_gri;
        pixel[1] = P_gri;
        pixel[2] = P_gri;

        //Revenim la pozitia curenta a pixelului in fisier
        if(lseek(fd, -sizeof(pixel), SEEK_CUR) == -1)
        {
            perror("Eroare la revenirea la pozitia curenta\n");
            close(fd);
            return;
        }

        //Scriem noile valori in fisier
        if(write(fd, pixel, sizeof(pixel)) == -1)
        {
            perror("Eroare la scriere pixelilor BMP\n");
            close(fd);
            return;
        }
    }

    // Închidem fisierul BMP
    close(fd);
}

void process_file(const char *filename, const char *output_dir)
{
    DIR *dir;
    if((dir = opendir(output_dir)) == NULL)
    {
        perror("Eroare deschidere director scriere");
        exit(1);
    }
    pid_t pid = fork();
    int status;
    int nr_linii = 0;
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
    char *base = extract_filename(filename);
    char output_filename[1024];
    sprintf(output_filename, "statistica_%s.txt", base);
    if(pid < 0)
    {
        perror("Eroare fork\n");
        exit(-1);
    }else if(pid == 0)
    {
        if (S_ISLNK(file_stat.st_mode)) {
            // Deschidem fisierul statistica_link.txt pentru scriere
            int output_link = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (output_link == -1) {
                perror("Eroare la crearea fisierului statistica.txt");
                return;
            }
            char target[1024];
            struct stat leg_stat;
            if (stat(filename, &leg_stat) == -1) {
                perror("Eroare la obtinerea informatiilor despre fisier");
                return;
            }

            ssize_t len = readlink(filename, target, sizeof(target) - 1);
            if (len != -1) {
                target[len] = '\0';
                sprintf(stats, "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier target: %ld\ndrepturi de acces user legatura: %s\ndrepturi de acces grup legatura: %s\ndrepturi de acces altii legatura: %s\n", filename, file_stat.st_size, leg_stat.st_size, permissions_user, permissions_group, permissions_other);
            }
            nr_linii = write(output_link, stats, strlen(stats));
            if (nr_linii == -1) {
                perror("Eroare scriere in fisier de statistici");
            }
            close(output_link);
        }else if (S_ISREG(file_stat.st_mode))
        {
            // Verificăm dacă fișierul are extensia ".bmp"
            if (is_bmp_file(filename)) {
                pid_t pid_bmp = fork();
                if(pid_bmp == -1)
                {
                    perror("Eroare la crearea procesului pt imaginea BMP\n");
                    exit(-1);
                }else if (pid_bmp == 0){//proces fiu
                    get_bmp_info(filename);
                }
                else {
                    int status;
                    waitpid(pid_bmp, &status, 0);
                    printf("S-a incheiat procesul pentru imaginea BMP cu pid-ul %d si codul %d\n", pid_bmp, WEXITSTATUS(status));
                }
                // Deschidem fisierul statistica_bmp.txt pentru scriere
                int output_bmp = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (output_bmp == -1) {
                    perror("Eroare la crearea fisierului statistica.txt");
                    return;
                }
                
                sprintf(stats, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, info_header.height, info_header.width, (long)file_stat.st_size, file_stat.st_uid, time_string, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
                nr_linii = write(output_bmp, stats, strlen(stats));
                if ( nr_linii == -1) {
                    perror("Eroare scriere in fisier de statistici");
                }
                close(output_bmp);
            }
            else {
                // Deschidem fisierul statistica_fisier.txt pentru scriere
                int output_fisier = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (output_fisier == -1) {
                    perror("Eroare la crearea fisierului statistica.txt");
                    return;
                }
                sprintf(stats, "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_size, file_stat.st_uid, time_string, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
                nr_linii = write(output_fisier, stats, strlen(stats));
                if (nr_linii == -1) {
                    perror("Eroare scriere in fisier de statistici");
                }
                close(output_fisier);
            }
        }else if (S_ISDIR(file_stat.st_mode))
        {
            // Deschidem fisierul statistica_folder.txt pentru scriere
            int output_folder = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (output_folder == -1) {
                perror("Eroare la crearea fisierului statistica.txt");
                return;
            }
            sprintf(stats, "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_uid, permissions_user, permissions_group, permissions_other);
            nr_linii = write(output_folder, stats, strlen(stats));
            if (nr_linii == -1) {
                perror("Eroare scriere in fisier de statistici");
            }
            close(output_folder);
        }else{
            //Nu se scrie nimic 
        }
        exit(nr_linii);
    }else {
        //daca e parinte
        pid = wait(&status);
        if(WIFEXITED(status))
        {
            printf("Child with id = %d exited with status code %d\n", pid, WEXITSTATUS(status));
        }
    }
    free(base);            
    closedir(dir);
    
}

void citire_director(const char *director, const char *output_dir)
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
            process_file(str, output_dir);
        }
    } 
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <fisier_intrare>\n", argv[0]);
        return 1;
    }

    citire_director(argv[1], argv[2]);
    return 0;
}
