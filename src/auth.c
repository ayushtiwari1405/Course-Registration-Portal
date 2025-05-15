#include "academia.h"

// Authenticates a user by checking their role, username, and password against the data in the user file.
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
    // Read each line of the user file.
    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        char frole[16], funame[64], fpass[64];
        int active;
        // Parse the line to extract user details.
        if (sscanf(line, "%63[^|]|%15[^|]|%63[^|]|%d", funame, frole, fpass, &active) == 4) {
            // Check if the extracted details match the provided credentials and if the user is active.
            if (strcmp(frole, role) == 0 && strcmp(funame, username) == 0 &&
                strcmp(fpass, password) == 0 && active == 1) {
                found = true;
                break;
            }
        }
        // Clear the line buffer for the next read.
        memset(line, 0, sizeof(line));
    }

    close(fd);
    sem_post(file_sem);
    return found;
}