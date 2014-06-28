
#include "UvdBitmapGenerator.h"
#include <stdlib.h>

#ifdef _MSC_VER
#define BITMAP_BGR
#endif

UvdBitmapGenerator::UvdBitmapGenerator(UvdState *state)
{
    m_state = state;
    m_bitmap = NULL;
    
    m_isConfidence4Only = false;
    m_boldThreshold = 100;
    
    MutexCreate(&m_lock);
}

UvdBitmapGenerator::~UvdBitmapGenerator()
{
}

void UvdBitmapGenerator::setBitmap(unsigned char *bitmap, int width, int height)
{
    m_bitmap = bitmap;
    m_bitmapWidth = width;
    m_bitmapHeight = height;
}

void UvdBitmapGenerator::update(double leftTime, double rightTime, double firstTime, double lastTime, double timeSlice)
{
    if (m_bitmap == NULL) return;
    
    memset(m_bitmap, 0, m_bitmapWidth * m_bitmapHeight * 4);
    
    bool havePointsInViewport = true;
    if (lastTime <= leftTime || firstTime >= rightTime)
    {
//        printf("no points in viewport.\n");
        havePointsInViewport = false;
    }
    
    double timeOffset = leftTime;
    
    m_state->lock();
    std::vector<K2> *points = m_state->points();
    
    // find index of first visible point
    
    unsigned int startIndex = 0;
    if (havePointsInViewport)
    {
        double startTime = firstTime;
        while (startTime < timeOffset)
        {
            if (startIndex >= points->size())
            {
                m_state->unlock();
                return;
            }
            
            K2 point = points->at(startIndex++);
            
            startTime = point.ri.time;
        }
    }
    
    double time = timeOffset;
    unsigned int index = startIndex;
    for (int i = 0; i < m_bitmapWidth; i++)
    {
        double prevTime = time;
        time += timeSlice;
        
        if ((int)prevTime % 86400 > (int)time % 86400)
        {
            for (int j = 0; j < m_bitmapHeight; j++)
            {
                putPixel(i, j, 0x7f, 0x7f, 0x7f);
            }
        }
        else if ((int)prevTime % 3600 > (int)time % 3600)
        {
            for (int j = 0; j < m_bitmapHeight; j++)
            {
                if (j % 3 != 0) continue;
                putPixel(i, j, 0x7f, 0x7f, 0x7f);
            }
        }
        
        if (!havePointsInViewport) continue;
        
        while (true)
        {
            if (index >= points->size()) break;
            K2 point = points->at(index);
            if (point.ri.time >= time) break;
            index++;
            if (m_isConfidence4Only && point.ri.confidence != 4) continue;
            
            float normAlt = point.alt / MAX_ALTITUDE;
            float y = (1.0 - normAlt) * (m_bitmapHeight - 1);
            
            unsigned char colorR, colorG, colorB;
            if (point.fuel == 0)
            {
                colorR = 0x7f;
                colorG = 0x7f;
                colorB = 0xff;
            }
            else
            {
                colorR = 0xff;
                colorG = 255 * (point.fuel / 100.0f);
                colorB = 255 * (point.fuel / 100.0f);
            }
            
            putPixel(i, y, colorR, colorG, colorB);
            if (point.ri.amplitude > m_boldThreshold)
            {
                putPixel(i, y - 1, colorR, colorG, colorB);
            }
        }
    }
    
    m_state->unlock();
}

void UvdBitmapGenerator::putPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    if (x >= m_bitmapWidth) return;
    if (y >= m_bitmapHeight) return;
    
    int pixelIndex = (x + y * m_bitmapWidth) * 4;
    
#ifndef BITMAP_BGR
    m_bitmap[pixelIndex] = r;
    m_bitmap[pixelIndex + 1] = g;
    m_bitmap[pixelIndex + 2] = b;
#else
    m_bitmap[pixelIndex] = b;
    m_bitmap[pixelIndex + 1] = g;
    m_bitmap[pixelIndex + 2] = r;
#endif
    m_bitmap[pixelIndex + 3] = 0xff;
}

void UvdBitmapGenerator::lock()
{
    MutexLock(&m_lock);
}

void UvdBitmapGenerator::unlock()
{
    MutexUnlock(&m_lock);
}
