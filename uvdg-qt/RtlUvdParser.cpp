
#include "RtlUvdParser.h"
#include <iostream>
#include <fstream>

#define DUPLICATE_DETECTOR_BUFFER_SIZE 1000

RtlUvdParser::RtlUvdParser(UvdState *state)
{
    m_state = state;
    
    m_lastTime = 0;
    m_day = 0;

    m_duplicateDetectorBuffer = (double *)calloc(1, DUPLICATE_DETECTOR_BUFFER_SIZE * sizeof(double));
    m_duplicateDetectorBufferIndex = 0;
}

double RtlUvdParser::processLine(char *line)
{
    // K1 14:57:41.207.405 [ 1776] {087} **** :01234
    // K2 14:57:41.212.757 [ 5352] {088} **** FL  770m [F025]+  F:40%
    
    if (line[0] != 'K') return -1.0;
    if (line[1] < '1' && line[1] > '4') return -1.0;
    
    RecvInfo ri;
    ri.hh = atoi(line + 3);
    ri.mm = atoi(line + 6);
    ri.ss = atoi(line + 9);
    int msec = atoi(line + 12);
    int usec = atoi(line + 16);
    ri.usec = msec * 1000 + usec;
    
    int seconds = ri.hh * 3600 + ri.mm * 60 + ri.ss;
    double time = seconds + ((double)ri.usec / 1000000.0);
    
    for (int i = 0; i < DUPLICATE_DETECTOR_BUFFER_SIZE; i++)
    {
        if (time == m_duplicateDetectorBuffer[i])
        {
//            printf("duplicated line, ignoring: %s\n", line);
            return -1.0;
        }
    }
    
    m_duplicateDetectorBuffer[m_duplicateDetectorBufferIndex] = time;
    m_duplicateDetectorBufferIndex++;
    if (m_duplicateDetectorBufferIndex == DUPLICATE_DETECTOR_BUFFER_SIZE) m_duplicateDetectorBufferIndex = 0;
    
    if (time < m_lastTime)
    {
        // crossed 00:00:00
        m_day++;
        printf("day cross %d (%02d:%02d:%02d (%lf) < %lf).\n", m_day, ri.hh, ri.mm, ri.ss, time, m_lastTime);
    }
    double fixedTime = time + (m_day * 24 * 60 * 60);
    ri.time = fixedTime;
    
    m_lastTime = time;
    
    sscanf(line + 30, "%02X", &ri.amplitude);
    
    ri.confidence = (line[34] == '*') + (line[35] == '*') + (line[36] == '*') + (line[37] == '*');
    
    if (line[1] == '1')
    {
        int tn = atoi(line + 40);
        
        K1 k1;
        k1.ri = ri;
        k1.tailNumber = tn;
        
        if (k1.tailNumber == 0)
        {
//            printf("bad tail number: %s.\n", line + 40);
        }
        else
        {
            m_state->processK1(k1);
        }
    }
    else if (line[1] == '2')
    {
        int alt = atoi(line + 42);
        int fuel = atoi(line + 59);

        K2 k2;
        k2.ri = ri;
        k2.alt = alt;
        k2.fuel = fuel;
        m_state->processK2(k2);
    }
    else if (line[1] == '3')
    {
        return -1.0;
    }
    else
    {
        return -1.0;
    }
    
    return fixedTime;
}

void RtlUvdParser::obtainAircraftStatistics()
{
/*    NSMutableDictionary *aircrafts = [NSMutableDictionary dictionary];
    for (NSDictionary *dict in m_occurrences)
    {
        NSString *tailNumber = dict[@"tailNumber"];
        NSNumber *first = dict[@"first"];
        NSNumber *last = dict[@"last"];

        NSMutableArray *occurrences = aircrafts[tailNumber];
        if (occurrences == nil)
        {
            occurrences = [NSMutableArray array];
            aircrafts[tailNumber] = occurrences;
        }
        
        NSDictionary *occurrence = @{@"first": first, @"last": last};
        [occurrences addObject:occurrence];
    }
    
    NSArray *sortedTailNumbers = [[aircrafts allKeys] sortedArrayUsingSelector:@selector(compare:)];
    
    NSLog(@"Unique aircrafts: %ld.", [[aircrafts allKeys] count]);
    for (NSString *tailNumber in sortedTailNumbers)
    {
        double duration = 0.0;
        NSArray *occurrences = aircrafts[tailNumber];
        NSMutableSet *days = [NSMutableSet set];
        for (NSDictionary *occurrence in occurrences)
        {
            double first = [occurrence[@"first"] doubleValue];
            double last = [occurrence[@"last"] doubleValue];
            
            int firstDay = (int)first / 86400;
            int lastDay = (int)last / 86400;
            
            [days addObject:[NSNumber numberWithInt:firstDay]];
            [days addObject:[NSNumber numberWithInt:lastDay]];
            
            duration += last - first;
        }
        
        NSLog(@"%05d: %ld occurrences, duration %.0lf mins over %ld day(s).",
              [tailNumber intValue], [occurrences count], duration / 60.0, [days count]);
    }*/
}

void RtlUvdParser::parseLogFile(const char *path)
{
    std::ifstream istream;
    istream.open(path);

    char line[256];
    while (istream.getline(line, 256))
    {
        processLine(line);
    }

    istream.close();
    
    // finalize
    
    m_state->finalizeLogFile();
    
//    printf("Confidence 3: K1 lines: %lu, K2 lines: %lu.\n", m_recvStats.k1Conf3Lines, m_recvStats.k2Conf3Lines);
//    printf("Confidence 4: K1 lines: %lu, K2 lines: %lu.\n", m_recvStats.k1Conf4Lines, m_recvStats.k2Conf4Lines);
//    printf("Days: %d.\n", m_day + 1);
    
    obtainAircraftStatistics();
}
