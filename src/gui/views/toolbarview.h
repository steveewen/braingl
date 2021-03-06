/*
 * toolbarview.h
 *
 * Created on: 14.06.2012
 * @author Ralph Schurade
 */

#ifndef TOOLBARVIEW_H_
#define TOOLBARVIEW_H_

#include "../../data/enums.h"

#include <QAbstractItemView>

class ToolBarView  : public QAbstractItemView
{
    Q_OBJECT

public:
    ToolBarView( QWidget* parent = 0 );
    virtual ~ToolBarView();

    QRect visualRect( const QModelIndex &index ) const;
    void scrollTo( const QModelIndex &index, ScrollHint hint = EnsureVisible );
    QModelIndex indexAt( const QPoint &point ) const;
    QModelIndex moveCursor( CursorAction cursorAction, Qt::KeyboardModifiers modifiers );
    int horizontalOffset() const;
    int verticalOffset() const;
    bool isIndexHidden( const QModelIndex &index ) const;
    void setSelection( const QRect &rect, QItemSelectionModel::SelectionFlags flags );
    QRegion visualRegionForSelection( const QItemSelection &selection ) const;

    int getSelected();

public slots:
    void selectionChanged( const QItemSelection &selected, const QItemSelection &deselected );

private:
    int m_selected;

signals:
    void sigSelectionChanged( int type );

};

#endif /* TOOLBARVIEW_H_ */
