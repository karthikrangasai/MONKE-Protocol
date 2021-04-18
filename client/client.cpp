#include <iostream>

#include "../src/MONKE.hpp"

int main(int argc, char const* argv[]) {
    MONKE monke = MONKE();

    char* ip = (char*)malloc(9 * sizeof(char));
    strcpy(ip, "127.0.0.1");

    bool connected = monke.connect(ip, 9);

    char filename[9] = "RFC.md";
    monke.send(filename, 8);

    cout << filename << endl;

    FILE* f = fopen(filename, "r");
    char file[2];
    file[0] = '\0';
    file[1] = '\0';

    int size = 300;
    while (fread(file, size, 1, f) > 0) {
        // cout << "Current send() call: " << file << endl;
        monke.send(file, size);
    }
    fclose(f);
    return 0;
}
