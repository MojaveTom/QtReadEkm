/*!
@file
@brief Main program to read EKM meters and save response in database.
@author Thomas A. DeMay
@date 2015
@par    Copyright (C) 2015  Thomas A. DeMay
@par
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    any later version.
@par
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
@par
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <QCoreApplication>
#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtSql>
#include <QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <unistd.h>
#include <stdlib.h>

#include "../SupportRoutines/supportfunctions.h"
#include "messages.h"

/* ********  Global variable declarations  ***************/
QTimeZone LocalTimeZone = QTimeZone(QTimeZone::systemTimeZoneId());
QTimeZone LocalStandardTimeZone = QTimeZone(LocalTimeZone.standardTimeOffset(QDateTime::currentDateTime()));


/* ********  Global function declarations  ***************/
bool ConnectSerial(const QString &serialDeviceName, QSerialPort **serialPortPtr);
bool SaveV3ResponseToDatabase(const ResponseV3Data &responseData);
bool SaveV4ResponseToDatabase(const uint8_t responseType, const ResponseV4Generic &response);
bool GetMeterV4Data(QSerialPort *serialPort, const QString meterId, const uint8_t requestType, ResponseV4Generic *response);
bool GetMeterV3Data(QSerialPort *serialPort, const QString meterId, ResponseV3Data *response);
bool ReadResponse(QSerialPort *serialPort, qint8 *msg, const qint64 msgSize);
bool WriteSerialMsg(QSerialPort *serialPort, const char *msg, const qint64 msgSize);
bool ValidateCRC(const uint8_t *msg, int numBytes);
bool SendControl(QSerialPort *serialPort, QString &meterId, OutputControlDef *ctrlMsg, const int msgSize);
bool SetMeterTime(QSerialPort *serialPort, QString &meterId);
bool InitializeMeters(QSerialPort *serialPort, const QStringList &args);
uint16_t computeEkmCrc(const uint8_t *dat, uint16_t len);

/* **********  Global function definitions   *************/

/*!
 * \brief InitializeMeters -- Once per program execution actions.
 *
 * Opens a connection to the database.
 *
 * For each meter in the args list, check to see if there is a table in the
 * database for the response from the meter.  If not, create the table.
 *
 * Each meter has its time set to the local standard time.
 *
 * \param serialPort    Pointer to serial port for communication with meter.
 * \param args  List of meter ids from command line.
 * \return true if successful, false otherwise.
 */
