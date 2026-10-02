#include "mainwindow.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QLineEdit>
#include <QLineF>
#include <QPushButton>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

#include <cmath>
#include <memory>

class SimulatorTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void initialScene();
    void squareRoute();
    void restartSquare();
    void diagonalPointFlight();
    void nearAndSameTarget_data();
    void nearAndSameTarget();
    void sceneBoundaries_data();
    void sceneBoundaries();
    void invalidCoordinates_data();
    void invalidCoordinates();
    void switchFromSquareToPoint();
    void invalidInputKeepsCurrentFlight();
    void timerMovesAndStops();

private:
    bool tick();
    void setTarget(const QString &x, const QString &y);
    void completeFlight(const QPointF &target);

    std::unique_ptr<MainWindow> window;
    QGraphicsView *view = nullptr;
    QGraphicsEllipseItem *uav = nullptr;
    QTimer *timer = nullptr;
    QLabel *status = nullptr;
    QLineEdit *inputX = nullptr;
    QLineEdit *inputY = nullptr;
    QPushButton *startButton = nullptr;
    QPushButton *flyButton = nullptr;
};

void SimulatorTest::init()
{
    window = std::make_unique<MainWindow>();
    view = window->findChild<QGraphicsView *>(QStringLiteral("graphicsView"));
    timer = window->findChild<QTimer *>(QStringLiteral("movementTimer"));
    status = window->findChild<QLabel *>(QStringLiteral("labelStatus"));
    inputX = window->findChild<QLineEdit *>(QStringLiteral("lineEditX"));
    inputY = window->findChild<QLineEdit *>(QStringLiteral("lineEditY"));
    startButton = window->findChild<QPushButton *>(QStringLiteral("btnStart"));
    flyButton = window->findChild<QPushButton *>(QStringLiteral("btnFly"));
    QVERIFY(view);
    QVERIFY(view->scene());
    QVERIFY(timer);
    QVERIFY(status);
    QVERIFY(inputX);
    QVERIFY(inputY);
    QVERIFY(startButton);
    QVERIFY(flyButton);
    uav = nullptr;
    for (QGraphicsItem *item : view->scene()->items()) {
        if (item->data(0).toString() == QStringLiteral("uav")) {
            uav = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
            break;
        }
    }
    QVERIFY(uav);
}

void SimulatorTest::cleanup()
{
    window.reset();
}

bool SimulatorTest::tick()
{
    return QMetaObject::invokeMethod(window.get(), "moveUAV", Qt::DirectConnection);
}

void SimulatorTest::setTarget(const QString &x, const QString &y)
{
    // setText also exercises validation of programmatically supplied values.
    inputX->setText(x);
    inputY->setText(y);
    flyButton->click();
}

void SimulatorTest::completeFlight(const QPointF &target)
{
    timer->stop();
    for (int step = 0; step < 600; ++step) {
        const double distance = QLineF(uav->pos(), target).length();
        if (distance <= 2.0) {
            // Resume before the final deterministic tick to verify arrival stops
            // the actual timer, rather than merely observing our manual pause.
            timer->start();
            QVERIFY(tick());
            QCOMPARE(uav->pos(), target);
            QVERIFY(!timer->isActive());
            QCOMPARE(status->text(), QStringLiteral("Цель достигнута"));
            QVERIFY(tick());
            QCOMPARE(uav->pos(), target);
            return;
        }
        const QPointF previous = uav->pos();
        QVERIFY(tick());
        QVERIFY(std::abs(QLineF(previous, uav->pos()).length() - 2.0) < 1e-9);
        QVERIFY(QLineF(uav->pos(), target).length() < distance);
        QVERIFY(std::isfinite(uav->pos().x()));
        QVERIFY(std::isfinite(uav->pos().y()));
        QVERIFY(view->scene()->sceneRect().contains(uav->sceneBoundingRect()));
    }
    QFAIL("The UAV did not reach its target within 600 steps");
}

