#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_PROCESSES 100

// Màu sắc cho console
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// Thông tin tiến trình con
typedef struct {
    HANDLE hProcess;   // Handle tiến trình
    HANDLE hThread;    // Handle luồng chính (Cần thiết để Pause/Resume)
    DWORD pid;         // Process ID
    char name[256];    // Tên lệnh
    char status[20];   // Trạng thái: Running, Stopped, Terminated
    int active;        // Đánh dấu slot này có đang dùng không
} Process;

Process process_list[MAX_PROCESSES];
int process_count = 0;

// Lấy đường dẫn hiện tại để in ra prompt ngầu hơn
void display_prompt() {
    char cwd[1024];
    if (GetCurrentDirectory(sizeof(cwd), cwd)) {
        printf(COLOR_CYAN "%s" COLOR_RESET " > ", cwd);
    } else {
        printf(COLOR_CYAN "myShell" COLOR_RESET " > ");
    }
}

// Cập nhật danh sách: Kiểm tra xem tiến trình nào đã tự động kết thúc
void refresh_process_list() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_list[i].active) {
            DWORD exitCode;
            if (GetExitCodeProcess(process_list[i].hProcess, &exitCode)) {
                if (exitCode != STILL_ACTIVE && strcmp(process_list[i].status, "Stopped") != 0) {
                    // Tiến trình đã chết tự nhiên
                    process_list[i].active = 0;
                    CloseHandle(process_list[i].hProcess);
                    CloseHandle(process_list[i].hThread);
                }
            }
        }
    }
}

// Phân tích lệnh (Hỗ trợ dấu ngoặc kép cho đường dẫn có khoảng trắng)
int parse_command(char *input, char **args) {
    int argc = 0;
    int in_quotes = 0;
    char *start = input;
    
    for (char *p = input; *p; p++) {
        if (*p == '"') {
            in_quotes = !in_quotes;
            if (!in_quotes) { *p = '\0'; args[argc++] = start + 1; start = p + 1; }
            else { start = p; }
        } else if (*p == ' ' && !in_quotes) {
            *p = '\0';
            if (p > start) args[argc++] = start;
            start = p + 1;
        }
    }
    if (*start) args[argc++] = start;
    args[argc] = NULL;
    return argc;
}

int is_background(char **args, int *argc) {
    if (*argc > 0 && strcmp(args[*argc - 1], "&") == 0) {
        args[*argc - 1] = NULL;
        (*argc)--;
        return 1;
    }
    return 0;
}

// --- CÁC LỆNH NỘI BỘ (BUILT-IN) ---

void cmd_cd(char **args) {
    if (args[1] == NULL) {
        char cwd[1024];
        GetCurrentDirectory(sizeof(cwd), cwd);
        printf("Current Directory: %s\n", cwd);
    } else {
        if (!SetCurrentDirectory(args[1])) {
            printf(COLOR_RED "Error: Cannot find path '%s'\n" COLOR_RESET, args[1]);
        }
    }
}

void cmd_list() {
    refresh_process_list();
    printf(COLOR_BOLD "\n%-10s %-20s %-15s\n" COLOR_RESET, "PID", "Name", "Status");
    printf("--------------------------------------------------\n");
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_list[i].active) {
            char *color = COLOR_GREEN;
            if (strcmp(process_list[i].status, "Stopped") == 0) color = COLOR_YELLOW;
            
            printf("%-10lu %-20s %s%s" COLOR_RESET "\n", 
                   process_list[i].pid, process_list[i].name, color, process_list[i].status);
            count++;
        }
    }
    if (count == 0) printf("No background processes running.\n");
    printf("\n");
}

void cmd_kill(char **args) {
    if (args[1] == NULL) { printf("Usage: kill <pid>\n"); return; }
    DWORD pid = atoi(args[1]);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_list[i].active && process_list[i].pid == pid) {
            if (TerminateProcess(process_list[i].hProcess, 1)) {
                printf(COLOR_RED "Process %lu terminated.\n" COLOR_RESET, pid);
                process_list[i].active = 0;
                CloseHandle(process_list[i].hProcess);
                CloseHandle(process_list[i].hThread);
            } else {
                printf(COLOR_RED "Failed to kill process.\n" COLOR_RESET);
            }
            return;
        }
    }
    printf("Process %lu not found.\n", pid);
}

