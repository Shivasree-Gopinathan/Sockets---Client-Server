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
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

//global variables
int clientdecsriptor;
int count = 0;
char client_rqst[1024];

// function declaration
void tarFile(int);
void searchFile(const char *, const char *, char *);
void filesBetweensize(const char *, int , int , char *[], int *);
void searchFilesByExtension(const char *, const char *[], int , char *[], int *);
void findFilesBetweenDate(const char *, time_t , time_t , char *[], int *);
void sendResponse(int , char *[], int );
void pclientrequest(int );

int main(int argc, char *argv[])
{
    int listen_socket, port_no; // variables for listening socket and port number
    socklen_t length; // to store length of socket address
    struct sockaddr_in serveraddress; // to store server address information
    // this will check is required arguments are given or not, if it is not given then it gives error
    if (argc != 2)
    {
        fprintf(stderr, "Correct syntex for command: %s <Port number>\n", argv[0]);
        exit(0);
    }
    //this if block is responsible for creating socket and if it is not created successfully then it gives error ane exit from here
    // AF_INET -> for internet socket, SOCK_STREAM -> type of socket here it is TCP socket, last argument is for protocol ,0 -> default protocol
    if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Could not create socket\n");
        exit(1);
    }
    // specifying address structure 
    serveraddress.sin_family = AF_INET;// specifying family
    serveraddress.sin_addr.s_addr = htonl(INADDR_ANY); // for any address 
    sscanf(argv[1], "%d", &port_no); // reads the port number and convert it to the integert
    serveraddress.sin_port = htons((uint16_t)port_no);// specifying port number, here we have to use conversion function because data format is different, int to network byteorder

    bind(listen_socket, (struct sockaddr *)&serveraddress, sizeof(serveraddress)); //Binds the socket to the specified port and address
    listen(listen_socket, 10); // start listening on socket with a backlog of 10

    while (1)
    {

        // connection accepting from client
        clientdecsriptor = accept(listen_socket, (struct sockaddr *)NULL, NULL);
        // fork new child process and call pclientrequest()
        if (fork() == 0)
        {
            close(listen_socket);
            pclientrequest(clientdecsriptor);
            exit(0);
        }
        else
        {
            close(clientdecsriptor);
        }
    }

    return 0;
}

// this function is responsible for searching files with provided size
void filesBetweensize(const char *directory, int s1, int s2, char *finalFiles[], int *counter)
{
    DIR *dir;
    struct dirent *root;

    if ((dir = opendir(directory)) == NULL)
    {
        return;
    }

    // this will iterate through all files in directory 
    while ((root = readdir(dir)) != NULL)
    {
        char loc[1024];
        //extracting name of file or directory
        snprintf(loc, sizeof(loc), "%s/%s", directory, root->d_name);
        struct stat info;

        // if current file is directory then again goes inside it and search with in it
        if (root->d_type == DT_DIR)
        {
            if (strcmp(root->d_name, ".") == 0 || strcmp(root->d_name, "..") == 0)
                continue;
            filesBetweensize(loc, s1, s2, finalFiles, counter);
        }
        else
        {
            // if file size match with our condition then all it to final files
            if (stat(loc, &info) == 0 && info.st_size >= s1 && info.st_size <= s2)
            {
                finalFiles[*counter] = strdup(loc);
                (*counter)++;
            }
        }
    }
    closedir(dir);
}

