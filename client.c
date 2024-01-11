//Name: Shivasree Gopinathan
//Student ID: 110123208
//Section: 1

//Student 2:
//Name : Jaykumar Mahendrakumar Kadiwala
//Student ID: 110122389
//Section: 1
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>

// Global variables declaration
//for storing socket descriptors,a flag to identify whether the program is acting as a server or a mirror.

int serverDes = 0;             // Socket descriptor for the server
int mirrorDes = 0;             // Socket descriptor for the mirror
int process_serv_or_mirror;    // Variable to store the active socket descriptor (server or mirror)
char message[1024];             // Character array to store messages

// Function declarations
void config_serv_or_mirror();   // Function to determine whether to use the server or mirror socket
void res_and_zip();             // Function to handle the response and zip logic
void receive_tar();             // Function to handle the reception of a tar file
void command_validation(char *);// Function to validate and modify the user-entered command

//Main function
int main(int argc, char *argv[])
{
    int port_no;
    struct sockaddr_in server_address;
// check for number of command-line arguments
    if (argc != 4)
    {
        printf("Call model:%s <IP> <server Port#> <mirror Port#>\n", argv[0]);
        exit(0);
    }

// Creates a socket for the server  
    if ((serverDes = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Socket not created\n");
        exit(1);
    }
//sets up server address information
    server_address.sin_family = AF_INET;
    sscanf(argv[2], "%d", &port_no);
    server_address.sin_port = htons((uint16_t)port_no);

    if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) < 0)
    {
        fprintf(stderr, "inet_pton() has failed\n");
        exit(2);
    }
