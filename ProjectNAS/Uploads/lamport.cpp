#include <iostream>
#include <cstdlib>
#include <list>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

using namespace std;

struct Poruka {
    int posiljatelj;
    int primatelj;
    int vrijeme;
    char tip;
};

struct Proces{
    int pid{};
    int sat{};
    list<Poruka> lista_zahtjeva;
    list<int> red_cekanja;
};

int broj_procesa = 0;
int broj_odgovora = 0;
struct sockaddr_in socketAddress;

int so;
queue<Poruka> primljenaPoruka;
pthread_mutex_t mutex_pthread;

void otvori() {
    so = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (so < 0) {
        exit(0);
    }
}

void pripregni(int pid) {
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(10000 + 0 * 10 + pid);
    socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(so, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0) {
        exit(0);
    }
}

void primi(Proces &proces) {
    ssize_t velicina = sizeof(socketAddress);
    Poruka poruka{};

    while (true) {
        velicina = recvfrom(so, &poruka, sizeof(poruka), 0, (struct sockaddr *)&socketAddress, (socklen_t *) &velicina);
        if (velicina < 0) {
            exit(0);
        }
        if (velicina < sizeof(poruka)) {
            exit(0);
        }
        pthread_mutex_lock(&mutex_pthread);
        cout << "P " << proces.pid << " primio " << poruka.tip << "(" << poruka.posiljatelj << ", " << poruka.vrijeme << ") od P" << poruka.posiljatelj << endl;
        primljenaPoruka.push(poruka);
        pthread_mutex_unlock(&mutex_pthread);
    }
}

void posalji(int pid, Poruka &poruka) {
    ssize_t velicina;
    socketAddress.sin_port = htons(10000 + 0 * 10 + pid);
    socketAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    velicina = sendto(so, &poruka, sizeof(poruka), 0, (struct sockaddr *)&socketAddress, sizeof(socketAddress));
    if (velicina < 0) {
        exit(0);
    }
    if (velicina < sizeof(poruka)) {
        exit(0);
    }
    pthread_mutex_lock(&mutex_pthread);
    cout << "P " << poruka.posiljatelj << " poslao " << poruka.tip << "(" << poruka.posiljatelj << ", " << poruka.vrijeme << ") k P" << poruka.primatelj << endl;
    pthread_mutex_unlock(&mutex_pthread);
}

void obrisi(list<Poruka> &listaPoruka, Poruka &poruka) {
    auto i = listaPoruka.begin();
    while (i != listaPoruka.end()) {
        if ((*i).posiljatelj == poruka.posiljatelj && (*i).vrijeme == poruka.vrijeme) {
            listaPoruka.erase(i++);
            break;
        } else {
            i++;
        }
    }
}

void pomakniVrijeme(Proces &proces, int sat){
    if(proces.sat < sat){
        proces.sat = sat++;
    }else{
        proces.sat++;
    }
    cout << "T(" << proces.pid << ")=" << proces.sat << endl;
}

bool provjeriPoruke(const Poruka &a, const Poruka &b){
    if(a.vrijeme == b.vrijeme && a.posiljatelj < b.posiljatelj) return true;
    if(a.vrijeme == b.vrijeme && a.posiljatelj > b.posiljatelj) return false;
    if(a.vrijeme < b.vrijeme) return true;
    if(a.vrijeme > b.vrijeme) return false;
    return true;
}

