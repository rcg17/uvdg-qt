
#include "GraphView.h"

#include <QtCore/QtCore>
#include <QtGui/QtGui>

#define OCCURRENCE_LANE_HEIGHT 15
#define OCCURRENCE_LANES_HEIGHT 76
#define OCCURRENCE_FIRST_LANE_OFFSET 1
#define TIME_SCROLLER_HEIGHT 20
#define NOTIFICATION_BOX_WIDTH 200
#define NOTIFICATION_BOX_HEIGHT 20
#define STATUS_BOX_WIDTH 400
#define STATUS_BOX_HEIGHT 20
#define NORM_REALTIME_MARKER_OFFSET 0.9

GraphView::GraphView(UvdState *state)
{
    m_state = state;

    m_bitmapGenerator = new UvdBitmapGenerator(state);
    m_image = NULL;

    m_state->lock();
    std::vector<K2> *points = m_state->points();
    if (points->size() > 0)
    {
        K2 firstPoint = points->at(0);
        m_firstTime = firstPoint.ri.time;
        K2 lastPoint = points->at(points->size() - 1);
        m_lastTime = lastPoint.ri.time;
    }
    else
    {
        m_firstTime = -1.0;
        m_lastTime = 0.0;
    }
    m_state->unlock();
    
    m_timeOffset = 0.0;
    m_timeSlice = 1.0;
    m_isLineCrossEnabled = false;

    m_downPoint = QPoint(-1, -1);
    m_isDraggingKnob = false;

    m_isNotificationShown = false;

    m_realtimeMarkerTime = -1.0;
    m_isLockedOnRealtimeMarker = false;

    m_isBeepingEnabled = true;
    m_beep = new QSound("beep.wav");
    m_shouldPlayBeep = false;
    m_lastBeepTimeLocal = 0;

    m_isShowingStatusBox = true;

    m_connectionStatus = QString("LOG ONLY");

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerFired()));
    m_timer->start(100);

    setWindowTitle("UVDG Timeline");

    setMouseTracking(true);
}

GraphView::~GraphView()
{
    delete m_bitmapGenerator;
    if (m_image != NULL) delete m_image;
    delete m_beep;
}

