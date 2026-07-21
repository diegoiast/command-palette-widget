// SPDX-License-Identifier: MIT

#include "CommandPaletteWidget/commandpalette.h"
#include <QApplication>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QMenuBar>
#include <QPainter>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "fuzzy.h"

#include <CommandPaletteWidget/CommandPalette>
#include <qnamespace.h>

namespace {

auto shadowColorForPalette(const QPalette &palette) -> QColor {
    auto window = palette.color(QPalette::Window);
    auto luminance = 0.299 * window.redF() + 0.587 * window.greenF() + 0.114 * window.blueF();
    return luminance < 0.5 ? QColor(255, 255, 255, 140) : QColor(0, 0, 0, 180);
}

} // namespace

CommandPalette::CommandPalette(QWidget *parent) : QFrame(parent) {
    setFrameShape(QFrame::StyledPanel);
    setStyleSheet(QString(R"(
        CommandPalette {
            background-color: palette(base);
            border: 1px solid palette(mid);
        }
    )"));

    auto shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(48);
    shadow->setOffset(0, 8);
    shadow->setColor(shadowColorForPalette(palette()));
    setGraphicsEffect(shadow);
    shadowEffect = shadow;

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    lineEdit = new QLineEdit(this);
    lineEdit->setPlaceholderText(tr("Type something here..."));
    lineEdit->setClearButtonEnabled(true);
    lineEdit->installEventFilter(this);
    lineEdit->setStyleSheet(QStringLiteral(R"(
        QLineEdit {
            padding: 5px 6px;
            border: none;
            border-bottom: 2px solid palette(highlight);
            background: transparent;
        }
    )"));

    listView = new QListView(this);
    listView->setAlternatingRowColors(true);
    listView->setStyleSheet(QString(R"(
        QListView {
            border: none;
            background: transparent;
            outline: none;
        }
        QScrollBar:vertical {
            width: 8px;
            background: transparent;
            margin: 4px 2px 4px 0px;
        }
        QScrollBar::handle:vertical {
            background: palette(mid);
            min-height: 24px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
            background: none;
            border: none;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical {
            width: 0px;
            height: 0px;
            background: none;
            border: none;
        }
        QScrollBar:horizontal {
            height: 8px;
            background: transparent;
            margin: 0px 4px 2px 4px;
        }
        QScrollBar::handle:horizontal {
            background: palette(mid);
            min-width: 24px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
            background: none;
            border: none;
        }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
        }
        QScrollBar::left-arrow:horizontal, QScrollBar::right-arrow:horizontal {
            width: 0px;
            height: 0px;
            background: none;
            border: none;
        }
    )"));

    filterModel = new CommandPaletteFilterModel(this);
    filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    filterModel->sort(0);
    listView->setModel(filterModel);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(filterModel, &QAbstractItemModel::rowsInserted, this, &CommandPalette::updateVisibility);
    connect(filterModel, &QAbstractItemModel::modelReset, this, &CommandPalette::updateVisibility);

    filterDebounceTimer = new QTimer(this);
    filterDebounceTimer->setSingleShot(true);
    filterDebounceTimer->setInterval(300);
    connect(filterDebounceTimer, &QTimer::timeout, this, &CommandPalette::updateFilter);

    connect(lineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (text.isEmpty()) {
            filterDebounceTimer->stop();
            updateFilter();
        } else {
            filterDebounceTimer->start();
        }
    });
    connect(lineEdit, &QLineEdit::returnPressed, this, [this]() {
        auto selected = listView->currentIndex();
        if (selected.isValid()) {
            selected = filterModel->mapToSource(selected);
            auto *sourceModel = filterModel->sourceModel();
            hide();
            emit didChooseItem(selected, sourceModel);
        } else {
            hide();
        }
    });
    connect(listView, &QAbstractItemView::activated, this, [this](QModelIndex index) {
        if (index.isValid()) {
            index = filterModel->mapToSource(index);
            auto *sourceModel = filterModel->sourceModel();
            hide();
            emit didChooseItem(index, sourceModel);
        } else {
            hide();
        }
    });
    connect(listView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
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

void CommandPalette::setDataModel(QAbstractItemModel *model) {
    filterModel->setSourceModel(model);
    rootIndex = QModelIndex();
}

void CommandPalette::setRootIndex(const QModelIndex &index) {
    rootIndex = index;
    if (index.isValid()) {
        listView->setRootIndex(filterModel->mapFromSource(rootIndex));
    }
}

void CommandPalette::setItemDelegate(QStyledItemDelegate *delegate) {
    listView->setItemDelegate(delegate);
}

void CommandPalette::setFilterModes(FilterModes modes) { filterModel->setFilterModes(modes); }

void CommandPalette::clearText() { lineEdit->clear(); }

bool CommandPalette::eventFilter(QObject *obj, QEvent *event) {
    if (!isVisible()) {
        return false;
    }

    if (obj == parentWidget() && event->type() == QEvent::Resize) {
        adjustSize();
        adjustPosition();
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

void CommandPalette::showEvent(QShowEvent *event) {
    QFrame::showEvent(event);
    adjustSize();
    adjustPosition();
}

void CommandPalette::hideEvent(QHideEvent *event) {
    QFrame::hideEvent(event);
    emit didHide();
}

void CommandPalette::changeEvent(QEvent *event) {
    QFrame::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        shadowEffect->setColor(shadowColorForPalette(palette()));
    }
}

void CommandPalette::updateVisibility() {
    auto hasVisibleRows = filterModel->rowCount(listView->rootIndex()) > 0;
    listView->setVisible(hasVisibleRows);
    adjustSize();
}

void CommandPalette::updateFilter() {
    filterModel->setFilterFixedString(lineEdit->text());
    filterModel->refreshSorting();
    auto root = filterModel->mapFromSource(rootIndex);
    listView->setRootIndex(root);

    if (filterModel->rowCount(root) > 0) {
        auto firstIndex = filterModel->index(0, 0, root);
        listView->setCurrentIndex(firstIndex);
    } else {
        listView->setCurrentIndex(QModelIndex());
    }
    updateVisibility();
}

void CommandPalette::handleKeyPress(QKeyEvent *event) {
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

void CommandPalette::selectNext() {
    auto root = listView->rootIndex();
    auto currentIndex = listView->currentIndex();
    auto rowCount = filterModel->rowCount(root);

    if (!currentIndex.isValid()) {
        if (rowCount > 0) {
            auto firstIndex = filterModel->index(0, 0, root);
            listView->setCurrentIndex(firstIndex);
            listView->scrollTo(firstIndex);
            update();
        }
        return;
    }

    auto currentRow = currentIndex.row();
    auto rowBelow = currentRow + 1;

    if (rowBelow < rowCount) {
        auto indexBelow = filterModel->index(rowBelow, currentIndex.column(), root);
        listView->setCurrentIndex(indexBelow);
        listView->scrollTo(indexBelow);
    } else if (rowCount > 0) {
        auto firstIndex = filterModel->index(0, 0, root);
        listView->setCurrentIndex(firstIndex);
        listView->scrollTo(firstIndex);
    }
    update();
}

void CommandPalette::selectPrev() {
    auto root = listView->rootIndex();
    auto currentIndex = listView->currentIndex();
    auto rowCount = filterModel->rowCount(root);

    if (!currentIndex.isValid()) {
        if (rowCount > 0) {
            auto lastIndex = filterModel->index(rowCount - 1, 0, root);
            listView->setCurrentIndex(lastIndex);
            listView->scrollTo(lastIndex);
            update();
        }
        return;
    }

    auto currentRow = currentIndex.row();
    auto rowAbove = currentRow - 1;

    if (rowAbove >= 0) {
        auto indexAbove = filterModel->index(rowAbove, currentIndex.column(), root);
        listView->setCurrentIndex(indexAbove);
        listView->scrollTo(indexAbove);
    } else if (rowCount > 0) {
        auto lastIndex = filterModel->index(rowCount - 1, 0, root);
        listView->setCurrentIndex(lastIndex);
        listView->scrollTo(lastIndex);
    }
    update();
}

void CommandPalette::adjustPosition() {
    if (auto parentWidget = this->parentWidget()) {
        auto rect = parentWidget->rect();
        auto x = (rect.width() - width()) / 2;
        auto y = 50;

        move(x, y);
        lineEdit->setFocus();
        raise();
    }
}

void CommandPalette::adjustSize() {
    auto margins = layout()->contentsMargins();
    auto lineEditHeight = lineEdit->sizeHint().height();
    auto frameWidth = this->frameWidth();
    auto focusMargin = style()->pixelMetric(QStyle::PM_FocusFrameVMargin, nullptr, lineEdit);
    auto spacing = layout()->spacing();
    auto lineEditTotalHeight = lineEditHeight + margins.top() + margins.bottom() +
                               2 * (frameWidth + focusMargin) + spacing;
    auto desiredWidth = 400;

    if (auto parentWidget = this->parentWidget()) {
        auto parentWidth = parentWidget->width();
        auto maxWidth = parentWidth - 50;
        setFixedWidth(std::min(desiredWidth, maxWidth));
    } else {
        setFixedWidth(desiredWidth);
    }

    if (listView->isVisible()) {
        auto maxVisibleItems = 10;
        auto rowCount = filterModel->rowCount(listView->rootIndex());
        auto visibleItems = std::min(rowCount, maxVisibleItems);
        auto rowsHeight = 0;
        auto hScrollNeeded = false;
        QStyleOptionViewItem option;

        auto availableWidth = width() - margins.left() - margins.right() - 2 * frameWidth -
                              2 * listView->frameWidth() - listView->contentsMargins().left() -
                              listView->contentsMargins().right();

        if (rowCount > maxVisibleItems) {
            availableWidth -= style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, listView);
        }

        option.initFrom(listView);
        for (auto i = 0; i < visibleItems; ++i) {
            rowsHeight += listView->sizeHintForRow(i);
            if (!hScrollNeeded) {
                if (listView->itemDelegate()
                        ->sizeHint(option, filterModel->index(i, 0, listView->rootIndex()))
                        .width() > availableWidth) {
                    hScrollNeeded = true;
                }
            }
        }

        if (hScrollNeeded) {
            rowsHeight += listView->horizontalScrollBar()->sizeHint().height();
        }

        rowsHeight += listView->contentsMargins().top() + listView->contentsMargins().bottom();
        rowsHeight += 2 * listView->frameWidth();
        auto totalHeight = lineEditTotalHeight + rowsHeight;
        setFixedHeight(totalHeight);

        listView->setVerticalScrollBarPolicy(rowCount > maxVisibleItems ? Qt::ScrollBarAsNeeded
                                                                        : Qt::ScrollBarAlwaysOff);
    } else {
        setFixedHeight(lineEditTotalHeight);
    }
}

static auto collectActionsFromMenu(QList<QAction *> &actions, QMenu *menu) -> void {
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

auto collectWidgetActions(QMainWindow *mainWindow) -> QList<QAction *> {
    auto addActions = [](auto &originalActions, auto const &newActions) {
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

ActionListModel::ActionListModel(QObject *parent) : QAbstractListModel(parent) {}

void ActionListModel::setActions(const QList<QAction *> &actions) {
    beginResetModel();
    m_actions = actions;
    endResetModel();
}

int ActionListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_actions.size();
}

QVariant ActionListModel::data(const QModelIndex &index, int role) const {
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

ActionDelegate::ActionDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void ActionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const {
    painter->save();

    auto rect = option.rect;
    auto selected = option.state & QStyle::State_Selected;

    if (selected) {
        painter->fillRect(rect, option.palette.color(QPalette::Highlight));
        painter->setPen(option.palette.color(QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.text().color());
    }

    auto iconSize =
        option.widget->style()->pixelMetric(QStyle::PM_ListViewIconSize, &option, option.widget);
    auto margin =
        option.widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, option.widget) +
        4;
    auto icon = index.data(ActionListModel::IconRole).value<QIcon>();
    auto text = index.data(ActionListModel::TextRole).toString();
    auto shortcut = index.data(ActionListModel::ShortcutRole).toString();
    auto padding = iconSize + 2 * margin;
    auto iconRect = QRect(rect.left() + margin, rect.top() + (rect.height() - iconSize) / 2,
                          iconSize, iconSize);
    auto textRect =
        QRect(rect.left() + padding, rect.top(), rect.width() - padding - margin, rect.height());

    auto shortcutFont = painter->font();
    auto shortcutColor =
        option.palette.color(selected ? QPalette::Normal : QPalette::Disabled,
                             selected ? QPalette::HighlightedText : QPalette::Text);

    if (selected) {
        shortcutColor.setAlpha(190);
    }
    icon.paint(painter, iconRect, Qt::AlignCenter);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, text);
    painter->setPen(shortcutColor);
    shortcutFont.setPointSizeF(shortcutFont.pointSizeF() - 1);
    painter->setFont(shortcutFont);
    painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, shortcut);
    painter->restore();
}

QSize ActionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    auto size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(size.height() + 4);
    return size;
}

CommandPaletteFilterModel::CommandPaletteFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
    setRecursiveFilteringEnabled(true);
}

QString CommandPaletteFilterModel::currentPattern() const {
    auto pattern = filterRegularExpression().pattern();
    if (auto commandPalette = qobject_cast<const CommandPalette *>(parent())) {
        if (auto lineEdit = commandPalette->findChild<QLineEdit *>()) {
            pattern = lineEdit->text();
        }
    }
    return pattern;
}

bool CommandPaletteFilterModel::filterAcceptsRow(int sourceRow,
                                                 const QModelIndex &sourceParent) const {
    auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    auto text = index.data(ActionListModel::TextRole).toString();
    auto pattern = currentPattern();

    if (pattern.isEmpty()) {
        return true;
    }

    if (m_modes.testFlag(CommandPalette::RemoveAccelerators)) {
        text.remove('&');
    }

    if (m_modes.testFlag(CommandPalette::FileMatch)) {
        return fileMatch(text, pattern);
    }

    if (m_modes.testFlag(CommandPalette::FuzzyMatch)) {
        return fuzzyMatch(text, pattern);
    }

    return text.contains(filterRegularExpression());
}

bool CommandPaletteFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    if (m_modes.testFlag(CommandPalette::FileMatch) ||
        m_modes.testFlag(CommandPalette::FuzzyMatch)) {
        auto pattern = currentPattern();
        if (!pattern.isEmpty()) {
            auto leftText = left.data(ActionListModel::TextRole).toString();
            auto rightText = right.data(ActionListModel::TextRole).toString();
            if (m_modes.testFlag(CommandPalette::RemoveAccelerators)) {
                leftText.remove('&');
                rightText.remove('&');
            }

            auto leftScore = Fuzzy::scoreSimple(pattern, leftText);
            auto rightScore = Fuzzy::scoreSimple(pattern, rightText);
            if (!qFuzzyCompare(leftScore, rightScore)) {
                return leftScore > rightScore;
            }
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

bool CommandPaletteFilterModel::fuzzyMatch(const QString &haystack, const QString &needle) const {
    return Fuzzy::levenshteinDistance(needle, haystack) < 3;
}

bool CommandPaletteFilterModel::fileMatch(const QString &haystack, const QString &needle) const {
    return Fuzzy::scoreSimple(needle, haystack) > 5;
}
