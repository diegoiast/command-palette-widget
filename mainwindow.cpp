#include "mainwindow.h"

#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStringListModel>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto centralWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(centralWidget);
    auto toggleCommandPaletteAction = new QAction(tr("Toggle Command Palette"), this);
    auto fileMenu = menuBar()->addMenu(tr("File"));
    commandPalette = new CommandPalette(this);
    textEdit = new QTextEdit(this);

    mainLayout->addWidget(textEdit);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    toggleCommandPaletteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(toggleCommandPaletteAction,
            &QAction::triggered,
            this,
            &MainWindow::toggleCommandPaletteVisibility);
    fileMenu->addAction(toggleCommandPaletteAction);

#if 0
    auto model1 = new QFileSystemModel(this);
    auto rootPath = QDir::currentPath();
    model1->setRootPath(rootPath);
    commandPalette->setDataModel(model1);
    auto index = model1->index(rootPath);
    commandPalette->setRootIndex(index);
#else
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
    commandPalette->setDataModel(model2);
#endif
}

void MainWindow::toggleCommandPaletteVisibility()
{
    if (commandPalette) {
        if (commandPalette->isVisible()) {
            commandPalette->hide();
        } else {
            commandPalette->show();
        }
    }
}
