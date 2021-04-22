#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <chrono>

#include "../src/MONKE.hpp"

const unsigned int barWidth = 100;

void drawProgress(size_t totalFileSize, size_t receivedFileSize, bool flush) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    unsigned short barWidth = w.ws_col - 9;
    unsigned short progress = (receivedFileSize * barWidth) / totalFileSize;

    std::cout << "[";
    for (unsigned short i = 0; i < progress; ++i) {
        std::cout << "=";
    }
    std::cout << ">";
    for (unsigned short i = 0; i < (barWidth - progress); ++i) {
        std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0 / barWidth) << " %";
    if (flush) {
        cout << '\r';
    } else {
        cout << "\r\n";
    }
    std::cout.flush();
}

int main(int argc, char const* argv[]) {
    MONKE monke = MONKE();

    monke.listen();

    char* buffer = nullptr;
    size_t n = monke.recv(&buffer);
    char* filename = (char*)malloc(n * sizeof(char));
    memcpy(filename, buffer, n * sizeof(char));

    free(buffer);
    buffer = nullptr;
    n = monke.recv(&buffer);
    size_t fileSize = atol(buffer);
    cout << "File size is: " << fileSize << endl
         << endl;

    auto start_millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    FILE* f = fopen(filename, "w+");
    char* contentBuffer = nullptr;
    size_t receivedSize = 0;
    n = monke.recv(&contentBuffer);
    char* content = nullptr;
    while (n > 0) {
        cout << "Received bytes = " << n << endl;
        drawProgress(fileSize, receivedSize, true);
        // if (n > 0) {
        content = (char*)malloc(n * sizeof(char));
        memcpy(content, contentBuffer, n);
        fwrite(content, sizeof(char), n, f);
        receivedSize += n;
        free(content);
        content = nullptr;
        n = monke.recv(&contentBuffer);
        // }
    }
    auto end_millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    cout << "Seg fault check\n";
    drawProgress(fileSize, receivedSize, false);
    cout << "Seg fault check\n";
    cout << "Seg fault check\n";
    cout << "Total time taken = " << (end_millis - start_millis) << " millisecons" << endl;
    cout << "Seg fault check\n";
    fclose(f);
    return 0;
}