// Function to process received messages and generate responses
void obradaZahtjeva(Proces &proces) {
    bool primljena = true;  // Flag to check if there is a received message
    Poruka poruka{}, odgovor{};

    // Lock the mutex to access the shared data structure (primljenaPoruka)
    pthread_mutex_lock(&mutex_pthread);

    // Check if there is a received message in the message queue
    if (!primljenaPoruka.empty()) {
        poruka = primljenaPoruka.front();  // Get the front message
        primljenaPoruka.pop();  // Remove the front message from the queue
        primljena = false;  // Set the flag to indicate that a message is received
    }

    // Unlock the mutex to release the lock on the shared data structure
    pthread_mutex_unlock(&mutex_pthread);

    // Process the received message if there is one
    if (!primljena) {
        // Check the type of the received message
        switch(poruka.tip){
            case 'Z':{
                // If the message is a request ('Z'), add it to the list and send a response ('O')
                proces.lista_zahtjeva.push_back(poruka);
                proces.lista_zahtjeva.sort(provjeriPoruke);  // Sort the list of requests
                pomakniVrijeme(proces, poruka.vrijeme);  // Update the process's logical clock
                odgovor.posiljatelj = proces.pid;
                odgovor.primatelj = poruka.posiljatelj;
                odgovor.tip = 'O';
                odgovor.vrijeme = proces.sat;
                posalji(odgovor.primatelj, odgovor);  // Send the response message
            }

            case 'O':{
                // If the message is a response ('O'), increment the response counter and update the clock
                broj_odgovora++;
                pomakniVrijeme(proces, poruka.vrijeme);
            }

            case 'I':{
                // If the message is an indication ('I'), remove the corresponding request from the list
                obrisi(proces.lista_zahtjeva, poruka);
            }
        }
    }
}
// Function to perform the KO (Critical Section Entry) operation
void kriticniOdsjecak(Proces &proces) {
    Poruka zahtjev{}, odlazna_poruka{};

    // Create a request message for entering the critical section
    zahtjev.posiljatelj = proces.pid;
    zahtjev.tip = 'Z';
    zahtjev.vrijeme = proces.sat;

    // Add the request to the process's list and sort the list based on the logical clocks
    proces.lista_zahtjeva.push_back(zahtjev);
    proces.lista_zahtjeva.sort(provjeriPoruke);

    // Send the request to all other processes
    for (int i = 1; i <= broj_procesa; i++) {
        if (i != proces.pid) {
            zahtjev.primatelj = i;
            posalji(i, zahtjev);
        }
    }

    // Wait until all processes respond or the process is at the front of the list
    do {
        obradaZahtjeva(proces);  // Process received messages
    } while ((broj_odgovora < (broj_procesa - 1)) || proces.lista_zahtjeva.front().posiljatelj != proces.pid);

    // Allow some time for synchronization
    sleep(3);

    // Remove the request from the process's list
    obrisi(proces.lista_zahtjeva, zahtjev);

    // Create and send an indication message to release the critical section
    odlazna_poruka.posiljatelj = proces.pid;
    odlazna_poruka.tip = 'I';
    odlazna_poruka.vrijeme = zahtjev.vrijeme;

    // Send the indication message to all other processes
    for (int i = 1; i <= broj_procesa; i++) {
        if (i != proces.pid) {
            odlazna_poruka.primatelj = i;
            posalji(i, odlazna_poruka);
        }
    }
}

// Function representing the main work of a process
void posao(Proces &proces) {
    otvori();
    pripregni(proces.pid);
    pthread_mutex_init(&mutex_pthread, nullptr);
    thread Pthread(primi, ref(proces));
    sleep(2);
    while (true) {
        bool provjera = false;
        if (!proces.red_cekanja.empty() && (proces.sat >= proces.red_cekanja.front())) {
            provjera = true;
            kriticniOdsjecak(proces);
            proces.red_cekanja.pop_front();
        }
        obradaZahtjeva(proces);
        if (!provjera) {
            sleep(1);
            cout << "Dogadaj(" << proces.pid << ")" << endl;
            pomakniVrijeme(proces, 0);
        }
    }
}

int main(int argc, char **argv) {
    Proces proces;
    int brojac = 0;
    vector<Proces> vProces(0);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "@") == 0) {
            brojac++;
        }

        if (brojac == 0) {
            proces.pid = i;
            proces.sat = atoi(argv[i]);
            vProces.push_back(proces);
        }
        else {
            if ((strcmp(argv[i], "@") != 0)) {
                vProces[brojac - 1].red_cekanja.push_back(atoi(argv[i]));
            }
        }
    }

    broj_procesa = vProces.size();
    if (broj_procesa != brojac) {
        cout << "Error" << endl;
        exit(0);
    }

    for (int i = 0; i < broj_procesa; i++) {
        if (fork() == 0) {
            posao(vProces[i]);
            exit(0);
        }
    }
    for (int i = 0; i < broj_procesa; i++) {
        wait(nullptr);
    }
    return 0;
}