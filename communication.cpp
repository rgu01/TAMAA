#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "communication.h"

#define BUFSIZE BUFSIZ

int initialize_sock(const char *ip, const int port)
{
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >= 0);
    int reuse = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    
    int ret = bind(sock,(struct sockaddr *)&address,sizeof(address));
    assert(ret != -1);

    ret = listen(sock,5);
    assert(ret != -1);

    return sock;
}

int accept_client(const int server_sockfd)
{
    int client_sockfd;
    socklen_t client_len;
    struct sockaddr_in client_address;

    client_len = sizeof(client_address);
    puts("ACCept: waiting for client");
    client_sockfd = accept(
        server_sockfd,
        (struct sockaddr*)&client_address,
        &client_len
    );
    inet_ntoa(client_address.sin_addr);
    ntohs(client_address.sin_port);
    printf("ACCept: get one client: %d\n", client_sockfd);

    return client_sockfd;
}

void write_file_test(const char* file_path)
{
    char buffer[2] = {'a', 'b'};
    int filefd = 0;
    FILE *fp = fopen(file_path, "w");  
    if(NULL == fp)  
    {  
        printf("File:\t%s Can Not Open To Write\n", file_path);  
        exit(1);  
    } 

    if(fwrite(buffer, sizeof(char), 2, fp) < 2)  
    {  
        printf("File:\t%s Write Failed\n", file_path);   
    }  
}

int receive_xml_from_mmt(const int client_sockfd, const char* file_path)
{
    char buffer[BUFSIZE];
    ssize_t read_return;
    FILE *fp;

    bzero(buffer, BUFSIZE);  
    int length = 0; 
    int progress = 0;

    if (client_sockfd < 0) {
        printf("RECeive: errno is :%d\n",errno);
    }else {
        printf("RECeive: start to receive file from client: %d\n", client_sockfd);

        fp = fopen(file_path, "w");  
        if(NULL == fp)  
        {  
            printf("File:\t%s Can Not Open To Write\n", file_path);  
            exit(1);  
        } 
        //while((length = recv(client_sockfd, buffer, BUFSIZE, 0)) > 0)  
        while((length = read(client_sockfd, buffer, BUFSIZE)) > 0)  
        {  
            printf("%d, ", progress++);  
            if(fwrite(buffer, sizeof(char), length, fp) < length)  
            {  
                printf("File:\t%s Write Failed\n", file_path);  
                break;  
            }  
            bzero(buffer, BUFSIZE);  
        }  
        
        fflush(fp);
        if(progress > 0)
        {
            printf("\n");
            printf("Receive File:\t%s From Server IP Successfully!\n\n", file_path); 
        } 
        else
        {
            return(-1);
        }
        fclose(fp);  
        //close(client_sockfd); 

        /*printf("RECeive: waiting for msg from client: %d\n", client_sockfd);
        read_return = read(client_sockfd, buffer, BUFSIZE);
        if(read_return < 0)
        {
            perror("RECeive: Could not read data from client");
            exit(EXIT_FAILURE);
        }
        else
        {
            if(read_return == 0)
            {
                return(-1);
            }
            else
            {
                if(read_return == 2 && buffer[0] == '\r' && buffer[1] == '\n')
                {
                    return 2;
                }
                else if(strcmp(buffer, "bye\r\n") == 0)
                {
                    printf("RECeive: A client has disconnected!\n");
                    return(-1);
                }
                else
                {
 
                }
            }
        }*/
    }

    return 0;  
}

int receive_msg_from_mmt(const int client_sockfd)
{
    char buffer[BUFSIZ];
    int filefd, n;
    ssize_t read_return;

    if (client_sockfd < 0) {
        printf("RECeive: errno is :%d\n",errno);
    }else {
        bzero(buffer, BUFSIZ); 
        printf("RECeive: waiting for msg from client: %d\n", client_sockfd);
        read_return = read(client_sockfd, buffer, BUFSIZ);
        if(read_return < 0)
        {
            perror("RECeive: Could not read data from client");
            exit(EXIT_FAILURE);
        }
        else
        {
            if(read_return == 0)
            {
                /*end of file*/
                return(0);
            }
            else
            {
                if(read_return == 2 && buffer[0] == '\r' && buffer[1] == '\n')
                {
                    //nothing
                    return 2;
                }
                else if(strcmp(buffer, "ENDOFXML\r\n") == 0)
                {
                    printf("RECeive: A client has disconnected!\n");
                    return(-1);
                }
                else
                {
                    send(client_sockfd, "got it\n", sizeof("got it\n"), 0);
                    printf("RECeive: Incoming message (length: %d): %s\n", read_return, buffer);
                }
            }
        }
    }

    return 0;

        /*filefd = open(file_path,
                O_WRONLY | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWUSR);
        if (filefd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        do {
            read_return = read(client_sockfd, buffer, BUFSIZ);
            if (read_return == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            if (write(filefd, buffer, read_return) == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        } while (read_return > 0);
        close(filefd);*/
}

int readMessageFromClient(int fileDescriptor)
{
	char buffer[BUFSIZ];
	char server_msg[256] = "I hear you, dude...\n";
	int nOfBytes;

	nOfBytes = read(fileDescriptor, buffer, BUFSIZ);
	if(nOfBytes < 0) {
		perror("Could not read data from client\n");
		exit(EXIT_FAILURE);
	}
	else
	if(nOfBytes == 0)
		/* End of file */
		return(-1);
	else if(strcmp(buffer, "quit") == 0){
		printf(">A client has disconnected.\n");
		return(-1);
	}
	else if(nOfBytes == 2 && buffer[0] == '\r' && buffer[1] == '\n')
    {
        //nothing
        return(2);
    }
    else
    {
		/* Data read */
		printf(">Incoming message: %s\n",  buffer);
		/* Sending a message back to the client, confirming that the message has been read */
        //printf("send a message back to client\n");
		//send(fileDescriptor, server_msg, sizeof(server_msg), 0); 
        //send(fileDescriptor, "hear you...", sizeof("hear you..."), 0);
    }
	return(0);
}

int send_msg_to_mmt(int client_sockfd, const char* msg, const int size)
{
    //int n = write(client_sockfd, "I got your message", 18);
    int n = send(client_sockfd, msg, size, 0);
    //int n = send(client_sockfd, "hear you...\n", sizeof("hear you...\n"), 0);
    if (n < 0) perror("SENd_MSG: ERROR writing to socket");
    printf("SENd_MSG: send msg to client %d\n\n", n);

    return n;
}

void send_file_to_mmt(int server_sockfd, const char *file_name)
{
    int filefd = open(file_name,O_RDONLY);
    assert(filefd > 0);
    struct stat file_stat;
    int length = 0;

    //为了获取文件大小
    fstat(filefd,&file_stat);

    /*length = sendfile(client_sockfd,filefd,NULL,file_stat.st_size);
    printf("SENd_file: send a file to client %d\n\n", length);*/

    struct sockaddr_in client;
    socklen_t client_addrlen = sizeof(client);
    int connfd = accept(server_sockfd,(struct sockaddr *)&client,&client_addrlen);
    if (connfd < 0) {
        printf("errno is :%d\n",errno);
    }else {
        length = sendfile(connfd,filefd,NULL,file_stat.st_size);
        printf("SENd_file: send a file to client %d\n\n", length);
        close(filefd);
        close(connfd);
    }
}


void close_communication(int sock)
{
       close(sock);
}
