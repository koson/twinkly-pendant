#if !defined TWINKLY_PATTERNS_H
#define      TWINKLY_PATTERNS_H

enum fade {
	THROB,
	FLASH,
	GLIMMER,
	HEARTBEAT,
	SPIN_FORWARD,
	SPIN_REVERSE,
	DOUBLE_SPIN_FORWARD,
	DOUBLE_SPIN_REVERSE,
	GLITTER
};

enum speed {
	LENTO = 0x80,
	LARGO = 0x40,
	ANDANTE = 0x20,
	ALLEGRO = 0x10,
	PRESTO = 0x08
};

typedef struct {
	uint8_t fade;
	uint8_t speed;
	uint8_t next_LED;
} Pattern;

#endif
