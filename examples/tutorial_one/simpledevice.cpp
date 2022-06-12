/*
   INDI Developers Manual
   Tutorial #1

   "Hello INDI"

   We construct a most basic (and useless) device driver to illustrate INDI.

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

/** \file simpledevice.cpp
    \brief Construct a basic INDI device with only one property to connect and disconnect.
    \author Jasem Mutlaq

    \example simpledevice.cpp
    A very minimal device! It also allows you to connect/disconnect and performs no other functions.
*/

#include "simpledevice.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <memory>
#include <cerrno>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/ioctl.h>

#define FLAT_CMD 6
#define FLAT_RES 8

std::unique_ptr<SimpleDevice> simpleDevice(new SimpleDevice());

SimpleDevice::SimpleDevice() : LightBoxInterface(this, true)
{
    setVersion(1, 1);
}


bool SimpleDevice::initProperties()
{
    INDI::DefaultDevice::initProperties();


    initDustCapProperties(getDeviceName(), MAIN_CONTROL_TAB);
    initLightBoxProperties(getDeviceName(), MAIN_CONTROL_TAB);

    LightIntensityN[0].min  = 0;
    LightIntensityN[0].max  = 255;
    LightIntensityN[0].step = 1;

    // Set DUSTCAP_INTEFACE later on connect after we verify whether it's flip-flat (dust cover + light) or just flip-man (light only)
    setDriverInterface(AUX_INTERFACE | LIGHTBOX_INTERFACE | DUSTCAP_INTERFACE);

    addAuxControls();

    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]()
                                        {
                                            return Handshake();
                                        });
    registerConnection(serialConnection);

    return true;
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *SimpleDevice::getDefaultName()
{
    return "SimpleDevice";
}

IPState SimpleDevice::ParkCap()
{
    sendCommand("close");
    return IPS_IDLE;
}

IPState SimpleDevice::UnParkCap()
{
    sendCommand("opena");
    return IPS_IDLE;
}

bool SimpleDevice::EnableLightBox(bool enable)
{
    if(!enable) {
        prevBrightness = 0;
    }
    char command[FLAT_CMD] = {0};

    snprintf(command, FLAT_CMD, "00%03d", prevBrightness);
    sendCommand(command);
    return true;
}

bool SimpleDevice::SetLightBoxBrightness(uint16_t value)
{
    prevBrightness = value;

    char command[FLAT_CMD] = {0};

    snprintf(command, FLAT_CMD, "00%03d", prevBrightness);
    sendCommand(command);
    return true;
}

bool SimpleDevice::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (processLightBoxNumber(dev, name, values, names, n))
        return true;

    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool SimpleDevice::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (processLightBoxText(dev, name, texts, names, n))
            return true;
    }

    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool SimpleDevice::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (processDustCapSwitch(dev, name, states, names, n))
            return true;

        if (processLightBoxSwitch(dev, name, states, names, n))
            return true;
    }

    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool SimpleDevice::ISSnoopDevice(XMLEle *root)
{
    snoopLightBox(root);

    return INDI::DefaultDevice::ISSnoopDevice(root);
}

bool SimpleDevice::saveConfigItems(FILE *fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);

    return saveLightBoxConfigItems(fp);
}

bool SimpleDevice::sendCommand(const char *command)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[100] = {0};
    int i = 0;

    tcflush(PortFD, TCIOFLUSH);

    LOGF_DEBUG("CMD <%s>", command);

    char buffer[FLAT_CMD + 1] = {0}; // space for terminating null
    snprintf(buffer, FLAT_CMD + 1, "%s\n", command);

    for (i = 0; i < 3; i++)
    {
        if ((rc = tty_write(PortFD, buffer, FLAT_CMD, &nbytes_written)) != TTY_OK)
        {
            usleep(50000);
            continue;
        }

    }

    if (i == 3)
    {
        tty_error_msg(rc, errstr, 100);
        LOGF_ERROR("%s error: %s.", command, errstr);
        return false;
    }

    return true;
}

bool SimpleDevice::Handshake()
{

    PortFD = serialConnection->getPortFD();

    /* Drop RTS */
    int i = 0;
    i |= TIOCM_RTS;
    if (ioctl(PortFD, TIOCMBIC, &i) != 0)
    {
        LOGF_ERROR("IOCTL error %s.", strerror(errno));
        return false;
    }

    i |= TIOCM_RTS;
    if (ioctl(PortFD, TIOCMGET, &i) != 0)
    {
        LOGF_ERROR("IOCTL error %s.", strerror(errno));
        return false;
    }


    return true;
}

bool SimpleDevice::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineProperty(&LightSP);
        defineProperty(&LightIntensityNP);
        defineProperty(&ParkCapSP);
    }
    else
    {
        deleteProperty(LightSP.name);
        deleteProperty(LightIntensityNP.name);
        deleteProperty(ParkCapSP.name);
    }

    updateLightBoxProperties();
    return true;
}