Nume si Prenume: Postolache Florin
Grupa: 325CC

	Structuri folosite in server:
		-unordered map in care cheia este id-ul unui client si valoarea
	de la o cheie este o structura de tip client
		-structura de client care tine date despre un anumit id pana
	la inchiderea serverului ( socketul pe care este conectat (-1 daca acel client
	s-a deconectat), vector de topicuri la care id-ul este abonat cu SF = 1 si un
	vector de pachete pe care clientul ar trebui sa le primeasca dupa ce se 
	reconecteaza) 
		-unordered map in care cheia este un socket pe care este conectat un
	client si valoarea de la o cheie este id-ul persoanei care este conectat pe
	acel socket
		-unordered map in care cheia este un topic ca string si valoarea
	de la o cheie este un vector cu toate id-urile care sunt abonate la acel
	topic
	
	Structuri folosite in client:
		-structura informatii -> contine adresa ip a clientului UDP,
	portul clientului UDP, un vector de caractere cu topicul primit de la
	UDP si un octet ce reprezinta tipul de date
				       -> aceasta structura o suprapun (ca in
	tema 1) peste payload-ul primit pe client dupa ce primesc un receive
	
		-structura mesaj_cu_int -> in cazul in care tipul de date din
	structura informatii este 0 suprapun aceasta structura peste restul
	payload-ului
					 -> ma folosesc apoi de campurile din
	aceasta structura pentru a construi numarul cerut
	
		-structura mesaj_cu_short_real -> in cazul in care tipul de 
	date din structura informatii este 1, suprapun aceasta structura peste
	restul payload-ului
					   	-> ma folosesc apoi de campurile 
	din aceasta structura pentru a construi numarul cerut
	
		-structura mesaj_cu_float -> in cazul in care tipul de date din
	structura informatii este 2, suprapun aceasta structura peste restul
	payload-ului
					   -> ma folosesc apoi de campurile din
	aceasta structura pentru a construi numarul cerut
	
		-structura mesaj_cu_string -> in cazul in care tipul de date din
	structura informatii este 3, suprapun aceasta structura peste restul
	payload-ului
					   -> ma folosesc apoi de campul din
	aceasta structura pentru a construi numarul cerut
	
	
	Logica in subscriber:
		-deschid un socket
		-dezactivez algoritmul lui Nagle pt acel socket
		-ma conectez la server
		-imediat dupa conectare trimit un mesaj in care se afla
	id-ul subscriber-ului (si alte date, care ma ajuta la identificarea
	tipului de mesaj in server)
		-creez cele doua seturi de socketi si adaug in cel principal
	socketul de stdin si cel pe care primesc date de la server
		-in cazul in care clientul scrie comanda exit,
	clientul iese din while-ul infinit si inchide socketul pe care comunica
	cu serverul. In acelasi timp serverul va primi un mesaj de 0 bytes care
	va semnifica ca subscriber-ul s-a deconectat.
		-in cazul in care clientul scrie o comanda de subscribe,
	citesc si celelalte doua argumente necesare, asigurnadu-ma ca exista si
	sunt valide, apoi trimit serverului un mesaj in care primul octet are
	valoarea 1, ce simbolizeaza un mesaj de subscribe. Langa octetul de mesaj
	asez in payload topicul si apoi SF-ul acestui topic. Trimit apoi mesajul.
	(octetii nefolositi raman 0)
		- in cazul in care clientul scrie o comanda de unsubscribe,
	citesc si topicul, asigurandu-ma ca exista si este valid, apoi trimit 
	serverului un mesaj in care primul octet are valoarea 2, 
	ce simbolizeaza un mesaj de unsubscribe. Langa octetul de mesaj
	asez in payload topicul. Trimit apoi mesajul. (octetii nefolositi raman 0)
		- in cazul in care clientul primeste un mesaj de la server, verific
	numarul de octeti care s-au primit. Daca s-au primit 0 octezi, inseamna ca
	serverul s-a inchis, deci si clientul trebuie sa se inchida. Daca s-a primit
	un alt numar de octeti, verific primul octet din payload care imi semnifica
	tipul de mesaj primit de la server. Daca acest octet are valoarea 8, inseamna
	ca este deja un client cu acest id conectat pe server. Daca are valoarea 1,
	inseamna ca mesajul este un mesaj de la un client UDP. Pe acesta il convertesc
	cu ajutorul structurilor prezentate mai sus.
	
	
	Logica in server:
		- deschid doi socketi (unul de TCP, si unul de UDP)
		- dezactivez algoritmul lui Nagle pt socketul TCP
		- leg cei doi socketi de datele serverului (portul si adresele ip)
		- pasivizez socketul TCP pentru a asculta doar conexiuni pe el
		- adaug cei doi socketi si stdin-ul la setul de socketi pe care comunic
		- Cazuri in functie de socketii pe care primesc date:
			a. Daca socketul pe care primesc date este socketul pasiv de TCP:
				1. Accept conexiunea noua si ii atribui acestui client
			un nou socket.
				2. Astept sa primesc si id-ul de la acesta ( trimit un
			payload in care primul octet are valoarea 0 si dupa care urmea-
			za id-ul acestui client.
				3. Daca este prima oara cand acest id, s-a conectat,
			creez o noua entitate in map-ul de clienti si setez socketul
			din structura client de la id-ul nou ca fiind noul socket primit
			dupa accept. Adaug acest socket in setul de socketi pe care astept
			date.
				4. Daca este un id care se deconectase si s-a reconectat,
			modific in structura de client din map, valoarea socketului pe care
			este conectat, la noul socket. In cazul in care, cat timp a fost 
			deconectat, s-au primit mesaje pe server de la topic-uri la care era
			abonat cu SF = 1, ii trimit acele mesaje.
				5. Daca exista deja conectat un client cu id-ul primit,
			trimit pe noul socket, un mesaj care anunta acel client sa se inchida
			(un payload in care se afla un octet cu valoarea 8). Inchid acel socket
			returnat de accept, fara sa il adaug in set.
			
			b. Daca socketul pe care primesc date este stdin-ul:
				1. Citesc maximum 10 caractere si verific ca acestea
			sa fie exact cuvantul exit, caz in care inchid fiecare socket
			creat pana in momentul curent, si inchid si serverul.
				2. Orice alta comanda o ignor.
				
			c. Daca socketul pe care primesc date este socket-ul UDP:
				1. Citesc intr-un buffer de 1570 de caractere mesajul.
				2. Extrag topicul din mesaj si verific daca in map-ul
			de topicuri exista deja un topic deschis cu acel nume.
			Daca nu exista, il creez. Daca acesta exista, trimit la toate
			persoanele din vectorul de map_de_abonamente[topic] 
			acest mesaj (doar daca sunt conectati). Daca nu sunt conectati
			si au SF-ul 1 la acest topic, adaug la vectorul de pachete de la
			acesti clienti, acest mesaj.
				3. Structura mesaj catre clienti, primit de la UDP:
			un octet care sa semnifice ca este un mesaj de tip UDP (valoarea 1)
			32 de biti care reprezinta adresa ip (in nbo) a serverului UDP 
			de la care s-a primit mesajul, 16 biti ce reprezinta portul (in nbo)
			serverului UDP de la care s-a primit mesajul, tot mesajul UDP
			primit in sine
			
			d. Daca socketul pe care primesc date este un socket pe care 
			comunic cu un client:
				1. Verific daca am primit 0 bytes de pe acel socket. In
			acest caz inseamna ca acel id s-a deconectat. Modific valoarea
			din structura client a campului socket_pe_care_este_conectat la 
			-1 pentru a simboliza ca s-a deconectat. Scot din setul de socketi
			si inchid socketul pe care comunica acel id.
				2. In cazul in care primesc mai multi bytes, inseamna ca 
			mesajul este unul de subscribe sau unsubscribe.
				3. Extrag primul octet pentru a-mi putea da seama ce tip de
			mesaj este. ( 1 este pentru subscribe si 2 este pentru unsubscribe)
				4. Daca este un subscribe, extrag topicul si SF-ul. Verific
			daca exista deja in map_de_abonamente, topicul la care vrea sa se
			inscrie. Daca nu exista, creez o noua intrare pt map_de_abonamente
			si la vectorul de abonati, corespunzator acestei intrari, adaug
			acest id. Daca exista, adaug la vectorul de la cheia topic, id-ul
			clientului. In ambele cazuri de mai sus, in cazul in care SF = 1,
			adaug acest topic, la vectorul de abonamente SF, corespunzator
			acestui id.
				5. Daca este un unsubscribe, extrag doar topicul. Verific
			ca acest topic sa existe in map_de_abonamente. Daca exista, caut
			id-ul persoanei, in vectorul de id-uri, de la cheia topic si daca 
			exista, elimin id-ul clientului de la care am primit cererea de
			unsubscribe. Verific apoi si in vectoul de abonamente SF al acestui
			id, daca exista acest topic. Daca exista, il elimin.
			
	
