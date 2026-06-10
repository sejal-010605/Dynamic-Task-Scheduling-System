#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <numeric>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
    void sleep_seconds(int s) { Sleep(s * 1000); }
#else
    #include <unistd.h>
    void sleep_seconds(int s) { sleep(s); }
#endif

using namespace std;

// OPTIMIZATION: use constexpr for compile-time constants
constexpr int AGING_THRESHOLD = 10;
constexpr int AGING_BOOST     = 1;

// Task struct
struct Task {
    string name;
    int    priority;          // 1=High, 2=Medium, 3=Low
    int    effectivePriority;
    time_t startTime;
    time_t endTime;
    time_t deadline;          // 0 = no deadline
    int    duration;
    time_t addedAt;

    int  waitingTime;
    int  turnaroundTime;
    bool deadlineMet;

    Task(const string& n, int p, time_t start, time_t end, time_t dl = 0)
        : name(n), priority(p), effectivePriority(p),
          startTime(start), endTime(end), deadline(dl),
          waitingTime(0), turnaroundTime(0), deadlineMet(true)
    {
        duration = static_cast<int>(end - start);   // OPTIMIZATION: direct subtraction
        addedAt  = time(nullptr);
    }

    void applyAging(time_t currentTime) {
        double waited     = difftime(currentTime, addedAt);
        int    boosts     = static_cast<int>(waited / AGING_THRESHOLD);
        effectivePriority = max(1, priority - boosts * AGING_BOOST);
    }
};

struct Metrics {
    string algoName;
    double avgWaitingTime;
    double avgTurnaroundTime;
    double throughput;
    int    deadlinesMissed;
    int    totalTasks;
    vector<string> executionOrder;
};

// OPTIMIZATION: return string directly, no need to copy via char buffer each time
string formatTime(time_t t) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return string(buf);
}

// OPTIMIZATION: use static constexpr array for priority labels (fast lookup)
constexpr const char* priorityLabel(int p) {
    switch (p) {
        case 1: return "High";
        case 2: return "Medium";
        case 3: return "Low";
        default: return "?";
    }
}

void printLine(char c = '-', int len = 65) {
    cout << string(len, c) << '\n';
}

// OPTIMIZATION: early length check to avoid constructing istringstream for invalid strings
bool stringToTime(const string& s, time_t& result) {
    if (s.length() != 19) return false;
    struct tm tm = {};
    istringstream ss(s);
    ss >> get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) return false;
    result = mktime(&tm);
    return result != -1;
}

void clearCin() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// FCFS simulation - OPTIMIZATION: pass const ref, accumulate stats inside loop
Metrics simulateFCFS(const vector<Task>& tasks) {
    if (tasks.empty()) return {"FCFS", 0, 0, 0, 0, 0, {}};
    
    vector<Task> taskCopy = tasks;   // need mutable copy for sorting
    sort(taskCopy.begin(), taskCopy.end(),
         [](const Task& a, const Task& b) { return a.startTime < b.startTime; });

    time_t current = taskCopy[0].startTime;
    int    deadlinesMissed = 0;
    double totalWT = 0, totalTAT = 0;
    vector<string> order;
    order.reserve(taskCopy.size());    // OPTIMIZATION: preallocate

    for (auto& t : taskCopy) {
        if (current < t.startTime) current = t.startTime;
        time_t execEnd = current + t.duration;
        t.waitingTime    = static_cast<int>(current - t.startTime);
        t.turnaroundTime = static_cast<int>(execEnd - t.startTime);
        totalWT += t.waitingTime;
        totalTAT += t.turnaroundTime;
        if (t.deadline > 0 && execEnd > t.deadline) deadlinesMissed++;
        order.push_back(t.name);
        current = execEnd;
    }

    int n = static_cast<int>(taskCopy.size());
    int makespan = static_cast<int>(current - taskCopy[0].startTime);
    double throughput = (makespan > 0) ? static_cast<double>(n) / makespan : 0.0;

    return {"FCFS", totalWT / n, totalTAT / n, throughput, deadlinesMissed, n, order};
}

