#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096

int main(int argc,char **argv)
{
    struct sockaddr_in     serv_addr;
    char                   buf[MAXLINE];
    int                    sock_id;
    int                    read_len;
    int                    send_len;
    FILE                   *fp;
    int                    i_ret;

    if (argc != 4)
    {
        printf("usage :%s ipaddr portnum filename\n", argv[0]);
        exit(0);
    }  

    if ((fp = fopen(argv[3],"r")) == NULL)
    {
        perror("Open file failed\n");
        exit(0);
    }

/*<-----------------------------------------socket---------------------------------------------->*/
    if ((sock_id = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Create socket failed\n");
        exit(0);
    }
/*<-----------------------------------------connect---------------------------------------------->*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    i_ret = connect(sock_id, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    if (-1 == i_ret)
    {
        printf("Connect socket failed\n");
        return -1;
    }
/*<-------------------------------------client send part---------------------------------->*/ 
    bzero(buf, MAXLINE);
    while ((read_len = fread(buf, sizeof(char), MAXLINE, fp)) >0 )
    {
        send_len = send(sock_id, buf, read_len, 0);
        if ( send_len < 0 )
        {
            perror("Send file failed\n");
            exit(0);
        }
        bzero(buf, MAXLINE);
    }  

    fclose(fp);
    close(sock_id);
    printf("Send Finish\n");
    return 0;
}
