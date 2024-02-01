#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <sched.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "lib/queue.h"
#include "lib/nlohmann/json.hpp"
#include "lib/customer.h"
#include "lib/account.h"
#include "lib/transaction.h"
#include "addresses.h"
#include <vector>
#include <sqlite3.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <unordered_map>


//using namespace std;
using json = nlohmann::json;

#define MAX_MSG_SIZE 1024
#define THREADPOOL_SIZE 20

/*Declare global variables*/
pthread_mutex_t admin_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sockets_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::unordered_map<pthread_t, bool> isBusy;
long ADMIN;
sqlite3* db;
queue* q;



void sendFile(int connfd, const char* filename){
        char recvBuff[MAX_MSG_SIZE], sendBuff[MAX_MSG_SIZE];

        memset(recvBuff, '0', sizeof(recvBuff));
        memset(sendBuff, '0', sizeof(sendBuff)); 
        //if file request was received
        //send file size

        ssize_t sent_bytes, remain_data;
        off_t offset = 0;
        int fd;
        fd = open(filename, O_RDONLY);
        if (fd == -1)
        {
                fprintf(stderr, "Error opening file --> %s", strerror(errno));

                //exit(EXIT_FAILURE);
                return;
        }
        struct stat file_stat;
        /* Get file stats */
        if (fstat(fd, &file_stat) < 0)
        {
                fprintf(stderr, "Error fstat --> %s", strerror(errno));

                //exit(EXIT_FAILURE);
                return;
        }

        //send filesize
        snprintf(sendBuff, sizeof(sendBuff), "%ld", file_stat.st_size);
        send(connfd, sendBuff, strlen(sendBuff),0); 

        //receive confirmation of receiving file size, aka "green light"
        int numbytes = recv(connfd,recvBuff,sizeof(recvBuff)-1,0);
        if (numbytes == -1){
        perror("recv");
        //exit(1);
        return;
        }
        recvBuff[numbytes] = '\0';
        printf("recv: %s\n", recvBuff);
        
        remain_data = file_stat.st_size;
        // printf("%ld, %ld", offset, remain_data);
        /* Sending file data */
        while (offset < remain_data) {
                ssize_t read_bytes = read(fd, sendBuff, sizeof(sendBuff));
                if (read_bytes <= 0) {
                        // Error or end of file
                        if (read_bytes < 0) {
                        perror("read");
                        }
                        break;
                }
                ssize_t sent_bytes = send(connfd, sendBuff, read_bytes, 0);
                if (sent_bytes <= 0) {
                        // Error or connection closed
                        if (sent_bytes < 0) {
                        perror("send");
                        }
                        break;
                }
                offset += sent_bytes;
                printf("Sent %ld bytes from file's data, offset is now: %ld\n", sent_bytes, offset);
                numbytes = recv(connfd,recvBuff,sizeof(recvBuff)-1,0);
                sendBuff[sent_bytes] = '\0';
                printf("%s\n", sendBuff);
                if (numbytes == -1){
                perror("recv");
                //exit(1);
                //return;
                break;
                }

                recvBuff[numbytes] = '\0';
                printf("recv: %s\n", recvBuff);
                }
        sprintf(sendBuff,"sendFile done");
        printf("%s\n", sendBuff);
        send(connfd, sendBuff, strlen(sendBuff),0); 
        
        close(fd);
}

