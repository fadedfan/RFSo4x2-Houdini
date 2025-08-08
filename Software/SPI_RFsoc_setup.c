#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>


/*
	LMK04228 devices are programmed using 24-bit registers. Each register consists of a 1-bit command field
	(R/W), a 2-bit multi-byte field (W1, W0), a 13-bit address field (A12 to A0) and a 8-bit data field (D7 to D0). The
	contents of each register is clocked in MSB first (R/W), and the LSB (D0) last. During programming, the CS*
	signal is held low. The serial data is clocked in on the rising edge of the SCK signal. After the LSB is clocked in,
	the CS* signal goes high to latch the contents into the shift register. It is recommended to program registers in
	numeric order -- for example, 0x000 to 0x1FFF -- to achieve proper device operation. Each register consists of
	one or more fields which control the device functionality. R/W bit = 0 is for SPI write. R/W bit = 1 is for SPI read.
	W1 and W0 shall be written as 0.
   
    The LMX2594 is programmed using 24-bit shift registers. The shift register consists of a R/W bit (MSB), followed
	by a 7-bit address field and a 16-bit data field. For the R/W bit, 0 is for write, and 1 is for read. The address field
	ADDRESS[6:0] is used to decode the internal register address. The remaining 16 bits form the data field
	DATA[15:0]. While CSB is low, serial data is clocked into the shift register upon the rising edge of clock (data is
	programmed MSB first). When CSB goes high, data is transferred from the data field into the selected register
	bank.
*/


#define ARRAY_SIZE(array) sizeof(array)/sizeof(array[0])

// SPI transfer settings
static const char *spi_device_lmk = "/dev/spidev0.0";
static const char *spi_device_lmx_1 = "/dev/spidev0.1";
static const char *spi_device_lmx_2 = "/dev/spidev0.2";
static uint8_t bits = 8;
static uint32_t speed = 1000000;
static uint32_t mode = SPI_MODE_0;
static uint16_t delay = 0;

// Register Data Entries (from Datasheet and TICS Pro)
uint16_t addr_array[] = {
    0x0000, 0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x000C,
    0x000D, 0x0100, 0x0101, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106,
    0x0107, 0x0108, 0x0109, 0x010A, 0x010B, 0x010C, 0x010D, 0x010E,
    0x010F, 0x0110, 0x0111, 0x0112, 0x0113, 0x0114, 0x0115, 0x0116,
    0x0117, 0x0118, 0x0119, 0x011A, 0x011B, 0x011C, 0x011D, 0x011E,
    0x011F, 0x0120, 0x0121, 0x0122, 0x0123, 0x0124, 0x0125, 0x0126,
    0x0127, 0x0128, 0x0129, 0x012A, 0x012B, 0x012C, 0x012D, 0x012E,
    0x012F, 0x0130, 0x0131, 0x0132, 0x0133, 0x0134, 0x0135, 0x0136,
    0x0137, 0x0138, 0x0139, 0x013A, 0x013B, 0x013C, 0x013D, 0x013E,
    0x013F, 0x0140, 0x0141, 0x0142, 0x0143, 0x0144, 0x0145, 0x0146,
    0x0147, 0x0148, 0x0149, 0x014A, 0x014B, 0x014C, 0x014D, 0x014E,
    0x014F, 0x0150, 0x0151, 0x0152, 0x0153, 0x0154, 0x0155, 0x0156,
    0x0157, 0x0158, 0x0159, 0x015A, 0x015B, 0x015C, 0x015D, 0x015E,
    0x015F, 0x0160, 0x0161, 0x0162, 0x0163, 0x0164, 0x0165, 0x0171,
    0x0172, 0x017C, 0x017D, 0x0166, 0x0167, 0x0168, 0x0169, 0x016A,
    0x016B, 0x016C, 0x016D, 0x016E, 0x0173, 0x0182, 0x0183, 0x0184,
    0x0185, 0x0188, 0x0189, 0x018A, 0x018B, 0x1FFD, 0x1FFE, 0x1FFF
}; //register 0x16e to 3b if want four wire mode

