
#include <QtWidgets>
#include <QMessageBox>
#include "datafileparser.h"

DataFileParser::DataFileParser(QFile *pDataFile)
{
    _pDataFile = pDataFile;
}

// Return false on error
bool DataFileParser::processDataFile(FileData * pData)
{
    QString line;
    bool bRet = true;

    /*-- Read until real data --*/
    qint32 state = 0; /* -1: error or end of file, 0: continue reading, 1 = start of data */
    do
    {
        bool bResult = readLineFromFile(&line);

        if (!bResult)
        {
            state = -1;
        }
        else if (line.size() == 1)
        {
            // Empty line, so skip line
        }
        else
        {
            if (line.left(2) == "//")
            {
                // Comment line, so skip line
            }
            else
            {
                // Ok first non comment and not empty line
                state = 1;
            }
        }
    } while (state == 0);


    // Next data line is already read

    if (state == -1)
    {        
        showError(tr("Invalid data file (while reading comments)"));
        bRet = false;
    }

    // Process register labels
    if (bRet)
    {
        pData->dataLabel.clear();
        pData->dataLabel.append(line.split(";"));
        _expectedFields = pData->dataLabel.size();
    }

#if 0
    // Read seconds line of data (register address)
    if (bRet)
    {
        if (readLineFromFile(&line))
        {
            pData->dataRegister.clear();
            pData->dataRegister.append(line.split(";"));
            if ((qint32)_expectedFields != pData->dataRegister.size())
            {
                showError(tr("Too many data header columns (while processing register line)"));
                bRet = false;
            }
        }
        else
        {
            showError(tr("Invalid data file (error while reading file)"));
            bRet = false;
        }
    }
#endif

    // read data
    if (bRet)
    {
        parseDataLines(pData->dataRows);
    }

    return bRet;
}

bool DataFileParser::parseDataLines(QList<QList<qint64> > & dataRows)
{
    QString line;
    bool bRet = true;
    bool bResult = true;

    // Read next line
    bResult = readLineFromFile(&line);

    // Init data row QLists to empty list
    QList<qint64> t;
    for (quint32 i = 0; i < _expectedFields; i++)
    {
        dataRows.append(t);
    }

    while (bResult && bRet)
    {
        QStringList paramList = line.split(";");

        if (paramList.size() != (qint32)_expectedFields)
        {
            bRet = false;
            break;
        }

        for (qint32 i = 0; i < paramList.size(); i++)
        {
            bool bError = false;
            const qint64 number = paramList[i].toLongLong(&bError);

            if (bError == false)
            {
                QString error = QString(tr("Invalid data (while processing data)\n Line:\"%1\"").arg(line));
                showError(error);
                bRet = false;
                break;
            }
            else
            {
                dataRows[i].append(number);
            }
        }

        // Check end of file
        if (_pDataFile->atEnd())
        {
            break;
        }

        // Read next line
        bResult = readLineFromFile(&line);

    }

    return bRet;
}

// Return false on error
bool DataFileParser::readLineFromFile(QString *pLine)
{
    bool bRet = false;
    char buf[512];
    qint32 lineLength;

    // Read first line of data (labels)
    lineLength = _pDataFile->readLine(buf, sizeof(buf));
    if (lineLength > 0)
    {
        bRet = true;
        *pLine = QString(buf);
    }

    return bRet;

}

void DataFileParser::showError(QString text)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("ModbusScope data file load error"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(text);
    msgBox.setStandardButtons(QMessageBox::Ok);
}

