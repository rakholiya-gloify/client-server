
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define DECLARE_CHAR_ARRAY(name, size) char name[size]
#define DECLARE_STRING_TEMPLATE(name, format) const char *name = format

volatile sig_atomic_t terminate_flag = 0;

// Function prototypes
void pclientrequest(int client_socket, int server_socket);

// Additional functions for handling specific commands
void fetching_file_data_cmd(const char *filename, int client_socket);
void handling_creating_zip_based_on_size_args_cmd(int size1, int size2, int client_socket);
void copy_file(const char *source_path, const char *dest_path);
void handling_the_file_extention_cmd(char *ext1, char *ext2, char *ext3, int client_socket);
void handling_date_zip_cmd(const char *date, int client_socket, const char *cmd);
void handle_getfda(const char *date, int client_socket);
void terminte_client_request_cmd(int server_socket, int client_socket);

void generic_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
time_t strtotime(const char *date)
{
    // converting the date string to time
    struct tm tm = {0};
    if (strptime(date, "%Y-%m-%d", &tm) == NULL)
    {
        DECLARE_STRING_TEMPLATE(error_date_msg, "OOPS!! We Are Facing Error while converting date!!!");
        generic_printf(error_date_msg);
        exit(EXIT_FAILURE);
    }
    return mktime(&tm); // type casting the date here also
}
int send_response_to_client(int socket, const char *data, size_t size)
{
    // using this function for transfering the data on socket
    ssize_t bytes_sent = send(socket, data, size, 0);

    if (bytes_sent == -1)
    {
        DECLARE_STRING_TEMPLATE(error_send_msg, "OOPS!! We Are Facing Error while sending data!!!");
        generic_printf(error_send_msg);
        return -1;
    }
    return 0;
}
void create_message(char *message, size_t message_size, const char *format, ...)
{
    // this function will create the msg for server
    va_list args;
    va_start(args, format);
    vsnprintf(message, message_size, format, args);
    va_end(args);
}

