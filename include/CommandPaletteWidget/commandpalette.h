// SPDX-License-Identifier: MIT

#pragma once

#include <QAbstractItemModel>
#include <QFrame>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

class QLineEdit;
class QListView;
class QMainWindow;
class CommandPaletteFilterModel;

class CommandPalette : public QFrame {
    Q_OBJECT

  public:
    enum FilterMode {
        NoFilter = 0x0,
        RemoveAccelerators = 0x1,
        FuzzyMatch = 0x2,
        FileMatch = 0x4,
    };
    Q_DECLARE_FLAGS(FilterModes, FilterMode)

    CommandPalette(QWidget *parent = nullptr);
    void setDataModel(QAbstractItemModel *);
    void setRootIndex(const QModelIndex &index);
    void setItemDelegate(QStyledItemDelegate *delegate);
    void setFilterModes(FilterModes modes);

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
    void didSelectItem(const QModelIndex index, const QAbstractItemModel *model);
    void didHide();

  private slots:
    void updateVisibility();

  private:
    void handleKeyPress(QKeyEvent *event);
    void adjustPosition();
    void adjustSize();

    QLineEdit *lineEdit;
    QListView *listView;
    CommandPaletteFilterModel *filterModel;
    QModelIndex rootIndex;
};

class ActionListModel : public QAbstractListModel {
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
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  private:
    QList<QAction *> m_actions;
};

class ActionDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    explicit ActionDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

class CommandPaletteFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit CommandPaletteFilterModel(QObject *parent = nullptr);
    void setFilterModes(CommandPalette::FilterModes modes) {
        beginResetModel();
        m_modes = modes;
        endResetModel();
    }

  protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

  private:
    CommandPalette::FilterModes m_modes = CommandPalette::NoFilter;

    bool fuzzyMatch(const QString &haystack, const QString &needle) const;
    bool fileMatch(const QString &haystack, const QString &needle) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CommandPalette::FilterModes)

QList<QAction *> collectWidgetActions(QMainWindow *mainWindow);