void pclientrequest(int socket){
    //listen commands from client in an infinite loop
    while (1)
    {
        //Clears the client_rqst buffer before reading the next client command
        memset(client_rqst, 0, sizeof(client_rqst));
        // listen command from client
        ssize_t length = read(clientdecsriptor, client_rqst, sizeof(client_rqst) - 1);
        client_rqst[length] = '\0';
        printf("Command from client : %s\n", client_rqst);
        if (strncmp(client_rqst, "getfz ", 6) == 0)
        {
            //tokenizing the client commdn to verify the no of args
            char *tokn;
            int s1, s2 ;
            //will store resultant files
            char *finalFiles[1024];
            //count variable
            int file_counter = 0;
            
            //tokenize the client command using whitespace as a delimiter 
            tokn = strtok(client_rqst + 6, " ");
            s1 = atoi(tokn);

            tokn = strtok(NULL, " ");
            s2 = atoi(tokn);

            tokn = strtok(NULL, " ");
            //calling filesBetweensize function to find files and send to client
            filesBetweensize(getenv("HOME"), s1, s2, finalFiles, &file_counter);
            sendResponse(strlen(finalFiles) > 0, finalFiles, file_counter);
        }
        else if (strncmp(client_rqst, "getfn ", 6) == 0) // check getfn command
        {
            char finalPath[1024] = {0}; // will store final path of file
            searchFile(getenv("HOME"), client_rqst + 6, finalPath); // calling the search file function for searching

            // if file found 
            if (strlen(finalPath) > 0)
            {
                struct stat file_stat;
                if (stat(finalPath, &file_stat) == 0)
                {
                    // creating response msg that conmtains file information and sent it to client
                    char respMsg[512];
                    snprintf(respMsg, sizeof(respMsg), "Filename: %s\nSize: %ld bytes\nDate created: %s",
                             client_rqst + 6, file_stat.st_size, ctime(&file_stat.st_ctime));

                    // Send the response back to the client
                    send(socket, respMsg, strlen(respMsg), 0);
                }
            }
            else 
            {
                const char *message = "Nothing found\n";
                //if file is not found then nothing found will sent to client
                send(socket, message, strlen(message), 0);
            }
        }
        else if (strncmp(client_rqst, "getft ", 6) == 0) //checks if the received command starts with "getft "
        {
            //initialize pointers to handle tokenization using space as a delimiter.
            char *tokn;
            char *deli = " ";
            const char *extensns[6];  // initializes an array to store extensions
            int no_of_extensions = 0; //keeps track of the number of extensions
            char *final_files[1024]; //initializes a buffer to hold the found files
            //required args to generate tar file
            char *tarArguments[1024] = {"tar", "-czf", "temp.tar.gz"};
            int arg_count = 3, file_count = 0;
            //skips the "getft " part of the command and starts tokenizing the remaining string using space as a delimiter
            tokn = strtok(client_rqst + 6, deli);
            //getting extension from client request
            while (tokn != NULL && no_of_extensions < 3)
            {
                
                extensns[no_of_extensions] = tokn;
                no_of_extensions++;
                tokn = strtok(NULL, deli);
            }
            //Calls the searchFilesByExtension function and send response to client
            searchFilesByExtension(getenv("HOME"), extensns, no_of_extensions, final_files, &file_count);
            sendResponse(strlen(final_files) > 0, final_files, file_count);
        }
        //////////////////////////////////
        else if (strncmp(client_rqst, "getfdb ", 7) == 0) //hecks if the received command starts with "getfdb ".
        {
            char startDateString[11] = "1970-01-01";  // represent the default start date
            char endDateStr[11]; 
            char *finalFiles[1024];
            char *tarArguments[1024] = {"tar", "-czf", "temp.tar.gz"};
            int file_counter = 0, argument_count = 2;
            //extracts the end date specified in the command after "getfdb "
            sscanf(client_rqst, "getfdb %s", endDateStr);

            //declare structures to hold start and end dates respectively.
            struct tm startDate;
            struct tm endDate;
            // initialize both date structures
            memset(&startDate, 0, sizeof(struct tm));
            memset(&endDate, 0, sizeof(struct tm));

            // converting date to structure type
            strptime(startDateString, "%Y-%m-%d", &startDate);
            strptime(endDateStr, "%Y-%m-%d", &endDate);
            //setting start time
            startDate.tm_hour = 0;
            startDate.tm_min = 0;
            startDate.tm_sec = 0;
            //set the time components of the start date to midnight and handle daylight saving time
            startDate.tm_isdst = -1;
            //setting end time
            endDate.tm_hour = 23;
            endDate.tm_min = 59;
            endDate.tm_sec = 59;
            endDate.tm_isdst = -1;
            //convert the struct tm format to time_t format for start and end dates, respectively
            time_t localStartTime = mktime(&startDate);
            time_t localEndTime = mktime(&endDate);
            //calls the findFilesBetweenDate function to search for files in the home directory within the specified date range and will send response to client
            findFilesBetweenDate(getenv("HOME"), localStartTime, localEndTime, finalFiles, &file_counter);
            sendResponse(strlen(finalFiles) > 0, finalFiles, file_counter);
        }
        else if (strncmp(client_rqst, "getfda ", 7) == 0)
        {
            char startDateString[11]; 
            char endDateStr[11] = "3000-12-31";
            char *finalFiles[1024];
            char *tarArguments[1024] = {"tar", "-czf", "temp.tar.gz"};
            int file_counter = 0, argument_count = 2;
           
            sscanf(client_rqst, "getfda %s", startDateString);

            struct tm startDate;
            struct tm endDate;
            memset(&startDate, 0, sizeof(struct tm));
            memset(&endDate, 0, sizeof(struct tm));

            
            strptime(startDateString, "%Y-%m-%d", &startDate);
            strptime(endDateStr, "%Y-%m-%d", &endDate);
            
            startDate.tm_hour = 0;
            startDate.tm_min = 0;
            startDate.tm_sec = 0;
            
            startDate.tm_isdst = -1;
            
            endDate.tm_hour = 23;
            endDate.tm_min = 59;
            endDate.tm_sec = 59;
            endDate.tm_isdst = -1;

            time_t localStartTime = mktime(&startDate);
            time_t localEndTime = mktime(&endDate);

            findFilesBetweenDate(getenv("HOME"), localStartTime, localEndTime, finalFiles, &file_counter);
            sendResponse(strlen(finalFiles) > 0, finalFiles, file_counter);
        }
        //handle quitc command
        else if (strcmp(client_rqst, "quitc") == 0)
        {
            close(socket);
            exit(0);
        }
        else
        {
            // if no command match found then say enter valid command
            const char *message = "Enter proper valid command\n";
            send(socket, message, strlen(message), 0);
        }
    } // end of while
}

