// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

    Parodius (Konami GX955) (c) 1990 Konami

    driver by Nicola Salmoria

***************************************************************************/

#include "emu.h"
#include "includes/parodius.h"
#include "includes/konamipt.h"

#include "cpu/m6809/konami.h" /* for the callback and the firq irq definition */
#include "cpu/z80/z80.h"
#include "machine/watchdog.h"
#include "sound/ym2151.h"
#include "sound/k053260.h"
#include "emupal.h"
#include "speaker.h"


INTERRUPT_GEN_MEMBER(parodius_state::parodius_interrupt)
{
	if (m_k052109->is_irq_enabled())
		device.execute().set_input_line(0, HOLD_LINE);
}

WRITE8_MEMBER(parodius_state::parodius_videobank_w)
{
	if (data & 0xf8)
		logerror("%04x: videobank = %02x\n",m_maincpu->pc(),data);

	/* bit 0 = select palette or work RAM at 0000-07ff */
	/* bit 1 = select 052109 or 053245 at 2000-27ff */
	/* bit 2 = select palette bank 0 or 1 */

	if (data & 1)
		m_bank0000->set_bank(2 + ((data & 4) >> 2));
	else
		m_bank0000->set_bank(0);

	m_bank2000->set_bank((data & 2) >> 1);
}

WRITE8_MEMBER(parodius_state::parodius_3fc0_w)
{
	if ((data & 0xf4) != 0x10)
		logerror("%04x: 3fc0 = %02x\n",m_maincpu->pc(),data);

	/* bit 0/1 = coin counters */
	machine().bookkeeping().coin_counter_w(0, data & 0x01);
	machine().bookkeeping().coin_counter_w(1, data & 0x02);

	/* bit 3 = enable char ROM reading through the video RAM */
	m_k052109->set_rmrd_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);

	/* other bits unknown */
}

WRITE8_MEMBER(parodius_state::parodius_sh_irqtrigger_w)
{
	m_audiocpu->set_input_line_and_vector(0, HOLD_LINE, 0xff);
}

#if 0
void parodius_state::sound_nmi_callback( int param )
{
	m_audiocpu->set_input_line(INPUT_LINE_NMI, ( m_nmi_enabled ) ? CLEAR_LINE : ASSERT_LINE );

	nmi_enabled = 0;
}
#endif

void parodius_state::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_NMI:
		m_audiocpu->set_input_line(INPUT_LINE_NMI, ASSERT_LINE);
		break;
	default:
		assert_always(false, "Unknown id in parodius_state::device_timer");
	}
}

WRITE8_MEMBER(parodius_state::sound_arm_nmi_w)
{
	m_audiocpu->set_input_line(INPUT_LINE_NMI, CLEAR_LINE);
	timer_set(attotime::from_usec(50), TIMER_NMI);  /* kludge until the K053260 is emulated correctly */
}

/********************************************/

void parodius_state::parodius_map(address_map &map)
{
	map(0x0000, 0x07ff).m(m_bank0000, FUNC(address_map_bank_device::amap8));
	map(0x0800, 0x1fff).ram();
	map(0x2000, 0x5fff).rw(m_k052109, FUNC(k052109_device::read), FUNC(k052109_device::write));
	map(0x2000, 0x27ff).m(m_bank2000, FUNC(address_map_bank_device::amap8));
	map(0x3f8c, 0x3f8c).portr("P1");
	map(0x3f8d, 0x3f8d).portr("P2");
	map(0x3f8e, 0x3f8e).portr("DSW3");
	map(0x3f8f, 0x3f8f).portr("DSW1");
	map(0x3f90, 0x3f90).portr("DSW2");
	map(0x3fa0, 0x3faf).rw(m_k053245, FUNC(k05324x_device::k053244_r), FUNC(k05324x_device::k053244_w));
	map(0x3fb0, 0x3fbf).w(m_k053251, FUNC(k053251_device::write));
	map(0x3fc0, 0x3fc0).r("watchdog", FUNC(watchdog_timer_device::reset_r)).w(FUNC(parodius_state::parodius_3fc0_w));
	map(0x3fc4, 0x3fc4).w(FUNC(parodius_state::parodius_videobank_w));
	map(0x3fc8, 0x3fc8).w(FUNC(parodius_state::parodius_sh_irqtrigger_w));
	map(0x3fcc, 0x3fcd).rw("k053260", FUNC(k053260_device::main_read), FUNC(k053260_device::main_write));
	map(0x6000, 0x9fff).bankr("bank1");            /* banked ROM */
	map(0xa000, 0xffff).rom().region("maincpu", 0x3a000);
}

