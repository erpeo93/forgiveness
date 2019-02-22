
//PlatformProcessHandle name(char* path, char* command, char* arguments)
DEBUG_PLATFORM_EXECUTE_COMMAND(DEBUGWin32ExecuteCommand)
{
    PlatformProcessHandle result = {};
    STARTUPINFO startupInfo = {};
    startupInfo.cb = sizeof( startupInfo );
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_SHOW;
    
    PROCESS_INFORMATION processInfo;
    
    if(CreateProcess(command, arguments, 0, 0, FALSE, 0, 0, path, &startupInfo, &processInfo))
    {
        *(HANDLE*) &result.OSHandle = processInfo.hProcess;
    }
    else
    {
        *( HANDLE* ) &result.OSHandle = INVALID_HANDLE_VALUE;
    }
    return result;
}

//PlatformProcessState name( PlatformProcessHandle handle )
DEBUG_PLATFORM_GET_PROCESS_STATE( DEBUGWin32GetProcessState )
{
    PlatformProcessState result = {};
    
    HANDLE process = *( HANDLE* ) &handle.OSHandle; 
    if( process != INVALID_HANDLE_VALUE )
    {
        result.startedSuccessfully = true;
        
        if( WaitForSingleObject( process, 0 ) == WAIT_OBJECT_0 )
        {
            DWORD returnCode = 0;
            GetExitCodeProcess( process, &returnCode );
            result.returnCode = returnCode;
            CloseHandle( process );
        }
        else
        {
            result.isRunning = true;
        }
    }
    
    return result;
}


void Win32KillProcessByName(const char *filename)
{
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof (pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);
    while (hRes)
    {
        if (strcmp(pEntry.szExeFile, filename) == 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
                                          (DWORD) pEntry.th32ProcessID);
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
}