bool InitializeMeters(QSerialPort *serialPort, const QStringList &args)
{
    qDebug("Begin");
    QSqlDatabase dbConn = QSqlDatabase::database(ConnectionName);

    if (!dbConn.isOpen())
    {
        qCritical() << "Unable to open database for meter data.";
        qInfo() << "Return false";
        return false;
    }
    QSqlQuery query(dbConn);

    /*! Do meter initialization tasks for each meter.  */
    foreach (QString meterId, args)
    {
        //! Expand meter id to all 12 characters.
        QString fullMeterId = meterId.rightJustified(sizeof(RequestMsgV4.meterId), '0', true);

        //!  Set the time in the meter to the computer's idea of the local standard time.
        if (!SetMeterTime(serialPort, fullMeterId))
        {
            qWarning("Unable to set meter time for meter %s.", qUtf8Printable(fullMeterId));
        }

        //! Check for the existence of database tables in which to store meter data.

        if (fullMeterId.toLongLong() >= 300000000)
        {
            // Is a v.4 meter.
            // Create the table if it doesn't exist.
            if (!query.exec(QString("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = '%1_A_RawMeterData'").arg(fullMeterId)))
            {
                qCritical("Unable to access INFORMATION_SCHEMA; assume data tables exist.");
                qInfo("    Query was: %s", qUtf8Printable(query.lastQuery()));
                qInfo("    Error was: %s", qUtf8Printable(query.lastError().text()));
            }
            else
            {
                qDebug("Successfully accessed INFORMATION_SCHEMA for query about existence of v.4 A data table.");
                if (!query.next())
                {
                    // the query returned no results => table does not exist.  Create it.
                    if (!query.exec(QString("CREATE TABLE `%1_A_RawMeterData` ("
                                            "`idRawMeterData` int(11) NOT NULL AUTO_INCREMENT,"
                                            "`ComputerTime` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) COMMENT 'Time that record written to database.',"
                                            "`MeterTime` datetime DEFAULT NULL COMMENT 'Meter time from response message.',"
                                            "`MeterId` varchar(12) DEFAULT NULL COMMENT 'Meter ID (Serial number) from response.',"
                                            "`MeterType` varchar(4) DEFAULT NULL,"
                                            "`DataType` varchar(4) DEFAULT NULL COMMENT 'Either \"V3\", \"V4A\" or \"V4B\"',"
                                            "`MeterData` binary(255) NOT NULL COMMENT 'Exact copy of entire response data from meter.',"
                                            "PRIMARY KEY (`idRawMeterData`),"
                                            "UNIQUE KEY `idRawMeterData_UNIQUE` (`idRawMeterData`)"
                                            ") ENGINE=InnoDB AUTO_INCREMENT=8281 DEFAULT CHARSET=utf8").arg(fullMeterId)))
                    {
                        qCritical("Unable to create RawMeterData A table for %s meter.  Assume table already exists.", qUtf8Printable(fullMeterId));
                        qInfo("    Query was: %s", qUtf8Printable(query.lastQuery()));
                        qInfo("    Error was: %s", qUtf8Printable(query.lastError().text()));
                    }
                    else
                    {
                        qInfo("Successfully created Table %s_A_RawMeterData", qUtf8Printable(fullMeterId));
                    }
                }
                qDebug("Table %s_A_RawMeterData exists.", qUtf8Printable(fullMeterId));
            }
            if (!query.exec(QString("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = '%1_B_RawMeterData'").arg(fullMeterId)))
            {
                qCritical("Unable to access INFORMATION_SCHEMA; assume data tables exist.");
                qInfo("    Query was: %s", qUtf8Printable(query.lastQuery()));
                qInfo("    Error was: %s", qUtf8Printable(query.lastError().text()));
            }
            else
            {
                qDebug("Successfully accessed INFORMATION_SCHEMA for query about existence of .v4 B data table.");
                if (!query.next())
                {
                    // the query returned no results => table does not exist.  Create it.
                    if (!query.exec(QString("CREATE TABLE `%1_B_RawMeterData` ("
                                            "`idRawMeterData` int(11) NOT NULL AUTO_INCREMENT,"
                                            "`ComputerTime` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) COMMENT 'Time that record written to database.',"
                                            "`MeterTime` datetime DEFAULT NULL COMMENT 'Meter time from response message.',"
                                            "`MeterId` varchar(12) DEFAULT NULL COMMENT 'Meter ID (Serial number) from response.',"
                                            "`MeterType` varchar(4) DEFAULT NULL,"
                                            "`DataType` varchar(4) DEFAULT NULL COMMENT 'Either \"V3\", \"V4A\" or \"V4B\"',"
                                            "`MeterData` binary(255) NOT NULL COMMENT 'Exact copy of entire response data from meter.',"
                                            "PRIMARY KEY (`idRawMeterData`),"
                                            "UNIQUE KEY `idRawMeterData_UNIQUE` (`idRawMeterData`)"
                                            ") ENGINE=InnoDB AUTO_INCREMENT=8281 DEFAULT CHARSET=utf8").arg(fullMeterId)))
                    {
                        qCritical("Unable to create RawMeterData B table for %s meter.  Assume table already exists.", qUtf8Printable(fullMeterId));
                        qInfo("    Query was: %s", qUtf8Printable(query.lastQuery()));
                        qInfo("    Error was: %s", qUtf8Printable(query.lastError().text()));
                    }
                    else
                    {
                        qInfo("Successfully created Table %s_B_RawMeterData", qUtf8Printable(fullMeterId));
                    }
                }
                qDebug("Table %s_B_RawMeterData exists.", qUtf8Printable(fullMeterId));
            }
        }
        else
        {
            // Is a v.3 meter.
            // Create the table if it doesn't exist.
            if (!query.exec(QString("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = '%1_RawMeterData'").arg(fullMeterId)))
            {
                qCritical("Unable to access INFORMATION_SCHEMA; assume data tables exist.");
                qInfo("    Query was: %s", qUtf8Printable(query.lastQuery()));
                qInfo("    Error was: %s", qUtf8Printable(query.lastError().text()));
            }
            else
            {
                qDebug("Successfully accessed INFORMATION_SCHEMA for query about existence of .v3 data table.");
                if (!query.next())
                {
                    // the query returned no results => table does not exist.  Create it.
                    if (!query.exec(QString("CREATE TABLE `%1_RawMeterData` ("
                                            "`idRawMeterData` int(11) NOT NULL AUTO_INCREMENT,"
                                            "`ComputerTime` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) COMMENT 'Time that record written to database.',"
                                            "`MeterTime` datetime DEFAULT NULL COMMENT 'Meter time from response message.',"
                                            "`MeterId` varchar(12) DEFAULT NULL COMMENT 'Meter ID (Serial number) from response.',"
                                            "`MeterType` varchar(4) DEFAULT NULL,"
                                            "`DataType` varchar(4) DEFAULT NULL COMMENT 'Either \"V3\", \"V4A\" or \"V4B\"',"
                                            "`MeterData` binary(255) NOT NULL COMMENT 'Exact copy of entire response data from meter.',"
                                            "PRIMARY KEY (`idRawMeterData`),"
                                            "UNIQUE KEY `idRawMeterData_UNIQUE` (`idRawMeterData`)"
                                            ") ENGINE=InnoDB AUTO_INCREMENT=8281 DEFAULT CHARSET=utf8").arg(fullMeterId)))
                    {
                        qCritical("Unable to create v.3 RawMeterData table for %s meter.  Assume table already exists.", qUtf8Printable(fullMeterId));
                        qInfo("    Query was: %s", qUtf8Printable(query.lastQuery()));
                        qInfo("    Error was: %s", qUtf8Printable(query.lastError().text()));
                    }
                    else
                    {
                        qInfo("Successfully created Table %s_RawMeterData", qUtf8Printable(fullMeterId));
                    }
                }
                qDebug("Table %s_RawMeterData exists.", qUtf8Printable(fullMeterId));
            }
        }
    }
    return true;
}

/*!
 * \brief ConnectSerial -- Create connection to serial device.
 * \param serialDeviceName  Name of serial device.
 * \param serialPortPtr     Pointer to pointer to serial port.
 * \return true if successful, false otherwise.
 */
