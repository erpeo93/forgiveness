#define TRANSLATION_FUNCTION(name) void name(EditorElement* root)
typedef TRANSLATION_FUNCTION(translate_editor_tab);
TRANSLATION_FUNCTION(noTranslation){}

inline void DoStuff(EditorElement* root)
{
    int a = 5;
}

#define Translate(functions, root) Translate_(functions, ArrayCount((functions)) - 1, root)
inline void Translate_(translate_editor_tab** translateFunctions, u32 currentVersion, EditorElement* root)
{
    u32 version = root->versionNumber;
    if(version <= currentVersion)
    {
        for(u32 versionIndex = version; versionIndex < currentVersion; ++versionIndex)
        {
            translateFunctions[versionIndex + 1](root);
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    root->versionNumber = currentVersion;
}