void SimulatorTest::initialScene()
{
    QCOMPARE(view->scene()->sceneRect(), QRectF(0, 0, 800, 600));
    QCOMPARE(uav->rect(), QRectF(0, 0, 20, 20));
    QCOMPARE(uav->brush().color(), QColor(Qt::red));
    QCOMPARE(uav->pos(), QPointF(50, 50));
    QCOMPARE(timer->interval(), 20);
    QVERIFY(!timer->isActive());
    QVERIFY(tick());
    QCOMPARE(uav->pos(), QPointF(50, 50));
}

void SimulatorTest::squareRoute()
{
    startButton->click();
    QVERIFY(timer->isActive());
    timer->stop();

    // Verify every step and all four corners, followed by the next lap.
    for (int step = 1; step <= 100; ++step) {
        QVERIFY(tick());
        QCOMPARE(uav->pos(), QPointF(50 + 2 * step, 50));
    }
    for (int step = 1; step <= 100; ++step) {
        QVERIFY(tick());
        QCOMPARE(uav->pos(), QPointF(250, 50 + 2 * step));
    }
    for (int step = 1; step <= 100; ++step) {
        QVERIFY(tick());
        QCOMPARE(uav->pos(), QPointF(250 - 2 * step, 250));
    }
    for (int step = 1; step <= 100; ++step) {
        QVERIFY(tick());
        QCOMPARE(uav->pos(), QPointF(50, 250 - 2 * step));
    }
    QVERIFY(tick());
    QCOMPARE(uav->pos(), QPointF(52, 50));
    QVERIFY(status->text().contains(QStringLiteral("квадрату")));
}

void SimulatorTest::restartSquare()
{
    setTarget(QStringLiteral("100"), QStringLiteral("150"));
    QVERIFY(timer->isActive());
    completeFlight(QPointF(100, 150));
    startButton->click();
    QCOMPARE(uav->pos(), QPointF(50, 50));
    QVERIFY(timer->isActive());
    timer->stop();
    QVERIFY(tick());
    QCOMPARE(uav->pos(), QPointF(52, 50));
    startButton->click();
    QCOMPARE(uav->pos(), QPointF(50, 50));
    QVERIFY(timer->isActive());
}

void SimulatorTest::diagonalPointFlight()
{
    setTarget(QStringLiteral("100"), QStringLiteral("100"));
    QVERIFY(timer->isActive());
    timer->stop();
    QVERIFY(tick());
    QVERIFY(std::abs(uav->pos().x() - (50.0 + std::sqrt(2.0))) < 1e-9);
    QVERIFY(std::abs(uav->pos().y() - (50.0 + std::sqrt(2.0))) < 1e-9);
    completeFlight(QPointF(100, 100));
}

void SimulatorTest::nearAndSameTarget_data()
{
    QTest::addColumn<QPointF>("target");
    QTest::newRow("same") << QPointF(50, 50);
    QTest::newRow("one-pixel-diagonal") << QPointF(51, 51);
    QTest::newRow("exact-step") << QPointF(52, 50);
}

void SimulatorTest::nearAndSameTarget()
{
    QFETCH(QPointF, target);
    setTarget(QString::number(target.x()), QString::number(target.y()));
    if (target == QPointF(50, 50)) {
        QVERIFY(!timer->isActive());
        QCOMPARE(status->text(), QStringLiteral("Цель достигнута"));
        QVERIFY(tick());
        QCOMPARE(uav->pos(), target);
    } else {
        QVERIFY(timer->isActive());
        completeFlight(target);
    }
    QVERIFY(std::isfinite(uav->pos().x()));
    QVERIFY(std::isfinite(uav->pos().y()));
}

void SimulatorTest::sceneBoundaries_data()
{
    QTest::addColumn<QPointF>("target");
    QTest::newRow("top-left") << QPointF(0, 0);
    QTest::newRow("bottom-right") << QPointF(780, 580);
    QTest::newRow("top-right") << QPointF(780, 0);
    QTest::newRow("bottom-left") << QPointF(0, 580);
}