bool ConnectSerial(const QString &serialDeviceName, QSerialPort **serialPortPtr)
{
    qInfo("Begin");
    if (serialPortPtr == NULL)
    {
        qDebug() << "Connect Serial called with serialPortPtr NULL.";
        qDebug() << "Return false";
        return false;
    }
    QSerialPort *serialPort = *serialPortPtr;
    /*!  If the serial port we're given is already connected to something,
     * close the connection.
     */
    if (serialPort != NULL)
    {
        serialPort->flush();
        serialPort->close();
        delete serialPort;
        serialPort = NULL;
        *serialPortPtr = serialPort;
    }
    /*! Get information about the serial device. */
    const QSerialPortInfo info(serialDeviceName);
    if (info.isNull())
    {
        qDebug("Serial port info for %s is null AND it is %s", qUtf8Printable(serialDeviceName), info.isBusy()?"busy.":"not busy.");
        qInfo() << "Available serial ports are:";
        QList<QSerialPortInfo> portList(QSerialPortInfo::availablePorts());
        foreach (QSerialPortInfo pl, portList) {
            qInfo() << "   " << pl.portName() << pl.description() << pl.systemLocation();
        }
        qInfo() << "Return false";
        return false;
    }
    qDebug() << "Got port info for port" << info.portName();
    if (info.isBusy())
    {
        qDebug() << serialDeviceName << "is busy";
        qInfo() << "Available serial ports are:";
        QList<QSerialPortInfo> portList(QSerialPortInfo::availablePorts());
        foreach (QSerialPortInfo pl, portList) {
            qInfo() << "   " << pl.portName() << pl.description() << pl.systemLocation();
        }
        qInfo() << "Return false";
        return false;
    }
    qDebug() << info.portName() << "is supposedly not busy.";
    QString s = QObject::tr("Port: ") + info.portName() + "\n"
            + QObject::tr("Location: ") + info.systemLocation() + "\n"
            + QObject::tr("Description: ") + info.description() + "\n"
            + QObject::tr("Manufacturer: ") + info.manufacturer() + "\n"
            + QObject::tr("Serial number: ") + info.serialNumber() + "\n"
            + QObject::tr("Vendor Identifier: ") + (info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString()) + "\n"
            + QObject::tr("Product Identifier: ") + (info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16) : QString()) + "\n"
            + QObject::tr("Busy: ") + (info.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) + "\n";

    qDebug() << (s);
    serialPort = new QSerialPort(info);
    qDebug() << "SerialPort is:" << serialPort;
    s = "serialPort:    baudRate:  " + QString::number(serialPort->baudRate())
            + "    dataBits:  " + QString::number(serialPort->dataBits())
            + "    flowControl:  " + QString::number(serialPort->flowControl())
            + "    parity:  " + QString::number(serialPort->parity())
            + "    stop bits:  " + QString::number(serialPort->stopBits())
            ;
    qDebug() << (s);
    /*! Set the serial port characteristics to EKM specs. */
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->setBaudRate(9600);
    serialPort->setDataBits(QSerialPort::Data7);
    serialPort->setParity(QSerialPort::EvenParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    if (!serialPort->open(QIODevice::ReadWrite))
    {
        qCritical("Could not open %s:  %s", qPrintable(info.portName()), qPrintable(serialPort->errorString()));
        qInfo() << "Available serial ports are:";
        QList<QSerialPortInfo> portList(QSerialPortInfo::availablePorts());
        foreach (QSerialPortInfo pl, portList) {
            qInfo() << "   " << pl.portName() << pl.description() << pl.systemLocation();
        }
    }
    else
    {
        qInfo() << (info.portName() + "  successfully opened.");
        s = "serialPort:    baudRate:  " + QString::number(serialPort->baudRate())
                + "    dataBits:  " + QString::number(serialPort->dataBits())
                + "    flowControl:  " + QString::number(serialPort->flowControl())
                + "    parity:  " + QString::number(serialPort->parity())
                + "    stop bits:  " + QString::number(serialPort->stopBits())
                ;
        qDebug() << (s);
        *serialPortPtr = serialPort;
    }
    qInfo() << "Return" << serialPort->isOpen();
    return serialPort->isOpen();
}

/*!
 * \brief SaveV3ResponseToDatabase -- Store a v.3 meter response in its database table.
 * \param responseData  Data from v.3 meter to save.
 * \return true if successful, false otherwise.
 */
bool SaveV3ResponseToDatabase(const ResponseV3Data &responseData)
{
    /*
     *  Assumes responseData is valid ResponseV3Data.
     */
    /*! Extract critical pieces of info from response. */
    QVariant meterTime = QDateTime(
                QDate(  QByteArray((char *)responseData.dateTime.year, 2).toInt() + 2000
                        , QByteArray((char *)responseData.dateTime.month, 2).toInt()
                        , QByteArray((char *)responseData.dateTime.day, 2).toInt())
                , QTime(QByteArray((char *)responseData.dateTime.hour, 2).toInt()
                        , QByteArray((char *)responseData.dateTime.minute, 2).toInt()
                        , QByteArray((char *)responseData.dateTime.second, 2).toInt()));
    QVariant meterId = QString(QByteArray((char *)responseData.meterId, sizeof(responseData.meterId)));
    QVariant meterType = QString(QByteArray((char *)responseData.model, 2).toHex());
    QVariant meterData = QByteArray((char *)responseData.fixed02, sizeof(responseData)); //!< Gets the response into a byte array.
    QVariant dataType = "V3";

    QSqlDatabase dbConn = QSqlDatabase::database(ConnectionName);

    if (!dbConn.isOpen())
    {
        qCritical() << "Unable to open database to save solar data.";
        qInfo() << "Return false";
        return false;
    }
    QSqlQuery query(dbConn);

    query.prepare("INSERT INTO RawMeterData (MeterTime, MeterId, DataType, MeterType, MeterData) VALUES (?, ?, ?, ?, ?)");
    query.bindValue(0, meterTime);
    query.bindValue(1, meterId);
    query.bindValue(2, meterType);
    query.bindValue(3, dataType);
    query.bindValue(4, meterData);
    if (!DontActuallyWriteDatabase)
    {
        if (!query.exec())
        {
            qCritical() << "Error inserting raw meter data record in database: " << query.lastError() << "\nQuery: " << query.lastQuery();
            qInfo() << "Return false";
            return false;
        }
        else
            qDebug() << "Inserting raw meter data was successful.";
    }
    else
    {
        qDebug() << "Did not execute " << query.lastQuery();
    }
    qInfo() << "Return true";
    return true;
}

/*!
 * \brief SaveV4ResponseToDatabase
 * \param responseType x30 if response A, otherwise B.
 * \param response      Data from meter.
 * \return true if successful, false otherwise.
 */
