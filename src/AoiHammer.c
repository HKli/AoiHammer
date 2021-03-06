#include "AoiHammer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <string.h>
#include <time.h>
#include "StorageStruct.h"

int main(int argc, char **argv)
{
    srand(time(NULL));
    for (uint64_t i = 0; i < 50000000; i++)
    {
        if (i % 1000000 == 0)
        {
            printf("Current: %ld\n", i);
        }
        uint64_t data = rand();
        insertData(i, data);
    }
    printf("Generate Done!\n");
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    fcntl(serverSocket, F_SETFL, SO_REUSEADDR | SO_REUSEPORT);
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8023);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Unable to bind port");
        exit(1);
    }
    if (listen(serverSocket, 20) == -1)
    {
        perror("Unable to listen");
        exit(1);
    }
    fd_set socketDescriptorSet;
    FD_ZERO(&socketDescriptorSet);
    FD_SET(serverSocket, &socketDescriptorSet);
    do
    {
        fd_set socketDescriptorSetCopy = socketDescriptorSet;
        int selectCount = select(FD_SETSIZE, &socketDescriptorSetCopy, NULL, NULL, NULL);
        if (selectCount == -1)
        {
            perror("Unable to select");
            exit(1);
        }
        for (int descriptor = 0; descriptor < FD_SETSIZE; descriptor++)
        {
            if (FD_ISSET(descriptor, &socketDescriptorSetCopy))
            {
                if (descriptor == serverSocket)
                {
                    struct sockaddr_in clientAddress;
                    socklen_t sizeofClientSocketAddr = sizeof(clientAddress);
                    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &sizeofClientSocketAddr);
                    FD_SET(clientSocket, &socketDescriptorSet);
                }
                else
                {
                    int readCount;
                    ioctl(descriptor, FIONREAD, &readCount);
                    if (readCount == 0)
                    {
                        close(descriptor);
                        FD_CLR(descriptor, &socketDescriptorSet);
                    }
                    else
                    {
                        char buffer[64];
                        memset(buffer, 0, sizeof(buffer));
                        read(descriptor, buffer, sizeof(buffer) - 1);
                        int64_t queryHash;
                        sscanf(buffer, "%ld", &queryHash);
                        pHashStore phash;
                        phash.hash = queryHash;
                        uint64_t resultId = startSearch(phash);
                        memset(buffer, 0, sizeof(buffer));
                        sprintf(buffer, "%lu\n", resultId);
                        write(descriptor, buffer, sizeof(buffer));
                        close(descriptor);
                        FD_CLR(descriptor, &socketDescriptorSet);
                    }
                }
            }
        }
    } while (1);
}
