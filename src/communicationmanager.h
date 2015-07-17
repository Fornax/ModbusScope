#ifndef COMMUNICATION_MANAGER_H
#define COMMUNICATION_MANAGER_H

#include <QObject>
#include <QList>
#include <QStringListModel>

#include "connectionmodel.h"
#include "registerdata.h"


//Forward declaration
class GuiModel;
class LogModel;
class ModbusMaster;
class QTimer;

class CommunicationManager : public QObject
{
    Q_OBJECT
public:
    explicit CommunicationManager(ConnectionModel * pConnectionModel, GuiModel * pGuiModel, LogModel * pLogModel, QObject *parent = 0);
    ~CommunicationManager();

    bool startCommunication(QList<RegisterData> registers);
    void stopCommunication();

    bool isActive();
    void resetCommunicationStats();

public slots:
    void handlePollDone(QList<bool> successList, QList<quint16> values);

signals:
    void registerRequest(QList<quint16> registerList);
    void requestStop();
    void handleReceivedData(QList<bool> successList, QList<double> values);

private slots:
    void readData();
    void masterStopped();

private:   

    ModbusMaster * _master;
    bool _active;
    QTimer * _pPollTimer;
    qint64 _lastPollStart;

    GuiModel * _pGuiModel;
    ConnectionModel * _pConnectionModel;
    LogModel * _pLogModel;
    QList<RegisterData> _registerlist;
};

#endif // COMMUNICATION_MANAGER_H