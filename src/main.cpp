#include <iostream>
#include <QApplication>
#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <cstdlib>
#include <thread>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QIcon>
#include <mutex>
std::mutex mtx;
bool is_nothing = false;
using namespace std;
void showNotification(const QString &title, const QString &message) {
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon();
    trayIcon->setIcon(QIcon::fromTheme("notification"));
    trayIcon->setVisible(true);
    trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 3000);
}
void Clear_Output(QTextEdit* text){
    text->clear();
}
int Check_For_Updates(QApplication &app, QTextEdit* &text){
    if (!text) {
        // Windowless case
        std::thread command_windowless([&]() {
            FILE* command_e = popen("echo | pkexec pacman -Syu 2>&1", "r");
            if (command_e == nullptr) {
                std::cerr << "Failed to run command\n";
                return;
            }

            char buffer[8192];
            string output;
            while (fgets(buffer, sizeof(buffer), command_e) != nullptr) {
                std::lock_guard<std::mutex> lock(mtx);
                output += buffer;
             //   cout << output << endl;
            }
            
            if (output.find("There is nothing to do") != string::npos){
                is_nothing = true;
            } else {
                is_nothing = false;
            }
            int exit_code = pclose(command_e);
        });
        cout << "Testing" << endl;
        command_windowless.join();
        if (is_nothing == false){
            showNotification("Update", "Your system is up to date!");
            
        }
        QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
        return 0;
    }
    std::thread command_executer([&](){
        FILE* command_e = popen("echo | pkexec pacman -Syu 2>&1", "r");
        if (command_e == nullptr) {
            std::cerr << "Failed to run command\n";
            return 1;
        }
        char buffer[8192];
        string output;
        while (fgets(buffer, sizeof(buffer), command_e) != nullptr) {
            output += buffer;
            QMetaObject::invokeMethod(text, "append", Qt::QueuedConnection,
                Q_ARG(QString, QString::fromLocal8Bit(buffer)));
        }
        int exit_code = pclose(command_e);
        if (exit_code == 0){
            thread Notif_serv([&]() {
                if (output.find("There is nothing to do")){
                    showNotification("Update", "Your system is up to date!");
                } else {
                    showNotification("Update", "Your system has been updated successfully!");
                }
                
            }); 
            Notif_serv.detach();
            return 0;
        }
        text->ensureCursorVisible();
        
    });
    command_executer.detach();
    return 0;
    
}

int main(int argc, char* argv[]) {
    bool Start_Window = true;
    for (int i = 1; i < argc; ++i) {
        if (string(argv[i]) == "-Wl") {
            Start_Window = false;
            break;
        }
    }

    if (Start_Window) {
        QApplication app(argc, argv);
        cout << "LOG: Checking for updates.." << endl;

        QWidget window;
        QTextEdit *text = new QTextEdit("", &window);
        window.setWindowTitle("Aquium Updator");
        window.setFixedSize(800, 400); 
        text->setGeometry(0, 0, 400, 400);

        QPushButton *button1 = new QPushButton("Check for updates", &window);
        button1->setGeometry(480, 40, 150, 50);
        QPushButton *clear_botton = new QPushButton("Clear output!",&window);
        clear_botton->setGeometry(480,100,150,50);
        QObject::connect(button1, &QPushButton::clicked, [&]() {
            Check_For_Updates(app,text);
        });
        QObject::connect(clear_botton,&QPushButton::clicked, [&](){
            Clear_Output(text);
        });
        
        window.show();

        return app.exec();  
    }

    if (!Start_Window) {
        QApplication app(argc, argv);
        QTextEdit* dummyText = nullptr;  
        Check_For_Updates(app,dummyText);
        return app.exec();  
    }

    return 0;
}
