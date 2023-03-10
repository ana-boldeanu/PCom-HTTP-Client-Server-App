#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

int main(int argc, char *argv[]) {
    char *message = NULL;          // Mesajul HTTP trimis catre server
    char *response = NULL;         // Raspunsul HTTP de la server
    int sockfd;                    // Socketul pe care se conecteaza clientul la server
    char *client_command = NULL;   // Comanda data de utilizator (citita de la tastatura)
    char *login_cookie = NULL;     // Cookie-ul intors de server dupa autentificarea cu succes
    char *token_JWT = NULL;        // Token-ul JWT intors de server la accesarea librariei cu succes
    int client_logged_in = 0;      // Statusul curent al utilizatorului (1 = autentificat)
    int client_authorized = 0;     // Permisiile de acces la librarie (1 = tokenul JWT a fost primit)

    while (1) {
        // Deschide conexiunea la server
        sockfd = open_connection(HOST_IP, HTTP_PORT, AF_INET, SOCK_STREAM, 0);

        fprintf(stdout, "Your command?\n");

        // Citeste o comanda de la tastatura
        client_command = calloc(COMMAND_LENGTH, sizeof(char));
        fgets(client_command, COMMAND_LENGTH, stdin);
        client_command[strlen(client_command) - 1] = '\0';

        // Verifica validitatea ei
        if (!is_valid_command(client_command)) {
            printf("ERROR: That command does not exit.\n");
            printf("Available commands: register, login, enter_library, get_books, ");
            printf("get_book, add_book, delete_book, logout, exit.\n\n");
            close(sockfd);
            continue;
        }

        
        // ========================================================================================
        // === Register ===========================================================================
        // ========================================================================================
        if (strcmp(client_command, "register") == 0) {
            // Clientul trebuie sa nu fie deja logat ca sa poata crea un cont nou
            if (client_logged_in) {
                fprintf(stdout, 
                       "ERROR. You are already logged in! Logout to register a new account.\n\n");
                close(sockfd);
                continue;
            }

            // Citeste datele utilizatorului
            char *username, *password;
            get_userdata(&username, &password);

            // Creeaza stringul JSON ce contine username si password
            char *userdata_string = create_userdata_json_string(username, password);

            // Creeaza mesajul care trebuie trimis serverului
            char **payload = &userdata_string;
            int nr_payload_fields = 1;
            message = compute_post_request(HOST_ADDR, PATH_REGISTER, PAYLOAD_TYPE, token_JWT, 
                                           payload, nr_payload_fields, NULL, 0);

            // Trimite mesajul
            send_to_server(sockfd, message);

            // Primeste raspunsul
            response = receive_from_server(sockfd);
            
            // Pastreaza o copie a raspunsului, pentru a o folosi cu strtok
            char *response_copy = calloc(strlen(response) + 1, sizeof(char));
            strcpy(response_copy, response);

            // Extrage statusul raspunsului
            char *status_code = strtok(response_copy, " \n");
            status_code = strtok(NULL, " \n");

            // Utilizator inregistrat cu succes
            if (status_code[0] == '2') {
                fprintf(stdout, "%s - OK. User registered successfully! Welcome ^_^\n\n", status_code);

            // Inregistrarea a esuat, afiseaza eroarea
            } else {
                fprintf(stdout, "%s - ", status_code);
                show_error(response);
            }

            free(userdata_string);
            free(username); free(password); free(response_copy);
        }


        // ========================================================================================
        // === Login =============================================================================
        // ========================================================================================
        if (strcmp(client_command, "login") == 0) {
            // Verifica daca utilizatorul este deja logat
            if (client_logged_in) {
                fprintf(stdout, "ERROR. User already logged in!\n\n");
                client_logged_in = 1;
                close(sockfd);
                continue;
            }

            // Citeste datele utilizatorului
            char *username, *password;
            get_userdata(&username, &password);

            // Creeaza stringul JSON ce contine username si password
            char *userdata_string = create_userdata_json_string(username, password);

            // Creeaza mesajul care trebuie trimis serverului
            char **payload = &userdata_string;
            int nr_payload_fields = 1;
            message = compute_post_request(HOST_ADDR, PATH_LOGIN, PAYLOAD_TYPE, 
                                          token_JWT, payload, nr_payload_fields, NULL, 0);

            // Trimite mesajul
            send_to_server(sockfd, message);

            // Primeste raspunsul
            response = receive_from_server(sockfd);

            // Pastreaza o copie a raspunsului, pentru a o folosi cu strtok
            char *response_copy = calloc(strlen(response) + 1, sizeof(char));
            strcpy(response_copy, response);

            // Extrage statusul raspunsului
            char *status_code = strtok(response_copy, " \n");
            status_code = strtok(NULL, " \n");

            // Utilizator logat cu succes
            if (status_code[0] == '2') {
                fprintf(stdout, "%s - OK. User logged in successfully!\n\n", status_code);

                // Retine faptul ca acest utilizator este logat
                client_logged_in = 1;

                // Din raspuns, extrage cookie-ul de autentificare
                char *set_cookie = strstr(response, "connect.sid");
                login_cookie = strtok(set_cookie, ": ;");

            // Autentificarea a esuat, extrage mesajul de eroare
            } else {
                fprintf(stdout, "%s - ", status_code);
                show_error(response);
            }

            free(userdata_string);
            free(username); free(password); free(response_copy);
        }


        // ========================================================================================
        // === Acces ==============================================================================
        // ========================================================================================
        if (strcmp(client_command, "enter_library") == 0) {
            if (!client_logged_in) {
                fprintf(stdout, "ERROR. You are not logged in!\n\n");
                close(sockfd);
                continue;
            }

            // Creeaza mesajul care trebuie trimis serverului
            char **cookies = &login_cookie;
            int nr_cookies = login_cookie == NULL ? 0 : 1;
            message = compute_get_request(HOST_ADDR, PATH_ACCESS, NULL, 
                                          token_JWT, cookies, nr_cookies);

            // Trimite mesajul
            send_to_server(sockfd, message);

            // Primeste raspunsul
            response = receive_from_server(sockfd);

            // Pastreaza o copie a raspunsului, pentru a o folosi cu strtok
            char *response_copy = calloc(strlen(response) + 1, sizeof(char));
            strcpy(response_copy, response);

            // Extrage statusul raspunsului
            char *status_code = strtok(response_copy, " \n");
            status_code = strtok(NULL, " \n");

            // Accesul bibliotecii s-a realizat cu succes
            if (status_code[0] == '2') {
                fprintf(stdout, "%s - OK. You're in.\n\n", status_code);

                // Retine faptul ca utilizatorul a primit acces
                client_authorized = 1;

                // Din raspuns, extrage token-ul JWT
                char *response_body = strstr(response, "{");
                if (response_body) {
                    token_JWT = strtok(response_body, "{\"}:");
                    token_JWT = strtok(NULL, "{\"}:");
                }

            // Raspunsul contine o eroare, pe care o afisez
            } else {
                fprintf(stdout, "%s - ", status_code);
                show_error(response);
            }

            free(response_copy);
        }


        // ========================================================================================
        // === Get Books ==========================================================================
        // ========================================================================================
        if (strcmp(client_command, "get_books") == 0) {
            if (!client_logged_in) {
                fprintf(stdout, "ERROR. You are not logged in!\n\n");
                close(sockfd);
                continue;
            }

            // Creeaza mesajul care trebuie trimis serverului
            message = compute_get_request(HOST_ADDR, PATH_BOOKS, NULL, token_JWT, NULL, 0);

            // Trimite mesajul
            send_to_server(sockfd, message);

            // Primeste raspunsul
            response = receive_from_server(sockfd);

            // Pastreaza o copie a raspunsului, pentru a o folosi cu strtok
            char *response_copy = calloc(strlen(response) + 1, sizeof(char));
            strcpy(response_copy, response);

            // Extrage statusul raspunsului
            char *status_code = strtok(response_copy, " \n");
            status_code = strtok(NULL, " \n");

            // Accesul bibliotecii s-a realizat cu succes
            if (status_code[0] == '2') {
                fprintf(stdout, "%s - OK. Reading time...\n\n", status_code);

                // Extrage lista de carti si afiseaz-o
                char *response_body = strstr(response, "{");
                if (response_body == NULL) {
                    fprintf(stdout, "Oops! This library is empty!\n\n");

                } else {
                    fprintf(stdout, "%s\n\n", response_body);
                }

            // Raspunsul contine o eroare, pe care o afisez
            } else {
                fprintf(stdout, "%s - ", status_code);
                show_error(response);
            }

            free(response_copy);
        }


        // ========================================================================================
        // === Get Book ===========================================================================
        // ========================================================================================
        if (strcmp(client_command, "get_book") == 0) {
            if (!client_logged_in) {
                fprintf(stdout, "ERROR. You are not logged in!\n\n");
                close(sockfd);
                continue;
            }

            fprintf(stdout, "id=");

            // Extrage ID-ul cartii
            char *bookID = calloc(ID_LENGTH, sizeof(char));
            fgets(bookID, BOOK_DATA_LENGTH, stdin);
            bookID[strlen(bookID) - 1] = '\0';

            // Verifica faptul ca ID-ul este un numar
            while ((strcmp(bookID, "0") != 0 && atoi(bookID) == 0)) {
                fprintf(stdout, "ERROR. ID must be a number. Try again?\n");
                fprintf(stdout, "id=");
                fgets(bookID, BOOK_DATA_LENGTH, stdin);
                bookID[strlen(bookID) - 1] = '\0';
            }

            // Construieste stringul ce contine calea pentru aceasta carte
            int path_size = strlen(PATH_BOOK_ID) + ID_LENGTH + 1;
            char *book_path = calloc(path_size, sizeof(char));
            strcpy(book_path, PATH_BOOK_ID);
            strcat(book_path, bookID);

            // Creeaza mesajul care trebuie trimis serverului
            message = compute_get_request(HOST_ADDR, book_path, NULL, token_JWT, NULL, 0);

            // Trimite mesajul
            send_to_server(sockfd, message);

            // Primeste raspunsul
            response = receive_from_server(sockfd);

            // Pastreaza o copie a raspunsului, pentru a o folosi cu strtok
            char *response_copy = calloc(strlen(response) + 1, sizeof(char));
            strcpy(response_copy, response);

            // Extrage statusul raspunsului
            char *status_code = strtok(response_copy, " \n");
            status_code = strtok(NULL, " \n");

            // Accesul bibliotecii s-a realizat cu succes si cartea exista
            if (status_code[0] == '2') {
                fprintf(stdout, "%s - OK. Here's your book!\n\n", status_code);

                // Extrage cartea
                char *response_body = strstr(response, "{");
                fprintf(stdout, "%s\n\n", response_body);

            // Raspunsul contine o eroare, pe care o afisez
            } else {
                fprintf(stdout, "%s - ", status_code);
                show_error(response);
            }

            free(bookID); free(book_path); free(response_copy);
        }


        // ========================================================================================
        // === Add Book ===========================================================================
        // ========================================================================================
        if (strcmp(client_command, "add_book") == 0) {
            if (!client_logged_in) {
                fprintf(stdout, "ERROR. You are not logged in!\n\n");
                close(sockfd);
                continue;
            }

            if (!client_authorized) {
                fprintf(stdout, "ERROR. You are not authorized! >_< Try enter_library?\n\n");
                close(sockfd);
                continue;
            }

            // Citeste datele despre noua carte
            char *title, *author, *genre, *publisher; int page_count;
            get_bookdata(&title, &author, &genre, &publisher, &page_count);

            // Creeaza stringul JSON ce contine informatii despre carte
            char *bookdata_string = create_bookdata_json_string(title, author, 
                                                            genre, publisher, page_count);

            // Creeaza mesajul care trebuie trimis serverului
            char **payload = &bookdata_string;
            int nr_payload_fields = 1;
            message = compute_post_request(HOST_ADDR, PATH_BOOKS, PAYLOAD_TYPE, 
                                           token_JWT, payload, nr_payload_fields, NULL, 0);

            // Trimite mesajul
            send_to_server(sockfd, message);

            // Primeste raspunsul
            response = receive_from_server(sockfd);

            // Pastreaza o copie a raspunsului, pentru a o folosi cu strtok
            char *response_copy = calloc(strlen(response) + 1, sizeof(char));
            strcpy(response_copy, response);

            // Extrage statusul raspunsului
            char *status_code = strtok(response_copy, " \n");
            status_code = strtok(NULL, " \n");

            // Accesul bibliotecii s-a realizat cu succes si cartea exista
            if (status_code[0] == '2') {
                fprintf(stdout, "%s - OK. Interesting choice, I added it.\n\n", status_code);

            // Raspunsul contine o eroare, pe care o afisez
            } else {
                fprintf(stdout, "%s - ", status_code);
                if (strcmp(status_code, "429") == 0) {
                    fprintf(stdout, "ERROR. Too many requests!\n\n");
                } else {
                    show_error(response);
                }
            }

            free(title); free(author); free(genre);
            free(publisher); free(response_copy); free(bookdata_string);
        }


        // ========================================================================================
        // === Delete Book ========================================================================
        // ========================================================================================
        if (strcmp(client_command, "delete_book") == 0) {
            if (!client_logged_in) {
                fprintf(stdout, "ERROR. You are not logged in!\n\n");
                close(sockfd);
                continue;
            }

            // Extrage ID-ul cartii
            fprintf(stdout, "id=");

            char *bookID = calloc(ID_LENGTH, sizeof(char));
            fgets(bookID, BOOK_DATA_LENGTH, stdin);
            bookID[strlen(bookID) - 1] = '\0';

            // Verifica faptul ca ID-ul este un numar
            while ((strcmp(bookID, "0") != 0 && atoi(bookID) == 0)) {
                fprintf(stdout, "ERROR. ID must be a number. Try again?\n");
                fprintf(stdout, "id=");
                fgets(bookID, BOOK_DATA_LENGTH, stdin);
                bookID[strlen(bookID) - 1] = '\0';
            }

            // Construieste stringul ce contine calea pentru aceasta carte
            int path_size = strlen(PATH_BOOK_ID) + ID_LENGTH + 1;
            char *book_path = calloc(path_size, sizeof(char));
            strcpy(book_path, PATH_BOOK_ID);
            strcat(book_path, bookID);

            // Creeaza mesajul care trebuie trimis serverului
            message = compute_delete_request(HOST_ADDR, book_path, token_JWT);

            // Trimite mesajul
            send_to_server(sockfd, message);

            // Primeste raspunsul
            response = receive_from_server(sockfd);
            
            // Pastreaza o copie a raspunsului, pentru a o folosi cu strtok
            char *response_copy = calloc(strlen(response) + 1, sizeof(char));
            strcpy(response_copy, response);

            // Extrage statusul raspunsului
            char *status_code = strtok(response_copy, " \n");
            status_code = strtok(NULL, " \n");

            // Accesul bibliotecii s-a realizat cu succes si cartea exista
            if (status_code[0] == '2') {
                fprintf(stdout, "%s - OK. It was too old anyway, I removed it.\n\n", status_code);

            // Raspunsul contine o eroare, pe care o afisez
            } else {
                fprintf(stdout, "%s - ", status_code);
                show_error(response);
            }

            free(bookID); free(book_path); free(response_copy);
        }


        // ========================================================================================
        // === Logout =============================================================================
        // ========================================================================================
        if (strcmp(client_command, "logout") == 0) {
            if (!client_logged_in) {
                fprintf(stdout, "ERROR. You are not logged in!\n\n");
                close(sockfd);
                continue;
            }

            // Creeaza mesajul care trebuie trimis serverului
            char **cookies = &login_cookie;
            int nr_cookies = login_cookie == NULL ? 0 : 1;
            message = compute_get_request(HOST_ADDR, PATH_LOGOUT, NULL, 
                                         token_JWT, cookies, nr_cookies);

            // Trimite mesajul
            send_to_server(sockfd, message);

            // Primeste raspunsul
            response = receive_from_server(sockfd);

            // Pastreaza o copie a raspunsului, pentru a o folosi cu strtok
            char *response_copy = calloc(strlen(response) + 1, sizeof(char));
            strcpy(response_copy, response);

            // Extrage statusul raspunsului
            char *status_code = strtok(response_copy, " \n");
            status_code = strtok(NULL, " \n");

            // Utilizator delogat cu succes
            if (status_code[0] == '2') {
                fprintf(stdout, "%s - OK. User logged out successfully. See you later~\n\n", 
                                status_code);
                token_JWT = NULL;
                client_logged_in = 0;
                client_authorized = 0;

            // A avut loc o eroare
            } else {
                fprintf(stdout, "%s - ", status_code);
                show_error(response);
            }

            free(response_copy);
        }


        // ========================================================================================
        // === Exit ===============================================================================
        // ========================================================================================
        if (strcmp(client_command, "exit") == 0) {
            free(client_command); client_command = NULL;
            close(sockfd);
            break;

        } else {
            if (message) free(message);
            if (response) free(response);
        }

        if (client_command) free(client_command);
        close(sockfd);
    }
    
    return 0;
}
