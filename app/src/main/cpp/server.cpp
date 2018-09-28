#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>

#include<netdb.h>
#include<ifaddrs.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#define LOG_TAG "SERVER"
#define ANDROID_NATIVE
#ifdef ANDROID_NATIVE
#include <android/log.h>
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#else
#define LOGI(...) {fprintf(stdout, __VA_ARGS__);fputs("\n", stdout);}
#define LOGE(...) {fprintf(stderr, __VA_ARGS__);fputs("\n", stderr);}

#endif


/* command */
#define PUT_TEXT   "TEXT"       //t
#define SET_DIR "SDIR"          //d
#define GET_DIR "GDIR"          //f
#define PUT_FILE "WRIT"         //w
#define GET_FILE "READ"         //r
#define LIST_DIR "LIST"         //l
#define GET_CHANNEL "GETC"      //g
#define RM_FILE     "RMFI"      //m

#define min(a,b)	((a<b)?a:b)
static char gCurrentFolder[PATH_MAX];
static int gServerPort = 0;
int ResponseOK(int fd, const char* comment)
{
    const char text[]= "OK\n";
    int n = write(fd,text, strlen(text));
    if (n < 0) {
        LOGE("ERROR writing to socket");
        return n;
    }
    if(comment){
        n = write(fd,comment, strlen(comment));
    }
    return n;
}
int ResponseFailed(int fd, const char* reason)
{
    char buffer[256];
    sprintf(buffer, "FAILED\n%s\n", reason);
    int n = write(fd,buffer, strlen(buffer));
    if (n < 0) {
        LOGE("ERROR writing to socket");
        return n;
    }
    return 0;
}

int HandleCommandList(int fd)
{
    char text[256];
    char buffer[1024];
    ResponseOK(fd, NULL);
    buffer[0] = 0;

    DIR *d;
    struct dirent *dir;

    d = opendir(gCurrentFolder);
    int remainsize = sizeof(buffer);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            remainsize -= strlen(dir->d_name)+1;
            if (remainsize < 0)
                break;
            strcat(buffer, dir->d_name);
            strcat(buffer, "\n");
        }
        closedir(d);
    }
    int len = strlen(buffer)+1;
    sprintf(text, "%d\n", len);
    int n = write(fd,text, strlen(text));
    if (n < 0) {
        LOGE("ERROR writing to socket 1");
        return n;
    }
    n = write(fd,buffer, strlen(buffer)+1);
    if (n < 0) {
        LOGE("ERROR writing to socket 2");
        return n;
    }
    return 0;
}

int HandleCommandText(int fd, char* arg)
{
    char buffer[256];
    int nLen = atoi(arg);
    if (nLen < 256)
        ResponseOK(fd, NULL);
    else {
        sprintf(buffer, "FAILED. Message too long, the maximum is %d characters.\n", (int) sizeof(buffer)-1);
        return ResponseFailed(fd, buffer);
    }

    //prepare to read message
    memset(buffer, 0, 256);
    int n = read(fd,buffer,255);
    if (n < 0) {
        LOGE("ERROR reading from socket");
    }
    buffer[n] = 0;
    LOGI("Client says: %s", buffer);
    ResponseOK(fd, NULL);
    return 0;
}
int HandleCommandSetDir(int fd, char* folder)
{
    char buffer[512];
    int res = chdir(folder);
    if (res ==0) {
        getcwd(gCurrentFolder, sizeof(gCurrentFolder));
        sprintf(buffer, "Now current folder is %s\n", gCurrentFolder);
        LOGI("%s", buffer);
        return ResponseOK(fd, buffer);
    }

    sprintf(buffer, "FAILED: error code =%d\n", res);
    return ResponseFailed(fd, buffer);

}
int HandleCommandRemoveFile(int fd, char* file)
{
    char buffer[512];

    int res = remove(file);
    if (res ==0) {
        sprintf(buffer, "File %s s removed.\n", file);
        return ResponseOK(fd, buffer);
    }

    sprintf(buffer, "FAILED: error code =%d\n", res);
    return ResponseFailed(fd, buffer);

}

int HandleCommandGetDir(int fd)
{
    return ResponseOK(fd, gCurrentFolder);
}
int HandleCommandPutFile(int fd, char* name, int len)
{
    char buffer[512];
    FILE* fp;
    fp = fopen(name, "wb");
    LOGI("open %s to save.", name);

    if(!fp){
        return ResponseFailed(fd, "Open file failed.");
    }
    ResponseOK(fd, NULL);

    memset(buffer, 0, sizeof(buffer));
    int remain = len;
    while(remain >0){

        int n = read(fd, buffer, min(remain, 512));
        LOGI("read %d bytes", n);
        if (n<0)
        {
            LOGE("ERROR reading socket!");
            fclose(fp);
            return ResponseFailed(fd, "Socket read error.");
        }
        int m = fwrite(buffer, 1, n, fp);
        if (m != n) {
            LOGE("ERROR writing file!");
            fclose(fp);
            return ResponseFailed(fd, "File I/O error.");
        }
        remain -= n;
    }
    fclose(fp);
    sprintf(buffer, "Received %d bytes\n", len);
    LOGI("%s", buffer);
    return ResponseOK(fd, buffer);
}
int HandleCommandGetFile(int fd, char* name)
{
    char buffer[512];
    FILE* fp;
    long len = 0;
    fp = fopen(name, "rb");
    if(!fp) {
        return ResponseFailed(fd, "Open file failed\n");
    }


    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);


    if (len == 0){
        fclose(fp);
        return ResponseFailed(fd, "File length is zero.n");
    }
    sprintf(buffer, "%ld\n", len);
    ResponseOK(fd, buffer);

    int remain = len;
    while (remain > 0){
        int n = fread(buffer, 1, min(remain, 512), fp);
        if (n < 0){
            break;
        }
        int m = write(fd, buffer, n);
        if (m!= n){
            break;
        }
        remain -= n;
    }
    fclose(fp);
    if (remain == 0) {
        LOGI("File %s transmitted OK.", name);
    } else {
        LOGE("%d bytes not transmitted!!", remain);
    }
    return 0;
}
int HandleCommandGetImage(int fd)
{
    char buffer[512];
    int len = 512*10000;
    int n = 0;
    sprintf(buffer, "%d\n", len);
    ResponseOK(fd, buffer);

    for (int i=0;i<10000; i++ ){
        sprintf(buffer, "\n%d- This is camera frame data\n", i+1);
        int m = write(fd, buffer, 512);

        if (m < 0)
        {
            LOGE("Write socket error!");
            break;
        }
        n += m;
    }
    LOGI("GetChannel transmited %d bytes.", n);
    return 0;
}

