#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#define MAX_PATH 1024

void showContent(int recursive, int size, int write, char* path, int firstDir){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct stat statbuf;
    
    dir = opendir(path);
    if(dir == NULL){
        printf("ERROR\ninvalide directory path\n");
        return;
    }

    char filePath[MAX_PATH];
    if(firstDir)
        printf("SUCCESS\n");

    while((entry = readdir(dir)) != NULL){
        if(strcmp(".", entry -> d_name) != 0 && strcmp("..", entry -> d_name) != 0){
            int valid = 1;
            snprintf(filePath, MAX_PATH, "%s/%s", path, entry -> d_name);
            int lstatValue = lstat(filePath, &statbuf);

            if(lstatValue == 0){
                if(write != -1 && (statbuf.st_mode & S_IWUSR) == 0)
                    valid = 0;
                if(size != -1 && (S_ISDIR(statbuf.st_mode) || statbuf.st_size > size))
                    valid = 0;
            }

            if(valid == 1){
                for (int i = 0; filePath[i] != '\0'; i++){
                    if(filePath[i] == ' ')
                        filePath[i] = '_';
                }
                printf("%s\n", filePath);
            }
            if(lstatValue == 0 && recursive != -1 && S_ISDIR(statbuf.st_mode))
                showContent(recursive, size, write, filePath, 0);
        }
    } 

    closedir(dir);
}

void list(int argc, char **argv){ 
    int recursive = -1;
    int size = -1;
    int write = -1;
    char* path;

    for(int i = 2; i < argc; i++){
        if(strcmp(argv[i], "recursive") == 0)
            recursive = 1;
        else if(strcmp(argv[i], "has_perm_write") == 0)
            write = 1;
        else if(strncmp(argv[i], "size_smaller=", 13) == 0)
            size = atoi(argv[i] + 13);
        else if(strncmp(argv[i], "path=", 5) == 0)
            path = argv[i] + 5;
    }
    showContent(recursive, size, write, path, 1);
}

void parse(int argc, char **argv){
    char* path = argv[2] + 5;
    int fd = -1;
    fd = open(path, O_RDONLY);
    if(fd == -1){
        printf("ERROR\nCould not open file\n");
        return;
    }

    unsigned char magic;
    short headerSize;
    unsigned char version;
    unsigned char noOfSections;
    int valid = 1;
    unsigned char type;

    lseek(fd, -1, SEEK_END);
    if(read(fd, &magic, 1) != 1){
        printf("ERROR\nwrong data\n");
        close(fd);
        return;
    }

    if(magic != '6'){
        printf("ERROR\nwrong magic\n");
        close(fd);
        return;
    }

    lseek(fd, -3, SEEK_END);
    if(read(fd, &headerSize, 2) != 2){
        printf("ERROR\nwrong header size\n");
        close(fd);
        return;
    }

    lseek(fd, -headerSize, SEEK_END);
    if(read(fd, &version, sizeof(char)) != 1){
        printf("ERROR\nwrong data\n");
        close(fd);
        return;
    }

    if(version < 92 || version > 201){
        printf("ERROR\nwrong version\n");
        close(fd);
        return;
    }

    if(read(fd, &noOfSections, sizeof(char)) != 1){
        printf("ERROR\nwrong data\n");
        close(fd);
        return;
    }

    if(noOfSections != 2 && (noOfSections < 8 || noOfSections > 16)){
        printf("ERROR\nwrong sect_nr\n");
        close(fd);
        return;
    }

    lseek(fd, 15, SEEK_CUR);
    if(read(fd, &type, sizeof(char)) != 1){
        printf("ERROR\nwrong data\n");
        close(fd);
        return;
    }

    if(type != 73 && type != 34)
        valid = 0;

    for (int i = 1; i < noOfSections && valid == 1; i++){
        lseek(fd, 23, SEEK_CUR);
        if(read(fd, &type, sizeof(char)) != 1){
            printf("ERROR\nwrong data\n");
            close(fd);
            return;
        }
        if(type != 73 && type != 34)
            valid = 0;
    }
    if(valid){
        printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, noOfSections);
        lseek(fd, -headerSize + 2, SEEK_END);
        int sectNameSize = 15;
        unsigned char sectName[sectNameSize];
        int sectSize;
        for (int i = 0; i < noOfSections; ++i){
            read(fd, sectName, sectNameSize);
            read(fd, &type, 1);
            lseek(fd, 4, SEEK_CUR);
            read(fd, &sectSize, sizeof(int));
            printf("section%d: %s %d %d\n", i + 1, sectName, type, sectSize);
        }
    }
    else{
        printf("ERROR\nwrong sect_types\n");
    }
    close(fd);
}

