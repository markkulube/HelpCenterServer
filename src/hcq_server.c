#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include "hcq.h"

#define INPUT_BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 3  
#define DELIM " \n"


#define PORT 53691
#define USER_NAME 32
#define USERTYPE 8
#define STATUS 32
#define BUFSIZE 128
#define GARBAGE 128

#ifndef PORT
  #define PORT 53691
#endif

#define MAX_BACKLOG 5
#define MAX_CONNECTIONS 12
#define BUF_SIZE 128


struct sockname {
    int sock_fd;
    char *username;
    char *usertype;
    char *status;
};


// Use global variables so we can have exactly one TA list and one student list
Ta *ta_list = NULL;
Student *stu_list = NULL;

Course *courses;  
int num_courses = 3;

fd_set all_fds;

/* Accept a connection. Note that a new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int fd, struct sockname *usernames) {
    int user_index = 0;
    while (user_index < MAX_CONNECTIONS && usernames[user_index].sock_fd != -1) {
        user_index++;
    }

    if (user_index == MAX_CONNECTIONS) {
        fprintf(stderr, "server: max concurrent connections\n");
        return -1;
    }

    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    usernames[user_index].sock_fd = client_fd;
    return client_fd;
}

int disconnect_student(char *stu_name, char *ta_name, struct sockname *usernames) {
    for (int index = 0; index < MAX_CONNECTIONS; index++){


                
                if ((usernames[index].sock_fd!=-1) && (strcmp(usernames[index].username, stu_name) == 0)) {
                    dprintf(usernames[index].sock_fd,"Go see TA: %s. You are now being disconnected.\n", ta_name);
                    if(close(usernames[index].sock_fd) == -1) {
                        perror("disconnect_student: closing student fd");
                    }

                    FD_CLR(usernames[index].sock_fd, &all_fds);
                    break;
                }
            }
    return 0;
}

/* Read a message from client_index and echo it back to them.
 * Return the fd if it has been closed or 0 otherwise.
 */
int read_from(int client_index, struct sockname *usernames) {


   
    int fd = usernames[client_index].sock_fd;

    int num_read = 0;
    char command[6];
    if (strcmp("T", usernames[client_index].usertype) == 0 || strcmp("S", usernames[client_index].usertype) == 0) {

        for (int j = 0; j < MAX_CONNECTIONS; j++) {

            if(usernames[j].sock_fd != -1 && usernames[j].sock_fd == usernames[client_index].sock_fd){

                    num_read = read(usernames[client_index].sock_fd, command, 6);

                    if (strcmp("T", usernames[client_index].usertype) == 0) {

                        command[5] ='\0';
                        if ((strncmp("stats", command, 5) == 0)) {
                            char *buf = malloc(sizeof(char)*1024);
                            buf=print_full_queue(stu_list);
                            dprintf(usernames[client_index].sock_fd, "%s\n", buf);
                            free(buf);
                            break;

                        } else if(strncmp("next", command, 4)==0){
                            if (stu_list != NULL) {
                                    disconnect_student(stu_list->name, usernames[client_index].username, usernames);
                                    next_overall(usernames[client_index].username, &ta_list, &stu_list);
                                }
                         
                        } else if(command[0] != '\r'){
                                dprintf(usernames[client_index].sock_fd, "Incorrect syntax2\n");
                            }
                    }

                    if (strcmp("S", usernames[client_index].usertype) == 0) {
                        command[5] ='\0';
                        if ((strncmp("stats", command, 5) == 0)) {
                            char *buf = malloc(sizeof(char)*1024);
                            buf=print_currently_serving(ta_list);
                            dprintf(usernames[client_index].sock_fd, "%s\n", buf);
                            free(buf);
                        } else if(command[0] != '\r'){
                            dprintf(usernames[client_index].sock_fd, "Incorrect syntax3\n");
                        }
                    }
            }
        }
    }

    

    if (num_read == 0 /*|| written != strlen(usercpy)*/) {
        close(usernames[client_index].sock_fd);
        usernames[client_index].sock_fd = -1;
        return fd;
    }

    return 0;
}