void* ProcessClientCommand(void* data)
{
    int fd = *(int*) data;
    free(data);
    int n;
    char buffer[256];
    memset(buffer, 0, 256);
    n = read(fd,buffer,255);

    if (n<0) {
        LOGE("Read client message error!");
        return (void*) -1;
    }
    buffer[n] = 0;

    char* pCmd;
    char* pArg = NULL;
    char* pArg2 = NULL;

    pCmd = strtok(buffer, "\n");
    if (!pCmd){
        LOGE("Unrecognized client message %s.", buffer);
        return (void*) -2;
    }
    pArg = strtok(NULL, "\n");
    if (pArg) {
        pArg2 = pArg + strlen(pArg) + 1;
    }
    LOGI("command: %s", pCmd);

    if (strcmp(pCmd, PUT_TEXT) ==0 ){
        HandleCommandText(fd, pArg);
    } else if(strcmp(pCmd, SET_DIR) ==0 ) {
        HandleCommandSetDir(fd, pArg);
    } else if(strcmp(pCmd, GET_DIR) ==0 ) {
        HandleCommandGetDir(fd);
    } else if(strcmp(pCmd, PUT_FILE) ==0 ) {
        HandleCommandPutFile(fd, pArg, atoi(pArg2));
    } else if(strcmp(pCmd, GET_FILE) ==0 ) {
        HandleCommandGetFile(fd, pArg);
    } else if(strcmp(pCmd, GET_CHANNEL) ==0 ) {
        HandleCommandGetImage(fd);
    } else if(strcmp(pCmd, RM_FILE) ==0 ) {
        HandleCommandRemoveFile(fd, pArg);
    } else if(strcmp(pCmd, LIST_DIR) ==0 ) {
        HandleCommandList(fd);
    }
    return (void*) 0;
}

int CreateServer(int port)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }
    /* Initialize socket structure */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    LOGI("Bind to %d.", port);
    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        LOGE("ERROR on binding.");
        return -1;
    }
    listen(sockfd,5);
    return sockfd;
}
int OnAccept(int fd)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen= sizeof(cli_addr);
    int newsockfd = accept(fd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        LOGE("ERROR on accept!");
    }
    return newsockfd;
}
static int gServeFd = -1;
static pthread_t gServerThread;

void* RunServerThread(void* pdata)
{
    (void) pdata;

    gServeFd = CreateServer(gServerPort);
    while (gServeFd > 0) {
        int fdClient = OnAccept(gServeFd);
        if (fdClient < 0){
            close(gServeFd);
            gServeFd = -1;
            break;
        }
        pthread_t thread;
        int* data = (int*) malloc(sizeof(int));
        *data = fdClient;
        pthread_create(&thread, NULL, ProcessClientCommand, (void*)data);

    }
    LOGI("Server terminated.");

    return NULL;
}
void ShowIp()
{
    FILE *f;
    char line[100] , *p , *c, *d;

    f = fopen("/proc/net/route" , "r");
    p = NULL;
    while(fgets(line , 100 , f))
    {
        p = strtok(line , " \t");   //Iface
        c = strtok(NULL , " \t");   //Destination
        d = strtok(NULL , " \t");   //Gateway

        if(p!=NULL && c!=NULL)
        {
            //if(strcmp(c , "00000000") == 0) //in Wuawei, used interface is Gateway==0;
            if(strcmp(d , "00000000") == 0)
            {
                LOGI("Default interface is : %s \n" , p);
                break;
            }
        }
    }
    if(p == NULL)
        return ;
    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;
    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, p, IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);

    /* display result */
    LOGI("IP: %s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

}
void RunServer(int serverPort, const char* workFolder)
{
    ShowIp();
    gServerPort = serverPort;
    if (workFolder)
        strncpy(gCurrentFolder, workFolder, sizeof(gCurrentFolder));
    else
        //set current folder as global, since cwd works for child thread only
        getcwd(gCurrentFolder, sizeof(gCurrentFolder));

    pthread_create(&gServerThread, NULL, RunServerThread, (void*) NULL);

}
void TermServer()
{

    if (gServeFd >=0){
        shutdown(gServeFd, SHUT_RDWR);
        close(gServeFd);
    }else
        return;
    gServeFd = -1;
    pthread_join(gServerThread, NULL);
}