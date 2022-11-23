#include <stdbool.h>

// for some reason arduino core decides it's a good idea to free bluetooth
// memory if this function returns false, which apparently is overriden in
// esp32-hal-bt.c but it doesn't work
bool btInUse() { return true; }
