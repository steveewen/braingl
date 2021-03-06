/*
 * BinghamRenderer.cpp
 *
 * Created on: 03.07.2012
 * @author Ralph Schurade
 */
#include "binghamrenderer.h"
#include "binghamrendererthread.h"

#include "glfunctions.h"

#include "../../data/datasets/datasetsh.h"
#include "../../data/enums.h"
#include "../../data/models.h"
#include "../../data/vptr.h"
#include "../../algos/fmath.h"
#include "../../algos/qball.h"

#include "../../data/mesh/tesselation.h"
#include "../../data/properties/propertygroup.h"

#include "../../thirdparty/newmat10/newmat.h"

#include <QtOpenGL/QGLShaderProgram>
#include <QDebug>
#include <QVector3D>
#include <QMatrix4x4>

#include <limits>
#include <stdint.h>

BinghamRenderer::BinghamRenderer( std::vector<std::vector<float> >* data ) :
    ObjectRenderer(),
    m_tris1( 0 ),
    vboIds( new GLuint[ 2 ] ),
    m_data( data ),
    m_scaling( 1.0 ),
    m_orient( 0 ),
    m_offset( 0 ),
    m_lodAdjust( 0 ),
    m_minMaxScaling( true),
    m_order( 4 ),
    m_render1( true ),
    m_render2( false ),
    m_render3( false )
{
}

BinghamRenderer::~BinghamRenderer()
{
    glDeleteBuffers(1, &( vboIds[ 0 ] ) );
    glDeleteBuffers(1, &( vboIds[ 1 ] ) );
}

void BinghamRenderer::init()
{
    initializeOpenGLFunctions();
    glGenBuffers( 2, vboIds );
}

void BinghamRenderer::draw( QMatrix4x4 p_matrix, QMatrix4x4 mv_matrix, int width, int height, int renderMode, PropertyGroup& props )
{
    if ( renderMode != 1 ) // we are drawing opaque objects
    {
        // obviously not opaque
        return;
    }

    setRenderParams( props );

    if ( m_orient == 0 )
    {
        return;
    }

    m_pMatrix = p_matrix;
    m_mvMatrix = mv_matrix;


    initGeometry( props );

    QGLShaderProgram* program = GLFunctions::getShader( "qball" );
    program->bind();

    program->setUniformValue( "u_alpha", 1.0f );
    program->setUniformValue( "u_renderMode", renderMode );
    program->setUniformValue( "u_canvasSize", width, height );
    program->setUniformValue( "D0", 9 );
    program->setUniformValue( "D1", 10 );
    program->setUniformValue( "D2", 11 );
    program->setUniformValue( "P0", 12 );

    // Set modelview-projection matrix
    program->setUniformValue( "mvp_matrix", p_matrix * mv_matrix );
    program->setUniformValue( "mv_matrixInvert", mv_matrix.inverted() );
    program->setUniformValue( "u_hideNegativeLobes", m_minMaxScaling );
    program->setUniformValue( "u_scaling", m_scaling );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vboIds[ 0 ] );
    glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 1 ] );
    setShaderVars( props );
    glDrawElements( GL_TRIANGLES, m_tris1, GL_UNSIGNED_INT, 0 );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

void BinghamRenderer::setShaderVars( PropertyGroup& props )
{
    QGLShaderProgram* program = GLFunctions::getShader( "qball" );
    program->bind();

    intptr_t offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation( "a_position" );
    program->enableAttributeArray( vertexLocation );
    glVertexAttribPointer( vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (const void *) offset );

    offset += sizeof(float) * 3;
    int offsetLocation = program->attributeLocation( "a_offset" );
    program->enableAttributeArray( offsetLocation );
    glVertexAttribPointer( offsetLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (const void *) offset );

    offset += sizeof(float) * 3;
    int radiusLocation = program->attributeLocation( "a_radius" );
    program->enableAttributeArray( radiusLocation );
    glVertexAttribPointer( radiusLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (const void *) offset );
}

