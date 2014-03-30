/* $Id: sendsms2mobyt.c 13 2012-06-13 15:08:09Z gcolpart $ */

/*
 * Copyright (c) 2007 Evolix <equipe@evolix.fr>
 * Copyright (c) 2007 Gregory Colpart <reg@evolix.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

// URL enconding (RFC 1738)
int urlencode (char *msg, char *encmsg) {

    char *h = "0123456789abcdef";
    unsigned int i;
    unsigned char c;
    int ind=0;

    for (i=0;i<strlen(msg);i++) {
        c = msg[i];

        if( ('a' <= c && c <= 'z')
        || ('A' <= c && c <= 'Z')
        || ('0' <= c && c <= '9')
        || c == '-' || c == '_' || c == '.' ) {
            encmsg[ind] = c;
            ind++;
        } else if( c == ' ' ) {
            encmsg[ind] = '+';
            ind++;
        } else {
            encmsg[ind] = '%';
            encmsg[ind+1] = h[c >> 4];
            encmsg[ind+2] = h[c & 0x0f];
            ind +=3;
        }
    }

    return ind-1;
}

// Display error
void myerror(char *msg) {

    syslog(LOG_ERR,"fatal error: %s\n",msg);

}

int main(void) {

    int sock;
    int errsv;

    FILE *conf = fopen("/etc/sendsms2smsmode.conf","r");
    char line[BUFSIZ];

    char *number1=NULL;
    char *number2=NULL;
    char *number3=NULL;
    char *ip="127.0.0.1";
    char *port="80";
    char *host="localhost";
    char *user="foo";
    char *pass="bar";

    while (fgets(line,BUFSIZ*sizeof(char),conf)) {

        size_t slen,len = strlen(line);
        char *first = calloc(len, sizeof(char));
        char *second = calloc(len, sizeof(char));

        slen = sscanf(line, "%s %s", first, second);

        // TODO : be more clean...
        if ((len < 2) || (!second) || (first[0] == '#')) {
            continue;
        } else {
            //printf("[%d][%d] line = %s %s\n",len,slen,first,second);
            // TODO : utiliser switch/case ?
            if (strcmp(first,"user") == 0) {
                user = strdup(second);
            } else if (strcmp(first,"pass") == 0) {
                pass = strdup(second);
            } else if (strcmp(first,"number1") == 0) {
                number1 = strdup(second);
            } else if (strcmp(first,"number2") == 0) {
                number2 = strdup(second);
            } else if (strcmp(first,"number3") == 0) {
                number3 = strdup(second);
            } else if (strcmp(first,"ip") == 0) {
                ip = strdup(second);
            } else if (strcmp(first,"port") == 0) {
                port = strdup(second);
            } else if (strcmp(first,"host") == 0) {
                host = strdup(second);
            } else {
                myerror("parse error");
                exit(-1);
            }
        }

        free(first);
        free(second);
    }

    fclose(conf);

    // debug
    /*
    printf("user = %s\n",user);
    printf("pass = %s\n",pass);
    printf("number1 = %s\n",number1);
    printf("number2 = %s\n",number2);
    printf("number3 = %s\n",number3);
    printf("ip = %s\n",ip);
    printf("port = %s\n",port);
    printf("host = %s\n",host);
    */

    char tmpmsg[BUFSIZ];

    char buf[BUFSIZ];
    char encbuf[BUFSIZ];
    char httpmsg[BUFSIZ];
    char result[BUFSIZ];
    size_t bytes_read;
    int bytes_enc;
    // designed to be piped
    int fd = STDIN_FILENO;

    struct sockaddr_in sa_in;

    memset(&sa_in,0,sizeof(struct sockaddr_in));
    sa_in.sin_family = AF_INET;
    sa_in.sin_port = htons(atoi(port));
    sa_in.sin_addr.s_addr = inet_addr(ip);

    // socket init 
    sock = socket(PF_INET, SOCK_STREAM, 0);
    errsv = errno;

    if (sock == -1) {
        snprintf(tmpmsg,23+strlen(strerror(errsv)),"error in socket init: %s\n",strerror(errsv));
        myerror(tmpmsg);
        exit(-1);
    }

    // TCP connection init
    if (connect(sock,(struct sockaddr *)&sa_in,sizeof(struct sockaddr)) == -1) {
        errsv = errno;
        snprintf(tmpmsg,33+strlen(strerror(errsv)),"error in tcp connection init: %s\n",strerror(errsv));
        myerror(tmpmsg);
        exit(-1);
    }

    // get data (only 160 first char)
    memset(&buf,0,BUFSIZ);
    bytes_read = read(fd,buf,160);

    // url encoding
    memset(&encbuf,0,BUFSIZ);
    bytes_enc = urlencode(buf,encbuf);
    // limit encbuf to 160 first char
    encbuf[159] = '\0';

    // debug
    //printf("buf (size = %zd) = \n\n%s \n\n",bytes_read,buf);
    //printf("encbuf (size = %d) = \n\n%s \n\n",bytes_enc,encbuf);

    // HTTP GET request (read Mobyt doc)
    // 103 is length of request without var
    snprintf(httpmsg,103+strlen(user)+strlen(pass)+strlen(number1)+strlen(host)+strlen(encbuf),"GET /http/1.6/sendSMS.do?pseudo=%s&pass=%s&numero=%s&message=%s HTTP/1.1\nHost: %s\nUser-Agent: Evolix SMS Agent\n\n",user,pass,number1,encbuf,host);

    // write/read datas
    write(sock,httpmsg,strlen(httpmsg));

    bytes_read = read(sock,result,1024);
    result[bytes_read-1] = '\0';

    // debug
    //printf("request = \n\n%s \n\nresult = \n\n%s",httpmsg,result);

    size_t len = strlen(result);
    char *subresult = calloc(len, sizeof(char));
    char *subsubresult = calloc(len, sizeof(char));

    // TODO : capture OK/KO to verify success
    subresult = strstr(result,"0 | ");
    if (subresult) {
        sscanf(subresult,"0 | %s",subsubresult);
        //snprintf(tmpmsg,24+strlen(subsubresult),"send sms with success: %s\n",subsubresult);
        //syslog(LOG_INFO,tmpmsg);
    } else {
        myerror("error while sending");
        snprintf(tmpmsg,22+strlen(result),"error while sending :\n%s",result);
        errx(1, tmpmsg);
        exit(-1);
    }

    if (number2) {

	snprintf(httpmsg,103+strlen(user)+strlen(pass)+strlen(number1)+strlen(host)+strlen(encbuf),"GET /http/1.6/sendSMS.do?pseudo=%s&pass=%s&numero=%s&message=%s HTTP/1.1\nHost: %s\nUser-Agent: Evolix SMS Agent\n\n",user,pass,number1,encbuf,host);
        write(sock,httpmsg,strlen(httpmsg));
        bytes_read = read(sock,result,1024);
        result[bytes_read-1] = '\0';
        // debug
        //printf("request = \n\n%s \n\nresult = \n\n%s",httpmsg,result);
    }

    if (number3) {

	snprintf(httpmsg,103+strlen(user)+strlen(pass)+strlen(number1)+strlen(host)+strlen(encbuf),"GET /http/1.6/sendSMS.do?pseudo=%s&pass=%s&numero=%s&message=%s HTTP/1.1\nHost: %s\nUser-Agent: Evolix SMS Agent\n\n",user,pass,number1,encbuf,host);
        write(sock,httpmsg,strlen(httpmsg));
        bytes_read = read(sock,result,1024);
        result[bytes_read-1] = '\0';
        // debug
        //printf("request = \n\n%s \n\nresult = \n\n%s",httpmsg,result);
    }

    close(sock);

    return 0;
}

