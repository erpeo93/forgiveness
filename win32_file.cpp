//void DEBUGWin32FreeFile( PlatformFile* file )
DEBUG_PLATFORM_FREE_FILE( DEBUGWin32FreeFile )
{
    VirtualFree( file->content, 0, MEM_RELEASE );
}

//PlatformFile DEBUGWin32ReadFile( char* fileName )
DEBUG_PLATFORM_READ_FILE(DEBUGWin32ReadFile)
{
    PlatformFile result = {};
    HANDLE fileHandle = CreateFile( fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0 );
    if( fileHandle != INVALID_HANDLE_VALUE )
    {
        LARGE_INTEGER fileSizeEx;
        if( GetFileSizeEx( fileHandle, &fileSizeEx ) )
        {
            u32 fileSize32 = SafeTruncateUInt64ToU32( fileSizeEx.QuadPart );
            result.content = VirtualAlloc( 0, fileSize32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
            if( result.content )
            {
                DWORD bytesRead;
                if( ReadFile( fileHandle, result.content, fileSize32, &bytesRead, 0 ) &&
                   ( fileSize32 == bytesRead ) )
                {
                    result.size = fileSize32;
                    
                    char* toZero = (char*) result.content;
                    toZero[result.size] = 0;
                }
                else
                {
                    result.content = 0;
                    DEBUGWin32FreeFile( &result );
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
        CloseHandle( fileHandle );
        
    }
    else
    {
        //TODO(leonardo): diagnostic
    }
    return result;
}

//b32 DEBUGWin32WriteFile(char* fileName, void* content, u32 fileSize)
DEBUG_PLATFORM_WRITE_FILE( DEBUGWin32WriteFile )
{
    b32 result = false;
    Assert( fileSize <= 0xFFFFFFFF );
    
    HANDLE fileHandle = CreateFile( fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0 );
    if( fileHandle != INVALID_HANDLE_VALUE )
    {
        DWORD bytesWritten;
        if( WriteFile( fileHandle, content, fileSize, &bytesWritten, 0 ) &&
           ( bytesWritten == fileSize ) )
        {
            result = true;
        }
        else
        {
            //TODO(leonardo: diagnostic
        }
        CloseHandle( fileHandle );				
    }
    else
    {
        //TODO(leonardo): diagnostic
    }
    return result;
}



// NOTE(Leonardo): "shippable" file API
struct Win32PlatformFileHandle
{
    char* name;
    HANDLE handle;
};

struct Win32PlatformFileGroup
{
    HANDLE findHandle;
    WIN32_FIND_DATAA toProcess;
};

//void name( PlatformFileHandle* handle, char* error )
internal PLATFORM_FILE_ERROR( Win32FileError )
{
#if FORGIVENESS_INTERNAL
    OutputDebugString( "FILE ERROR:" );
    OutputDebugString( error );
    OutputDebugString( "\n" );
#endif
    
    handle->noErrors = false;
}

//PlatformFileGroup* name( PlatformFileType type, char* folderPath )
internal PLATFORM_GET_ALL_FILE_BEGIN(Win32GetAllFilesBegin)
{
    char* extension = 0;
    switch( type )
    {
        case PlatformFile_compressedAsset:
        {
            extension = "*.pak";
        } break;
        
        case PlatformFile_uncompressedAsset:
        {
            extension = "*.upak";
        } break;
        
        case PlatformFile_savedGame:
        {
            extension = "*.fsav";
        } break;
        
        case PlatformFile_entityDefinition:
        {
            extension = "*.fed";
        } break;
        
        case PlatformFile_assetDefinition:
        {
            extension = "*.fad";
        } break;
        
        case PlatformFile_image:
        {
            extension = "*.png";
            
        } break;
        
        case PlatformFile_animation:
        {
            extension = "*.scml";
            
        } break;
        
        case PlatformFile_sound:
        {
            extension = "*.wav";
            
        } break;
        
        case PlatformFile_autocomplete:
        {
            extension = "*.autocomplete";
            
        } break;
        
        case PlatformFile_model:
        {
            extension = "*.obj";
        } break;
        
        case PlatformFile_all:
        {
            extension = "*.*";
        } break;
        InvalidDefaultCase;
    }
    
    
    
    char completePath[128];
    if( folderPath )
    {
        FormatString( completePath, sizeof( completePath ), "%s/%s",  folderPath, extension );
    }
    else
    {
        StrCpy( extension, StrLen( extension ), completePath, sizeof( completePath ) );
    }
    
    PlatformFileGroup result = {};
    
    Win32PlatformFileGroup* group = ( Win32PlatformFileGroup* ) VirtualAlloc( 0, sizeof( Win32PlatformFileGroup ), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
    result.platform = group;
    
    WIN32_FIND_DATAA findData;
    HANDLE findHandle = FindFirstFileA( completePath, &findData );
    
    while( findHandle != INVALID_HANDLE_VALUE )
    {
        ++result.fileCount;
        if( !FindNextFile( findHandle, &findData ) )
        {
            break;
        }
    }
    
    if( findHandle != INVALID_HANDLE_VALUE )
    {
        FindClose( findHandle );
    }
    
    group->findHandle = FindFirstFileA( completePath, &group->toProcess );
    return result;
}

//void name( PlatformSubdirNames* output, char* folderPath )
internal PLATFORM_GET_ALL_SUBDIRECTORIES_NAME(Win32GetAllSubdirectoriesName)
{
    char completePath[128];
    if( folderPath )
    {
        FormatString( completePath, sizeof( completePath ), "%s/%s",  folderPath, "*" );
    }
    else
    {
        StrCpy( "*", 1, completePath, sizeof( completePath ) );
    }
    
    output->subDirectoryCount = 0;
    
    WIN32_FIND_DATAA findData;
    HANDLE findHandle = FindFirstFileA( completePath, &findData );
    
    while( findHandle != INVALID_HANDLE_VALUE )
    {
        if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            char* dest = output->subdirs[output->subDirectoryCount++];
            StrCpy( findData.cFileName, StrLen( findData.cFileName ), dest );
        }
        
        if( !FindNextFile( findHandle, &findData ) )
        {
            break;
        }
    }
    
    if( findHandle != INVALID_HANDLE_VALUE )
    {
        FindClose( findHandle );
    }
}

internal PLATFORM_CLOSE_HANDLE_INTERNAL( Win32CloseHandle )
{
    Win32PlatformFileHandle* win32Handle = ( Win32PlatformFileHandle* ) handle->platform;
    Assert( win32Handle );
    if(!CloseHandle( win32Handle->handle ))
    {
        InvalidCodePath;
    }
    win32Handle->handle = 0;
}

//PlatformFileHandle name( PlatformFileGroup* group, char* folderPath )
internal PLATFORM_OPEN_NEXT_FILE( Win32OpenNextFile )
{
    PlatformFileHandle result = {};
    
    Win32PlatformFileHandle* win32Handle = 0;
    Win32PlatformFileGroup* win32Group = ( Win32PlatformFileGroup* ) group->platform;
    
    if( win32Group )
    {
        if( win32Group->findHandle != INVALID_HANDLE_VALUE )
        {
            win32Handle = ( Win32PlatformFileHandle* ) VirtualAlloc( 0, sizeof( Win32PlatformFileHandle ), 
                                                                    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
            result.platform = win32Handle;
            
            if( win32Handle )
            {
                if( folderPath )
                {
                    char completePath[128];
                    FormatString( completePath, sizeof( completePath ), "%s/%s", folderPath, win32Group->toProcess.cFileName );
                    win32Handle->handle = CreateFile( completePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0 );
                    Assert( StrLen( win32Group->toProcess.cFileName ) < ArrayCount( result.name ) );
                    StrCpy( win32Group->toProcess.cFileName, StrLen( win32Group->toProcess.cFileName ), result.name );
                }
                else
                {
                    win32Handle->handle = CreateFile( win32Group->toProcess.cFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0 );
                }
                
                LARGE_INTEGER fileSizeEx;
                if( GetFileSizeEx( win32Handle->handle, &fileSizeEx ) )
                {
                    result.fileSize = SafeTruncateUInt64ToU32( fileSizeEx.QuadPart );
                }
                
                result.noErrors = ( win32Handle->handle != INVALID_HANDLE_VALUE );
            }
        }
        
        if( !FindNextFileA( win32Group->findHandle, &win32Group->toProcess ) )
        {
            FindClose( win32Group->findHandle );
            win32Group->findHandle = INVALID_HANDLE_VALUE;
        }
    }
    
    return result;
}

//void name( PlatformFileHandle* handle, u64 offset, u32 size, void* dest )
internal PLATFORM_READ_FROM_FILE( Win32ReadFromFile )
{
    if( PlatformNoFileErrors( handle ) )
    {
        Win32PlatformFileHandle* win32Handle = ( Win32PlatformFileHandle* ) handle->platform;
        
        OVERLAPPED overlapped = {};
        overlapped.Offset = ( u32 ) ( ( offset >> 0 ) & 0xffffffff );
        overlapped.OffsetHigh = ( u32 ) ( ( offset >> 32 ) & 0xffffffff );
        
        u32 fileSize32 = SafeTruncateUInt64ToU32( size );
        
        DWORD bytesRead;
        if( ReadFile( win32Handle->handle, dest, fileSize32, &bytesRead, &overlapped ) &&
           ( fileSize32 == bytesRead ) )
        {
            // NOTE( Leonardo ): read completed successfully!
        }
        else
        {
            Win32FileError( handle, "error reading file" );
        }
    }
}

//void name( PlatformFileGroup* group )
internal PLATFORM_GET_ALL_FILE_END( Win32GetAllFilesEnd )
{
    Win32PlatformFileGroup* win32Group = ( Win32PlatformFileGroup* ) group->platform;
    if( win32Group )
    {
        if( win32Group->findHandle != INVALID_HANDLE_VALUE )
        {
            FindClose( win32Group->findHandle );
        }
        VirtualFree( win32Group, 0, MEM_RELEASE );
    }
}


internal void Win32BuildFullPath( char* ExeFullPath, char* fileName, 
                                 char* dest, i32 countDest )
{
    char* onePastLastSlash = ExeFullPath;
    char* holder = ExeFullPath;
    while( *holder )
    {
        if( *holder++ == '\\' )
        {
            onePastLastSlash = holder;
        }
    }
    
    i32 countPath = StrLen( ExeFullPath ) - StrLen( onePastLastSlash );
    StrCpy(ExeFullPath, countPath, fileName, StrLen(fileName),
           dest, countDest );
    
}


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


PLATFORM_DELETE_FILE_WILDCARDS(Win32DeleteFileWildcards)
{
	WIN32_FIND_DATA fd;
    char completeFileName[MAX_PATH];
    FormatString(completeFileName, sizeof(completeFileName), "%s\\%s", path, fileName);
    
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

PLATFORM_MOVE_FILE_OR_FOLDER(Win32MoveFileOrFolder)
{
	BOOL result = MoveFile(oldPath, newPath);
}

PLATFORM_COPY_FILE_OR_FOLDER(Win32CopyFileOrFolder)
{
    BOOL result = CopyFile(oldPath, newPath, true);
}
