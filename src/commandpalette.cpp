// SPDX-License-Identifier: MIT

#include <QApplication>
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
    lineEdit->setPlaceholderText(tr("Type something here..."));
    lineEdit->installEventFilter(this);

    listView = new QListView(this);
    listView->setAlternatingRowColors(true);

    filterModel = new QSortFilterProxyModel(this);
    filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    listView->setModel(filterModel);

    connect(lineEdit, &QLineEdit::textChanged, lineEdit, [this](const QString &text) {
        filterModel->setFilterFixedString(text);
        if (text.isEmpty()) {
            listView->setCurrentIndex(filterModel->mapFromSource(rootIndex));
            listView->setRootIndex(filterModel->mapFromSource(rootIndex));
        }
        updateVisibility();
    });
    connect(lineEdit, &QLineEdit::returnPressed, this, [this]() {
        auto selected = listView->currentIndex();
        if (selected.isValid()) {
            selected = filterModel->mapToSource(selected);
            emit didChooseItem(selected, filterModel->sourceModel());
        }
        hide();
    });
    connect(listView, &QAbstractItemView::activated, this, [this](QModelIndex index) {
        if (index.isValid()) {
            index = filterModel->mapToSource(index);
            emit didChooseItem(index, filterModel->sourceModel());
        }
        hide();
    });
    connect(listView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [this](const QItemSelection &selected, const QItemSelection &) {
                if (selected.indexes().size() == 0) {
                    return;
                }
                auto index = selected.indexes().first();
                emit didSelectItem(index, filterModel->sourceModel());
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

void CommandPalette::setItemDelegate(QStyledItemDelegate *delegate)
{
    listView->setItemDelegate(delegate);
}

void CommandPalette::clearText()
{
    lineEdit->clear();
}

bool CommandPalette::eventFilter(QObject *obj, QEvent *event)
{
    if (!isVisible()){
        return false;
    }

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
    emit didHide();
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

    int desiredWidth = 400;
    if (auto parentWidget = this->parentWidget()) {
        int parentWidth = parentWidget->width();
        int maxWidth = parentWidth - 50;
        setFixedWidth(std::min(desiredWidth, maxWidth));
    } else {
        setFixedWidth(desiredWidth);
    }

    if (listView->isVisible()) {
        auto itemHeight = listView->sizeHintForRow(0);
        if (itemHeight < 0) {
            itemHeight = lineEditHeight;
        }
        auto numItems = 7;
        auto totalHeight = lineEditTotalHeight + numItems * itemHeight;
        setFixedHeight(totalHeight);
    } else {
        setFixedHeight(lineEditTotalHeight);
    }
}

static auto collectActionsFromMenu(QList<QAction *> &actions, QMenu *menu) -> void
{
    if (!menu) {
        return;
    }

    for (auto &action : menu->actions()) {
        if (action->text().isEmpty()) {
            continue;
        }
        if (!actions.contains(action)) {
            actions.push_back(action);
        }

        if (auto *subMenu = action->menu()) {
            collectActionsFromMenu(actions, subMenu);
        }
    }
}

QList<QAction *> collectWidgetActions(QMainWindow *mainWindow)
{
    auto addActions = [](auto &originalActions, auto const  &newActions) {
        for (auto &action : newActions) {
            if (action->text().isEmpty()) {
                continue;
            }
            if (originalActions.contains(action)) {
              continue;
            }
            originalActions += action;
        }
    };
    
    QList<QAction *> allActions;
    for (auto &toolbar : mainWindow->findChildren<QToolBar *>()) {
        addActions(allActions, toolbar->actions());
    }

    if (auto menuBar = mainWindow->menuBar()) {
        for (auto &action : menuBar->actions()) {
            if (action->text().isEmpty()) {
                continue;
            }
            collectActionsFromMenu(allActions, action->menu());
        }
    }
    
    auto focused = mainWindow->focusWidget();
    if (focused) {
      addActions(allActions, focused->findChildren<QAction *>());
    }
    addActions(allActions, mainWindow->actions());
    return allActions;
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
        return {};
    }

    auto action = m_actions.at(index.row());

    switch (role) {
    case IconRole:
        if (action->icon().isNull()) {
            QIcon themeIcon = QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew);
            QSize iconSize = themeIcon.actualSize(QSize(32, 32));
            QPixmap emptyPixmap(iconSize);
            emptyPixmap.fill(Qt::transparent);
            return QIcon(emptyPixmap);
        }
        return action->icon();
    case TextRole:
        return action->text();
    case ShortcutRole:
        return action->shortcut().toString();
    case Qt::UserRole:
        return QVariant::fromValue(action);
    default:
        return {};
    }
}

ActionDelegate::ActionDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void ActionDelegate::paint(QPainter *painter,
                           const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    painter->save();
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    } else {
        painter->setPen(option.palette.text().color());
    }

    auto iconSize = option.widget->style()->pixelMetric(QStyle::PM_ListViewIconSize, &option, option.widget);
    auto margin = option.widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, option.widget);
    auto icon = index.data(ActionListModel::IconRole).value<QIcon>();
    auto text = index.data(ActionListModel::TextRole).toString();
    auto shortcut = index.data(ActionListModel::ShortcutRole).toString();
    auto rect = option.rect;
    auto padding = iconSize + 2 * margin;
    auto iconRect = QRect(rect.left() + margin, rect.top() + (rect.height() - iconSize) / 2, iconSize, iconSize);
    auto textRect = QRect(rect.left() + padding, rect.top(), rect.width() - padding - margin, rect.height());

    icon.paint(painter, iconRect, Qt::AlignCenter);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, text);
    painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, shortcut);
    painter->restore();
}
