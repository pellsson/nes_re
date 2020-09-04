//
// Pelle Lindblad
// pellelindblad <at> gmail <dot> com
//
// License: Use as you see fit <3
//
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

typedef enum hydlide_error
{
	HYDLIDE_ERROR_NONE,
	HYDLIDE_ERROR_PWD_BAD_LENGTH,
	HYDLIDE_ERROR_PWD_BAD_CHARACTER,
	HYDLIDE_ERROR_PWD_BAD_CHECKSUM,
	HYDLIDE_ERROR_PWD_BAD_PLAYER_COORDS,
	HYDLIDE_ERROR_PWD_BAD_MAP,
	HYDLIDE_ERROR_PWD_BAD_LEVEL,
	HYDLIDE_ERROR_PWD_BAD_TOO_MUCH_HP,
	HYDLIDE_ERROR_PWD_BAD_TOO_MUCH_MP,
	HYDLIDE_ERROR_PWD_INVALID_ITEM_STATE,
}
hydlide_error_t;

#define HYDLIDE_OK	HYDLIDE_ERROR_NONE

#define HYDLIDE_PASSWORD_LENGTH 16

#define HYDLIDE_FAIRY_GREEN	0x00
#define HYDLIDE_FAIRY_WHITE	0x01
#define HYDLIDE_FAIRY_RED	0x02
#define HYDLIDE_NUM_FAIRIES	0x03

#define HYDLIDE_ITEM_SWORD		0x00
#define HYDLIDE_ITEM_SHIELD		0x01
#define HYDLIDE_ITEM_LAMP		0x02
#define HYDLIDE_ITEM_CROSS		0x03
#define HYDLIDE_ITEM_MEDICINE	0x04
#define HYDLIDE_ITEM_MAGIC_POT	0x05
#define HYDLIDE_ITEM_KEY		0x06
#define HYDLIDE_ITEM_JEWEL		0x07
#define HYDLIDE_ITEM_RING		0x08
#define HYDLIDE_ITEM_RUBY		0x09
#define HYDLIDE_NUM_ITEMS		0x0A

typedef struct hydlide_state
{
	//
	// Player position x & y [0x36, 0x37]
	//
	uint8_t x;
	uint8_t y;
	//
	// Current hp [0x38]
	//
	uint8_t hp_current;
	//
	// Strength [0x39]
	//
	uint8_t strength;
	//
	// Current mp [0x3B]
	//
	uint8_t mp_current;
	//
	// Character level [0x3C]
	//
	uint8_t level;
	//
	// Max hp & mp [0x41, 0x42]
	//
	uint8_t hp_max;
	uint8_t mp_max;	
	//
	// Map square [0x47, 0x48, 0x49]
	//
	uint8_t map_absolute;
	uint8_t map_x;
	uint8_t map_y;
	//
	// ?? set to !sword, !cross, !magic-pot, !jewel [0x4F, 0x52, 0x54, 0x56]
	//
	uint8_t unknown_4F;
	uint8_t unknown_52;
	uint8_t unknown_54;
	uint8_t unknown_56;
	//
	// ?? Set if !(has-ruby) && unknown73 [0x58]
	//
	uint8_t unknown_58;
	//
	// Inventory items [0x59 -> 0x62]
	//
	uint8_t iventory[10];
	//
	// ?? [0x63]
	//
	uint8_t unknown_63;
	//
	// Fairies in collected [0x64, 0x65, 0x66]
	//
	uint8_t fairy[3];
	//
	// ?? has all fairies? [0x67]
	//
	uint8_t unknown_67;
	//
	// ?? All set to the same as has key (chests individually locked?) [0x68->0x6F]
	//
	uint8_t unknown_68_6F[8];
	//
	// ?? [0x73, 0x74]
	//
	uint8_t unknown_73;
	uint8_t unknown_74;
	//
	// ?? same as lamp? some is_lit? [0x75]
	//
	uint8_t unknown_75;
}
hydlide_state_t;

typedef struct hydlide_password
{
	uint8_t bytes[HYDLIDE_PASSWORD_LENGTH];
}
hydlide_password_t;