// Priority simulation - OPTIMIZATION: similar improvements
Metrics simulatePriority(const vector<Task>& tasks) {
    if (tasks.empty()) return {"Priority", 0, 0, 0, 0, 0, {}};
    
    vector<Task> taskCopy = tasks;
    sort(taskCopy.begin(), taskCopy.end(),
         [](const Task& a, const Task& b) {
             if (a.priority != b.priority) return a.priority < b.priority;
             return a.startTime < b.startTime;
         });

    time_t current = taskCopy[0].startTime;
    int    deadlinesMissed = 0;
    double totalWT = 0, totalTAT = 0;
    vector<string> order;
    order.reserve(taskCopy.size());

    for (auto& t : taskCopy) {
        if (current < t.startTime) current = t.startTime;
        time_t execEnd = current + t.duration;
        t.waitingTime    = static_cast<int>(current - t.startTime);
        t.turnaroundTime = static_cast<int>(execEnd - t.startTime);
        totalWT += t.waitingTime;
        totalTAT += t.turnaroundTime;
        if (t.deadline > 0 && execEnd > t.deadline) deadlinesMissed++;
        order.push_back(t.name);
        current = execEnd;
    }

    int n = static_cast<int>(taskCopy.size());
    int makespan = static_cast<int>(current - taskCopy[0].startTime);
    double throughput = (makespan > 0) ? static_cast<double>(n) / makespan : 0.0;

    return {"Priority", totalWT / n, totalTAT / n, throughput, deadlinesMissed, n, order};
}

// Comparison function (unchanged logic, but uses optimized simulation functions)
void runComparison(const vector<Task>& taskList) {
    if (taskList.empty()) {
        cout << "[!] No tasks to compare.\n";
        return;
    }

    Metrics fcfs = simulateFCFS(taskList);
    Metrics pri  = simulatePriority(taskList);

    vector<Metrics> results = {fcfs, pri};

    // Execution Order output (unchanged)
    printLine('=');
    cout << "  EXECUTION ORDER\n";
    printLine('=');

    cout << "\n  FCFS (by arrival time):\n";
    for (size_t i = 0; i < fcfs.executionOrder.size(); i++)
        cout << "    #" << (i+1) << "  " << fcfs.executionOrder[i] << "\n";

    cout << "\n  Priority Scheduling (High -> Medium -> Low):\n";
    for (size_t i = 0; i < pri.executionOrder.size(); i++)
        cout << "    #" << (i+1) << "  " << pri.executionOrder[i] << "\n";

    cout << "\n";
    printLine('=');
    cout << "  ALGORITHM COMPARISON\n";
    printLine('=');

    cout << "\n" << left
         << setw(22) << "Algorithm"
         << setw(18) << "Avg Wait (s)"
         << setw(22) << "Avg Turnaround (s)"
         << setw(16) << "Throughput"
         << "Deadlines Missed\n";
    printLine();

    for (const auto& m : results) {
        cout << setw(22) << m.algoName
             << setw(18) << fixed << setprecision(2) << m.avgWaitingTime
             << setw(22) << m.avgTurnaroundTime
             << setw(16) << setprecision(4) << m.throughput
             << m.deadlinesMissed << "/" << m.totalTasks << "\n";
    }

    auto best = min_element(results.begin(), results.end(),
        [](const Metrics& a, const Metrics& b) {
            return a.avgWaitingTime < b.avgWaitingTime;
        });

    printLine();
    cout << "\n  Best Algorithm for this task set: " << best->algoName << "\n";
    cout << "    Avg Waiting Time : " << fixed << setprecision(2) << best->avgWaitingTime << "s\n";
    cout << "    Avg Turnaround   : " << best->avgTurnaroundTime << "s\n";
    cout << "    Deadlines Missed : " << best->deadlinesMissed << "/" << best->totalTasks << "\n";

    cout << "\n  Analysis:\n";
    double improve = ((fcfs.avgWaitingTime - pri.avgWaitingTime) / max(fcfs.avgWaitingTime, 1.0)) * 100;

    if (pri.avgWaitingTime < fcfs.avgWaitingTime)
        cout << "  - Priority Scheduling reduced avg wait by "
             << fixed << setprecision(1) << improve << "% vs FCFS\n"
             << "    by running High priority tasks before lower ones.\n";
    else if (fcfs.avgWaitingTime < pri.avgWaitingTime)
        cout << "  - FCFS performed better because tasks arrived in a\n"
             << "    natural order — no reordering benefit was gained.\n";
    else
        cout << "  - Both algorithms performed equally for this task set.\n";

    if (best->deadlinesMissed == 0)
        cout << "  - All deadlines met under " << best->algoName << ".\n";
    else
        cout << "  - " << best->deadlinesMissed
             << " deadline(s) missed even under the best algorithm.\n";

    printLine('=');
}

// TaskScheduler class - OPTIMIZATION: conflict check uses direct time_t comparison
class TaskScheduler {
private:
    vector<Task> taskList;

