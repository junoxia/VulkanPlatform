#ifndef H_LOGMANAGER
#define H_LOGMANAGER

#include "common.h"

#ifndef ANDROID
    #include "..\\..\\windowsFramework\\include\\windowsLogManager.h"
#else
    #include "..\\..\\androidFramework\\include\\androidLogManager.h"
#endif

class CLogManager {
public:
    CLogManager();
    ~CLogManager();

#ifndef ANDROID
    void setLogFile(std::string s);
#endif

    void print(std::string s);
    void print(std::string s, int n); 
    void print(std::string s, int n0, int n1);
    void print(std::string s, float n);
    void print(std::string s, float n0, float n1);
    void print(std::string s0, std::string s1);
    void print(std::string s, float *n, int size);
    void print(std::string s, unsigned long int n); //uint32_t


private:
#ifndef ANDROID
    CWindowsLogManager windowsLogManager;
#else
    CAndroidLogManager androidLogManager;
#endif
};

#endif