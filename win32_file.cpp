struct Win32PlatformFileGroup
{
    MemoryPool memory;
};

//void name(PlatformFileHandle* handle, char* error)
internal PLATFORM_FILE_ERROR(Win32FileError)
{
#if FORGIVENESS_INTERNAL
    OutputDebugString("FILE ERROR:");
    OutputDebugString(error);
    OutputDebugString("\n");
#endif
    
    handle->noErrors = false;
}

struct FileExtension
{
    char extension[16];
};

internal FileExtension GetFileExtension(PlatformFileType type)
{
    FileExtension result = {};
    
    char* extension = 0;
    switch(type)
    {
        case PlatformFile_compressedAsset:
        {
            extension = "pak";
        } break;
        
        case PlatformFile_uncompressedAsset:
        {
            extension = "upak";
        } break;
        
        case PlatformFile_savedGame:
        {
            extension = "fsav";
        } break;
        
        case PlatformFile_image:
        {
            extension = "png";
            
        } break;
        
        case PlatformFile_font:
        {
            extension = "ttf";
            
        } break;
        
        case PlatformFile_skeleton:
        {
            extension = "scml";
            
        } break;
        
        case PlatformFile_sound:
        {
            extension = "wav";
            
        } break;
        
        case PlatformFile_model:
        {
            extension = "obj";
        } break;
        
        case PlatformFile_data:
        {
            extension = "dat";
        } break;
        
        case PlatformFile_markup:
        {
            extension = "tag";
        } break;
        
        case PlatformFile_reloadedAsset:
        {
            extension = "rll";
        } break;
        
        InvalidDefaultCase;
    }
    
    FormatString(result.extension, sizeof(result.extension), "%s", extension);
    
    return result;
}

//void name(PlatformSubdirNames* output, char* folderPath)
internal PLATFORM_GET_ALL_SUBDIRECTORIES(Win32GetAllSubdirectories)
{
    char completePath[128];
    if(folderPath)
    {
        FormatString(completePath, sizeof(completePath), "%s/%s",  folderPath, "*");
    }
    else
    {
        StrCpy("*", 1, completePath, sizeof(completePath));
    }
    
    output->count = 0;
    
    WIN32_FIND_DATAA findData;
    HANDLE findHandle = FindFirstFileA(completePath, &findData);
    
    while(findHandle != INVALID_HANDLE_VALUE)
    {
        if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if(!StrEqual(findData.cFileName, ".") && !StrEqual(findData.cFileName, ".."))
            {
                if(findData.cFileName[0] != '.')
                {
                    char* dest = output->names[output->count++];
                    StrCpy(findData.cFileName, StrLen(findData.cFileName), dest);
                }
            }
        }
        
        if(!FindNextFile(findHandle, &findData))
        {
            break;
        }
    }
    
    if(findHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(findHandle);
    }
}


//PlatformFileGroup name(PlatformFileType type, char* folderPath)
internal PLATFORM_GET_ALL_FILE_BEGIN(Win32GetAllFilesBegin)
{
    FileExtension ext = GetFileExtension(type);
    
    char pathString[128];
    pathString[0] = 0;
    char completePath[128];
    if(path)
    {
        FormatString(pathString, sizeof(pathString), "%s/", path);
    }
    FormatString(completePath, sizeof(completePath), "%s*.%s",  pathString, ext.extension);
    
    PlatformFileGroup result = {};
    Win32PlatformFileGroup* group = BootstrapPushStruct(Win32PlatformFileGroup, memory);
    result.path = PushNullTerminatedString(&group->memory, pathString);
    result.fileCount = 0;
    result.firstFileInfo = 0;
    result.platform = group;
    
    WIN32_FIND_DATAA findData;
    HANDLE findHandle = FindFirstFileA(completePath, &findData);
    while(findHandle != INVALID_HANDLE_VALUE)
    {
        ++result.fileCount;
        
        PlatformFileInfo* info = PushStruct(&group->memory, PlatformFileInfo);
        info->size = ((u64)findData.nFileSizeHigh << (u64)32) | ((u64)findData.nFileSizeLow);
        info->name = PushNullTerminatedString(&group->memory, findData.cFileName);
        
        info->next = result.firstFileInfo;
        result.firstFileInfo = info;
        
        if(!FindNextFile(findHandle, &findData))
        {
            break;
        }
    }
    
    if(findHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(findHandle);
    }
    
    return result;
}

internal PLATFORM_GET_ALL_FILE_END(Win32GetAllFilesEnd)
{
    Win32PlatformFileGroup* win32Group = (Win32PlatformFileGroup*) group->platform;
    if(win32Group)
    {
        Clear(&win32Group->memory);
    }
}

