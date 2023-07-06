#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <termios.h>
#include <pwd.h>
#include <dirent.h>
#include <stdlib.h>
#include <glob.h>
#include <cstring>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>
#include <deque>

using namespace std;
// #include <ncurses.h>

#define SH_RL_BUFSIZE 1024
#define size_of_buff 1024
#define CMD_LEN 1024

struct termios oldterm, newterm;

#define KEY_ENTER 0x000a
#define KEY_ESCAPE 0x001b
#define KEY_TAB 0x0009
#define KEY_ERASE oldterm.c_cc[VERASE]
#define KEY_BACKSPACE 0x0008
#define EOT 0x0004
#define KEY_CTRL_A 0x01
#define KEY_CTRL_E 0x0005

const int HISTORY_LIMIT = 1000;
#define HIST_SIZE 1000
#define HIST_DISPLAY_SIZE 1000
#define HISTFILESIZE 10000
/*
#define HIST_SIZE 1000
#define HIST_DISPLAY_SIZE 1000*/
#define ANSI_COLOR_GREEN "\x1b[32;1m"
#define ANSI_COLOR_RESET "\x1b[0m"

class shell_history
{
public:
    int index;
    char **commands;
    int maxSize;
    shell_history()
    {
        this->index = 0;
        this->maxSize = HISTFILESIZE;
        this->commands = (char **)malloc(sizeof(char *) * HISTFILESIZE);
        int fdRead = open(".myHistory.txt", O_CREAT | O_RDONLY, 0666);
        FILE *file = fdopen(fdRead, "r");
        char *command = NULL;
        size_t len = 0;
        while (getline(&command, &len, file) != -1)
        {
            commands[index] = (char *)malloc(sizeof(char) * strlen(command));
            strcpy(commands[index], command);
            commands[index][(int)strlen(commands[index]) - 1] = '\0';
            index++;
        }
        fclose(file);
    }
    void push(char *command)
    {
        if (this->index == this->maxSize)
        {
            this->maxSize = this->maxSize + HISTFILESIZE;
            this->commands = (char **)realloc(this->commands, sizeof(char *) * (this->maxSize));
        }
        this->commands[this->index] = (char *)malloc(sizeof(char) * CMD_LEN);
        strcpy(this->commands[this->index], command);
        this->commands[this->index][(int)strlen(command)] = '\0';
        this->index = this->index + 1;
    }
    void updateFile()
    {
        FILE *file = fopen(".myHistory.txt", "w");
        int ind = max(0, index - HISTFILESIZE);

        for (int i = ind; i < index; i++)
        {
            fprintf(file, "%s", this->commands[i]);

            if (i < index - 1)
                fprintf(file, "\n");
        }
        fclose(file);
    }
};

int pathlen = 0;
int handler = 0;
int verbose = 0;
int running;
set<pid_t> pids;
extern vector<string> commands;
int flag;

char getch(void);
int print_prompt();
int getpathlen();
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
void expandWildcards(const std::string &arg, std::vector<std::string> &args);
void Func_Executing_Command(string inputted_command);
vector<int> par_proc(int pid);
void all_proc(int pid, bool s_flag);
void IORedirection_func(string incoming_input_str, string outgoing_output_str);
vector<string> IODivide_func(string inputted_command);
vector<string> STRDivide_wrt_delim_func(string inputted_command, char delim);
vector<int> kill_processes(const char *filepath);
string trim_string(string s);
string trailing_trim(string s);
string leading_trim(string s);
void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
void sgn_err_func(const char *msg);
struct termios original_termios;

