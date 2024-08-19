#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_CONN 3

struct Entry {
char prompt[1024];
char options[3][50];
int answer_idx;
};

struct Player {
int fd;
int score;
char name[128];
};

void func_error(char* func){
    perror(func);
    exit(1);
}

void drop_player(struct Player* new_player){
    if(close((*new_player).fd) == -1) func_error("close");
    (*new_player).fd = -1;
    printf("Connection lost!\n"); if(fflush(stdout) == EOF) func_error("fflush");
}

int read_questions(struct Entry* arr, char* filename){
    
    FILE* fptr;
    int i = 0;
    char temp[50] = {0};

    if((fptr = fopen(filename,"r")) == NULL) func_error("fopen");

    while(1){
        
        if(fgets(arr[i].prompt, 128, fptr) == NULL) func_error("fgets");
        if(fscanf(fptr,"%s %s %s", arr[i].options[0], arr[i].options[1], arr[i].options[2]) == EOF) func_error("fscanf");
        if(fscanf(fptr,"%s", temp) == EOF) func_error("fscanf");
        for(int j = 0; j < 3; j++){
            if(strcmp(temp, arr[i].options[j]) == 0){
                arr[i].answer_idx = j;
                break;
            }
        }
        i++;
        if(fgets(temp, 50, fptr) == NULL) func_error("fgets");
        if(fgets(temp, 50, fptr) == NULL) {
            if(feof(fptr)) break;
            else func_error("fgets");
        }
        memset(temp, '\0', 50);
    }

    if(fclose(fptr) == EOF) func_error("fclose"); 
    return i;
}

