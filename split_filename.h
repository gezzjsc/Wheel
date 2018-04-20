#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #define SPLIT_FILENAME(x) strrchr(x,'\\')?strrchr(x,'\\')+1:x
#else
    #define SPLIT_FILENAME(x) strrchr(x,'/')?strrchr(x,'/')+1:x
#endif