void searchFilesByExtension(const char *directory, const char *extensionarray[], int extensionCount, char *finalFiles[], int *counter){
    DIR *dir;
    struct dirent *root;

    if ((dir = opendir(directory)) == NULL){
        return;
    }

    while ((root = readdir(dir)) != NULL){
        char loc[1024];
        //extracting name of file or directory
        snprintf(loc, sizeof(loc), "%s/%s", directory, root->d_name);

        // // if current file is directory then again goes inside it and search with in it
        if (root->d_type == DT_DIR){
            if (strcmp(root->d_name, ".") == 0 || strcmp(root->d_name, "..") == 0)
                continue;
            searchFilesByExtension(loc, extensionarray, extensionCount, finalFiles, counter);
        }
        else{
            // if extension matches with our extension then it will add it to extension array
            const char *extension = strrchr(root->d_name, '.');
            if (extension){
                //this will iterate through all extension if it matches then add it to final files
                for (int i = 0; i < extensionCount; i++){
                    if (strcmp(extension + 1, extensionarray[i]) == 0){
                        //joining path to final files
                        finalFiles[*counter] = strdup(loc);
                        (*counter)++;
                        break; // terminate loop if it matches
                    }
                }
            }
        }
    }
    closedir(dir);
}

void tarFile(int socket) {  // accept client socket as an argumrnt
    // info is a structure of stat type which is use to retrieve information about file
    struct stat info;
    if (stat("temp.tar.gz", &info) == -1) //this will check whether full file retrieved or not
    {
        // Error occurred
        return;
    }
    int size = info.st_size; // is use to store the size of file in bytes

    // sending the size of file to client
    send(socket, &size, sizeof(size), 0);
    //opening  temp.tar.gz in read mode
    FILE *file = fopen("temp.tar.gz", "rb");

    // this if will read data from file in size of 1024 or less and sent it to the client, once it is done it will close the file
    if (file)
    {
        char buffer[1024];
        int des;
        //read bytes and sent it to client
        while ((des = fread(buffer, 1, sizeof(buffer), file)) > 0)
        {
            //sending bytes to client
            send(socket, buffer, des, 0);
        }
        fclose(file);
    }
    printf("Sent all bytes.\n");
}

