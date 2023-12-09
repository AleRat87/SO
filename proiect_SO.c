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
    int fd = open(filename, O_RDWR);
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
    exit(0);
}
int nr_linii = 0;
void process_file(const char *filename, const char *output_dir)
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
    char *base = extract_filename(filename);
    char output_filename[1024];
    sprintf(output_filename, "%s/statistica_%s.txt", output_dir, base);
    
        if (S_ISLNK(file_stat.st_mode)) {
            // Deschidem fisierul statistica_link.txt pentru scriere
            int output_link = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (output_link == -1) {
                perror("Eroare la crearea fisierului statistica.txt");
                exit(-1);
            }
            char target[1024];
            struct stat leg_stat;
            if (stat(filename, &leg_stat) == -1) {
                perror("Eroare la obtinerea informatiilor despre fisier");
                exit(-1);
            }
            ssize_t len = readlink(filename, target, sizeof(target) - 1);
            if (len != -1) {
                target[len] = '\0';
                sprintf(stats, "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier target: %ld\ndrepturi de acces user legatura: %s\ndrepturi de acces grup legatura: %s\ndrepturi de acces altii legatura: %s\n", filename, file_stat.st_size, leg_stat.st_size, permissions_user, permissions_group, permissions_other);
            }
            nr_linii += 6;
            if ((write(output_link, stats, strlen(stats))) == -1) {
                perror("Eroare scriere in fisier de statistici");
                exit(-1);
            }
            close(output_link);
        }else if (S_ISREG(file_stat.st_mode))
        {
            // Verificăm dacă fișierul are extensia ".bmp"
            if (is_bmp_file(filename)) {
                // Deschidem fisierul statistica_bmp.txt pentru scriere
                int output_bmp = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (output_bmp == -1) {
                    perror("Eroare la crearea fisierului statistica.txt");
                    exit(-1);
                }
                
                sprintf(stats, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, info_header.height, info_header.width, (long)file_stat.st_size, file_stat.st_uid, time_string, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
                nr_linii += 10;
                if ( (write(output_bmp, stats, strlen(stats))) == -1) {
                    perror("Eroare scriere in fisier de statistici");
                    exit(-1);
                }
                close(output_bmp);
            }
            else {
                // Deschidem fisierul statistica_fisier.txt pentru scriere
                int output_fisier = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (output_fisier == -1) {
                    perror("Eroare la crearea fisierului statistica.txt");
                    exit(-1);
                }
                sprintf(stats, "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_size, file_stat.st_uid, time_string, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
                nr_linii += 8;
                if ((write(output_fisier, stats, strlen(stats))) == -1) {
                    perror("Eroare scriere in fisier de statistici");
                    exit(-1);
                }
                close(output_fisier);
            }
        }else if (S_ISDIR(file_stat.st_mode))
        {
            // Deschidem fisierul statistica_folder.txt pentru scriere
            int output_folder = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (output_folder == -1) {
                perror("Eroare la crearea fisierului statistica.txt");
                exit(-1);
            }
            sprintf(stats, "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_uid, permissions_user, permissions_group, permissions_other);
            nr_linii += 5;
            if ((write(output_folder, stats, strlen(stats))) == -1) {
                perror("Eroare scriere in fisier de statistici");
                exit(-1);
            }
            close(output_folder);
        }
        exit(nr_linii);
    
    free(base);            
}

void citire_director(const char *director, const char *output_dir, const char *caracter)
{

    DIR *dir;
    struct dirent *entry;
    pid_t pid;
    char str[1024];
    int pfd[2];
    struct stat file_stat;
    int nr_prop = 0;
    int status_iesire[1024];
    int nr_copii = 0;
    if((dir = opendir(director)) == NULL)
    {
        perror("Eroare deschidere director");
        exit(-1);
    }
    if(pipe(pfd) < 0)
    {
        perror("Eroare la crearea pipe-ului\n");
        exit(-1);
    }
    while((entry=readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
	        sprintf(str, "%s/%s", director, entry->d_name);
            if (lstat(str, &file_stat) == -1) {
                perror("Eroare la obtinerea informatiilor despre fisier intrare");
                exit(-1);
            }
            pid = fork();
            if(pid < 0)
            {
                perror("Eroare la crearea procesului copil principal");
                exit(-1);
            }else if(pid == 0)
            {
                close(pfd[1]);//inchid capatul de scriere al pipe-ului
                process_file(str, output_dir);
                // if(S_ISREG(file_stat.st_mode) && !is_bmp_file(str))//genereaza folosind cat continutul fisierului obisnuit si trimite acest continut celuilalt proces fiu creat de parinte
                // {
                //     close(pfd[0]);//inchide capatul de cititre al pipe-ului
                //     dup2(pfd[1], 1);//redirectez intrarea standard catre capatul de scriere al pipe-ului
                //     execlp("cat", "cat", str, NULL);
                //     printf("Eroare redirectare cat\n");
                // }
            }
            else{//proces parinte principal
                if(S_ISREG(file_stat.st_mode))
                {
                    if (is_bmp_file(str))
                    {
                        pid_t pid_bmp = fork();
                        if(pid_bmp == -1)
                        {
                            perror("Eroare la crearea procesului fiu pt imaginea BMP\n");
                            exit(-1);
                        }else if (pid_bmp == 0){//proces fiu
                            get_bmp_info(str);
                        }
                        else {
                            int status;
                            waitpid(pid_bmp, &status, 0);
                            if(WIFEXITED(status))
                                printf("S-a incheiat procesul pentru imaginea BMP cu pid-ul %d si codul %d\n", pid_bmp, WEXITSTATUS(status));
                        }
                    }
                    if(!is_bmp_file(str)){//daca e fisier obisnuit
                        //pid_t pid_sec = fork();
                        // if(pid_sec < 0)
                        // {
                        //     perror("Eroare fork secund\n");
                        //     exit(-1);
                        // }else if(pid_sec == 0){
                        //     close(pfd[1]);//inchidem capatul de scriere al pipe-ului
                        //     dup2(pfd[0], 0);
                        //     close(pfd[0]);//inchide capatul de citire al pipe-ului
                        //     execlp("bash", "bash", "p9.sh", caracter, NULL);
                        //     printf("Eroare la executie script\n");
                        //     //exit(-1);
                        // }
                        // else {
                        //     close(pfd[0]);
                        //     ssize_t bytes_read;
                        //     char buffer[1024];
                        //     int input_fd = open(str, O_RDWR);//maybe an error
                        //     close(pfd[0]);//inchidem capatul de citire al pipe-ului
                        //     while((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0)
                        //     {
                        //         write(pfd[1], buffer, bytes_read);//acest capat va primi numarul de propozitii de la copil
                        //     }
                        //     close(pfd[1]);//inchidem capatul de scriere
                        //     int status;
                        //     waitpid(pid_sec, &status, 0);
                        //     if(WIFEXITED(status))
                        //     {
                        //         nr_prop = nr_prop + WEXITSTATUS(status);
                        //         printf("S-a incheiat procesul pentru fisier obisnuit(propozitii) cu pid-ul %d si codul %d", pid_sec, WEXITSTATUS(status));
                        //         printf("Au fost identificate in total %d propozitii corecte care contin caracterul %s", nr_prop, caracter);
                        //     }
                        // }
                    }
                }
                status_iesire[nr_copii++] = 0;          
            }
            close(pfd[0]);
            close(pfd[1]);
        }
    } 
    for(int i = 0; i < nr_copii; ++i)
    {
        int status;
        pid = waitpid(-1, &status, 0);
        if(WIFEXITED(status))
        {
            status_iesire[i] = WEXITSTATUS(status);
            printf("S-a incheiat procesul cu pid-ul %d si codul %d\n", pid, status_iesire[i]);
        }
    }
    if(closedir(dir) == -1)
    {
        perror("Eroare inchidere director");
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Argumente insuficiente\n");
        return 1;
    }

    citire_director(argv[1], argv[2], argv[3]);
    return 0;
}