void receiveFile(int connfd, const char* filename){
        char recvBuff[MAX_MSG_SIZE], sendBuff[MAX_MSG_SIZE];

        memset(recvBuff, '0', sizeof(recvBuff));
        memset(sendBuff, '0', sizeof(sendBuff)); 
        ssize_t size, remain_data, len;
        FILE* received_file;
        //if file request was received
        //send confirmation
        sprintf(sendBuff,"go-file");
        printf("%s\n", sendBuff);
        send(connfd, sendBuff, strlen(sendBuff),0);  
        //receive filesize
        int numbytes = recv(connfd,recvBuff, sizeof(recvBuff),0);
        recvBuff[numbytes] = '\0';
        printf("recv: %s\n", recvBuff);
        sscanf(recvBuff, "%ld", &size);

        //send confirmation
        sprintf(sendBuff,"ok");
        printf("%s\n", sendBuff);
        send(connfd, sendBuff, strlen(sendBuff),0); 

        
        //prepare file to receive message aka open file
        received_file = fopen(filename, "w");
        if (received_file == NULL)
        {
                fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));
                return;
                //exit(EXIT_FAILURE);
        }

        //receive content of file
        remain_data = size;
        printf("%ld, %ld\n", remain_data, len);
        while ((remain_data > 0) && ((len = recv(connfd, recvBuff, sizeof(recvBuff), 0)) > 0))
        {
                recvBuff[len] = '\0';
                // printf("len: %ld ", len);
                printf("%s\n", recvBuff);
                snprintf(sendBuff, sizeof(sendBuff), "%ld",len);
                send(connfd, sendBuff, strlen(sendBuff), 0);
                fwrite(recvBuff, sizeof(char), len, received_file);
                remain_data -= len;
                fprintf(stdout, "Receive %ld bytes and we hope :- %ld bytes\n", len, remain_data);
                
        }

        if (len == 0)
        {
        // Transmission complete
        printf("File transfer completed.\n");
        }
        else if (len < 0)
        {
        // Error occurred during recv()
        perror("Error receiving data");
        }

        fclose(received_file);
}  

