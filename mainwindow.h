#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointF>
#include <QVector>

class QGraphicsEllipseItem;
class QGraphicsScene;
class QTimer;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_btnStart_clicked();
    void on_btnFly_clicked();
    void moveUAV();

private:
    enum class FlightMode { Idle, Square, Point };

    // Возвращает true, когда текущая цель достигнута.
    bool moveTowards(const QPointF &target);
    void updateUAVPosition();
    void showFlightStatus();
    void finishPointFlight();

    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QGraphicsEllipseItem *uav;
    QTimer *movementTimer;

    FlightMode flightMode = FlightMode::Idle;
    double uavX = 50.0;
    double uavY = 50.0;
    QPointF pointTarget;
    QVector<QPointF> squareRoute;
    int waypointIndex = 1;

    static constexpr int SceneWidth = 800;
    static constexpr int SceneHeight = 600;
    static constexpr int UAVDiameter = 20;
    static constexpr int TimerIntervalMs = 20;
    static constexpr double StepPixels = 2.0;
};

#endif // MAINWINDOW_H
