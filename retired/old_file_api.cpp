
// NOTE(leonardo): file I/O DEBUG
struct PlatformFile
{
    void* content;
    u32 size;
};

#define DEBUG_PLATFORM_FREE_FILE(name) void name(PlatformFile* file)
typedef DEBUG_PLATFORM_FREE_FILE(platform_free_file);

#define DEBUG_PLATFORM_READ_FILE(name) PlatformFile name(char* fileName)
typedef DEBUG_PLATFORM_READ_FILE(platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_FILE(name) b32 name(char* fileName, void* content, u32 fileSize)
typedef DEBUG_PLATFORM_WRITE_FILE(platform_write_entire_file);


//void DEBUGWin32FreeFile(PlatformFile* file)
DEBUG_PLATFORM_FREE_FILE(DEBUGWin32FreeFile)
{
    VirtualFree(file->content, 0, MEM_RELEASE);
}

//PlatformFile DEBUGWin32ReadFile(char* fileName)
DEBUG_PLATFORM_READ_FILE(DEBUGWin32ReadFile)
{
    PlatformFile result = {};
    HANDLE fileHandle = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSizeEx;
        if(GetFileSizeEx(fileHandle, &fileSizeEx))
        {
            u32 fileSize32 = SafeTruncateUInt64ToU32(fileSizeEx.QuadPart);
            result.content = VirtualAlloc(0, fileSize32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if(result.content)
            {
                DWORD bytesRead;
                if(ReadFile(fileHandle, result.content, fileSize32, &bytesRead, 0) &&
                   (fileSize32 == bytesRead))
                {
                    result.size = fileSize32;
                    
                    char* toZero = (char*) result.content;
                    toZero[result.size] = 0;
                }
                else
                {
                    result.content = 0;
                    DEBUGWin32FreeFile(&result);
                }
            }
            else
            {
                //TODO(leonardo: diagnostic
            }
        }
        else
        {
            //TODO(leonardo: diagnostic
        }
        CloseHandle(fileHandle);
        
    }
    else
    {
        //TODO(leonardo): diagnostic
    }
    return result;
}

//b32 DEBUGWin32WriteFile(char* fileName, void* content, u32 fileSize)
DEBUG_PLATFORM_WRITE_FILE(DEBUGWin32WriteFile)
{
    b32 result = false;
    Assert(fileSize <= 0xFFFFFFFF);
    
    HANDLE fileHandle = CreateFile(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if(WriteFile(fileHandle, content, fileSize, &bytesWritten, 0) &&
           (bytesWritten == fileSize))
        {
            result = true;
        }
        else
        {
            //TODO(leonardo: diagnostic
        }
        CloseHandle(fileHandle);				
    }
    else
    {
        //TODO(leonardo): diagnostic
    }
    return result;
}