void GraphView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    //

    m_bitmapGenerator->lock();
    painter.drawImage(QRect(0, 0, m_image->width(), m_image->height()), *m_image);
    m_bitmapGenerator->unlock();

    //

    double leftTime = screenLeftTime();
    double rightTime = screenRightTime();

    //

    painter.fillRect(QRect(0, height() - OCCURRENCE_LANES_HEIGHT, width(), OCCURRENCE_LANES_HEIGHT), QColor(0, 0, 0));

    // rate calculator

    if (m_downPoint.x() >= 0 && !m_isDraggingKnob)
    {
        painter.setPen(QColor(255, 255, 0, 255));
        painter.drawLine(m_downPoint, m_hoverPoint);
        
        int deltaX = m_hoverPoint.x() - m_downPoint.x();
        float deltaTime = deltaX * m_timeSlice;
        
        int deltaY = m_downPoint.y() - m_hoverPoint.y();
        float deltaAlt = (deltaY / (float)m_image->height()) * MAX_ALTITUDE;
        
        QString rateString;
        rateString.sprintf("%.1f m/s", deltaAlt / deltaTime);
        painter.drawText(QPoint(m_hoverPoint.x() + 4, m_hoverPoint.y() - 4), rateString);
    }

    // occurrence lanes
    
    double pendingLastTimes[10];
    for (int i = 0; i < 10; i++) pendingLastTimes[i] = 0.0;

    m_state->lock();

    std::vector<OccurrenceRecord> *occurrences = m_state->occurrences();
    std::vector<OccurrenceRecord>::iterator iter;
    for (iter = occurrences->begin(); iter != occurrences->end(); ++iter)
    {
        OccurrenceRecord record = *iter;
    
        if ((record.lastTime > leftTime && record.lastTime < rightTime)
            || (record.firstTime > leftTime && record.firstTime < rightTime)
            || (record.firstTime < leftTime && record.lastTime > rightTime))
        {
            float firstX = (record.firstTime - leftTime) / m_timeSlice;
            float lastX = (record.lastTime - leftTime) / m_timeSlice;

            int n;
            for (int i = 0; i < 10; i++)
            {
                if (record.firstTime > pendingLastTimes[i])
                {
                    pendingLastTimes[i] = record.lastTime;
                    n = i;
                    break;
                }
            }
            
            painter.setPen(QColor(255, 255, 0));

            QRect rect(firstX, height() - ((n + 1) * OCCURRENCE_LANE_HEIGHT) - OCCURRENCE_FIRST_LANE_OFFSET, ceil(lastX) - firstX, OCCURRENCE_LANE_HEIGHT);
            painter.fillRect(rect, QColor(255, 255, 0, 50));

            QString tailNumberString;
            tailNumberString.sprintf("%05d", record.tailNumber);

            QRect textRect = rect.adjusted(1, 1, -1, -1);

            QRect trueTextRect;
            painter.setPen(QColor(0, 0, 0, 0));
            painter.drawText(textRect, 0, tailNumberString, &trueTextRect);
            bool textFitsRect = trueTextRect.width() < rect.width();

            QColor textColor;
            if (rect.contains(m_hoverPoint))
            {
                QRect stripeRect = QRect(firstX, 0, lastX - firstX, height());
                painter.fillRect(stripeRect, QColor(255, 255, 0, 25));

                painter.setPen(QColor(255, 255, 0, 255));
                painter.drawLine(firstX, 0, firstX, height());
                painter.drawLine(lastX, 0, lastX, height());
                
                if (!textFitsRect)
                {
                    textRect.setWidth(trueTextRect.width());
                }

                textColor = QColor(255, 255, 255, 255);
            } else {
                painter.setPen(QColor(255, 255, 0, 178));
                textColor = QColor(192, 192, 192, 255);
            }

            painter.drawRect(rect);

            painter.setPen(textColor);
            painter.drawText(textRect, textFitsRect ? Qt::AlignCenter : Qt::AlignLeft, tailNumberString);
        }
    }

    // scroller
    painter.setPen(QColor(255, 255, 255, 255));
    painter.drawRect(QRect(0, 0, width(), TIME_SCROLLER_HEIGHT));

    if (m_state->points()->size() > 1)
    {
        float normalizedLeftTime = (leftTime - m_firstTime) / (m_lastTime - m_firstTime);
        float normalizedRightTime = (rightTime - m_firstTime) / (m_lastTime - m_firstTime);
        int knobLeftX = normalizedLeftTime * width();
        int knobRightX = normalizedRightTime * width();

        m_knobRect = QRect(knobLeftX, 1, knobRightX - knobLeftX, TIME_SCROLLER_HEIGHT - 2);
        painter.drawRect(m_knobRect);
        painter.fillRect(m_knobRect, QColor(255, 255, 255, 96));
    }

    m_state->unlock();

    // ground

    painter.setPen(QColor(255, 0, 0));
    painter.drawLine(0, height() - OCCURRENCE_LANES_HEIGHT, width(), height() - OCCURRENCE_LANES_HEIGHT);

    // line cross

    if (m_isLineCrossEnabled && m_hoverPoint.y() < height()- OCCURRENCE_LANES_HEIGHT && !m_isDraggingKnob)
    {
        painter.setPen(QColor(127, 127, 127, 178));
        painter.drawLine(m_hoverPoint.x(), 0, m_hoverPoint.x(), height());
        painter.drawLine(0, m_hoverPoint.y(), width(), m_hoverPoint.y());
    }

    // hover alt/time

    painter.setPen(QColor(255, 255, 255, 255));

    double hoverTime = timeForX(m_hoverPoint.x());
    int hoverAlt = altForY(m_hoverPoint.y());
    QString altTimeString = QString("alt %1 time %2").arg(hoverAlt).arg(timeString(hoverTime));
    painter.drawText(QRect(0, 0, width(), 20), Qt::AlignCenter, altTimeString);

    // times

    painter.drawText(QRect(0, TIME_SCROLLER_HEIGHT + 4, 200, 20), Qt::AlignLeft, timeString(leftTime));
    painter.drawText(QRect(width() - 200, TIME_SCROLLER_HEIGHT + 4, 200, 20), Qt::AlignRight, timeString(rightTime));

    // notification

    if (m_isNotificationShown)
    {
        painter.setPen(QColor(127, 127, 255, 255));
        QRect notificationRect((width() - NOTIFICATION_BOX_WIDTH) / 2, (height() - NOTIFICATION_BOX_HEIGHT) / 2, NOTIFICATION_BOX_WIDTH, NOTIFICATION_BOX_HEIGHT);
        painter.drawRect(notificationRect);
        painter.fillRect(notificationRect, QColor(127, 127, 255, 127));

        painter.setPen(QColor(255, 255, 255, 255));
        painter.drawText(notificationRect, Qt::AlignCenter | Qt::AlignVCenter, m_notificationText);
    }

    // realtime line

    if (m_realtimeMarkerTime > 0.0)
    {
        float x = xForTime(m_realtimeMarkerTime);
        painter.drawLine(x, 0, x, height());
    }

    // status box

    if (m_isShowingStatusBox)
    {
        painter.setPen(QColor(255, 255, 255, 255));
        QRect statusBoxRect((width() - STATUS_BOX_WIDTH) / 2, TIME_SCROLLER_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
        painter.drawRect(statusBoxRect);       

        QString statusString;
        statusString.sprintf("%s | %s | %s | %s | %s | BOLD %d",
                            m_state->isRealtimeStarted() ? "RT" : "LOG",
                            m_connectionStatus.toUtf8().data(),
                            m_isLockedOnRealtimeMarker ? "LOCK" : "NO LOCK",
                            m_isBeepingEnabled ? "BEEP" : "NO BEEP",
                            m_bitmapGenerator->isConfidence4Only() ? "C4" : "C3+C4",
                            m_bitmapGenerator->boldThreshold());

        painter.drawText(statusBoxRect, Qt::AlignCenter | Qt::AlignVCenter, statusString);
    }
}

