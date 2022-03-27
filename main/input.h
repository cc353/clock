#pragma once

typedef enum Key {
	Up, Down, Left, Right, Enter
} Key;

typedef void (*InputCallback)(Key key, bool press);

void input_init(InputCallback callback);