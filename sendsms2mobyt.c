/* v 0.2 23.06.2007 */

/* Copyright (c) 2007 Evolix <equipe@evolix.fr>
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

// URL enconding (RFC 1738)
int urlencode (char *msg, char *encmsg) {

    char *h = "0123456789abcdef";
    unsigned int i;
    char c;
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


int main(void) {

    int sock;
    int errsv;

    char buf[BUFSIZ];
    char encbuf[BUFSIZ];
    char httpmsg[BUFSIZ], httpmsg2[BUFSIZ];
    char result[BUFSIZ], result2[BUFSIZ];
    size_t bytes_read;
    int bytes_enc;
    // designed to be piped
    int fd = STDIN_FILENO;

    struct sockaddr_in sa_in;

    // Hum, 194.185.22.170 is actual IP of Mobyt
    // TODO : use hostname
    memset(&sa_in,0,sizeof(struct sockaddr_in));
    sa_in.sin_family = AF_INET;
    sa_in.sin_port = htons(80);
    sa_in.sin_addr.s_addr = inet_addr("194.185.22.170");

    // socket init 
    sock = socket(PF_INET, SOCK_STREAM, 0);
    errsv = errno;

    if (sock == -1) {
        printf("error in socket init : %s\n",strerror(errsv)); 
    }

    // TCP connection init
    if (connect(sock,(struct sockaddr *)&sa_in,sizeof(struct sockaddr)) == -1) {
        errsv = errno;
        printf("error in tcp connection init : %s\n",strerror(errsv)); 
    }

    // get data (only 160 first char)
    memset(&buf,0,BUFSIZ);
    bytes_read = read(fd,buf,160);

    // url encoding
    memset(&encbuf,0,BUFSIZ);
    bytes_enc = urlencode(buf,encbuf);

    // HTTP GET request (read Mobyt doc)
    // 167 is length of request without encbuf (*hack*, *hack*)
    // hey, you see my mobile number: +33688291152
    // TODO : put user/pass and mobile numbers in conffile
    snprintf(httpmsg,167+strlen(encbuf),"GET /sms/send.php?user=F02568&pass=snhom482&rcpt=%%2B33688291152&data=%s&sender=%%2B33491854329&qty=l HTTP/1.1\nHost: multilevel.mobyt.fr\nUser-Agent: Evolix SMS Agent\n\n",encbuf);

    // write/read datas
    write(sock,httpmsg,strlen(httpmsg));

    // TODO : capture OK to verify success
    // TODO : ajust offset
    // TODO : use recv()
    read(sock,result,198);

    // same with Alexandre mobile number
    snprintf(httpmsg2,167+strlen(encbuf),"GET /sms/send.php?user=F02568&pass=snhom482&rcpt=%%2B33632168886&data=%s&sender=%%2B33491854329&qty=l HTTP/1.1\nHost: multilevel.mobyt.fr\nUser-Agent: Evolix SMS Agent\n\n",encbuf);
    write(sock,httpmsg2,strlen(httpmsg2));
    read(sock,result2,198);
    
    close(sock);

    printf("buf (size = %zd) = \n\n%s \n\n",bytes_read,buf);
    printf("encbuf (size = %d) = \n\n%s \n\n",bytes_enc,encbuf);
    printf("request = \n\n%s \n\nresult = \n\n%s",httpmsg,result);
    printf("request = \n\n%s \n\nresult = \n\n%s",httpmsg2,result2);

    return 0;

}