void parodius_state::bank0000_map(address_map &map)
{
	map(0x0000, 0x07ff).ram();
	map(0x1000, 0x1fff).ram().w("palette", FUNC(palette_device::write8)).share("palette");
}

void parodius_state::bank2000_map(address_map &map)
{
	map(0x0000, 0x07ff).rw(m_k052109, FUNC(k052109_device::read), FUNC(k052109_device::write));
	map(0x0800, 0x0fff).rw(m_k053245, FUNC(k05324x_device::k053245_r), FUNC(k05324x_device::k053245_w));
}

void parodius_state::parodius_sound_map(address_map &map)
{
	map(0x0000, 0xefff).rom();
	map(0xf000, 0xf7ff).ram();
	map(0xf800, 0xf801).rw("ymsnd", FUNC(ym2151_device::read), FUNC(ym2151_device::write));
	map(0xfa00, 0xfa00).w(FUNC(parodius_state::sound_arm_nmi_w));
	map(0xfc00, 0xfc2f).rw("k053260", FUNC(k053260_device::read), FUNC(k053260_device::write));
}

/***************************************************************************

    Input Ports

***************************************************************************/

static INPUT_PORTS_START( parodius )
	PORT_START("P1")
	KONAMI8_ALT_B123(1)                     // button1 = power-up, button2 = shoot, button3 = missile

	PORT_START("P2")
	KONAMI8_ALT_B123(2)

	PORT_START("DSW1")
	KONAMI_COINAGE_LOC(DEF_STR( Free_Play ), "No Coin B", SW1)
	/* "No Coin B" = coins produce sound, but no effect on coin counter */

	PORT_START("DSW2")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )        PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )      PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )   PORT_DIPLOCATION("SW2:4,5")
	PORT_DIPSETTING(    0x18, "20000 80000" )
	PORT_DIPSETTING(    0x10, "30000 100000" )
	PORT_DIPSETTING(    0x08, "20000" )
	PORT_DIPSETTING(    0x00, "70000" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )   PORT_DIPLOCATION("SW2:6,7")
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Difficult ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Difficult ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )  PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Upright Controls" )      PORT_DIPLOCATION("SW3:2")
	PORT_DIPSETTING(    0x20, DEF_STR( Single ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Dual ) )
	PORT_SERVICE_DIPLOC( 0x40, IP_ACTIVE_LOW, "SW3:3" )
	PORT_DIPUNUSED_DIPLOC( 0x80, IP_ACTIVE_LOW, "SW3:4" )
INPUT_PORTS_END



/***************************************************************************

    Machine Driver

***************************************************************************/

void parodius_state::machine_start()
{
	membank("bank1")->configure_entries(0, 16, memregion("maincpu")->base(), 0x4000);
	membank("bank1")->set_entry(0);

	save_item(NAME(m_sprite_colorbase));
	save_item(NAME(m_layer_colorbase));
	save_item(NAME(m_layerpri));
}

void parodius_state::machine_reset()
{
	for (int i = 0; i < 3; i++)
	{
		m_layerpri[i] = 0;
		m_layer_colorbase[i] = 0;
	}

	m_sprite_colorbase = 0;
	m_bank0000->set_bank(0);
	m_bank2000->set_bank(0);
}

WRITE8_MEMBER( parodius_state::banking_callback )
{
	if (data & 0xf0)
		logerror("%s: setlines %02x\n", machine().describe_context(), data);

	membank("bank1")->set_entry((data & 0x0f) ^ 0x0f);
}

