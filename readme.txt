Tema 2 - Protocoale de Comunicatie - Aplicatie peste TCP

Student : Vladulescu Denis-Marian
Grupa : 324CB

In cadrul temei 2 am utilizat urmatoarele structuri de date :

- udp_message - structura de date conform specificatiilor din cerinta care contine :
    - camp pentru topicul mesajului
    - camp pentru tipul mesajului
    - camp pentru informatia utila a mesajului

- tcp_message - structura de date care respecta cerinta si anume implementarea unui protocol la nivel aplicatie peste TCP care contine :
    - camp pentru ip-ul de la care a fost primit mesajul
    - camp pentru portul de la care a fost primit mesajul
    - camp pentru topicul mesajului
    - camp pentru tipul mesajului
    - camp pentru informatia utila a mesajului

- subscriber - structura de date folosita pentru a reprezenta un element de tip subscriber(client efectiv) care contnie campurile :
    - camp pentru starea clientului, conectat/deconectat (camp folosit pentru a evita eliminare inutila din lista de subscriberi a unui client atunci cand se deconecteaza, existand posibilitatea ca acesta sa se reconecteze)
    - camp pentru socket-ul conexiunii curent a clientului cu server-ul
    - camp pentru id-ul clientului
    - camp pentru numarul de topicuri la care clientul este abonat
    - camp pentru topic-urile efective la care este abonat clientul

- client_request - structura de date folosita pentru a reprezenta o cerere a unui client, fie ea de subscribe, unsubscribe sau exit care contine campurile :
    - camp pentru tipul cererii
    - camp pentru topicul la care clientul doreste sa se aboneze/de la care clientul vrea sa se dezaboneze. Cand tipul cererii este de tip EXIT, campul topic va fi gol.

In continuare am sa descriu functionalitatea aplicatiei, pe baza functiilor ajutatoare definite in fisierele server_utils.c si subscriber_utils.c. In fisierele header server_utils.h si subscriber_utils.h sunt definite semnaturile functiilor apelate direct de server, respectiv sunt incluse toate bibliotecile necesare functionarii server-ului.

De mentionat este faptul ca atat in server_utils.c cat si in subscriber_utils.c am definit functiile send_all si recv_all, care au ca scop asigurarea faptului ca un mesaj TCP este trimis/primit in totalitate.



Functionalitatea serverului :

La pornirea acestuia, se dezactiveaza buffering-ul de output cu ajutorul instructiunii setvbuf.
Apoi, se initializeaza cei doi socketi, cel UDP si cel TCP, pe care server-ul va astepta conexiuni. Pentru initializarea acestora, in primul rand sunt creati cei 2 socketi, se face bind pe acestia cu portul primit ca argument la executarea binarului server-ului si cu ip-ul INADDR_ANY deoarece se doreste asteptarea de conexiuni de la orice adresa. De asemenea se va dezactiva algoritmul lui Nagle.
Se initializeaza ulterior vectorul de file descriptori necesar pentru a putea multiplexa I/O. Intrarile initiale sunt socket-ul UDP, socket-ul TCP si STDIN, deoarece server-ul poate primi si comenzi de la tastatura. Toate intrarile asteapta evenimente de tip POLLIN.
Se seteaza starea serverului ca fiind 1(pornit), iar apoi se intra in bucla care ramane activa cat timp server-ul este pornit(nu a primit comanda exit de la tastatura).

Cu ajutorul comenzii poll, se asteapta evenimente pe file descriptorii existenti.
Cand s-a primit o notificare, se itereaza prin acestia pentru a vedea de unde a venit notificare. Cand se gaseste file descriptor-ul in cauza se verifica daca acesta este :
    - cel UDP(deci s-a primit un mesaj de la clientul UDP preimplementat), caz in care se apeleaza functia handle_udp_message
    - daca acesta este cel TCP(deci s-a primit o cerere de conexiune de la un client), caz in care se apeleaza functia handle_tcp_message
    - daca acesta este cel de STDIN(deci s-a primit input de la tastatura), caz in care se apeleaza functia handle_stdin_message
    - sau daca este un file descriptor care reprezinta socket-ul unui client activ(deci s-a primit o cerere de subscribe/unsubscribe/exit), caz in care se apeleaza functia handle_client_message.


handle_udp_message:
Cand este apelata functia handle_udp_message, se defineste o structura struct sockaddr_in pe care se va face recvfrom.
Se pregateste o structura tcp_message care va fi populata cu ip-ul si portul intoarse de recvfrom in structura sockaddr_in definita anterior, respectiv cu tipul mesajului si topicul din mesajul UDP.
Se verifica apoi tipul mesajului primit (INT/SHORT_REAL/FLOAT/STRING), iar in functie de acesta este interpretat campul de date utile pentru a fi pus in mod corespunzator in campul de date utile al structurii de mesaj tcp.
Se apeleaza apoi functia send_to_users.

