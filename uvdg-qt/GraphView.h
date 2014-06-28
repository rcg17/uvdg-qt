
#ifndef __GRAPHVIEW_H__
#define __GRAPHVIEW_H__

#include <QImage>
#include <QTimer>
#include <QSound>
#include <QWidget>
#include "UvdState.h"
#include "UvdBitmapGenerator.h"

class GraphView : public QWidget
{
    Q_OBJECT

    UvdState *m_state;
    
    UvdBitmapGenerator *m_bitmapGenerator;
    QImage *m_image;

    double m_firstTime;
    double m_lastTime;
    double m_timeOffset;
    double m_timeSlice;
    bool m_isLineCrossEnabled;

    QPoint m_downPoint;
    QPoint m_hoverPoint;

    QRect m_knobRect;
    bool m_isDraggingKnob;
    int m_knobDragOffset;

    bool m_isNotificationShown;
    float m_notificationTimeLeft;
    QString m_notificationText;

    bool m_isStartingRealtimeMode;
    double m_realtimeMarkerTime;
    bool m_isLockedOnRealtimeMarker;

    qint64 m_lastTimeLocal;
    qint64 m_realtimeTickTimeLocal;
    qint64 m_lastUpdateTimeLocal;
    
    bool m_isBeepingEnabled;
    QSound *m_beep;
    bool m_shouldPlayBeep;
    qint64 m_lastBeepTimeLocal;
    
    bool m_isShowingStatusBox;
    
    QString m_connectionStatus;

    QTimer *m_timer;

    void showNotification(QString text);

    void putPixel(int x, int y, int r, int g, int b);
    void updateBitmap();

    QString timeString(double time);
    double screenLeftTime();
    double screenRightTime();
    double timeForX(int x);
    int altForY(int y);
    float xForTime(double time);
    void setTimeOffset(double timeOffset);
    float modifierMultiplier();
    void scrollToRealtimeMarker();
    void setConnectionStatus(QString status);

public:
    GraphView(UvdState *state);
    ~GraphView();

    void startRealtimeMode();
    void uvdStateChanged(double time);
    void tcpConnecting();
    void tcpConnected();
    void tcpReconnecting(int secondsToReconnect);
    void tcpDisconnected();

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

protected slots:
    void timerFired();

signals:
    void reconnectRequested();
    void disconnectRequested();

};

#endif
