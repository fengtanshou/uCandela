#ifndef PLOT2D_H_INC
#define PLOT2D_H_INC

#include <QObject>
#include <QList>
#include <QAbstractScrollArea>
#include <QColor>

namespace Plot
{

class Plot2D;
class Plot2DView;
class Plot2DCurve;
typedef QPointF DataPoint;
typedef QList<DataPoint> DataRow;

/** Plot2D
 * 
 */
class Plot2D  : public QObject
{
	Q_OBJECT;
public:
	Plot2D(QObject *parent = 0);
	~Plot2D();

	void update();
	void addCurve(Plot2DCurve *pc);
	void removeCurve(Plot2DCurve *pc);
	inline QList<Plot2DCurve*> items() const { return items_; }
	void emitDataChanged();

signals:
	void changed(void);
	
private:
	QList<Plot2DCurve*> items_;
};

/** Plot2DView
 *
 */
class Plot2DView : public QWidget
{
	Q_OBJECT;
public:
	Plot2DView(QWidget *parent = 0);
	Plot2DView(Plot2D *plot, QWidget *parent = 0);
	~Plot2DView();

	void setPlot(Plot2D *plot);
	inline Plot2D *plot() const { return plot_; }

public slots:
	void update();

protected:
	void paintEvent(QPaintEvent *);

private:
	Plot2D *plot_;
	QRectF extents_;
};

/** Plot2DCurve
 *
 */
class Plot2DCurve : public QObject
{
	Q_OBJECT;
public:
	Plot2DCurve(QObject *parent = 0);
	~Plot2DCurve();

	inline const DataRow &data() const { return data_; }
	inline DataRow &data() { return data_; }
	inline Plot2D *plot() { return plot_; }
	inline QString name(void) const { return name_; }
	inline QColor color(void) const { return color_; }
	inline QRectF extents() const { return extents_; }

	void setData(const DataRow &data);
	void setPlot(Plot2D *plot);
	void setName(QString name_);
	void setColor(QColor c);

	virtual void appendData(DataPoint pt);
	virtual void resetData();

	// void setStyle();
	// xxx style();
protected:
	DataRow data_;
	void data_push_back(DataPoint pt);
	void data_pop_front(void);

private:
	Plot2D *plot_;
	QColor color_;
	QString name_;
	QRectF extents_;
};

/** Plot2DCurveRRD - curve with RR append policy
 * 
 */
class Plot2DCurveRR : public Plot2DCurve
{
	Q_OBJECT;
public:
	Plot2DCurveRR(QObject *parent = 0);
	~Plot2DCurveRR();

	void setHistory(int h);
	inline int history() const { return history_; }

	virtual void appendData(DataPoint pt);
private:
	int history_;
};

} // namespace Plot

#endif /* PLOT2D_H_INC */
