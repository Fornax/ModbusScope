#ifndef SCOPEGUI_H
#define SCOPEGUI_H

#include <QObject>

// Foward declaration
class QCustomPlot;

class ScopeGui : public QObject
{
    Q_OBJECT
public:
    explicit ScopeGui(QCustomPlot * pGraph, QObject *parent);

    void ResetGraph(quint32 variableCount);

signals:

public slots:
    void PlotResults(bool bSuccess, QList<quint16> values);
    void SetYAxisAutoScale(int state);
    void SetXAxisAutoScale(int state);

private slots:
    void SelectionChanged();
    void MousePress();
    void MouseWheel();

private:
    typedef struct
    {
        bool bXAxisAutoScale;
        bool bYAxisAutoScale;
    } GuiSettings;

    QCustomPlot * _pGraph;

    GuiSettings _settings;
};

#endif // SCOPEGUI_H