FILE *open_file(const char *path, const char *mode)
{
    // this fucntion is for opening the file
    FILE *file = fopen(path, mode);
    if (file == NULL)
    {
        DECLARE_STRING_TEMPLATE(error_send_msg, "OOPS!! We Are Facing Error while opening file!!!");
        generic_printf(error_send_msg);
    }
    return file;
}
void copy_file(const char *source_path, const char *dest_path)
{
    // this function is for coping the file from source dir to dest dir
    DECLARE_STRING_TEMPLATE(source_file_open_mode, "rb");
    FILE *source_file = open_file(source_path, source_file_open_mode);
    DECLARE_STRING_TEMPLATE(dest_file_open_mode, "wb");
    FILE *dest_file = open_file(dest_path, dest_file_open_mode);
    if (source_file && dest_file)
    {
        DECLARE_CHAR_ARRAY(buffer, 1024);
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
        {
            fwrite(buffer, 1, bytesRead, dest_file);
        }
        fclose(source_file);
        fclose(dest_file);
    }
}
int send_tar_gz(int client_socket, const char *zip_filename)
{
    // this function will send the zip file to the client

    DECLARE_CHAR_ARRAY(response, 1024);
    system("tar czf temp.tar.gz .");
    DECLARE_STRING_TEMPLATE(tar_gz_file_open_mode, "rb");
    FILE *tar_gz_file = open_file("temp.tar.gz", tar_gz_file_open_mode);
    if (tar_gz_file == NULL)
    {
        DECLARE_STRING_TEMPLATE(error_zip_msg, "OOPS!! We Are Facing Error while opening temp.tar.gz!!!");
        generic_printf(error_zip_msg);
    }

    // looking for the size of the file
    fseek(tar_gz_file, 0, SEEK_END);
    long file_size = ftell(tar_gz_file);
    fseek(tar_gz_file, 0, SEEK_SET);

    // using buffer to read the file
    char *buffer = (char *)malloc(file_size);
    if (buffer == NULL)
    {
        DECLARE_STRING_TEMPLATE(error_allt_buff, "OOPS!! We Are Facing Error while allocating buffer!!!");
        generic_printf(error_allt_buff);
        fclose(tar_gz_file);
    }

    // reading the file data into the buffer
    fread(buffer, 1, file_size, tar_gz_file);
    create_message(response, sizeof(response), "Hey!!! Zip File %s has been Created", zip_filename);
    send_response_to_client(client_socket, response, strlen(response));

    // cleaning up the dir
    fclose(tar_gz_file);
    free(buffer);
    remove("temp.tar.gz");
    return 0;
}
void pclientrequest_mirror(int client_socket, int mirror_socket)
{
    DECLARE_CHAR_ARRAY(buffer, 1024);

    while (1)
    {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            break;
        }
        ssize_t bytes_sent = send(mirror_socket, buffer, bytes_received, 0);
        if (bytes_sent <= 0)
        {
            break;
        }
    }
    close(client_socket);
    close(mirror_socket);
}
int main()
{
    // creating the socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        DECLARE_STRING_TEMPLATE(error_server_socket_msg, "OOPS!! We Are Facing Error while creating server socket!!!\n");
        generic_printf(error_server_socket_msg);
        exit(EXIT_FAILURE);
    }

    // setting up the server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);
    DECLARE_STRING_TEMPLATE(server_conn_msg, "Congratulations!! Server is up on 8080 port...!!!\n");
    generic_printf(server_conn_msg);

    // binding the socket and server
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        DECLARE_STRING_TEMPLATE(error_server_socket_binding_msg, "OOPS!! We Are Facing Error while binding socket!!!\n");
        generic_printf(error_server_socket_binding_msg);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // here we are lookng for the client connections on the server
    if (listen(server_socket, 100) == -1) // max 100 con will be accepted
    {
        DECLARE_STRING_TEMPLATE(error_server_socket_listing_conn, "OOPS!! We Are Facing Error while listening for connections!!!\n");
        generic_printf(error_server_socket_listing_conn);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    int connection_count = 0;
    while (!terminate_flag)
    {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == -1)
        {
            DECLARE_STRING_TEMPLATE(error_acc_conn, "OOPS!! We Are Facing Error while accepting connection!!!\n");
            generic_printf(error_acc_conn);
            continue;
        }
        pid_t pid = fork();
        if (pid == -1)
        {
            DECLARE_STRING_TEMPLATE(error_acc_conn, "OOPS!! We Are Facing Error while forking!!!\n");
            generic_printf(error_acc_conn);
            close(client_socket);
            continue;
        }
        else if (pid == 0)
        {
            // this is the child process of the server for handling the client requests
            close(server_socket);
            if (connection_count <= 4)
            {
                pclientrequest(client_socket, server_socket);
            }
            else if (connection_count <= 8)
            {
                int mirror_socket = socket(AF_INET, SOCK_STREAM, 0);
                if (mirror_socket == -1)
                {
                    perror("Error creating mirror socket");
                    close(client_socket);
                    exit(EXIT_FAILURE);
                }

                struct sockaddr_in mirror_address;
                mirror_address.sin_family = AF_INET;
                mirror_address.sin_addr.s_addr = INADDR_ANY;
                mirror_address.sin_port = htons(8081); // Assuming a different port for the mirror

                if (connect(mirror_socket, (struct sockaddr *)&mirror_address, sizeof(mirror_address)) == -1)
                {
                    perror("Error connecting to mirror");
                    close(mirror_socket);
                    close(client_socket);
                    exit(EXIT_FAILURE);
                }

                pclientrequest_mirror(client_socket, mirror_socket);
                close(mirror_socket);
            }
            else if (connection_count > 8 &&connection_count % 2 == 0)
            {
                // Even connections are handled by the server
                pclientrequest(client_socket, server_socket);
            }
            close(client_socket);
            exit(EXIT_SUCCESS);
        }
        else
        {
            // cloing the parent client socket process
            close(client_socket);
            connection_count++;

            // will handle upto 100 connections
            if (connection_count == 100)
            {
                terminate_flag = 1;
            }
        }
    }
    close(server_socket);
    return 0;
}