void cmd_stop(char **args) {
    if (args[1] == NULL) { printf("Usage: stop <pid>\n"); return; }
    DWORD pid = atoi(args[1]);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_list[i].active && process_list[i].pid == pid) {
            // SỬA QUAN TRỌNG: SuspendThread dùng hThread, không phải hProcess
            if (SuspendThread(process_list[i].hThread) != (DWORD)-1) {
                strcpy(process_list[i].status, "Stopped");
                printf(COLOR_YELLOW "Process %lu paused.\n" COLOR_RESET, pid);
            } else {
                printf("Error suspending process.\n");
            }
            return;
        }
    }
    printf("Process %lu not found.\n", pid);
}

void cmd_resume(char **args) {
    if (args[1] == NULL) { printf("Usage: resume <pid>\n"); return; }
    DWORD pid = atoi(args[1]);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_list[i].active && process_list[i].pid == pid) {
            if (ResumeThread(process_list[i].hThread) != (DWORD)-1) {
                strcpy(process_list[i].status, "Running");
                printf(COLOR_GREEN "Process %lu resumed.\n" COLOR_RESET, pid);
            } else {
                printf("Error resuming process.\n");
            }
            return;
        }
    }
    printf("Process %lu not found.\n", pid);
}

void cmd_help() {
    printf(COLOR_BOLD "\n--- HELP MENU ---\n" COLOR_RESET);
    printf("1. command [args] [&] : Run program (add & for background)\n");
    printf("2. cd <path>          : Change directory\n");
    printf("3. list               : List background processes\n");
    printf("4. kill <pid>         : Kill a process\n");
    printf("5. stop <pid>         : Pause a process\n");
    printf("6. resume <pid>       : Resume a process\n");
    printf("7. cls                : Clear screen\n");
    printf("8. exit               : Quit shell\n\n");
}

// --- THỰC THI CHƯƠNG TRÌNH NGOÀI ---

void execute_external(char **args, int background) {
    char command_line[1024] = "";
    
    // Nối lại lệnh để chạy
    for (int i = 0; args[i] != NULL; i++) {
        strcat(command_line, "\""); 
        strcat(command_line, args[i]);
        strcat(command_line, "\" ");
    }

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    // Fix lỗi CMD cho file .bat hoặc lệnh nội bộ cmd
    char final_cmd[1024];
    if (strstr(args[0], ".bat") != NULL) {
        snprintf(final_cmd, sizeof(final_cmd), "cmd.exe /c %s", command_line);
    } else {
        strcpy(final_cmd, command_line);
    }

    if (!CreateProcessA(NULL, final_cmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        printf(COLOR_RED "Command not found or failed to execute.\n" COLOR_RESET);
        return;
    }

    if (background) {
        // Tìm slot trống để lưu process
        int slot = -1;
        for(int i=0; i<MAX_PROCESSES; i++) {
            if(!process_list[i].active) {
                slot = i;
                break;
            }
        }

        if (slot != -1) {
            process_list[slot].hProcess = pi.hProcess;
            process_list[slot].hThread = pi.hThread; // LƯU HANDLE THREAD
            process_list[slot].pid = pi.dwProcessId;
            strcpy(process_list[slot].name, args[0]);
            strcpy(process_list[slot].status, "Running");
            process_list[slot].active = 1;
            printf(COLOR_GREEN "[+] Background process started. PID: %lu\n" COLOR_RESET, pi.dwProcessId);
        } else {
            printf(COLOR_RED "Process list full!\n" COLOR_RESET);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    } else {
        // Chế độ foreground: chờ nó chạy xong
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

int main() {
    // Kích hoạt chế độ ANSI color trên Windows 10/11 (nếu cần)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(hOut, dwMode);

    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    int background;

    // Xóa rác trong struct
    memset(process_list, 0, sizeof(process_list));

    printf(COLOR_BOLD "WELCOME TO MY SHELL v2.0\n" COLOR_RESET);
    cmd_help();

    while (1) {
        display_prompt();
        if (fgets(input, MAX_INPUT, stdin) == NULL) break;

        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;

        int argc = parse_command(input, args);
        if (argc == 0) continue;

        background = is_background(args, &argc);

        // Xử lý lệnh
        if (strcmp(args[0], "exit") == 0) {
            printf("Goodbye!\n");
            break;
        } else if (strcmp(args[0], "help") == 0) {
            cmd_help();
        } else if (strcmp(args[0], "cls") == 0) {
            system("cls");
        } else if (strcmp(args[0], "cd") == 0) {
            cmd_cd(args);
        } else if (strcmp(args[0], "list") == 0) {
            cmd_list();
        } else if (strcmp(args[0], "kill") == 0) {
            cmd_kill(args);
        } else if (strcmp(args[0], "stop") == 0) {
            cmd_stop(args);
        } else if (strcmp(args[0], "resume") == 0) {
            cmd_resume(args);
        } else {
            execute_external(args, background);
        }
    }
    return 0;
}