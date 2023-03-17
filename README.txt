// Boldeanu Ana-Maria
// 321 CD
// PCom - HTTP Client Server Application

=========================== Cerinte implementate ===========================

  * register
  * login
  * enter_library
  * get_books
  * get_book
  * add_book 
  * delete_book
  * logout 
  * exit

============================ Structura arhivei =============================

    * client.c - Aplicatia principala
    * requests.h, requests.c - Definitii de constante si functii ajutatoare
    * parson.h, parson.c - Libraria folosita pentru parsarea fisierelor JSON
    * buffer.h, buffer.c - cele din Laboratorul 10
    * helpers.h, helpers.c - cele din Laboratorul 10
    * Makefile - Aplicatia se compileaza cu regula make


=========================== Descrierea rezolvarii ===========================

  In general, am pus explicatii pe cod cu absolut toti pasii pe care i-am urmat.

  In main(), este un loop infinit alcatuit din pasii urmatori:
1. Deschide conexiunea la server pe un socket.
2. Citeste comanda data de utilizator de la tastatura
3. Interpreteaza comanda
    - Verifica daca e valida;
    - Executa actiuni in functie de comanda primita;
    - Comanda exit duce la oprirea loop-ului.
4. Inchide conexiunea la server.


********** Trimitere mesaje **********

  Am folosit variabila client_logged_in pentru a retine daca utilizatorul s-a
autentificat deja. 
    - pentru register si login, nu permite utilizatorului sa se autentifice de 
mai multe ori.
    - in restul comenzilor, impiedica utilizatorul neautentificat din a accesa
serverul in alte moduri.

  Am folosit variabila client_authorized pentru a retine daca utilizatorul a
obtinut accesul la biblioteca. Folosesc aceasta variabila doar in cazul add_book,
pentru ca nu are sens ca utilizatorul sa scrie datele cartii daca serverul
oricum nu il va lasa sa o publice.

  Pentru valorile care trebuie neaparat sa fie numere (ID si page_count), nu las 
utilizatorul sa introduca alte valori (stringuri), ci astept input pana cand se 
ofera un numar. La server se trimite mesajul direct formatat corect.

  In rest, toate mesajele de eroare sunt obtinute in urma trimiterii unui mesaj 
spre server si interpretarea raspunsului acestuia, cu cod de eroare si continut.


********** Lucrul cu JSON ***********

  Pentru afisarea raspunsului serverului in cazul get_books si get_book, am
extras informatiile despre carti cu ajutorul operatiilor pe stringuri si le-am 
afisat in formatul JSON in care au venit de la server. 

  Pentru crearea de obiecte JSON in cazul register, login si add_book,
am folosit libraria Parson. Am considerat aceasta librarie foarte usor de folosit 
deoarece aveam nevoie doar de cateva functii:
    - json_value_init_object    // Creeaza obiectul JSON
    - json_object_set_number    // Adauga un camp cu valoare numar
    - json_object_set_string    // Adauga un camp de tipul string
    - json_serialize_to_string_pretty // Construieste stringul de format JSON
  
  Functiile care se ocupa efectiv de crearea stringului JSON care trebuie trimis 
catre server (asignare valori etc.) sunt implementate in requests.c:
    * create_userdata_json_string
    * create_bookdata_json_string


********** Construire requests ***********

    Am folosit 3 tipuri de HTTP requests: GET, POST si DELETE. Acestea 
sunt formatate in requests.c, in functiile aferente.

    Am pornit de la implementarea functiilor GET si POST din laborator, 
adaugand doar un nou parametru char *jwt_auth, folosit pentru cazurile 
in care trebuie adaugat si headerul Authorization corespunzator accesului 
in librarie cu tokenul JWT (Daca nu e necesar acest header, se poate da 
NULL in loc de jwt_auth si acesta va fi sarit).
    DELETE e foarte asemanator cu GET.


************* Flow comenzi ************

    1. Verifica starea curenta a utilizatorului.
    2. Daca este necesar, asteapta input de la tastatura.
    3. Construieste mesajul care trebuie trimis serverului.
    4. Trimite mesajul. 
    5. Primeste raspunsul.
    6. Extrage codul de HTTP status din raspuns: 

        I. Daca incepe cu cifra 2, afiseaza mesajul de succes. In plus, pentru:
            * login - actualizeaza starea utilizatorului in variabila
                      client_logged_in.
                    - extrage cookie-ul de autentificare din raspuns si 
                      retine-l in variabila locala login_cookie.
            * enter_library - actualizeaza starea utilizatorului in 
                      variabila client_authorized.
                            - extrage tokenul JWT din raspuns si retine-l 
                      in variabila locala token_JWT.
            * get_books - extrage array-ul JSON din raspuns, iar daca acesta 
                      este gol afiseaza un prompt relevant. Altfel, afiseaza 
                      rezultatul.
            * get_book - extrage si afiseaza JSON-ul cu informatii despre carte.
            * logout - actualizeaza client_logged_in, client_authorized si
                       reseteaza token_JWT.

        II. Altfel, a avut loc o eroare. Pentru a afisa eroarea, folosesc functia
            * show_error, care interpreteaza raspunsul serverului si afiseaza un 
            mesaj sugestiv pentru utilizator.

        