#include "common.h"
#include <driverlib/systick.h>

#ifdef __cplusplus 
extern "C" {
#endif
#include "utils/uartstdio.c"
#ifdef __cplusplus 
}
#endif

#include "enc28j60.h"

static volatile unsigned long g_ulFlags;

volatile unsigned long g_ulTickCounter = 0;

#define UIP_PERIODIC_TIMER_MS	500
#define UIP_ARP_TIMER_MS		10000

#define FLAG_SYSTICK		0
#define FLAG_RXPKT			1
#define FLAG_TXPKT			2
#define FLAG_RXPKTPEND		3
#define FLAG_ENC_INT		4

#define SYSTICKHZ		CLOCK_CONF_SECOND
#define SYSTICKMS		(1000 / SYSTICKHZ)

extern "C" {
void uip_log(char *msg) {
	printf("UIP: %s\n", msg);
}
}



/**
 * Various config options are present here
 *
 * MAC address of interface. Presumably unique, from a defunct Denmark tech
 * company.
 */
const uint8_t mac_addr[] = { 0x00, 0xC0, 0x033, 0x38, 0x22, 0xA4 };
/**
 * Uncomment this first line for a static IP. Leave it for DHCP. Don't touch
 * the second line.
 */
//#define STATIC_IP

#ifdef STATIC_IP
/**
 * Default IP address of gateway
 */
#define DEFAULT_IPADDR0 10
#define DEFAULT_IPADDR1 0
#define DEFAULT_IPADDR2 0
#define DEFAULT_IPADDR3 201
/**
 * Default netmask
 */
#define DEFAULT_NETMASK0 255
#define DEFAULT_NETMASK1 255
#define DEFAULT_NETMASK2 255
#define DEFAULT_NETMASK3 0

#endif

static void cpu_init(void) {
	// A safety loop in order to interrupt the MCU before setting the clock (wrongly)
	int i;
	for(i=0; i<1000000; i++);

	// Setup for 16MHZ external crystal, use 200MHz PLL and divide by 4 = 50MHz
	MAP_SysCtlClockSet(SYSCTL_SYSDIV_16 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
		SYSCTL_XTAL_16MHZ);
}

static void uart_init(void) {
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Configure PD0 and PD1 for UART
	MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
	MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
	MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTStdioInitExpClk(0, 115200);
}


class StellarisBusDriver : public ENCJ_STELLARIS::BusDriver {
private:
	/* SPI Pin Configuration */
	static const int SPI_PORT_BASE		= GPIO_PORTB_BASE;
	static const int SPI_SSI_BASE		= SSI2_BASE;
	static const int SPI_PORT_PERIPHERAL 	= SYSCTL_PERIPH_GPIOB;
	static const int SPI_SSI_PERIPHERAL  	= SYSCTL_PERIPH_SSI2;
	static const int SPI_SSI_RX_CONF	= GPIO_PB6_SSI2RX;
	static const int SPI_SSI_TX_CONF	= GPIO_PB7_SSI2TX;
	static const int SPI_SSI_CLK_CONF	= GPIO_PB4_SSI2CLK;
	static const int SPI_SSI_RX_PIN		= GPIO_PIN_6;
	static const int SPI_SSI_TX_PIN		= GPIO_PIN_7;
	static const int SPI_SSI_CLK_PIN	= GPIO_PIN_4;

	/* GPIO Pin Configuration */
	static const int PIN_CHIP_SELECT_PERIPH = SYSCTL_PERIPH_GPIOB;
	static const int PIN_CHIP_SELECT_BASE	= GPIO_PORTB_BASE;
	static const int PIN_CHIP_SELECT	= GPIO_PIN_5;

	static const int PIN_INT_PERIPH		= SYSCTL_PERIPH_GPIOE;
	static const int PIN_INT_BASE		= GPIO_PORTE_BASE;
	static const int PIN_INT		= GPIO_PIN_4;
	static const int PIN_INT_INT		= INT_GPIOE;
	
public:
	/* BusDriver interface implementations */
	void 	ChipSelect(uint8_t driverId) {
		MAP_GPIOPinWrite(PIN_CHIP_SELECT_BASE, PIN_CHIP_SELECT, 0);
	}

	void 	ChipDeSelect(uint8_t driverId) {
		MAP_GPIOPinWrite(PIN_CHIP_SELECT_BASE, PIN_CHIP_SELECT, PIN_CHIP_SELECT);
	}