static const uint8_t encode_table[] = 
{
	0x04, 0x00, 0x00, 0x09, 0x02, 0x01, 0x05, 0x06, 0x09, 0x02, 0x09, 0x07, 0x05, 0x07, 0x07, 0x06,
	0x00, 0x05, 0x02, 0x07, 0x07, 0x03, 0x01, 0x08, 0x01, 0x06, 0x00, 0x05, 0x08, 0x07, 0x03, 0x02,
	0x01, 0x06, 0x07, 0x02
};

static const uint8_t character_lookup[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0x0A, 0xFF, 0x0B, 0xFF, 0xFF,
	0x0C, 0x0D, 0xFF, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0xFF, 0x13, 0x14, 0x15, 0xFF, 0x16, 0xFF, 0x17,
	0x18, 0x19, 0x1A, 0x1B
};

static uint8_t get_max(uint8_t a, uint8_t b)
{
	return a < b ? b : a;
}

static uint8_t lsr_byte(uint8_t *v)
{
	const uint8_t carry = *v & 0x01;
	*v = (*v >> 1);

	return carry;
}

static uint8_t rol_byte(uint8_t *v, uint8_t c)
{
	const uint8_t carry = (*v & 0x80) >> 7;
	*v = (*v << 1) | c;

	return carry;
}

static hydlide_error_t init_password(hydlide_password_t *hp, const char *str)
{
	size_t i;

	if(HYDLIDE_PASSWORD_LENGTH != strlen(str))
	{
		return HYDLIDE_ERROR_PWD_BAD_LENGTH;
	}

	for(i = 0; i < HYDLIDE_PASSWORD_LENGTH; ++i)
	{
		char c = str[i];

		if(c >= '0' && c <= '9')
		{
			hp->bytes[i] = (c - '0');
		}
		else if(c >= 'A' && c <= 'Z')
		{
			const uint8_t index = 0x0A + c - 'A';
			const uint8_t val	= character_lookup[index];

			if(0xFF == val)
			{
				return HYDLIDE_ERROR_PWD_BAD_CHARACTER;
			}

			hp->bytes[i] = val;
		}
		else
		{
			return HYDLIDE_ERROR_PWD_BAD_CHARACTER;
		}
	}

	return HYDLIDE_OK;
}

static hydlide_error_t prepare_password(hydlide_password_t *hp)
{
	size_t i;

	uint8_t checksum;
	uint8_t table_index;

	const uint8_t last_byte = hp->bytes[HYDLIDE_PASSWORD_LENGTH - 1];

	for(i = 0; i < (HYDLIDE_PASSWORD_LENGTH - 1); i += 2)
	{
		hp->bytes[i] -= last_byte;

		if(0x80 & hp->bytes[i])
		{
			hp->bytes[i] += 0x1C;
		}
	}

	table_index = hp->bytes[HYDLIDE_PASSWORD_LENGTH - 2];

	for(i = 0; i < (HYDLIDE_PASSWORD_LENGTH - 2); ++i, ++table_index)
	{
		hp->bytes[i] -= encode_table[table_index];

		if(0x80 & hp->bytes[i])
		{
			hp->bytes[i] += 0x1C;
		}

		if(hp->bytes[i] >= 0x10)
		{
			return HYDLIDE_ERROR_PWD_BAD_CHARACTER;
		}
	}

	if(hp->bytes[11] & 0x04)
	{
		hp->bytes[12] = 0;
	}

	if(hp->bytes[11] & 0x08)
	{
		hp->bytes[10] ^= 0x0F;
	}

	hp->bytes[11] &= 0x03;

	checksum = 0;

	for(i = 0; i < (HYDLIDE_PASSWORD_LENGTH - 3); ++i)
	{
		checksum += hp->bytes[i];
	}

	if(hp->bytes[HYDLIDE_PASSWORD_LENGTH - 3] != (checksum & 0x0F))
	{
		return HYDLIDE_ERROR_PWD_BAD_CHECKSUM;
	}

	return HYDLIDE_OK;
}

