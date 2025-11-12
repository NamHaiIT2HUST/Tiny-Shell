#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_INPUT 256
#define MAX_ARGS 10

//Thông tin tiến trình con
typedef struct {
    HANDLE hProcess;   //tham chiếu đến tiến trình con
    DWORD pid;         //Số hiệu của tiến trình
    char name[MAX_INPUT];             //tên lệnh or chương trình thực thi
    char status[20];                    //Lưu trạng thái tiến trình
} Process;

Process process_list[100]; // Danh sách tiến trình con
int process_count = 0;        //Đếm số chương trình con trong list

void display_prompt() {
    printf("myShell> ");
}

//Phân tích lệnh
int parse_command(char *input, char **args) {
    int argc = 0;
    char *token = strtok(input, " \n");
    while (token != NULL && argc < MAX_ARGS) {
        args[argc++] = token;
        token = strtok(NULL, " \n");
    }
    args[argc] = NULL; //Phần tử cuối là NULL vì CreateProcessA yêu cầu kết thúc là NULL
    return argc;
}

// Hàm kiểm tra xem có chạy background không (dấu &)
int is_background(char **args, int argc) {
    if (argc > 0 && strcmp(args[argc - 1], "&") == 0) {
        args[argc - 1] = NULL; // Xóa dấu & khỏi danh sách tham số
        return 1;
    }
    return 0;
}

//Thực hiện lệnh mà người dùng yêu cầu
void execute_command(char **args, int background) {
    //Xử lí các lệnh quản lí shell
    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting myShell...\n");
        exit(0);
    } else if (strcmp(args[0], "list") == 0) {
        printf("List of background processes:\n");
        for (int i = 0; i < process_count; i++) {
            printf("PID: %lu, Name: %s, Status: %s\n", process_list[i].pid, process_list[i].name, process_list[i].status);
        }
        return;
    } else if (strcmp(args[0], "kill") == 0) {
        if (args[1] == NULL) {
            printf("Usage: kill <pid>\n");
            return;
        }
        DWORD pid = atoi(args[1]);
        for (int i = 0; i < process_count; i++) {
            if (process_list[i].pid == pid) {
                TerminateProcess(process_list[i].hProcess, 1);
                strcpy(process_list[i].status, "Terminated");
                printf("Process %lu terminated\n", pid);
                return;
            }
        }
        printf("Process %lu not found\n", pid);
        return;
    } else if (strcmp(args[0], "stop") == 0) {
        if (args[1] == NULL) {
            printf("Usage: stop <pid>\n");
            return;
        }
        DWORD pid = atoi(args[1]);
        for (int i = 0; i < process_count; i++) {
            if (process_list[i].pid == pid) {
                SuspendThread(process_list[i].hProcess);
                strcpy(process_list[i].status, "Stopped");
                printf("Process %lu stopped\n", pid);
                return;
            }
        }
        printf("Process %lu not found\n", pid);
        return;
    } else if (strcmp(args[0], "resume") == 0) {
        if (args[1] == NULL) {
            printf("Usage: resume <pid>\n");
            return;
        }
        DWORD pid = atoi(args[1]);
        for (int i = 0; i < process_count; i++) {
            if (process_list[i].pid == pid) {
                ResumeThread(process_list[i].hProcess);
                strcpy(process_list[i].status, "Running");
                printf("Process %lu resumed\n", pid);
                return;
            }
        }
        printf("Process %lu not found\n", pid);
        return;
    }

//tạo lệnh
    char command_line[1024] = "";
    for (int i = 0; args[i] != NULL; i++) {
        strcat(command_line, args[i]);
        strcat(command_line, " ");
    }

//thành phần chạy tiến trình con
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

//Check file .bat
    if (strstr(args[0], ".bat") != NULL) {
        char bat_command[1024];
        snprintf(bat_command, sizeof(bat_command), "cmd.exe /c %s", command_line);
        strcpy(command_line, bat_command);
    }

//Tạo tiến trình con
    BOOL success = CreateProcessA(NULL, command_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (!success) {
        printf("Failed to execute command: %lu\n", GetLastError());  //In ra màn hình báo lỗi nếu tạo tiến trình thất bại
        return;
    }

//Lưu tiến trình con vào list
    if (process_count < 100) {
        process_list[process_count].hProcess = pi.hProcess;
        process_list[process_count].pid = pi.dwProcessId;
        strcpy(process_list[process_count].name, args[0]);
        strcpy(process_list[process_count].status, "Running");
        process_count++;
    }

//background không chạy thì chờ tiến trình con hoàn thành
    if (!background) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        printf("Child process %lu running in background\n", pi.dwProcessId);
    }
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    int background;

    printf("WELCOME TO MY SHELL\n");
    printf("The following commands:\n");
    printf("list: List all background processes (ID, name, status)\n");
    printf("kill <pid>: Terminate a process\n");
    printf("stop <pid>: Stop a process\n");
    printf("resume <pid>: Resume a process\n");
    printf("exit: Exit my shell program\n");

    while (1) {
        display_prompt();
        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            printf("\n");
            continue;
        }

        input[strcspn(input, "\n")] = 0;               //Xóa lý tự ở cuối

        int argc = parse_command(input, args);       //check lệnh
        if (argc == 0) continue; //Không có lệnh

        background = is_background(args, argc);     //check background

        execute_command(args, background);         //Chạy lệnh yêu cầu
    }
    return 0;
}