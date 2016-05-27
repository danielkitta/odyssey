
// XXX: Legacy hack
using namespace std;

#include <config.h>
#include "PicDevice.h"
#include <ConfigFile.h>
#include <IO.h>
#include <Util.h>

#include <cstdio>
#include <stdexcept>


Pic1xf1xxxDevice::Pic1xf1xxxDevice(char *name)
	: PicDevice(name)
{
	/* Read in config bits */
	for(int i=0;; i++) {
		long value;
		char buf[16];

		snprintf(buf, sizeof buf, "configmask%d", i);
		if(!pic_config->get_integer(name, buf, &value))
			break;

		config_masks.push_back(value);
	}

	/* Create the memory map for this device. */
	this->memmap.push_back(IntPair(0, codesize));
	this->memmap.push_back(IntPair(CONFIG_MEM_START, 4));
	this->memmap.push_back(IntPair(CONFIG_MEM_START + CONFIG_WORD_OFFSET, config_masks.size()));
}

Pic1xf1xxxDevice::~Pic1xf1xxxDevice() {
}

void Pic1xf1xxxDevice::erase() {
	if(this->memtype != MEMTYPE_FLASH)
		throw runtime_error("Operation not supported by device");

	bulk_erase();
}


void Pic1xf1xxxDevice::program(DataBuffer& buf) {
	switch(this->memtype) {
	case MEMTYPE_FLASH:
		break;
	default:
		throw runtime_error("Unsupported memory type in device");
	}

	progress_total = this->codesize + 4 + config_masks.size() - 1;
	progress_count = 0;

	try {
		set_program_mode();
		write_program_memory(buf);

		/* Enter config memory space. */
		write_command(COMMAND_LOAD_CONFIG);
		io->shift_bits_out(0x7ffe, 16);
		io->usleep(1);

		write_id_memory(buf);

		/* Advance to address of first config word */
		for(int i=4; i < CONFIG_WORD_OFFSET; i++)
			write_command(COMMAND_INC_ADDRESS);

		write_config_memory(buf);

		pic_off();
	} catch(std::exception& e) {
		pic_off();
		throw;
	}
}


void Pic1xf1xxxDevice::read(DataBuffer& buf, bool verify) {
	this->progress_total = this->codesize + 4 + config_masks.size() - 1;
	this->progress_count = 0;

	try {
		set_program_mode();
		read_program_memory(buf, verify);

		/* Enter config memory space. */
		write_command(COMMAND_LOAD_CONFIG);
		io->shift_bits_out(0x7ffe, 16);
		io->usleep(1);

		read_id_memory(buf, verify);

		/* Advance to address of first config word */
		for(int i=4; i < CONFIG_WORD_OFFSET; i++)
			write_command(COMMAND_INC_ADDRESS);

		read_config_memory(buf, verify);

		this->pic_off();
	} catch(std::exception& e) {
		this->pic_off();
		throw;
	}
}


void Pic1xf1xxxDevice::bulk_erase() {
	try {
		this->set_program_mode();
		this->write_command(COMMAND_LOAD_CONFIG);
		this->io->shift_bits_out(0x7ffe, 16, 1);
		this->io->usleep(1);

		this->write_command(COMMAND_ERASE_PROG_MEM);
		this->io->usleep(this->erase_time);

		this->pic_off();
	} catch(const std::exception& e) {
		this->pic_off();
		throw;
	}
}


void Pic1xf1xxxDevice::write_program_memory(DataBuffer& buf) {
	unsigned int offset;

	try {
		for(offset=0; offset < this->codesize; offset++) {
			progress(offset);

			/* Skip but verify blank locations to save time */
			if(buf.isblank(offset)) {
				if(read_prog_data() != buf[offset])
					break;
			} else {
				if(!program_one_location(buf[offset], wordmask))
					break;
			}
			write_command(COMMAND_INC_ADDRESS);
			progress_count++;
		}
		if(offset < codesize)
			throw runtime_error("");
	} catch(const std::exception& e) {
		THROW_ERROR(runtime_error,
		  "Couldn't write program memory at address 0x%04x", offset);
	}
}


