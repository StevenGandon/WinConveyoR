#ifndef PLATFORM_TYPES_H_
#define PLATFORM_TYPES_H_

#ifdef _WIN32
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
#else
    #include <sys/types.h>
#endif

#endif /* !PLATFORM_TYPES_H_ */