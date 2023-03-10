#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

int is_valid_command(char *command) {
    if (strcmp(command, "register") == 0) return 1;
    if (strcmp(command, "login") == 0) return 1;
    if (strcmp(command, "enter_library") == 0) return 1;
    if (strcmp(command, "get_books") == 0) return 1;
    if (strcmp(command, "get_book") == 0) return 1;
    if (strcmp(command, "add_book") == 0) return 1;
    if (strcmp(command, "delete_book") == 0) return 1;
    if (strcmp(command, "logout") == 0) return 1;
    if (strcmp(command, "exit") == 0) return 1;
    return 0;
}

char *create_userdata_json_string(char *username, char *password) {
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);
    char *json_string;

    json_object_set_string(json_object, "username", username);
    json_object_set_string(json_object, "password", password);

    json_string = json_serialize_to_string_pretty(json_value);

    json_value_free(json_value);

    return json_string;
}


char *create_bookdata_json_string(char *title, char *author, char *genre, char *publisher, int page_count) {
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);
    char *json_string;

    json_object_set_string(json_object, "title", title);
    json_object_set_string(json_object, "author", author);
    json_object_set_string(json_object, "genre", genre);
    json_object_set_string(json_object, "publisher", publisher);
    json_object_set_number(json_object, "page_count", page_count);

    json_string = json_serialize_to_string_pretty(json_value);

    json_value_free(json_value);

    return json_string;
}

void show_error(char *response) {
    char *response_body = strstr(response, "{");
    if (response_body) {
        char *error_message = strtok(response_body, "{\"}:");
        error_message = strtok(NULL, "{\"}:");
        if (strstr(error_message, "Authorization header is missing")) {
            fprintf(stdout, "ERROR. You are not authorized! >_< Try enter_library?\n\n");

        } else if (strstr(error_message, "No book was")) {
            fprintf(stdout, "ERROR. That book doesn't exist!\n\n");
        } else {
            fprintf(stdout, "ERROR. %s\n\n", error_message);
        }
    }
}

void get_userdata(char **username, char **password) {
    *username = calloc(USERNAME_LENGTH, sizeof(char));
    *password = calloc(PASSWORD_LENGTH, sizeof(char));

    fprintf(stdout, "username=");
    fgets(*username, BOOK_DATA_LENGTH, stdin);
    (*username)[strlen(*username) - 1] = '\0';

    fprintf(stdout, "password=");
    fgets(*password, BOOK_DATA_LENGTH, stdin);
    (*password)[strlen(*password) - 1] = '\0';
}

void get_bookdata(char **title, char **author, char **genre, char **publisher,
                  int *page_count) {
    *title = calloc(BOOK_DATA_LENGTH, sizeof(char));
    *author = calloc(BOOK_DATA_LENGTH, sizeof(char));
    *genre = calloc(BOOK_DATA_LENGTH, sizeof(char));
    *publisher = calloc(BOOK_DATA_LENGTH, sizeof(char));
    char *page_count_str = calloc(BOOK_DATA_LENGTH, sizeof(char));

    fprintf(stdout, "title=");
    fgets(*title, BOOK_DATA_LENGTH, stdin);
    (*title)[strlen(*title) - 1] = '\0';

    fprintf(stdout, "author=");
    fgets(*author, BOOK_DATA_LENGTH, stdin);
    (*author)[strlen(*author) - 1] = '\0';

    fprintf(stdout, "genre=");
    fgets(*genre, BOOK_DATA_LENGTH, stdin);
    (*genre)[strlen(*genre) - 1] = '\0';

    fprintf(stdout, "publisher=");
    fgets(*publisher, BOOK_DATA_LENGTH, stdin);
    (*publisher)[strlen(*publisher) - 1] = '\0';
    
    fprintf(stdout, "page_count=");
    fgets(page_count_str, BOOK_DATA_LENGTH, stdin);
    page_count_str[strlen(page_count_str) - 1] = '\0';
    *page_count = atoi(page_count_str);

    // Verifica faptul ca page_count-ul este un numar
    while (strcmp(page_count_str, "0") != 0 && *page_count == 0) {
        fprintf(stdout, "ERROR. Page count must be a number. Try again?\n");
        fprintf(stdout, "page_count=");
        fgets(page_count_str, BOOK_DATA_LENGTH, stdin);
        page_count_str[strlen(page_count_str) - 1] = '\0';
        *page_count = atoi(page_count_str);
    }

    free(page_count_str);
}

char *compute_get_request(char *host, char *url, char *query_params, char *jwt_auth,
                            char **cookies, int cookies_count) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add Authorization header if available
    if (jwt_auth) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", jwt_auth);
        compute_message(message, line);
    }

    // Add headers and/or cookies, according to the protocol format
    memset(line, 0, LINELEN);    
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");

        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) {
                strcat(line, "; ");
            }
        }

        compute_message(message, line);   
    }

    // Add final new line
    compute_message(message, "");

    free(line);

    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char *jwt_auth, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    //Add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    /* Add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    int len = 0;
    for (int i = 0; i < body_data_fields_count; i++) {
        len = len + strlen(body_data[i]);
        if (i != body_data_fields_count - 1) {
            len++;
        }
    }

    sprintf(line, "Content-Length: %d", len);
    compute_message(message, line);

    // Add Authorization header if available
    if (jwt_auth) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", jwt_auth);
        compute_message(message, line);
    }

    // Add cookies
    memset(line, 0, LINELEN);
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        
        for (int i = 0; i < cookies_count; i++) {
                strcat(line, cookies[i]);
                if (i != cookies_count - 1) {
                    strcat(line, "; ");
                }
        }
        compute_message(message, line);   
    }

    // Add new line at end of header
    compute_message(message, "");

    // Add the actual payload data
    memset(line, 0, LINELEN);
    for (int i = 0; i < body_data_fields_count; i++) {
        strcat(body_data_buffer, body_data[i]);
        if (i != body_data_fields_count - 1) {
            strcat(body_data_buffer, "&");
        }
    }

    compute_message(message, body_data_buffer);

    free(line); free(body_data_buffer);

    return message;
}

char *compute_delete_request(char *host, char *url, char *jwt_auth) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Method name, URL and protocol type
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add Authorization header if available
    if (jwt_auth) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", jwt_auth);
        compute_message(message, line);
    }

    // Final new line
    compute_message(message, "");

    free(line);

    return message;
}