void pclientrequest(int client_socket, int server_socket)
{
    char buffer[1024];

    while (!terminate_flag)
    {
        // Receive the command from the client
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            // Error or client disconnected
            break;
        }
        buffer[bytes_received] = '\0';

        // tokanizing the client input and accessing the command and arguments
        char *command = strtok(buffer, " ");
        char *argument1 = strtok(NULL, " ");
        char *argument2 = strtok(NULL, " ");
        char *argument3 = strtok(NULL, " ");

        // Process the commads on server
        if (strcmp(command, "getfn") == 0)
        {
            fetching_file_data_cmd(argument1, client_socket);
        }
        else if (strcmp(command, "getfz") == 0)
        {
            int size1 = atoi(argument1);
            int size2 = atoi(argument2);
            handling_creating_zip_based_on_size_args_cmd(size1, size2, client_socket);
        }
        else if (strcmp(command, "getft") == 0)
        {
            handling_the_file_extention_cmd(argument1, argument2, argument3, client_socket);
        }
        else if (strcmp(command, "getfdb") == 0)
        {
            handling_date_zip_cmd(argument1, client_socket, "getfdb");
        }
        else if (strcmp(command, "getfda") == 0)
        {
            handling_date_zip_cmd(argument1, client_socket, "getfda");
        }
        else if (strcmp(command, "quitc") == 0)
        {
            terminte_client_request_cmd(server_socket, client_socket);
        }
        else
        {
            DECLARE_STRING_TEMPLATE(unk_cmd, "OOPS!! The SYstem Is not able to understand the Unknown cmd. Can you Please Try again???");
            send(client_socket, unk_cmd, strlen(unk_cmd), 0);
        }
    }

    // closing the client socket when it has finished the tasks
    close(client_socket);
}

void fetching_file_data_cmd(const char *filename, int client_socket)
{
    DECLARE_CHAR_ARRAY(path, 1024);
    create_message(path, sizeof(path), "./%s", filename);
    const char *file_open_mode = "rb";
    FILE *file = open_file(path, file_open_mode);
    if (file)
    {
        struct stat file_stat;
        if (stat(path, &file_stat) == 0)
        {
            DECLARE_CHAR_ARRAY(filename_message, 256);
            DECLARE_CHAR_ARRAY(size_message, 256);
            DECLARE_CHAR_ARRAY(date_created_message, 256);
            DECLARE_CHAR_ARRAY(permissions_message, 256);
            DECLARE_CHAR_ARRAY(response, 1024);
            create_message(permissions_message, sizeof(permissions_message), "%s is having the : %o permissions\n", filename, file_stat.st_mode & 0777);
            create_message(filename_message, sizeof(filename_message), "\nFile Name is: %s\n", filename);
            create_message(date_created_message, sizeof(date_created_message), "The %s created on: %s\n", filename, ctime(&file_stat.st_ctime));
            create_message(size_message, sizeof(size_message), "Size of the %s is : %ld bytes\n", filename, file_stat.st_size);
            create_message(response, sizeof(response), "%s%s%s%s", filename_message, size_message, date_created_message, permissions_message);
            if (send_response_to_client(client_socket, response, strlen(response)) == 0)
            {
                DECLARE_STRING_TEMPLATE(data_Sent_msg, "Hey!!! Data sent to client successfully\n");
                generic_printf(data_Sent_msg);
            }
        }
        fclose(file);
    }
    else
    {
        DECLARE_STRING_TEMPLATE(response_template, "Hey!!! Your Requested file %s is not found on %s path\n");
        DECLARE_CHAR_ARRAY(response, 256);
        DECLARE_CHAR_ARRAY(current_path, 1024);
        // File not found
        if (getcwd(current_path, sizeof(current_path)) != NULL)
        {
            create_message(response, sizeof(response), response_template, filename, current_path);
        }
        else
        {
            // Handle error if getcwd fails
            create_message(response, sizeof(response), "OOPS!!! Your Requested is File Not Found.\n");
        }
        if (send_response_to_client(client_socket, response, strlen(response)) == 0)
        {
            DECLARE_STRING_TEMPLATE(data_sent_msg1, "Hey!!! Data sent to client successfully\n");
            generic_printf(data_sent_msg1);
        }
    }
}

