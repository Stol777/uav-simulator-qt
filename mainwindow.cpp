#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QIntValidator>
#include <QPainter>
#include <QPen>
#include <QTimer>

#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene(this))
    , uav(nullptr)
    , movementTimer(new QTimer(this))
    , squareRoute{QPointF(50, 50), QPointF(250, 50),
                  QPointF(250, 250), QPointF(50, 250)}
{
    ui->setupUi(this);

    scene->setSceneRect(0, 0, SceneWidth, SceneHeight);
    scene->setBackgroundBrush(Qt::white);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Координаты обозначают левый верхний угол окружности.
    uav = scene->addEllipse(0, 0, UAVDiameter, UAVDiameter,
                           QPen(Qt::NoPen), QBrush(Qt::red));
    uav->setData(0, QStringLiteral("uav"));
    updateUAVPosition();

    ui->lineEditX->setValidator(new QIntValidator(0, SceneWidth - UAVDiameter, this));
    ui->lineEditY->setValidator(new QIntValidator(0, SceneHeight - UAVDiameter, this));

    movementTimer->setObjectName(QStringLiteral("movementTimer"));
    movementTimer->setInterval(TimerIntervalMs);
    movementTimer->setTimerType(Qt::PreciseTimer);
    connect(movementTimer, &QTimer::timeout, this, &MainWindow::moveUAV);

    // Кнопки связываются со слотами через connectSlotsByName в setupUi.
    ui->labelStatus->setText(QStringLiteral("БПЛА готов к запуску. Координаты: (50, 50)."));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnStart_clicked()
{
    // Каждый запуск квадрата начинается с первой вершины.
    movementTimer->stop();
    uavX = squareRoute.at(0).x();
    uavY = squareRoute.at(0).y();
    waypointIndex = 1;
    flightMode = FlightMode::Square;
    updateUAVPosition();
    showFlightStatus();
    movementTimer->start();
}

void MainWindow::on_btnFly_clicked()
{
    bool xIsInteger = false;
    bool yIsInteger = false;
    const int targetX = ui->lineEditX->text().trimmed().toInt(&xIsInteger);
    const int targetY = ui->lineEditY->text().trimmed().toInt(&yIsInteger);

    // Повторная проверка нужна и при программном изменении содержимого полей.
    if (!xIsInteger || !yIsInteger || targetX < 0 || targetX > SceneWidth - UAVDiameter
        || targetY < 0 || targetY > SceneHeight - UAVDiameter) {
        ui->labelStatus->setText(QStringLiteral("Введите целые координаты: X от 0 до 780, Y от 0 до 580."));
        return;
    }

    // Один таймер и один режим не допускают одновременного выполнения полётов.
    movementTimer->stop();
    pointTarget = QPointF(targetX, targetY);
    flightMode = FlightMode::Point;

    if (uavX == pointTarget.x() && uavY == pointTarget.y()) {
        finishPointFlight();
        return;
    }

    showFlightStatus();
    movementTimer->start();
}

void MainWindow::moveUAV()
{
    if (flightMode == FlightMode::Square) {
        if (moveTowards(squareRoute.at(waypointIndex))) {
            waypointIndex = (waypointIndex + 1) % squareRoute.size();
        }
        showFlightStatus();
    } else if (flightMode == FlightMode::Point) {
        if (moveTowards(pointTarget)) {
            finishPointFlight();
        } else {
            showFlightStatus();
        }
    } else {
        movementTimer->stop();
    }
}

bool MainWindow::moveTowards(const QPointF &target)
{
    const double dx = target.x() - uavX;
    const double dy = target.y() - uavY;
    const double distance = std::sqrt(dx * dx + dy * dy);

    // Последний шаг может быть короче двух пикселей: цель не перескакивается.
    // Этот случай также исключает деление на ноль при distance == 0.
    if (distance <= StepPixels) {
        uavX = target.x();
        uavY = target.y();
        updateUAVPosition();
        return true;
    }

    uavX += StepPixels * dx / distance;
    uavY += StepPixels * dy / distance;
    updateUAVPosition();
    return false;
}

void MainWindow::updateUAVPosition()
{
    uav->setPos(uavX, uavY);
}

void MainWindow::showFlightStatus()
{
    const QString position = QStringLiteral("X = %1, Y = %2")
                                 .arg(uavX, 0, 'f', 1)
                                 .arg(uavY, 0, 'f', 1);

    if (flightMode == FlightMode::Square) {
        ui->labelStatus->setText(QStringLiteral("Полёт по квадрату. %1").arg(position));
    } else if (flightMode == FlightMode::Point) {
        ui->labelStatus->setText(QStringLiteral("Полёт к точке (%1, %2). %3")
                                    .arg(pointTarget.x(), 0, 'f', 0)
                                    .arg(pointTarget.y(), 0, 'f', 0)
                                    .arg(position));
    }
}

void MainWindow::finishPointFlight()
{
    movementTimer->stop();
    flightMode = FlightMode::Idle;
    ui->labelStatus->setText(QStringLiteral("Цель достигнута"));
}
