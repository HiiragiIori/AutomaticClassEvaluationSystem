#ifndef CLIENT_H
#define CLIENT_H

#ifdef __cplusplus
extern "C"{
#endif

int sendAttendancePic(const char *serveraddr, const char *portnum);
int sendHeadupPic(const char *serveraddr, const char *portnum);

#ifdef __cplusplus
}
#endif

#endif