bool SaveV4ResponseToDatabase(const uint8_t responseType, const ResponseV4Generic &response)
{
    /*
     *  Assumes responseData is valid ResponseV4Data.
     */
    QString meterTable;
    QVariant dataType;
    QVariant meterTime = QDateTime(
                QDate(  QByteArray((char *)response.responseV4Generic.dateTime.year, 2).toInt() + 2000
                        , QByteArray((char *)response.responseV4Generic.dateTime.month, 2).toInt()
                        , QByteArray((char *)response.responseV4Generic.dateTime.day, 2).toInt())
                , QTime(QByteArray((char *)response.responseV4Generic.dateTime.hour, 2).toInt()
                        , QByteArray((char *)response.responseV4Generic.dateTime.minute, 2).toInt()
                        , QByteArray((char *)response.responseV4Generic.dateTime.second, 2).toInt()));
    QVariant meterId = QString(QByteArray((char *)response.responseV4Generic.meterId, sizeof(response.responseV4Generic.meterId)));
    QVariant meterType = QString(QByteArray((char *)response.responseV4Generic.model, 2).toHex());
    QVariant meterData = QByteArray((char *)response.responseV4Generic.fixed02, sizeof(response));
    if (responseType == '\x30')
    {
        meterTable = meterId.toString() + "_A_RawMeterData";
        dataType = "V4A";
    }
    else
    {
        meterTable = meterId.toString() + "_B_RawMeterData";
        dataType = "V4B";
    }
    qDebug("Response meterTime %s, meterId %s, meterType %s, dataType %s"
           , qUtf8Printable(meterTime.toString())
           , qUtf8Printable(meterId.toString())
           , qUtf8Printable(meterType.toString())
           , qUtf8Printable(dataType.toString()));

    QSqlDatabase dbConn = QSqlDatabase::database(ConnectionName);

    if (!dbConn.isOpen())
    {
        qCritical() << "Unable to open database to save solar data.";
        qInfo() << "Return false";
        return false;
    }
    QSqlQuery query(dbConn);

    query.prepare(QString("INSERT INTO %1 (MeterTime, MeterId, MeterType, DataType, MeterData) VALUES (?, ?, ?, ?, ?)").arg(meterTable));
    query.bindValue(0, meterTime);
    query.bindValue(1, meterId);
    query.bindValue(2, meterType);
    query.bindValue(3, dataType);
    query.bindValue(4, meterData);
    if (!DontActuallyWriteDatabase)
    {
        if (!query.exec())
        {
            qCritical("Error inserting raw meter %02x data record in database: %s\n Query:  %s"
                      , responseType
                      , qUtf8Printable(query.lastError().text())
                      , qUtf8Printable(query.lastQuery()));
            qInfo() << "Return false";
            return false;
        }
        else
            qDebug("Inserting raw meter %s data was successful.", qUtf8Printable(dataType.toString()));
    }
    else
    {
        qDebug() << "Did not execute " << query.lastQuery();
    }
    qInfo() << "Return true";
    return true;
}

/*!
 * \brief WriteSerialMsg -- Write a message to the meter.
 * \param serialPort  Serial port to use.
 * \param msg       Message to send.
 * \param msgSize   Size of message to send.
 * \return true if successful, false otherwise.
 */
bool WriteSerialMsg(QSerialPort *serialPort, const char *msg, const qint64 msgSize)
{
    qDebug() << "Begin";
    qint64 bytesWritten = 0;
    qInfo("msg is: %s", qUtf8Printable(QByteArray(msg, msgSize).toHex()));
    bytesWritten = serialPort->write(msg, msgSize);

    qDebug() << bytesWritten << "of" << msgSize << "bytes of msg written.";
    bool writeSuccess = serialPort->waitForBytesWritten(10000);
    if (!writeSuccess || (bytesWritten != msgSize))
    {
        qWarning() << "Unable to write message.  Wait status =" << writeSuccess;
        qInfo() << "Return false";
        return false;
    }
    qInfo() << "Successfully wrote message.";
    qDebug() << "Return true";
    return true;
}

/*!
 * \brief ReadResponse -- Read a response from the meter.
 *
 * Response is put into a character array so that it can have
 * structure imposed on it for ease of intreptation.
 *
 * \param serialPort  Serial port to read.
 * \param msg   Pointer to character array in which to put response.
 * \param msgSize   Expected size of response.
 *   msg array must be large enough to accomodate this many characters.
 * \return true if successful, false otherwise.
 */
bool ReadResponse(QSerialPort *serialPort, qint8 *msg, const qint64 msgSize)
{
    FlushDiagnostics();
    /* Install the saveMessageOutput message handler so that timing considerations
     * will not be impacted by terminal output.  Restore message handler before returning.
     */
    QtMessageHandler prevMsgHandler = qInstallMessageHandler(saveMessageOutput);
    qDebug() << "Begin";
    qDebug() << serialPort;
    qDebug() << "The message address is" << msg;
    qDebug() << msgSize << "is the size the response.";
    static const int maxTries = 10;
    qint64 bytesRead = 0, bytesThisRead = 0, prevBytesAvail = 0, bytesAvail = 0;
    QByteArray readData;
    int tryReadCount = 0;
    while (bytesRead < msgSize)
    {
        qDebug() << "Wait for ready read 10 sec.";
        if (!serialPort->waitForReadyRead(10000))
        {
            qWarning() << "Read timeout waiting for message";
            qInstallMessageHandler(prevMsgHandler);
            qDebug() << "Return false";
            return false;
        }
        qDebug() << "waitForReadyRead(10000) returned true.";
        bytesAvail = serialPort->bytesAvailable();
        while ((bytesAvail < msgSize) && (bytesAvail > prevBytesAvail))
        {
            /*! Compute the number of microsec to wait to get the rest of the message.
             *  Some extra time in the form of 12 extra characters is allowed.
             *  960 = characters per second at 9600 baud. (1 start bit, 7 data bits, 1 parity bit, 1 stop bit)
             *  Do arithmetic as indicated to preserve significance.
             */
            useconds_t usec = (((msgSize - bytesAvail + 12ll) * 1000000ll) / 960ll);
            qDebug("Waiting for %u usec to get %lld = (%lld - %lld) more bytes."
                   , usec
                   , (msgSize - bytesAvail)
                   , msgSize
                   , bytesAvail);
            usleep(usec);
            prevBytesAvail = bytesAvail;
            qDebug() << "Wait again for ready read 10 msec.";
            // bytesAvailable doesn't seem to update unless waitForReadyRead is called.
            if (!serialPort->waitForReadyRead(10))
                qWarning() << "Read timeout waiting for more bytes.  Continue.";
            else
                qDebug() << "waitForReadyRead(10) returned true.";
            qDebug() << "  Now get the count again.";
            bytesAvail = serialPort->bytesAvailable();
            qDebug("Now there are %lld bytes available.", bytesAvail);
        }
        qDebug() << "Now read the data.";
        readData = serialPort->readAll();       // Read into QByteArray.
        bytesThisRead = readData.size();
        if (serialPort->error() != QSerialPort::NoError)
        {
            qDebug("Data was ready for reading, but an error occurred trying to read.  We read %lld bytes.", bytesThisRead);
            qDebug() << "Error code is: " << serialPort->error() << "Error was: " << serialPort->errorString();
            qInstallMessageHandler(prevMsgHandler);
            qDebug() << "Return false";
            return false;
        }
        else if ((bytesThisRead == 0) && (tryReadCount < maxTries))
        {
            qDebug() << "Data was ready for reading, but no bytes were read.  Try reading again.";
            tryReadCount++;
            continue;
        }
        qDebug("%lld bytes read are: %s", bytesThisRead, qUtf8Printable(readData.toHex()));
        qDebug("Copying %lld bytes read to msg at %p"
               , qMax(qMin(bytesThisRead, (msgSize - bytesRead)), 0ll)
               , (void *)((qint8 *)msg + bytesRead));
        memcpy(((qint8 *)msg + bytesRead)
               , readData.constData()
               , qMax(qMin(bytesThisRead, (msgSize - bytesRead)), 0ll));
        bytesRead += qMax(qMin(bytesThisRead, (msgSize - bytesRead)), 0ll);
        qDebug("%lld bytes of Msg is now: %s\n", bytesRead, qUtf8Printable(QByteArray((char *)msg, bytesRead).toHex()));
    }

    qInstallMessageHandler(prevMsgHandler);
    qDebug() << "Return true";
    return true;
}