void extract(int argc, char **argv){
    char* path = argv[2] + 5;
    int section = atoi(argv[3] + 8);
    int currentSection = 0;
    int lineNo = atoi(argv[4] + 5);
    int currentLine = 1;
    int fd = -1;

    fd = open(path, O_RDONLY);
    if(fd == -1){
        printf("ERROR\ninvalid file\n");
        return;
    }

    unsigned char noOfSections;
    short headerSize = 0;

    lseek(fd, -3, SEEK_END);
    read(fd, &headerSize, 2);
    lseek(fd, -headerSize + 1, SEEK_END);
    read(fd, &noOfSections, sizeof(char));

    if(section < 1 || noOfSections < section){
        printf("ERROR\nwrong section\n");
        close(fd);
        return;
    }

    currentSection = (section - 1) * 24 + 16;
    lseek(fd, currentSection, SEEK_CUR);
    off_t filePosition;
    read(fd, &filePosition, 4);
    lseek(fd, filePosition, SEEK_SET);

    int bufferSize = 128;
    char* line = malloc(bufferSize * sizeof(char));
    int noOfCharacters = 0;
    int spaces = 1;
    char character = ' ';

    while(character != 0x00){
        read(fd, &character, 1);
        if(character == '\n')
            spaces ++;
        line[noOfCharacters ++] = character;
        if(noOfCharacters >= bufferSize){
            bufferSize *= 2;
            line = realloc(line, bufferSize);
        }
    }
    line[noOfCharacters] = '\0';

    if(spaces < lineNo){
        printf("ERROR\nwrong line\n%d\n", spaces);
        close(fd);
        return;
    }

    for (int i = strlen(line) - 1; i >= 0; --i){
        if(line[i] == '\n')
            currentLine ++;
        if(currentLine == lineNo){
            printf("SUCCESS\n");
            if(i != strlen(line) - 1)
                i --;
            while(1){
                character = line[i];
                if(character == '\n'){
                    printf("\n");
                    free(line);
                    close(fd);
                    return;
                }
                else if(i == 0){
                    printf("%c\n", character);
                    free(line);
                    close(fd);
                    return;
                }
                else{
                    printf("%c", character);
                }
                i --;
            }
        }
    }
}

void findall(char* path, int firstDir){
    DIR* dir = NULL;
    struct dirent *entry = NULL;
    struct stat statbuf;
    char filePath[MAX_PATH];

    dir = opendir(path);
    if(dir == NULL){
        printf("ERROR\ninvalide directory path\n");
        return;
    }

    if(firstDir)
        printf("SUCCESS\n");

    while((entry = readdir(dir)) != NULL){
        if(strcmp(".", entry -> d_name) != 0 && strcmp("..", entry -> d_name) != 0){
            snprintf(filePath, MAX_PATH, "%s/%s", path, entry -> d_name);
            int lstatValue = lstat(filePath, &statbuf);

            if(lstatValue == 0){
                if(S_ISDIR(statbuf.st_mode)){
                    findall(filePath, 0);
                }
                else{
                    int fd = -1;
                    fd = open(filePath, O_RDONLY);

                    if(fd == -1){
                        printf("ERROR\nCould not open file\n");
                        return;
                    }
                    int valid = 1;
                    unsigned char magic;
                    short headerSize;
                    unsigned char version;
                    unsigned char noOfSections;

                    lseek(fd, -1, SEEK_END);
                    read(fd, &magic, 1);
                    if(magic != '6')
                        valid = 0;

                    lseek(fd, -3, SEEK_END);
                    read(fd, &headerSize, 2);
                    lseek(fd, -headerSize, SEEK_END);

                    read(fd, &version, 1);
                    if(version < 92 || version > 201)
                        valid = 0;

                    read(fd, &noOfSections, 1);
                    if(noOfSections != 2 && (noOfSections < 8 || noOfSections > 16))
                        valid = 0;

                    if(valid == 1){
                        lseek(fd, 20, SEEK_CUR);
                        int sectionSize;
                        for (int i = 0; i < noOfSections; ++i){
                            read(fd, &sectionSize, 4);
                            if(sectionSize > 927){
                                valid = 0;
                                break;
                            }
                            lseek(fd, 20, SEEK_CUR);
                        }
                        if(valid == 1){
                            for (int i = 0; filePath[i] != '\0'; i++){
                                if(filePath[i] == ' ')
                                    filePath[i] = '_';
                            }
                            printf("%s\n", filePath);
                        }
                    }

                    close(fd);
                }
            }
        }
    }
}

int main(int argc, char **argv){
    if(argc >= 2) {
        if(strcmp(argv[1], "variant") == 0) {
            printf("68275\n");
        }
        else if(strcmp(argv[1], "list") == 0) {
            list(argc, argv);
        }
        else if(strcmp(argv[1], "parse") == 0){
            parse(argc, argv);
        }
        else if(strcmp(argv[1], "extract") == 0){
            extract(argc, argv);
        }
        else if(strcmp(argv[1], "findall") == 0){
            findall(argv[2] + 5, 1);
        }
    }
    return 0;
}