int main()
{
    Signal(SIGINT, sigint_handler);   // used to implement Ctrl-C
    Signal(SIGTSTP, sigtstp_handler); // used to implement Ctrl-Z
    // Signal(SIGCHLD, sigchld_handler);

    string exi = "exit";
    int status = 0;

    // loadHistory();
    while (true)
    {
        string cmd;
        bool flag_prog_background = false; // flag for background running

        pathlen = print_prompt();

        char ch, prevc = '\0';
        int cmdline_cnt = 0, cursor = 0;
        int num_matches = -1;

        shell_history shellHistory;

        char **history = shellHistory.commands;
        int hist_cur = max(0, shellHistory.index);

        while (1)
        {
            ch = getch();

            if (ch == KEY_CTRL_A)
            {
                cursor = 0;
                cout << "\033[" << pathlen + 3 << "G";
            }
            else if (ch == KEY_CTRL_E)
            {
                cout << "\033[" << pathlen + 3 << "G";
                cout << "\033[" << cmdline_cnt << "C";
                cursor = cmdline_cnt;
            }
            else if (ch == KEY_ERASE || ch == KEY_BACKSPACE)
            {
                if (cursor <= 0)
                {
                    continue;
                }
                putchar('\b');
                putchar(' ');
                putchar('\b');
                cmdline_cnt--;
                cursor--;
                if (cursor == cmdline_cnt)
                {
                    cmd.pop_back();
                    continue;
                }

                for (int i = cursor; i < cmdline_cnt; i++)
                {
                    cmd[i] = cmd[i + 1];
                }

                // Print the updated string or vector of characters
                cout << "\033[" << pathlen + 3 << "G";
                for (int i = 0; i < cmdline_cnt; i++)
                {
                    putchar(cmd[i]);
                }
                putchar(' ');
                putchar('\b');
                cout << "\033[" << pathlen + 3 << "G";
                cout << "\033[" << cursor - 1 << "C";

                // putchar(' ');
                cmd.pop_back();
            }
            else if (ch == KEY_ENTER)
            {
                if (prevc == KEY_TAB && (num_matches > 1 || num_matches < 0))
                {
                    prevc = ch;
                    continue;
                }
                putchar('\n');
                cmd.push_back('\n');
                cmdline_cnt++;
                break;
            }
            else if (ch == EOT)
            {
                putchar('\n');
            }
            else if (ch == KEY_ESCAPE)
            {
                if (getch() == '[')
                {
                    ch = getch();
                    if (ch == 'C')
                    {
                        if (cursor < cmd.length())
                        {
                            cout << "\033[1C";
                            cursor++;
                        }
                    }
                    else if (ch == 'D')
                    {
                        if (cursor > 0)
                        {
                            cout << "\033[1D";
                            cursor--;
                        }
                    }
                    else if (ch == 'A')
                    { // Up arrow key
                        cout << "\033[" << pathlen + 3 << "G";
                        cout << "\033[" << cmdline_cnt + 1 << "C";
                        cursor = cmdline_cnt;
                        for (int i = 0; i < cursor + 1; i++)
                            fputs("\b \b", stdout);

                        if (hist_cur >= 0 && shellHistory.index)
                        {
                            hist_cur--;
                            if (hist_cur == -1)
                                hist_cur = 0;
                            cmd = history[hist_cur];
                            cmdline_cnt, cursor = cmd.length() + 1;
                        }
                        for (int i = 0; i < cmd.length(); i++)
                        {
                            putchar(cmd[i]);
                        }
                        cmdline_cnt = cmd.length();
                        cursor = 0;

                        cout << "\033[" << pathlen + 3 << "G";
                        cout << "\033[" << cmdline_cnt << "C";
                        cursor = cmdline_cnt;
                    }
                    else if (ch == 'B')
                    { // Down arrow key
                        cout << "\033[" << pathlen + 3 << "G";
                        cout << "\033[" << cmdline_cnt + 1 << "C";
                        cursor = cmdline_cnt;
                        for (int i = 0; i < cursor + 1; i++)
                            fputs("\b \b", stdout);

                        if (hist_cur == shellHistory.index - 1)
                        {
                            cmd = "";
                            cmdline_cnt, cursor = 1;
                            cout << "\033[" << pathlen + 3 << "G";
                            cout << "\033[" << cmdline_cnt + 1 << "C";
                            cursor = cmdline_cnt;
                            for (int i = 0; i < cursor + 1; i++)
                                fputs("\b \b", stdout);
                            continue;
                        }

                        if (hist_cur < shellHistory.index - 1)
                        {
                            hist_cur++;

                            if (hist_cur == shellHistory.index)
                                hist_cur = shellHistory.index - 1;
                            cmd = history[hist_cur];
                        }
                        for (int i = 0; i < cmd.length(); i++)
                        {
                            putchar(cmd[i]);
                        }
                        cmdline_cnt = cmd.length();
                        cursor = 0;

                        cout << "\033[" << pathlen + 3 << "G";

                        cout << "\033[" << cmdline_cnt << "C";
                        cursor = cmdline_cnt;
                    }
                }
            }
            else if (ch >= 32)
            { /* Printing Characters of the ASCII Table */
                putchar(ch);

                if (cursor != cmdline_cnt)
                {
                    cmd.insert(cursor, 1, ch);
                    cout << "\033[" << pathlen + 3 << "G";
                    for (int i = 0; i < cmdline_cnt + 1; i++)
                    {
                        putchar(cmd[i]);
                    }
                    cout << "\033[" << pathlen + 3 << "G";
                    cout << "\033[" << cursor + 1 << "C";
                }
                else
                {
                    cmd.push_back(ch);
                }
                cmdline_cnt++;
                cursor++;
            }
            prevc = ch;
        }
        prevc = ch;

        cout << cmd << endl;

        char *cmdCopy = &cmd[0];
        shellHistory.push(cmdCopy);

        shellHistory.updateFile();

        cmd = trim_string(cmd);
        // cout << cmd;

        if (cmd[0] == 'c' and cmd[1] == 'd') // Implementing cd command
        {
            if (cmd.size() == 2)
                cout << "Give an argument or use cd ....\n";

            else
            {
                cmd = cmd.substr(3);
                const char *str = cmd.c_str();
                if (chdir(str) != 0)
                {
                    printf("ERROR: DIRECTORY NOT PRESENT\n");
                }
            }
            continue;
        }

        bool flag_wildcard = false;
        const char *c = cmd.c_str();
        for (int i = 0; i < cmd.length(); i++)
        {
            if (cmd[i] == '?' || cmd[i] == '*')
            {
                flag_wildcard = true;
                break;
            }
        }

        if (cmd.compare(exi) == 0)
        {
            printf("\n\n---- Terminating shell ----\n\n");
            break;
        }
        if (cmd.back() == '&')
        {
            // printf("\n\t[ Background process with PID %d done. ]\n", pid);
            flag_prog_background = true, cmd.back() = ' ';
        }

        if (flag_wildcard)
        {
            string arg;
            vector<string> args;
            stringstream ss(cmd);
            string c = "";
            for (int i = 0; cmd[i] != ' '; i++)
                c.push_back(cmd[i]);
            while (ss >> arg)
                expandWildcards(arg, args);
            cout << "Expanded arguments:" << endl;
            string expanded_arg_concatenated = "";
            if (cmd.find("sort") != string::npos)
            {
                expanded_arg_concatenated += "sort ";
                for (const auto &arg : args)
                {
                    expanded_arg_concatenated += arg + " ";
                }

                cout << expanded_arg_concatenated << endl;
                // cmd = "";
                cmd = expanded_arg_concatenated;
            }
            if (cmd.find("gedit") != string::npos)
            {
                expanded_arg_concatenated += "gedit ";
                for (const auto &arg : args)
                {
                    expanded_arg_concatenated += arg + " ";
                }

                cout << expanded_arg_concatenated << endl;
                // cmd = "";
                cmd = expanded_arg_concatenated;
            }
        }

        vector<string> String_piped_vector = STRDivide_wrt_delim_func(cmd, '|'); // splting the command having delimiter = '|' pipes

        if (String_piped_vector.size() == 1) // if above splitting not possible -- > no '|' present
        {
            vector<string> VS = IODivide_func(String_piped_vector[0]); // Input output division --> redirection

            string first_word = VS[0].substr(0, VS[0].find(" "));
            if (first_word.compare("delep") == 0)
            {
                vector<string> args;
                for (string te : STRDivide_wrt_delim_func(VS[0], ' '))
                    if (te.size()) // Ignore whitespaces
                        args.push_back(te);

                // Create a char* array for the arguments
                char *argv[args.size() + 1];
                for (int i = 0; i < args.size(); i++)
                    argv[i] = const_cast<char *>(args[i].c_str()); // Convert string to char *
                argv[args.size()] = NULL;

                int argcs = 0;
                while (argv[argcs] != NULL)
                {
                    argcs++;
                }

                if (argcs < 2)
                {
                    cerr << "USAGE: DELEP <FILEPATH>" << endl;
                    continue;
                }

                const char *filepath = argv[1];

                int pipe_fd[2];
                pipe(pipe_fd);
                pid_t child_pid = fork();
                if (child_pid == 0)
                {
                    close(pipe_fd[0]);
                    vector<int> idlist = kill_processes(filepath);

                    int n = idlist.size();

                    write(pipe_fd[1], &n, sizeof(n));

                    for (int i = 0; i < n; i++)
                    {

                        write(pipe_fd[1], &idlist[i], sizeof(idlist[i]));
                    }

                    close(pipe_fd[1]);

                    exit(0);
                }
                else
                {
                    close(pipe_fd[1]);

                    int status;
                    waitpid(child_pid, &status, 0);

                    int n;

                    read(pipe_fd[0], &n, sizeof(n));

                    vector<int> idlist(n);

                    for (int i = 0; i < n; i++)
                    {

                        read(pipe_fd[0], &idlist[i], sizeof(idlist[i]));
                    }
                    close(pipe_fd[0]);

                    int j = 0;
                    for (auto &i : idlist)
                    {
                        pid_t pid = i;

                        cout << "File is locked by the process with pid " << pid << " write yes if u want to kill it.  ";
                        string response;
                        cin >> response;
                        if (response == "yes")
                        {
                            j++;

                            kill(pid, SIGKILL);
                            cout << "That process is killed\n";
                        }
                    }

                    if (j != 0 && n == j)
                    {
                        remove(filepath);
                        cout << "File is removed\n";
                    }
                }
            }
            else
            {
                pid_t pid = fork();
                if (pid == 0) // child
                {
                    IORedirection_func(VS[1], VS[2]);
                    Func_Executing_Command(VS[0]); // command in execution
                    exit(0);                       // child exit
                }

                if (!flag_prog_background)
                {
                    waitpid(pid, &status, 0);
                } // wait for child if not background
                else
                {
                    printf("\n\t[ Background process with PID %d done. ]\n", pid);
                }
            }
        }

        else
        {
            int n = String_piped_vector.size(); // how many commands in pipes
            int file_descriptor1[2], file_descriptor2[2];

            for (int i = 0; i < n; i++)
            {
                vector<string> VS = IODivide_func(String_piped_vector[i]);
                if (i != n - 1)
                    pipe(file_descriptor2);

                pid_t pid = fork();

                // for each n commands fork()
                if (pid == 0)
                {
                    if ((i == 0) || (i == n - 1))
                        IORedirection_func(VS[1], VS[2]);

                    if (i > 0)
                        dup2(file_descriptor1[0], 0), close(file_descriptor1[0]), close(file_descriptor1[1]); // read all but first

                    if (i != n - 1)
                        close(file_descriptor2[0]), dup2(file_descriptor2[1], 1), close(file_descriptor2[1]); // write all but last

                    Func_Executing_Command(VS[0]); // command execution
                }

                if (i) // parent
                    close(file_descriptor1[0]), close(file_descriptor1[1]);

                if (i != n - 1)
                    file_descriptor1[0] = file_descriptor2[0], file_descriptor1[1] = file_descriptor2[1]; /// copy fd not last
                int status;
                waitpid(pid, &status, 0);
            }

            if (!flag_prog_background)
            {
                while (wait(&status) > 0)
                    ;
            }
            // all child return wait for it except background
        }
    }
    return 0;
}

