/*
 * datasetsurfaceset.cpp
 *
 *  Created on: Apr 22, 2013
 *      Author: boettgerj
 */

#include "datasetsurfaceset.h"

#include "../models.h"

#include "../mesh/trianglemesh2.h"
#include "../../gui/gl/meshrenderer.h"

DatasetSurfaceset::DatasetSurfaceset( QDir fn, Fn::DatasetType type ) :
        DatasetMesh( fn, type )
{

}

DatasetSurfaceset::~DatasetSurfaceset()
{
    // TODO Auto-generated destructor stub
}

void DatasetSurfaceset::addMesh( TriangleMesh2* tm, QString displayString )
{
    m_mesh.push_back( tm );
    m_displayList << displayString;
}

void DatasetSurfaceset::setProperties()
{
    m_properties["maingl"]->create( Fn::Property::D_SURFACE, m_displayList, 0, "general" );
    m_properties["maingl2"]->create( Fn::Property::D_SURFACE, m_displayList, 0, "general" );
}

TriangleMesh2* DatasetSurfaceset::getMesh( QString target )
{
    int n = properties( target )->get( Fn::Property::D_SURFACE ).toInt();
    return m_mesh[n];
}

void DatasetSurfaceset::draw( QMatrix4x4 pMatrix, QMatrix4x4 mvMatrix, int width, int height, int renderMode, QString target )
{
    if ( !properties( target )->get( Fn::Property::D_ACTIVE ).toBool() )
    {
        return;
    }
    if ( m_renderer == 0 )
    {
        m_renderer = new MeshRenderer( getMesh( target ) );
        m_renderer->setModel( Models::g() );
        m_renderer->init();
    }
    m_renderer->setMesh( getMesh( target ) );
    m_renderer->draw( pMatrix, mvMatrix, width, height, renderMode, properties( target ) );
}

bool DatasetSurfaceset::mousePick( int pickId, QVector3D pos, Qt::KeyboardModifiers modifiers, QString target )
{
    int paintMode = m_properties["maingl"]->get( Fn::Property::D_PAINTMODE ).toInt();
    if ( pickId == 0 || paintMode == 0 || !( modifiers & Qt::ControlModifier ) )
    {
        return false;
    }

    QColor color;
    if ( ( modifiers & Qt::ControlModifier ) && !( modifiers & Qt::ShiftModifier ) )
    {
        color = m_properties["maingl"]->get( Fn::Property::D_PAINTCOLOR ).value<QColor>();
    }
    else if ( ( modifiers & Qt::ControlModifier ) && ( modifiers & Qt::ShiftModifier ) )
    {
        color = m_properties["maingl"]->get( Fn::Property::D_COLOR ).value<QColor>();
    }
    else
    {
        return false;
    }

    QVector<int> picked = getMesh( target )->pick( pos, m_properties["maingl"]->get( Fn::Property::D_PAINTSIZE ).toFloat() );

    if ( picked.size() > 0 )
    {
        m_renderer->beginUpdateColor();
        for ( int i = 0; i < picked.size(); ++i )
        {
            m_renderer->updateColor( picked[i], color.redF(), color.greenF(), color.blueF(), 1.0 );
            for ( int m = 0; m < m_mesh.size(); m++ )
            {
                m_mesh[m]->setVertexColor( picked[i], color );
            }
        }
        m_renderer->endUpdateColor();
        return true;
    }
    return false;
}