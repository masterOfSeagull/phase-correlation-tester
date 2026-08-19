#include <QGuiApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTextStream>

#include "PhaseCorrelationEngine.h"

namespace {

QFile *logFile = nullptr;

void writeQtMessage(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    if (!logFile || !logFile->isOpen()) {
        return;
    }

    const char *level = "info";
    switch (type) {
    case QtDebugMsg:
        level = "debug";
        break;
    case QtInfoMsg:
        level = "info";
        break;
    case QtWarningMsg:
        level = "warning";
        break;
    case QtCriticalMsg:
        level = "critical";
        break;
    case QtFatalMsg:
        level = "fatal";
        break;
    }

    QTextStream stream(logFile);
    stream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [" << level << "] " << message << '\n';
    stream.flush();
}

} // namespace

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Phase Correlation Tester"));
    QGuiApplication::setOrganizationName(QStringLiteral("PhaseCorrelationTester"));

    QFile startupLog(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("phase-correlation-tester.log")));
    if (startupLog.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        logFile = &startupLog;
        qInstallMessageHandler(writeQtMessage);
        qInfo().noquote() << "Starting Phase Correlation Tester";
    }

    PhaseCorrelationEngine correlationEngine;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("correlationEngine"), &correlationEngine);
    engine.loadFromModule("PhaseCorrelationTester", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical().noquote() << "No QML root objects were created.";
        return -1;
    }

    return app.exec();
}
