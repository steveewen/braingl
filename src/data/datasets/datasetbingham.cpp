/*
 * datasetbingham.cpp
 *
 *  Created on: Nov 23, 2012
 *      Author: Ralph Schurade
 */
#include "datasetbingham.h"

#include "../../gui/gl/binghamrenderer.h"

DatasetBingham::DatasetBingham( QString filename, QVector<QVector<float> > data, nifti_image* header ) :
    DatasetNifti( filename, Fn::DatasetType::NIFTI_BINGHAM, header ),
    m_data( data ),
    m_renderer( 0 )
{
    m_properties.set( Fn::Property::RENDER_SLICE, 1, 1, 3, true );
    m_properties.set( Fn::Property::SCALING, 1.0f, 0.0f, 2.0f );
    m_properties.set( Fn::Property::OFFSET, 0.0f, -0.5f, 0.5f, true );
    m_properties.set( Fn::Property::ORDER, 4 );
    m_properties.set( Fn::Property::LOD, 2, 0, 4, true );
    m_properties.set( Fn::Property::RENDER_FIRST, true, true );
    m_properties.set( Fn::Property::RENDER_SECOND, false, true );
    m_properties.set( Fn::Property::RENDER_THIRD, false, true );


    examineDataset();
}

DatasetBingham::~DatasetBingham()
{
    m_properties.set( Fn::Property::ACTIVE, false );
    delete m_renderer;
}

QVector<QVector<float> >* DatasetBingham::getData()
{
    return &m_data;
}

void DatasetBingham::examineDataset()
{
    int type = m_properties.get( Fn::Property::DATATYPE ).toInt();
    int nx = m_properties.get( Fn::Property::NX ).toInt();
    int ny = m_properties.get( Fn::Property::NY ).toInt();
    int nz = m_properties.get( Fn::Property::NZ ).toInt();
    int dim = m_data.at( 0 ).size();
    m_properties.set( Fn::Property::DIM, dim );
    int size = nx * ny * nz * dim;

    if ( type == DT_FLOAT )
    {
        m_properties.set( Fn::Property::SIZE, static_cast<int>( size * sizeof(float) ) );
        m_properties.set( Fn::Property::MIN, -1.0f );
        m_properties.set( Fn::Property::MAX, 1.0f );
    }
    m_properties.set( Fn::Property::LOWER_THRESHOLD, m_properties.get( Fn::Property::MIN ).toFloat() );
    m_properties.set( Fn::Property::UPPER_THRESHOLD, m_properties.get( Fn::Property::MAX ).toFloat() );

    m_properties.set( Fn::Property::RENDER_UPPER_X, nx - 1 );
    m_properties.set( Fn::Property::RENDER_UPPER_Y, ny - 1 );
    m_properties.set( Fn::Property::RENDER_UPPER_Z, nz - 1 );
}

void DatasetBingham::createTexture()
{
}

void DatasetBingham::flipX()
{
}

void DatasetBingham::draw( QMatrix4x4 mvpMatrix, QMatrix4x4 mvMatrixInverse, QAbstractItemModel* globalModel, QAbstractItemModel* roiModel, QAbstractItemModel* dataModel )
{
    if ( m_renderer == 0 )
    {
        qDebug() << "ds bingham init renderer";
        m_renderer = new BinghamRenderer( &m_data, m_properties.get( Fn::Property::NX ).toInt(),
                                                  m_properties.get( Fn::Property::NY ).toInt(),
                                                  m_properties.get( Fn::Property::NZ ).toInt(),
                                                  m_properties.get( Fn::Property::DX ).toFloat(),
                                                  m_properties.get( Fn::Property::DY ).toFloat(),
                                                  m_properties.get( Fn::Property::DZ ).toFloat() );
        m_renderer->setModel( globalModel );
        m_renderer->init();
        qDebug() << "ds bingham init renderer done";
    }

    m_renderer->setRenderParams( m_properties.get( Fn::Property::SCALING ).toFloat(),
                                 m_properties.get( Fn::Property::RENDER_SLICE ).toInt(),
                                 m_properties.get( Fn::Property::OFFSET ).toFloat(),
                                 m_properties.get( Fn::Property::LOD ).toInt(),
                                 m_properties.get( Fn::Property::MINMAX_SCALING ).toBool(),
                                 m_properties.get( Fn::Property::ORDER ).toInt(),
                                 m_properties.get( Fn::Property::RENDER_FIRST ).toBool(),
                                 m_properties.get( Fn::Property::RENDER_SECOND ).toBool(),
                                 m_properties.get( Fn::Property::RENDER_THIRD ).toBool() );

    m_renderer->draw( mvpMatrix, mvMatrixInverse );
}

QString DatasetBingham::getValueAsString( int x, int y, int z )
{
    int nx = m_properties.get( Fn::Property::NX ).toInt();
    int ny = m_properties.get( Fn::Property::NY ).toInt();
    QVector<float> data = m_data.at( x + y * nx + z * nx * ny );
    return QString::number( data[0] ) + ", " + QString::number( data[1] ) + ", " + QString::number( data[2] ) + ", " + QString::number( data[3] ) +
     ", " + QString::number( data[4] ) + ", " + QString::number( data[5] ) + ", " + QString::number( data[6] ) + ", " + QString::number( data[7] ) +
     ", " + QString::number( data[8] );
}
