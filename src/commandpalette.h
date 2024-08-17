#pragma once

#include <QAbstractItemModel>
#include <QFrame>

class QLineEdit;
class QListView;
class QSortFilterProxyModel;

class CommandPalette : public QFrame
{
    Q_OBJECT

public:
    CommandPalette(QWidget *parent = nullptr);
    void setDataModel(QAbstractItemModel *);
    void setRootIndex(const QModelIndex &index);

public slots:
    void selectPrev();
    void selectNext();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void updateVisibility();

private:
    void handleKeyPress(QKeyEvent *event);
    void adjustPosition();
    void adjustSize();

    QLineEdit *lineEdit;
    QListView *listView;
    QSortFilterProxyModel *filterModel;
};