MACHINE_CONFIG_START(parodius_state::parodius)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", KONAMI, 3000000)        /* 053248 */
	MCFG_DEVICE_PROGRAM_MAP(parodius_map)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", parodius_state,  parodius_interrupt)
	MCFG_KONAMICPU_LINE_CB(WRITE8(*this, parodius_state, banking_callback))

	MCFG_DEVICE_ADD("audiocpu", Z80, 3579545)
	MCFG_DEVICE_PROGRAM_MAP(parodius_sound_map)    /* NMIs are triggered by the 053260 */

	MCFG_DEVICE_ADD("bank0000", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(bank0000_map)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_BIG)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(8)
	MCFG_ADDRESS_MAP_BANK_ADDR_WIDTH(13)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x0800)

	MCFG_DEVICE_ADD("bank2000", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(bank2000_map)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_BIG)
	MCFG_ADDRESS_MAP_BANK_DATA_WIDTH(8)
	MCFG_ADDRESS_MAP_BANK_ADDR_WIDTH(12)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x0800)

	MCFG_WATCHDOG_ADD("watchdog")

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(64*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MCFG_SCREEN_UPDATE_DRIVER(parodius_state, screen_update_parodius)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD("palette", 2048)
	MCFG_PALETTE_ENABLE_SHADOWS()
	MCFG_PALETTE_FORMAT(xBBBBBGGGGGRRRRR)

	MCFG_DEVICE_ADD("k052109", K052109, 0)
	MCFG_GFX_PALETTE("palette")
	MCFG_K052109_CB(parodius_state, tile_callback)

	MCFG_DEVICE_ADD("k053245", K053245, 0)
	MCFG_GFX_PALETTE("palette")
	MCFG_K05324X_OFFSETS(0, 0)
	MCFG_K05324X_CB(parodius_state, sprite_callback)

	MCFG_K053251_ADD("k053251")

	/* sound hardware */
	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	MCFG_DEVICE_ADD("ymsnd", YM2151, 3579545)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	MCFG_K053260_ADD("k053260", 3579545)
	MCFG_SOUND_ROUTE(0, "lspeaker", 0.70)
	MCFG_SOUND_ROUTE(1, "rspeaker", 0.70)
MACHINE_CONFIG_END

/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( parodius )
	ROM_REGION( 0x40000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "955l01.f5", 0x00000, 0x20000, CRC(49a658eb) SHA1(dd53060c4da99b8e1f896ebfec572296ef2b5665) )
	ROM_LOAD( "955l02.h5", 0x20000, 0x20000, CRC(161d7322) SHA1(a752f28c19c58263680221ad1119f2fd57df4723) )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "955e03.d14", 0x0000, 0x10000, CRC(940aa356) SHA1(e7466f049be48861fd2d929eed786bd48782b5bb) )

	ROM_REGION( 0x100000, "k052109", 0 )    /* tiles */
	ROM_LOAD32_WORD( "955d07.k19", 0x000000, 0x080000, CRC(89473fec) SHA1(0da18c4b078c3a30233a6f5c2b90032168136f58) )
	ROM_LOAD32_WORD( "955d08.k24", 0x000002, 0x080000, CRC(43d5cda1) SHA1(2c51bad4857d1d31456c6dc1e7d41326ea35468b) )

	ROM_REGION( 0x100000, "k053245", 0 ) /* graphics */
	ROM_LOAD32_WORD( "955d05.k13", 0x000000, 0x080000, CRC(7a1e55e0) SHA1(7a0e04ebde28d1e7b60aef3de926dc0e78662b1e) ) /* sprites */
	ROM_LOAD32_WORD( "955d06.k8",  0x000002, 0x080000, CRC(f4252875) SHA1(490f2e19b30cf8724e4b03b8d9f089c470ec13bd) ) /* sprites */

	ROM_REGION( 0x80000, "k053260", 0 ) /* 053260 samples */
	ROM_LOAD( "955d04.c5", 0x00000, 0x80000, CRC(e671491a) SHA1(79e71cb5212eb7d14d3479b0734ea0270473a66d) )
ROM_END

ROM_START( parodiuse ) /* Earlier version? */
	ROM_REGION( 0x40000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "2.f5", 0x00000, 0x20000, CRC(26a6410b) SHA1(06de782f593ab0da6d65376b66e273d6410c6c56) )
	ROM_LOAD( "3.h5", 0x20000, 0x20000, CRC(9410dbf2) SHA1(1c4d9317f83c33bace929a841ff4093d7178c428) )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "955e03.d14", 0x0000, 0x10000, CRC(940aa356) SHA1(e7466f049be48861fd2d929eed786bd48782b5bb) )

	ROM_REGION( 0x100000, "k052109", 0 )    /* tiles */
	ROM_LOAD32_WORD( "955d07.k19", 0x000000, 0x080000, CRC(89473fec) SHA1(0da18c4b078c3a30233a6f5c2b90032168136f58) )
	ROM_LOAD32_WORD( "955d08.k24", 0x000002, 0x080000, CRC(43d5cda1) SHA1(2c51bad4857d1d31456c6dc1e7d41326ea35468b) )

	ROM_REGION( 0x100000, "k053245", 0 ) /* graphics */
	ROM_LOAD32_WORD( "955d05.k13", 0x000000, 0x080000, CRC(7a1e55e0) SHA1(7a0e04ebde28d1e7b60aef3de926dc0e78662b1e) ) /* sprites */
	ROM_LOAD32_WORD( "955d06.k8",  0x000002, 0x080000, CRC(f4252875) SHA1(490f2e19b30cf8724e4b03b8d9f089c470ec13bd) ) /* sprites */

	ROM_REGION( 0x80000, "k053260", 0 ) /* 053260 samples */
	ROM_LOAD( "955d04.c5", 0x00000, 0x80000, CRC(e671491a) SHA1(79e71cb5212eb7d14d3479b0734ea0270473a66d) )