char getch(void)
{
    int c = 0;

    tcgetattr(0, &oldterm);
    memcpy(&newterm, &oldterm, sizeof(newterm));
    newterm.c_lflag &= ~(ICANON | ECHO);
    newterm.c_cc[VMIN] = 1;
    newterm.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &newterm);
    c = getchar();
    tcsetattr(0, TCSANOW, &oldterm);
    return c;
}

int print_prompt()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf(ANSI_COLOR_GREEN "%s> " ANSI_COLOR_RESET, cwd);
    string s(cwd);
    return s.length();
}
int getpathlen()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    string s(cwd);
    return s.length();
}

void sgn_err_func(const char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void sigint_handler(int sig)
{
    cout << "\n";
    if (handler)
        printf("SIGINT CAUGHT BY SIGNAL HANDLER\n"); // SIGINT send when CTRLC pressed, Signal is catched
}

void sigtstp_handler(int sig)
{
    cout << "\n";
    if (handler)
        printf("SIGTSTP CAUGHT BY SIGNAL HANDLER\n"); // SIGSTP sent when CTRLZ pressed, Catched signal

    pathlen = print_prompt();
    fflush(stdout);
}

void sigchld_handler(int sig)
{
    pid_t pid;
    int status;
    int f = 0;

    if (handler)
        printf("SIGCHILD HANLDER \n"); // SIGCHLD sent when child process stops/ends if it gets a CTRLZ signal

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) // rid of zombie process
    {
        if (pids.find(pid) != pids.end())
        {
            pids.erase(pid);
            if (flag == 0)
                running = 0;
        }
        if (pids.size() == 0)
        {
            running = 0;
            printf("\n\t[ Background process with PID %d done. ]\n", pid);
            f = 1;
        }
        if (handler)
            printf("SIGNAL HANDLER SIGCHILD DELETES %d\n", pid);
    }
    if (!((pid == 0) || (pid == -1 && errno == ECHILD))) // !(no child remain is zombie  or no child left to exist )/
        sgn_err_func("ERROR WAIT SIGCHLD HANDLER");

    if (handler)
        printf("EXITITNG SIGCHLD HANDLER");

    if (f)
    {
        pathlen = print_prompt();
        fflush(stdout);
    }
    return;
}

