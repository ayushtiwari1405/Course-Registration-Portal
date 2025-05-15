# Course Registration Portal

This project implements a basic academic management system with client-server architecture. It supports different user roles including students, faculty, and administrators, each with specific functionalities.

## Project Structure

The project has the following directory structure:

-   `src`:  Contains the C source files.
    -   `academia.h`:  Header file containing common definitions, structures, and function prototypes.
    -   `auth.c`:  Implements authentication and authorization.
    -   `client.c`:  Client-side application logic.
    -   `server.c`:  Server-side application logic.
    -   `comm.c`:  Client-server communication functions.
    -   `utilities.c`:  Utility functions.
    -   `student_ops.c`:  Student user operations.
    -   `faculty_ops.c`:  Faculty user operations.
    -   `admin_ops.c`:  Administrator user operations.
-   `data`:  Directory containing data files.
    -   `users.txt`:  Stores user information.
    -   `courses.txt`:  Stores course information.
-   `Makefile`:  Build file for compiling the project.

## Functionality (Based on File Names)

While the exact functionalities are within the source code, the project likely supports:

-   **Authentication:** Secure login for different user roles.
-   **User-Specific Operations:** Different sets of actions available to students, faculty, and administrators. Examples might include:
    -   **Students:** Viewing grades, enrolling in courses.
    -   **Faculty:** Managing courses, grading assignments.
    -   **Administrators:** User management, system configuration.
-   **Client-Server Interaction:** A client application to interact with a central server.

## How to Compile and Run

**(Note: These instructions assume you have `make` and a C compiler (like GCC) installed.)**

1.  **Compile the project:**
    -   Open your terminal and navigate to the root directory of the project (the directory containing the `src` folder, the `data` folder, and the `Makefile`).
    -   Run the `make` command:

        ```bash
        make
        ```

        This will use the `Makefile` to compile the source files and create the `server` and `client` executables.

2.  **Run the server:**

    ```bash
    ./server
    ```

    (The server might require specific command-line arguments, so check the `server.c` file or your `Makefile`.)

3.  **Run the client:** Open a new terminal and navigate to the root directory of the project:

    ```bash
    ./client <server_ip> <port>
    ```

    (The client will likely require the server's IP address and port number as command-line arguments. Check the `client.c` file.)