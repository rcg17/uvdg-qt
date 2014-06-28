
#ifndef __RXMAINWINDOW_H__
#define __RXMAINWINDOW_H__

#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#include "RtlUvdParser.h"
#include "UvdState.h"
#include "GraphView.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QSettings *m_settings;

    UvdState *m_state;
    RtlUvdParser *m_parser;

    QCheckBox *m_useLogSwitch;
    QPushButton *m_chooseLogFileButton;
    QLabel *m_logFilePathLabel;

    QCheckBox *m_useServerSwitch;
    QLineEdit *m_hostField;
    QLineEdit *m_portField;

    QPushButton *m_goButton;

    GraphView *m_graphView;

    QTcpSocket *m_socket;
    int m_reconnectDelay;
    int m_secondsToReconnect;
    QTimer *m_reconnectTimer;

    char *m_buffer;

    void updateControlsState(bool isLogEnabled, bool isServerEnabled);
    void stopReconnectTimer();
    void tcpConnect();
    void tcpReconnect();

public:
    MainWindow();
    ~MainWindow();

protected slots:
    void useLogSwitchAction(int state);
    void useServerSwitchAction(int state);
    void chooseLogFileAction();
    void goAction();

    void tcpConnected();
    void tcpHaveBytes();
    void tcpDisconnected();
    void tcpError(QAbstractSocket::SocketError);

    void reconnectTimerFired();

public slots:
    void requestReconnect();
    void requestDisconnect();
};

#endif