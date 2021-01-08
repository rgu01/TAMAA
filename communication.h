#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "common.h"

int initialize_sock(const char *ip, const int port);
int accept_client(const int server_sockfd);
int readMessageFromClient(int fileDescriptor);
int receive_msg_from_mmt(const int client_sockfd);
int receive_xml_from_mmt(const int client_sockfd, const char* file_path);
int send_msg_to_mmt(int client_sockfd, const char* msg, const int size);
void send_file_to_mmt(int client_sockfd, const char *file_name);
void close_communication(int sock);
void write_file_test(const char* file_path);

#endif