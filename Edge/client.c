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
#include <dirent.h>

#define MAXLINE 4096

#define DIRNAME "/home/openailab/hiiragiiori/face_rtsp_ptz_tcp/images/"

int getImgNum() {
	int count = 0;
	int filesize = 0;
	DIR *dir = NULL;
	struct dirent *entry;
	if ((dir = opendir(DIRNAME)) == NULL) {
		printf("Open Dir Failed!");
		return -1;
	}
	else {
		while (entry = readdir(dir)) {
			count ++;
		}
		closedir(dir);
	}
	return count - 2;
}

int sendAttendancePic(const char *serveraddr, const char *portnum)
{
    struct sockaddr_in     serv_addr;
    char                   buf[MAXLINE];
    int                    sock_id;
    int                    read_len;
    int                    send_len;
    FILE                   *fp;
    int                    i_ret;
	int i = 0;

	while(1)
	{
		char filepath[100];
		sprintf(filepath, "images/img_%d.jpg", i);
		if ((fp = fopen((const char *)filepath,"r")) == NULL)
	    {
	        perror("Open file failed\n");
			printf("%s\n", filepath);
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
		serv_addr.sin_port = htons(atoi(portnum));
		inet_pton(AF_INET, serveraddr, &serv_addr.sin_addr);

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

		printf("%s", filepath);
    	printf(" SEND SUCCESSFUL\n");
		i ++;
		if (i >= 14)
			break;
	}
	fclose(fp);
    close(sock_id);
    return 0;
}

int sendHeadupPic(const char *serveraddr, const char *portnum)
{
    struct sockaddr_in     serv_addr;
    char                   buf[MAXLINE];
    int                    sock_id;
    int                    read_len;
    int                    send_len;
    FILE                   *fp;
    int                    i_ret;
	int i = 0;

	while(1)
	{
		char filepath[100];
		sprintf(filepath, "images/img_%d.jpg", i);
		if ((fp = fopen((const char *)filepath,"r")) == NULL)
	    {
	        perror("Open file failed\n");
			printf("%s\n", filepath);
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
		serv_addr.sin_port = htons(atoi(portnum));
		inet_pton(AF_INET, serveraddr, &serv_addr.sin_addr);

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

		printf("%s", filepath);
    	printf(" SEND SUCCESSFUL\n");
		i ++;
		if (i >= 14)
			break;
	}
	fclose(fp);
    close(sock_id);
    return 0;
}

int main()
{
	const char *serveraddr = "192.168.123.233";
	const char *portnum = "233";
	
	sendAttendancePic(serveraddr, portnum);
}
