#include "plot2d.h"
#include <QtGui>
#include <QtGlobal>

namespace Plot
{

/**************************************
 * Plot2D
 **************************************/
Plot2D::Plot2D(QObject *parent)
	: QObject(parent)
	, items_()
{
}
	
Plot2D::~Plot2D()
{
}


void Plot2D::addCurve(Plot2DCurve *pc)
{
	if ( pc->plot() )
		pc->plot()->removeCurve(pc);

	pc->setPlot(this);
	items_.push_back(pc);
}

void Plot2D::removeCurve(Plot2DCurve *pc)
{
	Q_ASSERT(pc->plot() != this);
	const int r = items_.removeAll(pc);
	Q_ASSERT_X(r > 1, "removeCurve", "more than one pointer found");
	if ( r )
		pc->setPlot(0);
}

void Plot2D::emitDataChanged()
{
	emit changed();
}


/**************************************
 * Plot2View
 **************************************/
Plot2DView::Plot2DView(QWidget *parent)
	: QWidget(parent)
	, plot_(NULL)
{
}

Plot2DView::Plot2DView(Plot2D *plot, QWidget *parent)
	: QWidget(parent)
	, plot_(plot)
{
}

Plot2DView::~Plot2DView()
{
}

void Plot2DView::setPlot(Plot2D *plot)
{
	if ( plot_ == plot )
		return;

	if ( plot_ )
	{
		disconnect(plot_, SIGNAL(changed()), this, SLOT(update()));
		plot_ = NULL;
	}

	if ( plot )
	{
		connect(plot, SIGNAL(changed()), this, SLOT(update()));
		plot_ = plot;
	}
}

namespace
{
QRectF &operator+=(QRectF &l, const QRectF &r)
{
	l = l.united(r);
	return l;
}
}

void Plot2DView::update()
{
	// recompute extents
	if ( plot() )
	{
		Plot2DCurve *pc = 0;
		extents_ = QRectF();
		foreach(pc, plot()->items())
			extents_ += pc->extents();
	}
	QWidget::update();
}

void Plot2DView::paintEvent(QPaintEvent *ev)
{
	qDebug() << "Plot2DView:paintEvent" << ev->rect();
	
	QPainter painter(this);
	// TODO: draw axes
	qDebug() << "extents are " << extents_;

	// draw graphs
	if ( plot() )
	{
		Plot2DCurve *pc = 0;
		foreach(pc, plot()->items())
		{
			qDebug() << "drawing curve " << pc;
		}
	}
}

/**************************************
 * Plot2Curve
 **************************************/
Plot2DCurve::Plot2DCurve(QObject *parent)
	: QObject(parent)
	, data_()
	, plot_(0)
	, color_(Qt::black)
	, name_("")
{
}

Plot2DCurve::~Plot2DCurve()
{
}

void Plot2DCurve::setPlot(Plot2D *plot)
{
	plot_ = plot;
}

void Plot2DCurve::setData(const DataRow &data)
{
	data_ = data;
}

void Plot2DCurve::setColor(QColor c)
{
	color_ = c;
}

void Plot2DCurve::setName(QString name)
{
	name_ = name;
}

void Plot2DCurve::data_push_back(DataPoint pt)
{
	data_.append(pt);
}

void Plot2DCurve::data_pop_front(void)
{
	data_.pop_front();
}

void Plot2DCurve::appendData(DataPoint pt)
{
	data_push_back(pt);

	if ( plot() )
		plot()->emitDataChanged();
}

void Plot2DCurve::resetData()
{
	data_.clear();
}

/**************************************
 * Plot2CurveRR
 **************************************/
Plot2DCurveRR::Plot2DCurveRR(QObject *parent)
	: Plot2DCurve(parent)
	, history_(1)
{
}

Plot2DCurveRR::~Plot2DCurveRR()
{
}

void Plot2DCurveRR::setHistory(int h)
{
	history_ = h;
	// TODO: check data_.size() and trim/expand accordingly
}

void Plot2DCurveRR::appendData(DataPoint pt)
{
	data_push_back(pt);

	if ( data_.size() > history_ )
		data_pop_front();

	qDebug() << "queue size: " << data_.size();

	if ( plot() )
		plot()->emitDataChanged();
}
	
} // namespace Plot
