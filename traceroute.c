#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>


// 1's Comp. calculation like labwork 
unsigned short checksum(unsigned short  *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short ret;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    ret = ~sum;
    return ret;
}



int main(int argc, char *argv[])
{
    int sock;
    char send_buf[500], recv_buf[500], src_name[256], src_ip[15], dst_ip[15];
    struct ip *ip = (struct ip *)send_buf;
    struct icmp *icmp = (struct icmp *)(ip + 1);
    struct hostent *src_hp, *dst_hp;
    struct sockaddr_in src, dst;
    struct timeval t;
    int one;
    int bytes_sent, bytes_recv;
    int dst_addr_len, src_addr_len;
    int result;
    fd_set socks;
    int ttlvalue;
    one = 1;
    
    // Getting time to live value of packet from user
    printf("Enter the ttl value for traceroute: ");
    scanf("%d" ,&ttlvalue);
    
    /* Initialize variables */
  
    memset(send_buf, 0, sizeof(send_buf));

    // Controlled the usage
    if (argc < 2)
    {
        printf("\nYou should write like this : %s <dst_server>\n", argv[0]);
        printf("- dst_server is the target\n");
        exit(EXIT_FAILURE);
    }

    // controlled address
    if (getuid() != 0)
    {
        fprintf(stderr, "%s: Requires root privileges\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // getting source ip address
    if (gethostname(src_name, sizeof(src_name)) < 0)
    {
        exit(EXIT_FAILURE);
    }
    // controlled source address
    else
    {
        if ((src_hp = gethostbyname(src_name)) == NULL)
        {
            fprintf(stderr, "%s: Unknown source\n", src_name);
            exit(EXIT_FAILURE);
        }
        else
            ip->ip_src = (*(struct in_addr *)src_hp->h_addr);
    }

    // getting destination ip address
    if ((dst_hp = gethostbyname(argv[1])) == NULL)
    {
        if ((ip->ip_dst.s_addr = inet_addr(argv[1])) == -1)
        {
            fprintf(stderr, "%s: Unknown destination\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        ip->ip_dst = (*(struct in_addr *)dst_hp->h_addr);
        dst.sin_addr = (*(struct in_addr *)dst_hp->h_addr);
    }

    sprintf(src_ip, "%s", inet_ntoa(ip->ip_src));
    sprintf(dst_ip, "%s", inet_ntoa(ip->ip_dst));
    printf("Source IP: '%s' >>>>>>> Destination IP: '%s'\n\n", src_ip, dst_ip);

    // RAW SOCKET
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        exit(EXIT_FAILURE);
    }

    // IP structure controlled.
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        perror("setsockopt() for IP_HDRINCL error");
        exit(EXIT_FAILURE);
    }

    // ip structure define
    ip->ip_v = 4;
    ip->ip_hl = 5;
    ip->ip_tos = 0;
    ip->ip_len = htons(sizeof(send_buf));
    ip->ip_id = htons(321);
    ip->ip_off = htons(0);
    ip->ip_ttl = ttlvalue;
    ip->ip_p = IPPROTO_ICMP;
    ip->ip_sum = 0;

    // icmp structure define
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = 123;

    
    dst.sin_family = AF_INET;

    for (int i = 1; i <= ttlvalue; i++)
    {
        // header checksum calculation
        icmp->icmp_cksum = 0;
        ip->ip_sum = checksum((unsigned short *)send_buf, ip->ip_hl);
        icmp->icmp_cksum = checksum((unsigned short *)icmp, sizeof(send_buf) - sizeof(struct icmp));

        // getting destination address length.
        dst_addr_len = sizeof(dst);
        src_addr_len = sizeof(src);
       
        // 5.0 sn time-out set
        t.tv_sec = 5;
        t.tv_usec = 0;

        // set listen structure
        FD_ZERO(&socks);
        FD_SET(sock, &socks);
        if (setsockopt(sock, SOL_IP, IP_TTL, &i, sizeof(i)))
        {
            return -1;
        }
        // sending packet
        if ((bytes_sent = sendto(sock, send_buf, sizeof(send_buf), 0, (struct sockaddr *)&dst, dst_addr_len)) < 0)
        {
            printf("Failed to send packet.\n");
            fflush(stdout);
        }
        else
        {
            // listening response ya da time-out
            if ((result = select(sock + 1, &socks, NULL, NULL, &t)) < 0)
            {
                printf("Receiving packet error.\n");
            }
            else if (result > 0)
            {

                if ((bytes_recv = recvfrom(sock, recv_buf, sizeof(ip), 0, (struct sockaddr *)&src, (socklen_t *)&src_addr_len)) < 0)
                {
                    perror("recvfrom() error");

                    fflush(stdout);
                }
                //  dış networke atmıyor..
                else
                    printf(" Received from %s packet\n", inet_ntoa(src.sin_addr));
            }
            else
            {
                printf("Failed.........\n");
                
            }

        }
    }
    
    printf("\n %d packet\n\tEND\n" ,ttlvalue);


    return (0);
}

