#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include "helpers.h"
#include <bits/stdc++.h>
#include "structuri.h"

using namespace std;

void usage(char *file)
{
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

int max(int a, int b)
{
    if (a > b)
    {
        return a;
    }
    return b;
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int socketul, n, ret, r;
    struct sockaddr_in serv_addr;
    char id[11];
    packet m;
    uint8_t msg_type;

    if (argc < 4)
    {
        usage(argv[0]);
    }
    strcpy(id, argv[1]);

    //creez socket-ul pe care se va face conexiunea de tip TCP
    socketul = socket(AF_INET, SOCK_STREAM, 0);
    DIE(socketul < 0, "socket");

    //setez tcp NO_DELAY-ul pe socketul de conumicare
    int flag = 1;
    ret = setsockopt(socketul, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    DIE(ret < 0, "nu merge nodelay-ul");

    //completez datele serverului
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    //ma conectez la server
    ret = connect(socketul, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    //se trimite un mesaj cu id-ul utilizatorului
    memset(&m, 0, sizeof(packet));
    msg_type = 0;
    memcpy(m.payload, &msg_type, sizeof(uint8_t));
    memcpy(m.payload + sizeof(uint8_t), id, sizeof(id));

    int bytes_ramasii = sizeof(packet);
    int bytes_trimisii = 0;
    while (bytes_trimisii != sizeof(packet))
    {
        n = send(socketul, &m + bytes_trimisii, bytes_ramasii, 0);
        bytes_ramasii -= n;
        bytes_trimisii += n;
        DIE(n < 0, "Send\n");
    }

    fd_set read_fds;
    fd_set tmp_fds;
    int fdmax;

    //adaug in setul de socketi, socketii de stdin si socketul de comunicare
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(0, &read_fds);
    FD_SET(socketul, &read_fds);
    fdmax = socketul;

    while (1)
    {
        tmp_fds = read_fds;

        //selectez socket-ul pe care s-au primit informatii
        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        //daca primesc informatii de la tastatura
        if (FD_ISSET(0, &tmp_fds))
        {
            char buffer[200];
            memset(buffer, 0, 200);
            fgets(buffer, 199, stdin);
            //daca un client vrea sa se deconecteze
            if (strncmp(buffer, "exit", 4) == 0)
            {
                break;
            }
            //daca un client da subscribe si este un mesaj valid
            if (strncmp(buffer, "subscribe", 9) == 0 && strlen(buffer) > 10)
            {
                memset(&m, 0, sizeof(packet));
                char topic[51];
                msg_type = 1;
                memset(&topic, 0, 51);
                char *token = strtok(buffer, " \n");
                token = strtok(NULL, " \n");
                if (token != NULL)
                {
                    memcpy(m.payload, &msg_type, sizeof(uint8_t));
                    strcpy(topic, token);
                    memcpy(m.payload + sizeof(uint8_t), &topic, 51);

                    token = strtok(NULL, " \n");
                    if (token != NULL && isdigit(token[0]))
                    {
                        int SF = atoi(token);
                        memcpy(m.payload + sizeof(uint8_t) + 51, &SF, sizeof(int));
                        int bytes_ramasi = sizeof(packet);
                        int bytes_trimisi = 0;
                        while (bytes_trimisi != sizeof(packet))
                        {
                            n = send(socketul, &m + bytes_trimisi, bytes_ramasi, 0);
                            bytes_ramasi -= n;
                            bytes_trimisi += n;
                            DIE(n < 0, "Send\n");
                        }
                        DIE(n < 0, "DC NAIBA NU VREA SA TRIMITA MESAJUL!\n");
                        printf("Subscribed to topic.\n");
                    }
                }
            }
            //daca un client da unsubscribe si este un mesaj valid
            if (strncmp(buffer, "unsubscribe", 11) == 0 && strlen(buffer) > 12)
            {
                memset(&m, 0, sizeof(packet));
                char topic[51];
                msg_type = 2;
                memset(&topic, 0, 51);
                char *token = strtok(buffer, " \n");
                token = strtok(NULL, " \n");
                memcpy(m.payload, &msg_type, sizeof(uint8_t));
                strcpy(topic, token);
                memcpy(m.payload + sizeof(uint8_t), &topic, 51);

                int bytes_ramasi = sizeof(packet);
                int bytes_trimisi = 0;
                while (bytes_trimisi != sizeof(packet))
                {
                    n = send(socketul, &m + bytes_trimisi, bytes_ramasi, 0);
                    bytes_ramasi -= n;
                    bytes_trimisi += n;
                    DIE(n < 0, "send\n");
                }

                DIE(n < 0, "DC NAIBA NU VREA SA TRIMITA MESAJUL!\n");
                printf("Unsubscribed from topic.\n");
            }
        }
        else
        {
            //daca clientul primeste pachete
            memset(&m, 0, sizeof(packet));
            int bytes_ramasi_de_trimis = sizeof(packet);
            int bytes_primiti = 0;
            do
            {
                n = recv(socketul, &m + bytes_primiti, bytes_ramasi_de_trimis, 0);
                bytes_primiti += n;
                bytes_ramasi_de_trimis -= n;
                DIE(n < 0, "receive");
            } while (n != 0 && bytes_primiti != sizeof(packet));
            
            //daca se inchide serverul
            if (n == 0)
            {
                break;
            }
            else
            {
                memset(&msg_type, 0, sizeof(uint8_t));
                memcpy(&msg_type, m.payload, sizeof(uint8_t));

                //daca exista deja un client cu acest id, deja conectat
                if (msg_type == 8)
                {
                    break;
                }
                //daca este un mesaj cu informatii de la server
                if (msg_type == 1)
                {
                    //extrag adresa ip, portul, topicul si tipul de date al mesajului
                    struct informatii *detalii = (struct informatii *)(m.payload + 1);
                    char topic_bun[51];
                    memset(&topic_bun, 0, 51);
                    memcpy(&topic_bun, detalii->topic, 50);
                    struct in_addr ip;
                    ip.s_addr = detalii->adresa_ip;

                    //daca e short_real
                    if (detalii->tip_date == 1)
                    {
                        struct mesaj_cu_short_real *numar_real = (struct mesaj_cu_short_real *)(m.payload + 1 + sizeof(struct informatii));
                        double short_real = htons(numar_real->numar);
                        short_real /= 100;
                        printf("%s:%u - %s - SHORT_REAL - %.2f\n", inet_ntoa(ip), htons(detalii->port), topic_bun, short_real);
                    }

                    //daca e int
                    if (detalii->tip_date == 0)
                    {
                        struct mesaj_cu_int *numar_int = (struct mesaj_cu_int *)(m.payload + 1 + sizeof(struct informatii));
                        int numar_intreg = htonl(numar_int->numar);
                        if (numar_int->semn == 1)
                        {
                            numar_intreg = -1 * numar_intreg;
                        }
                        printf("%s:%u - %s - INT - %d\n", inet_ntoa(ip), htons(detalii->port), topic_bun, numar_intreg);
                    }

                    if (detalii->tip_date == 2)
                    {
                        struct mesaj_cu_float *numar_float = (struct mesaj_cu_float *)(m.payload + 1 + sizeof(struct informatii));
                        double float_number = htonl(numar_float->numar);
                        if (numar_float->semn == 1)
                        {
                            float_number = -1 * float_number;
                        }
                        for (int j = numar_float->putere; j > 0; j--)
                        {
                            float_number /= 10;
                        }
                        printf("%s:%u - %s - FLOAT - %.*f\n", inet_ntoa(ip), htons(detalii->port), topic_bun, numar_float->putere, float_number);
                    }

                    if (detalii->tip_date == 3)
                    {
                        struct mesaj_cu_string *sir_de_caractere = (struct mesaj_cu_string *)(m.payload + 1 + sizeof(struct informatii));
                        printf("%s:%u - %s - STRING - %s\n", inet_ntoa(ip), htons(detalii->port), topic_bun, sir_de_caractere->text);
                    }
                }
            }
        }
    }
    close(socketul);
    return 0;
}