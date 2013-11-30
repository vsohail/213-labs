#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define HTTP 80
/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
void pipe_handler(int sig);
void *my_proxy(void *fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_request(rio_t *rp, char *uri, char *host, int *port, char *req);

int main(int argc,char **argv)
{
  int listenfd,port;
  int *connfd;
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
int parse_request(rio_t *rp, char *uri, char *host, int *port, char *req)
{
  char buf[MAXLINE], method[MAXLINE], version [10];
  char *p, *path;
  int i=0;
  size_t n = Rio_readlineb(rp, buf, MAXLINE);
  if(n <= 0) {
    clienterror(rp->rio_fd, "Bad request", "000", "Empty Request", "Request is Empty");
    return -1;
  }
  while(!strcmp(buf,"\r\n")) {
    Rio_readlineb(rp,buf,MAXLINE);
  }
  sscanf(buf, "%s %s %s", method, uri, version);
  p = strstr(uri, "://");
  if (p == NULL) {
    clienterror(rp->rio_fd, "Bad request", "001", "Unsupported Protocol", "The Protocol requested is not supported");
    return -1;
  }
  p += 3;
  for (; *p != '\0' && *p != '/'; p++, i++) {
    host[i] = *p;
  }
  host[i+1]='\0';

  if (*p == '\0') {
    sprintf(uri, "/");
  }
  path = p;
  p = strstr(host, ":");
  if (!p) {
    *p = '\0';
    *port = HTTP;
  }
  else {
    *p = '\0';
    p++;
    *port = atoi(p);
  }

  sprintf(req, "%s %s HTTP/1.0\r\n", method, path);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    if (strstr(buf, ":") && !strstr(buf, "User-Agent:") && !strstr(buf, "Accept:")
        && !strstr(buf, "Accept-Encoding:") && !strstr(buf, "Connection:") && !strstr(buf, "Proxy-Connection:")) {
      sprintf(req, "%s",buf);
    }
    printf(">%s", buf);
  }
  sprintf(req, "%s",user_agent_hdr);
  sprintf(req, "%s",accept_hdr);
  sprintf(req, "%s",accept_encoding_hdr);
  sprintf(req, "Connection: close\r\n");
  sprintf(req, "Proxy-Connection: close\r\n");
  if (!strstr(req, "Host:")) {
    sprintf(req, "%sHost: %s\r\n",req, host);
  }
  sprintf(req, "\r\n");
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
void *my_proxy(void *fd)
{
  return NULL;
}
