#include "usb_control.h"
#include "fat.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/msc.h>

#include <string.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

// USB descriptor definitions

#define VENDOR_ID 0x26BA
#define PRODUCT_ID 0x8003
#define MAX_PACKET_SIZE 64

// Buffer provided to the USB implementation for control requests
static uint8_t usbd_control_buffer[128];

static const char * const usb_strings[] = {
    "Stecman",
    "Boot Switch",
    USB_SERIALNUM,
};

const struct usb_device_descriptor dev_descr = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,    //
    .bDeviceSubClass = 0x00, // These are specified in the interface descriptor
    .bDeviceProtocol = 0x00, //
    .bMaxPacketSize0 = MAX_PACKET_SIZE,
    .idVendor = VENDOR_ID,
    .idProduct = PRODUCT_ID,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

const struct usb_endpoint_descriptor endpoint_descriptors[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x01,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = MAX_PACKET_SIZE,
        .bInterval = 0,
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x81,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = MAX_PACKET_SIZE,
        .bInterval = 0,
    },
};

const struct usb_interface_descriptor msc_descriptor = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_MSC,
    .bInterfaceSubClass = USB_MSC_SUBCLASS_SCSI,
    .bInterfaceProtocol = USB_MSC_PROTOCOL_BBB,
    .iInterface = 0,
    .endpoint = endpoint_descriptors,
    .extra = NULL,
    .extralen = 0
};

const struct usb_interface ifaces[] = {
    {
        .num_altsetting = 1,
        .altsetting = &msc_descriptor,
    }
};

const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0xE0, // Bus powered and Support Remote Wake-up
    .bMaxPower = 25, // Max power = 50mA
    .interface = ifaces,
};

// ----------------------------------------------------------------------------

// Filesystem size is (SECTOR_COUNT * SECTOR_SIZE) in bytes
// Note that sector size cannot be changed as usb_msc_init defines it
#define SECTOR_COUNT 128
#define SECTOR_SIZE 512
#define BYTES_PER_SECTOR 512
#define SECTORS_PER_CLUSTER 1
#define RESERVED_SECTORS 1
#define FAT_COPIES 2
#define ROOT_ENTRIES 512
#define ROOT_ENTRY_LENGTH 32
#define FILEDATA_START_CLUSTER 3
#define DATA_REGION_SECTOR (RESERVED_SECTORS + FAT_COPIES + (ROOT_ENTRIES * ROOT_ENTRY_LENGTH) / BYTES_PER_SECTOR)
#define FILEDATA_START_SECTOR (DATA_REGION_SECTOR + (FILEDATA_START_CLUSTER - 2) * SECTORS_PER_CLUSTER)

static usbd_device* _usbd_dev;
static usbd_mass_storage* _msc_device;

struct VirtualFile {
    char* longName;
    struct FatDirEntry dir;
    void (*read) (uint8_t* output);
};

// Fixed contents of the boot sector
const uint8_t BootSector[] = {
    0xEB, 0x3C, 0x90,                        // code to jump to the bootstrap code
    'm', 'k', 'f', 's', '.', 'f', 'a', 't',  // OEM ID
    WBVAL(BYTES_PER_SECTOR),                 // bytes per sector
    SECTORS_PER_CLUSTER,                     // sectors per cluster
    WBVAL(RESERVED_SECTORS),                 // # of reserved sectors (1 boot sector)
    FAT_COPIES,                              // FAT copies (2)
    WBVAL(ROOT_ENTRIES),                     // root entries (512)
    WBVAL(SECTOR_COUNT),                     // total number of sectors
    FAT_MEDIA_FIXED_DISK,                    // media descriptor
    WBVAL(1),                                // sectors per FAT
    WBVAL(32),                               // sectors per track
    WBVAL(64),                               // number of heads
    QBVAL(0),                                // hidden sectors
    QBVAL(0),                                // large number of sectors
    0x00,                                    // drive number
    0x00,                                    // reserved
    0x29,                                    // extended boot signature
    QBVAL(0x55AA6922),                       // volume serial number
    'S', 'W', 'I', 'T', 'C', 'H', ' ', ' ', ' ', ' ', ' ',  // volume label
    'F', 'A', 'T', '1', '2', ' ', ' ', ' '   // filesystem type
};