uint8_t data_array[] = {
    0x90, 0x10, 0x00, 0x06, 0xD0, 0x5B, 0x00, 0x51,
    0x04, 0x6A, 0x55, 0x55, 0x01, 0x22, 0x00, 0x73,
    0x03, 0x6A, 0x55, 0x55, 0x00, 0x22, 0x00, 0xF0,
    0x30, 0x6A, 0x55, 0x55, 0x01, 0x22, 0x00, 0x73,
    0x03, 0x6A, 0x55, 0x55, 0x01, 0x22, 0x00, 0x72,
    0x03, 0x74, 0x55, 0x55, 0x01, 0x22, 0x00, 0x70,
    0x33, 0x6A, 0x55, 0x55, 0x00, 0x22, 0x00, 0xF0,
    0x30, 0x6A, 0x55, 0x55, 0x01, 0x22, 0x00, 0x73,
    0x03, 0x00, 0x03, 0x01, 0x40, 0x00, 0x01, 0x03,
    0x02, 0x09, 0x00, 0x00, 0x31, 0xFF, 0x7F, 0x18,
    0x1A, 0x06, 0x46, 0x06, 0x06, 0x00, 0x00, 0xC0,
    0x7F, 0x13, 0x02, 0x00, 0x00, 0x7D, 0x00, 0x7D,
    0x03, 0xC0, 0x07, 0xD0, 0xDA, 0x20, 0x00, 0x00,
    0x0B, 0x00, 0x19, 0x44, 0x00, 0x00, 0xA0, 0xAA,
    0x02, 0x15, 0x33, 0x00, 0x00, 0xC0, 0x59, 0x20,
    0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53
};

uint32_t lmx_array[] = {
	0x700000, 0x6F0000, 0x6E0000, 0x6D0000, 0x6C0000, 0x6B0000, 0x6A0000, 0x690021,
	0x680000, 0x670000, 0x663F80, 0x650011, 0x640000, 0x630000, 0x620200, 0x610888,
	0x600000, 0x5F0000, 0x5E0000, 0x5D0000, 0x5C0000, 0x5B0000, 0x5A0000, 0x590000,
	0x580000, 0x570000, 0x560000, 0x55D300, 0x540001, 0x530000, 0x521E00, 0x510000,
	0x506666, 0x4F0026, 0x4E00E5, 0x4D0000, 0x4C000C, 0x4B0940, 0x4A0000, 0x49003F,
	0x480001, 0x470081, 0x46C350, 0x450000, 0x4403E8, 0x430000, 0x4201F4, 0x410000,
	0x401388, 0x3F0000, 0x3E0322, 0x3D00A8, 0x3C0000, 0x3B0001, 0x3A8001, 0x390020,
	0x380000, 0x370000, 0x360000, 0x350000, 0x340820, 0x330080, 0x320000, 0x314180,
	0x300300, 0x2F0300, 0x2E07FC, 0x2DC0DF, 0x2C1F20, 0x2B0000, 0x2A0000, 0x290000,
	0x280000, 0x270001, 0x260000, 0x250104, 0x240140, 0x230004, 0x220000, 0x211E21,
	0x200393, 0x1F43EC, 0x1E318C, 0x1D318C, 0x1C0488, 0x1B0002, 0x1A0DB0, 0x190624,
	0x18071A, 0x17007C, 0x160001, 0x150401, 0x14C848, 0x1327B7, 0x120064, 0x110117,
	0x100080, 0x0F064F, 0x0E1E40, 0x0D4000, 0x0C5001, 0x0B00A8, 0x0A10D8, 0x090604,
	0x082000, 0x0740B2, 0x06C802, 0x0500C8, 0x040C43, 0x030642, 0x020500, 0x010809,
	0x00241C
};

size_t num_lmk = ARRAY_SIZE(addr_array);
size_t num_lmx = ARRAY_SIZE(lmx_array);

