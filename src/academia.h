#ifndef ACADEMIA_H
#define ACADEMIA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

//===================Constants===================
#define BUFF_SIZE 1024
#define USER_FILE "data/users.txt"
#define COURSE_FILE "data/courses.txt"

//=================== Semaphore for file operations===================
extern sem_t *file_sem;

void init_file_semaphore();
void cleanup_file_semaphore();

// ================= Communication=================
void sender(int sock, const char *msg);
void receiver(int sock, char *buf, size_t buflen);

// =================Authentication=================
bool authenticate(const char *role, const char *username, const char *password);

// =================Admin Operations=================
void add_user(const char *role, int client_sock);
void view_users(const char *role, int client_sock);
void activate_deactivate_user(int client_sock, int desired_status);
void update_user(const char *role, int client_sock);
int process_admin(int client_sock, int choice);

// =================Faculty Operations=================
void add_course(int client_sock, const char *faculty);
void remove_course(int client_sock, const char *faculty);
void view_course_enrollments(int client_sock, const char *faculty);
void view_offering_courses(int client_sock, const char *faculty);
void update_course_details(int client_sock, const char *faculty);
int process_faculty(int client_sock, int choice, const char *username);

// =================Student Operations=================
void enroll_course(int client_sock, const char *student);
void unenroll_course(int client_sock, const char *student);
void view_enrolled_courses(int client_sock, const char *student);
void view_all_courses(int client_sock);
int process_student(int client_sock, int choice, const char *username);

// =================Utilities=================
void password_change(int client_sock, const char *username, const char *role);
ssize_t safe_read_line(int fd, char *buf, size_t maxlen);
void trim_newline(char *str);

#endif