void GraphView::resizeEvent(QResizeEvent *event)
{
    m_bitmapGenerator->lock();
    if (m_image != NULL) delete m_image;
    m_image = new QImage(width(), height() - OCCURRENCE_LANES_HEIGHT, QImage::Format_RGB32);
    m_bitmapGenerator->setBitmap(m_image->bits(), m_image->width(), m_image->height());
    m_bitmapGenerator->unlock();

    if (m_isLockedOnRealtimeMarker)
    {
        scrollToRealtimeMarker();
    }

    updateBitmap();
}

void GraphView::mouseMoveEvent(QMouseEvent *event)
{
    m_hoverPoint = event->pos();

    if (m_isDraggingKnob)
    {
        double normalizedTimeOffset = (m_hoverPoint.x() - m_knobDragOffset) / (double)width();
        double timeOffset = (m_lastTime - m_firstTime) * normalizedTimeOffset;
        setTimeOffset(timeOffset);
        updateBitmap();
    }

    update();
}

void GraphView::mousePressEvent(QMouseEvent *event)
{
    m_downPoint = event->pos();

    if (m_knobRect.contains(m_downPoint))
    {
        m_isDraggingKnob = true;
        m_knobDragOffset = m_downPoint.x() - m_knobRect.x();
    }

    update();
}

void GraphView::mouseReleaseEvent(QMouseEvent *event)
{
    m_downPoint = QPoint(-1, -1);
    m_isDraggingKnob = false;
    update();
}

void GraphView::wheelEvent(QWheelEvent *event)
{
    int delta = event->delta();
    double timeOffset = m_timeOffset - delta * 0.05 * m_timeSlice * modifierMultiplier();
    setTimeOffset(timeOffset);
    
    updateBitmap();
}