/*!
 * \brief SendControl -- Send a control message to meter with interaction.
 *
 * Send request for "A" response to establish communication with a particular
 * meter.  Read response, send password message, read response,
 * send this message, read response, send close message.
 * \param serialPort
 * \param meterId   Full 12 character serial number of meter.
 * \param ctrlMsg   The message to send.
 * \param msgSize   The size of the message.
 * \return true if successful, false otherwise.
 */
bool SendControl(QSerialPort *serialPort, QString &meterId, OutputControlDef *ctrlMsg, const int msgSize)
{
    if (meterId.toLongLong() < 300000000)
    {
        qDebug() << "I only know how to send controls to v.4 Omnimeters.";
        qDebug() << "Return false";
        return false;
    }
    if (ctrlMsg == NULL)
    {
        qDebug () << "ctrlMsg is NULL";
        qDebug() << "Return false";
        return false;
    }
    {
        qint8 ackResponse[1];
        ResponseV4AData responseA;
        qInfo() << "Begin";
        static const int maxTries = 10;
        memcpy(RequestMsgV4.meterId, qPrintable(meterId), sizeof(RequestMsgV4.meterId));
        RequestMsgV4.reqType[1] = '\x30';       // \x30 to get A data

        qInfo("RequestMsgV4 is: %s", qUtf8Printable(QByteArray((const char *)RequestMsgV4.fixedBegin, sizeof(RequestMsgV4)).toHex()));

        int tryCount = 0;
        while (tryCount++ < maxTries)
        {
            qDebug() << "Started trying to get data.";
            WriteSerialMsg(serialPort, (const char *)RequestMsgV4.fixedBegin, sizeof(RequestMsgV4));
            if (ReadResponse(serialPort, (qint8 *)(&responseA), sizeof(responseA)))
                break;
        }
        if (tryCount >= maxTries)
        {
            qDebug() << "Could not get responseA from V4 meter" << meterId;
            qDebug() << "Return false";
            return false;
        }

        // Send the control message.
        if (WriteSerialMsg(serialPort, (const char *)PasswordMsg, sizeof(PasswordMsg)))
        {
            qDebug() << "Wrote Password message ok.";
            if (ReadResponse(serialPort, ackResponse, sizeof(ackResponse)))
            {
                if (ackResponse[0] == ResponseAck[0])
                {
                    qDebug() << "Got ACK from password";
                    if (WriteSerialMsg(serialPort, (const char *)ctrlMsg->SOH, msgSize))
                    {
                        qDebug() << "Wrote control message ok.";
                        if (ReadResponse(serialPort, ackResponse, sizeof(ackResponse)))
                        {
                            if (ackResponse[0] == ResponseAck[0])
                                qDebug() << "Got ACK from control";
                            else
                                qDebug() << "ack response wasn't ACK.";
                        }
                        else
                            qDebug() << "Could not get ack response.";
                    }
                    else
                        qDebug() << "Failed to write control message.";
                }
                else
                    qDebug() << "ack response wasn't ACK.";
            }
            else
                qDebug() << "Could not get ack response.";
        }
        else
            qDebug() << "Failed to write password message.";
    }

    WriteSerialMsg(serialPort, (const char *)CloseString, sizeof(CloseString));
    qDebug() << "Return true";
    return true;
}

/*!
 * \brief SetMeterTime -- Set the meter to this computer's idea of local standard time.
 *
 * Similar to SendControl() function.  Time message must be computed before sending.
 *
 * \param serialPort
 * \param meterId   Full 12 character serial number of meter.
 * \return true if successful, false otherwise.
 */
