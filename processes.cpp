#include <iostream>
#include <unistd.h>
#include <string>
#include <sys/wait.h>
// defining a fixed size for the buffer
#define bufferSize 5000

void processes(std::string arg) {
    // pipe bytesRead index readProcess = 0, writeProcess = 1
    enum {readProcess, writeProcess};
    int bytesRead;
    int wcToParentPipe[2];
    pid_t processID;
    pid_t wcProcessID;
    pid_t grepProcessID;
    pid_t psProcessID;
    char buffer[bufferSize];

    // pipe created
    if (pipe(wcToParentPipe) < 0) {
        perror("pipe error");
    // child forked
    } else if ((wcProcessID = fork()) < 0) {
        perror("fork error");
    // if child process
    } else if (wcProcessID == 0) {
        int grepToWCPipe[2];
        if (pipe(grepToWCPipe) < 0) {
            perror("pipe error");
        } else if ((grepProcessID = fork()) < 0) {
            perror("fork error");
        } else if (grepProcessID == 0) {
            int psToGrepPipe[2];
            if (pipe(psToGrepPipe) < 0) {
                perror("pipe error");
            } else if ((psProcessID = fork()) < 0) {
                perror("fork error");
            // ps process
            } else if (psProcessID == 0) {
                // close "read" end of the process
                close(psToGrepPipe[readProcess]);
                dup2(psToGrepPipe[writeProcess], STDOUT_FILENO);
                // execute system processes via argument passed to the program
                // child process calls "ps" and sends it to the pipe
                execlp("ps", "ps", "-A", (char*)NULL);
                // telling the parent process that we are done reading
                close(psToGrepPipe[writeProcess]);
            // grep process
            } else {
                wait(NULL);
                // unused pipes
                close(psToGrepPipe[writeProcess]);
                close(grepToWCPipe[readProcess]);
                // moving the information written in ps to grep
                dup2(psToGrepPipe[readProcess], STDIN_FILENO);
                dup2(grepToWCPipe[writeProcess], STDOUT_FILENO);
                // execute grep with specified argument given to execlp from the user
                execlp("grep", "grep", arg.c_str(), (char *)NULL);
                // close process pipes after execution
                close(psToGrepPipe[readProcess]);
                close(grepToWCPipe[writeProcess]);
            }
        // wc process
        } else {
            wait(NULL);
            close(grepToWCPipe[writeProcess]);
            close(wcToParentPipe[readProcess]);
            // moving information written in grep to wc
            dup2(grepToWCPipe[readProcess], STDIN_FILENO);
            dup2(wcToParentPipe[writeProcess], STDOUT_FILENO);
            // execute wc with specified argument given to grep from the user, now onto wc
            execlp("wc", "wc", "-l", (char *)NULL);
            // close process pipes after execution
            close(grepToWCPipe[readProcess]);
            close(wcToParentPipe[writeProcess]);
            
        }
    // parent process
    } else {
        wait(NULL);
        // close "write" end of the pipe process
        close(wcToParentPipe[writeProcess]);
        // reading from the pipe to read the data and store it into buffer
        bytesRead = read(wcToParentPipe[readProcess], buffer, bufferSize);
        // close reading process
        close(wcToParentPipe[readProcess]);
        // writing to the pipe so the child process can read it
        std::string output(buffer, bytesRead);
        std::cout << output;
    }
}

int main(int argc, char *argv[]) {

    std::string arg;
    int requiredArugmentCount = 2;

    if (argc > requiredArugmentCount) {
        std::cerr << "Too many arguments provided" << std::endl;
    } else if (argc == requiredArugmentCount) {
        arg = std::string(argv[1]);
        processes(arg);
    } else {
        std::cerr << "Missing argument" << std::endl;
    }
    return 0;
}