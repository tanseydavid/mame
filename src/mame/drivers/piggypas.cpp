// license:BSD-3-Clause
// copyright-holders:David Haywood
/*

Piggy Pass

hw platform unknown
game details unknown

*/

#include "emu.h"
#include "cpu/mcs51/mcs51.h"
#include "machine/i8255.h"
#include "machine/nvram.h"
#include "machine/ticket.h"
#include "sound/okim6295.h"
#include "video/hd44780.h"
#include "emupal.h"
#include "screen.h"
#include "speaker.h"

#include "piggypas.lh"


class piggypas_state : public driver_device
{
public:
	piggypas_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_hd44780(*this, "hd44780"),
		m_ticket(*this, "ticket"),
		m_digits(*this, "digit%u", 0U)
	{ }

	void piggypas(machine_config &config);
	void fidlstix(machine_config &config);
	DECLARE_INPUT_CHANGED_MEMBER(ball_sensor);
	DECLARE_CUSTOM_INPUT_MEMBER(ticket_r);
private:
	void output_digits();
	virtual void machine_start() override;
	virtual void machine_reset() override;
	DECLARE_WRITE8_MEMBER(ctrl_w);
	DECLARE_WRITE8_MEMBER(mcs51_tx_callback);
	DECLARE_WRITE8_MEMBER(led_strobe_w);
	DECLARE_READ8_MEMBER(lcd_latch_r);
	DECLARE_WRITE8_MEMBER(lcd_latch_w);
	DECLARE_WRITE8_MEMBER(lcd_control_w);
	HD44780_PIXEL_UPDATE(piggypas_pixel_update);

	required_device<mcs51_cpu_device> m_maincpu;
	required_device<hd44780_device> m_hd44780;
	required_device<ticket_dispenser_device> m_ticket;
	output_finder<4> m_digits;
	uint8_t   m_ctrl;
	uint8_t   m_lcd_latch;
	uint32_t  m_digit_latch;
	void piggypas_io(address_map &map);
	void piggypas_map(address_map &map);
	void fidlstix_io(address_map &map);
};


void piggypas_state::output_digits()
{
	// Serial output driver is UCN5833A
	m_digits[0] = bitswap<8>(m_digit_latch & 0xff, 7,6,4,3,2,1,0,5) & 0x7f;
	m_digits[1] = bitswap<8>((m_digit_latch >> 8) & 0xff, 7,6,4,3,2,1,0,5) & 0x7f;
	m_digits[2] = bitswap<8>((m_digit_latch >> 16) & 0xff, 7,6,4,3,2,1,0,5) & 0x7f;
	m_digits[3] = bitswap<8>((m_digit_latch >> 24) & 0xff, 7,6,4,3,2,1,0,5) & 0x7f;
}

WRITE8_MEMBER(piggypas_state::ctrl_w)
{
	if (!BIT(data, 2) && BIT(m_ctrl, 2))
		output_digits();

	m_ticket->motor_w(BIT(data, 6));

	m_ctrl = data;
}

WRITE8_MEMBER(piggypas_state::mcs51_tx_callback)
{
	m_digit_latch = (m_digit_latch >> 8) | (u32(data) << 24);
}

WRITE8_MEMBER(piggypas_state::led_strobe_w)
{
	if (!BIT(data, 0))
		m_digit_latch = 0;
}

READ8_MEMBER(piggypas_state::lcd_latch_r)
{
	return m_lcd_latch;
}

WRITE8_MEMBER(piggypas_state::lcd_latch_w)
{
	// P1.7 might also be used to reset DS1232 watchdog
	m_lcd_latch = data;
}

WRITE8_MEMBER(piggypas_state::lcd_control_w)
{
	// RXD (P3.0) = chip select
	// TXD (P3.1) = register select
	// INT0 (P3.2) = R/W
	if (BIT(data, 0))
	{
		if (BIT(data, 2))
			m_lcd_latch = m_hd44780->read(space, BIT(data, 1));
		else
			m_hd44780->write(space, BIT(data, 1), m_lcd_latch);
	}

	// T0 (P3.4) = output shift clock (serial data present at P1.0)
	// T1 (P3.5) = output latch enable
	if (BIT(data, 4))
		m_digit_latch = (m_digit_latch >> 1) | (BIT(m_lcd_latch, 0) << 31);
	if (BIT(data, 5) && !BIT(data, 4))
		output_digits();
}

void piggypas_state::piggypas_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
}

void piggypas_state::piggypas_io(address_map &map)
{
	map(0x0000, 0x07ff).ram().share("nvram");
	map(0x0800, 0x0803).rw("ppi", FUNC(i8255_device::read), FUNC(i8255_device::write));
	map(0x1000, 0x1000).rw("oki", FUNC(okim6295_device::read), FUNC(okim6295_device::write));
	map(0x1800, 0x1801).w(m_hd44780, FUNC(hd44780_device::write));
	map(0x1802, 0x1803).r(m_hd44780, FUNC(hd44780_device::read));
}

