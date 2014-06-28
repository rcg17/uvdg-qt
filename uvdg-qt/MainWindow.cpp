
#include "MainWindow.h"

MainWindow::MainWindow() : QMainWindow(NULL)
{
    m_settings = new QSettings("RCG17", "UVDG");

    if (!m_settings->contains("logFilePath")) m_settings->setValue("logFilePath", "");
    if (!m_settings->contains("serverHost")) m_settings->setValue("serverHost", "127.0.0.1");
    if (!m_settings->contains("serverPort")) m_settings->setValue("serverPort", "31003");

    setWindowTitle("UVDG");

    QWidget *centralWidget = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    centralWidget->setLayout(layout);

    setCentralWidget(centralWidget);

    m_useLogSwitch = new QCheckBox("Load log file", this);
    m_useLogSwitch->setChecked(true);
    connect(m_useLogSwitch, SIGNAL(stateChanged(int)), this, SLOT(useLogSwitchAction(int)));
    layout->addWidget(m_useLogSwitch);

    m_chooseLogFileButton = new QPushButton("Choose", this);
    connect(m_chooseLogFileButton, SIGNAL(clicked()), this, SLOT(chooseLogFileAction()));
    layout->addWidget(m_chooseLogFileButton);

    QString logFilePath = m_settings->value("logFilePath").toString();
    m_logFilePathLabel = new QLabel(logFilePath, this);
    layout->addWidget(m_logFilePathLabel);

    m_useServerSwitch = new QCheckBox("Connect to RTL-UVD", this);
    m_useServerSwitch->setChecked(true);
    connect(m_useServerSwitch, SIGNAL(stateChanged(int)), this, SLOT(useServerSwitchAction(int)));
    layout->addWidget(m_useServerSwitch);

    QLabel *hostLabel = new QLabel("Host:", this);
    layout->addWidget(hostLabel);

    m_hostField = new QLineEdit(this);
    m_hostField->setText(m_settings->value("serverHost").toString());
    layout->addWidget(m_hostField);

    QLabel *portLabel = new QLabel("Port:", this);
    layout->addWidget(portLabel);

    m_portField = new QLineEdit(this);
    m_portField->setText(m_settings->value("serverPort").toString());
    layout->addWidget(m_portField);

    m_goButton = new QPushButton("GO", this);
    connect(m_goButton, SIGNAL(clicked()), this, SLOT(goAction()));
    layout->addWidget(m_goButton);

    m_reconnectDelay = 0;
    m_secondsToReconnect = 1;
    m_reconnectTimer = NULL;

    m_buffer = (char *)malloc(256);
}

MainWindow::~MainWindow()
{
    delete m_settings;

    free(m_buffer);

    if (m_socket != NULL)
    {
        m_socket->disconnect();
        m_socket->abort();
        delete m_socket;
    }

    stopReconnectTimer();
}

void MainWindow::useLogSwitchAction(int state)
{
    bool isLogEnabled = state;
    bool isServerEnabled = m_useServerSwitch->isChecked();

    updateControlsState(isLogEnabled, isServerEnabled);
}

void MainWindow::useServerSwitchAction(int state)
{
    bool isLogEnabled = m_useLogSwitch->isChecked();
    bool isServerEnabled = state;

    updateControlsState(isLogEnabled, isServerEnabled);
}

void MainWindow::chooseLogFileAction()
{
    QString fileName = QFileDialog::getOpenFileName(NULL, "Open File",
                                                     "",
                                                     "Files (*.*)");

    if (fileName.size() == 0) return;

    m_settings->setValue("logFilePath", fileName);

    m_logFilePathLabel->setText(fileName);
}

void MainWindow::goAction()
{
    m_goButton->setEnabled(false);

    m_state = new UvdState();
    m_parser = new RtlUvdParser(m_state);

    if (m_useLogSwitch->isChecked())
    {
        QString logFileName = m_logFilePathLabel->text().section('/', -1);
        int yyyy, mm, dd;
        int gotTokens = sscanf(logFileName.toUtf8().data(), "rtl-uvd-log-%04d-%02d-%02d", &yyyy, &mm, &dd);
        if (gotTokens == 3)
        {
            m_state->setStartDate(yyyy, mm, dd);
        }

        m_parser->parseLogFile(m_logFilePathLabel->text().toUtf8().data());
    }

    m_graphView = new GraphView(m_state);
    m_graphView->show();

    if (m_useServerSwitch->isChecked())
    {
        m_state->startRealtimeMode();
        m_graphView->startRealtimeMode();

        connect(m_graphView, SIGNAL(reconnectRequested()), this, SLOT(requestReconnect()));
        connect(m_graphView, SIGNAL(disconnectRequested()), this, SLOT(requestDisconnect()));

        tcpConnect();

        m_settings->setValue("serverHost", m_hostField->text());
        m_settings->setValue("serverPort", m_portField->text());
    }

    hide();
}

void MainWindow::updateControlsState(bool isLogEnabled, bool isServerEnabled)
{
    m_chooseLogFileButton->setEnabled(isLogEnabled);
    
    m_hostField->setEnabled(isServerEnabled);
    m_portField->setEnabled(isServerEnabled);
    
    m_goButton->setEnabled(isLogEnabled || isServerEnabled);
}

void MainWindow::stopReconnectTimer()
{
    if (m_reconnectTimer != NULL)
    {
        delete m_reconnectTimer;
        m_reconnectTimer = NULL;
    }
}

void MainWindow::tcpConnected()
{
    m_reconnectDelay = 1;

    stopReconnectTimer();
    
    m_graphView->tcpConnected();
}

void MainWindow::tcpHaveBytes()
{
    while (m_socket->canReadLine())
    {
        m_socket->readLine(m_buffer, 256);
        double lineTime = m_parser->processLine(m_buffer);
        if (lineTime > 0.0)
        {
            m_graphView->uvdStateChanged(lineTime);
        }
    }
}

void MainWindow::tcpDisconnected()
{
    tcpReconnect();
}

void MainWindow::tcpError(QAbstractSocket::SocketError error)
{
    tcpReconnect();
}

void MainWindow::tcpConnect()
{
    if (m_reconnectDelay < 30)
    {
        m_reconnectDelay += 1;
    }

    stopReconnectTimer();

    m_graphView->tcpConnecting();

    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(connected()), this, SLOT(tcpConnected()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(tcpHaveBytes()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(tcpError(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(tcpDisconnected()));

    m_socket->connectToHost(m_hostField->text(), m_portField->text().toInt());
}

void MainWindow::tcpReconnect()
{
    m_graphView->tcpReconnecting(m_reconnectDelay);

    m_secondsToReconnect = m_reconnectDelay;

    if (m_socket != NULL)
    {
        m_socket->disconnect();
        m_socket->deleteLater();
        m_socket = NULL;
    }

    stopReconnectTimer();

    m_reconnectTimer = new QTimer(this);
    connect(m_reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnectTimerFired()));
    m_reconnectTimer->start(1000);
}

void MainWindow::reconnectTimerFired()
{
    m_secondsToReconnect -= 1;
    if (m_secondsToReconnect == 0)
    {
        tcpConnect();
    }
    else
    {
        m_graphView->tcpReconnecting(m_secondsToReconnect);
    }
}

void MainWindow::requestReconnect()
{
    m_reconnectDelay = 0;
    tcpConnect();
}

void MainWindow::requestDisconnect()
{
    stopReconnectTimer();

    if (m_socket != NULL)
    {
        m_socket->disconnect();
        m_socket->deleteLater();
        m_socket = NULL;
    }

    m_graphView->tcpDisconnected();
}