void doProcessing (int connfd)
{
        int a, result, numbytes;
        std::string thread_id_str = std::to_string(pthread_self());
        std::string requestFilePath = std::string("requests/request" + thread_id_str + ".json");
        std::string responseFilePath = std::string("requests/response" + thread_id_str + ".json");
        int adminConnected = 0;
        char recvBuff[MAX_MSG_SIZE], sendBuff[MAX_MSG_SIZE];

        pthread_mutex_lock(&sockets_mutex);
        isBusy[pthread_self()] = true;
        pthread_mutex_unlock(&sockets_mutex);

        /*receive data from the client*/
        numbytes = recv(connfd,recvBuff, sizeof(recvBuff),0);
        if (numbytes == -1){
                perror("recv");
                return;
                //exit(1);
        }

        printf("recv: %s\n", recvBuff);
        if(strcmp(recvBuff, "admin") == 0){
                if(!ADMIN){
                        pthread_mutex_lock(&admin_mutex);
                        adminConnected = 1;
                        ADMIN = pthread_self();
                        
                        
                        printf("admin1 TID: %ld", pthread_self());
                        //send confirmation of admin
                        sprintf(sendBuff,"admin ok");
                        printf("%s\n", sendBuff);
                        send(connfd, sendBuff, strlen(sendBuff),0); 
                        pthread_mutex_unlock(&admin_mutex);

                }else{
                        //send refusal of admin
                        sprintf(sendBuff,"Nok: Admin already connected");
                        printf("%s\n", sendBuff);
                        send(connfd, sendBuff, strlen(sendBuff),0); 
                        shutdown(connfd, SHUT_RDWR);
                        close(connfd);
                        return;
                }
        }else{
                //send confirmation of client
                sprintf(sendBuff,"client ok");
                printf("%s\n", sendBuff);
                send(connfd, sendBuff, strlen(sendBuff),0); 
        }
        
        while(isBusy[pthread_self()]){
                // std::cout << "isBusy " << isBusy[pthread_self()] << std::endl;
                memset(recvBuff, '0', sizeof(recvBuff));
                memset(sendBuff, '0', sizeof(sendBuff)); 
                char buffer[1];
                ssize_t bytesRead = recv(connfd, buffer, sizeof(buffer), MSG_PEEK);
                if (bytesRead == 0) {
                        // Connection has been closed by the remote host
                        //return 1;
                        pthread_mutex_lock(&sockets_mutex);
                        isBusy[pthread_self()] = false;
                        pthread_mutex_unlock(&sockets_mutex);
                        
                        
                        // printf("admin2 TID: %ld\n", pthread_self());
                        if(adminConnected && ADMIN == pthread_self()){
                                pthread_mutex_lock(&admin_mutex);
                                ADMIN = 0;
                                adminConnected = 0;
                                pthread_mutex_unlock(&admin_mutex);
                        }
                        break;
                } else if (bytesRead == -1) {
                        // Error occurred during recv
                        perror("recv");
                        //return -1;
                        pthread_mutex_lock(&sockets_mutex);
                        isBusy[pthread_self()] = false;
                        pthread_mutex_unlock(&sockets_mutex);
                        
                        
                        // printf("admin3 TID: %ld\n", pthread_self());
                        if(adminConnected && ADMIN == pthread_self()){
                                pthread_mutex_lock(&admin_mutex);
                                ADMIN = 0;
                                adminConnected = 0;
                                pthread_mutex_unlock(&admin_mutex);
                        }
                        break;
                }
                /*receive data from the client*/
                numbytes = recv(connfd,recvBuff, sizeof(recvBuff),0);
                if (numbytes == -1){
                        perror("recv");
                        //exit(1);
                        return;
                }
                
                recvBuff[numbytes] = '\0';
                printf("%d - %s\n",numbytes, recvBuff);
                //char* outfilename = "rec.txt";
                //char* infilename = "test.txt";
                if(strcmp(recvBuff, "request") == 0 && isBusy[pthread_self()]){
                        receiveFile(connfd, requestFilePath.c_str());
                        //get client confirmation
                        numbytes = recv(connfd,recvBuff,sizeof(recvBuff)-1,0);
                        if (numbytes == -1){
                        perror("recv");
                        //exit(1);
                        return;
                        }
                        recvBuff[numbytes] = '\0';
                        printf("recv: %s\n", recvBuff);
                        //pthread_t thread_id = pthread_self();
                        std::ifstream requestFile(requestFilePath);
                        std::ofstream responseFile(responseFilePath, std::ios::trunc);
                        json request;
                        requestFile >> request;
                        requestFile.close();
                        std::cout << "json: " << request.dump(4) << std::endl;
                        //printf("%s\n", request.dump(4));
                        std::string call = request["call"]; 
                        std::cout << call << std::endl;
                        if(strcmp(call.c_str(), "postCustomer") == 0){
                                Customer customer(request);
                                customer.insertIntoDatabase(db); 
                                sprintf(sendBuff,"postCustomer done");
                                printf("%s\n", sendBuff);
                                send(connfd, sendBuff, strlen(sendBuff),0); 
                        }else if(strcmp(call.c_str(), "postAccount") == 0){
                                Account account(request);
                                bool ok = account.insertIntoDatabase(db);
                                bool isConnection = request["connection"];
                                if (isConnection){
                                        Transaction transaction(request["account_id"], -1, request["customer_id"], 0.0, 0, request["date"]);
                                        bool rc = transaction.insertIntoDatabase(db);  
                                        if(!rc){
                                                ok = rc;
                                        }
                                }
                                if (ok){
                                        sprintf(sendBuff,"postAccount done");
                                }else{
                                        sprintf(sendBuff,"postAccount failed");
                                }
                                printf("%s\n", sendBuff); 
                                send(connfd, sendBuff, strlen(sendBuff),0);  
                        }else if(strcmp(call.c_str(), "postTransaction") == 0){
                                Transaction transaction(request); 
                                bool ok = transaction.insertIntoDatabase(db);
                                if (!ok){
                                        std::cout << "update src transaction failed!" << std::endl; 
                                }
                                int type = request["transaction_type"];
                                if (type % 2 == 0 && type != 0){
                                        double sum = request["sum"];
                                        bool rc = Account::updateAccount(db, request["account_id"], -sum);
                                        if (!rc){
                                                std::cout << "update src account failed!" << std::endl;
                                                ok = rc;
                                        }
                                        
                                } else if (type > 1) {
                                        bool rc = Account::updateAccount(db, request["account_id"], request["sum"]);
                                        if (!rc){
                                                std::cout << "update close account failed!" << std::endl;
                                                ok = rc;
                                        }

                                }
                                if (type == 4){
                                        Transaction destTransaction(request["dest_account_id"],request["account_id"], request["customer_id"], request["dest_sum"], 5, request["date"]);
                                        bool rc = destTransaction.insertIntoDatabase(db);
                                        if (!rc){
                                                std::cout << "update dest transaction failed!" << std::endl;
                                                ok = rc; 
                                        }
                                        rc = Account::updateAccount(db, request["dest_account_id"], request["dest_sum"]);
                                        if (!rc){
                                                std::cout << "update dest account failed!" << std::endl;
                                                ok = rc;
                                        }
                                }
                                if (ok){
                                        sprintf(sendBuff,"postTransaction done");
                                }else{
                                        sprintf(sendBuff,"postTransaction failed");
                                }
                                printf("%s\n", sendBuff);
                                send(connfd, sendBuff, strlen(sendBuff),0);  
                        }else if(strcmp(call.c_str(), "getCustomers") == 0){
                                json customers = Customer::getAllCustomer(db);
                                responseFile << customers;
                                responseFile.close();
                                std::cout << "json: " << request.dump(4) << std::endl;
                                sendFile(connfd, responseFilePath.c_str());
                        }else if(strcmp(call.c_str(), "getCustomerAccounts") == 0){
                                json accounts = Account::getAllAccountsByCustomerID(db, request["customer_id"]);
                                responseFile << accounts;
                                responseFile.close();
                                std::cout << "json: " << request.dump(4) << std::endl;
                                sendFile(connfd, responseFilePath.c_str());
                        }else if(strcmp(call.c_str(), "getAccountByID") == 0){
                                json accounts = Account::getAccountByID(db, request["account_id"]);
                                responseFile << accounts;
                                responseFile.close();
                                std::cout << "json: " << request.dump(4) << std::endl;
                                sendFile(connfd, responseFilePath.c_str());
                        }else if(strcmp(call.c_str(), "getAccounts") == 0){
                                json accounts = Account::getAllAccounts(db);
                                responseFile << accounts;
                                responseFile.close();
                                std::cout << "json: " << request.dump(4) << std::endl;
                                sendFile(connfd, responseFilePath.c_str());
                        }else if(strcmp(call.c_str(), "getAccountID") == 0){
                                json accountID = Account::getLastAccountID(db);
                                responseFile << accountID;
                                responseFile.close();
                                std::cout << "json: " << request.dump(4) << std::endl;
                                sendFile(connfd, responseFilePath.c_str());
                        }else if(strcmp(call.c_str(), "getCustomerTransaction") == 0){
                                json transactions = Transaction::getAllTransactionByCustomerID(db, request["customer_id"]);
                                responseFile << transactions;
                                responseFile.close();
                                std::cout << "json: " << request.dump(4) << std::endl;
                                sendFile(connfd, responseFilePath.c_str());  
                        }else if(strcmp(call.c_str(), "getAccountTransactions") == 0){
                                json transactions = Transaction::getAllTransactionByAccountID(db, request["account_id"]);
                                responseFile << transactions;
                                responseFile.flush();
                                responseFile.close();
                                
                                std::cout << "file: " << responseFilePath << std::endl;
                                std::cout << "json: " << request.dump(4) << std::endl;
                                sendFile(connfd, responseFilePath.c_str());
                        }else if(strcmp(call.c_str(), "loginCustomer") == 0){
                                json response = Customer::login(db, request["customer_name"], request["customer_password"]);
                                responseFile << response;
                                responseFile.close();
                                std::cout << "json: " << request.dump(4) << std::endl;
                                sendFile(connfd, responseFilePath.c_str());  
                                //???
                        // }else if(strcmp(call.c_str(), "deleteCustomerById") == 0){
                        //         bool response = Customer::deleteCustomerById(db, request["customer_id"]);
                        //         if(response){
                        //         sprintf(sendBuff,"deleteCustomerById done");
                        //         printf("%s\n", sendBuff);
                        //         send(connfd, sendBuff, strlen(sendBuff),0);

                        //         }else{
                        //         sprintf(sendBuff,"deleteCustomerById fail");
                        //         printf("%s\n", sendBuff);
                        //         send(connfd, sendBuff, strlen(sendBuff),0); 

                        //         } 

                        }else if(strcmp(call.c_str(), "kill") == 0){
                        
                        
                        // printf("admin4 TID: %ld\n", pthread_self());
                                if(adminConnected && ADMIN == pthread_self()){
                                        pthread_mutex_lock(&sockets_mutex);
                                        pthread_t thread_id = request["thread_id"];
                                        isBusy[thread_id] = false;
                                        // std::cout << "kill isBusy " << isBusy[thread_id] << std::endl;
                                        pthread_mutex_unlock(&sockets_mutex);
                                        sprintf(sendBuff,"kill done");
                                        printf("%s\n", sendBuff);
                                        send(connfd, sendBuff, strlen(sendBuff),0); 
                                }
                        }else if(strcmp(call.c_str(), "listsockets") == 0){
                        
                        
                        // printf("admin5 TID: %ld\n", pthread_self());
                                if(adminConnected && ADMIN == pthread_self()){
                                        json socketList;
                                        for(auto& pair: isBusy){
                                                if(pair.second && (pair.first != pthread_self())){
                                                        std::ifstream testRequestFile(std::string("requests/request" + std::to_string(pair.first) + ".json"));
                                                        std::ifstream requestClientFile(std::string("requests/request" + std::to_string(pair.first) + ".json"));
                                                        bool acceptable =  nlohmann::json::accept(testRequestFile);
                                                        // std::cout << "request file ok? " << acceptable << std::endl;
                                                        if (acceptable){
                                                                json element;
                                                                element["thread_id"] = pair.first;
                                                                std::cout << element.dump() << std::endl;
                                                                json requestClient;
                                                                requestClientFile >> requestClient;
                                                                std::cout << "json Client: " << requestClient.dump(4) << std::endl;
                                                                element["ID"] = requestClient["customer_id"];
                                                                std::cout << element.dump() << std::endl;
                                                                socketList.push_back(element);
                                                        }
                                                        testRequestFile.close(); 
                                                        requestClientFile.close(); 
                                                }
                                        }
                                        // std::ofstream adminResponseFile("adminResponse.json", std::ios::trunc);
                                        json result;
                                        result["response"] = socketList;
                                        responseFile << result;
                                        responseFile.close();
                                        std::cout << "json admin response: " << result.dump(4) << std::endl;
                                        sendFile(connfd, responseFilePath.c_str());  
                                        // std::string dump = socketList.dump();
                                        // sprintf(sendBuff,"%s", dump.c_str());
                                        // printf("%s\n", sendBuff);
                                        // send(connfd, sendBuff, strlen(sendBuff),0); 
 
                                }else{
                                        sprintf(sendBuff,"listsockets illegal request");
                                        printf("%s\n", sendBuff);
                                        send(connfd, sendBuff, strlen(sendBuff),0); 
                                }

                        }
                // }else if(strcmp(recvBuff, "file-send") == 0){   
                //         sendFile(connfd, responseFilePath.c_str());  
                
                //}else if(strcmp(recvBuff, "kill "))

                }else if(strcmp(recvBuff, "exit") == 0){
                        sprintf(sendBuff,"shutdown");
                        printf("%s\n", sendBuff);
                        send(connfd, sendBuff, strlen(sendBuff),0); 
                        printf("Socket: %s\n", sendBuff);
                        shutdown(connfd, SHUT_RDWR); 
                        close(connfd);
                        
                        
                        // printf("admin TID: %ld\n", pthread_self());
                        if(adminConnected && ADMIN == pthread_self()){
                                pthread_mutex_lock(&admin_mutex);
                                ADMIN = 0;
                                pthread_mutex_unlock(&admin_mutex);
                        }

                        pthread_mutex_lock(&sockets_mutex);
                        isBusy[pthread_self()] = false;
                        pthread_mutex_unlock(&sockets_mutex);
                        //break;
                // }else /*if(strcmp(recvBuff,"test") == 0)*/{
                        /*Extract the number*/
                        //sscanf(recvBuff, "%d", &a); 
                        // a = atoi(recvBuff);
                        /*Multiply the answer by 10*/  
                        // result = a * 10; 
                        
        //                 snprintf(sendBuff, sizeof(sendBuff), "%d", result);shutdown(connfd, SHUT_RDWR);
        // close(connfd);

                        // send(connfd, sendBuff, strlen(sendBuff),0); 

                }
                
        }
        //in case socket got closed by admin, close the socket for client
        remove(requestFilePath.c_str());
        remove(responseFilePath.c_str());
        shutdown(connfd, SHUT_RDWR);
        close(connfd);

}
 