//and finally connect to server
    if (connect(serverDes, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        fprintf(stderr, "connect not successful\n");
        exit(3);
    }

    // "connecting  to mirror "
    int mirror_port_number;
    struct sockaddr_in mirror_address;
//Creates a socket for the mirror
    if ((mirrorDes = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Mirror socket not created.\n");
        exit(1);
    }
//sets up mirror address information
    mirror_address.sin_family = AF_INET;
    sscanf(argv[3], "%d", &mirror_port_number);
    mirror_address.sin_port = htons((uint16_t)mirror_port_number);

    if (inet_pton(AF_INET, argv[1], &mirror_address.sin_addr) < 0)
    {
        fprintf(stderr, "inet_pton() not successful\n");
        exit(2);
    }
//connects to the mirror.
    if (connect(mirrorDes, (struct sockaddr *)&mirror_address, sizeof(mirror_address)) < 0)
    {
        perror("Error Connecting to mirror");
        exit(1);
    }

//  function to determine whether to use the server or mirror socket.
    config_serv_or_mirror();

    while (1)
    {
        printf("\nEnter command(for terminate enter 'quitc'):\n");
 // Reads user input,  validates the command syntax, 
        fgets(message, sizeof(message), stdin);
        size_t length = strlen(message);
        if (message[length - 1] == '\n') //removes the newline character,
        {
            message[length - 1] = '\0';
        }
        // check for command syntax
        command_validation(&message);
        // write the command to server
        write(process_serv_or_mirror, message, strlen(message));

      // FGETS 
        if (strncmp(message, "fgets ", 6) == 0)
        {                      
            res_and_zip();
        }
        // GETFZ

        else if (strncmp(message, "getfz ", 6) == 0)
        {
            res_and_zip();
        }

        //GETFN

        else if (strncmp(message, "getfn ", 6) == 0)
        {
            char res_msg[1024];
            ssize_t res_bytes = recv(process_serv_or_mirror, res_msg, sizeof(res_msg) - 1, 0);
            if (res_bytes > 0)
            {
                //we are null-terminating the received data
                res_msg[res_bytes] = '\0';
                printf("The File Information:\n%s\n", res_msg);
            }
        }

        //GETFT 

        else if (strncmp(message, "getft ", 6) == 0)
        {
            res_and_zip();
        }
        //GETFDA

        else if (strncmp(message, "getfda ", 7) == 0)
        {
            res_and_zip();
        }

        //GETFDB

        else if (strncmp(message, "getfdb ", 7) == 0)
        {
            res_and_zip();
        }

        //QUITC
        else if (strcmp(message, "quitc") == 0)
        {
            close(serverDes);
            printf("Disconnected from the server.\n");
            exit(0);
        }
        else
        {
            printf("The entered command is not valid\n");
        }
    }
    return 0;
}
//checks  whether the program is acting as a server or mirror based on the received flag from the server.
void config_serv_or_mirror(){
    int serv_or_mirror;
    
    // receive serv_or_mirror flag from server
    recv(serverDes, &serv_or_mirror, sizeof(int), 0);
    if (serv_or_mirror)
    {
        // set to mirror socket
        process_serv_or_mirror = mirrorDes;
    }
    else
    {
        // set to server socket
        process_serv_or_mirror = serverDes;
    }
}

//function to handles the response and zip logic
void res_and_zip(){
    int success_or_not;
    // get success_or_not flag from server
    recv(process_serv_or_mirror, &success_or_not, sizeof(int), 0);
    if (success_or_not == 1)
    {
        // receive tar file if success_or_not
        receive_tar();
        
    }
    else
    {
        // The server did not find files
        char res_message[1024];
        // recieve no file found msg
        ssize_t readed_bytes = recv(process_serv_or_mirror, res_message, sizeof(res_message) - 1, 0);
        if (readed_bytes > 0)
        {
            // Null-terminate the received data
            res_message[readed_bytes] = '\0';
        }
    }
}
//function to handles the reception of a tar file, including receiving file size and file data.
void receive_tar()
{
    int f_size;
    // Receive file size
    recv(process_serv_or_mirror, &f_size, sizeof(f_size), 0);
    printf("file size : %d",f_size);
    if (f_size == -1)
    {
        printf("The file is not found\n");
        return 0;
    }
    // here we are receiving the tar fgets file from server
    //  the we open the file for writing the data we received
    if (system("test -d ~/f23project") != 0) {
        system("mkdir ~/f23project");
    }
    char *temp_path = malloc(1024);
    strcpy(temp_path, getenv("HOME"));
    strcat(temp_path,"/f23project");   
    strcat(temp_path,"/temp.tar.gz");

    FILE *f1 = fopen(temp_path, "wb");
    if (!f1)
    {
        perror("fopen");
        close(serverDes);
        return 1;   
    }

    // Receiving the file and writing the file data
    char buff[1024];
    ssize_t received_data;
    int wrote_bytes;

    while (f_size > 0)
    {
        // we are receiving the  bytes from the server
        received_data = recv(process_serv_or_mirror, buff, sizeof(buff), 0);
        if (received_data == -1)
        {
            perror("Error receiving file data");
            return 0;

        }
        else if(received_data == 0){
            break;
        }
        // write bytes received to tar file
        wrote_bytes = fwrite(buff, 1, f_size, f1);
        printf("w - > %d\n", wrote_bytes);
        f_size -= received_data;
        printf("f -> %d\n",f_size);
    }
    fclose(f1);
}
//Validates and modifies the command entered by the user, specifically for the "getft" command.
void command_validation(char *msg){
    if (strncmp(msg, "getft ", 6) == 0)
    {
        char new_cmnds[1024];
        // We need to get the commands from the request
        sscanf(msg, "%s", new_cmnds);
        // an array to store tokens
        char *tokns[5];

        int no_of_tokens = 0;
        char *tokn = strtok(msg, " ");
        // checking the number of tokens
        while (tokn != NULL && no_of_tokens < 4)
        {
            tokns[no_of_tokens] = tokn;
            tokn = strtok(NULL, " ");
            no_of_tokens++;
        }

        if (no_of_tokens > 1)
        {
            // need to add a scpace before appending the first token
            strcat(new_cmnds, " ");
            // first token (command) is appeneded
            strcat(new_cmnds, tokns[1]);
        }

        for (int i = 2; i <= 4; i++)
        {
            if (i < no_of_tokens)
            {
                // Need to add a space before appending 
                strcat(new_cmnds, " ");
                // the token is getting appended
                strcat(new_cmnds, tokns[i]);
            }
        }
        if (no_of_tokens > 4)
        {
            printf("Enter only 3 extensions.\n");
        }
        strcpy(msg, new_cmnds);
    }
}