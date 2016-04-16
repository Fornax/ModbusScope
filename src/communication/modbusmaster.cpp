#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include "modbusmaster.h"
#include "settingsmodel.h"
#include "guimodel.h"

ModbusMaster::ModbusMaster(SettingsModel * pSettingsModel, GuiModel * pGuiModel, QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<ModbusResult>("ModbusResult");
    qRegisterMetaType<QMap<quint16, ModbusResult> >("QMap<quint16, ModbusResult>");

    _pSettingsModel = pSettingsModel;
    _pGuiModel = pGuiModel;

    connect(&_client, SIGNAL(stateChanged(QModbusDevice::State)), this, SLOT(handleConnectionStateChanged(QModbusDevice::State)));
    connect(&_client, SIGNAL(errorOccurred(QModbusDevice::Error)), this, SLOT(handleCommunicationError(QModbusDevice::Error)));
}

ModbusMaster::~ModbusMaster()
{

}

void ModbusMaster::createReadList(QList<quint16> registerList)
{

    // Clear list
    _readList.clear();

    // Create list of optimized reads
    qint32 regIndex = 0;
    while (regIndex < registerList.size())
    {
        quint32 count = 0;

        // get number of subsequent registers
        if (
                ((registerList.size() - regIndex) > 1)
                && (_pSettingsModel->consecutiveMax() > 1)
            )
        {
            bool bSubsequent;
            do
            {
                bSubsequent = false;

                // if next is current + 1, dan subsequent = true
                if (registerList.at(regIndex + count + 1) == registerList.at(regIndex + count) + 1)
                {
                    bSubsequent = true;
                    count++;
                }

                // Break loop when end of list
                if ((regIndex + count) >= ((uint)registerList.size() - 1))
                {
                    break;
                }

                // Limit number of register in 1 read
                if (count > (_pSettingsModel->consecutiveMax() - 1u - 1u))
                {
                    break;
                }

            } while(bSubsequent == true);
        }

        // At last one register
        count++;

        _readList.append(ModbusReadItem(registerList.at(regIndex), count));

        // Set register index to next register
        regIndex += count;
    }
}

bool ModbusMaster::startReadRegisterCluster()
{
    // Read next register cluster
    if (_readList.size() > 0)
    {
        QModbusDataUnit dataUnit(QModbusDataUnit::HoldingRegisters, _readList[0].address() - 40001,  _readList[0].count());

        _pReply = _client.sendReadRequest(dataUnit, _pSettingsModel->slaveId());

        connect(_pReply, SIGNAL(finished()), this, SLOT(handleReadDone()));

        return true;
    }
    else
    {
        // No registers left (should not happen)
        return false;
    }
}

void ModbusMaster::handleConnectionStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState)
    {
        // Connected: Make modbus work, start reading registers
        startReadRegisterCluster();
    }
}

void ModbusMaster::startConnection()
{
    _client.setTimeout( _pSettingsModel->timeout());
    _client.setNumberOfRetries(1);

    _client.setConnectionParameter(QModbusDevice::NetworkAddressParameter, _pSettingsModel->ipAddress());
    _client.setConnectionParameter(QModbusDevice::NetworkPortParameter, _pSettingsModel->port());

    if (_client.connectDevice() == false)
    {
        emit connectionFailed(_client.errorString());
    }
}

void ModbusMaster::handleReadDone()
{
    ModbusReadItem readCluster = _readList[0];
    quint32 success = 0;
    quint32 error = 0;

    _readList.removeFirst();

    if (_pReply->error() != QModbusDevice::NoError)
    {
        //Failure
        qDebug() << "Read error: " << _pReply->error() << ", " << _pReply->errorString();

        /* Log error */
        error++;

        if (readCluster.count() > 1)
        {
            // read every register separately
            for (qint32 idx = 0; idx < readCluster.count(); idx++)
            {
                _readList.insert(idx, ModbusReadItem(readCluster.address() + idx, 1));
            }

            qDebug() << "Group read failed: (" << _pReply->error() << "), " << _pReply->errorString();
        }
        else
        {
            // Register error
            qDebug() << "Single read failed: (" << _pReply->error() << "), " << _pReply->errorString();

            const ModbusResult result = ModbusResult(0, false);
            _resultMap.insert(readCluster.address(), result);
        }
    }
    else
    {
        // Success
        QModbusDataUnit dataUnit = _pReply->result();

        success++;
        for (uint i = 0; i < readCluster.count(); i++)
        {
            const quint16 registerAddr = readCluster.address() + i;
            const ModbusResult result = ModbusResult(dataUnit.value(i), true);
            _resultMap.insert(registerAddr, result);
        }
    }

    _pGuiModel->setCommunicationStats(_pGuiModel->communicationSuccessCount() + success, _pGuiModel->communicationErrorCount() + error);

    // Disconnect signals from reply object
    disconnect(_pReply, 0, 0, 0);

    _pReply->deleteLater();

    // Start next register cluster read
    if (startReadRegisterCluster() == false)
    {

        // Close connection
        _client.disconnectDevice();

        // No register to read
        emit modbusPollDone(_resultMap);
    }
}

void ModbusMaster::handleCommunicationError(QModbusDevice::Error err)
{
    if (err == QModbusDevice::ConnectionError)
    {
        emit connectionFailed(_client.errorString());
    }
}

void ModbusMaster::readRegisterList(QList<quint16> registerList)
{
    // Clear result map
    _resultMap.clear();

    // Create list of clusters of register to read
    createReadList(registerList);

    // Start communication
    startConnection();
}
