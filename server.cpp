#include "common.h"
#include "agent.h"
#include "tracer.h"
#include "server.h"
#include "xmlHandler.h"
#include "communication.h"
#include <pthread.h> 
#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <thread>
#include <chrono>
#include <future>

using namespace std;

bool go;
//int counter = 1;

void parse(vector<Agent> agents)
{
    const string path_if = "sh/model/ta_model_agents.if";
    const string path_xtr = "sh/traces/t-1.xtr";
    //string path_xtr = "sh/traces/t-"+std::to_string(counter)+".xtr";
    const string path_out_xml = "result/plan.xml";

    //counter++;
    clear();

    system("sh/cmd.sh");
    printf("lalala.....\n");
    /* Load model in intermediate format.
    */
    ifstream file_if(path_if);
    if (!file_if)
    {
        perror(path_if.c_str());
        exit(EXIT_FAILURE);
    }
    loadIF(file_if);
    file_if.close();

    /* Load trace.
    */
    ifstream file_xtr(path_xtr);
    if (!file_xtr)
    {
        perror(path_xtr.c_str());
        exit(EXIT_FAILURE);
    }
    //loadTrace(file_xtr);
    test(file_xtr, agents);
    file_xtr.close();

    createXmlFile(path_out_xml, agents);
}

void* connect(void* args)
//void connect()
{
    //const string path_if = "sh/model/ta_model_agents.if";
    //const string path_xtr = "sh/traces/t-1.xtr";
    const string path_out_xml = "result/plan.xml";
    vector<Agent> agents;

    const string path_model = "sh/model/ta_model_agents_test.xml";
    const char *ip = "localhost";
    const int port = 9779;
    struct timeval timeout = {10,0};
    int server_sock;
    int client_sock;
    int i, read_return, heartbeat = 0;
    fd_set activeFdSet, readFdSet;
    socklen_t size;

    try
    {
        server_sock = initialize_sock(ip, port);
        FD_ZERO(&activeFdSet);
        FD_SET(server_sock, &activeFdSet);

        while(go)
        {
            readFdSet = activeFdSet;
            if(select(FD_SETSIZE, &readFdSet, NULL, NULL, &timeout) < 0)
            {
                perror("Select failed\n");
            }
            timeout.tv_sec = 10;
            for(i = 0; i < FD_SETSIZE; i++)
            {
                if(FD_ISSET(i, &readFdSet))
                {
                    if(i == server_sock)
                    {
                        printf("SERver: accepting file..............\n");
                        client_sock = accept_client(server_sock);
                        FD_SET(client_sock, &activeFdSet);
                    }
                    else if(i == client_sock)
                    {
                        if(receive_xml_from_mmt(i, path_model.c_str()) != -1)
                        {
                            parse(agents);

                            close(i);
                            FD_CLR(i, &activeFdSet);
                            client_sock = -1;
                            send_file_to_mmt(server_sock, path_out_xml.c_str());
                            //go = false;
                        }
                        else
                        {
                            close(i);
                            FD_CLR(i, &activeFdSet);
                        }
                        
                    }
                }
            }
            
            
            printf("Tick: %d\n", heartbeat++);
            
        
                /*if(client_sock > 0)
                {
                    send_file_to_mmt(client_sock, path_out_xml.c_str());
                    close(client_sock);
                    FD_CLR(client_sock, &activeFdSet);
                    client_sock = -1;
                }*/
        }
        
        if(server_sock > 0)
        {
            close_communication(server_sock);
        }
    }
    catch (std::exception &e)
    {
        cerr << "SUB: Caught exception: " << e.what() << endl;
    }

    puts("SUB: Quit the sub process!");
    pthread_exit(0);
}


int main(int argc, char *argv[])
{
    pthread_t pid;
    string quit;
    //const string path_model = "sh/model/ta_model_agents_test.xml";

    go = true;
    int ret = pthread_create(&pid, NULL, connect, NULL);
    if (ret != 0)
    {
        cout << "MAIn: pthread_create error: error_code=" << ret << endl;
    }

    //write_file_test(path_model.c_str());

    while(true)
    {
        puts("MAIn: New round! please input your command:");
        std::cin >> quit;
        /*if(go == false)
        {
            go = true;
            ret = pthread_create(&pid, NULL, connect, NULL);
            if (ret != 0)
            {
                cout << "MAIn: pthread_create error: error_code=" << ret << endl;
            }
        }*/
        if(quit.compare("quit") == 0)
        {
            go = false;
            break;
        }
    }

    puts("MAIn: Quit the main program!\nPlease wait for the sub thread quit...");
    pthread_exit(NULL);
}