#pragma once

#include "app/view.hpp"
#include "data/session.hpp"
#include <QObject>
#include <atomic>
#include <iostream>

class BCWorker : public QObject {
    Q_OBJECT
public:
    BCWorker(Session& sess, QString path, QtStatusProxy* proxy)
        : session(sess), path(std::move(path)), proxy(proxy) {};

signals:
    void finished();
    void error(QString msg);

public slots:
    void load_trace() {
        std::cout << "Started loading " << path.toStdString() << std::endl;
        session.load_trace(path.toStdString(), proxy);
        if (cancelled) return;
        emit finished();
    }
    void request_cancel() { cancelled = true; }

private:
    Session& session;
    QString path;
    QtStatusProxy* proxy;
    std::atomic<bool> cancelled = false;
};