void GraphView::keyReleaseEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key == Qt::Key_Space)
    {
        m_isLineCrossEnabled = !m_isLineCrossEnabled;

        update();
    }
    else if (key == Qt::Key_Up)
    {
        m_timeSlice += 1.0;
        if (m_timeSlice > 10.0) m_timeSlice = 10.0;
        
        if (m_isLockedOnRealtimeMarker)
        {
            scrollToRealtimeMarker();
        }

        updateBitmap();
    }
    else if (key == Qt::Key_Down)
    {
        m_timeSlice -= 1.0;
        if (m_timeSlice < 1.0) m_timeSlice = 1.0;
        
        if (m_isLockedOnRealtimeMarker)
        {
            scrollToRealtimeMarker();
        }

        updateBitmap();
    }
    else if (key == Qt::Key_Left)
    {
        double timeOffset = m_timeOffset - (10.0 * m_timeSlice * modifierMultiplier());
        setTimeOffset(timeOffset);
        
        updateBitmap();
    }
    else if (key == Qt::Key_Right)
    {
        double timeOffset = m_timeOffset + (10.0 * m_timeSlice * modifierMultiplier());
        setTimeOffset(timeOffset);
        
        updateBitmap();
    }
    else if (key == Qt::Key_C)
    {
        bool isConfidence4Only = !m_bitmapGenerator->isConfidence4Only();
        m_bitmapGenerator->setConfidence4Only(isConfidence4Only);

        QString text;
        text.sprintf("Show only confidence 4 points: %s.", isConfidence4Only ? "ON" : "OFF");
        showNotification(text);

        updateBitmap();
    }
    else if (key == Qt::Key_Q)
    {
        int boldThreshold = m_bitmapGenerator->boldThreshold() + 10;
        if (boldThreshold > 250) boldThreshold = 250;
        m_bitmapGenerator->setBoldThreshold(boldThreshold);

        QString text;
        text.sprintf("Bold threshold set to %d.", boldThreshold);
        showNotification(text);

        updateBitmap();
    }
    else if (key == Qt::Key_A)
    {
        int boldThreshold = m_bitmapGenerator->boldThreshold() - 10;
        if (boldThreshold < 0) boldThreshold = 0;
        m_bitmapGenerator->setBoldThreshold(boldThreshold);

        QString text;
        text.sprintf("Bold threshold set to %d.", boldThreshold);
        showNotification(text);

        updateBitmap();
    }
    else if (key == Qt::Key_L)
    {
        if (!m_isLockedOnRealtimeMarker && m_state->isRealtimeStarted())
        {
            m_isLockedOnRealtimeMarker = true;
            
            scrollToRealtimeMarker();
            
            showNotification("Locked on realtime marker.");
        
            updateBitmap();
        }
    }
    else if (key == Qt::Key_B)
    {
        m_isBeepingEnabled = !m_isBeepingEnabled;

        if (!m_isBeepingEnabled) m_shouldPlayBeep = false;

        QString text;
        text.sprintf("Beep on new points: %s.", m_isBeepingEnabled ? "ON" : "OFF");
        showNotification(text);

        update();
    }
    else if (key == Qt::Key_R)
    {
        emit reconnectRequested();
    }
    else if (key == Qt::Key_D)
    {
        emit disconnectRequested();
    }
    else if (key == Qt::Key_I)
    {
        m_isShowingStatusBox = !m_isShowingStatusBox;
        
        update();
    }
    else if (key == Qt::Key_F1)
    {
        QString text;
        text += "Left/right arrows : time offset\n";
        text += "Up/down arrows : time scale\n";
        text += "Space : toggle line cross\n";
        text += "Q/A : change bold threshold\n";
        text += "C : toggle 4 points only confidence\n";
        text += "L : lock on realtime marker\n";
        text += "B : toggle beep on new points\n";
        text += "R/D : resconnect/disconnect\n";
        text += "I : toggle status box\n";
        text += "\n";
        text += "When changing time offset: Shift increases scroll speed, Alt decreases.";

        QMessageBox::information(this, "UVDG help", text);
    }
    else
    {
        QWidget::keyReleaseEvent(event);
    }
}

void GraphView::showNotification(QString text)
{
    m_isNotificationShown = true;
    m_notificationTimeLeft = 2.0f;
    m_notificationText = text;
}

void GraphView::timerFired()
{
    if (m_notificationTimeLeft >= 0.0f)
    {
        m_notificationTimeLeft -= 0.1f;
    }
    else if (m_isNotificationShown)
    {
        m_isNotificationShown = false;
        update();
    }

    qint64 timeLocal = QDateTime::currentMSecsSinceEpoch();
    if (m_state->isRealtimeStarted() && timeLocal > m_realtimeTickTimeLocal)
    {
        m_realtimeTickTimeLocal = timeLocal + 1000;

        m_realtimeMarkerTime = m_lastTime + ((timeLocal - m_lastTimeLocal) / 1000.0);
        
        if (m_isLockedOnRealtimeMarker)
        {
            scrollToRealtimeMarker();
        }
        
        updateBitmap();
    }

    if (m_shouldPlayBeep && m_lastBeepTimeLocal + 2000 < timeLocal)
    {
        m_beep->play();
        m_shouldPlayBeep = false;
        m_lastBeepTimeLocal = timeLocal;
    }
}