/*
 * This method locks down the connection queue then utilizes the queue.h push function
 * to add a connection to the queue. Then the mutex is unlocked and cond_signal is set
 * to alarm threads in cond_wait that a connection as arrived for reading
 */
void queueAdd(int value)
{
        /*Locks the mutex*/
        pthread_mutex_lock(&mutex);

        push(q, value);

        /*Unlocks the mutex*/
        pthread_mutex_unlock(&mutex);

        /* Signal waiting threads */
        pthread_cond_signal(&cond);
}

/*
 * This method locks down the connection queue then utilizes pthread_cond_wait() and waits
 * for a signal to indicate that there is an element in the queue. Then it proceeds to pop the 
 * connection off the queue and return it
 */
int queueGet()
{
       /*Locks the mutex*/
        pthread_mutex_lock(&mutex);

        /*Wait for element to become available*/
        while(empty(q) == 1)
        {
                printf("Thread %lu: \tWaiting for Connection\n", pthread_self());
                if(pthread_cond_wait(&cond, &mutex) != 0)
                {
                    perror("Cond Wait Error");
                }
        }

        /*We got an element, pass it back and unblock*/
        int val = peek(q);
        pop(q);

        /*Unlocks the mutex*/
        pthread_mutex_unlock(&mutex);

        return val;
}