//PlatformFileHandle name(PlatformFileGroup* group, PlatformFileInfo* info)
internal PLATFORM_OPEN_FILE(Win32OpenFile)
{
    PlatformFileHandle result = {};
    
    char completePath[128];
    FormatString(completePath, sizeof(completePath), "%s%s", group->path, info->name);
    
    HANDLE handle = CreateFile(completePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
    result.noErrors = (handle != INVALID_HANDLE_VALUE);
    
    Assert(sizeof(HANDLE) <= sizeof(void*));
    result.platform = (void*) handle;
    
    return result;
}

internal PLATFORM_CLOSE_FILE(Win32CloseFile)
{
    HANDLE toClose = (HANDLE) handle->platform;
    Assert(toClose);
    if(!CloseHandle(toClose))
    {
        Win32FileError(handle, "Error while closing file handle!");
    }
    handle->platform = 0;
}

//void name(PlatformFileHandle* handle, u64 offset, u32 size, void* dest)
internal PLATFORM_READ_FROM_FILE(Win32ReadFromFile)
{
    if(PlatformNoFileErrors(handle))
    {
        HANDLE toRead = (HANDLE) handle->platform;
        
        OVERLAPPED overlapped = {};
        overlapped.Offset = (u32) ((offset >> 0) & 0xffffffff);
        overlapped.OffsetHigh = (u32) ((offset >> 32) & 0xffffffff);
        
        u32 fileSize32 = SafeTruncateUInt64ToU32(size);
        
        DWORD bytesRead;
        if(ReadFile(toRead, dest, fileSize32, &bytesRead, &overlapped) &&
           (fileSize32 == bytesRead))
        {
            // NOTE(Leonardo): read completed successfully!
        }
        else
        {
            Win32FileError(handle, "error reading file");
        }
    }
}

PLATFORM_DELETE_FILES(Win32DeleteFiles)
{
    FileExtension ext = GetFileExtension(type);
    
	WIN32_FIND_DATA fd;
    char completeFileName[MAX_PATH];
    FormatString(completeFileName, sizeof(completeFileName), "%s\\*.%s", path, ext.extension);
    
	HANDLE hFind = FindFirstFile(completeFileName, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			FormatString(completeFileName, sizeof(completeFileName), "%s\\%s", path, fd.cFileName);
			DeleteFile(completeFileName);
		} while (FindNextFile(hFind, &fd));
		FindClose(hFind);
	}
}


PLATFORM_REPLACE_FILE(Win32ReplaceFile)
{
    FileExtension ext = GetFileExtension(type);
    char fileName[128];
    FormatString(fileName, sizeof(fileName), "%s/%s.%s", path, file, ext.extension);
    
    b32 result = false;
    Assert(size <= 0xFFFFFFFF);
    
    HANDLE fileHandle = CreateFile(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if(WriteFile(fileHandle, content, size, &bytesWritten, 0) &&
           (bytesWritten == size))
        {
            result = true;
        }
        else
        {
            //TODO(leonardo: diagnostic
            InvalidCodePath;
        }
        CloseHandle(fileHandle);				
    }
    else
    {
        //TODO(leonardo): diagnostic
    }
    return result;
}


internal void Win32BuildFullPath(char* ExeFullPath, char* fileName, 
                                 char* dest, i32 countDest)
{
    char* onePastLastSlash = ExeFullPath;
    char* holder = ExeFullPath;
    while(*holder)
    {
        if(*holder++ == '\\')
        {
            onePastLastSlash = holder;
        }
    }
    
    i32 countPath = StrLen(ExeFullPath) - StrLen(onePastLastSlash);
    StrCpy(ExeFullPath, countPath, fileName, StrLen(fileName),
           dest, countDest);
    
}

#if 0
PLATFORM_DELETE_FOLDER_RECURSIVE(Win32DeleteFolderRecursive)
{
	SHFILEOPSTRUCTA file_op = {0, FO_DELETE, path, "", FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, false, 0, ""};
	SHFileOperationA(&file_op);
}


PLATFORM_CREATE_FOLDER(Win32CreateFolder)
{
	b32 result = CreateDirectoryA(path, 0);
	return result;
}

PLATFORM_COPY_ALL_FILES(Win32CopyAllFiles)
{
    char pathSource[1024] = {};
    FormatString(pathSource, sizeof(pathSource), "%s", source);
    
    
	SHFILEOPSTRUCTA s = {0};
    s.wFunc = FO_COPY;
    
    s.pTo = dest;
    s.pFrom = pathSource;
    
    s.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | 
        FOF_NOERRORUI | FOF_NO_UI | FOF_FILESONLY;
    int res = SHFileOperationA(&s);
    
    if(res)
    {
        InvalidCodePath;
    }
}

PLATFORM_MOVE_FILE_OR_FOLDER(Win32MoveFileOrFolder)
{
	BOOL result = MoveFile(oldPath, newPath);
}

PLATFORM_COPY_FILE_OR_FOLDER(Win32CopyFileOrFolder)
{
    BOOL result = CopyFile(oldPath, newPath, true);
}
#endif
