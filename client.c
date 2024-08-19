/*******************************************************************************
 * Name        : client.c
 * Author      : Cormac Taylor
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

/*** socket/demo2/client.c ***/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define STDIN 0

void func_error(char* func){
    perror(func);
    exit(1);
}

void parse_connect(int argc, char** argv, int* server_fd){

    int opt;
    char flag_h = 0;
    char* IP_address = "127.0.0.1";
    int port_number = 25555;

    while ((opt = getopt(argc, argv, ":i:p:h")) != -1){
        switch(opt){
        case 'i':
            IP_address = optarg;
            break;
        case 'p':
            port_number = atoi(optarg);
            break;
        case 'h':
            flag_h = 1;
            break;
       default:
            fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
            exit(EXIT_FAILURE);
        }
    }

    if(flag_h){
        printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
        printf("  -i IP_address \tDefault to \"127.0.0.1\";\n");
        printf("  -p port_number \tDefault to 25555;\n");
        printf("  -h \t\t\tDisplay this help info.\n");
        exit(EXIT_SUCCESS);
    }

    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);

    /* STEP 1:
    Create a socket to talk to the server;
    */
    if((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) func_error("socket");
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    /* STEP 2:
    Try to connect to the server.
    */
    if(connect(*server_fd, (struct sockaddr *) &server_addr, addr_size) == -1) func_error("connect");
}


int main(int argc, char *argv[]){

    int    server_fd;
    parse_connect(argc, argv, &server_fd);

    char buffer[2048] = {0};
    int recvbytes = 0;

    if((recvbytes = read(server_fd, buffer, 1024)) == -1) func_error("read");
    buffer[recvbytes] = '\0';
    printf("%s ", buffer);
    if(fgets(buffer, 128, stdin) == NULL) func_error("fgets");
    if(write(server_fd, buffer, strlen(buffer) - 1) != strlen(buffer) - 1) func_error("write");

    fd_set myset;
    char answer[128];

    while(1){

        //get question
        FD_ZERO(&myset);
        FD_SET(server_fd, &myset);
        FD_SET(STDIN, &myset);

        if(select(server_fd + 1, &myset, NULL, NULL, NULL) == -1) func_error("select");

        if(FD_ISSET(server_fd, &myset)){
            if((recvbytes = read(server_fd, buffer, 2048)) == -1) func_error("read");

            if(recvbytes == 0) break;
            else {
                buffer[recvbytes] = '\0';
                printf("%s", buffer); if(fflush(stdout) == EOF) func_error("fflush");
            }
        }

        //get answer
        if(FD_ISSET(STDIN, &myset)){
            if(fgets(answer, 128, stdin) == NULL) func_error("fgets");
            if(write(server_fd, answer, 1) != 1) func_error("write");
        }
    }

    if(close(server_fd) == -1) func_error("close");
    return 0;
}