static void* connectionHandler(void*)
{
        int connfd = 0;
        pthread_t thread_id = pthread_self();
        pthread_mutex_lock(&sockets_mutex);
        isBusy[thread_id] = false;
        pthread_mutex_unlock(&sockets_mutex);

        /*Wait until tasks is available*/
        while(1)
        {
                connfd = queueGet();
                printf("Handler %lu: \tProcessing\n", pthread_self());
                /*Execute*/
                doProcessing(connfd);
        }
}

void* unixMain(void*){
        int unix_listenfd = 0, unix_connfd = 0;
        struct sockaddr_un unix_serv_addr; 
        int unix_rv;

        /*Socket creation and binding*/
        unix_listenfd = socket(AF_UNIX, SOCK_STREAM, 0);

        if (unix_listenfd <  0) 
        {
                perror("Error in socket creation");
                exit(1);
        }
        // Make sure the address we're planning to use isn't too long.
        if (strlen(UNIX_PATH) > sizeof(unix_serv_addr.sun_path) - 1) {
                printf("Server socket path too long: %s", UNIX_PATH);
                exit(0);
        }
        // Delete any file that already exists at the address. Make sure the deletion
        // succeeds. If the error is just that the file/directory doesn't exist, it's fine.
        if (remove(UNIX_PATH) == -1 && errno != ENOENT) {
                printf("remove-%s", UNIX_PATH);
                exit(0);
        }

        /* Zeroing server_addr struct */
        memset(&unix_serv_addr, 0, sizeof(unix_serv_addr));
        /* Construct server_addr struct */
        unix_serv_addr.sun_family = AF_UNIX;
        strncpy(unix_serv_addr.sun_path, UNIX_PATH, sizeof(unix_serv_addr.sun_path) - 1);
        int temp = 1;
        if (setsockopt(unix_listenfd, SOL_SOCKET, SO_REUSEADDR,  &temp, sizeof(int)) < 0)
                perror("setsockopt(SO_REUSEADDR) failed");


        unix_rv = bind(unix_listenfd, (struct sockaddr*)&unix_serv_addr, sizeof(unix_serv_addr)); 

        if (unix_rv <  0) 
        {
                perror("Error in binding");
                exit(1);
        }

        listen(unix_listenfd, 10); 

        /*Accept connection and push them onto the stack*/
        while(1)
        {
                printf("\nUnix Main: \t\t\tAccepting Connections\n");

                /*The accept call blocks until a connection is found
                 * then the connection is pushed onto the queue by queue_add*/
                queueAdd(accept(unix_listenfd, (struct sockaddr*)NULL, NULL)); 

                printf("Unix Main: \t\t\tConnection Completed\n\n");
        }

}

