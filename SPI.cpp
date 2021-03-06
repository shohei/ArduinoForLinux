#include "SPI.h"
#include <iostream>
#include <sys/types.h> // open
#include <sys/stat.h> // open
#include <fcntl.h>
#include <sstream> // std::ostringstream
#include <cstring> // strerror
#include <errno.h>
#include <assert.h>
#include <unistd.h> // close
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

namespace Arduino
{

Spi SPI;

Spi::Spi():
theFd(-1),
theSpiBus(0),
theCurrentSettings(1000,MSBFIRST,SPI_MODE0)
{

}

Spi::~Spi()
{
   if (theFd > 0)
   {
      std::cout << "Closing the SPI device" << std::endl;
   }
}

void Spi::begin()
{
   std::cout << "NOP: " << __PRETTY_FUNCTION__ << std::endl;
}

void Spi::end()
{
   std::cout << "NOP: " << __PRETTY_FUNCTION__ << std::endl;
}

void Spi::beginTransaction(SPISettings const & settings)
{
   std::cerr << "Not implemented" << std::endl;
   if (theFd == -1)
   {
      bool openSuccess = openSpiBus(theSpiBus, theChipSelect);
      if (!openSuccess)
      {
         std::cerr << "Error starting transaction, spidev failed to open" << std::endl;
         return;
      }

      setBitOrder(settings.theOrder);

      setDataMode(settings.theMode);
   }
   else
   {
      // SPI device already open, are settings still consistent...
      if (settings.theMode != theCurrentSettings.theMode)
      {
         setDataMode(settings.theMode);
      }

      if (settings.theOrder != theCurrentSettings.theOrder)
      {
         setBitOrder(settings.theOrder);
      }
   }

   theCurrentSettings = settings;
}

void Spi::endTransaction()
{
   std::cout << "NOP: " << __PRETTY_FUNCTION__ << std::endl;
}

void Spi::setBitOrder(SpiBitOrdering order)
{
   if (theFd == -1)
   {
      std::cerr << "Setting the bit ordering only does anything during a transaction" << std::endl;
      return;
   }

   uint8_t bo = (order == LSBFIRST ? SPI_LSB_FIRST : 0);

   if (ioctl(theFd, SPI_IOC_WR_LSB_FIRST, &bo) == -1)
   {
      std::cerr << "Error setting the bit ordering of write mode: " << strerror(errno) << std::endl;
      return;
   }

   std::cout << "Set the bit ordering to " << (order == LSBFIRST ? "LSB First" : "MSB First") << std::endl;

}

void Spi::setClockDivider(uint8_t divider)
{
   std::cerr << "Not implemented" << std::endl;
}

void Spi::setDataMode(SpiDataMode mode, int slaveSelectPin)
{
   if (theFd == -1)
   {
      std::cerr << "Setting the data mode only does anything during a transaction" << std::endl;
      return;
   }

   uint8_t modeVar;
   switch(mode)
   {
      case SPI_MODE0:
         modeVar = SPI_MODE_0;
         break;

      case SPI_MODE1:
         modeVar = SPI_MODE_1;
         break;

      case SPI_MODE2:
         modeVar = SPI_MODE_2;
         break;

      case SPI_MODE3:
         modeVar = SPI_MODE_3;
         break;

      default:
         std::cerr << "SPI data mode of " << (int) mode << " is invalid" << std::endl;
         return;
   }

   if (ioctl(theFd, SPI_IOC_WR_MODE, &modeVar) == -1)
   {
      std::cerr << "Error setting the data mode of write mode: " << strerror(errno) << std::endl;
      return;
   }

   std::cout << "Set the data mode to SPI_MODE" << (int) mode << std::endl;

}

uint8_t Spi::transfer(uint8_t byteOfData)
{
   if (theFd == -1)
   {
      std::cerr << "Bus not initialized" << std::endl;
      return 0;
   }

   uint8_t retData;

   struct spi_ioc_transfer iData;
   memset(&iData, 0, sizeof(iData));
   iData.tx_buf = (__u64) &byteOfData;
   iData.rx_buf = (__u64) &retData;
   iData.len = sizeof(retData);
   iData.speed_hz = theCurrentSettings.theSpeed;

   if (ioctl(theFd, SPI_IOC_MESSAGE(1), &iData) == -1)
   {
      std::cerr << "Encountered error sending 1 byte of SPI data: " << strerror(errno) << std::endl;
   }

   std::cout << "Returned val = " << (int) retData << std::endl;
   return retData;
}

void Spi::transfer(char* buf, int size)
{
   if (theFd == -1)
   {
      std::cerr << "Bus not initialized" << std::endl;
      return;
   }

   struct spi_ioc_transfer iData;
   memset(&iData, 0, sizeof(iData));
   iData.tx_buf = (__u64) buf;
   iData.rx_buf = (__u64) buf;
   iData.len = size;
   iData.speed_hz = theCurrentSettings.theSpeed;

   if (ioctl(theFd, SPI_IOC_MESSAGE(1), &iData) == -1)
   {
      std::cerr << "Encountered error sending " << size << " bytes of SPI data: " << strerror(errno) << std::endl;
   }
   else
   {
      std::cout << "Sent " << size << " bytes of data over SPI bus" << std::endl;
   }
}

uint16_t Spi::transfer(uint16_t wordOfData)
{
   if (theFd == -1)
   {
      std::cerr << "Bus not initialized" << std::endl;
      return 0;
   }

   uint16_t retData;

   struct spi_ioc_transfer iData;
   memset(&iData, 0, sizeof(iData));
   iData.tx_buf = (__u64) &wordOfData;
   iData.rx_buf = (__u64) &retData;
   iData.len = sizeof(retData);
   iData.speed_hz = theCurrentSettings.theSpeed;

   if (ioctl(theFd, SPI_IOC_MESSAGE(1), &iData) == -1)
   {
      std::cerr << "Encountered error sending 2 bytes of SPI data: " << strerror(errno) << std::endl;
   }

   std::cout << "Returned val = " << (int) retData << std::endl;
   return retData;
}

void Spi::usingInterrupt(int intNumber)
{
   std::cerr << "Not implemented" << std::endl;
}

void Spi::setSpiBus(int bus)
{
   if ( (theSpiBus != bus) && (theFd > 0) )
   {
      // We need to reopen the SPI bus device file
      close(theFd);
      theFd = -1;
   }
}

void Spi::setChipSelect(int cs)
{
   assert((cs == 1) || (cs == 0));

   if ( (theFd != -1) && (cs != theChipSelect) )
   {
      // Need to reopen the file
      close(theFd);
      theFd = -1;

      if (openSpiBus(theSpiBus, cs))
      {
         std::cout << "SPI chip select value = " << cs << std::endl;
         theChipSelect = cs;
      }
      else
      {
         std::cout << "Failed to set the SPI chip select value to " << cs << " for bus "
                   << theSpiBus << std::endl;
      }
   }
}

bool Spi::openSpiBus(int bus, int cs)
{
   assert(theFd == -1);

   std::ostringstream oss;
   oss << "/dev/spidev" << bus << "." << cs;

   theFd = open(oss.str().c_str(), O_RDWR);
   if (theFd == -1)
   {
      std::cerr << "Error opening SPI interface: " << oss.str() << ": " << strerror(errno) << std::endl;
      return false;
   }
   else
   {
      std::cout << "Opening SPI interface: " << oss.str() << std::endl;
      return true;
   }
}

}