void handling_creating_zip_based_on_size_args_cmd(int size1, int size2, int client_socket)
{
    DIR *dir = opendir(".");
    if (dir)
    {
        struct dirent *entry;
        int files_found = 0; // flag for checking the file found
        DECLARE_CHAR_ARRAY(zip_filename, 1024);
        DECLARE_CHAR_ARRAY(response, 1024);
        // creating the temporary dir for zip
        char temp_dir[] = "temp";
        mkdir(temp_dir, 0777);

        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_REG)
            {
                DECLARE_CHAR_ARRAY(filepath, 1024);
                create_message(filepath, sizeof(filepath), "./%s", entry->d_name);
                struct stat file_stat;
                if (stat(filepath, &file_stat) == 0)
                {
                    if (file_stat.st_size >= size1 && file_stat.st_size <= size2)
                    {
                        // coping the matching file to temp dir
                        DECLARE_CHAR_ARRAY(dest_filepath, 1024);
                        create_message(dest_filepath, sizeof(dest_filepath), "./%s/%s", temp_dir, entry->d_name);
                        copy_file(filepath, dest_filepath);
                        files_found = 1; // setting the positive flag for founded file
                    }
                }
            }
        }
        closedir(dir);

        if (files_found)
        {
            // creating the zip file based on given size arguments
            create_message(zip_filename, sizeof(zip_filename), "temp_%d_%d.tar.gz", size1, size2);

            // compressing the temp dirto a tar.gz file
            DECLARE_CHAR_ARRAY(tar_command, 2056);
            create_message(tar_command, sizeof(tar_command), "tar -czf %s %s", zip_filename, temp_dir);
            system(tar_command);

            // sending the zip file to the client
            if (send_tar_gz(client_socket, zip_filename))
            {
                DECLARE_STRING_TEMPLATE(data_sent_msg1, "Hey!!! Data sent to client successfully\n");
                generic_printf(data_sent_msg1);
                DECLARE_STRING_TEMPLATE(zip_msg, "Hey!!! Zip File Created: %s\n");
                generic_printf(zip_msg, zip_filename);
            }
        }
        else
        {
            // if file will not found
            DECLARE_STRING_TEMPLATE(no_file_response, "Sorry!!! No file found\n");
            if (send_response_to_client(client_socket, no_file_response, strlen(no_file_response)) == 0)
            {
                DECLARE_STRING_TEMPLATE(data_sent_msg1, "Hey!!! Data sent to client successfully\n");
                generic_printf(data_sent_msg1);
            }
            DECLARE_STRING_TEMPLATE(server_res, "Server Response: %s\n");
            generic_printf(server_res, no_file_response);
        }

        // cleaning up the temp dir
        DECLARE_CHAR_ARRAY(remove_command, 1024);
        create_message(remove_command, sizeof(remove_command), "rm -r %s", temp_dir);
        system(remove_command);
    }
}

int check_file_extension(const char *filename, const char *ext1, const char *ext2, const char *ext3)
{
    // in this function we are cheking the file extension on appropriate path
    const char *file_ext = strrchr(filename, '.');
    if (file_ext != NULL)
    {
        // if file extenion will match with any arguments
        if ((ext1 && strcmp(file_ext + 1, ext1) == 0) ||
            (ext2 && strcmp(file_ext + 1, ext2) == 0) ||
            (ext3 && strcmp(file_ext + 1, ext3) == 0))
        {
            return 1;
        }
    }
    return 0;
}