// File Allocation Table is a fixed value as there are no files larger than one cluster
// It might be an interesting exercise to generate this on the fly, but it's not required
const uint8_t FatSector[] = {
    0xf8, 0xff, 0xff, 0x00, 0xf0, 0xff, 0xff, 0x0f
};


/**
 * Callback for switch_position file contents
 */
static void readSwitch(uint8_t* output)
{
    output[0] = gpio_get(GPIOA, GPIO6) ? '1' : '0';
}

/**
 * Callback for switch_position_grub.vfg file contents
 */
static char grubConfigStr[] = "set os_hw_switch=0\n";
static void readGrubConfig(uint8_t* output)
{
    // Modify config with current switch value
    grubConfigStr[sizeof(grubConfigStr)-3] = gpio_get(GPIOA, GPIO6) ? '1' : '0';

    memcpy(output, &grubConfigStr, sizeof(grubConfigStr));
}

// "Files" to serve in the root directory
static struct VirtualFile _virtualFiles[] = {
    {
        .longName = "switch_position",
        .dir = {
            .name = "SWITCH~1",
            .ext = "   ",
            .size = 1
        },
        .read = readSwitch
    },
    {
        .longName = "switch_position_grub.cfg",
        .dir = {
            .name = "SWITCH~1",
            .ext = "CFG",
            .size = sizeof(grubConfigStr) - 1,
        },
        .read = readGrubConfig
    },
};


static void generate_dir_sector(uint8_t* output)
{
    for (uint8_t i = 0; i < COUNT_OF(_virtualFiles); ++i)
    {
        const char* longName = _virtualFiles[i].longName;
        struct FatDirEntry* entry = &_virtualFiles[i].dir;

        // Set sector automatically based on array index
        entry->start = FILEDATA_START_CLUSTER + i;

        // Write long file name and actual directory entry
        output += fat_write_lfn(longName, entry, output);
        output += fat_write_dir(entry, output);
    }
}

static int read_block(uint32_t lba, uint8_t *copy_to)
{
    memset(copy_to, 0, SECTOR_SIZE);

    switch (lba) {
        case 0: // sector 0 is the boot sector
            memcpy(copy_to, BootSector, sizeof(BootSector));

            // Add bootable partition signature
            copy_to[SECTOR_SIZE - 2] = 0x55;
            copy_to[SECTOR_SIZE - 1] = 0xAA;
            break;

        // FAT first and second copy
        case 1:
        case 2:
            memcpy(copy_to, FatSector, sizeof(FatSector));
            break;

        // Root directory entry
        case 3:
            generate_dir_sector(copy_to);
            break;

        default: {
            const uint16_t fileIndex = (lba - FILEDATA_START_SECTOR) / SECTORS_PER_CLUSTER;

            if (lba < FILEDATA_START_SECTOR || fileIndex >= COUNT_OF(_virtualFiles)) {
                // Out of virtual file range
                break;
            }

            // Read from virtual file
            _virtualFiles[fileIndex].read(copy_to);

            break;
        }
    }

    return 0;
}

static int write_block(uint32_t lba, const uint8_t *copy_from)
{
    (void)lba;
    (void)copy_from;
    // ignore writes
    return 0;
}


void usb_device_init(void)
{
    // Set up USB peripheral
    nvic_enable_irq(NVIC_USB_IRQ);

     _usbd_dev = usbd_init(
        &st_usbfs_v2_usb_driver,
        &dev_descr,
        &config,
        usb_strings,
        COUNT_OF(usb_strings),
        usbd_control_buffer,
        sizeof(usbd_control_buffer)
    );

    _msc_device = usb_msc_init(
        _usbd_dev,
        0x81, MAX_PACKET_SIZE, // In EP
        0x01, MAX_PACKET_SIZE, // Out EP
        "STECMAN", // SCSI vendor ID
        "SWITCH THING", // SCSI product ID
        "V1", // SCSI product revision level
        SECTOR_COUNT,
        read_block,
        write_block
    );
}

void usb_isr(void)
{
    // Handle USB requests as they come through
    usbd_poll(_usbd_dev);
}
