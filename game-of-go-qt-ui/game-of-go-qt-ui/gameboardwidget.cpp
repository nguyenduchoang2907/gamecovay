#include "gameboardwidget.h"
#include "ui_gameboardwidget.h"
#include <cmath>

#include <QMouseEvent>
#include <QDebug>
//: Constructor tạo một widget để hiển thị bảng cờ với kích thước và màu cụ thể.
GameBoardWidget::GameBoardWidget(int boardSize, int color, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GameBoardWidget)
    , boardSize(boardSize)
    , playerColor(color)
{
    ui->setupUi(this);
    setFixedWidth(fullWidth);
    setFixedHeight(fullWidth);

    cellWidth = (fullWidth - 2 * margin) / (boardSize - 1);
    lineWidth = 0.05 * cellWidth;
    stoneRadius = 0.45 * cellWidth;
    shadowDisabled = color == 0;

    if (color > 0) {
        shadow = new StoneWidget(color, stoneRadius, 0, 0.5, this);
        shadow->hide();
    }
}

GameBoardWidget::~GameBoardWidget()//Vẽ bảng cờ với các đường và ô tương ứng.
{
    delete ui;
}

void GameBoardWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    // Create a QPainter object
    QPainter painter(this);   // Tạo một đối tượng QPainter


    painter.setBrush(backgroundColor);// Đặt màu nền cho painter
    painter.drawRect(0, 0, fullWidth, fullWidth);// Vẽ hình chữ nhật với màu nền

    QPen pen;
    pen.setWidth(lineWidth);// Đặt độ dày của đường vẽ
    pen.setColor(lineColor);// Đặt màu của đường vẽ
    painter.setPen(pen);// Áp dụng thuộc tính cho bút vẽ

    for (int i = 0; i < boardSize; i++) {//vẽ ngang dọc
        int offset = margin + i * cellWidth;
        if (boardSize > 11) offset += i * (boardSize - 10) * 0.04;// Điều chỉnh vị trí đường vẽ nếu kích thước bảng lớn hơn 11
        int eps = 2 * lineWidth * 0;// Khoảng cách phụ để điều chỉnh đường vẽ
        painter.drawLine(offset, margin, offset, fullWidth - margin - eps);//vẽ dọc
        painter.drawLine(margin, offset, fullWidth - margin - eps, offset);//vẽ ngang
    }
}

void GameBoardWidget::mouseMoveEvent(QMouseEvent *event) { //Xử lý sự kiện khi di chuyển chuột trên bảng cờ.
    if (playerColor == 0 || shadowDisabled || shadow == nullptr) return;
    moveStoneShadow(pointToCoords(QPoint(event->pos().x(), event->pos().y())));
}

void GameBoardWidget::mousePressEvent(QMouseEvent *event) {//Xử lý sự kiện khi rời khỏi vùng bảng cờ.
    if (playerColor == 0) return;
    QString coords = pointToCoords(QPoint(event->pos().x(), event->pos().y()));
    if (stoneMap[coords] != nullptr) return;
    emit clicked(coords);
}

void GameBoardWidget::leaveEvent(QEvent *event) {
    if (playerColor == 0) return;
    setStoneShadowVisible(false);
}

void GameBoardWidget::drawStone(int color, QString coords, bool withMarker) {//Vẽ quân cờ (đánh cờ) lên bảng cờ.
    if (stoneMap[coords] != nullptr) {
        stoneMap[coords]->hide();
        stoneMap.erase(coords);
    }
    StoneWidget *stone = new StoneWidget(color, stoneRadius, withMarker, 1, this);
    QPoint point = coordsToPoint(coords);
    stone->setGeometry(point.x() - stoneRadius, point.y() - stoneRadius, 2 * stoneRadius, 2 * stoneRadius);
    stone->show();
    stoneMap[coords] = stone;
    update();
}

void GameBoardWidget::removeStones(QStringList coordsList) {//di chuyển quân cờ mô phỏng trên bảng cờ khi di chuyển chuột.
    for (QString &coords: coordsList) {
        if (stoneMap[coords] != nullptr) {
            stoneMap[coords]->hide();
            stoneMap.erase(coords);
        }
    }
    update();
}

void GameBoardWidget::moveStoneShadow(QString coords) {//Hiển thị hoặc ẩn quân cờ mô phỏng.
    if (shadow == nullptr) return;
    if (stoneMap[coords] != nullptr) return;
    QPoint point = coordsToPoint(coords);
    shadow->setGeometry(point.x() - stoneRadius, point.y() - stoneRadius, 2 * stoneRadius, 2 * stoneRadius);
    shadow->show();
    update();
}

void GameBoardWidget::setStoneShadowVisible(bool visible) {//Tắt hoặc bật quân cờ mô phỏng.
    if (shadow == nullptr) return;
    if (visible) shadow->show();
    else shadow->hide();
    update();
}

void GameBoardWidget::setStoneShadowDisabled(bool disabled) {
    if (shadow == nullptr) return;
    shadowDisabled = disabled;
    if (disabled) {
        shadow->hide();
    }
}

void GameBoardWidget::drawTerritory(int color, QStringList coordsList) {//Vẽ lãnh thổ trên bảng cờ.
    int width = cellWidth / 5;
    for (QString coords: coordsList) {
        TerritoryWidget *territory = new TerritoryWidget(color, width, this);
        QPoint point = coordsToPoint(coords);
        territory->setGeometry(point.x() - width / 2, point.y() - width / 2, width, width);
        territory->show();
        territoryMap[coords] = territory;
    }
    update();
}

void GameBoardWidget::removeAllTerritory() {//Xóa tất cả lãnh thổ trên bảng cờ.
    for (auto &entry: territoryMap) {
        entry.second->hide();
    }
    territoryMap.clear();
}

QString GameBoardWidget::pointToCoords(QPoint point) {//Chuyển đổi tọa độ pixel chuột thành tọa độ trên bảng cờ.
       // Chuyển đổi tọa độ pixel chuột thành tọa độ trên bảng cờ.
     int x = point.x();
    int y = point.y();
        // Đảm bảo tọa độ không vượt ra ngoài biên của bảng cờ

    if (x < margin) x = margin;
    if (x > fullWidth - margin) x = fullWidth - margin;
    if (y < margin) y = margin;
    if (y > fullWidth - margin) y = fullWidth - margin;
    // Tính toán cột và hàng dựa trên tọa độ pixel chuột

    int col = (int) round(static_cast<double>(x - margin) / cellWidth);
    int row = boardSize - 1 - (int) round(static_cast<double>(y - margin) / cellWidth);

    char colChar = (char) (col + 'A');
    if (colChar >= 'I') colChar++;

    return QString(colChar) + QString::number(row + 1);// Trả về tọa độ trên bảng cờ dưới dạng chuỗi
}

QPoint GameBoardWidget::coordsToPoint(QString coords) {//Chuyển đổi tọa độ trên bảng cờ thành tọa độ pixel.
    // Chuyển đổi tọa độ trên bảng cờ thành tọa độ pixel.

    char colChar = coords[0].toLatin1();
    if (colChar >= 'J') colChar--;

    int col = colChar - 'A';// Xác định cột dựa trên ký tự A-Z

    int row = coords.mid(1).toInt() - 1;// Xác định hàng từ phần sau ký tự đầu tiên của chuỗi

    double x = margin + col * cellWidth;// Xác định tọa độ x
    double y = fullWidth - (margin + row * cellWidth);// Xác định tọa độ y, đảm bảo hệ tọa độ từ góc trên bên trái

    return QPoint(x, y);
}
