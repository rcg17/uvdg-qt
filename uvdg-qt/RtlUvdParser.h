
#ifndef __RTLUVDPARSER_H__
#define __RTLUVDPARSER_H__

#include "UvdState.h"

class RtlUvdParser
{
    UvdState *m_state;
    
    double *m_duplicateDetectorBuffer;
    int m_duplicateDetectorBufferIndex;
    
    double m_lastTime;
    int m_day;
    
    void obtainAircraftStatistics();
    
public:
    RtlUvdParser(UvdState *state);
    
    double processLine(char *line);
    void parseLogFile(const char *path);
};

#endif
