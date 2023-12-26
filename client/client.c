#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// variable for definign the char array
#define DECLARE_CHAR_ARRAY(name, size) char name[size]
// declaring the string template
#define DECLARE_STRING_TEMPLATE(name, format) const char *name = format

// definig the functions skeleton
void send_command(int socket, const char *command);
int validate_command_syntax(const char *command);
void receive_and_process_response(int socket);
int is_valid_date_format(const char *date);
int return_err_value = 0;
int return_success_value = 1;

void generic_printf(const char *format, ...)
{
    // this is univrasal function for printing
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
int create_socket(int domain, int type, int protocol)
{
    // function for creating the socket
    int socket_fd = socket(domain, type, protocol);
    if (socket_fd == -1)
    {
        DECLARE_STRING_TEMPLATE(con_msg, "OOPS!!! We are facing Error while creating socket...\n");
        generic_printf(con_msg);
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}
void initialize_server_address(struct sockaddr_in *address, const char *ip, int port)
{
    // fucntion for initilizing the server address
    if (address != NULL)
    {
        address->sin_family = AF_INET;
        address->sin_addr.s_addr = inet_addr(ip);
        address->sin_port = htons(port);
    }
    else
    {
        DECLARE_STRING_TEMPLATE(msg, "OOPS!!! You might provide the NULL pointer for address. Can you Please verify Once???\n");
        generic_printf(msg);
    }
}
size_t find_newline_length(const char *str) {
    // function for finding the length of new line
    return strcspn(str, "\n");
}
void handle_connection(int socket_fd, const struct sockaddr *server_addr, socklen_t addr_len)
{

    // function for handling the connection with server
    if (connect(socket_fd, server_addr, addr_len) == -1)
    {
        DECLARE_STRING_TEMPLATE(msg,"OOPS!!! We are facing Error while connecting to server...");
        generic_printf(msg);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
}
void read_command(char *buffer, size_t buffer_size)
{

    // fucntion for reading the given commands while running the client
    if (fgets(buffer, buffer_size, stdin) != NULL)
    {
        size_t length = find_newline_length(buffer); // removing new line char from the arguments and cmd
        if (buffer[length] == '\n')
        {
            buffer[length] = '\0';
        }
    }
    else
    {
        DECLARE_STRING_TEMPLATE(msg,"OOPS!!! We are facing Error while reading input\n");
        generic_printf(msg);
    }
}
int main()
{
    // creating the socker
    int client_socket = create_socket(AF_INET, SOCK_STREAM, 0);

    // setting up the server
    struct sockaddr_in server_address;
    initialize_server_address(&server_address, "127.0.0.1", 8080); // Server port

    // connecting the client with the server
    handle_connection(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    DECLARE_STRING_TEMPLATE(con_msg,"Congrats!!! Client Has Been Successfully Connected To The Server.\n");
    generic_printf(con_msg);
    DECLARE_CHAR_ARRAY(command, 512);
    while (1)
    {
        DECLARE_STRING_TEMPLATE(etr_msg,"Client$: ");
        generic_printf(etr_msg);
        read_command(command, sizeof(command));
        if (validate_command_syntax(command))
        {
            // sending the command to the server after validating
            send_command(client_socket, command);
            // receiving the response from the server of given input
            receive_and_process_response(client_socket);
        }
        else
        {
            DECLARE_STRING_TEMPLATE(msg,"OOPS!!! It Seems Invalid Syntax of Given Command. Can you Please Try Again???\n");
            generic_printf(msg);
        }
    }

    //closing the client sockert
    close(client_socket);

    return return_err_value;
}

void send_command(int socket, const char *command)
{
    // sending the command
    send(socket, command, strlen(command), 0);
}
int validate_and_handle_error(const char *value, const char *error_message)
{
    // validating the handling the error
    if (value == NULL)
    {
        generic_printf(error_message);
        return return_err_value;
    }
    return return_success_value;
}
int validate_not_null(const char *error_message, int num_strings, ...) {
    // validating and handling the not null values
    va_list args;
    va_start(args, num_strings);

    for (int i = 0; i < num_strings; ++i) {
        const char *current_str = va_arg(args, const char *);
        if (current_str == NULL) {
            generic_printf(error_message);
            va_end(args);
            return return_err_value;
        }
    }

    va_end(args);
    return return_success_value;
}
int validate_size_range(int size1, int size2, const char *error_message)
{
    // validating and handling the size range
    if (size1 < 0 || size2 < 0 || size1 > size2)
    {
        generic_printf(error_message);
        return return_err_value;
    }
    return return_success_value;
}
int validate_command_syntax(const char *command)
{
    DECLARE_CHAR_ARRAY(command_copy, 512);
    // tokenizing the command and fatching the arguments
    strcpy(command_copy, command);
    char *command_type = strtok(command_copy, " ");
    if (command_type != NULL && strcmp(command_type, "getfn") == return_err_value)
    {
        char *filename = strtok(NULL, " ");
        int result = validate_and_handle_error(filename, "OOPS!!! Invalid Syntax of getfn.\nExample Cmd=> getfn sam.txt\n");
        if (result == return_err_value)
        {
            return return_err_value;
        }
        return return_success_value;
    }
    else if (strcmp(command_type, "getfz") == return_err_value)
    {
        char *size1_str = strtok(NULL, " ");
        char *size2_str = strtok(NULL, " ");
        int result = validate_not_null("OOPS!!! Invalid Syntax of getfz.\nExample Cmd=> getfz 123 12345678\n", 2, size1_str, size2_str);
        if (result == return_err_value)
        {
            return return_err_value;
        }
        int size1 = atoi(size1_str);
        int size2 = atoi(size2_str);
        int size_res = validate_size_range(size1, size2, "OOPS!!! Invalid size values for getfz command. Ensure size1 >= 0, size2 >= 0, and size1 <= size2.\n");
        if (result == return_err_value)
        {
            return return_err_value;
        }
        return return_success_value;
    }
    else if (strcmp(command_type, "getft") == return_err_value)
    {
        char *ext1 = strtok(NULL, " ");
        int result = validate_not_null("OOPS!!! Invalid syntax for getft command.\nExample Cmd=> getft c jpg txt\n At leat one Extension is require.\n", 1, ext1);
        if (result == return_err_value)
        {
            return return_err_value;
        }
        char *ext2 = strtok(NULL, " ");
        char *ext3 = strtok(NULL, " ");
        return return_success_value;
    }
    else if (strcmp(command_type, "getfdb") == return_err_value || strcmp(command_type, "getfda") == return_err_value)
    {
        char *date = strtok(NULL, " ");
        if (date == NULL || !is_valid_date_format(date))
        {
            DECLARE_STRING_TEMPLATE(message,"OOPS!!! Invalid syntax for %s command.\nExample Cmd: %s YYYY-MM-DD\n");
            generic_printf(message, command_type, command_type);
            return return_err_value;
        }
        return return_success_value;
    }
    else if (strcmp(command_type, "quitc") == return_err_value)
    {
        char *arg = strtok(NULL, " ");
        if (arg != NULL)
        {
            DECLARE_STRING_TEMPLATE(quitc_err_msg,"OOPS!!! Invalid syntax for %s command.\n Example cmd: %s\n");
            generic_printf(quitc_err_msg, command_type);
            return return_err_value;
        }
        else
        {
            DECLARE_STRING_TEMPLATE(quitc_err_msg1,"We Are going to close the Requested Client...Thank You!!! See you Soon!!!\n");
            generic_printf(quitc_err_msg1);
            close(socket);
            exit(EXIT_SUCCESS);
        }
        return return_success_value;
    }
    else
    {
        // for unrecognized command
        DECLARE_STRING_TEMPLATE(unk_cmd_msg,"Sorry!!! System Is Not Able To Understand The Given Command.Can You Please Try Again???\n");
        generic_printf(unk_cmd_msg);
        return return_err_value;
    }
    return return_success_value;
}
int is_valid_numeric_date(const char *date) {
    // validating the numneric date format
    for (int i = 0; i < 10; i++) {
        if (i != 4 && i != 7 && (date[i] < '0' || date[i] > '9')) {
            return return_err_value;
        }
    }
    return return_success_value;
}
int is_valid_date_format(const char *date)
{
    // validating the date by checking yhe string, length and numeric value
    if (strlen(date) != 10)
    {
        return return_err_value;
    }
    if (date[4] != '-' || date[7] != '-')
    {
        return return_err_value;
    }
    return is_valid_numeric_date(date);
}

void receive_and_process_response(int socket)
{
    // this function will talk with the server for requesting the accepting the response
    char buffer[1024];
    ssize_t bytes_received = recv(socket, buffer, sizeof(buffer), 0);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        DECLARE_STRING_TEMPLATE(server_reply,"Server Reply: %s\n");
        generic_printf(server_reply, buffer);
        if (strstr(buffer, "Closing the Client") != NULL)
        {
            DECLARE_STRING_TEMPLATE(close_clt_msg,"Closing the client...\n");
            generic_printf(close_clt_msg);
            close(socket);
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        DECLARE_STRING_TEMPLATE(er_server_res,"Error receiving server response.\n");
        generic_printf(er_server_res, buffer);
    }
}