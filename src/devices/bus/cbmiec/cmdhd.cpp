// license:BSD-3-Clause
// copyright-holders:Curt Coder
/**********************************************************************

    CMD HD hard drive emulation

**********************************************************************/

#include "emu.h"
#include "cmdhd.h"
#include "machine/msm6242.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6502_TAG       "m6502"
#define M6522_1_TAG     "m6522_1"
#define M6522_2_TAG     "m6522_2"
#define I8255A_TAG      "i8255a"
#define RTC72421A_TAG   "rtc"
#define SCSIBUS_TAG     "scsi"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(CMD_HD, cmd_hd_device, "cmdhd", "CMD HD")


//-------------------------------------------------
//  ROM( cmd_hd )
//-------------------------------------------------

ROM_START( cmd_hd )
	ROM_REGION( 0x8000, M6502_TAG, 0 )
	ROM_LOAD( "cmd_hd_bootrom_v280.bin", 0x0000, 0x8000, CRC(da68435d) SHA1(defd8bc04a52904b8a3560f11c82126619513a10) )

	ROM_REGION( 0x104, "plds", 0 )
	ROM_LOAD( "pal16l8_1", 0x000, 0x001, NO_DUMP )
	ROM_LOAD( "pal16l8_2", 0x000, 0x001, NO_DUMP )
	ROM_LOAD( "pal16l8_3", 0x000, 0x001, NO_DUMP )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const tiny_rom_entry *cmd_hd_device::device_rom_region() const
{
	return ROM_NAME( cmd_hd );
}


//-------------------------------------------------
//  ADDRESS_MAP( cmd_hd_mem )
//-------------------------------------------------

void cmd_hd_device::cmd_hd_mem(address_map &map)
{
	map(0x0000, 0x7fff).ram();
	map(0x8000, 0xffff).rom().region(M6502_TAG, 0);
	map(0x8000, 0x800f).mirror(0x1f0).w(M6522_1_TAG, FUNC(via6522_device::write));
	map(0x8400, 0x840f).mirror(0x1f0).w(M6522_2_TAG, FUNC(via6522_device::write));
	map(0x8800, 0x8803).mirror(0x1fc).w(I8255A_TAG, FUNC(i8255_device::write));
	map(0x8c00, 0x8c0f).mirror(0x1f0).w(RTC72421A_TAG, FUNC(rtc72421_device::write));
	map(0x8f00, 0x8f00).mirror(0xff).w(FUNC(cmd_hd_device::led_w));
}


//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

MACHINE_CONFIG_START(cmd_hd_device::device_add_mconfig)
	MCFG_DEVICE_ADD(M6502_TAG, M6502, 2000000)
	MCFG_DEVICE_PROGRAM_MAP(cmd_hd_mem)

	MCFG_DEVICE_ADD(M6522_1_TAG, VIA6522, 2000000)
	MCFG_DEVICE_ADD(M6522_2_TAG, VIA6522, 2000000)
	MCFG_DEVICE_ADD(I8255A_TAG, I8255A, 0)
	MCFG_DEVICE_ADD(RTC72421A_TAG, RTC72421, XTAL(32'768))

	MCFG_DEVICE_ADD(SCSIBUS_TAG, SCSI_PORT, 0)
	MCFG_SCSIDEV_ADD(SCSIBUS_TAG ":" SCSI_PORT_DEVICE1, "harddisk", SCSIHD, SCSI_ID_0)
MACHINE_CONFIG_END



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  cmd_hd_device - constructor
//-------------------------------------------------

cmd_hd_device::cmd_hd_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, CMD_HD, tag, owner, clock),
		device_cbm_iec_interface(mconfig, *this),
		m_maincpu(*this, M6502_TAG),
		m_scsibus(*this, SCSIBUS_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cmd_hd_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cmd_hd_device::device_reset()
{
}


//-------------------------------------------------
//  cbm_iec_srq -
//-------------------------------------------------

void cmd_hd_device::cbm_iec_srq(int state)
{
}


//-------------------------------------------------
//  cbm_iec_atn -
//-------------------------------------------------

void cmd_hd_device::cbm_iec_atn(int state)
{
}


//-------------------------------------------------
//  cbm_iec_data -
//-------------------------------------------------

void cmd_hd_device::cbm_iec_data(int state)
{
}


//-------------------------------------------------
//  cbm_iec_reset -
//-------------------------------------------------

void cmd_hd_device::cbm_iec_reset(int state)
{
	if (!state)
	{
		device_reset();
	}
}


//-------------------------------------------------
//  led_w -
//-------------------------------------------------

WRITE8_MEMBER( cmd_hd_device::led_w )
{
	/*

	    bit     description

	    0
	    1
	    2
	    3
	    4
	    5
	    6
	    7

	*/
}
