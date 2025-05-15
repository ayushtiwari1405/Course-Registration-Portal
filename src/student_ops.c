#include "academia.h"

#define BUFF_SIZE 1024

// Enrolls a student in a course by adding their name to the course's enrolled list.
void enroll_course(int client_sock, const char *student) {
    char course_id[32];
    sender(client_sock, "Enter course ID to enroll:");
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

    // Read and preserve header line
    if (safe_read_line(fd, lines[count], sizeof(lines[count])) > 0) {
        if (lines[count][strlen(lines[count]) - 1] != '\n') strcat(lines[count], "\n");
        count++;
    }

    while (safe_read_line(fd, lines[count], sizeof(lines[count]) - 2) > 0) {
        if (lines[count][0] == '\n' || lines[count][0] == '\0') continue;
        if (lines[count][strlen(lines[count]) - 1] != '\n') strcat(lines[count], "\n");

        char cid[32], cname[64], cfac[64], cseats[8], enrolled[256] = {0};
        sscanf(lines[count], "%31[^|]|%63[^|]|%63[^|]|%7[^|]|%255[^\n]", cid, cname, cfac, cseats, enrolled);

        if (strcmp(cid, course_id) == 0) {
            found = 1;

            // Check if already enrolled
            int already_enrolled = 0;
            char enrolled_copy[256];
            strncpy(enrolled_copy, enrolled, sizeof(enrolled_copy) - 1);
            enrolled_copy[sizeof(enrolled_copy) - 1] = '\0';

            char *tok = strtok(enrolled_copy, ",");
            while (tok) {
                if (strcmp(tok, student) == 0) {
                    already_enrolled = 1;
                    break;
                }
                tok = strtok(NULL, ",");
            }

            if (already_enrolled) {
                close(fd);
                sem_post(file_sem);
                sender(client_sock, "Already enrolled.");
                return;
            }

            // Append student to enrolled list
            char new_enrolled[512];
            if (strlen(enrolled) > 0)
                snprintf(new_enrolled, sizeof(new_enrolled), "%s,%s", enrolled, student);
            else
                snprintf(new_enrolled, sizeof(new_enrolled), "%s", student);

            snprintf(lines[count], sizeof(lines[count]), "%s|%s|%s|%s|%s\n", cid, cname, cfac, cseats, new_enrolled);
        }
        count++;
    }

    if (!found) {
        close(fd);
        sem_post(file_sem);
        sender(client_sock, "Course ID not found.");
        return;
    }

    // Truncate and write updated content
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    for (int i = 0; i < count; i++) {
        if (lines[i][0] != '\0') {
            if (lines[i][strlen(lines[i]) - 1] != '\n') strcat(lines[i], "\n");
            write(fd, lines[i], strlen(lines[i]));
        }
    }

    fsync(fd);
    close(fd);
    sem_post(file_sem);
    sender(client_sock, "Enrolled successfully.");
}

// Unenrolls a student from a course by removing their name from the enrolled list.
void unenroll_course(int client_sock, const char *student) {
    char course_id[32];
    sender(client_sock, "Enter course id to unenroll:");
    receiver(client_sock, course_id, sizeof(course_id));

    sem_wait(file_sem);
    int fd = open(COURSE_FILE, O_RDWR);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    char lines[100][1024];
    int count = 0, found = 0, changed = 0;
    char line[1024];
    int header_handled = 0;

    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        if (line[0] == '\n' || line[0] == '\0') continue;

        if (!header_handled) {
            if (strncmp(line, "CourseID", 8) == 0) {
                snprintf(lines[count++], sizeof(lines[0]), "%s\n", line);
            } else {
                snprintf(lines[count++], sizeof(lines[0]), "CourseID|CourseName|Faculty|Seats|EnrolledList\n");
            }
            header_handled = 1;
            if (strncmp(line, "CourseID", 8) == 0) continue;
        }

        char cid[32], cname[64], cfac[64], cseats[8], enrolled_list[256] = "";
        int fields = sscanf(line, "%31[^|]|%63[^|]|%63[^|]|%7[^|]|%255[^\n]",
                            cid, cname, cfac, cseats, enrolled_list);
        if (fields < 4) continue;

        if (strcmp(cid, course_id) == 0) {
            found = 1;
            char new_enrolled[256] = "";
            char enrolled_copy[256];
            strncpy(enrolled_copy, enrolled_list, sizeof(enrolled_copy));
            enrolled_copy[sizeof(enrolled_copy) - 1] = '\0';

            int first = 1, was_enrolled = 0;
            char *tok = strtok(enrolled_copy, ",");
            while (tok) {
                if (strcmp(tok, student) != 0) {
                    if (!first) strcat(new_enrolled, ",");
                    strcat(new_enrolled, tok);
                    first = 0;
                } else {
                    was_enrolled = 1;
                }
                tok = strtok(NULL, ",");
            }

            if (!was_enrolled) {
                close(fd);
                sem_post(file_sem);
                sender(client_sock, "Not enrolled in this course.");
                return;
            }

            snprintf(lines[count++], sizeof(lines[0]), "%s|%s|%s|%s|%s\n",
                     cid, cname, cfac, cseats, new_enrolled);
            changed = 1;
        } else {
            snprintf(lines[count++], sizeof(lines[0]), "%s\n", line);
        }
    }

    if (!found) {
        close(fd);
        sem_post(file_sem);
        sender(client_sock, "Course not found.");
        return;
    }

    if (!changed) {
        close(fd);
        sem_post(file_sem);
        sender(client_sock, "Not enrolled in this course.");
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

    sender(client_sock, "Unenrolled successfully.");
}

