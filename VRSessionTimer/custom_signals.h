#ifndef CUSTOM_SIGNALS_H
#define CUSTOM_SIGNALS_H

#include <QObject>

class custom_signals : public QObject
{
    Q_OBJECT
public:
    explicit custom_signals(QObject *parent = 0);

private:
    int m_shouldClose;

signals:
    void shouldClose(int value);

public slots:
};

#endif // CUSTOM_SIGNALS_H
