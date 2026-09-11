#include <stdio.h>
#include "csapp.h"
#include <string.h>
#include <pthread.h>
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
#define MAX_CHAR_SIZE 4096
#define MAX_LINE 4096
#define MAX_BUFF 4096
unsigned orderclock=0;
typedef struct{
    char uri[MAX_LINE];
    char data[MAX_OBJECT_SIZE];
    size_t size;
    int timestamp;
} cache_entry;
typedef struct{
    cache_entry entry[10];
    pthread_rwlock_t lock;
    size_t cache_size;
} cache;
cache webcache;
void LRU_replace(cache* webcache,char*data,char*uri,size_t size){
    int index=0;
    int flagstamp=(*webcache).entry[0].timestamp;
for(int i=1;i<10;i++){
if((*webcache).entry[i].timestamp<flagstamp){
    index=i;
    flagstamp=(*webcache).entry[i].timestamp;
}
}
    (*webcache).cache_size=(*webcache).cache_size+size-(*webcache).entry[index].size;
   strcpy((*webcache).entry[index].uri,uri);
    memcpy((*webcache).entry[index].data,data,size);
    (*webcache).entry[index].size=size;
    orderclock++;
    (*webcache).entry[index].timestamp=orderclock;
}
void writecache(cache*webcache,char*data,char*uri,size_t size){
int index=0;
while((*webcache).entry[index].size!=0){
    index++;
}
(*webcache).cache_size=(*webcache).cache_size+size;
strcpy((*webcache).entry[index].uri,uri);
 memcpy((*webcache).entry[index].data,data,size);
(*webcache).entry[index].size=size;
orderclock++;
(*webcache).entry[index].timestamp=orderclock;
}
int  readcache(char*uri){
    for(int i=0;i<10;i++){
     if(strcmp(uri,webcache.entry[i].uri)==0){
        return i;
    }
}
    return -1;
}
void* thread(void*adrconf){
    Pthread_detach(pthread_self());
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
    int connfd=*((int*)adrconf);
    Rio_readinitb(&rio,connfd);
        Rio_readlineb(&rio,buf,MAX_LINE);
        sscanf(buf,"%s %s %s",method,uri,version);
       char*p=uri+7;
       char*q=strpbrk(p,"/?");
       if(q!=NULL){
       char saved=*q;
       *q='\0';
       char * col=strchr(p,':');
       if(col!=NULL){
        size_t len1=col-p;
        size_t len2=q-(col+1);
        memcpy(host,p,len1);
        host[len1]='\0';
        memcpy(port,col+1,len2);
        port[len2]='\0';
        *q=saved;
        strcpy(path,q);
       }else{
        size_t len=q-p;
        memcpy(host,p,len);
        host[len]='\0';
        *q=saved;
        strcpy(path,q);
       }
    }
       else{
        char*col=strchr(p,':');
        if(col!=NULL){
            size_t len=col-p;
            memcpy(host,p,len);
            host[len]='\0';
            strcpy(port,col+1);
        }else{
            strcpy(host,p);
        }
       }
    pthread_rwlock_wrlock(&webcache.lock);
    char localdata[MAX_OBJECT_SIZE]="";
    int  localsize=0;

        if(readcache(uri)!=-1){
        int hitindex=readcache(uri);
         memcpy(localdata,webcache.entry[hitindex].data,webcache.entry[hitindex].size);
            localsize=webcache.entry[hitindex].size;
            orderclock++;
            webcache.entry[hitindex].timestamp=orderclock;
        }
 
    pthread_rwlock_unlock(&webcache.lock);
    if(localsize!=0){
        Rio_writen(connfd,localdata,localsize);
        Close(connfd);
        free(adrconf);
        return NULL;
    }
    while(Rio_readlineb(&rio,buf,MAX_LINE)>0){
        if(!strcmp(buf,"\r\n")){
            break;
        }
        if(strncasecmp(buf,"Host:",5)==0){
            strcpy(hostline,buf);
        }
        if(strncasecmp(buf,"Connection:",11)==0){
            continue;
        }
        if(strncasecmp(buf,"User-Agent:",11)==0){
            continue;
        }
        if(strncasecmp(buf,"Proxy-Connection:",17)==0){
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
    size_t object_size=0;
    char object_buf[MAX_OBJECT_SIZE];
    int cacheable=1;
    while((n=Rio_readnb(&server_rio,buf,MAX_BUFF))>0){
        Rio_writen(connfd,buf,n);
        if(object_size+n>MAX_OBJECT_SIZE){
            cacheable=0;
        }else{
            memcpy(object_buf+object_size,buf,n);
            object_size+=n;
        }
    }
    Close(served);
    Close(connfd);
    if(cacheable){
            pthread_rwlock_wrlock(&webcache.lock);
            int index=0;
            for(int i=0;i<10;i++){
                if(webcache.entry[i].size!=0){
                    index++;
                }
            }
            if(index<10){
                writecache(&webcache,object_buf,uri,object_size);
            }else{
                LRU_replace(&webcache,object_buf,uri,object_size);
            }
            pthread_rwlock_unlock(&webcache.lock);
        }
    
    free(adrconf);
    return NULL;
}
int main(int argc,char** argv)
{   int listenfd;
    struct sockaddr_storage clientaddr;
    pthread_rwlock_init(&webcache.lock,NULL);
    socklen_t clientlen;
    listenfd=open_listenfd(argv[1]);
    while(1){
        socklen_t clinetlen=sizeof(clientaddr);
        int*connfdadr=malloc(sizeof(int));
        *connfdadr=Accept(listenfd,(SA*)&clientaddr,&clinetlen);
        pthread_t tid;
        Pthread_create(&tid,NULL,thread,connfdadr);
}
return 0;
}
