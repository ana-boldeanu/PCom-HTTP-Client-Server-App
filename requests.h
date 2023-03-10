#ifndef _REQUESTS_
#define _REQUESTS_

#include "parson.h"

#define HOST_IP "34.241.4.235"
#define HOST_ADDR "ec2-34-241-4-235.eu-west-1.compute.amazonaws.com"
#define HTTP_PORT 8080
#define PAYLOAD_TYPE "application/json"
#define PATH_REGISTER "/api/v1/tema/auth/register"
#define PATH_LOGIN "/api/v1/tema/auth/login"
#define PATH_ACCESS "/api/v1/tema/library/access"
#define PATH_BOOKS "/api/v1/tema/library/books"
#define PATH_BOOK_ID "/api/v1/tema/library/books/"
#define PATH_LOGOUT "/api/v1/tema/auth/logout"

#define COMMAND_LENGTH 40
#define USERNAME_LENGTH 20
#define PASSWORD_LENGTH 20
#define USERDATA_STRING_LENGTH 50
#define ID_LENGTH 10
#define BOOK_DATA_LENGTH 50
#define MESSAGE_LENGTH 100

// Returns 1 if the given command is a valid one
int is_valid_command(char *command);

// Creates and returns a JSON-format string containing 2 fields, username and password
char *create_userdata_json_string(char *username, char *password);

// Creates and returns a JSON-format string containing informations about a book
char *create_bookdata_json_string(char *title, char *author, char *genre, char *publisher, int page_count);

// Extrage mesajul de eroare din raspunsul serverului
void show_error(char *response);

// Citeste de la tastatura username-ul si parola utilizatorului
void get_userdata(char **username, char **password);

// Citeste de la tastatura date despre o carte
void get_bookdata(char **title, char **author, char **genre, char **publisher, int *page_count);

// Computes and returns a GET request string (query_params and cookies can be set to NULL if not needed)
// jwt_auth is the JWT token required for the Authorization Header (it is NULL if no Authorization is needed)
char *compute_get_request(char *host, char *url, char *query_params, char *jwt_auth,
							char **cookies, int cookies_count);

// Computes and returns a POST request string (cookies can be NULL if not needed)
// jwt_auth is the JWT token required for the Authorization Header (it is NULL if no Authorization is needed)
char *compute_post_request(char *host, char *url, char* content_type, char *jwt_auth, char **body_data,
							int body_data_fields_count, char** cookies, int cookies_count);

// Computes and returns a DELETE request string (jwt_auth can be NULL if Authorization isn't needed)
char *compute_delete_request(char *host, char *url, char *jwt_auth);

#endif
