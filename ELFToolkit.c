#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define READ_BYTES 10
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"
#define CAVE_SIZE 1024

void print_usage(char *program_name) {
    printf("\nUsage: %s <file_path> <action> [num_bytes | byte_sequence]\n", program_name);
    printf("Actions:\n");
    printf("  r <num_bytes>      - Read the first <num_bytes> bytes of the file (default is 10)\n");
    printf("  c                  - Search for code caves in the file\n");
    printf("  s                  - Show file size\n");
    printf("  f <byte_sequence>  - Find a sequence of bytes in the file\n");
    printf("  p                  - Display file permissions and metadata\n");
}

void read_bytes_from_file(FILE *file, int num_bytes) {
    unsigned char *buffer = malloc(num_bytes);
    if (!buffer) {
        printf("\nError allocating memory for buffer\n");
        return;
    }

    int bytes_read = fread(buffer, 1, num_bytes, file);
    printf("\nOffset      Hexadecimal                  ASCII\n");
    printf("------------------------------------------------------\n");

    for (int i = 0; i < bytes_read; i += 16) {
        printf("0x%08X  ", i);

        for (int j = 0; j < 16 && i + j < bytes_read; j++) {
            printf("%02X ", buffer[i + j]);
        }

        if (bytes_read - i < 16) {
            for (int k = 0; k < (16 - (bytes_read - i)); k++) {
                printf("   ");
            }
        }

        printf("  ");

        for (int j = 0; j < 16 && i + j < bytes_read; j++) {
            unsigned char ch = buffer[i + j];
            printf((ch >= 32 && ch <= 126) ? "%c" : ".");
        }

        printf("\n");
    }

    free(buffer);
}

void find_code_caves(FILE *file) {
    unsigned char byte;
    long start = -1, length = 0;

    while (fread(&byte, 1, 1, file) == 1) {
        if (byte == 0x00) {
            if (start == -1) start = ftell(file) - 1;
            length++;
        } else {
            if (length >= CAVE_SIZE)
                printf("\n[+] Code Cave: 0x%lX - 0x%lX\n", start, ftell(file) - 2);
            start = -1;
            length = 0;
        }
    }

    if (length >= CAVE_SIZE)
        printf("[+] Code Cave: 0x%lX - 0x%lX\n", start, ftell(file) - 1);
}

void display_file_size(FILE *file) {
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("\nFile size: %ld bytes\n", file_size);
}

void display_file_info(const char *file_path) {
    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0) {
        perror("\nError retrieving file information");
        return;
    }

    printf("\nFile Permissions: ");
    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

    printf("\nFile Size: %lld bytes\n", (long long)file_stat.st_size);
    printf("Last Access: %s", ctime(&file_stat.st_atime));
    printf("Last Modification: %s", ctime(&file_stat.st_mtime));
    printf("Last Status Change: %s", ctime(&file_stat.st_ctime));
}

void search_byte_sequence(FILE *file, const unsigned char *sequence, size_t seq_len) {
    unsigned char buffer[4096];
    size_t offset = 0;
    size_t matched = 0;

    while (!feof(file)) {
        size_t read_bytes = fread(buffer, 1, sizeof(buffer), file);
        for (size_t i = 0; i < read_bytes; i++) {
            if (buffer[i] == sequence[matched]) {
                matched++;
                if (matched == seq_len) {
                    printf("\n[+] Sequence found at offset: 0x%lX\n", offset + i - seq_len + 1);
                    matched = 0;
                }
            } else {
                matched = 0;
            }
        }
        offset += read_bytes;
    }
}

int main(int argc, char **argv) {
    printf("ELFToolkit 0.4 \nWritten by R4idb0y\n");

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        printf("\nError opening file: %s\n", argv[1]);
        return 1;
    }

    if (strcmp(argv[2], "r") == 0) {
        int num_bytes = (argc == 4) ? atoi(argv[3]) : READ_BYTES;
        read_bytes_from_file(file, num_bytes);
    } else if (strcmp(argv[2], "c") == 0) {
        find_code_caves(file);
    } else if (strcmp(argv[2], "s") == 0) {
        display_file_size(file);
    } else if (strcmp(argv[2], "f") == 0 && argc == 4) {
        search_byte_sequence(file, (unsigned char *)argv[3], strlen(argv[3]));
    } else if (strcmp(argv[2], "p") == 0) {
        fclose(file); 
        display_file_info(argv[1]);
        return 0;
    } else {
        printf("\nInvalid action: %s\n", argv[2]);
        print_usage(argv[0]);
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}
