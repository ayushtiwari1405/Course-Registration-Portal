#include "academia.h"

sem_t *file_sem;

void init_file_semaphore() {
    sem_unlink("/file_sem");
    file_sem = sem_open("/file_sem", O_CREAT | O_EXCL, 0644, 1);
    if (file_sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}

void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

void cleanup_file_semaphore() {
    sem_close(file_sem);
    sem_unlink("/file_sem");
}

void password_change(int client_sock, const char *username, const char *role) {
    char  newpass[64];
    sender(client_sock, "Enter new password:");
    receiver(client_sock, newpass, sizeof(newpass));

    sem_wait(file_sem);
    
    int fd = open(USER_FILE, O_RDWR);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    char lines[100][256];
    int count = 0, found = 0;

    lseek(fd, 0, SEEK_SET);

    // Read and preserve header line
    if (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        snprintf(lines[count], sizeof(lines[count]), "Username|Role|Password|Status\n");
        count++;
    }

    // Read and process remaining lines
    while (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        char funame[64], frole[16], fpass[64]; 
        int active;

        if (sscanf(lines[count], "%63[^|]|%15[^|]|%63[^|]|%d", funame, frole, fpass, &active) == 4) {
            if (strcmp(frole, role) == 0 && strcmp(funame, username) == 0) {
                snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%d\n", 
                         funame, frole, newpass, active);
                found = 1;
            } else {
                snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%d\n", 
                         funame, frole, fpass, active);
            }
        }
        count++;
    }

    if (!found) {
        close(fd);
        sem_post(file_sem);
        sender(client_sock, "User not found.");
        return;
    }

    // Rewrite updated content to file
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    for (int i = 0; i < count; i++) {
        write(fd, lines[i], strlen(lines[i]));
    }
    fsync(fd);
    close(fd);
    
    sem_post(file_sem);
    sender(client_sock, "User updated.");
}

ssize_t safe_read_line(int fd, char *buf, size_t maxlen) {
    size_t i = 0;
    char c;
    while (i < maxlen - 1) {
        ssize_t n = read(fd, &c, 1);
        if (n == 1) {
            if (c == '\n') break;
            buf[i++] = c;
        } else if (n == 0) break;
        else return -1;
    }
    buf[i] = '\0';
    return i;
}