send_to_users:
Functia send_to_users itereaza prin toti clientii activi si verifica, cu ajutorul functiei match_topic, daca un client este abonat la topicul mesajului primit, caz in care se apeleaza functia send_all pentru a se trimite pachetul tcp format anterior, iar in caz contrar trece la urmatorul client.

match_topic:
Functia match_topic trece prin toate topicurile la care este abonat un client(inclusiv cele care contin wildcard-uri) si verifica daca topicul mesajului curent primit face match pe vreunul din topicurile clientului.


handle_tcp_message:
Cand este apelata functia handle_tcp_message, server-ul face accept pe socket-ul tcp pentru a primi conexiunea initiata de client.
Server-ul face apoi recv_all pentru a primi ID-ul clientului care doreste sa se conecteze.
Se cauta apoi in lista de clienti activi, cu ajutorul find_subscriber_by_id, daca exista deja un client cu acelasi ID care este activ, caz in care se va afisa mesajul "Client X is already connected.", iar server-ul va trimite clientului, cu ajutorul functiei send_client_conn_reply, un raspuns de tip 'N'(not acknowledged) pentru ca acela din urma sa stie sa nu mai astepte si sa se inchida.
Daca a fost gasit clientul in lista de clienti, dar este deconectat, atunci se accepta conexiunea, se modifica socket-ul cu cel returnat de functia accept, se seteaza starea utilizatorului ca fiind conectat, se afiseaza mesajul "New client X connected from IP:PORT.", se adauga, cu ajutorul functiei add_new_fd_in_poll, in structura de poll file descriptor-ul socketului nou creat si se trimite clientului un mesaj de tip 'A'(acknowledged) pentru ca acesta sa stie ca totul este in regula si conexiunea este stabilita in mod normal.
Daca nu a fost gasit clientul in lista de clienti, se procedeaza ca in cazul anterior, cand a fost gasit, dar nu era online, doar ca se va apela functia add_new_subscriber.

add_new_subscriber:
Functia add_new_subscriber verifica daca numarul curent de subcriberi este 0, caz in care va aloca un vector de dimensiune 1 si va pune in acesta un element de tip subscriber cu informatiile primite ca parametru.
Daca vectorul de subscriberi avea deja subscriberi, se va realoca vectorul cu dimensiunea acestuia + 1 si apoi se introduce noul client.


handle_stdin_message:
Cand este apelata functia handle_stdin_message, server-ul isi declara un buffer, iar apoi citeste in acesta cu ajutorul apelului de sistem read mesajul de la tastatura.
Daca mesajul este "exit", starea server-ului va fi setata ca oprit.
Pentru orice alta comanda, sever-ul nu va efectua nicio operatie.

Un detaliu interesant de implementat a fost ca atunci cand server-ul trece pe starea oprit, inainte sa inchida toti socketii clientilor, acesta va trimite pe acestia un mesaj TCP de tip 'X' care sa le spuna clientilor sa se inchida deoarece server-ul se inchide si nu mai are rost sa ramana activi.


handle_client_message:
Cand este apelata functia handle_client_message, server-ul face recv_all pe socket-ul pe care a fost primita notificare.
Se cauta apoi subscriber-ul care are asociat acel socket cu ajutorul functiei find_subscriber_by_socket.
Se interpreteaza informatiile primite ca un tcp_message.
Se verifica daca lungimea este <= 0, iar daca socket-ul clientului nu este -1, inseamna ca s-a intamplat ceva cu clientul, iar conexiunea va fi inchisa. File descriptor-ul asociat clientului va fi scos din vectorul de poll cu ajutorul functiei remove_fd_from_poll, care va si redimensiona vectorul dupa eliminare.
Altfel, daca socket-ul este cumva -1, s-a intamplat o eroare, caz in care se afiseaza tot mesajul de deconectare.
Se face apoi switch pe tipul mesajului primit de la client :
    - daca tipul este '+', care, conform logicii implementate de mine, semnifica dorinta unui client de a se abona la un topic, se elimina caracterul \n de la finalul topicului(acest lucru se putea realiza si in client, dar am ales sa il fac in server), iar apoi se apeleaza functia subscribe_to_topic.
    - daca tipul este '-', care, conform logicii implementate de mine, semnifica dorinta unui client de a se dezabona de la un topic, se elimina caracterul \n de la finalul topcilui, iar apoi se apeleaza functia unsubscribe_from_topic.
    - daca tipul este '0', care, conform logicii implementate de mine, semnifica dorinta unui client de a se deconecta de la server, se seteaza starea clientului ca fiind deconectat, socket-ul este marcat ca fiind -1, se inchide socket-ul pe care a fost primit mesajul si se acesta se elimina din vectorul de poll.

