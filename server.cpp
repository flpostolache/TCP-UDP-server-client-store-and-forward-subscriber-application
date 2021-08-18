#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include "helpers.h"
#include "structuri.h"

using namespace std;

void usage(char *file)
{
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

//functie care determina maximul dintre 2 numere
int max(int a, int b)
{
    if (a > b)
    {
        return a;
    }
    return b;
}

//functie care transforma un vector de caractere intr-un string
string convertToString(char *a, int size)
{
    int i;
    string s = "";
    for (i = 0; i < size && a[i] != '\0'; i++)
    {
        s = s + a[i];
    }
    return s;
}

int main(int argc, char *argv[])
{

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int portno, socket_listen_tcp, socket_udp;

    struct sockaddr_in serv_addr;
    int n, i, ret, este_inca_deschis = 1, r;
    int bytes_trimisi, bytes_ramas_de_trimis, bytes_primiti, bytes_ramas_de_primit;

    unordered_map<string, struct client> map_de_client;
    unordered_map<string, vector<string>> map_de_abonamente;
    unordered_map<int, string> corespondenta_sokcet_id;

    packet m;
    uint8_t msg_type;

    fd_set read_total_fds;
    fd_set tmp_total_fds;
    int fdmax_total;

    if (argc < 2)
    {
        usage(argv[0]);
    }

    FD_ZERO(&read_total_fds);
    FD_ZERO(&tmp_total_fds);

    //creez socket-ul pe care se vor astepta conexiuni de tip TCP
    socket_listen_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(socket_listen_tcp < 0, "socket_tcp");

    //setez tcp NO_DELAY-ul pe socketul de listening
    int flag = 1;
    ret = setsockopt(socket_listen_tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    DIE(ret < 0, "nu merge nodelay-ul");

    //creez socket-ul pe care se vor astepta mesaje de tip UDP
    socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(socket_udp < 0, "socket_udp");

    //convertesc portul primit ca parametru
    portno = atoi(argv[1]);
    DIE(portno == 0, "atoi");

    //completez datele serverului
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    //leg datele de identificare a serverului de socket-ul de listen
    ret = bind(socket_listen_tcp, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind_tcp");

    //pasivizez socket-ul TCP pentru a primi doar cereri de conexiune
    ret = listen(socket_listen_tcp, INT_MAX);
    DIE(ret < 0, "listen_tcp");

    //adaug socket-ul in lista de socketi pe care pot primi informatii
    FD_SET(socket_listen_tcp, &read_total_fds);
    fdmax_total = socket_listen_tcp;

    //leg datele de identificare a serverului de socket-ul pe care se vor primi informatii UDP
    ret = bind(socket_udp, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind_udp");

    //adaug socket-ul in lista de socketi pe care pot primi informatii
    FD_SET(socket_udp, &read_total_fds);
    //adaug si stdin-ul pentru a putea citi date de la tastatura
    FD_SET(0, &read_total_fds);
    fdmax_total = max(socket_listen_tcp, socket_udp);

    //cat timp serverul nu a primit comanda exit
    while (este_inca_deschis)
    {
        tmp_total_fds = read_total_fds;

        //selectez socket-ul pe care s-au primit informatii
        ret = select(fdmax_total + 1, &tmp_total_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (i = 0; i <= fdmax_total; i++)
        {
            if (FD_ISSET(i, &tmp_total_fds))
            {
                //daca socket-ul e socketul pe care se conecteaza noi clienti tcp
                if (i == socket_listen_tcp)
                {
                    struct sockaddr_in cli_addr;
                    socklen_t clilen = sizeof(cli_addr);
                    memset(&cli_addr, 0, sizeof(sockaddr_in));
                    uint8_t tip_mesaj;
                    char id[11];
                    
                    //accept un nou client
                    int newsockfd = accept(socket_listen_tcp, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(newsockfd < 0, "accept");

                    memset(&m, 0, sizeof(packet));

                    //astept sa aflu id-ul clientului care tocmai s-a conectat
                    bytes_ramas_de_trimis = sizeof(packet);
                    bytes_primiti = 0;
                    do
                    {
                        n = recv(newsockfd, &m + bytes_primiti, bytes_ramas_de_trimis, 0);
                        bytes_primiti += n;
                        bytes_ramas_de_trimis -= n;
                        DIE(n < 0, "receive");
                    } while (n != 0 && bytes_primiti != sizeof(packet));

                    memcpy(&tip_mesaj, m.payload, 1);
                    memcpy(id, m.payload + sizeof(uint8_t), 11);

                    string id_str = convertToString(id, 11);

                    //daca este un client cu un id inexistent in sistem
                    if (map_de_client.find(id_str) == map_de_client.end())
                    {
                        //creez o entitate noua de tip client in care salvez datele noului utilizator
                        client nou;
                        nou.socket_pe_care_este_conectat = newsockfd;
                        map_de_client.insert(make_pair(id_str, nou));
                        //dezactivez algoritmul lui Neagle pt socket
                        r = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
                        DIE(r < 0, "setsockop din conectare nou client\n");

                        //adaug noul socket in setul de socketi
                        FD_SET(newsockfd, &read_total_fds);
                        if (newsockfd > fdmax_total)
                        {
                            fdmax_total = newsockfd;
                        }

                        //afisez ca s-a conectat un nou utilizator
                        printf("New client %s connected from %s:%u.\n", id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                        corespondenta_sokcet_id.insert(make_pair(newsockfd, id_str));
                    }
                    else
                    {
                        //daca clientul este deja in sistem, dar a fost deconectat
                        if (map_de_client[id_str].socket_pe_care_este_conectat == -1)
                        {
                            map_de_client[id_str].socket_pe_care_este_conectat = newsockfd;
                            //afisez ca s-a conectat un client
                            printf("New client %s connected from %s:%u.\n", id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                            //dezactivez algoritmul lui Neagle pt socket
                            r = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
                            DIE(r < 0, "setsockop din conectare nou client\n");
                            //adaug noul socket in setul de socketi
                            FD_SET(newsockfd, &read_total_fds);
                            if (newsockfd > fdmax_total)
                            {
                                fdmax_total = newsockfd;
                            }
                            corespondenta_sokcet_id.insert(make_pair(newsockfd, id_str));
                            //in cazul in care un client era deconectat si avea niste abonamente la care
                            //avea SF = 1, trimitem acele mesaje 
                            for (unsigned q = 0; q < map_de_client[id_str].pachete_in_asteptare.size(); q++)
                            {
                                memset(&m, 0, sizeof(packet));
                                memcpy(m.payload, map_de_client[id_str].pachete_in_asteptare.at(q).payload, sizeof(packet));

                                bytes_ramas_de_trimis = sizeof(packet);
                                bytes_trimisi = 0;
                                while(bytes_trimisi != sizeof(packet)){
                                    n = send(newsockfd, &m + bytes_trimisi, bytes_ramas_de_trimis, 0);
                                    bytes_trimisi += n;
                                    bytes_ramas_de_trimis -= n;
                                    DIE(n < 0, "snd\n");
                                }
                            }
                            //stergem acele mesaje
                            map_de_client[id_str].pachete_in_asteptare.clear();
                        }
                        else
                        {
                            //cazul in care exista deja un client conectat cu acest ID
                            printf("Client %s already connected.\n", id);
                            memset(&m, 0, sizeof(packet));
                            msg_type = 8;
                            memcpy(m.payload, &msg_type, sizeof(uint8_t));
                            //trimit un mesaj acelui client sa se inchida
                            bytes_ramas_de_trimis = sizeof(packet);
                            bytes_trimisi = 0;
                            while(bytes_trimisi != sizeof(packet)){
                                n = send(newsockfd, &m + bytes_trimisi, bytes_ramas_de_trimis, 0);
                                bytes_trimisi += n;
                                bytes_ramas_de_trimis -= n;
                                DIE(n < 0, "snd\n");
                            }
                            close(newsockfd);
                        }
                    }
                }
                //daca se primeste informatie de la tastatura
                else if (i == 0)
                {
                    char buffer[10];
                    memset(buffer, 0, 10);
                    fgets(buffer, 9, stdin);
                    //daca este orice altceva in afara de exit, ignor ceea ce primesc de la tastatura
                    if (strncmp(buffer, "exit", 4) == 0)
                    {
                        //serverul se va inchide
                        este_inca_deschis = 0;
                        //inchid toti socketii din read_total_fds si clientii vor primi automat un mesaj cu 0 biti
                        //ceea ce ii va determina sa se inchida
                        for (int j = 0; j <= fdmax_total; j++)
                        {
                            if (FD_ISSET(j, &read_total_fds) && j != 0 && j != socket_listen_tcp && j != socket_udp)
                            {
                                FD_CLR(j, &read_total_fds);
                                close(j);
                            }
                        }
                        break;
                    }
                }
                //daca socketul este socketul pe care se primesc informatii UDP
                else if (i == socket_udp)
                {
                    char informatii_udp[1570];
                    char topic[51];
                    unsigned char tip_date;
                    struct sockaddr_in cli_addr;
                    memset(&cli_addr, 0, sizeof(sockaddr_in));
                    socklen_t socklen = sizeof(cli_addr);

                    memset(topic, 0, sizeof(topic));
                    memset(informatii_udp, 0, 1570);
                    memset(&tip_date, 0, sizeof(unsigned char));

                    //primesc un mesaj de tip UDP
                    int r = recvfrom(socket_udp, informatii_udp, 1570, 0, (struct sockaddr *)&cli_addr, &socklen);


                    memcpy(&topic, &informatii_udp, 50);
                    string topic_string;
                    topic_string = convertToString(topic, 51);
                    //verific daca topicul mesajului udp mai exista deja in baza de date a
                    //programului
                    if (map_de_abonamente.find(topic_string) == map_de_abonamente.end())
                    {
                        //daca nu a mai fost deschis acest topic pana acum (nici de client nici de UDP)
                        //creez o intrare in map-ul de topicuri 
                        vector<string> tmp;
                        map_de_abonamente.insert(make_pair(topic_string, tmp));
                    }
                    else
                    {
                        //daca exista deja cineva care a deschis acel topic, trimit mesajul venit de pe UDP
                        //tuturor clientilor care au dat subscribe la acel topic si sunt conectati
                        vector<string> clienti_la_care_trimit = map_de_abonamente.at(topic_string);

                        for (auto l = clienti_la_care_trimit.begin(); l != clienti_la_care_trimit.end(); l++)
                        {
                            client abonat = map_de_client.at(*l);
                            //daca clientul este conectat si este abonat la topicul primit, trimit mai departe
                            //mesajul acestuia 
                            if (abonat.socket_pe_care_este_conectat != -1)
                            {
                                memset(&m, 0, sizeof(packet));
                                msg_type = 1;
                                memcpy(m.payload, &msg_type, 1);
                                memcpy(m.payload + 1, &(cli_addr.sin_addr), sizeof(uint32_t));
                                memcpy(m.payload + 5, &(cli_addr.sin_port), 2);
                                memcpy(m.payload + 7, informatii_udp, 1570);
                                bytes_ramas_de_trimis = sizeof(packet);
                                bytes_trimisi = 0;
                                while(bytes_trimisi != sizeof(packet)){
                                    n = send(abonat.socket_pe_care_este_conectat, &m + bytes_trimisi, bytes_ramas_de_trimis, 0);
                                    bytes_trimisi += n;
                                    bytes_ramas_de_trimis -= n;
                                    DIE(n < 0, "snd\n");
                                }
                            }
                            else
                            {
                                //daca nu, verific daca pt acest topic, clientul s-a abonat cu SF, caz in care stochez mesajul,
                                //pentru a-l trimite cand se va reconecta
                                vector<string>::iterator parcugem_pachete;
                                parcugem_pachete = find(abonat.abonamente_SF.begin(), abonat.abonamente_SF.end(), topic_string);
                                if (parcugem_pachete != abonat.abonamente_SF.end())
                                {
                                    memset(&m, 0, sizeof(packet));
                                    msg_type = 1;
                                    memcpy(m.payload, &msg_type, 1);
                                    memcpy(m.payload + 1, &(cli_addr.sin_addr), sizeof(uint32_t));
                                    memcpy(m.payload + 5, &(cli_addr.sin_port), 2);
                                    memcpy(m.payload + 7, informatii_udp, 1570);
                                    map_de_client.at(*l).pachete_in_asteptare.push_back(m);
                                }
                            }
                        }
                    }
                }
                //cazul in care un client deja conectat incearca sa comunice cu serverul
                else
                {

                    memset(&m, 0, sizeof(packet));
                    bytes_ramas_de_trimis = sizeof(packet);
                    bytes_primiti = 0;
                    do
                    {
                        n = recv(i, &m + bytes_primiti, bytes_ramas_de_trimis, 0);
                        bytes_primiti += n;
                        bytes_ramas_de_trimis -= n;
                        DIE(n < 0, "receive\n");
                    } while (n != 0 && bytes_primiti != sizeof(packet));


                    //cazul in care un client se deconecteaza
                    if (n == 0)
                    {
                        string id_de_scos = corespondenta_sokcet_id.at(i);
                        cout << "Client " << id_de_scos << " disconnected.\n";
                        map_de_client[id_de_scos].socket_pe_care_este_conectat = -1;
                        FD_CLR(i, &read_total_fds);
                        close(i);
                        corespondenta_sokcet_id.erase(i);
                    }
                    else
                    {
                        char topic[51];
                        memset(topic, 0, 51);

                        memcpy(&msg_type, m.payload, sizeof(uint8_t));
                        //daca este un subscribe
                        if (msg_type == 1)
                        {
                            int SF;
                            //extrag topicul si SF-ul abonamentului
                            memcpy(&topic, m.payload + sizeof(uint8_t), 51);
                            memcpy(&SF, m.payload + sizeof(uint8_t) + 51, sizeof(int));
                            string topic_string_primit = convertToString(topic, 51);
                            string id_la_care_dau_subscribe = corespondenta_sokcet_id.at(i);

                            //verific daca topicul nu exista in baza de date
                            //daca nu, il creez si adaug ca la acest topic este abonat clientul
                            //cu id-ul corespunzator
                            if (map_de_abonamente.find(topic_string_primit) == map_de_abonamente.end())
                            {
                                vector<string> tmp;
                                tmp.push_back(id_la_care_dau_subscribe);
                                map_de_abonamente.insert(make_pair(topic_string_primit, tmp));
                                if (SF == 1)
                                {
                                    //daca SF-ul este 1, il adaug si la lista de abonamente cu SF = 1
                                    map_de_client.at(id_la_care_dau_subscribe).abonamente_SF.push_back(topic_string_primit);
                                }
                            }
                            else
                            {
                                //caut daca in vectorul de persoane abonate de la topicul primit daca persoana noua abonata este deja in vectorul de abonati
                                if (find(map_de_abonamente[topic_string_primit].begin(), map_de_abonamente[topic_string_primit].end(), id_la_care_dau_subscribe) == map_de_abonamente[topic_string_primit].end())
                                {
                                    //daca nu este, il adaug
                                    map_de_abonamente[topic_string_primit].push_back(id_la_care_dau_subscribe);
                                    if (SF == 1)
                                    {
                                        //daca SF-ul este 1, il adaug si la lista de abonamente cu SF = 1
                                        map_de_client.at(id_la_care_dau_subscribe).abonamente_SF.push_back(topic_string_primit);
                                    }
                                }
                            }
                        }
                        //daca este un unsubscribe
                        if (msg_type == 2)
                        {
                            memcpy(&topic, m.payload + sizeof(uint8_t), 51);

                            memcpy(&topic, m.payload + sizeof(uint8_t), 51);
                            string topic_string_primit = convertToString(topic, 51);

                            //daca exista topicul la care abonatul vrea sa dea unsubscribe
                            string id_la_care_trb_sa_dau_unsubscribe = corespondenta_sokcet_id.at(i);
                            if (map_de_abonamente.find(topic_string_primit) != map_de_abonamente.end())
                            {
                                vector<string>::iterator it;
                                //sterg id-ul abonatului din lista de abonati la un topic
                                it = find(map_de_abonamente[topic_string_primit].begin(), map_de_abonamente[topic_string_primit].end(), id_la_care_trb_sa_dau_unsubscribe);
                                if (it != map_de_abonamente[topic_string_primit].end())
                                {
                                    map_de_abonamente[topic_string_primit].erase(it);
                                    //daca era abonat la un topic si cu SF = 1
                                    //sterg topicul si din lista cu abonamente cu SF = 1 al acelui client
                                    it = find(map_de_client.at(id_la_care_trb_sa_dau_unsubscribe).abonamente_SF.begin(), map_de_client.at(id_la_care_trb_sa_dau_unsubscribe).abonamente_SF.end(), topic_string_primit);
                                    if (it != map_de_client.at(id_la_care_trb_sa_dau_unsubscribe).abonamente_SF.end())
                                    {
                                        map_de_client.at(id_la_care_trb_sa_dau_unsubscribe).abonamente_SF.erase(it);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    close(socket_listen_tcp);
    close(socket_udp);
    close(0);
    return 0;
}