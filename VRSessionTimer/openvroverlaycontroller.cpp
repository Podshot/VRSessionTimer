//====== Copyright Valve Corporation, All rights reserved. =======


#include "openvroverlaycontroller.h"


#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QtWidgets/QWidget>
#include <QMouseEvent>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsEllipseItem>
#include <QCursor>
#include <QTextStream>
#include <iostream>

using namespace vr;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
COpenVROverlayController *s_pSharedVRController = NULL;

COpenVROverlayController *COpenVROverlayController::SharedInstance()
{
    if ( !s_pSharedVRController )
    {
        s_pSharedVRController = new COpenVROverlayController();
    }
    return s_pSharedVRController;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
COpenVROverlayController::COpenVROverlayController()
    : BaseClass()
    , m_eLastHmdError( vr::VRInitError_None )
    , m_eCompositorError( vr::VRInitError_None )
    , m_eOverlayError( vr::VRInitError_None )
    , m_strVRDriver( "No Driver" )
    , m_strVRDisplay( "No Display" )
    , m_pOpenGLContext( NULL )
    , m_pScene( NULL )
    , m_pOffscreenSurface ( NULL )
    , m_pFbo( NULL )
    , m_pWidget( NULL )
    , m_pPumpEventsTimer( NULL )
    , m_lastMouseButtons( 0 )
    , m_ulOverlayHandle( vr::k_ulOverlayHandleInvalid )
    , m_bManualMouseHandling( false )
{
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
COpenVROverlayController::~COpenVROverlayController()
{
}


//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a QString
//-----------------------------------------------------------------------------
QString GetTrackedDeviceString( vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop )
{
    char buf[128];
    vr::TrackedPropertyError err;
    pHmd->GetStringTrackedDeviceProperty( unDevice, prop, buf, sizeof( buf ), &err );
    if( err != vr::TrackedProp_Success )
    {
        return QString( "Error Getting String: " ) + pHmd->GetPropErrorNameFromEnum( err );
    }
    else
    {
        return buf;
    }
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool COpenVROverlayController::Init()
{
    bool bSuccess = true;

    m_strName = "vrsessiontimer";

    QStringList arguments = qApp->arguments();

    int nNameArg = arguments.indexOf( "-name" );
    if( nNameArg != -1 && nNameArg + 2 <= arguments.size() )
    {
        m_strName = arguments.at( nNameArg + 1 );
    }

    QSurfaceFormat format;
    format.setMajorVersion( 2 );
    format.setMinorVersion( 1 );
    //format.setProfile( QSurfaceFormat::CompatibilityProfile );
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setSamples(16);


    m_pOpenGLContext = new QOpenGLContext();
    m_pOpenGLContext->setFormat( format );
    bSuccess = m_pOpenGLContext->create();
    if( !bSuccess )
        return false;

    // create an offscreen surface to attach the context and FBO to
    m_pOffscreenSurface = new QOffscreenSurface();
    m_pOffscreenSurface->setFormat(m_pOpenGLContext->format());
    m_pOffscreenSurface->create();
    m_pOpenGLContext->makeCurrent(m_pOffscreenSurface);

    m_pScene = new QGraphicsScene();
    connect(m_pScene, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(OnSceneChanged(const QList<QRectF>&)));

    // Loading the OpenVR Runtime
    bSuccess = ConnectToVRRuntime();

    bSuccess = bSuccess && vr::VRCompositor() != NULL;

    if( vr::VROverlay() )
    {
        std::string sKey = m_strName.toStdString();
        m_pch = sKey.c_str();
        //vr::VROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay( sKey.c_str(), m_strName.toStdString().c_str(), &m_ulOverlayHandle, &m_ulOverlayThumbnailHandle );

        std::string pchKeyStr = "(Init) Created overlay with pchKey: ";
        std::string initOverlayHandleStr = "(Init) Initial overlay handle: ";
        std::string changedOverlayHandleStr = "(Init) Initialized overlay handle: ";
        initOverlayHandleStr.append((m_ulOverlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
        pchKeyStr.append(m_pch);
        Log(QString::fromStdString(pchKeyStr));
        Log(QString::fromStdString(initOverlayHandleStr));

        vr::VROverlayError overlayError = vr::VROverlay()->CreateOverlay( m_pch, m_strName.toStdString().c_str(), &m_ulOverlayHandle);

        changedOverlayHandleStr.append((m_ulOverlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
        Log(QString::fromStdString(changedOverlayHandleStr));

        bSuccess = bSuccess && overlayError == vr::VROverlayError_None;
    } else {
        Log(QString::fromStdString("No VROverlay()"));
    }

    if( bSuccess )
    {
        vr::VROverlay()->SetOverlayWidthInMeters( m_ulOverlayHandle, 1.5f );
        vr::VROverlay()->SetOverlayInputMethod( m_ulOverlayHandle, vr::VROverlayInputMethod_Mouse );

        m_pPumpEventsTimer = new QTimer( this );
        connect(m_pPumpEventsTimer, SIGNAL( timeout() ), this, SLOT( OnTimeoutPumpEvents() ) );
        m_pPumpEventsTimer->setInterval( 20 );
        m_pPumpEventsTimer->start();

    }
    return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void COpenVROverlayController::Shutdown()
{
    DisconnectFromVRRuntime();

    delete m_pScene;
    delete m_pFbo;
    delete m_pOffscreenSurface;

    if( m_pOpenGLContext )
    {
        //		m_pOpenGLContext->destroy();
        delete m_pOpenGLContext;
        m_pOpenGLContext = NULL;
    }
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void COpenVROverlayController::OnSceneChanged( const QList<QRectF>& )
{
    // skip rendering if the overlay isn't visible
    if( !vr::VROverlay() ||
            //        ( !vr::VROverlay()->IsOverlayVisible( m_ulOverlayHandle ) && !vr::VROverlay()->IsOverlayVisible( m_ulOverlayThumbnailHandle ) ) )
            !vr::VROverlay()->IsOverlayVisible( m_ulOverlayHandle ) ) {

        std::string overlayStatus = "(OnSceneChanged) VROverlay running: ";
        overlayStatus.append(vr::VROverlay() ? "true" : "false");
        std::string overlayShow = "(OnSceneChanged) Overlay Shown: ";
        if (vr::VROverlay()) {
            overlayShow.append(vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle) ? "true" : "false");
            Log(QString::fromStdString(overlayShow));
        }
        Log(QString::fromStdString(overlayStatus));

        return;
    }

    m_pOpenGLContext->makeCurrent( m_pOffscreenSurface );
    m_pFbo->bind();

    QOpenGLPaintDevice device( m_pFbo->size() );
    QPainter painter( &device );

    m_pScene->render( &painter );

    m_pFbo->release();

    GLuint unTexture = m_pFbo->texture();
    if( unTexture != 0 )
    {
        std::string overlayHandleStr = "(OnSceneChanged) Overlay handle: ";
        overlayHandleStr.append((m_ulOverlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
        Log(QString::fromStdString(overlayHandleStr));
#if defined _WIN64 || defined _LP64
        Log(QString::fromStdString("(OnSceneChanged) Using 'uint64_t' conversion for texture"));
        vr::Texture_t texture = { (void*)((uint64_t)unTexture), vr::TextureType_OpenGL, vr::ColorSpace_Auto };
#else
        Log("(OnSceneChanged) Using 'void*' conversion for texture");
        vr::Texture_t texture = { (void*)unTexture, vr::TextureType_OpenGL, vr::ColorSpace_Auto };
#endif
        vr::VROverlay()->SetOverlayTexture( m_ulOverlayHandle, &texture );
        //vr::VROverlay()->SetOverlayAlpha(m_ulOverlayHandle, 1.0);
    } else {
        Log(QString::fromStdString("(OnSceneChanged) unTexture was 0"));
    }
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void COpenVROverlayController::OnTimeoutPumpEvents()
{
    if( !vr::VRSystem() )
        return;


    if( m_bManualMouseHandling )
    {
        // tell OpenVR to make some events for us
        for( vr::TrackedDeviceIndex_t unDeviceId = 1; unDeviceId < vr::k_unControllerStateAxisCount; unDeviceId++ )
        {
            if( vr::VROverlay()->HandleControllerOverlayInteractionAsMouse( m_ulOverlayHandle, unDeviceId ) )
            {
                break;
            }
        }
    }

    vr::VREvent_t vrEvent;
    while( vr::VROverlay()->PollNextOverlayEvent( m_ulOverlayHandle, &vrEvent, sizeof( vrEvent )  ) )
    {
        switch( vrEvent.eventType )
        {
        case vr::VREvent_MouseMove:
        {
            QPointF ptNewMouse( vrEvent.data.mouse.x, vrEvent.data.mouse.y );
            QPoint ptGlobal = ptNewMouse.toPoint();
            QGraphicsSceneMouseEvent mouseEvent( QEvent::GraphicsSceneMouseMove );
            mouseEvent.setWidget( NULL );
            mouseEvent.setPos( ptNewMouse );
            mouseEvent.setScenePos( ptGlobal );
            mouseEvent.setScreenPos( ptGlobal );
            mouseEvent.setLastPos( m_ptLastMouse );
            mouseEvent.setLastScenePos( m_pWidget->mapToGlobal( m_ptLastMouse.toPoint() ) );
            mouseEvent.setLastScreenPos( m_pWidget->mapToGlobal( m_ptLastMouse.toPoint() ) );
            mouseEvent.setButtons( m_lastMouseButtons );
            mouseEvent.setButton( Qt::NoButton );
            mouseEvent.setModifiers( 0 );
            mouseEvent.setAccepted( false );

            m_ptLastMouse = ptNewMouse;
            QApplication::sendEvent( m_pScene, &mouseEvent );

            OnSceneChanged( QList<QRectF>() );
        }
            break;

        case vr::VREvent_MouseButtonDown:
        {
            Qt::MouseButton button = vrEvent.data.mouse.button == vr::VRMouseButton_Right ? Qt::RightButton : Qt::LeftButton;

            m_lastMouseButtons |= button;

            QPoint ptGlobal = m_ptLastMouse.toPoint();
            QGraphicsSceneMouseEvent mouseEvent( QEvent::GraphicsSceneMousePress );
            mouseEvent.setWidget( NULL );
            mouseEvent.setPos( m_ptLastMouse );
            mouseEvent.setButtonDownPos( button, m_ptLastMouse );
            mouseEvent.setButtonDownScenePos( button, ptGlobal);
            mouseEvent.setButtonDownScreenPos( button, ptGlobal );
            mouseEvent.setScenePos( ptGlobal );
            mouseEvent.setScreenPos( ptGlobal );
            mouseEvent.setLastPos( m_ptLastMouse );
            mouseEvent.setLastScenePos( ptGlobal );
            mouseEvent.setLastScreenPos( ptGlobal );
            mouseEvent.setButtons( m_lastMouseButtons );
            mouseEvent.setButton( button );
            mouseEvent.setModifiers( 0 );
            mouseEvent.setAccepted( false );

            QApplication::sendEvent( m_pScene, &mouseEvent );
        }
            break;

        case vr::VREvent_MouseButtonUp:
        {
            Qt::MouseButton button = vrEvent.data.mouse.button == vr::VRMouseButton_Right ? Qt::RightButton : Qt::LeftButton;
            m_lastMouseButtons &= ~button;

            QPoint ptGlobal = m_ptLastMouse.toPoint();
            QGraphicsSceneMouseEvent mouseEvent( QEvent::GraphicsSceneMouseRelease );
            mouseEvent.setWidget( NULL );
            mouseEvent.setPos( m_ptLastMouse );
            mouseEvent.setScenePos( ptGlobal );
            mouseEvent.setScreenPos( ptGlobal );
            mouseEvent.setLastPos( m_ptLastMouse );
            mouseEvent.setLastScenePos( ptGlobal );
            mouseEvent.setLastScreenPos( ptGlobal );
            mouseEvent.setButtons( m_lastMouseButtons );
            mouseEvent.setButton( button );
            mouseEvent.setModifiers( 0 );
            mouseEvent.setAccepted( false );

            QApplication::sendEvent(  m_pScene, &mouseEvent );
        }
            break;

        case vr::VREvent_OverlayShown:
        {
            m_pWidget->repaint();
        }
            break;

        case vr::VREvent_Quit:
            QApplication::exit();
            break;
        }
    }

    //if( m_ulOverlayThumbnailHandle != vr::k_ulOverlayHandleInvalid )
    //{
    while( vr::VROverlay()->PollNextOverlayEvent( m_ulOverlayHandle, &vrEvent, sizeof( vrEvent)  ) )
    {
        switch( vrEvent.eventType )
        {
        case vr::VREvent_OverlayShown:
        {
            m_pWidget->repaint();
        }
            break;
        }
    }
    //}

}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void COpenVROverlayController::SetWidget( QWidget *pWidget )
{
    if( m_pScene )
    {
        // all of the mouse handling stuff requires that the widget be at 0,0
        pWidget->move( 0, 0 );
        m_pScene->addWidget( pWidget );
    } else {
        Log(QString::fromStdString("(SetWidget) m_pScene doesn't exist"));
    }

    std::string widgetStr = "(SetWidget) pWidget is NULL: ";
    widgetStr.append((pWidget == NULL) ? "true" : "false");
    Log(QString::fromStdString(widgetStr));

    m_pWidget = pWidget;

    widgetStr = "(SetWidget) m_pWidget is NULL: ";
    widgetStr.append((pWidget == NULL) ? "true" : "false");
    Log(QString::fromStdString(widgetStr));

    m_pFbo = new QOpenGLFramebufferObject( pWidget->width(), pWidget->height(), GL_TEXTURE_2D );
    m_pFbo->setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

    if( vr::VROverlay() )
    {
        vr::HmdVector2_t vecWindowSize =
        {
            (float)pWidget->width(),
            (float)pWidget->height()
        };
        //vr::VROverlay()->SetOverlayMouseScale( m_ulOverlayHandle, &vecWindowSize );
        std::string overlayHandleStr = "(SetWidget) Overlay handle: ";
        overlayHandleStr.append((m_ulOverlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
        Log(QString::fromStdString(overlayHandleStr));

        vr::VROverlay()->ShowOverlay(m_ulOverlayHandle);

        std::string isShown = "(SetWidget) Overlay is visible: ";
        isShown.append(vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle) ? "true" : "false");
        Log(QString::fromStdString(isShown));
        //vr::VROverlay()->ShowDashboard(m_pch);
    } else {
        Log(QString::fromStdString("(SetWidget) VROverlay is not running"));
    }

}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool COpenVROverlayController::ConnectToVRRuntime()
{
    m_eLastHmdError = vr::VRInitError_None;
    vr::IVRSystem *pVRSystem = vr::VR_Init( &m_eLastHmdError, vr::VRApplication_Overlay );

    if ( m_eLastHmdError != vr::VRInitError_None )
    {
        m_strVRDriver = "No Driver";
        m_strVRDisplay = "No Display";
        return false;
    }

    m_strVRDriver = GetTrackedDeviceString(pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
    m_strVRDisplay = GetTrackedDeviceString(pVRSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

    return true;
}


void COpenVROverlayController::DisconnectFromVRRuntime()
{
    vr::VR_Shutdown();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
QString COpenVROverlayController::GetVRDriverString()
{
    return m_strVRDriver;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
QString COpenVROverlayController::GetVRDisplayString()
{
    return m_strVRDisplay;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool COpenVROverlayController::BHMDAvailable()
{
    return vr::VRSystem() != NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

vr::HmdError COpenVROverlayController::GetLastHmdError()
{
    return m_eLastHmdError;
}

void COpenVROverlayController::ShowWidget() {
    if (!vr::VROverlay() || !vr::VRSystem()) {

        std::string overlayStatus = "(ShowWidget) VROverlay(): ";
        std::string systemStatus = "(ShowWidget) VRSystem(): ";
        overlayStatus.append(vr::VROverlay() ? "initialized" : "invalid");
        systemStatus.append(vr::VRSystem() ? "initialized" : "invalid");
        Log(QString::fromStdString(overlayStatus));
        Log(QString::fromStdString(systemStatus));

        return;
    }
    if (vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle)) {
        Log(QString::fromStdString("(Show Widget) Overlay already visible"));
        return;
    }

    std::string overlayHandleStr = "(ShowWidget) Overlay handle: ";
    overlayHandleStr.append((m_ulOverlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
    Log(QString::fromStdString(overlayHandleStr));

    vr::VROverlay()->ShowOverlay(m_ulOverlayHandle);

    std::string isShown = "(ShowWidget) Overlay is visible: ";
    isShown.append(vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle) ? "true" : "false");
    Log(QString::fromStdString(isShown));
}

void COpenVROverlayController::HideWidget() {
    if (!vr::VROverlay() || !vr::VRSystem()) {

        std::string overlayStatus = "(HideWidget) VROverlay(): ";
        std::string systemStatus = "(HideWidget) VRSystem(): ";
        overlayStatus.append(vr::VROverlay() ? "initialized" : "invalid");
        systemStatus.append(vr::VRSystem() ? "initialized" : "invalid");
        Log(QString::fromStdString(overlayStatus));
        Log(QString::fromStdString(systemStatus));

        return;
    }
    if (!vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle)) {

        Log(QString::fromStdString("(HideWidget) Overlay is not visible"));

        return;
    }

    std::string overlayHandleStr = "(HideWidget) Overlay handle: ";
    overlayHandleStr.append((m_ulOverlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
    Log(QString::fromStdString(overlayHandleStr));

    vr::VROverlay()->HideOverlay(m_ulOverlayHandle);

    std::string isShown = "(HideWidget) Overlay is visible: ";
    isShown.append(vr::VROverlay()->IsOverlayVisible(m_ulOverlayHandle) ? "true" : "false");
    Log(QString::fromStdString(isShown));
}


void COpenVROverlayController::Log(QString str) {
    //QMutexLocker locker(&m_lineLoggerMutex);

    QFile f(QString::fromStdString("log.log"));
    //doRollLogsIfNeeded(static_cast<qint64>(f.size() + lineToBelogged.length()));

    // Do not open in append mode but seek() to avoid warning for unseekable
    // devices, note that if open is made with WriteOnly without Append, the
    // file gets truncated
    if (!f.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QTextStream out(stdout);
        out << "CANNOT OPEN LOG FILE: " << QString::fromStdString("log.log");
        return;
    }
    // seek() does nothing on sequential devices, this is in essence what QFile
    // does when Append flag is set in open() but without warning (on Qt 4.8.3)
    // However, Qt 4.8.1 issues the warning, so check it explicitly
    if (!f.isSequential())
    {
        f.seek(f.size());
    }

    QTextStream out(&f);
    out << str << endl;
}

void COpenVROverlayController::Log(std::string str) {
    Log(QString::fromStdString(str));
}

