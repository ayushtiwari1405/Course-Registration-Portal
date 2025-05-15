#include "academia.h"

#define BUFF_SIZE 1024
#define PORT 8080

// Clears the input buffer to prevent issues with subsequent reads.
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Reads a non-empty line of input from the user. Prompts the user until a non-empty input is provided.
void read_non_empty_line(char *buffer, size_t size, const char *prompt) {
    do {
        printf("%s", prompt);
        fgets(buffer, size, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove trailing newline
        if (strlen(buffer) == 0) {
            printf("Input cannot be empty. Please enter a valid value.\n");
        }
    } while (strlen(buffer) == 0);
}

// Prints the menu options based on the user's role.
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

// Reads a line of text from the user (ensuring it's not empty) and sends it to the server.
void send_text(int sock, char *buf) {
    do {
        fgets(buf, BUFF_SIZE, stdin);
        buf[strcspn(buf, "\n")] = 0; // Remove trailing newline
        if (strlen(buf) == 0) {
            printf("Input cannot be empty. Please enter a valid value:\n");
        }
    } while (strlen(buf) == 0);
    send(sock, buf, strlen(buf), 0);
}

// Handles multi-step interactions with the server, such as when the server prompts for more input.
void handle_multistep(int sock) {
    char buffer[BUFF_SIZE];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer), 0);
        if (n <= 0) break; // Connection closed or error
        buffer[n] = '\0';
        printf("Server: %s\n", buffer);
        if (strncmp(buffer, "Enter", 5) == 0) {
            send_text(sock, buffer);
        } else {
            break; // No more prompts from the server
        }
    }
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFF_SIZE];
    char role[16], username[64], password[64];
    int choice;

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    // Convert IPv4 or IPv6 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }
    printf("============Welcome Back to Academia :: Course Registration System============\nLogin Type:\n");
    read_non_empty_line(role, sizeof(role), "Enter role (admin/student/faculty): ");
    read_non_empty_line(username, sizeof(username), "Enter username: ");
    read_non_empty_line(password, sizeof(password), "Enter password: ");

    // Send authentication information to the server
    snprintf(buffer, sizeof(buffer), "AUTH:%s:%s:%s", role, username, password);
    send(sock, buffer, strlen(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
    // Receive authentication response from the server
    recv(sock, buffer, sizeof(buffer), 0);

    // Check if authentication was successful
    if (strcmp(buffer, "AUTH_SUCCESS") != 0) {
        printf("Authentication failed. Exiting.\n");
        close(sock);
        return 1;
    }
    printf("Login successful!\n");

    // Determine the logout choice based on the user's role
    int logout_choice = (strcmp(role, "admin") == 0) ? 9 : 6;

    // Main loop for interacting with the server
    while (1) {
        print_menu(role);
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        // Send the menu choice to the server
        snprintf(buffer, sizeof(buffer), "MENU:%s:%d", role, choice);
        send(sock, buffer, strlen(buffer), 0);

        // Handle logout
        if (choice == logout_choice) {
            int n = recv(sock, buffer, sizeof(buffer), 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("\n%s\n", buffer);
            }
            printf("Exiting...\n");
            break;
        }

        // Determine if the current choice requires multi-step interaction
        int multistep = 0;
        if (strcmp(role, "admin") == 0 && (choice >= 1 && choice <= 8)) multistep = 1;
        if (strcmp(role, "student") == 0 && (choice >= 1 && choice <= 5)) multistep = 1;
        if (strcmp(role, "faculty") == 0 && (choice >= 1 && choice <= 5)) multistep = 1;

        // Handle multi-step or single-step responses from the server
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

    // Close the socket
    close(sock);
    return 0;
}