void expandWildcards(const std::string &arg, std::vector<std::string> &args)
{
    glob_t globBuffer;
    memset(&globBuffer, 0, sizeof(globBuffer));
    int globResult = glob(arg.c_str(), GLOB_TILDE, NULL, &globBuffer);
    if (globResult == 0)
    {
        for (size_t i = 0; i < globBuffer.gl_pathc; ++i)
            args.push_back(globBuffer.gl_pathv[i]);
    }
    globfree(&globBuffer);
}
vector<int> kill_processes(const char *filepath)
{
    string cmd = "lsof " + string(filepath) + " | awk '{if ($5 == \"REG\") print $2}'";
    FILE *lsof = popen(cmd.c_str(), "r");
    if (!lsof)
    {
        cerr << "Error running lsof command" << endl;
    }

    char buf[1024];
    vector<int> list;
    while (fgets(buf, 1024, lsof))
    {
        pid_t pid = atoi(buf);
        list.push_back(pid);
    }
    pclose(lsof);
    return list;
}

handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        sgn_err_func("Signal error");
    return (old_action.sa_handler);
}

string leading_trim(string s)
{
    string lt;
    for (int i = 0; i < s.size(); i++)
        if (!isspace(s[i]))
            return s.substr(i);
    return lt;
}

string trailing_trim(string s)
{
    string tt;
    int n = s.size();
    for (int i = 0; i < n; i++)
        if (!isspace(s[n - 1 - i]))
            return s.substr(0, n - i);
    return tt;
}

