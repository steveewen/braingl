/*
 * datasetbingham.cpp
 *
 * Created on: Nov 23, 2012
 * @author Ralph Schurade
 */
#include "datasetbingham.h"
#include "../models.h"

#include "../../gui/gl/binghamrenderer.h"

DatasetBingham::DatasetBingham( QDir filename, std::vector<std::vector<float> > data, nifti_image* header ) :
    DatasetNifti( filename, Fn::DatasetType::NIFTI_BINGHAM, header ),
    m_data( data ),
    m_renderer( 0 )
{
    m_properties["maingl"].createFloat( Fn::Property::D_SCALING, 1.0f, 0.1f, 2.0f, "general" );
    m_properties["maingl"].createInt( Fn::Property::D_OFFSET, 0, -1, 1, "general" );
    m_properties["maingl"].createInt( Fn::Property::D_ORDER, 4 );
    m_properties["maingl"].createInt( Fn::Property::D_LOD, 2, 0, 4, "general" );
    m_properties["maingl"].createBool( Fn::Property::D_RENDER_FIRST, true, "general" );
    m_properties["maingl"].createBool( Fn::Property::D_RENDER_SECOND, false, "general" );
    m_properties["maingl"].createBool( Fn::Property::D_RENDER_THIRD, false, "general" );
    m_properties["maingl"].createBool( Fn::Property::D_RENDER_SAGITTAL, false, "general" );
    m_properties["maingl"].createBool( Fn::Property::D_RENDER_CORONAL, false, "general" );
    m_properties["maingl"].createBool( Fn::Property::D_RENDER_AXIAL, true, "general" );

    examineDataset();

    PropertyGroup props2( m_properties["maingl"] );
    m_properties.insert( "maingl2", props2 );
    m_properties["maingl2"].getProperty( Fn::Property::D_ACTIVE )->setPropertyTab( "general" );
}

DatasetBingham::~DatasetBingham()
{
    m_properties["maingl"].set( Fn::Property::D_ACTIVE, false );
    delete m_renderer;
}

std::vector<std::vector<float> >* DatasetBingham::getData()
{
    return &m_data;
}

void DatasetBingham::examineDataset()
{
    int type = m_properties["maingl"].get( Fn::Property::D_DATATYPE ).toInt();
    int nx = m_properties["maingl"].get( Fn::Property::D_NX ).toInt();
    int ny = m_properties["maingl"].get( Fn::Property::D_NY ).toInt();
    int nz = m_properties["maingl"].get( Fn::Property::D_NZ ).toInt();
    int dim = m_data.at( 0 ).size();
    m_properties["maingl"].createInt( Fn::Property::D_DIM, dim );
    int size = nx * ny * nz * dim;

    if ( type == DT_FLOAT )
    {
        m_properties["maingl"].createInt( Fn::Property::D_SIZE, static_cast<int>( size * sizeof(float) ) );
        m_properties["maingl"].createFloat( Fn::Property::D_MIN, -1.0f );
        m_properties["maingl"].createFloat( Fn::Property::D_MAX, 1.0f );
    }
    m_properties["maingl"].createFloat( Fn::Property::D_LOWER_THRESHOLD, m_properties["maingl"].get( Fn::Property::D_MIN ).toFloat() );
    m_properties["maingl"].createFloat( Fn::Property::D_UPPER_THRESHOLD, m_properties["maingl"].get( Fn::Property::D_MAX ).toFloat() );
}

void DatasetBingham::createTexture()
{
}

void DatasetBingham::draw( QMatrix4x4 pMatrix, QMatrix4x4 mvMatrix, int width, int height, int renderMode, QString target )
{
    if ( !properties( target ).get( Fn::Property::D_ACTIVE ).toBool() )
    {
        return;
    }

    if ( m_resetRenderer )
    {
        if ( m_renderer != 0 )
        {
            delete m_renderer;
            m_renderer = 0;
        }
        m_resetRenderer = false;
    }

    if ( m_renderer == 0 )
    {
        qDebug() << "ds bingham init renderer";
        m_renderer = new BinghamRenderer( &m_data );
        m_renderer->init();
        qDebug() << "ds bingham init renderer done";
    }

    m_renderer->draw( pMatrix, mvMatrix, width, height, renderMode, properties( target ) );
}

QString DatasetBingham::getValueAsString( int x, int y, int z )
{
    std::vector<float> data = m_data[ getId( x, y, z ) ];
    return QString::number( data[0] ) + ", " + QString::number( data[1] ) + ", " + QString::number( data[2] ) + ", " + QString::number( data[3] ) +
     ", " + QString::number( data[4] ) + ", " + QString::number( data[5] ) + ", " + QString::number( data[6] ) + ", " + QString::number( data[7] ) +
     ", " + QString::number( data[8] );
}

void DatasetBingham::flipX()
{
    int nx = m_properties["maingl"].get( Fn::Property::D_NX ).toInt();
    int ny = m_properties["maingl"].get( Fn::Property::D_NY ).toInt();
    int nz = m_properties["maingl"].get( Fn::Property::D_NZ ).toInt();

    for ( int x = 0; x < nx / 2; ++x )
    {
        for ( int y = 0; y < ny; ++y )
        {
            for ( int z = 0; z < nz; ++z )
            {
                std::vector<float> tmp = m_data[getId( x, y, z) ];
                m_data[getId( x, y, z) ] = m_data[getId( nx - x, y, z) ];
                m_data[getId( nx - x, y, z) ] = tmp;
            }
        }
    }
    m_resetRenderer = true;
    Models::g()->submit();
}

void DatasetBingham::flipY()
{
    int nx = m_properties["maingl"].get( Fn::Property::D_NX ).toInt();
    int ny = m_properties["maingl"].get( Fn::Property::D_NY ).toInt();
    int nz = m_properties["maingl"].get( Fn::Property::D_NZ ).toInt();

    for ( int x = 0; x < nx; ++x )
    {
        for ( int y = 0; y < ny / 2; ++y )
        {
            for ( int z = 0; z < nz; ++z )
            {
                std::vector<float> tmp = m_data[getId( x, y, z) ];
                m_data[getId( x, y, z) ] = m_data[getId( x, ny - y, z) ];
                m_data[getId( x, ny - y, z) ] = tmp;
            }
        }
    }
    m_resetRenderer = true;
    Models::g()->submit();
}

void DatasetBingham::flipZ()
{
    int nx = m_properties["maingl"].get( Fn::Property::D_NX ).toInt();
    int ny = m_properties["maingl"].get( Fn::Property::D_NY ).toInt();
    int nz = m_properties["maingl"].get( Fn::Property::D_NZ ).toInt();

    for ( int x = 0; x < nx; ++x )
    {
        for ( int y = 0; y < ny; ++y )
        {
            for ( int z = 0; z < nz / 2; ++z )
            {
                std::vector<float> tmp = m_data[getId( x, y, z) ];
                m_data[getId( x, y, z) ] = m_data[getId( x, y, nz - z) ];
                m_data[getId( x, y, nz - z) ] = tmp;
            }
        }
    }
    m_resetRenderer = true;
    Models::g()->submit();
}
