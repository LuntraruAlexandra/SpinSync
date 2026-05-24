
# Explicatia Codului Sursa - SpinSync

Acest document descrie logica de functionare a programului SpinSync, explicand modul in care componentele hardware interactioneaza prin intermediul software-ului si algoritmii matematici care asigura controlul in timp real.

## 1. Includerea bibliotecilor si definitiile (Hardware-ul)
La inceputul programului sunt incluse bibliotecile necesare pentru controlul perifericelor:
* `AccelStepper` se ocupa de gestionarea motorului pas cu pas.
* `Adafruit_GFX` si `Adafruit_ST7789` controleaza ecranul TFT color prin protocolul hardware SPI.
* `RF24` si `SPI` gestioneaza modulul radio nRF24L01 pentru a trimite comenzi wireless catre LED-urile de pe discul rotativ.

Pinii sunt configurati intuitiv: butoanele de control sunt conectate pe pinii analogici (`A0, A1, A2`), receptorul IR se afla pe pinul `A4`, iar buzzerul pe `A5`. Motorul pas cu pas este actionat prin pinii digitali `DIR` (care stabileste sensul de rotatie) si `STP` (care primeste impulsurile de pas/clock).

## 2. Logica Matematica (Tintele si Cercul)
Motorul pas cu pas folosit are o rezolutie nativa de `200 de pasi pentru o rotatie completa` (1.8 grade per pas). Jocul utilizeaza 8 LED-uri distribuite perfect egal pe circumferinta discului. 

Programul calculeaza automat pozitiile teoretice ale acestor LED-uri in vectorul constant `TARGET_POSITIONS`: primul se afla la pasul `0`, al doilea la pasul `25`, al treilea la `50`, continuand simetric pana la ultimul, aflat la pasul `175`.

Variabila `TOLERANCE = 15` defineste o fereastra de eroare de plus sau minus 15 pasi in jurul fiecarei tinte fixe. Deoarece discul se invarte rapid, ar fi imposibil din punct de vedere uman sa se loveasca pasul exact; aceasta toleranta creeaza o zona de declansare valida de aproximativ 27 de grade pentru fiecare LED.

## 3. Masina de Stari (Evolutia Jocului)
In functia `setup()`, sistemul se initializeaza: ecranul este configurat, conexiunea seriala porneste pentru debugging la 115200 baud, modulul radio este setat pe emisie, pinii pentru butoane primesc rezistente interne de pull-up, iar motorul isi seteaza limitele maxime de viteza.

Intreaga logica din `loop()` este guvernata de o Masina de Stari Finita (`switch(currentState)`), care schimba comportamentul programului in mod dinamic:

* **START_MENU:** Starea initiala in care ecranul afiseaza titlul jocului si asteapta apasarea butonului `BUT1` pentru a porni.
* **CALIBRATION:** Etapa critica de aliniere. Motoarele pas cu pas nu au feedback nativ de pozitie absoluta la pornire. Din acest motiv, roata incepe o miscare de rotatie foarte lenta. Utilizatorul trebuie sa priveasca discul si sa apese din nou `BUT1` exact in momentul in care primul LED trece prin dreptul stativului fix (reperul de referinta). In acea milisecunda, programul ruleaza `stepper.setCurrentPosition(0)`, fixand matematic originea sistemului pe baza geometriei fizice a aparatului.
* **SELECT_MODE:** Ecranul solicita alegerea nivelului de dificultate. Apasarea `BUT1` configureaza o viteza de joc de 35 (Usor), `BUT2` seteaza 65 (Mediu), iar `BUT3` seteaza 110 (Rapid). Imediat dupa selectare, motorul primeste parametrii stabiliti si se trece in starea de joc activ.
* **PLAYING:** Aceasta este starea principala. Motorul primeste o tinta de pasi simbolica si uriasa (`stepper.moveTo(40000)`), iar functia non-blocanta `stepper.run()` este apelata fortat la fiecare iteratie a loop-ului. Aceasta misca motorul cu cate un pas doar daca a sosit timpul, permitand restului de cod sa verifice senzorii in paralel, fara intreruperi sau sacadari ale miscarii.

## 4. Algoritmul de Detectie a Tragerii (Sincronizarea)
In timp ce discul se invarte in starea `PLAYING`, software-ul interogheaza constant starea receptorului IR (`digitalRead(IR_RECEIVER) == LOW`). Cand utilizatorul apasa pe telecomanda si trimite un impuls luminos, se declanseaza instantaneu urmatorul algoritm:

1. **Normalizarea Pozitiei:** Se citeste pozitia absoluta a motorului raportata la punctul de calibrare si se aplica operatia modulo: `currentPos % STEPS_PER_REV`. Acest lucru transforma contorul total de pasi intr-un index simplu de la 0 la 199, reprezentand coordonata exacta pe cercul curent.
2. **Calculul Distantei pe Cerc:** Programul bucleaza prin cele 8 pozitii teoretice ale LED-urilor si calculeaza diferenta absoluta dintre punctul tragerii si pozitia tintei. Deoarece lucrul se desfasoara pe o geometrie circulara, apare o problema la trecerea prin zero: daca utilizatorul trage la pasul 199, iar LED-ul se afla la pasul 0, diferenta aritmetica simpla este 199 pasi, desi geometric ei se afla la doar 1 pas distanta. Linia de cod `if (diff > STEPS_PER_REV / 2) { diff = STEPS_PER_REV - diff; }` corecteaza aceasta eroare, alegand mereu drumul cel mai scurt pe cerc.
3. **Hit (Ocherit):** Daca distanta minima calculata este mai mica sau egala cu `TOLERANCE` (15 pasi), tragerea este considerata corecta. Daca LED-ul respectiv nu fusese deja activat, scorul creste, se deseneaza noul scor pe ecran (folosind o stergere partiala cu `fillRect` pentru a preveni lag-ul), buzzerul scoate un sunet scurt de confirmare de inalta frecventa (`2000 Hz`) si se trimite un pachet radio wireless catre discul mobil cu indexul LED-ului pentru a fi aprins fizic.
4. **Victorie:** Daca scorul ajunge la 8 (toate LED-urile au fost aprinse), motorul se opreste, se masoara timpul total de joc prin functia `millis()`, se calculeaza un scor de performanta ponderat in functie de dificultate si timp, iar starea devine `WIN`.
5. **Miss (Ratat):** Daca distanta calculata fata de toate cele 8 LED-uri este mai mare de 15 pasi, utilizatorul a tras in gol. Motorul se opreste instant, se executa un semnal acustic grav si lung de eroare (`300 Hz`), toate LED-urile se sting prin transmisie radio, iar jocul trece in starea `GAME_OVER`.

## 5. Starile Finale (WIN / GAME_OVER)
In aceste faze, executia dinamica este oprita. Ecranul TFT afiseaza panoul final cu statistici (Timp, Scor de performanta sau Mesaj de esec) si sistemul ramane in asteptare. La apasarea butoanelor, toate variabilele se reseteaza, vectorii de stare se golesc si masina de stari revine la `START_MENU`, pregatita pentru o noua runda.