void SimulatorTest::sceneBoundaries()
{
    QFETCH(QPointF, target);
    setTarget(QString::number(target.x()), QString::number(target.y()));
    QVERIFY(timer->isActive());
    completeFlight(target);
    QVERIFY(view->scene()->sceneRect().contains(uav->sceneBoundingRect()));
}

void SimulatorTest::invalidCoordinates_data()
{
    QTest::addColumn<QString>("x");
    QTest::addColumn<QString>("y");
    QTest::newRow("empty-x") << QString() << QStringLiteral("50");
    QTest::newRow("empty-y") << QStringLiteral("50") << QString();
    QTest::newRow("text-x") << QStringLiteral("abc") << QStringLiteral("50");
    QTest::newRow("text-y") << QStringLiteral("50") << QStringLiteral("abc");
    QTest::newRow("fraction") << QStringLiteral("50.5") << QStringLiteral("50");
    QTest::newRow("negative-x") << QStringLiteral("-1") << QStringLiteral("50");
    QTest::newRow("negative-y") << QStringLiteral("50") << QStringLiteral("-1");
    QTest::newRow("x-too-large") << QStringLiteral("781") << QStringLiteral("50");
    QTest::newRow("y-too-large") << QStringLiteral("50") << QStringLiteral("581");
    QTest::newRow("integer-overflow") << QStringLiteral("99999999999999999999") << QStringLiteral("50");
}

void SimulatorTest::invalidCoordinates()
{
    QFETCH(QString, x);
    QFETCH(QString, y);
    setTarget(x, y);
    QVERIFY(!timer->isActive());
    QCOMPARE(uav->pos(), QPointF(50, 50));
    QVERIFY(status->text().contains(QStringLiteral("Введите целые координаты")));
    QVERIFY(tick());
    QCOMPARE(uav->pos(), QPointF(50, 50));
}

void SimulatorTest::switchFromSquareToPoint()
{
    startButton->click();
    QVERIFY(timer->isActive());
    timer->stop();
    for (int step = 0; step < 10; ++step)
        QVERIFY(tick());
    QCOMPARE(uav->pos(), QPointF(70, 50));

    setTarget(QStringLiteral("70"), QStringLiteral("150"));
    QVERIFY(timer->isActive());
    QCOMPARE(uav->pos(), QPointF(70, 50));
    timer->stop();
    QVERIFY(tick());
    QCOMPARE(uav->pos(), QPointF(70, 52));
    completeFlight(QPointF(70, 150));
}

void SimulatorTest::invalidInputKeepsCurrentFlight()
{
    startButton->click();
    QVERIFY(timer->isActive());
    setTarget(QStringLiteral("781"), QStringLiteral("50"));
    QVERIFY(timer->isActive());
    QVERIFY(status->text().contains(QStringLiteral("Введите целые координаты")));
    timer->stop();
    QVERIFY(tick());
    QCOMPARE(uav->pos(), QPointF(52, 50));
}

void SimulatorTest::timerMovesAndStops()
{
    QSignalSpy timeoutSpy(timer, &QTimer::timeout);
    setTarget(QStringLiteral("51"), QStringLiteral("51"));
    QVERIFY(timer->isActive());
    QTRY_VERIFY_WITH_TIMEOUT(!timeoutSpy.isEmpty(), 2000);
    QCOMPARE(uav->pos(), QPointF(51, 51));
    QVERIFY(!timer->isActive());
    QCOMPARE(status->text(), QStringLiteral("Цель достигнута"));
    QTest::qWait(60);
    QCOMPARE(timeoutSpy.count(), 1);
    QCOMPARE(uav->pos(), QPointF(51, 51));
}

QTEST_MAIN(SimulatorTest)
#include "tst_simulator.moc"
