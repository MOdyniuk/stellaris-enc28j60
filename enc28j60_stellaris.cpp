#include "enc28j60_stellaris.h"
#include "enc28j60.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

void
ENC28J60::Glue::ChipSelect(ENC28J60::Driver *driver) {
	MAP_GPIOPinWrite(StellarisConfig::PIN_CHIP_SELECT_BASE, StellarisConfig::PIN_CHIP_SELECT, 0);
}

void
ENC28J60::Glue::ChipDeSelect(ENC28J60::Driver *driver) {
	MAP_GPIOPinWrite(StellarisConfig::PIN_CHIP_SELECT_BASE, StellarisConfig::PIN_CHIP_SELECT, StellarisConfig::PIN_CHIP_SELECT);
}

uint8_t
ENC28J60::Glue::SpiSend(ENC28J60::Driver *driver, uint8_t msg) {
	unsigned long val;
	MAP_SSIDataPut(StellarisConfig::SPI_SSI_BASE, msg);
	MAP_SSIDataGet(StellarisConfig::SPI_SSI_BASE, &val);
	return (uint8_t)val;
}

void
ENC28J60::Glue::PinSet(ENC28J60::Driver *driver, ENC28J60::PinType pin, ENC28J60::PinValue value) {
}

void
ENC28J60::Glue::Init(ENC28J60::Driver *driver) {
	// Configure SSI2 for ENC28J60 usage
	// GPIO_PB4 is CLK
	// GPIO_PB6 is RX
	// GPIO_PB7 is TX
	MAP_SysCtlPeripheralEnable(StellarisConfig::SPI_PORT_PERIPHERAL);
	MAP_SysCtlPeripheralEnable(StellarisConfig::SPI_SSI_PERIPHERAL);
	MAP_GPIOPinConfigure(StellarisConfig::SPI_SSI_CLK_CONF);
	MAP_GPIOPinConfigure(StellarisConfig::SPI_SSI_RX_CONF);
	MAP_GPIOPinConfigure(StellarisConfig::SPI_SSI_TX_CONF);
	MAP_GPIOPinTypeSSI(StellarisConfig::SPI_PORT_BASE, StellarisConfig::SPI_SSI_CLK_PIN | StellarisConfig::SPI_SSI_TX_PIN | StellarisConfig::SPI_SSI_RX_PIN);
	MAP_SSIConfigSetExpClk(StellarisConfig::SPI_SSI_BASE, MAP_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
			SSI_MODE_MASTER, 1000000, 8);
	MAP_SSIEnable(StellarisConfig::SPI_SSI_BASE);

	unsigned long b;
	while(MAP_SSIDataGetNonBlocking(StellarisConfig::SPI_SSI_BASE, &b)) {}

	MAP_SysCtlPeripheralEnable(StellarisConfig::PIN_CHIP_SELECT_PERIPH);
	MAP_SysCtlPeripheralEnable(StellarisConfig::PIN_INT_PERIPH);
	MAP_GPIOPinTypeGPIOOutput(StellarisConfig::PIN_CHIP_SELECT_BASE, StellarisConfig::PIN_CHIP_SELECT);

	MAP_GPIOPinTypeGPIOInput(StellarisConfig::PIN_INT_BASE, StellarisConfig::PIN_INT);

	MAP_GPIOPinWrite(StellarisConfig::PIN_CHIP_SELECT_BASE, StellarisConfig::PIN_CHIP_SELECT, StellarisConfig::PIN_CHIP_SELECT);
	MAP_IntEnable(StellarisConfig::PIN_INT_INT);

	MAP_GPIOIntTypeSet(StellarisConfig::PIN_INT_BASE, StellarisConfig::PIN_INT, GPIO_FALLING_EDGE);
	MAP_GPIOPinIntClear(StellarisConfig::PIN_INT_BASE, StellarisConfig::PIN_INT);
	MAP_GPIOPinIntEnable(StellarisConfig::PIN_INT_BASE, StellarisConfig::PIN_INT);
}

void
ENC28J60::Glue::Delay(uint32_t ms) {
	MAP_SysCtlDelay(((MAP_SysCtlClockGet()/3)/1000)*ms);
}

void
ENC28J60::Glue::OnReceive(ENC28J60::Driver *driver, uint16_t data_count) {
	uip_len = data_count;
	driver->RBM(uip_buf, data_count);

	if (BUF->type == htons(UIP_ETHTYPE_IP))
	{
		uip_arp_ipin();
		uip_input();

		if (uip_len > 0)
		{
			uip_arp_out();
			driver->Send(uip_buf, uip_len);
			uip_len = 0;
		}
	}
	else if (BUF->type == htons(UIP_ETHTYPE_ARP))
	{
		uip_arp_arpin();
		if(uip_len > 0)
		{
			driver->Send(uip_buf, uip_len);
			uip_len = 0;
		}
	}
}
