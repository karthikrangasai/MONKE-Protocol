#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <chrono>

#include "../src/MONKE.hpp"

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

    char* ip = (char*)malloc(9 * sizeof(char));
    strcpy(ip, "127.0.0.1");

    bool connected = monke.connect(ip, 9);

    // char filename[] = "Doctor_Strange.mp4";
    // char filename[] = "small_bunny_1080p_60fps.mp4";
    char filename[] = "RFC.md";
    // char filename[] = "Demo.mp4";
    // char filename[] = "assignment_2.pdf";
    monke.send(filename, strlen(filename));

    FILE* f = fopen(filename, "r");
    fseek(f, 0L, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    cout << "File size is: " << fileSize << endl
         << endl;
    char _fileSize[10];
    memset(_fileSize, 0, 10);
    sprintf(_fileSize, "%ld", fileSize);

    monke.send(_fileSize, strlen(_fileSize));

    int size = 300;
    char fileBuffer[size];
    memset(fileBuffer, 0, size);
    size_t bytes_read = 0, bytes_sent = 0;
    auto start_millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    while ((bytes_read = fread(fileBuffer, 1, size, f)) > 0) {
        monke.send(fileBuffer, bytes_read);
        bytes_sent += bytes_read;
        drawProgress(fileSize, bytes_sent, true);
    }
    auto end_millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    drawProgress(fileSize, bytes_sent, false);
    cout << "Total time taken = " << (end_millis - start_millis) << " millisecons" << endl;
    fclose(f);
    return 0;
}