bool SetMeterTime(QSerialPort *serialPort, QString &meterId)
{
    bool retVal = false;
    if (meterId.toLongLong() < 300000000)
    {
        qDebug() << "I only know how to set time to v.4 Omnimeters; others ignored.";
        qDebug() << "Return true";
        return true;
    }
    {
        qint8 ackResponse[1];
        ResponseV4AData responseA;
        qInfo() << "Begin";
        static const int maxTries = 10;
        memcpy(RequestMsgV4.meterId, qPrintable(meterId), sizeof(RequestMsgV4.meterId));
        RequestMsgV4.reqType[1] = '\x30';       // \x30 to get A data

        qInfo("RequestMsgV4 is: %s", qUtf8Printable(QByteArray((const char *)RequestMsgV4.fixedBegin, sizeof(RequestMsgV4)).toHex()));

        int tryCount = 0;
        while (tryCount++ < maxTries)
        {
            qDebug() << "Started trying to get data.";
            WriteSerialMsg(serialPort, (const char *)RequestMsgV4.fixedBegin, sizeof(RequestMsgV4));
            if (ReadResponse(serialPort, (qint8 *)(&responseA), sizeof(responseA)))
                break;
        }
        if (tryCount >= maxTries)
        {
            qDebug() << "Could not get responseA from V4 meter" << meterId;
            qDebug() << "Return false";
            return false;
        }

        // Send the control message.
        if (WriteSerialMsg(serialPort, (const char *)PasswordMsg, sizeof(PasswordMsg)))
        {
            qDebug() << "Wrote Password message ok.";
            if (ReadResponse(serialPort, ackResponse, sizeof(ackResponse)))
            {
                if (ackResponse[0] == ResponseAck[0])
                {
                    qDebug() << "Got ACK from password";
                    SetTimeMsgDef setTime = SetTimeMsg;
                    QDateTime timeNow = QDateTime::currentDateTime();
                    qInfo() << "The current time is:  " << timeNow;
                    memcpy((void *)&(setTime.dateTime), qPrintable(
                               timeNow.toTimeZone(LocalStandardTimeZone)
                               .toString("yyMMdd00HHmmss")), sizeof(setTime.dateTime));
                    int dow = (timeNow.date().dayOfWeek() % 7) + 1;  // Convert Qt's week day number to EKM's.
                    setTime.dateTime.weekday[0] = (dow / 256) + 48;
                    setTime.dateTime.weekday[1] = (dow % 256) + 48;

                    uint16_t crc = computeEkmCrc((uint8_t *)(setTime.SOH) + 1, sizeof(SetTimeMsgDef) - 3);
                    setTime.crc[0] = (crc >> 8) & 0x7f;
                    setTime.crc[1] = crc & 0x7f;
                    qInfo("setTime after CRC: %s", qUtf8Printable(QByteArray((const char *)&setTime, sizeof(setTime)).toHex()));
                    if (ValidateCRC((const uint8_t *)(&setTime.SOH) + 1,  sizeof(SetTimeMsgDef) - 3))
                        qDebug("We think the CRC is OK.");
                    else
                        qDebug("We think the CRC is NOT OK.");
                    if (WriteSerialMsg(serialPort, (const char *)setTime.SOH, sizeof(SetTimeMsgDef)))
                    {
                        qDebug() << "Wrote set time message ok.";
                        if (ReadResponse(serialPort, ackResponse, sizeof(ackResponse)))
                        {
                            if (ackResponse[0] == ResponseAck[0])
                            {
                                qDebug() << "Got ACK from setting time";
                                retVal = true;
                            }
                            else
                                qDebug() << "ack response wasn't ACK.";
                        }
                        else
                            qDebug() << "Could not get ack response.";
                    }
                    else
                        qDebug() << "Failed to write set time message.";
                }
                else
                    qDebug() << "ack response wasn't ACK.";
            }
            else
                qDebug() << "Could not get ack response.";
        }
        else
            qDebug() << "Failed to write password message.";
    }

    WriteSerialMsg(serialPort, (const char *)CloseString, sizeof(CloseString));
    qDebug() << "Return" << retVal;
    return retVal;
}

/*!
 * \brief GetMeterV4Data -- Get a data response from a v.4 Omnimeter.
 *
 * Send request for data, read response.  Does not close meter since
 * there may be another request for data soon.
 *
 * \param serialPort
 * \param meterId   Full 12 character serial number of meter.
 * \param requestType   Either x30 for "A" data or x31 for "B" data.
 * \param response  Pointer to response data.
 * \return true if successful, false otherwise.
 */
bool GetMeterV4Data(QSerialPort *serialPort
                    , const QString meterId
                    , const uint8_t requestType
                    , ResponseV4Generic *response)
{
    qInfo() << "Begin";
    static const int maxTries = 10;
    memcpy(RequestMsgV4.meterId, qPrintable(meterId), sizeof(RequestMsgV4.meterId));
    RequestMsgV4.reqType[1] = requestType;       // \x30 to get A data

    qInfo("RequestMsgV4 is: %s", qUtf8Printable(QByteArray((const char *)RequestMsgV4.fixedBegin, sizeof(RequestMsgV4)).toHex()));

    int tryCount = 0;
    while (tryCount++ < maxTries)
    {
        if (serialPort->clear())
            qDebug("Cleared serial port data.");
        else
            qWarning("Clearing serial port data had error:  %s", qUtf8Printable(serialPort->errorString()));
        serialPort->clearError();
        qDebug() << "Started trying to get data.";
        WriteSerialMsg(serialPort, (const char *)RequestMsgV4.fixedBegin, sizeof(RequestMsgV4));
        if (ReadResponse(serialPort, (qint8 *)response, sizeof(*response)))
            break;
    }
    if (tryCount >= maxTries)
    {
        qDebug("Could not get response %x from V4 meter %s"
               , requestType
               , qUtf8Printable(meterId));
        qDebug() << "Return false";
        return false;
    }
    qDebug() << "Return true";
    return true;
}

/*!
 * \brief GetMeterV3Data -- Get a data response from a v.3 Omnimeter.
 * \param serialPort
 * \param meterId   Full 12 character serial number of meter.
 * \param response
 * \return true if successful, false otherwise.
 */
bool GetMeterV3Data(QSerialPort *serialPort, const QString meterId, ResponseV3Data *response)
{
    qInfo() << "Begin";
    static const int maxTries = 10;
    memcpy(RequestMsgV3.meterId, qPrintable(meterId), sizeof(RequestMsgV3.meterId));

    qInfo("RequestMsgV3 is: %s", qUtf8Printable(QByteArray((const char *)RequestMsgV3.fixedBegin, sizeof(RequestMsgV3)).toHex()));

    int tryCount = 0;
    while (tryCount++ < maxTries)
    {
        serialPort->clearError();
        qDebug() << "Started trying to get data.";
        WriteSerialMsg(serialPort, (const char *)RequestMsgV3.fixedBegin, sizeof(RequestMsgV3));
        if (ReadResponse(serialPort, (qint8 *)response, sizeof(*response)))
            break;
    }
    if (tryCount >= maxTries)
    {
        qDebug() << "Could not get response from V3 meter" << meterId;
        qDebug() << "Return false";
        return false;
    }
    WriteSerialMsg(serialPort, (const char *)CloseString, sizeof(CloseString));
    qDebug() << "Return true";
    return true;
}

/*!
 * \brief ValidateCRC
 * \param msg   Pointer to beginning of part of message on which to compute CRC.
 * \param numBytes  Number of bytes to include in CRC.
 * \return true if CRC in message matches computed CRC.
 */
