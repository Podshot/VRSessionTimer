//====== Copyright Valve Corporation, All rights reserved. =======

#include "overlaycontroller.h"
#include <iostream>

OverlayController *s_pSharedVRController = NULL;

OverlayController *OverlayController::Instance()
{
    if ( !s_pSharedVRController )
    {
        s_pSharedVRController = new OverlayController();
    }
    return s_pSharedVRController;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
OverlayController::OverlayController()
{
    lastHMDError = vr::VRInitError_None;
    compositorError = vr::VRInitError_None;
    overlayError = vr::VRInitError_None;
    overlayHandle = vr::k_ulOverlayHandleInvalid;
    m_strName = "vrsessiontimer";
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
OverlayController::~OverlayController()
{
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool OverlayController::Init()
{
    bool bSuccess = true;

    m_strName = "vrsessiontimer";

    // Loading the OpenVR Runtime
    bSuccess = ConnectToVRRuntime();

    bSuccess = bSuccess && vr::VRCompositor() != NULL;

    if( vr::VROverlay() )
    {
        std::string sKey = m_strName.toStdString();
        pch = sKey.c_str();
        //vr::VROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay( sKey.c_str(), m_strName.toStdString().c_str(), &overlayHandle, &m_ulOverlayThumbnailHandle );

        std::string pchKeyStr = "(Init) Created overlay with pchKey: ";
        std::string initOverlayHandleStr = "(Init) Initial overlay handle: ";
        std::string changedOverlayHandleStr = "(Init) Initialized overlay handle: ";
        initOverlayHandleStr.append((overlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
        pchKeyStr.append(pch);
        Log(QString::fromStdString(pchKeyStr));
        Log(QString::fromStdString(initOverlayHandleStr));

        vr::VROverlayError overlayError = vr::VROverlay()->CreateOverlay( pch, m_strName.toStdString().c_str(), &overlayHandle);

        vr::VROverlay()->SetOverlayFromFile(overlayHandle, "C:/test.png");

        changedOverlayHandleStr.append((overlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
        Log(QString::fromStdString(changedOverlayHandleStr));

        bSuccess = bSuccess && overlayError == vr::VROverlayError_None;
    } else {
        Log(QString::fromStdString("No VROverlay()"));
    }

    if( bSuccess )
    {
        vr::VROverlay()->SetOverlayWidthInMeters( overlayHandle, 1.5f );

    }
    return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void OverlayController::Shutdown()
{
    DisconnectFromVRRuntime();

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool OverlayController::ConnectToVRRuntime()
{
    lastHMDError = vr::VRInitError_None;
    vr::IVRSystem *pVRSystem = vr::VR_Init( &lastHMDError, vr::VRApplication_Overlay );

    if ( lastHMDError != vr::VRInitError_None )
    {
        return false;
    }
    return true;
}


void OverlayController::DisconnectFromVRRuntime()
{
    vr::VR_Shutdown();
}

void OverlayController::ShowWidget() {
    if (!vr::VROverlay() || !vr::VRSystem()) {

        std::string overlayStatus = "(ShowWidget) VROverlay(): ";
        std::string systemStatus = "(ShowWidget) VRSystem(): ";
        overlayStatus.append(vr::VROverlay() ? "initialized" : "invalid");
        systemStatus.append(vr::VRSystem() ? "initialized" : "invalid");
        Log(QString::fromStdString(overlayStatus));
        Log(QString::fromStdString(systemStatus));

        return;
    }
    if (vr::VROverlay()->IsOverlayVisible(overlayHandle)) {
        Log(QString::fromStdString("(Show Widget) Overlay already visible"));
        return;
    }

    std::string overlayHandleStr = "(ShowWidget) Overlay handle: ";
    overlayHandleStr.append((overlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
    Log(QString::fromStdString(overlayHandleStr));

    vr::VROverlay()->ShowOverlay(overlayHandle);

    std::string isShown = "(ShowWidget) Overlay is visible: ";
    isShown.append(vr::VROverlay()->IsOverlayVisible(overlayHandle) ? "true" : "false");
    Log(QString::fromStdString(isShown));
}

void OverlayController::HideWidget() {
    if (!vr::VROverlay() || !vr::VRSystem()) {

        std::string overlayStatus = "(HideWidget) VROverlay(): ";
        std::string systemStatus = "(HideWidget) VRSystem(): ";
        overlayStatus.append(vr::VROverlay() ? "initialized" : "invalid");
        systemStatus.append(vr::VRSystem() ? "initialized" : "invalid");
        Log(QString::fromStdString(overlayStatus));
        Log(QString::fromStdString(systemStatus));

        return;
    }
    if (!vr::VROverlay()->IsOverlayVisible(overlayHandle)) {

        Log(QString::fromStdString("(HideWidget) Overlay is not visible"));

        return;
    }

    std::string overlayHandleStr = "(HideWidget) Overlay handle: ";
    overlayHandleStr.append((overlayHandle == vr::k_ulOverlayHandleInvalid) ? "invalid" : "valid");
    Log(QString::fromStdString(overlayHandleStr));

    vr::VROverlay()->HideOverlay(overlayHandle);

    std::string isShown = "(HideWidget) Overlay is visible: ";
    isShown.append(vr::VROverlay()->IsOverlayVisible(overlayHandle) ? "true" : "false");
    Log(QString::fromStdString(isShown));
}


void OverlayController::Log(QString str) {
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

void OverlayController::Log(std::string str) {
    Log(QString::fromStdString(str));
}