static hydlide_error_t read_state(hydlide_state_t *state, hydlide_password_t *hp)
{
	size_t i;
	uint8_t temp;

	memset(state, 0, sizeof(*state));

	for(i = 0; i < 3; ++i)
	{
		state->fairy[i] = lsr_byte(&hp->bytes[12]) ? 0xFF : 0x00;
	}

	state->unknown_63 = lsr_byte(&hp->bytes[12]) ? 0xFF : 0x00;
	
	rol_byte(&state->hp_current, lsr_byte(&hp->bytes[11]));
	rol_byte(&state->mp_current, lsr_byte(&hp->bytes[11]));
	rol_byte(&state->hp_current, lsr_byte(&hp->bytes[10]));
	rol_byte(&state->mp_current, lsr_byte(&hp->bytes[10]));

	state->unknown_73 = lsr_byte(&hp->bytes[10]) ? 0xFF : 0x00;
	state->unknown_74 = lsr_byte(&hp->bytes[10]) ? 0xFF : 0x00;

	for(i = 0; i < 5; ++i)
	{
		rol_byte(&state->hp_current, lsr_byte(&hp->bytes[9 - i]));
		rol_byte(&state->mp_current, lsr_byte(&hp->bytes[9 - i]));
		rol_byte(&state->map_absolute, lsr_byte(&hp->bytes[9 - i]));

		state->iventory[HYDLIDE_ITEM_RUBY - i] = lsr_byte(&hp->bytes[9 - i]) ? 0xFF : 0x00;
	}

	rol_byte(&state->x, lsr_byte(&hp->bytes[4]));
	rol_byte(&state->y, lsr_byte(&hp->bytes[4]));
	rol_byte(&state->map_absolute, lsr_byte(&hp->bytes[4]));

	state->iventory[HYDLIDE_ITEM_MEDICINE] = lsr_byte(&hp->bytes[4]) ? 0xFF : 0x00;

	for(i = 0; i < 4; ++i)
	{
		rol_byte(&state->x, lsr_byte(&hp->bytes[3 - i]));
		rol_byte(&state->y, lsr_byte(&hp->bytes[3 - i]));
		rol_byte(&state->level, lsr_byte(&hp->bytes[3 - i]));

		state->iventory[HYDLIDE_ITEM_CROSS - i] = lsr_byte(&hp->bytes[3 - i]) ? 0xFF : 0x00;
	}

	state->unknown_67 = (state->fairy[0] & state->fairy[1] & state->fairy[2]);
	
	temp = (state->level << 0x01);
	temp = (temp + (temp << 0x02)) + 0x0A;

	state->hp_max = temp;
	state->mp_max = temp;

	if(0 != state->iventory[HYDLIDE_ITEM_SWORD])
	{
		temp += 0x0A;
	}

	if(temp >= 0x64)
	{
		temp = 0x64;
	}

	state->strength = temp;

	state->unknown_4F = state->iventory[HYDLIDE_ITEM_SWORD] ^ 0xFF;
	state->unknown_52 = state->iventory[HYDLIDE_ITEM_CROSS] ^ 0xFF;
	state->unknown_54 = state->iventory[HYDLIDE_ITEM_MAGIC_POT] ^ 0xFF;
	state->unknown_56 = state->iventory[HYDLIDE_ITEM_JEWEL] ^ 0xFF;
	state->unknown_58 = (state->iventory[HYDLIDE_ITEM_RUBY] ^ 0xFF) & state->unknown_73;

	for(i = 0; i != 0x08; ++i)
	{
		state->unknown_68_6F[i] = state->iventory[HYDLIDE_ITEM_KEY];
	}

	state->unknown_75 = state->iventory[HYDLIDE_ITEM_LAMP];

	temp = state->map_absolute;

	for(i = 0; i < 0x100; ++i) // cant ever happen right? :)
	{
		if(temp < 5)
		{
			break;
		}
 
		temp -= 0x05;
	}
	
	state->map_x = temp;
	state->map_y = (uint8_t)(i & 0xFF);

	return HYDLIDE_OK;
}

