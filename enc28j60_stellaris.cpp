#include "enc28j60_stellaris.h"
#include "enc28j60.h"

void
ENCJ_STELLARIS::BusDriver::ChipSelect(void* driverId) {
	MAP_GPIOPinWrite(StellarisENC28J60Configuration::PIN_CHIP_SELECT_BASE, StellarisENC28J60Configuration::PIN_CHIP_SELECT, 0);
}

void
ENCJ_STELLARIS::BusDriver::ChipDeSelect(void* driverId) {
	MAP_GPIOPinWrite(StellarisENC28J60Configuration::PIN_CHIP_SELECT_BASE, StellarisENC28J60Configuration::PIN_CHIP_SELECT, StellarisENC28J60Configuration::PIN_CHIP_SELECT);
}

uint8_t
ENCJ_STELLARIS::BusDriver::SpiSend(void *driverId, uint8_t msg) {
	unsigned long val;
	MAP_SSIDataPut(StellarisENC28J60Configuration::SPI_SSI_BASE, msg);
	MAP_SSIDataGet(StellarisENC28J60Configuration::SPI_SSI_BASE, &val);
	return (uint8_t)val;
}

void ENCJ_STELLARIS::BusDriver::PinSet(void *driverId, ENCJ_STELLARIS::PinType pin, ENCJ_STELLARIS::PinValue value) {
}

/* Initialization code which is not part of the interface */
void ENCJ_STELLARIS::BusDriver::Init(void *driverId) {
	// Configure SSI2 for ENC28J60 usage
	// GPIO_PB4 is CLK
	// GPIO_PB6 is RX
	// GPIO_PB7 is TX
	MAP_SysCtlPeripheralEnable(StellarisENC28J60Configuration::SPI_PORT_PERIPHERAL);
	MAP_SysCtlPeripheralEnable(StellarisENC28J60Configuration::SPI_SSI_PERIPHERAL);
	MAP_GPIOPinConfigure(StellarisENC28J60Configuration::SPI_SSI_CLK_CONF);
	MAP_GPIOPinConfigure(StellarisENC28J60Configuration::SPI_SSI_RX_CONF);
	MAP_GPIOPinConfigure(StellarisENC28J60Configuration::SPI_SSI_TX_CONF);
	MAP_GPIOPinTypeSSI(StellarisENC28J60Configuration::SPI_PORT_BASE, StellarisENC28J60Configuration::SPI_SSI_CLK_PIN | StellarisENC28J60Configuration::SPI_SSI_TX_PIN | StellarisENC28J60Configuration::SPI_SSI_RX_PIN);
	MAP_SSIConfigSetExpClk(StellarisENC28J60Configuration::SPI_SSI_BASE, MAP_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
			SSI_MODE_MASTER, 1000000, 8);
	MAP_SSIEnable(StellarisENC28J60Configuration::SPI_SSI_BASE);

	unsigned long b;
	while(MAP_SSIDataGetNonBlocking(StellarisENC28J60Configuration::SPI_SSI_BASE, &b)) {}

	MAP_SysCtlPeripheralEnable(StellarisENC28J60Configuration::PIN_CHIP_SELECT_PERIPH);
	MAP_SysCtlPeripheralEnable(StellarisENC28J60Configuration::PIN_INT_PERIPH);
	MAP_GPIOPinTypeGPIOOutput(StellarisENC28J60Configuration::PIN_CHIP_SELECT_BASE, StellarisENC28J60Configuration::PIN_CHIP_SELECT);

	MAP_GPIOPinTypeGPIOInput(StellarisENC28J60Configuration::PIN_INT_BASE, StellarisENC28J60Configuration::PIN_INT);

	MAP_GPIOPinWrite(StellarisENC28J60Configuration::PIN_CHIP_SELECT_BASE, StellarisENC28J60Configuration::PIN_CHIP_SELECT, StellarisENC28J60Configuration::PIN_CHIP_SELECT);
	MAP_IntEnable(StellarisENC28J60Configuration::PIN_INT_INT);

	MAP_GPIOIntTypeSet(StellarisENC28J60Configuration::PIN_INT_BASE, StellarisENC28J60Configuration::PIN_INT, GPIO_FALLING_EDGE);
	MAP_GPIOPinIntClear(StellarisENC28J60Configuration::PIN_INT_BASE, StellarisENC28J60Configuration::PIN_INT);
	MAP_GPIOPinIntEnable(StellarisENC28J60Configuration::PIN_INT_BASE, StellarisENC28J60Configuration::PIN_INT);
}
