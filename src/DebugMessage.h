#ifndef DEBUG_MESSAGE_LOGGER_H
#define DEBUG_MESSAGE_LOGGER_H

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QString>
#include <QtCore/QTextStream>

#define DEBUG_MESSAGE(message)                                     \
    do                                                             \
    {                                                              \
        QString debugMessageText;                                  \
        QTextStream debugMessageStream(&debugMessageText);         \
        debugMessageStream << message;                             \
        (DebugMessageLogger::instance().writeDebugMessage)(        \
            debugMessageText);                                     \
    } while (0)

    

class DebugMessageLogger
{
public:
    static DebugMessageLogger& instance()
    {
        static DebugMessageLogger logger;
        return logger;
    }

    bool isOpen()
    {
        QMutexLocker locker(&mutex_);
        return file_.isOpen();
    }

    bool writeDebugMessage(const QString& message)
    {
        QMutexLocker locker(&mutex_);
        if (!file_.isOpen())
            return false;

        const QString timestamp = QDateTime::currentDateTime().toString(
            QString::fromLatin1("yyyy-MM-dd HH:mm:ss,zzz"));
        const QByteArray line = (timestamp + QString::fromLatin1(" [")
            + message + QString::fromLatin1("]\n")).toUtf8();

        if (file_.write(line) != line.size())
            return false;

        file_.flush();
        return file_.pos() < maxFileSize() || rotateFiles();
    }

    bool flush()
    {
        QMutexLocker locker(&mutex_);
        return file_.isOpen() && file_.flush();
    }

private:
    DebugMessageLogger()
        : basePath_([]() {
              QDir dir(QCoreApplication::applicationDirPath());
              if (dir.dirName().compare(QStringLiteral("build"), Qt::CaseInsensitive) == 0)
                  dir.cdUp();
              return dir.absoluteFilePath(QStringLiteral("debug_message"));
          }()),
          file_(basePath_ + QString::fromLatin1(".txt"))
    {
        QDir().mkpath(QFileInfo(file_).absolutePath());
        if (file_.open(QIODevice::WriteOnly | QIODevice::Append)
            && file_.size() >= maxFileSize())
            rotateFiles();
    }

    static qint64 maxFileSize()
    {
        return Q_INT64_C(1024) * 1024 * 1024;
    }

    QString backupPath(int index) const
    {
        return basePath_ + QString::number(index)
            + QString::fromLatin1(".txt");
    }

    bool rotateFiles()
    {
        if (!file_.flush())
            return false;
        file_.close();

        const QString oldest = backupPath(10);
        if (QFile::exists(oldest) && !QFile::remove(oldest))
            return false;

        for (int index = 9; index >= 1; --index)
        {
            const QString source = backupPath(index);
            if (QFile::exists(source)
                && !QFile::rename(source, backupPath(index + 1)))
                return false;
        }

        if (!QFile::rename(file_.fileName(), backupPath(1)))
            return false;
        return file_.open(QIODevice::WriteOnly | QIODevice::Append);
    }

    DebugMessageLogger(const DebugMessageLogger&);
    DebugMessageLogger& operator=(const DebugMessageLogger&);

    QMutex mutex_;
    QString basePath_;
    QFile file_;
};


#endif
