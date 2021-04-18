#include <iostream>

#include "../src/MONKE.hpp"

int main(int argc, char const* argv[]) {
    MONKE monke = MONKE();

    monke.listen();

    char* buffer = nullptr;
    size_t n = monke.recv(&buffer);
    char* filename = (char*)malloc(n * sizeof(char));
    cout << (buffer == nullptr) << endl;
    cout << buffer << endl;
    memcpy(filename, buffer, n * sizeof(char));

    cout << filename << endl;

    char* fileBuffer = nullptr;
    FILE* f = fopen(filename, "w+");
    n = monke.recv(&fileBuffer);
    while (n > 0) {
        fwrite(fileBuffer, 1, n, f);
        n = monke.recv(&fileBuffer);
    }
    fclose(f);
    return 0;
}
