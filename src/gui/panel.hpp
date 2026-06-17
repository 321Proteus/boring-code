#include <QWidget>
#include <QAbstractItemView>
#include <QLabel>
#include <QTableView>
#include <QListWidget>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QPainter>
#include <QShortcut>
#include <QScrollBar>
#include <algorithm>
#include <QElapsedTimer>
#include <qevent.h>

class SearchBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit SearchBarWidget(QWidget* parent = nullptr) : QWidget(parent) {

        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("SearchBarWidget { background-color: lightgray; border-radius: 4px 4px 0px 0px; }");

        searchEdit = new QLineEdit(this);
        matchLabel = new QLabel(this);
        matchLabel->setText("0");

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->addWidget(searchEdit);
        layout->addWidget(matchLabel);

        this->setFixedHeight(30);
        this->setVisible(false);

    }

    QLineEdit* searchEdit;
    QLabel* matchLabel;

};

class MatchScrollBar : public QScrollBar {
    Q_OBJECT
public:
    explicit MatchScrollBar(QWidget* parent = nullptr) : QScrollBar(parent) {}

    void setMatches(const QList<QModelIndex>& rows, int total) {
        this->rows = rows;
        this->total = total;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QScrollBar::paintEvent(event);
        QStyleOptionSlider opt;
        initStyleOption(&opt);

        QRect bb = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, this);

        QPainter painter(this);
        painter.setClipRect(event->rect());
        painter.setBrush(QColor(255, 12, 0, 180));
        painter.setPen(Qt::NoPen);

        QList<QRect> marks;

        for (const QModelIndex& row : rows) {
            double pos = (double)row.row() / total;

            int x = bb.top();
            int y = pos * bb.height();
            int w = width();
            int h = std::max(bb.height() / total, 1);

            marks.append(QRect(x, y, w, h));
        }

        painter.drawRects(marks);
    }

private:
    QList<QModelIndex> rows;
    int total;

};

class PanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit PanelWidget(QLayout* parent = nullptr, QAbstractItemView* view = nullptr)
        : QWidget(parent->parentWidget()), view(view) {

        setAttribute(Qt::WA_StyledBackground, true);
        parent->replaceWidget(view, this);
        view->setParent(this);

        QBoxLayout* parentBox = qobject_cast<QBoxLayout*>(parent);
        parentBox->setStretch(parentBox->indexOf(this), 1);

        m_searchBar = new SearchBarWidget(this);

        view->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        QShortcut* searchShortcut = new QShortcut(QKeySequence::Find, this);
        searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(searchShortcut, &QShortcut::activated, this, &PanelWidget::toggleSearchBar);
        
        connect(m_searchBar->searchEdit, &QLineEdit::textChanged, this, &PanelWidget::search);

    };

    void paintEvent(QPaintEvent* event) override {
        QStyleOption opt;
        opt.initFrom(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_FrameWindow, &opt, &p, this);
    }

    void resizeEvent(QResizeEvent* event) override {
        int margin = 6;
        view->resize(width() - margin*2, height() - margin*2);
        view->move(margin, margin);
        m_searchBar->resize(0.8 * width(), 30);
        m_searchBar->move(0.1 * width(), height() - 30);
    }

private slots:
    void toggleSearchBar() {
        if (m_searchBar->isVisible()) {
            m_searchBar->hide();
            m_searchBar->searchEdit->clearFocus();
        } else {
            m_searchBar->show();
            m_searchBar->searchEdit->setFocus();
        }
    }

    void search() {

        view->selectionModel()->blockSignals(true);
        view->setUpdatesEnabled(false);

        view->selectionModel()->clearSelection();

        const QString& query = m_searchBar->searchEdit->text();
        if (query.trimmed() == "") {
            m_searchBar->matchLabel->setText("0");    
            view->setUpdatesEnabled(true);
            view->selectionModel()->blockSignals(false);
            return;
        }

        QAbstractItemModel* model = view->model();
        QModelIndexList found;

        if (lastQuery != "" && query.startsWith(lastQuery)) {
            QModelIndexList filtered;
            std::copy_if(lastMatches.begin(), lastMatches.end(), std::back_inserter(filtered), [&](const QModelIndex &index) {
                return index.data(Qt::DisplayRole).toString().contains(query, Qt::CaseInsensitive);
            });
            found = std::move(filtered);
        } else {

            for (int col=0;col<model->columnCount();col++) {
                QModelIndex start = model->index(0, col);
                QModelIndexList col_matches = model->match(start, Qt::DisplayRole, query, -1, Qt::MatchContains | Qt::MatchWrap);
                found.append(col_matches);
            }

        }

        int count = found.size();
        m_searchBar->matchLabel->setText(found.size() ? QString::number(found.size()) : "Not found");

        MatchScrollBar* bar = new MatchScrollBar(view);
        bar->setMatches(found, model->rowCount());
        view->setVerticalScrollBar(bar);

        lastQuery = query;
        lastMatches = found;

        view->setUpdatesEnabled(true);
        view->selectionModel()->blockSignals(false);

    }

private:
    QString lastQuery;
    QModelIndexList lastMatches;

    // void searchNext();
    // void searchPrev();

private:
    QAbstractItemView* view;
    SearchBarWidget* m_searchBar;
};

class TracePanel : public PanelWidget {
    Q_OBJECT
public:
    explicit TracePanel(QLayout* parent, QTableView* view)
        : PanelWidget(parent, view) {}
};

class CodePanel : public PanelWidget {
    Q_OBJECT
public:
    explicit CodePanel(QLayout* parent, QListWidget* view)
        : PanelWidget(parent, view) {}
};