void game(struct Entry* question, int num_questions, struct Player* player){

    fd_set myset;
    int recvbytes = 0;
    int maxfd = -1;
    char quest_buff[2048] = {0};

    printf("The game starts now!\n"); 

    for(int q_ind = 0; q_ind < num_questions; q_ind++){

        printf("<Press any key to continue>"); if(fflush(stdout) == EOF) func_error("fflush");

        if(fgets(quest_buff, 128, stdin) == NULL) func_error("fgets");

        //print question
        sprintf(quest_buff, "\nQuestion %d: %s", q_ind + 1, question[q_ind].prompt);
        printf("%s", quest_buff); if(fflush(stdout) == EOF) func_error("fflush");
        for(int j = 0; j < 3; j++){
            sprintf(quest_buff + strlen(quest_buff), "Press %d: %s\n", j + 1, question[q_ind].options[j]);
            printf("%d: %s\n", j + 1, question[q_ind].options[j]); if(fflush(stdout) == EOF) func_error("fflush");
        }
        
        for(int k = 0; k < MAX_CONN; k++){
            if(write(player[k].fd, quest_buff, strlen(quest_buff)) != strlen(quest_buff)) func_error("write");
        }

        //reinitalize fd-set
        FD_ZERO(&myset);
        maxfd = -1;
        for(int i = 0; i < MAX_CONN; i++){
            if(player[i].fd != -1) {
                FD_SET(player[i].fd, &myset);
                if(player[i].fd > maxfd) maxfd = player[i].fd;
            }
        }

        //monitor fd set
        if(select(maxfd + 1, &myset, NULL, NULL, NULL) == -1) func_error("select");

        //assign points
        char ans_buff = -1;
        for(int j = 0; j < MAX_CONN; j++){
            if(player[j].fd != -1 && FD_ISSET(player[j].fd, &myset)){
                if((recvbytes = read(player[j].fd, &ans_buff, 1)) == -1) func_error("read");
                if(recvbytes == 0){
                    FD_CLR(player[j].fd, &myset);
                    drop_player(&(player[j]));

                    for(int i = 0; i < MAX_CONN; i++){
                        if(i != j) if(close(player[i].fd) == -1) func_error("close");
                    }
                    exit(EXIT_SUCCESS);

                } else {
                    if(question[q_ind].answer_idx == ans_buff - '1') player[j].score++;
                    else player[j].score--;
                }
            }
        }

        //print answer
        sprintf(quest_buff, "Question %d answer: %s\n", q_ind + 1, question[q_ind].options[question[q_ind].answer_idx]);
        printf("%s", quest_buff); if(fflush(stdout) == EOF) func_error("fflush");
        for(int j = 0; j < MAX_CONN; j++){
            if(player[j].fd != -1){
                if(write(player[j].fd, quest_buff, strlen(quest_buff)) != strlen(quest_buff)) func_error("write");
            }
        }
        sprintf(quest_buff, "\nScore board:\n");
        for(int j = 0; j < MAX_CONN; j++){ 
            sprintf(quest_buff + strlen(quest_buff), "%s: %d\n", player[j].name, player[j].score);
        }
        printf("%s", quest_buff);
    }
    struct Player winner[MAX_CONN];
    int max_score = player[0].score;
    for(int i = 1; i < MAX_CONN; i++){
        if(max_score < player[i].score) max_score = player[i].score;
    }
    printf("\nCongrats,");
    for(int i = 0; i < MAX_CONN; i++){
        if(max_score == player[i].score) printf(" %s", player[i].name);
        if(close(player[i].fd) == -1) func_error("close");
    }
    printf("!\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]){

    int opt;
    char flag_h = 0;
    char* question_file = "questions.txt";
    char* IP_address = "127.0.0.1";
    int port_number = 25555;

    while ((opt = getopt(argc, argv, ":f:i:p:h")) != -1){
        switch(opt){
        case 'f':
            question_file = optarg;
            break;
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
        printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", argv[0]); if(fflush(stdout) == EOF) func_error("fflush");
        printf("  -f question_file \tDefault to \"questions.txt\";\n"); if(fflush(stdout) == EOF) func_error("fflush");
        printf("  -i IP_address \tDefault to \"127.0.0.1\";\n"); if(fflush(stdout) == EOF) func_error("fflush");
        printf("  -p port_number \tDefault to 25555;\n"); if(fflush(stdout) == EOF) func_error("fflush");
        printf("  -h \t\t\tDisplay this help info.\n"); if(fflush(stdout) == EOF) func_error("fflush");
        exit(EXIT_SUCCESS);
    }
    
    int    server_fd;
    int    client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);

    /* STEP 1
        Create and set up a socket
    */
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) func_error("socket");
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    /* STEP 2
        Bind the file descriptor with address structure
        so that clients can find the address
    */
    if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) func_error("bind");

    /* STEP 3
        Listen to at most 5 incoming connections
    */
    if (listen(server_fd, MAX_CONN) != 0) func_error("listen");

    struct Entry questions[50] = {0};
    int num_questions = read_questions(questions, question_file);
    printf("Welcome to 392 Trivia!\n"); if(fflush(stdout) == EOF) func_error("fflush");

    /* STEP 4
        Accept connections from clients
        to enable communication
    */
    fd_set myset;
    FD_SET(server_fd, &myset);
    int maxfd = server_fd;
    int n_conn = 0;

    struct Player players[MAX_CONN] = {0};
    for(int i = 0; i < MAX_CONN; i++) players[i].fd = -1;

    int recvbytes = 0;
    char name_prompt[] = "Please type your name:";
    char  buffer[1024];

    while(1){

        //reinitalize fd-set
        FD_ZERO(&myset);
        FD_SET(server_fd, &myset);
        maxfd = server_fd;
        for(int i = 0; i < MAX_CONN; i++){
            if(players[i].fd != -1){
                FD_SET(players[i].fd, &myset);
                if(players[i].fd > maxfd) maxfd = players[i].fd;
            }
        }

        //monitor fd set
        if(select(maxfd + 1, &myset, NULL, NULL, NULL) == -1) func_error("select");

        //check new incoming connection
        recvbytes = 0;
        if(FD_ISSET(server_fd, &myset)){
            if((client_fd  =   accept(server_fd, (struct sockaddr*)&in_addr, &addr_size)) == -1) func_error("accept");
            if(n_conn < MAX_CONN){

                struct Player* new_player;
                for(int i = 0; i < MAX_CONN; i++){
                    if(players[i].fd == -1){
                        players[i].fd = client_fd;
                        new_player = &(players[i]);
                        break;
                    }
                }

                //request name
                printf("New connection detected!\n"); if(fflush(stdout) == EOF) func_error("fflush");
                if(write((*new_player).fd, name_prompt, strlen(name_prompt)) != strlen(name_prompt)) func_error("write");

                //recive name
                if((recvbytes = read((*new_player).fd, (*new_player).name, 128)) == -1) func_error("read");
                if(recvbytes == 0) drop_player(new_player);
                else {
                    (*new_player).name[recvbytes] = '\0';
                    printf("Hi %s!\n", (*new_player).name); if(fflush(stdout) == EOF) func_error("fflush");
                    n_conn++;
                }
                if(n_conn == MAX_CONN) game(questions, num_questions, players);
                
            } else {
                if(close(client_fd) == -1) func_error("close");
                printf("Max connection reached!\n"); if(fflush(stdout) == EOF) func_error("fflush");
            }
        }

        for(int i = 0; i < MAX_CONN; i++){
            if(players[i].fd != -1 && FD_ISSET(players[i].fd, &myset)){
                if((recvbytes = read(players[i].fd, buffer, 1024)) == -1) func_error("read");
                if(recvbytes == 0){
                    FD_CLR(players[i].fd, &myset);
                    drop_player(&(players[i]));
                    n_conn--;
                }
            }
        }
    }

    if(close(server_fd) == -1) func_error("close");
    exit(EXIT_SUCCESS);
}
