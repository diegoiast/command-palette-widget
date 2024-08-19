// SPDX-License-Identifier: MIT

#pragma once

#include <QAbstractItemModel>
#include <QFrame>
#include <QSet>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

class QLineEdit;
class QListView;
class QSortFilterProxyModel;
class QMainWindow;

class CommandPalette : public QFrame
{
    Q_OBJECT

public:
    CommandPalette(QWidget *parent = nullptr);
    void setDataModel(QAbstractItemModel *);
    void setRootIndex(const QModelIndex &index);

public slots:
    void clearText();
    void selectPrev();
    void selectNext();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

signals:
    void didChooseItem(const QModelIndex index, const QAbstractItemModel *model);

private slots:
    void updateVisibility();

private:
    void handleKeyPress(QKeyEvent *event);
    void adjustPosition();
    void adjustSize();

    QLineEdit *lineEdit;
    QListView *listView;
    QSortFilterProxyModel *filterModel;
    QModelIndex rootIndex;
};

class ActionListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IconRole = Qt::DecorationRole,
        TextRole = Qt::DisplayRole,
        ShortcutRole = Qt::UserRole + 1
    };

    explicit ActionListModel(QObject *parent = nullptr);
    void setActions(const QList<QAction *> &actions);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QList<QAction *> m_actions; // Stores the actions directly
};

// TODO unused yet, eventually will be used for the ActionsListModel
class ActionDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ActionDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

QList<QAction *> collectWidgetActions(QMainWindow *mainWindow);
