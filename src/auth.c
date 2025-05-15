#include "academia.h"

bool authenticate(const char *role, const char *username, const char *password) {
    sem_wait(file_sem);
    
    printf("Trying to open: %s\n", USER_FILE);
    int fd = open(USER_FILE, O_RDONLY);
    if (fd < 0) {
        sem_post(file_sem);
        printf("Failed to open user file. %d\n", fd);
        return false;
    }
    printf("User file opened.\n");
    char line[256];
    bool found = false;
    lseek(fd, 0, SEEK_SET);
    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        char frole[16], funame[64], fpass[64];
        int active;
        if (sscanf(line, "%63[^|]|%15[^|]|%63[^|]|%d", funame, frole, fpass, &active) == 4) {
            if (strcmp(frole, role) == 0 && strcmp(funame, username) == 0 &&
                strcmp(fpass, password) == 0 && active == 1) {
                found = true;
                break;
            }
        }
        memset(line, 0, sizeof(line));
    }

    close(fd);
    sem_post(file_sem);
    return found;
}