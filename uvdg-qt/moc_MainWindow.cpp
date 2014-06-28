/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "MainWindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MainWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      18,   12,   11,   11, 0x09,
      42,   12,   11,   11, 0x09,
      69,   11,   11,   11, 0x09,
      91,   11,   11,   11, 0x09,
     102,   11,   11,   11, 0x09,
     117,   11,   11,   11, 0x09,
     132,   11,   11,   11, 0x09,
     150,   11,   11,   11, 0x09,
     189,   11,   11,   11, 0x09,
     211,   11,   11,   11, 0x0a,
     230,   11,   11,   11, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_MainWindow[] = {
    "MainWindow\0\0state\0useLogSwitchAction(int)\0"
    "useServerSwitchAction(int)\0"
    "chooseLogFileAction()\0goAction()\0"
    "tcpConnected()\0tcpHaveBytes()\0"
    "tcpDisconnected()\0"
    "tcpError(QAbstractSocket::SocketError)\0"
    "reconnectTimerFired()\0requestReconnect()\0"
    "requestDisconnect()\0"
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MainWindow *_t = static_cast<MainWindow *>(_o);
        switch (_id) {
        case 0: _t->useLogSwitchAction((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->useServerSwitchAction((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->chooseLogFileAction(); break;
        case 3: _t->goAction(); break;
        case 4: _t->tcpConnected(); break;
        case 5: _t->tcpHaveBytes(); break;
        case 6: _t->tcpDisconnected(); break;
        case 7: _t->tcpError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 8: _t->reconnectTimerFired(); break;
        case 9: _t->requestReconnect(); break;
        case 10: _t->requestDisconnect(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MainWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow,
      qt_meta_data_MainWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MainWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