    // OPTIMIZATION: simpler overlap check without difftime
    bool hasConflict(const Task& a, const Task& b) const {
        return !(a.endTime <= b.startTime || b.endTime <= a.startTime);
    }

public:
    void addTask(const string& name, int priority,
                 time_t start, time_t end, time_t deadline = 0) {
        for (const auto& t : taskList) {
            if (t.name == name) {
                cout << "[!] Task '" << name << "' already exists.\n";
                return;
            }
        }
        Task nt(name, priority, start, end, deadline);
        for (const auto& t : taskList) {
            if (hasConflict(nt, t))
                cout << "[!] Warning: '" << name
                     << "' overlaps with '" << t.name << "'.\n";
        }
        taskList.push_back(std::move(nt));   // OPTIMIZATION: move instead of copy
        cout << "[+] Added: " << name
             << " | " << priorityLabel(priority)
             << " | " << (end - start) << "s\n";
    }

    void deleteTask(const string& name) {
        auto it = remove_if(taskList.begin(), taskList.end(),
                            [&](const Task& t){ return t.name == name; });
        if (it != taskList.end()) {
            taskList.erase(it, taskList.end());
            cout << "[-] Deleted: " << name << "\n";
        } else {
            cout << "[!] Not found: " << name << "\n";
        }
    }

    void updateTask(const string& name, int newPri,
                    time_t newStart, time_t newEnd, time_t newDl) {
        for (auto& t : taskList) {
            if (t.name == name) {
                t.priority         = newPri;
                t.effectivePriority = newPri;
                t.startTime        = newStart;
                t.endTime          = newEnd;
                t.duration         = static_cast<int>(newEnd - newStart);
                t.deadline         = newDl;
                t.addedAt          = time(nullptr);
                cout << "[~] Updated: " << name << "\n";
                return;
            }
        }
        cout << "[!] Not found: " << name << "\n";
    }

    void listTasks() const {
        if (taskList.empty()) { cout << "[i] No tasks.\n"; return; }

        vector<Task> sorted = taskList;
        sort(sorted.begin(), sorted.end(), [](const Task& a, const Task& b){
            if (a.priority != b.priority) return a.priority < b.priority;
            return a.startTime < b.startTime;
        });

        printLine('=');
        cout << "  Tasks (" << sorted.size() << ")\n";
        printLine('=');
        for (size_t i = 0; i < sorted.size(); i++) {
            const Task& t = sorted[i];
            cout << "  [" << i+1 << "] " << t.name
                 << " | " << priorityLabel(t.priority)
                 << " | " << formatTime(t.startTime)
                 << " -> " << formatTime(t.endTime)
                 << " (" << t.duration << "s)";
            if (t.deadline > 0)
                cout << " | Deadline: " << formatTime(t.deadline);
            cout << "\n";
        }
        printLine('=');
    }

    void runScheduler() {
        if (taskList.empty()) { cout << "[i] No tasks.\n"; return; }

        time_t now = time(nullptr);
        vector<Task> tasks = taskList;
        for (auto& t : tasks) t.applyAging(now);

        sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b){
            if (a.effectivePriority != b.effectivePriority)
                return a.effectivePriority < b.effectivePriority;
            return a.startTime < b.startTime;
        });

        printLine('=');
        cout << "  Running: Priority Scheduling (real-time)\n";
        printLine('=');

        int deadlinesMissed = 0;

        for (auto& t : tasks) {
            now = time(nullptr);
            if (now < t.startTime) {
                cout << "[*] Waiting " << static_cast<int>(t.startTime - now)
                     << "s for '" << t.name << "'...\n";
                sleep_seconds(static_cast<int>(t.startTime - now));
            }
            time_t execStart = time(nullptr);
            cout << "\n[>] Running: " << t.name
                 << " | " << priorityLabel(t.effectivePriority)
                 << " | Start: " << formatTime(execStart) << "\n";
            sleep_seconds(t.duration);
            time_t execEnd = time(nullptr);

            if (t.deadline > 0 && execEnd > t.deadline) {
                deadlinesMissed++;
                cout << "[x] DEADLINE MISSED: " << t.name
                     << " (was " << formatTime(t.deadline) << ")\n";
            } else {
                cout << "[ok] Done: " << t.name
                     << " at " << formatTime(execEnd) << "\n";
            }
        }

        printLine('=');
        cout << "  Deadlines missed: " << deadlinesMissed
             << "/" << taskList.size() << "\n";
        printLine('=');
    }

    void compare() {
        runComparison(taskList);
    }

    const vector<Task>& getTaskList() const { return taskList; }
};

