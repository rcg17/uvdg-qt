
#include "UvdState.h"
#include <list>
#include <stdlib.h>

#define HIDE_TAILNUMBERS true

UvdState::UvdState()
{
    m_isRealtimeMode = false;
    m_realtimeStartTime = -1.0;
    
    m_yyyy = 0;
    
    memset(&m_recvStats, 0, sizeof(RecvStats));
    
    MutexCreate(&m_lock);
}

UvdState::~UvdState()
{
    MutexDestroy(&m_lock);
}

void UvdState::processK1(K1 k1)
{
    preprocess(k1.ri.time);
    
    if (k1.ri.confidence == 3)
    {
        m_recvStats.k1Conf3Lines++;
    }
    else
    {
        m_recvStats.k1Conf4Lines++;
    }
    
    OccurrenceRecord record;
    if (m_pendingOccurrences.count(k1.tailNumber) == 0)
    {
        record.tailNumber = k1.tailNumber;
        record.firstTime = k1.ri.time;
        record.lastTime = k1.ri.time;
        m_pendingOccurrences[k1.tailNumber] = record;
    }
    else
    {
        record = m_pendingOccurrences[k1.tailNumber];
        if (record.lastTime + 100.0 < k1.ri.time)
        {
            // finalize old occurrence
            lock();
            m_occurrences.push_back(record);
            unlock();
            
            // and replace with new
            record.tailNumber = k1.tailNumber;
            record.firstTime = k1.ri.time;
            record.lastTime = k1.ri.time;
        }
        else
        {
            record.lastTime = k1.ri.time;
        }
        
        m_pendingOccurrences[k1.tailNumber] = record;
    }
    
    postprocess(k1.ri.time);
}

void UvdState::processK2(K2 k2)
{
    preprocess(k2.ri.time);
    
    if (k2.ri.confidence == 3)
    {
        m_recvStats.k2Conf3Lines++;
    }
    else
    {
        m_recvStats.k2Conf4Lines++;
    }
    
    lock();
    m_points.push_back(k2);
    unlock();
    
    postprocess(k2.ri.time);
}

void UvdState::finalizeLogFile()
{
    printf("finalizing log file.\n");
    
    std::map<int, OccurrenceRecord>::iterator iter;
    for (iter = m_pendingOccurrences.begin(); iter != m_pendingOccurrences.end(); ++iter)
    {
        OccurrenceRecord record = (OccurrenceRecord)iter->second;
        m_occurrences.push_back(record);
    }
    
    m_pendingOccurrences.clear();
    
    if (HIDE_TAILNUMBERS)
    {
        char randomTailNumber[6];
        randomTailNumber[5] = '\x00';
        
        std::vector<OccurrenceRecord>::iterator iter;
        for (iter = m_occurrences.begin(); iter != m_occurrences.end(); ++iter)
        {
            (*iter).tailNumber = rand() % 100000;
        }
    }
}

void UvdState::preprocess(double currentTime)
{
    if (m_isRealtimeMode)
    {
        if (m_realtimeStartTime < 0.0)
        {
            printf("first realtime line.\n");
            m_realtimeStartTime = currentTime;
        }
        
        m_lastTime = currentTime;
    }
}

void UvdState::postprocess(double currentTime)
{
    std::list<int> finalizedOccurrences;
    std::map<int, OccurrenceRecord>::iterator iter;
    for (iter = m_pendingOccurrences.begin(); iter != m_pendingOccurrences.end(); ++iter)
    {
        int tailNumber = iter->first;
        OccurrenceRecord record = (OccurrenceRecord)iter->second;
        if (record.lastTime + 100.0 < currentTime)
        {
            finalizedOccurrences.push_back(tailNumber);
        }
    }
    
    lock();
    std::list<int>::iterator listIter;
    for (listIter = finalizedOccurrences.begin(); listIter != finalizedOccurrences.end(); ++listIter)
    {
        int tailNumber = *listIter;
        OccurrenceRecord record = m_pendingOccurrences[tailNumber];
        if (record.lastTime - record.firstTime > 1.0)
        {
            m_occurrences.push_back(record);
        }
        
        iter = m_pendingOccurrences.find(tailNumber);
        m_pendingOccurrences.erase(iter);
    }
    unlock();
}

void UvdState::lock()
{
    if (m_isRealtimeMode)
    {
        MutexLock(&m_lock);
    }
}

void UvdState::unlock()
{
    if (m_isRealtimeMode)
    {
        MutexUnlock(&m_lock);
    }
}

std::vector<OccurrenceRecord> *UvdState::occurrences()
{
    if (!m_isRealtimeMode)
    {
        return &m_occurrences;
    }
    else
    {
        m_tempOccurrences = m_occurrences;
        
        std::map<int, OccurrenceRecord>::iterator iter;
        for (iter = m_pendingOccurrences.begin(); iter != m_pendingOccurrences.end(); ++iter)
        {
            OccurrenceRecord record = (OccurrenceRecord)iter->second;
            record.lastTime = m_lastTime;
            m_tempOccurrences.push_back(record);
        }
        
        return &m_tempOccurrences;
    }
}
