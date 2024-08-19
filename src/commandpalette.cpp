#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QMenuBar>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QToolBar>
#include <QVBoxLayout>

#include <CommandPaletteWidget/CommandPalette>

CommandPalette::CommandPalette(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);
    setLineWidth(2);
    setAutoFillBackground(true);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    lineEdit = new QLineEdit(this);
    lineEdit->setPlaceholderText("Type something here...");
    lineEdit->installEventFilter(this);

    listView = new QListView(this);
    listView->setAlternatingRowColors(true);

    filterModel = new QSortFilterProxyModel(this);
    filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    listView->setModel(filterModel);

    connect(lineEdit, &QLineEdit::textChanged, [this](const QString &text) {
        filterModel->setFilterFixedString(text);
        if (text.isEmpty()) {
            listView->setCurrentIndex(filterModel->mapFromSource(rootIndex));
            listView->setRootIndex(filterModel->mapFromSource(rootIndex));
        }
        updateVisibility();
    });
    connect(lineEdit, &QLineEdit::returnPressed, this, [this]() {
        hide();
        auto selected = listView->currentIndex();
        if (selected.isValid()) {
            emit didChooseItem(selected, filterModel->sourceModel());
        }
    });
    connect(listView, &QAbstractItemView::activated, this, [this](QModelIndex index) {
        hide();
        if (index.isValid()) {
            emit didChooseItem(index, filterModel->sourceModel());
        }
    });

    layout->addWidget(lineEdit);
    layout->addWidget(listView);
    setLayout(layout);

    if (auto parentWidget = this->parentWidget()) {
        parentWidget->installEventFilter(this);
    }
    installEventFilter(this);
    hide();
}

void CommandPalette::setDataModel(QAbstractItemModel *model)
{
    filterModel->setSourceModel(model);
    if (model) {
        rootIndex = model->index(0, 0);
        rootIndex = filterModel->mapFromSource(rootIndex);
    }
}

void CommandPalette::setRootIndex(const QModelIndex &index)
{
    rootIndex = index;
    if (index.isValid()) {
        listView->setRootIndex(filterModel->mapFromSource(rootIndex));
    }
}

void CommandPalette::clearText()
{
    lineEdit->clear();
}

bool CommandPalette::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == parentWidget() && event->type() == QEvent::Resize) {
        adjustPosition();
        adjustSize();
        return true;
    }

    if (event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            hide();
            return true;
        }
    }

    if (obj == lineEdit && event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
            handleKeyPress(keyEvent);
            return true;
        }
    }
    return QFrame::eventFilter(obj, event);
}

void CommandPalette::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    adjustPosition();
    adjustSize();
}

void CommandPalette::hideEvent(QHideEvent *event)
{
    QFrame::hideEvent(event);
}

void CommandPalette::updateVisibility()
{
    auto visibleRowCount = 0;
    for (auto row = 0; row < filterModel->rowCount(); ++row) {
        auto index = filterModel->index(row, 0);
        if (index.isValid()
            && filterModel->data(index, Qt::DisplayRole)
                   .toString()
                   .contains(filterModel->filterRegularExpression())) {
            ++visibleRowCount;
        }
    }
    listView->setVisible(visibleRowCount > 0);
    adjustSize();
}

void CommandPalette::handleKeyPress(QKeyEvent *event)
{
    if (!listView->isVisible()) {
        return;
    }

    switch (event->key()) {
    case Qt::Key_Up:
        selectPrev();
        break;
    case Qt::Key_Down:
        selectNext();
        break;
    default:
        break;
    }
}

void CommandPalette::selectNext()
{
    auto currentIndex = listView->currentIndex();
    auto rowCount = filterModel->rowCount();
    if (!currentIndex.isValid()) {
        if (rowCount > 0) {
            auto sourceRootIndex = filterModel->mapToSource(rootIndex);
            listView->setCurrentIndex(filterModel->mapFromSource(sourceRootIndex));
        }
        return;
    }

    auto currentRow = currentIndex.row();
    auto rowBelow = currentRow + 1;
    if (rowBelow < rowCount) {
        auto indexBelow = filterModel->index(rowBelow, currentIndex.column());
        listView->setCurrentIndex(indexBelow);
    } else if (rowCount > 0) {
        auto firstIndex = filterModel->index(0, 0);
        listView->setCurrentIndex(firstIndex);
    }
}

void CommandPalette::selectPrev()
{
    auto currentIndex = listView->currentIndex();
    auto rowCount = filterModel->rowCount();
    if (!currentIndex.isValid()) {
        if (rowCount > 0) {
            auto lastIndex = filterModel->index(rowCount - 1, 0);
            listView->setCurrentIndex(lastIndex);
        }
        return;
    }

    auto currentRow = currentIndex.row();
    auto rowAbove = currentRow - 1;
    if (rowAbove >= 0) {
        auto indexAbove = filterModel->index(rowAbove, currentIndex.column());
        listView->setCurrentIndex(indexAbove);
    } else if (rowCount > 0) {
        auto lastIndex = filterModel->index(rowCount - 1, 0);
        listView->setCurrentIndex(lastIndex);
    }
}

