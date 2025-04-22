#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <ncurses.h>
#include <string.h>
#include <signal.h>


int cpu_usage = 0;
long memory_free = 0;
long memory_total = 0;
char process_list[10][256];

// Function to monitor CPU usage
void* monitor_cpu(void* arg) {
    while(1) {
        FILE *fp;
        char buffer[256];
        fp = popen("top -bn1 | grep \"Cpu(s)\" | sed \"s/.*, *\\([0-9.]*\\)%* id.*/\\1/\" | awk '{print 100 - $1}'", "r");
        if (fp == NULL) {
            perror("Error reading CPU usage");
            exit(1);
        }

        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            cpu_usage = atoi(buffer);
        }
        fclose(fp);
        usleep(1000000); // Update every second
    }
    return NULL;
}

// Function to monitor memory usage
void* monitor_memory(void* arg) {
    while(1) {
        struct sysinfo si;
        sysinfo(&si);
        memory_total = si.totalram / (1024 * 1024); // Convert to MB
        memory_free = si.freeram / (1024 * 1024); // Convert to MB
        usleep(1000000); // Update every second
    }
    return NULL;
}

// Function to list processes (simplified)
void* monitor_processes(void* arg) {
    while(1) {
        FILE *fp;
        char buffer[256];
        fp = popen("ps aux --sort=-%mem | head -n 11", "r");
        if (fp == NULL) {
            perror("Error reading process list");
            exit(1);
        }

        int i = 0;
        while (fgets(buffer, sizeof(buffer), fp) != NULL && i < 10) {
            strcpy(process_list[i], buffer);
            i++;
        }
        fclose(fp);
        usleep(1000000);
    }
    return NULL;
}

// Function to display information on the ncurses window
void display_info() {
    clear();
    mvprintw(1, 1, "Real-Time Process Monitoring Dashboard");
    mvprintw(3, 1, "CPU Usage: %d%%", cpu_usage);
    mvprintw(4, 1, "Memory: %ldMB free / %ldMB total", memory_free, memory_total);
    mvprintw(6, 1, "Top 10 Processes:");

    for (int i = 0; i < 10; i++) {
        mvprintw(7 + i, 1, "%s", process_list[i]);
    }

    mvprintw(18, 1, "Press 'q' to quit, 'k' to kill process, 'p' to suspend process");

    refresh();
}


void handle_input(char input) {
    int pid;
    char command[256];
    if (input == 'q') {
        
        endwin();
        exit(0);
    } else if (input == 'k') {
        
        mvprintw(19, 1, "Enter PID to kill: ");
        refresh();
        echo();
        char pid_input[10];
        wgetstr(stdscr, pid_input);
        pid = atoi(pid_input);
        snprintf(command, sizeof(command), "kill %d", pid);
        system(command);
        noecho();
    } else if (input == 'p') {
    
        mvprintw(19, 1, "Enter PID to suspend: ");
        refresh();
        echo();
        char pid_input[10];
        wgetstr(stdscr, pid_input);
        pid = atoi(pid_input);
        snprintf(command, sizeof(command), "kill -STOP %d", pid);
        system(command);
        noecho();
    }
}


int main() {
    
    initscr();
    noecho();
    cbreak();
    timeout(1000); 

    
    pthread_t cpu_thread, memory_thread, process_thread;
    pthread_create(&cpu_thread, NULL, monitor_cpu, NULL);
    pthread_create(&memory_thread, NULL, monitor_memory, NULL);
    pthread_create(&process_thread, NULL, monitor_processes, NULL);

    
    while (1) {
        display_info();

        int ch = getch();
        if (ch != ERR) {
            handle_input(ch); 
        }
    }

    
    pthread_join(cpu_thread, NULL);
    pthread_join(memory_thread, NULL);
    pthread_join(process_thread, NULL);

    // End ncurses
    endwin();
    return 0;
}
