#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096

int server(const char *portnum)
{
    struct sockaddr_in     serv_addr;
    struct sockaddr_in     clie_addr;
    char                   buf[MAXLINE];
    int                    sock_id;
    int                    link_id;
    int                    recv_len;
    int                    write_leng;
    int                    clie_addr_len;
    FILE                   *fp;

    // if (argc != 3) 
    // {  
    //     printf("usage :%s portnum filename\n", argv[0]);  
    //     exit(0);
    // }  

    int i = 0;
/*<-----------------------------------------socket---------------------------------------------->*/
    if ((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Create socket failed\n");
        exit(0);
    }
/*<-----------------------------------------bind------------------------------------------------->*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(portnum));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 )
    {
        perror("Bind socket failed\n");
        exit(0);
    }
/*<-----------------------------------------listen----------------------------------------------->*/
    if (-1 == listen(sock_id, 10))
    {
        perror("Listen socket failed\n");
        exit(0);
    }
/*<-------------------------------------server receive part---------------------------------->*/
    // while (1) {  
    //”a+“ 以附加方式打开可读写的文件。若文件不存在，则会建立该文件，如果文件存在，写入的数据会被加到文件尾后，即文件原先的内容会被保留。 （原来的EOF符不保留）

    printf("\nWaiting for data...\n");
    while (1)
    {
        char *filepath[100];
        sprintf(filepath, "/home/openailab/hiiragiiori/oneface_rtsp_ptz_tcp/images/names.txt", i);
        if ((fp = fopen((const char *)filepath, "a+")) == NULL)
        {
            perror("Open file failed\n");
            exit(0);
        }
    
        clie_addr_len = sizeof(clie_addr);
    /*<-----------------------------------------accept----------------------------------------------->*/        
        link_id = accept(sock_id, (struct sockaddr *)&clie_addr, &clie_addr_len);
        if (-1 == link_id) {
            perror("Accept socket failed\n");
            exit(0);
        }
        bzero(buf, MAXLINE);
        while (recv_len = recv(link_id, buf, MAXLINE, 0))
        {
            if(recv_len < 0)
            {
                printf("Recieve Data From Server Failed!\n");
                break;
            }  
            //printf("#");
            write_leng = fwrite(buf, sizeof(char), recv_len, fp);
            if (write_leng < recv_len)
            {
                printf("Write file failed\n");
                break;
            }
            bzero(buf,MAXLINE);
        }
    
        //printf("Finish Recieve\n");
        printf("File: ");
        printf(filepath);
        printf(" RECEIVED\n");
        fclose(fp);
        close(link_id);
        i ++;

        if (i > 12)
            break;
    }

    close(sock_id);
    return 0;
}

int main()
{
    const char *portnum = "233";
    char *filepath[100];
    //int i = 0;
    server(portnum);
    //}
}