int main(void) {


     Course *courses;
    if ((courses = malloc(sizeof(Course) * 3)) == NULL) {
        perror("malloc for course list\n");
        exit(1);
    }
    strcpy(courses[0].code, "CSC108");
    strcpy(courses[1].code, "CSC148");
    strcpy(courses[2].code, "CSC209");

    struct sockname usernames[MAX_CONNECTIONS];
    for (int index = 0; index < MAX_CONNECTIONS; index++) {
        usernames[index].sock_fd = -1;
        usernames[index].username = NULL;
    }

    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("server: socket");
        exit(1);
    }

    // Set information about the port (and IP) we want to be connected to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    // This should always be zero. On some systems, it won't error if you
    // forget, but on others, you'll get mysterious errors. So zero it.
    memset(&server.sin_zero, 0, 8);

    // Bind the selected port to the socket.
    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("server: bind");
        close(sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(sock_fd);
        exit(1);
    }

    // The client accept - message accept loop. First, we prepare to listen to multiple
    // file descriptors by initializing a set of file descriptors.
    int max_fd = sock_fd;
    
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);


    while (1) {

        // select updates the fd_set it receives, so we always use a copy and retain the original.
        fd_set listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }


        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, usernames);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            //printf("Connected user: ");
            dprintf(client_fd, "Welcome to the Help Centre, what is your name?\n");
            char username[USER_NAME];
            int user_len= 0;
            int num_read = read(client_fd, username, USER_NAME);
            for (int i = 0; i < num_read; i++)
            {
                if (username[i] == '\n')
                {   
                    username[i] = '\0';
                    user_len = i + 1;
                }
            }

            int usertype_index = 0;
            for (int index = 0; index < MAX_CONNECTIONS; index++)
            {
                if (usernames[index].sock_fd == client_fd)
                {   usertype_index = index;
                    usernames[index].username = malloc(sizeof(char) * user_len);
                    strncpy(usernames[index].username, username, user_len);
                    usernames[index].username[user_len-1]='\0';
                }
            }

            // Ask for the usertype
            char usertype[USERTYPE];
            dprintf(client_fd, "Are you are a TA or Student(enter T or S)\n");
            while(1) {
                num_read = read(client_fd, usertype, USERTYPE);
                usertype[1]='\0';
                usernames[usertype_index].usertype = malloc(sizeof(char)*2);
                strncpy(usernames[usertype_index].usertype, usertype, 2);
                usernames[usertype_index].usertype[1]='\0';

                if ((strcmp("T", usernames[usertype_index].usertype) == 0) | (strcmp("S", usernames[usertype_index].usertype) == 0))
                {  
                    break;
                } else  {
                    dprintf(client_fd, "Invalid (enter T or S)\n");
                }
            }
            

            // reading 6 bytes for the course code
            char course[7] = {'\0'};
            if (strcmp("T", usertype) == 0) {

                dprintf(client_fd, "Valid commands for TA:\n\tstats\n\tnext\n\t(or use Ctrl-C to leave)\n");
                add_ta(&ta_list, usernames[usertype_index].username);
            }
            if (strcmp("S", usertype) == 0) {
                 dprintf(client_fd,"Valid courses: CSC108, CSC148, CSC209 Which course are you asking about?\n");
                 while(1){
                    num_read = read(client_fd, course, 8);
                    course[6]='\0';
                    if ((strcmp("CSC108", course) == 0) || (strcmp("CSC148", course) == 0) || (strcmp("CSC209", course) == 0)) {
                        add_student(&stu_list, usernames[usertype_index].username, course, courses, num_courses);
                        dprintf(client_fd,"You have been entered into the queue. While you wait, you can use the command stats to see which TAs are currently serving students.\n");
                        break;
                    } else if(course[0] != '\r') {
                        dprintf(client_fd, "Incorrect syntax1\n");
                    }
                }      
            }

            if (strcmp(usernames[usertype_index].username, "")== 0)
            {   
                usernames[usertype_index].sock_fd =-1;
                usernames[usertype_index].username =NULL;
                FD_CLR(usernames[usertype_index].sock_fd, &all_fds);
            }
                
        }

        // Next, check the clients.
        // NOTE: We could do some tricks with nready to terminate this loop early.
        for (int index = 0; index < MAX_CONNECTIONS; index++) {

            if (usernames[index].sock_fd > -1 && FD_ISSET(usernames[index].sock_fd, &listen_fds)) {
                // Note: never reduces max_fd
                int client_closed = read_from(index, usernames);

                if (client_closed > 0) {
                    FD_CLR(client_closed, &all_fds);
                    //printf("Disconnected user: %s client: %d \n",usernames[index].username, client_closed);
                    remove_ta(&ta_list, usernames[index].username);
                    free(usernames[index].username);
                    usernames[index].username = NULL;
                } else if (client_closed == 0) {
                    //printf("Echoing message from user: %s client: %d\n", usernames[index].username, usernames[index].sock_fd);

                }              
            }
        }
    }

    // Should never get here.
    return 1;
}