	uint8_t SpiSend(uint8_t driverId, uint8_t msg) {
		unsigned long val;
		MAP_SSIDataPut(SPI_SSI_BASE, msg);
		MAP_SSIDataGet(SPI_SSI_BASE, &val);
		return (uint8_t)val;
	}

	void	PinSet(uint8_t driverId, ENCJ_STELLARIS::PinType pin, ENCJ_STELLARIS::PinValue value) {
	}

	/* Initialization code which is not part of the interface */
	void	SpiInit() {
		// Configure SSI2 for ENC28J60 usage
		// GPIO_PB4 is CLK
		// GPIO_PB6 is RX
		// GPIO_PB7 is TX
		MAP_SysCtlPeripheralEnable(SPI_PORT_PERIPHERAL);
		MAP_SysCtlPeripheralEnable(SPI_SSI_PERIPHERAL);
		MAP_GPIOPinConfigure(SPI_SSI_CLK_CONF);
		MAP_GPIOPinConfigure(SPI_SSI_RX_CONF);
		MAP_GPIOPinConfigure(SPI_SSI_TX_CONF);
		MAP_GPIOPinTypeSSI(SPI_PORT_BASE, SPI_SSI_CLK_PIN | SPI_SSI_TX_PIN | SPI_SSI_RX_PIN);
		MAP_SSIConfigSetExpClk(SPI_SSI_BASE, MAP_SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
				SSI_MODE_MASTER, 1000000, 8);
		MAP_SSIEnable(SPI_SSI_BASE);

		unsigned long b;
		while(MAP_SSIDataGetNonBlocking(SSI2_BASE, &b)) {}
	}

	void InitPins() {
		MAP_SysCtlPeripheralEnable(PIN_CHIP_SELECT_PERIPH);
		MAP_SysCtlPeripheralEnable(PIN_INT_PERIPH);
		MAP_GPIOPinTypeGPIOOutput(PIN_CHIP_SELECT_BASE, PIN_CHIP_SELECT);

		MAP_GPIOPinTypeGPIOInput(PIN_INT_BASE, PIN_INT);

		MAP_GPIOPinWrite(PIN_CHIP_SELECT_BASE, PIN_CHIP_SELECT, PIN_CHIP_SELECT);
		MAP_IntEnable(PIN_INT_INT);

		MAP_GPIOIntTypeSet(PIN_INT_BASE, PIN_INT, GPIO_FALLING_EDGE);
		MAP_GPIOPinIntClear(PIN_INT_BASE, PIN_INT);
		MAP_GPIOPinIntEnable(PIN_INT_BASE, PIN_INT);
	}

	void Init() {
		InitPins();
		SpiInit();
	}
};