subscribe_to_topic:
Functia subscribe_to_topic verifica daca numarul curent de topicuri la care este abonat utilizatorul este 0, caz in care aloca spatiu pentru un element, iar in caz contrar realoca spatiul deja existent cu dimensiune numar topicuri + 1.
Se introduce apoi in vector noul topic.

unsubscribe_from_topic:
Similar cu functia subscribe_to_topic, functia unsubscribe_from_topic, functia elimina topicul primit ca parametru din lista utilizatorului, iar apoi realoca spatiul cu dimensiune numar topicuri - 1.
In cazul in care numarul de elemente ramase va fi 0, se elibereaza memoria vectorului, iar apoi se seteaza pe NULL.


Atunci cand sever-ul isi incheie executia, pe langa trimiterea mesajelor de inchidere catre clienti, acesta elibereaza vectorii de poll si de subscriberi.



Functionalitatea clientului :

La pornirea acestuia, se dezactiveaza buffering-ul de output la fel ca si in cazul serverului.
Se apeleaza apoi functia initialize_socket, care, in mod similar cu functia initialize_sockets a server-ului, initializeaza un socket TCP folosit pentru stabilirea unei conexiuni cu server-ul. De data aceasta, structura sockaddr_in va avea campul IP setat cu IP-ul server-ului si campul port cu portul server-ului.
Se apeleaza apoi functia connect_to_server, care va incerca conectarea la server cu ajutorul functiei connect.
Daca reuseste conectarea, clientul va trimite server-ului ID-ul acestuia, iar apoi va astepta de la server raspunsul, fie de tip 'N', fie de tip 'A', conform explicatiilor anterioare.
Daca mesajul este de tip 'A', continuam, daca este de tip 'N', se inchide socket-ul si se incheie executia clientului.

Se initializeaza apoi sirul, fix de data aceasta de 2 file descriptori pentru poll, socket-ul TCP pentru conexiunea cu server-ul si STDIN.
Apoi, intr-o bucla infinita, se face poll pe cei 2 file descriptori, iar daca se primeste o notificare pe vreunul se apeleaza functiile adecvate :
    - daca se primeste notificare pe socket-ul TCP(adica clientul a primit un mesaj de la server) se va apela functia handle_tcp_message
    - daca se primeste notificare pe STDIN(adica a fost primita o comanda de la tastatura) se va apela fnunctia handle_stdin_message.


handle_tcp_message:
Cand este apelata functia handle_tcp_message, clientul apeleaza functia receive_message_from_serer, care apeleaza, la randul ei, functia recv_all, pentru a primi tot mesajul de la server.
In functie de tipul mesajului(0, 1, 2 sau 3) se vor afisa mesajele corespunzatoare, conform cerintei.
Daca mesajul este de tip 'X', semnificand faptul ca se inchide server-ul, clientul isi inchide socket-ul cu server-ul si isi va incheia executia.


handle_stdin_message:
Cand este apelata functia handle_stdin_message, clientul face apelul de sistem read pentru a prelua mesajul furnizat de la tastatura. In functie de mesaj, se vor intampla actiuni diferite :
    - daca mesajul este "exit", clientul va face un mesaj tcp cu tipul '0' pe care il va trimite server-ului ca acesta din urma sa stie despre client ca se deconecteaza, iar apoi clientul inchide socket-ul cu server-ul si isi incheie executia
    - daca mesajul este "subscribe <X>", clientul va face un mesaj tcp cu tipul '+', punand in campul de topic topicul primit de la tastatura, iar apoi trimite acesta mesaj server-ului si afiseaza mesajul de abonare cu succes, "Subscribed to topic <X>."
    - daca mesajul este "unsubscribe <X>", clientul va face un mesaj tcp cu tipul '-', punand in campul de topic topicul primit de la tastatura, iar apoi trimite acest mesaj server-ului si afiseaza mesajul de dezabonare cu succes, "Unsubscribed from topic <X>."
    - pentru orice alta comanda, se afisa mesajul "Invalid command" si nu se va realiza nicio actiune.


Atunci cand clientul isi incheie executia, inchide socket-ul de comunicare cu server-ul.