// Views the courses the student is currently enrolled in.
void view_enrolled_courses(int client_sock, const char *student) {
    sem_wait(file_sem);

    int fd = open(COURSE_FILE, O_RDONLY);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    char line[1024], msg[BUFF_SIZE] = "";
    lseek(fd, 0, SEEK_SET);
    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        if (strncmp(line, "CourseID", 8) == 0 || line[0] == '\n' || line[0] == '\0') continue;

        char cid[32], cname[64], cfac[64], cseats[8], enrolled_list[256] = "";
        sscanf(line, "%31[^|]|%63[^|]|%63[^|]|%7[^|]|%255[^\n]", cid, cname, cfac, cseats, enrolled_list);

        if (strlen(enrolled_list) > 0) {
            char enrolled_copy[256];
            strncpy(enrolled_copy, enrolled_list, sizeof(enrolled_copy) - 1);
            enrolled_copy[sizeof(enrolled_copy) - 1] = '\0';

            char *tok = strtok(enrolled_copy, ",");
            while (tok) {
                if (strcmp(tok, student) == 0) {
                    char tmp[256];
                    snprintf(tmp, sizeof(tmp), "%s (%s)\n", cname, cid);
                    strcat(msg, tmp);
                    break;
                }
                tok = strtok(NULL, ",");
            }
        }
    }

    close(fd);
    sem_post(file_sem);

    if (strlen(msg) == 0)
        sender(client_sock, "No enrolled courses.");
    else
        sender(client_sock, msg);
}

// Views all available courses in the COURSE_FILE.
void view_all_courses(int client_sock) {
    sem_wait(file_sem);

    int fd = open(COURSE_FILE, O_RDONLY);
    if (fd < 0) {
        sem_post(file_sem);
        sender(client_sock, "File error.");
        return;
    }

    char line[1024], msg[BUFF_SIZE] = "";
    lseek(fd, 0, SEEK_SET);
    while (safe_read_line(fd, line, sizeof(line)) > 0) {
        char cid[32], cname[64], cfac[64], cseats[8], enrolled[256];
        sscanf(line, "%31[^|]|%63[^|]|%63[^|]|%7[^|]|%255[^\n]", cid, cname, cfac, cseats, enrolled);
        char entry[256];
        snprintf(entry, sizeof(entry), "Course ID: %s | Name: %s | Faculty: %s | Seats: %s\n",
                 cid, cname, cfac, cseats);
        strcat(msg, entry);
    }

    close(fd);
    sem_post(file_sem);

    if (strlen(msg) == 0) sender(client_sock, "No courses available.");
    else sender(client_sock, msg);
}

// Processes the student's choice of action.
int process_student(int client_sock, int choice, const char *username) {
    switch (choice) {
        case 1: view_all_courses(client_sock); break;
        case 2: enroll_course(client_sock, username); break;
        case 3: unenroll_course(client_sock, username); break;
        case 4: view_enrolled_courses(client_sock, username); break;
        case 5: password_change(client_sock, username, "student"); break;
        case 6: sender(client_sock, "Logout successful. Goodbye."); return 0;
        default: sender(client_sock, "Invalid student choice."); break;
    }
    return 1;
}