static hydlide_error_t validate_state(const hydlide_state_t *state)
{
	if((state->x >= 0x16) || (state->y >= 0x16))
	{
		return HYDLIDE_ERROR_PWD_BAD_PLAYER_COORDS;
	}

	if(state->map_absolute >= 0x23)
	{
		return HYDLIDE_ERROR_PWD_BAD_MAP;
	}

	if(state->level >= 0x0A)
	{
		return HYDLIDE_ERROR_PWD_BAD_LEVEL;
	}

	if(state->hp_max < state->hp_current || state->hp_max > 100)
	{
		return HYDLIDE_ERROR_PWD_BAD_TOO_MUCH_HP;
	}

	if(state->mp_max < state->mp_current || state->mp_max > 100)
	{
		return HYDLIDE_ERROR_PWD_BAD_TOO_MUCH_MP;
	}

	if(0 == state->iventory[HYDLIDE_ITEM_KEY])
	{
		if(state->iventory[HYDLIDE_ITEM_JEWEL] | state->iventory[HYDLIDE_ITEM_RING])
		{
			return HYDLIDE_ERROR_PWD_INVALID_ITEM_STATE;
		}
	}

	if(0 != state->iventory[HYDLIDE_ITEM_MEDICINE])
	{
		if(0 != state->unknown_63)
		{
			return HYDLIDE_ERROR_PWD_INVALID_ITEM_STATE;
		}
	}

	if(0 == state->unknown_67)
	{
		if(state->iventory[HYDLIDE_ITEM_MEDICINE] | state->unknown_63)
		{
			return HYDLIDE_ERROR_PWD_INVALID_ITEM_STATE;
		}
	}

	if(0 == state->iventory[HYDLIDE_ITEM_CROSS])
	{
		if(state->iventory[HYDLIDE_ITEM_LAMP])
		{
			return HYDLIDE_ERROR_PWD_INVALID_ITEM_STATE;
		}
	}

	if(0 == state->unknown_73)
	{
		if(state->iventory[HYDLIDE_ITEM_RUBY])
		{
			return HYDLIDE_ERROR_PWD_INVALID_ITEM_STATE;
		}
	}

	if(0 == state->unknown_67)
	{
		if(state->unknown_73)
		{
			return HYDLIDE_ERROR_PWD_INVALID_ITEM_STATE;
		}
	}

	if(0 == state->unknown_74)
	{
		if(state->unknown_73)
		{
			return HYDLIDE_ERROR_PWD_INVALID_ITEM_STATE;
		}
	}

	return HYDLIDE_OK;
}

hydlide_error_t hydlide_state_from_password(hydlide_state_t *state, const char *password)
{
	hydlide_error_t rc;
	hydlide_password_t hp;

	if(HYDLIDE_OK != (rc = init_password(&hp, password)))
	{
		return rc;
	}

	if(HYDLIDE_OK != (rc = prepare_password(&hp)))
	{
		return rc;
	}
	
	if(HYDLIDE_OK != (rc = read_state(state, &hp)))
	{
		return rc;
	}

	return validate_state(state);
}

static const char * get_item_name(uint8_t item_id)
{
	switch(item_id)
	{
		case HYDLIDE_ITEM_SWORD:		return "Sword";
		case HYDLIDE_ITEM_SHIELD:		return "Shield";
		case HYDLIDE_ITEM_LAMP:			return "Lamp";
		case HYDLIDE_ITEM_CROSS:		return "Cross";
		case HYDLIDE_ITEM_MEDICINE:		return "Medicine";
		case HYDLIDE_ITEM_MAGIC_POT:	return "Pot";
		case HYDLIDE_ITEM_KEY:			return "Key";
		case HYDLIDE_ITEM_JEWEL:		return "Jewel";
		case HYDLIDE_ITEM_RING:			return "Ring";
		case HYDLIDE_ITEM_RUBY:			return "Ruby";
	}

	return "INVALID_ITEM";
}

static const char * get_fairy_color(uint8_t fairy_id)
{
	switch(fairy_id)
	{
		case HYDLIDE_FAIRY_GREEN:	return "Green";
		case HYDLIDE_FAIRY_WHITE:	return "White";
		case HYDLIDE_FAIRY_RED:		return "Red";
	}

	return "INVALID_FAIRY";
}

