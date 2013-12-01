#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
/* You won't lose style points for including these long lines in your code */
//static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
//static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
//static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
void pipe_handler(int sig);
void *my_proxy(void *fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse(char *buf, char *uri, char *host, int *port, char *req);

int main(int argc,char **argv)
{
  int listenfd,port,*connfd;
  socklen_t client_len;
  struct sockaddr_in client_addr;
  pthread_t tid;

  if(argc != 2) {
    fprintf(stderr,"usage: %s <port>\n",argv[0]);
    exit(1);
  }
  Signal(SIGPIPE,pipe_handler);
  port = atoi(argv[1]);
  listenfd = Open_listenfd(port);
  client_len = sizeof(struct sockaddr_in);
  while(1) {
    connfd = Malloc(sizeof(int));
    *connfd = Accept(listenfd,(SA *)&client_addr,&client_len);
    Pthread_create(&tid,NULL,my_proxy,connfd);
  }
  return 0;
}
void pipe_handler(int sig)
{
  return;
}

int parse(char *buf, char *uri, char *host, int *port, char *req)
{
  char method[MAXLINE];
  char *p, *path;
  int i=0;
  sscanf(buf, "%s %s", method, uri);
  p = strstr(uri, "://");
  p += 3;
  while(*p != '\0' && *p != '/') {
    host[i] = *p;
    p++;i++;
  }
  host[i+1]='\0';

  if (*p == '\0') {
    sprintf(uri, "/");
  }
  path = p;
  p = strstr(host, ":");
  if (!p) {
    *port = 80;
  }
  else {
    *p = '\0';
    p++;
    *port = atoi(p);
  }
  sprintf(req, "%s %s HTTP/1.0\r\n\r\n", method, path);
  //strcat(req, "\r\n");
  return 0;

}
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Proxy Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Proxy server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
void *my_proxy(void *fd_proxy)
{
  int clientfd, fd,port;
  char port_str[10];
  size_t n;
  char response[MAXBUF], req[MAXBUF];
  char buf[MAXLINE],host[MAXLINE], url[MAXLINE];
  rio_t rio;
  Pthread_detach(pthread_self()); 
  fd = *((int *)fd_proxy);
  Free(fd_proxy);
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  if (parse(buf, url, host, &port, req) == 0) {
    sprintf(port_str,"%d",port);
    clientfd = open_clientfd_r(host,port_str);
    Rio_readinitb(&rio, clientfd);
    Rio_writen(clientfd, req, strlen(req));
    while (1) {
      if ((n = Rio_readnb(&rio, response, MAXBUF)) ==-1) {
        break;
      }
      Rio_writen(fd, response, n);
    }
    Close(clientfd);
  }
  Close(fd);
  return NULL;
}