void piggypas_state::fidlstix_io(address_map &map)
{
	map(0x0000, 0x07ff).ram().share("nvram");
	map(0x0800, 0x0803).rw("ppi", FUNC(i8255_device::read), FUNC(i8255_device::write));
	map(0x1000, 0x1000).rw("oki", FUNC(okim6295_device::read), FUNC(okim6295_device::write));
	map(0x1800, 0x1800).nopw(); // input matrix scan?
}


INPUT_CHANGED_MEMBER(piggypas_state::ball_sensor)
{
	m_maincpu->set_input_line(1, newval ? CLEAR_LINE : ASSERT_LINE);
}

CUSTOM_INPUT_MEMBER(piggypas_state::ticket_r)
{
	return m_ticket->line_r();
}

static INPUT_PORTS_START( piggypas )
	PORT_START("IN0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_COIN3)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_COIN4)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_CUSTOM)  PORT_CUSTOM_MEMBER(DEVICE_SELF, piggypas_state, ticket_r, nullptr)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD)  PORT_NAME("Gate sensor")   PORT_CODE(KEYCODE_G)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_COIN1)

	PORT_START("IN1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD)   PORT_NAME("Decrease")   PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD)   PORT_NAME("Exit")       PORT_CODE(KEYCODE_E)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD)   PORT_NAME("Last")       PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD)   PORT_NAME("Run")        PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD)   PORT_NAME("Increase")   PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD)   PORT_NAME("Enter")      PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD)   PORT_NAME("Next")       PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE)  PORT_NAME("Program")

	PORT_START("IN2")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CHANGED_MEMBER(DEVICE_SELF, piggypas_state, ball_sensor, 0) // ball sensor
INPUT_PORTS_END


void piggypas_state::machine_start()
{
	m_digits.resolve();

	m_digit_latch = 0;
}

void piggypas_state::machine_reset()
{
}

HD44780_PIXEL_UPDATE(piggypas_state::piggypas_pixel_update)
{
	if (pos < 8)
		bitmap.pix16(y, (line * 8 + pos) * 6 + x) = state;
}

MACHINE_CONFIG_START(piggypas_state::piggypas)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", I80C31, XTAL(8'448'000)) // OKI M80C31F or M80C154S
	MCFG_DEVICE_PROGRAM_MAP(piggypas_map)
	MCFG_DEVICE_IO_MAP(piggypas_io)
	MCFG_MCS51_PORT_P1_OUT_CB(WRITE8(*this, piggypas_state, led_strobe_w))
	MCFG_MCS51_PORT_P3_IN_CB(IOPORT("IN2"))
	MCFG_MCS51_SERIAL_TX_CB(WRITE8(*this, piggypas_state, mcs51_tx_callback))
//  MCFG_DEVICE_VBLANK_INT_DRIVER("screen", piggypas_state,  irq0_line_hold)

	MCFG_NVRAM_ADD_0FILL("nvram") // DS1220AD

	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_UPDATE_DEVICE("hd44780", hd44780_device, screen_update)
	MCFG_SCREEN_SIZE(16*6, 8)
	MCFG_SCREEN_VISIBLE_AREA(0, 16*6-1, 0, 8-1)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD("palette", 2)
	MCFG_DEFAULT_LAYOUT(layout_piggypas)

	MCFG_HD44780_ADD("hd44780")
	MCFG_HD44780_LCD_SIZE(1, 16)
	MCFG_HD44780_PIXEL_UPDATE_CB(piggypas_state, piggypas_pixel_update)

	/* sound hardware */
	SPEAKER(config, "mono").front_center();

	MCFG_DEVICE_ADD("oki", OKIM6295, XTAL(8'448'000) / 8, okim6295_device::PIN7_HIGH) // clock and pin 7 not verified
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MCFG_DEVICE_ADD("ppi", I8255A, 0) // OKI M82C55A-2
	MCFG_I8255_IN_PORTA_CB(IOPORT("IN1"))
	MCFG_I8255_OUT_PORTB_CB(WRITE8(*this, piggypas_state, ctrl_w))
	MCFG_I8255_IN_PORTC_CB(IOPORT("IN0"))

	MCFG_TICKET_DISPENSER_ADD("ticket", attotime::from_msec(100), TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH)
MACHINE_CONFIG_END

MACHINE_CONFIG_START(piggypas_state::fidlstix)
	piggypas(config);

	MCFG_DEVICE_MODIFY("maincpu")
	MCFG_DEVICE_IO_MAP(fidlstix_io)
	MCFG_MCS51_SERIAL_TX_CB(NOOP)
	MCFG_MCS51_PORT_P1_IN_CB(READ8(*this, piggypas_state, lcd_latch_r))
	MCFG_MCS51_PORT_P1_OUT_CB(WRITE8(*this, piggypas_state, lcd_latch_w))
	MCFG_MCS51_PORT_P3_OUT_CB(WRITE8(*this, piggypas_state, lcd_control_w))
MACHINE_CONFIG_END




ROM_START( piggypas )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pigypass.u6", 0x00000, 0x10000, CRC(c8dc4e26) SHA1(f9643945f84fe2679742922abf5a92b77bf59e4c) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "pigypass.u14", 0x00000, 0x40000, CRC(855504c1) SHA1(dfe91943057fa66798c8395348cf703cb11468d2) )
ROM_END


ROM_START( hoopshot )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hoopshot.u6", 0x00000, 0x08000, CRC(fa8ae8aa) SHA1(2266a775fba7c8f8e3e24441aca6c4b89a6d1ec7) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "hoopshot.u14", 0x00000, 0x40000, CRC(748462b5) SHA1(ccb8f1dbb6471b134c1e97699383c3ef139c42c3) )
ROM_END



ROM_START( rndrndqs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "round.u6", 0x00000, 0x08000, CRC(3eb64b10) SHA1(66051cdd6be33f4f7249be1c8d56e5e43c838163) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "round.u14", 0x00000, 0x40000, CRC(36d1c07a) SHA1(3c978d4d03d8dbf79e1afe7dc46209d9ac4d3cc3) )
ROM_END

ROM_START( fidlstix )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fiddle.u6", 0x00000, 0x08000, CRC(48125bf1) SHA1(5772fe3c0987fc6b2508da5efe3c4c3c179b76a1) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "fiddle.u14", 0x00000, 0x40000, CRC(baf4e1cd) SHA1(ae153f832cbd188e9f3f357a1a1f68cc8264d346) )
ROM_END

// bad dump of program rom
ROM_START( jackbean )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "beanstlk.u6", 0x00000, 0x10000, BAD_DUMP CRC(127c4d6c) SHA1(c864293f42e81a1b8e5dcb12abc1c0019853792e) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "beanstlk.u14", 0x00000, 0x40000, CRC(e33ef0a3) SHA1(337ce3d0c901b0b3241d76601eaad6e3e2724e1a) )
ROM_END

// bad dump of program rom
ROM_START( dumpump )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dump-ump.u6", 0x00000, 0x08000, BAD_DUMP CRC(410fc27e) SHA1(d9505c11f4844b9b58c12b3ff6b860357a4be75e))

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "dump-ump.u14", 0x00000, 0x20000, CRC(08bc7bb5) SHA1(2355783ec614d8f4e1dca3cb415a97a28411157b))
ROM_END