void* inetMain(void*){

        int listenfd = 0, connfd = 0;
        struct sockaddr_in serv_addr; 
        int inet_rv;


        /*Socket creation and binding*/
        listenfd = socket(AF_INET, SOCK_STREAM, 0);

        if (listenfd <  0) 
        {
                perror("Error in socket creation");
                exit(1);
        }

        /* Zeroing server_addr struct */
        memset(&serv_addr, 0, sizeof(serv_addr));
        /* Construct server_addr struct */
        serv_addr.sin_family = AF_INET;
        inet_pton(AF_INET, SERVER_ADDRESS, &(serv_addr.sin_addr));
        serv_addr.sin_port = htons(PORT_NUMBER); 
        int temp = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) < 0)
                perror("setsockopt(SO_REUSEADDR) failed");


        inet_rv = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

        if (inet_rv <  0) 
        {
                perror("Error in binding");
                exit(1);
        }

        listen(listenfd, 10); 

        /*Accept connection and push them onto the stack*/
        while(1)
        {
                printf("\nInet Main: \t\t\tAccepting Connections\n");

                /*The accept call blocks until a connection is found
                 * then the connection is pushed onto the queue by queue_add*/
                queueAdd(accept(listenfd, (struct sockaddr*)NULL, NULL)); 

                printf("Inet Main: \t\t\tConnection Completed\n\n");
        }
}



int main(int argc, char *argv[])
{
        q = createQueue(THREADPOOL_SIZE + 10);
        /*Initialize the mutex global variable*/
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_init(&admin_mutex, NULL);
        ADMIN = 0;
        int result = sqlite3_open("db/database.db", &db);
        if (result != SQLITE_OK) {
                printf("Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(db));
                return 1;
        }
        /*Declare the thread pool array*/
        pthread_t threadPool[THREADPOOL_SIZE], unix_thread, inet_thread;

        /*Make Thread Pool*/
        for(int i = 0; i < THREADPOOL_SIZE; i++) 
        {
                pthread_create(&threadPool[i], NULL, connectionHandler, (void *) NULL);
        }

        unlink (UNIX_PATH) ;

        pthread_create(&unix_thread, NULL, unixMain, NULL);
        pthread_create(&inet_thread, NULL, inetMain, NULL);

        for(int i = 0; i < THREADPOOL_SIZE; i++)
        {
                pthread_join(threadPool[i], NULL);
        }

        pthread_join(unix_thread, NULL);
        pthread_join(inet_thread, NULL);
        sqlite3_close(db);
        unlink (UNIX_PATH) ;
}
