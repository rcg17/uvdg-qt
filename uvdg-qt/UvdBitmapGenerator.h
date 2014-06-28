
#ifndef __UVDBITMAPGENERATOR_H__
#define __UVDBITMAPGENERATOR_H__

#include "UvdState.h"
#include "Mutex.h"

#define MAX_ALTITUDE 11000.0f

class UvdBitmapGenerator
{
    UvdState *m_state;
    
    unsigned char *m_bitmap;
    int m_bitmapWidth;
    int m_bitmapHeight;
    
    bool m_isConfidence4Only;
    int m_boldThreshold;
    
    Mutex m_lock;
    
    void putPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
    
public:
    UvdBitmapGenerator(UvdState *state);
    ~UvdBitmapGenerator();
    
    void update(double leftTime, double rightTime, double firstTime, double lastTime, double timeSlice);
    
    void setBitmap(unsigned char *bitmap, int width, int height);
    unsigned char *bitmap() { return m_bitmap; }
    
    void setConfidence4Only(bool flag) { m_isConfidence4Only = flag; }
    bool isConfidence4Only() { return m_isConfidence4Only; }
    
    void setBoldThreshold(int threshold) { m_boldThreshold = threshold; }
    int boldThreshold() { return m_boldThreshold; }
    
    void lock();
    void unlock();
};

#endif
