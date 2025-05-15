#include "academia.h"

// Adds a new course to the COURSE_FILE.
void add_course(int client_sock, const char *faculty) {
    char course_id[32], course_name[64], seats[8];

    // Get inputs
    sender(client_sock, "Enter course id:");
    receiver(client_sock, course_id, sizeof(course_id));
    trim_newline(course_id);

    sender(client_sock, "Enter course name:");
    receiver(client_sock, course_name, sizeof(course_name));
    trim_newline(course_name);

    sender(client_sock, "Enter total seats:");
    receiver(client_sock, seats, sizeof(seats));
    trim_newline(seats);

    // Lock access
    sem_wait(file_sem);

    int fd = open(COURSE_FILE, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    // Write header if file is empty
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == 0) {
        const char *header = "CourseID|CourseName|Faculty|Seats|EnrolledList\n";
        write(fd, header, strlen(header));
    }

    // Check for duplicate course ID
    lseek(fd, 0, SEEK_SET);
    char line[512];
    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        if (line[0] == '\n' || line[0] == '\0' || strncmp(line, "CourseID", 8) == 0)
            continue;

        char existing_id[64];
        if (sscanf(line, "%63[^|]|", existing_id) == 1) {
            if (strcmp(existing_id, course_id) == 0) {
                close(fd);
                sem_post(file_sem);
                sender(client_sock, "Course ID already exists.");
                return;
            }
        }
    }

    // Add new course with empty enrolled list
    lseek(fd, 0, SEEK_END);
    char entry[512];
    snprintf(entry, sizeof(entry), "%s|%s|%s|%s|%s\n", course_id, course_name, faculty, seats, "");
    write(fd, entry, strlen(entry));
    fsync(fd);
    close(fd);

    sem_post(file_sem);
    sender(client_sock, "Course added.");
}

// Removes a course from the COURSE_FILE.
void remove_course(int client_sock, const char *faculty) {
    char course_id[32];
    sender(client_sock, "Enter course id to remove:");
    receiver(client_sock, course_id, sizeof(course_id));

    sem_wait(file_sem);

    int fd = open(COURSE_FILE, O_RDWR);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    char lines[100][1024];
    int count = 0, found = 0;

    // Preserve header
    if (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        snprintf(lines[count], sizeof(lines[count]), "CourseID|CourseName|Faculty|Seats|EnrolledList\n");
        count++;
    }

    while (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        char cid[32], cname[64], cfac[64], cseats[8], enrolled[256] = {0};
        sscanf(lines[count], "%31[^|]|%63[^|]|%63[^|]|%7[^|]|%255[^\n]", cid, cname, cfac, cseats, enrolled);

        if (strcmp(cid, course_id) == 0 && strcmp(cfac, faculty) == 0) {
            found = 1;
            continue;
        }

        snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%s|%s\n", cid, cname, cfac, cseats, enrolled);
        count++;
    }

    if (!found) {
        close(fd);
        sem_post(file_sem);
        sender(client_sock, "Course not found or not owned by you.");
        return;
    }

    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    for (int i = 0; i < count; i++) {
        write(fd, lines[i], strlen(lines[i]));
    }

    fsync(fd);
    close(fd);
    sem_post(file_sem);
    sender(client_sock, "Course removed.");
}

// Updates the details of an existing course in COURSE_FILE.
void update_course_details(int client_sock, const char *faculty) {
    char course_id[32], new_name[64], new_seats[8];
    sender(client_sock, "Enter course id to update:");
    receiver(client_sock, course_id, sizeof(course_id));
    sender(client_sock, "Enter new course name:");
    receiver(client_sock, new_name, sizeof(new_name));
    sender(client_sock, "Enter new total seats:");
    receiver(client_sock, new_seats, sizeof(new_seats));

    sem_wait(file_sem);

    int fd = open(COURSE_FILE, O_RDWR);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    char lines[100][1024];
    int count = 0, found = 0;

    // Preserve header
    if (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        snprintf(lines[count], sizeof(lines[count]), "CourseID|CourseName|Faculty|Seats|EnrolledList\n");
        count++;
    }

    while (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        char cid[32], cname[64], cfac[64], cseats[8], enrolled[512] = "";
        sscanf(lines[count], "%31[^|]|%63[^|]|%63[^|]|%7[^|]|%511[^\n]", cid, cname, cfac, cseats, enrolled);

        if (strcmp(cid, course_id) == 0 && strcmp(cfac, faculty) == 0) {
            found = 1;
            snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%s|%s\n", cid, new_name, faculty, new_seats, enrolled);
        } else {
            snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%s|%s\n", cid, cname, cfac, cseats, enrolled);
        }

        count++;
    }

    if (!found) {
        close(fd);
        sem_post(file_sem);
        sender(client_sock, "Course not found or not owned by you.");
        return;
    }

    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    for (int i = 0; i < count; i++) {
        write(fd, lines[i], strlen(lines[i]));
    }
    fsync(fd);
    close(fd);

    sem_post(file_sem);
    sender(client_sock, "Course details updated.");
}

// Views the courses offered by the logged-in faculty.
void view_offering_courses(int client_sock, const char *faculty) {
    sem_wait(file_sem);

    int fd = open(COURSE_FILE, O_RDONLY);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    char line[1024], msg[BUFF_SIZE] = "";
    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        // Skip header and empty lines
        if (strncmp(line, "CourseID", 8) == 0 || line[0] == '\n' || line[0] == '\0')
            continue;

        char cid[32], cname[64], cfac[64], cseats[8], enrolled_list[256] = "";

        int fields = sscanf(line, "%31[^|]|%63[^|]|%63[^|]|%7[^|]|%255[^\n]",
                            cid, cname, cfac, cseats, enrolled_list);

        if (fields >= 4 && strcmp(cfac, faculty) == 0) {
            char entry[512];
            snprintf(entry, sizeof(entry), "Course ID: %s | Name: %s | Seats: %s | Enrolled: %s\n",
                     cid, cname, cseats, strlen(enrolled_list) > 0 ? enrolled_list : "None");
            strcat(msg, entry);
        }
    }

    close(fd);
    sem_post(file_sem);

    sender(client_sock, strlen(msg) > 0 ? msg : "No offering courses.");
}

// Processes the faculty's choice of action.
int process_faculty(int client_sock, int choice, const char *username) {
    switch (choice) {
        case 1: view_offering_courses(client_sock, username); break;
        case 2: add_course(client_sock, username); break;
        case 3: remove_course(client_sock, username); break;
        case 4: update_course_details(client_sock, username); break;
        case 5: password_change(client_sock, username, "faculty"); break;
        case 6: sender(client_sock, "Logout successful. Goodbye."); return 0;
        default: sender(client_sock, "Invalid option. Try again."); break;
    }
    return 1;
}