void Pic1xf1xxxDevice::read_program_memory(DataBuffer& buf, bool verify) {
	unsigned int offset;

	try {
		for(offset=0; offset < codesize; offset++) {
			progress(offset);
			const uint32_t data = read_prog_data();

			if(verify) {
				if(data != buf[offset])
					throw runtime_error("");
			} else {
				buf[offset] = data;
			}
			write_command(COMMAND_INC_ADDRESS);
			progress_count++;
		}
	} catch(std::exception& e) {
		THROW_ERROR(runtime_error, "%s at address 0x%04x",
		  verify ? "Verification failed" : "Couldn't read program memory",
		  offset);
	}
}


void Pic1xf1xxxDevice::write_id_memory(DataBuffer& buf) {
	unsigned int offset;

	try {
		for(offset=0; offset < 4; offset++) {
			progress(CONFIG_MEM_START + offset);

			if(!program_one_location(buf[CONFIG_MEM_START + offset], wordmask))
				throw runtime_error("");

			write_command(COMMAND_INC_ADDRESS);
			progress_count++;
		}
	} catch(const std::exception& e) {
		THROW_ERROR(runtime_error,
			"Couldn't write ID memory at address 0x%04x",
			CONFIG_MEM_START + offset);
	}
}


void Pic1xf1xxxDevice::read_id_memory(DataBuffer& buf, bool verify) {
	unsigned int offset;

	try {
		for(offset=0; offset < 4; offset++) {
			progress(CONFIG_MEM_START + offset);
			const uint32_t data = read_prog_data();

			if(verify) {
				if(data != buf[CONFIG_MEM_START + offset])
					throw runtime_error("");
			} else {
				buf[CONFIG_MEM_START + offset] = data;
			}
			write_command(COMMAND_INC_ADDRESS);
			progress_count++;
		}
	} catch(std::exception& e) {
		THROW_ERROR(runtime_error, "%s at address 0x%04x",
			verify ? "Verification failed" : "Couldn't read ID memory",
			CONFIG_MEM_START + offset);
	}
}


void Pic1xf1xxxDevice::write_config_memory(DataBuffer& buf) {
	const unsigned int base = CONFIG_MEM_START + CONFIG_WORD_OFFSET;
	unsigned int i;

	try {
		for(i = 0; i < config_masks.size(); i++) {
			progress(base + i);

			if(!program_one_location(buf[base + i], config_masks[i]))
				throw runtime_error("");

			write_command(COMMAND_INC_ADDRESS);
			progress_count++;
		}
	} catch(std::exception& e) {
		THROW_ERROR(runtime_error,
			"Couldn't write config word at address 0x%04x", base + i);
	}
}


void Pic1xf1xxxDevice::read_config_memory(DataBuffer& buf, bool verify) {
	const unsigned int base = CONFIG_MEM_START + CONFIG_WORD_OFFSET;
	unsigned int i;

	try {
		for(i = 0; i < config_masks.size(); i++) {
			progress(base + i);

			const uint32_t data = read_prog_data();
			const uint32_t mask = config_masks[i];

			if(verify) {
				if((data & mask) != (buf[base + i] & mask))
					throw runtime_error("");
			} else {
				buf[base + i] = data;
			}
			write_command(COMMAND_INC_ADDRESS);
			progress_count++;
		}
	} catch(std::exception& e) {
		THROW_ERROR(runtime_error, "%s at address 0x%04x",
			verify ? "Verification failed" : "Couldn't read config word",
			base + i);
	}
}


bool Pic1xf1xxxDevice::program_one_location(uint32_t data, uint32_t mask) {
	unsigned int count;

	/* Program up to the initial count */
	for(count=1; count <= this->program_count; count++) {
		if(this->program_cycle(data, mask))
			break;
	}
	if(count > this->program_count)
		return false;

	/* Program count*multiplier more times */
	count *= this->program_multiplier;
	while(count > 0) {
		if(!this->program_cycle(data, mask))
			return false;
		count--;
	}
	return true;
}


UIntPair Pic1xf1xxxDevice::read_deviceid() {
	/* Enter config memory space. */
	write_command(COMMAND_LOAD_CONFIG);
	io->shift_bits_out(0x7ffe, 16);	/* Dummy write of all 1's */
	io->usleep(1);

	/* Advance to the address of the device ID */
	for(int i=0; i < REVISION_ID_OFFSET; i++)
		write_command(COMMAND_INC_ADDRESS);

	const uint32_t revid = read_prog_data();
	write_command(COMMAND_INC_ADDRESS);
	const uint32_t devid = read_prog_data();

	return UIntPair(devid, revid);
}
