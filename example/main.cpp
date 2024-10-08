// SPDX-License-Identifier: MIT

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileSystemModel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QStringListModel>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <CommandPaletteWidget/CommandPalette>

class MainWindow : public QMainWindow
{
    CommandPalette *commandPalette;
    QTextEdit *textEdit;

public:
    MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , commandPalette(new CommandPalette(this))
        , textEdit(new QTextEdit(this))
    {
        auto centralWidget = new QWidget(this);
        auto mainLayout = new QVBoxLayout(centralWidget);
        auto toolbar = addToolBar(tr("Main Toolbar"));
        auto fileMenu = menuBar()->addMenu(tr("File"));

        auto rootPath = QDir::currentPath();
        auto model1 = new QFileSystemModel(this);
        auto index = model1->index(rootPath);
        auto model2 = new QStringListModel({"All you need is love.",
                                            "We all want to change the world.",
                                            "Here comes the sun.",
                                            "I get by with a little help from my friends.",
                                            "With a little help from my friends.",
                                            "Let it be.",
                                            "Come together, right now.",
                                            "I want to hold your hand.",
                                            "Help! I need somebody.",
                                            "Imagine all the people living life in peace.",
                                            "You say goodbye and I say hello.",
                                            "Yesterday, all my troubles seemed so far away.",
                                            "I am the walrus.",
                                            "Lucy in the sky with diamonds.",
                                            "Hey Jude, don't make it bad."},
                                           this);

        auto model3 = new ActionListModel(this);

        auto quitAction = new QAction(tr("Quit"), this);
        quitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
        connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
        toolbar->addAction(quitAction);

        mainLayout->addWidget(textEdit);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        centralWidget->setLayout(mainLayout);
        setCentralWidget(centralWidget);

        auto chooseFile = new QAction(tr("Choose file"), this);
        chooseFile->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
        fileMenu->addAction(chooseFile);
        toolbar->addAction(chooseFile);
        connect(chooseFile, &QAction::triggered, this, [this, model1, index, rootPath]() {
            if (commandPalette->isVisible()) {
                commandPalette->hide();
            } else {
                model1->setRootPath(rootPath);
                commandPalette->setDataModel(model1);
                commandPalette->setRootIndex(index);
                commandPalette->clearText();
                commandPalette->show();
            }
        });

        auto chooseString = new QAction(tr("Choose quote"), this);
        fileMenu->addAction(chooseString);
        toolbar->addAction(chooseString);
        chooseString->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
        connect(chooseString, &QAction::triggered, this, [this, model2]() {
            if (commandPalette->isVisible()) {
                commandPalette->hide();
            } else {
                commandPalette->setDataModel(model2);
                commandPalette->clearText();
                commandPalette->show();
            }
        });

        auto chooseCommands = new QAction(tr("Choose commands"), this);
        fileMenu->addAction(chooseCommands);
        toolbar->addAction(chooseCommands);
        chooseCommands->setShortcut(QKeySequence(Qt::ALT | Qt::CTRL | Qt::Key_P));
        model3->setActions(collectWidgetActions(this));
        connect(chooseCommands, &QAction::triggered, this, [this, model3]() {
            if (commandPalette->isVisible()) {
                commandPalette->hide();
            } else {
                commandPalette->setDataModel(model3);
                commandPalette->clearText();
                commandPalette->show();
            }
        });

        fileMenu->addSeparator();
        fileMenu->addAction(quitAction);

        connect(commandPalette,
                &CommandPalette::didChooseItem,
                this,
                [model1, model2, model3](const QModelIndex index, const QAbstractItemModel *model) {
                    if (model == model1) {
                        QMessageBox msgBox;
                        msgBox.setWindowTitle("File");
                        msgBox.setText(index.data().toString());
                        msgBox.exec();
                    }
                    if (model == model2) {
                        QMessageBox msgBox;
                        msgBox.setWindowTitle("Message");
                        msgBox.setText(index.data().toString());
                        msgBox.exec();
                    }
                    if (model == model3) {
                        auto data = model3->data(index, Qt::UserRole);
                        auto action = data.value<QAction *>();
                        if (action) {
                            action->trigger();
                        }
                    }
                });
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(800, 600);
    window.show();
    return app.exec();
}
