#include <QMainWindow>
#include "src/commandpalette.h"

class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void toggleCommandPaletteVisibility();

private:
    QTextEdit *textEdit;
    CommandPalette *commandPalette;
};