// Helper functions (unchanged except for minor efficiency)
int readPriority() {
    int p;
    cout << "    Priority (1=High, 2=Medium, 3=Low): ";
    while (!(cin >> p) || p < 1 || p > 3) {
        clearCin();
        cout << "    Enter 1, 2, or 3: ";
    }
    clearCin();
    return p;
}

bool readTimeRange(time_t& start, time_t& end) {
    string s, e;
    while (true) {
        cout << "    Start (YYYY-MM-DD HH:MM:SS): "; getline(cin, s);
        cout << "    End   (YYYY-MM-DD HH:MM:SS): "; getline(cin, e);
        if (!stringToTime(s, start)) { cout << "    [!] Bad start format.\n"; continue; }
        if (!stringToTime(e, end))   { cout << "    [!] Bad end format.\n";   continue; }
        if (end <= start) {            // OPTIMIZATION: direct comparison
            cout << "    [!] End must be after start.\n"; continue;
        }
        return true;
    }
}

int main() {
    TaskScheduler scheduler;
    string        cmd;

    printLine('=');
    cout << "  Task Scheduler — FCFS vs Priority Comparison\n";
    cout << "  4th Semester B.Tech DSA Project\n";
    printLine('=');
    cout << "  Commands: add | delete | update | list | run | compare | exit\n";
    printLine('=');
    cout << "\n";

    while (true) {
        cout << "scheduler> ";
        getline(cin, cmd);

        cmd.erase(0, cmd.find_first_not_of(" \t"));
        if (!cmd.empty())
            cmd.erase(cmd.find_last_not_of(" \t") + 1);

        if (cmd == "add") {
            int count = 0;
            cout << "How many tasks? ";
            while (!(cin >> count) || count <= 0) {
                clearCin();
                cout << "Enter valid number: ";
            }
            clearCin();

            for (int i = 0; i < count; i++) {
                cout << "\n  Task " << (i+1) << ":\n";
                string name;
                cout << "    Name: "; getline(cin, name);
                int    p = readPriority();
                time_t start, end;
                readTimeRange(start, end);

                string dlChoice;
                cout << "    Set a deadline? (y/n): "; getline(cin, dlChoice);
                time_t deadline = 0;
                if (dlChoice == "y" || dlChoice == "Y") {
                    string dlStr;
                    while (true) {
                        cout << "    Deadline (YYYY-MM-DD HH:MM:SS): ";
                        getline(cin, dlStr);
                        if (stringToTime(dlStr, deadline) && deadline > end) break;
                        cout << "    [!] Deadline must be after end time.\n";
                    }
                }
                scheduler.addTask(name, p, start, end, deadline);
            }
        }
        else if (cmd == "delete") {
            string name;
            cout << "Task name: "; getline(cin, name);
            scheduler.deleteTask(name);
        }
        else if (cmd == "update") {
            string name;
            cout << "Task name: "; getline(cin, name);
            const auto& tasks = scheduler.getTaskList();
            auto it = find_if(tasks.begin(), tasks.end(),
                              [&](const Task& t){ return t.name == name; });
            if (it == tasks.end()) { cout << "[!] Not found.\n"; continue; }

            int    p  = it->priority;
            time_t s  = it->startTime, e = it->endTime, dl = it->deadline;
            string ch;

            cout << "Update priority? (y/n): "; getline(cin, ch);
            if (ch == "y" || ch == "Y") p = readPriority();

            cout << "Update time? (y/n): "; getline(cin, ch);
            if (ch == "y" || ch == "Y") readTimeRange(s, e);

            cout << "Update deadline? (y/n): "; getline(cin, ch);
            if (ch == "y" || ch == "Y") {
                string dlStr;
                cout << "    Deadline (YYYY-MM-DD HH:MM:SS): ";
                getline(cin, dlStr);
                stringToTime(dlStr, dl);
            }
            scheduler.updateTask(name, p, s, e, dl);
        }
        else if (cmd == "list")    { scheduler.listTasks();   }
        else if (cmd == "run")     { scheduler.runScheduler(); }
        else if (cmd == "compare") { scheduler.compare();     }
        else if (cmd == "exit")    { cout << "Goodbye!\n"; break; }
        else if (!cmd.empty()) {
            cout << "[!] Commands: add | delete | update | list | run | compare | exit\n";
        }
    }
    return 0;
}