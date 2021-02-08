#ifndef ISO7816_EXCEPTION_H
#define ISO7816_EXCEPTION_H

#include <string>
#include <iostream>
#include <exception>

// signalling exceptions used to exit the current decoding

class ISO7816ExceptionProtocol : public std::exception
{
public:
    inline ISO7816ExceptionProtocol(const char* errorDetails = NULL):mErrorDetails(errorDetails){}

    std::string GetDetails(void) { return mErrorDetails;}

private:
    std::string mErrorDetails;
};


#endif // of ISO7816_EXCEPTION_H
