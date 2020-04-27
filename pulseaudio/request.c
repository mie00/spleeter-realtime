#include <stdio.h> /* printf, sprintf */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */

#define error(msg) {perror(msg); ret = -2; goto done;}

int create_socket() {
    int ret = 0;

    int portno =        8083;
    char *host =        "127.0.0.1";

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd;

    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL) error("ERROR, no such host");

    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    /* connect the socket */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    
    ret = sockfd;
done:
    return ret;
}

int send_sound(int sockfd, unsigned long channel, unsigned long sample_rate, long len, void* input, void* output) {
    int ret = 0;

    int bytes, sent, received, total;

    bytes = write(sockfd,&channel,sizeof(channel));
    if (bytes != sizeof(channel)) {
        error("cannot send channel")
    }

    bytes = write(sockfd,&sample_rate,sizeof(sample_rate));
    if (bytes != sizeof(sample_rate)) {
        error("cannot send sample_rate")
    }

    bytes = write(sockfd,&len,sizeof(long));
    if (bytes != sizeof(long)) {
        error("cannot send len")
    }

    total = len * sizeof(float);
    received = 0;
    sent = 0;

    do {
        bytes = write(sockfd,input+sent,total-sent);
        if (bytes < 0)
            error("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);


    do {
        bytes = read(sockfd,output+received,total-received);
        if (bytes < 0)
            error("ERROR reading response from socket");
        if (bytes == 0)
            break;
        received+=bytes;
    } while (received < total);

    if (received < total) {
        error("received less data")
    }

done:
    return ret;
}