int main(void) {
	static struct uip_eth_addr eth_addr;
	uip_ipaddr_t ipaddr;

	cpu_init();
	uart_init();
	printf("Welcome\n");

	StellarisBusDriver busDriver;
	busDriver.Init();
	// One line config! Woo!
	ENCJ_STELLARIS::ENC28J60 chip(mac_addr, 0, busDriver);

	printf("ENC28J60 online\n");

	//
	// Configure SysTick for a periodic interrupt.
	//
	MAP_SysTickPeriodSet(MAP_SysCtlClockGet() / SYSTICKHZ);
	MAP_SysTickEnable();
	MAP_SysTickIntEnable();	

	uip_init();

	// Dump MAC into uIP
	for (int i = 0; i < 6; ++i)
	{
		eth_addr.addr[i] = mac_addr[i];
	}
	uip_setethaddr(eth_addr);


#ifdef STATIC_IP
	// Static IP
	uip_ipaddr
		( ipaddr
		, DEFAULT_IPADDR0
		, DEFAULT_IPADDR1
		, DEFAULT_IPADDR2
		, DEFAULT_IPADDR3
		);
	uip_sethostaddr(ipaddr);

	// Static Netmask
	uip_ipaddr
		( ipaddr
		, DEFAULT_NETMASK0
		, DEFAULT_NETMASK1
		, DEFAULT_NETMASK2
		, DEFAULT_NETMASK3
		);
	uip_setnetmask(ipaddr);

#ifdef _DEBUG
	printf("IP: %d.%d.%d.%d\n", DEFAULT_IPADDR0, DEFAULT_IPADDR1,
		DEFAULT_IPADDR2, DEFAULT_IPADDR3);
#endif

#else
	// DHCP
	uip_ipaddr(ipaddr, 0, 0, 0, 0);
	uip_sethostaddr(ipaddr);
	uip_ipaddr(ipaddr, 0, 0, 0, 0);
	uip_setnetmask(ipaddr);

//#ifdef _DEBUG
	printf("Waiting for IP address...\n");
//#endif

#endif

	httpd_init();

#ifndef STATIC_IP

	// DHCP request
	dhcpc_init(mac_addr, 6);
	dhcpc_request();

#endif


	long lPeriodicTimer, lARPTimer;
	lPeriodicTimer = lARPTimer = 0;

	//int i; // = MAP_GPIOPinRead(GPIO_PORTA_BASE, ENC_INT) & ENC_INT;
	while(true) {
		//MAP_IntDisable(INT_UART0);
		MAP_SysCtlSleep();
		//MAP_IntEnable(INT_UART0);

		//i = MAP_GPIOPinRead(GPIO_PORTA_BASE, ENC_INT) & ENC_INT;
		/*while(i != 0 && g_ulFlags == 0) {
		i = MAP_GPIOPinRead(GPIO_PORTA_BASE, ENC_INT) & ENC_INT;
		}*/

		// Check the interrupt status for our INT pin, act if it is set.
		if( HWREGBITW(&g_ulFlags, FLAG_ENC_INT) == 1 ) {
			HWREGBITW(&g_ulFlags, FLAG_ENC_INT) = 0;
			chip.Interrupt();
		}

		// Count along with the system tick.
		if( HWREGBITW(&g_ulFlags, FLAG_SYSTICK) == 1) {
			HWREGBITW(&g_ulFlags, FLAG_SYSTICK) = 0;
			lPeriodicTimer += SYSTICKMS;
			lARPTimer += SYSTICKMS;
#ifdef _DEBUG
			printf("%d %d\n", lPeriodicTimer, lARPTimer);
#endif
		}


		if( lPeriodicTimer > UIP_PERIODIC_TIMER_MS ) {
			lPeriodicTimer = 0;


			int l;
			for(l = 0; l < UIP_CONNS; l++) {
				uip_periodic(l);

				//
				// If the above function invocation resulted in data that
				// should be sent out on the network, the global variable
				// uip_len is set to a value > 0.
				//
				if(uip_len > 0) {
					uip_arp_out();
					chip.Send(uip_buf, uip_len);
					uip_len = 0;
				}
			}

			for(l = 0; l < UIP_UDP_CONNS; l++) {
				uip_udp_periodic(l);
				if( uip_len > 0) {
					uip_arp_out();
					chip.Send(uip_buf, uip_len);
					uip_len = 0;
				}
			}
		}

		if( lARPTimer > UIP_ARP_TIMER_MS) {
			lARPTimer = 0;
			uip_arp_timer();
		}

	}

	return 0;
}


void dhcpc_configured(const struct dhcpc_state *s)
{
	uip_sethostaddr(&s->ipaddr);
	uip_setnetmask(&s->netmask);
	uip_setdraddr(&s->default_router);
	printf("IP: %d.%d.%d.%d\n", s->ipaddr[0] & 0xff, s->ipaddr[0] >> 8,
		s->ipaddr[1] & 0xff, s->ipaddr[1] >> 8);
}

#ifdef __cplusplus
extern "C" {
#endif
void SysTickIntHandler(void)
{
	//
	// Increment the system tick count.
	//
	g_ulTickCounter++;

	//
	// Indicate that a SysTick interrupt has occurred.
	//
	HWREGBITW(&g_ulFlags, FLAG_SYSTICK) = 1;
	//g_ulFlags |= FLAG_SYSTICK;
}
#ifdef __cplusplus
}
#endif

// Handle interrupts from the INT pin
#ifdef __cplusplus
extern "C" {
#endif
void GPIOPortEIntHandler(void) {
	uint8_t p = MAP_GPIOPinIntStatus(GPIO_PORTE_BASE, true) & 0xFF;

	MAP_GPIOPinIntClear(GPIO_PORTE_BASE, p);

	HWREGBITW(&g_ulFlags, FLAG_ENC_INT) = 1;
}
#ifdef __cplusplus
}
#endif

// Used in uip_timer.c
clock_time_t clock_time(void)
{
	return((clock_time_t)g_ulTickCounter);
}