void BinghamRenderer::initGeometry( PropertyGroup& props )
{
    float x = Models::getGlobal( Fn::Property::G_SAGITTAL ).toFloat();
    float y = Models::getGlobal( Fn::Property::G_CORONAL ).toFloat();
    float z = Models::getGlobal( Fn::Property::G_AXIAL ).toFloat();

    float zoom = Models::getGlobal( Fn::Property::G_ZOOM ).toFloat();
    float moveX = Models::getGlobal( Fn::Property::G_MOVEX ).toFloat();
    float moveY = Models::getGlobal( Fn::Property::G_MOVEY ).toFloat();


    int renderPeaks = (int)m_render1 * 1 + (int)m_render2 * 2 + (int)m_render3 * 4;

    int lod = m_lodAdjust;

    QString s = createSettingsString( {x, y, z, m_orient, m_minMaxScaling, renderPeaks, lod, zoom, moveX, moveY, m_offset } );

    if ( s == m_previousSettings || m_orient == 0 )
    {
        return;
    }
    m_previousSettings = s;

    int numVerts = tess::n_vertices( lod );
    int numTris = tess::n_faces( lod );

    int numThreads = GLFunctions::idealThreadCount;

    std::vector<BinghamRendererThread*> threads;
    // create threads
    for ( int i = 0; i < numThreads; ++i )
    {
        threads.push_back( new BinghamRendererThread( i, m_data, m_pMatrix, m_mvMatrix, props ) );
    }

    // run threads
    for ( int i = 0; i < numThreads; ++i )
    {
        threads[i]->start();
    }

    // wait for all threads to finish
    for ( int i = 0; i < numThreads; ++i )
    {
        threads[i]->wait();
    }

    std::vector<float> verts;
    // combine verts from all threads
    for ( int i = 0; i < numThreads; ++i )
    {
        verts.insert( verts.end(), threads[i]->getVerts()->begin(), threads[i]->getVerts()->end() );
    }

    for ( int i = 0; i < numThreads; ++i )
    {
        delete threads[i];
    }

    int numBalls = verts.size() / ( numVerts * 7 );
    m_tris1 = numTris * numBalls * 3;

    glDeleteBuffers(1, &( vboIds[ 0 ] ) );
    glDeleteBuffers(1, &( vboIds[ 1 ] ) );
    glGenBuffers( 2, vboIds );

    glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 1 ] );
    glBufferData( GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(), GL_STATIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    verts.clear();

    const int* faces = tess::faces( lod );
    std::vector<int>indexes;
    indexes.reserve( m_tris1 );

    for ( int currentBall = 0; currentBall < numBalls; ++currentBall )
    {
        for ( int i = 0; i < numTris; ++i )
        {
            indexes.push_back( faces[i*3] + numVerts * currentBall );
            indexes.push_back( faces[i*3+1] + numVerts * currentBall );
            indexes.push_back( faces[i*3+2] + numVerts * currentBall );
        }
    }

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vboIds[ 0 ] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(GLuint), &indexes[0], GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    indexes.clear();
}

void BinghamRenderer::setRenderParams( PropertyGroup& props )
{
    int slice = (int)props.get( Fn::Property::D_RENDER_AXIAL ).toBool() +
                    (int)props.get( Fn::Property::D_RENDER_CORONAL ).toBool() * 2 +
                    (int)props.get( Fn::Property::D_RENDER_SAGITTAL ).toBool() * 4;

    m_scaling = props.get( Fn::Property::D_SCALING ).toFloat();
    m_orient = slice;
    m_offset = props.get( Fn::Property::D_OFFSET ).toFloat();
    m_lodAdjust = props.get( Fn::Property::D_LOD ).toInt();
    m_minMaxScaling = props.get( Fn::Property::D_MINMAX_SCALING ).toBool();
    m_order = props.get( Fn::Property::D_ORDER ).toInt();
    m_render1 = props.get( Fn::Property::D_RENDER_FIRST ).toBool();
    m_render2 = props.get( Fn::Property::D_RENDER_SECOND ).toBool();
    m_render3 = props.get( Fn::Property::D_RENDER_THIRD ).toBool();
}