bool ValidateCRC(const uint8_t *msg, int numBytes)
{
    // Compute CRC from msg for numBytes; then compare to the next two bytes.
    qDebug() << "Begin";
    uint16_t crc = computeEkmCrc(msg, numBytes);
    qDebug("Computed CRC is %04x", crc);
    uint16_t msgCrc = msg[numBytes] * 256 + msg[numBytes + 1];
    qDebug("Message CRC  is %04x", msgCrc);

    qDebug() << "Return" << (crc == msgCrc);
    return (crc == msgCrc);
}

/*!
 * \brief main -- The whole tamale.
 * \param argc
 * \param argv
 * \return Program success indicator.
 */
int main(int argc, char *argv[])
{
    /*
     * DON'T ShowDiagnosticsSince OR DumpDebugInfo OR FlushDiagnostics
     * DIAGNOSTICS TILL AFTER OPTIONS PROCESSED.
     *
     * THIS MAY AVOID TRYING TO WRITE DIAGNOSTICS TO MAIN DATABASE SINCE
     * IT MAY NOT HAVE A DebugInfo TABLE.
     */
    QCoreApplication a(argc, argv);
    StartTime = QDateTime::currentDateTime();
    // Silent till options processed.
    qInstallMessageHandler(saveMessageOutput);
    qInfo() << "Begin";
    DetermineCommitTag(a.arguments().at(0), __FILE__);

    /*
     * Local variable declarations
     */
    QSerialPort *serialPort = NULL;
    int interval = 0, repeatCount = 0, aToBRatio = 10, aDataCount = 0;

    /*
     * Process command line options
     */
    QCommandLineParser parser;
    parser.setApplicationDescription("\nReadHouseAndWater\n"
                                     "     Program to read House Electrical power usage and Water usage and store in database.");
    parser.addHelpOption();

    parser.addPositionalArgument("meterId", "Meter serial numbers to read.", "[meter id] ...");
    QCommandLineOption serialDeviceOption(QStringList() << "s" << "serial-name"
                                          , "The name of the serial device. [cu.usbserial-AH034Y93]"
                                          , "Name"
                                          , "cu.usbserial-AH034Y93");
    QCommandLineOption intervalOption(QStringList() << "i" << "interval", "Time between successive reads of meters listed.\n"
                                                                          "If zero read only once.", "minutes"
                                      , "1");
    QCommandLineOption repeatCountOption(QStringList() << "r" << "repeat-count", "Number of times to read the meters listed.\n"
                                                                                 "If zero read forever.", "count"
                                         , "0");
    QCommandLineOption aToBRatioOption(QStringList() << "n" << "a-to-b-ratio", "Number of times to read A data from V4 meters before reading B data.\n"
                                                                               "If zero don't read B data.", "count"
                                       , "15");
    QCommandLineOption databaseOption(QStringList() << "d" << "database", "Database id string.", "URL"
                                      , "");        // for example "QMYSQL://user:xxxxxx@host/schema"
    QCommandLineOption envVarNameOption(QStringList() << "e" << "DbEnvName", "Name of environment variable containing database id string.", "Name"
                                      , "EKMdatabase");
    QCommandLineOption debugDatabaseOption(QStringList() << "B" << "debug-database", "Database id string for debug info.", "URL"
                                           , "");
    QCommandLineOption showDiagnosticsOption(QStringList() << "D" << "show-diagnostics"
                                             , "Print saved diagnostics to terminal at runtime.");
    QCommandLineOption immediateDiagnosticsOption(QStringList() << "S" << "immediate-diagnostics"
                                                  , "Print diagnostic info to terminal immediately.");
    QCommandLineOption dontWriteDatabaseOption(QStringList() << "W" << "dont-write"
                                               , "If specified, don't actually write to the database.");
    parser.addOption(serialDeviceOption);
    parser.addOption(intervalOption);
    parser.addOption(repeatCountOption);
    parser.addOption(aToBRatioOption);
    parser.addOption(databaseOption);
    parser.addOption(envVarNameOption);
    parser.addOption(debugDatabaseOption);
    parser.addOption(showDiagnosticsOption);
    parser.addOption(immediateDiagnosticsOption);
    parser.addOption(dontWriteDatabaseOption);
    parser.process(a);

    ShowDiagnostics = parser.isSet(showDiagnosticsOption);
    ImmediateDiagnostics = parser.isSet(immediateDiagnosticsOption);
    if (ImmediateDiagnostics)
        qInstallMessageHandler(terminalMessageOutput);
    else
        qInstallMessageHandler(saveMessageOutput);

    DontActuallyWriteDatabase = parser.isSet(dontWriteDatabaseOption);
    qDebug() << "DontActuallyWriteDatabase: " << DontActuallyWriteDatabase;

    QString serialDevice = parser.value(serialDeviceOption);
    qDebug() << "Using serialDevice" << serialDevice;

    QString databaseConnString = parser.value(databaseOption);
    if (databaseConnString.isEmpty())
    {
        databaseConnString = QProcessEnvironment::systemEnvironment().value(parser.value(envVarNameOption));
        if (databaseConnString.isEmpty())
        {
            qCritical("No database connection string found.  Return -3");
            return -3;
        }
    }
    qDebug() << "Using database connection string: " << databaseConnString;

    addConnectionFromString(databaseConnString);

    databaseConnString = parser.value(debugDatabaseOption);
    if (!databaseConnString.isEmpty())
    {
        qDebug() << "Using database connection string for debug info: " << databaseConnString;
        addConnectionFromString(databaseConnString, true);
    }

    interval = parser.value(intervalOption).toInt();
    qInfo("Interval between successive meter reads is %d minutes.", interval);
    repeatCount = parser.value(repeatCountOption).toInt();
    if (repeatCount <= 0)
        repeatCount = INT32_MAX;
    if (interval == 0)
        qInfo("Read meters just once.");
    else
        qInfo("Number of times to read meters is %d.", repeatCount);
    
    aToBRatio = parser.value(aToBRatioOption).toInt();

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty())
    {
        qCritical("You must supply at least one meter id on the command line.");
        qDebug("Return 1");
        return 1;
    }

    /*  Command line options processed.  */
    FlushDiagnostics();
    DumpDebugInfo();

    {   // block to isolate temporary variables.
        qint64 t = QDateTime::currentMSecsSinceEpoch() / 60000;     // number of min since epoch
        t = t / interval;                                           // number of intervals since epoch
        t = t % aToBRatio;                              // number of intervals till next B read.
        aDataCount = t - 1;                     // set count
        qInfo("interval = %d; aToBRatio = %d; aDataCount = %d", interval, aToBRatio, aDataCount);
    }

    if (!ConnectSerial(serialDevice, &serialPort))
    {
        qFatal("Could not connect serial device.");
    }
    qDebug() << "Connected to serial device.";
    qDebug() << "SerialPort is:" << serialPort;

    if (!InitializeMeters(serialPort, args))
    {
        qCritical("Unable to initialize meters.");
        return -1;
    }

    QDate today = QDate::currentDate();

    /*! Loop till we have read all meters the number of times in repeatCount. */
    do
    {
        foreach (QString meterId, args)
        {
            QString fullMeterId = meterId.rightJustified(sizeof(RequestMsgV4.meterId), '0', true);
            qInfo() << "Getting data from meter:" << fullMeterId;
            if (fullMeterId.toLongLong() >= 300000000)
            {
                qInfo() << "The meter is a v.4 meter.";
                ResponseV4Generic responseA;
                ResponseV4Generic responseB;
                if (GetMeterV4Data(serialPort, fullMeterId, '\x30', &responseA))
                {
                    qDebug() << "Got V4 meter data";
                    if (ValidateCRC(((uint8_t *)(responseA.responseV4Generic.fixed02) + 1), 252))
                        qDebug() << "responseA crc is valid.";
                    else
                        qDebug() << "responseA crc is NOT valid.";
                    if (!SaveV4ResponseToDatabase('\x30', responseA))
                    {
                        qDebug() << "Could not save V4 response to database.";
                    }
                    else
                    {
                        qDebug() << "Saved V4 response to database.";
                    }
                }
                if ((++aDataCount >= aToBRatio) && GetMeterV4Data(serialPort, fullMeterId, '\x31', &responseB))
                {
                    aDataCount = 0;
                    qDebug() << "Got V4 meter data";
                    if (ValidateCRC(((uint8_t *)(responseB.responseV4Generic.fixed02) + 1), 252))
                        qDebug() << "responseB crc is valid.";
                    else
                        qDebug() << "responseB crc is NOT valid.";
                    if (!SaveV4ResponseToDatabase('\x31', responseB))
                    {
                        qDebug() << "Could not save V4 response to database.";
                    }
                    else
                    {
                        qDebug() << "Saved V4 response to database.";
                    }
                }

                /*! Close the communication with this meter. */
                WriteSerialMsg(serialPort, (const char *)CloseString, sizeof(CloseString));

                /*! Set the meter time every day at midnight for each v.4 meter. */
                if (today != QDate::currentDate())
                    SetMeterTime(serialPort, fullMeterId);

                /*
             * Output 2 is assumed connected to a relay that controls the
             * sprinkler controller "Rain Sensor", such that if the relay
             * is CLOSED, watering is enabled.  If the relay is OPEN,
             * watering is disabled.
             */
                if (QFileInfo::exists(QDir::homePath() + "/.WeatherWet"))
                {       // Weather program has determined that it is WET out there
                    // Disable watering.
                    qDebug("Make sure watering is DISabled.  Current output state is 0x%02x (-0x31 = 0x%02x)"
                           , responseA.responseV4Adata.outState[0]
                            , responseA.responseV4Adata.outState[0] - 0x31);
                    if (((responseA.responseV4Adata.outState[0] - 0x31) & 1) != 1)  // if off, turn on
                        SendControl(serialPort, fullMeterId, &Output2OnMsg, sizeof(Output2OnMsg));
                }
                else
                {       // Weather program has determined that it is DRY out there
                    // Enable watering.
                    qDebug("Make sure watering is ENabled.  Current output state is 0x%02x (-0x31 = 0x%02x)"
                           , responseA.responseV4Adata.outState[0]
                            , responseA.responseV4Adata.outState[0] - 0x31);
                    if (((responseA.responseV4Adata.outState[0] - 0x31) & 1) == 1)  // if on, turn off
                        SendControl(serialPort, fullMeterId, &Output2OffMsg, sizeof(Output2OffMsg));
                }
            }
            else
            {
                qInfo() << "The meter is a v.3 meter.";
                ResponseV3Data response;
                if (GetMeterV3Data(serialPort, fullMeterId, &response))
                    qDebug() << "Got V3 meter data";
                if (ValidateCRC(((uint8_t *)(response.fixed02) + 1), 252))
                    qDebug() << "response crc is valid.";
                else
                    qDebug() << "response crc is NOT valid.";
                if (!SaveV3ResponseToDatabase(response))
                {
                    qDebug() << "Could not save V3 response to database.";
                }
                else
                {
                    qDebug() << "Saved V3 response to database.";
                }
            }
        }

        // Check for existence of magic file ".CloseReadEKM" and quit if seen.

        if (QFile::exists(QDir::homePath() + "/.CloseReadEKM"))
        {
            qDebug("Quitting because magic file \".CloseReadEKM\" seen.");
            QFile::remove(QDir::homePath() + "/.CloseReadEKM");
            qDebug("Magic file \".CloseReadEKM\" deleted.");
            break;          // break out of while loop that keeps us reading data.
        }

        today = QDate::currentDate();       // Update "today" after all meters read.
        if ((interval > 0) && (repeatCount > 1))
        {
            DumpDebugInfo();
            /* interval*60000 is the number of millisec between reads.
             * Compute the number of millisec we're into this period,
             * subtract from the number of millisec in an interval
             * and multiply by 1000 to get the number of microsec to sleep.
             */
            useconds_t usecToSleep = ((interval * 60000ll) - (QDateTime::currentMSecsSinceEpoch() % (interval*60000ll))) * 1000;
            qInfo("Sleeping for %u micro sec (almost %d minutes).", usecToSleep, interval);
            usleep(usecToSleep);
        }
    } while (--repeatCount > 0);

    qDebug() << "End program";
    DumpDebugInfo();
    return 0;
}