static void dbg_dump_state(const hydlide_state_t *state)
{
	size_t i;

	printf("Player at : %d, %d\n", state->x, state->y);
	printf("Health    : %d / %d\n", state->hp_current, state->hp_max);
	printf("Magic     : %d / %d\n", state->mp_current, state->mp_max);
	printf("Strength  : %d\n", state->strength);
	printf("Level     : %d\n", state->level);
	printf("Map       : %d (%d, %d)\n", state->map_absolute, state->map_x, state->map_y);
	printf("Items     : ");

	for(i = 0; i < HYDLIDE_NUM_ITEMS; ++i)
	{
		if(0 != state->iventory[i])
		{
			printf("%s, ", get_item_name(i));
		}
	}
	
	printf("\n");
	printf("Fairies   : ");

	for(i = 0; i < HYDLIDE_NUM_FAIRIES; ++i)
	{
		if(0 != state->fairy[i])
		{
			printf("%s, ", get_fairy_color(i));
		}
	}

	printf("\n");
	printf("Unknowns  : 0x4F: %02X   0x52: %02X   0x54: %02X   0x56 : %02X   0x58 : %02X\n", state->unknown_4F, state->unknown_52, state->unknown_54, state->unknown_56, state->unknown_58);
	printf("Unknowns  : 0x63: %02X   0x67: %02X\n", state->unknown_63, state->unknown_67);
	printf("[68...6F] : ");

	for(i = 0; i < 8; ++i)
	{
		printf("%02X ", state->unknown_68_6F[i]);
	}

	printf("\n");
	printf("[73...75] : %02X %02X %02X\n", state->unknown_73, state->unknown_74, state->unknown_75);
}

static void write_state(const hydlide_state_t *state, hydlide_password_t *hp)
{
	size_t i;

	uint8_t level			= state->level;
	uint8_t x				= state->x;
	uint8_t y				= state->y;
	uint8_t hp_current		= state->hp_current;
	uint8_t mp_current		= state->mp_current;
	uint8_t map_absolute	= state->map_absolute;

	for(i = 0; i < 4; ++i)
	{
		hp->bytes[i] |= lsr_byte(&x) ? 0x01 : 0x00;
		hp->bytes[i] |= lsr_byte(&y) ? 0x02 : 0x00;
		hp->bytes[i] |= lsr_byte(&level) ? 0x04 : 0x00;

		hp->bytes[i] |= state->iventory[i] ? 0x08 : 0x00;
	}

	hp->bytes[4] |= lsr_byte(&x) ? 0x01 : 0x00;
	hp->bytes[4] |= lsr_byte(&y) ? 0x02 : 0x00;
	hp->bytes[4] |= lsr_byte(&map_absolute) ? 0x04 : 0x00;
	hp->bytes[4] |= state->iventory[HYDLIDE_ITEM_MEDICINE] ? 0x08 : 0x00;

	for(i = 0; i < 5; ++i)
	{
		hp->bytes[5 + i] |= lsr_byte(&hp_current) ? 0x01 : 0x00;
		hp->bytes[5 + i] |= lsr_byte(&mp_current) ? 0x02 : 0x00;
		hp->bytes[5 + i] |= lsr_byte(&map_absolute) ? 0x04 : 0x00;
		hp->bytes[5 + i] |= state->iventory[HYDLIDE_ITEM_MAGIC_POT + i] ? 0x08 : 0x00;
	}

	hp->bytes[10] |= lsr_byte(&hp_current) ? 0x01 : 0x00;
	hp->bytes[10] |= lsr_byte(&mp_current) ? 0x02 : 0x00;
	hp->bytes[10] |= state->unknown_73 ? 0x04 : 0x00;
	hp->bytes[10] |= state->unknown_74 ? 0x08 : 0x00;

	hp->bytes[11] |= lsr_byte(&hp_current) ? 0x01 : 0x00;
	hp->bytes[11] |= lsr_byte(&mp_current) ? 0x02 : 0x00;

	hp->bytes[12] |= state->fairy[0] ? 0x01 : 0x00;
	hp->bytes[12] |= state->fairy[1] ? 0x02 : 0x00;
	hp->bytes[12] |= state->fairy[2] ? 0x04 : 0x00;
	hp->bytes[12] |= state->unknown_63 ? 0x08 : 0x00;
}

static void update_password(hydlide_password_t *hp)
{
	size_t i;
	uint8_t last_byte;
	uint8_t table_index;
	uint8_t checksum = 0;

	for(i = 0; i < (HYDLIDE_PASSWORD_LENGTH - 3); ++i)
	{
		checksum += hp->bytes[i];
	}

	hp->bytes[HYDLIDE_PASSWORD_LENGTH - 3] = checksum & 0x0F;
	
	table_index = rand() % (sizeof(encode_table) - (HYDLIDE_PASSWORD_LENGTH - 2));
	hp->bytes[HYDLIDE_PASSWORD_LENGTH - 2] = table_index;

	for(i = 0; i < (HYDLIDE_PASSWORD_LENGTH - 2); ++i, ++table_index)
	{
		hp->bytes[i] += encode_table[table_index];

		if(hp->bytes[i] >= 0x1C)
		{
			hp->bytes[i] -= 0x1C;
		}
	}

	last_byte = rand() & 0x0F;

	hp->bytes[HYDLIDE_PASSWORD_LENGTH - 1] = last_byte;

	for(i = 0; i < (HYDLIDE_PASSWORD_LENGTH - 1); i += 2)
	{
		hp->bytes[i] += last_byte;

		if(hp->bytes[i] >= 0x1C)
		{
			hp->bytes[i] -= 0x1C;
		}
	}
}

