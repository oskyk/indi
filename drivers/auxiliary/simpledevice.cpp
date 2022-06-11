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
    sendCommand("close", 5);
    return IPS_IDLE;
}

IPState SimpleDevice::UnParkCap()
{
    sendCommand("open", 4);
    return IPS_IDLE;
}

bool SimpleDevice::EnableLightBox(bool enable)
{
    if(!enable) {
        prevBrightness = 0;
    }
    char command[3] = {0};

    snprintf(command, 3, "%03d", prevBrightness);
    sendCommand(command, 3);
    return true;
}

bool SimpleDevice::SetLightBoxBrightness(uint16_t value)
{
    prevBrightness = value;

    char command[3] = {0};

    snprintf(command, 3, "%03d", prevBrightness);
    sendCommand(command, 3);
    return true;
}

bool SimpleDevice::sendCommand(const char *command, int len)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF] = {0};
    int i = 0;

    tcflush(PortFD, TCIOFLUSH);

    LOGF_DEBUG("CMD <%s>", command);

    char buffer[len + 1] = {0}; // space for terminating null
    snprintf(buffer, len + 1, "%s\n", command);

    for (i = 0; i < 3; i++)
    {
        if ((rc = tty_write(PortFD, buffer, len, &nbytes_written)) != TTY_OK)
        {
            usleep(50000);
            continue;
        }

    }

    if (i == 3)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
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