ROM_END

ROM_START( parodiusj )
	ROM_REGION( 0x40000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "955e01.f5", 0x00000, 0x20000, CRC(49baa334) SHA1(8902fbb2228111b15de6537bd168241933df134d) )
	ROM_LOAD( "955e02.h5", 0x20000, 0x20000, CRC(14010d6f) SHA1(69fe162ea08c3bd4b3e78e9d10d278bd15444af4) )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "955e03.d14", 0x0000, 0x10000, CRC(940aa356) SHA1(e7466f049be48861fd2d929eed786bd48782b5bb) )

	ROM_REGION( 0x100000, "k052109", 0 )    /* tiles */
	ROM_LOAD32_WORD( "955d07.k19", 0x000000, 0x080000, CRC(89473fec) SHA1(0da18c4b078c3a30233a6f5c2b90032168136f58) )
	ROM_LOAD32_WORD( "955d08.k24", 0x000002, 0x080000, CRC(43d5cda1) SHA1(2c51bad4857d1d31456c6dc1e7d41326ea35468b) )

	ROM_REGION( 0x100000, "k053245", 0 ) /* graphics */
	ROM_LOAD32_WORD( "955d05.k13", 0x000000, 0x080000, CRC(7a1e55e0) SHA1(7a0e04ebde28d1e7b60aef3de926dc0e78662b1e) ) /* sprites */
	ROM_LOAD32_WORD( "955d06.k8",  0x000002, 0x080000, CRC(f4252875) SHA1(490f2e19b30cf8724e4b03b8d9f089c470ec13bd) ) /* sprites */

	ROM_REGION( 0x80000, "k053260", 0 ) /* 053260 samples */
	ROM_LOAD( "955d04.c5", 0x00000, 0x80000, CRC(e671491a) SHA1(79e71cb5212eb7d14d3479b0734ea0270473a66d) )
ROM_END

ROM_START( parodiusa )
	ROM_REGION( 0x40000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "b-18.f5", 0x00000, 0x20000, CRC(006356cd) SHA1(795011233059472c841c30831442a71579dff2b9) )
	ROM_LOAD( "b-19.h5", 0x20000, 0x20000, CRC(e5a16417) SHA1(a49567817fd4948e33913fab66106b8e16100b6a) )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "955e03.d14", 0x0000, 0x10000, CRC(940aa356) SHA1(e7466f049be48861fd2d929eed786bd48782b5bb) ) /* Labeled as D-20 */

	ROM_REGION( 0x100000, "k052109", 0 )    /* tiles */
	ROM_LOAD32_WORD( "955d07.k19", 0x000000, 0x080000, CRC(89473fec) SHA1(0da18c4b078c3a30233a6f5c2b90032168136f58) )
	ROM_LOAD32_WORD( "955d08.k24", 0x000002, 0x080000, CRC(43d5cda1) SHA1(2c51bad4857d1d31456c6dc1e7d41326ea35468b) )

	ROM_REGION( 0x100000, "k053245", 0 ) /* graphics */
	ROM_LOAD32_WORD( "955d05.k13", 0x000000, 0x080000, CRC(7a1e55e0) SHA1(7a0e04ebde28d1e7b60aef3de926dc0e78662b1e) ) /* sprites */
	ROM_LOAD32_WORD( "955d06.k8",  0x000002, 0x080000, CRC(f4252875) SHA1(490f2e19b30cf8724e4b03b8d9f089c470ec13bd) ) /* sprites */

	ROM_REGION( 0x80000, "k053260", 0 ) /* 053260 samples */
	ROM_LOAD( "955d04.c5", 0x00000, 0x80000, CRC(e671491a) SHA1(79e71cb5212eb7d14d3479b0734ea0270473a66d) )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

GAME( 1990, parodius,  0,        parodius, parodius, parodius_state, empty_init, ROT0, "Konami", "Parodius DA! (World, set 1)", MACHINE_SUPPORTS_SAVE )
GAME( 1990, parodiuse, parodius, parodius, parodius, parodius_state, empty_init, ROT0, "Konami", "Parodius DA! (World, set 2)", MACHINE_SUPPORTS_SAVE )
GAME( 1990, parodiusj, parodius, parodius, parodius, parodius_state, empty_init, ROT0, "Konami", "Parodius DA! (Japan)",        MACHINE_SUPPORTS_SAVE )
GAME( 1990, parodiusa, parodius, parodius, parodius, parodius_state, empty_init, ROT0, "Konami", "Parodius DA! (Asia)",         MACHINE_SUPPORTS_SAVE )