// bad dump of program rom
ROM_START( 3lilpigs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "3-pigs.u6", 0x00000, 0x10000, BAD_DUMP CRC(1db9d754) SHA1(9b1db9c9bb155ebb5509970476b20b9dda6d3021) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "3-pigs.u14", 0x00000, 0x40000, CRC(62eb76e2) SHA1(c4cad241dedf2c290f9bf80038415fe39b3ce17d) )
ROM_END





// COPYRIGHT (c) 1990, 1991, 1992, DOYLE & ASSOC., INC.   VERSION 04.40
GAME( 1992, piggypas,  0,    piggypas, piggypas, piggypas_state,  empty_init, ROT0, "Doyle & Assoc.", "Piggy Pass (version 04.40)", MACHINE_IS_SKELETON_MECHANICAL )
// COPYRIGHT (c) 1990, 1991, 1992, DOYLE & ASSOC., INC.   VERSION 05.22
GAME( 1992, hoopshot,  0,    piggypas, piggypas, piggypas_state,  empty_init, ROT0, "Doyle & Assoc.", "Hoop Shot (version 05.22)", MACHINE_IS_SKELETON_MECHANICAL )
// Quick $ilver   Development Co.    10/08/96      ROUND  REV 6
GAME( 1996, rndrndqs,  0,    fidlstix, piggypas, piggypas_state,  empty_init, ROT0, "Quick $ilver Development Co.", "Round and Round (Rev 6) (Quick $ilver)", MACHINE_IS_SKELETON_MECHANICAL )
//  Quick$ilver   Development Co.    10/02/95      -FIDDLESTIX-       REV 15T
GAME( 1995, fidlstix,  0,    fidlstix, piggypas, piggypas_state,  empty_init, ROT0, "Quick $ilver Development Co.", "Fiddle Stix (1st Rev)", MACHINE_IS_SKELETON_MECHANICAL )
// bad dump, so version unknown
GAME( 199?, jackbean,  0,    piggypas, piggypas, piggypas_state,  empty_init, ROT0, "Doyle & Assoc.", "Jack & The Beanstalk (Doyle & Assoc.?)", MACHINE_IS_SKELETON_MECHANICAL )
// bad dump, so version unknown
GAME( 199?, dumpump,   0,    piggypas, piggypas, piggypas_state,  empty_init, ROT0, "Doyle & Assoc.", "Dump The Ump", MACHINE_IS_SKELETON_MECHANICAL )
// bad dump, so version unknown
GAME( 199?, 3lilpigs,  0,    piggypas, piggypas, piggypas_state,  empty_init, ROT0, "Doyle & Assoc.", "3 Lil' Pigs", MACHINE_IS_SKELETON_MECHANICAL )
