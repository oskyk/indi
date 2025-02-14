/*
   INDI Developers Manual
   Tutorial #1

   "Hello INDI"

   We construct a most basic (and useless) device driver to illustate INDI.

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

/** \file simpledevice.h
    \brief Construct a basic INDI device with only one property to connect and disconnect.
    \author Jasem Mutlaq

    \example simpledevice.h
    A very minimal device! It also allows you to connect/disconnect and performs no other functions.
*/

#pragma once

#include "defaultdevice.h"
#include "indilightboxinterface.h"
#include "indidustcapinterface.h"

class SimpleDevice : public INDI::DefaultDevice, public INDI::LightBoxInterface, public INDI::DustCapInterface
{
public:
    SimpleDevice();
    virtual ~SimpleDevice() = default;

    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISSnoopDevice(XMLEle *root) override;

protected:
    const char *getDefaultName() override;

    virtual bool initProperties() override;
    bool updateProperties() override;

    // From Dust Cap
    virtual IPState ParkCap() override;
    virtual IPState UnParkCap() override;

    // From Light Box
    virtual bool SetLightBoxBrightness(uint16_t value) override;
    virtual bool EnableLightBox(bool enable) override;

    virtual bool saveConfigItems(FILE *fp) override;
private:
    bool sendCommand(const char *command);
    bool Handshake();

    uint8_t prevCoverStatus{ 0xFF };
    uint8_t prevLightStatus{ 0xFF };
    uint8_t prevBrightness{ 0xFF };
    int PortFD{ -1 };

    Connection::Serial *serialConnection{ nullptr };
};