void handling_the_file_extention_cmd(char *ext1, char *ext2, char *ext3, int client_socket)
{
    // checking 1 ext provided or not
    if (ext1 == NULL)
    {
        DECLARE_STRING_TEMPLATE(message, "OOPS!!! Invalid syntax for getft command.\nExample Cmd=> getft c jpg txt\n At leat one Extension is require.\n");
        generic_printf(message);
        return;
    }

    DIR *dir = opendir(".");
    if (dir)
    {
        struct dirent *entry;
        int files_found = 0;
        DECLARE_CHAR_ARRAY(zip_filename, 1024);
        DECLARE_CHAR_ARRAY(response, 1024);
        char temp_dir[] = "temp";
        mkdir(temp_dir, 0777);
        const char *extensions[] = {ext1, ext2, ext3};
        for (int i = 0; i < 3 && extensions[i] != NULL; ++i)
        {
            // Reset the directory stream
            rewinddir(dir);

            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_type == DT_REG)
                {
                    DECLARE_CHAR_ARRAY(filepath, 1024);
                    snprintf(filepath, sizeof(filepath), "./%s", entry->d_name);
                    if (check_file_extension(entry->d_name, extensions[i], NULL, NULL))
                    {
                        // coping the file from tem dir
                        DECLARE_CHAR_ARRAY(dest_filepath, 1024);
                        create_message(dest_filepath, sizeof(dest_filepath), "./%s/%s", temp_dir, entry->d_name);
                        copy_file(filepath, dest_filepath);
                        create_message(response + strlen(response), sizeof(response) - strlen(response),
                                       "File: %s\nSize: %d bytes\nDate Created: %s\nPermissions: %o\n",
                                       entry->d_name, entry->d_reclen, ctime(&entry->d_ino), entry->d_type);

                        files_found = 1;
                    }
                }
            }
        }

        closedir(dir);

        if (files_found)
        {
            create_message(zip_filename, sizeof(zip_filename), "temp_%s", ext1);
            if (ext2 != NULL)
            {
                strncat(zip_filename, "_", sizeof(zip_filename) - strlen(zip_filename) - 1);
                strncat(zip_filename, ext2, sizeof(zip_filename) - strlen(zip_filename) - 1);
            }

            if (ext3 != NULL)
            {
                strncat(zip_filename, "_", sizeof(zip_filename) - strlen(zip_filename) - 1);
                strncat(zip_filename, ext3, sizeof(zip_filename) - strlen(zip_filename) - 1);
            }

            strncat(zip_filename, ".tar.gz", sizeof(zip_filename) - strlen(zip_filename) - 1);

            // complressing the temp dir and creating zip file
            DECLARE_CHAR_ARRAY(tar_command, 2056);
            create_message(tar_command, sizeof(tar_command), "tar -czf %s %s", zip_filename, temp_dir);

            system(tar_command);

            // sending the response to the client
            if (send_tar_gz(client_socket, zip_filename) == 0)
            {
                DECLARE_STRING_TEMPLATE(message, "Congratulations!!! Zip File Created: %s\n");
                generic_printf(message, zip_filename);
            }
        }
        else
        {
            // for sending the not available file
            DECLARE_STRING_TEMPLATE(no_file_response, "OOPS!! There is Not Any File Availanle!!!");
            if (send_response_to_client(client_socket, no_file_response, strlen(no_file_response)) == 0)
            {
                DECLARE_STRING_TEMPLATE(message, "Hey!!! Data sent to client successfully\n");
                generic_printf(message);
            }
            DECLARE_STRING_TEMPLATE(message, "Server Response: %s\n");
            generic_printf(message, no_file_response);
        }

        // cleaning up and removing the temp dir
        DECLARE_CHAR_ARRAY(remove_command, 1024);
        create_message(remove_command, sizeof(remove_command), "rm -r %s", temp_dir);
        system(remove_command);
    }
}