void GraphView::updateBitmap()
{
    m_bitmapGenerator->lock();
    m_bitmapGenerator->update(screenLeftTime(), screenRightTime(), m_firstTime, m_lastTime, m_timeSlice);
    m_bitmapGenerator->unlock();

    m_lastUpdateTimeLocal = QDateTime::currentMSecsSinceEpoch();

    update();
}

QString GraphView::timeString(double time)
{
    if (time < 0.0) time = 0.0;

    int t = (int)time;
    int dd = t / 86400;
    t %= 86400;
    int hh = t / 3600;
    t %= 3600;
    int mm = t / 60;
    t %= 60;
    int ss = t;

    QString result;
    int year, month, day;
    if (m_state->getStartDate(&year, &month, &day))
    {
        QDate date;
        date.setDate(year, month, day);
        date.addDays(dd);

        result.sprintf("%04d-%02d-%02d / %02d:%02d:%02d", date.year(), date.month(), date.day(), hh, mm, ss);
    }
    else
    {
        result.sprintf("%d / %02d:%02d:%02d", dd, hh, mm, ss);
    }

    return result;
}

double GraphView::screenLeftTime()
{
    return m_firstTime + m_timeOffset;
}

double GraphView::screenRightTime()
{
    return screenLeftTime() + m_image->width() * m_timeSlice;
}

double GraphView::timeForX(int x)
{
    return (x * m_timeSlice) + screenLeftTime();
}

int GraphView::altForY(int y)
{
    int alt = (1.0f - (y / (float)m_image->height())) * MAX_ALTITUDE;
    return alt >= 0 ? alt : 0;
}

float GraphView::xForTime(double time)
{
    return (time - screenLeftTime()) / m_timeSlice;
}

void GraphView::setTimeOffset(double timeOffset)
{
    // do not allow negative time
    
    if (m_firstTime + timeOffset < 0.0)
    {
        m_timeOffset = -m_firstTime;
    }
    else
    {
        m_timeOffset = timeOffset;
    }
    
    if (m_isLockedOnRealtimeMarker)
    {
        m_isLockedOnRealtimeMarker = false;
        
        showNotification("Lock on realtime marker lost.");
    }
}

float GraphView::modifierMultiplier()
{
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    if (modifiers.testFlag(Qt::ShiftModifier))
    {
        return 5.0f;
    }

    if (modifiers.testFlag(Qt::AltModifier))
    {
        return 0.2f;
    }

    return 1.0f;
}

void GraphView::scrollToRealtimeMarker()
{
    m_timeOffset = m_realtimeMarkerTime - m_firstTime - (m_image->width() * NORM_REALTIME_MARKER_OFFSET) * m_timeSlice;
}

void GraphView::startRealtimeMode()
{
    m_isStartingRealtimeMode = true;
}

void GraphView::uvdStateChanged(double time)
{
    if (m_isStartingRealtimeMode)
    {
        m_isStartingRealtimeMode = false;
        m_isLockedOnRealtimeMarker = true;
    }

    if (m_firstTime < 0.0)
    {
        m_firstTime = time;
    }
    
    m_lastTime = time;
    m_realtimeMarkerTime = time;
    m_lastTimeLocal = QDateTime::currentMSecsSinceEpoch();
    m_realtimeTickTimeLocal = m_lastTimeLocal + 1000;
    
    if (m_isBeepingEnabled)
    {
        m_shouldPlayBeep = true;
    }
    
    if (m_isLockedOnRealtimeMarker)
    {
        scrollToRealtimeMarker();
    }
    
    if (m_lastUpdateTimeLocal + 500 > QDateTime::currentMSecsSinceEpoch())
    {
    }
    else
    {
        updateBitmap();
    }
}

void GraphView::setConnectionStatus(QString status)
{
    m_connectionStatus = status;
    update();
}

void GraphView::tcpConnecting()
{
    setConnectionStatus("CONN ...");
}

void GraphView::tcpConnected()
{
    setConnectionStatus("CONN OK");
}

void GraphView::tcpReconnecting(int secondsToReconnect)
{
    setConnectionStatus(QString("RECONN IN %1").arg(secondsToReconnect));
}

void GraphView::tcpDisconnected()
{
    setConnectionStatus("CONN OFF");
    showNotification("Disconnected.");
}
