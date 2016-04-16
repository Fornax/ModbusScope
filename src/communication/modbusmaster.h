#ifndef MODBUSMASTER_H
#define MODBUSMASTER_H

#include <QObject>
#include <QMap>
#include <QModbusTcpClient>

#include "modbusresult.h"

/* Forward declaration */
class SettingsModel;
class GuiModel;

class ModbusReadItem
{
public:
    ModbusReadItem(quint16 address, quint8 count)
    {
        _address = address;
        _count = count;
    }

    quint16 address(void) { return _address; }
    quint16 count(void) { return _count; }

private:
   quint16 _address;
   quint8 _count;

};

class ModbusMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusMaster(SettingsModel * pSettingsModel, GuiModel *pGuiModel, QObject *parent = 0);
    virtual ~ModbusMaster();

signals:
    void modbusPollDone(QMap<quint16, ModbusResult> modbusResults);
    void connectionFailed(QString errorString);
    void disconnectClient();

public slots:
    void readRegisterList(QList<quint16> registerList);

private slots:
    void handleConnectionStateChanged(QModbusDevice::State state);
    void handleReadDone();
    void handleCommunicationError(QModbusDevice::Error err);

private:

    void startConnection();
    void createReadList(QList<quint16> registerList);
    bool startReadRegisterCluster();

    QModbusTcpClient _client;
    QModbusReply * _pReply;

    QMap<quint16, ModbusResult> _resultMap;
    QList<ModbusReadItem> _readList;

    SettingsModel * _pSettingsModel;
    GuiModel * _pGuiModel;

};

#endif // MODBUSMASTER_H