static char reverse_character_lookup(const uint8_t b)
{
	size_t i;
	
	if(b <= 0x09)
	{
		return '0' + b;
	}

	for(i = 0x0A; i < sizeof(character_lookup); ++i)
	{
		if(character_lookup[i] == b)
		{
			return 'A' + (i - 0x0A);
		}
	}

	assert(NULL == "Ooops... this shouldn't be possible, we failed somewhere else in the write process");

	return 0; 
}

static void finalize_password(const hydlide_password_t *hp, char *str)
{
	size_t i;

	for(i = 0; i < HYDLIDE_PASSWORD_LENGTH; ++i)
	{
		str[i] = reverse_character_lookup(hp->bytes[i]);
	}

	return;
}


hydlide_error_t hydlide_generate_password(const hydlide_state_t *state, const char **out)
{
	hydlide_error_t rc;
	hydlide_password_t hp;

	char plain_text[HYDLIDE_PASSWORD_LENGTH + 1];

	if(HYDLIDE_OK != (rc = validate_state(state)))
	{
		return rc;
	}

	memset(&hp, 0, sizeof(hp));
	memset(plain_text, 0, sizeof(plain_text));

	write_state(state, &hp);
	update_password(&hp);
	finalize_password(&hp, plain_text);

	*out = strdup(plain_text);

	return HYDLIDE_OK;
}

int main()
{
#define TEST_KNOWN

#ifndef TEST_KNOWN

	char *password;
	hydlide_state_t state;
	memset(&state, 0, sizeof(state));

	state.fairy[0] = state.fairy[1] = state.fairy[2] = 0xFF;
	state.hp_current = state.mp_current = state.hp_max = state.mp_max = 100;
	memset(state.iventory, 0xFF, sizeof(state.iventory));
	state.level = 0x9;
	state.map_absolute = 0x22;
	state.y = state.x = 0x15;
	state.unknown_67 = 0xFF;
	state.unknown_73 = 0xFF;
	state.unknown_74 = 0xFF;

	dbg_dump_state(&state);

	if(HYDLIDE_OK != hydlide_generate_password(&state, &password))
	{
		printf("Invalid state...\n");
		return -1;
	}

	printf("Generated password : %s\n", password);
	free(password);

#else
	size_t i;

	const char * const test_pwd[] =
	{
		"H75LBMGDN8YPWB74", // Start	20	Cross, Pot
		"QHHL8LW59B3BLPL2", // Outside Sword Dungeon	20	Sword, Lamp, Key, etc.
		"NKLBGJWBK7QKD653", // Wasp's Forest	30	Fairy1, Jewel, etc.
		"TNXDGJ0LD309J3Q7", // Outside Shield Dungeon (Desert)	50	Shield, etc.
		"MVTJLLM8R5WM95G3", // Shore across Boralis's Castle	80	Ring etc.
		"KBYKQHWMWQY8R787", // Shore across Boralis's Castle	90	Everything
	};

	for(i = 0; i < (sizeof(test_pwd) / sizeof(test_pwd[0])); ++i)
	{
		char *regen;

		hydlide_state_t state;
		const char * const current = test_pwd[i];
		hydlide_error_t rc = hydlide_state_from_password(&state, current);

		if(HYDLIDE_OK != rc)
		{
			printf("Password \"%s\" is 0x%08X\n", current, rc);
			continue;
		}

		printf("Password \"%s\" is VALID! :D\n", current);
		dbg_dump_state(&state);

		if(HYDLIDE_OK == (rc = hydlide_generate_password(&state, &regen)))
		{
			printf("## Regenerated password was \"%s\"\n", regen);
			free(regen);
		}
		else
		{
			printf("## Password regeneration failed with 0x%08X ##\n", rc);
		}

		printf("----------------------\n");
	}
#endif
	return 0;
}