void handling_date_zip_cmd(const char *date, int client_socket, const char *cmd)
{
    DIR *dir = opendir(".");
    if (dir)
    {
        struct dirent *entry;
        int files_found = 0;
        DECLARE_CHAR_ARRAY(zip_filename, 1024);
        DECLARE_CHAR_ARRAY(response, 1024);
        char temp_dir[] = "temp";
        mkdir(temp_dir, 0777);

        time_t specified_time = strtotime(date);

        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_REG)
            {
                DECLARE_CHAR_ARRAY(filepath, 1024);
                snprintf(filepath, sizeof(filepath), "./%s", entry->d_name);
                struct stat file_stat;
                if (stat(filepath, &file_stat) == 0)
                {
                    time_t file_time = file_stat.st_ctime;
                    printf("File Path: %s\n", filepath);
                    printf("File Creation Time: %s", ctime(&file_time));
                    printf("Specified Time: %s", ctime(&specified_time));
                    printf("Time Difference: %f seconds\n", difftime(file_time, specified_time));

                    if (strcmp(cmd, "getfdb") == 0)
                    {
                        if (difftime(file_time, specified_time) <= 0)
                        {
                            DECLARE_CHAR_ARRAY(dest_filepath, 2056);
                            create_message(dest_filepath, sizeof(dest_filepath), "./%s/%s", temp_dir, entry->d_name);
                            copy_file(filepath, dest_filepath);
                            create_message(response + strlen(response), sizeof(response) - strlen(response),
                                           "File: %s\nSize: %ld bytes\nDate Created: %s\nPermissions: %o\n",
                                           entry->d_name, file_stat.st_size, ctime(&file_stat.st_ctime), file_stat.st_mode & 0777);

                            files_found = 1;
                        }
                    }
                    if (strcmp(cmd, "getfda") == 0)
                    {
                        if (difftime(file_stat.st_ctime, specified_time) >= 0)
                        {
                            DECLARE_CHAR_ARRAY(dest_filepath, 2056);
                            create_message(dest_filepath, sizeof(dest_filepath), "./%s/%s", temp_dir, entry->d_name);
                            copy_file(filepath, dest_filepath);
                            create_message(response + strlen(response), sizeof(response) - strlen(response),
                                           "File: %s\nSize: %ld bytes\nDate Created: %s\nPermissions: %o\n",
                                           entry->d_name, file_stat.st_size, ctime(&file_stat.st_ctime), file_stat.st_mode & 0777);

                            files_found = 1;
                        }
                    }
                }
            }
        }

        closedir(dir);

        if (files_found)
        {
            create_message(zip_filename, sizeof(zip_filename), "temp_%s.tar.gz", date);
            DECLARE_CHAR_ARRAY(tar_command, 2056);
            create_message(tar_command, sizeof(tar_command), "tar -czf %s %s", zip_filename, temp_dir);
            system(tar_command);
            send_tar_gz(client_socket, zip_filename);
        }
        else
        {
            DECLARE_STRING_TEMPLATE(no_file_response, "Sorry!!! There Is No File Available\n");
            send_response_to_client(client_socket, no_file_response, strlen(no_file_response));
        }
        DECLARE_CHAR_ARRAY(remove_command, 1024);
        create_message(remove_command, sizeof(remove_command), "rm -r %s", temp_dir);
        system(remove_command);
    }
}

void terminte_client_request_cmd(int server_socket, int client_socket)
{
    DECLARE_STRING_TEMPLATE(quit_msg, "Hey!!! Quit command received. Closing client...\n");
    generic_printf(quit_msg);
    send_response_to_client(client_socket, "Closing the Client", strlen("Closing the Client"));
    close(client_socket);
    terminate_flag = 1;
    close(server_socket);
    exit(EXIT_SUCCESS);
}