//this searchFile will take 3 arguments, directory -> it is path where to start search , name of file, path to the file
void searchFile(const char *directory, const char *name, char *finalpath){
    DIR *dir;
    struct dirent *root;

    // if opening directory fails then exit
    if ((dir = opendir(directory)) == NULL)
        return;

    // iterate through all files in directory
    while ((root = readdir(dir)) != NULL)
    {
        char loc[1024];
        //extracting name of file or directory
        
        snprintf(loc, sizeof(loc), "%s/%s", directory, root->d_name);
        //printf("%s  ::  ", loc );
        //printf("%d\n", strcmp(root->d_name, name));
        //printf("%s  %s\n",root->d_name, name);
        //check if it is directory
        if (root->d_type == DT_DIR)
        {
            //if current file is directory then it continue
            if (strcmp(root->d_name, ".") == 0 || strcmp(root->d_name, "..") == 0)
                continue;
            //this will do for all sub directories
            searchFile(loc, name, finalpath);
        }
        //if file matches then copy the path and close directory
        else if (strcmp(root->d_name, name) == 0)
        {
            strcpy(finalpath, loc);
            closedir(dir);
            return;
        }
    }

    closedir(dir);
}


void sendResponse(int matchedFiles, char *finalFiles[], int file_counter){
    //required arguments for creating tar file
    char *tarArguments[1024] = {"tar", "-czf", "temp.tar.gz"};
    int sucess, argumentCount = 3;
    //if file is found sets success to 1, sends a success flag to the client, and appends the file paths (finalFiles[]) to the tarArguments array.
    if (matchedFiles)
    {
        sucess = 1;
        send(clientdecsriptor, &sucess, sizeof(int), 0);
        for (int i = 0; i < file_counter; i++)
        {
            //adding files to tar   arguments
            tarArguments[argumentCount++] = finalFiles[i];
        }
        // marking end of arguments
        tarArguments[argumentCount] = NULL; 
        //child process executing tar command
        int childProcessId = fork();
        if (childProcessId == 0)
        {
            execvp("tar", tarArguments);
            perror("execvp");
            exit(1);
        }
        else
        {
            int stats;
            waitpid(childProcessId, &stats, 0);
        }
        // sending generated tar file to client
        tarFile(clientdecsriptor);
    }
    else
    {
        sucess = 0;
        send(clientdecsriptor, &sucess, sizeof(int), 0);
        const char *message = "Nothing found\n";
        //No file find informing client
        send(clientdecsriptor, message, strlen(message), 0);
    }
}

void findFilesBetweenDate(const char *directory, time_t d1, time_t d2, char *finalFiles[], int *counter)
{
    DIR *dir;
    struct dirent *root;

    if ((dir = opendir(directory)) == NULL){
        return;
    }

    // iterate through all files
    while ((root = readdir(dir)) != NULL)
    {
        char loc[1024];
        //extracting name of file or directory
        snprintf(loc, sizeof(loc), "%s/%s", directory, root->d_name);
        struct stat info;

        // // if current file is directory then again goes inside it and search with in it
        if (root->d_type == DT_DIR)
        {
            if (strcmp(root->d_name, ".") == 0 || strcmp(root->d_name, "..") == 0)
                continue;
            findFilesBetweenDate(loc, d1, d2, finalFiles, counter);
        }
        else
        {
            // check time if it matches with in date then add into final files
            if (stat(loc, &info) == 0)
            {
                //converting the timeformat
                struct tm *localTime = localtime(&info.st_mtime);
                time_t localFileTime = mktime(localTime);
                if (localFileTime >= d1 && localFileTime <= d2)
                {
                    finalFiles[*counter] = strdup(loc);
                    (*counter)++;
                }
            }
        }
    }
    closedir(dir);
}

