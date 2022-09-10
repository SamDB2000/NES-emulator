#include "Bus.h"

Bus::Bus()
{
	// Clear RAM contents upon construction, just in case
	// for (auto& i : cpuRam) i = 0x0;

	// Connect CPU to communication bus
	cpu.ConnectBus(this);
}

Bus::~Bus()
{
}


void Bus::cpuWrite(uint16_t addr, uint8_t data) 
{
	// The cartridge gets priority over what happens on the bus
	// But for now this should be irrelevant and possibly even a bug source
	if (cart->cpuWrite(addr, data)) {

	}
	// Check for correct address locations
	else if (addr >= 0x0000 && addr <= 0x1FFF) {
		// Uses mirroring with 2kb
		cpuRam[addr & 0x07FF] = data;
	}
	else if (addr >= 2000 && addr <= 0x3FFF) {
		// Uses mirroring and writes directly to the PPU
		ppu.cpuWrite(addr & 0x0007, data);
	}
}


uint8_t Bus::cpuRead(uint16_t addr, bool bReadOnly) 
{
	uint8_t data = 0x00;

	// Comment cartridge override for cpu testing

	// if the cartridge is trying to interfere, nothing happens
	if (cart->cpuRead(addr, data)) {
		// cartridge address space
	}
	// check for correct address locations (8kb range for ram)
	else if (addr >= 0x0000 && addr <= 0x1FFF) {
		// Mirrored every 2048 bytes
		data = cpuRam[addr & 0x07FF];
	}
	else if (addr >= 2000 && addr <= 0x3FFF) {
		data = ppu.cpuRead(addr & 0x0007, bReadOnly);
	}
	return data;
}

void Bus::insertCartridge(const std::shared_ptr<Cartridge>& cartridge) {
	this->cart = cartridge;
	ppu.ConnectCartridge(cartridge);
}

void Bus::reset() {
	cart->reset();
	cpu.reset();
	ppu.reset();
	nSystemClockCounter = 0;
}

void Bus::clock() {
	// The running frequency is controlled by whatever calls this function.
	// So here we "divide" the clock as necessary and call the
	// peripheral devices clock() function at the correct times

	// The fasted clock frequency the digial system cares about is
	// equivalent to the PPU clock. So the PPU is clocked
	// each times this function is called
	ppu.clock();

	// The cpu clock runs 3x slower than the ppu clock so we only call
	// it's clock function every 3 times this function is called.
	// We use nSystemClockCounter to keep track of this
	if (nSystemClockCounter % 3 == 0) {
		cpu.clock();
	}

	// The PPU is capable of emitting an interrupt to indicate the
	// vertical blanking period has been entered. If it has, we need
	// to send that irq to the CPU.
	if (ppu.nmi) {
		ppu.nmi = false;
		cpu.nmi();
	}

	nSystemClockCounter++;
}