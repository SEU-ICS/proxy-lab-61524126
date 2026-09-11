#include <stdio.h>
#include "csapp.h"
#include <string.h>
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
#define MAX_CHAR_SIZE 4096
#define MAX_LINE 4096
#define MAX_BUFF 4096
int main(int argc,char** argv)
{   int listenfd;
    struct sockaddr_storage clientaddr;
    socklen_t clientlen;
    char method[MAX_CHAR_SIZE];
    char uri[MAX_CHAR_SIZE];
    char version[MAX_CHAR_SIZE];
    rio_t rio;
    char buf[MAX_CHAR_SIZE];
    char host[MAX_LINE];
    char port[20]="80";
    char path[MAX_LINE]="/";
    char hostline[MAX_LINE];
    char other_request[MAX_LINE]="";
    char request[MAX_LINE]="";
    char connection[MAX_LINE]="Connection: close\r\n";
    char proxyconnection[MAX_LINE]="Proxy-Connection: close\r\n";
    char end[MAX_LINE]="\r\n";
    int n=0;
    listenfd=open_listenfd(argv[1]);
    while(1){
        socklen_t clinetlen=sizeof(clientaddr);
        int connfd=Accept(listenfd,(SA*)&clientaddr,&clinetlen);
        Rio_readinitb(&rio,connfd);
        Rio_readlineb(&rio,buf,MAX_LINE);
        sscanf(buf,"%s %s %s",method,uri,version);
       char*p=uri+7;
       char*q=strpbrk(p,"/?");
       if(q!=NULL){
       char*saved=q;
       *q='\0';
       char * col=strchr(p,':');
       if(col!=NULL){
        size_t len1=col-p;
        size_t len2=q-(col+1);
        memcpy(host,p,len1);
        memcpy(port,col+1,len2);
        *q=saved;
        strcpy(path,q);
       }else{
        size_t len=q-p;
        memcpy(host,p,len);
        q=saved;
        strcpy(path,q);
       }
    }
       else{
        char*col=strchr(p,':');
        if(col!=NULL){
            size_t len=col-p;
            memcpy(host,p,len);
            strcpy(port,col+1);
        }else{
            strcpy(host,p);
        }
       }
       
    while(Rio_readlineb(&rio,buf,MAX_LINE)>0){
        if(!strcmp(buf,"\r\n")){
            break;
        }
        if(strncasecmp(buf,"Host:",5)==0){
            strcpy(hostline,buf);
        }
        if(strncasecmp(buf,"Connection:",11)){
            continue;
        }
        if(strncasecmp(buf,"User-Agent:",11)){
            continue;
        }
        if(strncasecmp(buf,"Proxy-Connection:",17)){
            continue;
        }
        else{
            strcat(other_request,buf);
        }
    }

     snprintf(request,sizeof(request),"%s %s HTTP/1.0\r\n",method,path);
    strcat(request,hostline);
    strcat(request,connection);
    strcat(request,proxyconnection);
    strcat(request,user_agent_hdr);
    strcat(request,other_request);
    strcat(request,end);
    int served=open_clientfd(host,port);
    Rio_writen(served,request,strlen(request));
    rio_t server_rio;
    Rio_readinitb(&server_rio,served);
    while((n=Rio_readnb(&server_rio,buf,MAX_BUFF))>0){
        Rio_writen(connfd,buf,n);
    }
}
return 0;
}
