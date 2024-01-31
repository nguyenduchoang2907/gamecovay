#include "challengewindow.h"
#include "ui_challengewindow.h"
#include "socket.h"
#include "playwidget.h"
#include "mainwindow.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QMainWindow>
#include <QDebug>

ChallengeWindow::ChallengeWindow(QWidget *parent)
    : QMainWindow(parent)
        // Khởi tạo cửa sổ giao diện người dùng

    , ui(new Ui::ChallengeWindow)
{
    ui->setupUi(this);
    invitedUsername = "";        // Khởi tạo biến lưu tên người chơi được mời

    socket = Socket::getInstance();    // Lấy getInstance của socket từ lớp Socket (Socket Singleton)

    // Kết nối các sự kiện từ socket và UI với các hàm xử lý tương ứng

    connect(socket, &Socket::messageReceived, this, &ChallengeWindow::onMessageReceived);
    connect(ui->list_online, &QListWidget::currentTextChanged, this, &ChallengeWindow::onPlayerChanged);
    connect(MainWindow::getInstance(), SIGNAL(matchSetUp()), this, SLOT(hide()));

    ui->btn_challenge->setDisabled(true);    // Vô hiệu hóa nút thách đấu khi cửa sổ được tạo

    socket->sendMessage("LSTONL");    // Gửi tin nhắn yêu cầu danh sách người chơi đang online ("LSTONL") đến máy chủ

}

ChallengeWindow::~ChallengeWindow()
{
    delete ui;    // Hủy đối tượng UI

}

// Hàm xử lý khi nhận messeage từ máy chủ

void ChallengeWindow::onMessageReceived(QString msgtype, QString payload) {
    if (msgtype == "LSTONL") {        // Xóa danh sách người chơi và cập nhật lại từ tin nhắn nhận được

        ui->list_online->clear();
        QStringList entries = payload.split("\n", Qt::SkipEmptyParts);
        for (QString entry: entries) {
            // QString username = entry.section(" ", 0, 0);
            // QString status = entry.section(" ", 1);
            // ui->list_online->addItem(username + " (" + status + ")");
            ui->list_online->addItem(entry);
        }
        return;
    }

    if (msgtype == "CHGONL") {        // Gửi yêu cầu cập nhật danh sách người chơi đang online khi có thay đổi

        socket->sendMessage("LSTONL");
        return;
    }

    if (msgtype == "INVRES") {        // Xử lý phản hồi từ người chơi khi được mời thách đấu

        QStringList params = payload.split("\n", Qt::SkipEmptyParts);
        QString username = params[0];
        QString reply = params[1];
        if (reply == "DECLINE") {            // Hiển thị thông báo nếu người chơi từ chối thách đấu

            QMessageBox::information(this, "Message", "Player " + username + " declined your challenge");
            ui->btn_challenge->setText("Challenge");
            ui->btn_challenge->setEnabled(true);
        }
        return;
    }
}
// Hàm xử lý khi người chơi được chọn thay đổi

void ChallengeWindow::onPlayerChanged(const QString &currentPlayer) {
    // if (currentPlayer.contains("In game")) {
    //     ui->btn_challenge->setDisabled(true);
    // } else {
    //     ui->btn_challenge->setEnabled(true);
    // }

    ui->btn_challenge->setEnabled(true);
}

// Hàm xử lý khi nút thách đấu được nhấn
void ChallengeWindow::on_btn_challenge_clicked()
{
    PlayWidget *playWidget = static_cast<PlayWidget *>(parentWidget());        // Lấy thể hiện của lớp PlayWidget từ widget cha

        // Lấy tên người chơi được chọn từ danh sách

    QString username = ui->list_online->currentItem()->text();
    username = username.mid(0, username.indexOf('(')).trimmed();
    int noneTimeSystem = playWidget->getTimeSystem() == 0;

    socket->sendMessage("INVITE", QString("%1\n%2\n%3\n%4 %5 %6 %7\n%8\n")    // Gửi tin nhắn mời thách đấu đến người chơi được chọn

                                      .arg(username)
                                      .arg(playWidget->getBoardSize())
                                      .arg(playWidget->getKomi())
                                      .arg(playWidget->getTimeSystem())
                                      .arg(noneTimeSystem ? -1 : playWidget->getMainTimeSeconds())
                                      .arg(noneTimeSystem ? -1 : playWidget->getByoyomiTimeSeconds())
                                      .arg(noneTimeSystem ? -1 : playWidget->getByoyomiPeriods())
                                      .arg(playWidget->getRanked()));
    ui->btn_challenge->setText("Waiting for reply...");    // Cập nhật trạng thái nút thách đấu và lưu tên người chơi được mời

    ui->btn_challenge->setEnabled(false);
    invitedUsername = username;
}

// Xử lý sự kiện đóng cửa sổ
void ChallengeWindow::closeEvent(QCloseEvent *event) {
    if (ui->btn_challenge->text().startsWith("Waiting")) {
        if (QMessageBox::question(
            this,
            "Confirmation",
            "Do you want to cancel your challenge?"
        ) == QMessageBox::Yes) {
            socket->sendMessage("INVCCL", invitedUsername + "\n");
            event->accept();
            this->deleteLater();
        } else {
            event->ignore();
        }
    }
}
