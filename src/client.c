#include "academia.h"

#define BUFF_SIZE 1024
#define PORT 8080

void clear_input_buffer() {
    int c; while ((c = getchar()) != '\n' && c != EOF);
}

void read_non_empty_line(char *buffer, size_t size, const char *prompt) {
    do {
        printf("%s", prompt);
        fgets(buffer, size, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) == 0) {
            printf("Input cannot be empty. Please enter a valid value.\n");
        }
    } while (strlen(buffer) == 0);
}

void print_menu(const char *role) {
        printf("Client process ID: %d\n", getpid());
    if (strcmp(role, "admin") == 0) {
        printf("\n================Welcome To Admin Menu================\n");
        printf("1. Add Student\n2. View Student Details\n3. Add Faculty\n4. View Faculty Details\n5. Activate Student\n6. Block Student\n7. Modify Student Details\n8. Modify Faculty Details\n9. Logout and Exit\n");
    } else if (strcmp(role, "student") == 0) {
        printf("\n================Welcome To Student Menu================\n");
        printf("1. View All Course\n2. Enroll(pick) New Course\n3. Drop Course\n4. View Enrolled Course Details\n5. Change Password\n6. Logout and Exit\n");
    } else if (strcmp(role, "faculty") == 0) {
        printf("\n================Welcome To Faculty Menu================\n");
        printf("1. View Offering Courses\n2. Add New Course\n3. Remove Course From Catalog\n4. Modify Course Details\n5. Change Password\n6. Logout and Exit\n");
    }
}

void send_text(int sock, char *buf) {
    do {
        fgets(buf, BUFF_SIZE, stdin);
        buf[strcspn(buf, "\n")] = 0;
        if (strlen(buf) == 0) {
            printf("Input cannot be empty. Please enter a valid value:\n");
        }
    } while (strlen(buf) == 0);
    send(sock, buf, strlen(buf), 0);
}

void handle_multistep(int sock) {
    char buffer[BUFF_SIZE];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        printf("Server: %s\n", buffer);
        if (strncmp(buffer, "Enter", 5) == 0) {
            send_text(sock, buffer);
        } else {
            break;
        }
    }
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFF_SIZE];
    char role[16], username[64], password[64];
    int choice;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket error"); exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed"); exit(EXIT_FAILURE);
    }
    printf("============Welcome Back to Academia :: Course Registration System============\nLogin Type:\n");
    read_non_empty_line(role, sizeof(role), "Enter role (admin/student/faculty): ");
    read_non_empty_line(username, sizeof(username), "Enter username: ");
    read_non_empty_line(password, sizeof(password), "Enter password: ");

    snprintf(buffer, sizeof(buffer), "AUTH:%s:%s:%s", role, username, password);
    send(sock, buffer, strlen(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, sizeof(buffer), 0);

    if (strcmp(buffer, "AUTH_SUCCESS") != 0) {
        printf("Authentication failed. Exiting.\n");
        close(sock); return 1;
    }
    printf("Login successful!\n");

    int logout_choice = (strcmp(role, "admin") == 0) ? 9 : 6;
    
    while (1) {
        print_menu(role);
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        snprintf(buffer, sizeof(buffer), "MENU:%s:%d", role, choice);
        send(sock, buffer, strlen(buffer), 0);

        if (choice == logout_choice) {
            int n = recv(sock, buffer, sizeof(buffer), 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("\n%s\n", buffer);
            }
            printf("Exiting...\n");
            break;
        }

        int multistep = 0;
        if (strcmp(role, "admin") == 0 && (choice >= 1 && choice <= 8)) multistep = 1;
        if (strcmp(role, "student") == 0 && (choice >=1 && choice <=5)) multistep = 1;
        if (strcmp(role, "faculty") == 0 && (choice >=1 && choice <=5)) multistep = 1;

        if (multistep) {
            handle_multistep(sock);
        } else {
            memset(buffer, 0, sizeof(buffer));
            int n = recv(sock, buffer, sizeof(buffer), 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("Server: %s\n", buffer);
            }
        }
    }

    close(sock);
    return 0;
}