string trim_string(string s)
{
    return trailing_trim(leading_trim(s));
}

vector<string> STRDivide_wrt_delim_func(string inputted_command, char delim) // divide string with delimiters
{
    vector<string> vector_result;
    stringstream ss(inputted_command);
    string temp;

    // vector of string
    while (getline(ss, temp, delim))
        vector_result.push_back(temp);

    return vector_result;
}

vector<string> IODivide_func(string inputted_command)
{
    vector<string> vector_result(3);
    vector<string> output = STRDivide_wrt_delim_func(inputted_command, '>'); // '>' indicate output redirection

    if (output.size() == 1)
    {
        vector<string> input = STRDivide_wrt_delim_func(inputted_command, '<'); // '<' indicate input redirection

        if (input.size() == 1)
        {
            vector_result[0] = trim_string(output[0]); // when splitting of input and output results in a single string
            return vector_result;
        }

        else
        {
            vector_result[1] = trim_string(input[1]); // when input.size() > 1 and output.size() == 1
            vector_result[0] = trim_string(input[0]);
            return vector_result;
        }
    }

    vector<string> input = STRDivide_wrt_delim_func(inputted_command, '<');

    if (input.size() == 1)
    {
        // no '<'
        vector_result[2] = trim_string(output[1]);
        vector_result[0] = trim_string(output[0]);
        return vector_result;
    }

    if (STRDivide_wrt_delim_func(output[0], '<').size() == 1) // Input, output redirection
    {
        // Split the second argument into output and input
        vector<string> output_input = STRDivide_wrt_delim_func(output[1], '<');

        vector_result[2] = trim_string(output_input[0]), vector_result[1] = trim_string(output_input[1]);
        vector_result[0] = trim_string(output[0]);
        return vector_result;
    }

    vector<string> inputted_command_input = STRDivide_wrt_delim_func(output[0], '<'); // Split the first argument into commmand and input

    // Trim the whitespaces in outgoing_output_str and in
    vector_result[1] = trim_string(inputted_command_input[1]), vector_result[2] = trim_string(output[1]);
    vector_result[0] = trim_string(inputted_command_input[0]);
    return vector_result;
}

