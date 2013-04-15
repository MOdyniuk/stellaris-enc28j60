/**
 * @file enc28j60.h
 *
 * Prototype for ENC28J60 controller class. For use with the Stellaris
 * Launchpad, Stellaris-Pins library and a compatible ENC28J60 board.
 * 
 * It is possible to use this library without the Stellaris-Pins library, but
 * you get to write out the additional pin information yourself.
 */

#ifndef ENC28J60_STELLARIS_CHAPMAN_H_
#define ENC28J60_STELLARIS_CHAPMAN_H_

#include <stdint.h>
#include "enc28j60reg.h"

extern "C" void UARTprintf(const char *pcString, ...);
#define printf UARTprintf

namespace ENCJ_STELLARIS
{
	typedef enum {
		Reset
	} PinType;

	typedef enum {
		Low = 0,
		High = 1
	} PinValue;

	class ENC28J60;

	class BusDriver
	{
	public:
		static void Init(void *driverId);
		static void ChipSelect(void *driverId);
		static void ChipDeSelect(void *driverId);
		static uint8_t SpiSend(void *driverId, uint8_t msg);
		static void PinSet(void *driverId, PinType pin, PinValue value);
		static void Delay(uint32_t ms);
		static void OnReceive(ENC28J60 *driver, uint16_t data_count);
	};

	/**
	 * ENC28J60 Class
	 */
	class ENC28J60
	{
	public:
		/**
		 * Primary constructor, all arguments are optional except the MAC
		 * address.
		 *
		 * Blank arguments default to XPG's boosterpack pinout.
		 */
		ENC28J60();

		void Init(const uint8_t *mac);
		void Receive(void);
		bool Send(const uint8_t *buf, uint16_t count);
		void Reset(void);
		void Interrupt(void);
		void RBM(uint8_t *buf, uint16_t len);	// Read Buffer Memory

	private:
		/** Private Methods **/

		/* Setup */
		void InitConfig(const uint8_t *mac);

		uint8_t SPISend(uint8_t msg);

		/* Low level register controls */

		uint8_t RCR(uint8_t reg);				// Read Control Register
		uint8_t RCRM(uint8_t reg);				// Read Control Register MAC/MII
		void WCR(uint8_t reg, uint8_t val);		// Write Control Register
		void WBM(const uint8_t *buf, uint16_t len);	// Write Buffer Memory

		void BFS(uint8_t reg, uint8_t mask);	// Bit Field Set
		void BFC(uint8_t reg, uint8_t mask);	// Bit Field Clear

		/* High level register controls */

		void SwitchBank(uint8_t new_bank);	// Switch active memory bank

		uint8_t ReadRegister(uint8_t reg, uint8_t bank);
		uint8_t ReadMIIRegister(uint8_t reg, uint8_t bank);
		void WriteRegister(uint8_t reg, uint8_t bank, uint8_t value);

		// Batch bit field set controls
		void BitsFieldSet(uint8_t reg, uint8_t bank, uint8_t mask);
		void BitsFieldClear(uint8_t reg, uint8_t bank, uint8_t mask);

		// PHY memory access
		uint16_t ReadPHY(uint8_t address);
		void WritePHY(uint8_t address, uint16_t value);

		// Mass memory operations
		void SetRXMemoryArea(uint16_t startAddr, uint16_t endAddr);
		void SetMACAddress(const uint8_t *macAddr);
		void GetMACAddress(uint8_t *macAddr);

	private:
		/* Private fields */
		uint8_t		activeBank;	// Current memory bank
		uint16_t	nextPacket;	// Next data packet ptr (to internal ENC28J60 memory)
	};


} // Namespace ENCJ_STELLARIS

#endif // ENC28J60_STELLARIS_CHAPMAN_H_