void CommandPalette::adjustPosition()
{
    if (auto parentWidget = this->parentWidget()) {
        auto rect = parentWidget->rect();
        auto x = (rect.width() - width()) / 2;
        auto y = 50;
        move(x, y);
        lineEdit->setFocus();
        raise();
    }
}

void CommandPalette::adjustSize()
{
    auto lineEditHeight = lineEdit->sizeHint().height();
    auto margins = layout()->contentsMargins();
    auto lineEditTotalHeight = lineEditHeight + margins.top() + margins.bottom();

    // Determine the width to use
    int desiredWidth = 400;
    if (auto parentWidget = this->parentWidget()) {
        int parentWidth = parentWidget->width();
        int maxWidth = parentWidth - 50; // 50 pixels padding
        setFixedWidth(std::min(desiredWidth, maxWidth));
    } else {
        setFixedWidth(desiredWidth); // Default width if no parent
    }

    // Adjust the height based on visibility of the list view
    if (listView->isVisible()) {
        auto itemHeight = listView->sizeHintForRow(0);
        if (itemHeight < 0) {
            itemHeight = lineEditHeight; // Fallback if item height is not valid
        }
        auto numItems = 7;
        auto totalHeight = lineEditTotalHeight + numItems * itemHeight;
        setFixedHeight(totalHeight);
    } else {
        setFixedHeight(lineEditTotalHeight);
    }
}

QList<QAction *> collectActionsFromMenu(QMenu *menu, QSet<QAction *> &visitedActions)
{
    QList<QAction *> actions;

    if (!menu)
        return actions;

    for (auto *action : menu->actions()) {
        if (!visitedActions.contains(action)) {
            actions.append(action);
            visitedActions.insert(action);
        }

        if (auto *subMenu = action->menu()) {
            actions += collectActionsFromMenu(subMenu, visitedActions);
        }
    }

    return actions;
}

// Wrapper function to initialize the visited actions set
QList<QAction *> collectActionsFromMenu(QMenu *menu)
{
    QSet<QAction *> visitedActions;
    return collectActionsFromMenu(menu, visitedActions);
}

QList<QAction *> collectWidgetActions(QMainWindow *mainWindow)
{
    QList<QAction *> actions;

    // Collect actions from toolbars
    for (auto *toolbar : mainWindow->findChildren<QToolBar *>()) {
        actions += toolbar->actions();
    }

    // Collect actions from menu bar
    if (auto *menuBar = mainWindow->menuBar()) {
        for (auto *action : menuBar->actions()) {
            actions += collectActionsFromMenu(action->menu());
        }
    }

    // Collect actions from subwidgets
    for (auto *widget : mainWindow->findChildren<QWidget *>()) {
        for (auto *action : widget->findChildren<QAction *>()) {
            if (!actions.contains(action)) {
                actions.append(action);
            }
        }
    }

    return actions;
}

ActionListModel::ActionListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

void ActionListModel::setActions(const QList<QAction *> &actions)
{
    beginResetModel();
    m_actions = actions;
    endResetModel();
}

int ActionListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_actions.size();
}

QVariant ActionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_actions.size()) {
        return QVariant();
    }

    auto action = m_actions.at(index.row());

    switch (role) {
    case IconRole:
        return action->icon();
    case TextRole:
        return action->text();
    case ShortcutRole:
        return action->shortcut().toString();
    case Qt::UserRole:
        return QVariant::fromValue(action);
    default:
        return QVariant();
    }
}

ActionDelegate::ActionDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void ActionDelegate::paint(QPainter *painter,
                           const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    QIcon icon = index.data(ActionListModel::IconRole).value<QIcon>();
    QString text = index.data(ActionListModel::TextRole).toString();
    QString shortcut = index.data(ActionListModel::ShortcutRole).toString();

    QRect rect = option.rect;
    QRect iconRect = QRect(rect.left() + 5, rect.top() + 5, 16, 16);
    QRect textRect = QRect(rect.left() + 26, rect.top(), rect.width() - 26, rect.height() / 2);
    QRect shortcutRect = QRect(rect.left() + 26,
                               rect.top() + rect.height() / 2,
                               rect.width() - 26,
                               rect.height() / 2);

    icon.paint(painter, iconRect, Qt::AlignCenter);

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
    painter->drawText(shortcutRect, Qt::AlignRight | Qt::AlignVCenter, shortcut);
}

QSize ActionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(200, 40); // Adjust as needed
}
