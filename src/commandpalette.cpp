#include "commandpalette.h"

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

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
        updateVisibility();
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
}

void CommandPalette::setRootIndex(const QModelIndex &index)
{
    if (!index.isValid()) {
        qWarning() << "Invalid QModelIndex provided for setting root index.";
        return;
    }
    listView->setRootIndex(filterModel->mapFromSource(index));
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
            auto firstIndex = filterModel->index(0, 0);
            listView->setCurrentIndex(firstIndex);
        }
        return;
    }

    auto currentRow = currentIndex.row();
    auto currentColumn = currentIndex.column();
    auto rowBelow = currentRow + 1;
    auto indexBelow
        = currentIndex.sibling(currentRow, currentColumn).model()->index(rowBelow, currentColumn);
    if (indexBelow.isValid()) {
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
    auto currentColumn = currentIndex.column();
    auto rowAbove = currentRow - 1;
    auto indexAbove
        = currentIndex.sibling(currentRow, currentColumn).model()->index(rowAbove, currentColumn);
    if (indexAbove.isValid()) {
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
