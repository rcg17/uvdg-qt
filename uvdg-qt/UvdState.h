
#ifndef __UVDSTATE_H__
#define __UVDSTATE_H__

#include <map>
#include <vector>
#include "Mutex.h"

typedef struct {
    int hh, mm, ss, usec;
    double time;
    int amplitude;
    int confidence;
} RecvInfo;

typedef struct {
    RecvInfo ri;
    int tailNumber;
} K1;

typedef struct {
    RecvInfo ri;
    int alt;
    int fuel;
} K2;

typedef struct {
    int tailNumber;
    double firstTime;
    double lastTime;
} OccurrenceRecord;

typedef struct {
    unsigned long k1Conf3Lines;
    unsigned long k2Conf3Lines;
    unsigned long k1Conf4Lines;
    unsigned long k2Conf4Lines;
} RecvStats;

class UvdState
{
    std::map<int, OccurrenceRecord> m_pendingOccurrences;
    std::vector<OccurrenceRecord> m_occurrences;
    std::vector<OccurrenceRecord> m_tempOccurrences;
    std::vector<K2> m_points;
    
    bool m_isRealtimeMode;
    double m_realtimeStartTime;
    double m_lastTime;
    
    int m_yyyy, m_mm, m_dd;
    
    RecvStats m_recvStats;
    
    Mutex m_lock;

    void preprocess(double currentTime);
    void postprocess(double currentTime);

public:
    UvdState();
    ~UvdState();
    
    void processK1(K1 k1);
    void processK2(K2 k2);
    void finalizeLogFile();

    void setStartDate(int yyyy, int mm, int dd) { m_yyyy = yyyy; m_mm = mm; m_dd = dd; }
    void startRealtimeMode() { m_isRealtimeMode = true; }

    std::vector<OccurrenceRecord> *occurrences();
    std::vector<K2> *points() { return &m_points; }
    bool isRealtimeStarted() { return m_isRealtimeMode && m_realtimeStartTime > 0.0; }
    bool getStartDate(int *yyyy, int *mm, int *dd) { *yyyy = m_yyyy; *mm = m_mm; *dd = m_dd; return m_yyyy != 0; }
    
    void lock();
    void unlock();
};

#endif
