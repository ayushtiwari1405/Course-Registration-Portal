#include "academia.h"

// Adds a new user (student or faculty) to the user database file.
void add_user(const char *role, int client_sock) {
    char username[64], password[64];
    sender(client_sock, "Enter username:");
    receiver(client_sock, username, sizeof(username));
    sender(client_sock, "Enter password:");
    receiver(client_sock, password, sizeof(password));
    //printf("Thread_id: %ld waiting for the lock\n", pthread_self());
    sem_wait(file_sem);
    //printf("Thread_id: %ld got the lock\n", pthread_self());

    int fd = open(USER_FILE, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    char line[256];
    lseek(fd, 0, SEEK_END);
    // Iterate through the existing users to check for duplicate usernames.
    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        char existing_name[64], existing_role[16], existing_pass[64];
        int active;
        if (sscanf(line, "%63[^|]|%15[^|]|%63[^|]|%d", existing_name, existing_role, existing_pass, &active) == 4) {
            if (strcmp(existing_name, username) == 0) {
                close(fd);
                sem_post(file_sem);
                sender(client_sock, "Username already exists.");
                return;
            }
        }
    }
    //printf("Going to sleep for 20 seconds\n");
    //sleep(20);
    //printf("Thread %d done sleeping\n", pthread_self());
    char new_entry[256];
    snprintf(new_entry, sizeof(new_entry), "\n%s|%s|%s|1", username, role, password);
    // Append the new user information to the file.
    write(fd, new_entry, strlen(new_entry));
    // Ensure the changes are written to disk immediately.
    fsync(fd);
    close(fd);

    sem_post(file_sem);
    sender(client_sock, "User added successfully.");
}

// Views the details of a specific user based on their role and username.
void view_users(const char *role, int client_sock) {
    sem_wait(file_sem);

    int fd = open(USER_FILE, O_RDONLY);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "Could not open user file.");
        return;
    }
    char username[64];
    sender(client_sock, "Enter username:");
    receiver(client_sock, username, sizeof(username));

    char line[256];
    int found = 0;
    lseek(fd, 0, SEEK_SET);
    // Read each line from the user file to find the matching user.
    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        char funame[64], frole[16], fpass[64];
        int active;
        if (sscanf(line, "%63[^|]|%15[^|]|%63[^|]|%d", funame, frole, fpass, &active) == 4) {
            // Check if the read user matches the requested role, username, and is active.
            if (strcmp(frole, role) == 0 && strcmp(funame, username) == 0 && active == 1) {
                char msg[128];
                snprintf(msg, sizeof(msg), "Username: %s\n, Role: %s\n, Status: %s\n", funame, frole, active ? "Active" : "Inactive");
                sender(client_sock, msg);
                found = 1;
            }
        }
    }

    close(fd);
    sem_post(file_sem);

    if (!found) {
        sender(client_sock, "No users found for this role.");
    }
}

// Activates or deactivates a student user based on the provided username.
void activate_deactivate_user(int client_sock, int desired_status) {
    char username[64];
    sender(client_sock, "Enter username:");
    receiver(client_sock, username, sizeof(username));

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

    // Read the header line if it exists.
    if (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        snprintf(lines[count], sizeof(lines[count]), "Username|Role|Password|Status\n");
        count++;
    }
    // Read all the user entries from the file.
    while (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        char funame[64], frole[16], fpass[64];
        int active;

        // Parse each line to find the target student user.
        if (sscanf(lines[count], "%63[^|]|%15[^|]|%63[^|]|%d", funame, frole, fpass, &active) == 4) {
            // If the user is a student and the username matches, update the status.
            if (strcmp(frole, "student") == 0 && strcmp(funame, username) == 0) {
                snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%d\n",
                         funame, frole, fpass, desired_status);
                found = 1;
            } else {
                // Keep the existing entry as is.
                snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%d\n",
                         funame, frole, fpass, active);
            }
        }
        count++;
    }

    // If the student user was not found.
    if (!found) {
        close(fd);
        sem_post(file_sem);
        sender(client_sock, "Student user not found.");
        return;
    }
    // Truncate the file to remove the old content.
    ftruncate(fd, 0);
    // Reset the file pointer to the beginning.
    lseek(fd, 0, SEEK_SET);
    // Write the modified user data back to the file.
    for (int i = 0; i < count; i++) {
        write(fd, lines[i], strlen(lines[i]));
    }
    // Ensure all writes are completed.
    fsync(fd);
    close(fd);

    sem_post(file_sem);
    sender(client_sock, desired_status == 1 ? "Student activated successfully." : "Student deactivated successfully.");
}

// Updates the password of a specific user based on their role and username.
void update_user(const char *role, int client_sock) {
    char username[64], newpass[64];
    sender(client_sock, "Enter username:");
    receiver(client_sock, username, sizeof(username));
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
    // Read the header line if it exists.
    if (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        snprintf(lines[count], sizeof(lines[count]), "Username|Role|Password|Status\n");
        count++;
    }
    // Read all user entries from the file.
    while (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        char funame[64], frole[16], fpass[64];
        int active;

        // Parse each line to find the target user.
        if (sscanf(lines[count], "%63[^|]|%15[^|]|%63[^|]|%d", funame, frole, fpass, &active) == 4) {
            // If the user's role and username match, update the password.
            if (strcmp(frole, role) == 0 && strcmp(funame, username) == 0) {
                snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%d\n",
                         funame, frole, newpass, active);
                found = 1;
            } else {
                // Keep the existing entry as is.
                snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%d\n",
                         funame, frole, fpass, active);
            }
        }
        count++;
    }

    // If the user was not found.
    if (!found) {
        close(fd);
        sem_post(file_sem);
        sender(client_sock, "User not found.");
        return;
    }
    // Truncate the file.
    ftruncate(fd, 0);
    // Reset the file pointer.
    lseek(fd, 0, SEEK_SET);
    // Write the updated user data back to the file.
    for (int i = 0; i < count; i++) {
        write(fd, lines[i], strlen(lines[i]));
    }
    // Ensure data is written to disk.
    fsync(fd);
    close(fd);

    sem_post(file_sem);
    sender(client_sock, "User updated.");
}

// Processes the admin's choice of action.
int process_admin(int client_sock, int choice) {
    switch (choice) {
        case 1: add_user("student", client_sock); break;
        case 2: view_users("student", client_sock); break;
        case 3: add_user("faculty", client_sock); break;
        case 4: view_users("faculty", client_sock); break;
        case 5: activate_deactivate_user(client_sock, 1); break;
        case 6: activate_deactivate_user(client_sock, 0); break;
        case 7: update_user("student", client_sock); break;
        case 8: update_user("faculty", client_sock); break;
        case 9: sender(client_sock, "Logout successful. Goodbye."); return 0;
        default: sender(client_sock, "Invalid admin choice."); break;
    }
    return 1;
}