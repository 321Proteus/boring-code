#pragma once

#include "view.hpp"
#include "data/session.hpp"
#include "core/view.hpp"
#include <QObject>
#include <atomic>
#include <iostream>
#include <memory>
#include <vector>

class BCWorker : public QObject {
    Q_OBJECT
public:
    BCWorker(Session& sess, QString path, QtStatusProxy* proxy)
        : session(sess), path(std::move(path)), proxy(proxy) {};

signals:
    void finished();
    void traceReady(std::shared_ptr<std::vector<BCTraceEntry>>);
    void error(QString msg);

public slots:
    void load_trace() {
        std::cout << "Started loading " << path.toStdString() << std::endl;
        session.load_trace(path.toStdString(), proxy);
        if (cancelled) return;
        emit traceReady(BCTraceViewModel::precompute_trace(*session.database.get(), *proxy));
        emit finished();
    }
    void request_cancel() { cancelled = true; }

private:
    Session& session;
    QString path;
    QtStatusProxy* proxy;
    std::atomic<bool> cancelled = false;
};