void IORedirection_func(string incoming_input_str, string outgoing_output_str)
{
    // input and output files are argument
    int fdinput, fdoutput;

    if (incoming_input_str.size())
    {
        fdinput = open(incoming_input_str.c_str(), O_RDONLY); // Open input changing file in read only permission
        if (fdinput < 0)
        {
            cout << "ERROR IN OPENING FILE:  " << incoming_input_str << endl;
            exit(EXIT_FAILURE);
        }

        if (dup2(fdinput, 0) < 0) // input redirect
        {
            cout << "ERROR IN REDG INPUT" << endl;
            exit(EXIT_FAILURE);
        }
    }

    if (outgoing_output_str.size())
    {
        fdoutput = open(outgoing_output_str.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU); // Open output changing file
        if (dup2(fdoutput, 1) < 0)                                                           // output redirect
        {
            cout << "ERROR IN REDIRECTING OUTPUT" << endl;
            exit(EXIT_FAILURE);
        }
    }
}

vector<int> par_proc(int pid)
{
    vector<int> parProc;

    // Use the "ps" command to get the parent process ID
    string cmd = "ps -o ppid= -p " + to_string(pid);
    FILE *fp = popen(cmd.c_str(), "r");
    if (fp == nullptr)
    {
        cerr << "Error: Failed to run the ps command" << endl;
        return parProc;
    }

    int ppid;
    if (fscanf(fp, "%d", &ppid) == 1)
    {
        parProc.push_back(ppid);
        // Recursively get the parent process IDs
        if (ppid != 0)
        {
            vector<int> grandparProc = par_proc(ppid);
            parProc.insert(parProc.end(), grandparProc.begin(), grandparProc.end());
        }
    }
    pclose(fp);

    return parProc;
}

void all_proc(int pid, bool s_flag)
{
    cout << "Process ID: " << pid << endl;
    vector<int> parProc = par_proc(pid);
    if (parProc.empty())
    {
        cout << "No parent process found" << endl;
    }
    else
    {
        for (int ppid : parProc)
        {
            cout << "Parent Process ID: " << ppid << endl;
        }
    }

    if (s_flag)
    {
        parProc.insert(parProc.begin(), pid);

        for (int pid : parProc)
        {
            if (pid == 0)
                break;

            string s = "/proc/" + to_string(pid) + "/task/" + to_string(pid) + "/children";
            ifstream file(s);
            if (!file)
            {
                cout << "File not found!" << endl;
                return;
            }
            string word;
            int children = 0;
            while (file >> word)
            {
                children++;
            }
            file.close();
            cout << "Number of children for process with pid  " << pid << ": " << children << endl;
            if (children >= 10)
            {
                cout << "MALWARE PROCESS MIGHT BE " << pid << endl;
                break;
            } else cout << "No malware found as of now\n";
        }
    }
}

void Func_Executing_Command(string inputted_command)
{
    // divide comm and arg
    vector<string> args;
    for (string str : STRDivide_wrt_delim_func(inputted_command, ' '))
        if (str.size())
            args.push_back(str);

    // arguments argv
    char *argv[args.size() + 1];
    for (int i = 0; i < args.size(); i++)
        argv[i] = const_cast<char *>(args[i].c_str()); // string -- > char *
    argv[args.size()] = NULL;                          // end by NULL

    if (strcmp(argv[0], "sb") == 0)
    {
        int argc1 = 0;
        while (argv[argc1] != NULL)
        {
            argc1++;
        }

        if (argc1 < 2)
        {
            cerr << "Error: Missing process ID argument" << endl; // Input should be as sb 'pid' or sb 'pid' -suggest
            exit(1);
        }

        int pid = stoi(argv[1]);
        bool flag = false;
        if (argc1 > 2 && string(argv[2]) == "-suggest")
        {
            flag = true;
        }

        all_proc(pid, flag);
        exit(0);
    }

    char *const *argv1 = argv;
    if (execvp(args[0].c_str(), argv1) == -1)
        perror("execvp");
    exit(1);
}