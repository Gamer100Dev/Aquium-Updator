#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QStyle>
#include <QTimer>
#include <QPalette>

class UpdaterWindow : public QWidget
{
    Q_OBJECT

public:

    UpdaterWindow(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle("Aquium Updater");
        setFixedSize(900, 600);

        qApp->setStyle("Fusion");

        setupUi();
        applyTheme();

        flushTimer = new QTimer(this);
        flushTimer->setInterval(16);

        connect(flushTimer, &QTimer::timeout, this, &UpdaterWindow::flushOutput);
        flushTimer->start();

        themeTimer = new QTimer(this);
        connect(themeTimer, &QTimer::timeout, this, &UpdaterWindow::applyTheme);
        themeTimer->start(2000);
    }

private:

    void setupUi()
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        mainLayout->setContentsMargins(28, 28, 28, 28);
        mainLayout->setSpacing(14);

        titleLabel = new QLabel("Aquium Updater");
        titleLabel->setObjectName("title");

        subtitleLabel = new QLabel("Keep your system updated and secure");
        subtitleLabel->setObjectName("subtitle");

        spinner = new QProgressBar;
        spinner->setRange(0, 0);
        spinner->setTextVisible(false);
        spinner->setFixedHeight(5);
        spinner->hide();

        output = new QPlainTextEdit;
        output->setReadOnly(true);
        output->setUndoRedoEnabled(false);

        checkButton = new QPushButton("Check for Updates");
        updateButton = new QPushButton("Update System");
        flatpakButton = new QPushButton("Update System + Flatpak");
        clearButton = new QPushButton("Clear Output");

        updateButton->hide();
        flatpakButton->hide();

        QHBoxLayout *buttonLayout = new QHBoxLayout;

        buttonLayout->addWidget(checkButton);
        buttonLayout->addWidget(updateButton);
        buttonLayout->addWidget(flatpakButton);
        buttonLayout->addWidget(clearButton);

        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(subtitleLabel);
        mainLayout->addWidget(spinner);
        mainLayout->addLayout(buttonLayout);
        mainLayout->addWidget(output);

        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
        trayIcon->show();

        process = new QProcess(this);

        connect(checkButton, &QPushButton::clicked, this, &UpdaterWindow::checkUpdates);
        connect(updateButton, &QPushButton::clicked, this, &UpdaterWindow::updateSystem);
        connect(flatpakButton, &QPushButton::clicked, this, &UpdaterWindow::updateSystemFlatpak);
        connect(clearButton, &QPushButton::clicked, output, &QPlainTextEdit::clear);

        connect(process, &QProcess::readyReadStandardOutput, this, &UpdaterWindow::readOutput);
        connect(process, &QProcess::readyReadStandardError, this, &UpdaterWindow::readOutput);

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &UpdaterWindow::processFinished);
    }

    bool isDark()
    {
        return qApp->palette().color(QPalette::Window).lightness() < 128;
    }

    void applyTheme()
    {
        if (isDark())
        {
            setStyleSheet(R"(
                QWidget { background:#1e1e1e; color:#f2f2f2; font-size:14px; }
                QLabel#title { font-size:28px; font-weight:bold; color:white; }
                QLabel#subtitle { color:#bbbbbb; }

                QPushButton {
                    background:#0a84ff;
                    color:white;
                    border-radius:12px;
                    padding:10px;
                    min-height:42px;
                    border:none;
                }

                QPushButton:hover { background:#3a9dff; }
                QPushButton:disabled { background:#555; }

                QPlainTextEdit {
                    background:#2a2a2a;
                    color:#f2f2f2;
                    border-radius:14px;
                    border:1px solid #444;
                    padding:10px;
                    font-family:monospace;
                }

                QProgressBar {
                    background:#444;
                    border-radius:3px;
                    height:5px;
                }

                QProgressBar::chunk { background:#0a84ff; }
            )");
        }
        else
        {
            setStyleSheet(R"(
                QWidget { background:#f5f5f7; color:#111; font-size:14px; }
                QLabel#title { font-size:28px; font-weight:bold; }
                QLabel#subtitle { color:#666; }

                QPushButton {
                    background:#007aff;
                    color:white;
                    border-radius:12px;
                    padding:10px;
                    min-height:42px;
                    border:none;
                }

                QPushButton:hover { background:#3395ff; }
                QPushButton:disabled { background:#999; }

                QPlainTextEdit {
                    background:white;
                    color:black;
                    border-radius:14px;
                    border:1px solid #d0d0d0;
                    padding:10px;
                    font-family:monospace;
                }

                QProgressBar {
                    background:#d0d0d0;
                    border-radius:3px;
                    height:5px;
                }

                QProgressBar::chunk { background:#007aff; }
            )");
        }
    }

private slots:

    void checkUpdates()
    {
        prepareUi();
        currentMode = "check";
        output->clear();
        subtitleLabel->setText("Checking for updates...");
        QProcess sudo_process;
        QString program = "pkexec";
        
        process->start("sh", {"-c", "checkupdates && flatpak remote-ls --updates"});
    }

    void updateSystem()
    {
        prepareUi();
        currentMode = "update";
        subtitleLabel->setText("Installing system updates...");
        process->start("pkexec", {"pacman", "-Syu", "--noconfirm"});
    }

    void updateSystemFlatpak()
    {
        prepareUi();
        currentMode = "flatpak";
        subtitleLabel->setText("Installing system and Flatpak updates...");
        process->start("sh", {"-c", "pkexec pacman -Syu --noconfirm && flatpak update -y"});
    }

    void readOutput()
    {
        pendingOutput += process->readAllStandardOutput();
        pendingOutput += process->readAllStandardError();
    }

    void flushOutput()
    {
        if (!pendingOutput.isEmpty())
        {
            output->appendPlainText(pendingOutput);
            pendingOutput.clear();
        }
    }

    void processFinished(int exitCode, QProcess::ExitStatus)
    {
        spinner->hide();

        checkButton->setEnabled(true);
        clearButton->setEnabled(true);

        if (currentMode == "check")
        {
            QString text = output->toPlainText().trimmed();

            if (text.isEmpty())
            {
                subtitleLabel->setText("Your system is up to date");
                trayIcon->showMessage("Aquium Updater", "System is up to date");
            }
            else
            {
                subtitleLabel->setText("Updates available");
                updateButton->show();
                flatpakButton->show();
                trayIcon->showMessage("Aquium Updater", "Updates available");
            }

            return;
        }

        updateButton->hide();
        flatpakButton->hide();

        if (exitCode == 0)
        {
            subtitleLabel->setText("Update completed successfully");
            trayIcon->showMessage("Aquium Updater", "Update successful");
        }
        else
        {
            subtitleLabel->setText("Update failed");
            trayIcon->showMessage("Aquium Updater", "Update failed");
        }
    }

private:

    void prepareUi()
    {
        spinner->show();

        checkButton->setEnabled(false);
        clearButton->setEnabled(false);

        updateButton->hide();
        flatpakButton->hide();
    }

    QString currentMode;

    QString pendingOutput;

    QTimer *flushTimer;
    QTimer *themeTimer;

    QLabel *titleLabel;
    QLabel *subtitleLabel;

    QPlainTextEdit *output;

    QPushButton *checkButton;
    QPushButton *updateButton;
    QPushButton *flatpakButton;
    QPushButton *clearButton;

    QProgressBar *spinner;

    QProcess *process;

    QSystemTrayIcon *trayIcon;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    UpdaterWindow window;
    window.show();

    return app.exec();
}