// Helper Function for SPI channel configuration
int configure_spi_device(const char *spi_device) {
    int spi = open(spi_device, O_RDWR);
    if (spi < 0) {
        printf("Can't open SPI device: %s\n", spi_device);
        return -1;
    }

    if (ioctl(spi, SPI_IOC_WR_MODE32, &mode) == -1) {
        printf("Can't set SPI mode\n");
        close(spi);
        return -1;
    }

    if (ioctl(spi, SPI_IOC_RD_MODE32, &mode) == -1) {
        printf("Can't get SPI mode\n");
        close(spi);
        return -1;
    }

    if (ioctl(spi, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        printf("Can't set bits per word\n");
        close(spi);
        return -1;
    }

    if (ioctl(spi, SPI_IOC_RD_BITS_PER_WORD, &bits) == -1) {
        printf("Can't get bits per word\n");
        close(spi);
        return -1;
    }

    if (ioctl(spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        printf("Can't set max speed (Hz)\n");
        close(spi);
        return -1;
    }

    if (ioctl(spi, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
        printf("Can't get max speed (Hz)\n");
        close(spi);
        return -1;
    }
    return spi;
}

// Helper Function for SPI write LMK04828
void spi_write_lmk(int spi, uint16_t reg_addr, uint8_t data) {
    uint8_t tx[3];
    uint8_t rx[3];
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = delay,
        .speed_hz = 0,
        .bits_per_word = 0,
    };

    // SPI Write: R/W=0, W1/W0=00, then 13-bit address + 8-bit data
    tx[0] = (uint8_t)((reg_addr >> 8) & 0x1F);
    tx[1] = (uint8_t)(reg_addr & 0xFF);
    tx[2] = (uint8_t) data;

    if (ioctl(spi, SPI_IOC_MESSAGE(1), &tr) < 1) {
        printf("SPI write failed");
    }
}

// Helper Function for SPI write LMX2594
void spi_write_lmx(int spi, uint32_t data) {
    uint8_t tx[3];
    uint8_t rx[3];
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = delay,
        .speed_hz = 0,
        .bits_per_word = 0,
    };

    tx[0] = (uint8_t)((data >> 16) & 0xFF);
    tx[1] = (uint8_t)((data >> 8) & 0xFF);
    tx[2] = (uint8_t) (data & 0xFF);

    if (ioctl(spi, SPI_IOC_MESSAGE(1), &tr) < 1) {
        printf("SPI write failed");
    }
}

// Helper Function for SPI read
uint8_t spi_read_lmk(int spi, uint16_t reg_addr) {
    uint8_t tx[3];
    uint8_t rx[3];
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = delay,
        .speed_hz = 0,
        .bits_per_word = 0,
    };

    // Send read command (R/W = 1)
    tx[0] = 0x80 | ((reg_addr >> 8) & 0x1F); // R/W=1
    tx[1] = reg_addr & 0xFF;
    tx[2] = 0x00;                            // Dummy

    memset(rx, 0, sizeof(rx));
    if (ioctl(spi, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("Failed to send SPI read command");
        return 0xFF;
    }
    return rx[2];  // The data byte from the register
}


// Helper function for setting GPIO
void gpio_init(){
	int valuefd, exportfd, directionfd;
	printf("GPIO setting...\n");
	exportfd = open("/sys/class/gpio/export", O_WRONLY);

	write(exportfd, "542", 4);
	write(exportfd, "543", 4);
	write(exportfd, "547", 4);
	close(exportfd);

	printf("GPIO exported successfully\n");

	// Update the direction of the GPIO to be an output
	directionfd = open("/sys/class/gpio/gpio542/direction", O_RDWR);
	write(directionfd, "out", 4);
	close(directionfd);
	directionfd = open("/sys/class/gpio/gpio543/direction", O_RDWR);
	write(directionfd, "out", 4);
	close(directionfd);
	directionfd = open("/sys/class/gpio/gpio547/direction", O_RDWR);
	write(directionfd, "out", 4);
	close(directionfd);

	printf("GPIO direction set as output successfully\n");

	// Get the GPIO value ready to be toggled
	valuefd = open("/sys/class/gpio/gpio542/value", O_RDWR);
	write(valuefd, "1", 2);
	write(valuefd, "0", 2);
	close(valuefd);
	valuefd = open("/sys/class/gpio/gpio543/value", O_RDWR);
	write(valuefd, "0", 2);
	close(valuefd);
	valuefd = open("/sys/class/gpio/gpio547/value", O_RDWR);
	write(valuefd, "0", 2);
	close(valuefd);

	printf("GPIO setting done!\n");
}

int main(){
    int spi;

    // Config GPIO
    gpio_init();


    // Configure SPI
    int spi_lmk = configure_spi_device(spi_device_lmk);
    if (spi_lmk < 0) {
        printf("Failed to initialize LMK04828 device\n");
        return 1;
    }

    int spi_lmx_1 = configure_spi_device(spi_device_lmx_1);
	if (spi_lmx_1 < 0) {
		printf("Failed to initialize LMX2594 device\n");
		return 1;
	}

	int spi_lmx_2 = configure_spi_device(spi_device_lmx_2);
	if (spi_lmx_2 < 0) {
		printf("Failed to initialize LMX2594 device\n");
		return 1;
	}

    printf("Config finished!\n\r");
    usleep(100000);

    //Program Reg for LMK04828
    for (size_t i = 0; i < num_lmk; i++) {
        spi_write_lmk(spi_lmk, addr_array[i], data_array[i]);
    }
    printf("LMK04828 is ready to be used! \n\r");
    usleep(1000);

    //Program Reg for 2 LMX2594
    for (size_t j = 0; j < num_lmx; j++) {
        spi_write_lmx(spi_lmx_1, lmx_array[j]);
    }
    usleep(1000);
    for (size_t k = 0; k < num_lmx; k++) {
		spi_write_lmx(spi_lmx_2, lmx_array[k]);
    }
    printf("LMX2594 is ready to be used! \n\r");
    usleep(100000);


    //Read Deice type ID when 4 wire mode enabled (LMK04828)
//    while(1){
//        uint8_t value = spi_read_lmk(spi, 0x003);
//        printf("Device type ID is %u.\n\r", value);
//        usleep(1000000);
//    }
}
