#ifndef OVERLAYCONTROLLER_H
#define OVERLAYCONTROLLER_H

#ifdef _WIN32
#pragma once
#endif

#include "openvr.h"
#include <QtCore/QtCore>

class OverlayController {

public:
    static OverlayController *Instance();

public:
    OverlayController();
    virtual ~OverlayController();

    bool Init();
    void Shutdown();

    void ShowWidget();
    void HideWidget();
    void Log(QString str);
    void Log(std::string str);

private:
    bool ConnectToVRRuntime();
    void DisconnectFromVRRuntime();

private:
    vr::HmdError lastHMDError;
    vr::HmdError compositorError;
    vr::HmdError overlayError;
    vr::Compositor_OverlaySettings overlaySettings;
    vr::VROverlayHandle_t overlayHandle;
    QString m_strName;

    const char *pch;
};

#endif // OVERLAYCONTROLLER_H
