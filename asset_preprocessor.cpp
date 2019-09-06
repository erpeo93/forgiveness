#include "forg_token.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include "forg_base.h"
#include "forg_shared.h"

#include "forg_token.cpp"

#define OpenEnum() OpenStruct()
void OpenStruct()
{
    printf("{\n");
}

#define ReturnOpenEnum() ReturnOpenStruct()
void ReturnOpenStruct()
{
    printf("\n");
    OpenStruct();
}


#define CloseEnum() CloseStruct()
void CloseStruct()
{
    printf("};\n\n");
}

void Enum()
{
    printf(",\n");
}

void Field()
{
    printf(";\n");
}


struct SubFolders
{
    int count;
    char names[32][64];
};

void GetAllSubdirectoriesNoSpaces(SubFolders* folders, char* folderPath)
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
    
    folders->count = 0;
    
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
                    char* dest = folders->names[folders->count++];
                    StrCpy(findData.cFileName, StrLen(findData.cFileName), dest);
                    
                    for(u32 charIndex = 0; charIndex < StrLen(findData.cFileName); ++charIndex)
                    {
                        if(dest[charIndex] == ' ')
                        {
                            dest[charIndex] = '_';
                        }
                    }
                    
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

int main(int argc, char** argv)
{
    printf("enum AssetType");
    ReturnOpenEnum();
    
    char metaAssetTable[2048];
    char metaSubtypesTable[2048];
    
    char* metaAssetPtr = metaAssetTable;
    metaAssetPtr += sprintf(metaAssetPtr, "char* metaAsset_assetType[] = \n{\n");
    metaAssetPtr += sprintf(metaAssetPtr, "\"Invalid\", \n");
    
    
    char* metaSubtypesPtr = metaSubtypesTable;
    metaSubtypesPtr += sprintf(metaSubtypesPtr, "MetaAssetType metaAsset_subTypes[AssetType_Count] = \n{\n");
    metaSubtypesPtr += sprintf(metaSubtypesPtr, "{%d, NULL},\n", 0);
    
    printf("AssetType_Invalid,\n");
    
    char* path = "../server/assets/raw";
    
    SubFolders folders;
    GetAllSubdirectoriesNoSpaces(&folders, path);
    
    SubFolders* allSubFolders = (SubFolders*) malloc(sizeof(SubFolders) * folders.count);
    for(int folderIndex = 0; folderIndex < folders.count; ++folderIndex)
    {
        char* folderName = folders.names[folderIndex];
        printf("AssetType_%s", folderName);
        Enum();
        
        
        
        metaAssetPtr += sprintf(metaAssetPtr, "\"%s\",\n", folderName);
        
        SubFolders* subFolders = allSubFolders + folderIndex;
        char subpath[128];
        sprintf(subpath, "%s/%s", path, folderName);
        GetAllSubdirectoriesNoSpaces(subFolders, subpath);
        
        metaSubtypesPtr += sprintf(metaSubtypesPtr, "{%d, metaAsset_%s},\n", subFolders->count, folderName);
    }
    
    printf("AssetType_Count\n");
    CloseEnum();
    
    
    sprintf(metaAssetPtr, "};\n\n");
    printf(metaAssetTable);
    printf("\n");
    
    
    
    
    char metaSubAssetTable[2048];
    for(int folderIndex = 0; folderIndex < folders.count; ++folderIndex)
    {
        char* metaSubAssetPtr = metaSubAssetTable;
        char* folderName = folders.names[folderIndex];
        
        metaSubAssetPtr += sprintf(metaSubAssetPtr, "char* metaAsset_%s[] = {\n", folderName);
        
        
        printf("enum Asset%sType", folderName);
        ReturnOpenEnum();
        
        SubFolders* typeSubFolders = allSubFolders + folderIndex;
        for(int subFolderIndex = 0; subFolderIndex < typeSubFolders->count; ++subFolderIndex)
        {
            char* subFolderName = typeSubFolders->names[subFolderIndex];
            printf("Asset%s_%s", folderName, subFolderName);
            Enum();
            
            metaSubAssetPtr += sprintf(metaSubAssetPtr, "\"%s\",\n", subFolderName);
        }
        
        printf("Asset%s_Count\n", folderName);
        CloseEnum();
        
        metaSubAssetPtr += sprintf(metaSubAssetPtr, "};\n");
        
        printf(metaSubAssetTable);
    }
    
    
    
    sprintf(metaSubtypesPtr, "};\n\n");
    printf(metaSubtypesTable);
    printf("\n");
    
}

